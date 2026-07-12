#include "unity.h"
#include "drv/xpt2046.h"
#include "mock_spi.h"
#include "mock_gpio.h"
#include "mock_os.h"
#include "mock_seq.h"

#define XMIN 100u
#define XMAX 200u
#define YMIN 1000u
#define YMAX 2000u
#define HOR_RES 320u
#define VER_RES 240u

static drv_gpio_t cs_pin, irq_pin;
static int cs_id, irq_id;
static drv_spi_bus_t *bus;
static xpt2046_t *touch;

void setUp(void)
{
    mock_seq_reset();
    mock_spi_reset();
    mock_gpio_reset();
    mock_os_reset();

    cs_pin = (drv_gpio_t){ &cs_id, 1 };
    irq_pin = (drv_gpio_t){ &irq_id, 2 };
    bus = drv_spi_bus_create((void *)0x5000);

    xpt2046_config_t cfg = {
        .bus = bus, .cs = cs_pin, .irq = irq_pin,
        .raw_x_min = XMIN, .raw_x_max = XMAX, .raw_y_min = YMIN, .raw_y_max = YMAX,
    };
    touch = NULL;
    TEST_ASSERT_EQUAL(DRV_OK, XPT2046_Init(&touch, &cfg));

    mock_spi_reset_log();
    mock_seq_reset();
    mock_gpio_set_level(irq_pin, false); /* pressed for the whole file */
}

void tearDown(void) {}

static void queue_sample(uint16_t v)
{
    uint8_t g[3] = { 0, (uint8_t)(v >> 4), (uint8_t)((v << 4) & 0xFF) };
    mock_spi_queue_rx(g, 3);
}

/* Feeds a ReadPoint call an exact (raw_x, raw_y) pair (discard reads use the
 * same value, so averaging is a no-op). */
static bool read_point_at(uint16_t raw_x, uint16_t raw_y, uint16_t rotation, uint16_t *px, uint16_t *py)
{
    queue_sample(raw_x); queue_sample(raw_x); queue_sample(raw_x);
    queue_sample(raw_y); queue_sample(raw_y); queue_sample(raw_y);
    return XPT2046_ReadPoint(touch, px, py, HOR_RES, VER_RES, (uint8_t)rotation);
}

void test_corners_all_rotations(void)
{
    uint16_t px, py;

    /* min corner (raw_x=XMIN, raw_y=YMIN) */
    TEST_ASSERT_TRUE(read_point_at(XMIN, YMIN, 3, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(0, px); TEST_ASSERT_EQUAL_UINT16(0, py);

    TEST_ASSERT_TRUE(read_point_at(XMIN, YMIN, 0, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(HOR_RES - 1, px); TEST_ASSERT_EQUAL_UINT16(0, py);

    TEST_ASSERT_TRUE(read_point_at(XMIN, YMIN, 1, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(HOR_RES - 1, px); TEST_ASSERT_EQUAL_UINT16(VER_RES - 1, py);

    TEST_ASSERT_TRUE(read_point_at(XMIN, YMIN, 2, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(0, px); TEST_ASSERT_EQUAL_UINT16(VER_RES - 1, py);

    /* max corner (raw_x=XMAX, raw_y=YMAX) */
    TEST_ASSERT_TRUE(read_point_at(XMAX, YMAX, 3, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(HOR_RES - 1, px); TEST_ASSERT_EQUAL_UINT16(VER_RES - 1, py);

    TEST_ASSERT_TRUE(read_point_at(XMAX, YMAX, 0, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(0, px); TEST_ASSERT_EQUAL_UINT16(VER_RES - 1, py);

    TEST_ASSERT_TRUE(read_point_at(XMAX, YMAX, 1, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(0, px); TEST_ASSERT_EQUAL_UINT16(0, py);

    TEST_ASSERT_TRUE(read_point_at(XMAX, YMAX, 2, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(HOR_RES - 1, px); TEST_ASSERT_EQUAL_UINT16(0, py);
}

void test_clamping_below_min_and_above_max(void)
{
    uint16_t px, py;

    TEST_ASSERT_TRUE(read_point_at(XMIN - 50, YMIN - 50, 3, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(0, px);
    TEST_ASSERT_EQUAL_UINT16(0, py);

    TEST_ASSERT_TRUE(read_point_at(XMAX + 500, YMAX + 500, 3, &px, &py));
    TEST_ASSERT_EQUAL_UINT16(HOR_RES - 1, px);
    TEST_ASSERT_EQUAL_UINT16(VER_RES - 1, py);
}

void test_degenerate_calibration_rejected(void)
{
    uint16_t xmin, xmax, ymin, ymax;
    XPT2046_GetCalibration(touch, &xmin, &xmax, &ymin, &ymax);
    TEST_ASSERT_EQUAL_UINT16(XMIN, xmin);
    TEST_ASSERT_EQUAL_UINT16(XMAX, xmax);

    XPT2046_SetCalibration(touch, 150, 150, YMIN, YMAX); /* xmin == xmax: invalid */

    XPT2046_GetCalibration(touch, &xmin, &xmax, &ymin, &ymax);
    TEST_ASSERT_EQUAL_UINT16(XMIN, xmin);
    TEST_ASSERT_EQUAL_UINT16(XMAX, xmax);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_corners_all_rotations);
    RUN_TEST(test_clamping_below_min_and_above_max);
    RUN_TEST(test_degenerate_calibration_rejected);
    return UNITY_END();
}
