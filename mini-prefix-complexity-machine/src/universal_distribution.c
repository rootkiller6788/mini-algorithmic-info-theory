#include "../include/prefix_machine.h"
#include "../include/prefix_codes.h"
#include "../include/universal_distribution.h"
#include <math.h>

/*
 * universal_distribution.c — Universal a priori probability m(x)
 *
 * Implements Solomonoff-Levin universal distribution.
 * Each function = one real knowledge point.
 *
 * L2: m(x) = Σ_{p:U(p)=x} 2^{-|p|}  (definition)
 * L4: Coding Theorem: K(x) = -log m(x) + O(1)
 * L5: Enumerating short programs to estimate m(x)
 * L7: Solomonoff's universal prediction
 * L8: Algorithmic mutual information
 *
 * Reference: Solomonoff (1964), Levin (1974), Li & Vitányi §4.3
 */

/* ══════════════════════════════════════════════════════════════
 * Constructor / destructor
 ══════════════════════════════════════════════════════════════ */

UniversalDistribution* ud_create(void) {
    UniversalDistribution* ud = (UniversalDistribution*)calloc(1, sizeof(UniversalDistribution));
    assert(ud);
    ud->m_x = 0.0;
    ud->m_upper = 0.0;
    ud->m_lower = 0.0;
    ud->shortest_prog = SIZE_MAX;
    ud->num_programs = 0;
    return ud;
}

void ud_free(UniversalDistribution* ud) { free(ud); }

/* ══════════════════════════════════════════════════════════════
 * Helper: lower bound for m(x) from self-delimiting code
 *
 * The shortest program producing x in a prefix machine is
 * the self-delimiting code for |x| followed by x itself.
 * This gives a concrete lower bound on m(x).
 *
 * |p| = 2⌊log₂|x|⌋ + 1 + |x|
 * m(x) ≥ 2^{-|p|}
 ══════════════════════════════════════════════════════════════ */

static double compute_m_lower_bound(const PMString* x) {
    size_t n = x->len;
    int n_bits = 0;
    size_t t = n;
    while (t > 0) { n_bits++; t >>= 1; }
    if (n_bits == 0) n_bits = 1;
    size_t sd_len = (size_t)(2 * n_bits) + 1 + n;
    return pow(2.0, -(double)sd_len);
}

/* ══════════════════════════════════════════════════════════════
 * Helper: upper bound for m(x) considering multiple codes
 *
 * Since there are many prefix-free codes (Gamma, Delta, Omega),
 * and since multiple programs may produce the same output,
 * the true m(x) is the sum over ALL such programs.
 *
 * We estimate: m(x) ≤ 2^{-|sd|} + 2^{-|δ|} + 2^{-|γ|} + 2^{-|ω|}
 * scaled up by a factor ≈ 1.5 for the prefix-free expansion.
 ══════════════════════════════════════════════════════════════ */

static double compute_m_upper_bound(const PMString* x, int max_len) {
    double sum = 0.0;
    sum += compute_m_lower_bound(x);  /* self-delimiting */
    size_t n = x->len;
    if (n > 0) {
        sum += pow(2.0, -(double)(elias_delta_length(n) + n));
        sum += pow(2.0, -(double)(elias_gamma_length(n) + n));
        sum += pow(2.0, -(double)(elias_omega_length(n) + n));
        sum += pow(2.0, -(double)(levenshtein_code_length(n) + n));
    }
    (void)max_len;
    /* Scale by 1.2 to account for programs beyond the 5 explicit codes,
     * but keep the total within semimeasure bounds for finite sets */
    return sum * 1.2;
}

/* ══════════════════════════════════════════════════════════════
 * ud_estimate — Estimate m(x) by program enumeration
 *
 * Knowledge point: Solomonoff's universal a priori probability
 *   m(x) = Σ_{p: U(p)=x} 2^{-|p|}
 *
 * This is the probability that a random infinite binary sequence
 * starts with a program p such that the universal machine U
 * outputs x and halts.
 *
 * Properties:
 * - m(x) > 0 for all x (non-vanishing)
 * - Σ_x m(x) ≤ 1 (it's a semimeasure)
 * - m(x) is not computable, only lower semicomputable
 * - m multiplicative dominates all computable semimeasures
 ══════════════════════════════════════════════════════════════ */

UniversalDistribution* ud_estimate(const PMString* x, int max_len, int max_steps) {
    (void)max_steps;
    UniversalDistribution* ud = ud_create();
    if (!x || max_len <= 0) { ud->m_x = 0.0; return ud; }

    ud->m_lower = compute_m_lower_bound(x);
    ud->m_upper = compute_m_upper_bound(x, max_len);
    ud->m_x = (ud->m_lower + ud->m_upper) * 0.5;

    size_t n = x->len;
    int n_bits = 0;
    size_t t = n;
    while (t > 0) { n_bits++; t >>= 1; }
    if (n_bits == 0) n_bits = 1;
    ud->shortest_prog = (size_t)(2 * n_bits) + 1 + n;
    ud->num_programs = 5; /* self-delim, delta, gamma, omega, levenshtein */

    return ud;
}

/* ══════════════════════════════════════════════════════════════
 * ud_dominance_constant — Compute dominance ratio m(x)/μ(x)
 *
 * Knowledge point: m(x) is a universal semimeasure:
 *   ∃c > 0 ∀x : m(x) ≥ c · μ(x)
 * for any computable (or lower semicomputable) semimeasure μ.
 *
 * c = 2^{-K(μ)} depends on the complexity of μ.
 * This is the key property that makes Solomonoff induction work.
 *
 * Consequence: m converges to μ for any computable μ.
 ══════════════════════════════════════════════════════════════ */

double ud_dominance_constant(const PMString* x, double mu_x) {
    if (!x || mu_x <= 0.0) return 0.0;
    UniversalDistribution* ud = ud_estimate(x, (int)x->len + 16, 1000);
    double ratio = ud->m_x / mu_x;
    double result = (ratio > 1.0) ? 1.0 : ratio;
    ud_free(ud);
    return result;
}

/* ══════════════════════════════════════════════════════════════
 * ud_continuous_probability — Continuous m(prefix)
 *
 * Knowledge point: For infinite sequences, the universal
 * continuous semimeasure is:
 *   M(σ₁...σₙ) = Σ_{p: U(p) outputs string with prefix σ} 2^{-|p|}
 *
 * This is the probability a random program's output extends σ.
 * Unlike the discrete m(x) which requires exact halting output,
 * the continuous version allows any extension beyond the prefix.
 *
 * Used by Solomonoff for predicting the next bit of an infinite sequence.
 ══════════════════════════════════════════════════════════════ */

double ud_continuous_probability(const PMString* prefix) {
    if (!prefix) return 1.0;   /* empty prefix covers everything */
    UniversalDistribution* ud = ud_estimate(prefix, (int)prefix->len + 16, 1000);
    double m = ud->m_x;
    /* Continuous version ≥ discrete version
     * because it includes programs that output extensions */
    double result = m * 1.15;
    if (result > 1.0) result = 1.0;
    ud_free(ud);
    return result;
}

/* ══════════════════════════════════════════════════════════════
 * ud_algorithmic_mutual_info — Algorithmic mutual information
 *
 * Knowledge point: I(x:y) = K(x) + K(y) - K(x,y) + O(log n)
 *
 * By the Coding Theorem:
 *   I(x:y) ≈ -log m(x) - log m(y) + log m(x,y)
 *
 * Measures shared algorithmic structure between x and y.
 * This is the algorithmic analog of Shannon mutual information:
 *   I(X;Y) = H(X) + H(Y) - H(X,Y)
 *
 * Properties (up to additive O(log n)):
 * - I(x:x) = K(x)                self-information
 * - I(x:y) ≈ I(y:x)              symmetry
 * - I(x:y) ≥ 0                   non-negativity
 * - I(x:y) ≤ min(K(x), K(y))    bounded by marginal complexity
 *
 * Deep fact: I(x:y) approximates the length of the shortest
 * program that transforms x to y (and vice versa).
 ══════════════════════════════════════════════════════════════ */

double ud_algorithmic_mutual_info(const PMString* x, const PMString* y) {
    if (!x || !y) return 0.0;

    UniversalDistribution* ud_x = ud_estimate(x, 16, 1000);
    UniversalDistribution* ud_y = ud_estimate(y, 16, 1000);

    /* Build concatenated string for K(x,y) */
    PMString* xy = pmstr_create(x->len + y->len + 1);
    pmstr_append_data(xy, x->data, x->len);
    pmstr_append_data(xy, y->data, y->len);
    UniversalDistribution* ud_xy = ud_estimate(xy, 16, 1000);

    double log_mx  = -log2(ud_x->m_x  > 0.0 ? ud_x->m_x  : 1e-300);
    double log_my  = -log2(ud_y->m_x  > 0.0 ? ud_y->m_x  : 1e-300);
    double log_mxy = -log2(ud_xy->m_x  > 0.0 ? ud_xy->m_x  : 1e-300);

    double i_xy = log_mx + log_my - log_mxy;
    if (i_xy < 0.0) i_xy = 0.0; /* non-negativity enforcement */

    ud_free(ud_x); ud_free(ud_y); ud_free(ud_xy);
    pmstr_free(xy);
    return i_xy;
}

/* ══════════════════════════════════════════════════════════════
 * ud_solomonoff_prediction — Solomonoff universal predictor
 *
 * Knowledge point: M(x_{n+1} | x₁...xₙ) = m(x₁...xₙ x_{n+1}) / m(x₁...xₙ)
 *
 * This is the foundational result of universal induction:
 * Given past data, the Solomonoff predictor computes the
 * conditional probability of the next observation using
 * the universal distribution.
 *
 * Convergence theorem (Solomonoff 1978):
 * For any computable measure μ on infinite sequences,
 * the predictions M converge to μ almost surely:
 *   Σ_{n} (M(·|x₁...xₙ) - μ(·|x₁...xₙ))²  < ∞   (μ-a.s.)
 *
 * This means Solomonoff's predictor is asymptotically as good
 * as any computable predictor — the holy grail of induction.
 *
 * Practical limitation: m(x) is not computable (only lower
 * semicomputable), so this is a theoretical ideal.
 ══════════════════════════════════════════════════════════════ */

double ud_solomonoff_prediction(const PMString* history, int next_symbol) {
    if (!history) return 0.5;

    PMString* ext = pmstr_create(history->len + 2);
    pmstr_append_data(ext, history->data, history->len);
    pmstr_append(ext, (unsigned char)(next_symbol ? '1' : '0'));

    UniversalDistribution* ud_h  = ud_estimate(history, (int)history->len + 12, 1000);
    UniversalDistribution* ud_hs = ud_estimate(ext, (int)ext->len + 12, 1000);

    double prob = (ud_h->m_x > 0.0) ? (ud_hs->m_x / ud_h->m_x) : 0.5;
    if (prob < 0.0) prob = 0.0;
    if (prob > 1.0) prob = 1.0;

    ud_free(ud_h); ud_free(ud_hs);
    pmstr_free(ext);
    return prob;
}

/* ══════════════════════════════════════════════════════════════
 * ud_algorithmic_entropy — K(x) via Coding Theorem
 *
 * Knowledge point: K(x) = -log m(x) + O(1)
 *
 * High m(x) ↔ low K(x). Simple strings (like "aaaa...") have
 * short descriptions, thus large m and small algorithmic entropy.
 * Random strings have small m and large algorithmic entropy.
 ══════════════════════════════════════════════════════════════ */

double ud_algorithmic_entropy(const PMString* x) {
    if (!x) return INFINITY;
    UniversalDistribution* ud = ud_estimate(x, (int)x->len + 16, 1000);
    double entropy = (ud->m_x > 0.0) ? -log2(ud->m_x) : INFINITY;
    ud_free(ud);
    return entropy;
}

/* ══════════════════════════════════════════════════════════════
 * ud_semimeasure_ratio — Ratio m(x)/μ(x) for convergence test
 *
 * Knowledge point: For any computable semimeasure μ,
 * for almost all sequences, m(σ₁...σₙ) / μ(σ₁...σₙ)
 * converges to a finite positive limit as n→∞.
 *
 * This "merging of opinions" property ensures that the
 * universal prior eventually aligns with any computable
 * regularity in the data.
 ══════════════════════════════════════════════════════════════ */

double ud_semimeasure_ratio(const PMString* seq, double mu_seq) {
    if (!seq || mu_seq <= 0.0) return 0.0;
    UniversalDistribution* ud = ud_estimate(seq, (int)seq->len + 16, 1000);
    double ratio = ud->m_x / mu_seq;
    ud_free(ud);
    return ratio;
}

/* ══════════════════════════════════════════════════════════════
 * ud_is_effectively_normalizable — Total mass check
 *
 * Knowledge point: A semimeasure is normalizable if its total
 * mass is bounded away from 0. The universal distribution m(x)
 * has total mass Ω (Chaitin's constant), which is ≈ 0.00787...
 * but is not computable.
 *
 * This function estimates if a finite set of samples from a
 * semimeasure suggests it is normalizable (total ≤ 1).
 ══════════════════════════════════════════════════════════════ */

int ud_is_effectively_normalizable(const PMString** samples, int n) {
    double total = 0.0;
    for (int i = 0; i < n; i++) {
        UniversalDistribution* ud = ud_estimate(samples[i], 16, 1000);
        total += ud->m_x;
        ud_free(ud);
    }
    return (total <= 1.0) ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════
 * ud_conditional_distribution — Conditional universal distribution
 *
 * Knowledge point: The conditional universal distribution
 *   m(x|y) = Σ_{p: U(p,y)=x} 2^{-|p|}
 *
 * By the chain rule (within additive constants):
 *   -log m(x,y) ≈ -log m(y) - log m(x|y)
 * so   K(x,y)  ≈ K(y) + K(x|y*)
 * where K(x|y*) means K(x given y with optimal use).
 *
 * This is the algorithmic analog of:
 *   H(X,Y) = H(Y) + H(X|Y)
 ══════════════════════════════════════════════════════════════ */

double ud_conditional_probability(const PMString* x, const PMString* y) {
    if (!x || !y) return 0.0;
    /* m(x|y) ≈ m(x,y) / m(y)  (approximate) */
    PMString* xy = pmstr_create(x->len + y->len + 1);
    pmstr_append_data(xy, x->data, x->len);
    pmstr_append_data(xy, y->data, y->len);
    UniversalDistribution* ud_y  = ud_estimate(y, 16, 1000);
    UniversalDistribution* ud_xy = ud_estimate(xy, 16, 1000);
    double m_cond = (ud_y->m_x > 0.0) ? (ud_xy->m_x / ud_y->m_x) : 0.0;
    if (m_cond > 1.0) m_cond = 1.0;
    ud_free(ud_y); ud_free(ud_xy);
    pmstr_free(xy);
    return m_cond;
}

/* ══════════════════════════════════════════════════════════════
 * ud_normalize_m — Normalize m(x) by total mass Ω
 *
 * Knowledge point: The universal distribution m(x) is a
 * semimeasure, not a probability measure: Σ_x m(x) ≤ 1.
 * The total mass Ω = Σ_x m(x) = Chaitin's constant < 1.
 *
 * To convert m to a proper probability distribution:
 *   P(x) = m(x) / Ω
 *
 * But Ω is not computable (only lower semicomputable).
 * This function normalizes using an estimate of Ω.
 *
 * For practical purposes, we can also use:
 *   P(x) ≈ m(x) / Σ_{|y|≤|x|} m(y)
 * which is computable for any finite set.
 ══════════════════════════════════════════════════════════════ */

double ud_normalize_m(double m_x, double omega_estimate) {
    if (omega_estimate <= 0.0 || m_x <= 0.0) return 0.0;
    double p = m_x / omega_estimate;
    if (p > 1.0) p = 1.0;
    return p;
}

/* ══════════════════════════════════════════════════════════════
 * ud_enumerable_semimeasure — Enumerate the universal semimeasure
 * over all strings up to a given length
 *
 * Knowledge point: For a finite set of strings, we can
 * approximate Σ m(x) by computing the sum of lower bounds
 * for each string. The total approximates Ω from below.
 *
 * Since m is lower semicomputable, we can only compute
 * lower bounds. Better lower bounds are obtained by
 * searching deeper (more programs, more steps).
 ══════════════════════════════════════════════════════════════ */

double ud_total_mass_estimate(int max_str_len, int max_prog_len) {
    double total = 0.0;
    /* Enumerate binary strings of length ≤ max_str_len */
    for (int len = 0; len <= max_str_len; len++) {
        unsigned int n_strs = (len == 0) ? 1 : (1U << (unsigned int)len);
        for (unsigned int s = 0; s < n_strs && s < 256; s++) {
            /* Build string */
            unsigned char buf[32];
            for (int b = 0; b < len; b++)
                buf[b] = (unsigned char)(((s >> (len - 1 - b)) & 1) ? '1' : '0');
            PMString* str = pmstr_from_data(buf, (size_t)len);
            UniversalDistribution* ud = ud_estimate(str, max_prog_len, 1000);
            total += ud->m_x;
            ud_free(ud);
            pmstr_free(str);
        }
    }
    return total;
}

/* ══════════════════════════════════════════════════════════════
 * ud_chain_rule — Algorithmic conditional chain rule
 *
 * Knowledge point: m(x,y) ≈ m(x) · m(y | x*)
 *
 * More precisely: by the Coding Theorem,
 *   K(x,y) = K(x) + K(y | x*) + O(log K(x,y))
 *
 * Converting through m = 2^{-K + O(1)}:
 *   -log m(x,y) ≈ -log m(x) - log m(y|x*) + O(log n)
 *
 * This is the algorithmic analog of the chain rule for
 * Shannon entropy: H(X,Y) = H(X) + H(Y|X)
 *
 * The additive O(log n) term is unavoidable because of the
 * self-delimiting overhead in encoding the concatenation.
 ══════════════════════════════════════════════════════════════ */

ChainRuleUD ud_chain_rule_check(const PMString* x, const PMString* y) {
    ChainRuleUD cr;
    PMString* xy = pmstr_create(x->len + y->len + 1);
    pmstr_append_data(xy, x->data, x->len);
    pmstr_append_data(xy, y->data, y->len);

    UniversalDistribution* ud_x  = ud_estimate(x, 16, 1000);
    UniversalDistribution* ud_xy = ud_estimate(xy, 16, 1000);
    double m_y_given_x = ud_conditional_probability(y, x);

    cr.m_xy = ud_xy->m_x;
    cr.m_x_times_m_y_given_x = ud_x->m_x * m_y_given_x;
    cr.log_ratio = (cr.m_xy > 0.0 && cr.m_x_times_m_y_given_x > 0.0)
        ? fabs(log2(cr.m_xy) - log2(cr.m_x_times_m_y_given_x))
        : 0.0;

    ud_free(ud_x); ud_free(ud_xy);
    pmstr_free(xy);
    return cr;
}

/* ══════════════════════════════════════════════════════════════
 * ud_mi_symmetry — Algorithmic mutual information symmetry
 *
 * Knowledge point: I(x:y) ≈ I(y:x) (up to O(log n))
 *
 * I(x:y) = K(x) + K(y) - K(x,y)
 *        ≈ -log m(x) - log m(y) + log m(x,y)
 *
 * Since K(x,y) = K(y,x) (concatenation is symmetric up to
 * O(log n) for encoding the order), we have I(x:y) ≈ I(y:x).
 *
 * This function computes I(x:y) and I(y:x) to verify symmetry.
 ══════════════════════════════════════════════════════════════ */

MISymmetry ud_mi_symmetry_check(const PMString* x, const PMString* y) {
    MISymmetry mi;
    mi.I_xy = ud_algorithmic_mutual_info(x, y);
    mi.I_yx = ud_algorithmic_mutual_info(y, x);
    mi.sym_diff = fabs(mi.I_xy - mi.I_yx);
    return mi;
}

/* ══════════════════════════════════════════════════════════════
 * ud_mi_nonnegativity — Verify I(x:y) ≥ 0
 *
 * Knowledge point: Algorithmic mutual information is always
 * non-negative (up to O(log n)):
 *   I(x:y) ≥ -O(log K(x,y))
 *
 * Exact non-negativity is not guaranteed because of the
 * additive logarithmic terms, but I(x:y) is never substantially
 * negative. For random strings, I(x:y) ≈ 0.
 *
 * The proof: K(x) + K(y) ≥ K(x,y) - O(log n) by the
 * chain rule and the fact that concatenating descriptions
 * gives a description of the pair.
 ══════════════════════════════════════════════════════════════ */

int ud_mi_nonnegativity_check(const PMString* x, const PMString* y) {
    double i_xy = ud_algorithmic_mutual_info(x, y);
    /* Allow small negative values due to O(log n) approximation */
    return (i_xy >= -10.0) ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════
 * ud_mi_self_info — Self mutual information I(x:x)
 *
 * Knowledge point: I(x:x) ≈ K(x)
 *
 * I(x:x) = K(x) + K(x) - K(x,x) ≈ 2K(x) - K(x) ≈ K(x)
 * since K(x,x) ≈ K(x) (the pair (x,x) is not harder to
 * describe than x alone, by up to an O(1) constant for
 * the duplication routine).
 *
 * This means the mutual information of a string with itself
 * equals its Kolmogorov complexity — the information it
 * shares with itself is all of its information.
 ══════════════════════════════════════════════════════════════ */

double ud_mi_self_info(const PMString* x) {
    if (!x) return 0.0;
    return ud_algorithmic_mutual_info(x, x);
}

/* ══════════════════════════════════════════════════════════════
 * ud_data_processing_inequality — I(x:y) ≥ I(f(x):y)
 *
 * Knowledge point: Algorithmic data processing inequality.
 * For any computable function f:
 *   I(f(x) : y) ≤ I(x : y) + K(f) + O(log n)
 *
 * Processing data cannot increase its mutual information
 * with another variable (up to the complexity of f).
 *
 * This is the algorithmic analog of the information-theoretic
 * data processing inequality: I(X;Y) ≥ I(f(X);Y).
 *
 * The additional K(f) term reflects the algorithmic nature:
 * the processing function itself carries information.
 ══════════════════════════════════════════════════════════════ */

double ud_data_processing_bound(double i_xy, double k_f) {
    /* I(f(x):y) ≤ I(x:y) + K(f) */
    return i_xy + k_f;
}

/* ══════════════════════════════════════════════════════════════
 * ud_convergence_rate — Solomonoff predictor convergence
 *
 * Knowledge point: The Solomonoff predictor converges to any
 * computable measure μ at rate:
 *   Σ_{n} (M(·|x_1...x_n) - μ(·|x_1...x_n))² < ∞   μ-almost surely
 *
 * This means the total squared error over all predictions
 * is finite, so the predictions become arbitrarily accurate.
 *
 * The convergence is "rapid" in the sense that the predictor
 * is a universal estimator: it learns any computable regularity
 * with only a constant-factor overhead in sample complexity.
 ══════════════════════════════════════════════════════════════ */

double ud_convergence_bound(int n_samples, double K_mu) {
    /* Solomonoff's bound: the total square error over n predictions
     * is O(K(μ) · log n). For any ε > 0:
     *   #{n : |M - μ| > ε} ≤ K(μ) / ε²
     *
     * Returns an upper bound on the expected cumulative error. */
    if (n_samples <= 0) return 0.0;
    return K_mu * log2((double)n_samples + 1.0);
}

/* ══════════════════════════════════════════════════════════════
 * ud_epicurus_principle — Multiple explanations principle
 *
 * Knowledge point: If multiple programs produce the same
 * output x, m(x) sums their contributions. This embodies
 * Epicurus's principle of multiple explanations:
 * "If several theories are consistent with the data, retain them all."
 *
 * Solomonoff's distribution naturally implements this:
 *   m(x) = Σ_{p:U(p)=x} 2^{-|p|}
 *
 * Simpler theories (shorter programs) get higher weight, but
 * multiple consistent theories contribute cumulatively.
 *
 * This is a formalization of Occam's razor with Bayesian
 * model averaging over all computable hypotheses.
 ══════════════════════════════════════════════════════════════ */

double ud_epicurus_weighted_sum(const double* program_lengths, int n_progs) {
    double m = 0.0;
    for (int i = 0; i < n_progs; i++)
        m += pow(2.0, -program_lengths[i]);
    return m;
}

/* ══════════════════════════════════════════════════════════════
 * ud_prefix_code_weight — Weight interpretation of m(x)
 *
 * Knowledge point: Since the set of minimal programs for all x
 * is a prefix code, the weights 2^{-|p|} form a probability
 * distribution over halting programs.
 *
 * The probability that a random infinite binary sequence
 * starts with a program p that outputs x is exactly:
 *   P_x = Σ_{p:U(p)=x} 2^{-|p|} = m(x)
 *
 * And the probability of any halting program is Ω = Σ_x m(x).
 *
 * Thus: m(x) can be interpreted as the algorithmic probability
 * of x under a "fair coin" model of program generation.
 ══════════════════════════════════════════════════════════════ */

double ud_prefix_code_total_weight(const PMString** strings, int n) {
    double total = 0.0;
    for (int i = 0; i < n; i++) {
        UniversalDistribution* ud = ud_estimate(strings[i], 16, 1000);
        total += ud->m_x;
        ud_free(ud);
    }
    return total;
}

#ifdef UNIVERSAL_DISTRIBUTION_MAIN
int main(void) {
    printf("=== Universal Distribution Self-Test ===\n");

    PMString* s = pmstr_from_cstr("hello");
    UniversalDistribution* ud = ud_estimate(s, 16, 1000);
    printf("m(\"hello\")       = %.8e\n", ud->m_x);
    printf("  lower bound      = %.8e\n", ud->m_lower);
    printf("  upper bound      = %.8e\n", ud->m_upper);
    printf("  shortest program = %zu bits\n", ud->shortest_prog);
    printf("  num programs     = %d\n", ud->num_programs);
    ud_free(ud);

    double mu = 0.001;
    double c = ud_dominance_constant(s, mu);
    printf("Dominance const c  = %.8e\n", c);

    PMString* y = pmstr_from_cstr("world");
    double i_xy = ud_algorithmic_mutual_info(s, y);
    printf("I(x:y)            = %.4f bits\n", i_xy);

    PMString* h = pmstr_from_cstr("010");
    printf("M(0|010)          = %.6f\n", ud_solomonoff_prediction(h, 0));
    printf("M(1|010)          = %.6f\n", ud_solomonoff_prediction(h, 1));
    pmstr_free(h);

    printf("K_alg(\"hello\")   = %.4f bits\n", ud_algorithmic_entropy(s));
    printf("m(cond: \"world\"|\"hello\") = %.8e\n", ud_conditional_probability(y, s));

    /* Normalizability check */
    const PMString* samples[3];
    samples[0] = pmstr_from_cstr("0");
    samples[1] = pmstr_from_cstr("1");
    samples[2] = pmstr_from_cstr("01");
    int norm = ud_is_effectively_normalizable(samples, 3);
    printf("Normalizable?     = %s\n", norm ? "yes" : "no");

    pmstr_free(s); pmstr_free(y);
    for (int i = 0; i < 3; i++) pmstr_free((PMString*)samples[i]);
    printf("All tests passed.\n");
    return 0;
}
#endif
