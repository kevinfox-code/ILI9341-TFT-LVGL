#ifndef BOARD_STM32F411_H_
#define BOARD_STM32F411_H_

/*
 * STM32F411 board mapping template for ILI9341 + XPT2046.
 *
 * Copy this file and rename it for your MCU, then update every macro to match
 * the CubeMX-generated symbols in your main.h and spi.h.
 *
 * GPIO labels (CS, DC, RESET, T_CS, T_IRQ) must match the User Labels you
 * assigned in CubeMX → Pinout & Configuration → GPIO.
 */

/* ── Display (ILI9341 on SPI1) ───────────────────────────────────────────── */
#define BOARD_DISP_SPI_HANDLE        (&hspi1)
#define BOARD_DISP_CS_PORT           CS_GPIO_Port
#define BOARD_DISP_CS_PIN            CS_Pin
#define BOARD_DISP_DC_PORT           DC_GPIO_Port
#define BOARD_DISP_DC_PIN            DC_Pin
#define BOARD_DISP_RESET_PORT        RESET_GPIO_Port
#define BOARD_DISP_RESET_PIN         RESET_Pin
#define BOARD_DISP_HOR_RES           320u   /* logical width after rotation  */
#define BOARD_DISP_VER_RES           240u   /* logical height after rotation */
#define BOARD_DISP_ROTATION          3u     /* 0=portrait  3=landscape       */

/* ── Touch (XPT2046 on SPI2) ─────────────────────────────────────────────── */
#define BOARD_TOUCH_SPI_HANDLE       (&hspi2)
#define BOARD_TOUCH_CS_PORT          T_CS_GPIO_Port
#define BOARD_TOUCH_CS_PIN           T_CS_Pin
#define BOARD_TOUCH_IRQ_PORT         T_IRQ_GPIO_Port
#define BOARD_TOUCH_IRQ_PIN          T_IRQ_Pin

/* ── Touch calibration defaults ──────────────────────────────────────────── */
/* Run touch_calibrate() once with printf enabled to obtain values for your   */
/* specific panel, then update these defaults.                                 */
#define BOARD_TOUCH_RAW_X_MIN        262u
#define BOARD_TOUCH_RAW_X_MAX        1872u
#define BOARD_TOUCH_RAW_Y_MIN        230u
#define BOARD_TOUCH_RAW_Y_MAX        1872u

#endif /* BOARD_STM32F411_H_ */
