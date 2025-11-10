/**
 * @file lv_draw_dma2d.h
 * @brief STM32 DMA2D (Chrom-ART) Draw Unit for LVGL 9.4
 * 
 * This driver integrates STM32 DMA2D peripheral as an LVGL draw unit for
 * hardware-accelerated pixel operations including fills and image blits.
 * 
 * Features:
 * - Hardware-accelerated fill operations
 * - Image blitting with color format conversion
 * - Alpha blending support
 * - Parallel operation with software renderer
 * - Optional interrupt-driven async operation with OS support
 */

#ifndef LV_DRAW_DMA2D_H
#define LV_DRAW_DMA2D_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

#if LV_USE_DRAW_DMA2D

/*********************
 *      DEFINES
 *********************/
#ifndef LV_DRAW_DMA2D_HAL_INCLUDE
    #define LV_DRAW_DMA2D_HAL_INCLUDE "stm32f4xx_hal.h"  /* Change per your STM32 family */
#endif

#ifndef LV_USE_DRAW_DMA2D_INTERRUPT
    #define LV_USE_DRAW_DMA2D_INTERRUPT 0
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize DMA2D draw unit
 * Call this once during application initialization after LVGL init
 */
void lv_draw_dma2d_init(void);

/**
 * Deinitialize DMA2D draw unit
 */
void lv_draw_dma2d_deinit(void);

#if LV_USE_DRAW_DMA2D_INTERRUPT
/**
 * DMA2D transfer complete interrupt handler
 * Must be called from the DMA2D global interrupt handler
 * 
 * Example:
 * void DMA2D_IRQHandler(void)
 * {
 *     lv_draw_dma2d_transfer_complete_interrupt_handler();
 * }
 */
void lv_draw_dma2d_transfer_complete_interrupt_handler(void);
#endif

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_DRAW_DMA2D */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_DRAW_DMA2D_H */
