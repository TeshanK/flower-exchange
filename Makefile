configure:
	@ cmake -S . -B build

build: configure
	@ cmake --build build

build-server: configure
	@ cmake --build build --target server

build-client: configure
	@ cmake --build build --target client

run-server: build-server
	@ ./build/server/server

run-client: build-client
	@ ./build/client/client

clean:
	@ rm -rf build

.PHONY: configure build build-server build-client run-server run-client clean