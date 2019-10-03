#ifndef __COROUTINE_SYNC_H_H_H__
#define __COROUTINE_SYNC_H_H_H__

#include "common.h"
#include "ds.h"


struct _co_mutex {
    _co_thread_t    *owner;
    _co_list_t      link;
    _co_list_t      wait;
    // if a coroutine holds a mutex, then flag is 1, otherwise it is 0
    int             flag;
    // if the primary coroutine waits a mutex, then pflag is 1, otherwise it is 0
    int             pflag;
};

struct _co_cond {
    _co_list_t      link;
    _co_list_t      wait;
    // whether a coroutine waits for a conditional variable, if so, it is 1, otherwise it is 0
    int             flag;
    // if the primary coroutine waits for a cond variable, then pflag is 1, otherwise it is 0
    int             pflag;
};


#endif
