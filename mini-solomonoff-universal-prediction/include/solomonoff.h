/**
 * solomonoff.h - Algorithmic Probability: Solomonoff's Universal Prior
 *
 * M(x) = sum_{p: U(p)=x*} 2^{-|p|} where U is a universal monotone
 * machine. Formalizes Occam's razor: simpler explanations (shorter
 * programs) have exponentially higher prior probability.
 *
 * M is a semimeasure: sum_x M(x) ≤ 1 (not necessarily = 1).
 * The "missing" probability mass is assigned to non-halting programs.
 *
 * Key properties:
 *   - Dominance: M dominates all computable semimeasures μ:
 *     ∃c ∀x: M(x) ≥ 2^{-c} · μ(x)
 *   - Convergence: M(x_{n+1}|x_1...x_n) → μ(x_{n+1}|x_1...x_n)
 *     for any computable measure μ, with μ-probability 1.
 *
 * Reference: Solomonoff, R.J. "A Formal Theory of Inductive Inference"
 *            Information and Control, 1964.
 * Reference: Li & Vitanyi, "An Introduction to Kolmogorov Complexity
 *            and Its Applications", 4th ed, 2019, Chapter 4.
 * Reference: Hutter, M. "Universal Artificial Intelligence", 2005.
 *
 * Curriculum:
 *   MIT 6.840/18.400 - Algorithmic Information Theory
 *   CMU 15-751 - A Theorist's Toolkit
 *   Cambridge Part III - Advanced Complexity Theory
 *   Oxford - Advanced Topics in Computational Complexity
 *   Stanford CS254 - Kolmogorov complexity segment
 */

#ifndef SOLOMONOFF_H
#define SOLOMONOFF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

/* ================================================================
 * Configuration Constants
 * ================================================================ */

/** Maximum length of a binary string in bits. */
#define SOLOMONOFF_MAX_STRING_LENGTH 256

/** Maximum program length in bits. */
#define SOLOMONOFF_MAX_PROGRAM_LENGTH 64

/** Maximum number of programs to enumerate simultaneously. */
#define SOLOMONOFF_MAX_PROGRAMS       65536

/** Maximum output length from any program execution. */
#define SOLOMONOFF_MAX_OUTPUT_LENGTH  1024

/** Numerical tolerance for floating-point comparisons. */
#define SOLOMONOFF_EPSILON            1e-12

/** Maximum dovetailing steps. */
#define SOLOMONOFF_MAX_DOVETAIL_STEPS 10000000ULL

/** Maximum depth for program enumeration (max bits). */
#define SOLOMONOFF_MAX_DEPTH          32

/** Default number of top programs to track for approximation. */
#define SOLOMONOFF_TOP_K              1000

/** Maximum number of programs in active execution pool. */
#define SOLOMONOFF_POOL_SIZE          8192

/* ================================================================
 * Core Data Types
 * ================================================================ */

/**
 * algprob_t - Algorithmic probability value.
 * M(x) ∈ [0, 1] is a semimeasure. Stored as double for practical
 * computation, but conceptually represents a real in [0,1].
 */
typedef double algprob_t;

/**
 * binary_string_t - Finite binary sequence over {0,1}.
 *
 * Representation invariants:
 *   - Empty string ε has length = 0, bits[0] = 0.
 *   - Bits are packed MSB-first within each byte: bit index i
 *     corresponds to the (i % 8)-th MSB of byte bits[i / 8].
 *   - Trailing bits in the last byte are zero-padded.
 *   - length ≤ SOLOMONOFF_MAX_STRING_LENGTH.
 *
 * This is the fundamental data unit: programs, outputs, and
 * observed sequences are all binary strings.
 */
typedef struct {
    uint8_t bits[SOLOMONOFF_MAX_STRING_LENGTH / 8 + 1];
    size_t length;
} binary_string_t;

/**
 * program_t - A self-delimiting program for the universal machine.
 *
 * A program p is a binary string. When executed on the universal
 * monotone machine U, it produces (possibly infinite) output U(p).
 * The weight 2^{-|p|} is its algorithmic prior probability.
 *
 * For prefix-free machines: sum_p 2^{-|p|} ≤ 1 (Kraft inequality).
 * For monotone machines: the weight is the measure of the cylinder
 * set of all infinite sequences extending p.
 */
typedef struct {
    binary_string_t code;         /**< Program binary code */
    size_t length;                /**< |p| in bits */
    double weight;                /**< 2^{-|p|} */
    bool halts;                   /**< Whether program has halted */
    binary_string_t output;       /**< Output produced so far (monotone) */
    uint64_t steps_to_halting;    /**< Number of steps before halting */
    uint64_t steps_executed;      /**< Total steps executed so far */
    bool is_active;               /**< Whether still in execution pool */
} program_t;

/*
 * universal_machine_state_t is now defined as um_state_t in universal_machine.h.
 * Forward-declare for use in pool_entry_t (defined in enumeration.h).
 * (The full definition lives in universal_machine.h.)
 */
typedef struct um_state_t universal_machine_state_t;

/**
 * enumeration_state_t - State of program enumeration.
 *
 * Enumerates all binary strings in length-lexicographic order:
 * ε, 0, 1, 00, 01, 10, 11, 000, ...
 *
 * This is the "program space" over which we sum to compute M(x).
 */
typedef struct {
    binary_string_t current;        /**< Current enumerated program */
    size_t current_length;          /**< Current program length */
    size_t total_enumerated;         /**< Total programs enumerated so far */
    size_t max_programs;            /**< Maximum programs to enumerate */
} enumeration_state_t;

/**
 * dovetail_state_t - Dovetailing execution state.
 *
 * Dovetailing interleaves execution of multiple programs to handle
 * non-termination: program i gets step when global_step % f(i) == 0.
 * This ensures every program that halts eventually gets enough steps.
 */
typedef struct {
    program_t *programs;            /**< Array of programs being executed */
    size_t num_programs;            /**< Number of programs in pool */
    size_t capacity;                /**< Pool capacity */
    uint64_t global_step;           /**< Global execution step counter */
    double total_probability_mass;  /**< Sum of weights of halting programs */
    size_t halts_found;             /**< Number of programs that halted */
} dovetail_state_t;

/**
 * prediction_result_t - Result of a prediction query.
 *
 * Given observed prefix x, predict the probability that the next
 * bit is 0 or 1. Also includes the algorithmic probability M(x0)
 * and M(x1) for the two possible continuations.
 */
typedef struct {
    algprob_t prob_next_zero;       /**< M(x0) / (M(x0) + M(x1)) */
    algprob_t prob_next_one;        /**< M(x1) / (M(x0) + M(x1)) */
    algprob_t M_x0;                 /**< Algorithmic probability of x·0 */
    algprob_t M_x1;                 /**< Algorithmic probability of x·1 */
    algprob_t M_x;                  /**< Algorithmic probability of x */
    double confidence;              /**< M(x0)+M(x1)-M(x) as confidence */
    /** Bayesian interpretation:
     *  If we use M as a prior, then after observing x, the posterior
     *  probability that the next bit is b is M(xb)/M(x) (for prefix M).
     *  Using monotone M: P(b|x) ≈ M(xb) / (M(x0) + M(x1)).
     */
} prediction_result_t;

/**
 * solomonoff_ensemble_t - Ensemble of programs with their contributions.
 *
 * Stores the top-k programs that contribute most to M(x) for a given x.
 * Used for analysis and visualization of which "explanations" dominate.
 */
typedef struct {
    program_t *top_programs;        /**< Array of top programs */
    size_t num_top;                 /**< Number of programs stored */
    size_t capacity;                /**< Maximum capacity */
    algprob_t total_M;              /**< Total probability mass accumulated */
    algprob_t top_k_mass;           /**< Mass contributed by top-k */
    double coverage_ratio;          /**< top_k_mass / total_M */
} solomonoff_ensemble_t;

/* ================================================================
 * Binary String Operations (L1 - Definitions)
 * ================================================================ */

/**
 * Initialize a binary string to empty (ε).
 * Complexity: O(1)
 */
void bs_init(binary_string_t *s);

/**
 * Initialize from a C string of '0' and '1' characters.
 * Returns true on success, false if invalid character found
 * or string too long.
 * Complexity: O(n) where n = strlen(str)
 */
bool bs_from_cstr(binary_string_t *s, const char *str);

/**
 * Get the i-th bit (0-indexed from left/MSB).
 * Returns -1 if index out of bounds.
 * Complexity: O(1)
 */
int bs_get_bit(const binary_string_t *s, size_t i);

/**
 * Set the i-th bit to value (0 or 1).
 * Returns true on success.
 * Complexity: O(1)
 */
bool bs_set_bit(binary_string_t *s, size_t i, int value);

/**
 * Append a single bit (0 or 1) to the end.
 * Returns true on success.
 * Complexity: O(1)
 */
bool bs_append_bit(binary_string_t *s, int bit);

/**
 * Concatenate t to the end of s.
 * Returns true on success.
 * Complexity: O(|t|)
 */
bool bs_concat(binary_string_t *s, const binary_string_t *t);

/**
 * Extract the prefix of length k from s into result.
 * If k > s->length, copies entire s.
 * Complexity: O(k / 8)
 */
void bs_prefix(const binary_string_t *s, size_t k, binary_string_t *result);

/**
 * Check if prefix is a prefix of s.
 * Complexity: O(min(|prefix|, |s|))
 */
bool bs_has_prefix(const binary_string_t *s, const binary_string_t *prefix);

/**
 * Lexicographic comparison (shortlex order: first by length, then lex).
 * Returns -1 if a < b, 0 if a == b, 1 if a > b.
 * Complexity: O(min(|a|, |b|))
 */
int bs_compare_shortlex(const binary_string_t *a, const binary_string_t *b);

/**
 * Copy src to dst.
 * Complexity: O(|src|)
 */
void bs_copy(binary_string_t *dst, const binary_string_t *src);

/**
 * Check if two binary strings are equal.
 * Complexity: O(min(|a|, |b|))
 */
bool bs_equal(const binary_string_t *a, const binary_string_t *b);

/**
 * Print binary string to file (as '0'/'1' characters).
 * Complexity: O(n)
 */
void bs_print(FILE *fp, const binary_string_t *s);

/**
 * Compute the number of shared prefix bits.
 * Complexity: O(min(|a|, |b|))
 */
size_t bs_common_prefix_len(const binary_string_t *a, const binary_string_t *b);

/**
 * Count number of 1-bits (Hamming weight / population count).
 * Complexity: O(n)
 */
size_t bs_popcount(const binary_string_t *s);

/* ================================================================
 * Core Algorithmic Probability (L1-L4)
 * ================================================================ */

/**
 * Compute Solomonoff's algorithmic probability M(x) by enumerating all
 * programs up to `depth` bits and dovetailing their execution.
 *
 * M(x) = sum_{p: U(p) outputs x*} 2^{-|p|}
 *
 * where U is the universal monotone machine embedded in this library,
 * and x* means any extension of x. The sum is over all programs p
 * (up to the specified depth) whose output has x as a prefix.
 *
 * Parameters:
 *   x      - The target binary string (observation).
 *   depth  - Maximum program length to consider (bits).
 *   max_steps - Maximum total dovetailing steps.
 *
 * Returns: M(x) ∈ [0, 1] (may be 0 if no program produces x within
 *          the given depth and step bounds).
 *
 * Theorem (Solomonoff, 1964): M is a universal semimeasure; it
 * multiplicatively dominates every computable semimeasure.
 *
 * Complexity: O(2^depth) in worst case (enumerating all programs).
 * Reference: Li & Vitanyi, Theorem 4.3.1
 */
algprob_t solomonoff_M(const binary_string_t *x,
                       size_t depth,
                       uint64_t max_steps);

/**
 * Compute M(x) using only programs up to `depth` that halt within
 * `max_steps` on any single execution (not total dovetailing time).
 * This is a coarser but faster approximation.
 *
 * Complexity: O(2^depth · max_steps) worst case.
 */
algprob_t solomonoff_M_fast(const binary_string_t *x,
                            size_t depth,
                            uint64_t max_steps);

/**
 * Compute Levin's time-bounded algorithmic probability Pt(x).
 *
 * Pt(x) = sum_{p: U(p)=x in ≤ t steps} 2^{-|p|}
 *
 * Unlike M(x) which sums over all programs that eventually produce x,
 * Pt(x) includes a time penalty: only programs that produce x within
 * t steps contribute. As t → ∞, Pt(x) → M(x).
 *
 * This is a computable approximation to M(x).
 *
 * Reference: Levin, L.A. "Universal Sequential Search Problems", 1973.
 *
 * Complexity: O(|{p : |p| ≤ depth}| · t/depth) with dovetailing.
 */
algprob_t levin_Pt(const binary_string_t *x,
                   size_t depth,
                   uint64_t tmax);

/**
 * Compute conditional algorithmic probability M(x | y).
 *
 * M(x | y) = M(yx) / M(y)  [naive approach]
 *
 * More properly, M(x|y) = sum_{p: U(p, y) outputs x*} 2^{-|p|}
 * where y is provided as auxiliary input to the universal machine.
 *
 * This implementation uses the ratio form for computability.
 *
 * Parameters:
 *   x, y  - Binary strings. x is the target, y is the condition.
 *   depth - Maximum program length.
 *   max_steps - Maximum dovetailing steps.
 *
 * Returns M(x|y) or 0.0 if M(y) = 0.
 */
algprob_t solomonoff_conditional_M(const binary_string_t *x,
                                   const binary_string_t *y,
                                   size_t depth,
                                   uint64_t max_steps);

/**
 * Predict the next bit given observed prefix x.
 *
 * Uses algorithmic probability to compute:
 *   P(next=0 | x) = M(x0) / (M(x0) + M(x1))
 *   P(next=1 | x) = M(x1) / (M(x0) + M(x1))
 *
 * This is the core of Solomonoff induction: given past observations,
 * predict the future using the universal prior.
 *
 * The prediction converges (with μ-probability 1) to the true
 * conditional probability for any computable measure μ.
 *
 * Reference: Solomonoff, "Complexity-based induction systems", 1978.
 * Theorem: Hutter, "On Universal Prediction and Bayesian Confirmation", 2007.
 */
prediction_result_t solomonoff_predict(const binary_string_t *x,
                                        size_t depth,
                                        uint64_t max_steps);

/**
 * Compute the total probability mass explained by top-k programs.
 * Stores the ensemble in `ensemble` for analysis.
 *
 * This reveals which "theories" (programs) dominate the explanation
 * of the observed data x.
 */
solomonoff_ensemble_t solomonoff_explain(const binary_string_t *x,
                                           size_t depth,
                                           uint64_t max_steps,
                                           size_t top_k);

/**
 * Free resources associated with an ensemble.
 */
void solomonoff_ensemble_free(solomonoff_ensemble_t *e);

/**
 * Compute the algorithmic information content (≈ -log₂ M(x)).
 * This is the negative logarithm of the algorithmic probability,
 * providing a quantity measured in bits that approximates the
 * Kolmogorov complexity K(x) up to an additive constant.
 *
 * K(x) = -log₂ M(x) + O(1)
 *
 * This follows from the Coding Theorem (Levin, 1974).
 */
double solomonoff_algorithmic_info(const binary_string_t *x,
                                   size_t depth,
                                   uint64_t max_steps);

/* ================================================================
 * Semimeasure Properties (L4 - Theorems)
 * ================================================================ */

/**
 * Verify the semimeasure property: sum_{|x| ≤ n} M(x) ≤ 1.
 *
 * Enumerates all strings up to length n and computes the sum
 * of their algorithmic probabilities. Returns the total mass.
 *
 * If total ≈ 1 with small gap, the gap represents the probability
 * of non-halting programs at this depth.
 */
double solomonoff_verify_semimeasure(size_t n, size_t depth, uint64_t max_steps);

/**
 * Compute the Kraft sum: Σ_p 2^{-|p|} over enumerated programs.
 * For prefix-free sets, this must be ≤ 1. Our enumeration produces
 * all binary strings, so the Kraft sum is the total measure of the
 * Cantor space of programs at the given depth.
 *
 * Σ_{|p| ≤ d} 2^{-|p|} = (d + 1) for all binary strings.
 */
double solomonoff_kraft_sum(size_t depth);

/**
 * Verify the dominance property: M(x) ≥ 2^{-c} · μ(x) for a
 * specific computable measure μ.
 *
 * Tests with μ being a simple Bernoulli(p) distribution.
 */
bool solomonoff_verify_dominance(double p, size_t n_trials,
                                  size_t depth, uint64_t max_steps);

/* ================================================================
 * Convergence Tracking (L4 - Theorems)
 * ================================================================ */

/**
 * Track the convergence of M(xb|xb*)/M(x) over increasing depth.
 * Returns the error |M_d(xb)/M_d(x) - true_prob| as a function
 * of depth.
 *
 * As depth → ∞, this error → 0 for any computable measure.
 */
typedef struct {
    size_t depth;
    algprob_t predicted_prob;
    algprob_t true_prob;
    double abs_error;
} convergence_point_t;

/**
 * Compute convergence points for a sequence generated by a known
 * Bernoulli process. This empirically demonstrates Solomonoff's
 * convergence theorem.
 */
size_t solomonoff_convergence_track(const binary_string_t *sequence,
                                     double true_bias,
                                     size_t max_depth,
                                     size_t depth_step,
                                     uint64_t max_steps,
                                     convergence_point_t *points,
                                     size_t max_points);

#endif /* SOLOMONOFF_H */
