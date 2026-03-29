#ifndef BOARD_DRIVERS_H_
#define BOARD_DRIVERS_H_

#include <stdbool.h>
#include <stdint.h>
#include "ILI9341.h"
#include "XPT2046.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Touch calibration values (raw ADC min/max for each axis).
 *
 * Defaults are loaded from the selected board header at startup.
 * Override at runtime with Board_SetTouchCalibration().
 * Restore defaults with Board_ResetTouchCalibration().
 */
typedef struct {
    uint16_t raw_x_min;
    uint16_t raw_x_max;
    uint16_t raw_y_min;
    uint16_t raw_y_max;
} Board_TouchCalibration_t;

const ILI9341_Config_t *Board_GetDisplayConfig(void);
const XPT2046_Config_t *Board_GetTouchConfig(void);

uint16_t Board_GetDisplayHorRes(void);
uint16_t Board_GetDisplayVerRes(void);
uint8_t  Board_GetDisplayRotation(void);

void Board_GetTouchCalibration(Board_TouchCalibration_t *cal);
void Board_SetTouchCalibration(const Board_TouchCalibration_t *cal);
void Board_ResetTouchCalibration(void);
bool Board_TouchCalibrationIsOverridden(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_DRIVERS_H_ */
