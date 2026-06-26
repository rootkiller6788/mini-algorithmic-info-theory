/*
 * demo_coding_theorem.c - Coding Theorem Demonstration
 *
 * Demonstrates the relationship between code length and probability
 * in MDL: shorter codes for more probable events.
 *
 * Run: make demos
 */
#include <stdio.h>
#include <math.h>
#include "../include/mdl_core.h"
#include "../include/nml.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main(void) {
    printf("=== MDL Coding Theorem Demo ===\n\n");
    printf("Kraft inequality: sum 2^{-L(x)} <= 1\n");
    printf("MDL principle: L(x) = -log_2 P(x)\n\n");

    double mu_arr[] = {0.5,-0.3,1.2,0.1,-0.8,0.7,0.3,-0.1,1.0,-0.5};
    MDLData* d = mdl_data_from_array(mu_arr, 10);

    double dl = mdl_two_part_gaussian(d);
    double prob = pow(2.0, -dl);
    printf("Two-part Gaussian DL: %.2f bits\n", dl);
    printf("Implied probability: 2^{-DL} = %.6e\n", prob);
    printf("(This is the maximum probability assignable to this\n");
    printf(" dataset under the Gaussian model class.)\n\n");

    printf("Comparison of code lengths:\n");
    int ns[] = {10, 50, 100, 500, 1000};
    for (int i = 0; i < 5; i++) {
        double pc = nml_parametric_complexity_approx(ns[i], 2);
        printf("  n=%4d, k=2: parametric complexity = %.4f bits\n",
               ns[i], pc);
    }
    printf("(Parametric complexity grows as O(k log n))\n");

    mdl_data_free(d);
    printf("\n=== Coding theorem demo complete ===\n");
    return 0;
}
