/**
 * @file TouchController.h
 * @brief LVGL input driver and calibration helpers for XPT2046.
 */
#ifndef TOUCH_CONTROLLER_H_
#define TOUCH_CONTROLLER_H_

#include "lvgl.h"
#include "XPT2046.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize the XPT2046 driver and register it as an LVGL pointer device.
 * Call this once, before you call touch_calibrate() or start your main loop.
 */
void TouchController_Init(const XPT2046_Config_t *config);

/**
 * Run a 4-point on-screen calibration.
 * Blocks while prompting the user to tap four corner crosses in sequence.
 * Updates the touch-to-pixel mapping when done.
 * MUST be called from the LVGL task or before vTaskStartScheduler().
 */
void touch_calibrate(void);

/**
 * Poll the touch controller and update cached state.
 * Call from the LVGL task loop (keeps SPI out of the LVGL read callback).
 */
void TouchController_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* TOUCH_CONTROLLER_H_ */
