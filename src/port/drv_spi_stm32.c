#include "drv/drv_spi.h"
#include "drv/drv_config.h"
#include <string.h>

#ifndef DRV_SPI_MAX_BUSES
#define DRV_SPI_MAX_BUSES 2
#endif

#ifndef DRV_SPI_TIMEOUT_MS
#define DRV_SPI_TIMEOUT_MS 100u
#endif

/* HAL fires TxCplt from the DMA TC interrupt while the SPI shifter may still
 * be emptying the last byte; bound the wait for BSY to clear so a stuck bus
 * cannot hang an ISR forever. */
#ifndef DRV_SPI_BSY_SPIN_LIMIT
#define DRV_SPI_BSY_SPIN_LIMIT 10000u
#endif

struct drv_spi_bus {
    bool in_use;
    SPI_HandleTypeDef *hspi;
    drv_mutex_t lock;
    drv_sem_t   dma_done;
    volatile bool tx_busy;
    drv_spi_tx_done_hook_t hook;
    void *hook_ctx;
};

static struct drv_spi_bus s_buses[DRV_SPI_MAX_BUSES];

drv_spi_bus_t *drv_spi_bus_create(void *hal_spi_handle)
{
    if (hal_spi_handle == NULL) {
        return NULL;
    }
    for (int i = 0; i < DRV_SPI_MAX_BUSES; i++) {
        if (s_buses[i].in_use && s_buses[i].hspi == (SPI_HandleTypeDef *)hal_spi_handle) {
            return &s_buses[i];
        }
    }
    for (int i = 0; i < DRV_SPI_MAX_BUSES; i++) {
        if (!s_buses[i].in_use) {
            s_buses[i].in_use = true;
            s_buses[i].hspi = (SPI_HandleTypeDef *)hal_spi_handle;
            s_buses[i].lock = drv_mutex_create("drv_spi_bus");
            s_buses[i].dma_done = drv_sem_create_binary("drv_spi_dma");
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
    if (bus == NULL) {
        return DRV_ERR_PARAM;
    }
    return drv_mutex_lock(bus->lock, timeout_ms);
}

drv_status_t drv_spi_bus_unlock(drv_spi_bus_t *bus)
{
    if (bus == NULL) {
        return DRV_ERR_PARAM;
    }
    return drv_mutex_unlock(bus->lock);
}

drv_status_t drv_spi_tx(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len, uint32_t timeout_ms)
{
    if (bus == NULL || data == NULL) {
        return DRV_ERR_PARAM;
    }
    HAL_StatusTypeDef st = HAL_SPI_Transmit(bus->hspi, (uint8_t *)data, (uint16_t)len, timeout_ms);
    return (st == HAL_OK) ? DRV_OK : DRV_ERR_HW;
}

drv_status_t drv_spi_txrx(drv_spi_bus_t *bus, const uint8_t *tx, uint8_t *rx, uint32_t len, uint32_t timeout_ms)
{
    if (bus == NULL || tx == NULL || rx == NULL) {
        return DRV_ERR_PARAM;
    }
    HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(bus->hspi, (uint8_t *)tx, rx, (uint16_t)len, timeout_ms);
    return (st == HAL_OK) ? DRV_OK : DRV_ERR_HW;
}

uint32_t drv_spi_max_dma_len(void)
{
    return 65534u;
}

drv_status_t drv_spi_tx_dma(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len)
{
    if (bus == NULL || data == NULL) {
        return DRV_ERR_PARAM;
    }
    if (len > drv_spi_max_dma_len()) {
        return DRV_ERR_PARAM;
    }
    bus->tx_busy = true;
    HAL_StatusTypeDef st = HAL_SPI_Transmit_DMA(bus->hspi, (uint8_t *)data, (uint16_t)len);
    if (st != HAL_OK) {
        bus->tx_busy = false;
        return DRV_ERR_HW;
    }
    return DRV_OK;
}

drv_status_t drv_spi_wait_tx_done(drv_spi_bus_t *bus, uint32_t timeout_ms)
{
    if (bus == NULL) {
        return DRV_ERR_PARAM;
    }
    return drv_sem_take(bus->dma_done, timeout_ms);
}

bool drv_spi_tx_busy(drv_spi_bus_t *bus)
{
    if (bus == NULL) {
        return false;
    }
    return bus->tx_busy;
}

void drv_spi_bus_set_tx_done_hook(drv_spi_bus_t *bus, drv_spi_tx_done_hook_t hook, void *ctx)
{
    if (bus == NULL) {
        return;
    }
    bus->hook = hook;
    bus->hook_ctx = ctx;
}

void drv_spi_tx_complete_isr(drv_spi_bus_t *bus)
{
    if (bus == NULL) {
        return;
    }

    uint32_t spins = 0u;
    while (__HAL_SPI_GET_FLAG(bus->hspi, SPI_FLAG_BSY) && spins < DRV_SPI_BSY_SPIN_LIMIT) {
        spins++;
    }

    bus->tx_busy = false;

    if (bus->hook != NULL) {
        bus->hook(bus->hook_ctx);
    }

    drv_sem_give(bus->dma_done);
}
