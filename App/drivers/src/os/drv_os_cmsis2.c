#include "drv/drv_os.h"
#include "cmsis_os2.h"

/* Declared, not included, to stay free of any CubeMX/HAL header: both are
 * standard HAL entry points with a fixed signature. */
extern void HAL_Delay(uint32_t Delay);
extern uint32_t HAL_GetTick(void);

static bool kernel_running(void)
{
    return osKernelGetState() == osKernelRunning;
}

drv_mutex_t drv_mutex_create(const char *name)
{
    /* Backed by a binary semaphore, not osMutexNew: the ILI9341 async-flush
     * contract releases the bus lock from the SPI/DMA tx-complete ISR, and a
     * CMSIS-RTOS2 mutex cannot be released from ISR context (osErrorISR
     * leaves it permanently owned, deadlocking the next lock). A semaphore
     * can. The trade-off is losing priority inheritance. */
    osSemaphoreAttr_t attr = {0};
    attr.name = name;
    return (drv_mutex_t)osSemaphoreNew(1u, 1u, &attr);
}

static uint32_t ms_to_ticks(uint32_t timeout_ms)
{
    if (timeout_ms == DRV_OS_WAIT_FOREVER) {
        return osWaitForever;
    }
    uint32_t freq = osKernelGetTickFreq();
    if (freq == 0u) {
        freq = 1000u;
    }
    return (uint32_t)(((uint64_t)timeout_ms * freq) / 1000u);
}

drv_status_t drv_mutex_lock(drv_mutex_t m, uint32_t timeout_ms)
{
    if (m == NULL) {
        return DRV_ERR_PARAM;
    }
    if (!kernel_running()) {
        /* Pre-kernel: single context, nothing to contend with. */
        return DRV_OK;
    }
    if (drv_in_isr()) {
        return DRV_ERR_STATE;
    }
    osStatus_t st = osSemaphoreAcquire((osSemaphoreId_t)m, ms_to_ticks(timeout_ms));
    if (st == osOK) {
        return DRV_OK;
    }
    if (st == osErrorTimeout) {
        return DRV_ERR_TIMEOUT;
    }
    return DRV_ERR_HW;
}

drv_status_t drv_mutex_unlock(drv_mutex_t m)
{
    if (m == NULL) {
        return DRV_ERR_PARAM;
    }
    if (!kernel_running()) {
        return DRV_OK;
    }
    osStatus_t st = osSemaphoreRelease((osSemaphoreId_t)m);
    return (st == osOK) ? DRV_OK : DRV_ERR_HW;
}

drv_sem_t drv_sem_create_binary(const char *name)
{
    osSemaphoreAttr_t attr = {0};
    attr.name = name;
    return (drv_sem_t)osSemaphoreNew(1, 0, &attr);
}

drv_status_t drv_sem_take(drv_sem_t s, uint32_t timeout_ms)
{
    if (s == NULL) {
        return DRV_ERR_PARAM;
    }
    if (!kernel_running()) {
        /* Bare pre-kernel fallback: busy-poll would require ISR-driven give,
         * which cannot happen before the kernel starts; treat as immediate
         * timeout unless already given. */
        osStatus_t st = osSemaphoreAcquire((osSemaphoreId_t)s, 0);
        return (st == osOK) ? DRV_OK : DRV_ERR_TIMEOUT;
    }
    if (drv_in_isr() && timeout_ms != 0u) {
        return DRV_ERR_STATE;
    }
    osStatus_t st = osSemaphoreAcquire((osSemaphoreId_t)s, ms_to_ticks(timeout_ms));
    if (st == osOK) {
        return DRV_OK;
    }
    if (st == osErrorTimeout) {
        return DRV_ERR_TIMEOUT;
    }
    return DRV_ERR_HW;
}

drv_status_t drv_sem_give(drv_sem_t s)
{
    if (s == NULL) {
        return DRV_ERR_PARAM;
    }
    osStatus_t st = osSemaphoreRelease((osSemaphoreId_t)s);
    return (st == osOK) ? DRV_OK : DRV_ERR_HW;
}

void drv_delay_ms(uint32_t ms)
{
    if (kernel_running()) {
        osDelay(ms);
    } else {
        HAL_Delay(ms);
    }
}

uint32_t drv_tick_ms(void)
{
    return HAL_GetTick();
}

bool drv_in_isr(void)
{
    uint32_t ipsr;
    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr));
    return ipsr != 0u;
}

bool drv_os_kernel_running(void)
{
    return kernel_running();
}
