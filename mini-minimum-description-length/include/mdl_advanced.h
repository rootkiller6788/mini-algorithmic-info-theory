/*
 * mdl_advanced.h ? Advanced MDL Topics (L7-L8)
 *
 * L7 Applications: MDL in model selection vs AIC/BIC, predictive MDL,
 *                  MDL change detection in real-world signals.
 * L8 Advanced: mixture MDL, minimum message length (MML),
 *              MDL for model averaging, prequential analysis.
 *
 * Reference: Grunwald (2007) Ch. 14-17
 *            Wallace & Boulton (1968) "An Information Measure for Classification"
 *            Rissanen (1984) "Universal Coding, Information, Prediction, and Estimation"
 *            Hansen & Yu (2001) "Model Selection and MDL"
 */

#ifndef MDL_ADVANCED_H
#define MDL_ADVANCED_H

#include "mdl_core.h"
#include "nml.h"

/* Model Selection Information Criteria (L7)
 * Compare MDL with AIC, BIC, and other criteria. */

typedef struct {
    double aic;
    double aicc;
    double bic;
    double mdl_two_part;
    double mdl_nml;
    double hannan_quinn;
    int    n_params;
    int    n_samples;
} ModelCriteria;

ModelCriteria* mdl_criteria_compute(const MDLData* data, int k, double nll);
void           mdl_criteria_print(const ModelCriteria* c);
void           mdl_criteria_free(ModelCriteria* c);

/* Predictive MDL / Prequential Analysis (L8)
 * Dawid's prequential principle: evaluate models by sequential
 * prediction performance rather than fit to full dataset. */

typedef struct {
    double cumulative_log_loss;
    double mean_log_loss;
    double final_regret;
    double* sequential_losses;
    int     n_steps;
} PrequentialResult;

PrequentialResult* mdl_prequential_gaussian(const MDLData* data, int k);
PrequentialResult* mdl_prequential_bernoulli(const MDLData* data);
double             mdl_prequential_cumulative_loss(const PrequentialResult* pr);
void               mdl_prequential_print(const PrequentialResult* pr);
void               mdl_prequential_free(PrequentialResult* pr);

/* Minimum Message Length (MML) Connection (L8)
 * Wallace's MML is closely related to MDL but uses a different
 * prior encoding. MML87 approximation for exponential families. */

double mdl_mml87_estimate(const MDLData* data, int k);
double mdl_mdl_mml_divergence(const MDLData* data, int k);

/* MDL for Model Averaging (L8)
 * Instead of selecting one model, average with MDL-based weights.
 * The "Mixture MDL" criterion: -log sum_w P(x|M). */

double mdl_mixture_weight(int n, int k, double nll);
double mdl_mixture_mdl(const double* nlls, const int* ks, int n_models, int n);
int    mdl_optimal_model_by_weights(const double* nlls, const int* ks, int n_models, int n);

/* MDL in Decision Trees (L7, L5)
 * MDL pruning for decision trees: encode tree structure + prediction errors.
 * Quinlan & Rivest (1989). */

typedef struct {
    int     n_nodes;
    int     n_leaves;
    int     max_depth;
    double  mdl_cost;
    double  classification_error;
} MDLDecisionTree;

double mdl_decision_tree_cost(int n_samples, int n_leaves,
                               int n_errors, int n_classes);

/* Change Detection in Real-World Signals (L7 Application)
 * MDL for detecting structural breaks in GPS/navigation data,
 * climate time series, and industrial process monitoring. */

typedef struct {
    int     n_breakpoints;
    int*    breakpoints;
    double* segment_models;  /* one per segment */
    double  total_cost;
    int     data_length;
} MDLStructuralBreak;

MDLStructuralBreak* mdl_detect_breaks(const MDLData* data,
                                       double threshold, int max_breaks);
void                mdl_breaks_print(const MDLStructuralBreak* sb);
void                mdl_breaks_free(MDLStructuralBreak* sb);

/* MDL for Image Segmentation (L7 Application)
 * Use MDL to determine optimal number of regions in image segmentation. */

typedef struct {
    int     n_regions;
    int*    region_sizes;
    double  mdl_cost;
    int     total_pixels;
} MDLSegmentation;

double mdl_segmentation_cost(int n_pixels, int n_regions,
                              const int* region_sizes, int n_values);

#endif /* MDL_ADVANCED_H */
