#include <sys/types.h>
#include <sys/socket.h>

#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/coroutine.h"
    #include "coroutine/event.h"
}

TEST(EventTest, InitAndDispatch) {

    ASSERT_FALSE(co_framework_init());

    ASSERT_FALSE(_co_eventsys_init());
    ASSERT_EQ(_co_eventsys_dispatch(), 0);
    _co_eventsys_destroy();

    ASSERT_FALSE(_co_eventsys_init());
    co_socket_t *fd = co_socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_FALSE(!fd);
    ASSERT_EQ(_co_eventsys_dispatch(), 0);
    _co_eventsys_destroy();

    ASSERT_FALSE(co_framework_destroy());

}
