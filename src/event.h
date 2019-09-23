#ifndef __COROUTINE_EVENT_H_H_H__
#define __COROUTINE_EVENT_H_H_H__

#include "common.h"

extern int _co_eventsys_init();
extern void _co_eventsys_destroy();
extern int _co_eventsys_dispatch();

#endif
