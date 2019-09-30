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

#define DEFAULT_BUFFER_SIZE 1024

struct cargs {
    co_socket_t *sockfd;
    int v;
    const char *msg;
};

void *work(void *args) {

    size_t i;
    struct cargs *p = (struct cargs *)args;
    struct sockaddr_in addr;
    size_t nleft;
    size_t nwrite;
    size_t len = strlen(p->msg);
    size_t nread;
    char buf[DEFAULT_BUFFER_SIZE];

    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    if(!inet_aton("127.0.0.1", &addr.sin_addr)) {
        fprintf(stderr, "coroutine %d transform the ip address to in_addr error\n", p->v);
        return NULL;
    }

    if(co_connect(p->sockfd, (const struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "coroutine connect to 127.0.0.1:9000 error: %s\n", strerror(errno));
        return NULL;
    }

    for(i = 0; i < 10; ++ i) {
        nleft = len;
        while(nleft) {
            nwrite = co_write(p->sockfd, (const void *)(p->msg + len - nleft), nleft);
            if(nwrite < 0) {
                fprintf(stderr, "coroutine %d write error: %s\n", p->v, strerror(errno));
                return NULL;
            } else {
                nleft -= nwrite;
            }
        }
        nread = co_read(p->sockfd, (void *)buf, DEFAULT_BUFFER_SIZE);
        if(nread < 0) {
            fprintf(stderr, "coroutine %d read error: %s\n", p->v, strerror(errno));
            return NULL;
        } else if (nread == 0) {
            fprintf(stderr, "coroutine %d find peer close\n");
            return NULL;
        }
        write(STDOUT_FILENO, (const void *)buf, nread);
    }

    return NULL;

}

int main(int argc, char *argv[]) {

    size_t i;
    int err;
    co_socket_t *sockfd[2];
    co_thread_t *co[2];
    struct cargs args[2];

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "initialize the coroutine framework error: %s\n", strerror(err));
        exit(-1);
    }

    for(i = 0; i < 2; ++i) {
        if(!(sockfd[i] = co_socket(AF_INET, SOCK_STREAM, 0))) {
            fprintf(stderr, "create the socket descriptor %d error: %s\n", i, strerror(errno));
            exit(-1);
        }
        args[i].sockfd = sockfd[i];
        args[i].v = i;
        if(i == 0) {
            args[i].msg = "aaaaa\n";
        } else {
            args[i].msg = "bbbbb\n";
        }
        if(!(co[i] = coroutine_create(work, &args[i]))) {
            fprintf(stderr, "create the coroutine %d error\n", i);
            exit(-1);
        }
    }

    for(i = 0; i < 2; ++i) {
        coroutine_join(co[i], NULL);
    }

    for(i = 0; i < 2; ++i) {
        co_close(sockfd[i]);
    }

    if(co_framework_destroy() < 0) {
        fprintf(stderr, "teardown the coroutine framework error: %s\n", strerror(errno));
        exit(-1);
    }

    return 0;

}
