#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_

#include "main.h"
#include "spi.h"

/*
 * Board contract and board selector for display + touch resources.
 *
 * One board header must define the BOARD_* mapping macros below.
 * If no explicit board define is provided, STM32F446 is used by default.
 *
 * To add a new board:
 *   1. Copy boards/board_stm32f446.h → boards/board_your_mcu.h
 *   2. Define all BOARD_* macros for your MCU's pins and SPI handles
 *   3. Add the new board to your CMakeLists.txt BOARD_TARGET mapping
 *   4. Add a #elif block below to include your new header
 */

#if defined(BOARD_STM32F411)
#include "boards/board_stm32f411.h"
#elif defined(BOARD_STM32F446)
#include "boards/board_stm32f446.h"
#else
/* Default to STM32F446 if no board is explicitly selected */
#include "boards/board_stm32f446.h"
#endif

/* ── Required board mapping contract ─────────────────────────────────────── */

#ifndef BOARD_DISP_SPI_HANDLE
#error "Board mapping must define BOARD_DISP_SPI_HANDLE"
#endif

#ifndef BOARD_DISP_CS_PORT
#error "Board mapping must define BOARD_DISP_CS_PORT"
#endif

#ifndef BOARD_DISP_CS_PIN
#error "Board mapping must define BOARD_DISP_CS_PIN"
#endif

#ifndef BOARD_DISP_DC_PORT
#error "Board mapping must define BOARD_DISP_DC_PORT"
#endif

#ifndef BOARD_DISP_DC_PIN
#error "Board mapping must define BOARD_DISP_DC_PIN"
#endif

#ifndef BOARD_DISP_RESET_PORT
#error "Board mapping must define BOARD_DISP_RESET_PORT"
#endif

#ifndef BOARD_DISP_RESET_PIN
#error "Board mapping must define BOARD_DISP_RESET_PIN"
#endif

#ifndef BOARD_DISP_HOR_RES
#error "Board mapping must define BOARD_DISP_HOR_RES"
#endif

#ifndef BOARD_DISP_VER_RES
#error "Board mapping must define BOARD_DISP_VER_RES"
#endif

#ifndef BOARD_DISP_ROTATION
#error "Board mapping must define BOARD_DISP_ROTATION"
#endif

#ifndef BOARD_TOUCH_SPI_HANDLE
#error "Board mapping must define BOARD_TOUCH_SPI_HANDLE"
#endif

#ifndef BOARD_TOUCH_CS_PORT
#error "Board mapping must define BOARD_TOUCH_CS_PORT"
#endif

#ifndef BOARD_TOUCH_CS_PIN
#error "Board mapping must define BOARD_TOUCH_CS_PIN"
#endif

#ifndef BOARD_TOUCH_IRQ_PORT
#error "Board mapping must define BOARD_TOUCH_IRQ_PORT"
#endif

#ifndef BOARD_TOUCH_IRQ_PIN
#error "Board mapping must define BOARD_TOUCH_IRQ_PIN"
#endif

#ifndef BOARD_TOUCH_RAW_X_MIN
#error "Board mapping must define BOARD_TOUCH_RAW_X_MIN"
#endif

#ifndef BOARD_TOUCH_RAW_X_MAX
#error "Board mapping must define BOARD_TOUCH_RAW_X_MAX"
#endif

#ifndef BOARD_TOUCH_RAW_Y_MIN
#error "Board mapping must define BOARD_TOUCH_RAW_Y_MIN"
#endif

#ifndef BOARD_TOUCH_RAW_Y_MAX
#error "Board mapping must define BOARD_TOUCH_RAW_Y_MAX"
#endif

/* ── Compile-time value validation ───────────────────────────────────────── */

#if (BOARD_DISP_HOR_RES == 0u) || (BOARD_DISP_VER_RES == 0u)
#error "Display resolution must be non-zero"
#endif

#if (BOARD_DISP_ROTATION > 3u)
#error "Display rotation must be in range [0..3]"
#endif

#if (BOARD_TOUCH_RAW_X_MIN >= BOARD_TOUCH_RAW_X_MAX)
#error "Invalid touch calibration: RAW_X_MIN must be < RAW_X_MAX"
#endif

#if (BOARD_TOUCH_RAW_Y_MIN >= BOARD_TOUCH_RAW_Y_MAX)
#error "Invalid touch calibration: RAW_Y_MIN must be < RAW_Y_MAX"
#endif

#endif /* BOARD_CONFIG_H_ */
