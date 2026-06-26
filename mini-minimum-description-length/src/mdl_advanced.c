/*
 * mdl_advanced.c ? Advanced MDL Implementation (L7 Applications, L8 Advanced)
 *
 * Implements: model criteria (AIC/BIC/MDL comparison), prequential analysis,
 * MML connection, mixture MDL, decision tree MDL, structural break detection,
 * segmentation MDL.
 *
 * Reference: Grunwald (2007) "The Minimum Description Length Principle" Ch.14-17
 *            Wallace & Boulton (1968) "An Information Measure for Classification"
 *            Rissanen (1984) "Universal Coding, Information, Prediction, and Estimation"
 *            Hansen & Yu (2001) "Model Selection and the Principle of MDL"
 *            Quinlan & Rivest (1989) "Inferring Decision Trees Using MDL"
 */

#include "../include/mdl_advanced.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

/* Model Selection Information Criteria */

ModelCriteria* mdl_criteria_compute(const MDLData* data, int k, double nll) {
    /*
     * Compute all major model selection criteria for comparison.
     *
     * AIC  (Akaike 1974):  2k - 2 log L
     * AICc (Hurvich & Tsai): AIC + 2k(k+1)/(n-k-1)  small-sample correction
     * BIC  (Schwarz 1978):  k log n - 2 log L
     * MDL two-part:         (k/2)log n + NLL
     * MDL NML:              NLL + log C_n
     * Hannan-Quinn:         2k log log n - 2 log L
     *
     * All in bits. nll is negative log-likelihood (in nats).
     */
    if (!data || data->n == 0) return NULL;
    size_t n = data->n;
    double nll_bits = nll / M_LN2;

    ModelCriteria* c = (ModelCriteria*)malloc(sizeof(ModelCriteria));
    assert(c != NULL);
    c->n_params = k;
    c->n_samples = (int)n;

    /* AIC: penalize each parameter by 2 nats */
    c->aic = 2.0 * (double)k + 2.0 * nll;

    /* AICc: small-sample correction */
    if ((int)n > k + 1) {
        c->aicc = c->aic + 2.0 * (double)k * (double)(k + 1)
                          / (double)((int)n - k - 1);
    } else {
        c->aicc = c->aic;
    }

    /* BIC: penalty grows with log(n) */
    c->bic = (double)k * log((double)n) + 2.0 * nll;

    /* MDL two-part: (k/2) log2(n) bits */
    c->mdl_two_part = 0.5 * (double)k * log2((double)n) + nll_bits;

    /* MDL NML: (k/2) log2(n/(2pi)) bits */
    c->mdl_nml = nml_parametric_complexity_approx((int)n, k) + nll_bits;

    /* Hannan-Quinn: 2k log log n */
    double log_log_n = log(log((double)n));
    if ((int)n > 1)
        c->hannan_quinn = 2.0 * (double)k * log_log_n + 2.0 * nll;
    else
        c->hannan_quinn = c->aic;

    return c;
}

void mdl_criteria_print(const ModelCriteria* c) {
    if (!c) return;
    printf("Model Criteria (k=%d, n=%d):\n", c->n_params, c->n_samples);
    printf("  AIC (Akaike):     %.4f nats\n", c->aic);
    printf("  AICc (corrected): %.4f nats\n", c->aicc);
    printf("  BIC (Schwarz):    %.4f nats\n", c->bic);
    printf("  MDL two-part:     %.4f bits\n", c->mdl_two_part);
    printf("  MDL NML:          %.4f bits\n", c->mdl_nml);
    printf("  Hannan-Quinn:     %.4f nats\n", c->hannan_quinn);
}

void mdl_criteria_free(ModelCriteria* c) {
    free(c);
}

/* Prequential Analysis (Dawid 1984) */

PrequentialResult* mdl_prequential_gaussian(const MDLData* data, int k) {
    /*
     * Prequential MDL: evaluate model by sequential prediction.
     *
     * For each t from k+1 to n:
     *   1. Fit model to data[0..t-1]
     *   2. Predict data[t] and compute log loss
     *
     * The cumulative loss measures predictive performance.
     * Regret = cumulative_loss - cum_loss of best fixed model.
     */
    if (!data || data->n < (size_t)(k + 2)) return NULL;
    size_t n = data->n;

    PrequentialResult* pr = (PrequentialResult*)malloc(
        sizeof(PrequentialResult));
    assert(pr != NULL);
    pr->n_steps = (int)(n - (size_t)k);
    pr->sequential_losses = (double*)calloc(
        (size_t)pr->n_steps, sizeof(double));
    assert(pr->sequential_losses != NULL);

    pr->cumulative_log_loss = 0.0;
    pr->final_regret = 0.0;

    for (size_t t = (size_t)k; t < n; t++) {
        /* Fit Gaussian to data[0..t-1] */
        double sum = 0.0, sum_sq = 0.0;
        for (size_t i = 0; i < t; i++) {
            sum += data->values[i];
            sum_sq += data->values[i] * data->values[i];
        }
        double mu = sum / (double)t;
        double var = sum_sq / (double)t - mu * mu;
        if (var < 1e-12) var = 1e-12;

        /* Predict data[t] */
        double x_t = data->values[t];
        double log_loss = 0.5 * (log(2.0 * M_PI * var)
                          + (x_t - mu) * (x_t - mu) / var);
        pr->sequential_losses[t - (size_t)k] = log_loss / M_LN2;
        pr->cumulative_log_loss += log_loss / M_LN2;
    }

    pr->mean_log_loss = pr->cumulative_log_loss / (double)pr->n_steps;

    /* Final regret: compare to best fixed model fitted to all data */
    double full_mu = mdl_data_mean(data);
    double full_var = mdl_data_variance(data, full_mu);
    if (full_var < 1e-12) full_var = 1e-12;
    double best_loss = 0.5 * (double)n * (log(2.0 * M_PI * full_var) + 1.0)
                       / M_LN2;
    pr->final_regret = pr->cumulative_log_loss - best_loss;

    return pr;
}

PrequentialResult* mdl_prequential_bernoulli(const MDLData* data) {
    /*
     * Prequential MDL for Bernoulli model.
     *
     * Predict each bit t using Laplace's rule of succession
     * (or equivalently, Bayesian updating with Beta(1/2,1/2) prior).
     */
    if (!data || data->n < 2) return NULL;
    size_t n = data->n;

    PrequentialResult* pr = (PrequentialResult*)malloc(
        sizeof(PrequentialResult));
    assert(pr != NULL);
    pr->n_steps = (int)n;
    pr->sequential_losses = (double*)calloc(n, sizeof(double));
    assert(pr->sequential_losses != NULL);

    pr->cumulative_log_loss = 0.0;
    double k = 0.5;  /* Jeffreys prior: Beta(0.5, 0.5) */

    for (size_t t = 0; t < n; t++) {
        double p_pred = (k + 0.5) / ((double)t + 1.0);
        double x_t = data->values[t];

        double loss;
        if (x_t > 0.5) loss = -log2(p_pred);
        else           loss = -log2(1.0 - p_pred);

        pr->sequential_losses[t] = loss;
        pr->cumulative_log_loss += loss;

        if (x_t > 0.5) k += 1.0;
    }

    pr->mean_log_loss = pr->cumulative_log_loss / (double)n;

    /* Best fixed model */
    double k_total = 0.0;
    for (size_t i = 0; i < n; i++)
        if (data->values[i] > 0.5) k_total += 1.0;
    double p_mle = k_total / (double)n;
    if (p_mle < 1e-12) p_mle = 1e-12;
    if (p_mle > 1.0 - 1e-12) p_mle = 1.0 - 1e-12;
    double best_loss = -k_total * log2(p_mle)
                       - ((double)n - k_total) * log2(1.0 - p_mle);
    pr->final_regret = pr->cumulative_log_loss - best_loss;

    return pr;
}

double mdl_prequential_cumulative_loss(const PrequentialResult* pr) {
    return pr ? pr->cumulative_log_loss : 0.0;
}

void mdl_prequential_print(const PrequentialResult* pr) {
    if (!pr) return;
    printf("PrequentialResult(steps=%d):\n", pr->n_steps);
    printf("  Cumulative log loss: %.4f bits\n", pr->cumulative_log_loss);
    printf("  Mean log loss/step:  %.6f bits\n", pr->mean_log_loss);
    printf("  Final regret:        %.4f bits\n", pr->final_regret);
}

void mdl_prequential_free(PrequentialResult* pr) {
    if (pr) {
        free(pr->sequential_losses);
        free(pr);
    }
}

/* MML Connection (Wallace & Freeman 1987) */

double mdl_mml87_estimate(const MDLData* data, int k) {
    /*
     * MML87 approximate message length for k-parameter exponential family:
     *
     * MML87 = -log h(theta) + (k/2)log(kappa_k) + (k/2)
     *        - log P(x|theta) + (k/2)log(Fisher)
     *
     * where h(theta) is the prior, kappa_k is the lattice constant.
     *
     * For unit prior and Gaussian: MML87 approx NLL + (k/2)log(n) + const.
     * This differs from MDL by the constant and log factor.
     *
     * Returns: MML87 message length in bits.
     */
    if (!data || data->n == 0 || k <= 0) return 0.0;
    size_t n = data->n;
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;

    /* NLL in bits */
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0) / M_LN2;

    /* MML penalty: (k/2)log(n) + (k/2)(1 + log(kappa_k))
     * where kappa_k is the k-dimensional optimal quantizing lattice constant.
     * For k=1: kappa_1 = 1/12, for k=2: kappa_2 approx 0.0802 */
    double kappa;
    switch (k) {
        case 1: kappa = 1.0 / 12.0; break;
        case 2: kappa = 0.0802; break;
        case 3: kappa = 0.0633; break;
        default: kappa = 0.0500; break;
    }

    double mml_penalty = 0.5 * (double)k * log2((double)n)
                       + 0.5 * (double)k * (1.0 + log2(kappa));
    /* Add Fisher information determinant penalty */
    mml_penalty += 0.5 * log2(2.0);  /* det I approx 2 for unit variance */

    return nll + mml_penalty;
}

double mdl_mdl_mml_divergence(const MDLData* data, int k) {
    /*
     * Compute divergence between MDL (two-part) and MML87.
     *
     * MDL:  NLL + (k/2) log n
     * MML:  NLL + (k/2) log n + (k/2)(1 + log kappa_k) + ...
     *
     * Divergence (MML - MDL) is typically O(k).
     * For k=1: divergence approx 0.5 * log2(12/e) bits.
     *
     * Returns: MML_cost - MDL_cost in bits.
     */
    if (!data || data->n == 0 || k <= 0) return 0.0;
    double mml = mdl_mml87_estimate(data, k);
    size_t n = data->n;
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0) / M_LN2;
    double mdl = nll + 0.5 * (double)k * log2((double)n);
    return mml - mdl;
}

/* Mixture MDL / Model Averaging */

double mdl_mixture_weight(int n, int k, double nll) {
    /*
     * Compute MDL-based model weight for model averaging.
     *
     * w(M) = exp(-MDL(M)) / sum_j exp(-MDL(M_j))
     *
     * This is the exponential of the negative description length,
     * corresponding to the Bayesian posterior with a universal prior.
     *
     * Returns: log weight = -MDL in nats.
     */
    double mdl_cost = 0.5 * (double)k * log((double)n / (2.0 * M_PI))
                     + nll;
    return -mdl_cost;
}

double mdl_mixture_mdl(const double* nlls, const int* ks,
                        int n_models, int n) {
    /*
     * Mixture MDL: -log sum_k exp(-MDL_k)
     *
     * This is the code length of the mixture model, which is always
     * better than or equal to the best single model.
     *
     * Returns: mixture MDL in nats.
     */
    if (!nlls || !ks || n_models <= 0) return 0.0;

    double max_weight = -DBL_MAX;
    double* weights = (double*)malloc((size_t)n_models * sizeof(double));
    assert(weights != NULL);

    for (int i = 0; i < n_models; i++) {
        weights[i] = mdl_mixture_weight(n, ks[i], nlls[i]);
        if (weights[i] > max_weight) max_weight = weights[i];
    }

    double sum_exp = 0.0;
    for (int i = 0; i < n_models; i++)
        sum_exp += exp(weights[i] - max_weight);

    double log_sum = max_weight + log(sum_exp);
    free(weights);
    return -log_sum;
}

int mdl_optimal_model_by_weights(const double* nlls, const int* ks,
                                  int n_models, int n) {
    /*
     * Select model with highest MDL mixture weight.
     * Equivalent to selecting the model minimizing MDL.
     */
    if (!nlls || !ks || n_models <= 0) return -1;
    int best = 0;
    double best_mdl = DBL_MAX;

    for (int i = 0; i < n_models; i++) {
        double mdl = 0.5 * (double)ks[i] * log((double)n / (2.0 * M_PI))
                    + nlls[i];
        if (mdl < best_mdl) {
            best_mdl = mdl;
            best = i;
        }
    }
    return best;
}

/* MDL Decision Tree Pruning */

double mdl_decision_tree_cost(int n_samples, int n_leaves,
                               int n_errors, int n_classes) {
    /*
     * MDL cost for a decision tree (Quinlan & Rivest 1989):
     *
     * L(tree) + L(exceptions | tree)
     *
     * L(tree) = n_leaves * (1 + log2(n_classes))
     *          + n_internal_nodes * log2(n_attributes)
     *
     * L(exceptions) = n_errors * log2(n_samples)
     *                + n_correct * log2(accuracy)
     *
     * Simplified version for binary classification.
     *
     * Returns: total cost in bits.
     */
    if (n_samples <= 0 || n_leaves <= 0) return 0.0;

    /* Tree structure cost */
    int n_internal = (n_leaves > 1) ? n_leaves - 1 : 0;
    double tree_cost = (double)n_internal * log2((double)n_samples)
                     + (double)n_leaves * log2((double)n_classes);

    /* Exception cost */
    int n_correct = n_samples - n_errors;
    double exception_cost = 0.0;
    if (n_errors > 0)
        exception_cost += (double)n_errors * log2((double)n_samples);
    if (n_correct > 0) {
        double acc = (double)n_correct / (double)n_samples;
        exception_cost -= (double)n_correct * log2(acc);
    }

    return tree_cost + exception_cost;
}

/* Structural Break Detection (L7 Application) */

MDLStructuralBreak* mdl_detect_breaks(const MDLData* data,
                                        double threshold, int max_breaks) {
    /*
     * MDL-based structural break detection using binary segmentation.
     *
     * Starting with the full series, test each potential break point:
     *   Compute MDL cost with break minus MDL cost without break.
     *   If improvement > threshold, accept break and recurse.
     *
     * Applications: GPS trajectory segmentation, climate change detection,
     *               industrial process monitoring.
     */
    if (!data || data->n < 4 || max_breaks < 1) return NULL;
    size_t n = data->n;

    MDLStructuralBreak* sb = (MDLStructuralBreak*)malloc(
        sizeof(MDLStructuralBreak));
    assert(sb != NULL);
    sb->n_breakpoints = 0;
    sb->data_length = (int)n;
    sb->total_cost = 0.0;

    int capacity = max_breaks;
    sb->breakpoints = (int*)malloc((size_t)capacity * sizeof(int));
    sb->segment_models = (double*)malloc(
        (size_t)(capacity + 1) * 2.0 * sizeof(double));
    assert(sb->breakpoints && sb->segment_models);

    /* Compute baseline cost (no breaks) */
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;
    double baseline_cost = 0.5 * (double)n * (log(2.0 * M_PI * var)
                           + 1.0) / M_LN2 + log2((double)n);

    sb->segment_models[0] = mu;
    sb->segment_models[1] = var;
    sb->total_cost = baseline_cost;

    /* Simple greedy binary segmentation */
    double* segment_cost = (double*)malloc(((size_t)n + 1) * sizeof(double));
    assert(segment_cost != NULL);

    for (int bp_count = 0; bp_count < max_breaks; bp_count++) {
        double best_improvement = 0.0;
        int best_bp = -1;

        /* Try each position as a new break point */
        for (size_t bp = 2; bp < n - 1; bp++) {
            /* Check if bp is already used */
            int already_used = 0;
            for (int j = 0; j < sb->n_breakpoints; j++) {
                if (abs((int)bp - sb->breakpoints[j]) < 2) {
                    already_used = 1;
                    break;
                }
            }
            if (already_used) continue;

            /* MDL improvement = baseline - cost_with_new_break */
            struct { double mu; double var; double cost; int len; } seg[2];
            seg[0].len = (int)bp;
            seg[1].len = (int)(n - bp);

            double sum0 = 0.0, sum_sq0 = 0.0;
            for (size_t i = 0; i < bp; i++) {
                sum0 += data->values[i];
                sum_sq0 += data->values[i] * data->values[i];
            }
            seg[0].mu = sum0 / (double)bp;
            seg[0].var = sum_sq0 / (double)bp - seg[0].mu * seg[0].mu;
            if (seg[0].var < 1e-12) seg[0].var = 1e-12;

            double sum1 = 0.0, sum_sq1 = 0.0;
            for (size_t i = bp; i < n; i++) {
                sum1 += data->values[i];
                sum_sq1 += data->values[i] * data->values[i];
            }
            seg[1].mu = sum1 / (double)(n - bp);
            seg[1].var = sum_sq1 / (double)(n - bp)
                        - seg[1].mu * seg[1].mu;
            if (seg[1].var < 1e-12) seg[1].var = 1e-12;

            double new_cost = 0.0;
            for (int s = 0; s < 2; s++) {
                new_cost += 0.5 * (double)seg[s].len
                          * (log(2.0 * M_PI * seg[s].var) + 1.0) / M_LN2;
                new_cost += log2((double)seg[s].len);
            }
            /* Penalty for extra break point */
            new_cost += log2((double)n);

            double improvement = baseline_cost - new_cost;
            if (improvement > best_improvement) {
                best_improvement = improvement;
                best_bp = (int)bp;
            }
        }

        if (best_improvement < threshold) break;

        /* Accept this break point */
        sb->breakpoints[sb->n_breakpoints] = best_bp;
        sb->n_breakpoints++;

        /* Update segment models (simplified: just store means for now) */
        sb->segment_models[2 * bp_count] = (double)best_bp;
        sb->segment_models[2 * bp_count + 1] = best_improvement;
    }

    free(segment_cost);
    return sb;
}

void mdl_breaks_print(const MDLStructuralBreak* sb) {
    if (!sb) return;
    printf("MDLStructuralBreak(n=%d, breakpoints=%d, cost=%.4f bits):\n",
           sb->data_length, sb->n_breakpoints, sb->total_cost);
    for (int i = 0; i < sb->n_breakpoints; i++)
        printf("  Break %d at index %d\n", i, sb->breakpoints[i]);
}

void mdl_breaks_free(MDLStructuralBreak* sb) {
    if (sb) {
        free(sb->breakpoints);
        free(sb->segment_models);
        free(sb);
    }
}

/* MDL for Image Segmentation */

double mdl_segmentation_cost(int n_pixels, int n_regions,
                              const int* region_sizes, int n_values) {
    /*
     * MDL cost for image segmentation:
     *
     * L(segmentation) + L(pixel values | segmentation)
     *
     * L(segmentation) = n_regions * log2(n_pixels)  (encode boundaries)
     *
     * L(pixel values | segmentation):
     *   For each region, encode pixel values assuming uniform distribution.
     *   Cost = sum_r size_r * log2(n_values)
     *
     * Returns: total cost in bits.
     */
    if (n_pixels <= 0 || n_regions <= 0 || !region_sizes) return 0.0;

    /* Boundary cost: encode n_regions boundaries */
    double boundary_cost = (double)n_regions * log2((double)n_pixels);

    /* Pixel value cost within regions */
    double pixel_cost = 0.0;
    int total_check = 0;
    for (int r = 0; r < n_regions; r++) {
        int size = region_sizes[r];
        total_check += size;
        if (size > 0 && n_values > 1) {
            pixel_cost += (double)size * log2((double)n_values);
        }
    }
    /* Adjust for unaccounted pixels */
    if (total_check < n_pixels) {
        pixel_cost += (double)(n_pixels - total_check)
                    * log2((double)n_values);
    }

    return boundary_cost + pixel_cost;
}

#ifdef MDL_ADV_MAIN
int main(void) {
    printf("=== MDL Advanced Self-Test ===\n\n");

    /* Criteria comparison */
    double test_arr[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
                         1.5, 2.5, 3.5, 4.5, 5.5};
    MDLData* d = mdl_data_from_array(test_arr, 15);
    double mu = mdl_data_mean(d);
    double var = mdl_data_variance(d, mu);
    double nll = 0.5 * 15.0 * (log(2.0*M_PI*var) + 1.0);

    printf("Model criteria:\n");
    ModelCriteria* crit = mdl_criteria_compute(d, 2, nll);
    mdl_criteria_print(crit);
    mdl_criteria_free(crit);

    /* Prequential analysis */
    double seq_arr[] = {0.1, 0.2, 0.15, 0.3, 0.25, 0.4, 0.35,
                         5.1, 5.2, 5.0, 5.3, 5.1, 4.9, 5.2, 5.4};
    MDLData* seq = mdl_data_from_array(seq_arr, 15);
    printf("\nPrequential Gaussian:\n");
    PrequentialResult* pr = mdl_prequential_gaussian(seq, 2);
    mdl_prequential_print(pr);
    mdl_prequential_free(pr);

    /* MML */
    printf("\nMML connection:\n");
    printf("  MML87 (k=2): %.4f bits\n", mdl_mml87_estimate(d, 2));
    printf("  MDL-MML divergence: %.4f bits\n",
           mdl_mdl_mml_divergence(d, 2));

    /* Mixture MDL */
    double nlls[] = {nll, nll * 0.9, nll * 0.85};
    int ks[] = {1, 2, 3};
    printf("\nMixture MDL: %.4f nats\n",
           mdl_mixture_mdl(nlls, ks, 3, 15));
    printf("  Optimal model: %d\n",
           mdl_optimal_model_by_weights(nlls, ks, 3, 15));

    /* Decision tree */
    printf("\nDecision tree MDL:\n");
    printf("  Tree(n=1000, leaves=10, errors=50): %.4f bits\n",
           mdl_decision_tree_cost(1000, 10, 50, 5));

    /* Structural breaks */
    printf("\nStructural break detection:\n");
    MDLStructuralBreak* sb = mdl_detect_breaks(seq, 2.0, 3);
    mdl_breaks_print(sb);
    mdl_breaks_free(sb);

    /* Segmentation */
    printf("\nSegmentation MDL:\n");
    int region_sizes[] = {1000, 800, 1200, 500};
    printf("  Seg(n=3500, regions=4): %.4f bits\n",
           mdl_segmentation_cost(3500, 4, region_sizes, 256));

    mdl_data_free(d);
    mdl_data_free(seq);
    printf("\n=== All Advanced tests completed ===\n");
    return 0;
}
#endif /* MDL_ADV_MAIN */
