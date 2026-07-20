#include "unity.h"
#include "clock_protocol.h"

#include <string.h>

void setUp(void) {}
void tearDown(void) {}

/* ---- helpers ------------------------------------------------------------ */

/* Feeds a whole string; returns the number of completed commands and the
 * last one seen. */
static int feed_str(clock_line_acc_t *acc, const char *s, clock_cmd_t *last)
{
    int count = 0;
    for (const char *p = s; *p != '\0'; p++) {
        clock_cmd_t cmd;
        if (clock_proto_feed(acc, *p, &cmd)) {
            count++;
            *last = cmd;
        }
    }
    return count;
}

static clock_cmd_t feed_one(const char *line)
{
    clock_line_acc_t acc;
    clock_line_acc_init(&acc);
    clock_cmd_t cmd;
    memset(&cmd, 0, sizeof(cmd));
    TEST_ASSERT_EQUAL_INT(1, feed_str(&acc, line, &cmd));
    return cmd;
}

/* ---- leap year / validation --------------------------------------------- */

static void test_leap_year_rules(void)
{
    TEST_ASSERT_TRUE(clock_is_leap_year(2024));
    TEST_ASSERT_TRUE(clock_is_leap_year(2000));  /* /400 */
    TEST_ASSERT_FALSE(clock_is_leap_year(2100)); /* /100 but not /400 */
    TEST_ASSERT_FALSE(clock_is_leap_year(2025));
    TEST_ASSERT_FALSE(clock_is_leap_year(2026));
}

static void test_datetime_valid_ranges(void)
{
    clock_datetime_t dt = { 2026, 7, 19, 23, 59, 59, 0 };
    TEST_ASSERT_TRUE(clock_datetime_valid(&dt));

    dt.year = 1999; TEST_ASSERT_FALSE(clock_datetime_valid(&dt)); dt.year = 2026;
    dt.year = 2100; TEST_ASSERT_FALSE(clock_datetime_valid(&dt)); dt.year = 2026;
    dt.month = 0;   TEST_ASSERT_FALSE(clock_datetime_valid(&dt));
    dt.month = 13;  TEST_ASSERT_FALSE(clock_datetime_valid(&dt)); dt.month = 7;
    dt.day = 0;     TEST_ASSERT_FALSE(clock_datetime_valid(&dt));
    dt.day = 32;    TEST_ASSERT_FALSE(clock_datetime_valid(&dt)); dt.day = 19;
    dt.hours = 24;  TEST_ASSERT_FALSE(clock_datetime_valid(&dt)); dt.hours = 0;
    dt.minutes = 60; TEST_ASSERT_FALSE(clock_datetime_valid(&dt)); dt.minutes = 0;
    dt.seconds = 60; TEST_ASSERT_FALSE(clock_datetime_valid(&dt));

    TEST_ASSERT_FALSE(clock_datetime_valid(NULL));
}

static void test_datetime_valid_month_lengths(void)
{
    clock_datetime_t dt = { 2026, 4, 31, 0, 0, 0, 0 }; /* April has 30 */
    TEST_ASSERT_FALSE(clock_datetime_valid(&dt));
    dt.day = 30;
    TEST_ASSERT_TRUE(clock_datetime_valid(&dt));

    dt.month = 2; dt.day = 29; dt.year = 2025; /* not a leap year */
    TEST_ASSERT_FALSE(clock_datetime_valid(&dt));
    dt.year = 2024; /* leap year */
    TEST_ASSERT_TRUE(clock_datetime_valid(&dt));
    dt.day = 30;
    TEST_ASSERT_FALSE(clock_datetime_valid(&dt));
}

/* ---- weekday ------------------------------------------------------------ */

static void test_weekday_known_dates(void)
{
    TEST_ASSERT_EQUAL_UINT8(7, clock_weekday(2026, 7, 19));  /* Sunday    */
    TEST_ASSERT_EQUAL_UINT8(1, clock_weekday(2026, 7, 20));  /* Monday    */
    TEST_ASSERT_EQUAL_UINT8(6, clock_weekday(2000, 1, 1));   /* Saturday  */
    TEST_ASSERT_EQUAL_UINT8(4, clock_weekday(2024, 2, 29));  /* Thursday  */
    TEST_ASSERT_EQUAL_UINT8(4, clock_weekday(2099, 12, 31)); /* Thursday  */
    TEST_ASSERT_EQUAL_UINT8(3, clock_weekday(2025, 12, 31)); /* Wednesday */
}

/* ---- SET parsing --------------------------------------------------------- */

static void test_set_valid(void)
{
    clock_cmd_t cmd = feed_one("SET 2026-07-19T14:30:22\n");
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_SET, cmd.type);
    TEST_ASSERT_EQUAL_UINT16(2026, cmd.dt.year);
    TEST_ASSERT_EQUAL_UINT8(7, cmd.dt.month);
    TEST_ASSERT_EQUAL_UINT8(19, cmd.dt.day);
    TEST_ASSERT_EQUAL_UINT8(14, cmd.dt.hours);
    TEST_ASSERT_EQUAL_UINT8(30, cmd.dt.minutes);
    TEST_ASSERT_EQUAL_UINT8(22, cmd.dt.seconds);
    TEST_ASSERT_EQUAL_UINT8(7, cmd.dt.weekday); /* Sunday */
}

static void test_set_crlf_terminated(void)
{
    clock_cmd_t cmd = feed_one("SET 2024-02-29T00:00:00\r\n");
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_SET, cmd.type);
    TEST_ASSERT_EQUAL_UINT8(29, cmd.dt.day);
    TEST_ASSERT_EQUAL_UINT8(4, cmd.dt.weekday); /* Thursday */
}

static void test_set_bad_format(void)
{
    const char *bad[] = {
        "SET 2026-7-19T14:30:22\n",   /* single-digit month */
        "SET 2026-07-19 14:30:22\n",  /* space instead of T */
        "SET 2026/07/19T14:30:22\n",  /* wrong separators */
        "SET 2026-07-19T14:30\n",     /* missing seconds */
        "SET 2026-07-19T14:30:2x\n",  /* non-digit */
        "SET \n",                     /* empty arg */
        "SET 2026-07-19T14:30:223\n", /* trailing junk */
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        clock_cmd_t cmd = feed_one(bad[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(CLOCK_CMD_ERROR, cmd.type, bad[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(CLOCK_ERR_BADFMT, cmd.err, bad[i]);
    }
}

static void test_set_bad_values(void)
{
    const char *bad[] = {
        "SET 2026-02-30T00:00:00\n", /* Feb 30 */
        "SET 2025-02-29T00:00:00\n", /* Feb 29, non-leap */
        "SET 2026-13-01T00:00:00\n", /* month 13 */
        "SET 2026-00-01T00:00:00\n", /* month 0 */
        "SET 2026-07-00T00:00:00\n", /* day 0 */
        "SET 2026-07-19T24:00:00\n", /* hour 24 */
        "SET 2026-07-19T23:60:00\n", /* minute 60 */
        "SET 2026-07-19T23:00:60\n", /* second 60 */
        "SET 1999-12-31T23:59:59\n", /* below RTC range */
        "SET 2100-01-01T00:00:00\n", /* above RTC range */
    };
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        clock_cmd_t cmd = feed_one(bad[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(CLOCK_CMD_ERROR, cmd.type, bad[i]);
        TEST_ASSERT_EQUAL_INT_MESSAGE(CLOCK_ERR_BADVAL, cmd.err, bad[i]);
    }
}

/* ---- other verbs --------------------------------------------------------- */

static void test_time_query_and_ping(void)
{
    clock_cmd_t cmd = feed_one("TIME?\n");
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_GET, cmd.type);

    cmd = feed_one("PING\n");
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_PING, cmd.type);

    cmd = feed_one("BOGUS 123\n");
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_ERROR, cmd.type);
    TEST_ASSERT_EQUAL_INT(CLOCK_ERR_BADCMD, cmd.err);

    /* Verbs are case-sensitive by design. */
    cmd = feed_one("time?\n");
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_ERROR, cmd.type);
    TEST_ASSERT_EQUAL_INT(CLOCK_ERR_BADCMD, cmd.err);
}

/* ---- accumulator behavior ------------------------------------------------ */

static void test_partial_input_across_feeds(void)
{
    clock_line_acc_t acc;
    clock_line_acc_init(&acc);
    clock_cmd_t cmd;

    /* Bytes arrive in arbitrary chunks — nothing completes until the LF. */
    TEST_ASSERT_EQUAL_INT(0, feed_str(&acc, "SET 2026-0", &cmd));
    TEST_ASSERT_EQUAL_INT(0, feed_str(&acc, "7-19T14:30:22", &cmd));
    TEST_ASSERT_EQUAL_INT(1, feed_str(&acc, "\n", &cmd));
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_SET, cmd.type);
}

static void test_blank_lines_ignored(void)
{
    clock_line_acc_t acc;
    clock_line_acc_init(&acc);
    clock_cmd_t cmd;
    TEST_ASSERT_EQUAL_INT(0, feed_str(&acc, "\n\r\n\n", &cmd));
}

static void test_multiple_commands_in_one_stream(void)
{
    clock_line_acc_t acc;
    clock_line_acc_init(&acc);
    clock_cmd_t cmd;
    int n = feed_str(&acc, "PING\nTIME?\nSET 2026-01-01T00:00:00\n", &cmd);
    TEST_ASSERT_EQUAL_INT(3, n);
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_SET, cmd.type); /* last one */
    TEST_ASSERT_EQUAL_UINT8(4, cmd.dt.weekday);     /* 2026-01-01 is a Thursday */
}

static void test_overflow_line_rejected_then_recovers(void)
{
    clock_line_acc_t acc;
    clock_line_acc_init(&acc);
    clock_cmd_t cmd;

    for (int i = 0; i < 200; i++) {
        TEST_ASSERT_FALSE(clock_proto_feed(&acc, 'A', &cmd));
    }
    TEST_ASSERT_TRUE(clock_proto_feed(&acc, '\n', &cmd));
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_ERROR, cmd.type);
    TEST_ASSERT_EQUAL_INT(CLOCK_ERR_OVERFLOW, cmd.err);

    /* The garbage must not poison the next command. */
    TEST_ASSERT_EQUAL_INT(1, feed_str(&acc, "PING\n", &cmd));
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_PING, cmd.type);
}

/* ---- formatters ----------------------------------------------------------- */

static void test_formatters(void)
{
    clock_datetime_t dt = { 2026, 7, 19, 14, 30, 22, 7 };
    char buf[CLOCK_PROTO_RESP_MAX];

    TEST_ASSERT_GREATER_THAN_INT(0, clock_format_time_resp(buf, sizeof(buf), &dt));
    TEST_ASSERT_EQUAL_STRING("TIME 2026-07-19T14:30:22\r\n", buf);

    TEST_ASSERT_GREATER_THAN_INT(0, clock_format_broadcast(buf, sizeof(buf), &dt));
    TEST_ASSERT_EQUAL_STRING("RTC 2026-07-19T14:30:22\r\n", buf);

    TEST_ASSERT_GREATER_THAN_INT(0, clock_format_set_ok(buf, sizeof(buf), &dt));
    TEST_ASSERT_EQUAL_STRING("OK SET 2026-07-19T14:30:22\r\n", buf);

    TEST_ASSERT_GREATER_THAN_INT(0, clock_format_err(buf, sizeof(buf), CLOCK_ERR_BADVAL));
    TEST_ASSERT_EQUAL_STRING("ERR BADVAL\r\n", buf);

    TEST_ASSERT_GREATER_THAN_INT(0, clock_format_pong(buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("PONG " CLOCK_PROTO_IDENT "\r\n", buf);
}

static void test_formatters_reject_small_buffers(void)
{
    clock_datetime_t dt = { 2026, 7, 19, 14, 30, 22, 7 };
    char tiny[8];
    TEST_ASSERT_EQUAL_INT(0, clock_format_time_resp(tiny, sizeof(tiny), &dt));
    TEST_ASSERT_EQUAL_INT(0, clock_format_broadcast(tiny, sizeof(tiny), &dt));
    TEST_ASSERT_EQUAL_INT(0, clock_format_set_ok(tiny, sizeof(tiny), &dt));
    TEST_ASSERT_EQUAL_INT(0, clock_format_err(tiny, 3, CLOCK_ERR_BADVAL));
    TEST_ASSERT_EQUAL_INT(0, clock_format_time_resp(NULL, 99, &dt));
    TEST_ASSERT_EQUAL_INT(0, clock_format_time_resp(tiny, sizeof(tiny), NULL));
}

/* ---- round trip ----------------------------------------------------------- */

static void test_parse_format_round_trip(void)
{
    clock_cmd_t cmd = feed_one("SET 2031-12-31T23:59:59\n");
    TEST_ASSERT_EQUAL_INT(CLOCK_CMD_SET, cmd.type);
    char buf[CLOCK_PROTO_RESP_MAX];
    TEST_ASSERT_GREATER_THAN_INT(0, clock_format_set_ok(buf, sizeof(buf), &cmd.dt));
    TEST_ASSERT_EQUAL_STRING("OK SET 2031-12-31T23:59:59\r\n", buf);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_leap_year_rules);
    RUN_TEST(test_datetime_valid_ranges);
    RUN_TEST(test_datetime_valid_month_lengths);
    RUN_TEST(test_weekday_known_dates);
    RUN_TEST(test_set_valid);
    RUN_TEST(test_set_crlf_terminated);
    RUN_TEST(test_set_bad_format);
    RUN_TEST(test_set_bad_values);
    RUN_TEST(test_time_query_and_ping);
    RUN_TEST(test_partial_input_across_feeds);
    RUN_TEST(test_blank_lines_ignored);
    RUN_TEST(test_multiple_commands_in_one_stream);
    RUN_TEST(test_overflow_line_rejected_then_recovers);
    RUN_TEST(test_formatters);
    RUN_TEST(test_formatters_reject_small_buffers);
    RUN_TEST(test_parse_format_round_trip);
    UNITY_END();
    return 0;
}
