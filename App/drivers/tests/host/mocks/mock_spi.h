#ifndef MOCK_SPI_H_
#define MOCK_SPI_H_
#include "drv/drv_spi.h"
#include <stdint.h>

typedef enum {
    MOCK_SPI_TX,
    MOCK_SPI_TXRX,
    MOCK_SPI_TX_DMA,
} mock_spi_kind_t;

#ifndef MOCK_SPI_LOG_BYTES_CAP
#define MOCK_SPI_LOG_BYTES_CAP 16
#endif

typedef struct {
    int seq;
    mock_spi_kind_t kind;
    const uint8_t *ptr;      /* raw pointer passed by the driver, for offset math */
    uint32_t len;
    uint8_t bytes[MOCK_SPI_LOG_BYTES_CAP]; /* first bytes_len bytes, copied for inspection */
    uint32_t bytes_len;
} mock_spi_event_t;

void mock_spi_reset(void);

/* Clears only the recorded event log (and RX queue), leaving any already
 * created drv_spi_bus_t objects — and their locks/hooks — intact. Use this
 * between phases of a single test (e.g. "ignore Init's own traffic") where
 * mock_spi_reset() would invalidate a bus a driver instance still holds a
 * pointer to. */
void mock_spi_reset_log(void);

int  mock_spi_event_count(void);
const mock_spi_event_t *mock_spi_event(int index);

/* Bytes fed back to the driver's next drv_spi_txrx() calls, FIFO order. */
void mock_spi_queue_rx(const uint8_t *bytes, uint32_t n);

/* One-shot: makes the next drv_spi_tx_dma() call fail with this status. */
void mock_spi_force_next_tx_dma_result(drv_status_t st);

/* Caps drv_spi_max_dma_len() for this test (0 = use the real default, 65534). */
void mock_spi_set_max_dma_len(uint32_t max_len);

/* Simulates the DMA-complete ISR firing for `bus`: invokes the registered
 * tx-done hook, then gives the bus's DMA-done semaphore — mirrors
 * drv_spi_tx_complete_isr() in the real STM32 port. */
void mock_spi_fire_tx_complete(drv_spi_bus_t *bus);

/* Simulates the bus lock being held by someone else, so the next
 * drv_spi_bus_lock() on it returns DRV_ERR_TIMEOUT immediately. */
void mock_spi_force_bus_busy(drv_spi_bus_t *bus, bool busy);

#endif /* MOCK_SPI_H_ */
