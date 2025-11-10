/**
 * @file lv_st_spi_display.c
 * @brief STM32 SPI Display Driver Implementation for LVGL 9.4
 * 
 * This is an example implementation for ILI9341-based SPI displays.
 * Modify the initialization commands and functions for your specific display.
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_st_spi_display.h"

#if LV_USE_ST_SPI_DISPLAY

#include <string.h>

/*********************
 *      DEFINES
 *********************/

/* ILI9341 Commands - Modify for your display controller */
#define ILI9341_NOP        0x00
#define ILI9341_SWRESET    0x01
#define ILI9341_SLPOUT     0x11
#define ILI9341_NORON      0x13
#define ILI9341_INVOFF     0x20
#define ILI9341_DISPON     0x29
#define ILI9341_CASET      0x2A
#define ILI9341_PASET      0x2B
#define ILI9341_RAMWR      0x2C
#define ILI9341_MADCTL     0x36
#define ILI9341_COLMOD     0x3A

/* MADCTL bits */
#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    uint16_t width;
    uint16_t height;
    spi_display_orientation_t orientation;
    bool is_direct_mode;
} spi_display_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void delay_ms(uint32_t ms);

/**********************
 *  STATIC VARIABLES
 **********************/
extern SPI_HandleTypeDef SPI_DISPLAY_SPI_HANDLE;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_display_t * lv_st_spi_display_create(uint16_t width, uint16_t height, bool use_direct_mode)
{
    /* Hardware initialization */
    if (!lv_st_spi_display_hw_init()) {
        LV_LOG_ERROR("Failed to initialize SPI display hardware");
        return NULL;
    }
    
    /* Create LVGL display */
    lv_display_t * disp = lv_display_create(width, height);
    if (disp == NULL) {
        LV_LOG_ERROR("Failed to create display");
        return NULL;
    }
    
    /* Allocate context */
    spi_display_ctx_t * ctx = lv_malloc(sizeof(spi_display_ctx_t));
    if (ctx == NULL) {
        LV_LOG_ERROR("Failed to allocate context");
        lv_display_delete(disp);
        return NULL;
    }
    
    ctx->width = width;
    ctx->height = height;
    ctx->orientation = SPI_DISPLAY_ORIENTATION_PORTRAIT;
    ctx->is_direct_mode = use_direct_mode;
    
    /* Configure buffers */
    if (use_direct_mode) {
        /* Direct mode - full framebuffer */
        uint32_t fb_size = width * height * 2;  /* RGB565 = 2 bytes per pixel */
        void * fb = lv_malloc(fb_size);
        
        if (fb == NULL) {
            LV_LOG_ERROR("Failed to allocate framebuffer");
            lv_free(ctx);
            lv_display_delete(disp);
            return NULL;
        }
        
        lv_display_set_buffers(disp, fb, NULL, fb_size, LV_DISPLAY_RENDER_MODE_DIRECT);
    } else {
        /* Partial mode - smaller buffers */
        uint32_t buf_size = width * 40 * 2;  /* 40 lines buffer */
        void * buf1 = lv_malloc(buf_size);
        void * buf2 = lv_malloc(buf_size);  /* Double buffering recommended for DMA */
        
        if (buf1 == NULL) {
            LV_LOG_ERROR("Failed to allocate buffer 1");
            lv_free(ctx);
            lv_display_delete(disp);
            return NULL;
        }
        
        lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    }
    
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_driver_data(disp, ctx);
    
    LV_LOG_INFO("SPI display created: %dx%d, %s mode", 
                width, height, use_direct_mode ? "direct" : "partial");
    
    return disp;
}

bool lv_st_spi_display_hw_init(void)
{
    /* Hardware reset sequence */
    LCD_CS_HIGH();
    LCD_RST_LOW();
    delay_ms(10);
    LCD_RST_HIGH();
    delay_ms(120);
    
    /* Send initialization commands for ILI9341 */
    /* Modify these for your specific display controller */
    
    /* Software reset */
    lv_st_spi_display_send_cmd(ILI9341_SWRESET);
    delay_ms(150);
    
    /* Exit sleep mode */
    lv_st_spi_display_send_cmd(ILI9341_SLPOUT);
    delay_ms(120);
    
    /* Pixel format: 16-bit/pixel (RGB565) */
    lv_st_spi_display_send_cmd(ILI9341_COLMOD);
    uint8_t data = 0x55;
    lv_st_spi_display_send_data(&data, 1);
    
    /* Memory Access Control */
    lv_st_spi_display_send_cmd(ILI9341_MADCTL);
    data = MADCTL_MX | MADCTL_BGR;
    lv_st_spi_display_send_data(&data, 1);
    
    /* Normal display mode */
    lv_st_spi_display_send_cmd(ILI9341_NORON);
    delay_ms(10);
    
    /* Display on */
    lv_st_spi_display_send_cmd(ILI9341_DISPON);
    delay_ms(120);
    
    LV_LOG_INFO("SPI display hardware initialized");
    return true;
}

void lv_st_spi_display_set_orientation(lv_display_t * disp, spi_display_orientation_t orientation)
{
    if (disp == NULL) return;
    
    spi_display_ctx_t * ctx = lv_display_get_driver_data(disp);
    if (ctx == NULL) return;
    
    ctx->orientation = orientation;
    
    /* Configure MADCTL register based on orientation */
    uint8_t madctl = MADCTL_BGR;  /* Assuming BGR order, change if needed */
    
    switch (orientation) {
        case SPI_DISPLAY_ORIENTATION_PORTRAIT:
            madctl |= MADCTL_MX;
            lv_display_set_resolution(disp, ctx->width, ctx->height);
            break;
            
        case SPI_DISPLAY_ORIENTATION_LANDSCAPE:
            madctl |= MADCTL_MV;
            lv_display_set_resolution(disp, ctx->height, ctx->width);
            break;
            
        case SPI_DISPLAY_ORIENTATION_PORTRAIT_INV:
            madctl |= MADCTL_MY;
            lv_display_set_resolution(disp, ctx->width, ctx->height);
            break;
            
        case SPI_DISPLAY_ORIENTATION_LANDSCAPE_INV:
            madctl |= MADCTL_MV | MADCTL_MY | MADCTL_MX;
            lv_display_set_resolution(disp, ctx->height, ctx->width);
            break;
    }
    
    lv_st_spi_display_send_cmd(ILI9341_MADCTL);
    lv_st_spi_display_send_data(&madctl, 1);
}

void lv_st_spi_display_send_cmd(uint8_t cmd)
{
    LCD_DC_LOW();   /* Command mode */
    LCD_CS_LOW();
    HAL_SPI_Transmit(&SPI_DISPLAY_SPI_HANDLE, &cmd, 1, HAL_MAX_DELAY);
    LCD_CS_HIGH();
}

void lv_st_spi_display_send_data(const uint8_t * data, uint32_t len)
{
    LCD_DC_HIGH();  /* Data mode */
    LCD_CS_LOW();
    
#if SPI_DISPLAY_USE_DMA
    /* Use DMA for large transfers */
    if (len > 64) {
        HAL_SPI_Transmit_DMA(&SPI_DISPLAY_SPI_HANDLE, (uint8_t *)data, len);
        /* Wait for DMA completion */
        while (HAL_SPI_GetState(&SPI_DISPLAY_SPI_HANDLE) != HAL_SPI_STATE_READY);
    } else {
        HAL_SPI_Transmit(&SPI_DISPLAY_SPI_HANDLE, (uint8_t *)data, len, HAL_MAX_DELAY);
    }
#else
    HAL_SPI_Transmit(&SPI_DISPLAY_SPI_HANDLE, (uint8_t *)data, len, HAL_MAX_DELAY);
#endif
    
    LCD_CS_HIGH();
}

void lv_st_spi_display_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    /* Column Address Set */
    lv_st_spi_display_send_cmd(ILI9341_CASET);
    uint8_t data[4];
    data[0] = (x1 >> 8) & 0xFF;
    data[1] = x1 & 0xFF;
    data[2] = (x2 >> 8) & 0xFF;
    data[3] = x2 & 0xFF;
    lv_st_spi_display_send_data(data, 4);
    
    /* Page Address Set */
    lv_st_spi_display_send_cmd(ILI9341_PASET);
    data[0] = (y1 >> 8) & 0xFF;
    data[1] = y1 & 0xFF;
    data[2] = (y2 >> 8) & 0xFF;
    data[3] = y2 & 0xFF;
    lv_st_spi_display_send_data(data, 4);
    
    /* Memory Write */
    lv_st_spi_display_send_cmd(ILI9341_RAMWR);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    /* Set window to the area being flushed */
    lv_st_spi_display_set_window(area->x1, area->y1, area->x2, area->y2);
    
    /* Calculate data size */
    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area) * 2;  /* RGB565 */
    
    /* Send pixel data */
    lv_st_spi_display_send_data(px_map, size);
    
    /* Signal LVGL that flushing is complete */
    lv_display_flush_ready(disp);
}

static void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

#endif /* LV_USE_ST_SPI_DISPLAY */
