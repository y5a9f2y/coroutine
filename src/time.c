#include <errno.h>
#include <sys/time.h>

#include "time.h"
#include "sched.h"

_co_time_t co_get_current_time() {
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
    if((now = co_get_current_time()) < 0) {
        return -1;
    }

    if(t + now <= 0) {
        errno = EOVERFLOW;
        return -1;
    }

    if(_co_current) {
        _co_current->state = _COROUTINE_STATE_SLEEPING;
        _co_list_delete(&_co_current->link);
    } else {
        _co_scheduler->state = _COROUTINE_STATE_SLEEPING;
    }
    _co_time_heap_insert(_co_scheduler->sleepq, t + now, _co_current);

    // trigger to switch the context
    _co_switch();

    return 0;

}
