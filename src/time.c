#include <errno.h>
#include <sys/time.h>

#include "time.h"

_co_time_t _co_get_current_time() {
    struct timeval tv;
    if(gettimeofday(&tv, NULL) < 0) {
        return -1;
    }
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

int co_usleep(_co_time_t t) {

    if(t < 0) {
        errno = EINVAL;
        return -1;
    }

    if(t == 0) {
        return 0;
    }

    _co_time_t now;
    if((now = _co_get_current_time()) < 0) {
        return -1;
    }

    if(t + now <= 0) {
        errno = EOVERFLOW;
        return -1;
    }


    return 0;
}
