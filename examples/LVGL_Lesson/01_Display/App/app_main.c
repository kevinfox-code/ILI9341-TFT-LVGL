#include "app_main.h"
#include "gui/gui.h"
#include "system.h"
#include "drv_constants.h"

#include "lvgl.h"
#include "drv/drv_setup.h"
#include "lv_ili9341.h"
#include "lv_xpt2046.h"

#include "cmsis_os2.h"

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

#define LV_BUF_ROWS 10
#define BYTES_PER_PX (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))

LV_ATTRIBUTE_MEM_ALIGN
static uint8_t s_lv_buf1[DRV_DISP_HOR_RES * LV_BUF_ROWS * BYTES_PER_PX];
LV_ATTRIBUTE_MEM_ALIGN
static uint8_t s_lv_buf2[DRV_DISP_HOR_RES * LV_BUF_ROWS * BYTES_PER_PX];

static lv_display_t *s_lv_disp;

static osMessageQueueId_t s_shared_queue;

static void lvgl_wake_cb(void *user)
{
    (void)user;
    /* No dedicated GUI-task notification wired up in this example: the GUI
     * task already polls at a fixed 5ms tick, which is fast enough for a
     * resistive touch panel. A real low-latency app would notify a waiting
     * GUI task from here instead. */
}

void App_Init(void)
{
    lv_init();

    if (DRV_Setup() != DRV_OK) {
        System_Fatal();
    }

    s_lv_disp = lv_ili9341_create(DRV_GetDisplay(), s_lv_buf1, s_lv_buf2, sizeof(s_lv_buf1));
    if (s_lv_disp == NULL) {
        System_Fatal();
    }

    lv_indev_t *touch = lv_xpt2046_create(DRV_GetTouch(), s_lv_disp);
    if (touch == NULL) {
        System_Fatal();
    }
    lv_xpt2046_set_wake_cb(lvgl_wake_cb, NULL);

    gui_load();
}

/* ---- App tasks --------------------------------------------------------- */

static const osThreadAttr_t k_lvgl_task_attr = {
    .name = "lvglTask",
    .stack_size = 2048 * 4,
    .priority = (osPriority_t)osPriorityLow,
};
static const osThreadAttr_t k_below_normal_attr = {
    .name = "belowNormalTask",
    .stack_size = 128 * 4,
    .priority = (osPriority_t)osPriorityBelowNormal,
};
static const osThreadAttr_t k_read_time_attr = {
    .name = "readAndLoadTime",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow3,
};
static const osThreadAttr_t k_dma_uart_attr = {
    .name = "dmaUart",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow2,
};
static const osThreadAttr_t k_update_time_gui_attr = {
    .name = "updateTimeGui",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow7,
};
static const osTimerAttr_t k_lvgl_timer_attr = { .name = "lvglTimer" };
static const osMessageQueueAttr_t k_shared_queue_attr = { .name = "sharedQueue" };

static void lvgl_tick_timer_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(1);
}

static void lvgl_task(void *arg)
{
    (void)arg;
    for (;;) {
        osDelay(5);
        lv_timer_handler();
    }
}

static void below_normal_task(void *arg)
{
    (void)arg;
    static volatile uint32_t debug_counter = 0;
    for (;;) {
        osDelay(1000);
        System_ToggleDebugLed();
        debug_counter++;
        printf("Debug Counter: %" PRIu32 "\n", debug_counter);
    }
}

static void read_and_load_time_task(void *arg)
{
    (void)arg;
    for (;;) {
        /* Once per second, push seconds-since-midnight for the UART task
         * to format and send. */
        osDelay(1000);
        uint32_t message = System_GetSecondsSinceMidnight();
        osMessageQueuePut(s_shared_queue, &message, 0, 0);
    }
}

static void dma_uart_sender_task(void *arg)
{
    (void)arg;
    for (;;) {
        uint32_t message = 0;
        if (osMessageQueueGet(s_shared_queue, &message, NULL, osWaitForever) == osOK) {
            uint32_t hours = message / 3600U;
            uint32_t minutes = (message % 3600U) / 60U;
            uint32_t secs = message % 60U;

            char buf[32];
            int len = snprintf(buf, sizeof(buf), "RTC %02u:%02u:%02u\r\n",
                               (unsigned)hours, (unsigned)minutes, (unsigned)secs);
            if (len > 0) {
                System_UartSendLine(buf, (uint16_t)len);
            }
        }
    }
}

static void update_time_gui_task(void *arg)
{
    (void)arg;
    for (;;) {
        osDelay(1000);

        char time_str[16], date_str[16];
        System_GetClockStrings(time_str, sizeof(time_str), date_str, sizeof(date_str));

        lv_lock();
        gui_update_clock(time_str, date_str);
        lv_unlock();
    }
}

void App_CreateTasks(void)
{
    osTimerId_t lvgl_timer = osTimerNew(lvgl_tick_timer_cb, osTimerPeriodic, NULL, &k_lvgl_timer_attr);
    osTimerStart(lvgl_timer, 1);

    s_shared_queue = osMessageQueueNew(16, sizeof(uint32_t), &k_shared_queue_attr);

    osThreadNew(lvgl_task, NULL, &k_lvgl_task_attr);
    osThreadNew(below_normal_task, NULL, &k_below_normal_attr);
    osThreadNew(read_and_load_time_task, NULL, &k_read_time_attr);
    osThreadNew(dma_uart_sender_task, NULL, &k_dma_uart_attr);
    osThreadNew(update_time_gui_task, NULL, &k_update_time_gui_attr);
}
