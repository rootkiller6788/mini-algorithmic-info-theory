/*
 * mdl_core.c — Core MDL implementation
 *
 * Implements: universal integer codes, two-part codes for exponential
 * families, NML codes, data management, model selection, polynomial
 * fitting, histogram model, change-point detection, clustering, AR models.
 *
 * Reference: Rissanen (1978, 1984, 1996, 2012)
 *            Grünwald (2007) "The Minimum Description Length Principle"
 *            Barron, Rissanen, Yu (1998) "The MDL Principle in Modeling"
 *            Hansen & Yu (2001) "MDL Model Selection"
 */

#include "../include/mdl_core.h"
#include <math.h>
#include <string.h>
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
   MDL Data API
   ══════════════════════════════════════════════════════════════ */

MDLData* mdl_data_create(size_t cap) {
    MDLData* d = (MDLData*)malloc(sizeof(MDLData));
    assert(d != NULL);
    if (cap == 0) cap = 64;
    d->values = (double*)malloc(cap * sizeof(double));
    assert(d->values != NULL);
    d->n = 0;
    d->cap = cap;
    return d;
}

MDLData* mdl_data_from_array(const double* arr, size_t n) {
    MDLData* d = mdl_data_create(n + 1);
    for (size_t i = 0; i < n; i++) {
        d->values[i] = arr[i];
    }
    d->n = n;
    return d;
}

MDLData* mdl_data_from_file(const char* filename) {
    FILE* f = fopen(filename, "r");
    if (!f) return NULL;
    MDLData* d = mdl_data_create(128);
    double v;
    while (fscanf(f, "%lf", &v) == 1) {
        mdl_data_append(d, v);
    }
    fclose(f);
    return d;
}

MDLData* mdl_data_clone(const MDLData* src) {
    if (!src) return NULL;
    MDLData* d = mdl_data_create(src->n + 1);
    for (size_t i = 0; i < src->n; i++)
        d->values[i] = src->values[i];
    d->n = src->n;
    return d;
}

void mdl_data_append(MDLData* d, double v) {
    if (d->n >= d->cap) {
        d->cap = d->cap * 2 + 1;
        d->values = (double*)realloc(d->values, d->cap * sizeof(double));
        assert(d->values != NULL);
    }
    d->values[d->n++] = v;
}

double mdl_data_get(const MDLData* d, size_t i) {
    assert(d && i < d->n);
    return d->values[i];
}

void mdl_data_set(MDLData* d, size_t i, double v) {
    assert(d && i < d->n);
    d->values[i] = v;
}

double mdl_data_mean(const MDLData* d) {
    if (!d || d->n == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < d->n; i++) sum += d->values[i];
    return sum / (double)d->n;
}

double mdl_data_variance(const MDLData* d, double mean) {
    if (!d || d->n <= 1) return 0.0;
    double sum_sq = 0.0;
    for (size_t i = 0; i < d->n; i++) {
        double diff = d->values[i] - mean;
        sum_sq += diff * diff;
    }
    return sum_sq / (double)(d->n); /* MLE variance */
}

double mdl_data_min(const MDLData* d) {
    if (!d || d->n == 0) return 0.0;
    double m = d->values[0];
    for (size_t i = 1; i < d->n; i++)
        if (d->values[i] < m) m = d->values[i];
    return m;
}

double mdl_data_max(const MDLData* d) {
    if (!d || d->n == 0) return 0.0;
    double m = d->values[0];
    for (size_t i = 1; i < d->n; i++)
        if (d->values[i] > m) m = d->values[i];
    return m;
}

static int cmp_double(const void* a, const void* b) {
    double diff = *(const double*)a - *(const double*)b;
    if (diff < 0) return -1;
    if (diff > 0) return 1;
    return 0;
}

void mdl_data_sort(MDLData* d) {
    if (!d || d->n <= 1) return;
    qsort(d->values, d->n, sizeof(double), cmp_double);
}

void mdl_data_print(const MDLData* d, size_t max_show) {
    if (!d) { printf("(null)\n"); return; }
    printf("MDLData[%zu]:", d->n);
    size_t show = d->n < max_show ? d->n : max_show;
    for (size_t i = 0; i < show; i++)
        printf(" %.4f", d->values[i]);
    if (d->n > max_show) printf(" ...");
    printf("\n");
}

void mdl_data_free(MDLData* d) {
    if (d) {
        free(d->values);
        free(d);
    }
}

/* ══════════════════════════════════════════════════════════════
   Universal Integer Codes (Elias, Rissanen)
   ══════════════════════════════════════════════════════════════ */

double mdl_elias_gamma_code(size_t n) {
    /* Elias γ: 2⌊log₂ n⌋ + 1 bits for n ≥ 1 */
    if (n == 0) return 1.0;
    size_t nn = n;
    int log_n = 0;
    while (nn > 1) { nn >>= 1; log_n++; }
    return 2.0 * (double)log_n + 1.0;
}

double mdl_elias_delta_code(size_t n) {
    /* Elias δ: ⌊log₂ n⌋ + 1 + 2⌊log₂(⌊log₂ n⌋+1)⌋ bits */
    if (n <= 1) return 2.0;
    size_t nn = n;
    int L = 0;
    while (nn > 1) { nn >>= 1; L++; }
    int LL = 0;
    int Lp = L + 1;
    while (Lp > 1) { Lp >>= 1; LL++; }
    return (double)L + 1.0 + 2.0 * (double)LL;
}

double mdl_elias_omega_code(size_t n) {
    /* Elias ω: recursive encoding of length */
    if (n <= 1) return 2.0;
    double bits = 1.0; /* terminating 0 */
    size_t k = n;
    while (k > 1) {
        size_t kk = k;
        int len = 0;
        while (kk > 0) { kk >>= 1; len++; }
        bits += (double)len;
        k = (size_t)(len - 1);
    }
    return bits;
}

double mdl_rissanen_logstar_code(size_t n) {
    /* Rissanen's log* code: iterative logarithm
     * log* n = log₂ n + log₂ log₂ n + ... (sum of positive terms)
     * Code length: log* n + c where c ≈ log₂(2.865)
     */
    if (n <= 1) return 2.0;
    double logstar = 0.0;
    double x = (double)n;
    while (x > 1.0) {
        double lx = log2(x);
        logstar += lx;
        x = lx;
    }
    /* Add universal constant ≈ log₂(2.865064...) */
    return logstar + 1.5185;
}

double mdl_universal_integer_bits(size_t n) {
    /* Use Rissanen's log* code as the default universal integer code */
    return mdl_rissanen_logstar_code(n);
}

/* ══════════════════════════════════════════════════════════════
   Two-Part Code Implementations
   ══════════════════════════════════════════════════════════════ */

double mdl_two_part_gaussian(const MDLData* data) {
    /*
     * Two-part code for Gaussian N(μ, σ²):
     * L(μ, σ) = ½log n + ½log n (encode two real parameters at precision 1/√n)
     * L(x|μ,σ) = (n/2)log(2πσ²) + (1/(2σ²)) Σ(x_i-μ)²
     *
     * All in nats, converted to bits by dividing by ln 2.
     */
    if (!data || data->n < 2) return 0.0;
    size_t n = data->n;
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;

    /* Model cost: 2 parameters × ½ log₂ n bits each */
    double model_cost = log2((double)n);

    /* Data cost: negative log-likelihood in bits */
    double data_cost = 0.5 * (double)n * (log2(2.0 * M_PI * var) + 1.0 / M_LN2);

    return model_cost + data_cost;
}

double mdl_two_part_bernoulli(const MDLData* data) {
    /*
     * Two-part for Bernoulli(p):
     * L(p) = ½ log₂ n  (one probability parameter)
     * L(x|p) = -k log₂ p - (n-k) log₂ (1-p)
     * where k = Σ x_i (x_i ∈ {0,1} assumed)
     */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double k = 0.0;
    for (size_t i = 0; i < n; i++) k += data->values[i];
    double p = k / (double)n;
    if (p < 1e-12) p = 1e-12;
    if (p > 1.0 - 1e-12) p = 1.0 - 1e-12;

    double model_cost = 0.5 * log2((double)n);
    double data_cost = -k * log2(p) - ((double)n - k) * log2(1.0 - p);

    return model_cost + data_cost;
}

double mdl_two_part_poisson(const MDLData* data) {
    /*
     * Two-part for Poisson(λ):
     * L(λ) = ½ log₂ n  (one parameter, non-negative real)
     * L(x|λ) = -Σ [x_i log λ - λ - log(x_i!)]
     * Using Stirling's approx for log factorial.
     */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double sum = 0.0;
    double sum_log_fact = 0.0;
    for (size_t i = 0; i < n; i++) {
        double x = data->values[i];
        if (x < 0) x = 0;
        sum += x;
        /* Stirling: log(x!) ≈ x log x - x + ½log(2πx) */
        if (x > 0) {
            sum_log_fact += x * log(x) - x + 0.5 * log(2.0 * M_PI * x);
        }
    }
    double lambda = sum / (double)n;
    if (lambda < 1e-12) lambda = 1e-12;

    double model_cost = 0.5 * log2((double)n);
    double nll = -sum * log(lambda) + (double)n * lambda + sum_log_fact;
    double data_cost = nll / M_LN2;

    return model_cost + data_cost;
}

double mdl_two_part_multinomial(const MDLData* data, int m) {
    /*
     * Two-part for Multinomial(m, p₁,...,p_m):
     * L(p) = ((m-1)/2) log₂ n  (m-1 free probability parameters)
     * L(x|p) = -Σ_j n_j log p_j
     */
    if (!data || data->n == 0 || m <= 1) return 0.0;
    size_t n = data->n;
    double* counts = (double*)calloc((size_t)m, sizeof(double));
    assert(counts != NULL);

    for (size_t i = 0; i < n; i++) {
        int cat = (int)data->values[i];
        if (cat >= 0 && cat < m) counts[cat] += 1.0;
        else if (cat >= m) counts[m - 1] += 1.0;
    }

    double model_cost = 0.5 * (double)(m - 1) * log2((double)n);
    double data_cost = 0.0;
    for (int j = 0; j < m; j++) {
        double p_hat = counts[j] / (double)n;
        if (p_hat > 1e-12) {
            data_cost -= counts[j] * log2(p_hat);
        }
    }
    free(counts);
    return model_cost + data_cost;
}

double mdl_two_part_exponential(const MDLData* data) {
    /*
     * Two-part for Exponential(λ):
     * L(λ) = ½ log₂ n
     * L(x|λ) = -n log(λ) + λ Σ x_i
     */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        if (data->values[i] < 0) return DBL_MAX;
        sum += data->values[i];
    }
    double lambda = (double)n / sum;
    if (lambda < 1e-12) lambda = 1e-12;

    double model_cost = 0.5 * log2((double)n);
    double nll = -(double)n * log(lambda) + lambda * sum;
    double data_cost = nll / M_LN2;

    return model_cost + data_cost;
}

double mdl_two_part_gamma(const MDLData* data) {
    /*
     * Two-part for Gamma(α, β):
     * L(α, β) = log₂ n  (two parameters)
     * L(x|α,β) = -nα log β + n log Γ(α) - (α-1) Σ log x_i + β Σ x_i
     *
     * Using method-of-moments for α, β (simplified MLE).
     */
    if (!data || data->n < 2) return 0.0;
    size_t n = data->n;
    double sum = 0.0, sum_log = 0.0;
    for (size_t i = 0; i < n; i++) {
        double x = data->values[i];
        if (x <= 0) x = 1e-12;
        sum += x;
        sum_log += log(x);
    }
    double mean = sum / (double)n;
    double var = mdl_data_variance(data, mean);
    if (var < 1e-12) var = 1e-12;

    /* Method of moments: α = mean²/var, β = mean/var */
    double alpha = mean * mean / var;
    double beta = mean / var;
    if (alpha < 0.1) alpha = 0.1;
    if (beta < 1e-12) beta = 1e-12;

    double model_cost = log2((double)n);

    /* log Γ(α) using Stirling: log Γ(a) ≈ (a-½)log a - a + ½log(2π) */
    double log_gamma_alpha = (alpha - 0.5) * log(alpha) - alpha
                           + 0.5 * log(2.0 * M_PI);
    double nll = -(double)n * alpha * log(beta) + (double)n * log_gamma_alpha
                 - (alpha - 1.0) * sum_log + beta * sum;
    double data_cost = nll / M_LN2;

    return model_cost + data_cost;
}

/* ══════════════════════════════════════════════════════════════
   NML (Normalized Maximum Likelihood) Codes
   ══════════════════════════════════════════════════════════════ */

double mdl_nml_parametric_complexity(int n, int k, double fisher_info_det) {
    /*
     * Parametric complexity for k-parameter exponential family:
     * log C_n ≈ (k/2) log(n/(2π)) + log ∫_Θ √det I(θ) dθ
     *
     * For many standard models, the integral is known analytically.
     * Returns: approximate log C_n / ln 2 (in bits)
     */
    if (n <= 0 || k <= 0) return 0.0;
    double log_c = 0.5 * (double)k * log((double)n / (2.0 * M_PI));
    if (fisher_info_det > 0) {
        log_c += log(sqrt(fisher_info_det));
    }
    return log_c / M_LN2;
}

double mdl_nml_gaussian(const MDLData* data) {
    /*
     * NML code for Gaussian N(μ, σ²) model class.
     *
     * P_nml(x) = P(x|μ̂, σ̂²) / C_n
     * where C_n = Σ_y P(y|μ̂(y), σ̂²(y))
     *
     * For Gaussian: log C_n ≈ log n + (1/2)log(2/π) + log Γ((n-1)/2)
     *
     * -log P_nml(x) = -log P(x|μ̂, σ̂²) + log C_n
     */
    if (!data || data->n < 2) return 0.0;
    size_t n = data->n;
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;

    /* NLL: (n/2)log(2πσ²) + n/2 (since Σ(x_i-μ̂)²/σ̂² = n) */
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0);

    /* Parametric complexity for Gaussian (k=2) */
    /* log C_n ≈ log n + ½log(2/π) + log Γ((n-1)/2) + constant */
    double parametric_complexity = log((double)n)
        + 0.5 * log(2.0 / M_PI);
    /* log Γ((n-1)/2) via Stirling */
    double arg = ((double)n - 1.0) / 2.0;
    if (arg > 0) {
        parametric_complexity += (arg - 0.5) * log(arg) - arg
                               + 0.5 * log(2.0 * M_PI);
    }

    return (nll + parametric_complexity) / M_LN2;
}

double mdl_nml_bernoulli(const MDLData* data) {
    /*
     * NML code for Bernoulli(p) model class.
     * C_n = Σ_{k=0}^n (n choose k) (k/n)^k ((n-k)/n)^{n-k}
     *
     * Use known approximation:
     * log C_n ≈ ½ log(πn/2)  (Rissanen 1996)
     */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double k = 0.0;
    for (size_t i = 0; i < n; i++) k += data->values[i];
    double p_hat = k / (double)n;
    if (p_hat < 1e-12) p_hat = 1e-12;
    if (p_hat > 1.0 - 1e-12) p_hat = 1.0 - 1e-12;

    double nll = -k * log(p_hat) - ((double)n - k) * log(1.0 - p_hat);
    double parametric_complexity = 0.5 * log(M_PI * (double)n / 2.0);

    return (nll + parametric_complexity) / M_LN2;
}

double mdl_nml_multinomial(const MDLData* data, int m) {
    /*
     * NML for Multinomial(m, p₁...p_m):
     * C_n = Σ_{n₁+...+n_m=n} (n! / Π n_j!) Π (n_j/n)^{n_j}
     *
     * Approx: log C_n ≈ ((m-1)/2) log(n/(2π)) + log(π^{m/2} / Γ(m/2))
     */
    if (!data || data->n == 0 || m <= 1) return 0.0;
    size_t n = data->n;
    double* counts = (double*)calloc((size_t)m, sizeof(double));
    assert(counts != NULL);
    for (size_t i = 0; i < n; i++) {
        int cat = (int)data->values[i];
        if (cat >= 0 && cat < m) counts[cat] += 1.0;
        else if (cat >= m) counts[m - 1] += 1.0;
    }

    double nll = 0.0;
    for (int j = 0; j < m; j++) {
        double pj = counts[j] / (double)n;
        if (pj > 1e-12) nll -= counts[j] * log(pj);
    }

    double pc = 0.5 * (double)(m - 1) * log((double)n / (2.0 * M_PI));
    free(counts);
    return (nll + pc) / M_LN2;
}

double mdl_nml_poisson(const MDLData* data) {
    /*
     * NML for Poisson(λ):
     * C_n ≈ Σ_{t} P(t|λ̂(t)) where sum over sufficient statistics
     * For Poisson: log C_n ≈ ½ log n
     */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double sum = 0.0;
    double sum_log_fact = 0.0;
    for (size_t i = 0; i < n; i++) {
        double x = data->values[i];
        if (x < 0) x = 0;
        sum += x;
        if (x > 0) {
            sum_log_fact += x * log(x) - x + 0.5 * log(2.0 * M_PI * x);
        }
    }
    double lambda = sum / (double)n;
    if (lambda < 1e-12) lambda = 1e-12;

    double nll = -sum * log(lambda) + (double)n * lambda + sum_log_fact;
    double pc = 0.5 * log((double)n);

    return (nll + pc) / M_LN2;
}

double mdl_nml_linear_regression(const MDLData* x, const MDLData* y) {
    /*
     * NML for Linear Regression: y = β₀ + β₁x + ε, ε ~ N(0, σ²)
     *
     * k = 3 parameters (β₀, β₁, σ²)
     * log C_n ≈ (3/2) log n + constant
     */
    if (!x || !y || x->n != y->n || x->n < 3) return 0.0;
    size_t n = x->n;

    /* Least squares fit: y = β₀ + β₁ x */
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

    /* Residual variance */
    double rss = 0.0;
    for (size_t i = 0; i < n; i++) {
        double y_pred = beta0 + beta1 * x->values[i];
        double res = y->values[i] - y_pred;
        rss += res * res;
    }
    double var = rss / (double)n;
    if (var < 1e-12) var = 1e-12;

    /* NLL for Gaussian */
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0);

    /* Parametric complexity for k=3 */
    double pc = 1.5 * log((double)n / (2.0 * M_PI));

    return (nll + pc) / M_LN2;
}

/* ══════════════════════════════════════════════════════════════
   Redundancy and Regret (L4)
   ══════════════════════════════════════════════════════════════ */

double mdl_redundancy_bound(int n, int k) {
    /*
     * Expected redundancy of MDL for k-parameter model:
     * E[ -log P_mdl(X) ] - n·H(θ) ≈ (k/2) log(n/(2π)) + log ∫ √det I(θ) dθ
     *
     * Returns bits.
     */
    if (n <= 0 || k <= 0) return 0.0;
    return 0.5 * (double)k * log2((double)n / (2.0 * M_PI));
}

double mdl_worst_case_regret(int n, int k) {
    /*
     * Worst-case regret: maximum difference between MDL and
     * the best model in hindsight.
     *
     * For k-parameter exponential family:
     *   R_n ≈ (k/2) log n + O(1)
     */
    if (n <= 0 || k <= 0) return 0.0;
    return 0.5 * (double)k * log2((double)n) + 1.0;
}

int mdl_consistency_test(const MDLData* data, int true_k, double tolerance) {
    /*
     * Check if MDL selects the right model.
     * For data generated from k-true parameters, MDL should select
     * a model with complexity k as n → ∞.
     *
     * Heuristic: compute DL for k-1, k, k+1 and see if k is best.
     */
    if (!data || data->n < 10) return -1;
    size_t n = data->n;

    /* Estimate DL using Gaussian MDL for each k */
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;

    /* NLL is the same regardless of k for a given model */
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0) / M_LN2;

    /* Compare costs with different parametric complexity */
    double cost_km1 = nll + mdl_redundancy_bound((int)n, true_k - 1);
    double cost_k   = nll + mdl_redundancy_bound((int)n, true_k);
    double cost_kp1 = nll + mdl_redundancy_bound((int)n, true_k + 1);

    if (cost_k < cost_km1 - tolerance && cost_k < cost_kp1 - tolerance)
        return 1; /* Correctly selects true_k */
    return 0;
}

/* ══════════════════════════════════════════════════════════════
   Model Selection
   ══════════════════════════════════════════════════════════════ */

MDLResult* mdl_select_create(int n_models) {
    MDLResult* r = (MDLResult*)malloc(sizeof(MDLResult));
    assert(r != NULL);
    r->n_models = n_models;
    r->costs = (double*)calloc((size_t)n_models, sizeof(double));
    assert(r->costs != NULL);
    r->best_model_index = 0;
    r->best_cost = DBL_MAX;
    r->selected_complexity = 0;
    return r;
}

void mdl_select_set_cost(MDLResult* r, int idx, double cost) {
    assert(r && idx >= 0 && idx < r->n_models);
    r->costs[idx] = cost;
    if (cost < r->best_cost) {
        r->best_cost = cost;
        r->best_model_index = idx;
    }
}

int mdl_select_best(MDLResult* r) {
    return r ? r->best_model_index : -1;
}

void mdl_select_print(const MDLResult* r) {
    if (!r) return;
    printf("MDL Model Selection (n=%d models):\n", r->n_models);
    for (int i = 0; i < r->n_models; i++) {
        printf("  Model %d: DL = %.4f bits%s\n",
               i, r->costs[i],
               (i == r->best_model_index) ? " ★ BEST" : "");
    }
    printf("Selected: Model %d (cost=%.4f bits)\n",
           r->best_model_index, r->best_cost);
}

void mdl_select_free(MDLResult* r) {
    if (r) {
        free(r->costs);
        free(r);
    }
}

/* ══════════════════════════════════════════════════════════════
   Polynomial MDL
   ══════════════════════════════════════════════════════════════ */

/*
 * Gaussian elimination for solving normal equations (A'A)c = A'y
 * where A is the Vandermonde matrix.
 */
static int solve_vandermonde(const double* x, const double* y, size_t n,
                              int degree, double* coeffs) {
    int d = degree + 1; /* number of coefficients */
    if ((size_t)d > n) return 0;

    /* Build normal equations matrix M = A'A, vector b = A'y */
    double* M = (double*)calloc((size_t)(d * d), sizeof(double));
    double* bvec = (double*)calloc((size_t)d, sizeof(double));
    assert(M && bvec);

    for (size_t i = 0; i < n; i++) {
        double xpow = 1.0;
        double* row = (double*)malloc((size_t)d * sizeof(double));
        assert(row);
        for (int j = 0; j < d; j++) {
            row[j] = xpow;
            xpow *= x[i];
        }
        for (int j = 0; j < d; j++) {
            for (int k = j; k < d; k++) {
                M[j * d + k] += row[j] * row[k];
            }
            bvec[j] += row[j] * y[i];
        }
        free(row);
    }
    /* Fill symmetric */
    for (int j = 0; j < d; j++)
        for (int k = 0; k < j; k++)
            M[j * d + k] = M[k * d + j];

    /* Gaussian elimination with partial pivoting */
    for (int col = 0; col < d; col++) {
        /* Find pivot */
        int pivot = col;
        double max_val = fabs(M[col * d + col]);
        for (int row = col + 1; row < d; row++) {
            if (fabs(M[row * d + col]) > max_val) {
                max_val = fabs(M[row * d + col]);
                pivot = row;
            }
        }
        if (max_val < 1e-12) { free(M); free(bvec); return 0; }

        /* Swap rows */
        if (pivot != col) {
            for (int j = 0; j < d; j++) {
                double tmp = M[col * d + j];
                M[col * d + j] = M[pivot * d + j];
                M[pivot * d + j] = tmp;
            }
            double tmp = bvec[col];
            bvec[col] = bvec[pivot];
            bvec[pivot] = tmp;
        }

        /* Eliminate below */
        for (int row = col + 1; row < d; row++) {
            double factor = M[row * d + col] / M[col * d + col];
            for (int j = col; j < d; j++)
                M[row * d + j] -= factor * M[col * d + j];
            bvec[row] -= factor * bvec[col];
        }
    }

    /* Back substitution */
    for (int i = d - 1; i >= 0; i--) {
        double sum = bvec[i];
        for (int j = i + 1; j < d; j++)
            sum -= M[i * d + j] * coeffs[j];
        coeffs[i] = sum / M[i * d + i];
    }

    free(M);
    free(bvec);
    return 1;
}

MDLPolynomial* mdl_poly_fit(const MDLData* x, const MDLData* y, int degree) {
    if (!x || !y || x->n != y->n || degree < 0) return NULL;
    size_t n = x->n;

    MDLPolynomial* p = (MDLPolynomial*)malloc(sizeof(MDLPolynomial));
    assert(p != NULL);
    p->degree = degree;
    p->coeffs = (double*)calloc((size_t)(degree + 1), sizeof(double));
    assert(p->coeffs != NULL);

    if (solve_vandermonde(x->values, y->values, n, degree, p->coeffs)) {
        /* Compute MSE */
        double rss = 0.0;
        for (size_t i = 0; i < n; i++) {
            double pred = mdl_poly_evaluate(p, x->values[i]);
            double err = y->values[i] - pred;
            rss += err * err;
        }
        p->mse = rss / (double)n;
        if (p->mse < 1e-12) p->mse = 1e-12;

        /* MDL cost: L(coeffs) + L(data|coeffs) */
        /* L(coeffs): (degree+1)/2 * log₂ n bits per parameter */
        double model_cost = 0.5 * (double)(degree + 1) * log2((double)n);
        /* L(data|coeffs): -log likelihood of Gaussian noise */
        double data_cost = 0.5 * (double)n * (log2(2.0 * M_PI * p->mse)
                          + 1.0 / M_LN2);
        p->two_part_cost = model_cost + data_cost;
    } else {
        p->mse = DBL_MAX;
        p->two_part_cost = DBL_MAX;
    }

    return p;
}

int mdl_poly_optimal_degree(const MDLData* x, const MDLData* y,
                             int max_degree) {
    if (!x || !y || max_degree < 0) return 0;
    int best_deg = 0;
    double best_cost = DBL_MAX;

    for (int deg = 0; deg <= max_degree; deg++) {
        MDLPolynomial* p = mdl_poly_fit(x, y, deg);
        if (p && p->two_part_cost < best_cost) {
            best_cost = p->two_part_cost;
            best_deg = deg;
        }
        mdl_poly_free(p);
    }
    return best_deg;
}

double mdl_poly_evaluate(const MDLPolynomial* p, double x) {
    if (!p) return 0.0;
    /* Horner's method */
    double result = p->coeffs[p->degree];
    for (int i = p->degree - 1; i >= 0; i--)
        result = result * x + p->coeffs[i];
    return result;
}

void mdl_poly_print(const MDLPolynomial* p) {
    if (!p) { printf("(null)\n"); return; }
    printf("MDLPolynomial(degree=%d, MSE=%.6f, DL=%.4f bits):\n",
           p->degree, p->mse, p->two_part_cost);
    printf("  y = ");
    for (int i = p->degree; i >= 0; i--) {
        if (fabs(p->coeffs[i]) < 1e-10) continue;
        if (i < p->degree) printf(" + ");
        printf("%.6f", p->coeffs[i]);
        if (i > 0) printf("·x");
        if (i > 1) printf("^%d", i);
    }
    printf("\n");
}

void mdl_poly_free(MDLPolynomial* p) {
    if (p) {
        free(p->coeffs);
        free(p);
    }
}

/* ══════════════════════════════════════════════════════════════
   Histogram MDL
   ══════════════════════════════════════════════════════════════ */

MDLHistogram* mdl_histogram_create(const MDLData* data, int n_bins) {
    if (!data || data->n == 0 || n_bins < 1) return NULL;

    MDLHistogram* h = (MDLHistogram*)malloc(sizeof(MDLHistogram));
    assert(h != NULL);
    h->n_bins = n_bins;
    h->counts = (int*)calloc((size_t)n_bins, sizeof(int));
    h->edges  = (double*)calloc((size_t)(n_bins + 1), sizeof(double));
    assert(h->counts && h->edges);

    double min_v = mdl_data_min(data);
    double max_v = mdl_data_max(data);
    double range = max_v - min_v;
    if (range < 1e-12) range = 1.0;
    h->bin_width = range / (double)n_bins;

    for (int i = 0; i <= n_bins; i++)
        h->edges[i] = min_v + (double)i * h->bin_width;

    for (size_t i = 0; i < data->n; i++) {
        double v = data->values[i];
        int bin = (int)((v - min_v) / h->bin_width);
        if (bin < 0) bin = 0;
        if (bin >= n_bins) bin = n_bins - 1;
        h->counts[bin]++;
    }

    h->mdl_cost = mdl_histogram_cost(h);
    return h;
}

int mdl_histogram_optimal_bins(const MDLData* data, int max_bins) {
    if (!data || max_bins < 1) return 1;
    int best_bins = 1;
    double best_cost = DBL_MAX;

    for (int k = 1; k <= max_bins; k++) {
        MDLHistogram* h = mdl_histogram_create(data, k);
        if (h && h->mdl_cost < best_cost) {
            best_cost = h->mdl_cost;
            best_bins = k;
        }
        mdl_histogram_free(h);
    }
    return best_bins;
}

double mdl_histogram_cost(const MDLHistogram* h) {
    /*
     * MDL cost for histogram:
     * L(k) + L(bin edges | k) + L(counts | edges)
     *
     * L(k): universal code for integer k (number of bins)
     * L(edges): k+1 real numbers at precision 1/√n
     * L(counts): -log multinomial likelihood
     */
    if (!h) return 0.0;

    int total = 0;
    for (int i = 0; i < h->n_bins; i++) total += h->counts[i];
    if (total == 0) return 0.0;
    size_t n = (size_t)total;

    /* Model cost: encode k (number of bins) + bin edges */
    double model_cost = mdl_universal_integer_bits((size_t)h->n_bins);
    /* Encode (k-1) internal edges as real parameters */
    model_cost += 0.5 * (double)(h->n_bins - 1) * log2((double)n);

    /* Data cost: -log likelihood */
    double data_cost = 0.0;
    for (int i = 0; i < h->n_bins; i++) {
        if (h->counts[i] > 0) {
            double p = (double)h->counts[i] / (double)total;
            data_cost -= (double)h->counts[i] * log2(p);
        }
    }

    return model_cost + data_cost;
}

void mdl_histogram_print(const MDLHistogram* h) {
    if (!h) return;
    printf("MDLHistogram(k=%d, DL=%.4f bits):\n", h->n_bins, h->mdl_cost);
    for (int i = 0; i < h->n_bins; i++) {
        printf("  [%.4f, %.4f): %d\n",
               h->edges[i], h->edges[i+1], h->counts[i]);
    }
}

void mdl_histogram_free(MDLHistogram* h) {
    if (h) {
        free(h->counts);
        free(h->edges);
        free(h);
    }
}

/* ══════════════════════════════════════════════════════════════
   Change Point Detection
   ══════════════════════════════════════════════════════════════ */

/*
 * Dynamic programming for optimal change point segmentation.
 * Cost(segment) = n_seg·log₂(n_seg·σ²_seg) + penalty for each change point.
 */
static double segment_cost(const double* data, size_t start, size_t end) {
    size_t len = end - start;
    if (len == 0) return 0.0;

    double sum = 0.0, sum_sq = 0.0;
    for (size_t i = start; i < end; i++) {
        sum += data[i];
        sum_sq += data[i] * data[i];
    }
    double mean = sum / (double)len;
    double var = sum_sq / (double)len - mean * mean;
    if (var < 1e-12) var = 1e-12;

    /* Gauss NLL for segment: (len/2)log(2πσ²) + len/2 */
    double seg_cost = 0.5 * (double)len * (log(2.0 * M_PI * var)
                     + 1.0) / M_LN2;
    /* Encode segment mean and variance */
    seg_cost += log2((double)len);
    return seg_cost;
}

MDLChangePoint* mdl_changepoint_detect(const MDLData* data,
                                        int max_changes) {
    if (!data || data->n < 2 || max_changes < 1) return NULL;
    size_t n = data->n;

    /* DP: dp[i][c] = min cost for first i points with c change points */
    int C = max_changes + 1;
    double** dp = (double**)malloc((n + 1) * sizeof(double*));
    int** split = (int**)malloc((n + 1) * sizeof(int*));
    for (size_t i = 0; i <= n; i++) {
        dp[i] = (double*)malloc((size_t)C * sizeof(double));
        split[i] = (int*)malloc((size_t)C * sizeof(int));
        for (int c = 0; c < C; c++) {
            dp[i][c] = DBL_MAX;
            split[i][c] = 0;
        }
    }

    /* Base: 0 change points */
    for (size_t i = 1; i <= n; i++) {
        dp[i][0] = segment_cost(data->values, 0, i);
        split[i][0] = 0;
    }
    dp[0][0] = 0.0;

    /* DP: for each number of change points */
    for (int cp = 1; cp < C; cp++) {
        dp[0][cp] = DBL_MAX;
        for (size_t i = 1; i <= n; i++) {
            dp[i][cp] = DBL_MAX;
            for (size_t j = 1; j < i; j++) {
                if (dp[j][cp - 1] == DBL_MAX) continue;
                double cost = dp[j][cp - 1] + segment_cost(data->values, j, i);
                /* Penalty for additional change point */
                cost += log2((double)n);
                if (cost < dp[i][cp]) {
                    dp[i][cp] = cost;
                    split[i][cp] = (int)j;
                }
            }
        }
    }

    /* Find optimal number of change points */
    int best_cp = 0;
    double best_cost = dp[n][0];
    for (int cp = 1; cp < C; cp++) {
        if (dp[n][cp] < best_cost) {
            best_cost = dp[n][cp];
            best_cp = cp;
        }
    }

    /* Backtrack */
    MDLChangePoint* result = (MDLChangePoint*)malloc(sizeof(MDLChangePoint));
    assert(result != NULL);
    result->n_changes = best_cp;
    result->n_segments = best_cp + 1;
    result->total_cost = best_cost;
    result->change_points = (int*)malloc((size_t)best_cp * sizeof(int));
    result->segment_means = (double*)malloc(
        (size_t)(best_cp + 1) * sizeof(double));
    result->segment_vars = (double*)malloc(
        (size_t)(best_cp + 1) * sizeof(double));
    assert(result->change_points && result->segment_means
           && result->segment_vars);

    size_t pos = n;
    for (int c = best_cp; c >= 1; c--) {
        size_t prev = (size_t)split[pos][c];
        result->change_points[c - 1] = (int)prev;

        /* Compute segment statistics */
        double sum = 0.0, sum_sq = 0.0;
        size_t seg_len = pos - prev;
        for (size_t i = prev; i < pos; i++) {
            sum += data->values[i];
            sum_sq += data->values[i] * data->values[i];
        }
        result->segment_means[c] = sum / (double)seg_len;
        double var = sum_sq / (double)seg_len
                     - result->segment_means[c] * result->segment_means[c];
        result->segment_vars[c] = (var > 0) ? var : 1e-12;
        pos = prev;
    }
    /* First segment */
    {
        double sum = 0.0, sum_sq = 0.0;
        for (size_t i = 0; i < pos; i++) {
            sum += data->values[i];
            sum_sq += data->values[i] * data->values[i];
        }
        result->segment_means[0] = sum / (double)pos;
        double var = sum_sq / (double)pos
                     - result->segment_means[0] * result->segment_means[0];
        result->segment_vars[0] = (var > 0) ? var : 1e-12;
    }

    /* Cleanup DP */
    for (size_t i = 0; i <= n; i++) {
        free(dp[i]);
        free(split[i]);
    }
    free(dp);
    free(split);

    return result;
}

int mdl_changepoint_optimal_count(const MDLData* data, int max_changes) {
    MDLChangePoint* cp = mdl_changepoint_detect(data, max_changes);
    int count = cp ? cp->n_changes : 0;
    mdl_changepoint_free(cp);
    return count;
}

void mdl_changepoint_print(const MDLChangePoint* cp) {
    if (!cp) return;
    printf("MDLChangePoint(n_changes=%d, segments=%d, DL=%.4f bits):\n",
           cp->n_changes, cp->n_segments, cp->total_cost);
    for (int i = 0; i < cp->n_changes; i++) {
        printf("  Change at index %d\n", cp->change_points[i]);
    }
    for (int i = 0; i < cp->n_segments; i++) {
        printf("  Segment %d: μ=%.4f, σ²=%.4f\n",
               i, cp->segment_means[i], cp->segment_vars[i]);
    }
}

void mdl_changepoint_free(MDLChangePoint* cp) {
    if (cp) {
        free(cp->change_points);
        free(cp->segment_means);
        free(cp->segment_vars);
        free(cp);
    }
}

/* ══════════════════════════════════════════════════════════════
   Clustering MDL
   ══════════════════════════════════════════════════════════════ */

double mdl_cluster_cost(const MDLData* data, int dim,
                         const MDLCluster* c) {
    /*
     * MDL cost for k-means clustering:
     * L(k) + L(cluster assignments) + L(data | centroids)
     *
     * L(k): universal code for k
     * L(assignments): n·log₂ k (encode cluster per point)
     * L(data | centroids): Gaussian NLL
     */
    if (!data || !c || dim < 1 || c->n_points == 0) return 0.0;
    size_t n = c->n_points;

    /* Cost of encoding k */
    double cost = mdl_universal_integer_bits((size_t)c->k);

    /* Cost of encoding cluster assignments */
    cost += (double)n * log2((double)c->k);

    /* Cost of encoding data given centroids */
    double* cluster_var = (double*)calloc((size_t)c->k, sizeof(double));
    double* cluster_count = (double*)calloc((size_t)c->k, sizeof(double));
    assert(cluster_var && cluster_count);

    for (size_t i = 0; i < n; i++) {
        int label = c->assignments[i];
        if (label < 0 || label >= c->k) continue;
        cluster_count[label] += 1.0;
        for (int d = 0; d < dim; d++) {
            double diff = data->values[i * dim + d]
                        - c->centroids[label * dim + d];
            cluster_var[label] += diff * diff;
        }
    }

    for (int kk = 0; kk < c->k; kk++) {
        if (cluster_count[kk] > 0) {
            cluster_var[kk] /= (cluster_count[kk] * (double)dim);
            if (cluster_var[kk] < 1e-12) cluster_var[kk] = 1e-12;
            size_t nk = (size_t)cluster_count[kk];
            cost += 0.5 * (double)nk * (double)dim
                  * (log2(2.0 * M_PI * cluster_var[kk]) + 1.0 / M_LN2);
            /* Encode centroid: dim parameters */
            cost += 0.5 * (double)dim * log2((double)nk);
        }
    }

    free(cluster_var);
    free(cluster_count);
    return cost;
}

MDLCluster* mdl_cluster_kmeans_mdl(const MDLData* data, int dim,
                                    int max_k) {
    if (!data || dim < 1 || max_k < 1) return NULL;
    size_t n = data->n / (size_t)dim;
    if (n == 0) return NULL;

    int best_k = 1;
    double best_cost = DBL_MAX;
    int* best_assign = (int*)malloc(n * sizeof(int));
    double* best_centroids = (double*)malloc(
        (size_t)(max_k * dim) * sizeof(double));
    assert(best_assign && best_centroids);

    for (int k = 1; k <= max_k && (size_t)k <= n; k++) {
        /* Initialize centroids by picking first k points */
        double* centroids = (double*)calloc((size_t)(k * dim),
                                            sizeof(double));
        int* assignments = (int*)malloc(n * sizeof(int));
        assert(centroids && assignments);

        for (int c = 0; c < k; c++) {
            for (int d = 0; d < dim; d++) {
                centroids[c * dim + d] = data->values[c * dim + d];
            }
        }

        /* Lloyd's k-means: 10 iterations */
        for (int iter = 0; iter < 10; iter++) {
            /* Assign points to nearest centroid */
            for (size_t i = 0; i < n; i++) {
                double min_dist = DBL_MAX;
                int best_c = 0;
                for (int c = 0; c < k; c++) {
                    double dist = 0.0;
                    for (int d = 0; d < dim; d++) {
                        double diff = data->values[i * dim + d]
                                    - centroids[c * dim + d];
                        dist += diff * diff;
                    }
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_c = c;
                    }
                }
                assignments[i] = best_c;
            }

            /* Update centroids */
            double* new_centroids = (double*)calloc(
                (size_t)(k * dim), sizeof(double));
            double* counts = (double*)calloc((size_t)k, sizeof(double));
            assert(new_centroids && counts);

            for (size_t i = 0; i < n; i++) {
                int c = assignments[i];
                counts[c] += 1.0;
                for (int d = 0; d < dim; d++) {
                    new_centroids[c * dim + d] += data->values[i * dim + d];
                }
            }
            for (int c = 0; c < k; c++) {
                if (counts[c] > 0) {
                    for (int d = 0; d < dim; d++) {
                        centroids[c * dim + d] = new_centroids[c * dim + d]
                                               / counts[c];
                    }
                }
            }
            free(new_centroids);
            free(counts);
        }

        /* Compute MDL cost */
        MDLCluster tmp = { k, assignments, n, 0.0, centroids, 0 };
        tmp.centroids = centroids;
        double cost = mdl_cluster_cost(data, dim, &tmp);

        if (cost < best_cost) {
            best_cost = cost;
            best_k = k;
            memcpy(best_assign, assignments, n * sizeof(int));
            memcpy(best_centroids, centroids,
                   (size_t)(k * dim) * sizeof(double));
        }

        free(centroids);
        free(assignments);
    }

    MDLCluster* result = (MDLCluster*)malloc(sizeof(MDLCluster));
    assert(result != NULL);
    result->k = best_k;
    result->assignments = best_assign;
    result->n_points = n;
    result->total_cost = best_cost;
    result->centroids = best_centroids;

    return result;
}

int mdl_cluster_optimal_k(const MDLData* data, int dim, int max_k) {
    MDLCluster* c = mdl_cluster_kmeans_mdl(data, dim, max_k);
    int optimal = c ? c->k : 1;
    mdl_cluster_free(c);
    return optimal;
}

void mdl_cluster_print(const MDLCluster* c) {
    if (!c) return;
    printf("MDLCluster(k=%d, n=%zu, DL=%.4f bits):\n",
           c->k, c->n_points, c->total_cost);
    for (int i = 0; i < c->k; i++) {
        printf("  Cluster %d: centroid(", i);
        for (int d = 0; d < 1; d++) /* show 1st dimension */
            printf("%.4f", c->centroids[i]);
        printf(")\n");
    }
}

void mdl_cluster_free(MDLCluster* c) {
    if (c) {
        free(c->assignments);
        free(c->centroids);
        free(c);
    }
}

/* ══════════════════════════════════════════════════════════════
   Time Series AR Model MDL
   ══════════════════════════════════════════════════════════════ */

MDLARModel* mdl_ar_fit(const MDLData* data, int order) {
    if (!data || data->n < (size_t)order + 2 || order < 0) return NULL;
    size_t n = data->n;

    MDLARModel* ar = (MDLARModel*)malloc(sizeof(MDLARModel));
    assert(ar != NULL);
    ar->ar_order = order;
    ar->ar_coeffs = (double*)calloc((size_t)order, sizeof(double));
    assert(ar->ar_coeffs != NULL);

    if (order == 0) {
        ar->noise_var = mdl_data_variance(data, mdl_data_mean(data));
        if (ar->noise_var < 1e-12) ar->noise_var = 1e-12;
        ar->mdl_cost = 0.5 * (double)n * (log2(2.0 * M_PI * ar->noise_var)
                       + 1.0 / M_LN2) + 0.5 * log2((double)n);
        return ar;
    }

    /* Yule-Walker equations for AR(p): solve R·a = r
     * R is Toeplitz autocorrelation matrix, r is autocorrelation vector */

    /* Compute autocorrelations */
    double* acf = (double*)calloc((size_t)(order + 1), sizeof(double));
    assert(acf != NULL);
    for (size_t i = 0; i < n; i++) {
        for (int lag = 0; lag <= order; lag++) {
            if (i + (size_t)lag < n) {
                acf[lag] += data->values[i] * data->values[i + (size_t)lag];
            }
        }
    }
    for (int lag = 0; lag <= order; lag++)
        acf[lag] /= (double)(n - (size_t)lag);

    /* Levinson-Durbin recursion */
    double* coeffs = (double*)calloc((size_t)order, sizeof(double));
    double prev_var = acf[0];
    assert(coeffs != NULL);

    for (int p = 0; p < order; p++) {
        double reflection = acf[p + 1];
        for (int j = 0; j < p; j++)
            reflection -= coeffs[j] * acf[p - j];

        double rho = reflection / prev_var;

        for (int j = 0; j < (p + 1) / 2; j++) {
            double tmp = coeffs[j];
            coeffs[j] -= rho * coeffs[p - 1 - j];
            if (j != p - 1 - j)
                coeffs[p - 1 - j] -= rho * tmp;
        }
        if (p < order - 1)
            coeffs[p] = rho;

        prev_var *= (1.0 - rho * rho);
        if (prev_var < 1e-12) prev_var = 1e-12;
    }

    memcpy(ar->ar_coeffs, coeffs, (size_t)order * sizeof(double));
    ar->noise_var = prev_var;

    /* MDL cost */
    ar->mdl_cost = mdl_ar_cost(data, ar);

    free(acf);
    free(coeffs);
    return ar;
}

double mdl_ar_cost(const MDLData* data, const MDLARModel* ar) {
    if (!data || !ar) return DBL_MAX;
    size_t n = data->n;
    int p = ar->ar_order;

    /* Model cost: p parameters + noise variance */
    double model_cost = 0.5 * (double)(p + 1) * log2((double)n);

    /* Data cost: prediction errors */
    double nll = 0.0;
    size_t effective_n = 0;
    for (size_t t = (size_t)p; t < n; t++) {
        double pred = 0.0;
        for (int lag = 0; lag < p; lag++) {
            pred += ar->ar_coeffs[lag] * data->values[t - 1 - (size_t)lag];
        }
        double err = data->values[t] - pred;
        nll += err * err;
        effective_n++;
    }
    double var = (effective_n > 0) ? nll / (double)effective_n : ar->noise_var;
    if (var < 1e-12) var = 1e-12;

    double data_cost = 0.5 * (double)effective_n * (log2(2.0 * M_PI * var)
                       + 1.0 / M_LN2);

    return model_cost + data_cost;
}

int mdl_ar_optimal_order(const MDLData* data, int max_order) {
    if (!data || max_order < 0) return 0;
    int best_order = 0;
    double best_cost = DBL_MAX;

    for (int p = 0; p <= max_order; p++) {
        MDLARModel* ar = mdl_ar_fit(data, p);
        if (ar && ar->mdl_cost < best_cost) {
            best_cost = ar->mdl_cost;
            best_order = p;
        }
        mdl_ar_free(ar);
    }
    return best_order;
}

void mdl_ar_print(const MDLARModel* ar) {
    if (!ar) return;
    printf("MDLARModel(order=%d, noise_var=%.6f, DL=%.4f bits):\n",
           ar->ar_order, ar->noise_var, ar->mdl_cost);
    printf("  Coefficients: [");
    for (int i = 0; i < ar->ar_order; i++) {
        printf("%.6f%s", ar->ar_coeffs[i],
               (i < ar->ar_order - 1) ? ", " : "");
    }
    printf("]\n");
}

void mdl_ar_free(MDLARModel* ar) {
    if (ar) {
        free(ar->ar_coeffs);
        free(ar);
    }
}

/* ══════════════════════════════════════════════════════════════
   Information-Theoretic Utilities
   ══════════════════════════════════════════════════════════════ */

double mdl_log2(double x) {
    return (x > 0) ? log2(x) : -DBL_MAX;
}

double mdl_log2_loss(double predicted, double actual) {
    /* Log loss for probabilistic prediction */
    double p = predicted;
    if (p < 1e-12) p = 1e-12;
    if (p > 1.0 - 1e-12) p = 1.0 - 1e-12;
    if (actual > 0.5) return -log2(p);
    return -log2(1.0 - p);
}

double mdl_kl_divergence_gaussian(double mu1, double var1,
                                   double mu2, double var2) {
    /* D_KL( N(μ₁,σ₁²) || N(μ₂,σ₂²) )
     * = ½[ log(σ₂²/σ₁²) + σ₁²/σ₂² + (μ₁-μ₂)²/σ₂² - 1 ] */
    if (var1 <= 0 || var2 <= 0) return 0.0;
    return 0.5 * (log(var2 / var1) + var1 / var2
           + (mu1 - mu2) * (mu1 - mu2) / var2 - 1.0) / M_LN2;
}

double mdl_kl_divergence_bernoulli(double p, double q) {
    if (p <= 0 || p >= 1 || q <= 0 || q >= 1) return 0.0;
    return p * log2(p / q) + (1.0 - p) * log2((1.0 - p) / (1.0 - q));
}

double mdl_kl_divergence_poisson(double lambda1, double lambda2) {
    if (lambda1 <= 0 || lambda2 <= 0) return 0.0;
    return (lambda1 * log(lambda1 / lambda2) - lambda1 + lambda2) / M_LN2;
}

size_t mdl_min_description_length_bound(const MDLData* data) {
    /*
     * Absolute lower bound on description length:
     * MDL ≥ n·H(θ̂) (the entropy of the data under the MLE distribution)
     */
    if (!data || data->n == 0) return 0;
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;
    /* Entropy of Gaussian: ½ log₂(2πeσ²) per sample */
    double H_per_sample = 0.5 * log2(2.0 * M_PI * exp(1.0) * var);
    return (size_t)ceil((double)data->n * H_per_sample);
}

/* ══════════════════════════════════════════════════════════════
   Self-Test
   ══════════════════════════════════════════════════════════════ */

#ifdef MDL_MAIN
int main(void) {
    printf("=== MDL Core Self-Test ===\n");

    /* Data creation */
    double arr[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    MDLData* d = mdl_data_from_array(arr, 10);
    printf("Data: "); mdl_data_print(d, 10);
    printf("  mean=%.4f, var=%.4f\n",
           mdl_data_mean(d), mdl_data_variance(d, mdl_data_mean(d)));

    /* Integer codes */
    printf("\nUniversal integer codes for n=100:\n");
    printf("  Elias gamma: %.2f bits\n", mdl_elias_gamma_code(100));
    printf("  Elias delta: %.2f bits\n", mdl_elias_delta_code(100));
    printf("  Elias omega: %.2f bits\n", mdl_elias_omega_code(100));
    printf("  Rissanen log*: %.2f bits\n", mdl_rissanen_logstar_code(100));

    /* Two-part codes */
    printf("\nTwo-part codes:\n");
    printf("  Gaussian:  %.4f bits\n", mdl_two_part_gaussian(d));
    printf("  Exponential: %.4f bits\n", mdl_two_part_exponential(d));

    /* NML codes */
    printf("\nNML codes:\n");
    printf("  Gaussian NML: %.4f bits\n", mdl_nml_gaussian(d));

    /* Bernoulli example */
    double bin_arr[] = {1,0,1,1,0,1,0,1,1,0,1,1,0,1,0};
    MDLData* bin = mdl_data_from_array(bin_arr, 15);
    printf("  Bernoulli 2-part: %.4f bits\n", mdl_two_part_bernoulli(bin));
    printf("  Bernoulli NML: %.4f bits\n", mdl_nml_bernoulli(bin));

    /* Polynomial */
    MDLData* x = mdl_data_from_array(arr, 10);
    double y_arr[] = {2.1, 4.0, 6.2, 8.1, 9.9, 12.1, 14.0, 16.2,
                       18.0, 19.9};
    MDLData* y = mdl_data_from_array(y_arr, 10);
    printf("\nPolynomial fit:\n");
    for (int deg = 0; deg <= 3; deg++) {
        MDLPolynomial* p = mdl_poly_fit(x, y, deg);
        if (p) {
            mdl_poly_print(p);
            mdl_poly_free(p);
        }
    }
    printf("Optimal degree: %d\n", mdl_poly_optimal_degree(x, y, 3));

    /* Histogram */
    printf("\nHistogram:\n");
    printf("Optimal bins: %d\n", mdl_histogram_optimal_bins(d, 10));

    /* Change point */
    double cp_arr[] = {0.1, 0.2, 0.1, 0.3, 0.2, /* segment 1 */
                        5.1, 5.2, 4.9, 5.0, 5.3, /* segment 2 */
                        0.0, -0.1, 0.2, -0.1, 0.1}; /* segment 3 */
    MDLData* cp_data = mdl_data_from_array(cp_arr, 15);
    printf("\nChange point detection:\n");
    MDLChangePoint* cpt = mdl_changepoint_detect(cp_data, 3);
    mdl_changepoint_print(cpt);
    mdl_changepoint_free(cpt);

    /* AR model */
    printf("\nAR model:\n");
    printf("Optimal AR order: %d\n", mdl_ar_optimal_order(d, 3));

    /* Model selection */
    printf("\nModel selection:\n");
    MDLResult* sel = mdl_select_create(4);
    mdl_select_set_cost(sel, 0, mdl_two_part_gaussian(d));
    mdl_select_set_cost(sel, 1, mdl_two_part_exponential(d));
    mdl_select_set_cost(sel, 2, mdl_nml_gaussian(d));
    mdl_select_set_cost(sel, 3, 150.0);
    mdl_select_print(sel);
    mdl_select_free(sel);

    /* Cleanup */
    mdl_data_free(d);
    mdl_data_free(bin);
    mdl_data_free(x);
    mdl_data_free(y);
    mdl_data_free(cp_data);

    printf("\n=== All self-tests completed ===\n");
    return 0;
}
#endif /* MDL_MAIN */
