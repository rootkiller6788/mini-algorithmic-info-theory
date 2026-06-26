/*
 * demo_chaitin_omega.c 〞 Chaitin's Omega Constant Demo
 *
 * Knowledge points: 次 = 曳_{p: U(p) halts} 2^{-|p|}
 * L4: Properties of 次 (random, non-computable, lower semicomputable)
 * L7: 次 encodes halting problem for programs ≒ n bits
 *
 * Reference: Chaitin (1975), Calude (2002), Li & Vit芍nyi ∫3.6
 * Courses: MIT 6.841 ∫6, Stanford CS254 ∫5
 */
#include "../include/prefix_machine.h"
#include <stdio.h>
int main(void) {
    printf("=== Chaitin's Omega Demo ===\n\n");
    printf("次 = 曳_{p: U(p) halts} 2^{-|p|}\n\n");
    for (int max_len = 2; max_len <= 14; max_len++) {
        double omega = pm_chaitin_omega_estimate(max_len, 1000);
        printf("  max_len=%3d  次 ＞ %.10f\n", max_len, omega);
    }
    printf("\nProperties:\n");
    printf("  0 < 次 < 1, 次 is algorithmically random\n");
    printf("  次 is lower semicomputable but not computable\n");
    printf("  次 encodes the halting problem\n");
    printf("  曳_x 2^{-K(x)} = 次  (with equality for universal U)\n\n");
    printf("Prefix halting probability:\n");
    for (int max_len = 2; max_len <= 14; max_len++) {
        double ph = pm_omega_prefix_halting_prob(max_len);
        printf("  max_len=%3d  P(halt) ＞ %.10f\n", max_len, ph);
    }
    printf("\nDemo complete.\n");
    return 0;
}
