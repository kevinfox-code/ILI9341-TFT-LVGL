#include "mock_os.h"
#include <string.h>

#ifndef MOCK_OS_MAX_MUTEXES
#define MOCK_OS_MAX_MUTEXES 8
#endif
#ifndef MOCK_OS_MAX_SEMS
#define MOCK_OS_MAX_SEMS 8
#endif

typedef struct {
    bool in_use;
    int lock_depth;
    int lock_calls;
    int unlock_calls;
    bool forced_busy;
} mock_mutex_state_t;

typedef struct {
    bool in_use;
    int count;
} mock_sem_state_t;

static mock_mutex_state_t s_mutexes[MOCK_OS_MAX_MUTEXES];
static mock_sem_state_t   s_sems[MOCK_OS_MAX_SEMS];
static uint32_t s_tick = 0;
static bool s_in_isr = false;

void mock_os_reset(void)
{
    memset(s_mutexes, 0, sizeof(s_mutexes));
    memset(s_sems, 0, sizeof(s_sems));
    s_tick = 0;
    s_in_isr = false;
}

bool mock_os_mutex_balance_ok(void)
{
    for (int i = 0; i < MOCK_OS_MAX_MUTEXES; i++) {
        if (s_mutexes[i].in_use && s_mutexes[i].lock_depth != 0) {
            return false;
        }
    }
    return true;
}

uint32_t mock_os_get_tick(void)
{
    return s_tick;
}

void mock_os_advance_tick(uint32_t ms)
{
    s_tick += ms;
}

void mock_os_set_in_isr(bool in_isr)
{
    s_in_isr = in_isr;
}

void mock_mutex_force_busy(drv_mutex_t m, bool busy)
{
    mock_mutex_state_t *mu = (mock_mutex_state_t *)m;
    if (mu != NULL) {
        mu->forced_busy = busy;
    }
}

drv_mutex_t drv_mutex_create(const char *name)
{
    (void)name;
    for (int i = 0; i < MOCK_OS_MAX_MUTEXES; i++) {
        if (!s_mutexes[i].in_use) {
            s_mutexes[i].in_use = true;
            s_mutexes[i].lock_depth = 0;
            s_mutexes[i].forced_busy = false;
            return &s_mutexes[i];
        }
    }
    return NULL;
}

drv_status_t drv_mutex_lock(drv_mutex_t m, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (m == NULL) {
        return DRV_ERR_PARAM;
    }
    mock_mutex_state_t *mu = (mock_mutex_state_t *)m;
    mu->lock_calls++;
    if (mu->forced_busy) {
        return DRV_ERR_TIMEOUT;
    }
    mu->lock_depth++;
    return DRV_OK;
}

drv_status_t drv_mutex_unlock(drv_mutex_t m)
{
    if (m == NULL) {
        return DRV_ERR_PARAM;
    }
    mock_mutex_state_t *mu = (mock_mutex_state_t *)m;
    mu->unlock_calls++;
    mu->lock_depth--;
    return DRV_OK;
}

drv_sem_t drv_sem_create_binary(const char *name)
{
    (void)name;
    for (int i = 0; i < MOCK_OS_MAX_SEMS; i++) {
        if (!s_sems[i].in_use) {
            s_sems[i].in_use = true;
            s_sems[i].count = 0;
            return &s_sems[i];
        }
    }
    return NULL;
}

drv_status_t drv_sem_take(drv_sem_t s, uint32_t timeout_ms)
{
    (void)timeout_ms;
    if (s == NULL) {
        return DRV_ERR_PARAM;
    }
    mock_sem_state_t *se = (mock_sem_state_t *)s;
    if (se->count > 0) {
        se->count--;
        return DRV_OK;
    }
    return DRV_ERR_TIMEOUT;
}

drv_status_t drv_sem_give(drv_sem_t s)
{
    if (s == NULL) {
        return DRV_ERR_PARAM;
    }
    ((mock_sem_state_t *)s)->count++;
    return DRV_OK;
}

void drv_delay_ms(uint32_t ms)
{
    mock_os_advance_tick(ms);
}

uint32_t drv_tick_ms(void)
{
    return s_tick;
}

bool drv_in_isr(void)
{
    return s_in_isr;
}

bool drv_os_kernel_running(void)
{
    return true;
}
