/**
 * @file TouchController.c
 * @brief LVGL input driver for the XPT2046 touch controller.
 */
#include "TouchController.h"
#include "XPT2046.h"
#include "main.h"
#include "board_config.h"
#include "board_drivers.h"
#include "lvgl.h"
#include <stdio.h>
#include <inttypes.h>      /* PRIu16 */

#ifdef FREERTOS
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#endif

#ifdef FREERTOS
#define DELAY_MS(ms) osDelay(ms)
#else
#define DELAY_MS(ms) HAL_Delay(ms)
#endif

static volatile bool touch_pressed = false;
static volatile uint16_t touch_x = 0;
static volatile uint16_t touch_y = 0;

#ifdef FREERTOS
extern osThreadId_t lvglTaskHandle;
#endif

static Board_TouchCalibration_t touch_cal;

/* Map raw-Y → pixel-X (0..hor_res-1) */
static lv_coord_t map_x(uint16_t raw) {
    lv_coord_t w = lv_disp_get_hor_res(NULL);
    if(raw <= touch_cal.raw_y_min) raw = touch_cal.raw_y_min;
    if(raw >= touch_cal.raw_y_max) raw = touch_cal.raw_y_max;
    return (lv_coord_t)((uint32_t)(raw - touch_cal.raw_y_min) * (w - 1) / (touch_cal.raw_y_max - touch_cal.raw_y_min));
}

/* Map raw-X → pixel-Y (0..ver_res-1) */
static lv_coord_t map_y(uint16_t raw) {
    lv_coord_t h = lv_disp_get_ver_res(NULL);
    if(raw <= touch_cal.raw_x_min) raw = touch_cal.raw_x_min;
    if(raw >= touch_cal.raw_x_max) raw = touch_cal.raw_x_max;
    return (lv_coord_t)((uint32_t)(raw - touch_cal.raw_x_min) * (h - 1) / (touch_cal.raw_x_max - touch_cal.raw_x_min));
}

/* Draw a small green crosshair at (x,y) on the given screen */
static void draw_cross(lv_obj_t *scr, lv_coord_t x, lv_coord_t y) {
    static const lv_point_precise_t ph[] = {{-10,0},{10,0}};
    lv_obj_t *h = lv_line_create(scr);
    lv_line_set_points(h, ph, 2);
    lv_obj_set_style_line_color(h, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(h, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_pos(h, x, y);

    static const lv_point_precise_t pv[] = {{0,-10},{0,10}};
    lv_obj_t *v = lv_line_create(scr);
    lv_line_set_points(v, pv, 2);
    lv_obj_set_style_line_color(v, lv_color_hex(0x00FF00), LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_style_line_width(v, 2, LV_PART_MAIN|LV_STATE_DEFAULT);
    lv_obj_set_pos(v, x, y);
}

/**
 * Run a 4-point on-screen calibration.
 * Tap each green + in sequence; raw values are printed via printf so you can
 * compute new BOARD_TOUCH_RAW_*_MIN/MAX defaults for your board header.
 */
void touch_calibrate(void) {
#ifdef FREERTOS
    configASSERT(xPortIsInsideInterrupt() == pdFALSE);
#endif

    lv_obj_t *cal = lv_obj_create(NULL);
    lv_disp_load_scr(cal);
    lv_obj_clear_flag(cal, LV_OBJ_FLAG_SCROLLABLE);

    lv_coord_t w = lv_disp_get_hor_res(NULL);
    lv_coord_t h = lv_disp_get_ver_res(NULL);

    /* Inset 20 px from each corner */
    const lv_point_t corners[4] = {
        {20,       20},       /* top-left  */
        {w - 21,   20},       /* top-right */
        {w - 21,   h - 21},   /* bot-right */
        {20,       h - 21}    /* bot-left  */
    };

    uint16_t raw_x[4], raw_y[4];
    for(int i = 0; i < 4; i++) {
        printf("Starting calibration point %d\r\n", i);

        lv_obj_clean(cal);
        draw_cross(cal, corners[i].x, corners[i].y);

        lv_obj_t *lbl = lv_label_create(cal);
        lv_label_set_text(lbl, "Touch the green +");
        lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 40);

        lv_timer_handler();
        lv_refr_now(NULL);
        DELAY_MS(100);

        uint16_t rx, ry;
        printf("Waiting for press...\r\n");
        do {
            DELAY_MS(10);
        } while(!XPT2046_GetTouch(&rx, &ry));

        raw_x[i] = rx;
        raw_y[i] = ry;
        printf("Cal %d: raw=(%u,%u)\r\n", i, rx, ry);

        printf("Waiting for release...\r\n");
        do {
            DELAY_MS(10);
        } while(XPT2046_GetTouch(&rx, &ry));

        printf("Point %d complete\r\n", i);
        DELAY_MS(100);
    }

    printf("Calibration complete!\r\n");

    {
        Board_TouchCalibration_t cal_data = {
            .raw_x_min = raw_x[0],
            .raw_x_max = raw_x[0],
            .raw_y_min = raw_y[0],
            .raw_y_max = raw_y[0],
        };

        for(int i = 1; i < 4; i++) {
            if(raw_x[i] < cal_data.raw_x_min) cal_data.raw_x_min = raw_x[i];
            if(raw_x[i] > cal_data.raw_x_max) cal_data.raw_x_max = raw_x[i];
            if(raw_y[i] < cal_data.raw_y_min) cal_data.raw_y_min = raw_y[i];
            if(raw_y[i] > cal_data.raw_y_max) cal_data.raw_y_max = raw_y[i];
        }

        Board_SetTouchCalibration(&cal_data);
        Board_GetTouchCalibration(&touch_cal);

        printf("Calibration saved: X[%u..%u] Y[%u..%u]\r\n",
               touch_cal.raw_x_min,
               touch_cal.raw_x_max,
               touch_cal.raw_y_min,
               touch_cal.raw_y_max);
    }

    lv_obj_t *blank = lv_obj_create(NULL);
    lv_scr_load(blank);
    lv_obj_del(cal);
    lv_timer_handler();
    lv_refr_now(NULL);
}

void TouchController_Poll(void) {
    uint16_t xr = 0;
    uint16_t yr = 0;

    if(XPT2046_GetTouch(&xr, &yr)) {
        touch_pressed = true;
        touch_x = (uint16_t)map_x(yr);
        touch_y = (uint16_t)map_y(xr);
    } else {
        touch_pressed = false;
    }
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    (void)indev;
    data->state = touch_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = (lv_coord_t)touch_x;
    data->point.y = (lv_coord_t)touch_y;
}

void TouchController_Init(const XPT2046_Config_t *config) {
    XPT2046_Init(config);
    Board_GetTouchCalibration(&touch_cal);
    touch_pressed = false;
    lv_indev_t *ind = lv_indev_create();
    lv_indev_set_type(ind, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(ind, touchpad_read);
}

/* PENIRQ EXTI handler: notify LVGL task — no SPI or LVGL calls inside an ISR. */
void HAL_GPIO_EXTI_Callback(uint16_t pin) {
    if(pin == BOARD_TOUCH_IRQ_PIN) {
#ifdef FREERTOS
        if(lvglTaskHandle != NULL) {
            BaseType_t hpw = pdFALSE;
            xTaskNotifyFromISR((TaskHandle_t)lvglTaskHandle, 0x01u, eSetBits, &hpw);
            portYIELD_FROM_ISR(hpw);
        }
#endif
    }
}
