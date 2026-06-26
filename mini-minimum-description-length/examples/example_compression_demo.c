/*
 * example_compression_demo.c - MDL Compression Demo
 *
 * Demonstrates MDL as a compression principle: data compression
 * via model-based encoding (two-part codes).
 *
 * Run: make examples
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../include/mdl_core.h"
#include "../include/nml.h"

int main(void) {
    printf("=== MDL Compression Demo ===\n\n");

    /* Dataset 1: Gaussian data */
    double gauss[] = {2.1, 1.8, 2.3, 1.9, 2.0, 2.2, 1.7, 2.4, 2.1, 1.9,
                      2.0, 2.3, 1.8, 2.2, 2.1, 2.0, 1.9, 2.3, 2.1, 2.0};
    MDLData* gd = mdl_data_from_array(gauss, 20);

    double baseline = (double)gd->n * 32.0;  /* naive: 32 bits per float */
    double gauss_2part = mdl_two_part_gaussian(gd);
    double gauss_nml = nml_gaussian_full(gd);

    printf("Dataset: Gaussian N(2, 0.04), n=20\n");
    printf("  Raw storage (32-bit float): %.0f bits\n", baseline);
    printf("  Two-part Gaussian code:     %.2f bits (%.1f%% compression)\n",
           gauss_2part, 100.0 * (1.0 - gauss_2part / baseline));
    printf("  NML Gaussian code:          %.2f bits (%.1f%% compression)\n\n",
           gauss_nml, 100.0 * (1.0 - gauss_nml / baseline));

    /* Dataset 2: Bernoulli data */
    double bern[] = {1,0,1,1,0,1,0,1,1,0,1,1,0,1,0,1,0,0,1,1,
                     1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,
                     1,1,0,1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1,1};
    MDLData* bd = mdl_data_from_array(bern, 60);

    double bern_raw = (double)bd->n;  /* 1 bit per bit */
    double bern_2part = mdl_two_part_bernoulli(bd);
    double bern_nml = nml_bernoulli_full(bd);

    printf("Dataset: Bernoulli(p=0.6), n=60\n");
    printf("  Raw storage (1 bit/bit):  %.0f bits\n", bern_raw);
    printf("  Two-part Bernoulli code:  %.2f bits\n", bern_2part);
    printf("  NML Bernoulli code:       %.2f bits\n\n", bern_nml);

    /* Dataset 3: Poisson data */
    double pois[] = {2,3,1,4,2,0,3,2,5,1,2,3,0,2,4,1,3,2,1,2,
                     3,2,1,4,2,3,0,2,4,1,3,2,1,4,2,3,1,2,3,1};
    MDLData* pd = mdl_data_from_array(pois, 40);

    double pois_raw = (double)pd->n * 32.0;
    double pois_2part = mdl_two_part_poisson(pd);
    double pois_nml = nml_poisson_full(pd);

    printf("Dataset: Poisson(lambda=2), n=40\n");
    printf("  Raw storage (32-bit int): %.0f bits\n", pois_raw);
    printf("  Two-part Poisson code:    %.2f bits (%.1f%% compression)\n",
           pois_2part, 100.0 * (1.0 - pois_2part / pois_raw));
    printf("  NML Poisson code:         %.2f bits (%.1f%% compression)\n\n",
           pois_nml, 100.0 * (1.0 - pois_nml / pois_raw));

    /* Summary */
    printf("=== Compression Summary ===\n");
    printf("Model-based encoding compresses by capturing data regularities.\n");
    printf("The two-part code L(M)+L(D|M) formalizes Occam's razor:\n");
    printf("  simpler models = shorter L(M) but longer L(D|M)\n");
    printf("  complex models = longer L(M) but shorter L(D|M)\n");
    printf("MDL finds the optimal trade-off.\n");

    mdl_data_free(gd); mdl_data_free(bd); mdl_data_free(pd);
    return 0;
}
