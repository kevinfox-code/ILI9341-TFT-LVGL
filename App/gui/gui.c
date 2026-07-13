#include "gui.h"
#include "lvgl.h"
#include "drv/xpt2046.h"
#include <stddef.h>

static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_date_label = NULL;
static lv_obj_t *s_btn_label = NULL;
static lv_obj_t *s_touch_readout = NULL;
static lv_obj_t *s_touch_marker = NULL;
static uint32_t s_tap_count = 0;

static void btn_clicked_cb(lv_event_t *e)
{
    (void)e;
    s_tap_count++;
    lv_label_set_text_fmt(s_btn_label, "Taps: %" LV_PRIu32, s_tap_count);
}

/* Mirrors g_xpt2046_debug onto the screen so touch behavior and raw
 * calibration values can be read off the panel without a debugger. */
static void touch_dbg_timer_cb(lv_timer_t *tm)
{
    (void)tm;
    lv_label_set_text_fmt(s_touch_readout, "raw %u,%u  px %u,%u %s",
                          (unsigned)g_xpt2046_debug.raw_x,
                          (unsigned)g_xpt2046_debug.raw_y,
                          (unsigned)g_xpt2046_debug.px,
                          (unsigned)g_xpt2046_debug.py,
                          g_xpt2046_debug.pen_down ? "DOWN" : "");
    if (g_xpt2046_debug.pen_down) {
        lv_obj_set_pos(s_touch_marker,
                       (int32_t)g_xpt2046_debug.px - 6,
                       (int32_t)g_xpt2046_debug.py - 6);
        lv_obj_remove_flag(s_touch_marker, LV_OBJ_FLAG_HIDDEN);
    }
}

void gui_load(void)
{
    lv_obj_t *scr = lv_scr_act();

    /* Explicit colors everywhere: the panel washed out the default theme,
     * so don't depend on it for contrast. */
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    /* Erratic touch coords register as drags; never let them scroll the UI
     * out of view. */
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello, LVGL!");
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -60);

    s_date_label = lv_label_create(scr);
    lv_label_set_text(s_date_label, "--/--/----");
    lv_obj_set_style_text_color(s_date_label, lv_color_white(), 0);
    lv_obj_align(s_date_label, LV_ALIGN_CENTER, 0, -20);

    s_time_label = lv_label_create(scr);
    lv_label_set_text(s_time_label, "--:--:--");
    lv_obj_set_style_text_color(s_time_label, lv_color_white(), 0);
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, 20);

    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 140, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 70);
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(btn, lv_color_white(), 0);
    lv_obj_set_style_border_width(btn, 2, 0);
    lv_obj_add_event_cb(btn, btn_clicked_cb, LV_EVENT_CLICKED, NULL);

    s_btn_label = lv_label_create(btn);
    lv_label_set_text(s_btn_label, "Taps: 0");
    lv_obj_set_style_text_color(s_btn_label, lv_color_white(), 0);
    lv_obj_center(s_btn_label);

    /* Touch debug overlay: live raw/mapped coordinates top-left, red dot at
     * the mapped touch point. */
    s_touch_readout = lv_label_create(scr);
    lv_label_set_text(s_touch_readout, "raw -,-  px -,-");
    lv_obj_set_style_text_color(s_touch_readout, lv_palette_main(LV_PALETTE_YELLOW), 0);
    lv_obj_align(s_touch_readout, LV_ALIGN_TOP_LEFT, 4, 4);

    s_touch_marker = lv_obj_create(scr);
    lv_obj_set_size(s_touch_marker, 12, 12);
    lv_obj_set_style_radius(s_touch_marker, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(s_touch_marker, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_set_style_bg_opa(s_touch_marker, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_touch_marker, 0, 0);
    /* Must not swallow touches or scroll the screen. */
    lv_obj_remove_flag(s_touch_marker, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(s_touch_marker, LV_OBJ_FLAG_IGNORE_LAYOUT);
    lv_obj_add_flag(s_touch_marker, LV_OBJ_FLAG_HIDDEN);

    lv_timer_create(touch_dbg_timer_cb, 100, NULL);
}

void gui_update_clock(const char *time_str, const char *date_str)
{
    if (s_time_label != NULL && time_str != NULL) {
        lv_label_set_text(s_time_label, time_str);
    }
    if (s_date_label != NULL && date_str != NULL) {
        lv_label_set_text(s_date_label, date_str);
    }
}
