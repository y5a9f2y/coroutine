#ifndef __COROUTINE_DS_H_H_H__
#define __COROUTINE_DS_H_H_H__

#include "common.h"

struct _co_list {
    _co_list_t *next;
    _co_list_t *prev;
};

struct _co_time_heap_node {
    _co_time_t timeout;
};

struct _co_time_heap {
    size_t  cap;
    size_t  size;
    _co_time_heap_node_t *heap;
};

#endif
