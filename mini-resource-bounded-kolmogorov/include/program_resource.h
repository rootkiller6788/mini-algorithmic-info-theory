/**
 * program_resource.h — Program Enumeration and Resource Management
 *
 * Manages program enumeration under resource bounds for Kolmogorov
 * complexity computations. Provides:
 *
 *   - Lexicographic program enumeration
 *   - Resource constraint checking (time/space limits)
 *   - Prefix-free code generation
 *   - UTM simulation with cutoffs
 *   - Program counting and statistics
 *   - Binary string manipulation utilities
 *
 * References:
 *   Li & Vitanyi, "An Introduction to Kolmogorov Complexity", 4th ed. (2019)
 *   Zvonkin & Levin, "The Complexity of Finite Objects" (1970)
 *   Chaitin, "A Theory of Program Size Formally Identical to Information Theory" (1975)
 */

#ifndef PROGRAM_RESOURCE_H
#define PROGRAM_RESOURCE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ==================================================================
 * L1: Core Types
 * ================================================================== */

/** Execution context for a program running on a resource-bounded UTM.
 *  Tracks the program, input, tape, head position, and resource usage.
 */
typedef struct {
    char   *program;
    size_t  prog_len;
    char   *input;
    size_t  input_len;
    char   *tape;
    size_t  tape_head;
    size_t  tape_len;
    size_t  steps_executed;
    size_t  time_limit;
    size_t  space_limit;
    bool    halted;
    bool    time_exceeded;
    bool    space_exceeded;
    bool    prefix_violation;
} UTMContext;

/** Program pool metadata for enumeration. */
typedef struct {
    char   **programs;
    size_t  *lengths;
    size_t   capacity;
    size_t   count;
    size_t   max_length;
    bool     prefix_free_pool;
} ProgramPool;

/** Output from a program run. */
typedef struct {
    char   *output;
    size_t  output_len;
    size_t  steps_used;
    size_t  space_used;
    bool    terminated;
    bool    prefix_ok;
} ProgramOutput;

/** Prefix-free code tree node.
 *  A complete binary tree tracking which codewords are reserved.
 *  Leaf = codeword; internal = potentially extendable.
 */
typedef struct {
    bool leaf;
    int  leaf_index;
    struct PrefixTreeNode *left;
    struct PrefixTreeNode *right;
} PrefixTreeNode;

/** Lexicographic enumeration state.
 *  Tracks progress through all binary strings up to a given length.
 */
typedef struct {
    char   *current;
    size_t  current_len;
    size_t  max_len;
    size_t  total_enumerated;
    bool    done;
} LexEnumState;

/** Resource profile for a program execution batch. */
typedef struct {
    size_t  programs_tested;
    size_t  outputs_collected;
    size_t  total_time_used;
    size_t  total_space_used;
    size_t  timeouts;
    size_t  space_violations;
    double  acceptance_ratio;
} ResourceProfile;

/* ==================================================================
 * L2: UTM Emulation
 * ================================================================== */

/** Initialize execution context for a simple UTM.
 *  The UTM interprets a binary instruction set:
 *   00 = move left,  01 = move right
 *   10 = flip bit at head, 11 = halt
 *  The tape starts with input written, followed by blanks. */
UTMContext* utm_context_create(const char *program, size_t prog_len,
                                const char *input, size_t input_len,
                                size_t time_limit, size_t space_limit);

/** Execute one step of the UTM. Returns false if halted/limit exceeded. */
bool utm_step_one(UTMContext *ctx);

/** Execute until halt or resource exhaustion. Returns termination reason. */
ProgramOutput utm_run_all(UTMContext *ctx);

/** Check if program is prefix-free (no proper prefix is also a valid program). */
bool utm_check_prefix_free(const char *program, size_t prog_len,
                            ProgramPool *valid_programs);

/** Free UTM context. */
void utm_context_free(UTMContext *ctx);

/* ==================================================================
 * L3: Program Enumeration
 * ================================================================== */

/** Create a program pool up to max_length. */
ProgramPool program_pool_create(size_t max_length);

/** Enumerate all binary programs up to max_length, filter by prefix-free.
 *  Stores valid programs in the pool. */
size_t program_pool_enumerate_prefix_free(ProgramPool *pool, size_t max_length);

/** Enumerate programs in lexicographic order.
 *  Calls callback for each program with its length. */
void program_pool_lexicographic_enumerate(
    size_t max_length,
    void (*callback)(const char *program, size_t prog_len, void *user_data),
    void *user_data);

/** Count programs of given length. 2^n total, but fewer if prefix-free. */
size_t program_pool_count_at_length(size_t n, bool prefix_free);

/** Next program in lexicographic order (increments binary counter). */
bool program_pool_next(LexEnumState *state);

/** Initialize lexicographic enumeration. */
LexEnumState lex_enum_init(size_t max_len);

/** Free program pool. */
void program_pool_free(ProgramPool *pool);

/** Free program output. */
void program_output_free(ProgramOutput *out);

/* ==================================================================
 * L4: Resource Constraint Utilities
 * ================================================================== */

/** Upper bound on K^t(x): |x| + O(log|x|) via self-printing trick.
 *  print(x) program has length |x| + print_loop_overhead. */
size_t resource_upper_bound_Kt(const char *x, size_t len);

/** Check if a function f is time-constructible: can compute f(n) in O(f(n)). */
bool resource_is_time_constructible(size_t (*f)(size_t), size_t max_n);

/** Check if a function f is space-constructible. */
bool resource_is_space_constructible(size_t (*f)(size_t), size_t max_n);

/** Time-constructible linear function. */
size_t f_time_linear(size_t n);

/** Time-constructible quadratic function. */
size_t f_time_quadratic(size_t n);

/** Time-constructible exponential function (2^n). */
size_t f_time_exponential(size_t n);

/** Space-constructible linear function. */
size_t f_space_linear(size_t n);

/** Space-constructible log function. */
size_t f_space_log(size_t n);

/** Compute the overhead of simulating one UTM on another.
 *  If U simulates V in time O(t log t), overhead = log t. */
double resource_simulation_overhead(size_t steps);

/* ==================================================================
 * L5: Program Counting / Universal Distribution
 * ================================================================== */

/** Count programs up to length L that produce a given output.
 *  Brute-force enumeration under time bound T. */
size_t resource_count_producing(const char *output, size_t out_len,
                                 size_t max_prog_len, size_t time_limit);

/** Estimate universal distribution m(x) by program sampling.
 *  m(x) = sum_{p: U(p)=x} 2^{-|p|}, approximated by truncation. */
double resource_universal_distribution(const char *x, size_t len,
                                        size_t max_prog_len, size_t time_limit);

/** Kraft inequality sum: Σ 2^{-|p|} over all valid prefix-free programs ≤ 1.
 *  Useful for verifying prefix-free code properties. */
double resource_kraft_sum(size_t max_len);

/** Compute Solomonoff prior: P(bit=1 | history) ∉ m(history·1)/m(history·0). */
double resource_solomonoff_next_bit(const char *history, size_t hist_len,
                                     size_t max_prog_len, size_t time_limit);

/* ==================================================================
 * L6: Binary String Utilities
 * ================================================================== */

/** Convert a binary string to a canonical self-delimiting encoding.
 *  x → 1^{|x|} 0 x  (length encoded in unary, then data). */
char* binary_self_delimiting_encode(const char *x, size_t len, size_t *out_len);

/** Decode self-delimiting encoding. Returns NULL on failure. */
char* binary_self_delimiting_decode(const char *enc, size_t enc_len,
                                     size_t *out_len);

/** Pair encoding: map (x,y) to a single binary string.
 *  Using the standard 7x·1y coding: write lengths in unary with delimiters. */
char* binary_pair_encode(const char *x, size_t xlen,
                          const char *y, size_t ylen, size_t *out_len);

/** Decode a pair-encoded string back into x and y. */
bool binary_pair_decode(const char *pair, size_t pair_len,
                         char **x, size_t *xlen, char **y, size_t *ylen);

/** Compute the binary logarithm of a number (integer floor). */
size_t binary_ilog2(size_t n);

/** Print binary string as 0/1 sequence for debugging. */
void binary_print(const char *x, size_t len, const char *label);

/** Print resource profile. */
void resource_profile_print(const ResourceProfile *rp);

#endif /* PROGRAM_RESOURCE_H */
