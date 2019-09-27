#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coroutine/coroutine.h"

void *print(void *arg) {
    int p = *(int *)arg;
    fprintf(stdout, "the argument of print is %d\n", p);
    return NULL;
}

int main(int argc, char *argv[]) {

    int err = 0;

    co_thread_t *c1 = NULL;
    co_thread_t *c2 = NULL;
    int arg1 = 1;
    int arg2 = 2;

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "initialize the coroutine framework error: %s\n", strerror(errno));
        exit(-1);
    }

    if(!(c1 = coroutine_create(print, (void *)&arg1))) {
        fprintf(stderr, "create the coroutine 1 error\n");
        exit(-1);
    }

    if(!(c2 = coroutine_create(print, (void *)&arg2))) {
        fprintf(stderr, "create the coroutine 2 error\n");
        exit(-1);
    }

    coroutine_join(c1, NULL);
    coroutine_join(c2, NULL);

    co_framework_destroy();

    return 0;

}
