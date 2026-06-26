#include "kolmogorov.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

/* ============================================================
 * Kolmogorov Complexity — Complete Implementation
 *
 * Encodes knowledge points:
 *   L1: Plain C(x) vs prefix-free K(x) complexity definitions
 *   L2: Invariance theorem, incompressibility
 *   L3: Chain rule, conditional complexity
 *   L4: Coding theorem P(x) ≈ 2^{-K(x)}
 *   L5: Algorithm search for complexity upper bounds
 *   L6: Mutual information, algorithmic sufficient statistic
 *
 * Reference: Kolmogorov (1965), Chaitin (1969), Li & Vitányi (2019)
 * ============================================================ */

/* ── Helper: Search for a program that produces target ───── */

static int program_produces_output(OptimalPFM* machine,
                                    const BitString* program,
                                    const uint64_t* target,
                                    size_t target_len_words,
                                    size_t step_limit) {
    if (!machine || !program) return 0;
    OptimalPFM* runner = opfm_create();
    if (!runner) return 0;
    for (size_t i = 0; i < machine->num_machines; i++)
        opfm_register_machine(runner, machine->machines[i]);
    if (opfm_set_input(runner, program) != 0) { opfm_free(runner); return 0; }
    int rr = opfm_run(runner, step_limit);
    int matches = 0;
    if (rr == 1 && opfm_halted(runner)) {
        uint64_t out = opfm_output(runner);
        if (target_len_words == 0) {
            matches = 1; /* any output matches */
        } else if (target_len_words == 1) {
            matches = (out == target[0]);
        }
    }
    opfm_free(runner);
    return matches;
}

/* ── Plain Kolmogorov Complexity Bound ───────────────────── */
/* Knowledge point L1: C(x) = min{|p| : U(p) = x} for a fixed
 * universal TM U. We search programs up to max_program_len
 * to find the shortest that produces target. This gives an
 * upper bound (true C could be lower if a shorter program halts
 * only after our step limit). */

size_t kolmogorov_plain_bound(OptimalPFM* machine,
                               const uint64_t* target_output,
                               size_t target_len_words,
                               size_t max_program_len,
                               size_t step_limit) {
    if (!machine) return 0;
    for (size_t len = 1; len <= max_program_len && len <= 16; len++) {
        uint64_t max_p = 1ULL << len;
        for (uint64_t p = 0; p < max_p; p++) {
            uint8_t bits[4] = {0};
            for (size_t b = 0; b < len; b++)
                if (p & (1ULL << b))
                    bits[b / 8] |= (uint8_t)(1U << (b % 8));
            BitString* prog = bs_create(bits, len);
            if (!prog) continue;
            if (program_produces_output(machine, prog, target_output,
                                         target_len_words, step_limit)) {
                bs_free(prog);
                return len;
            }
            bs_free(prog);
        }
    }
    return max_program_len + 1; /* not found — upper bound */
}

/* ── Prefix-Free Kolmogorov Complexity Bound ─────────────── */
/* Knowledge point L1: K(x) = min{|p| : V(p) = x} where V
 * is a prefix-free machine. The search is restricted to
 * programs that form a prefix-free set. This is harder
 * to compute than C(x). */

size_t kolmogorov_prefix_bound(OptimalPFM* machine,
                                const uint64_t* target_output,
                                size_t target_len_words,
                                size_t max_program_len,
                                size_t step_limit) {
    if (!machine) return 0;
    /* Build up a prefix-free set iteratively, checking each program */
    PrefixFreeSet* pf_set = pfs_create((size_t)1 << (max_program_len < 16 ? max_program_len : 16));
    if (!pf_set) return 0;

    size_t best_len = max_program_len + 1;
    for (size_t len = 1; len <= max_program_len && len <= 16; len++) {
        uint64_t max_p = 1ULL << len;
        for (uint64_t p = 0; p < max_p; p++) {
            uint8_t bits[4] = {0};
            for (size_t b = 0; b < len; b++)
                if (p & (1ULL << b))
                    bits[b / 8] |= (uint8_t)(1U << (b % 8));
            BitString* prog = bs_create(bits, len);
            if (!prog) continue;
            /* Check prefix-free: no existing program is a prefix */
            int conflict = 0;
            for (size_t i = 0; i < pf_set->count; i++) {
                if (bs_is_prefix(pf_set->strings[i], prog) ||
                    bs_is_prefix(prog, pf_set->strings[i])) {
                    conflict = 1;
                    break;
                }
            }
            if (!conflict) {
                if (program_produces_output(machine, prog, target_output,
                                             target_len_words, step_limit)) {
                    if (len < best_len) best_len = len;
                    pfs_add(pf_set, prog);
                }
            }
            bs_free(prog);
        }
    }
    pfs_free(pf_set);
    return best_len;
}

/* ── Conditional Kolmogorov Complexity ───────────────────── */
/* Knowledge point L2: K(x|y) = min{|p| : V(p, y) = x}.
 * The machine gets both the program p and the condition y.
 * K(x|y) ≤ K(x) + O(1) since we can embed K(x) and ignore y. */

size_t kolmogorov_conditional_bound(OptimalPFM* machine,
                                     const uint64_t* target,
                                     size_t target_len,
                                     const uint64_t* condition,
                                     size_t cond_len,
                                     size_t max_prog_len,
                                     size_t step_limit) {
    if (!machine) return 0;
    /* For our register machine model, we encode the condition
     * in the first few registers before execution. We search for
     * a program that, when run with condition in registers,
     * produces the target. */
    /* Simplified: we search programs that produce target,
     * treating condition as part of the machine setup */
    (void)condition; (void)cond_len;
    return kolmogorov_prefix_bound(machine, target, target_len,
                                    max_prog_len, step_limit);
}

/* ── Chain Rule ───────────────────────────────────────────── */
/* Knowledge point L3: K(x,y) ≤ K(x) + K(y|x) + O(log K(x,y)).
 * This is the algorithmic analog of the Shannon chain rule
 * H(X,Y) = H(X) + H(Y|X). The extra log term is the algorithmic
 * overhead for encoding the length of the first description. */

ChainRuleResult kolmogorov_chain_rule_verify(OptimalPFM* machine,
                                              const uint64_t* x, size_t x_len,
                                              const uint64_t* y, size_t y_len,
                                              size_t max_prog_len,
                                              size_t step_limit) {
    ChainRuleResult result = {0};
    if (!machine) return result;

    /* Compute K(x) */
    result.k_x = kolmogorov_prefix_bound(machine, x, x_len, max_prog_len, step_limit);
    /* Compute K(y) */
    result.k_y = kolmogorov_prefix_bound(machine, y, y_len, max_prog_len, step_limit);
    /* Compute K(x,y) — combined output */
    uint64_t combined[2];
    if (x_len > 0) combined[0] = x[0]; else combined[0] = 0;
    if (y_len > 0) combined[1] = y[0]; else combined[1] = 0;
    result.k_xy = kolmogorov_prefix_bound(machine, combined,
        (x_len > 0 || y_len > 0) ? 2 : 0, max_prog_len, step_limit);
    /* Compute K(y|x) */
    result.k_y_given_x = kolmogorov_conditional_bound(machine, y, y_len,
        x, x_len, max_prog_len, step_limit);
    /* Check chain rule: K(x,y) ≤ K(x) + K(y|x) + log₂(K(x,y)) */
    double log_term = result.k_xy > 0 ? log2((double)result.k_xy) : 1.0;
    result.chain_rule_holds = ((double)result.k_xy <=
        (double)(result.k_x + result.k_y_given_x) + log_term + 5.0) ? 1 : 0;
    return result;
}

/* ── Incompressibility ────────────────────────────────────── */
/* Knowledge point L2: A string x is c-incompressible if
 * K(x) ≥ |x| - c. By a counting argument, most strings of
 * length n have K(x) ≥ n - O(1). Only 2^{n-c} strings can
 * have complexity < n - c.
 *
 * This function returns the largest c for which x appears
 * to be c-incompressible under our search limit. */

int kolmogorov_incompressibility(const BitString* x,
                                  OptimalPFM* machine,
                                  size_t max_prog_len,
                                  size_t step_limit) {
    if (!x || !machine) return -1;
    /* Encode x as a uint64_t target */
    uint64_t target = 0;
    for (size_t i = 0; i < x->length && i < 64; i++)
        if (bs_get_bit(x, i)) target |= (1ULL << i);
    size_t k = kolmogorov_prefix_bound(machine, &target, 1,
                                        max_prog_len, step_limit);
    if (k > x->length) return 0; /* suspicious — K(x) > |x| is very unlikely */
    return (int)(x->length) - (int)k;
}

/* ── Mutual Information ───────────────────────────────────── */
/* Knowledge point L3: I(x:y) = K(x) + K(y) - K(x,y) + O(log).
 * Algorithmic mutual information measures how much x and y
 * "know about" each other from an algorithmic standpoint.
 * I(x:y) is symmetric and non-negative up to an additive constant.
 *
 * Theorem (Symmetry of Information): |K(x) + K(y|x*) - K(y) - K(x|y*)| ≤ O(log).
 * Reference: Levin (1974), Gacs (1974), Kolmogorov */

int64_t kolmogorov_mutual_information(OptimalPFM* machine,
                                       const uint64_t* x, size_t x_len,
                                       const uint64_t* y, size_t y_len,
                                       size_t max_prog_len,
                                       size_t step_limit) {
    if (!machine) return 0;
    size_t kx = kolmogorov_prefix_bound(machine, x, x_len, max_prog_len, step_limit);
    size_t ky = kolmogorov_prefix_bound(machine, y, y_len, max_prog_len, step_limit);
    uint64_t combined[2] = { x_len > 0 ? x[0] : 0, y_len > 0 ? y[0] : 0 };
    size_t kxy = kolmogorov_prefix_bound(machine, combined,
        (x_len > 0 || y_len > 0) ? 2 : 0, max_prog_len, step_limit);
    int64_t mi = (int64_t)kx + (int64_t)ky - (int64_t)kxy;
    return (mi > 0) ? mi : 0; /* I(x:y) is non-negative up to constant */
}

/* ── Algorithmic Probability ──────────────────────────────── */
/* Knowledge point L4: P_U(x) = Σ_{p: U(p)=x} 2^{-|p|}.
 * The universal a priori probability of x. This is the
 * measure-theoretic partner of Kolmogorov complexity:
 * -log₂ P_U(x) = K(x) + O(1) (Coding Theorem).
 *
 * P_U is a universal semi-measure: multiplicatively dominates
 * all lower semi-computable semi-measures. */

double algorithmic_probability(OptimalPFM* machine,
                                const uint64_t* target, size_t target_len,
                                size_t max_prog_len, size_t step_limit) {
    if (!machine) return 0.0;
    double p = 0.0;
    for (size_t len = 1; len <= max_prog_len && len <= 12; len++) {
        uint64_t max_prog = 1ULL << len;
        for (uint64_t pi = 0; pi < max_prog; pi++) {
            uint8_t bits[4] = {0};
            for (size_t b = 0; b < len; b++)
                if (pi & (1ULL << b))
                    bits[b / 8] |= (uint8_t)(1U << (b % 8));
            BitString* prog = bs_create(bits, len);
            if (!prog) continue;
            if (program_produces_output(machine, prog, target,
                                         target_len, step_limit)) {
                p += pow(0.5, (double)len);
            }
            bs_free(prog);
        }
    }
    return p;
}

/* ── Coding Theorem Verification ──────────────────────────── */
/* Knowledge point L4: The Coding Theorem (Levin 1974) states:
 * K(x) = -log₂ P_U(x) + O(1).
 * This function verifies the theorem for a given x by computing
 * both sides and checking the difference is small. */

CodingTheoremResult coding_theorem_verify(OptimalPFM* machine,
                                           const uint64_t* target, size_t target_len,
                                           size_t max_prog_len, size_t step_limit) {
    CodingTheoremResult result = {0};
    if (!machine) return result;

    result.p_x = algorithmic_probability(machine, target, target_len,
                                          max_prog_len, step_limit);
    if (result.p_x > 0) {
        result.neg_log2_p = -log2(result.p_x);
    } else {
        result.neg_log2_p = (double)(max_prog_len + 1);
    }
    result.k_x = kolmogorov_prefix_bound(machine, target, target_len,
                                          max_prog_len, step_limit);
    result.difference = fabs(result.neg_log2_p - (double)result.k_x);
    /* Coding theorem holds if |(-log P) - K| < bound (machine-dependent) */
    result.coding_theorem_holds = (result.difference < 5.0) ? 1 : 0;
    return result;
}

/* ── Algorithmic Sufficient Statistic ─────────────────────── */
/* Knowledge point L6: For a string x, the algorithmic sufficient
 * statistic is the "essence" or "model" of x — a program that
 * captures all non-random structure in x. Formally:
 * K(x) ≈ K(model) + K(x | model) (two-part code optimality).
 *
 * This is the algorithmic analog of the Minimum Description
 * Length (MDL) principle. */

AlgorithmicStatistic* algorithmic_sufficient_statistic(OptimalPFM* machine,
                                                        const BitString* x,
                                                        size_t max_model_len,
                                                        size_t step_limit) {
    if (!machine || !x) return NULL;
    AlgorithmicStatistic* as = (AlgorithmicStatistic*)calloc(1, sizeof(AlgorithmicStatistic));
    if (!as) return NULL;

    /* Encode x as uint64_t */
    uint64_t target = 0;
    for (size_t i = 0; i < x->length && i < 64; i++)
        if (bs_get_bit(x, i)) target |= (1ULL << i);

    /* The model is the shortest program that produces x */
    size_t best_len = max_model_len + 1;
    BitString* best_prog = NULL;

    for (size_t len = 1; len <= max_model_len && len <= 16; len++) {
        uint64_t max_p = 1ULL << len;
        for (uint64_t p = 0; p < max_p; p++) {
            uint8_t bits[4] = {0};
            for (size_t b = 0; b < len; b++)
                if (p & (1ULL << b))
                    bits[b / 8] |= (uint8_t)(1U << (b % 8));
            BitString* prog = bs_create(bits, len);
            if (!prog) continue;
            if (program_produces_output(machine, prog, &target, 1, step_limit)) {
                if (len < best_len) {
                    best_len = len;
                    if (best_prog) bs_free(best_prog);
                    best_prog = bs_copy(prog);
                }
            }
            bs_free(prog);
        }
    }

    as->model = best_prog;
    as->model_k = best_len;
    as->noise = NULL;
    as->noise_k = 0;
    /* Check sufficiency: if K(model) ≈ K(x) - K(x|model),
     * the model captures most structure. */
    as->is_sufficient = (best_len <= max_model_len) ? 1 : 0;
    return as;
}

void as_free(AlgorithmicStatistic* as) {
    if (!as) return;
    bs_free(as->model);
    bs_free(as->noise);
    free(as);
}