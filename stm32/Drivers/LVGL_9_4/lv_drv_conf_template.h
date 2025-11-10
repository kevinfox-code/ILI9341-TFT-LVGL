/**
 * @file lv_drv_conf_template.h
 * @brief LVGL 9.4 STM32 Driver Configuration Template
 * 
 * Copy this file as lv_drv_conf.h to your project and modify the settings
 * to enable/disable drivers and configure their behavior.
 * 
 * This configuration file works alongside lv_conf.h for LVGL core settings.
 */

#ifndef LV_DRV_CONF_H
#define LV_DRV_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 * GENERAL SETTINGS
 *********************/

/* STM32 HAL header file to include */
/* Examples: "stm32f4xx_hal.h", "stm32u5xx_hal.h", "stm32h7xx_hal.h" */
#define LV_STM32_HAL_INCLUDE "stm32f4xx_hal.h"

/*********************
 * LTDC DRIVER
 *********************/

/* Enable LTDC (LCD-TFT Display Controller) driver */
#define LV_USE_ST_LTDC 0

#if LV_USE_ST_LTDC
    /* Enable DMA2D for flushing in partial mode
     * Note: Cannot be used together with LV_USE_DRAW_DMA2D */
    #define LV_ST_LTDC_USE_DMA2D_FLUSH 0
#endif

/*********************
 * DMA2D DRAW UNIT
 *********************/

/* Enable DMA2D (Chrom-ART) as an LVGL draw unit
 * Note: Cannot be used together with LV_ST_LTDC_USE_DMA2D_FLUSH */
#define LV_USE_DRAW_DMA2D 0

#if LV_USE_DRAW_DMA2D
    /* STM32 HAL header for DMA2D definitions */
    #define LV_DRAW_DMA2D_HAL_INCLUDE LV_STM32_HAL_INCLUDE
    
    /* Enable interrupt-driven async DMA2D operation
     * Requires LV_USE_OS to be enabled in lv_conf.h */
    #define LV_USE_DRAW_DMA2D_INTERRUPT 0
#endif

/*********************
 * NEOCHROM GPU
 *********************/

/* Enable NeoChrom GPU support (STM32U5 and similar) */
#define LV_USE_NEMA_GFX 0

#if LV_USE_NEMA_GFX
    /* Enable NeoChrom VG for vector graphics */
    #define LV_USE_NEMA_VG 0
    
    #if LV_USE_NEMA_VG
        /* Maximum display resolution for VG memory allocation */
        #define LV_NEMA_GFX_MAX_RESX 800
        #define LV_NEMA_GFX_MAX_RESY 480
    #endif
    
    /* HAL implementation selection */
    #define LV_NEMA_HAL_CUSTOM        0
    #define LV_NEMA_HAL_STM32_DEFAULT 1
    
    #define LV_USE_NEMA_HAL LV_NEMA_HAL_STM32_DEFAULT
    
    /* NeoChrom library path (relative to project root) */
    /* User must add this to include paths and link the library */
    /* Example: "Middlewares/LVGL/lvgl/libs/nema_gfx/include" */
#endif

/*********************
 * SPI DISPLAY DRIVER
 *********************/

/* Enable SPI display driver template */
#define LV_USE_ST_SPI_DISPLAY 0

#if LV_USE_ST_SPI_DISPLAY
    /* SPI handle name (must match your STM32CubeMX configuration) */
    #define SPI_DISPLAY_SPI_HANDLE hspi1
    
    /* GPIO Pin Definitions - modify to match your hardware */
    /* Chip Select (CS) */
    #define LCD_CS_GPIO_Port GPIOA
    #define LCD_CS_Pin GPIO_PIN_4
    
    /* Data/Command (DC or RS) */
    #define LCD_DC_GPIO_Port GPIOA
    #define LCD_DC_Pin GPIO_PIN_3
    
    /* Reset (RST) */
    #define LCD_RST_GPIO_Port GPIOA
    #define LCD_RST_Pin GPIO_PIN_2
    
    /* Display dimensions */
    #define SPI_DISPLAY_WIDTH  240
    #define SPI_DISPLAY_HEIGHT 320
    
    /* Enable DMA for SPI transfers (improves performance) */
    #define SPI_DISPLAY_USE_DMA 0
#endif

/*********************
 * TOUCH CONTROLLER
 *********************/

/* Enable XPT2046 touch controller (SPI-based resistive touch) */
#define LV_USE_XPT2046 0

#if LV_USE_XPT2046
    /* SPI handle for touch controller */
    #define TOUCH_SPI_HANDLE hspi2
    
    /* Touch CS pin */
    #define TOUCH_CS_GPIO_Port GPIOB
    #define TOUCH_CS_Pin GPIO_PIN_12
    
    /* Touch IRQ pin (optional) */
    #define TOUCH_IRQ_GPIO_Port GPIOB
    #define TOUCH_IRQ_Pin GPIO_PIN_10
    
    /* Touch calibration values (adjust for your display) */
    #define XPT2046_X_MIN 200
    #define XPT2046_X_MAX 3800
    #define XPT2046_Y_MIN 200
    #define XPT2046_Y_MAX 3800
    
    /* Invert axes if needed */
    #define XPT2046_INV_X 0
    #define XPT2046_INV_Y 0
    
    /* Swap X and Y if needed */
    #define XPT2046_XY_SWAP 0
#endif

/*********************
 * I2C TOUCH CONTROLLER
 *********************/

/* Enable GT911 capacitive touch controller */
#define LV_USE_GT911 0

#if LV_USE_GT911
    /* I2C handle */
    #define TOUCH_I2C_HANDLE hi2c1
    
    /* GT911 I2C address (typically 0xBA or 0x28) */
    #define GT911_I2C_ADDR 0xBA
    
    /* Touch interrupt pin (optional) */
    #define GT911_INT_GPIO_Port GPIOB
    #define GT911_INT_Pin GPIO_PIN_5
    
    /* Touch reset pin (optional) */
    #define GT911_RST_GPIO_Port GPIOB
    #define GT911_RST_Pin GPIO_PIN_4
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_DRV_CONF_H */
