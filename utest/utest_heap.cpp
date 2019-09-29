#include <iostream>
#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/coroutine.h"
    #include "coroutine/sched.h"
    #include "coroutine/ds.h"
}

TEST(HeapTest, Life) {

    ASSERT_FALSE(co_framework_init());

    _co_time_heap_t *h = _co_time_heap_create();
    ASSERT_TRUE(h);
    ASSERT_TRUE(h->cap >= h->size);
    ASSERT_EQ(h->size, 0);
    ASSERT_TRUE(h->heap);

    co_thread_t *carr[11];
    for(int i = 10; i > 0; --i) {
        carr[i] = coroutine_create(NULL, NULL);
        ASSERT_FALSE(_co_time_heap_insert(h, i, carr[i]));
    }
    ASSERT_FALSE(_co_time_heap_insert(h, 0, NULL));

    for(size_t i = 0; i <= 10; ++i) {
        if(h->heap[i].co) {
            ASSERT_TRUE(h->heap[i].co->tlelink == &h->heap[i]);
        }
    }

    _co_time_heap_node_t node = _co_time_heap_top(h);
    ASSERT_FALSE(node.co);
    ASSERT_EQ(node.timeout, 0);
    node = _co_time_heap_delete(h, h->heap);
    ASSERT_FALSE(node.co);
    ASSERT_EQ(node.timeout, 0);

    for(int i = 1; i <= 10; ++i) {
        node = _co_time_heap_delete(h, h->heap);
        ASSERT_TRUE(node.co);
        ASSERT_TRUE(node.co == carr[i]);
        ASSERT_EQ(node.timeout, i);
        for(size_t j = 0; j < h->size; ++ j) {
            if(h->heap[j].co) {
                ASSERT_TRUE(h->heap[j].co->tlelink == &h->heap[j]);
            }
        }
    }

    ASSERT_TRUE(_co_time_heap_empty(h));

    for(int i = 10; i > 0; --i) {
        ASSERT_FALSE(_co_time_heap_insert(h, i, carr[i]));
    }
    ASSERT_FALSE(_co_time_heap_insert(h, 0, NULL));
    node = _co_time_heap_delete(h, carr[5]->tlelink);
    ASSERT_TRUE(node.co);
    ASSERT_TRUE(node.co == carr[5]);
    ASSERT_EQ(node.timeout, 5);
    for(int i = 0; i < 10; ++i) {
        node = _co_time_heap_delete(h, h->heap);
        if(i == 0) {
            ASSERT_FALSE(node.co);
            ASSERT_EQ(node.timeout, 0);
        } else if(i < 5) {
            ASSERT_TRUE(node.co);
            ASSERT_TRUE(node.co == carr[i]);
            ASSERT_EQ(node.timeout, i);
        } else {
            ASSERT_TRUE(node.co);
            ASSERT_TRUE(node.co == carr[i+1]);
            ASSERT_EQ(node.timeout, i+1);
        }
    }

    _co_time_heap_destroy(h);
    ASSERT_FALSE(co_framework_destroy());

}
