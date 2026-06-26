#ifndef KOLMOGOROV_H
#define KOLMOGOROV_H

#include "binary_string.h"
#include "prefix_machine.h"
#include <stdint.h>

/* ============================================================
 * Kolmogorov Complexity
 *
 * C(x) = min{|p| : U(p) = x} — plain complexity
 * K(x) = min{|p| : V(p) = x} — prefix-free complexity
 * where V is a prefix-free optimal machine.
 *
 * Knowledge points:
 *   - Plain Kolmogorov complexity C(x)
 *   - Prefix-free Kolmogorov complexity K(x)
 *   - Invariance theorem: C_U(x) ≤ C_V(x) + O(1)
 *   - Conditional complexity K(x|y)
 *   - Chain rule: K(x,y) ≤ K(x) + K(y|x) + O(1)
 *   - Incompressibility: K(x) ≥ |x| - O(1) for random x
 *   - Coding theorem: P(x) ≈ 2^{-K(x)}
 *   - Algorithmic sufficient statistic
 *   - Mutual information I(x:y) = K(x) + K(y) - K(x,y)
 * ============================================================ */

/* ---- Plain Kolmogorov Complexity ---- */
/* C(x) = min{|p| : U(p) = x} for a fixed universal TM U.
 * Each computation returns an upper bound.
 * The true C(x) is non-computable. */

typedef uint64_t (*TMOutputFunc)(const BitString* program, const BitString* input);

/* Returns an upper bound on C(x) by searching programs ≤ max_len */
size_t kolmogorov_plain_bound(OptimalPFM* machine,
                               const uint64_t* target_output,
                               size_t target_len_words,
                               size_t max_program_len,
                               size_t step_limit);

/* ---- Prefix-Free Kolmogorov Complexity ---- */
/* K(x) = min{|p| : V(p) = x} where V is prefix-free.
 * K(x) ≥ C(x) (since prefix-free programs are a subset)
 * K(x) ≤ C(x) + K(C(x)) + O(1)   */

size_t kolmogorov_prefix_bound(OptimalPFM* machine,
                                const uint64_t* target_output,
                                size_t target_len_words,
                                size_t max_program_len,
                                size_t step_limit);

/* ---- Conditional Kolmogorov Complexity ---- */
/* K(x|y) = min{|p| : V(p,y) = x}
 * K(x|y) ≤ K(x) + O(1) */
size_t kolmogorov_conditional_bound(OptimalPFM* machine,
                                     const uint64_t* target,
                                     size_t target_len,
                                     const uint64_t* condition,
                                     size_t cond_len,
                                     size_t max_prog_len,
                                     size_t step_limit);

/* ---- Chain Rule ---- */
/* K(x,y) ≤ K(x) + K(y|x) + O(log K(x,y))
 * K(x,y) ≤ K(x) + K(y) + O(log K(x,y)) (symmetry) */
typedef struct {
    size_t k_x;
    size_t k_y;
    size_t k_xy;
    size_t k_y_given_x;
    int    chain_rule_holds;  /* 1 if K(x,y) ≤ K(x) + K(y|x) + O(log) */
} ChainRuleResult;

ChainRuleResult kolmogorov_chain_rule_verify(OptimalPFM* machine,
                                              const uint64_t* x, size_t x_len,
                                              const uint64_t* y, size_t y_len,
                                              size_t max_prog_len,
                                              size_t step_limit);

/* ---- Incompressibility ---- */
/* A string x is c-incompressible if K(x) ≥ |x| - c.
 * Most strings are incompressible (by counting argument).
 * Returns the maximal c such that x is c-incompressible. */
int kolmogorov_incompressibility(const BitString* x,
                                  OptimalPFM* machine,
                                  size_t max_prog_len,
                                  size_t step_limit);

/* ---- Mutual Information ---- */
/* I(x:y) = K(x) + K(y) - K(x,y) + O(log)
 * Measures how much algorithmic information x and y share. */
int64_t kolmogorov_mutual_information(OptimalPFM* machine,
                                       const uint64_t* x, size_t x_len,
                                       const uint64_t* y, size_t y_len,
                                       size_t max_prog_len,
                                       size_t step_limit);

/* ---- Algorithmic Probability ---- */
/* P_U(x) = Σ_{p: U(p)=x} 2^{-|p|}
 * The universal a priori probability of x. */
double algorithmic_probability(OptimalPFM* machine,
                                const uint64_t* target, size_t target_len,
                                size_t max_prog_len, size_t step_limit);

/* ---- Coding Theorem ---- */
/* -log₂ P(x) = K(x) + O(1)
 * Verifies that algorithmic probability and Kolmogorov complexity
 * are essentially equivalent measures of information content. */
typedef struct {
    double p_x;
    double neg_log2_p;
    size_t k_x;
    double difference;
    int    coding_theorem_holds;  /* 1 if |(-log P) - K| < bound */
} CodingTheoremResult;

CodingTheoremResult coding_theorem_verify(OptimalPFM* machine,
                                           const uint64_t* target, size_t target_len,
                                           size_t max_prog_len, size_t step_limit);

/* ---- Algorithmic Sufficient Statistic ---- */
/* For string x, find a "model" (program) that captures the
 * essential structure of x, separating signal from noise. */
typedef struct {
    BitString* model;       /* the sufficient statistic */
    size_t     model_k;     /* K(model) */
    BitString* noise;       /* the random part */
    size_t     noise_k;     /* K(noise|model) */
    int        is_sufficient; /* K(x) ≈ K(model) + K(x|model) */
} AlgorithmicStatistic;

AlgorithmicStatistic* algorithmic_sufficient_statistic(OptimalPFM* machine,
                                                        const BitString* x,
                                                        size_t max_model_len,
                                                        size_t step_limit);
void as_free(AlgorithmicStatistic* as);

#endif /* KOLMOGOROV_H */
