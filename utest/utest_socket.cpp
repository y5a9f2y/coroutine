#include <sys/types.h>
#include <sys/socket.h>

#include "gtest/gtest.h"

extern "C" {
    #include "coroutine/socket.h"
}

TEST(SocketTest, Life) {

    ASSERT_FALSE(co_framework_init());

    co_socket_t *fd = co_socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_FALSE(!fd);

    ASSERT_TRUE(_CO_SOCKET_PTR(&fd->link) == fd);
    ASSERT_TRUE(!_co_list_empty(_co_socket_list));
    ASSERT_TRUE(_CO_SOCKET_PTR(_co_socket_list->next) == fd);

    co_close(fd);
    ASSERT_TRUE(!_co_list_empty(_co_socket_pool));
    ASSERT_TRUE(_CO_SOCKET_PTR(_co_socket_pool->next) == fd);

    co_socket_t *nfd = co_socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_FALSE(!nfd);
    ASSERT_TRUE(fd == nfd);
    ASSERT_TRUE(_co_list_empty(_co_socket_pool));
    ASSERT_TRUE(!_co_list_empty(_co_socket_list));

    _co_list_delete(&nfd->link);
    _co_socket_destroy(nfd);

    co_framework_destroy();

}

TEST(SocketTest, Property) {

    ASSERT_FALSE(co_framework_init());

    co_socket_t *fd = co_socket(AF_INET, SOCK_STREAM, 0);

    ASSERT_FALSE(_co_socket_polling_get(fd));
    _co_socket_polling_set(fd);
    ASSERT_TRUE(_co_socket_polling_get(fd));
    _co_socket_polling_unset(fd);
    ASSERT_FALSE(_co_socket_polling_get(fd));

    ASSERT_FALSE(_co_socket_flag_get(fd, _COSOCKET_READ_INDEX));
    _co_socket_flag_set(fd, _COSOCKET_READ_INDEX);
    ASSERT_TRUE(_co_socket_flag_get(fd, _COSOCKET_READ_INDEX));
    _co_socket_flag_unset(fd, _COSOCKET_READ_INDEX);
    ASSERT_FALSE(_co_socket_flag_get(fd, _COSOCKET_READ_INDEX));

    ASSERT_FALSE(_co_socket_flag_get(fd, 0xf));

    co_close(fd);

    co_framework_destroy();

}
