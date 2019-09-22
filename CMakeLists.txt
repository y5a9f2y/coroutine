cmake_minimum_required(VERSION 2.8)

project(coroutine)

set(CMAKE_VERBOSE_MAKEFILE on)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

file(GLOB_RECURSE COROUTINE_CORE src/*)
add_library(coroutine STATIC ${COROUTINE_CORE})
