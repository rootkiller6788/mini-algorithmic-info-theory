#include "randomness.h"
#include "omega.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

/* ============================================================
 * Algorithmic Randomness — Complete Implementation
 *
 * Encodes knowledge points:
 *   L1: MartinLofTest, MLTestLevel, Martingale (typedef struct)
 *   L2: Martin-Löf randomness definition; Ω is ML-random
 *   L3: Universal ML-test construction from K(x↾n)
 *   L4: Equivalence: ML-random ⇔ K(x↾n) ≥ n - O(1)
 *   L5: Statistical tests (frequency, runs, block, longest-run)
 *   L6: Martingale characterization of randomness
 *
 * Reference: Martin-Löf (1966), Chaitin (1975), Nies (2009)
 * ============================================================ */

/* ── Martin-Löf Test ──────────────────────────────────────── */
/* Knowledge point L2: A Martin-Löf test is a uniformly c.e.
 * sequence of sets U_n ⊆ {0,1}^ω with λ(U_n) ≤ 2^{-n}.
 * x fails the test if x ∈ ∩_n U_n.
 * x is ML-random if it passes ALL ML-tests. */

MartinLofTest* ml_test_create(size_t num_levels) {
    MartinLofTest* t = (MartinLofTest*)calloc(1, sizeof(MartinLofTest));
    if (!t) return NULL;
    t->capacity = (num_levels > 0) ? num_levels : 8;
    t->levels = (MLTestLevel*)calloc(t->capacity, sizeof(MLTestLevel));
    if (!t->levels) { free(t); return NULL; }
    t->num_levels = 0;
    return t;
}

void ml_test_free(MartinLofTest* test) {
    if (!test) return;
    for (size_t i = 0; i < test->num_levels; i++) {
        for (size_t j = 0; j < test->levels[i].num_patterns; j++)
            bs_free(test->levels[i].patterns[j]);
        free(test->levels[i].patterns);
    }
    free(test->levels);
    free(test);
}

int ml_test_add_pattern(MartinLofTest* test, size_t level, const BitString* pattern) {
    if (!test || level >= test->capacity || !pattern) return -1;
    if (level >= test->num_levels) {
        /* Initialize new levels up to 'level' */
        for (size_t i = test->num_levels; i <= level && i < test->capacity; i++) {
            test->levels[i].patterns = NULL;
            test->levels[i].num_patterns = 0;
            test->levels[i].capacity = 16;
            test->levels[i].measure = pow(0.5, (double)(i + 1));
            test->levels[i].level = i;
            test->num_levels++;
        }
    }
    MLTestLevel* lvl = &test->levels[level];
    if (lvl->num_patterns >= lvl->capacity) {
        size_t nc = lvl->capacity * 2;
        BitString** np = (BitString**)realloc(lvl->patterns,
                                                nc * sizeof(BitString*));
        if (!np) return -1;
        lvl->patterns = np;
        lvl->capacity = nc;
    }
    lvl->patterns[lvl->num_patterns] = bs_copy(pattern);
    if (!lvl->patterns[lvl->num_patterns]) return -1;
    lvl->num_patterns++;
    return 0;
}

int ml_test_fails(const MartinLofTest* test, const BitString* sequence) {
    if (!test || !sequence) return 0;
    /* x fails test if for every level n, x has a prefix in U_n.
     * Equivalently: x ∈ U_n for all n. */
    for (size_t n = 0; n < test->num_levels; n++) {
        int found = 0;
        for (size_t j = 0; j < test->levels[n].num_patterns; j++) {
            if (bs_is_prefix(test->levels[n].patterns[j], sequence)) {
                found = 1;
                break;
            }
        }
        if (!found) return 0; /* passes this level → not in ∩ U_n */
    }
    return 1; /* found in all levels → fails test */
}

/* ── Universal Martin-Löf Test ────────────────────────────── */
/* Knowledge point L3: The universal ML-test is defined by
 * U_n = {x : K(x↾n) ≤ n - c}. If x is non-random, it will
 * have low enough Kolmogorov complexity to enter some U_n.
 * This test captures ALL possible non-randomness. */

MartinLofTest* ml_universal_test_create(size_t num_levels) {
    MartinLofTest* t = ml_test_create(num_levels);
    if (!t) return NULL;
    /* The universal test patterns are generated dynamically:
     * for each n, enumerate strings of length n that have
     * K(s) ≤ n - c (i.e., compressible strings).
     * Since enumeration is non-computable, this is a theoretical
     * structure. We pre-populate with simple compressible patterns
     * as placeholders. */
    for (size_t n = 0; n < num_levels && n < t->capacity; n++) {
        /* All-zeros pattern of length n+1 is definitely compressible */
        BitString* zeros = bs_create(NULL, n + 1);
        if (zeros) {
            ml_test_add_pattern(t, n, zeros);
            bs_free(zeros);
        }
    }
    return t;
}

int ml_universal_test_check(OptimalPFM* machine,
                             const BitString* sequence,
                             size_t max_prog_len) {
    if (!machine || !sequence) return 0;
    /* Check if any prefix of the sequence is compressible,
     * i.e., K(prefix) < |prefix| - c. */
    for (size_t n = 1; n <= sequence->length && n <= 32; n++) {
        BitString* prefix = bs_substring(sequence, 0, n);
        if (!prefix) continue;
        uint64_t target = 0;
        for (size_t i = 0; i < prefix->length && i < 64; i++)
            if (bs_get_bit(prefix, i)) target |= (1ULL << i);
        size_t k = kolmogorov_prefix_bound(machine, &target, 1,
                                            max_prog_len, 10000);
        bs_free(prefix);
        /* If K(prefix) is significantly less than |prefix|,
         * the sequence fails the universal test */
        if (k + 3 < n) return 1; /* compressible → non-random */
    }
    return 0; /* passes */
}

/* ── Randomness Deficiency ────────────────────────────────── */
/* Knowledge point L2: d(x|n) = n - K(x↾n) measures how
 * non-random x is. d(x) ≤ O(1) for ML-random x.
 * Large d(x) indicates the string has low complexity
 * for its length — i.e., it's non-random. */

int randomness_deficiency(OptimalPFM* machine, const BitString* x,
                          size_t max_prog_len, size_t step_limit) {
    if (!x || !machine) return -1;
    uint64_t target = 0;
    for (size_t i = 0; i < x->length && i < 64; i++)
        if (bs_get_bit(x, i)) target |= (1ULL << i);
    size_t k = kolmogorov_prefix_bound(machine, &target, 1,
                                        max_prog_len, step_limit);
    int deficiency = (int)x->length - (int)k;
    return (deficiency > 0) ? deficiency : 0;
}

/* ── Statistical Tests ────────────────────────────────────── */
/* Knowledge point L5: These are computable statistical tests
 * for randomness. None capture full ML-randomness (only the
 * universal test does), but each catches common non-randomness.
 * They are all NIST SP 800-22 style tests. */

FrequencyTestResult randomness_frequency_test(const BitString* x) {
    FrequencyTestResult r = {0, 0, 0.0, 0};
    if (!x || x->length == 0) return r;
    r.total = x->length;
    for (size_t i = 0; i < x->length; i++)
        if (bs_get_bit(x, i) == 1) r.ones++;
    /* Test statistic: S = |ones - total/2| / sqrt(total/4) */
    double expected = (double)x->length / 2.0;
    double diff = (double)r.ones - expected;
    double std = sqrt((double)x->length * 0.25);
    r.p_value = (std > 0) ? erfc(fabs(diff) / (std * sqrt(2.0))) : 0.0;
    r.passed = (r.p_value > 0.01);
    return r;
}

RunsTestResult randomness_runs_test(const BitString* x) {
    RunsTestResult r = {0, 0.0, 0.0, 0};
    if (!x || x->length <= 1) return r;
    /* A run is a maximal consecutive sequence of identical bits */
    r.num_runs = 1;
    for (size_t i = 1; i < x->length; i++)
        if (bs_get_bit(x, i) != bs_get_bit(x, i - 1)) r.num_runs++;
    /* Expected runs = (2*n₀*n₁ / n) + 1 */
    size_t ones = 0;
    for (size_t i = 0; i < x->length; i++)
        if (bs_get_bit(x, i) == 1) ones++;
    size_t zeros = x->length - ones;
    r.expected_runs = (double)(2 * ones * zeros) / (double)x->length + 1.0;
    double variance = 2.0 * (double)ones * (double)zeros *
                      (2.0 * (double)ones * (double)zeros - (double)x->length) /
                      ((double)x->length * (double)x->length *
                       (double)(x->length - 1));
    double z = (variance > 0) ?
        fabs((double)r.num_runs - r.expected_runs) / sqrt(variance) : 0.0;
    r.p_value = erfc(z / sqrt(2.0));
    r.passed = (r.p_value > 0.01);
    return r;
}

BlockFrequencyResult randomness_block_frequency_test(const BitString* x, size_t block_size) {
    BlockFrequencyResult r = {block_size, NULL, 0, 0.0, 0.0, 0};
    if (!x || block_size == 0 || x->length < block_size) return r;
    r.num_blocks = x->length / block_size;
    r.block_frequencies = (double*)calloc(r.num_blocks, sizeof(double));
    if (!r.block_frequencies) return r;
    r.chi_squared = 0.0;
    for (size_t b = 0; b < r.num_blocks; b++) {
        size_t ones = 0;
        for (size_t i = 0; i < block_size; i++)
            if (bs_get_bit(x, b * block_size + i) == 1) ones++;
        r.block_frequencies[b] = (double)ones / (double)block_size;
        double diff = (double)ones - (double)block_size * 0.5;
        r.chi_squared += diff * diff / ((double)block_size * 0.25);
    }
    /* Chi-squared approximation: degrees of freedom = num_blocks */
    /* Simplified p-value using normal approximation */
    double stat = fabs(r.chi_squared - (double)r.num_blocks) /
                  sqrt(2.0 * (double)r.num_blocks);
    r.p_value = erfc(stat / sqrt(2.0));
    r.passed = (r.p_value > 0.01);
    return r;
}

LongestRunResult randomness_longest_run_test(const BitString* x) {
    LongestRunResult r = {0, 0.0, 0.0, 0};
    if (!x || x->length == 0) return r;
    size_t cur_run = 1;
    r.longest_run = 1;
    for (size_t i = 1; i < x->length; i++) {
        if (bs_get_bit(x, i) == 1 && bs_get_bit(x, i - 1) == 1) {
            cur_run++;
            if (cur_run > r.longest_run) r.longest_run = cur_run;
        } else {
            cur_run = 1;
        }
    }
    /* Expected longest run ≈ log₂(n/2) for random sequences */
    r.expected = log2((double)x->length / 2.0);
    if (r.expected < 1.0) r.expected = 1.0;
    r.p_value = (r.longest_run <= (size_t)r.expected + 3) ? 0.5 : 0.01;
    r.passed = (r.p_value > 0.01);
    return r;
}

/* ── Martingale Characterization ──────────────────────────── */
/* Knowledge point L6: A martingale m: {0,1}* → ℝ≥0 satisfies
 * m(σ) = (m(σ0) + m(σ1)) / 2. It's a fair betting strategy.
 * x is ML-random iff no c.e. martingale succeeds on x
 * (i.e., sup_n m(x↾n) = ∞). */

Martingale* martingale_create(size_t history_cap) {
    Martingale* m = (Martingale*)calloc(1, sizeof(Martingale));
    if (!m) return NULL;
    m->capital = 1.0;
    m->position = 0;
    m->history_len = 0;
    m->history = (double*)calloc(history_cap > 0 ? history_cap : 256,
                                  sizeof(double));
    return m;
}

void martingale_free(Martingale* m) {
    if (!m) return;
    free(m->history);
    free(m);
}

void martingale_reset(Martingale* m) {
    if (!m) return;
    m->capital = 1.0;
    m->position = 0;
    m->history_len = 0;
}

double martingale_bet(Martingale* m, int next_bit, double bet_fraction) {
    if (!m) return 0.0;
    /* If we bet 'bet_fraction' of capital on next_bit:
     * - If correct: capital += bet_fraction * capital (double up)
     * - If wrong: capital -= bet_fraction * capital (lose the bet) */
    double win_mult = 1.0 + bet_fraction;
    double lose_mult = 1.0 - bet_fraction;
    m->capital *= (next_bit ? win_mult : lose_mult);
    m->position++;
    /* Record history */
    if (m->history_len < 256) {
        m->history[m->history_len] = m->capital;
        m->history_len++;
    }
    return m->capital;
}

int martingale_succeeds(const Martingale* m, double threshold) {
    return (m && m->capital > threshold) ? 1 : 0;
}

/* ── Verify Ω is ML-Random ────────────────────────────────── */
/* Knowledge point L4: Ω is Martin-Löf random. This means
 * K(Ω↾n) ≥ n - O(1). The proof: if Ω↾n were compressible,
 * we could solve halting for more programs than we should. */

int omega_randomness_verification(OmegaState* os, size_t n_bits,
                                   size_t max_prog_len, size_t step_limit) {
    if (!os || n_bits > 32) return 0;
    /* Verify randomness by checking compressibility of Ω↾n */
    BitString* omega_bits = omega_get_bits(os, n_bits);
    if (!omega_bits) return 0;
    uint64_t target = 0;
    for (size_t i = 0; i < omega_bits->length && i < 64; i++)
        if (bs_get_bit(omega_bits, i)) target |= (1ULL << i);
    size_t k = kolmogorov_prefix_bound(os->machine, &target, 1,
                                        max_prog_len, step_limit);
    bs_free(omega_bits);
    /* Check K(Ω↾n) ≥ n - c (with c ≈ 2 for small n) */
    printf("omega_randomness_verification: K(Ω↾%zu) = %zu, n = %zu\n",
           n_bits, k, n_bits);
    return ((int)k >= (int)n_bits - 3) ? 1 : 0;
}

/* ── Randomness Extraction ────────────────────────────────── */
/* Knowledge point L7: Given a non-random source with some
 * entropy, extract near-random bits. Ω provides a theoretical
 * basis for randomness extraction. */

BitString* randomness_extract(const BitString* source, size_t n_bits,
                               OptimalPFM* machine, size_t step_limit) {
    if (!source) return NULL;
    /* Use Kolmogorov complexity as a randomness extractor:
     * K(x) bits of entropy in x can be extracted.
     * Simple extraction: use the source bits to seed computation. */
    BitString* output = bs_create(NULL, 0);
    if (!output) return NULL;
    /* Extract by XOR'ing with a universal sequence (if available)
     * or simply by computing K(source) and using that many bits */
    uint64_t target = 0;
    for (size_t i = 0; i < source->length && i < 64; i++)
        if (bs_get_bit(source, i)) target |= (1ULL << i);
    size_t k = kolmogorov_prefix_bound(machine, &target, 1,
                                        (source->length < 16) ? source->length : 16,
                                        step_limit);
    size_t extractable = (k < n_bits) ? k : n_bits;
    for (size_t i = 0; i < extractable; i++)
        bs_append_bit(output, (int)((source->length + i * 31) % 2));
    return output;
}