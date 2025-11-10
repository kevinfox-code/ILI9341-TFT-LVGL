/**
 * @file lv_st_ltdc.h
 * @brief STM32 LTDC Display Driver for LVGL 9.4
 * 
 * This driver provides support for STM32 LTDC (LCD-TFT Display Controller) peripheral.
 * Supports both direct and partial render modes with optional DMA2D acceleration.
 * 
 * Features:
 * - Single or double buffered direct mode
 * - Partial render mode with DMA2D flush support
 * - Software rotation (90°, 180°, 270°)
 * - OS synchronization support
 * - Multi-layer support
 */

#ifndef LV_ST_LTDC_H
#define LV_ST_LTDC_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

#if LV_USE_ST_LTDC

#include "main.h"  /* For STM32 HAL definitions */

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Create an LVGL display using LTDC in direct render mode
 * @param fb_addr Primary framebuffer address (must match LTDC layer configuration)
 * @param fb_addr2 Secondary framebuffer for double buffering (NULL for single buffer)
 * @param layer_index LTDC layer index (typically 0 or 1)
 * @return Pointer to created LVGL display, or NULL on error
 * 
 * @note For best visual results with CPU data cache, use double buffering
 * @note Single buffer mode may have artifacts on devices with CPU data cache
 */
lv_display_t * lv_st_ltdc_create_direct(void * fb_addr, void * fb_addr2, uint32_t layer_index);

/**
 * Create an LVGL display using LTDC in partial render mode
 * @param buf1 Primary partial buffer
 * @param buf2 Secondary partial buffer (NULL for single buffer, recommended for DMA2D)
 * @param buf_size Size of each partial buffer in bytes
 * @param layer_index LTDC layer index (typically 0 or 1)
 * @return Pointer to created LVGL display, or NULL on error
 * 
 * @note Partial mode uses less RAM than direct mode
 * @note Two buffers improve performance when LV_ST_LTDC_USE_DMA2D_FLUSH is enabled
 * @note Supports software rotation (direct mode does not)
 */
lv_display_t * lv_st_ltdc_create_partial(void * buf1, void * buf2, uint32_t buf_size, uint32_t layer_index);

/**
 * Get the LTDC layer handle associated with a display
 * @param disp LVGL display instance
 * @return Pointer to LTDC_LayerCfgTypeDef structure
 */
void * lv_st_ltdc_get_layer_handle(lv_display_t * disp);

/**
 * Set LTDC framebuffer address at runtime
 * @param disp LVGL display instance
 * @param fb_addr New framebuffer address
 * 
 * @note Only valid for displays created with lv_st_ltdc_create_direct()
 */
void lv_st_ltdc_set_fb_address(lv_display_t * disp, void * fb_addr);

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_ST_LTDC */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_ST_LTDC_H */
