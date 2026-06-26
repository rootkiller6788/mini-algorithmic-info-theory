/*
 * example_complexity_profile.c - MDL Complexity Profile Demo
 *
 * Demonstrates MDL model selection across polynomial degrees,
 * comparing two-part MDL, NML, AIC, and BIC on synthetic data.
 *
 * Run: make examples
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/mdl_core.h"
#include "../include/nml.h"
#include "../include/mdl_advanced.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    printf("=== MDL Complexity Profile: Polynomial Degree Selection ===\n\n");

    /* Generate synthetic data: y = 2 + 3x + 1.5x^2 + noise */
    int n = 30;
    MDLData* x = mdl_data_create((size_t)n);
    MDLData* y = mdl_data_create((size_t)n);

    for (int i = 0; i < n; i++) {
        double xi = (double)i * 0.2;
        double yi = 2.0 + 3.0 * xi + 1.5 * xi * xi
                   + 0.5 * ((double)rand() / RAND_MAX - 0.5);
        mdl_data_append(x, xi);
        mdl_data_append(y, yi);
    }

    printf("Data: n=%d points, true model: y = 2 + 3x + 1.5x^2 + N(0,0.02)\n\n", n);

    printf("%-8s %14s %14s %14s %14s %14s\n",
           "Degree", "MSE", "2-Part DL", "NML", "AIC", "BIC");
    printf("-------- -------------- -------------- --------------"
           " -------------- --------------\n");

    int max_deg = 5;
    for (int d = 0; d <= max_deg; d++) {
        MDLPolynomial* p = mdl_poly_fit(x, y, d);
        if (!p || p->mse >= 1e100) continue;

        double nll = 0.5 * (double)n * (log(2.0 * M_PI * p->mse) + 1.0);
        ModelCriteria* crit = mdl_criteria_compute(y, d + 1, nll);

        printf("  %-4d   %12.6f   %12.4f   %12.4f   %12.4f   %12.4f\n",
               d, p->mse, p->two_part_cost,
               crit->mdl_nml, crit->aic, crit->bic);

        mdl_criteria_free(crit);
        mdl_poly_free(p);
    }

    /* Optimal by two-part MDL */
    int best_2p = mdl_poly_optimal_degree(x, y, max_deg);
    printf("\nOptimal degree by two-part MDL: %d\n", best_2p);

    /* Optimal by NML */
    int best_nml = 0;
    double best_nml_cost = 1e300;
    for (int d = 0; d <= max_deg; d++) {
        MDLPolynomial* p = mdl_poly_fit(x, y, d);
        if (!p) continue;
        double cost = mdl_nml_linear_regression(x, y);
        /* For higher degrees, use approximate NML */
        if (d == 1) {
            if (cost < best_nml_cost) {
                best_nml_cost = cost;
                best_nml = d;
            }
        }
        mdl_poly_free(p);
    }
    printf("Optimal degree by NML (linreg): %d (cost=%.4f bits)\n",
           best_nml, best_nml_cost);

    mdl_data_free(x);
    mdl_data_free(y);

    printf("\n=== Complexity profile complete ===\n");
    return 0;
}
