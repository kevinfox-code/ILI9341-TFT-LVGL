/**
 * @file TouchController.h
 * @brief Calibration and input driver helpers.
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
 * This will block, prompting the user to tap the four crosses in sequence.
 * After it returns, the touch → pixel mapping is updated.
 * MUST be called from the LVGL task or before vTaskStartScheduler().
 */
void touch_calibrate(void);

/**
 * Poll the touch controller and update cached state.
 * Call from the LVGL task loop (no SPI in the LVGL read callback).
 */
void TouchController_Poll(void);

#ifdef __cplusplus
}
#endif

#endif /* TOUCH_CONTROLLER_H_ */
