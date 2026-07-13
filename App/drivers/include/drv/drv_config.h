#ifndef DRV_CONFIG_H_
#define DRV_CONFIG_H_

/*
 * Contract gate between this library and the application.
 *
 * The application provides "drv_constants.h" on its include path (see
 * templates/drv_constants_template.h for the required macros). This header
 * pulls it in, pulls in the HAL family header it names, and validates the
 * required macros are all present with sane values.
 *
 * Only drv_setup.c and drv_isr_glue.c include this header (and therefore the
 * only library sources that see CubeMX/HAL symbols by name).
 */

#include "drv_constants.h"

#ifndef DRV_HAL_HEADER
#error "drv_constants.h must define DRV_HAL_HEADER, e.g. \"stm32f4xx_hal.h\""
#endif
#include DRV_HAL_HEADER

#ifndef DRV_DISP_SPI_MX_INIT
#error "drv_constants.h must define DRV_DISP_SPI_MX_INIT()"
#endif
#ifndef DRV_TOUCH_SPI_MX_INIT
#error "drv_constants.h must define DRV_TOUCH_SPI_MX_INIT()"
#endif

#ifndef DRV_DISP_SPI_HANDLE
#error "drv_constants.h must define DRV_DISP_SPI_HANDLE"
#endif
#ifndef DRV_DISP_CS_PORT
#error "drv_constants.h must define DRV_DISP_CS_PORT"
#endif
#ifndef DRV_DISP_CS_PIN
#error "drv_constants.h must define DRV_DISP_CS_PIN"
#endif
#ifndef DRV_DISP_DC_PORT
#error "drv_constants.h must define DRV_DISP_DC_PORT"
#endif
#ifndef DRV_DISP_DC_PIN
#error "drv_constants.h must define DRV_DISP_DC_PIN"
#endif
#ifndef DRV_DISP_RESET_PORT
#error "drv_constants.h must define DRV_DISP_RESET_PORT"
#endif
#ifndef DRV_DISP_RESET_PIN
#error "drv_constants.h must define DRV_DISP_RESET_PIN"
#endif
#ifndef DRV_DISP_HOR_RES
#error "drv_constants.h must define DRV_DISP_HOR_RES"
#endif
#ifndef DRV_DISP_VER_RES
#error "drv_constants.h must define DRV_DISP_VER_RES"
#endif
#ifndef DRV_DISP_ROTATION
#error "drv_constants.h must define DRV_DISP_ROTATION"
#endif

#ifndef DRV_TOUCH_SPI_HANDLE
#error "drv_constants.h must define DRV_TOUCH_SPI_HANDLE"
#endif
#ifndef DRV_TOUCH_CS_PORT
#error "drv_constants.h must define DRV_TOUCH_CS_PORT"
#endif
#ifndef DRV_TOUCH_CS_PIN
#error "drv_constants.h must define DRV_TOUCH_CS_PIN"
#endif
#ifndef DRV_TOUCH_IRQ_PORT
#error "drv_constants.h must define DRV_TOUCH_IRQ_PORT"
#endif
#ifndef DRV_TOUCH_IRQ_PIN
#error "drv_constants.h must define DRV_TOUCH_IRQ_PIN"
#endif
#ifndef DRV_TOUCH_IRQ_EXTI_LINE
#error "drv_constants.h must define DRV_TOUCH_IRQ_EXTI_LINE"
#endif

#ifndef DRV_TOUCH_RAW_X_MIN
#error "drv_constants.h must define DRV_TOUCH_RAW_X_MIN"
#endif
#ifndef DRV_TOUCH_RAW_X_MAX
#error "drv_constants.h must define DRV_TOUCH_RAW_X_MAX"
#endif
#ifndef DRV_TOUCH_RAW_Y_MIN
#error "drv_constants.h must define DRV_TOUCH_RAW_Y_MIN"
#endif
#ifndef DRV_TOUCH_RAW_Y_MAX
#error "drv_constants.h must define DRV_TOUCH_RAW_Y_MAX"
#endif

#if (DRV_DISP_HOR_RES == 0u) || (DRV_DISP_VER_RES == 0u)
#error "Display resolution must be non-zero"
#endif

#if (DRV_DISP_ROTATION > 3u)
#error "Display rotation must be in range [0..3]"
#endif

#if (DRV_TOUCH_RAW_X_MIN >= DRV_TOUCH_RAW_X_MAX)
#error "Invalid touch calibration: RAW_X_MIN must be < RAW_X_MAX"
#endif

#if (DRV_TOUCH_RAW_Y_MIN >= DRV_TOUCH_RAW_Y_MAX)
#error "Invalid touch calibration: RAW_Y_MIN must be < RAW_Y_MAX"
#endif

#endif /* DRV_CONFIG_H_ */
