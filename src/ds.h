#ifndef __COROUTINE_DS_H_H_H__
#define __COROUTINE_DS_H_H_H__

#include "common.h"

struct _co_list {
    _co_list_t *next;
    _co_list_t *prev;
};

#endif
