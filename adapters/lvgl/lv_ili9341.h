#ifndef LV_ILI9341_H_
#define LV_ILI9341_H_
#include "lvgl.h"
#include "drv/ili9341.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Caller owns buf1/buf2 (sized hor_res * N_rows * 2, LV_ATTRIBUTE_MEM_ALIGN).
 * buf2 may be NULL for single-buffered rendering. */
lv_display_t *lv_ili9341_create(ili9341_t *disp, uint8_t *buf1, uint8_t *buf2, uint32_t buf_bytes);

#ifdef __cplusplus
}
#endif
#endif /* LV_ILI9341_H_ */
