#ifndef __COROUTINE_SOCKET_H_H_H__
#define __COROUTINE_SOCKET_H_H_H__

#include "common.h"
#include "ds.h"

struct _co_socket {
    int             fd;
    _co_thread_t    *rco;
    _co_thread_t    *wco;
    _co_list_t      link;
    int             flag;
    int             polling;
};

#define _COSOCKET_READ_INDEX     (0x0)
#define _COSOCKET_WRITE_INDEX    (0x1)

void _co_socket_flag_set(_co_socket_t *, int);
int _co_socket_flag_get(_co_socket_t *, int);
void _co_socket_flag_unset(_co_socket_t *, int);

void _co_socket_polling_set(_co_socket_t *);
void _co_socket_polling_unset(_co_socket_t *);
int _co_socket_polling_get(_co_socket_t *);

#endif
