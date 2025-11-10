/**
 * @file lv_st_ltdc.c
 * @brief STM32 LTDC Display Driver Implementation for LVGL 9.4
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_st_ltdc.h"

#if LV_USE_ST_LTDC

#include <string.h>

/*********************
 *      DEFINES
 *********************/
#ifndef LV_ST_LTDC_USE_DMA2D_FLUSH
    #define LV_ST_LTDC_USE_DMA2D_FLUSH 0
#endif

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    LTDC_HandleTypeDef * hltdc;
    uint32_t layer_index;
    void * fb_addr;
    void * fb_addr2;
    bool is_partial;
    uint32_t fb_width;
    uint32_t fb_height;
    lv_color_format_t cf;
#if LV_USE_OS
    lv_thread_sync_t sync;
#endif
} ltdc_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void flush_direct_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static void flush_partial_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
static lv_color_format_t ltdc_pf_to_lv_cf(uint32_t pf);
static uint8_t get_pixel_size(lv_color_format_t cf);

#if LV_ST_LTDC_USE_DMA2D_FLUSH
static void dma2d_flush(ltdc_ctx_t * ctx, const lv_area_t * area, uint8_t * px_map);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
extern LTDC_HandleTypeDef hltdc;  /* Assumes STM32CubeMX generated this */

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_display_t * lv_st_ltdc_create_direct(void * fb_addr, void * fb_addr2, uint32_t layer_index)
{
    /* Validate parameters */
    if (fb_addr == NULL || layer_index >= 2) {
        LV_LOG_ERROR("Invalid parameters: fb_addr=%p, layer_index=%lu", fb_addr, layer_index);
        return NULL;
    }

    /* Get LTDC layer configuration */
    LTDC_LayerCfgTypeDef * layer_cfg;
    if (layer_index == 0) {
        layer_cfg = &hltdc.LayerCfg[0];
    } else {
        layer_cfg = &hltdc.LayerCfg[1];
    }

    /* Extract display parameters from LTDC configuration */
    uint32_t width = layer_cfg->ImageWidth;
    uint32_t height = layer_cfg->ImageHeight;
    lv_color_format_t cf = ltdc_pf_to_lv_cf(layer_cfg->PixelFormat);

    if (cf == LV_COLOR_FORMAT_UNKNOWN) {
        LV_LOG_ERROR("Unsupported LTDC pixel format: 0x%lx", layer_cfg->PixelFormat);
        return NULL;
    }

    /* Create LVGL display */
    lv_display_t * disp = lv_display_create(width, height);
    if (disp == NULL) {
        LV_LOG_ERROR("Failed to create display");
        return NULL;
    }

    /* Allocate and initialize driver context */
    ltdc_ctx_t * ctx = lv_malloc(sizeof(ltdc_ctx_t));
    if (ctx == NULL) {
        LV_LOG_ERROR("Failed to allocate driver context");
        lv_display_delete(disp);
        return NULL;
    }

    memset(ctx, 0, sizeof(ltdc_ctx_t));
    ctx->hltdc = &hltdc;
    ctx->layer_index = layer_index;
    ctx->fb_addr = fb_addr;
    ctx->fb_addr2 = fb_addr2;
    ctx->is_partial = false;
    ctx->fb_width = width;
    ctx->fb_height = height;
    ctx->cf = cf;

    /* Set framebuffer address in LTDC if not already set */
    HAL_LTDC_SetAddress(&hltdc, (uint32_t)fb_addr, layer_index);

    /* Configure LVGL display */
    uint32_t fb_size = width * height * get_pixel_size(cf);
    lv_display_set_buffers(disp, fb_addr, fb_addr2, fb_size, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(disp, flush_direct_cb);
    lv_display_set_color_format(disp, cf);
    lv_display_set_driver_data(disp, ctx);

#if LV_USE_OS
    lv_thread_sync_init(&ctx->sync);
#endif

    LV_LOG_INFO("LTDC direct mode display created: %lux%lu, layer %lu", width, height, layer_index);
    return disp;
}

lv_display_t * lv_st_ltdc_create_partial(void * buf1, void * buf2, uint32_t buf_size, uint32_t layer_index)
{
    /* Validate parameters */
    if (buf1 == NULL || buf_size == 0 || layer_index >= 2) {
        LV_LOG_ERROR("Invalid parameters: buf1=%p, buf_size=%lu, layer_index=%lu", buf1, buf_size, layer_index);
        return NULL;
    }

    /* Get LTDC layer configuration */
    LTDC_LayerCfgTypeDef * layer_cfg;
    if (layer_index == 0) {
        layer_cfg = &hltdc.LayerCfg[0];
    } else {
        layer_cfg = &hltdc.LayerCfg[1];
    }

    /* Extract display parameters from LTDC configuration */
    uint32_t width = layer_cfg->ImageWidth;
    uint32_t height = layer_cfg->ImageHeight;
    lv_color_format_t cf = ltdc_pf_to_lv_cf(layer_cfg->PixelFormat);
    void * fb_addr = (void *)layer_cfg->FBStartAdress;

    if (cf == LV_COLOR_FORMAT_UNKNOWN) {
        LV_LOG_ERROR("Unsupported LTDC pixel format: 0x%lx", layer_cfg->PixelFormat);
        return NULL;
    }

    if (fb_addr == NULL) {
        LV_LOG_ERROR("LTDC framebuffer address not configured");
        return NULL;
    }

    /* Create LVGL display */
    lv_display_t * disp = lv_display_create(width, height);
    if (disp == NULL) {
        LV_LOG_ERROR("Failed to create display");
        return NULL;
    }

    /* Allocate and initialize driver context */
    ltdc_ctx_t * ctx = lv_malloc(sizeof(ltdc_ctx_t));
    if (ctx == NULL) {
        LV_LOG_ERROR("Failed to allocate driver context");
        lv_display_delete(disp);
        return NULL;
    }

    memset(ctx, 0, sizeof(ltdc_ctx_t));
    ctx->hltdc = &hltdc;
    ctx->layer_index = layer_index;
    ctx->fb_addr = fb_addr;
    ctx->fb_addr2 = NULL;
    ctx->is_partial = true;
    ctx->fb_width = width;
    ctx->fb_height = height;
    ctx->cf = cf;

    /* Configure LVGL display */
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_partial_cb);
    lv_display_set_color_format(disp, cf);
    lv_display_set_driver_data(disp, ctx);

#if LV_USE_OS
    lv_thread_sync_init(&ctx->sync);
#endif

    LV_LOG_INFO("LTDC partial mode display created: %lux%lu, layer %lu", width, height, layer_index);
    return disp;
}

void * lv_st_ltdc_get_layer_handle(lv_display_t * disp)
{
    if (disp == NULL) return NULL;
    ltdc_ctx_t * ctx = lv_display_get_driver_data(disp);
    if (ctx == NULL) return NULL;
    return &ctx->hltdc->LayerCfg[ctx->layer_index];
}

void lv_st_ltdc_set_fb_address(lv_display_t * disp, void * fb_addr)
{
    if (disp == NULL || fb_addr == NULL) return;
    ltdc_ctx_t * ctx = lv_display_get_driver_data(disp);
    if (ctx == NULL || ctx->is_partial) return;
    
    ctx->fb_addr = fb_addr;
    HAL_LTDC_SetAddress(ctx->hltdc, (uint32_t)fb_addr, ctx->layer_index);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void flush_direct_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    LV_UNUSED(area);
    LV_UNUSED(px_map);
    
    ltdc_ctx_t * ctx = lv_display_get_driver_data(disp);
    
    /* For direct mode, just wait for VSYNC if needed */
    /* The framebuffer is already being displayed by LTDC */
    
#if LV_USE_OS
    /* Optional: wait for VSYNC to avoid tearing */
    /* This would require LTDC interrupt configuration */
#endif

    /* Clean data cache if enabled */
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_CleanDCache();
#endif

    lv_display_flush_ready(disp);
}

static void flush_partial_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    ltdc_ctx_t * ctx = lv_display_get_driver_data(disp);
    
#if LV_ST_LTDC_USE_DMA2D_FLUSH
    /* Use DMA2D for parallel flushing */
    dma2d_flush(ctx, area, px_map);
#else
    /* Software memory copy */
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    uint8_t px_size = get_pixel_size(ctx->cf);
    
    uint8_t * fb_line = (uint8_t *)ctx->fb_addr + 
                        (area->y1 * ctx->fb_width + area->x1) * px_size;
    uint8_t * src_line = px_map;
    
    for (uint32_t y = 0; y < h; y++) {
        memcpy(fb_line, src_line, w * px_size);
        fb_line += ctx->fb_width * px_size;
        src_line += w * px_size;
    }
    
    /* Clean data cache if enabled */
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_CleanDCache();
#endif
    
    lv_display_flush_ready(disp);
#endif
}

#if LV_ST_LTDC_USE_DMA2D_FLUSH
static void dma2d_flush(ltdc_ctx_t * ctx, const lv_area_t * area, uint8_t * px_map)
{
    /* DMA2D-based flushing for improved performance */
    /* This requires DMA2D peripheral to be enabled and configured */
    /* Implementation would use DMA2D memory-to-memory transfer */
    
    /* For now, fall back to software copy */
    LV_LOG_WARN("DMA2D flush not yet implemented, using software copy");
    
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    uint8_t px_size = get_pixel_size(ctx->cf);
    
    uint8_t * fb_line = (uint8_t *)ctx->fb_addr + 
                        (area->y1 * ctx->fb_width + area->x1) * px_size;
    uint8_t * src_line = px_map;
    
    for (uint32_t y = 0; y < h; y++) {
        memcpy(fb_line, src_line, w * px_size);
        fb_line += ctx->fb_width * px_size;
        src_line += w * px_size;
    }
    
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
    SCB_CleanDCache();
#endif
    
    lv_display_flush_ready(lv_display_get_default());
}
#endif

static lv_color_format_t ltdc_pf_to_lv_cf(uint32_t pf)
{
    switch (pf) {
        case LTDC_PIXEL_FORMAT_RGB565:
            return LV_COLOR_FORMAT_RGB565;
        case LTDC_PIXEL_FORMAT_RGB888:
            return LV_COLOR_FORMAT_RGB888;
        case LTDC_PIXEL_FORMAT_ARGB8888:
            return LV_COLOR_FORMAT_ARGB8888;
        case LTDC_PIXEL_FORMAT_ARGB1555:
            return LV_COLOR_FORMAT_ARGB1555;
        case LTDC_PIXEL_FORMAT_ARGB4444:
            return LV_COLOR_FORMAT_ARGB4444;
        default:
            return LV_COLOR_FORMAT_UNKNOWN;
    }
}

static uint8_t get_pixel_size(lv_color_format_t cf)
{
    switch (cf) {
        case LV_COLOR_FORMAT_RGB565:
        case LV_COLOR_FORMAT_ARGB1555:
        case LV_COLOR_FORMAT_ARGB4444:
            return 2;
        case LV_COLOR_FORMAT_RGB888:
            return 3;
        case LV_COLOR_FORMAT_ARGB8888:
            return 4;
        default:
            return 0;
    }
}

#endif /* LV_USE_ST_LTDC */
