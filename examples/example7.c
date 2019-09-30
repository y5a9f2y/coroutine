#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "coroutine/coroutine.h"

void *work(void *args) {
    size_t i;
    int *p = (int *)args;
    for(i = 0; i < 10; ++ i) {
        fprintf(stdout, "coroutine %d running\n", *p);
        co_usleep(1000000);
    }
}

int main(int argc, char *argv[]) {

    int err;
    size_t i;
    co_thread_t *co[2];
	int args[2];

    if((err = co_framework_init()) < 0) {
        fprintf(stderr, "initialize the coroutine framework error: %s\n", strerror(errno));
        exit(-1);
    }

    for(i = 0; i < 2; ++i) {
		args[i] = i;
        if(!(co[i] = coroutine_create(work, &args[i]))) {
            fprintf(stderr, "create the coroutine %d error\n", i);
            exit(-1);
        }
    }
    
    for(i = 0; i < 2; ++i) {
        coroutine_join(co[i], NULL);
    }

    if(co_framework_destroy() < 0) {
        fprintf(stderr, "teardown the coroutine framework error\n");
        exit(-1);
    }

    return 0;

}
