#ifndef __COROUTINE_H_H_H__
#define __COROUTINE_H_H_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct _co_thread   co_thread_t;
typedef struct _co_socket   co_socket_t;
typedef struct _co_mutex    co_mutex_t;
typedef struct _co_cond     co_cond_t;
typedef long long           co_time_t;

extern int co_framework_init();
extern int co_framework_destroy();
extern co_thread_t *coroutine_create(void *(*)(void *), void *);
extern void coroutine_exit(void *);
extern void coroutine_setdetachstate(co_thread_t *, int);
extern int coroutine_getdetachstate(co_thread_t *);
extern int coroutine_join(co_thread_t *, void **);
extern void coroutine_force_schedule();

extern co_socket_t *co_socket(int, int, int);
extern int co_bind(co_socket_t *, const struct sockaddr *, socklen_t);
extern int co_listen(co_socket_t *, int);
extern int co_close(co_socket_t *);
extern co_socket_t *co_accept(co_socket_t *, struct sockaddr *, socklen_t *);
extern int co_socket_get_fd(co_socket_t *);
extern int co_connect(co_socket_t *, const struct sockaddr *, socklen_t);

extern ssize_t co_read(co_socket_t *, void *, size_t);
extern ssize_t co_write(co_socket_t *, const void *, size_t);
extern ssize_t co_recv(co_socket_t *, void *, size_t, int);
extern ssize_t co_send(co_socket_t *, const void *, size_t, int);
extern ssize_t co_recvfrom(co_socket_t *, void *, size_t, int, struct sockaddr *, socklen_t *);
extern ssize_t co_sendto(co_socket_t *, const void *, size_t, int,
    const struct sockaddr *, socklen_t);

extern co_mutex_t *co_mutex_create();
extern int co_mutex_destroy(co_mutex_t *);
extern int co_mutex_lock(co_mutex_t *);
extern int co_mutex_unlock(co_mutex_t *);
extern int co_mutex_trylock(co_mutex_t *);

extern co_cond_t *co_cond_create();
extern int co_cond_destroy(co_cond_t *);
extern void co_cond_wait(co_cond_t *);
extern void co_cond_signal(co_cond_t *);
extern void co_cond_broadcast(co_cond_t *);

extern int co_usleep(co_time_t);

#define COROUTINE_FLAG_JOINABLE        (0x0)
#define COROUTINE_FLAG_NONJOINABLE     (0x1)

#endif
