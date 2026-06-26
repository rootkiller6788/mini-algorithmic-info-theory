/**
 * example_crypto_app.c - Cryptographic incompressibility demo
 *
 * Demonstrates how Kolmogorov complexity principles apply to:
 *   - Pseudorandom generator (PRG) evaluation
 *   - One-way function (OWF) hardness estimation
 *   - Compressibility-based randomness testing
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/resource_bounded_k.h"
#include "../include/complexity_approx.h"

int main(void) {
    printf("=== Cryptographic Compressibility Demo ===\n\n");

    /* Simulated PRG outputs: one structured, one random-like */
    const char *weak_prg   = "1111111111111111"; /* all 1s - predictable */
    const char *strong_prg = "0110100110010110"; /* mixed - more random */
    size_t n = 16;

    /* 1. Compressibility ratio (lower = more compressible = weaker cipher) */
    double ratio_weak   = rbk_compressibility_ratio(weak_prg, n, 65536);
    double ratio_strong = rbk_compressibility_ratio(strong_prg, n, 65536);
    printf("PRG Compressibility:\n");
    printf("  Weak PRG ratio:   %.2f (lower = more structure)\n", ratio_weak);
    printf("  Strong PRG ratio: %.2f (near 1.0 = incompressible)\n", ratio_strong);

    /* 2. Cryptographic weakness gap */
    size_t gap_weak   = rbk_crypto_weakness_gap(weak_prg, n, 65536);
    size_t gap_strong = rbk_crypto_weakness_gap(strong_prg, n, 65536);
    printf("\nWeakness Gap (|x| - K(x)):\n");
    printf("  Weak PRG gap:   %zu bits (large = detectable)\n", gap_weak);
    printf("  Strong PRG gap: %zu bits (small = hard to detect)\n", gap_strong);

    /* 3. One-way function hardness simulation */
    double owf = rbk_owf_hardness(strong_prg, n, "0011", 4, 65536);
    printf("\nOWF Hardness Estimate: %.2f\n", owf);

    /* 4. Approximate incompressibility test */
    printf("\nIncompressibility Test (slack=4):\n");
    printf("  Weak PRG:   %s\n",
           approx_incompressibility_test(weak_prg, n, 4) ? "PASS" : "FAIL");
    printf("  Strong PRG: %s\n",
           approx_incompressibility_test(strong_prg, n, 4) ? "PASS" : "FAIL");

    /* 5. Entropy-based determinism detection */
    printf("\nEntropy Determinism Test (threshold=0.3):\n");
    printf("  Weak PRG:   %s\n",
           entropy_determinism_test(weak_prg, n, 0.3) ? "DETERMINISTIC" : "RANDOM");
    printf("  Strong PRG: %s\n",
           entropy_determinism_test(strong_prg, n, 0.3) ? "DETERMINISTIC" : "RANDOM");

    /* 6. Martingale test */
    MartingaleApprox m_weak   = martingale_compressibility_test(weak_prg, n);
    MartingaleApprox m_strong = martingale_compressibility_test(strong_prg, n);
    printf("\nMartingale Test:\n");
    printf("  Weak PRG:   capital=%.4f, reject=%s\n",
           m_weak.capital, m_weak.reject_randomness ? "YES" : "NO");
    printf("  Strong PRG: capital=%.4f, reject=%s\n",
           m_strong.capital, m_strong.reject_randomness ? "YES" : "NO");

    /* 7. Classification */
    int cls_w = rbk_classify_string(weak_prg, n, 65536);
    int cls_s = rbk_classify_string(strong_prg, n, 65536);
    printf("\nClassification (0=random, 1=structured, 2=simple):\n");
    printf("  Weak PRG:   class=%d\n", cls_w);
    printf("  Strong PRG: class=%d\n", cls_s);

    printf("\n=== Summary ===\n");
    printf("Resource-bounded K reveals structural patterns in PRG outputs.\n");
    printf("Incompressibility = pseudorandomness guarantee.\n");

    return 0;
}