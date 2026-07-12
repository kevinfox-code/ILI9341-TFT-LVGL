#include "gui.h"
#include "lvgl.h"
#include <stddef.h>

static lv_obj_t *s_time_label = NULL;
static lv_obj_t *s_date_label = NULL;

void gui_load(void)
{
    lv_obj_t *scr = lv_scr_act();

    lv_obj_t *label = lv_label_create(scr);
    lv_label_set_text(label, "Hello, LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -60);

    s_date_label = lv_label_create(scr);
    lv_label_set_text(s_date_label, "--/--/----");
    lv_obj_align(s_date_label, LV_ALIGN_CENTER, 0, -20);

    s_time_label = lv_label_create(scr);
    lv_label_set_text(s_time_label, "--:--:--");
    lv_obj_align(s_time_label, LV_ALIGN_CENTER, 0, 20);
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
