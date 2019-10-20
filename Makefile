.PHONY: default

default:
	@mkdir -p build/include
	@cp src/coroutine.h build/include
	@cd build && cmake .. && make

release: default
	@mkdir -p output/coroutine/lib
	@cp build/include/coroutine.h output/coroutine
	@cp build/lib/libcoroutine.a output/coroutine/lib

test:
	@cd third && tar xzf googletest.tar.gz
	@mkdir -p third/googletest/build
	@cd third/googletest/build && cmake .. && make
	@mkdir -p utest/build/include
	@if [ ! -d "$(shell pwd)/utest/build/include/coroutine" ]; then \
		ln -s $(shell pwd)/src $(shell pwd)/utest/build/include/coroutine; \
	 fi
	@cd utest/build && cmake .. && make
	@./utest/build/bin/coroutine_test
	@cd utest && sh ./coverage.sh

clean:
	@if [ -d "build" ]; then rm -rf build; fi
	@if [ -d "output" ]; then rm -rf output; fi
	@cd third && if [ -d "googletest" ]; then rm -rf googletest; fi
	@cd utest && if [ -d "build" ]; then rm -rf build; fi

uninstall:
	@cd /usr/local/include && if [ -d "coroutine" ]; then rm -rf coroutine; fi
	@cd /usr/local/lib && if [ -f "libcoroutine.a" ]; then rm libcoroutine.a; fi

install:
	@mkdir -p /usr/local/include/coroutine
	@cp build/include/coroutine.h /usr/local/include/coroutine
	@cp build/lib/libcoroutine.a /usr/local/lib/libcoroutine.a
