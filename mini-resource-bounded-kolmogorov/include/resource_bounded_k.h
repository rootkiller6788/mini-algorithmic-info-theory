/**
 * resource_bounded_k.h — Resource-Bounded Kolmogorov Complexity
 *
 * Core definitions and functions for time-bounded K^t(x), space-bounded
 * K^s(x), prefix complexity, conditional complexity, structure functions,
 * randomness deficiency, and incompressibility certificates.
 *
 * References:
 *   Li & Vitanyi, "An Introduction to Kolmogorov Complexity", 4th ed. (2019)
 *   Sipser, "A Complexity Theoretic Approach to Randomness" (1983)
 *   Ko, "On the Notion of Infinite Pseudorandom Sequences" (1986)
 *   Hutter, "Universal Artificial Intelligence" (2005)
 *   Trakhtenbrot, "A Survey of Russian Approaches to Perebor" (1984)
 */

#ifndef RESOURCE_BOUNDED_K_H
#define RESOURCE_BOUNDED_K_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ==================================================================
 * Type Definitions (L1: Definitions)
 * ================================================================== */

/** Universal Turing machine program with resource constraints.
 *  A program is a binary string that a UTM interprets; resource bounds
 *  constrain its execution (time steps, tape cells).
 */
typedef struct {
    char   *code;
    size_t  length;
    size_t  time_limit;
    size_t  space_limit;
    bool    prefix_free;
} RBKProgram;

/** Kolmogorov complexity measurement result.
 *  Contains the computed complexity value, a witness program achieving it,
 *  and metadata about the computation process.
 */
typedef struct {
    size_t   K_value;
    char    *witness_program;
    size_t   witness_length;
    size_t   time_used;
    size_t   space_used;
    bool     exact;
    double   confidence;
} RBKComplexity;

/** Kolmogorov structure function h_x(k).
 *  For each complexity bound k, finds the minimal set S with K(S) <= k
 *  that best explains x (minimizing K(x|S)).
 */
typedef struct {
    size_t   k;
    size_t   min_model_complexity;
    size_t   data_to_model;
    size_t   total_complexity;
    char    *best_set;
} RBKStructureFn;

/** Resource-bounded randomness test result.
 *  Evaluates whether a string appears random under resource constraints
 *  using martingale-based tests and deficiency measures.
 */
typedef struct {
    double   deficiency;
    bool     passes_tests;
    size_t   bits_tested;
    double   martingale_limit;
} RBKRandomnessResult;

/** Time-constructible function descriptor.
 *  f is time-constructible if a TM can compute f(n) in O(f(n)) time.
 */
typedef struct {
    size_t (*time_fun)(size_t n);
    char    name[32];
    bool    is_constructible;
} TimeConstructible;

/** Space-constructible function descriptor. */
typedef struct {
    size_t (*space_fun)(size_t n);
    char    name[32];
    bool    is_constructible;
} SpaceConstructible;

/** Resource-bounded universal distribution record. */
typedef struct {
    double   m_value;
    size_t   programs_contributing;
    double   convergence_rate;
} RBKUniversalDist;

/* ==================================================================
 * L2: Core Complexity Functions
 * ================================================================== */

RBKComplexity rbk_K_time(const char *x, size_t len, size_t t);
RBKComplexity rbk_K_space(const char *x, size_t len, size_t s);
RBKComplexity rbk_K_time_space(const char *x, size_t len, size_t t, size_t s);
RBKComplexity rbk_K_conditional(const char *x, size_t xlen,
                                 const char *y, size_t ylen, size_t t);
RBKComplexity rbk_K_prefix(const char *x, size_t len, size_t t);
RBKComplexity rbk_K_joint(const char *x, size_t xlen,
                           const char *y, size_t ylen, size_t t);
double rbk_mutual_information(const char *x, size_t xlen,
                               const char *y, size_t ylen, size_t t);

/* ==================================================================
 * L3: Mathematical Structures
 * ================================================================== */

bool rbk_check_monotonicity(const char *x, size_t len,
                             const size_t *time_bounds, size_t num_bounds);
size_t rbk_upper_bound_size(size_t x_len);
bool rbk_kraft_inequality(const size_t *lengths, size_t count);
char* rbk_self_delimiting_encode(const char *x, size_t len, size_t *out_len);
char* rbk_self_delimiting_decode(const char *enc, size_t enc_len, size_t *out_len);
char* rbk_pair_encode(const char *x, size_t xlen,
                       const char *y, size_t ylen, size_t *out_len);
void rbk_pair_decode(const char *pair, size_t pair_len,
                      char **x, size_t *xlen, char **y, size_t *ylen);
void rbk_lexicographic_enum(size_t max_len,
                             void (*callback)(const char*, size_t, void*),
                             void *user_data);

/* ==================================================================
 * L4: Fundamental Theorems
 * ================================================================== */

size_t rbk_invariance_overhead(const RBKProgram *simulator_v, size_t t);
int64_t rbk_symmetry_of_info(const char *x, size_t xlen,
                              const char *y, size_t ylen, size_t t);
size_t rbk_coding_theorem_error(const char *x, size_t len, size_t t);
bool rbk_chaitin_diagonalize(size_t resource_bound);

/* ==================================================================
 * L5: Analysis Functions
 * ================================================================== */

RBKStructureFn* rbk_structure_function(const char *x, size_t len,
                                        size_t max_k, size_t *num_points);
RBKStructureFn rbk_sufficient_statistic(const char *x, size_t len, size_t t);
RBKRandomnessResult rbk_randomness_deficiency(const char *x, size_t len, size_t t);
size_t rbk_mdl_select(const char *data, size_t data_len,
                       const char **models, size_t model_count, size_t t);
double rbk_compressibility_ratio(const char *x, size_t len, size_t t);
bool rbk_incompressibility_cert(const char *x, size_t len, size_t k, size_t t);
size_t rbk_count_programs_producing(const char *output, size_t out_len,
                                     size_t max_prog_len, size_t t);
double rbk_universal_distribution(const char *x, size_t len, size_t t);
int rbk_classify_string(const char *x, size_t len, size_t t);

/* ==================================================================
 * L6: Resource-Bound Hierarchy Functions
 * ================================================================== */

size_t rbk_time_advantage(const char *x, size_t len, size_t t1, size_t t2);
size_t rbk_space_time_tradeoff(const char *x, size_t len,
                                size_t k_target, size_t s);
bool rbk_is_time_constructible(size_t (*f)(size_t), size_t max_n);
bool rbk_is_space_constructible(size_t (*f)(size_t), size_t max_n);

/* ==================================================================
 * L7: Cryptographic Applications
 * ================================================================== */

double rbk_prg_incompressibility_ratio(const char *keystream, size_t len, size_t t);
size_t rbk_crypto_weakness_gap(const char *output, size_t len, size_t t);
double rbk_owf_hardness(const char *y, size_t ylen,
                         const char *x_hint, size_t xlen, size_t t);

/* ==================================================================
 * L8: Advanced — Information Distance, Meta-Complexity
 * ================================================================== */

double rbk_information_distance(const char *x, size_t xlen,
                                 const char *y, size_t ylen, size_t t);
double rbk_normalized_information_distance(const char *x, size_t xlen,
                                            const char *y, size_t ylen, size_t t);
size_t rbk_quine_construct(char *buffer, size_t buf_cap);
size_t rbk_self_print_overhead(void);
int rbk_which_more_complex(const char *a, size_t alen,
                            const char *b, size_t blen, size_t t);
size_t rbk_meta_complexity_adversary(const char *x, size_t len,
                                      size_t t_target, size_t num_rounds);

/* ==================================================================
 * Utility Functions
 * ================================================================== */

void rbk_complexity_free(RBKComplexity *c);
void rbk_structure_fn_free(RBKStructureFn *sf, size_t count);
void rbk_print_complexity(const RBKComplexity *c);
void rbk_print_structure_fn(const RBKStructureFn *sf, size_t count);
void rbk_print_randomness(const RBKRandomnessResult *r);

#endif /* RESOURCE_BOUNDED_K_H */
