/*
 * test_prefix_machine.c — Tests for prefix machines and complexity
 *
 * L1-L4: Core prefix complexity definitions and theorems
 */
#include "../include/prefix_machine.h"
#include <stdio.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; } \
    else { printf("FAIL: %s (line %d)\n", msg, __LINE__); } \
} while(0)

int main(void) {
    printf("=== test_prefix_machine ===\n");

    /* ── String API ─────────────────────────────────────── */
    PMString* s = pmstr_from_cstr("hello");
    TEST_ASSERT(s != NULL, "pmstr_from_cstr");
    TEST_ASSERT(s->len == 5, "string length");
    TEST_ASSERT(pmstr_equals(s, s), "string equality self");
    TEST_ASSERT(pmstr_len(s) == 5, "pmstr_len");

    PMString* clone = pmstr_clone(s);
    TEST_ASSERT(pmstr_equals(s, clone), "pmstr_clone equality");
    pmstr_free(clone);

    PMString* empty = pmstr_create(0);
    TEST_ASSERT(empty->len == 0, "pmstr_create empty");
    pmstr_free(empty);

    /* ── Prefix-Free Sets ───────────────────────────────── */
    PMString* pf[4];
    pf[0] = pmstr_from_cstr("0");
    pf[1] = pmstr_from_cstr("10");
    pf[2] = pmstr_from_cstr("110");
    pf[3] = pmstr_from_cstr("111");
    TEST_ASSERT(pm_is_prefix_free(pf, 4) == 1, "prefix-free set {0,10,110,111}");
    for (int i = 0; i < 4; i++) pmstr_free(pf[i]);

    PMString* npf[3];
    npf[0] = pmstr_from_cstr("0");
    npf[1] = pmstr_from_cstr("01");
    npf[2] = pmstr_from_cstr("10");
    TEST_ASSERT(pm_is_prefix_free(npf, 3) == 0, "non-prefix-free {0,01,10}");
    for (int i = 0; i < 3; i++) pmstr_free(npf[i]);

    /* ── Kraft Inequality ───────────────────────────────── */
    PMString* kf[3];
    kf[0] = pmstr_from_cstr("0");
    kf[1] = pmstr_from_cstr("10");
    kf[2] = pmstr_from_cstr("11");
    double ksum = pm_lebesgue_measure(kf, 3);
    TEST_ASSERT(pm_kraft_check(kf, 3) == 1, "Kraft check passes");
    double expected_ksum = pow(2.0, -1) + pow(2.0, -2) + pow(2.0, -2);
    TEST_ASSERT(fabs(ksum - expected_ksum) < 1e-6, "Kraft sum = 1.0");
    for (int i = 0; i < 3; i++) pmstr_free(kf[i]);

    /* ── Prefix Complexity ──────────────────────────────── */
    size_t k_hello = pm_prefix_complexity(s);
    TEST_ASSERT(k_hello > 0, "prefix complexity > 0");
    TEST_ASSERT(k_hello >= s->len, "K(x) >= |x|");

    /* ── Plain to Prefix Gap ────────────────────────────── */
    size_t k_100 = pm_plain_to_prefix_gap(100);
    TEST_ASSERT(k_100 > 100, "K(x) > C(x) for conversion");

    /* ── Self-Delimiting Codes ──────────────────────────── */
    PMString* sd = pm_self_delimiting(s);
    TEST_ASSERT(sd != NULL, "self-delimiting non-null");
    size_t sd_predicted = pm_self_delimiting_length(s->len);
    TEST_ASSERT(sd->len == sd_predicted, "self-delimiting length match");
    TEST_ASSERT(sd->len > s->len, "self-delimiting longer");
    pmstr_free(sd);

    /* ── Elias Gamma ────────────────────────────────────── */
    for (size_t n = 1; n <= 20; n++) {
        PMString* g = pm_elias_gamma_encode(n);
        size_t decoded = pm_elias_gamma_decode(g);
        TEST_ASSERT(decoded == n, "Elias gamma roundtrip");
        pmstr_free(g);
    }

    /* ── Elias Delta ────────────────────────────────────── */
    for (size_t n = 1; n <= 100; n += 5) {
        PMString* d = pm_elias_delta_encode(n);
        size_t decoded = pm_elias_delta_decode(d);
        TEST_ASSERT(decoded == n, "Elias delta roundtrip");
        pmstr_free(d);
    }

    /* ── Monotone Machine ───────────────────────────────── */
    PrefixMachine* pm = pm_create(4, 2, "test-pm");
    TEST_ASSERT(pm != NULL, "pm_create");
    pm_add_transition(pm, 0, 0, 0, 0, 1);
    pm_add_transition(pm, 0, 1, 1, 1, 1);
    int is_mono = pm_is_monotone(pm);
    TEST_ASSERT(is_mono == 0 || is_mono == 1, "pm_is_monotone valid");
    pm_free(pm);

    /* ── Chaitin Omega ──────────────────────────────────── */
    double omega = pm_chaitin_omega_estimate(8, 1000);
    TEST_ASSERT(omega > 0.0, "Omega > 0");
    TEST_ASSERT(omega < 1.0, "Omega < 1");

    /* ── Coding Theorem ─────────────────────────────────── */
    double m_hello = pm_universal_probability(s, 8);
    TEST_ASSERT(m_hello > 0.0, "m(x) > 0");
    double bound = pm_coding_theorem_bound(m_hello);
    TEST_ASSERT(bound > 0.0, "coding theorem bound > 0");

    /* ── Shannon-Fano / Huffman ─────────────────────────── */
    double probs[4] = {0.4, 0.3, 0.2, 0.1};
    int* sf_lens = pm_shannon_fano_lengths(probs, 4);
    TEST_ASSERT(sf_lens != NULL, "SF lengths non-null");
    { double ks = 0.0; for (int i = 0; i < 4; i++) ks += pow(2.0, -(double)sf_lens[i]);
      TEST_ASSERT(ks <= 1.0 + 1e-9, "SF satisfies Kraft"); }
    free(sf_lens);

    int* huf_lens = pm_huffman_lengths(probs, 4);
    TEST_ASSERT(huf_lens != NULL, "Huffman lengths non-null");
    { double ks = 0.0; for (int i = 0; i < 4; i++) ks += pow(2.0, -(double)huf_lens[i]);
      TEST_ASSERT(ks <= 1.0 + 1e-9, "Huffman satisfies Kraft"); }
    free(huf_lens);

    pmstr_free(s);

    printf("\n=== Results: %d / %d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
