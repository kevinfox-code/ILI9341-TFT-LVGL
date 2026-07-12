#ifndef DRV_SPI_H_
#define DRV_SPI_H_
#include <stdint.h>
#include <stdbool.h>
#include "drv/drv_os.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct drv_spi_bus drv_spi_bus_t;   /* opaque; defined in drv_spi_stm32.c */

typedef void (*drv_spi_tx_done_hook_t)(void *ctx);

drv_spi_bus_t *drv_spi_bus_create(void *hal_spi_handle /* SPI_HandleTypeDef* */);
drv_status_t drv_spi_bus_lock(drv_spi_bus_t *bus, uint32_t timeout_ms);
drv_status_t drv_spi_bus_unlock(drv_spi_bus_t *bus);

/* Blocking transfers. Caller must hold the bus lock. */
drv_status_t drv_spi_tx(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len, uint32_t timeout_ms);
drv_status_t drv_spi_txrx(drv_spi_bus_t *bus, const uint8_t *tx, uint8_t *rx, uint32_t len, uint32_t timeout_ms);

/* Non-blocking DMA TX. Caller must hold the bus lock and keep holding it until
 * completion is observed. len must not exceed drv_spi_max_dma_len(); the caller
 * chunks larger transfers. */
drv_status_t drv_spi_tx_dma(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len);
uint32_t     drv_spi_max_dma_len(void);          /* 65534 on STM32F4 HAL */
drv_status_t drv_spi_wait_tx_done(drv_spi_bus_t *bus, uint32_t timeout_ms); /* takes DMA sem */
bool         drv_spi_tx_busy(drv_spi_bus_t *bus);

/* Registers a hook invoked (in ISR context) from drv_spi_tx_complete_isr, after
 * the BSY-clear spin and before the DMA-done semaphore is given. */
void drv_spi_bus_set_tx_done_hook(drv_spi_bus_t *bus, drv_spi_tx_done_hook_t hook, void *ctx);

/* ISR entry — forward from HAL TxCplt for this bus's handle (see drv_isr.h). */
void drv_spi_tx_complete_isr(drv_spi_bus_t *bus);

#ifdef __cplusplus
}
#endif
#endif /* DRV_SPI_H_ */
