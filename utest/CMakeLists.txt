cmake_minimum_required(VERSION 2.8)

project(coroutine_test)

set(CMAKE_VERBOSE_MAKEFILE on)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_CXX_FLAGS "-Wall -g -std=c++11")

include_directories("../third/googletest/include")
include_directories("./build/include")

file(GLOB_RECURSE COROUTINE_CORE ../src/*)
file(GLOB COROUTINE_TEST_CORE ./*)

add_executable(coroutine_test ${COROUTINE_TEST_CORE} ${COROUTINE_CORE})
target_link_libraries(coroutine_test
    ${PROJECT_SOURCE_DIR}/../third/googletest/build/lib/libgtest.a
    ${PROJECT_SOURCE_DIR}/../third/googletest/build/lib/libgtest_main.a
    pthread)
