/*
 * randomness.h — Algorithmic Randomness: Martin-Löf Tests & Core Theory
 *
 * Core definitions (L1-L2) bridging algorithmic randomness with
 * practical ML-based estimation.
 *
 * Martin-Löf (1966): An infinite binary sequence is random if it passes
 * all effective (computably enumerable) sequential tests.
 *
 * Three equivalent characterizations:
 *   1. ML-randomness via sequential tests
 *   2. Martingale characterization (Schnorr, 1971)
 *   3. Incompressibility (Kolmogorov/Chaitin/Levin)
 *
 * This module provides ML-assisted approximations of these notions.
 *
 * Reference: Nies "Computability and Randomness" (2009) Ch 1-3.
 *            Downey & Hirschfeldt "Algorithmic Randomness and Complexity"
 *            Li & Vitányi §2.4-2.5, §3.1-3.4
 * Courses: MIT 6.841, Stanford CS254, Berkeley CS278, CMU 15-855
 *          Oxford Advanced Complexity, Cambridge Part III
 */

#ifndef RANDOMNESS_H
#define RANDOMNESS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ══════════════════════════════════════════════════════════════
   L1: Core Data Structures — Definitions
   ══════════════════════════════════════════════════════════════ */

/* ── Binary string (the object of randomness testing) ───────── */
typedef struct {
    unsigned char* data;
    size_t         len;     /* length in bits */
    size_t         cap;     /* capacity in bytes */
} RandomBitString;

/* ── A single randomness test (Martin-Löf test component) ──── */
typedef struct {
    char*   name;            /* test name, e.g., "Frequency", "Runs" */
    int     level;           /* significance level index (m in ML test) */
    double  critical_value;  /* threshold at this level */
    double  statistic;       /* computed test statistic */
    int     passed;          /* 1 if statistic ≤ critical_value */
    double  p_value;         /* p-value of the test */
    void*   params;          /* test-specific parameters */
} RandomnessTest;

/* ── Martin-Löf test: a uniformly c.e. sequence of tests ───── */
typedef struct {
    RandomnessTest* tests;   /* array of component tests */
    int             n_tests;
    int             cap;
    char*           name;    /* "Martin-Löf Sequential Test" */
    int             is_ml_test;   /* 1 if satisfies ML nesting */
} MLTest;

/* ── Supermartingale for algorithmic randomness ────────────── */
typedef struct {
    double* values;          /* capital values along prefixes */
    size_t   n_prefixes;
    double   initial_capital;
    double   final_capital;
    int      succeeds;       /* 1 if unbounded growth */
} RandomMartingale;

/* ── Randomness profile: comprehensive characterization ────── */
typedef struct {
    double  martin_lof_score;     /* 0-1: 1=random, 0=non-random */
    double  schnorr_score;
    double  kolmogorov_estimate;  /* estimated K(x)/|x| */
    double  entropy_per_bit;      /* Shannon entropy per bit */
    double  chi_square_pvalue;    /* χ² goodness-of-fit p-value */
    double  serial_correlation;   /* lag-1 autocorrelation */
    int     is_ml_random;         /* 1 if ML-random at level 2^(-10) */
    int     compression_ratio;    /* compression ratio percentile */
    int     n_tests_passed;
    int     n_tests_total;
    void*   ml_classifier_output; /* opaque ML classifier result */
} RandomnessProfile;

/* ── Enum for randomness type classification ───────────────── */
typedef enum {
    RAND_ML_RANDOM  = 0,  /* passes all effective tests */
    RAND_SCHNORR    = 1,  /* computable martingale bound */
    RAND_COMPUTABLE = 2,  /* no computable strategy makes money */
    RAND_STOCHASTIC = 3,  /* consistent with some measure */
    RAND_WEAK       = 4,  /* passes only simple tests */
    RAND_NONRANDOM  = 5   /* fails basic tests */
} RandomnessType;

/* ══════════════════════════════════════════════════════════════
   L2: Core Concepts — ML-Randomness Estimation
   ══════════════════════════════════════════════════════════════ */

/**
 * ml_test_create — Allocate a Martin-Löf sequential test.
 * @name: descriptive name for this test suite
 */
MLTest* ml_test_create(const char* name);

/**
 * ml_test_add_component — Add a sub-test (frequency, runs, etc.)
 * to the Martin-Löf test.
 */
void ml_test_add_component(MLTest* t, const char* name,
                           double critical_value, double statistic);

/**
 * ml_test_evaluate — Evaluate whether data passes the ML test.
 * Returns 0 (rejected) or 1 (passed) at the given significance level.
 */
int ml_test_evaluate(MLTest* t, int significance_level);

/**
 * ml_test_is_universal — Check if this test dominates all
 * computable sequential tests (universality property).
 */
int ml_test_is_universal(const MLTest* t);

/**
 * ml_test_print — Pretty-print test results.
 */
void ml_test_print(const MLTest* t);

/**
 * ml_test_free — Release all memory.
 */
void ml_test_free(MLTest* t);

/* ══════════════════════════════════════════════════════════════
   L3: Mathematical Structures — Sequential Tests & Martingales
   ══════════════════════════════════════════════════════════════ */

/**
 * sequential_test_union — Combine multiple ML tests into a
 * stronger universal test. Corresponds to the computable union
 * of c.e. open sets U_m = {σ : ∃τ⊇σ with τ∈U_{m+1}}.
 */
MLTest* sequential_test_union(MLTest** tests, int count);

/**
 * martingale_create — Initialize a supermartingale for betting
 * on a binary sequence. Capital never decreases on average.
 */
RandomMartingale* martingale_create(double initial_capital);

/**
 * martingale_bet_at — Compute the martingale value after seeing
 * prefix σ. If the sequence is non-random, the martingale grows
 * without bound. (Schnorr 1971)
 */
void martingale_update(RandomMartingale* m, const RandomBitString* prefix);

/**
 * martingale_is_schnorr — Check if the martingale is computable
 * (Schnorr randomness). Returns 1 if the limit is computable.
 */
int martingale_is_schnorr(const RandomMartingale* m);

/**
 * martingale_free — Release martingale memory.
 */
void martingale_free(RandomMartingale* m);

/* ══════════════════════════════════════════════════════════════
   L4: Fundamental Theorems
   ══════════════════════════════════════════════════════════════ */

/**
 * ml_universal_test_theorem — Martin-Löf (1966):
 *   There exists a universal sequential test δ such that
 *   x is ML-random iff ∃c ∀m δ(x|m) ≤ c.
 *
 * This function constructs such a universal test for a given
 * sequence by aggregating all known effective tests.
 */
MLTest* ml_universal_test(void);

/**
 * equivalence_theorem_check — Verify the three-way equivalence:
 *   ML-random ⇔ Schnorr-random ⇔ incompressible (for computable measures).
 * Returns a confidence score [0,1].
 */
double equivalence_theorem_check(const RandomBitString* x);

/**
 * no_halting_detector — Theorem: There is no general algorithm
 * to decide if a Turing machine halts. Corollary: there is no
 * computable randomness test that perfectly classifies all sequences.
 * This function demonstrates this via diagonalization on finite data.
 */
int no_halting_detector_demonstration(const RandomBitString* x, int max_steps);

/* ══════════════════════════════════════════════════════════════
   L5: Algorithms — Statistical Randomness Tests
   ══════════════════════════════════════════════════════════════ */

/**
 * frequency_test — Monobit test: proportion of 1s should be ≈ 0.5.
 * Returns p-value (two-tailed).
 */
double frequency_test(const RandomBitString* s);

/**
 * runs_test — Count runs of consecutive identical bits.
 * Test statistic follows χ² distribution.
 */
double runs_test(const RandomBitString* s);

/**
 * longest_run_of_ones — Maximum consecutive 1s.
 * NIST SP 800-22 test.
 */
double longest_run_test(const RandomBitString* s);

/**
 * binary_matrix_rank — Test for linear dependence among
 * fixed-length blocks (Marsaglia's DIEHARD test).
 */
double binary_matrix_rank_test(const RandomBitString* s, int matrix_size);

/**
 * cumulative_sums_test — Forward/reverse cumulative sums.
 * Should stay within random-walk bounds.
 */
double cumulative_sums_test(const RandomBitString* s);

/**
 * approximate_entropy_test — Compare block entropy to uniform
 * distribution entropy (ApEn test from NIST SP 800-22).
 */
double approximate_entropy_test(const RandomBitString* s, int block_size);

/**
 * serial_test — Frequency of all possible overlapping m-bit patterns.
 */
double serial_test(const RandomBitString* s, int pattern_len);

/**
 * spectral_test — DFT-based test for periodic features.
 */
double spectral_test(const RandomBitString* s);

/**
 * autocorrelation_test — Lag-k autocorrelation test.
 */
double autocorrelation_test(const RandomBitString* s, int lag);

/**
 * poker_test — χ² test on frequencies of m-bit blocks.
 */
double poker_test(const RandomBitString* s, int m);

/* ══════════════════════════════════════════════════════════════
   L5: NIST SP 800-22 Test Battery (complete implementations)
   ══════════════════════════════════════════════════════════════ */

#define NIST_N_TESTS 15

typedef struct {
    char    name[64];
    double  p_value;
    int     passed;        /* 1 if p_value ≥ 0.01 */
} NISTTestResult;

typedef struct {
    NISTTestResult results[NIST_N_TESTS];
    int            n_passed;
    int            n_total;
    double         min_p_value;
    double         uniformity_pvalue;  /* p-value of the p-values */
} NISTTestBattery;

/**
 * nist_test_battery_run — Run all 15 NIST SP 800-22 tests
 * on a bit string. Returns comprehensive results.
 */
NISTTestBattery* nist_test_battery_run(const RandomBitString* s);
void nist_test_battery_print(const NISTTestBattery* b);
void nist_test_battery_free(NISTTestBattery* b);

/* ══════════════════════════════════════════════════════════════
   L6: Canonical Problems
   ══════════════════════════════════════════════════════════════ */

/**
 * randomness_profile_create — Compute comprehensive randomness profile.
 * Merges ML-theoretic, statistical, and compression-based metrics.
 */
RandomnessProfile* randomness_profile_create(const RandomBitString* s);

/**
 * randomness_profile_classify — Classify the randomness type
 * based on profile metrics.
 */
RandomnessType randomness_profile_classify(const RandomnessProfile* p);

void randomness_profile_print(const RandomnessProfile* p);
void randomness_profile_free(RandomnessProfile* p);

/* ══════════════════════════════════════════════════════════════
   Bit String Utilities
   ══════════════════════════════════════════════════════════════ */

RandomBitString* rbs_create(size_t bit_capacity);
RandomBitString* rbs_from_bytes(const unsigned char* bytes, size_t n_bytes);
RandomBitString* rbs_from_hex(const char* hex);
RandomBitString* rbs_from_binary(const char* bin_str);
RandomBitString* rbs_random(size_t n_bits, unsigned int* seed);
RandomBitString* rbs_clone(const RandomBitString* s);
void rbs_append_bit(RandomBitString* s, int bit);
void rbs_append_byte(RandomBitString* s, unsigned char byte);
int  rbs_get_bit(const RandomBitString* s, size_t i);
void rbs_set_bit(RandomBitString* s, size_t i, int bit);
void rbs_flip_bit(RandomBitString* s, size_t i);
size_t rbs_count_ones(const RandomBitString* s);
double rbs_density(const RandomBitString* s);
int rbs_equals(const RandomBitString* a, const RandomBitString* b);
void rbs_print(const RandomBitString* s, size_t max_bits);
void rbs_free(RandomBitString* s);

#endif /* RANDOMNESS_H */
