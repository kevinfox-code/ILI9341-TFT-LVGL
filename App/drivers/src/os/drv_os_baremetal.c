#include "drv/drv_os.h"
#include <stddef.h>

extern void HAL_Delay(uint32_t Delay);
extern uint32_t HAL_GetTick(void);

#if defined(__arm__) && !defined(__aarch64__)
static inline void bm_disable_irq(void) { __asm volatile ("cpsid i" ::: "memory"); }
static inline void bm_enable_irq(void)  { __asm volatile ("cpsie i" ::: "memory"); }
#else
/* Host build (unit tests): no real interrupts to mask; critical sections
 * are a no-op here since the test process provides its own concurrency
 * story (see tests/host). */
static inline void bm_disable_irq(void) {}
static inline void bm_enable_irq(void)  {}
#endif

typedef struct {
    volatile uint8_t locked;
    bool in_use;
} bm_mutex_t;

typedef struct {
    volatile uint8_t signaled;
    bool in_use;
} bm_sem_t;

#ifndef DRV_OS_BM_MAX_MUTEXES
#define DRV_OS_BM_MAX_MUTEXES 8
#endif
#ifndef DRV_OS_BM_MAX_SEMS
#define DRV_OS_BM_MAX_SEMS 8
#endif

static bm_mutex_t s_mutexes[DRV_OS_BM_MAX_MUTEXES];
static bm_sem_t   s_sems[DRV_OS_BM_MAX_SEMS];

drv_mutex_t drv_mutex_create(const char *name)
{
    (void)name;
    for (int i = 0; i < DRV_OS_BM_MAX_MUTEXES; i++) {
        if (!s_mutexes[i].in_use) {
            s_mutexes[i].in_use = true;
            s_mutexes[i].locked = 0u;
            return (drv_mutex_t)&s_mutexes[i];
        }
    }
    return NULL;
}

drv_status_t drv_mutex_lock(drv_mutex_t m, uint32_t timeout_ms)
{
    if (m == NULL) {
        return DRV_ERR_PARAM;
    }
    bm_mutex_t *mu = (bm_mutex_t *)m;
    uint32_t start = drv_tick_ms();
    for (;;) {
        bm_disable_irq();
        bool got = (mu->locked == 0u);
        if (got) {
            mu->locked = 1u;
        }
        bm_enable_irq();
        if (got) {
            return DRV_OK;
        }
        if (timeout_ms != DRV_OS_WAIT_FOREVER &&
            (drv_tick_ms() - start) >= timeout_ms) {
            return DRV_ERR_TIMEOUT;
        }
    }
}

drv_status_t drv_mutex_unlock(drv_mutex_t m)
{
    if (m == NULL) {
        return DRV_ERR_PARAM;
    }
    bm_mutex_t *mu = (bm_mutex_t *)m;
    bm_disable_irq();
    mu->locked = 0u;
    bm_enable_irq();
    return DRV_OK;
}

drv_sem_t drv_sem_create_binary(const char *name)
{
    (void)name;
    for (int i = 0; i < DRV_OS_BM_MAX_SEMS; i++) {
        if (!s_sems[i].in_use) {
            s_sems[i].in_use = true;
            s_sems[i].signaled = 0u;
            return (drv_sem_t)&s_sems[i];
        }
    }
    return NULL;
}

drv_status_t drv_sem_take(drv_sem_t s, uint32_t timeout_ms)
{
    if (s == NULL) {
        return DRV_ERR_PARAM;
    }
    bm_sem_t *se = (bm_sem_t *)s;
    uint32_t start = drv_tick_ms();
    for (;;) {
        if (se->signaled) {
            se->signaled = 0u;
            return DRV_OK;
        }
        if (timeout_ms != DRV_OS_WAIT_FOREVER &&
            (drv_tick_ms() - start) >= timeout_ms) {
            return DRV_ERR_TIMEOUT;
        }
    }
}

drv_status_t drv_sem_give(drv_sem_t s)
{
    if (s == NULL) {
        return DRV_ERR_PARAM;
    }
    ((bm_sem_t *)s)->signaled = 1u;
    return DRV_OK;
}

void drv_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

uint32_t drv_tick_ms(void)
{
    return HAL_GetTick();
}

bool drv_in_isr(void)
{
#if defined(__arm__) && !defined(__aarch64__)
    uint32_t ipsr;
    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr));
    return ipsr != 0u;
#else
    return false;
#endif
}

bool drv_os_kernel_running(void)
{
    return false;
}
