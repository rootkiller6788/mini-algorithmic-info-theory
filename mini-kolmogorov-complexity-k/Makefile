CC = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
AR = ar

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,build/%.o,$(SRC))
LIB = build/libkolmogorov.a

EXAMPLES_SRC = $(wildcard examples/*.c)
EXAMPLES_BIN = $(patsubst examples/%.c,build/example_%,$(EXAMPLES_SRC))

BENCH_SRC = $(wildcard benches/*.c)
BENCH_BIN = $(patsubst benches/%.c,build/bench_%,$(BENCH_SRC))

all: $(LIB) tests/run_tests examples benches

$(LIB): $(OBJ)
	@mkdir -p build
	$(AR) rcs $@ $^

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

tests/run_tests: tests/test_main.c $(LIB)
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lkolmogorov -lm

test: tests/run_tests
	cd tests && ./run_tests

examples: $(EXAMPLES_BIN)

build/example_%: examples/%.c $(LIB)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lkolmogorov -lm

benches: $(BENCH_BIN)

build/bench_%: benches/%.c $(LIB)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< -Lbuild -lkolmogorov -lm

clean:
	rm -rf build/ tests/run_tests

run-examples: examples
	@for bin in $(EXAMPLES_BIN); do echo "=== $$bin ===" && ./$$bin && echo ""; done

run-benches: benches
	@for bin in $(BENCH_BIN); do echo "=== $$bin ===" && ./$$bin && echo ""; done

.PHONY: all test clean examples benches run-examples run-benches
