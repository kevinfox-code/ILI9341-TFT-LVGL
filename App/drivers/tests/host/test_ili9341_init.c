#include "unity.h"
#include "drv/ili9341.h"
#include "mock_spi.h"
#include "mock_gpio.h"
#include "mock_os.h"
#include "mock_seq.h"

static drv_gpio_t cs_pin, dc_pin, reset_pin;
static int cs_id, dc_id, reset_id;
static drv_spi_bus_t *bus;

void setUp(void)
{
    mock_seq_reset();
    mock_spi_reset();
    mock_gpio_reset();
    mock_os_reset();

    cs_pin = (drv_gpio_t){ &cs_id, 1 };
    dc_pin = (drv_gpio_t){ &dc_id, 2 };
    reset_pin = (drv_gpio_t){ &reset_id, 3 };

    bus = drv_spi_bus_create((void *)0x1000);
}

void tearDown(void) {}

/* Every single-byte command/data write ILI9341_Init emits, in order,
 * including the trailing MADCTL write for rotation 3 (0xE8). */
static const uint8_t k_expected_bytes[] = {
    0x01,
    0x28,
    0xCF, 0x00, 0xC1, 0x30,
    0xED, 0x64, 0x03, 0x12, 0x81,
    0xE8, 0x85, 0x00, 0x78,
    0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,
    0xF7, 0x20,
    0xEA, 0x00, 0x00,
    0xC0, 0x23,
    0xC1, 0x10,
    0xC5, 0x3e, 0x28,
    0xC7, 0x86,
    0x3A, 0x55,
    0xB1, 0x00, 0x18,
    0xB6, 0x08, 0x82, 0x27,
    0xF2, 0x00,
    0x26, 0x01,
    0x11,
    0x29,
    0x36, 0xE8,
};

void test_init_reset_pulse_ordering(void)
{
    ili9341_t *d = NULL;
    ili9341_config_t cfg = {
        .bus = bus, .cs = cs_pin, .dc = dc_pin, .reset = reset_pin,
        .hor_res = 320, .ver_res = 240, .rotation = 3,
    };
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_Init(&d, &cfg));

    TEST_ASSERT_TRUE(mock_gpio_event_count() >= 2);
    TEST_ASSERT_EQUAL_PTR(reset_pin.port, mock_gpio_event(0)->pin.port);
    TEST_ASSERT_FALSE(mock_gpio_event(0)->level);
    TEST_ASSERT_EQUAL_PTR(reset_pin.port, mock_gpio_event(1)->pin.port);
    TEST_ASSERT_TRUE(mock_gpio_event(1)->level);
}

void test_init_command_byte_sequence(void)
{
    ili9341_t *d = NULL;
    ili9341_config_t cfg = {
        .bus = bus, .cs = cs_pin, .dc = dc_pin, .reset = reset_pin,
        .hor_res = 320, .ver_res = 240, .rotation = 3,
    };
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_Init(&d, &cfg));

    int n = mock_spi_event_count();
    TEST_ASSERT_EQUAL_INT((int)(sizeof(k_expected_bytes) / sizeof(k_expected_bytes[0])), n);
    for (int i = 0; i < n; i++) {
        const mock_spi_event_t *e = mock_spi_event(i);
        TEST_ASSERT_EQUAL_INT(MOCK_SPI_TX, e->kind);
        TEST_ASSERT_EQUAL_UINT32(1u, e->len);
        TEST_ASSERT_EQUAL_HEX8(k_expected_bytes[i], e->bytes[0]);
    }
}

void test_init_locks_and_unlocks_bus(void)
{
    ili9341_t *d = NULL;
    ili9341_config_t cfg = {
        .bus = bus, .cs = cs_pin, .dc = dc_pin, .reset = reset_pin,
        .hor_res = 320, .ver_res = 240, .rotation = 3,
    };
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_Init(&d, &cfg));
    TEST_ASSERT_TRUE(mock_os_mutex_balance_ok());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_reset_pulse_ordering);
    RUN_TEST(test_init_command_byte_sequence);
    RUN_TEST(test_init_locks_and_unlocks_bus);
    return UNITY_END();
}
