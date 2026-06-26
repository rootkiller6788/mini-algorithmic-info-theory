# Makefile for mini-solomonoff-universal-prediction
# Algorithmic Probability: Solomonoff's Universal Prior
#
# Reference: Li & Vitanyi, "An Introduction to Kolmogorov Complexity
#            and Its Applications", 4th ed, 2019.

CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -Wno-unused-parameter -O2 -g
INCLUDES = -Iinclude
LDFLAGS  = -lm

SRCDIR   = src
INCDIR   = include
TSTDIR   = tests
EXADIR   = examples
DEMDIR   = demos
BNCDIR   = benches
BUILDDIR = build

SRCS     = $(wildcard $(SRCDIR)/*.c)
OBJS     = $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SRCS))
TESTS    = $(patsubst $(TSTDIR)/%.c,$(BUILDDIR)/%,$(wildcard $(TSTDIR)/*.c))
EXAMPLES = $(patsubst $(EXADIR)/%.c,$(BUILDDIR)/%,$(wildcard $(EXADIR)/*.c))
DEMOS    = $(patsubst $(DEMDIR)/%.c,$(BUILDDIR)/%,$(wildcard $(DEMDIR)/*.c))
BENCHES  = $(patsubst $(BNCDIR)/%.c,$(BUILDDIR)/%,$(wildcard $(BNCDIR)/*.c))

.PHONY: all clean test examples demos benches

all: $(OBJS)
	@echo "Library objects built: $(OBJS)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test binaries
$(BUILDDIR)/test_%: $(TSTDIR)/test_%.c $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(OBJS) -o $@ $(LDFLAGS)

$(BUILDDIR)/example_%: $(EXADIR)/example_%.c $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(OBJS) -o $@ $(LDFLAGS)

$(BUILDDIR)/demo_%: $(DEMDIR)/demo_%.c $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(OBJS) -o $@ $(LDFLAGS)

$(BUILDDIR)/bench_%: $(BNCDIR)/bench_%.c $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) $< $(OBJS) -o $@ $(LDFLAGS)

test: $(TESTS)
	@echo "Running tests..."
	@for t in $(TESTS); do echo "  $$t:"; $$t && echo "    PASS" || echo "    FAIL"; done
	@echo "All tests done."

examples: $(EXAMPLES)
	@echo "Running examples..."
	@for e in $(EXAMPLES); do echo "  $$e:"; $$e; done

demos: $(DEMOS)
	@echo "Running demos..."
	@for d in $(DEMOS); do echo "  $$d:"; $$d; done

benches: $(BENCHES)
	@echo "Running benchmarks..."
	@for b in $(BENCHES); do echo "  $$b:"; $$b; done

clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/test_* $(BUILDDIR)/example_* $(BUILDDIR)/demo_* $(BUILDDIR)/bench_*
	@echo "Cleaned."
