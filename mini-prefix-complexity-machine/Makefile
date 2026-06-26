CC := gcc
CFLAGS := -Wall -Wextra -O2 -I./include -std=c11
AR := ar

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
LIB := build/libprefix.a

all: $(LIB)

$(LIB): $(OBJ)
	@mkdir -p build
	$(AR) rcs $@ $^

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

test: $(LIB)
	@echo "===== Running Tests ====="
	@for t in tests/*.c; do \
		if [ -f "$$t" ]; then \
			echo "--- $$t ---"; \
			$(CC) $(CFLAGS) "$$t" -Lbuild -lprefix -o build/test_$$(basename $$t .c) -lm && \
			./build/test_$$(basename $$t .c); \
		fi; \
	done

examples: $(LIB)
	@echo "===== Building Examples ====="
	@for e in examples/*.c; do \
		if [ -f "$$e" ]; then \
			$(CC) $(CFLAGS) "$$e" -Lbuild -lprefix -o build/ex_$$(basename $$e .c) -lm; \
		fi; \
	done

clean:
	rm -rf build/

distclean: clean
	rm -f tests/*.exe

.PHONY: all test clean distclean examples
