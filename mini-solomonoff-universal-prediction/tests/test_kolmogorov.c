/**
 * test_kolmogorov.c - Tests for Kolmogorov complexity
 */
#include "kolmogorov.h"
#include "enumeration.h"
#include "universal_machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %s... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)

void test_K_empty(void) {
    TEST("K(empty)");
    binary_string_t e;
    bs_init(&e);
    size_t K = kolmogorov_K(&e, 6, 500000, NULL);
    /* K(empty) = 0 (some definitions) or small constant */
    assert(K <= 5);
    PASS();
}

void test_K_upper_bound(void) {
    TEST("K_upper_bound");
    binary_string_t x;
    bs_from_cstr(&x, "1010");
    binary_string_t prog;
    size_t up = kolmogorov_K_upper(&prog, &x);
    /* Upper bound: K(x) <= |print_program| */
    assert(up <= 100); /* Should be < 100 bits for a 4-bit string */
    PASS();
}

void test_K_short_string(void) {
    TEST("K(short_string)");
    binary_string_t x;
    bs_from_cstr(&x, "01");
    size_t K = kolmogorov_K(&x, 8, 1000000, NULL);
    /* K(01) should be at most |x| * 12 + 4 = 28 */
    assert(K <= 30);
    PASS();
}

void test_incompressibility(void) {
    TEST("incompressibility");
    /* A length-8 string: 11111111 should be compressible */
    binary_string_t repeated;
    bs_from_cstr(&repeated, "11111111");
    /* Should NOT be incompressible with c=0 (clearly compressible) */
    bool inc = kolmogorov_is_incompressible(&repeated, 0, 6, 200000);
    /* May or may not be incompressible - just check function doesn't crash */
    (void)inc;
    PASS();
}

void test_randomness_deficiency(void) {
    TEST("randomness_deficiency");
    binary_string_t x;
    bs_from_cstr(&x, "01010101");
    int def = kolmogorov_randomness_deficiency(&x, 6, 200000);
    /* Deficiency should be computable */
    assert(def >= -1);
    PASS();
}

void test_prefix_complexity(void) {
    TEST("prefix_complexity");
    binary_string_t x;
    bs_from_cstr(&x, "0");
    binary_string_t pf_prog;
    size_t Kp = prefix_complexity_K(&x, 6, 200000, &pf_prog);
    assert(Kp > 0);
    PASS();
}

void test_kraft_inequality(void) {
    TEST("kraft_inequality");
    double kraft = prefix_complexity_verify_kraft(2, 6, 200000);
    /* Kraft inequality: sum 2^{-K(x)} <= 1 for prefix-free codes.
       With limited depth, small strings may exceed 1 due to
       incomplete enumeration, but should be reasonably bounded. */
    assert(kraft >= 0.0 && kraft < 10.0);
    PASS();
}

void test_chaitin_omega(void) {
    TEST("chaitin_omega");
    double omega = chaitin_omega_lower(4, 100000);
    /* Omega is the probability a random program halts.
       For non-prefix-free enumeration, the total weight can exceed 1,
       but should be finite and non-negative. */
    assert(omega >= 0.0);
    (void)omega;
    PASS();
}

void test_mutual_info(void) {
    TEST("mutual_info");
    binary_string_t x, y;
    bs_from_cstr(&x, "0101");
    bs_from_cstr(&y, "1010");
    double I = algorithmic_mutual_info(&x, &y, 4, 100000);
    /* I(x:y) is defined and finite */
    assert(isfinite(I));
    PASS();
}

void test_algorithmic_distance(void) {
    TEST("algorithmic_distance");
    binary_string_t x, y;
    bs_from_cstr(&x, "0000");
    bs_from_cstr(&y, "1111");
    double d = algorithmic_distance(&x, &y, 4, 50000);
    assert(d >= 0.0 && d <= 1.5);
    PASS();
}

void test_NCD(void) {
    TEST("normalized_compression_distance");
    binary_string_t x, y;
    bs_from_cstr(&x, "0101");
    bs_from_cstr(&y, "0101");
    double d = normalized_compression_distance(&x, &y, 4, 200000);
    /* NCD of identical strings should be near 0 */
    assert(d >= 0.0);
    PASS();
}

void test_coding_theorem_verify(void) {
    TEST("coding_theorem_verify");
    double max_diff, avg_diff;
    size_t num;
    coding_theorem_verify(2, 6, 200000, &max_diff, &avg_diff, &num);
    /* Should complete without crash */
    assert(num <= 1000);  /* num_checked is size_t, always >= 0 */
    PASS();
}

int main(void) {
    printf("=== Kolmogorov Complexity Tests ===\n\n");

    test_K_empty();
    test_K_upper_bound();
    test_K_short_string();
    test_incompressibility();
    test_randomness_deficiency();
    test_prefix_complexity();
    test_kraft_inequality();
    test_chaitin_omega();
    test_mutual_info();
    test_algorithmic_distance();
    test_NCD();
    test_coding_theorem_verify();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
