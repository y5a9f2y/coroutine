#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ds.h"
#include "socket.h"
#include "sched.h"
#include "event.h"
#include "sync.h"

static size_t _sys_stack_size = 0;
static size_t _sys_page_size = 0;

_co_sched_t *_co_scheduler = NULL;
_co_thread_t *_co_current = NULL;
_co_list_t *_co_socket_list = NULL;
_co_list_t *_co_socket_pool = NULL;
_co_list_t *_co_stack_pool = NULL;
_co_list_t *_co_thread_pool = NULL;
_co_list_t *_co_mutex_pool = NULL;
_co_list_t *_co_cond_pool = NULL;
_co_time_heap_t *_co_timeout_heap = NULL;

// static declaration of the functions
static _co_stack_t *_co_stack_create();
static void _co_stack_recycle(_co_stack_t *);
static void _co_stack_destroy(_co_stack_t *);
static void _co_switch_save_stack();
static void _co_schedule();
static void _co_schedule_main();
static void _coroutine_recycle(_co_thread_t *);
static void _coroutine_destroy(_co_thread_t *);
static void _co_list_thread_destroy(_co_list_t *, int);
static void _co_list_cond_destroy(_co_list_t *);
static void _co_list_mutex_destroy(_co_list_t *);
static void _co_list_stack_destroy(_co_list_t *);
static void _co_list_socket_teardown(_co_list_t *, int);

static void _co_framework_destroy_pool();

// code segments of the scheduler
int co_framework_init() {

    struct rlimit rlim;

    if(_co_scheduler) {
        return 0;
    }

    if(getrlimit(RLIMIT_STACK, &rlim) < 0) {
        return errno;
    }
    _sys_stack_size = (rlim.rlim_cur < 0) ? _DEFAULT_STACK_SIZE : rlim.rlim_cur;
    if((_sys_page_size = sysconf(_SC_PAGESIZE)) < 0) {
        if(!errno) {
            return EINVAL;
        } else {
            return errno;
        }
    }

    if(_co_eventsys_init() < 0) {
        return errno;
    }

    if(!(_co_stack_pool = _co_list_create()) ||
        !(_co_thread_pool = _co_list_create()) ||
        !(_co_socket_list = _co_list_create()) ||
        !(_co_socket_pool = _co_list_create()) || 
        !(_co_mutex_pool = _co_list_create()) ||
        !(_co_cond_pool = _co_list_create())) {
        _co_framework_destroy_pool();
        return ENOMEM;
    }

    if(!(_co_scheduler = (_co_sched_t *)malloc(sizeof(_co_sched_t)))) {
        _co_framework_destroy_pool();
        return ENOMEM;
    }
    memset(_co_scheduler, 0, sizeof(_co_scheduler));

    if(!(_co_scheduler->readyq = _co_list_create()) ||
        !(_co_scheduler->sleepq = _co_list_create()) ||
        !(_co_scheduler->iowaitq = _co_list_create()) ||
        !(_co_scheduler->mutexq = _co_list_create()) || 
        !(_co_scheduler->condq = _co_list_create()) || 
        !(_co_scheduler->zombieq = _co_list_create()) ||
        !(_co_scheduler->joinq = _co_list_create()) ||
        !(_co_scheduler->stk = _co_stack_create())) {
        _co_framework_destroy_pool();
        co_framework_destroy();
        return ENOMEM;
    }

    return 0;

}

static void _co_framework_destroy_pool() {

    _co_list_mutex_destroy(_co_mutex_pool);
    _co_list_cond_destroy(_co_cond_pool);
    _co_list_thread_destroy(_co_thread_pool, 1);
    _co_list_stack_destroy(_co_stack_pool);
    _co_list_socket_teardown(_co_socket_list, 1);
    _co_list_socket_teardown(_co_socket_pool, 0);

}

static void _co_list_socket_teardown(_co_list_t *l, int flag) {

    _co_list_t *next;
    _co_socket_t *sock;

    if(!l) {
        return;
    }

    while(!_co_list_empty(l)) {
        next = _co_list_delete(l->next);
        sock = _CO_SOCKET_PTR(next);
        if(flag) {
            co_close(sock);
        } else {
            _co_socket_destroy(sock);
        }
    }

    _co_list_destroy(l);

}

static void _co_list_thread_destroy(_co_list_t *l, int flag) {

    _co_list_t *next;
    _co_thread_t *co;

    if(!l) {
        return;
    }

    while(!_co_list_empty(l)) {
        next = _co_list_delete(l->next);
        co = _CO_THREAD_LINK_PTR(next);
        _coroutine_destroy(co);
    }

    if(flag) {
        _co_list_destroy(l);
    }

}

static void _co_list_mutex_destroy(_co_list_t *l) {

    _co_list_t *next;
    _co_mutex_t *mu;

    if(!l) {
        return;
    }

    while(!_co_list_empty(l)) {
        next = _co_list_delete(l->next);
        mu = _CO_MUTEX_PTR(next);
        _co_list_thread_destroy(&mu->wait, 0);
        _co_mutex_destroy(mu);
    }

    _co_list_destroy(l);

}

static void _co_list_cond_destroy(_co_list_t *l) {

    _co_list_t *next;
    _co_cond_t *cond;

    if(!l) {
        return;
    }

    while(!_co_list_empty(l)) {
        next = _co_list_delete(l->next);
        cond = _CO_COND_PTR(next);
        _co_list_thread_destroy(&cond->wait, 0);
        _co_cond_destroy(cond);
    }

    _co_list_destroy(l);

}

static void _co_list_stack_destroy(_co_list_t *l) {

    _co_list_t *next;
    _co_stack_t *stk;

    if(!l) {
        return;
    }

    while(!_co_list_empty(l)) {
        next = _co_list_delete(l->next);
        stk = _CO_STACK_PTR(next);
        _co_stack_destroy(stk);
    }

    _co_list_destroy(l);

}


int co_framework_destroy() {

    if(_co_current) {
        errno = EINVAL;
        return -1;
    }

    if(!_co_scheduler) {
        return 0;
    }

    _co_list_thread_destroy(_co_scheduler->readyq, 1);
    _co_list_thread_destroy(_co_scheduler->sleepq, 1);
    _co_list_thread_destroy(_co_scheduler->iowaitq, 1);
    _co_list_thread_destroy(_co_scheduler->zombieq, 1);
    _co_list_thread_destroy(_co_scheduler->joinq, 1);
    _co_stack_destroy(_co_scheduler->stk);
    _co_list_mutex_destroy(_co_scheduler->mutexq);
    _co_list_cond_destroy(_co_scheduler->condq);
    free(_co_scheduler);
    _co_eventsys_destroy();
    _co_framework_destroy_pool();

    _co_scheduler = NULL;
    _co_current = NULL;
    _co_socket_list = NULL;
    _co_socket_pool = NULL;
    _co_stack_pool = NULL;
    _co_thread_pool = NULL;
    _co_mutex_pool = NULL;
    _co_cond_pool = NULL;

    return 0;

}


// code segments of the thread
_co_thread_t *coroutine_create(_co_fp_t fn, void *args) {

    _co_thread_t *alloc;
    _co_list_t *link;

    if(!_co_list_empty(_co_thread_pool)) {
        link = _co_list_delete(_co_thread_pool->next);
        alloc = _CO_THREAD_LINK_PTR(link);
    } else {
        alloc = (_co_thread_t *)malloc(sizeof(_co_thread_t));
        if(!alloc) {
            return NULL;
        }
        memset(alloc, 0, sizeof(alloc));
    }

    _co_list_init(&alloc->link);
    _co_list_init(&alloc->tlelink);

    alloc->fn = fn;
    alloc->args = args;
    alloc->state = _COROUTINE_STATE_READY;
    alloc->flag = COROUTINE_FLAG_JOINABLE;
    alloc->join = NULL;
    alloc->join_cnt = 0;
    alloc->stk = NULL;

    _co_list_insert(_co_scheduler->readyq, &alloc->link);

    return alloc;

}

static void _coroutine_recycle(_co_thread_t *thread) {

    if(thread) {
        _co_list_init(&thread->tlelink);
        _co_list_insert(_co_thread_pool, &thread->link);
        _co_stack_recycle(thread->stk);
        thread->stk = NULL;
    }

}

static void _coroutine_destroy(_co_thread_t *thread) {
    if(thread) {
        _co_stack_recycle(thread->stk);
        free(thread);
    }
}

void coroutine_exit(void *ret) {

    // the coroutine_exit function must be called in the start function of the coroutine
    if(!_co_current) {
        return;
    }

    // handle the state of the coroutine
    if(coroutine_getdetachstate(_co_current) == COROUTINE_FLAG_JOINABLE) {

        _co_current->ret = ret;
        _co_current->state = _COROUTINE_STATE_ZOMBIE;
        _co_list_insert(_co_scheduler->zombieq, &_co_current->link);

        // signal the coroutine which waits for the current coroutine
        if(_co_current->join) {
            _co_current->join->state = _COROUTINE_STATE_READY;
            _co_list_delete(&_co_current->join->link);
            _co_list_insert(_co_scheduler->readyq, &_co_current->join->link);
        }

    } else {
        _coroutine_recycle(_co_current);
    }

    // trigger to switch the context
    _co_switch();

}

void coroutine_setdetachstate(_co_thread_t *thread, int detachstate) {
    switch(detachstate) {
        case COROUTINE_FLAG_JOINABLE:
            thread->flag &= (~(1 << _COROUTINE_FLAG_JOINABLE_IDX));
            break;
        case COROUTINE_FLAG_NONJOINABLE:
            thread->flag |= (1 << _COROUTINE_FLAG_JOINABLE_IDX);
            break;
    }
}

int coroutine_getdetachstate(_co_thread_t *thread) {
    if(thread->flag & (1 << _COROUTINE_FLAG_JOINABLE_IDX)) {
        return COROUTINE_FLAG_NONJOINABLE;
    }
    return COROUTINE_FLAG_JOINABLE;
}

int coroutine_join(_co_thread_t *thread, void **value_ptr) {

    if(coroutine_getdetachstate(thread) == COROUTINE_FLAG_NONJOINABLE) {
        return EINVAL;
    }

    if(thread->join_cnt) {
        return EAGAIN;
    }

    if(_co_current && _co_current->join == thread) {
        return EDEADLK;
    }

    if(_co_current && _co_current == thread) {
        return EDEADLK;
    }

    if(thread->state == _COROUTINE_STATE_ZOMBIE) {
        if(value_ptr) {
            *value_ptr = thread->ret;
        }
        _co_list_delete(&thread->link);
        _coroutine_recycle(thread);
        return 0;
    }

    while(1) {
        switch(thread->state) {
            case _COROUTINE_STATE_READY:
            case _COROUTINE_STATE_SLEEPING:
            case _COROUTINE_STATE_IO_WAITING:
            case _COROUTINE_STATE_LOCK_WAITING:
            case _COROUTINE_STATE_COND_WAITING:
            case _COROUTINE_STATE_JOIN_WAITING:
                // if the main coroutine join the current coroutine, only add the counter,
                // otherwise, set the join coroutine pointer at the same time
                if(_co_current) {
                    thread->join = _co_current;
                    _co_current->state = _COROUTINE_STATE_JOIN_WAITING;
                    _co_list_insert(_co_scheduler->joinq, &_co_current->link);
                }
                ++thread->join_cnt;
                
                // trigger to switch the context 
                _co_switch();

                break;
            case _COROUTINE_STATE_ZOMBIE:
                if(value_ptr) {
                    *value_ptr = thread->ret;
                }
                _co_list_delete(&thread->link);
                _coroutine_recycle(thread);
                return 0;
            case _COROUTINE_STATE_RUNNING:
            default:
                return EINVAL;
        }
    }

    return 0;

}

static void _co_switch_save_stack() {

    char flag = 0;

    if(!_co_current->stk) {
        if(!(_co_current->stk = _co_stack_create())) {
            return;
        }
    }

    _co_current->stk->size = _co_scheduler->stk->end - &flag;

    memcpy((void *)(_co_current->stk->end - _co_current->stk->size),
            (const void *)&flag, (size_t)(_co_current->stk->size));

}

void _co_switch() {
  
    if(_co_current) {
        // if the current coroutine is not the primary coroutine then save the context here
        // at the very first time, the _co_current is also NULL, so the switch is safe
        _co_switch_save_stack();
        swapcontext(&_co_current->ctx, &_co_scheduler->ctx);
    } else {
        // if the current coroutine is the primary coroutine, then just do the schedule process
        _co_schedule();
    }

}

static void _co_schedule_main() {

    _co_current->ret = _co_current->fn(_co_current->args);

    // handle the state of the coroutine
    if(coroutine_getdetachstate(_co_current) == COROUTINE_FLAG_JOINABLE) {

        _co_current->state = _COROUTINE_STATE_ZOMBIE;
        _co_list_insert(_co_scheduler->zombieq, &_co_current->link);

        // signal the coroutine which waits for the current coroutine
        if(_co_current->join) {
            _co_list_delete(&_co_current->join->link);
            _co_list_insert(_co_scheduler->readyq, &_co_current->join->link);
        }

    } else {
        _coroutine_recycle(_co_current);
    }

    // trigger to switch the context
    _co_switch();

}

static void _co_schedule() {

    _co_list_t *link;
    _co_thread_t *next;

    while(1) {

        while(!_co_list_empty(_co_scheduler->readyq)) {


            // get the ready coroutine
            link = _co_list_delete(_co_scheduler->readyq->next);
            next = _CO_THREAD_LINK_PTR(link);

            // set the current coroutine
            _co_current = next;

            if(!_co_current->stk) {
                getcontext(&_co_current->ctx);
                _co_current->ctx.uc_stack.ss_sp = (void *)(_co_scheduler->stk->start);
                _co_current->ctx.uc_stack.ss_size = _sys_stack_size;
                _co_current->ctx.uc_link = &_co_scheduler->ctx;
                makecontext(&_co_current->ctx, _co_schedule_main, 0);
            } else {
                memcpy((void *)(_co_scheduler->stk->end - _co_current->stk->size),
                        (const void *)(_co_current->stk->end - _co_current->stk->size),
                        (size_t)(_co_current->stk->size));
            }
            _co_current->state = _COROUTINE_STATE_RUNNING;
            swapcontext(&_co_scheduler->ctx, &_co_current->ctx);

        }

        _co_eventsys_dispatch();

        if(_co_list_empty(_co_scheduler->readyq)) {
            break;
        }

    }
    
    _co_current = NULL;

}

// code segments of the stack
static _co_stack_t *_co_stack_create() {

    int zerofd;
    char *stk;

    // get stack from the object pool
    _co_stack_t *alloc;
    _co_list_t *link;
    if(!_co_list_empty(_co_stack_pool)) {
        link = _co_list_delete(_co_stack_pool->next);
        alloc = _CO_STACK_PTR(link);
        return alloc;
    }

    // alloc new stack
    if((zerofd = open("/dev/zero", O_RDWR | O_CLOEXEC, 0)) == -1) {
        return NULL;
    }

    stk = (char *)mmap(NULL, _sys_stack_size + 2 * _sys_page_size, PROT_READ | PROT_WRITE, 
            MAP_PRIVATE, zerofd, 0);
    if((void *)(stk) == MAP_FAILED) {
        close(zerofd);
        return NULL;
    }

    close(zerofd);

    if((mprotect((void *)stk, _sys_page_size, PROT_NONE) == -1) || 
            (mprotect((void *)(stk + _sys_stack_size + _sys_page_size), 
                      _sys_page_size, PROT_NONE) == -1)) {
        munmap((void *)stk, _sys_stack_size + 2 * _sys_page_size);
        return NULL;
    }

    alloc = (_co_stack_t *)malloc(sizeof(_co_stack_t));
    if(!alloc) {
        munmap((void *)stk, _sys_stack_size + 2 * _sys_page_size);
        return NULL;
    }

    memset(alloc, 0, sizeof(alloc));
    _co_list_init(&alloc->link);
    alloc->rstart = stk;
    alloc->rend = stk + _sys_stack_size + 2 * _sys_page_size; 
    alloc->start = stk + _sys_page_size;
    alloc->end = stk + _sys_stack_size + _sys_page_size;
    alloc->size = 0;

    return alloc;

}

static void _co_stack_destroy(_co_stack_t *stk) {
    if(stk) {
        munmap((void *)stk->rstart, _sys_stack_size + 2 * _sys_page_size); 
        free(stk);
    }
}

static void _co_stack_recycle(_co_stack_t *stk) {

    if(stk) {
        stk->size = 0;
        _co_list_insert(_co_stack_pool, &stk->link);
    }

}
