#include "board_drivers.h"
#include "board_config.h"

static const ILI9341_Config_t display_config = {
    .hspi        = BOARD_DISP_SPI_HANDLE,
    .cs_port     = BOARD_DISP_CS_PORT,
    .cs_pin      = BOARD_DISP_CS_PIN,
    .dc_port     = BOARD_DISP_DC_PORT,
    .dc_pin      = BOARD_DISP_DC_PIN,
    .reset_port  = BOARD_DISP_RESET_PORT,
    .reset_pin   = BOARD_DISP_RESET_PIN,
    .hor_res     = BOARD_DISP_HOR_RES,
    .ver_res     = BOARD_DISP_VER_RES,
};

static const XPT2046_Config_t touch_config = {
    .hspi      = BOARD_TOUCH_SPI_HANDLE,
    .cs_port   = BOARD_TOUCH_CS_PORT,
    .cs_pin    = BOARD_TOUCH_CS_PIN,
    .irq_port  = BOARD_TOUCH_IRQ_PORT,
    .irq_pin   = BOARD_TOUCH_IRQ_PIN,
};

static Board_TouchCalibration_t touch_calibration_defaults = {
    .raw_x_min = BOARD_TOUCH_RAW_X_MIN,
    .raw_x_max = BOARD_TOUCH_RAW_X_MAX,
    .raw_y_min = BOARD_TOUCH_RAW_Y_MIN,
    .raw_y_max = BOARD_TOUCH_RAW_Y_MAX,
};

static Board_TouchCalibration_t touch_calibration_active = {
    .raw_x_min = BOARD_TOUCH_RAW_X_MIN,
    .raw_x_max = BOARD_TOUCH_RAW_X_MAX,
    .raw_y_min = BOARD_TOUCH_RAW_Y_MIN,
    .raw_y_max = BOARD_TOUCH_RAW_Y_MAX,
};

static bool touch_calibration_overridden = false;

const ILI9341_Config_t *Board_GetDisplayConfig(void)
{
    return &display_config;
}

const XPT2046_Config_t *Board_GetTouchConfig(void)
{
    return &touch_config;
}

uint16_t Board_GetDisplayHorRes(void)
{
    return BOARD_DISP_HOR_RES;
}

uint16_t Board_GetDisplayVerRes(void)
{
    return BOARD_DISP_VER_RES;
}

uint8_t Board_GetDisplayRotation(void)
{
    return BOARD_DISP_ROTATION;
}

void Board_GetTouchCalibration(Board_TouchCalibration_t *cal)
{
    if(cal == NULL) return;
    *cal = touch_calibration_active;
}

void Board_SetTouchCalibration(const Board_TouchCalibration_t *cal)
{
    if(cal == NULL) return;
    if((cal->raw_x_min >= cal->raw_x_max) || (cal->raw_y_min >= cal->raw_y_max)) return;
    touch_calibration_active = *cal;
    touch_calibration_overridden = true;
}

void Board_ResetTouchCalibration(void)
{
    touch_calibration_active = touch_calibration_defaults;
    touch_calibration_overridden = false;
}

bool Board_TouchCalibrationIsOverridden(void)
{
    return touch_calibration_overridden;
}
