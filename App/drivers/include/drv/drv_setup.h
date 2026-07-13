#ifndef DRV_SETUP_H_
#define DRV_SETUP_H_
#include "drv/drv_os.h"
#include "drv/ili9341.h"
#include "drv/xpt2046.h"

#ifdef __cplusplus
extern "C" {
#endif

/* One-call bring-up built from the application's drv_constants.h macros.
 * Creates the SPI bus(es), fills both chip config structs, and initializes
 * both chips. Safe to call before osKernelStart(). */
drv_status_t DRV_Setup(void);
ili9341_t   *DRV_GetDisplay(void);
xpt2046_t   *DRV_GetTouch(void);

#ifdef __cplusplus
}
#endif
#endif /* DRV_SETUP_H_ */
