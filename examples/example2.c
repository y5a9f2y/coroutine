#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "coroutine/coroutine.h"

#define DEFAULT_ECHO_SERVER_IP "0.0.0.0"
#define DEFAULT_ECHO_SERVER_PORT 9000
#define DEFAULT_ECHO_SERVER_BACKLOG 1024
#define DEFAULT_ECHO_SERVER_BUFFER 4096

void *echo(void *args) {

    ssize_t nread;
    ssize_t nleft;
    ssize_t nwrite;
    co_socket_t *conn = (co_socket_t *)args;
    char buffer[DEFAULT_ECHO_SERVER_BUFFER];

    while(1) {

        if((nread = co_read(conn, (void *)buffer, DEFAULT_ECHO_SERVER_BUFFER)) <= 0) {
            co_close(conn);
            return NULL;
        }
        nleft = nread;
        while(nleft) {
            if((nwrite = co_write(conn, (const void *)(buffer + nread - nleft), nleft)) < 0) {
                co_close(conn);
                return NULL;
            }
            nleft -= nwrite;
        }

    }

    return NULL;

}

int main(int argc, char *argv[]) {

    int err = 0;
    co_socket_t *listen_fd;
    struct sockaddr_in listen_addr;
    co_socket_t *conn;
    struct sockaddr_in conn_addr;
    socklen_t conn_addrlen = sizeof(conn_addr);
    co_thread_t *c;

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "initialize the coroutine framework error: %s\n", strerror(errno));
        exit(-1);
    }

    if(!(listen_fd = co_socket(AF_INET, SOCK_STREAM, 0))) {
        fprintf(stderr, "create the listen socket error\n");
        exit(-1);
    }

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(DEFAULT_ECHO_SERVER_PORT);
    if(!inet_aton(DEFAULT_ECHO_SERVER_IP, &listen_addr.sin_addr)) {
        fprintf(stderr, "transform the bind ip address to in_addr structure error\n");
        exit(-1);
    }

    if(co_bind(listen_fd, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0) {
        fprintf(stderr, "bind the listen socket error: %s\n", strerror(errno));
        exit(-1);
    }

    if(co_listen(listen_fd, DEFAULT_ECHO_SERVER_BACKLOG) < 0) {
        fprintf(stderr, "listen to the echo server port %d error: %s\n",
            DEFAULT_ECHO_SERVER_PORT, strerror(errno));
        exit(-1);
    }

    while(1) {
        if(!(conn = co_accept(listen_fd, (struct sockaddr *)&conn_addr, &conn_addrlen))) {
            fprintf(stderr, "recieve a connection from the listen socket error: %s\n",
                strerror(errno));
            continue;
        } else {
            fprintf(stderr, "recieve a connection from %s:%d\n",
                inet_ntoa(conn_addr.sin_addr), ntohs(conn_addr.sin_port));
        }
        if(!(c = coroutine_create(echo, (void *)conn))) {
            fprintf(stderr, "create the a new coroutine for %s:%d error\n",
                inet_ntoa(conn_addr.sin_addr), ntohs(conn_addr.sin_port));
            continue;
        }
        coroutine_setdetachstate(c, COROUTINE_FLAG_NONJOINABLE);
    }


    co_framework_destroy();

    return 0;

}
