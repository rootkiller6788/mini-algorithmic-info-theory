/*
 * two_part.h — Two-Part Code Construction for MDL
 *
 * L2 Core Concepts: explicit separation L(M) + L(D|M).
 * L5 Algorithms: constructing two-part codes for common model classes.
 *
 * Crude MDL: Choose model class, encode parameters explicitly,
 * then encode data given parameters.
 *
 * Key question: how many bits to encode continuous parameters?
 * Answer (Rissanen): encode each real parameter at precision O(1/√n),
 * giving cost (1/2)log₂ n per parameter.
 *
 * Reference: Rissanen (1989) "Stochastic Complexity in Statistical Inquiry"
 *            Barron & Cover (1991) "MDL and Two-Part Codes"
 */

#ifndef TWO_PART_H
#define TWO_PART_H

#include "mdl_core.h"

/* ══════════════════════════════════════════════════════════════
   Two-Part Code Construction
   ══════════════════════════════════════════════════════════════ */

/**
 * twopart_encode_integer — Encode integer n as a two-part code:
 * Part 1: unary encoding of ⌊log₂ n⌋ (the magnitude)
 * Part 2: binary encoding of n
 * Total: 2⌊log₂ n⌋ + 1 bits (Elias gamma)
 */
double twopart_encode_integer(size_t n);

/**
 * twopart_encode_real — Encode real parameter θ at optimal precision.
 * Per Rissanen, truncate to precision δ = 1/√n.
 * Cost: ½ log₂ n + log₂(1/δ') where δ' is prior range.
 */
double twopart_encode_real(double theta, double range_min, double range_max,
                            size_t n);

/**
 * twopart_encode_vector — Encode a k-dimensional real vector.
 * Cost: (k/2) log₂ n bits total.
 */
double twopart_encode_vector(int k, size_t n);

/**
 * twopart_code_linear_regression — Construct two-part code for
 * linear regression y = Xβ + ε.
 *
 * Part 1: encode β (k params) → (k/2)log₂ n bits
 * Part 2: encode residuals (n-k) → Gaussian NLL
 */
double twopart_code_linear_regression(const MDLData* x, const MDLData* y);

/**
 * twopart_code_polynomial — Two-part code for polynomial model.
 *
 * Part 1: encode degree d (universal integer code) + coefficients
 *         (d+1 params at ½log₂ n each)
 * Part 2: encode residuals ~ Gaussian
 */
double twopart_code_polynomial(const MDLData* x, const MDLData* y, int degree);

/**
 * twopart_code_histogram — Two-part code for histogram with k bins.
 *
 * Part 1: encode k (integer) + bin edges (k-1 reals)
 * Part 2: encode counts (multinomial likelihood)
 */
double twopart_code_histogram(const MDLData* data, int n_bins);

/**
 * twopart_code_piecewise_constant — Two-part code for piecewise
 * constant model (k segments).
 *
 * Part 1: k change-points × log₂ n each + k segment means
 * Part 2: Gaussian residuals per segment
 */
double twopart_code_piecewise_constant(const MDLData* data,
                                        const int* change_points,
                                        int n_changes);

/**
 * twopart_code_gaussian_mixture — Two-part code for k Gaussian components.
 *
 * Part 1: k-1 mixture weights ((k-1)/2 log₂ n), k means, k variances
 * Part 2: data assigned to components + Gaussian NLL per component
 */
double twopart_code_gaussian_mixture(const MDLData* data, int k);

/* ══════════════════════════════════════════════════════════════
   Two-Part Code Selection
   ══════════════════════════════════════════════════════════════ */

/**
 * twopart_select_model — Given a set of two-part costs, select best model.
 * Returns index of model minimizing total cost.
 */
int twopart_select_model(const double* costs, int n_models);

/**
 * twopart_compare_models — Compare two models using two-part MDL.
 * Returns negative if model1 is better (lower cost).
 */
double twopart_compare_models(double cost1, double cost2);

/**
 * twopart_optimal_polynomial_degree — Find degree d ∈ [0, max_deg]
 * minimizing two-part DL.
 */
int twopart_optimal_polynomial_degree(const MDLData* x, const MDLData* y,
                                       int max_degree);

/**
 * twopart_optimal_histogram_bins — Find bin count k ∈ [1, max_bins]
 * minimizing two-part DL.
 */
int twopart_optimal_histogram_bins(const MDLData* data, int max_bins);

/**
 * twopart_optimal_mixture_components — Find optimal k for Gaussian
 * mixture via two-part MDL.
 */
int twopart_optimal_mixture_components(const MDLData* data, int max_k);

#endif /* TWO_PART_H */
