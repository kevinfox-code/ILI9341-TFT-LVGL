#include "unity.h"
#include "drv/ili9341.h"
#include "mock_spi.h"
#include "mock_gpio.h"
#include "mock_os.h"
#include "mock_seq.h"

static drv_gpio_t cs_pin, dc_pin, reset_pin;
static int cs_id, dc_id, reset_id;
static drv_spi_bus_t *bus;
static ili9341_t *disp;

void setUp(void)
{
    mock_seq_reset();
    mock_spi_reset();
    mock_gpio_reset();
    mock_os_reset();

    cs_pin = (drv_gpio_t){ &cs_id, 1 };
    dc_pin = (drv_gpio_t){ &dc_id, 2 };
    reset_pin = (drv_gpio_t){ &reset_id, 3 };
    bus = drv_spi_bus_create((void *)0x2000);

    ili9341_config_t cfg = {
        .bus = bus, .cs = cs_pin, .dc = dc_pin, .reset = reset_pin,
        .hor_res = 320, .ver_res = 240, .rotation = 3,
    };
    disp = NULL;
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_Init(&disp, &cfg));

    /* Isolate the window/flush traffic from Init's own SPI log. */
    mock_spi_reset_log();
    mock_seq_reset();
}

void tearDown(void) {}

static void assert_window(int start_index, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    /* 0x2A, x0hi, x0lo, x1hi, x1lo, 0x2B, y0hi, y0lo, y1hi, y1lo, 0x2C */
    const mock_spi_event_t *e;
    e = mock_spi_event(start_index + 0);
    TEST_ASSERT_EQUAL_HEX8(0x2A, e->bytes[0]);
    e = mock_spi_event(start_index + 1); TEST_ASSERT_EQUAL_HEX8((uint8_t)(x0 >> 8), e->bytes[0]);
    e = mock_spi_event(start_index + 2); TEST_ASSERT_EQUAL_HEX8((uint8_t)(x0 & 0xFF), e->bytes[0]);
    e = mock_spi_event(start_index + 3); TEST_ASSERT_EQUAL_HEX8((uint8_t)(x1 >> 8), e->bytes[0]);
    e = mock_spi_event(start_index + 4); TEST_ASSERT_EQUAL_HEX8((uint8_t)(x1 & 0xFF), e->bytes[0]);
    e = mock_spi_event(start_index + 5);
    TEST_ASSERT_EQUAL_HEX8(0x2B, e->bytes[0]);
    e = mock_spi_event(start_index + 6); TEST_ASSERT_EQUAL_HEX8((uint8_t)(y0 >> 8), e->bytes[0]);
    e = mock_spi_event(start_index + 7); TEST_ASSERT_EQUAL_HEX8((uint8_t)(y0 & 0xFF), e->bytes[0]);
    e = mock_spi_event(start_index + 8); TEST_ASSERT_EQUAL_HEX8((uint8_t)(y1 >> 8), e->bytes[0]);
    e = mock_spi_event(start_index + 9); TEST_ASSERT_EQUAL_HEX8((uint8_t)(y1 & 0xFF), e->bytes[0]);
    e = mock_spi_event(start_index + 10);
    TEST_ASSERT_EQUAL_HEX8(0x2C, e->bytes[0]);
}

void test_flush_window_full_screen(void)
{
    uint8_t px[2] = { 0x12, 0x34 };
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_Flush(disp, 0, 0, 319, 239, px, 2, 100));
    assert_window(0, 0, 0, 319, 239);
}

void test_flush_window_1x1(void)
{
    uint8_t px[2] = { 0xAA, 0xBB };
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_Flush(disp, 5, 7, 5, 7, px, 2, 100));
    assert_window(0, 5, 7, 5, 7);
}

void test_madctl_per_rotation(void)
{
    struct { uint8_t rot; uint8_t madctl; } cases[] = {
        { 0, 0x48 }, { 1, 0x28 }, { 2, 0x88 }, { 3, 0xE8 },
    };
    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        mock_spi_reset_log();
        TEST_ASSERT_EQUAL(DRV_OK, ILI9341_SetRotation(disp, cases[i].rot));
        TEST_ASSERT_EQUAL_INT(2, mock_spi_event_count());
        TEST_ASSERT_EQUAL_HEX8(0x36, mock_spi_event(0)->bytes[0]);
        TEST_ASSERT_EQUAL_HEX8(cases[i].madctl, mock_spi_event(1)->bytes[0]);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_flush_window_full_screen);
    RUN_TEST(test_flush_window_1x1);
    RUN_TEST(test_madctl_per_rotation);
    return UNITY_END();
}
