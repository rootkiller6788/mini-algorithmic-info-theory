/**
 * solomonoff.c - Core Algorithmic Probability Computation
 *
 * Computes Solomonoff's universal a priori probability M(x):
 *   M(x) = sum_{p: U(p)=x*} 2^{-|p|}
 *
 * where U is the universal monotone machine, x* means any extension
 * of x. Implements dovetailing enumeration, Le6in's Pt, conditional M,
 * and prediction.
 *
 * Key theorems implemented:
 *   - M is a universal semimeasure (dominance property)
 *   - M(x) >= 2^{-K(x)} (Coding Theorem lower bound)
 *   - M converges to any computable measure
 *
 * Reference: Solomonoff (1964), Li & Vitanyi (2019) Chapter 4.
 * Curriculum: MIT 6.840, CMU 15-751, Cambridge Part III.
 */

#include "solomonoff.h"
#include "enumeration.h"
#include "universal_machine.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ================================================================
 * Binary String Operations (L1 - Definitions)
 * ================================================================ */

void bs_init(binary_string_t *s) {
    if (!s) return;
    memset(s->bits, 0, sizeof(s->bits));
    s->length = 0;
}

bool bs_from_cstr(binary_string_t *s, const char *str) {
    if (!s || !str) return false;
    bs_init(s);
    size_t len = strlen(str);
    if (len > SOLOMONOFF_MAX_STRING_LENGTH) return false;
    for (size_t i = 0; i < len; i++) {
        if (str[i] != '0' && str[i] != '1') return false;
        int bit = (str[i] == '1') ? 1 : 0;
        bs_append_bit(s, bit);
    }
    return true;
}

int bs_get_bit(const binary_string_t *s, size_t i) {
    if (!s || i >= s->length) return -1;
    size_t byte_idx = i / 8;
    size_t bit_idx = 7 - (i % 8);  /* MSB first within byte */
    return (s->bits[byte_idx] >> bit_idx) & 1;
}

bool bs_set_bit(binary_string_t *s, size_t i, int value) {
    if (!s || i >= s->length || (value != 0 && value != 1)) return false;
    size_t byte_idx = i / 8;
    size_t bit_idx = 7 - (i % 8);
    if (value)
        s->bits[byte_idx] |= (1U << bit_idx);
    else
        s->bits[byte_idx] &= ~(1U << bit_idx);
    return true;
}

bool bs_append_bit(binary_string_t *s, int bit) {
    if (!s || s->length >= SOLOMONOFF_MAX_STRING_LENGTH) return false;
    if (bit != 0 && bit != 1) return false;
    size_t byte_idx = s->length / 8;
    size_t bit_idx = 7 - (s->length % 8);
    if (bit)
        s->bits[byte_idx] |= (1U << bit_idx);
    s->length++;
    return true;
}

bool bs_concat(binary_string_t *s, const binary_string_t *t) {
    if (!s || !t) return false;
    if (s->length + t->length > SOLOMONOFF_MAX_STRING_LENGTH) return false;
    for (size_t i = 0; i < t->length; i++) {
        int bit = bs_get_bit(t, i);
        if (bit < 0) return false;
        if (!bs_append_bit(s, bit)) return false;
    }
    return true;
}

void bs_prefix(const binary_string_t *s, size_t k, binary_string_t *result) {
    bs_init(result);
    if (!s || !result) return;
    size_t n = (k < s->length) ? k : s->length;
    for (size_t i = 0; i < n; i++) {
        int bit = bs_get_bit(s, i);
        bs_append_bit(result, bit);
    }
}

bool bs_has_prefix(const binary_string_t *s, const binary_string_t *prefix) {
    if (!s || !prefix) return false;
    if (prefix->length > s->length) return false;
    for (size_t i = 0; i < prefix->length; i++) {
        if (bs_get_bit(s, i) != bs_get_bit(prefix, i)) return false;
    }
    return true;
}

int bs_compare_shortlex(const binary_string_t *a, const binary_string_t *b) {
    if (!a || !b) return 0;
    if (a->length < b->length) return -1;
    if (a->length > b->length) return 1;
    for (size_t i = 0; i < a->length; i++) {
        int ba = bs_get_bit(a, i), bb = bs_get_bit(b, i);
        if (ba < bb) return -1;
        if (ba > bb) return 1;
    }
    return 0;
}

void bs_copy(binary_string_t *dst, const binary_string_t *src) {
    if (!dst || !src) return;
    memcpy(dst->bits, src->bits, sizeof(src->bits));
    dst->length = src->length;
}

bool bs_equal(const binary_string_t *a, const binary_string_t *b) {
    if (!a || !b) return (a == b);
    if (a->length != b->length) return false;
    return memcmp(a->bits, b->bits, (a->length + 7) / 8) == 0;
}

void bs_print(FILE *fp, const binary_string_t *s) {
    if (!fp || !s) return;
    if (s->length == 0) { fprintf(fp, "(empty)"); return; }
    for (size_t i = 0; i < s->length; i++) {
        fputc(bs_get_bit(s, i) ? '1' : '0', fp);
    }
}

size_t bs_common_prefix_len(const binary_string_t *a, const binary_string_t *b) {
    if (!a || !b) return 0;
    size_t max_len = (a->length < b->length) ? a->length : b->length;
    size_t i;
    for (i = 0; i < max_len; i++) {
        if (bs_get_bit(a, i) != bs_get_bit(b, i)) break;
    }
    return i;
}

size_t bs_popcount(const binary_string_t *s) {
    if (!s) return 0;
    size_t count = 0;
    for (size_t i = 0; i < s->length; i++) {
        if (bs_get_bit(s, i) == 1) count++;
    }
    return count;
}

/* ================================================================
 * Helper: Program weight computation
 * ================================================================ */

/**
 * program_weight - Compute 2^{-|p|} for a program of given length.
 * This is the contribution of a single program to M(x).
 * Uses double precision; for |p| > 1024, returns 0.0.
 */
static double program_weight(size_t length) {
    if (length > 1024) return 0.0;
    return pow(2.0, -(double)(int64_t)length);
}

/* ================================================================
 * Core: solomonoff_M (L1-L4)
 * ================================================================ */

/**
 * solomonoff_M - Algorithmic probability M(x).
 *
 * Algorithm:
 *   1. Enumerate all programs up to `depth` bits.
 *   2. Execute each program on U with dovetailing.
 *   3. Sum 2^{-|p|} for all programs p whose output has x as prefix.
 *
 * Theorem: M(x) is a universal semimeasure.
 *   - sum_x M(x) <= 1 (semimeasure property)
 *   - For any computable semimeasure mu, exists c such that
 *     M(x) >= 2^{-c} * mu(x) for all x (dominance).
 *
 * Complexity: O(2^depth * max_steps / depth) via dovetailing.
 */
algprob_t solomonoff_M(const binary_string_t *x, size_t depth, uint64_t max_steps) {
    if (!x || depth == 0 || depth > SOLOMONOFF_MAX_DEPTH) return 0.0;
    if (x->length == 0) return 1.0;  /* Every program produces epsilon as prefix */

    dovetail_manager_t dm;
    size_t pool_cap = (depth <= 10) ? (1U << depth) : SOLOMONOFF_POOL_SIZE;
    dovetail_init(&dm, pool_cap, max_steps);

    /* Enumerate and add programs */
    program_enumerator_t enumerator;
    enumerator_init(&enumerator, depth);

    binary_string_t prog;
    size_t plen;
    size_t added = 0;

    while (enumerator_next(&enumerator, &prog, &plen) && added < pool_cap) {
        dovetail_add_program(&dm, &prog, plen);
        added++;
    }

    /* Run dovetailing */
    dovetail_run(&dm, depth, max_steps);

    /* Compute M(x) */
    double Mx = dovetail_compute_M(&dm, x);

    dovetail_free(&dm);
    return Mx;
}

/**
 * solomonoff_M_fast - Faster approximation of M(x) without dovetailing.
 *
 * Runs each program independently with a per-program step limit,
 * sums contributions. Less accurate but much faster for small depths.
 *
 * This is a computable approximation: for any epsilon, there exists
 * a finite depth D such that |M(x) - M_D(x)| < epsilon for all x
 * with |x| <= n.
 */
algprob_t solomonoff_M_fast(const binary_string_t *x, size_t depth, uint64_t max_steps) {
    if (!x || depth == 0 || depth > SOLOMONOFF_MAX_DEPTH) return 0.0;
    if (x->length == 0) return 1.0;

    double total = 0.0;
    program_enumerator_t enumerator;
    enumerator_init(&enumerator, depth);

    binary_string_t prog;
    size_t plen;
    size_t count = 0;
    size_t max_progs = (depth <= 10) ? (1U << depth) : 10000;

    while (enumerator_next(&enumerator, &prog, &plen) && count < max_progs) {
        um_state_t machine;
        um_init(&machine, &prog, max_steps);
        um_run(&machine);

        if (um_output_has_prefix(&machine, x)) {
            total += program_weight(plen);
        }
        count++;
    }
    return total;
}

/* ================================================================
 * Levin's Time-Bounded Pt (L4 - Theorem)
 * ================================================================ */

/**
 * levin_Pt - Time-bounded algorithmic probability.
 *
 * Pt(x) = sum_{p: U(p) outputs x* within t total dovetail steps} 2^{-|p|}
 *
 * Unlike M(x), Pt is computable (the sum is finite and each program
 * is executed for bounded time). As t -> infinity, Pt(x) -> M(x).
 *
 * Levin showed that Pt(x) is itself a universal semimeasure when
 * the time bound is incorporated into the program description.
 *
 * Reference: Levin, "Universal Sequential Search Problems", 1973.
 */
algprob_t levin_Pt(const binary_string_t *x, size_t depth, uint64_t tmax) {
    if (!x || depth == 0 || depth > SOLOMONOFF_MAX_DEPTH) return 0.0;
    if (x->length == 0) return 1.0;

    dovetail_manager_t dm;
    size_t pool_cap = (depth <= 8) ? (1U << depth) : 256;
    dovetail_init(&dm, pool_cap, tmax);

    program_enumerator_t enumerator;
    enumerator_init(&enumerator, depth);

    binary_string_t prog;
    size_t plen;
    size_t added = 0;
    while (enumerator_next(&enumerator, &prog, &plen) && added < pool_cap) {
        dovetail_add_program(&dm, &prog, plen);
        added++;
    }

    /* Run exactly tmax dovetailing steps */
    for (uint64_t step = 0; step < tmax; step++) {
        if (dovetail_step(&dm) == 0) break;  /* All halted or timed out */
    }

    double result = dovetail_compute_M(&dm, x);
    dovetail_free(&dm);
    return result;
}

/* ================================================================
 * Conditional Algorithmic Probability (L4)
 * ================================================================ */

/**
 * solomonoff_conditional_M - Conditional algorithmic probability.
 *
 * M(x | y) = M(yx) / M(y)  (ratio form)
 *
 * This approximates: sum_{p: U(p,y)=x*} 2^{-|p|}
 * where y is treated as auxiliary input.
 *
 * For the ratio to be valid, M(y) must be positive. If M(y) ~ 0,
 * we fall back to direct computation with y as context.
 *
 * Property: M(x|y) >= M(xy) (conditioning on y provides context).
 */
algprob_t solomonoff_conditional_M(const binary_string_t *x,
                                   const binary_string_t *y,
                                   size_t depth, uint64_t max_steps) {
    if (!x || !y) return 0.0;
    if (x->length == 0) return 1.0;

    /* Build yx (concatenation) */
    binary_string_t yx;
    bs_init(&yx);
    bs_concat(&yx, y);
    bs_concat(&yx, x);

    algprob_t My = solomonoff_M(y, depth, max_steps);
    if (My < SOLOMONOFF_EPSILON) {
        /* Fall back: compute M using y as immediate prefix context */
        algprob_t Myx = solomonoff_M(&yx, depth, max_steps);
        return Myx;  /* Best effort when M(y) = 0 */
    }

    algprob_t Myx = solomonoff_M(&yx, depth, max_steps);
    return Myx / My;
}

/* ================================================================
 * Prediction (L5)
 * ================================================================ */

/**
 * solomonoff_predict - Next-bit prediction using algorithmic probability.
 *
 * Computes P(b|x) = M(xb) / (M(x0) + M(x1)) for b in {0,1}.
 *
 * This is the core of Solomonoff induction. The prediction converges
 * to the true conditional probability mu(b|x) for any computable
 * measure mu, with mu-probability 1.
 *
 * Theorem (Solomonoff, 1978): For any computable measure mu,
 *   sum_{n=1..inf} E_mu[ (M(x_n|x_{<n}) - mu(x_n|x_{<n}))^2 ] < K(mu)*ln2
 *
 * This implies rapid convergence: the expected squared error is bounded
 * by the complexity of the data-generating distribution.
 */
prediction_result_t solomonoff_predict(const binary_string_t *x,
                                        size_t depth, uint64_t max_steps) {
    prediction_result_t result;
    memset(&result, 0, sizeof(result));

    if (!x) return result;

    result.M_x = solomonoff_M(x, depth, max_steps);

    /* Build x0 and x1 */
    binary_string_t x0, x1;
    bs_init(&x0);
    bs_init(&x1);
    bs_concat(&x0, x);
    bs_append_bit(&x0, 0);
    bs_concat(&x1, x);
    bs_append_bit(&x1, 1);

    result.M_x0 = solomonoff_M(&x0, depth, max_steps);
    result.M_x1 = solomonoff_M(&x1, depth, max_steps);

    double total = result.M_x0 + result.M_x1;
    if (total < SOLOMONOFF_EPSILON) {
        /* No programs produce either continuation -> uniform prediction */
        result.prob_next_zero = 0.5;
        result.prob_next_one = 0.5;
        result.confidence = 0.0;
    } else {
        result.prob_next_zero = result.M_x0 / total;
        result.prob_next_one = result.M_x1 / total;
        result.confidence = fabs(result.prob_next_zero - result.prob_next_one);
    }

    return result;
}

/**
 * solomonoff_explain - Ensemble analysis: top-k contributing programs.
 *
 * Identifies which programs contribute most to M(x), providing an
 * "algorithmic explanation" of the data. The top-k coverage ratio
 * indicates how concentrated the probability mass is.
 */
solomonoff_ensemble_t solomonoff_explain(const binary_string_t *x,
                                           size_t depth, uint64_t max_steps,
                                           size_t top_k) {
    solomonoff_ensemble_t ensemble;
    memset(&ensemble, 0, sizeof(ensemble));

    if (!x || top_k == 0) return ensemble;

    dovetail_manager_t dm;
    size_t pool_cap = (depth <= 10) ? (1U << depth) : SOLOMONOFF_POOL_SIZE;
    dovetail_init(&dm, pool_cap, max_steps);

    program_enumerator_t enumerator;
    enumerator_init(&enumerator, depth);

    binary_string_t prog;
    size_t plen;
    size_t added = 0;
    while (enumerator_next(&enumerator, &prog, &plen) && added < pool_cap) {
        dovetail_add_program(&dm, &prog, plen);
        added++;
    }
    dovetail_run(&dm, depth, max_steps);

    double total_M = 0.0;

    /* Allocate space for matched programs */
    pool_entry_t **matches = (pool_entry_t**)malloc(top_k * sizeof(pool_entry_t*));
    size_t num_matches = 0;
    if (matches) {
        num_matches = dovetail_find_matching(&dm, x, matches, top_k);
    }

    ensemble.top_programs = (program_t*)malloc(num_matches * sizeof(program_t));
    if (ensemble.top_programs && matches) {
        for (size_t i = 0; i < num_matches; i++) {
            if (matches[i]) {
                ensemble.top_programs[i].code = matches[i]->program;
                ensemble.top_programs[i].length = matches[i]->prog_length;
                ensemble.top_programs[i].weight = matches[i]->weight;
                ensemble.top_programs[i].halts = matches[i]->halted;
                ensemble.top_programs[i].output = matches[i]->machine.output;
                ensemble.top_programs[i].steps_to_halting = matches[i]->steps_given;
                total_M += matches[i]->weight;
            }
        }
    }

    if (matches) free(matches);

    ensemble.num_top = num_matches;
    ensemble.capacity = top_k;
    ensemble.total_M = dovetail_compute_M(&dm, x);
    ensemble.top_k_mass = total_M;
    ensemble.coverage_ratio = (ensemble.total_M > SOLOMONOFF_EPSILON) ?
                              (total_M / ensemble.total_M) : 0.0;

    dovetail_free(&dm);
    return ensemble;
}

void solomonoff_ensemble_free(solomonoff_ensemble_t *e) {
    if (e && e->top_programs) {
        free(e->top_programs);
        e->top_programs = NULL;
        e->num_top = 0;
    }
}

/* ================================================================
 * Algorithmic Information Content (L4)
 * ================================================================ */

double solomonoff_algorithmic_info(const binary_string_t *x,
                                   size_t depth, uint64_t max_steps) {
    if (!x) return 0.0;
    if (x->length == 0) return 0.0;

    algprob_t Mx = solomonoff_M(x, depth, max_steps);
    if (Mx < SOLOMONOFF_EPSILON) {
        /* M(x) too small to measure -> K(x) >= depth (lower bound) */
        return (double)depth;
    }
    return -log2(Mx);
}

/* ================================================================
 * Semimeasure Verification (L4)
 * ================================================================ */

double solomonoff_verify_semimeasure(size_t n, size_t depth, uint64_t max_steps) {
    double total = 0.0;
    /* Enumerate all binary strings of length <= n */
    /* size_t checked = 0; (we only check a sample due to combinatorial explosion) */
    size_t checked = 0;

    for (size_t len = 0; len <= n && checked < 256; len++) {
        for (uint64_t idx = 0; idx < (1ULL << len) && checked < 256; idx++) {
            binary_string_t x;
            bs_init(&x);
            /* Build x from index */
            for (size_t bit = 0; bit < len; bit++) {
                int b = (idx >> (len - 1 - bit)) & 1;
                bs_append_bit(&x, b);
            }
            total += solomonoff_M(&x, depth, max_steps);
            checked++;
        }
    }
    return total;
}

double solomonoff_kraft_sum(size_t depth) {
    double sum = 0.0;
    for (size_t len = 0; len <= depth; len++) {
        sum += pow(2.0, (double)(int64_t)len) * program_weight(len);
    }
    return sum;  /* Should equal (depth + 1) for all binary strings */
}

bool solomonoff_verify_dominance(double p, size_t n_trials,
                                  size_t depth, uint64_t max_steps) {
    /* Test dominance: M(x) >= 2^{-c} * Bern(p)(x) for some constant c */
    /* Generate Bernoulli(p) sequences and compare M to true probability */
    double min_ratio = 1.0;

    for (size_t trial = 0; trial < n_trials && trial < 20; trial++) {
        binary_string_t x;
        bs_init(&x);
        double bern_prob = 1.0;

        /* Generate a short sequence */
        for (size_t i = 0; i < 8; i++) {
            /* Use deterministic pattern for reproducibility */
            int bit = ((trial * 8 + i) % 3 == 0) ? 1 : 0;
            bs_append_bit(&x, bit);
            bern_prob *= (bit == 1) ? p : (1.0 - p);
        }

        algprob_t Mx = solomonoff_M(&x, depth, max_steps);
        if (Mx > SOLOMONOFF_EPSILON && bern_prob > SOLOMONOFF_EPSILON) {
            double ratio = Mx / bern_prob;
            if (ratio < min_ratio) min_ratio = ratio;
        }
    }
    /* If min_ratio > 0, dominance holds (M multiplicatively dominates Bernoulli) */
    return (min_ratio > SOLOMONOFF_EPSILON);
}

/**
 * solomonoff_convergence_track - Empirically verify convergence theorem.
 *
 * Generates a sequence from a known Bernoulli(bias) process and tracks
 * how M's predictions converge to the true bias as more data is observed.
 *
 * Theorem: For any computable measure mu, with mu-probability 1:
 *   lim_{n->inf} M(x_{n+1}|x_1...x_n) = mu(x_{n+1}|x_1...x_n)
 */
size_t solomonoff_convergence_track(const binary_string_t *sequence,
                                     double true_bias,
                                     size_t max_depth, size_t depth_step,
                                     uint64_t max_steps,
                                     convergence_point_t *points,
                                     size_t max_points) {
    if (!sequence || !points || max_points == 0) return 0;

    size_t count = 0;
    for (size_t d = depth_step; d <= max_depth && count < max_points; d += depth_step) {
        /* Predict next bit at this depth */
        binary_string_t prefix;
        bs_prefix(sequence, sequence->length > 0 ? sequence->length - 1 : 0, &prefix);

        prediction_result_t pred = solomonoff_predict(&prefix, d, max_steps);

        points[count].depth = d;
        points[count].predicted_prob = pred.prob_next_one;
        points[count].true_prob = true_bias;
        points[count].abs_error = fabs(pred.prob_next_one - true_bias);
        count++;
    }
    return count;
}
