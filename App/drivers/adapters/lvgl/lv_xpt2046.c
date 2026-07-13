#include "lv_xpt2046.h"

typedef struct {
    xpt2046_t *touch;
    lv_display_t *disp;
} lv_xpt2046_ctx_t;

static lv_xpt2046_ctx_t s_ctx;
static void (*s_wake_cb)(void *user);
static void *s_wake_user;

static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    lv_xpt2046_ctx_t *ctx = (lv_xpt2046_ctx_t *)lv_indev_get_user_data(indev);

    uint32_t hor_res = lv_display_get_horizontal_resolution(ctx->disp);
    uint32_t ver_res = lv_display_get_vertical_resolution(ctx->disp);
    uint8_t rotation = (uint8_t)lv_display_get_rotation(ctx->disp);

    uint16_t px = 0, py = 0;
    if (XPT2046_ReadPoint(ctx->touch, &px, &py, (uint16_t)hor_res, (uint16_t)ver_res, rotation)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = (lv_coord_t)px;
        data->point.y = (lv_coord_t)py;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static void pen_down_isr_cb(void *user)
{
    (void)user;
    if (s_wake_cb != NULL) {
        s_wake_cb(s_wake_user);
    }
}

lv_indev_t *lv_xpt2046_create(xpt2046_t *touch, lv_display_t *disp)
{
    if (touch == NULL || disp == NULL) {
        return NULL;
    }

    s_ctx.touch = touch;
    s_ctx.disp = disp;

    lv_indev_t *indev = lv_indev_create();
    if (indev == NULL) {
        return NULL;
    }
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, disp);
    lv_indev_set_user_data(indev, &s_ctx);
    lv_indev_set_read_cb(indev, read_cb);

    XPT2046_SetPenDownCallback(touch, pen_down_isr_cb, NULL);

    return indev;
}

void lv_xpt2046_set_wake_cb(void (*cb)(void *user), void *user)
{
    s_wake_cb = cb;
    s_wake_user = user;
}
