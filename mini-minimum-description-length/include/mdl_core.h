/*
 * mdl_core.h — Minimum Description Length Core Definitions
 *
 * L1 Definitions: MDL principle, data types, model abstractions.
 * L2 Core Concepts: two-part codes, crude vs refined MDL, universal codes.
 * L3 Mathematical Structures: integer coding, prefix codes, redundancy.
 *
 * MDL Principle (Rissanen, 1978):
 *   Given data x, select model M minimizing L(M) + L(x|M).
 *   Crude MDL:  L(M) = explicit encoding of parameters
 *   Refined MDL: L(M) via NML (Normalized Maximum Likelihood)
 *
 * Reference: Rissanen "Optimal Estimation of Parameters" (2012)
 *            Grünwald "The Minimum Description Length Principle" (2007)
 * Courses: MIT 6.441, Stanford EE376A, Helsinki CS (Rissanen's group)
 */

#ifndef MDL_CORE_H
#define MDL_CORE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ══════════════════════════════════════════════════════════════
   Core Data Types
   ══════════════════════════════════════════════════════════════ */

/** Generic data array for MDL analysis */
typedef struct {
    double* values;
    size_t  n;
    size_t  cap;
} MDLData;

/** Two-part code: explicit separation of model and data description */
typedef struct {
    double  model_cost;      /* L(M) in bits */
    double  data_cost;       /* L(x|M) in bits */
    double  total_cost;      /* L(M) + L(x|M) */
    double* params;          /* model parameters */
    int     n_params;        /* number of parameters */
    int     model_class_id;  /* which model class */
} TwoPartCode;

/** NML code (Normalized Maximum Likelihood) */
typedef struct {
    double  nml_cost;                /* -log P_nml(x) in bits */
    double  parametric_complexity;   /* log C_n = log Σ_y P(y|θ̂(y)) */
    double* mle_params;             /* maximum likelihood estimate */
    int     n_params;
    int     data_size;
} NMLCode;

/** MDL result from model selection */
typedef struct {
    int     best_model_index;
    double  best_cost;
    double* costs;          /* all candidate costs */
    int     n_models;
    int     selected_complexity;
} MDLResult;

/** Polynomial model for MDL regression */
typedef struct {
    int     degree;
    double* coeffs;         /* length degree + 1 */
    double  mse;            /* mean squared error */
    double  two_part_cost;  /* total description length */
} MDLPolynomial;

/** Histogram model with MDL-optimal bin count */
typedef struct {
    int     n_bins;
    int*    counts;
    double* edges;          /* length n_bins + 1 */
    double  bin_width;
    double  mdl_cost;
} MDLHistogram;

/** Change point detection model */
typedef struct {
    int*    change_points;  /* indices of change points */
    int     n_changes;
    int     n_segments;
    double  total_cost;     /* total MDL cost */
    double* segment_means;
    double* segment_vars;
} MDLChangePoint;

/** MDL cluster model */
typedef struct {
    int     k;              /* number of clusters */
    int*    assignments;    /* cluster label per data point */
    size_t  n_points;
    double  total_cost;
    double* centroids;      /* centroid coordinates, k * dim */
    int     dim;            /* dimensionality of data */
} MDLCluster;

/** Time series model with MDL AR-order selection */
typedef struct {
    int     ar_order;       /* selected AR order */
    double* ar_coeffs;      /* AR coefficients */
    double  noise_var;      /* estimated noise variance */
    double  mdl_cost;
} MDLARModel;

/** MDL configuration/criterion enum */
typedef enum {
    MDL_CRUDE    = 0,  /* Two-part code: L(M) + L(D|M) */
    MDL_REFINED  = 1,  /* NML code: -log P_nml(D) */
    MDL_BAYESIAN = 2,  /* Mixture MDL: -log ∫ P(D|θ) dπ(θ) */
    MDL_PREDICTIVE = 3 /* Predictive MDL / prequential */
} MDLCriterion;

/* ══════════════════════════════════════════════════════════════
   MDL Data API
   ══════════════════════════════════════════════════════════════ */

MDLData*  mdl_data_create(size_t cap);
MDLData*  mdl_data_from_array(const double* arr, size_t n);
MDLData*  mdl_data_from_file(const char* filename);
MDLData*  mdl_data_clone(const MDLData* d);
void      mdl_data_append(MDLData* d, double v);
double    mdl_data_get(const MDLData* d, size_t i);
void      mdl_data_set(MDLData* d, size_t i, double v);
double    mdl_data_mean(const MDLData* d);
double    mdl_data_variance(const MDLData* d, double mean);
double    mdl_data_min(const MDLData* d);
double    mdl_data_max(const MDLData* d);
void      mdl_data_sort(MDLData* d);
void      mdl_data_print(const MDLData* d, size_t max_show);
void      mdl_data_free(MDLData* d);

/* ══════════════════════════════════════════════════════════════
   Universal Code Lengths for Integers (L3)
   ══════════════════════════════════════════════════════════════ */

/**
 * mdl_universal_integer_code — Rissanen's universal code for integers.
 * L(n) = log* n + log c where c ≈ 2.865064
 * Elias delta, Elias gamma, and Rissanen's log* codes.
 */
double mdl_elias_gamma_code(size_t n);
double mdl_elias_delta_code(size_t n);
double mdl_elias_omega_code(size_t n);
double mdl_rissanen_logstar_code(size_t n);
double mdl_universal_integer_bits(size_t n);

/* ══════════════════════════════════════════════════════════════
   Two-Part Code Fundamentals (L2, L5)
   ══════════════════════════════════════════════════════════════ */

/**
 * mdl_two_part_gaussian — L(M) + L(x|M) for Gaussian model.
 * Model: N(μ, σ²), parameters encoded with precision 1/√n per Rissanen.
 * L(μ, σ) ≈ log n + log n  (two real parameters)
 * L(x|μ,σ) = -log likelihood = (n/2)log(2πσ²) + (1/(2σ²)) Σ(x_i-μ)²
 */
double mdl_two_part_gaussian(const MDLData* data);

/**
 * mdl_two_part_bernoulli — Two-part code for binary data.
 * L(p) = ½ log n (for probability parameter)
 * L(data|p) = -k log p - (n-k) log(1-p)
 */
double mdl_two_part_bernoulli(const MDLData* data);

/**
 * mdl_two_part_poisson — Two-part code for count data.
 */
double mdl_two_part_poisson(const MDLData* data);

/**
 * mdl_two_part_multinomial — Two-part code for categorical data
 * with m categories.
 */
double mdl_two_part_multinomial(const MDLData* data, int m);

/**
 * mdl_two_part_exponential — Two-part code for exponential model.
 */
double mdl_two_part_exponential(const MDLData* data);

/**
 * mdl_two_part_gamma — Two-part code for gamma model.
 */
double mdl_two_part_gamma(const MDLData* data);

/* ══════════════════════════════════════════════════════════════
   NML (Normalized Maximum Likelihood) — Refined MDL (L5)
   ══════════════════════════════════════════════════════════════ */

/**
 * mdl_nml_parametric_complexity — Compute the parametric complexity
 * (normalization term) for a model class.
 *
 * C_n = Σ_{y ∈ Y^n} P(y | θ̂(y))
 *
 * For exponential families with k parameters:
 *   log C_n ≈ (k/2) log(n/(2π)) + log ∫_Θ √det I(θ) dθ
 */
double mdl_nml_parametric_complexity(int n, int k, double fisher_info_det);

/**
 * mdl_nml_gaussian — NML code length for Gaussian model.
 * NML cost = -log P(x|θ̂(x)) + log C_n
 */
double mdl_nml_gaussian(const MDLData* data);

/**
 * mdl_nml_bernoulli — NML code for Bernoulli model.
 */
double mdl_nml_bernoulli(const MDLData* data);

/**
 * mdl_nml_multinomial — NML code for multinomial model.
 */
double mdl_nml_multinomial(const MDLData* data, int m);

/**
 * mdl_nml_poisson — NML code for Poisson model.
 */
double mdl_nml_poisson(const MDLData* data);

/**
 * mdl_nml_linear_regression — NML code for linear regression.
 * y = Xβ + ε, ε ~ N(0, σ²)
 */
double mdl_nml_linear_regression(const MDLData* x, const MDLData* y);

/* ══════════════════════════════════════════════════════════════
   Redundancy and Regret (L4 Theorems)
   ══════════════════════════════════════════════════════════════ */

/**
 * mdl_redundancy — Expected redundancy for parametric model classes.
 * For k-parameter exponential family: redundancy ≈ (k/2) log(n/(2π))
 *   + log ∫ √det I(θ) dθ + o(1).
 */
double mdl_redundancy_bound(int n, int k);

/**
 * mdl_regret — Worst-case regret of MDL compared to the best model.
 * R_n = max_x [ -log P_mdl(x) - min_θ (-log P(x|θ)) ]
 */
double mdl_worst_case_regret(int n, int k);

/**
 * mdl_asymptotic_consistency_check — Verify that MDL selects the
 * correct model class as n → ∞.
 */
int mdl_consistency_test(const MDLData* data, int true_k, double tolerance);

/* ══════════════════════════════════════════════════════════════
   Model Selection API
   ══════════════════════════════════════════════════════════════ */

MDLResult* mdl_select_create(int n_models);
void       mdl_select_set_cost(MDLResult* r, int idx, double cost);
int        mdl_select_best(MDLResult* r);
void       mdl_select_print(const MDLResult* r);
void       mdl_select_free(MDLResult* r);

/* ══════════════════════════════════════════════════════════════
   Polynomial MDL (L6)
   ══════════════════════════════════════════════════════════════ */

MDLPolynomial* mdl_poly_fit(const MDLData* x, const MDLData* y, int degree);
int            mdl_poly_optimal_degree(const MDLData* x, const MDLData* y,
                                       int max_degree);
double         mdl_poly_evaluate(const MDLPolynomial* p, double x);
void           mdl_poly_print(const MDLPolynomial* p);
void           mdl_poly_free(MDLPolynomial* p);

/* ══════════════════════════════════════════════════════════════
   Histogram MDL (L6)
   ══════════════════════════════════════════════════════════════ */

MDLHistogram* mdl_histogram_create(const MDLData* data, int n_bins);
int           mdl_histogram_optimal_bins(const MDLData* data, int max_bins);
double        mdl_histogram_cost(const MDLHistogram* h);
void          mdl_histogram_print(const MDLHistogram* h);
void          mdl_histogram_free(MDLHistogram* h);

/* ══════════════════════════════════════════════════════════════
   Change Point Detection (L7)
   ══════════════════════════════════════════════════════════════ */

MDLChangePoint* mdl_changepoint_detect(const MDLData* data, int max_changes);
int             mdl_changepoint_optimal_count(const MDLData* data,
                                              int max_changes);
void            mdl_changepoint_print(const MDLChangePoint* cp);
void            mdl_changepoint_free(MDLChangePoint* cp);

/* ══════════════════════════════════════════════════════════════
   Clustering MDL (L7)
   ══════════════════════════════════════════════════════════════ */

MDLCluster* mdl_cluster_kmeans_mdl(const MDLData* data, int dim,
                                   int max_k);
int         mdl_cluster_optimal_k(const MDLData* data, int dim,
                                   int max_k);
double      mdl_cluster_cost(const MDLData* data, int dim,
                              const MDLCluster* c);
void        mdl_cluster_print(const MDLCluster* c);
void        mdl_cluster_free(MDLCluster* c);

/* ══════════════════════════════════════════════════════════════
   Time Series / AR Model MDL (L7)
   ══════════════════════════════════════════════════════════════ */

MDLARModel* mdl_ar_fit(const MDLData* data, int order);
int         mdl_ar_optimal_order(const MDLData* data, int max_order);
double      mdl_ar_cost(const MDLData* data, const MDLARModel* ar);
void        mdl_ar_print(const MDLARModel* ar);
void        mdl_ar_free(MDLARModel* ar);

/* ══════════════════════════════════════════════════════════════
   Information-Theoretic Utilities
   ══════════════════════════════════════════════════════════════ */

double mdl_log2(double x);
double mdl_log2_loss(double predicted, double actual);
double mdl_kl_divergence_gaussian(double mu1, double var1,
                                   double mu2, double var2);
double mdl_kl_divergence_bernoulli(double p, double q);
double mdl_kl_divergence_poisson(double lambda1, double lambda2);
size_t mdl_min_description_length_bound(const MDLData* data);

#endif /* MDL_CORE_H */
