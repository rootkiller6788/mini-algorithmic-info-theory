/*
 * example_randomness_analysis.c — Detecting randomness vs structure
 * using Kolmogorov complexity heuristics.
 *
 * L6: Canonical problem — is a given string random?
 * L7: Application — randomness testing for cryptography, quality assurance.
 */

#include "../include/kolmogorov.h"
#include "../include/string_tools.h"
#include "../include/compression.h"
#include "../include/entropy.h"
/* Note: kstr_random is declared in string_tools.h */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

/* Generate a "structured" string that looks random to simple tests */
static KString* generate_pseudo_low_complexity(int n) {
    /* Digits of π: 3.141592653589793... */
    KString* s = kstr_create((size_t)n);
    const char* pi = "314159265358979323846264338327950288419716939937510"
                     "58209749445923078164062862089986280348253421170679";
    for (int i = 0; i < n; i++) {
        kstr_append(s, (unsigned char)pi[i % strlen(pi)]);
    }
    return s;
}

int main(void) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Randomness vs. Structure Analyzer       ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    int test_len = 64;

    /* 1. Truly compressible (constant) */
    KString* constant = kstr_create((size_t)test_len);
    for (int i = 0; i < test_len; i++) kstr_append(constant, 'A');

    /* 2. Simple pattern (de Bruijn-like) */
    KString* pattern = kstr_create((size_t)test_len);
    for (int i = 0; i < test_len; i++)
        kstr_append(pattern, (unsigned char)('A' + (i % 26)));

    /* 3. Pseudo-random via LCG */
    KString* prng = kstr_create((size_t)test_len);
    unsigned int seed = (unsigned int)time(NULL);
    for (int i = 0; i < test_len; i++) {
        seed = seed * 1103515245U + 12345U;
        kstr_append(prng, (unsigned char)((seed >> 16) & 0xFF));
    }

    /* 4. Digits of π (looks random, but low Kolmogorov complexity) */
    KString* pi_digits = generate_pseudo_low_complexity(test_len);

    KString* samples[] = {constant, pattern, prng, pi_digits};
    const char* labels[] = {"Constant (AAA...)", "Cyclic Pattern",
                            "LCG Pseudo-Random", "Digits of π"};
    int n_samples = 4;

    printf("%-25s  %8s  %8s  %8s  %8s  %8s  %8s\n",
           "Sample", "H(bits)", "H_norm", "Min-H", "C8-incomp", "C32-incomp", "δ(x)");
    printf("%-25s  %8s  %8s  %8s  %8s  %8s  %8s\n",
           "──────", "───────", "───────", "───────", "───────", "────────", "────");

    for (int i = 0; i < n_samples; i++) {
        KString* s = samples[i];
        double H  = shannon_entropy_bits(s);
        double nH = normalized_entropy(s, 256);
        double Hm = min_entropy(s);
        int c8  = k_is_incompressible(s, 8);
        int c32 = k_is_incompressible(s, 32);
        int rd  = k_randomness_deficiency_estimate(s);

        printf("%-25s  %8.4f  %8.4f  %8.4f  %8s  %8s  %8d\n",
               labels[i], H, nH, Hm,
               c8 ? "yes" : "no", c32 ? "yes" : "no", rd);
    }

    printf("\n───────────────────────────────────────\n");
    printf("Analysis:\n");
    printf("• Constant string has H ≈ 0, highly compressible, low deficiency\n");
    printf("• Cyclic pattern has moderate H, compressible by LZ algorithms\n");
    printf("• LCG PRNG shows high H, but it is deterministic (finite state)\n");
    printf("• Digits of π: high H, but K(π) ≈ O(log n) — structured!\n");
    printf("  This illustrates the gap between Shannon entropy and Kolmogorov\n");
    printf("  complexity: π has high entropy but low information content.\n");

    kstr_free(constant);
    kstr_free(pattern);
    kstr_free(prng);
    kstr_free(pi_digits);

    return 0;
}
