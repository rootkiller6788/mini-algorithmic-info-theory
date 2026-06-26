/**
 * example_time_bounded_k.c - Demonstrate time-bounded Kolmogorov complexity
 */
#include <stdio.h>
#include <stdlib.h>
#include "../include/resource_bounded_k.h"

int main(void) {
    printf("=== Time-Bounded Kolmogorov Complexity Demo ===\n\n");
    const char *s1 = "0101010101010101";
    const char *s2 = "0010110111011111";

    printf("String 1 (repeating): \"%s\"\n", s1);
    RBKComplexity k1_short = rbk_K_time(s1, 16, 1000);
    RBKComplexity k1_long  = rbk_K_time(s1, 16, 65536);
    printf("  K^1000 = %zu, K^65536 = %zu\n", k1_short.K_value, k1_long.K_value);
    printf("  Compressibility: %.2f\n", rbk_compressibility_ratio(s1, 16, 65536));
    rbk_complexity_free(&k1_short); rbk_complexity_free(&k1_long);

    printf("\nString 2 (irregular): \"%s\"\n", s2);
    RBKComplexity k2 = rbk_K_time(s2, 16, 65536);
    printf("  K^65536 = %zu\n", k2.K_value);
    printf("  Compressibility: %.2f\n", rbk_compressibility_ratio(s2, 16, 65536));
    int cls = rbk_classify_string(s2, 16, 65536);
    printf("  Class: %s\n", cls == 0 ? "incompressible" : cls == 1 ? "structured" : "simple");
    rbk_complexity_free(&k2);

    printf("\n=== Comparison ===\n");
    int more = rbk_which_more_complex(s1, 16, s2, 16, 65536);
    printf("More complex string: %s\n", more == 0 ? "S1" : more == 1 ? "S2" : "equal");

    printf("\n=== Randomness Test ===\n");
    RBKRandomnessResult rr1 = rbk_randomness_deficiency(s1, 16, 65536);
    rbk_print_randomness(&rr1);
    RBKRandomnessResult rr2 = rbk_randomness_deficiency(s2, 16, 65536);
    rbk_print_randomness(&rr2);

    return 0;
}