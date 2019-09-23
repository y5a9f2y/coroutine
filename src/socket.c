#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"
#include "sched.h"

static void _co_socket_destroy(_co_socket_t *);
static int _co_socket_flag_verify(int);

_co_socket_t *co_socket(int domain, int type, int protocol) {

    int fd;
    _co_socket_t *alloc;

    if((fd = socket(domain, type, protocol)) < 0) {
        return NULL;
    }

    if(fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        return NULL;
    }

    if(!(alloc = (_co_socket_t *)malloc(sizeof(_co_socket_t)))) {
        return NULL;
    }

    alloc->fd = fd;
    alloc->flag = 0;
    alloc->co = _co_current;
    alloc->polling = 0;
    _co_list_insert(_co_socket_list, &alloc->link);

    return alloc;

}

static void _co_socket_destroy(_co_socket_t *cosockfd) {
    free(cosockfd);
}

void _co_socket_flag_set(_co_socket_t *cosockfd, int flag) {
    if(_co_socket_flag_verify(flag)) {
        cosockfd->flag |= (1 << flag);
    }
}

int _co_socket_flag_get(_co_socket_t *cosockfd, int flag) {
    if(_co_socket_flag_verify(flag)) {
        return cosockfd->flag & (1 << flag);
    }
}

void _co_socket_flag_unset(_co_socket_t *cosockfd, int flag) {
    if(_co_socket_flag_verify(flag)) {
        cosockfd->flag &= (~(1 << flag));
    }
}

int _co_socket_polling_get(_co_socket_t *cosockfd) {
    return cosockfd->polling;
}

void _co_socket_polling_set(_co_socket_t *cosockfd) {
    cosockfd->polling = 1;
}

void _co_socket_polling_unset(_co_socket_t *cosockfd) {
    cosockfd->polling = 0;
}

static int _co_socket_flag_verify(int flag) {
    switch(flag) {
        case _COSOCKET_READ_INDEX:
        case _COSOCKET_WRITE_INDEX:
            return 1;
        default:
            return 0;
    }
}

int co_bind(_co_socket_t *cosockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(cosockfd->fd, addr, addrlen);
}

int co_listen(_co_socket_t *cosockfd, int backlog) {
    return listen(cosockfd->fd, backlog);
}

int co_close(_co_socket_t *cosockfd) {
    int fd = cosockfd->fd;
    _co_socket_destroy(cosockfd);
    return close(fd);
}

_co_socket_t *co_accept(_co_socket_t *cosockfd, struct sockaddr *addr, socklen_t *addrlen) {

    int err;
    int fd = 0;
    _co_socket_t *alloc = NULL;

    cosockfd->co = _co_current;

    while(1) {
        if((fd = accept(cosockfd->fd, addr, addrlen)) < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                _co_socket_flag_set(cosockfd, _COSOCKET_READ_INDEX);
                // trigger to switch the context
                if(_co_current) {
                    _co_current->state = _COROUTINE_STATE_IO_WAITING;
                    _co_list_delete(&_co_current->link);
                    _co_list_insert(_co_scheduler->iowaitq, &_co_current->link);
                }
                _co_switch();
            } else if(errno == EINTR) {
                continue;
            } else {
                return NULL;
            }
        } else {
            if(fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
                return NULL;
            }
            if(!(alloc = (_co_socket_t *)malloc(sizeof(_co_socket_t)))) {
                errno = ENOMEM;
                return NULL;
            }
            alloc->fd = fd;
            alloc->flag = 0;
            alloc->co = _co_current;
            _co_list_insert(_co_socket_list, &alloc->link);
            return alloc;
        }
    }

    return NULL;

}

ssize_t co_read(co_socket_t *cosockfd, void *buf, size_t count) {

    ssize_t n;

    cosockfd->co = _co_current;

    while(1) {
        if((n = read(cosockfd->fd, buf, count)) < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                _co_socket_flag_set(cosockfd, _COSOCKET_READ_INDEX);
                // trigger to switch the context
                if(_co_current) {
                    _co_current->state = _COROUTINE_STATE_IO_WAITING;
                    _co_list_delete(&_co_current->link);
                    _co_list_insert(_co_scheduler->iowaitq, &_co_current->link);
                }
                _co_switch();
            } else if(errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        } else {
            return n;
        }
    }

    return -1;

}

ssize_t co_write(co_socket_t *cosockfd, const void *buf, size_t count) {

    ssize_t n;

    cosockfd->co = _co_current;

    while(1) {
        if((n = write(cosockfd->fd, buf, count)) < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                _co_socket_flag_set(cosockfd, _COSOCKET_WRITE_INDEX);
                // trigger to switch the context
                if(_co_current) {
                    _co_current->state = _COROUTINE_STATE_IO_WAITING;
                    _co_list_delete(&_co_current->link);
                    _co_list_insert(_co_scheduler->iowaitq, &_co_current->link);
                }
                _co_switch();
            } else if(errno == EINTR) {
                continue;
            } else {
                return -1;
            }
        } else {
            return n;
        }
    }

    return -1;

}
