/*
 * LCDController.h — LVGL display port adapter for ILI9341
 */

#ifndef INC_LCDCONTROLLER_H_
#define INC_LCDCONTROLLER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "ILI9341.h"

/* Initialize low level display driver */
void lv_port_disp_init(const ILI9341_Config_t *config);

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL */
void disp_enable_update(void);

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL */
void disp_disable_update(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /* INC_LCDCONTROLLER_H_ */
