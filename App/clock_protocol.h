#ifndef CLOCK_PROTOCOL_H_
#define CLOCK_PROTOCOL_H_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*
 * clock_protocol — the UART time-sync wire protocol, as pure C.
 *
 * This module is deliberately HAL-free (no CubeMX/STM32 headers) so it can
 * be unit-tested on the host (see tests/host/). App/app_main.c feeds it raw
 * UART bytes and applies the results through System_* calls; the browser
 * dashboard (dashboard/rtc-dashboard.html) speaks the same protocol over
 * Web Serial.
 *
 * Wire format (ASCII lines, LF or CRLF terminated):
 *
 *   host -> device                      device -> host
 *   ------------------------------      --------------------------------
 *   SET 2026-07-19T14:30:22             OK SET 2026-07-19T14:30:22
 *                                       ERR BADFMT | ERR BADVAL | ...
 *   TIME?                               TIME 2026-07-19T14:30:22
 *   PING                                PONG rtc-clock 1.0
 *
 *   (unsolicited, 1 Hz)                 RTC 2026-07-19T14:30:22
 */

#define CLOCK_PROTO_LINE_MAX 48u /* longest accepted command line (excl. terminator) */
#define CLOCK_PROTO_RESP_MAX 48u /* fits every response incl. "\r\n\0" */

#define CLOCK_PROTO_IDENT "rtc-clock 1.0"

typedef struct {
    uint16_t year;    /* 2000..2099 (STM32 RTC stores a 2-digit year) */
    uint8_t  month;   /* 1..12 */
    uint8_t  day;     /* 1..31, validated against month/leap year */
    uint8_t  hours;   /* 0..23 */
    uint8_t  minutes; /* 0..59 */
    uint8_t  seconds; /* 0..59 */
    uint8_t  weekday; /* ISO 1=Mon .. 7=Sun (matches HAL RTC_WEEKDAY_*) */
} clock_datetime_t;

typedef enum {
    CLOCK_CMD_NONE = 0, /* no complete line yet (or blank line) */
    CLOCK_CMD_SET,      /* valid SET — dt holds the parsed value */
    CLOCK_CMD_GET,      /* TIME? */
    CLOCK_CMD_PING,     /* PING */
    CLOCK_CMD_ERROR,    /* complete line, but rejected — see err */
} clock_cmd_type_t;

typedef enum {
    CLOCK_ERR_NONE = 0,
    CLOCK_ERR_BADCMD,   /* unknown verb */
    CLOCK_ERR_BADFMT,   /* SET argument not YYYY-MM-DDTHH:MM:SS */
    CLOCK_ERR_BADVAL,   /* well-formed but impossible (Feb 30, 25:00, ...) */
    CLOCK_ERR_OVERFLOW, /* line exceeded CLOCK_PROTO_LINE_MAX */
} clock_err_t;

typedef struct {
    clock_cmd_type_t type;
    clock_err_t err;      /* meaningful when type == CLOCK_CMD_ERROR */
    clock_datetime_t dt;  /* meaningful when type == CLOCK_CMD_SET */
} clock_cmd_t;

/* Line accumulator: owns partial-line state between feed calls. */
typedef struct {
    char buf[CLOCK_PROTO_LINE_MAX];
    size_t len;
    bool overflow;
} clock_line_acc_t;

void clock_line_acc_init(clock_line_acc_t *acc);

/* Feed one received byte. Returns true when a full line has been parsed,
 * with the verdict in *cmd (never CLOCK_CMD_NONE when returning true —
 * blank lines return false). CR bytes are ignored, so CRLF just works. */
bool clock_proto_feed(clock_line_acc_t *acc, char c, clock_cmd_t *cmd);

/* Validation helpers (also used directly by tests). */
bool clock_is_leap_year(uint16_t year);
bool clock_datetime_valid(const clock_datetime_t *dt);

/* ISO weekday (1=Mon..7=Sun) for a Gregorian calendar date. */
uint8_t clock_weekday(uint16_t year, uint8_t month, uint8_t day);

/* Response formatters. Each writes a CRLF-terminated, NUL-terminated string
 * and returns its length (excluding the NUL), or 0 if the buffer is too
 * small / input invalid. */
int clock_format_time_resp(char *out, size_t n, const clock_datetime_t *dt);
int clock_format_broadcast(char *out, size_t n, const clock_datetime_t *dt);
int clock_format_set_ok(char *out, size_t n, const clock_datetime_t *dt);
int clock_format_err(char *out, size_t n, clock_err_t err);
int clock_format_pong(char *out, size_t n);

#endif /* CLOCK_PROTOCOL_H_ */
