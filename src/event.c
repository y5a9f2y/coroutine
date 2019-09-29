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

    for(p = _co_socket_list->next; p != _co_socket_list; p = p->next) {
        cosockfd = _CO_SOCKET_PTR(p);
        if(_co_socket_polling_get(cosockfd)) {
            pflag = 1;
            continue;
        }
        ev.events = 0;
        if(_co_socket_flag_get(cosockfd, _COSOCKET_READ_INDEX)) {
            ev.events |= EPOLLIN;
            ev.events |= EPOLLHUP;
            ev.events |= EPOLLRDHUP;
        }
        if(_co_socket_flag_get(cosockfd, _COSOCKET_WRITE_INDEX)) {
            ev.events |= EPOLLOUT;
        }
        if(!ev.events) {
            continue;
        }
        ev.events |= EPOLLERR;
        ev.data.ptr = (void *)cosockfd;
        if(epoll_ctl(_co_eventsys_fd, EPOLL_CTL_ADD, cosockfd->fd, &ev) < 0) {
            return -1;
        }
        _co_socket_polling_set(cosockfd);
        pflag = 1;
    }

    if(pflag) {
        while(1) {
            if((nevents = epoll_wait(_co_eventsys_fd, polling, _CO_EVENTSYS_BUFFER, -1)) < 0) {
                switch(errno) {
                    case EINTR:
                        continue;
                    default:
                        return -1;
                }
            } else {
                for(idx = 0; idx < nevents; ++idx) {
                    cosockfd = (_co_socket_t *)(polling[idx].data.ptr);
                    _co_socket_flag_unset(cosockfd, _COSOCKET_READ_INDEX);
                    _co_socket_flag_unset(cosockfd, _COSOCKET_WRITE_INDEX);
                    if(cosockfd->co) {
                        cosockfd->co->state = _COROUTINE_STATE_READY;
                        _co_list_delete(&cosockfd->co->link);
                        _co_list_insert(_co_scheduler->readyq, &cosockfd->co->link);
                    }
                    if(epoll_ctl(_co_eventsys_fd, EPOLL_CTL_DEL, cosockfd->fd, NULL) < 0) {
                        return -1;
                    }
                    _co_socket_polling_unset(cosockfd);
                }
                break;
            }
        }
    } else {
        if(!_co_time_heap_empty(_co_scheduler->sleepq)) {
            node = _co_time_heap_top(_co_scheduler->sleepq);
            now = _co_get_current_time();
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
