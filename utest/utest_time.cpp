#include <errno.h>

#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/coroutine.h"
    #include "coroutine/time.h"
}

TEST(TimeTest, Get) {
    _co_time_t t1 = co_get_current_time();
    _co_time_t t2 = co_get_current_time();
    ASSERT_TRUE(t2 >= t1);
}

TEST(TimeTest, Sleep) {
    ASSERT_EQ(co_usleep(-1), -1);
    ASSERT_EQ(errno, EINVAL);
    ASSERT_EQ(co_usleep(0), 0);
    ASSERT_EQ(co_usleep((_co_time_t)(9223372036854775807L)), -1);
    ASSERT_EQ(errno, EOVERFLOW);
    ASSERT_FALSE(co_framework_init());
    ASSERT_EQ(co_usleep(2000000), 0);
    ASSERT_FALSE(co_framework_destroy());
}
