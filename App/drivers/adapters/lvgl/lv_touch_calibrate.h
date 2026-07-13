#ifndef LV_TOUCH_CALIBRATE_H_
#define LV_TOUCH_CALIBRATE_H_
#include "lvgl.h"
#include "drv/xpt2046.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint16_t raw_x_min, raw_x_max, raw_y_min, raw_y_max;
} lv_xpt2046_cal_result_t;

typedef void (*lv_xpt2046_log_cb_t)(const char *fmt, ...);

/* Runs a 4-point on-screen calibration on the currently active LVGL screen's
 * display. Applies the result via XPT2046_SetCalibration and, if
 * out_result is non-NULL, also returns it so the app can persist it.
 * log_cb may be NULL for silent operation. Must be called from the GUI
 * task/thread (not from an ISR). Returns false if calibration could not
 * complete (e.g. NULL args). */
bool lv_xpt2046_calibrate(xpt2046_t *touch, lv_display_t *disp,
                          lv_xpt2046_log_cb_t log_cb,
                          lv_xpt2046_cal_result_t *out_result);

#ifdef __cplusplus
}
#endif
#endif /* LV_TOUCH_CALIBRATE_H_ */
