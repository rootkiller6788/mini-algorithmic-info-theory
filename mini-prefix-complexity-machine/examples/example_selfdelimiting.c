/*
 * example_selfdelimiting.c — Self-delimiting codes example
 *
 * L1: Prefix-free codes and self-delimiting encoding
 * L3: Kraft inequality, Elias codes
 * L5: Universal integer codes comparison
 *
 * Demonstrates various self-delimiting encodings for integers
 * and strings, comparing their efficiency.
 */
#include "../include/prefix_machine.h"
#include "../include/prefix_codes.h"
#include <stdio.h>
#include <math.h>

static void compare_integer_codes(int n) {
    printf("\n--- Integer n=%d ---\n", n);

    /* Plain binary */
    int bin_len = 0;
    int tmp = n;
    while (tmp > 0) { bin_len++; tmp >>= 1; }
    if (bin_len == 0) bin_len = 1;
    printf("  Plain binary     : %d bits\n", bin_len);

    /* Elias gamma */
    PMString* gc = pm_elias_gamma_encode((size_t)n);
    size_t decoded_g = pm_elias_gamma_decode(gc);
    printf("  Elias gamma      : %zu bits (decoded: %zu)\n", gc->len, decoded_g);
    pmstr_free(gc);

    /* Elias delta */
    PMString* dc = pm_elias_delta_encode((size_t)n);
    size_t decoded_d = pm_elias_delta_decode(dc);
    printf("  Elias delta      : %zu bits (decoded: %zu)\n", dc->len, decoded_d);
    pmstr_free(dc);

    /* Theoretical lengths */
    printf("  Theoretical: gamma=%zu delta=%zu omega=%zu levenshtein=%zu\n",
           elias_gamma_length((size_t)n),
           elias_delta_length((size_t)n),
           elias_omega_length((size_t)n),
           levenshtein_code_length((size_t)n));
}

int main(void) {
    printf("=== Self-Delimiting Codes Example ===\n\n");
    printf("A prefix code ensures no codeword is a prefix of another.\n");
    printf("For integers, this allows variable-length encoding\n");
    printf("without needing to know the length in advance.\n");

    /* ── Integer Code Comparison ──────────────────────────── */
    int test_vals[] = {1, 7, 15, 100, 1000, 1000000, -1};
    for (int i = 0; test_vals[i] >= 0; i++)
        compare_integer_codes(test_vals[i]);

    /* ── String Self-Delimiting ───────────────────────────── */
    printf("\n--- String Self-Delimiting ---\n");
    const char* strings[] = {"a", "ab", "hello", "hello world!", NULL};
    for (int i = 0; strings[i]; i++) {
        PMString* s = pmstr_from_cstr(strings[i]);
        PMString* sd = pm_self_delimiting(s);
        size_t pred_len = pm_self_delimiting_length(s->len);
        printf("  \"%s\": |x|=%zu, |xbar|=%zu (predicted %zu)\n",
               strings[i], s->len, sd->len, pred_len);
        printf("    xbar bits: ");
        for (size_t j = 0; j < sd->len && j < 80; j++)
            putchar(sd->data[j]);
        putchar('\n');
        pmstr_free(s);
        pmstr_free(sd);
    }

    /* ── Kraft Verification ────────────────────────────────── */
    printf("\n--- Kraft Inequality Verification ---\n");
    PMString* codes[4];
    codes[0] = pmstr_from_cstr("0");
    codes[1] = pmstr_from_cstr("10");
    codes[2] = pmstr_from_cstr("110");
    codes[3] = pmstr_from_cstr("111");
    printf("  Set: {0, 10, 110, 111}\n");
    printf("  Prefix-free?  %s\n", pm_is_prefix_free(codes, 4) ? "YES" : "NO");
    printf("  Kraft sum:    %.6f\n", pm_lebesgue_measure(codes, 4));
    printf("  Kraft holds?  %s\n", pm_kraft_check(codes, 4) ? "YES" : "NO");
    for (int i = 0; i < 4; i++) pmstr_free(codes[i]);

    /* ── Golomb Code Table ─────────────────────────────────── */
    printf("\n--- Golomb Code Table (m=4) ---\n");
    int* glens = pc_golomb_lengths(16, 4);
    printf("  n: ");
    for (int n = 0; n < 16; n++) printf("%3d", n);
    printf("\n  l: ");
    for (int n = 0; n < 16; n++) printf("%3d", glens[n]);
    printf("\n");
    double gkraft = 0.0;
    for (int n = 0; n < 16; n++) gkraft += pow(2.0, -(double)glens[n]);
    printf("  Σ 2^{-l} = %.6f (Kraft: %s)\n", gkraft,
           gkraft <= 1.0 + 1e-9 ? "YES" : "NO");
    free(glens);

    printf("\nExample complete.\n");
    return 0;
}
