#ifndef ILI9341_H
#define ILI9341_H

#include "stm32f4xx_hal.h"
#include "lvgl.h"

#define ILI9341_WIDTH     240
#define ILI9341_HEIGHT    320

/**
 * @brief ILI9341 Display Configuration Structure
 */
typedef struct {
    SPI_HandleTypeDef *hspi;        /* SPI peripheral handle */
    GPIO_TypeDef *cs_port;          /* Chip Select GPIO port */
    uint16_t cs_pin;                /* Chip Select GPIO pin */
    GPIO_TypeDef *dc_port;          /* Data/Command GPIO port */
    uint16_t dc_pin;                /* Data/Command GPIO pin */
    GPIO_TypeDef *reset_port;       /* Reset GPIO port */
    uint16_t reset_pin;             /* Reset GPIO pin */
    uint16_t hor_res;               /* Logical horizontal resolution after rotation */
    uint16_t ver_res;               /* Logical vertical resolution after rotation */
} ILI9341_Config_t;

void ILI9341_Init(const ILI9341_Config_t *config);
void ILI9341_FillScreen(uint16_t color);
void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ILI9341_SetRotation(uint8_t rotation);
void ILI9341_WriteCommand(uint8_t cmd);
void ILI9341_WriteData(uint8_t data);
void ILI9341_WriteData16(uint16_t data);
void ILI9341_CancelFlushCallback(void);


//LVGL
void ILI9341_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void ILI9341_DrawBitmap(uint16_t w, uint16_t h, uint8_t *s);
void ILI9341_DrawBitmapDMA(uint16_t w, uint16_t h, uint8_t *s, lv_display_t *disp);

#endif
