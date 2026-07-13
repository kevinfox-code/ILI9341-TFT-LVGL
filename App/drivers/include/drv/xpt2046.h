#ifndef DRV_XPT2046_H_
#define DRV_XPT2046_H_
#include <stdint.h>
#include <stdbool.h>
#include "drv/drv_os.h"
#include "drv/drv_spi.h"
#include "drv/drv_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DRV_TOUCH_BUS_TIMEOUT_MS
#define DRV_TOUCH_BUS_TIMEOUT_MS 10u
#endif

typedef struct {
    drv_spi_bus_t *bus;
    drv_gpio_t cs;
    drv_gpio_t irq;              /* irq.port == NULL -> no PENIRQ wired */
    uint16_t raw_x_min, raw_x_max, raw_y_min, raw_y_max;
} xpt2046_config_t;

typedef struct xpt2046 xpt2046_t;

/* Debug snapshot of the most recent touch activity, updated on every
 * XPT2046_ReadPoint() call. Not part of the driver logic — exists so a
 * debugger live watch (e.g. Cortex-Debug "Live Watch") can monitor touch
 * behavior without halting the target. Watch `g_xpt2046_debug`. */
typedef struct {
    uint16_t raw_x, raw_y;     /* last raw ADC pair (valid while pen down)   */
    uint16_t px, py;           /* last mapped screen coordinates             */
    uint8_t  pen_down;         /* 1 while the panel reports a press          */
    uint32_t read_count;       /* total ReadPoint polls                      */
    uint32_t press_count;      /* polls that returned a valid press          */
} xpt2046_debug_t;

extern volatile xpt2046_debug_t g_xpt2046_debug;

drv_status_t XPT2046_Init(xpt2046_t **out, const xpt2046_config_t *cfg);
bool         XPT2046_PenDown(xpt2046_t *t);                 /* reads PENIRQ GPIO */
bool         XPT2046_ReadRaw(xpt2046_t *t, uint16_t *rx, uint16_t *ry);
bool         XPT2046_ReadPoint(xpt2046_t *t, uint16_t *px, uint16_t *py,
                               uint16_t hor_res, uint16_t ver_res, uint8_t rotation);
void         XPT2046_SetCalibration(xpt2046_t *t, uint16_t xmin, uint16_t xmax,
                                    uint16_t ymin, uint16_t ymax);   /* validates min<max */
void         XPT2046_GetCalibration(xpt2046_t *t, uint16_t *xmin, uint16_t *xmax,
                                    uint16_t *ymin, uint16_t *ymax);
void         XPT2046_PenIRQ_ISR(xpt2046_t *t);   /* forward EXTI here; see drv_isr.h */

typedef void (*xpt2046_pen_cb_t)(void *user);
void         XPT2046_SetPenDownCallback(xpt2046_t *t, xpt2046_pen_cb_t cb, void *user);

#ifdef __cplusplus
}
#endif
#endif /* DRV_XPT2046_H_ */
