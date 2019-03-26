build/Makefile:
	mkdir -p build
	cd build; cmake -Wno-dev ..

format: build/Makefile
	cd build; make format
