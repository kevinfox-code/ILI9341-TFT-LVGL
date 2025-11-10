/**
 * @file lv_nema_gfx.h
 * @brief STM32 NeoChrom GPU Driver for LVGL 9.4
 * 
 * This driver integrates the NeoChrom GPU (found on STM32U5 and similar) as an
 * LVGL draw unit for hardware-accelerated 2D graphics and vector graphics.
 * 
 * Features:
 * - Hardware-accelerated 2D rendering (fills, images, blending)
 * - Vector graphics support with NeoChrom VG
 * - SVG rendering capabilities
 * - TSC (ThinkSilicon Compression) image format support
 * - GPU tessellation and matrix operations (U5F/U5G cores)
 * 
 * Requirements:
 * - NeoChrom GPU peripheral
 * - Pre-compiled NeoChrom libraries (libs/nema_gfx)
 * - Proper linker configuration for NeoChrom libraries
 */

#ifndef LV_NEMA_GFX_H
#define LV_NEMA_GFX_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl.h"

#if LV_USE_NEMA_GFX

/* NeoChrom library includes */
#include "nema_core.h"
#include "nema_utils.h"
#include "nema_blender.h"

#if LV_USE_NEMA_VG
#include "nema_vg.h"
#include "nema_vg_context.h"
#include "nema_vg_paint.h"
#endif

/*********************
 *      DEFINES
 *********************/
#ifndef LV_NEMA_GFX_MAX_RESX
    #define LV_NEMA_GFX_MAX_RESX 800
#endif

#ifndef LV_NEMA_GFX_MAX_RESY
    #define LV_NEMA_GFX_MAX_RESY 480
#endif

/* HAL implementation selection */
#define LV_NEMA_HAL_CUSTOM        0
#define LV_NEMA_HAL_STM32_DEFAULT 1

#ifndef LV_USE_NEMA_HAL
    #define LV_USE_NEMA_HAL LV_NEMA_HAL_STM32_DEFAULT
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/

/**
 * Initialize NeoChrom draw unit
 * Call this once during application initialization after LVGL init
 */
void lv_nema_gfx_init(void);

/**
 * Deinitialize NeoChrom draw unit
 */
void lv_nema_gfx_deinit(void);

#if LV_USE_NEMA_VG
/**
 * Initialize NeoChrom VG (Vector Graphics) support
 * Call after lv_nema_gfx_init() if vector graphics are needed
 */
void lv_nema_vg_init(void);

/**
 * Deinitialize NeoChrom VG
 */
void lv_nema_vg_deinit(void);
#endif

/**********************
 *      MACROS
 **********************/

#endif /* LV_USE_NEMA_GFX */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV_NEMA_GFX_H */
