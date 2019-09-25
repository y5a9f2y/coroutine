.PHONY: default

default:
	@mkdir -p build/include
	@cp src/coroutine.h build/include
	@cd build && cmake .. && make

release: default
	@mkdir -p output/coroutine/lib
	@cp build/include/coroutine.h output/coroutine
	@cp -R build/lib/libcoroutine.a output/coroutine/lib

test:
	@mkdir -p third/googletest/build
	@cd third/googletest/build && cmake .. && make
	@mkdir -p utest/build/include
	@ln -s $(shell pwd)/src utest/build/include/coroutine
	@cd utest/build && cmake .. && make
	@./utest/build/bin/coroutine_test
	@cd utest && sh ./coverage.sh

clean:
	@if [ -d "build" ]; then rm -rf build; fi
	@if [ -d "output" ]; then rm -rf output; fi
	@cd third/googletest && if [ -d "build" ]; then rm -rf build; fi
	@cd utest && if [ -d "build" ]; then rm -rf build; fi

