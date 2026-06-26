/*
 * nml.h — Normalized Maximum Likelihood Codes
 *
 * L3 Mathematical Structures: NML as a universal code.
 * L5 Algorithms: NML computation for common model families.
 *
 * NML (Shtarkov, 1987; Rissanen, 1996):
 *   P_nml(x) = P(x | θ̂(x)) / C_n
 *   where C_n = Σ_{y∈Y^n} P(y | θ̂(y)) is the normalization constant.
 *
 * The NML code is the unique minimax optimal universal code.
 *
 * Reference: Rissanen "Fisher Information and Stochastic Complexity" (1996)
 *            Grünwald (2007) Ch. 8-11
 *            Shtarkov (1987)
 */

#ifndef NML_H
#define NML_H

#include "mdl_core.h"

/* ══════════════════════════════════════════════════════════════
   NML Parametric Complexity
   ══════════════════════════════════════════════════════════════ */

/**
 * nml_parametric_complexity_approx — Compute parametric complexity
 * using Rissanen's asymptotic formula:
 *
 *   log C_n ≈ (k/2) log(n/(2π)) + log ∫_Θ √det I(θ) dθ + o(1)
 *
 * where k = number of parameters, I(θ) = Fisher information matrix.
 */
double nml_parametric_complexity_approx(int n, int k);

/**
 * nml_parametric_complexity_fisher — Compute parametric complexity
 * including Fisher information determinant.
 *
 * log C_n ≈ (k/2) log(n/(2π)) + log V(Θ)
 * where V(Θ) = ∫_Θ √det I(θ) dθ is the Jeffreys volume.
 */
double nml_parametric_complexity_fisher(int n, int k, double jeffreys_volume);

/**
 * nml_fisher_information_gaussian — Determinant of Fisher info for
 * Gaussian N(μ, σ²).
 * det I(μ, σ²) = 2/σ⁶
 */
double nml_fisher_information_gaussian(double sigma_sq);

/**
 * nml_fisher_information_bernoulli — Fisher information for Bernoulli(p).
 * I(p) = 1 / (p(1-p))
 */
double nml_fisher_information_bernoulli(double p);

/**
 * nml_fisher_information_poisson — Fisher information for Poisson(λ).
 * I(λ) = 1/λ
 */
double nml_fisher_information_poisson(double lambda);

/**
 * nml_fisher_information_exponential — Fisher info for Exponential(λ).
 * I(λ) = 1/λ²
 */
double nml_fisher_information_exponential(double lambda);

/* ══════════════════════════════════════════════════════════════
   NML Code Lengths for Specific Families
   ══════════════════════════════════════════════════════════════ */

/**
 * nml_gaussian_known_mean — NML for N(μ=0, σ²) with unknown variance.
 * log C_n ≈ ½log(πn/2)
 */
double nml_gaussian_known_mean(const MDLData* data);

/**
 * nml_gaussian_known_variance — NML for N(μ, σ²=1) with unknown mean.
 * log C_n ≈ ½log(πn/2)
 */
double nml_gaussian_known_variance(const MDLData* data);

/**
 * nml_gaussian_full — NML for N(μ, σ²) with both parameters unknown.
 * log C_n ≈ log n + ½log(2/π) + log Γ((n-1)/2) + constant
 */
double nml_gaussian_full(const MDLData* data);

/**
 * nml_bernoulli_full — NML for Bernoulli(p).
 * log C_n ≈ ½log(πn/2)  (asymptotic)
 * Exact: C_n = Σ_{k=0}^n (n choose k) (k/n)^k ((n-k)/n)^{n-k}
 */
double nml_bernoulli_full(const MDLData* data);

/**
 * nml_multinomial_full — NML for Multinomial with m categories.
 * log C_n ≈ ((m-1)/2) log(n/(2π)) + log(π^(m/2) / Γ(m/2))
 */
double nml_multinomial_full(const MDLData* data, int m);

/**
 * nml_poisson_full — NML for Poisson(λ).
 * log C_n ≈ ½log n (approximate, the constant is complex)
 */
double nml_poisson_full(const MDLData* data);

/**
 * nml_exponential_full — NML for Exponential(λ).
 * log C_n ≈ ½log n + ½log(2/π)
 */
double nml_exponential_full(const MDLData* data);

/**
 * nml_linear_regression_full — NML for linear regression y = Xβ + ε.
 * k = p + 1 parameters (p coefficients + σ²).
 * log C_n ≈ ((p+1)/2) log(n/(2π)) + log ∫ √det(I) dβdσ
 */
double nml_linear_regression_full(const MDLData* x, const MDLData* y);

/* ══════════════════════════════════════════════════════════════
   NML Model Selection
   ══════════════════════════════════════════════════════════════ */

/**
 * nml_select — Select best model class by NML.
 * Returns index of model minimizing NML cost.
 */
int nml_select(const double* nml_costs, int n_models);

/**
 * nml_compare — Compare two nested models using NML.
 * Returns NML cost(model2) - NML cost(model1).
 * Positive: model1 preferred (more complex model not worth it).
 */
double nml_compare_models(int n, int k1, int k2, double nll1, double nll2);

/**
 * nml_redunancy_rate — Compute redundancy rate:
 * (nml_cost - nll) / n → (k ln n) / (2n) as n → ∞
 */
double nml_redundancy_rate(const MDLData* data, int k);

/**
 * nml_asymptotic_equivalence — Verify NML ≈ Bayes with Jeffreys prior
 * for large n.
 */
double nml_bayes_jeffreys_gap(const MDLData* data, int k);

/**
 * nml_model_selection_power — Estimate statistical power of NML model
 * selection to detect k_true parameters over k_null parameters.
 * Uses chi-squared approximation to the likelihood ratio test.
 * Returns: estimated power in [0, 1].
 */
double nml_model_selection_power(int n, int k_true, int k_null,
                                  double effect_size);

#endif /* NML_H */
