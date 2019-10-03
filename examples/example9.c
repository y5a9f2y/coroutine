#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "coroutine/coroutine.h"

#define PROXY_LISTEN_IP "0.0.0.0"
#define PROXY_LISTEN_PORT 9001
#define TARGET_LISTEN_IP "127.0.0.1"
#define TARGET_LISTEN_PORT 9000
#define DEFAULT_PROXY_BACKLOG 4096
#define PROXY_BUFFER_SIZE 4096

struct cargs {
    co_socket_t *from;
    co_socket_t *to;
};

void *pass(void *args) {

    ssize_t nread;
    ssize_t nwrite;
    ssize_t nleft;
    char buffer[PROXY_BUFFER_SIZE];
    struct cargs *p = (struct cargs *)args;

    while(1) {
        if((nread = co_read(p->from, (void *)buffer, PROXY_BUFFER_SIZE)) <= 0) {
            co_close(p->from);
            co_close(p->to);
            return NULL;
        }
        nleft = nread;
        while(nleft) {
            if((nwrite = co_write(p->to, (const void *)(buffer + nread - nleft), nleft)) < 0) {
                co_close(p->from);
                co_close(p->to);
                return NULL;
            }
            nleft -= nwrite;
        }
    }

    return NULL;

}

void *work(void *args) {

    size_t i;

    struct sockaddr_in addr;
    co_socket_t *target;

    co_thread_t *c[2];
    struct cargs a[2];


    if(!(target = co_socket(AF_INET, SOCK_STREAM, 0))) {
        fprintf(stderr, "create the target socket error: %s\n");
        return NULL;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(TARGET_LISTEN_PORT);
    if(!inet_aton(TARGET_LISTEN_IP, &addr.sin_addr)) {
        fprintf(stderr, "transfrom the target ip address %s to sin_addr error\n", TARGET_LISTEN_IP);
        return NULL;
    }

    if(co_connect(target, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "connect to %s:%d error: %s\n",
            TARGET_LISTEN_IP, TARGET_LISTEN_PORT, strerror(errno));
        return NULL;
    }

    a[0].from = (co_socket_t *)args;
    a[0].to = target;
    a[1].from = a[0].to;
    a[1].to = a[0].from;

    for(i = 0; i < 2; ++i) {
        if(!(c[i] = coroutine_create(pass, (void *)&a[i]))) {
            fprintf(stderr, "create the pass routine %d error: %s\n", i, strerror(errno));
            return NULL;
        }
    }

    for(i = 0; i < 2; ++i) {
        coroutine_join(c[i], NULL);
    }

    return NULL;

}

int main(int argc, char *argv) {

    co_socket_t *listen_fd;
    struct sockaddr_in listen_addr;
    int err;

    co_socket_t *conn;
    struct sockaddr_in conn_addr;
    socklen_t conn_addrlen = sizeof(conn_addr);

    co_thread_t *c;

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "initialize the coroutine framework error: %s\n", strerror(err));
        exit(-1);
    }


    if(!(listen_fd = co_socket(AF_INET, SOCK_STREAM, 0))) {
        fprintf(stderr, "create the listen socket error: %s\n", strerror(errno));
        exit(-1);
    }

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(PROXY_LISTEN_PORT);
    if(!inet_aton(PROXY_LISTEN_IP, &listen_addr.sin_addr)) {
        fprintf(stderr, "transform the bind ip address %s to sin_addr error\n", PROXY_LISTEN_IP);
        exit(-1);
    }

    if(co_bind(listen_fd, (const struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        fprintf(stderr, "bind %s:%d error: %s\n", PROXY_LISTEN_IP,
            PROXY_LISTEN_PORT, strerror(errno));
        exit(-1);
    }

    if(co_listen(listen_fd, DEFAULT_PROXY_BACKLOG) < 0) {
        fprintf(stderr, "listen %s:%d error: %s\n", PROXY_LISTEN_IP,
            PROXY_LISTEN_PORT, strerror(errno));
        exit(-1);
    }

    while(1) {
        if(!(conn = co_accept(listen_fd, (struct sockaddr *)&conn_addr, &conn_addrlen)) < 0) {
            fprintf(stderr, "accept a new connection error: %s\n", strerror(errno));
            continue;
        } else {
            fprintf(stderr, "recieve a connection from %s:%d\n",
                inet_ntoa(conn_addr.sin_addr), ntohs(conn_addr.sin_port));
        }
        if(!(c = coroutine_create(work, (void *)conn))) {
            fprintf(stderr, "create a new coroutine error: %s\n", strerror(errno));
            continue;
        }
        coroutine_setdetachstate(c, COROUTINE_FLAG_NONJOINABLE);
    }


    if(!co_framework_destroy()) {
        fprintf(stderr, "teardown the coroutine framework error: %s\n", strerror(errno));
        exit(-1);
    }

}

