/*
 * example_complexity_profile.c — Compute and display Kolmogorov
 * complexity profiles for various types of strings.
 *
 * L6: Demonstrates complexity analysis on real strings.
 */

#include "../include/kolmogorov.h"
#include "../include/string_tools.h"
#include "../include/compression.h"
#include "../include/entropy.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║  Kolmogorov Complexity Profile Analyzer       ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");

    /* Example strings with different complexity characteristics */
    const char* examples[] = {
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",      /* highly compressible */
        "ABABABABABABABABABABABABABABABABAB",         /* periodic */
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ",                  /* low complexity (alphabet) */
        "The quick brown fox jumps over the lazy dog", /* natural language */
        "cstP!x9$w#2mK@qL",                           /* pseudo-random */
    };
    int n = sizeof(examples) / sizeof(examples[0]);

    for (int i = 0; i < n; i++) {
        KString* s = kstr_from_cstr(examples[i]);

        printf("───────────────────────────────────────────\n");
        printf("String %d: ", i + 1);
        kstr_print(s);
        printf("Length: %zu bytes\n", s->len);

        /* Complexity upper bound */
        size_t ub = k_complexity_upper_bound(s);
        printf("  K(x) upper bound : %zu bits (%.2f bits/byte)\n",
               ub * 8, (double)(ub * 8) / (double)s->len);

        /* Shannon entropy */
        double H = shannon_entropy_bits(s);
        printf("  Shannon entropy  : %.4f bits/symbol\n", H);

        /* Normalized entropy */
        double nH = normalized_entropy(s, 256);
        printf("  Normalized entropy: %.4f\n", nH);

        /* Min-entropy */
        double Hmin = min_entropy(s);
        printf("  Min-entropy      : %.4f bits\n", Hmin);

        /* Redundancy */
        double red = redundancy(s, 256);
        printf("  Redundancy       : %.4f\n", red);

        /* Incompressibility */
        int incomp8  = k_is_incompressible(s, 8);
        int incomp32 = k_is_incompressible(s, 32);
        printf("  8-incompressible : %s\n", incomp8 ? "yes" : "no");
        printf("  32-incompressible: %s\n", incomp32 ? "yes" : "no");

        /* Randomness deficiency */
        int rd = k_randomness_deficiency_estimate(s);
        printf("  Randomness def.  : %d bits\n", rd);

        /* Compression ratios using multiple algorithms */
        printf("  Compression ratios:\n");
        printf("    LZ77   : %.4f\n", lz77_ratio(s));
        printf("    LZ78   : %.4f\n", lz78_ratio(s));
        printf("    LZW    : %.4f\n", lzw_ratio(s));
        printf("    RLE    : %.4f\n", rle_ratio(s));
        printf("    Huffman: %.4f\n", huffman_ratio(s));

        /* Block entropy */
        if (s->len >= 4) {
            double H2 = block_entropy(s, 2);
            printf("  Block entropy H₂ : %.4f\n", H2);
            double H3 = block_entropy(s, 3);
            printf("  Block entropy H₃ : %.4f\n", H3);
        }

        printf("\n");
        kstr_free(s);
    }

    return 0;
}
