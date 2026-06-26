/**
 * prediction.h - Solomonoff Universal Prediction Framework
 *
 * Core prediction algorithms based on algorithmic probability.
 * Given an observed sequence x_1...x_n, predict x_{n+1} using the
 * universal prior M. This implements Solomonoff induction.
 *
 * Key results:
 *   1. M converges to the true distribution for any computable measure.
 *   2. The total number of prediction errors is bounded by K(mu).
 *   3. M(x_{n+1}|x_1...x_n) -> mu(x_{n+1}|x_1...x_n) with mu-probability 1.
 *
 * Reference: Solomonoff, "Complexity-based induction systems", 1978.
 *            Hutter, "Universal Artificial Intelligence", 2005.
 *
 * Curriculum: MIT 6.841, Cambridge Part III, Stanford CS254
 */

#ifndef PREDICTION_H
#define PREDICTION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "solomonoff.h"

/* ---- Prediction Configuration ---- */
typedef struct {
    size_t depth;                /* Maximum program length to consider */
    uint64_t max_steps;          /* Maximum dovetailing steps */
    bool use_prefix_free;        /* Use prefix-free machine? */
    double confidence_threshold; /* Minimum confidence to make prediction */
    size_t top_k;                /* Top-k programs for ensemble analysis */
} prediction_config_t;

/* ---- Prediction Results ---- */
typedef struct {
    double prob_0;               /* P(next=0 | context) */
    double prob_1;               /* P(next=1 | context) */
    int predicted_bit;           /* -1 if uncertain, 0 or 1 otherwise */
    double confidence;           /* |prob_0 - prob_1| or entropy-based */
    size_t num_programs_used;    /* Number of programs that contributed */
    double total_prob_mass;      /* Sum of weights of matching programs */
    uint64_t steps_used;         /* Total computation steps used */
    /* Convergence diagnostic */
    double convergence_estimate; /* Estimated error from true probability */
} prediction_t;

/* ---- Sequential Predictor State ---- */
typedef struct {
    binary_string_t observed;    /* Accumulated observed bits */
    prediction_config_t config;  /* Configuration */
    prediction_t last_prediction;/* Most recent prediction */
    size_t num_predictions;      /* Total predictions made */
    size_t num_correct;          /* Number of correct predictions */
    double cumulative_loss;      /* Log loss: -sum log P(x_i|x_{<i}) */
    double running_M;            /* M(observed) for current prefix */
} sequential_predictor_t;

/* ---- Core Prediction API ---- */

/* Initialize predictor with configuration */
void predictor_init(sequential_predictor_t *sp, const prediction_config_t *config);

/* Reset predictor to empty context */
void predictor_reset(sequential_predictor_t *sp);

/* Predict next bit given current observed context */
prediction_t predictor_predict_next(sequential_predictor_t *sp);

/* Feed an observed bit and update internal state */
void predictor_observe(sequential_predictor_t *sp, int bit);

/* Predict the next k bits (multi-step lookahead) */
void predictor_predict_k(sequential_predictor_t *sp, size_t k,
                          double *prob_sequence, size_t seq_len);

/* Get algorithmic probability of the observed sequence so far */
double predictor_current_M(const sequential_predictor_t *sp);

/* Get prediction accuracy statistics */
void predictor_stats(const sequential_predictor_t *sp,
                     double *accuracy, double *avg_log_loss,
                     size_t *total_predictions);

/* ---- Batch Prediction ---- */

/* Predict next bit for an arbitrary binary string context */
prediction_t predict_next_bit(const binary_string_t *context,
                               size_t depth, uint64_t max_steps);

/* Predict the full continuation of a sequence up to max_additional bits */
size_t predict_continuation(const binary_string_t *prefix,
                             size_t max_additional_bits,
                             int *continuation, size_t cont_capacity,
                             size_t depth, uint64_t max_steps);

/* ---- Model Comparison ---- */

/* Compare M-based prediction to a Bernoulli(p) baseline */
typedef struct {
    double M_log_loss;           /* Cumulative log loss of M */
    double baseline_log_loss;    /* Cumulative log loss of baseline */
    double relative_improvement; /* Positive means M outperforms */
    size_t n_samples;
} model_comparison_t;

/* Compare Solomonoff predictor to a simple Bernoulli model */
model_comparison_t compare_to_bernoulli(const sequential_predictor_t *sp,
                                         double bernoulli_p);

/* ---- Convergence Analysis ---- */

/* Track prediction error vs depth */
typedef struct {
    size_t depth;
    double predicted_prob;
    double true_prob;
    double abs_error;
    double squared_error;
} convergence_record_t;

/* Record convergence data for a known distribution */
size_t convergence_study(const binary_string_t *sequence,
                          double true_bias,
                          size_t min_depth, size_t max_depth, size_t step,
                          uint64_t max_steps,
                          convergence_record_t *records, size_t max_records);

/* ---- Ensemble Analysis ---- */

/* Analyze which programs dominate the prediction */
typedef struct {
    binary_string_t program;
    size_t length;
    double weight;
    double contribution;   /* Weight * indicator[match] */
    binary_string_t output;
    bool produces_0;       /* This program predicts next bit = 0 */
    bool produces_1;       /* This program predicts next bit = 1 */
} ensemble_member_t;

size_t prediction_ensemble(const binary_string_t *context,
                            size_t depth, uint64_t max_steps,
                            ensemble_member_t *members, size_t max_members);

#endif
