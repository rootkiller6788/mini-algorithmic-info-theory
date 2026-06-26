/*
 * example_randomness_testing.c — Statistical and algorithmic randomness tests
 *
 * L6: Applies Martin-Löf randomness tests and statistical tests
 * (frequency, runs, block frequency, longest run) to binary sequences.
 * Demonstrates the difference between computable statistical tests
 * and the universal Martin-Löf test.
 *
 * Also demonstrates that Ω bits pass all statistical tests,
 * confirming (empirically) their algorithmic randomness.
 */

#include "../include/binary_string.h"
#include "../include/randomness.h"
#include "../include/omega.h"
#include "../include/prefix_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Print binary string in groups of 8 for readability */
static void print_pretty_bits(const BitString* bs) {
    if (!bs) return;
    for (size_t i = 0; i < bs->length; i++) {
        printf("%d", bs_get_bit(bs, i));
        if ((i + 1) % 8 == 0 && i + 1 < bs->length) printf(" ");
    }
}

int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  Algorithmic Randomness Testing Suite        ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    /* ── Test 1: Known random-like sequence ── */
    printf("── Test 1: Balanced random-like sequence ──\n");
    uint8_t rnd_data[] = {0x5A, 0x3C, 0x7F, 0xE1, 0x99, 0xAA, 0x55, 0x0F};
    BitString* seq1 = bs_create(rnd_data, 64);
    printf("Sequence (%zu bits): ", seq1->length);
    print_pretty_bits(seq1);
    printf("\n");

    FrequencyTestResult freq = randomness_frequency_test(seq1);
    printf("  Frequency test: ones=%zu/%zu, p=%.4f → %s\n",
           freq.ones, freq.total, freq.p_value,
           freq.passed ? "PASS ✓" : "FAIL ✗");

    RunsTestResult runs = randomness_runs_test(seq1);
    printf("  Runs test: %zu runs (expected≈%.1f), p=%.4f → %s\n",
           runs.num_runs, runs.expected_runs, runs.p_value,
           runs.passed ? "PASS ✓" : "FAIL ✗");

    LongestRunResult lr = randomness_longest_run_test(seq1);
    printf("  Longest run of 1s: %zu, p=%.4f → %s\n",
           lr.longest_run, lr.p_value,
           lr.passed ? "PASS ✓" : "FAIL ✗");

    BlockFrequencyResult bf = randomness_block_frequency_test(seq1, 8);
    printf("  Block frequency (8-bit blocks): χ²=%.2f, p=%.4f → %s\n",
           bf.chi_squared, bf.p_value,
           bf.passed ? "PASS ✓" : "FAIL ✗");
    free(bf.block_frequencies);
    bs_free(seq1);

    /* ── Test 2: Highly non-random sequence ── */
    printf("\n── Test 2: Highly non-random sequence (all zeros) ──\n");
    BitString* seq2 = bs_create(NULL, 64);
    for (size_t i = 0; i < 64; i++) bs_set_bit(seq2, i, 0);
    seq2->length = 64;
    printf("Sequence (%zu bits): ", seq2->length);
    print_pretty_bits(seq2);
    printf("\n");

    freq = randomness_frequency_test(seq2);
    printf("  Frequency test: ones=%zu/%zu, p=%.4f → %s\n",
           freq.ones, freq.total, freq.p_value,
           freq.passed ? "PASS ✓" : "FAIL ✗");

    runs = randomness_runs_test(seq2);
    printf("  Runs test: %zu runs (expected≈%.1f), p=%.4f → %s\n",
           runs.num_runs, runs.expected_runs, runs.p_value,
           runs.passed ? "PASS ✓" : "FAIL ✗");

    bs_free(seq2);

    /* ── Test 3: Omega bits randomness ── */
    printf("\n── Test 3: Randomness of Omega bits ──\n");
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_HALT, 0, 0);
    OptimalPFM* machine = opfm_create();
    opfm_register_machine(machine, prog);
    OmegaState* os = omega_create(machine);
    omega_approximate_to_size(os, 8);

    BitString* omega_bits = omega_get_bits(os, 16);
    if (omega_bits) {
        printf("Ω↾16: ");
        print_pretty_bits(omega_bits);
        printf("\n");

        freq = randomness_frequency_test(omega_bits);
        printf("  Frequency test: %s\n", freq.passed ? "PASS ✓" : "FAIL ✗");

        runs = randomness_runs_test(omega_bits);
        printf("  Runs test: %s\n", runs.passed ? "PASS ✓" : "FAIL ✗");

        lr = randomness_longest_run_test(omega_bits);
        printf("  Longest run test: %s\n", lr.passed ? "PASS ✓" : "FAIL ✗");

        bs_free(omega_bits);
    }

    /* ── Test 4: Martingale strategy ── */
    printf("\n── Test 4: Martingale betting simulation ──\n");
    uint8_t mart_data[] = {0xFF, 0xFF, 0xFF, 0xFF}; /* predictable */
    BitString* seq4 = bs_create(mart_data, 32);

    Martingale* mart = martingale_create(64);
    printf("  Initial capital: $%.2f\n", mart->capital);
    printf("  Betting on predictable sequence:\n");
    for (size_t i = 0; i < seq4->length; i++) {
        int bit = bs_get_bit(seq4, i);
        double prev = mart->capital;
        martingale_bet(mart, bit, 0.3);
        if (i % 8 == 0 && i > 0) {
            printf("    After %zu bits: $%.2f → $%.2f\n",
                   i + 1, prev, mart->capital);
        }
    }
    printf("  Final capital: $%.2f\n", mart->capital);
    printf("  Martingale %s\n",
           martingale_succeeds(mart, 2.0) ? "SUCCEEDS (threshold 2.0)" : "FAILS");
    martingale_free(mart);
    bs_free(seq4);

    /* Cleanup */
    omega_free(os);
    opfm_free(machine);

    printf("\nDone.\n");
    return 0;
}
