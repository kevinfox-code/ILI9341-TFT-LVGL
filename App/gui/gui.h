#ifndef GUI_H_
#define GUI_H_

/* Builds the demo screen (greeting + clock labels) on the active LVGL
 * display. Call once during App_Init(), after the display/indev are
 * created. */
void gui_load(void);

/* Updates the clock labels. Caller must hold the LVGL lock (lv_lock()/
 * lv_unlock()) around this call — LVGL objects are not thread-safe. */
void gui_update_clock(const char *time_str, const char *date_str);

#endif /* GUI_H_ */
