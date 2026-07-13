#include "unity.h"
#include "drv/xpt2046.h"
#include "mock_spi.h"
#include "mock_gpio.h"
#include "mock_os.h"
#include "mock_seq.h"

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
    bus = drv_spi_bus_create((void *)0x4000);

    xpt2046_config_t cfg = {
        .bus = bus, .cs = cs_pin, .irq = irq_pin,
        .raw_x_min = 262, .raw_x_max = 1872, .raw_y_min = 230, .raw_y_max = 1872,
    };
    touch = NULL;
    TEST_ASSERT_EQUAL(DRV_OK, XPT2046_Init(&touch, &cfg));

    mock_spi_reset_log();
    mock_gpio_reset();
    mock_seq_reset();
}

void tearDown(void) {}

static void queue_sample(uint16_t v)
{
    uint8_t g[3] = { 0, (uint8_t)(v >> 4), (uint8_t)((v << 4) & 0xFF) };
    mock_spi_queue_rx(g, 3);
}

void test_read_raw_averages_discard_first_of_each_axis(void)
{
    mock_gpio_set_level(irq_pin, false); /* pressed */

    queue_sample(9999); /* X discard, value irrelevant */
    queue_sample(500);  /* X1 */
    queue_sample(502);  /* X2 */
    queue_sample(8888); /* Y discard */
    queue_sample(300);  /* Y1 */
    queue_sample(302);  /* Y2 */

    uint16_t rx = 0, ry = 0;
    TEST_ASSERT_TRUE(XPT2046_ReadRaw(touch, &rx, &ry));
    TEST_ASSERT_EQUAL_UINT16(501, rx);
    TEST_ASSERT_EQUAL_UINT16(301, ry);

    TEST_ASSERT_EQUAL_INT(6, mock_spi_event_count());
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL_HEX8(0xD0, mock_spi_event(i)->bytes[0]);
    }
    for (int i = 3; i < 6; i++) {
        TEST_ASSERT_EQUAL_HEX8(0x90, mock_spi_event(i)->bytes[0]);
    }
}

void test_bus_lock_timeout_never_touches_cs(void)
{
    mock_gpio_set_level(irq_pin, false); /* pressed, so we get past the PENIRQ gate */
    mock_spi_force_bus_busy(bus, true);

    uint16_t rx = 0, ry = 0;
    TEST_ASSERT_FALSE(XPT2046_ReadRaw(touch, &rx, &ry));
    TEST_ASSERT_EQUAL_INT(0, mock_spi_event_count());
    TEST_ASSERT_EQUAL_INT(-1, mock_gpio_last_write_index(cs_pin));

    mock_spi_force_bus_busy(bus, false);
}

void test_pen_up_skips_spi_traffic(void)
{
    mock_gpio_set_level(irq_pin, true); /* released */

    uint16_t rx = 0, ry = 0;
    TEST_ASSERT_FALSE(XPT2046_ReadRaw(touch, &rx, &ry));
    TEST_ASSERT_EQUAL_INT(0, mock_spi_event_count());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_read_raw_averages_discard_first_of_each_axis);
    RUN_TEST(test_bus_lock_timeout_never_touches_cs);
    RUN_TEST(test_pen_up_skips_spi_traffic);
    return UNITY_END();
}
