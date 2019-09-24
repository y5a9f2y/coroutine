#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/ds.h"
}

TEST(ListTest, Create) {
    _co_list_t *l = ::_co_list_create();
    ASSERT_FALSE(!l);
    ASSERT_EQ(l, l->next);
    ASSERT_EQ(l, l->prev);
}
