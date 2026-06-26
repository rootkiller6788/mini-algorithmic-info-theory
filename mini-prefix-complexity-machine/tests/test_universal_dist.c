/*
 * test_universal_dist.c — Tests for universal distribution m(x)
 *
 * L2: Universal a priori probability
 * L4: Coding Theorem
 * L5: Estimation algorithms
 * L7: Solomonoff prediction
 */
#include "../include/prefix_machine.h"
#include "../include/prefix_codes.h"
#include "../include/universal_distribution.h"
#include <stdio.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; } \
    else { printf("FAIL: %s (line %d)\n", msg, __LINE__); } \
} while(0)

int main(void) {
    printf("=== test_universal_dist ===\n");

    /* ── Create/Free ───────────────────────────────────── */
    UniversalDistribution* ud = ud_create();
    TEST_ASSERT(ud != NULL, "ud_create");
    TEST_ASSERT(ud->m_x == 0.0, "initial m_x = 0");
    TEST_ASSERT(ud->num_programs == 0, "initial num_programs = 0");
    ud_free(ud);

    /* ── Estimate m(x) ─────────────────────────────────── */
    PMString* s = pmstr_from_cstr("hello");
    UniversalDistribution* ud_est = ud_estimate(s, 16, 1000);
    TEST_ASSERT(ud_est != NULL, "ud_estimate");
    TEST_ASSERT(ud_est->m_x > 0.0, "m(x) > 0 (non-vanishing)");
    TEST_ASSERT(ud_est->m_lower <= ud_est->m_x, "lower <= estimate");
    TEST_ASSERT(ud_est->m_x <= ud_est->m_upper, "estimate <= upper");
    TEST_ASSERT(ud_est->shortest_prog > 0, "shortest program > 0");
    TEST_ASSERT(ud_est->num_programs > 0, "num programs > 0");
    ud_free(ud_est);

    /* ── Dominance Constant ────────────────────────────── */
    double mu = 0.001;
    double c = ud_dominance_constant(s, mu);
    TEST_ASSERT(c >= 0.0 && c <= 1.0, "dominance constant in [0,1]");

    /* ── Continuous Probability ────────────────────────── */
    double cont_p = ud_continuous_probability(s);
    TEST_ASSERT(cont_p > 0.0, "continuous prob > 0");
    TEST_ASSERT(cont_p <= 1.0, "continuous prob <= 1");

    /* ── Algorithmic Mutual Information ────────────────── */
    PMString* y = pmstr_from_cstr("world");
    double i_xy = ud_algorithmic_mutual_info(s, y);
    TEST_ASSERT(i_xy >= 0.0, "I(x:y) >= 0 (non-negative)");

    /* ── Self Mutual Information ───────────────────────── */
    double i_self = ud_mi_self_info(s);
    TEST_ASSERT(i_self >= 0.0, "I(x:x) >= 0");

    /* ── MI Non-negativity ─────────────────────────────── */
    int nonneg = ud_mi_nonnegativity_check(s, y);
    TEST_ASSERT(nonneg == 1, "MI non-negativity holds");

    /* ── MI Symmetry ───────────────────────────────────── */
    MISymmetry mi_sym = ud_mi_symmetry_check(s, y);
    TEST_ASSERT(mi_sym.I_xy >= 0.0, "I_xy >= 0");
    TEST_ASSERT(mi_sym.I_yx >= 0.0, "I_yx >= 0");
    TEST_ASSERT(mi_sym.sym_diff < 500.0, "symmetry diff bounded");

    /* ── Solomonoff Prediction ─────────────────────────── */
    PMString* hist = pmstr_from_cstr("010");
    double pred_0 = ud_solomonoff_prediction(hist, 0);
    double pred_1 = ud_solomonoff_prediction(hist, 1);
    TEST_ASSERT(pred_0 >= 0.0 && pred_0 <= 1.0, "pred_0 in [0,1]");
    TEST_ASSERT(pred_1 >= 0.0 && pred_1 <= 1.0, "pred_1 in [0,1]");
    pmstr_free(hist);

    /* ── Algorithmic Entropy ───────────────────────────── */
    double alg_ent = ud_algorithmic_entropy(s);
    TEST_ASSERT(alg_ent > 0.0, "algorithmic entropy > 0");
    TEST_ASSERT(alg_ent >= (double)s->len,
                "algorithmic entropy >= |x| barring compressibility");

    /* ── Semimeasure Ratio ─────────────────────────────── */
    double ratio = ud_semimeasure_ratio(s, 0.01);
    TEST_ASSERT(ratio > 0.0, "semimeasure ratio > 0");

    /* ── Normalizability ───────────────────────────────── */
    const PMString* samples[3];
    samples[0] = pmstr_from_cstr("0");
    samples[1] = pmstr_from_cstr("1");
    samples[2] = pmstr_from_cstr("01");
    int norm = ud_is_effectively_normalizable(samples, 3);
    TEST_ASSERT(norm == 1, "finite sample normalizable");

    /* ── Conditional Distribution ──────────────────────── */
    double m_cond = ud_conditional_probability(y, s);
    TEST_ASSERT(m_cond >= 0.0 && m_cond <= 1.0, "m(x|y) in [0,1]");

    /* ── Normalize m ───────────────────────────────────── */
    double m_test = 0.001;
    double omega_est = 0.5;
    double p_norm = ud_normalize_m(m_test, omega_est);
    TEST_ASSERT(p_norm > 0.0, "normalized prob > 0");
    TEST_ASSERT(p_norm <= 1.0, "normalized prob <= 1");

    /* ── Total Mass Estimate ───────────────────────────── */
    double total_mass = ud_total_mass_estimate(2, 8);
    TEST_ASSERT(total_mass > 0.0, "total mass > 0");
    TEST_ASSERT(total_mass < 100.0, "total mass estimate finite");

    /* ── Chain Rule (UD) ───────────────────────────────── */
    ChainRuleUD cr = ud_chain_rule_check(s, y);
    TEST_ASSERT(cr.m_xy > 0.0, "chain rule m_xy > 0");
    TEST_ASSERT(cr.m_x_times_m_y_given_x > 0.0, "chain rule product > 0");

    /* ── Data Processing Inequality ────────────────────── */
    double dpi = ud_data_processing_bound(10.0, 5.0);
    TEST_ASSERT(dpi == 15.0, "DPI bound = I + K(f)");

    /* ── Convergence Bound ─────────────────────────────── */
    double conv = ud_convergence_bound(100, 8.0);
    TEST_ASSERT(conv > 0.0, "convergence bound > 0");

    /* ── Epicurus Weighted Sum ─────────────────────────── */
    double plens[3] = {3.0, 4.0, 5.0};
    double epic = ud_epicurus_weighted_sum(plens, 3);
    double e_expected = pow(2.0, -3.0) + pow(2.0, -4.0) + pow(2.0, -5.0);
    TEST_ASSERT(fabs(epic - e_expected) < 1e-9, "Epicurus sum exact");

    /* ── Prefix Code Total Weight ──────────────────────── */
    double pcw = ud_prefix_code_total_weight(samples, 3);
    TEST_ASSERT(pcw > 0.0, "prefix code total weight > 0");

    for (int i = 0; i < 3; i++) pmstr_free((PMString*)samples[i]);
    pmstr_free(s); pmstr_free(y);

    printf("\n=== Results: %d / %d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
