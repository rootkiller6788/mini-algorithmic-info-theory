/**
 * kolmogorov.c - Kolmogorov Complexity Estimation
 *
 * K(x) = min{ |p| : U(p) = x }
 *
 * Implements:
 *   1. Plain Kolmogorov complexity K(x)
 *   2. Prefix-free Kolmogorov complexity
 *   3. Conditional complexity K(x|y)
 *   4. Joint complexity K(x,y)
 *   5. Incompressibility testing
 *   6. Randomness deficiency
 *   7. Algorithmic mutual information
 *   8. Normalized information distance
 *   9. Coding theorem verification
 *
 * Reference: Li & Vitanyi, 4th ed, 2019.
 *            Kolmogorov, "Three approaches to information", 1965.
 *            Chaitin, "A theory of program size", 1975.
 *
 * Curriculum: MIT 6.840, Berkeley CS172, Cambridge Part III.
 */

#include "kolmogorov.h"
#include "enumeration.h"
#include "universal_machine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ================================================================
 * Plain Kolmogorov Complexity K(x) (L1-L2)
 * ================================================================ */

/**
 * kolmogorov_K - Estimate K(x) by enumerating programs.
 *
 * Enumerates all programs up to depth bits in shortlex order,
 * executes each, and returns the length of the shortest program
 * whose output equals x.
 *
 * K(x) = min{ |p| : U(p) = x }
 *
 * If no program within depth produces x, returns (depth + 1)
 * as a lower bound.
 *
 * By the Invariance Theorem, the choice of U affects K(x) by
 * at most an additive constant c_U that depends only on U.
 */
size_t kolmogorov_K(const binary_string_t *x, size_t depth, uint64_t max_steps,
                    binary_string_t *shortest_program) {
    if (!x) return SIZE_MAX;
    if (x->length == 0) return 0;  /* K(empty) = 0 */

    if (shortest_program) bs_init(shortest_program);

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

    size_t K = dovetail_compute_K(&dm, x, shortest_program);
    dovetail_free(&dm);

    return (K == SIZE_MAX) ? (depth + 1) : K;
}

/**
 * kolmogorov_K_upper - Explicit upper bound: K(x) <= |print_program|.
 *
 * Constructs a "print" program that directly outputs x.
 * This gives the trivial bound: K(x) <= |x| * c_print + O(1),
 * where c_print is the cost of outputting one bit.
 *
 * For our machine, c_print = 12 bits per data bit (LDK + OUT).
 * This is not tight; a smarter encoding would achieve c_print = 1.
 */
size_t kolmogorov_K_upper(binary_string_t *constructed_program,
                          const binary_string_t *x) {
    if (!constructed_program || !x) return 0;
    return um_build_print_program(constructed_program, x);
}

/**
 * kolmogorov_K_pair - Joint complexity K(x,y).
 *
 * Uses a self-delimiting encoding to combine x and y into a single
 * string. The encoding: EliasGamma(|x|) + x + y.
 *
 * This implies: K(x,y) <= K(x) + K(y) + O(log K(x))
 * because we can concatenate the programs for x and y, with a
 * small O(log K(x)) overhead to specify where x's program ends.
 */
size_t kolmogorov_K_pair(const binary_string_t *x,
                         const binary_string_t *y,
                         size_t depth, uint64_t max_steps) {
    if (!x || !y) return SIZE_MAX;

    /* Build self-delimiting encoding of (x,y) */
    binary_string_t encoded;
    bs_init(&encoded);

    /* Encode |x| using Elias Gamma */
    size_t nx = x->length;
    if (nx == 0) {
        bs_append_bit(&encoded, 0);
    } else {
        int nbits = 0;
        size_t tmp = nx;
        while (tmp > 0) { tmp >>= 1; nbits++; }
        for (int i = 0; i < nbits - 1; i++) bs_append_bit(&encoded, 0);
        for (int i = nbits - 1; i >= 0; i--) bs_append_bit(&encoded, (nx >> i) & 1);
    }
    bs_concat(&encoded, x);
    bs_concat(&encoded, y);

    return kolmogorov_K(&encoded, depth, max_steps, NULL);
}

/**
 * kolmogorov_conditional_K - Conditional complexity K(x|y).
 *
 * K(x|y) = min{ |p| : U(p, y) = x }
 *
 * Since our machine doesn't natively support auxiliary input,
 * we simulate it using self-delimiting encoding:
 * K(x|y) ~ min{ |p| : U(p . '|' . y) = x }
 *
 * The key property: K(x|y) <= K(x) always. Knowing y can only
 * reduce the complexity of describing x.
 */
size_t kolmogorov_conditional_K(const binary_string_t *x,
                                const binary_string_t *y,
                                size_t depth, uint64_t max_steps) {
    if (!x || !y) return SIZE_MAX;
    if (x->length == 0) return 0;

    /* For the conditional case, use K(xy) - K(y) approximation */
    binary_string_t xy;
    bs_init(&xy);
    bs_concat(&xy, y);
    bs_concat(&xy, x);

    size_t K_xy = kolmogorov_K(&xy, depth, max_steps, NULL);
    size_t K_y = kolmogorov_K(y, depth, max_steps, NULL);

    /* K(x|y) ~ K(xy) - K(y) in the algorithmic sense */
    if (K_xy == SIZE_MAX || K_y == SIZE_MAX) return SIZE_MAX;
    return (K_xy > K_y) ? (K_xy - K_y) : 0;
}

/* ================================================================
 * Incompressibility and Randomness (L2-L4)
 * ================================================================ */

bool kolmogorov_is_incompressible(const binary_string_t *x, size_t c,
                                  size_t depth, uint64_t max_steps) {
    if (!x) return false;
    size_t K = kolmogorov_K(x, depth, max_steps, NULL);
    if (K == SIZE_MAX) {
        /* No program found -> K(x) >= depth+1, which is high */
        return (depth >= x->length + c);
    }
    /* K(x) >= |x| - c means incompressible */
    return (K + c >= x->length);
}

int kolmogorov_randomness_deficiency(const binary_string_t *x,
                                     size_t depth, uint64_t max_steps) {
    if (!x) return -1;
    size_t K = kolmogorov_K(x, depth, max_steps, NULL);
    if (K == SIZE_MAX) return -1;
    /* Deficiency = |x| - K(x). Positive = compressible, ~0 = random */
    return (int)(x->length) - (int)K;
}

/**
 * kolmogorov_test_randomness - Martin-Lof style test via K(x).
 *
 * The universal Martin-Lof test:
 *   delta(x) = |x| - K(x||x|)
 *
 * x fails the test at level m if delta(x) > m.
 * Returns: the maximum m for which x fails, or 0 if x passes all levels.
 */
size_t kolmogorov_test_randomness(const binary_string_t *x,
                                  size_t depth, uint64_t max_steps) {
    if (!x) return 0;
    int deficiency = kolmogorov_randomness_deficiency(x, depth, max_steps);
    if (deficiency <= 0) return 0;
    return (size_t)deficiency;
}

/* ================================================================
 * Prefix Complexity (L3)
 * ================================================================ */

/**
 * prefix_complexity_K - Prefix-free Kolmogorov complexity.
 *
 * Uses self-delimiting programs: each program is prefixed with
 * an encoding of its own length, making the set prefix-free.
 *
 * For prefix complexity:
 *   K_prefix(x) = min{ |p| : U(p) = x and no p' in P is prefix of p }
 *
 * The Kraft inequality holds: sum_x 2^{-K_prefix(x)} <= 1.
 */
size_t prefix_complexity_K(const binary_string_t *x, size_t depth,
                           uint64_t max_steps,
                           binary_string_t *shortest_prefix_program) {
    if (!x) return SIZE_MAX;
    if (x->length == 0) return 0;

    if (shortest_prefix_program) bs_init(shortest_prefix_program);

    size_t best = SIZE_MAX;
    dovetail_manager_t dm;
    size_t pool_cap = (depth <= 10) ? (1U << depth) : SOLOMONOFF_POOL_SIZE;
    dovetail_init(&dm, pool_cap, max_steps);

    /* Enumerate prefix-free programs:
       Each program is (length encoding) + (raw bits)
       Length encoding uses Elias Gamma */
    program_enumerator_t enumerator;
    enumerator_init(&enumerator, depth);

    binary_string_t raw_prog;
    size_t raw_len;
    while (enumerator_next(&enumerator, &raw_prog, &raw_len) && dm.pool_size < pool_cap) {
        /* Make prefix-free by prepending length */
        binary_string_t pf_prog;
        bs_init(&pf_prog);
        um_make_prefix_free(&pf_prog, &raw_prog);
        dovetail_add_program(&dm, &pf_prog, pf_prog.length);
    }

    dovetail_run(&dm, depth, max_steps);
    best = dovetail_compute_K(&dm, x, shortest_prefix_program);
    dovetail_free(&dm);

    return (best == SIZE_MAX) ? (depth + 1) : best;
}

double prefix_complexity_verify_kraft(size_t n, size_t depth, uint64_t max_steps) {
    double sum = 0.0;
    size_t checked = 0;
    for (size_t len = 0; len <= n; len++) {
        uint64_t max_idx = (len == 0) ? 0 : (1ULL << len) - 1;
        for (uint64_t idx = 0; idx <= max_idx && checked < 128; idx++) {
            binary_string_t x;
            bs_init(&x);
            for (size_t b = 0; b < len; b++) {
                bs_append_bit(&x, (idx >> (len - 1 - b)) & 1);
            }
            size_t K = prefix_complexity_K(&x, depth, max_steps, NULL);
            if (K != SIZE_MAX) sum += pow(2.0, -(double)(int64_t)K);
            checked++;
        }
    }
    return sum;
}

/**
 * chaitin_omega_lower - Lower bound on Chaitin's Omega.
 *
 * Omega = sum_{p halts} 2^{-|p|}
 *
 * Omega is algorithmically random: its first n bits encode the
 * solution to the halting problem for all programs of length <= n.
 *
 * This function runs programs up to `depth` and returns the sum of
 * weights of programs that are observed to halt within `max_steps`.
 *
 * As max_steps -> infinity, this converges to the true Omega.
 * Every additional halting program increases the lower bound.
 *
 * Omega is the "probability that a randomly chosen program halts."
 *
 * Reference: Chaitin, "Algorithmic Information Theory", 1987.
 *            Calude, "Information and Randomness", 2002.
 */
double chaitin_omega_lower(size_t depth, uint64_t max_steps) {
    dovetail_manager_t dm;
    size_t pool_cap = (depth <= 10) ? (1U << depth) : 1000;
    dovetail_init(&dm, pool_cap, max_steps);

    program_enumerator_t enumerator;
    enumerator_init(&enumerator, depth);

    binary_string_t prog;
    size_t plen;
    while (enumerator_next(&enumerator, &prog, &plen) && dm.pool_size < pool_cap) {
        dovetail_add_program(&dm, &prog, plen);
    }

    dovetail_run(&dm, depth, max_steps);
    double omega = dovetail_total_probability(&dm);
    dovetail_free(&dm);

    return omega;
}

/* ================================================================
 * Algorithmic Mutual Information (L5)
 * ================================================================ */

double algorithmic_mutual_info(const binary_string_t *x,
                               const binary_string_t *y,
                               size_t depth, uint64_t max_steps) {
    if (!x || !y) return 0.0;

    size_t Kx = kolmogorov_K(x, depth, max_steps, NULL);
    size_t Ky = kolmogorov_K(y, depth, max_steps, NULL);
    size_t Kxy = kolmogorov_K_pair(x, y, depth, max_steps);

    if (Kx == SIZE_MAX || Ky == SIZE_MAX || Kxy == SIZE_MAX)
        return 0.0;

    /* I(x:y) = K(x) + K(y) - K(x,y) */
    return (double)((int64_t)Kx + (int64_t)Ky - (int64_t)Kxy);
}

double algorithmic_distance(const binary_string_t *x,
                            const binary_string_t *y,
                            size_t depth, uint64_t max_steps) {
    if (!x || !y) return 1.0;

    size_t Kx = kolmogorov_K(x, depth, max_steps, NULL);
    size_t Ky = kolmogorov_K(y, depth, max_steps, NULL);
    size_t Kx_given_y = kolmogorov_conditional_K(x, y, depth, max_steps);
    size_t Ky_given_x = kolmogorov_conditional_K(y, x, depth, max_steps);

    if (Kx == SIZE_MAX || Ky == SIZE_MAX) return 1.0;

    size_t max_cond = (Kx_given_y > Ky_given_x) ? Kx_given_y : Ky_given_x;
    size_t max_K = (Kx > Ky) ? Kx : Ky;

    if (max_K == 0) return 0.0;
    return (double)max_cond / (double)max_K;
}

double normalized_compression_distance(const binary_string_t *x,
                                       const binary_string_t *y,
                                       size_t depth, uint64_t max_steps) {
    if (!x || !y) return 1.0;

    binary_string_t xy;
    bs_init(&xy);
    bs_concat(&xy, x);
    bs_concat(&xy, y);

    size_t Cx = kolmogorov_K(x, depth, max_steps, NULL);
    size_t Cy = kolmogorov_K(y, depth, max_steps, NULL);
    size_t Cxy = kolmogorov_K(&xy, depth, max_steps, NULL);

    if (Cx == SIZE_MAX || Cy == SIZE_MAX || Cxy == SIZE_MAX)
        return 1.0;

    /* NCD = (C(xy) - min(C(x), C(y))) / max(C(x), C(y)) */
    size_t min_C = (Cx < Cy) ? Cx : Cy;
    size_t max_C = (Cx > Cy) ? Cx : Cy;

    if (max_C == 0) return 0.0;
    return (double)((int64_t)Cxy - (int64_t)min_C) / (double)max_C;
}

/* ================================================================
 * Coding Theorem Verification (L4 - Theorems)
 * ================================================================ */

/**
 * coding_theorem_verify - Verify K(x) = -log M(x) + O(1).
 *
 * For each string x of length n, compute both K(x) and -log2(M(x)),
 * and measure the difference. The Coding Theorem asserts that the
 * maximum difference is bounded by a constant (depending on U).
 */
void coding_theorem_verify(size_t n, size_t depth, uint64_t max_steps,
                           double *max_diff, double *avg_diff, size_t *num_checked) {
    if (!max_diff || !avg_diff || !num_checked) return;

    *max_diff = 0.0;
    *avg_diff = 0.0;
    *num_checked = 0;
    double total_diff = 0.0;

    for (size_t len = 1; len <= n; len++) {
        if (len > 6) break;  /* Keep verification tractable */
        uint64_t max_idx = (1ULL << len) - 1;
        for (uint64_t idx = 0; idx <= max_idx; idx++) {
            binary_string_t x;
            bs_init(&x);
            for (size_t b = 0; b < len; b++) {
                bs_append_bit(&x, (idx >> (len - 1 - b)) & 1);
            }

            size_t K = kolmogorov_K(&x, depth, max_steps, NULL);
            algprob_t Mx = solomonoff_M(&x, depth, max_steps);

            if (K != SIZE_MAX && Mx > SOLOMONOFF_EPSILON) {
                double logM = -log2(Mx);
                double diff = fabs((double)K - logM);
                total_diff += diff;
                (*num_checked)++;
                if (diff > *max_diff) *max_diff = diff;
            }
        }
    }

    if (*num_checked > 0) *avg_diff = total_diff / (double)(*num_checked);
}

double levin_coding_theorem_constant(size_t n, size_t depth, uint64_t max_steps) {
    double max_c = 0.0;
    for (size_t len = 1; len <= n && len <= 6; len++) {
        uint64_t max_idx = (1ULL << len) - 1;
        for (uint64_t idx = 0; idx <= max_idx; idx++) {
            binary_string_t x;
            bs_init(&x);
            for (size_t b = 0; b < len; b++)
                bs_append_bit(&x, (idx >> (len - 1 - b)) & 1);

            size_t K = kolmogorov_K(&x, depth, max_steps, NULL);
            algprob_t Mx = solomonoff_M(&x, depth, max_steps);
            if (K != SIZE_MAX && Mx > SOLOMONOFF_EPSILON) {
                /* M(x) <= c_U * 2^{-K(x)} */
                /* c_U >= M(x) * 2^{K(x)} */
                double c = Mx * pow(2.0, (double)K);
                if (c > max_c) max_c = c;
            }
        }
    }
    return max_c;
}
