#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coroutine/coroutine.h"

struct coargs {
    co_cond_t *cond;
    int v;
};

void *work(void *args) {
    struct coargs *p = (struct coargs *)args;
    fprintf(stdout, "coroutine %d waits for executing\n", p->v);
    co_cond_wait(p->cond);
    fprintf(stderr, "coroutine %d executing\n", p->v);
    return NULL;
}

int main(int argc, char *argv[]) {

    int err;
    size_t i;
    co_thread_t *co[3];
    co_cond_t *cond;
    struct coargs args[3];

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "initialize the coroutine framework error: %s\n", strerror(err));
        exit(-1);
    }

    if(!(cond = co_cond_create())) {
        fprintf(stderr, "create the condition variable error: %s\n", strerror(errno));
        exit(-1);
    }

    for(i = 0; i < 3; ++ i) {
        args[i].v = i;
        args[i].cond = cond;
        if(!(co[i] = coroutine_create(work, &args[i]))) {
            fprintf(stderr, "create the coroutine %d error\n", i);
            exit(-1);
        }
    }

    coroutine_force_schedule();
    co_cond_broadcast(cond);

    for(i = 0; i < 3; ++i) {
        coroutine_join(co[i], NULL);
    }

    if(!co_cond_destroy(cond) < 0) {
        fprintf(stderr, "destroy the condition variable error: %s\n", strerror(errno));
    }

    if(co_framework_destroy() < 0) {
        fprintf(stderr, "teardown the coroutine framework error: %s\n", strerror(errno));
        exit(-1);
    }

    return 0;

}
