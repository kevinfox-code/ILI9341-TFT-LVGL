#include "unity.h"
#include "drv/drv_os.h"
#include <pthread.h>
#include <time.h>

void mock_hal_tick_reset(void);

void setUp(void)
{
    mock_hal_tick_reset();
}

void tearDown(void) {}

void test_semaphore_take_times_out(void)
{
    drv_sem_t s = drv_sem_create_binary("t");
    TEST_ASSERT_NOT_NULL(s);
    drv_status_t st = drv_sem_take(s, 20);
    TEST_ASSERT_EQUAL(DRV_ERR_TIMEOUT, st);
}

static drv_sem_t s_shared_sem;

static void *giver_thread(void *arg)
{
    (void)arg;
    struct timespec ts = { 0, 5 * 1000 * 1000 }; /* 5ms */
    nanosleep(&ts, NULL);
    drv_sem_give(s_shared_sem); /* simulates an ISR giving the semaphore */
    return NULL;
}

void test_give_from_isr_releases_pending_take(void)
{
    s_shared_sem = drv_sem_create_binary("shared");
    TEST_ASSERT_NOT_NULL(s_shared_sem);

    pthread_t th;
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&th, NULL, giver_thread, NULL));

    drv_status_t st = drv_sem_take(s_shared_sem, 2000);
    TEST_ASSERT_EQUAL(DRV_OK, st);

    pthread_join(th, NULL);
}

void test_mutex_lock_unlock_roundtrip(void)
{
    drv_mutex_t m = drv_mutex_create("m");
    TEST_ASSERT_NOT_NULL(m);
    TEST_ASSERT_EQUAL(DRV_OK, drv_mutex_lock(m, DRV_OS_WAIT_FOREVER));
    TEST_ASSERT_EQUAL(DRV_OK, drv_mutex_unlock(m));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_semaphore_take_times_out);
    RUN_TEST(test_give_from_isr_releases_pending_take);
    RUN_TEST(test_mutex_lock_unlock_roundtrip);
    return UNITY_END();
}
