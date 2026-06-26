/*
 * example_randomness_analysis.c - MDL Randomness Analysis
 *
 * Demonstrates MDL-based randomness analysis: distinguishing
 * random from structured data via description length.
 *
 * Run: make examples
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../include/mdl_core.h"
#include "../include/mdl_advanced.h"

int main(void) {
    printf("=== MDL Randomness Analysis ===\n\n");

    int n = 50;

    /* Structured data: linear trend */
    MDLData* structured = mdl_data_create((size_t)n);
    for (int i = 0; i < n; i++)
        mdl_data_append(structured, 0.5 * (double)i + 1.0);

    /* Random data: Gaussian noise */
    MDLData* random_gauss = mdl_data_create((size_t)n);
    for (int i = 0; i < n; i++)
        mdl_data_append(random_gauss,
            5.0 + 2.0 * ((double)rand() / RAND_MAX - 0.5));

    /* Structured with change point */
    MDLData* changepoint = mdl_data_create((size_t)n);
    for (int i = 0; i < n; i++) {
        if (i < n/2)
            mdl_data_append(changepoint, 1.0 + 0.1 * ((double)rand()/RAND_MAX - 0.5));
        else
            mdl_data_append(changepoint, 5.0 + 0.1 * ((double)rand()/RAND_MAX - 0.5));
    }

    /* Analyze each dataset */
    printf("Dataset 1: Linear trend y = 0.5*x + 1\n");
    printf("  Two-part Gaussian DL:   %.2f bits\n",
           mdl_two_part_gaussian(structured));
    printf("  Optimal poly degree:    %d\n",
           mdl_poly_optimal_degree(structured, structured, 3));

    printf("\nDataset 2: Random Gaussian N(5, 1.33)\n");
    printf("  Two-part Gaussian DL:   %.2f bits\n",
           mdl_two_part_gaussian(random_gauss));
    printf("  Optimal AR order:       %d\n",
           mdl_ar_optimal_order(random_gauss, 3));

    printf("\nDataset 3: Step function (change point at n/2)\n");
    printf("  Two-part Gaussian DL:   %.2f bits\n",
           mdl_two_part_gaussian(changepoint));
    /* Detect change points */
    MDLChangePoint* cp = mdl_changepoint_detect(changepoint, 3);
    printf("  Detected change points: %d\n", cp ? cp->n_changes : 0);
    if (cp && cp->n_changes > 0) {
        printf("  Change at index:        %d\n", cp->change_points[0]);
        printf("  Total MDL cost:         %.2f bits\n", cp->total_cost);
    }
    mdl_changepoint_free(cp);

    /* MDL complexity ratio: structured vs. random */
    double dl_structured = mdl_two_part_gaussian(structured);
    double dl_random = mdl_two_part_gaussian(random_gauss);
    double ratio = dl_structured / dl_random;

    printf("\n=== Randomness Index ===\n");
    printf("DL(structured) / DL(random) = %.4f\n", ratio);
    printf("(Ratio < 1 indicates compressible structure)\n");
    printf("(Ratio ~= 1 indicates incompressible randomness)\n");

    /* Prequential analysis */
    printf("\n=== Prequential (Sequential) Analysis ===\n");
    PrequentialResult* pr_s = mdl_prequential_gaussian(structured, 2);
    PrequentialResult* pr_r = mdl_prequential_gaussian(random_gauss, 2);
    if (pr_s && pr_r) {
        printf("Structured: mean log-loss = %.4f bits, regret = %.4f\n",
               pr_s->mean_log_loss, pr_s->final_regret);
        printf("Random:     mean log-loss = %.4f bits, regret = %.4f\n",
               pr_r->mean_log_loss, pr_r->final_regret);
        printf("Prequential confirms: structured data is more predictable.\n");
    }
    mdl_prequential_free(pr_s);
    mdl_prequential_free(pr_r);

    mdl_data_free(structured);
    mdl_data_free(random_gauss);
    mdl_data_free(changepoint);

    printf("\n=== Randomness analysis complete ===\n");
    return 0;
}
