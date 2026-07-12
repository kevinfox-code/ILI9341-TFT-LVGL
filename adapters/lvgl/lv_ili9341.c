#include "lv_ili9341.h"
#include "drv/drv_policy.h"
#include <string.h>

#ifndef LV_ILI9341_MAX_INSTANCES
#define LV_ILI9341_MAX_INSTANCES 1
#endif

typedef struct {
    bool in_use;
    ili9341_t *ili;
    /* Set right before ILI9341_FlushAsync, cleared atomically either by the
     * done_cb (normal completion) or by the flush_cb itself when it decides
     * to cancel a stalled transfer — whichever gets there first wins, and
     * the other becomes a no-op. This is what prevents a stalled transfer's
     * late done_cb from calling lv_display_flush_ready() a second time. */
    lv_display_t *volatile pending;
} lv_ili9341_adapter_t;

static lv_ili9341_adapter_t s_adapters[LV_ILI9341_MAX_INSTANCES];

static lv_display_t *claim_and_clear_pending(lv_ili9341_adapter_t *a)
{
    lv_display_t *d;
    __asm volatile ("cpsid i" ::: "memory");
    d = a->pending;
    a->pending = NULL;
    __asm volatile ("cpsie i" ::: "memory");
    return d;
}

static void flush_done_cb(void *user)
{
    lv_ili9341_adapter_t *a = (lv_ili9341_adapter_t *)user;
    lv_display_t *d = claim_and_clear_pending(a);
    if (d != NULL) {
        lv_display_flush_ready(d);
    }
}

static void disp_flush(lv_display_t *lv_disp, const lv_area_t *area, uint8_t *px_map)
{
    lv_ili9341_adapter_t *a = (lv_ili9341_adapter_t *)lv_display_get_user_data(lv_disp);

#if DRV_DROP_FRAME_IF_BUSY
    if (ILI9341_FlushBusy(a->ili)) {
        lv_display_flush_ready(lv_disp);
        return;
    }
#else
    if (ILI9341_WaitFlushDone(a->ili, DRV_DMA_BUSY_WAIT_TIMEOUT_MS) != DRV_OK) {
        claim_and_clear_pending(a);
        lv_display_flush_ready(lv_disp);
        return;
    }
#endif

    uint32_t w = (uint32_t)(area->x2 - area->x1 + 1);
    uint32_t h = (uint32_t)(area->y2 - area->y1 + 1);
    uint32_t len_bytes = w * h * 2u;

    a->pending = lv_disp;
    drv_status_t st = ILI9341_FlushAsync(a->ili,
                                          (uint16_t)area->x1, (uint16_t)area->y1,
                                          (uint16_t)area->x2, (uint16_t)area->y2,
                                          px_map, len_bytes, flush_done_cb, a);
    if (st != DRV_OK) {
        claim_and_clear_pending(a);
        lv_display_flush_ready(lv_disp);
    }
}

lv_display_t *lv_ili9341_create(ili9341_t *disp, uint8_t *buf1, uint8_t *buf2, uint32_t buf_bytes)
{
    if (disp == NULL || buf1 == NULL || buf_bytes == 0u) {
        return NULL;
    }

    lv_ili9341_adapter_t *a = NULL;
    for (int i = 0; i < LV_ILI9341_MAX_INSTANCES; i++) {
        if (!s_adapters[i].in_use) {
            a = &s_adapters[i];
            break;
        }
    }
    if (a == NULL) {
        return NULL;
    }

    uint16_t hor_res = 0, ver_res = 0;
    ILI9341_GetResolution(disp, &hor_res, &ver_res);

    lv_display_t *lv_disp = lv_display_create(hor_res, ver_res);
    if (lv_disp == NULL) {
        return NULL;
    }

    memset(a, 0, sizeof(*a));
    a->in_use = true;
    a->ili = disp;

    lv_display_set_user_data(lv_disp, a);
    lv_display_set_flush_cb(lv_disp, disp_flush);
    lv_display_set_buffers(lv_disp, buf1, buf2, buf_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    return lv_disp;
}
