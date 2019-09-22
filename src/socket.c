#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "socket.h"

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

    return alloc;

}

int co_bind(_co_socket_t *cosockfd, const struct sockaddr *addr, socklen_t addrlen) {

    return bind(cosockfd->fd, addr, addrlen);

}

int co_listen(_co_socket_t *cosockfd, int backlog) {

    return listen(cosockfd->fd, backlog);

}

int co_close(_co_socket_t *cosockfd) {

    return close(cosockfd->fd);

}

int co_accept(_co_socket_t *cosockfd) {

    return 0;

}
