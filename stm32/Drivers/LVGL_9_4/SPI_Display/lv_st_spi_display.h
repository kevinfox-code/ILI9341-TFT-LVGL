/**
 * @file lv_st_spi_display.h
 * @brief STM32 SPI Display Driver Template for LVGL 9.4
 * 
 * This is a template/example driver for SPI-based displays using STM32 HAL.
 * Demonstrates how to create a display driver for common SPI TFT displays.
 * 
 * Features:
 * - STM32 HAL SPI integration
 * - Support for RGB565 displays (easily adaptable)
 * - Direct and partial render modes
 * - DMA support for improved performance
 * - Command/Data pin control
 * - Chip select (CS) handling
 * 
 * Supported Display Types:
 * - ILI9341 (240x320)
 * - ST7735 (128x160, 80x160)
 * - ST7789 (240x240, 240x320)
 * - Any similar SPI RGB565 display
 */

#ifndef LV_ST_SPI_DISPLAY_H
#define LV_ST_SPI_DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

#if LV_USE_ST_SPI_DISPLAY

#include "main.h"  /* For STM32 HAL and GPIO definitions */

/*********************
 *      DEFINES
 *********************/

/* SPI Configuration - User should modify these */
#ifndef SPI_DISPLAY_SPI_HANDLE
    #define SPI_DISPLAY_SPI_HANDLE hspi1  /* Change to your SPI handle */
#endif

/* GPIO Pin Definitions - User should modify these to match hardware */
#ifndef LCD_CS_GPIO_Port
    #define LCD_CS_GPIO_Port GPIOA
#endif

#ifndef LCD_CS_Pin
    #define LCD_CS_Pin GPIO_PIN_4
#endif

#ifndef LCD_DC_GPIO_Port
    #define LCD_DC_GPIO_Port GPIOA
#endif

#ifndef LCD_DC_Pin
    #define LCD_DC_Pin GPIO_PIN_3
#endif

#ifndef LCD_RST_GPIO_Port
    #define LCD_RST_GPIO_Port GPIOA
#endif

#ifndef LCD_RST_Pin
    #define LCD_RST_Pin GPIO_PIN_2
#endif

/* Display dimensions */
#ifndef SPI_DISPLAY_WIDTH
    #define SPI_DISPLAY_WIDTH 240
#endif

#ifndef SPI_DISPLAY_HEIGHT
    #define SPI_DISPLAY_HEIGHT 320
#endif

/* DMA support */
#ifndef SPI_DISPLAY_USE_DMA
    #define SPI_DISPLAY_USE_DMA 0
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**
 * Display orientation enum
 */
typedef enum {
    SPI_DISPLAY_ORIENTATION_PORTRAIT = 0,
    SPI_DISPLAY_ORIENTATION_LANDSCAPE = 1,
    SPI_DISPLAY_ORIENTATION_PORTRAIT_INV = 2,
    SPI_DISPLAY_ORIENTATION_LANDSCAPE_INV = 3
} spi_display_orientation_t;

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an LVGL display for SPI TFT
 * @param width Display width in pixels
 * @param height Display height in pixels
 * @param use_direct_mode true for direct mode, false for partial mode
 * @return Pointer to created LVGL display, or NULL on error
 */
lv_display_t * lv_st_spi_display_create(uint16_t width, uint16_t height, bool use_direct_mode);

/**
 * Initialize the SPI display hardware
 * Sends initialization commands to the display controller
 * @return true on success, false on error
 */
bool lv_st_spi_display_hw_init(void);

/**
 * Set display orientation
 * @param disp LVGL display instance
 * @param orientation Display orientation
 */
void lv_st_spi_display_set_orientation(lv_display_t * disp, spi_display_orientation_t orientation);

/**
 * Send a command to the display
 * @param cmd Command byte
 */
void lv_st_spi_display_send_cmd(uint8_t cmd);

/**
 * Send data to the display
 * @param data Pointer to data buffer
 * @param len Length of data in bytes
 */
void lv_st_spi_display_send_data(const uint8_t * data, uint32_t len);

/**
 * Set display window (column and row addresses)
 * @param x1 Start X coordinate
 * @param y1 Start Y coordinate
 * @param x2 End X coordinate
 * @param y2 End Y coordinate
 */
void lv_st_spi_display_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**********************
 *      MACROS
 **********************/

/* Helper macros for GPIO control */
#define LCD_CS_LOW()   HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET)
#define LCD_CS_HIGH()  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET)
#define LCD_DC_LOW()   HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET)
#define LCD_DC_HIGH()  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET)
#define LCD_RST_LOW()  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_RESET)
#define LCD_RST_HIGH() HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET)

#endif /* LV_USE_ST_SPI_DISPLAY */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_ST_SPI_DISPLAY_H */
