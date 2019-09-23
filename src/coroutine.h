#ifndef __COROUTINE_H_H_H__
#define __COROUTINE_H_H_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct _co_thread      co_thread_t;
typedef struct _co_socket      co_socket_t;

extern int co_framework_init();
extern void co_framework_destroy();
extern co_thread_t *coroutine_create(void *(*)(void *), void *);
extern void coroutine_exit(void *);
extern void coroutine_setdetachstate(co_thread_t *, int);
extern int coroutine_getdetachstate(co_thread_t *);
extern int coroutine_join(co_thread_t *);

extern co_socket_t *co_socket(int, int, int);
extern int co_bind(co_socket_t *, const struct sockaddr *, socklen_t);
extern int co_listen(co_socket_t *, int);
extern int co_close(co_socket_t *);
extern co_socket_t *co_accept(co_socket_t *, struct sockaddr *, socklen_t *);
extern ssize_t co_read(co_socket_t *, void *, size_t);
extern ssize_t co_write(co_socket_t *, const void *, size_t);

#define COROUTINE_FLAG_JOINABLE        (0x0)
#define COROUTINE_FLAG_NONJOINABLE     (0x1)

#endif
