#include "XPT2046.h"
#include "display_runtime_config.h"

/* Command bytes */
#define CMD_X   0xD0
#define CMD_Y   0x90

static const XPT2046_Config_t *xpt_config = NULL;

#define CS_LOW()   HAL_GPIO_WritePin(xpt_config->cs_port, xpt_config->cs_pin, GPIO_PIN_RESET)
#define CS_HIGH()  HAL_GPIO_WritePin(xpt_config->cs_port, xpt_config->cs_pin, GPIO_PIN_SET)

/* Reads one 12-bit value (throws away bottom 4 bits) */
static uint16_t spi_xfer(uint8_t cmd) {
    uint8_t tx[3] = { cmd, 0, 0 }, rx[3];
    HAL_SPI_TransmitReceive(xpt_config->hspi, tx, rx, 3, DISPLAY_SPI_TIMEOUT_MS);
    return (((uint16_t)rx[1] << 8) | rx[2]) >> 4;
}

void XPT2046_Init(const XPT2046_Config_t *config) {
    xpt_config = config;
    CS_HIGH();
}

bool XPT2046_TouchDetected(void) {
    /* If IRQ port is not configured, return false (polling mode not supported without IRQ) */
    if (xpt_config->irq_port == NULL) {
        return false;
    }
    /* PENIRQ is active-low: true when finger is present */
    return HAL_GPIO_ReadPin(xpt_config->irq_port, xpt_config->irq_pin) == GPIO_PIN_RESET;
}

bool XPT2046_GetTouch(uint16_t *x, uint16_t *y) {
    if(!XPT2046_TouchDetected()) return false;

    CS_LOW();
    /* Discard first read of each axis (settling time), then average two. */
    spi_xfer(CMD_X);
    uint16_t x1 = spi_xfer(CMD_X);
    uint16_t x2 = spi_xfer(CMD_X);
    spi_xfer(CMD_Y);
    uint16_t y1 = spi_xfer(CMD_Y);
    uint16_t y2 = spi_xfer(CMD_Y);
    CS_HIGH();

    *x = (x1 + x2) >> 1;
    *y = (y1 + y2) >> 1;
    return true;
}
