#ifndef __COROUTINE_COMMON_H_H_H__
#define __COROUTINE_COMMON_H_H_H__

#include "coroutine.h"

typedef struct _co_sched            _co_sched_t;
typedef struct _co_thread           _co_thread_t;
typedef struct _co_stack            _co_stack_t;
typedef struct _co_list             _co_list_t;
typedef struct _co_socket           _co_socket_t;
typedef struct _co_mutex            _co_mutex_t;
typedef struct _co_cond             _co_cond_t;
typedef long long                   _co_time_t;
typedef struct _co_time_heap        _co_time_heap_t;
typedef struct _co_time_heap_node   _co_time_heap_node_t;

#define _CO_DS_OFFSET(ty, field) ((size_t)&(((ty *)0)->field))

#define _CO_STACK_PTR(lk) \
    ((_co_stack_t *)((char *)(lk) - _CO_DS_OFFSET(_co_stack_t, link)))

#define _CO_THREAD_LINK_PTR(tk) \
    ((_co_thread_t *)((char *)(tk) - _CO_DS_OFFSET(_co_thread_t, link)))

#define _CO_SOCKET_PTR(sk) \
    ((_co_socket_t *)((char *)(sk) - _CO_DS_OFFSET(_co_socket_t, link)))

#define _CO_MUTEX_PTR(mk) \
    ((_co_mutex_t *)((char *)(mk) - _CO_DS_OFFSET(_co_mutex_t, link)))

#define _CO_COND_PTR(ck) \
    ((_co_cond_t *)((char *)(ck) - _CO_DS_OFFSET(_co_cond_t, link)))

typedef void *(*_co_fp_t)(void *);

// the function declaration of the data structure

_co_list_t *_co_list_create();
void _co_list_init(_co_list_t *);
void _co_list_destroy(_co_list_t *);
void _co_list_insert(_co_list_t *, _co_list_t *);
_co_list_t *_co_list_delete(_co_list_t *);
int _co_list_empty(_co_list_t *);
void _co_switch();

void _co_socket_destroy(_co_socket_t *);
void _co_mutex_destroy(_co_mutex_t *);
void _co_cond_destroy(_co_cond_t *);

_co_time_heap_t *_co_time_heap_create();
void _co_time_heap_destroy(_co_time_heap_t *);
int _co_time_heap_expand(_co_time_heap_t *);
int _co_time_heap_insert(_co_time_heap_t *, _co_time_t, _co_thread_t *);
_co_time_heap_node_t _co_time_heap_delete(_co_time_heap_t *, _co_time_heap_node_t *);
int _co_time_heap_empty(_co_time_heap_t *);
_co_time_heap_node_t _co_time_heap_top(_co_time_heap_t *);


// the variable declaration
    
extern _co_list_t       *_co_socket_list;
extern _co_list_t       *_co_socket_pool;
extern _co_list_t       *_co_thread_pool;
extern _co_list_t       *_co_stack_pool;
extern _co_list_t       *_co_mutex_pool;
extern _co_list_t       *_co_cond_pool;
extern _co_thread_t     *_co_current;
extern _co_sched_t      *_co_scheduler;
#endif
