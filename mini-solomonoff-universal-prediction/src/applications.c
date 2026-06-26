/**
 * applications.c - Applications of Algorithmic Probability (L7)
 *
 * Real-world applications of Solomonoff's universal prior:
 *   1. Time series prediction
 *   2. Anomaly detection
 *   3. Binary classification via algorithmic probability
 *   4. Sequence complexity profiling
 *   5. Change point detection
 *   6. Universal compression baseline
 *   7. Randomness testing
 *   8. Pattern discovery
 *
 * Reference: Hutter, "Universal Artificial Intelligence", 2005.
 *            Li & Vitanyi, Chapter 8 (Applications).
 *            Gammerman & Vovk, "Algorithmic learning theory".
 *
 * Curriculum:
 *   MIT 6.841 - AIXI and Universal AI
 *   CMU 15-751 - Applications of Kolmogorov complexity
 *   Princeton COS 551 - Algorithmic information in cryptography
 */

#include "solomonoff.h"
#include "kolmogorov.h"
#include "enumeration.h"
#include "prediction.h"
#include "universal_machine.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ================================================================
 * 1. Time Series Prediction (L7 - Application)
 * ================================================================ */

/**
 * solomonoff_time_series_predict - Predict next value in binary time series.
 *
 * Given a history of binary observations x_1,...,x_n, use algorithmic
 * probability to predict x_{n+1}. The ratio M(x0)/M(x1) determines
 * which continuation is more algorithmically plausible.
 *
 * This naturally captures periodic patterns, repetition, and other
 * computable regularities without explicit feature engineering.
 *
 * Application domains: financial markets, sensor failure, network
 * patterns, climate binary indicators.
 */
prediction_t solomonoff_time_series_predict(const int *history, size_t history_len,
                                              size_t depth, uint64_t max_steps) {
    binary_string_t ctx;
    bs_init(&ctx);
    for (size_t i = 0; i < history_len && ctx.length < SOLOMONOFF_MAX_STRING_LENGTH; i++) {
        bs_append_bit(&ctx, history[i] ? 1 : 0);
    }
    return predict_next_bit(&ctx, depth, max_steps);
}

/**
 * solomonoff_time_series_forecast - Multi-step ahead forecast.
 *
 * Predicts the next k bits using greedy sequential prediction:
 * at each step, predict the most probable next bit and feed it back
 * as if observed.
 */
size_t solomonoff_time_series_forecast(const int *history, size_t history_len,
                                         size_t forecast_horizon,
                                         int *forecast, size_t forecast_capacity,
                                         size_t depth, uint64_t max_steps) {
    if (forecast_capacity < forecast_horizon) return 0;
    binary_string_t ctx;
    bs_init(&ctx);
    for (size_t i = 0; i < history_len; i++) {
        bs_append_bit(&ctx, history[i] ? 1 : 0);
    }
    binary_string_t cur = ctx;
    for (size_t k = 0; k < forecast_horizon; k++) {
        prediction_t p = predict_next_bit(&cur, depth, max_steps);
        int next_bit = (p.prob_1 >= p.prob_0) ? 1 : 0;
        forecast[k] = next_bit;
        bs_append_bit(&cur, next_bit);
    }
    return forecast_horizon;
}

/* ================================================================
 * 2. Anomaly Detection (L7 - Application)
 * ================================================================ */

/**
 * solomonoff_anomaly_score - Anomaly score for a sequence.
 *
 * score = -log_2 M(x_n | x_1...x_{n-1})
 *
 * High scores indicate x_n is algorithmically surprising given its
 * context. Used for intrusion detection, fraud detection, industrial
 * monitoring, and medical diagnostics.
 *
 * Reference: Ferri & Hernandez-Orallo, "Anomaly detection via
 *            Kolmogorov complexity", 2021.
 */
double solomonoff_anomaly_score(const int *sequence, size_t seq_len,
                                  size_t depth, uint64_t max_steps) {
    if (seq_len < 2) return 0.0;
    binary_string_t prefix, full;
    bs_init(&prefix);
    bs_init(&full);
    for (size_t i = 0; i < seq_len - 1; i++) {
        bs_append_bit(&prefix, sequence[i] ? 1 : 0);
        bs_append_bit(&full, sequence[i] ? 1 : 0);
    }
    bs_append_bit(&full, sequence[seq_len - 1] ? 1 : 0);

    algprob_t M_prefix = solomonoff_M(&prefix, depth, max_steps);
    algprob_t M_full = solomonoff_M(&full, depth, max_steps);
    if (M_prefix < SOLOMONOFF_EPSILON) return 10.0;
    double conditional = M_full / M_prefix;
    if (conditional < SOLOMONOFF_EPSILON) return 20.0;
    return -log2(conditional);
}

size_t solomonoff_anomaly_detect_batch(const int *sequence, size_t seq_len,
                                         size_t window_size, double threshold,
                                         size_t *anomaly_positions, size_t max_positions,
                                         size_t depth, uint64_t max_steps) {
    size_t found = 0;
    for (size_t start = 0; start + window_size <= seq_len && found < max_positions; start++) {
        double score = solomonoff_anomaly_score(sequence + start, window_size,
                                                  depth, max_steps);
        if (score > threshold) anomaly_positions[found++] = start;
    }
    return found;
}

/* ================================================================
 * 3. Binary Classification (L7 - Application)
 * ================================================================ */

int solomonoff_classify(const binary_string_t *sample,
                         const binary_string_t *class0_data, size_t class0_count,
                         const binary_string_t *class1_data, size_t class1_count,
                         size_t depth, uint64_t max_steps) {
    binary_string_t ctx0, ctx1;
    bs_init(&ctx0);
    bs_init(&ctx1);
    for (size_t i = 0; i < class0_count; i++) bs_concat(&ctx0, &class0_data[i]);
    for (size_t i = 0; i < class1_count; i++) bs_concat(&ctx1, &class1_data[i]);
    algprob_t M0 = solomonoff_conditional_M(sample, &ctx0, depth, max_steps);
    algprob_t M1 = solomonoff_conditional_M(sample, &ctx1, depth, max_steps);
    return (M0 >= M1) ? 0 : 1;
}

/* ================================================================
 * 4. Complexity Profiling (L7 - Application)
 * ================================================================ */

size_t solomonoff_complexity_profile(const int *sequence, size_t seq_len,
                                       double *complexity_values, size_t max_values,
                                       size_t depth, uint64_t max_steps) {
    binary_string_t prefix;
    bs_init(&prefix);
    size_t count = 0;
    for (size_t i = 0; i < seq_len && count < max_values; i++) {
        bs_append_bit(&prefix, sequence[i] ? 1 : 0);
        complexity_values[count++] = solomonoff_algorithmic_info(&prefix, depth, max_steps);
    }
    return count;
}

/* ================================================================
 * 5. Change Point Detection (L7 - Application)
 * ================================================================ */

size_t solomonoff_change_point_detect(const int *sequence, size_t seq_len,
                                        double threshold,
                                        size_t *change_points, size_t max_points,
                                        size_t depth, uint64_t max_steps) {
    binary_string_t full;
    bs_init(&full);
    for (size_t i = 0; i < seq_len; i++) bs_append_bit(&full, sequence[i] ? 1 : 0);
    double K_full = solomonoff_algorithmic_info(&full, depth, max_steps);
    size_t found = 0;
    for (size_t k = 1; k < seq_len - 1 && found < max_points; k++) {
        binary_string_t left, right;
        bs_init(&left); bs_init(&right);
        for (size_t i = 0; i < k; i++) bs_append_bit(&left, sequence[i] ? 1 : 0);
        for (size_t i = k; i < seq_len; i++) bs_append_bit(&right, sequence[i] ? 1 : 0);
        double Kl = solomonoff_algorithmic_info(&left, depth, max_steps);
        double Kr = solomonoff_algorithmic_info(&right, depth, max_steps);
        if (Kl + Kr < K_full - threshold) change_points[found++] = k;
    }
    return found;
}

/* ================================================================
 * 6. Compression Ratio (L7 - Application)
 * ================================================================ */

double solomonoff_compression_ratio(const int *sequence, size_t seq_len,
                                      size_t depth, uint64_t max_steps) {
    binary_string_t x;
    bs_init(&x);
    for (size_t i = 0; i < seq_len; i++) bs_append_bit(&x, sequence[i] ? 1 : 0);
    double K_est = solomonoff_algorithmic_info(&x, depth, max_steps);
    if (seq_len == 0) return 1.0;
    return K_est / (double)seq_len;
}

/* ================================================================
 * 7. Randomness Testing (L7 - Application)
 * ================================================================ */

bool solomonoff_randomness_test(const int *sequence, size_t seq_len,
                                  double significance_level,
                                  size_t depth, uint64_t max_steps) {
    if (seq_len == 0) return true;
    binary_string_t x;
    bs_init(&x);
    for (size_t i = 0; i < seq_len; i++) bs_append_bit(&x, sequence[i] ? 1 : 0);
    int deficiency = kolmogorov_randomness_deficiency(&x, depth, max_steps);
    double ratio = (double)deficiency / (double)seq_len;
    return (ratio <= significance_level);
}

/* ================================================================
 * 8. Pattern Discovery (L7 - Application)
 * ================================================================ */

size_t solomonoff_discover_pattern(const int *sequence, size_t seq_len,
                                     binary_string_t *best_program,
                                     size_t depth, uint64_t max_steps) {
    binary_string_t x;
    bs_init(&x);
    for (size_t i = 0; i < seq_len; i++) bs_append_bit(&x, sequence[i] ? 1 : 0);
    return kolmogorov_K(&x, depth, max_steps, best_program);
}
