#include "app_main.h"
#include "system.h"
#include "drv_constants.h"

#include "drv/drv_setup.h"

#include "cmsis_os2.h"

#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>

/* TouchGFX entry points. Declared here rather than via app_touchgfx.h /
 * TouchGFXGeneratedHAL.cpp includes because App/ deliberately doesn't take
 * the TouchGFX include paths — these two C symbols are the whole contract.
 * MX_TouchGFX_Process() enters HAL::taskEntry() and never returns;
 * touchgfxSignalVSync() paces the render loop (safe from task context —
 * it's a 0-timeout queue put). */
void MX_TouchGFX_Process(void);
void touchgfxSignalVSync(void);
/* Board glue (Profiles/<board>/TouchGFX/target/TouchGFXDisplayDriver.cpp):
 * creates the SPI-transfer pump task; must run after DRV_Setup(). */
void TouchGFXDisplayDriver_Init(void);

static osMessageQueueId_t s_shared_queue;

/* ---- App tasks --------------------------------------------------------- */

static const osThreadAttr_t k_touchgfx_task_attr = {
    .name = "TouchGFX",
    .stack_size = 4096,
    .priority = (osPriority_t)osPriorityNormal,
};
static const osThreadAttr_t k_below_normal_attr = {
    .name = "belowNormalTask",
    .stack_size = 512 * 4,
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
static const osTimerAttr_t k_vsync_timer_attr = { .name = "tgfxVsync" };
static const osMessageQueueAttr_t k_shared_queue_attr = { .name = "sharedQueue" };

static void vsync_timer_cb(void *arg)
{
    (void)arg;
    touchgfxSignalVSync();
    /* Pet the watchdog here: this timer already paces the display at ~60Hz,
     * so its continued firing is a reasonable proxy for "the screen is
     * still alive". */
    System_PetWatchdog();
}

static void touchgfx_task(void *arg)
{
    (void)arg;
    /* Display/touch bring-up must happen in task context: pre-kernel, the
     * first FreeRTOS object creation masks interrupts until osKernelStart(),
     * so the HAL tick is dead and the driver's reset-pulse HAL_Delay() never
     * returns. (MX_TouchGFX_Init() in main.c already created RTOS objects.) */
    if (DRV_Setup() != DRV_OK) {
        System_Fatal();
    }
    TouchGFXDisplayDriver_Init();
    MX_TouchGFX_Process(); /* never returns */
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

void App_CreateTasks(void)
{
    /* ~60Hz VSYNC tick; SPI throughput is the real frame-rate limit. */
    osTimerId_t vsync_timer = osTimerNew(vsync_timer_cb, osTimerPeriodic, NULL, &k_vsync_timer_attr);
    osTimerStart(vsync_timer, 16);

    s_shared_queue = osMessageQueueNew(16, sizeof(uint32_t), &k_shared_queue_attr);

    osThreadNew(touchgfx_task, NULL, &k_touchgfx_task_attr);
    osThreadNew(below_normal_task, NULL, &k_below_normal_attr);
    osThreadNew(read_and_load_time_task, NULL, &k_read_time_attr);
    osThreadNew(dma_uart_sender_task, NULL, &k_dma_uart_attr);
}
