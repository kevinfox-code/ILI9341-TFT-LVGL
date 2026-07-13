/* Minimal HAL_Delay/HAL_GetTick stand-ins so src/os/drv_os_baremetal.c can be
 * linked and exercised directly on the host, without any STM32 HAL. Backed
 * by a real monotonic wall clock so genuine cross-thread timing (a "give"
 * from another thread racing a "take" timeout) behaves the same way it
 * would against the real HAL_GetTick on target. */
#include <stdint.h>
#include <time.h>

static struct timespec s_epoch;
static int s_epoch_set = 0;

static void ensure_epoch(void)
{
    if (!s_epoch_set) {
        clock_gettime(CLOCK_MONOTONIC, &s_epoch);
        s_epoch_set = 1;
    }
}

uint32_t HAL_GetTick(void)
{
    ensure_epoch();
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int64_t ms = (int64_t)(now.tv_sec - s_epoch.tv_sec) * 1000 +
                 (now.tv_nsec - s_epoch.tv_nsec) / 1000000;
    return (uint32_t)ms;
}

void HAL_Delay(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000u;
    ts.tv_nsec = (long)(ms % 1000u) * 1000000L;
    nanosleep(&ts, NULL);
}

void mock_hal_tick_reset(void)
{
    clock_gettime(CLOCK_MONOTONIC, &s_epoch);
    s_epoch_set = 1;
}
