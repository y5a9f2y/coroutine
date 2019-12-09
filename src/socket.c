#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"
#include "sched.h"

static void _co_socket_recyle(_co_socket_t *);
static int _co_socket_flag_verify(int);
static int _co_socket_polling_verify(int);

_co_socket_t *co_socket(int domain, int type, int protocol) {

    int fd;
    _co_socket_t *alloc;
    _co_list_t *next;

    if((fd = socket(domain, type, protocol)) < 0) {
        return NULL;
    }

    if(fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
        return NULL;
    }

    if(!_co_list_empty(_co_socket_pool)) {
        next = _co_list_delete(_co_socket_pool->next);
        alloc = _CO_SOCKET_PTR(next);
    } else {
        if(!(alloc = (_co_socket_t *)malloc(sizeof(_co_socket_t)))) {
            return NULL;
        }
    }

    alloc->fd = fd;
    alloc->flag = 0;
    alloc->rco = NULL;
    alloc->wco = NULL;
    alloc->polling = 0;
    _co_list_insert(_co_socket_list, &alloc->link);

    return alloc;

}

void _co_socket_destroy(_co_socket_t *cosockfd) {
    if(cosockfd) {
        free(cosockfd);
    }
}

static void _co_socket_recycle(_co_socket_t *cosockfd) {
    if(cosockfd) {
        _co_list_delete(&cosockfd->link);
        _co_list_insert(_co_socket_pool, &cosockfd->link);
    }
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
    return 0;
}

void _co_socket_flag_unset(_co_socket_t *cosockfd, int flag) {
    if(_co_socket_flag_verify(flag)) {
        cosockfd->flag &= (~(1 << flag));
    }
}

void _co_socket_polling_set(_co_socket_t *cosockfd, int flag) {
    if(_co_socket_polling_verify(flag)) {
        cosockfd->polling |= (1 << flag);
    }
}

int _co_socket_polling_get(_co_socket_t *cosockfd, int flag) {
    if(_co_socket_polling_verify(flag)) {
        return cosockfd->polling & (1 << flag);
    }
    return 0;
}

void _co_socket_polling_unset(_co_socket_t *cosockfd, int flag) {
    if(_co_socket_polling_verify(flag)) {
        cosockfd->polling &= (~(1 << flag));
    }
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

static int _co_socket_polling_verify(int flag) {
    switch(flag) {
        case _COSOCKET_READ_POLLING_INDEX:
        case _COSOCKET_WRITE_POLLING_INDEX:
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
    _co_socket_recycle(cosockfd);
    return close(fd);
}

int co_socket_get_fd(_co_socket_t *cosockfd) {
    return cosockfd->fd;
}

_co_socket_t *co_accept(_co_socket_t *cosockfd, struct sockaddr *addr, socklen_t *addrlen) {

    int err;
    int fd = 0;
    _co_socket_t *alloc = NULL;

    if(cosockfd->rco) {
        errno = EBUSY;
        return NULL;
    }

    cosockfd->rco = _co_current;

    while(1) {
        if((fd = accept(cosockfd->fd, addr, addrlen)) < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                _co_socket_flag_set(cosockfd, _COSOCKET_READ_INDEX);
                // trigger to switch the context
                if(_co_current) {
                    _co_current->state = _COROUTINE_STATE_IO_WAITING;
                    _co_list_delete(&_co_current->link);
                    _co_list_insert(_co_scheduler->iowaitq, &_co_current->link);
                } else {
                    _co_scheduler->state = _COROUTINE_STATE_IO_WAITING;
                }
                _co_switch();
            } else if(errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else {
            if(fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
                break;
            }
            if(!(alloc = (_co_socket_t *)malloc(sizeof(_co_socket_t)))) {
                errno = ENOMEM;
                break;
            }
            alloc->fd = fd;
            alloc->flag = 0;
            alloc->polling = 0;
            alloc->rco = NULL;
            alloc->wco = NULL;
            _co_list_insert(_co_socket_list, &alloc->link);
            cosockfd->rco = NULL;
            return alloc;
        }
    }

    cosockfd->rco = NULL;
    return NULL;

}

ssize_t co_read(_co_socket_t *cosockfd, void *buf, size_t len) {
    return co_recv(cosockfd, buf, len, 0);
}

ssize_t co_recv(_co_socket_t *cosockfd, void *buf, size_t len, int flags) {
    return co_recvfrom(cosockfd, buf, len, flags, NULL, NULL);
}

ssize_t co_recvfrom(_co_socket_t *cosockfd, void *buf, size_t len, int flags,
    struct sockaddr *addr, socklen_t *addrlen) {

    ssize_t n;

    if(cosockfd->rco) {
        errno = EBUSY;
        return -1;
    }

    cosockfd->rco = _co_current;

    while(1) {
        if((n = recvfrom(cosockfd->fd, buf, len, flags, addr, addrlen)) < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                _co_socket_flag_set(cosockfd, _COSOCKET_READ_INDEX);
                // trigger to switch the context
                if(_co_current) {
                    _co_current->state = _COROUTINE_STATE_IO_WAITING;
                    _co_list_delete(&_co_current->link);
                    _co_list_insert(_co_scheduler->iowaitq, &_co_current->link);
                } else {
                    _co_scheduler->state = _COROUTINE_STATE_IO_WAITING;
                }
                _co_switch();
            } else if(errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else {
            cosockfd->rco = NULL;
            return n;
        }
    }

    cosockfd->rco = NULL;
    return -1;

}

ssize_t co_write(_co_socket_t *cosockfd, const void *buf, size_t len) {
    return co_send(cosockfd, buf, len, 0);
}

ssize_t co_send(_co_socket_t *cosockfd, const void *buf, size_t len, int flags) {
    return co_sendto(cosockfd, buf, len, flags, NULL, 0);
}

ssize_t co_sendto(_co_socket_t *cosockfd, const void *buf, size_t len, int flags,
    const struct sockaddr *addr, socklen_t addrlen) {

    ssize_t n;

    if(cosockfd->wco) {
        errno = EBUSY;
        return -1;
    }

    cosockfd->wco = _co_current;

    while(1) {
        if((n = sendto(cosockfd->fd, buf, len, flags, addr, addrlen)) < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                _co_socket_flag_set(cosockfd, _COSOCKET_WRITE_INDEX);
                // trigger to switch the context
                if(_co_current) {
                    _co_current->state = _COROUTINE_STATE_IO_WAITING;
                    _co_list_delete(&_co_current->link);
                    _co_list_insert(_co_scheduler->iowaitq, &_co_current->link);
                } else {
                    _co_scheduler->state = _COROUTINE_STATE_IO_WAITING;
                }
                _co_switch();
            } else if(errno == EINTR) {
                continue;
            } else {
                break;
            }
        } else {
            cosockfd->wco = NULL;
            return n;
        }
    }

    cosockfd->wco = NULL;
    return -1;

}

int co_connect(_co_socket_t *cosockfd, const struct sockaddr *addr, socklen_t addrlen) {

    int err;
    int optval;
    int optlen;

    if(cosockfd->wco) {
        errno = EBUSY;
        return -1;
    }

    cosockfd->wco = _co_current;

    while(1) {
        if((err = connect(cosockfd->fd, addr, addrlen)) < 0) {
            if(errno == EINTR) {
                continue;
            } else if(errno == EINPROGRESS) {
                _co_socket_flag_set(cosockfd, _COSOCKET_WRITE_INDEX);
                // trigger to switch the context
                if(_co_current) {
                    _co_current->state = _COROUTINE_STATE_IO_WAITING;
                    _co_list_delete(&_co_current->link);
                    _co_list_insert(_co_scheduler->iowaitq, &_co_current->link);
                } else {
                    _co_scheduler->state = _COROUTINE_STATE_IO_WAITING;
                }
                _co_switch();
            } else if(errno == EISCONN) {
                break;
            } else {
                cosockfd->wco = NULL;
                return -1;
            }
        } else {
            break;
        }
    }
    optlen = sizeof(optval);
    if(getsockopt(cosockfd->fd, SOL_SOCKET, SO_ERROR, (void *)&optval, &optlen) < 0) {
        cosockfd->wco = NULL;
        return -1;
    }
    if(optval) {
        errno = optval;
        cosockfd->wco = NULL;
        return -1;
    }

    cosockfd->wco = NULL;
    return 0;

}
