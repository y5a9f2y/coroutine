#!/bin/bash


for line in `find build/CMakeFiles/coroutine_test.dir -name *.gcno`; do
    coverage=$(gcov -n -c "${line}")
    echo ${coverage}
done

