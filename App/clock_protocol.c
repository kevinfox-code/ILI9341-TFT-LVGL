#include "clock_protocol.h"

#include <stdio.h>
#include <string.h>

/* ---- helpers ----------------------------------------------------------- */

static bool parse_fixed_uint(const char *s, size_t digits, uint32_t *out)
{
    uint32_t v = 0;
    for (size_t i = 0; i < digits; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return false;
        }
        v = v * 10u + (uint32_t)(s[i] - '0');
    }
    *out = v;
    return true;
}

bool clock_is_leap_year(uint16_t year)
{
    return ((year % 4u) == 0u && (year % 100u) != 0u) || ((year % 400u) == 0u);
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t k_days[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month < 1u || month > 12u) {
        return 0;
    }
    if (month == 2u && clock_is_leap_year(year)) {
        return 29;
    }
    return k_days[month - 1u];
}

bool clock_datetime_valid(const clock_datetime_t *dt)
{
    if (dt == NULL) {
        return false;
    }
    if (dt->year < 2000u || dt->year > 2099u) {
        return false;
    }
    if (dt->month < 1u || dt->month > 12u) {
        return false;
    }
    if (dt->day < 1u || dt->day > days_in_month(dt->year, dt->month)) {
        return false;
    }
    if (dt->hours > 23u || dt->minutes > 59u || dt->seconds > 59u) {
        return false;
    }
    return true;
}

uint8_t clock_weekday(uint16_t year, uint8_t month, uint8_t day)
{
    /* Sakamoto's algorithm: 0=Sunday..6=Saturday. */
    static const uint8_t k_t[12] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
    uint32_t y = year;
    if (month < 3u) {
        y -= 1u;
    }
    uint32_t dow = (y + y / 4u - y / 100u + y / 400u + k_t[month - 1u] + day) % 7u;
    /* Map to ISO 1=Mon..7=Sun. */
    return (dow == 0u) ? 7u : (uint8_t)dow;
}

/* ---- line accumulator --------------------------------------------------- */

void clock_line_acc_init(clock_line_acc_t *acc)
{
    acc->len = 0;
    acc->overflow = false;
}

/* Parses "YYYY-MM-DDTHH:MM:SS" (exactly 19 chars). */
static clock_err_t parse_datetime(const char *s, size_t len, clock_datetime_t *dt)
{
    if (len != 19u) {
        return CLOCK_ERR_BADFMT;
    }
    if (s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':' || s[16] != ':') {
        return CLOCK_ERR_BADFMT;
    }
    uint32_t year, month, day, hours, minutes, seconds;
    if (!parse_fixed_uint(&s[0], 4, &year) ||
        !parse_fixed_uint(&s[5], 2, &month) ||
        !parse_fixed_uint(&s[8], 2, &day) ||
        !parse_fixed_uint(&s[11], 2, &hours) ||
        !parse_fixed_uint(&s[14], 2, &minutes) ||
        !parse_fixed_uint(&s[17], 2, &seconds)) {
        return CLOCK_ERR_BADFMT;
    }
    dt->year = (uint16_t)year;
    dt->month = (uint8_t)month;
    dt->day = (uint8_t)day;
    dt->hours = (uint8_t)hours;
    dt->minutes = (uint8_t)minutes;
    dt->seconds = (uint8_t)seconds;
    if (!clock_datetime_valid(dt)) {
        return CLOCK_ERR_BADVAL;
    }
    dt->weekday = clock_weekday(dt->year, dt->month, dt->day);
    return CLOCK_ERR_NONE;
}

static void parse_line(const char *line, size_t len, clock_cmd_t *cmd)
{
    memset(cmd, 0, sizeof(*cmd));

    if (len == 5u && memcmp(line, "TIME?", 5u) == 0) {
        cmd->type = CLOCK_CMD_GET;
        return;
    }
    if (len == 4u && memcmp(line, "PING", 4u) == 0) {
        cmd->type = CLOCK_CMD_PING;
        return;
    }
    if (len >= 4u && memcmp(line, "SET ", 4u) == 0) {
        clock_err_t err = parse_datetime(line + 4, len - 4u, &cmd->dt);
        if (err == CLOCK_ERR_NONE) {
            cmd->type = CLOCK_CMD_SET;
        } else {
            cmd->type = CLOCK_CMD_ERROR;
            cmd->err = err;
        }
        return;
    }
    cmd->type = CLOCK_CMD_ERROR;
    cmd->err = CLOCK_ERR_BADCMD;
}

bool clock_proto_feed(clock_line_acc_t *acc, char c, clock_cmd_t *cmd)
{
    if (c == '\r') {
        return false; /* tolerate CRLF terminators */
    }
    if (c != '\n') {
        if (acc->len < sizeof(acc->buf)) {
            acc->buf[acc->len++] = c;
        } else {
            acc->overflow = true; /* keep discarding until newline */
        }
        return false;
    }

    /* Newline: judge the accumulated line, then reset for the next one. */
    bool was_overflow = acc->overflow;
    size_t len = acc->len;
    acc->len = 0;
    acc->overflow = false;

    if (was_overflow) {
        memset(cmd, 0, sizeof(*cmd));
        cmd->type = CLOCK_CMD_ERROR;
        cmd->err = CLOCK_ERR_OVERFLOW;
        return true;
    }
    if (len == 0u) {
        return false; /* blank line — ignore */
    }
    parse_line(acc->buf, len, cmd);
    return true;
}

/* ---- formatters --------------------------------------------------------- */

static int format_stamp(char *out, size_t n, const char *prefix, const clock_datetime_t *dt)
{
    if (out == NULL || dt == NULL) {
        return 0;
    }
    int len = snprintf(out, n, "%s %04u-%02u-%02uT%02u:%02u:%02u\r\n",
                       prefix,
                       (unsigned)dt->year, (unsigned)dt->month, (unsigned)dt->day,
                       (unsigned)dt->hours, (unsigned)dt->minutes, (unsigned)dt->seconds);
    return (len > 0 && (size_t)len < n) ? len : 0;
}

int clock_format_time_resp(char *out, size_t n, const clock_datetime_t *dt)
{
    return format_stamp(out, n, "TIME", dt);
}

int clock_format_broadcast(char *out, size_t n, const clock_datetime_t *dt)
{
    return format_stamp(out, n, "RTC", dt);
}

int clock_format_set_ok(char *out, size_t n, const clock_datetime_t *dt)
{
    return format_stamp(out, n, "OK SET", dt);
}

int clock_format_err(char *out, size_t n, clock_err_t err)
{
    const char *tag;
    switch (err) {
    case CLOCK_ERR_BADCMD:   tag = "BADCMD";   break;
    case CLOCK_ERR_BADFMT:   tag = "BADFMT";   break;
    case CLOCK_ERR_BADVAL:   tag = "BADVAL";   break;
    case CLOCK_ERR_OVERFLOW: tag = "OVERFLOW"; break;
    default:                 return 0;
    }
    int len = snprintf(out, n, "ERR %s\r\n", tag);
    return (len > 0 && (size_t)len < n) ? len : 0;
}

int clock_format_pong(char *out, size_t n)
{
    int len = snprintf(out, n, "PONG %s\r\n", CLOCK_PROTO_IDENT);
    return (len > 0 && (size_t)len < n) ? len : 0;
}
