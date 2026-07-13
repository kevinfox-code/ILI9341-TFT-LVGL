#ifndef MOCK_OS_H_
#define MOCK_OS_H_
#include "drv/drv_os.h"

/* Deterministic, single-threaded fake of drv_os.h for host unit tests.
 * Mutexes/semaphores never actually block (the test process is
 * single-threaded) — they track state so tests can assert on it instead. */

void mock_os_reset(void);

/* True if every mutex created since the last reset has an equal number of
 * lock/unlock calls (no imbalance left over at test teardown). */
bool mock_os_mutex_balance_ok(void);

uint32_t mock_os_get_tick(void);
void     mock_os_advance_tick(uint32_t ms);
void     mock_os_set_in_isr(bool in_isr);

/* Makes the next drv_mutex_lock() on this mutex fail with DRV_ERR_TIMEOUT,
 * simulating contention from another thread, without needing real threads. */
void mock_mutex_force_busy(drv_mutex_t m, bool busy);

#endif /* MOCK_OS_H_ */
