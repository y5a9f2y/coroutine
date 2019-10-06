#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "sched.h"
#include "sync.h"

static void _co_mutex_recycle(_co_mutex_t *);
static void _co_cond_recycle(_co_cond_t *);

_co_mutex_t *co_mutex_create() {

    _co_list_t *link;
    _co_mutex_t *alloc;

    if(!_co_list_empty(_co_mutex_pool)) {
        link = _co_list_delete(_co_mutex_pool->next);
        alloc = _CO_MUTEX_PTR(link);
    } else {
        if(!(alloc = (_co_mutex_t *)malloc(sizeof(_co_mutex_t)))) {
            return NULL;
        }
    }
    _co_list_init(&alloc->wait);
    alloc->owner = NULL;
    alloc->flag = 0;
    alloc->pflag = 0;

    _co_list_insert(_co_scheduler->mutexq, &alloc->link);
    
    return alloc;

}

void _co_mutex_destroy(_co_mutex_t *m) {

    if(m) {
        free(m);
    }

}

static void _co_mutex_recycle(_co_mutex_t *m) {

    if(m) {
        _co_list_delete(&m->link);
        _co_list_insert(_co_mutex_pool, &m->link);
    }

}

int co_mutex_destroy(_co_mutex_t *m) {

    if(m) {
        if(m->flag || !_co_list_empty(&m->wait) || m->pflag) {
            errno = EBUSY;
            return -1;
        }
        _co_mutex_recycle(m);
    }

    return 0;

}

int co_mutex_lock(_co_mutex_t *m) {

    if(!m) {
        errno = EINVAL;
        return -1;
    }

    if(m->flag && m->owner == _co_current) {
        errno = EDEADLK;
        return -1;
    }

    while(1) {

        // hold the mutex
        if(!m->flag) {
            m->flag = 1;
            m->owner = _co_current;
            return 0;
        }

        if(_co_current) {
            _co_current->state = _COROUTINE_STATE_LOCK_WAITING;
            _co_list_delete(&_co_current->link);
            _co_list_insert(&m->wait, &_co_current->link);
        } else {
            _co_scheduler->state = _COROUTINE_STATE_LOCK_WAITING;
            m->pflag = 1;
        }

        // trigger to switch the context
        _co_switch();

    }

    errno = EINVAL;
    return -1;

}

int co_mutex_trylock(_co_mutex_t *m) {

    if(!m) {
        errno = EINVAL;
        return -1;
    }

    if(m->flag) {
        if(m->owner == _co_current) {
            errno = EDEADLK;
        } else {
            errno = EBUSY;
        }
        return -1;
    }

    m->flag = 1;
    m->owner = _co_current;
    return 0;

}

int co_mutex_unlock(_co_mutex_t *m) {

    _co_list_t *next;
    _co_thread_t *co;

    if(!m) {
        errno = EINVAL;
        return -1;
    }

    if(!m->flag || m->owner != _co_current) {
        errno = EPERM;
        return -1;
    }

    if(!_co_list_empty(&m->wait)) {
        next = _co_list_delete(m->wait.prev);
        co = _CO_THREAD_LINK_PTR(next);
        co->state = _COROUTINE_STATE_READY;
        _co_list_insert(_co_scheduler->readyq, &co->link);
    } else {
        if(m->pflag) {
            m->pflag = 0;
            _co_scheduler->state = _COROUTINE_STATE_READY;
        }
    }
    m->flag = 0;
    m->owner = NULL;
    return 0;

}

_co_cond_t *co_cond_create() {

    _co_list_t *link;
    _co_cond_t *alloc;

    if(!_co_list_empty(_co_cond_pool)) {
        link = _co_list_delete(_co_cond_pool->next);
        alloc = _CO_COND_PTR(link);
    } else {
        if(!(alloc = (_co_cond_t *)malloc(sizeof(_co_cond_t)))) {
            return NULL;
        }
    }

    alloc->flag = 0;
    alloc->pflag = 0;
    _co_list_init(&alloc->wait);
    _co_list_insert(_co_scheduler->condq, &alloc->link);

    return alloc;

}

void _co_cond_destroy(_co_cond_t *cond) {

    if(cond) {
        free(cond);
    }

}

static void _co_cond_recycle(_co_cond_t *cond) {

    if(cond) {
        _co_list_delete(&cond->link);
        _co_list_insert(_co_cond_pool, &cond->link);
    }

}

int co_cond_destroy(_co_cond_t *cond) {

    if(cond) {
        if(cond->flag || cond->pflag) {
            errno = EBUSY;
            return -1;
        }
        _co_cond_recycle(cond);
    }

    return 0;

}

void co_cond_wait(_co_cond_t *cond) {

    cond->flag = 1;
    if(_co_current) {
        _co_current->state = _COROUTINE_STATE_COND_WAITING;   
        _co_list_delete(&_co_current->link);
        _co_list_insert(&cond->wait, &_co_current->link);
    } else {
        cond->pflag = 1;
        _co_scheduler->state = _COROUTINE_STATE_COND_WAITING;
    }
    // trigger to switch the context
    _co_switch();

}

void co_cond_signal(_co_cond_t *cond) {

    _co_list_t *link;
    _co_thread_t *co;

    if(_co_list_empty(&cond->wait) && !cond->pflag) {
        cond->flag = 0;
        return;
    }

    if(!_co_list_empty(&cond->wait)) {
        link = _co_list_delete(cond->wait.prev);
        co = _CO_THREAD_LINK_PTR(link);
        co->state = _COROUTINE_STATE_READY;
        _co_list_insert(_co_scheduler->readyq, &co->link);
    } else {
        cond->pflag = 0;
        _co_scheduler->state = _COROUTINE_STATE_READY;
    }

    if(_co_list_empty(&cond->wait) && !cond->pflag) {
        cond->flag = 0;
    }

}

void co_cond_broadcast(_co_cond_t *cond) {

    _co_list_t *link;
    _co_thread_t *co;

    while(!_co_list_empty(&cond->wait)) {
        link = _co_list_delete(cond->wait.next);
        co = _CO_THREAD_LINK_PTR(link);
        co->state = _COROUTINE_STATE_READY;
        _co_list_insert(_co_scheduler->readyq, &co->link);
    }
    if(cond->pflag) {
        cond->pflag = 0;
        _co_scheduler->state = _COROUTINE_STATE_READY;
    }

    cond->flag = 0;

}
