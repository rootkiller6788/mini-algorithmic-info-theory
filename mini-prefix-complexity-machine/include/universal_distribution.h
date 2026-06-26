/*
 * universal_distribution.h — Universal a priori probability m(x)
 *
 * L2: m(x) = Σ_{p: U(p)=x} 2^{-|p|}
 * This is the probability that a random infinite binary sequence
 * has a prefix p such that U(p) = x.
 *
 * L4: Coding Theorem: K(x) = -log m(x) + O(1)
 * This connects Kolmogorov complexity to probability theory.
 *
 * L5: Algorithm for estimating m(x) by enumerating short programs.
 *
 * Reference: Solomonoff (1964), Levin (1974), Li & Vitányi §4.3
 * Courses: MIT 6.841 §5, Stanford CS254 §4
 */

#ifndef UNIVERSAL_DISTRIBUTION_H
#define UNIVERSAL_DISTRIBUTION_H

#include "prefix_machine.h"

typedef struct {
    double  m_x;           /* universal a priori probability */
    double  m_upper;       /* upper bound estimate */
    double  m_lower;       /* lower bound estimate */
    size_t  shortest_prog; /* shortest known program length */
    int     num_programs;  /* number of programs found */
} UniversalDistribution;

UniversalDistribution* ud_create(void);
void ud_free(UniversalDistribution* ud);

/* Estimate m(x) by enumerating programs up to max_len */
UniversalDistribution* ud_estimate(const PMString* x, int max_len, int max_steps);

/* Dominant idea: m(x) dominates any computable semimeasure μ:
 *   ∃c ∀x: m(x) ≥ c · μ(x)
 */
double ud_dominance_constant(const PMString* x, double mu_x);

/* The continuous universal distribution:
 * m(x₁...xₙ) where the output is an infinite sequence
 */
double ud_continuous_probability(const PMString* prefix);

/* Mutual information in algorithmic information theory:
 * I(x:y) = K(x) + K(y) - K(x,y) + O(log n)
 * ≈ -log m(x) - log m(y) + log m(x,y)
 */
double ud_algorithmic_mutual_info(const PMString* x, const PMString* y);

/* Solomonoff's universal prior for sequence prediction:
 * M(xₙ₊₁ | x₁...xₙ) = m(x₁...xₙ₊₁) / m(x₁...xₙ)
 */
double ud_solomonoff_prediction(const PMString* history, int next_symbol);

/* Algorithmic entropy via Coding Theorem: K(x) = -log m(x) + O(1) */
double ud_algorithmic_entropy(const PMString* x);

/* Semimeasure ratio m(x)/mu(x) — convergence diagnostic */
double ud_semimeasure_ratio(const PMString* seq, double mu_seq);

/* Check if a set of samples suggests a normalizable semimeasure */
int ud_is_effectively_normalizable(const PMString** samples, int n);

/* Conditional universal distribution m(x|y) ≈ m(x,y)/m(y) */
double ud_conditional_probability(const PMString* x, const PMString* y);

/* Normalize m(x) by total mass (omega estimate) to get proper measure */
double ud_normalize_m(double m_x, double omega_estimate);

/* ── Total Mass ────────────────────────────────────────────── */
double ud_total_mass_estimate(int max_str_len, int max_prog_len);

/* ── Chain Rule ────────────────────────────────────────────── */
typedef struct {
    double m_xy;
    double m_x_times_m_y_given_x;
    double log_ratio;
} ChainRuleUD;

ChainRuleUD ud_chain_rule_check(const PMString* x, const PMString* y);

/* ── Mutual Information Symmetry ───────────────────────────── */
typedef struct {
    double I_xy;
    double I_yx;
    double sym_diff;
} MISymmetry;

MISymmetry ud_mi_symmetry_check(const PMString* x, const PMString* y);

/* ── MI Properties ─────────────────────────────────────────── */
int    ud_mi_nonnegativity_check(const PMString* x, const PMString* y);
double ud_mi_self_info(const PMString* x);

/* ── Data Processing Inequality ────────────────────────────── */
double ud_data_processing_bound(double i_xy, double k_f);

/* ── Convergence ───────────────────────────────────────────── */
double ud_convergence_bound(int n_samples, double K_mu);

/* ── Epicurus Principle ────────────────────────────────────── */
double ud_epicurus_weighted_sum(const double* program_lengths, int n_progs);

/* ── Prefix Code Total Weight ──────────────────────────────── */
double ud_prefix_code_total_weight(const PMString** strings, int n);

#endif /* UNIVERSAL_DISTRIBUTION_H */
