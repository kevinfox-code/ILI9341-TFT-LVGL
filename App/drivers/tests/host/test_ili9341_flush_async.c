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

static int s_cb_count;
static void *s_cb_user;

static void done_cb(void *user)
{
    s_cb_count++;
    s_cb_user = user;
}

void setUp(void)
{
    mock_seq_reset();
    mock_spi_reset();
    mock_gpio_reset();
    mock_os_reset();
    s_cb_count = 0;
    s_cb_user = NULL;

    cs_pin = (drv_gpio_t){ &cs_id, 1 };
    dc_pin = (drv_gpio_t){ &dc_id, 2 };
    reset_pin = (drv_gpio_t){ &reset_id, 3 };
    bus = drv_spi_bus_create((void *)0x3000);

    ili9341_config_t cfg = {
        .bus = bus, .cs = cs_pin, .dc = dc_pin, .reset = reset_pin,
        .hor_res = 320, .ver_res = 240, .rotation = 3,
    };
    disp = NULL;
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_Init(&disp, &cfg));

    mock_spi_reset_log();
    mock_gpio_reset();
    mock_seq_reset();
}

void tearDown(void) {}

void test_single_chunk_flush_completes_and_fires_cb_once(void)
{
    static uint8_t buf[200];
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_FlushAsync(disp, 0, 0, 9, 9, buf, sizeof(buf), done_cb, disp));

    TEST_ASSERT_TRUE(ILI9341_FlushBusy(disp));
    TEST_ASSERT_FALSE(mock_os_mutex_balance_ok()); /* bus lock still held */

    /* DC high + CS low happened before the tx_dma call. */
    int dma_idx = -1;
    for (int i = 0; i < mock_spi_event_count(); i++) {
        if (mock_spi_event(i)->kind == MOCK_SPI_TX_DMA) {
            dma_idx = i;
            break;
        }
    }
    TEST_ASSERT_TRUE(dma_idx >= 0);
    int dc_idx = mock_gpio_last_write_index(dc_pin);
    int cs_idx = mock_gpio_last_write_index(cs_pin);
    TEST_ASSERT_TRUE(dc_idx >= 0 && mock_gpio_event(dc_idx)->level);
    TEST_ASSERT_TRUE(cs_idx >= 0 && !mock_gpio_event(cs_idx)->level);
    TEST_ASSERT_TRUE(mock_gpio_event(dc_idx)->seq < mock_spi_event(dma_idx)->seq);
    TEST_ASSERT_TRUE(mock_gpio_event(cs_idx)->seq < mock_spi_event(dma_idx)->seq);

    mock_spi_fire_tx_complete(bus);

    TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    TEST_ASSERT_EQUAL_PTR(disp, s_cb_user);
    TEST_ASSERT_FALSE(ILI9341_FlushBusy(disp));
    TEST_ASSERT_TRUE(mock_gpio_current_level(cs_pin));
    TEST_ASSERT_TRUE(mock_os_mutex_balance_ok());
}

/* mock_spi_event_count() also includes the eleven single-byte window-set
 * writes (0x2A/0x2B/0x2C + address bytes) that precede the first DMA chunk,
 * so chunking assertions look only at MOCK_SPI_TX_DMA-kind events. */
static int dma_event_count(void)
{
    int n = 0;
    for (int i = 0; i < mock_spi_event_count(); i++) {
        if (mock_spi_event(i)->kind == MOCK_SPI_TX_DMA) {
            n++;
        }
    }
    return n;
}

static const mock_spi_event_t *nth_dma_event(int n)
{
    int seen = 0;
    for (int i = 0; i < mock_spi_event_count(); i++) {
        if (mock_spi_event(i)->kind == MOCK_SPI_TX_DMA) {
            if (seen == n) {
                return mock_spi_event(i);
            }
            seen++;
        }
    }
    return NULL;
}

void test_full_frame_chunks_into_three_dma_calls(void)
{
    static uint8_t buf[153600];
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_FlushAsync(disp, 0, 0, 319, 239, buf, sizeof(buf), done_cb, disp));

    TEST_ASSERT_EQUAL_INT(1, dma_event_count());
    TEST_ASSERT_EQUAL_UINT32(65534u, nth_dma_event(0)->len);
    TEST_ASSERT_EQUAL_PTR(buf, nth_dma_event(0)->ptr);
    TEST_ASSERT_FALSE(mock_gpio_current_level(cs_pin));

    mock_spi_fire_tx_complete(bus);
    TEST_ASSERT_EQUAL_INT(2, dma_event_count());
    TEST_ASSERT_EQUAL_UINT32(65534u, nth_dma_event(1)->len);
    TEST_ASSERT_EQUAL_PTR(buf + 65534u, nth_dma_event(1)->ptr);
    TEST_ASSERT_FALSE(mock_gpio_current_level(cs_pin));
    TEST_ASSERT_EQUAL_INT(0, s_cb_count);

    mock_spi_fire_tx_complete(bus);
    TEST_ASSERT_EQUAL_INT(3, dma_event_count());
    TEST_ASSERT_EQUAL_UINT32(153600u - 2u * 65534u, nth_dma_event(2)->len);
    TEST_ASSERT_EQUAL_PTR(buf + 2u * 65534u, nth_dma_event(2)->ptr);
    TEST_ASSERT_FALSE(mock_gpio_current_level(cs_pin));
    TEST_ASSERT_EQUAL_INT(0, s_cb_count);

    mock_spi_fire_tx_complete(bus);
    TEST_ASSERT_EQUAL_INT(3, dma_event_count()); /* no 4th chunk */
    TEST_ASSERT_TRUE(mock_gpio_current_level(cs_pin));
    TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    TEST_ASSERT_FALSE(ILI9341_FlushBusy(disp));
}

void test_tx_dma_failure_rolls_back_cleanly(void)
{
    mock_spi_force_next_tx_dma_result(DRV_ERR_HW);
    static uint8_t buf[16];
    drv_status_t st = ILI9341_FlushAsync(disp, 0, 0, 3, 3, buf, sizeof(buf), done_cb, disp);

    TEST_ASSERT_EQUAL(DRV_ERR_HW, st);
    TEST_ASSERT_TRUE(mock_gpio_current_level(cs_pin));
    TEST_ASSERT_TRUE(mock_os_mutex_balance_ok());
    TEST_ASSERT_EQUAL_INT(0, s_cb_count);
    TEST_ASSERT_FALSE(ILI9341_FlushBusy(disp));
}

void test_second_flush_while_busy_returns_state_error(void)
{
    static uint8_t buf[16];
    TEST_ASSERT_EQUAL(DRV_OK, ILI9341_FlushAsync(disp, 0, 0, 3, 3, buf, sizeof(buf), done_cb, disp));
    TEST_ASSERT_EQUAL(DRV_ERR_STATE, ILI9341_FlushAsync(disp, 0, 0, 3, 3, buf, sizeof(buf), done_cb, disp));

    /* First flush is unaffected by the rejected second call. */
    mock_spi_fire_tx_complete(bus);
    TEST_ASSERT_EQUAL_INT(1, s_cb_count);
    TEST_ASSERT_FALSE(ILI9341_FlushBusy(disp));
    TEST_ASSERT_TRUE(mock_os_mutex_balance_ok());
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_single_chunk_flush_completes_and_fires_cb_once);
    RUN_TEST(test_full_frame_chunks_into_three_dma_calls);
    RUN_TEST(test_tx_dma_failure_rolls_back_cleanly);
    RUN_TEST(test_second_flush_while_busy_returns_state_error);
    return UNITY_END();
}
