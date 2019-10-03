#ifndef __COROUTINE_SCHED_H_H_H__
#define __COROUTINE_SCHED_H_H_H__

#include <ucontext.h>
#include <stddef.h>

#include "common.h"
#include "ds.h"

#define _COROUTINE_STATE_READY          (0x0)
#define _COROUTINE_STATE_RUNNING        (0x1)
#define _COROUTINE_STATE_ZOMBIE         (0x2)
#define _COROUTINE_STATE_SLEEPING       (0x3)
#define _COROUTINE_STATE_IO_WAITING     (0x4)
#define _COROUTINE_STATE_LOCK_WAITING   (0x5)
#define _COROUTINE_STATE_COND_WAITING   (0x6)
#define _COROUTINE_STATE_JOIN_WAITING   (0x7)

#define _COROUTINE_FLAG_JOINABLE_IDX    (0x0)

#define _DEFAULT_STACK_SIZE         (1048576)

struct _co_sched {
    // the context of the scheduler
    ucontext_t      ctx;
    // coroutine stack
    _co_stack_t     *stk;
    // idle queue
    _co_list_t      *readyq;
    // sleep queue
    _co_time_heap_t *sleepq;
    // io-waiting queue
    _co_list_t      *iowaitq;
    // mutex-waiting queue
    _co_list_t      *mutexq;
    // cond-waiting queue
    _co_list_t      *condq;
    // zombie queue
    _co_list_t      *zombieq;
    // join queue
    _co_list_t      *joinq;
    // primary state
    int             state;
};

struct _co_thread {
    // the context of the scheduler
    ucontext_t              ctx;
    // the queue of the coroutine(readyq/iowaitq/lockwaitq/zombieq/joinq)
    _co_list_t              link;
    // the heap of the timeout event
    _co_time_heap_node_t    *tlelink;
    // the coroutine which wait the current coroutine
    _co_thread_t            *join;
    // the number of coroutine waiting for the current coroutine
    size_t                  join_cnt;
    // start function
    _co_fp_t                fn;
    // start function parameters
    void                    *args;
    // return value
    void                    *ret;
    // coroutine stack
    _co_stack_t             *stk;
    // coroutine state
    int                     state;
    // coroutine flag
    int                     flag;
};

struct _co_stack {
    // the list element of the object pool
    _co_list_t      link;
    // the start of the stack & redzone
    char            *rstart;
    // the end of the stack & redzone
    char            *rend;
    // the start of the stack
    char            *start;
    // the end of the stack
    char            *end;
    // the size of the stack
    ptrdiff_t       size;
};

#endif
