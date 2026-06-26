CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -I./include
LDFLAGS = -lm
SRC_DIR = src
INC_DIR = include
TST_DIR = tests
EX_DIR = examples
BCH_DIR = benches
BLD_DIR = build

SRCS = $(SRC_DIR)/resource_bounded_k.c \
       $(SRC_DIR)/universal_search.c \
       $(SRC_DIR)/complexity_approx.c \
       $(SRC_DIR)/program_resource.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(BLD_DIR)/%.o,$(SRCS))

TEST_SRC = $(TST_DIR)/test_main.c
TEST_BIN = $(BLD_DIR)/test_main

EX1_SRC = $(EX_DIR)/example_time_bounded_k.c
EX1_BIN = $(BLD_DIR)/example_time_bounded_k
EX2_SRC = $(EX_DIR)/example_levin_search.c
EX2_BIN = $(BLD_DIR)/example_levin_search
EX3_SRC = $(EX_DIR)/example_crypto_app.c
EX3_BIN = $(BLD_DIR)/example_crypto_app

BENCH_SRC = $(BCH_DIR)/bench_rbk.c
BENCH_BIN = $(BLD_DIR)/bench_rbk

.PHONY: all test examples bench clean

all: test examples

$(BLD_DIR):
	mkdir -p $(BLD_DIR)

$(BLD_DIR)/%.o: $(SRC_DIR)/%.c | $(BLD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

test: $(OBJS) $(TEST_SRC) | $(BLD_DIR)
	$(CC) $(CFLAGS) $(TEST_SRC) $(OBJS) $(LDFLAGS) -o $(TEST_BIN)
	./$(TEST_BIN)

examples: $(EX1_BIN) $(EX2_BIN) $(EX3_BIN)

$(EX1_BIN): $(OBJS) $(EX1_SRC) | $(BLD_DIR)
	$(CC) $(CFLAGS) $(EX1_SRC) $(OBJS) $(LDFLAGS) -o $@

$(EX2_BIN): $(OBJS) $(EX2_SRC) | $(BLD_DIR)
	$(CC) $(CFLAGS) $(EX2_SRC) $(OBJS) $(LDFLAGS) -o $@

$(EX3_BIN): $(OBJS) $(EX3_SRC) | $(BLD_DIR)
	$(CC) $(CFLAGS) $(EX3_SRC) $(OBJS) $(LDFLAGS) -o $@

bench: $(OBJS) $(BENCH_SRC) | $(BLD_DIR)
	$(CC) $(CFLAGS) $(BENCH_SRC) $(OBJS) $(LDFLAGS) -o $(BENCH_BIN)
	./$(BENCH_BIN)

clean:
	rm -rf $(BLD_DIR)
