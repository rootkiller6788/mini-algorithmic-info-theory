/*
 * two_part.c — Two-Part Code implementation
 *
 * Explicit construction of two-part codes for MDL.
 * Each function computes L(M) + L(D|M) for a specific model class.
 *
 * Reference: Rissanen (1989), Barron & Cover (1991)
 *            Grünwald (2007) Ch. 5 "Crude MDL"
 */

#include "../include/two_part.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <float.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_LN2
#define M_LN2 0.69314718055994530942
#endif

/* ══════════════════════════════════════════════════════════════
   Two-Part Code Construction
   ══════════════════════════════════════════════════════════════ */

double twopart_encode_integer(size_t n) {
    /* Elias gamma code: 2⌊log₂ n⌋ + 1 bits */
    return mdl_elias_gamma_code(n);
}

double twopart_encode_real(double theta, double range_min, double range_max,
                            size_t n) {
    /*
     * Encode real parameter θ ∈ [range_min, range_max] at optimal
     * precision δ = 1/√n (Rissanen's recommendation).
     *
     * Note: theta not used in cost — the cost depends only on precision
     * and range, not the specific value. Theta is for API consistency.
     *
     * Bits = log₂((range_max - range_min) / δ)
     *      = log₂((range_max - range_min) * √n)
     *      = ½ log₂ n + log₂(range_max - range_min)
     */
    (void)theta;  /* cost depends on precision/range, not specific value */
    double range = range_max - range_min;
    if (range <= 0) range = 1.0;
    double bits = 0.5 * log2((double)n) + log2(range);
    if (bits < 1.0) bits = 1.0;
    return bits;
}

double twopart_encode_vector(int k, size_t n) {
    /* Encode k real parameters at precision 1/√n each */
    return 0.5 * (double)k * log2((double)n);
}

double twopart_code_linear_regression(const MDLData* x, const MDLData* y) {
    if (!x || !y || x->n != y->n || x->n < 2) return DBL_MAX;
    size_t n = x->n;
    int k = 2; /* β₀, β₁ */

    /* Part 1: encode β parameters */
    double model_cost = twopart_encode_vector(k, n);

    /* Part 2: encode residuals (same as Gaussian NLL for 1D regression) */
    double sum_x = 0.0, sum_y = 0.0, sum_xx = 0.0, sum_xy = 0.0;
    for (size_t i = 0; i < n; i++) {
        sum_x  += x->values[i];
        sum_y  += y->values[i];
        sum_xx += x->values[i] * x->values[i];
        sum_xy += x->values[i] * y->values[i];
    }
    double denom = (double)n * sum_xx - sum_x * sum_x;
    if (fabs(denom) < 1e-12) return DBL_MAX;
    double beta1 = ((double)n * sum_xy - sum_x * sum_y) / denom;
    double beta0 = (sum_y - beta1 * sum_x) / (double)n;

    double rss = 0.0;
    for (size_t i = 0; i < n; i++) {
        double pred = beta0 + beta1 * x->values[i];
        double err = y->values[i] - pred;
        rss += err * err;
    }
    double var = rss / (double)n;
    if (var < 1e-12) var = 1e-12;

    double data_cost = 0.5 * (double)n * (log2(2.0 * M_PI * var)
                       + 1.0 / M_LN2);

    return model_cost + data_cost;
}

double twopart_code_polynomial(const MDLData* x, const MDLData* y,
                                int degree) {
    if (!x || !y || x->n != y->n || degree < 0) return DBL_MAX;
    size_t n = x->n;

    /* Part 1: encode degree d + (d+1) coefficients */
    double model_cost = twopart_encode_integer((size_t)degree)
                      + twopart_encode_vector(degree + 1, n);

    /* Part 2: fit polynomial and encode residuals */
    MDLPolynomial* p = mdl_poly_fit(x, y, degree);
    if (!p || p->mse >= DBL_MAX) {
        mdl_poly_free(p);
        return DBL_MAX;
    }

    double data_cost = 0.5 * (double)n * (log2(2.0 * M_PI * p->mse)
                       + 1.0 / M_LN2);
    mdl_poly_free(p);

    return model_cost + data_cost;
}

double twopart_code_histogram(const MDLData* data, int n_bins) {
    if (!data || n_bins < 1) return DBL_MAX;

    /* Build histogram */
    MDLHistogram* h = mdl_histogram_create(data, n_bins);
    if (!h) return DBL_MAX;

    int total = 0;
    for (int i = 0; i < n_bins; i++) total += h->counts[i];

    /* Part 1: encode k + k-1 bin edges */
    double model_cost = twopart_encode_integer((size_t)n_bins)
                      + twopart_encode_vector(n_bins - 1, (size_t)total);

    /* Part 2: multinomial likelihood */
    double data_cost = 0.0;
    for (int i = 0; i < n_bins; i++) {
        if (h->counts[i] > 0) {
            double p = (double)h->counts[i] / (double)total;
            data_cost -= (double)h->counts[i] * log2(p);
        }
    }

    mdl_histogram_free(h);
    return model_cost + data_cost;
}

double twopart_code_piecewise_constant(const MDLData* data,
                                        const int* change_points,
                                        int n_changes) {
    if (!data || !change_points || n_changes < 0) return DBL_MAX;
    size_t n = data->n;

    /* Part 1: encode change point positions + segment means */
    double model_cost = twopart_encode_integer((size_t)n_changes);
    /* Each change point: log₂ n bits to specify position */
    model_cost += (double)n_changes * log2((double)n);
    /* Each segment: one mean (½ log₂ n) + one variance (½ log₂ n) */
    model_cost += (double)(n_changes + 1) * log2((double)n);

    /* Part 2: encode data within each segment */
    double data_cost = 0.0;
    size_t seg_start = 0;
    for (int s = 0; s <= n_changes; s++) {
        size_t seg_end = (s < n_changes) ? (size_t)change_points[s] : n;
        if (seg_end <= seg_start) { seg_start = seg_end; continue; }

        double sum = 0.0, sum_sq = 0.0;
        size_t seg_len = seg_end - seg_start;
        for (size_t i = seg_start; i < seg_end; i++) {
            sum += data->values[i];
            sum_sq += data->values[i] * data->values[i];
        }
        double mean = sum / (double)seg_len;
        double var = sum_sq / (double)seg_len - mean * mean;
        if (var < 1e-12) var = 1e-12;

        /* Gaussian NLL for segment */
        data_cost += 0.5 * (double)seg_len * (log2(2.0 * M_PI * var)
                     + 1.0 / M_LN2);
        seg_start = seg_end;
    }

    return model_cost + data_cost;
}

double twopart_code_gaussian_mixture(const MDLData* data, int k) {
    /*
     * Simplified two-part code for Gaussian mixture with k components.
     * Uses 1D data for simplicity.
     */
    if (!data || k < 1 || data->n == 0) return DBL_MAX;
    size_t n = data->n;

    /* Part 1: encode k-1 mixture weights + k means + k variances */
    double model_cost = twopart_encode_integer((size_t)k)
                      + twopart_encode_vector(k - 1, n)    /* weights */
                      + twopart_encode_vector(k, n)        /* means */
                      + twopart_encode_vector(k, n);       /* variances */

    /* Part 2: use k-means assignment as proxy for mixture */
    /* Fit k clusters using simple 1D k-means */
    double* centroids = (double*)malloc((size_t)k * sizeof(double));
    int* assignments = (int*)malloc(n * sizeof(int));
    assert(centroids && assignments);

    /* Initialize centroids at quantiles */
    MDLData* sorted = mdl_data_clone(data);
    mdl_data_sort(sorted);
    for (int i = 0; i < k; i++) {
        size_t idx = (size_t)i * n / (size_t)k;
        centroids[i] = sorted->values[idx];
    }
    mdl_data_free(sorted);

    /* 10 iterations of k-means */
    for (int iter = 0; iter < 10; iter++) {
        /* Assign */
        for (size_t i = 0; i < n; i++) {
            double min_d = DBL_MAX;
            int best = 0;
            for (int c = 0; c < k; c++) {
                double d = fabs(data->values[i] - centroids[c]);
                if (d < min_d) { min_d = d; best = c; }
            }
            assignments[i] = best;
        }
        /* Update centroids */
        double* sums = (double*)calloc((size_t)k, sizeof(double));
        double* counts = (double*)calloc((size_t)k, sizeof(double));
        assert(sums && counts);
        for (size_t i = 0; i < n; i++) {
            int c = assignments[i];
            sums[c] += data->values[i];
            counts[c] += 1.0;
        }
        for (int c = 0; c < k; c++) {
            if (counts[c] > 0) centroids[c] = sums[c] / counts[c];
        }
        free(sums);
        free(counts);
    }

    /* Compute component-wise Gaussian NLL */
    double* comp_vars = (double*)calloc((size_t)k, sizeof(double));
    double* comp_counts = (double*)calloc((size_t)k, sizeof(double));
    assert(comp_vars && comp_counts);
    for (size_t i = 0; i < n; i++) {
        int c = assignments[i];
        double diff = data->values[i] - centroids[c];
        comp_vars[c] += diff * diff;
        comp_counts[c] += 1.0;
    }

    double data_cost = 0.0;
    for (int c = 0; c < k; c++) {
        if (comp_counts[c] > 0) {
            double v = comp_vars[c] / comp_counts[c];
            if (v < 1e-12) v = 1e-12;
            size_t nc = (size_t)comp_counts[c];
            data_cost += 0.5 * (double)nc * (log2(2.0 * M_PI * v)
                         + 1.0 / M_LN2);
        }
    }

    free(centroids);
    free(assignments);
    free(comp_vars);
    free(comp_counts);

    return model_cost + data_cost;
}

/* ══════════════════════════════════════════════════════════════
   Two-Part Code Selection
   ══════════════════════════════════════════════════════════════ */

int twopart_select_model(const double* costs, int n_models) {
    if (!costs || n_models <= 0) return -1;
    int best = 0;
    double best_cost = costs[0];
    for (int i = 1; i < n_models; i++) {
        if (costs[i] < best_cost) {
            best_cost = costs[i];
            best = i;
        }
    }
    return best;
}

double twopart_compare_models(double cost1, double cost2) {
    /* Negative means model1 is preferred */
    return cost1 - cost2;
}

int twopart_optimal_polynomial_degree(const MDLData* x, const MDLData* y,
                                       int max_degree) {
    if (!x || !y || max_degree < 0) return 0;
    int best = 0;
    double best_cost = DBL_MAX;

    for (int d = 0; d <= max_degree; d++) {
        double cost = twopart_code_polynomial(x, y, d);
        if (cost < best_cost) {
            best_cost = cost;
            best = d;
        }
    }
    return best;
}

int twopart_optimal_histogram_bins(const MDLData* data, int max_bins) {
    if (!data || max_bins < 1) return 1;
    int best = 1;
    double best_cost = DBL_MAX;

    for (int k = 1; k <= max_bins; k++) {
        double cost = twopart_code_histogram(data, k);
        if (cost < best_cost) {
            best_cost = cost;
            best = k;
        }
    }
    return best;
}

int twopart_optimal_mixture_components(const MDLData* data, int max_k) {
    if (!data || max_k < 1) return 1;
    int best = 1;
    double best_cost = DBL_MAX;

    for (int k = 1; k <= max_k; k++) {
        double cost = twopart_code_gaussian_mixture(data, k);
        if (cost < best_cost) {
            best_cost = cost;
            best = k;
        }
    }
    return best;
}
