.PHONY: help \
	configure-debug build-debug test-debug \
	configure-release build-release test-release \
	configure-relwithdebinfo build-relwithdebinfo test-relwithdebinfo \
	configure-asan build-asan test-asan \
	configure-ubsan build-ubsan test-ubsan \
	configure-msan build-msan test-msan \
	configure-tsan build-tsan test-tsan \
	run run-example \
	stress stress-debug stress-release stress-relwithdebinfo stress-asan stress-ubsan stress-msan stress-tsan \
	stress-determinism stress-determinism-debug stress-determinism-release stress-determinism-relwithdebinfo stress-determinism-asan stress-determinism-ubsan stress-determinism-msan stress-determinism-tsan \
	test-all clean clean-output

help:
	@echo "Flower Exchange Makefile targets"
	@echo ""
	@echo "Build and test:"
	@echo "  make build-debug         # cmake --build --preset build-debug"
	@echo "  make test-debug          # ctest --preset test-debug"
	@echo "  make build-release       # cmake --build --preset build-release"
	@echo "  make test-release        # ctest --preset test-release"
	@echo "  make build-relwithdebinfo       # cmake --build --preset build-relwithdebinfo"
	@echo "  make test-relwithdebinfo        # ctest --preset test-relwithdebinfo"
	@echo "  make build-asan          # cmake --build --preset build-asan"
	@echo "  make test-asan           # ctest --preset test-asan"
	@echo "  make build-ubsan         # cmake --build --preset build-ubsan"
	@echo "  make test-ubsan          # ctest --preset test-ubsan"
	@echo "  make build-msan          # cmake --build --preset build-msan"
	@echo "  make test-msan           # ctest --preset test-msan"
	@echo "  make build-tsan          # cmake --build --preset build-tsan"
	@echo "  make test-tsan           # ctest --preset test-tsan"
	@echo ""
	@echo "Run application:"
	@echo "  make run                 # launch REPL (build/src/flower_exchange)"
	@echo "  make run-example         # process input_files/example1_orders.csv"
	@echo ""
	@echo "Stress tests (name filtered):"
	@echo "  make stress              # run stress suite on debug/release/relwithdebinfo/asan/ubsan/msan/tsan"
	@echo "  make stress-determinism  # run 5-run determinism stress on all build variants"
	@echo "  make stress-debug        # run stress suite on debug only"
	@echo "  make stress-release      # run stress suite on release only"
	@echo "  make stress-relwithdebinfo # run stress suite on relwithdebinfo only"
	@echo "  make stress-asan         # run stress suite on asan only"
	@echo "  make stress-ubsan        # run stress suite on ubsan only"
	@echo "  make stress-msan         # run stress suite on msan only"
	@echo "  make stress-tsan         # run stress suite on tsan only"
	@echo ""
	@echo "Utilities:"
	@echo "  make test-all            # run debug/release/asan/ubsan/msan/tsan tests"
	@echo "  make clean               # remove build directories"
	@echo "  make clean-output        # remove generated CSV reports"

configure-debug:
	cmake --preset debug

build-debug: configure-debug
	cmake --build --preset build-debug

test-debug: build-debug
	ctest --preset test-debug

configure-release:
	cmake --preset release

build-release: configure-release
	cmake --build --preset build-release

test-release: build-release
	ctest --preset test-release

configure-relwithdebinfo:
	cmake --preset relwithdebinfo

build-relwithdebinfo: configure-relwithdebinfo
	cmake --build --preset build-relwithdebinfo

test-relwithdebinfo: build-relwithdebinfo
	ctest --preset test-relwithdebinfo

configure-asan:
	cmake --preset asan

build-asan: configure-asan
	cmake --build --preset build-asan

test-asan: build-asan
	ctest --preset test-asan

configure-ubsan:
	cmake --preset ubsan

build-ubsan: configure-ubsan
	cmake --build --preset build-ubsan

test-ubsan: build-ubsan
	ctest --preset test-ubsan

configure-msan:
	cmake --preset msan

build-msan: configure-msan
	cmake --build --preset build-msan

test-msan: build-msan
	ctest --preset test-msan

configure-tsan:
	cmake --preset tsan

build-tsan: configure-tsan
	cmake --build --preset build-tsan

test-tsan: build-tsan
	ctest --preset test-tsan

run: build-release
	./build-release/src/flower_exchange

run-example: build-release
	printf "PROCESS input_files/example1_orders.csv\nQUIT\n" | ./build-release/src/flower_exchange

stress-debug: configure-debug build-debug
	FLOWER_EXCHANGE_RUN_STRESS=1 ctest --test-dir build --output-on-failure -R StressLargeFiles

stress-release: configure-release build-release
	FLOWER_EXCHANGE_RUN_STRESS=1 ctest --test-dir build-release --output-on-failure -R StressLargeFiles

stress-relwithdebinfo: configure-relwithdebinfo build-relwithdebinfo
	FLOWER_EXCHANGE_RUN_STRESS=1 ctest --test-dir build-relwithdebinfo --output-on-failure -R StressLargeFiles

stress-asan: configure-asan build-asan
	FLOWER_EXCHANGE_RUN_STRESS=1 ctest --test-dir build-asan --output-on-failure -R StressLargeFiles

stress-ubsan: configure-ubsan build-ubsan
	FLOWER_EXCHANGE_RUN_STRESS=1 ctest --test-dir build-ubsan --output-on-failure -R StressLargeFiles

stress-msan: configure-msan build-msan
	MSAN_OPTIONS=halt_on_error=0:exit_code=0 FLOWER_EXCHANGE_RUN_STRESS=1 ctest --test-dir build-msan --output-on-failure -R StressLargeFiles

stress-tsan: configure-tsan build-tsan
	FLOWER_EXCHANGE_RUN_STRESS=1 ctest --test-dir build-tsan --output-on-failure -R StressLargeFiles

stress: stress-debug stress-release stress-relwithdebinfo stress-asan stress-ubsan stress-msan stress-tsan

stress-determinism-debug: configure-debug build-debug
	FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 ctest --test-dir build --output-on-failure -R StressLargeFiles

stress-determinism-release: configure-release build-release
	FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 ctest --test-dir build-release --output-on-failure -R StressLargeFiles

stress-determinism-relwithdebinfo: configure-relwithdebinfo build-relwithdebinfo
	FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 ctest --test-dir build-relwithdebinfo --output-on-failure -R StressLargeFiles

stress-determinism-asan: configure-asan build-asan
	FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 ctest --test-dir build-asan --output-on-failure -R StressLargeFiles

stress-determinism-ubsan: configure-ubsan build-ubsan
	FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 ctest --test-dir build-ubsan --output-on-failure -R StressLargeFiles

stress-determinism-msan: configure-msan build-msan
	MSAN_OPTIONS=halt_on_error=0:exit_code=0 FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 ctest --test-dir build-msan --output-on-failure -R StressLargeFiles

stress-determinism-tsan: configure-tsan build-tsan
	FLOWER_EXCHANGE_RUN_STRESS_DETERMINISM=1 ctest --test-dir build-tsan --output-on-failure -R StressLargeFiles

stress-determinism: stress-determinism-debug stress-determinism-release stress-determinism-relwithdebinfo stress-determinism-asan stress-determinism-ubsan stress-determinism-msan stress-determinism-tsan

test-all: test-debug test-release test-relwithdebinfo test-asan test-ubsan test-msan test-tsan

clean:
	rm -rf build build-release build-relwithdebinfo build-asan build-ubsan build-msan build-tsan

clean-output:
	rm -f output/*_reports.csv
