#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coroutine/coroutine.h"

struct cargs {
    co_mutex_t *mu;
    int v;
};

void *work(void *args) {
    size_t i;
    struct cargs *p = (struct cargs *)args;
    co_mutex_lock(p->mu);
    for(i = 0; i < 10; ++ i) {
        fprintf(stdout, "coroutine %d running\n", p->v);
        co_usleep(1000000);
    }
    co_mutex_unlock(p->mu);
}

int main(int argc, char *argv[]) {

    int err;
    size_t i;
    co_thread_t *co[2];
    struct cargs args[2];
    co_mutex_t *mu;

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "initialize the coroutine framework error: %s\n", strerror(errno));
        exit(-1);
    }

    if(!(mu = co_mutex_create())) {
        fprintf(stderr, "create the mutex error\n");
        exit(-1);
    }

    for(i = 0; i < 2; ++i) {
        args[i].mu = mu;
        args[i].v = i;
        if(!(co[i] = coroutine_create(work, &args[i]))) {
            fprintf(stderr, "create the coroutine %d error\n", i);
            exit(-1);
        }
    }
    
    for(i = 0; i < 2; ++i) {
        coroutine_join(co[i], NULL);
    }

    if(co_mutex_destroy(mu) < 0) {
        fprintf(stderr, "destroy the mutex error: %s\n", strerror(errno));
    }

    if(co_framework_destroy() < 0) {
        fprintf(stderr, "teardown the coroutine framework error\n");
        exit(-1);
    }

    return 0;

}
