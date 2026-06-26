/*
 * example_universal_dist.c — Universal distribution & Solomonoff induction
 *
 * L2: m(x) = Σ 2^{-|p|} over programs producing x
 * L4: Coding Theorem: K(x) = -log m(x) + O(1)
 * L7: Solomonoff's universal predictor for sequence prediction
 *
 * Demonstrates algorithmic probability estimation and
 * the universal prediction framework.
 */
#include "../include/prefix_machine.h"
#include "../include/prefix_codes.h"
#include "../include/universal_distribution.h"
#include <stdio.h>
#include <math.h>

static void show_distribution(const char* label, const char* str) {
    PMString* s = pmstr_from_cstr(str);
    UniversalDistribution* ud = ud_estimate(s, 16, 1000);
    double K_from_m = (ud->m_x > 0.0) ? -log2(ud->m_x) : 1e10;
    size_t K_pm = pm_prefix_complexity(s);

    printf("  %-16s |x|=%2zu  m(x)=%14.6e  -log m=%8.3f  K_est=%3zu\n",
           label, s->len, ud->m_x, K_from_m, K_pm);

    ud_free(ud);
    pmstr_free(s);
}

static void predict_next_bit(const char* history_str) {
    PMString* history = pmstr_from_cstr(history_str);
    double p0 = ud_solomonoff_prediction(history, 0);
    double p1 = ud_solomonoff_prediction(history, 1);
    printf("  History \"%s\": P(0)=%.6f  P(1)=%.6f  Predict: %d\n",
           history_str, p0, p1, (p0 > p1) ? 0 : 1);
    pmstr_free(history);
}

int main(void) {
    printf("=== Universal Distribution & Solomonoff Induction ===\n\n");

    /* ── m(x) for various strings ──────────────────────────── */
    printf("--- Universal A Priori Probability m(x) ---\n");
    printf("  m(x) = Σ_{p:U(p)=x} 2^{-|p|}\n\n");
    show_distribution("empty", "");
    show_distribution("0", "0");
    show_distribution("1", "1");
    show_distribution("ab", "ab");
    show_distribution("hello", "hello");
    show_distribution("aaaaa", "aaaaa");
    show_distribution("010101", "010101");
    show_distribution("random7", "x7k2p9m");

    printf("\n  Interpretation: Simple/repetitive strings have higher m(x)\n");
    printf("  because shorter programs can generate them.\n");

    /* ── Coding Theorem verification ───────────────────────── */
    printf("\n--- Coding Theorem: K(x) = -log m(x) + O(1) ---\n");
    double test_values[] = {0.5, 0.1, 0.01, 0.001, 0.0001};
    for (int i = 0; i < 5; i++) {
        double bound = pm_coding_theorem_bound(test_values[i]);
        printf("  m = %.4f  →  -log₂(m) = %.4f bits\n", test_values[i], bound);
    }

    /* ── Solomonoff Prediction ─────────────────────────────── */
    printf("\n--- Solomonoff Universal Prediction ---\n");
    printf("  M(x_{n+1} | x₁...xₙ) = m(x₁...xₙ x_{n+1}) / m(x₁...xₙ)\n\n");

    predict_next_bit("");
    predict_next_bit("0");
    predict_next_bit("00");
    predict_next_bit("000");
    predict_next_bit("010");
    predict_next_bit("0101");
    predict_next_bit("01010");

    /* ── Algorithmic Mutual Information ────────────────────── */
    printf("\n--- Algorithmic Mutual Information ---\n");
    printf("  I(x:y) = K(x) + K(y) - K(x,y) + O(log n)\n\n");

    const char* pairs[][2] = {
        {"hello", "hello"},
        {"hello", "world"},
        {"abc",   "def"},
        {"aaaa",  "aaaa"},
        {"aaaa",  "bbbb"},
    };
    for (int i = 0; i < 5; i++) {
        PMString* x = pmstr_from_cstr(pairs[i][0]);
        PMString* y = pmstr_from_cstr(pairs[i][1]);
        double i_xy = ud_algorithmic_mutual_info(x, y);
        printf("  I(\"%s\" : \"%s\") = %.4f bits\n",
               pairs[i][0], pairs[i][1], i_xy);
        pmstr_free(x); pmstr_free(y);
    }

    /* ── Dominance Property ────────────────────────────────── */
    printf("\n--- Dominance Property ---\n");
    PMString* s = pmstr_from_cstr("test");
    double mu_vals[] = {0.1, 0.01, 0.001, 0.0001};
    for (int i = 0; i < 4; i++) {
        double c = ud_dominance_constant(s, mu_vals[i]);
        printf("  c(μ=%.4f) = %.6f  [m(x) >= c·μ(x)]\n", mu_vals[i], c);
    }

    /* ── Convergence ───────────────────────────────────────── */
    printf("\n--- Convergence of Solomonoff Predictor ---\n");
    printf("  For any computable measure μ, Solomonoff's M converges.\n");
    int sample_sizes[] = {10, 100, 1000, 10000};
    for (int i = 0; i < 4; i++) {
        double conv = ud_convergence_bound(sample_sizes[i], 8.0);
        printf("  n=%5d: cumulative error bound ≈ %.3f\n",
               sample_sizes[i], conv);
    }

    pmstr_free(s);
    printf("\nExample complete.\n");
    return 0;
}
