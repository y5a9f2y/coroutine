#include <errno.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "ds.h"
#include "socket.h"
#include "event.h"
#include "sched.h"

#define _CO_EVENTSYS_SIZE_HINT      (1048576)
#define _CO_EVENTSYS_BUFFER         (4096)
#define _CO_EVENTSYS_TIMEOUT        (10)

static int _co_eventsys_fd = 0;

int _co_eventsys_init() {
    if((_co_eventsys_fd = epoll_create(_CO_EVENTSYS_SIZE_HINT)) < 0) {
        return -1;
    }
    return 0;
}

int _co_eventsys_dispatch() {

    _co_list_t *p = NULL;
    _co_socket_t *cosockfd = NULL;
    int nevents = 0;
    struct epoll_event ev;
    struct epoll_event polling[_CO_EVENTSYS_BUFFER];
    int idx = 0;
    int pflag = 0;
    _co_time_heap_node_t node;
    _co_time_t now;

    int mod_read = 0;
    int mod_write = 0;
    int mod_to_delete = 0;
    int mod_to_add = 0;


    for(p = _co_socket_list->next; p != _co_socket_list; p = p->next) {

        cosockfd = _CO_SOCKET_PTR(p);

        mod_read = 0;
        mod_write = 0;
        mod_to_add = 0;

        if(!_co_socket_polling_get(cosockfd, _COSOCKET_READ_POLLING_INDEX) &&
            _co_socket_flag_get(cosockfd, _COSOCKET_READ_INDEX)) {
            mod_read = 1;
        }

        if(!_co_socket_polling_get(cosockfd, _COSOCKET_WRITE_POLLING_INDEX) &&
            _co_socket_flag_get(cosockfd, _COSOCKET_WRITE_INDEX)) {
            mod_write = 1;
        }

        if(mod_read && mod_write) {
            ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLOUT | EPOLLERR;
            _co_socket_polling_set(cosockfd, _COSOCKET_READ_POLLING_INDEX);
            _co_socket_polling_set(cosockfd, _COSOCKET_WRITE_POLLING_INDEX);
            mod_to_add = 1;
        } else if(mod_read) {
            if(_co_socket_flag_get(cosockfd, _COSOCKET_WRITE_INDEX)) {
                ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLOUT | EPOLLERR;
            } else {
                ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
                mod_to_add = 1;
            }
            _co_socket_polling_set(cosockfd, _COSOCKET_READ_POLLING_INDEX);
        } else if(mod_write) {
            if(_co_socket_flag_get(cosockfd, _COSOCKET_READ_INDEX)) {
                ev.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLOUT | EPOLLERR;
            } else {
                ev.events = EPOLLOUT | EPOLLERR; 
                mod_to_add = 1;
            }
            _co_socket_polling_set(cosockfd, _COSOCKET_WRITE_POLLING_INDEX);
        } else {
            if(_co_socket_polling_get(cosockfd, _COSOCKET_READ_POLLING_INDEX) ||
                _co_socket_polling_get(cosockfd, _COSOCKET_WRITE_POLLING_INDEX)) {
                pflag = 1;
            }
            continue;
        }

        ev.data.ptr = (void *)cosockfd;
        if(mod_to_add) {
            if(epoll_ctl(_co_eventsys_fd, EPOLL_CTL_ADD, cosockfd->fd, &ev) < 0) {
                return -1;
            }
        } else {
            if(epoll_ctl(_co_eventsys_fd, EPOLL_CTL_MOD, cosockfd->fd, &ev) < 0) {
                return -1;
            }
        }
        pflag = 1;

    }

    if(pflag) {
        while(1) {
            if((nevents = epoll_wait(_co_eventsys_fd, polling,
                _CO_EVENTSYS_BUFFER, _CO_EVENTSYS_TIMEOUT)) < 0) {
                switch(errno) {
                    case EINTR:
                        continue;
                    default:
                        return -1;
                }
            } else {

                for(idx = 0; idx < nevents; ++idx) {

                    mod_read = 0;
                    mod_write = 0;
                    mod_to_delete = 0;

                    cosockfd = (_co_socket_t *)(polling[idx].data.ptr);

                    if(polling[idx].events | (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                        if(cosockfd->rco) {
                            cosockfd->rco->state = _COROUTINE_STATE_READY;
                            _co_list_delete(&cosockfd->rco->link);
                            _co_list_insert(_co_scheduler->readyq, &cosockfd->rco->link);
                        } else {
                            _co_scheduler->state = _COROUTINE_STATE_READY;
                        }
                        _co_socket_flag_unset(cosockfd, _COSOCKET_READ_INDEX);
                        _co_socket_polling_unset(cosockfd, _COSOCKET_READ_POLLING_INDEX);
                        mod_read = 1;
                    }

                    if(polling[idx].events | (EPOLLOUT | EPOLLERR)) {
                        if(cosockfd->wco) {
                            cosockfd->wco->state = _COROUTINE_STATE_READY;
                            _co_list_delete(&cosockfd->wco->link);
                            _co_list_insert(_co_scheduler->readyq, &cosockfd->wco->link);
                        } else {
                            _co_scheduler->state = _COROUTINE_STATE_READY;
                        }
                        _co_socket_flag_unset(cosockfd, _COSOCKET_WRITE_INDEX);
                        _co_socket_polling_unset(cosockfd, _COSOCKET_WRITE_POLLING_INDEX);
                        mod_write = 1;
                    }

                    if(mod_read && mod_write) {
                        mod_to_delete = 1;
                    } else if(mod_read) {
                        if(_co_socket_flag_get(cosockfd, _COSOCKET_WRITE_INDEX)) {
                            ev.events |= (EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR);
                            ev.data.ptr = (void *)cosockfd;
                            if(epoll_ctl(_co_eventsys_fd, EPOLL_CTL_MOD, cosockfd->fd, &ev) < 0) {
                                return -1;
                            }
                        } else {
                            mod_to_delete = 1;
                        }
                    } else if(mod_write) {
                        if(_co_socket_flag_get(cosockfd, _COSOCKET_READ_INDEX)) {
                            ev.events |= (EPOLLOUT | EPOLLERR);
                            ev.data.ptr = (void *)cosockfd;
                            if(epoll_ctl(_co_eventsys_fd, EPOLL_CTL_MOD, cosockfd->fd, &ev) < 0) {
                                return -1;
                            }
                        } else {
                            mod_to_delete = 1;
                        }
                    }

                    if(mod_to_delete) {
                        if(epoll_ctl(_co_eventsys_fd, EPOLL_CTL_DEL, cosockfd->fd, NULL) < 0) {
                            return -1;
                        }
                    }

                }
                break;
            }
        }
    } else {
        if(!_co_time_heap_empty(_co_scheduler->sleepq) &&
            _co_scheduler->state != _COROUTINE_STATE_READY && 
            _co_scheduler->state != _COROUTINE_STATE_RUNNING) {
            node = _co_time_heap_top(_co_scheduler->sleepq);
            now = co_get_current_time();
            if(node.timeout > now) {
                usleep(node.timeout - now);
            }
        }
    }

    return 0;

}

void _co_eventsys_destroy() {
    close(_co_eventsys_fd);
}
