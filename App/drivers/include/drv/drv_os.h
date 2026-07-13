#ifndef DRV_OS_H_
#define DRV_OS_H_
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DRV_OS_WAIT_FOREVER 0xFFFFFFFFu

typedef void *drv_mutex_t;   /* NULL = invalid */
typedef void *drv_sem_t;     /* NULL = invalid */

typedef enum {
    DRV_OK = 0,
    DRV_ERR_TIMEOUT,
    DRV_ERR_PARAM,
    DRV_ERR_STATE,
    DRV_ERR_HW
} drv_status_t;

drv_mutex_t  drv_mutex_create(const char *name);
drv_status_t drv_mutex_lock(drv_mutex_t m, uint32_t timeout_ms);
drv_status_t drv_mutex_unlock(drv_mutex_t m);

drv_sem_t    drv_sem_create_binary(const char *name);   /* created EMPTY (count 0) */
drv_status_t drv_sem_take(drv_sem_t s, uint32_t timeout_ms);
drv_status_t drv_sem_give(drv_sem_t s);                 /* usable from ISR and thread */

void     drv_delay_ms(uint32_t ms);
uint32_t drv_tick_ms(void);
bool     drv_in_isr(void);
bool     drv_os_kernel_running(void);   /* false before osKernelStart / on bare metal */

#ifdef __cplusplus
}
#endif
#endif /* DRV_OS_H_ */
