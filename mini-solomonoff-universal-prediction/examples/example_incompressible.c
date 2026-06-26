/**
 * example_incompressible.c - Incompressibility Demonstration
 *
 * Demonstrates the incompressibility method: most strings are
 * algorithmically random (K(x) ~ |x|), while structured strings
 * are compressible (K(x) << |x|).
 *
 * The incompressibility method is a powerful proof technique:
 * if a property holds for all compressible strings, it must
 * hold for most strings (since only a tiny fraction are compressible).
 */
#include "kolmogorov.h"
#include "universal_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("=== Incompressibility Demo ===\n\n");
    printf("Testing K(x) for various 8-bit strings:\n\n");

    const char *test_strings[] = {
        "00000000",  /* Highly compressible (repetition) */
        "11111111",  /* Highly compressible */
        "01010101",  /* Compressible (alternating) */
        "00110011",  /* Compressible (period 4) */
        "01101001",  /* Less structure */
        "10101100",  /* More random-looking */
        NULL
    };

    printf("%-12s %6s %6s %s\n", "String", "K(x)", "|x|", "Compressible?");
    printf("%-12s %6s %6s %s\n", "--------", "----", "---", "-------------");

    for (int i = 0; test_strings[i]; i++) {
        binary_string_t x;
        bs_from_cstr(&x, test_strings[i]);

        size_t K = kolmogorov_K(&x, 6, 200000, NULL);
        int deficiency = kolmogorov_randomness_deficiency(&x, 6, 200000);
        bool comp = !kolmogorov_is_incompressible(&x, 0, 6, 200000);

        printf("%-12s %6zu %6zu %s (def=%d)\n",
               test_strings[i], K, x.length,
               comp ? "YES" : "NO",
               deficiency);
    }

    printf("\nKey insight: only a fraction 2^{-c} of strings of length n\n");
    printf("have K(x) < n - c. Most strings are algorithmically random.\n");
    printf("This is the foundation of the incompressibility method.\n");

    return 0;
}
