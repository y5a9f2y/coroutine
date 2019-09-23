.PHONY: default

default:
	@mkdir -p build/include
	@cd build && cmake .. && make

release: default
	@mkdir output
	@cp src/coroutine.h build/include
	@cp -R build/include output/
	@cp -R build/lib output/

clean:
	@if [ -d "build" ]; then rm -rf build; fi
	@if [ -d "output" ]; then rm -rf output; fi

