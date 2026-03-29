/**
 * XPT2046.h — SPI touch controller driver for LVGL
 * (c) Kevin Fox 2025
 */
#ifndef XPT2046_H_
#define XPT2046_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/**
 * @brief XPT2046 Touch Controller Configuration Structure
 */
typedef struct {
    SPI_HandleTypeDef *hspi;        /* SPI peripheral handle */
    GPIO_TypeDef *cs_port;          /* Chip Select GPIO port */
    uint16_t cs_pin;                /* Chip Select GPIO pin */
    GPIO_TypeDef *irq_port;         /* IRQ (PENIRQ) GPIO port (optional, can be NULL) */
    uint16_t irq_pin;               /* IRQ (PENIRQ) GPIO pin */
} XPT2046_Config_t;

void XPT2046_Init(const XPT2046_Config_t *config);
bool XPT2046_TouchDetected(void);
bool XPT2046_GetTouch(uint16_t *x, uint16_t *y);

#endif /* XPT2046_H_ */
