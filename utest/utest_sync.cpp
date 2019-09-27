#include <errno.h>
#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/coroutine.h"
    #include "coroutine/ds.h"
    #include "coroutine/sched.h"
    #include "coroutine/sync.h"
}

TEST(MutexTest, Life) {

    ASSERT_FALSE(co_framework_init());

    ASSERT_TRUE(_co_list_empty(_co_mutex_pool));
    ASSERT_TRUE(_co_list_empty(_co_scheduler->mutexq));

    co_mutex_t *mu = co_mutex_create();
    ASSERT_TRUE(_co_list_empty(&mu->wait));
    ASSERT_TRUE(mu);
    ASSERT_FALSE(co_mutex_lock(mu));
    ASSERT_TRUE(mu->flag);
    ASSERT_TRUE(_co_list_empty(&mu->wait));
    ASSERT_EQ(co_mutex_lock(mu), -1);
    ASSERT_EQ(errno, EDEADLK);
    ASSERT_EQ(co_mutex_trylock(mu), -1);
    ASSERT_EQ(errno, EDEADLK);
    ASSERT_FALSE(_co_list_empty(_co_scheduler->mutexq));
    ASSERT_TRUE(_co_scheduler->mutexq->next == &mu->link);
    ASSERT_EQ(co_mutex_destroy(mu), -1);
    ASSERT_EQ(errno, EBUSY);
    ASSERT_FALSE(co_mutex_unlock(mu));
    ASSERT_FALSE(co_mutex_destroy(mu));
    ASSERT_TRUE(_co_list_empty(_co_scheduler->mutexq));
    ASSERT_TRUE(!_co_list_empty(_co_mutex_pool));
    ASSERT_TRUE(_co_mutex_pool->next == &mu->link);

    mu = co_mutex_create();
    ASSERT_TRUE(_co_list_empty(_co_mutex_pool));
    ASSERT_TRUE(!_co_list_empty(_co_scheduler->mutexq));
    ASSERT_TRUE(_co_scheduler->mutexq->next == &mu->link);
    ASSERT_EQ(mu->flag, 0);
    ASSERT_FALSE(co_mutex_trylock(mu));
    ASSERT_EQ(co_mutex_trylock(mu), -1);
    ASSERT_EQ(errno, EDEADLK);
    ASSERT_FALSE(co_mutex_unlock(mu));
    ASSERT_FALSE(co_mutex_destroy(mu));

    ASSERT_FALSE(co_framework_destroy());

}

TEST(CondTest, Life) {

    ASSERT_FALSE(co_framework_init());

    ASSERT_TRUE(_co_list_empty(_co_cond_pool));
    ASSERT_TRUE(_co_list_empty(_co_scheduler->condq));

    co_cond_t *cond = co_cond_create();
    ASSERT_TRUE(cond);
    ASSERT_TRUE(_co_list_empty(&cond->wait));
    ASSERT_EQ(cond->flag, 0);
    ASSERT_FALSE(_co_list_empty(_co_scheduler->condq));
    co_cond_wait(cond);
    ASSERT_EQ(cond->flag, 1);
    ASSERT_TRUE(_co_list_empty(&cond->wait));
    co_cond_signal(cond);
    ASSERT_EQ(cond->flag, 0);
    co_cond_destroy(cond);
    ASSERT_TRUE(!_co_list_empty(_co_cond_pool));
    ASSERT_TRUE(_co_list_empty(_co_scheduler->condq));

    cond = co_cond_create();
    ASSERT_TRUE(_co_list_empty(_co_cond_pool));
    ASSERT_TRUE(!_co_list_empty(_co_scheduler->condq));
    ASSERT_EQ(cond->flag, 0);
    co_cond_wait(cond);
    ASSERT_EQ(cond->flag, 1);
    ASSERT_TRUE(_co_list_empty(&cond->wait));
    co_cond_broadcast(cond);
    ASSERT_EQ(cond->flag, 0);
    co_cond_destroy(cond);

    ASSERT_FALSE(co_framework_destroy());

}
