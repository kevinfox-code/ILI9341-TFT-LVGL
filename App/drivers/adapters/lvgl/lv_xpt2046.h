#ifndef LV_XPT2046_H_
#define LV_XPT2046_H_
#include "lvgl.h"
#include "drv/xpt2046.h"

#ifdef __cplusplus
extern "C" {
#endif

lv_indev_t *lv_xpt2046_create(xpt2046_t *touch, lv_display_t *disp);

/* Registers a waker invoked (ISR context) on PENIRQ pen-down, e.g. to notify
 * the GUI task to run lv_timer_handler() sooner. NULL clears it. */
void lv_xpt2046_set_wake_cb(void (*cb)(void *user), void *user);

#ifdef __cplusplus
}
#endif
#endif /* LV_XPT2046_H_ */
