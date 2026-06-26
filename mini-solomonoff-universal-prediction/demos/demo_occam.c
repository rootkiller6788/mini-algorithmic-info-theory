/**
 * demo_occam.c - Occam's Razor Formalized
 *
 * Demonstrates that M(x) assigns higher probability to simpler
 * explanations (shorter programs) for the same data.
 *
 * Key insight: Solomonoff's M(x) = sum 2^{-|p|} for programs p
 * that output x*. This means:
 *   - A program of length 10 contributes weight 2^{-10} = 1/1024
 *   - A program of length 20 contributes weight 2^{-20} = 1/1048576
 *
 * Shorter programs are exponentially more influential!
 * This is Occam's razor made mathematically precise.
 */
#include "solomonoff.h"
#include "universal_machine.h"
#include <stdio.h>
#include <math.h>

int main(void) {
    printf("=== Occam's Razor: Algorithmic Probability Weights ===\n\n");

    printf("Program length  ->  Weight 2^{-|p|}  ->  Probability\n");
    printf("%-16s %-20s %s\n", "---------------", "--------------------", "-----------");

    for (size_t len = 0; len <= 16; len++) {
        double weight = pow(2.0, -(double)(int64_t)len);
        printf("|p| = %-10zu  2^{-%zu} = %-16g  (= %.2e)\n",
               len, len, weight, weight);
    }

    printf("\nKey observations:\n");
    printf("  1. Shortest program (|p|=0): weight = 1.0 (dominates!)\n");
    printf("  2. |p|=10: weight ~ 0.001 (still significant)\n");
    printf("  3. |p|=20: weight ~ 10^{-6} (negligible)\n");
    printf("  4. |p|=60: weight ~ 10^{-18} (astronomically small)\n\n");

    printf("This exponential decay in weight formalizes Occam's razor:\n");
    printf("  'Entities should not be multiplied beyond necessity'\n");
    printf("  -> Simpler theories (shorter programs) have exponentially\n");
    printf("     higher prior probability.\n\n");

    printf("Practical implication: When we compute M(x) for data x,\n");
    printf("the contribution is dominated by the SHORTEST programs\n");
    printf("that can produce x. Longer programs matter very little.\n");
    printf("This is why algorithmic probability naturally favors\n");
    printf("simple explanations of data.\n");

    return 0;
}
