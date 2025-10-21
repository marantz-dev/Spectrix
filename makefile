build:
	cmake -B build -G Ninja 
	Ninja -C build
clean:
	rm -rf build
	rm -rf .cache
