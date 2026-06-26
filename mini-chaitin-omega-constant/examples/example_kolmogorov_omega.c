/*
 * example_kolmogorov_omega.c — Kolmogorov complexity and Ω relationship
 *
 * L6: Demonstrates the relationship between Kolmogorov complexity K(x),
 * algorithmic probability P(x), and Chaitin's Ω constant.
 *
 * Key relationships demonstrated:
 *  - Coding Theorem: K(x) ≈ -log₂ P(x)
 *  - Ω = Σ_x P_U(x) (total algorithmic probability)
 *  - Incompressibility and randomness
 *  - Mutual information between strings
 */

#include "../include/binary_string.h"
#include "../include/prefix_machine.h"
#include "../include/kolmogorov.h"
#include "../include/omega.h"
#include "../include/halting.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  Kolmogorov Complexity ↔ Omega Relationship ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    /* Create a prefix-free machine */
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_HALT, 0, 0);

    OptimalPFM* machine = opfm_create();
    opfm_register_machine(machine, prog);

    /* ── Compute K(x) for various strings ── */
    printf("── Kolmogorov Complexity Upper Bounds ──\n");
    printf("  (Searching programs up to length 8, step limit 1000)\n\n");

    uint64_t targets[] = {0, 1, 2, 3, 7, 15, 31};
    const char* names[] = {"empty", "1", "2", "3", "7", "15", "31"};

    for (int i = 0; i < 7; i++) {
        size_t ka = kolmogorov_plain_bound(machine, &targets[i], 1, 8, 1000);
        size_t kb = kolmogorov_prefix_bound(machine, &targets[i], 1, 8, 1000);
        printf("  K_plain(%s) ≤ %zu, K_prefix(%s) ≤ %zu\n",
               names[i], ka, names[i], kb);
    }

    /* ── Coding Theorem Verification ── */
    printf("\n── Coding Theorem: K(x) ≈ -log₂ P(x) ──\n");
    printf("  Theorem (Levin 1974): K(x) = -log₂ P_U(x) + O(1)\n\n");

    for (int i = 0; i < 5; i++) {
        CodingTheoremResult ct = coding_theorem_verify(
            machine, &targets[i], 1, 8, 1000);
        printf("  x=%-4s  P(x)=%.6f  -log₂P(x)=%.3f  K(x)=%zu  "
               "diff=%.3f  hold=%s\n",
               names[i], ct.p_x, ct.neg_log2_p, ct.k_x,
               ct.difference, ct.coding_theorem_holds ? "✓" : "✗");
    }

    /* ── Mutual Information ── */
    printf("\n── Algorithmic Mutual Information I(x:y) ──\n");
    printf("  I(x:y) = K(x) + K(y) - K(x,y) + O(log)\n\n");

    struct { uint64_t x; uint64_t y; const char* xn; const char* yn; } pairs[] = {
        {1, 2, "1", "2"},
        {1, 1, "1", "1"},    /* identical */
        {3, 7, "3", "7"},
    };
    for (int i = 0; i < 3; i++) {
        int64_t mi = kolmogorov_mutual_information(
            machine, &pairs[i].x, 1, &pairs[i].y, 1, 8, 1000);
        printf("  I(%s : %s) = %lld\n", pairs[i].xn, pairs[i].yn,
               (long long)mi);
    }

    /* ── Omega and Algorithmic Probability ── */
    printf("\n── Ω equals total algorithmic probability ──\n");
    printf("  Ω = Σ_x P_U(x) = Σ_{p: U(p) halts} 2^{-|p|}\n\n");

    OmegaState* os = omega_create(machine);
    omega_approximate_to_size(os, 6);

    /* Compute P(x) for small x via enumeration */
    double total_prob = 0.0;
    printf("  Individual algorithmic probabilities:\n");
    for (int i = 0; i < 4; i++) {
        double px = algorithmic_probability(machine, &targets[i], 1, 6, 1000);
        total_prob += px;
        printf("    P(%s) ≈ %.6f\n", names[i], px);
    }
    printf("  Σ P(x) [first 4] ≈ %.6f\n", total_prob);
    printf("  Ω (from halting) ≈ %.6f\n", os->omega_lower_bound);
    printf("  Ω - ΣP ≈ %.6f (difference due to unenumerated outputs)\n",
           os->omega_lower_bound - total_prob);

    /* ── Incompressibility Analysis ── */
    printf("\n── Incompressibility Analysis ──\n");
    printf("  Most strings of length n have K(x) ≥ n - O(1)\n");

    /* Test several bitstrings */
    const char* test_patterns[] = {
        "0000000000000000", /* highly compressible */
        "0101010101010101", /* pattern — compressible */
        "0110100110010110", /* 16 bits */
        "1100101000111010", /* 16 bits */
    };
    for (int i = 0; i < 4; i++) {
        size_t len = strlen(test_patterns[i]);
        BitString* bs = bs_create(NULL, len);
        for (size_t j = 0; j < len; j++)
            bs_set_bit(bs, j, test_patterns[i][j] - '0');
        bs->length = len;

        int c_incomp = kolmogorov_incompressibility(bs, machine, 8, 1000);
        printf("  x=%-16s  |x|=%zu  c-incompressible: c=%d (%s)\n",
               test_patterns[i], len, c_incomp,
               (c_incomp >= 0) ? "appears random" : "compressible");
        bs_free(bs);
    }

    /* ── Algorithmic Sufficient Statistic ── */
    printf("\n── Algorithmic Sufficient Statistic ──\n");
    uint8_t pattern[] = {0xAA, 0xAA}; /* 10101010 10101010 */
    BitString* x = bs_create(pattern, 16);
    AlgorithmicStatistic* as = algorithmic_sufficient_statistic(
        machine, x, 8, 1000);
    if (as && as->model) {
        printf("  String: ");
        bs_print(x);
        printf("  K(model) ≤ %zu, sufficient: %s\n",
               as->model_k,
               as->is_sufficient ? "YES ✓" : "NO ✗");
    }
    as_free(as);
    bs_free(x);

    /* Cleanup */
    omega_free(os);
    opfm_free(machine);

    printf("\nDone.\n");
    return 0;
}
