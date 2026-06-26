# Makefile — mini-algorithmic-randomness-ml
# Algorithmic Randomness meets Machine Learning
#
# Combines Martin-Löf randomness tests with neural network methods
# for practical estimation of incomputable quantities (K(x), ML-randomness).
#
# Reference: Nies (2009), Downey & Hirschfeldt (2010), Goodfellow et al. (2014)

CC       = gcc
CFLAGS   = -Wall -Wextra -O2 -std=c99
INC      = -I./include
LDFLAGS  = -lm
AR       = ar
ARFLAGS  = rcs

SRC_DIR  = src
INC_DIR  = include
TST_DIR  = tests
EXM_DIR  = examples
BLD_DIR  = build
DOC_DIR  = docs
BNC_DIR  = benches

# Core module objects
CORE_OBJS = $(BLD_DIR)/randomness_core.o \
            $(BLD_DIR)/randomness_ml.o \
            $(BLD_DIR)/compression_nn.o \
            $(BLD_DIR)/gan_randomness.o

LIB       = $(BLD_DIR)/libalgorithmic_randomness.a

# Test targets
TEST_BINS = $(BLD_DIR)/test_randomness

# Example targets
EXM_SRCS  = $(wildcard $(EXM_DIR)/*.c)
EXM_BINS  = $(patsubst $(EXM_DIR)/%.c,$(BLD_DIR)/example_%,$(EXM_SRCS))

.PHONY: all clean test tests examples lib build_dir

all: build_dir lib examples tests

build_dir:
	@mkdir -p $(BLD_DIR)

# ── Library ─────────────────────────────────────────────────────
lib: $(LIB)

$(LIB): $(CORE_OBJS)
	$(AR) $(ARFLAGS) $@ $^

$(BLD_DIR)/randomness_core.o: $(SRC_DIR)/randomness_core.c $(INC_DIR)/randomness.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BLD_DIR)/randomness_ml.o: $(SRC_DIR)/randomness_ml.c $(INC_DIR)/randomness_ml.h $(INC_DIR)/randomness.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BLD_DIR)/compression_nn.o: $(SRC_DIR)/compression_nn.c $(INC_DIR)/compression_nn.h $(INC_DIR)/randomness.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BLD_DIR)/gan_randomness.o: $(SRC_DIR)/gan_randomness.c $(INC_DIR)/gan_randomness.h $(INC_DIR)/randomness.h
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# ── Self-test binaries (each .c has a #ifdef MAIN block) ─────────
test: tests

tests: $(TEST_BINS)
	@echo "=== Running tests ==="
	./$(BLD_DIR)/test_randomness || exit 1

$(BLD_DIR)/test_randomness: $(TST_DIR)/test_randomness.c $(CORE_OBJS)
	$(CC) $(CFLAGS) $(INC) $< $(CORE_OBJS) -o $@ $(LDFLAGS)

$(BLD_DIR)/test_%: $(TST_DIR)/%.c $(CORE_OBJS)
	$(CC) $(CFLAGS) $(INC) $< $(CORE_OBJS) -o $@ $(LDFLAGS)

# Standalone module tests (each src file with -Dxxx_MAIN)
$(BLD_DIR)/test_standalone_randomness_core: $(SRC_DIR)/randomness_core.c
	$(CC) $(CFLAGS) $(INC) -DRANDOMNESS_CORE_MAIN $< -o $@ $(LDFLAGS)

$(BLD_DIR)/test_standalone_randomness_ml: $(SRC_DIR)/randomness_ml.c $(SRC_DIR)/randomness_core.c
	$(CC) $(CFLAGS) $(INC) -DRANDOMNESS_ML_MAIN $^ -o $@ $(LDFLAGS)

$(BLD_DIR)/test_standalone_compression_nn: $(SRC_DIR)/compression_nn.c $(SRC_DIR)/randomness_core.c
	$(CC) $(CFLAGS) $(INC) -DCOMPRESSION_NN_MAIN $^ -o $@ $(LDFLAGS)

$(BLD_DIR)/test_standalone_gan_randomness: $(SRC_DIR)/gan_randomness.c $(SRC_DIR)/randomness_core.c $(SRC_DIR)/randomness_ml.c
	$(CC) $(CFLAGS) $(INC) -DGAN_RANDOMNESS_MAIN $^ -o $@ $(LDFLAGS)

standalone_tests: \
	$(BLD_DIR)/test_standalone_randomness_core \
	$(BLD_DIR)/test_standalone_randomness_ml \
	$(BLD_DIR)/test_standalone_compression_nn \
	$(BLD_DIR)/test_standalone_gan_randomness
	@echo "=== Running standalone: randomness_core ===" && ./$(BLD_DIR)/test_standalone_randomness_core
	@echo "=== Running standalone: randomness_ml ===" && ./$(BLD_DIR)/test_standalone_randomness_ml
	@echo "=== Running standalone: compression_nn ===" && ./$(BLD_DIR)/test_standalone_compression_nn
	@echo "=== Running standalone: gan_randomness ===" && ./$(BLD_DIR)/test_standalone_gan_randomness

# ── Examples ─────────────────────────────────────────────────────
examples: $(EXM_BINS)

$(BLD_DIR)/example_%: $(EXM_DIR)/%.c $(CORE_OBJS)
	$(CC) $(CFLAGS) $(INC) $< $(CORE_OBJS) -o $@ $(LDFLAGS)

# ── Benchmarks ───────────────────────────────────────────────────
bench: $(BNC_DIR)/bench_randomness.c $(CORE_OBJS)
	$(CC) $(CFLAGS) $(INC) $< $(CORE_OBJS) -o $(BLD_DIR)/bench_randomness $(LDFLAGS)
	./$(BLD_DIR)/bench_randomness

# ── Documentation ────────────────────────────────────────────────
docs:
	@echo "Documentation files in $(DOC_DIR)/:"
	@ls -la $(DOC_DIR)/

# ── Clean ────────────────────────────────────────────────────────
clean:
	rm -rf $(BLD_DIR)
	@echo "Build directory cleaned."

distclean: clean
	rm -f $(LIB)

# ── Valgrind (memory check) ──────────────────────────────────────
valgrind: tests
	valgrind --leak-check=full --error-exitcode=1 ./$(BLD_DIR)/test_randomness || exit 1

# ── Lines of code ────────────────────────────────────────────────
loc:
	@echo "include/:" && wc -l $(INC_DIR)/*.h
	@echo "src/:" && wc -l $(SRC_DIR)/*.c
	@echo "Total:" && find . \( -path "./include/*.h" -o -path "./src/*.c" \) -exec cat {} + | wc -l

# ── Help ─────────────────────────────────────────────────────────
help:
	@echo "mini-algorithmic-randomness-ml"
	@echo "Targets:"
	@echo "  all            — Build library, tests, examples"
	@echo "  lib            — Build static library"
	@echo "  tests          — Build and run all tests"
	@echo "  standalone_tests — Build and run standalone module tests"
	@echo "  examples       — Build all example programs"
	@echo "  bench          — Run performance benchmarks"
	@echo "  clean          — Remove build artifacts"
	@echo "  loc            — Count lines of code"
	@echo "  valgrind       — Run tests with valgrind memory check"
