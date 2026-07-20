#include "app_main.h"
#include "system.h"
#include "clock_protocol.h"
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
static const osThreadAttr_t k_time_sync_attr = {
    .name = "timeSync",
    .stack_size = 256 * 4,
    .priority = (osPriority_t)osPriorityLow1,
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
        /* Once per second, push a full RTC snapshot for the UART task to
         * format and broadcast (the dashboard renders its live clock from
         * these lines). */
        osDelay(1000);
        clock_datetime_t dt;
        if (System_GetDateTime(&dt)) {
            osMessageQueuePut(s_shared_queue, &dt, 0, 0);
        }
    }
}

static void dma_uart_sender_task(void *arg)
{
    (void)arg;
    for (;;) {
        clock_datetime_t dt;
        if (osMessageQueueGet(s_shared_queue, &dt, NULL, osWaitForever) == osOK) {
            char buf[CLOCK_PROTO_RESP_MAX];
            int len = clock_format_broadcast(buf, sizeof(buf), &dt);
            if (len > 0) {
                System_UartSendLine(buf, (uint16_t)len);
            }
        }
    }
}

/* Consumes host commands (SET / TIME? / PING) from the UART RX ring and
 * answers them. The wire protocol itself lives in clock_protocol.c, which
 * is HAL-free and unit-tested on the host (tests/host/). */
static void time_sync_task(void *arg)
{
    (void)arg;
    static clock_line_acc_t acc;
    clock_line_acc_init(&acc);
    System_UartStartReceive();

    for (;;) {
        uint8_t rx[32];
        size_t n = System_UartReceive(rx, sizeof(rx));
        if (n == 0) {
            osDelay(20);
            continue;
        }
        for (size_t i = 0; i < n; i++) {
            clock_cmd_t cmd;
            if (!clock_proto_feed(&acc, (char)rx[i], &cmd)) {
                continue;
            }

            char resp[CLOCK_PROTO_RESP_MAX];
            int len = 0;
            switch (cmd.type) {
            case CLOCK_CMD_SET:
                if (System_SetDateTime(&cmd.dt)) {
                    len = clock_format_set_ok(resp, sizeof(resp), &cmd.dt);
                } else {
                    len = clock_format_err(resp, sizeof(resp), CLOCK_ERR_BADVAL);
                }
                break;
            case CLOCK_CMD_GET: {
                clock_datetime_t now;
                System_GetDateTime(&now);
                len = clock_format_time_resp(resp, sizeof(resp), &now);
                break;
            }
            case CLOCK_CMD_PING:
                len = clock_format_pong(resp, sizeof(resp));
                break;
            case CLOCK_CMD_ERROR:
                len = clock_format_err(resp, sizeof(resp), cmd.err);
                break;
            default:
                break;
            }
            if (len > 0) {
                System_UartSendLine(resp, (uint16_t)len);
            }
        }
    }
}

void App_CreateTasks(void)
{
    /* ~60Hz VSYNC tick; SPI throughput is the real frame-rate limit. */
    osTimerId_t vsync_timer = osTimerNew(vsync_timer_cb, osTimerPeriodic, NULL, &k_vsync_timer_attr);
    osTimerStart(vsync_timer, 16);

    System_CommsInit();
    s_shared_queue = osMessageQueueNew(16, sizeof(clock_datetime_t), &k_shared_queue_attr);

    osThreadNew(touchgfx_task, NULL, &k_touchgfx_task_attr);
    osThreadNew(below_normal_task, NULL, &k_below_normal_attr);
    osThreadNew(read_and_load_time_task, NULL, &k_read_time_attr);
    osThreadNew(dma_uart_sender_task, NULL, &k_dma_uart_attr);
    osThreadNew(time_sync_task, NULL, &k_time_sync_attr);
}
