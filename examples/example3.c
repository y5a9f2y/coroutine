#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coroutine/coroutine.h"

void *print1(void *args) {

    int *p = (int *)args;
    fprintf(stdout, "the argument of print1 is: %d\n", *p);
    *p = 0xf;
    coroutine_exit(args);
    return NULL;

}

void *print2(void *args) {

    void *result;
    co_thread_t *co = (co_thread_t *)args;
    fprintf(stdout, "wait for the coroutine 1 to execute\n");
    coroutine_join(co, &result);
    fprintf(stdout, "finish to wait for the coroutine 1 to execute, the return value is %d\n",
        *(int *)result);
    return NULL;

}

int main(int argc, char *argv[]) {

    int err;
    int args = 0xa;
    co_thread_t *c1;
    co_thread_t *c2;

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "init the coroutine framework error: %s\n", strerror(err));
        exit(-1);
    }

    if(!(c1 = coroutine_create(print1, (void *)&args))) {
        fprintf(stderr, "create coroutine 1 error\n");
        exit(-1);
    }

    if(!(c2 = coroutine_create(print2, (void *)c1))) {
        fprintf(stderr, "create coroutine 2 error\n");
        exit(-1);
    }

    coroutine_join(c2, NULL);

    co_framework_destroy();

    return 0;

}
