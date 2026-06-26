#ifndef RANDOMNESS_H
#define RANDOMNESS_H

#include "binary_string.h"
#include "kolmogorov.h"
#include "omega.h"
#include <stdint.h>

/* ============================================================
 * Algorithmic Randomness — Martin-Löf Tests
 *
 * A sequence is algorithmically random (Martin-Löf random) if
 * it passes ALL effective (computably enumerable) statistical
 * tests. Ω is the canonical example of an ML-random sequence.
 *
 * Knowledge points:
 *   - Martin-Löf randomness definition
 *   - Universal Martin-Löf test
 *   - Ω is 1-random (Martin-Löf random)
 *   - Equivalent characterizations (martingales, compressibility)
 *   - Randomness deficiency for finite strings
 *   - Statistical tests: frequency, runs, universal test
 *   - Relation to Kolmogorov complexity: x is random iff K(x) ≥ |x| - O(1)
 * ============================================================ */

/* ---- Martin-Löf Test ---- */
/* A Martin-Löf test is a uniformly c.e. sequence of open sets
 * U₁ ⊇ U₂ ⊇ ... in Cantor space, with λ(U_n) ≤ 2^{-n}.
 * A sequence x fails the test if x ∈ ∩_n U_n.
 * x is ML-random if it passes all ML-tests. */

typedef struct {
    BitString** patterns;   /* patterns defining the open set */
    size_t      num_patterns;
    size_t      capacity;
    double      measure;    /* λ(U_n) ≤ 2^{-n} */
    size_t      level;      /* n in U_n */
} MLTestLevel;

typedef struct {
    MLTestLevel* levels;
    size_t       num_levels;
    size_t       capacity;
} MartinLofTest;

MartinLofTest* ml_test_create(size_t num_levels);
void           ml_test_free(MartinLofTest* test);
int            ml_test_add_pattern(MartinLofTest* test, size_t level, const BitString* pattern);
int            ml_test_fails(const MartinLofTest* test, const BitString* sequence);

/* ---- Universal Martin-Löf Test ---- */
/* There exists a universal ML-test: if a sequence fails ANY ML-test,
 * it fails the universal one. The universal test is based on:
 *   U_n = {x : K(x↾n) ≤ n - c} for appropriate c. */
MartinLofTest* ml_universal_test_create(size_t num_levels);
int            ml_universal_test_check(OptimalPFM* machine,
                                        const BitString* sequence,
                                        size_t max_prog_len);

/* ---- Randomness Deficiency ---- */
/* d(x|n) = n - K(x) measures how far x is from being random.
 * d(x|n) ≥ 0 for most x. Large d indicates non-randomness. */
int randomness_deficiency(OptimalPFM* machine, const BitString* x,
                          size_t max_prog_len, size_t step_limit);

/* ---- Statistical Tests (all computable, none capture full ML-randomness) ---- */

/* Frequency test: check if proportion of 1s ≈ 1/2 */
typedef struct {
    size_t ones;
    size_t total;
    double p_value;
    int    passed;
} FrequencyTestResult;

FrequencyTestResult randomness_frequency_test(const BitString* x);

/* Runs test: check if runs of consecutive bits follow expected distribution */
typedef struct {
    size_t num_runs;
    double expected_runs;
    double p_value;
    int    passed;
} RunsTestResult;

RunsTestResult randomness_runs_test(const BitString* x);

/* Block frequency test: check frequency in blocks of size m */
typedef struct {
    size_t block_size;
    double* block_frequencies;
    size_t  num_blocks;
    double  chi_squared;
    double  p_value;
    int     passed;
} BlockFrequencyResult;

BlockFrequencyResult randomness_block_frequency_test(const BitString* x, size_t block_size);

/* Longest run of ones test */
typedef struct {
    size_t longest_run;
    double expected;
    double p_value;
    int    passed;
} LongestRunResult;

LongestRunResult randomness_longest_run_test(const BitString* x);

/* ---- Martingale Characterization ---- */
/* A martingale is a betting strategy: m(σ) = (m(σ0)+m(σ1))/2.
 * x is ML-random iff no c.e. martingale succeeds on x. */

typedef struct {
    double  capital;
    size_t  position;
    double* history;
    size_t  history_len;
} Martingale;

Martingale* martingale_create(size_t history_cap);
void        martingale_free(Martingale* m);
void        martingale_reset(Martingale* m);
double      martingale_bet(Martingale* m, int next_bit, double bet_fraction);
int         martingale_succeeds(const Martingale* m, double threshold);

/* ---- Verify Ω is ML-Random ---- */
/* Demonstrates that the first n bits of Ω satisfy:
 * K(Ω↾n) ≥ n - c for some constant c (i.e., are incompressible). */
int omega_randomness_verification(OmegaState* os, size_t n_bits,
                                   size_t max_prog_len, size_t step_limit);

/* ---- Randomness Extraction ---- */
/* Given a non-random source with some entropy, extract
 * near-random bits. Ω provides a theoretically perfect source. */
BitString* randomness_extract(const BitString* source, size_t n_bits,
                               OptimalPFM* machine, size_t step_limit);

#endif /* RANDOMNESS_H */
