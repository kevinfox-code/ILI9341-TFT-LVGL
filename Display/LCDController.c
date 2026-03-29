/*
 * LCDController.c — LVGL display port adapter for ILI9341
 */

#include "LCDController.h"
#include <stdbool.h>
#include "main.h"
#include "board_config.h"
#include "board_drivers.h"
#include "display_runtime_config.h"
#include "ILI9341.h"
#include "lvgl.h"

#ifdef FREERTOS
#include "cmsis_os2.h"
#endif

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */

static lv_display_t *disp = NULL;
extern volatile bool ili_dma_busy;

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

void lv_port_disp_init(const ILI9341_Config_t *config)
{
    uint16_t hor_res = Board_GetDisplayHorRes();
    uint16_t ver_res = Board_GetDisplayVerRes();

    ILI9341_Init(config);
    ILI9341_SetRotation(Board_GetDisplayRotation());

    disp = lv_display_create(hor_res, ver_res);
    lv_display_set_flush_cb(disp, disp_flush);

    /* Two buffers for partial rendering — DMA transfers the active buffer while
     * LVGL renders into the other (ping-pong). Each buffer holds 10 rows. */
    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_1[BOARD_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];

    LV_ATTRIBUTE_MEM_ALIGN
    static uint8_t buf_2_2[BOARD_DISP_HOR_RES * 10 * BYTE_PER_PIXEL];

    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

static volatile bool disp_flush_enabled = true;

void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if(!disp_flush_enabled) {
        lv_display_flush_ready(disp_drv);
        return;
    }

    /* Wait for previous DMA transfer to finish if it's still running */
#if DISPLAY_DROP_FRAME_IF_DMA_BUSY
    if(ili_dma_busy) {
        lv_display_flush_ready(disp_drv);
        return;
    }
#else
    {
        uint32_t start = HAL_GetTick();
        while(ili_dma_busy) {
            if((HAL_GetTick() - start) >= DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS) {
                /* Suppress the ISR's flush_ready for the stalled transfer, then
                 * signal ready ourselves — one call total, no double-fire. */
                ILI9341_CancelFlushCallback();
                lv_display_flush_ready(disp_drv);
                return;
            }
#ifdef FREERTOS
            osDelay(DISPLAY_DMA_BUSY_WAIT_SLICE_MS);
#else
            HAL_Delay(DISPLAY_DMA_BUSY_WAIT_SLICE_MS);
#endif
        }
    }
#endif

    ili_dma_busy = true;

    ILI9341_SetWindow(area->x1, area->y1, area->x2, area->y2);

    int height = area->y2 - area->y1 + 1;
    int width  = area->x2 - area->x1 + 1;

    ILI9341_DrawBitmapDMA(width, height, px_map, disp_drv);
}
