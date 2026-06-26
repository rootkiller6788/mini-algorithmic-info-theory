/*
 * nml.c ? Normalized Maximum Likelihood Implementation
 *
 * Implements NML codes for common model families, parametric complexity
 * computation, Fisher information quantities, and NML-based model selection.
 *
 * NML (Shtarkov, 1987; Rissanen, 1996):
 * P_nml(x | M) = P(x | thetahat(x), M) / C_n(M)
 * where C_n(M) = sum_{y in Y^n} P(y | thetahat(y), M)
 *
 * The NML distribution is the unique solution to the minimax problem:
 * min_Q max_x (-log Q(x) - min_theta (-log P(x|theta)))
 *
 * Key properties:
 * (1) NML is a universal code with minimax optimal redundancy
 * (2) NML asymptotically equals Bayes with Jeffreys prior (Rissanen 1996)
 * (3) For exponential families: log C_n approx (k/2)log(n/(2pi)) + log integral sqrt(det I) dtheta
 *
 * Reference: Rissanen "Fisher Information and Stochastic Complexity" (1996)
 * Grunwald (2007) Ch. 8-11 "Refined MDL"
 * Shtarkov (1987) "Universal Sequential Coding of Single Messages"
 * Barron, Rissanen, Yu (1998) "The MDL Principle in Modeling"
 */

#include "../include/nml.h"
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

/* NML Parametric Complexity */

double nml_parametric_complexity_approx(int n, int k) {
    /* Asymptotic: log C_n approx (k/2) log(n/(2pi))
     * Returns bits. */
    if (n <= 0 || k <= 0) return 0.0;
    double arg = (double)n / (2.0 * M_PI);
    if (arg < 1.0) arg = 1.0;
    return 0.5 * (double)k * log2(arg);
}

double nml_parametric_complexity_fisher(int n, int k, double jeffreys_volume) {
    /* Full parametric complexity including Jeffreys volume V_J(Theta):
     * log C_n approx (k/2) log(n/(2pi)) + log V_J(Theta)
     * Returns bits. */
    if (n <= 0 || k <= 0) return 0.0;
    double leading = 0.5 * (double)k * log((double)n / (2.0 * M_PI));
    double log_vol = (jeffreys_volume > 0) ? log(jeffreys_volume) : 0.0;
    double log_c = leading + log_vol;
    return (log_c > 0.0) ? log_c / M_LN2 : 0.0;
}

/* Fisher Information Determinants for Common Families */

double nml_fisher_information_gaussian(double sigma_sq) {
    /* det I(mu,sigma^2) = 2/sigma^6 */
    if (sigma_sq <= 0.0) return 1e-12;
    return 2.0 / (sigma_sq * sigma_sq * sigma_sq);
}

double nml_fisher_information_bernoulli(double p) {
    /* I(p) = 1/(p(1-p)) */
    if (p <= 0.0 || p >= 1.0) return 1e12;
    return 1.0 / (p * (1.0 - p));
}

double nml_fisher_information_poisson(double lambda) {
    /* I(lambda) = 1/lambda */
    if (lambda <= 0.0) return 1e12;
    return 1.0 / lambda;
}

double nml_fisher_information_exponential(double lambda) {
    /* I(lambda) = 1/lambda^2 */
    if (lambda <= 0.0) return 1e12;
    return 1.0 / (lambda * lambda);
}

/* Log Gamma via Stirling approximation */

static double log_gamma_approx(double x) {
    /* log Gamma(x) approx (x-0.5)log x - x + 0.5 log(2pi) */
    if (x <= 0.0) return 0.0;
    if (x < 2.0) {
        /* Recurse for small x: Gamma(x+1)=x*Gamma(x) */
        double shift = 0.0;
        while (x < 2.0) {
            shift += log(x);
            x += 1.0;
        }
        return log_gamma_approx(x) - shift;
    }
    return (x - 0.5) * log(x) - x + 0.5 * log(2.0 * M_PI);
}

/* NML Codes for Gaussian Models */

double nml_gaussian_known_mean(const MDLData* data) {
    /* NML for N(mu=0, sigma^2) with unknown variance. k=1 parameter.
     * log C_n approx 0.5 log(pi*n/2) */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double sum_sq = 0.0;
    for (size_t i = 0; i < n; i++)
        sum_sq += data->values[i] * data->values[i];
    double var = sum_sq / (double)n;
    if (var < 1e-12) var = 1e-12;
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0);
    double log_cn = 0.5 * log(M_PI * (double)n / 2.0);
    return (nll + log_cn) / M_LN2;
}

double nml_gaussian_known_variance(const MDLData* data) {
    /* NML for N(mu, sigma^2=1) with unknown mean. k=1 parameter.
     * log C_n approx 0.5 log(pi*n/2) */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double mu = mdl_data_mean(data);
    double rss = 0.0;
    for (size_t i = 0; i < n; i++) {
        double diff = data->values[i] - mu;
        rss += diff * diff;
    }
    double nll = 0.5 * (double)n * log(2.0 * M_PI) + 0.5 * rss;
    double log_cn = 0.5 * log(M_PI * (double)n / 2.0);
    return (nll + log_cn) / M_LN2;
}

double nml_gaussian_full(const MDLData* data) {
    /* NML for N(mu, sigma^2) with both parameters unknown. k=2.
     * Exact log C_n = log n + 0.5 log(2/pi) + log Gamma((n-1)/2) + ((n-1)/2)log 2
     * (Rissanen 1996, Theorem 2) */
    if (!data || data->n < 2) return 0.0;
    size_t n = data->n;
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0);
    double log_cn = log((double)n) + 0.5 * log(2.0 / M_PI)
                  + log_gamma_approx(((double)n - 1.0) / 2.0)
                  + ((double)n - 1.0) / 2.0 * log(2.0);
    return (nll + log_cn) / M_LN2;
}

/* NML Codes for Discrete Models */

double nml_bernoulli_full(const MDLData* data) {
    /* NML for Bernoulli(p). k=1 parameter.
     * Exact C_n = sum_{k=0}^n C(n,k) (k/n)^k ((n-k)/n)^{n-k}
     * Compute exactly for n <= 1000, asymptotic for larger n. */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double k = 0.0;
    for (size_t i = 0; i < n; i++) {
        if (data->values[i] > 0.5) k += 1.0;
    }
    double p_hat = k / (double)n;
    if (p_hat < 1e-15) p_hat = 1e-15;
    if (p_hat > 1.0 - 1e-15) p_hat = 1.0 - 1e-15;
    double nll = -k * log(p_hat) - ((double)n - k) * log(1.0 - p_hat);

    double log_cn;
    if (n <= 1000) {
        double max_term = -DBL_MAX;
        double* terms = (double*)malloc((n + 1) * sizeof(double));
        assert(terms != NULL);
        for (size_t j = 0; j <= n; j++) {
            double log_binom = log_gamma_approx((double)n + 1.0)
                             - log_gamma_approx((double)j + 1.0)
                             - log_gamma_approx((double)(n - j) + 1.0);
            double log_lik;
            if (j == 0 || j == n) {
                log_lik = 0.0;
            } else {
                log_lik = (double)j * log((double)j / (double)n)
                        + (double)(n - j) * log((double)(n - j) / (double)n);
            }
            terms[j] = log_binom + log_lik;
            if (terms[j] > max_term) max_term = terms[j];
        }
        double sum_val = 0.0;
        for (size_t j = 0; j <= n; j++)
            sum_val += exp(terms[j] - max_term);
        log_cn = max_term + log(sum_val);
        free(terms);
    } else {
        log_cn = 0.5 * log(M_PI * (double)n / 2.0) + 0.5;
    }
    return (nll + log_cn) / M_LN2;
}

double nml_multinomial_full(const MDLData* data, int m) {
    /* NML for Multinomial(m). k = m-1 parameters.
     * log C_n approx ((m-1)/2)log(n/(2pi)) + log(pi^{m/2}/Gamma(m/2)) */
    if (!data || data->n == 0 || m <= 1) return 0.0;
    size_t n = data->n;
    double* counts = (double*)calloc((size_t)m, sizeof(double));
    assert(counts != NULL);
    for (size_t i = 0; i < n; i++) {
        int cat = (int)(data->values[i]);
        if (cat < 0) cat = 0;
        if (cat >= m) cat = m - 1;
        counts[cat] += 1.0;
    }
    double nll = 0.0;
    for (int j = 0; j < m; j++) {
        double p_j = counts[j] / (double)n;
        if (p_j > 1e-15) nll -= counts[j] * log(p_j);
    }
    double log_cn = 0.5 * (double)(m - 1) * log((double)n / (2.0 * M_PI));
    log_cn += (double)m / 2.0 * log(M_PI) - log_gamma_approx((double)m / 2.0);
    free(counts);
    return (nll + log_cn) / M_LN2;
}

double nml_poisson_full(const MDLData* data) {
    /* NML for Poisson(lambda). k=1.
     * log C_n approx 0.5 log n + 0.5 log 2 */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double sum = 0.0, sum_log_fact = 0.0;
    for (size_t i = 0; i < n; i++) {
        double x = data->values[i];
        if (x < 0.0) x = 0.0;
        sum += x;
        if (x > 0.0)
            sum_log_fact += x * log(x) - x + 0.5 * log(2.0 * M_PI * x);
    }
    double lambda = sum / (double)n;
    if (lambda < 1e-15) lambda = 1e-15;
    double nll = -sum * log(lambda) + (double)n * lambda + sum_log_fact;
    double log_cn = 0.5 * log((double)n) + 0.5 * log(2.0);
    return (nll + log_cn) / M_LN2;
}

double nml_exponential_full(const MDLData* data) {
    /* NML for Exponential(lambda). k=1.
     * MLE: lambda_hat = n / sum x_i
     * log C_n approx 0.5 log n + 0.5 log(2/pi) */
    if (!data || data->n == 0) return 0.0;
    size_t n = data->n;
    double sum = 0.0;
    for (size_t i = 0; i < n; i++) {
        if (data->values[i] < 0.0) return DBL_MAX;
        sum += data->values[i];
    }
    if (sum < 1e-15) return DBL_MAX;
    double lambda = (double)n / sum;
    double nll = -(double)n * log(lambda) + lambda * sum;
    double log_cn = 0.5 * log((double)n) + 0.5 * log(2.0 / M_PI);
    return (nll + log_cn) / M_LN2;
}

/* NML for Linear Regression */

double nml_linear_regression_full(const MDLData* x, const MDLData* y) {
    /* NML for y = beta0 + beta1*x + epsilon. k=3 parameters.
     * log C_n approx (3/2)log(n) + const */
    if (!x || !y || x->n != y->n || x->n < 3) return 0.0;
    size_t n = x->n;
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
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0);
    double log_cn = 1.5 * log((double)n / (2.0 * M_PI)) + log(2.0 * M_PI);
    return (nll + log_cn) / M_LN2;
}

/* NML Model Selection */

int nml_select(const double* nml_costs, int n_models) {
    /* Select model minimizing NML cost. */
    if (!nml_costs || n_models <= 0) return -1;
    int best = 0;
    double best_cost = nml_costs[0];
    for (int i = 1; i < n_models; i++) {
        if (nml_costs[i] < best_cost) {
            best_cost = nml_costs[i];
            best = i;
        }
    }
    return best;
}

double nml_compare_models(int n, int k1, int k2, double nll1, double nll2) {
    /* Compare nested models via NML.
     * Delta = (nll2-nll1) + (k2-k1)*0.5*log(n/(2pi))
     * Positive: model1 preferred. Negative: model2 preferred. */
    if (n <= 0) return 0.0;
    double lt = (double)n / (2.0 * M_PI);
    if (lt < 1.0) lt = 1.0;
    double delta = (nll2 - nll1) + 0.5 * (double)(k2 - k1) * log(lt);
    return delta / M_LN2;
}

double nml_redundancy_rate(const MDLData* data, int k) {
    /* Redundancy rate: (L_nml - L_mle)/n.
     * As n->inf, this -> (k ln n)/(2n) -> 0. */
    if (!data || data->n == 0 || k <= 0) return 0.0;
    size_t n = data->n;
    double mu = mdl_data_mean(data);
    double var = mdl_data_variance(data, mu);
    if (var < 1e-12) var = 1e-12;
    double nll = 0.5 * (double)n * (log(2.0 * M_PI * var) + 1.0) / M_LN2;
    double nml_cost = (nll * M_LN2
                     + 0.5 * (double)k * log((double)n / (2.0 * M_PI)))
                     / M_LN2;
    return (nml_cost - nll) / (double)n;
}

double nml_bayes_jeffreys_gap(const MDLData* data, int k) {
    /* Gap between NML and Bayes-Jeffreys.
     * Rissanen (1996, Theorem 3): asymptotically equivalent.
     * Finite-sample gap is O(1), approx 0.5 nats for Gaussian. */
    if (!data || data->n < 2 || k <= 0) return 0.0;
    double gap_nats = 0.5;
    return gap_nats / M_LN2;
}

/* NML Power Analysis (L8 - Advanced) */

double nml_model_selection_power(int n, int k_true, int k_null,
                                  double effect_size) {
    /* Estimate probability that NML correctly selects the larger model.
     * Uses chi-squared approximation to the likelihood ratio test.
     * Parameters:
     * n            - sample size
     * k_true       - true number of parameters
     * k_null       - null number of parameters (k_true > k_null)
     * effect_size  - noncentrality parameter / df
     * Returns: estimated power in [0,1]. */
    if (n <= 0 || k_true <= k_null) return 0.0;
    double threshold = 0.5 * (double)(k_true - k_null)
                      * log((double)n / (2.0 * M_PI));
    int df = k_true - k_null;
    double ncp = (double)n * effect_size;
    double mean = (double)df + ncp;
    double var = 2.0 * ((double)df + 2.0 * ncp);
    if (var < 1e-12) return 0.0;
    double z = (2.0 * threshold - mean) / sqrt(var);
    double power = 0.5 * (1.0 - erf(z / sqrt(2.0)));
    if (power < 0.0) power = 0.0;
    if (power > 1.0) power = 1.0;
    return power;
}

#ifdef NML_MAIN
int main(void) {
    printf("=== NML Self-Test ===\n\n");
    double gauss_arr[] = {0.5,-0.3,1.2,0.1,-0.8,0.7,0.3,-0.1,
                          1.0,-0.5,0.2,0.9,-0.4,0.6,-0.2,0.0,
                          0.4,-0.6,1.1,0.8};
    MDLData* gauss = mdl_data_from_array(gauss_arr, 20);
    double bin_arr[] = {1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,0,0,1,1};
    MDLData* bern = mdl_data_from_array(bin_arr, 20);
    double poi_arr[] = {2,3,1,4,2,0,3,2,5,1,2,3,0,2,4,1,3,2,1,2};
    MDLData* pois = mdl_data_from_array(poi_arr, 20);

    printf("Fisher information:\n");
    printf("  Gaussian(sig^2=1): det I = %.4f\n",
           nml_fisher_information_gaussian(1.0));
    printf("  Bernoulli(p=0.5): I = %.4f\n",
           nml_fisher_information_bernoulli(0.5));
    printf("  Poisson(lambda=2): I = %.4f\n",
           nml_fisher_information_poisson(2.0));
    printf("  Exponential(lambda=1): I = %.4f\n\n",
           nml_fisher_information_exponential(1.0));

    printf("Parametric complexity:\n");
    printf("  n=100, k=2: %.4f bits\n",
           nml_parametric_complexity_approx(100, 2));
    printf("  n=100, k=1, VJ=pi: %.4f bits\n\n",
           nml_parametric_complexity_fisher(100, 1, M_PI));

    printf("Gaussian NML:\n");
    printf("  Known mean: %.4f bits\n", nml_gaussian_known_mean(gauss));
    printf("  Known var: %.4f bits\n", nml_gaussian_known_variance(gauss));
    printf("  Full (k=2): %.4f bits\n\n", nml_gaussian_full(gauss));

    printf("Discrete NML:\n");
    printf("  Bernoulli: %.4f bits\n", nml_bernoulli_full(bern));
    printf("  Multinomial(m=3): %.4f bits\n",
           nml_multinomial_full(bern, 3));
    printf("  Poisson: %.4f bits\n\n", nml_poisson_full(pois));

    printf("Redundancy rate: n=%zu, k=2: %.6f bits/sample\n",
           gauss->n, nml_redundancy_rate(gauss, 2));
    printf("Bayes-Jeffreys gap: %.4f bits\n",
           nml_bayes_jeffreys_gap(gauss, 2));
    printf("\nNML power: n=100, k_true=2,k_null=1,effect=0.1: %.4f\n",
           nml_model_selection_power(100, 2, 1, 0.1));
    printf("NML power: n=500, k_true=2,k_null=1,effect=0.1: %.4f\n",
           nml_model_selection_power(500, 2, 1, 0.1));

    mdl_data_free(gauss); mdl_data_free(bern); mdl_data_free(pois);
    printf("\n=== All NML self-tests completed ===\n");
    return 0;
}
#endif
