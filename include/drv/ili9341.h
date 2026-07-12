#ifndef DRV_ILI9341_H_
#define DRV_ILI9341_H_
#include <stdint.h>
#include <stdbool.h>
#include "drv/drv_os.h"
#include "drv/drv_spi.h"
#include "drv/drv_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ILI9341_MAX_LINE_PX
#define ILI9341_MAX_LINE_PX 320u
#endif

typedef struct {
    drv_spi_bus_t *bus;
    drv_gpio_t cs, dc, reset;
    uint16_t hor_res, ver_res;   /* logical, after rotation */
    uint8_t  rotation;           /* 0..3 */
} ili9341_config_t;

typedef struct ili9341 ili9341_t;   /* opaque instance; static storage inside driver */

/* done_cb fires in ISR context when an async flush completes (after CS raised). */
typedef void (*ili9341_done_cb_t)(void *user);

drv_status_t ILI9341_Init(ili9341_t **out, const ili9341_config_t *cfg);
drv_status_t ILI9341_SetRotation(ili9341_t *d, uint8_t rot);
drv_status_t ILI9341_FillScreen(ili9341_t *d, uint16_t rgb565);
drv_status_t ILI9341_DrawPixel(ili9341_t *d, uint16_t x, uint16_t y, uint16_t rgb565);
drv_status_t ILI9341_Flush(ili9341_t *d, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                           const uint8_t *px, uint32_t len_bytes, uint32_t timeout_ms); /* blocking */
drv_status_t ILI9341_FlushAsync(ili9341_t *d, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                                const uint8_t *px, uint32_t len_bytes,
                                ili9341_done_cb_t cb, void *user);
bool         ILI9341_FlushBusy(ili9341_t *d);
drv_status_t ILI9341_WaitFlushDone(ili9341_t *d, uint32_t timeout_ms);
drv_status_t ILI9341_Sleep(ili9341_t *d, bool on);   /* 0x10 / 0x11 + delays */
void         ILI9341_GetResolution(ili9341_t *d, uint16_t *hor_res, uint16_t *ver_res);

#ifdef __cplusplus
}
#endif
#endif /* DRV_ILI9341_H_ */
