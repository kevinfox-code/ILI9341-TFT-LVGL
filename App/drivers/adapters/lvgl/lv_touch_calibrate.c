#include "lv_touch_calibrate.h"
#include "drv/drv_os.h"

static void log_fmt(lv_xpt2046_log_cb_t log_cb, const char *msg)
{
    if (log_cb != NULL) {
        log_cb("%s", msg);
    }
}

static void draw_cross(lv_obj_t *scr, lv_coord_t x, lv_coord_t y)
{
    static const lv_point_precise_t ph[] = { { -10, 0 }, { 10, 0 } };
    lv_obj_t *h = lv_line_create(scr);
    lv_line_set_points(h, ph, 2);
    lv_obj_set_style_line_color(h, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(h, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(h, x, y);

    static const lv_point_precise_t pv[] = { { 0, -10 }, { 0, 10 } };
    lv_obj_t *v = lv_line_create(scr);
    lv_line_set_points(v, pv, 2);
    lv_obj_set_style_line_color(v, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(v, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_pos(v, x, y);
}

bool lv_xpt2046_calibrate(xpt2046_t *touch, lv_display_t *disp,
                          lv_xpt2046_log_cb_t log_cb,
                          lv_xpt2046_cal_result_t *out_result)
{
    if (touch == NULL || disp == NULL) {
        return false;
    }

    lv_obj_t *cal = lv_obj_create(NULL);
    lv_disp_load_scr(cal);
    lv_obj_clear_flag(cal, LV_OBJ_FLAG_SCROLLABLE);

    lv_coord_t w = (lv_coord_t)lv_display_get_horizontal_resolution(disp);
    lv_coord_t h = (lv_coord_t)lv_display_get_vertical_resolution(disp);

    const lv_point_t corners[4] = {
        { 20, 20 },
        { (lv_coord_t)(w - 21), 20 },
        { (lv_coord_t)(w - 21), (lv_coord_t)(h - 21) },
        { 20, (lv_coord_t)(h - 21) },
    };

    uint16_t raw_x[4], raw_y[4];
    for (int i = 0; i < 4; i++) {
        log_fmt(log_cb, "Starting calibration point");

        lv_obj_clean(cal);
        draw_cross(cal, corners[i].x, corners[i].y);

        lv_obj_t *lbl = lv_label_create(cal);
        lv_label_set_text(lbl, "Touch the green +");
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 40);

        lv_timer_handler();
        lv_refr_now(NULL);
        drv_delay_ms(100);

        uint16_t rx, ry;
        do {
            drv_delay_ms(10);
        } while (!XPT2046_ReadRaw(touch, &rx, &ry));

        raw_x[i] = rx;
        raw_y[i] = ry;
        log_fmt(log_cb, "Calibration point captured");

        do {
            drv_delay_ms(10);
        } while (XPT2046_ReadRaw(touch, &rx, &ry));

        drv_delay_ms(100);
    }

    lv_xpt2046_cal_result_t result = {
        .raw_x_min = raw_x[0], .raw_x_max = raw_x[0],
        .raw_y_min = raw_y[0], .raw_y_max = raw_y[0],
    };
    for (int i = 1; i < 4; i++) {
        if (raw_x[i] < result.raw_x_min) result.raw_x_min = raw_x[i];
        if (raw_x[i] > result.raw_x_max) result.raw_x_max = raw_x[i];
        if (raw_y[i] < result.raw_y_min) result.raw_y_min = raw_y[i];
        if (raw_y[i] > result.raw_y_max) result.raw_y_max = raw_y[i];
    }

    XPT2046_SetCalibration(touch, result.raw_x_min, result.raw_x_max,
                           result.raw_y_min, result.raw_y_max);
    if (out_result != NULL) {
        *out_result = result;
    }

    log_fmt(log_cb, "Calibration complete");

    lv_obj_t *blank = lv_obj_create(NULL);
    lv_scr_load(blank);
    lv_obj_del(cal);
    lv_timer_handler();
    lv_refr_now(NULL);

    return true;
}
