#include "mock_spi.h"
#include "mock_os.h"
#include "mock_seq.h"
#include <string.h>

#ifndef MOCK_SPI_MAX_BUSES
#define MOCK_SPI_MAX_BUSES 2
#endif
#ifndef MOCK_SPI_MAX_EVENTS
#define MOCK_SPI_MAX_EVENTS 4096
#endif
#ifndef MOCK_SPI_RX_QUEUE_CAP
#define MOCK_SPI_RX_QUEUE_CAP 4096
#endif

struct drv_spi_bus {
    bool in_use;
    void *hal_handle;
    drv_mutex_t lock;
    drv_sem_t dma_done;
    bool tx_busy;
    drv_spi_tx_done_hook_t hook;
    void *hook_ctx;
};

static struct drv_spi_bus s_buses[MOCK_SPI_MAX_BUSES];
static mock_spi_event_t s_events[MOCK_SPI_MAX_EVENTS];
static int s_event_count = 0;

static uint8_t s_rx_queue[MOCK_SPI_RX_QUEUE_CAP];
static uint32_t s_rx_head = 0, s_rx_tail = 0;

static bool s_force_dma_fail = false;
static drv_status_t s_forced_dma_result = DRV_OK;
static uint32_t s_max_dma_len = 0; /* 0 => default */

void mock_spi_reset(void)
{
    memset(s_buses, 0, sizeof(s_buses));
    memset(s_events, 0, sizeof(s_events));
    s_event_count = 0;
    s_rx_head = s_rx_tail = 0;
    s_force_dma_fail = false;
    s_forced_dma_result = DRV_OK;
    s_max_dma_len = 0;
}

void mock_spi_reset_log(void)
{
    memset(s_events, 0, sizeof(s_events));
    s_event_count = 0;
    s_rx_head = s_rx_tail = 0;
    s_force_dma_fail = false;
    s_forced_dma_result = DRV_OK;
}

int mock_spi_event_count(void)
{
    return s_event_count;
}

const mock_spi_event_t *mock_spi_event(int index)
{
    if (index < 0 || index >= s_event_count) {
        return NULL;
    }
    return &s_events[index];
}

void mock_spi_queue_rx(const uint8_t *bytes, uint32_t n)
{
    for (uint32_t i = 0; i < n && s_rx_tail < MOCK_SPI_RX_QUEUE_CAP; i++) {
        s_rx_queue[s_rx_tail++] = bytes[i];
    }
}

void mock_spi_force_next_tx_dma_result(drv_status_t st)
{
    s_force_dma_fail = true;
    s_forced_dma_result = st;
}

void mock_spi_set_max_dma_len(uint32_t max_len)
{
    s_max_dma_len = max_len;
}

void mock_spi_force_bus_busy(drv_spi_bus_t *bus, bool busy)
{
    if (bus != NULL) {
        mock_mutex_force_busy(bus->lock, busy);
    }
}

static void log_event(mock_spi_kind_t kind, const uint8_t *ptr, uint32_t len)
{
    if (s_event_count >= MOCK_SPI_MAX_EVENTS) {
        return;
    }
    mock_spi_event_t *e = &s_events[s_event_count++];
    e->seq = mock_next_seq();
    e->kind = kind;
    e->ptr = ptr;
    e->len = len;
    uint32_t copy_len = (len < MOCK_SPI_LOG_BYTES_CAP) ? len : MOCK_SPI_LOG_BYTES_CAP;
    if (ptr != NULL && copy_len > 0) {
        memcpy(e->bytes, ptr, copy_len);
    }
    e->bytes_len = copy_len;
}

drv_spi_bus_t *drv_spi_bus_create(void *hal_spi_handle)
{
    for (int i = 0; i < MOCK_SPI_MAX_BUSES; i++) {
        if (s_buses[i].in_use && s_buses[i].hal_handle == hal_spi_handle) {
            return &s_buses[i];
        }
    }
    for (int i = 0; i < MOCK_SPI_MAX_BUSES; i++) {
        if (!s_buses[i].in_use) {
            s_buses[i].in_use = true;
            s_buses[i].hal_handle = hal_spi_handle;
            s_buses[i].lock = drv_mutex_create("mock_spi_bus");
            s_buses[i].dma_done = drv_sem_create_binary("mock_spi_dma");
            s_buses[i].tx_busy = false;
            s_buses[i].hook = NULL;
            s_buses[i].hook_ctx = NULL;
            return &s_buses[i];
        }
    }
    return NULL;
}

drv_status_t drv_spi_bus_lock(drv_spi_bus_t *bus, uint32_t timeout_ms)
{
    if (bus == NULL) return DRV_ERR_PARAM;
    return drv_mutex_lock(bus->lock, timeout_ms);
}

drv_status_t drv_spi_bus_unlock(drv_spi_bus_t *bus)
{
    if (bus == NULL) return DRV_ERR_PARAM;
    return drv_mutex_unlock(bus->lock);
}

drv_status_t drv_spi_tx(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (bus == NULL || data == NULL) return DRV_ERR_PARAM;
    log_event(MOCK_SPI_TX, data, len);
    return DRV_OK;
}

drv_status_t drv_spi_txrx(drv_spi_bus_t *bus, const uint8_t *tx, uint8_t *rx, uint32_t len, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (bus == NULL || tx == NULL || rx == NULL) return DRV_ERR_PARAM;
    log_event(MOCK_SPI_TXRX, tx, len);
    for (uint32_t i = 0; i < len; i++) {
        if (s_rx_head < s_rx_tail) {
            rx[i] = s_rx_queue[s_rx_head++];
        } else {
            rx[i] = 0;
        }
    }
    return DRV_OK;
}

uint32_t drv_spi_max_dma_len(void)
{
    return (s_max_dma_len != 0) ? s_max_dma_len : 65534u;
}

drv_status_t drv_spi_tx_dma(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len)
{
    if (bus == NULL || data == NULL) return DRV_ERR_PARAM;
    if (len > drv_spi_max_dma_len()) return DRV_ERR_PARAM;

    log_event(MOCK_SPI_TX_DMA, data, len);

    if (s_force_dma_fail) {
        s_force_dma_fail = false;
        return s_forced_dma_result;
    }

    bus->tx_busy = true;
    return DRV_OK;
}

drv_status_t drv_spi_wait_tx_done(drv_spi_bus_t *bus, uint32_t timeout_ms)
{
    if (bus == NULL) return DRV_ERR_PARAM;
    return drv_sem_take(bus->dma_done, timeout_ms);
}

bool drv_spi_tx_busy(drv_spi_bus_t *bus)
{
    return (bus != NULL) ? bus->tx_busy : false;
}

void drv_spi_bus_set_tx_done_hook(drv_spi_bus_t *bus, drv_spi_tx_done_hook_t hook, void *ctx)
{
    if (bus == NULL) return;
    bus->hook = hook;
    bus->hook_ctx = ctx;
}

void drv_spi_tx_complete_isr(drv_spi_bus_t *bus)
{
    if (bus == NULL) return;
    bus->tx_busy = false;
    if (bus->hook != NULL) {
        bus->hook(bus->hook_ctx);
    }
    drv_sem_give(bus->dma_done);
}

void mock_spi_fire_tx_complete(drv_spi_bus_t *bus)
{
    drv_spi_tx_complete_isr(bus);
}
