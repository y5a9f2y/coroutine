#include <errno.h>
#include <stdlib.h>

#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/coroutine.h"
    #include "coroutine/sched.h"
}

static void *_coroutine_test_fn1(void *);
static void *_coroutine_test_fn2(void *);

TEST(CoroutineTest, LiveAndCall) {

    ASSERT_FALSE(co_framework_init());

    ASSERT_TRUE(_co_list_empty(_co_scheduler->readyq));
    int args = 0x0;
    co_thread_t *co = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co);
    ASSERT_TRUE(_CO_THREAD_LINK_PTR(&co->link) == co);
    ASSERT_TRUE(_CO_THREAD_TLELINK_PTR(&co->tlelink) == co);
    ASSERT_TRUE(!_co_list_empty(_co_scheduler->readyq));
    ASSERT_TRUE(_co_scheduler->readyq->next == &co->link);
    ASSERT_TRUE(_co_list_empty(_co_thread_pool));
    ASSERT_TRUE(_co_list_empty(_co_stack_pool));
    ASSERT_EQ(coroutine_join(co), 0);
    ASSERT_TRUE(co->ret == &args);
    ASSERT_EQ(args, 0xa);
    ASSERT_TRUE(!_co_list_empty(_co_thread_pool));
    ASSERT_TRUE(!_co_list_empty(_co_stack_pool));
    ASSERT_TRUE(_co_list_empty(_co_scheduler->readyq));

    co_thread_t *co1 = coroutine_create(_coroutine_test_fn2, &args);
    co_thread_t *co2 = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co1);
    ASSERT_FALSE(!co2);
    ASSERT_TRUE(co1 == co);
    ASSERT_TRUE(co1->stk == NULL);
    ASSERT_TRUE(!_co_list_empty(_co_scheduler->readyq));
    ASSERT_TRUE(_co_list_empty(_co_thread_pool));
    ASSERT_TRUE(!_co_list_empty(_co_stack_pool));
    ASSERT_TRUE(_co_scheduler->readyq->next == &co2->link);
    ASSERT_TRUE(co2->link.next == &co1->link);
    ASSERT_TRUE(co1->link.next == _co_scheduler->readyq);
    coroutine_join(co1);
    ASSERT_TRUE(_co_list_empty(_co_scheduler->readyq));
    ASSERT_TRUE(_co_scheduler->zombieq->next == &co2->link);
    ASSERT_TRUE(co2->link.next == _co_scheduler->zombieq);
    ASSERT_TRUE(!_co_list_empty(_co_thread_pool));
    ASSERT_TRUE(!_co_list_empty(_co_stack_pool));
    coroutine_join(co2);
    ASSERT_TRUE(_co_list_empty(_co_scheduler->zombieq));

    co_framework_destroy();

}

TEST(CoroutineTest, DetachState) {

    ASSERT_FALSE(co_framework_init());

    int args = 0x0;
    co_thread_t *co = coroutine_create(_coroutine_test_fn1, &args);
    co_thread_t *co1 = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co);
    ASSERT_FALSE(!co1);
    ASSERT_EQ(coroutine_getdetachstate(co), COROUTINE_FLAG_JOINABLE);

    coroutine_setdetachstate(co, COROUTINE_FLAG_NONJOINABLE);
    ASSERT_EQ(coroutine_getdetachstate(co), COROUTINE_FLAG_NONJOINABLE);

    coroutine_setdetachstate(co, COROUTINE_FLAG_JOINABLE);
    ASSERT_EQ(coroutine_getdetachstate(co), COROUTINE_FLAG_JOINABLE);

    coroutine_setdetachstate(co, COROUTINE_FLAG_NONJOINABLE);
    ASSERT_TRUE(!_co_list_empty(_co_scheduler->readyq));
    ASSERT_TRUE(_co_list_empty(_co_thread_pool));
    ASSERT_TRUE(_co_list_empty(_co_stack_pool));

    coroutine_join(co1);
    ASSERT_TRUE(co->ret == (void *)&args);
    ASSERT_TRUE(co1->ret == (void *)&args);
    ASSERT_TRUE(_co_list_empty(_co_scheduler->zombieq));

    co_framework_destroy();

}

TEST(CoroutineTest, JoinExceptions) {

    ASSERT_FALSE(co_framework_init());

    int args = 0x0;
    co_thread_t *co = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co);
    coroutine_setdetachstate(co, COROUTINE_FLAG_NONJOINABLE);
    ASSERT_EQ(coroutine_join(co), EINVAL);

    co = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co);
    co->join_cnt = 1;
    ASSERT_EQ(coroutine_join(co), EAGAIN);

    co = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co);
    _co_current = co;
    co->join = co;
    ASSERT_EQ(coroutine_join(co), EDEADLK);
    _co_current = NULL;

    co = coroutine_create(_coroutine_test_fn1, &args);
    co_thread_t *co1 = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co);
    _co_current = co;
    _co_current->join = co1;
    ASSERT_EQ(coroutine_join(co1), EDEADLK);
    _co_current = NULL;

    co = coroutine_create(_coroutine_test_fn1, &args);
    ASSERT_FALSE(!co);
    co->state = _COROUTINE_STATE_RUNNING;
    ASSERT_EQ(coroutine_join(co), EINVAL);

    co_framework_destroy();

}

static void *_coroutine_test_fn1(void *args) {
    int *p = (int *)args;
    *p = 0xa;
    return args;
}

static void *_coroutine_test_fn2(void *args) {
    int *p = (int *)args;
    *p = 0xb;
    coroutine_exit(args);
    return NULL;
}
