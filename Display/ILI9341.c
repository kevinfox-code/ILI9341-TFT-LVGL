#include "ILI9341.h"
#include "main.h"
#include "lvgl.h"
#include "display_runtime_config.h"
#include <stdbool.h>

#ifdef FREERTOS
#include "cmsis_os2.h"
#endif

#define DISPLAY_DELAY_MS(ms) HAL_Delay(ms)

static const ILI9341_Config_t *ili_config = NULL;
static lv_display_t *dma_display = NULL;
volatile bool ili_dma_busy = false;

#define CS_LOW()  HAL_GPIO_WritePin(ili_config->cs_port, ili_config->cs_pin, GPIO_PIN_RESET)
#define CS_HIGH() HAL_GPIO_WritePin(ili_config->cs_port, ili_config->cs_pin, GPIO_PIN_SET)

#define DC_LOW()  HAL_GPIO_WritePin(ili_config->dc_port, ili_config->dc_pin, GPIO_PIN_RESET)
#define DC_HIGH() HAL_GPIO_WritePin(ili_config->dc_port, ili_config->dc_pin, GPIO_PIN_SET)

#define RESET_LOW()  HAL_GPIO_WritePin(ili_config->reset_port, ili_config->reset_pin, GPIO_PIN_RESET)
#define RESET_HIGH() HAL_GPIO_WritePin(ili_config->reset_port, ili_config->reset_pin, GPIO_PIN_SET)

static void ILI9341_Write(uint8_t *data, uint16_t size) {
    HAL_SPI_Transmit(ili_config->hspi, data, size, DISPLAY_SPI_TIMEOUT_MS);
}

void ILI9341_WriteCommand(uint8_t cmd) {
    DC_LOW();
    CS_LOW();
    ILI9341_Write(&cmd, 1);
    CS_HIGH();
}

void ILI9341_WriteData(uint8_t data) {
    DC_HIGH();
    CS_LOW();
    ILI9341_Write(&data, 1);
    CS_HIGH();
}

void ILI9341_WriteData16(uint16_t data) {
    uint8_t buf[2] = {data >> 8, data & 0xFF};
    DC_HIGH();
    CS_LOW();
    ILI9341_Write(buf, 2);
    CS_HIGH();
}

void ILI9341_Reset(void) {
    RESET_LOW();
    DISPLAY_DELAY_MS(20);
    RESET_HIGH();
    DISPLAY_DELAY_MS(150);
}

void ILI9341_Init(const ILI9341_Config_t *config) {
    ili_config = config;
    ILI9341_Reset();

    ILI9341_WriteCommand(0x01);
    DISPLAY_DELAY_MS(10);
    ILI9341_WriteCommand(0x28);

    ILI9341_WriteCommand(0xCF);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0xC1);
    ILI9341_WriteData(0x30);

    ILI9341_WriteCommand(0xED);
    ILI9341_WriteData(0x64);
    ILI9341_WriteData(0x03);
    ILI9341_WriteData(0x12);
    ILI9341_WriteData(0x81);

    ILI9341_WriteCommand(0xE8);
    ILI9341_WriteData(0x85);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x78);

    ILI9341_WriteCommand(0xCB);
    ILI9341_WriteData(0x39);
    ILI9341_WriteData(0x2C);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x34);
    ILI9341_WriteData(0x02);

    ILI9341_WriteCommand(0xF7);
    ILI9341_WriteData(0x20);

    ILI9341_WriteCommand(0xEA);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);

    ILI9341_WriteCommand(0xC0);
    ILI9341_WriteData(0x23);

    ILI9341_WriteCommand(0xC1);
    ILI9341_WriteData(0x10);

    ILI9341_WriteCommand(0xC5);
    ILI9341_WriteData(0x3e);
    ILI9341_WriteData(0x28);

    ILI9341_WriteCommand(0xC7);
    ILI9341_WriteData(0x86);

    ILI9341_WriteCommand(0x3A);
    ILI9341_WriteData(0x55);

    ILI9341_WriteCommand(0xB1);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x18);

    ILI9341_WriteCommand(0xB6);
    ILI9341_WriteData(0x08);
    ILI9341_WriteData(0x82);
    ILI9341_WriteData(0x27);

    ILI9341_WriteCommand(0xF2);
    ILI9341_WriteData(0x00);

    ILI9341_WriteCommand(0x26);
    ILI9341_WriteData(0x01);

    ILI9341_WriteCommand(0x11);
    DISPLAY_DELAY_MS(120);
    ILI9341_WriteCommand(0x29);
}

void ILI9341_FillScreen(uint16_t color) {
    uint16_t w = ili_config->hor_res;
    uint16_t h = ili_config->ver_res;

    ILI9341_WriteCommand(0x2A); // Column address
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData((w - 1) >> 8);
    ILI9341_WriteData((w - 1) & 0xFF);

    ILI9341_WriteCommand(0x2B); // Row address
    ILI9341_WriteData(0x00);
    ILI9341_WriteData(0x00);
    ILI9341_WriteData((h - 1) >> 8);
    ILI9341_WriteData((h - 1) & 0xFF);

    ILI9341_WriteCommand(0x2C); // Memory write

    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    /* Stack-safe line buffer; w is at most 320 for a 2.4" panel */
    uint8_t line_buf[320 * 2];
    for (int i = 0; i < w; i++) {
        line_buf[i * 2]     = hi;
        line_buf[i * 2 + 1] = lo;
    }

    DC_HIGH();
    CS_LOW();
    for (int y = 0; y < h; y++) {
        HAL_SPI_Transmit(ili_config->hspi, line_buf, (uint16_t)(w * 2), DISPLAY_SPI_TIMEOUT_MS);
    }
    CS_HIGH();
}

void ILI9341_SetRotation(uint8_t m)
{
    ILI9341_WriteCommand(0x36);
    switch (m) {
        case 0:
            ILI9341_WriteData(0x48); // MX | BGR
            break;
        case 1:
            ILI9341_WriteData(0x28); // MY | BGR
            break;
        case 2:
            ILI9341_WriteData(0x88); // MX | MY | BGR
            break;
        case 3:
            ILI9341_WriteData(0xE8); // MX | MY | MV | BGR (landscape)
            break;
    }
}


// LVGL
void ILI9341_SetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ILI9341_WriteCommand(0x2A);
    ILI9341_WriteData(x0 >> 8);
    ILI9341_WriteData(x0 & 0xFF);
    ILI9341_WriteData(x1 >> 8);
    ILI9341_WriteData(x1 & 0xFF);

    ILI9341_WriteCommand(0x2B);
    ILI9341_WriteData(y0 >> 8);
    ILI9341_WriteData(y0 & 0xFF);
    ILI9341_WriteData(y1 >> 8);
    ILI9341_WriteData(y1 & 0xFF);

    ILI9341_WriteCommand(0x2C);
}

void ILI9341_DrawBitmap(uint16_t w, uint16_t h, uint8_t *s)
{
    /* 0x2C was already sent by ILI9341_SetWindow; go straight to data */
    DC_HIGH();
    CS_LOW();
    HAL_SPI_Transmit(ili_config->hspi, s, (uint16_t)(w * h * 2), DISPLAY_SPI_TIMEOUT_MS);
    CS_HIGH();
}

void ILI9341_DrawBitmapDMA(uint16_t w, uint16_t h, uint8_t *s, lv_display_t *disp)
{
    // Memory write command (0x2C) is already sent by ILI9341_SetWindow
    DC_HIGH();
    CS_LOW();
    dma_display = disp;
    HAL_SPI_Transmit_DMA(ili_config->hspi, s, w * h * 2);
}

void ILI9341_CancelFlushCallback(void)
{
    dma_display = NULL;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(ili_config == NULL) {
        return;
    }

    if(hspi == ili_config->hspi) {
        CS_HIGH();
        ili_dma_busy = false;
        if(dma_display) {
            lv_display_flush_ready(dma_display);
            dma_display = NULL;
        }
    }
}
