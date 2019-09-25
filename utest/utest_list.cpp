#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/ds.h"
}

static void _list_test_delete_setup(_co_list_t *, _co_list_t *, _co_list_t *, _co_list_t *);

TEST(ListTest, Create) {
    _co_list_t *l = _co_list_create();
    ASSERT_FALSE(!l);
    ASSERT_EQ(l, l->next);
    ASSERT_EQ(l, l->prev);
    _co_list_destroy(l);
}

TEST(ListTest, Init) {
    _co_list_t l;
    l.next = l.prev = NULL;
    _co_list_init(&l);
    ASSERT_TRUE(&l == l.next);
    ASSERT_TRUE(&l == l.prev);
}

TEST(ListTest, Insert) {
    _co_list_t l;
    _co_list_t node;
    _co_list_init(&l);
    _co_list_insert(&l, &node);
    ASSERT_TRUE(l.next == &node);
    ASSERT_TRUE(l.prev == &node);
    ASSERT_TRUE(node.next == &l);
    ASSERT_TRUE(node.prev == &l);
}

TEST(ListTest, Delete) {

    _co_list_t l;
    _co_list_t node1;
    _co_list_t node2;
    _co_list_t node3;

    _list_test_delete_setup(&l, &node1, &node2, &node3);
    _co_list_delete(&node1);
    ASSERT_TRUE(l.next == &node2);
    ASSERT_TRUE(l.prev == &node3);
    ASSERT_TRUE(node2.prev == &l);
    ASSERT_TRUE(node2.next == &node3);

    _list_test_delete_setup(&l, &node1, &node2, &node3);
    _co_list_delete(&node2);
    ASSERT_TRUE(node1.prev == &l);
    ASSERT_TRUE(node1.next == &node3);
    ASSERT_TRUE(node3.prev == &node1);
    ASSERT_TRUE(node3.next == &l);

    _list_test_delete_setup(&l, &node1, &node2, &node3);
    _co_list_delete(&node3);
    ASSERT_TRUE(node2.next == &l);
    ASSERT_TRUE(node2.prev == &node1);
    ASSERT_TRUE(l.prev == &node2);
    ASSERT_TRUE(l.next == &node1);

}

TEST(ListTest, IsEmpty) {

    _co_list_t l;
    _co_list_t node1;

    _co_list_init(&l);
    ASSERT_EQ(::_co_list_empty(&l), 1);

    _co_list_insert(&l, &node1);
    ASSERT_EQ(::_co_list_empty(&l), 0);

}

static void _list_test_delete_setup(_co_list_t *l, _co_list_t *n1,
    _co_list_t *n2, _co_list_t *n3) {

    _co_list_init(l);
    _co_list_insert(l, n3);
    _co_list_insert(l, n2);
    _co_list_insert(l, n1);

}
