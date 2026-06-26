# Makefile �� mini-minimum-description-length
# MDL Principle: min_M [ L(M) + L(D|M) ]
# Reference: Rissanen (1978, 1996), Grunwald (2007)

CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -g
LDFLAGS = -lm
INCDIR = ./include
SRCDIR = ./src
TESTDIR = ./tests
EXAMPLEDIR = ./examples
BENCHDIR = ./benches
DEMODIR = ./demos
BUILDDIR = ./build

SRCS = $(SRCDIR)/mdl_core.c $(SRCDIR)/nml.c $(SRCDIR)/two_part.c $(SRCDIR)/mdl_advanced.c
OBJS = $(BUILDDIR)/mdl_core.o $(BUILDDIR)/nml.o $(BUILDDIR)/two_part.o $(BUILDDIR)/mdl_advanced.o

TEST_SRC = $(TESTDIR)/test_main.c
TEST_BIN = $(BUILDDIR)/test_main

.PHONY: all test clean

all: test

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INCDIR)/*.h
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

$(BUILDDIR)/test_main: $(SRCS) $(TEST_SRC) $(INCDIR)/*.h
	$(CC) $(CFLAGS) -I$(INCDIR) -o $@ $(TEST_SRC) $(SRCS) $(LDFLAGS)

test: $(BUILDDIR)/test_main
	./$(BUILDDIR)/test_main

clean:
	rm -f $(BUILDDIR)/*.o $(BUILDDIR)/*.exe $(BUILDDIR)/test_main

info:
	@echo "=== mini-minimum-description-length ==="
	@wc -l $(INCDIR)/*.h $(SRCDIR)/*.c
