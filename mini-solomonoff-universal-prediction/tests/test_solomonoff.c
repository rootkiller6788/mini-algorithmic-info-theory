/**
 * test_solomonoff.c - Tests for algorithmic probability M(x)
 */
#include "solomonoff.h"
#include "kolmogorov.h"
#include "enumeration.h"
#include "universal_machine.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;
#define TEST(name) do { tests_run++; printf("  TEST %s... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)

void test_bs_init(void) {
    TEST("bs_init");
    binary_string_t s;
    bs_init(&s);
    assert(s.length == 0);
    PASS();
}

void test_bs_from_cstr(void) {
    TEST("bs_from_cstr");
    binary_string_t s;
    assert(bs_from_cstr(&s, "0101"));
    assert(s.length == 4);
    assert(bs_get_bit(&s, 0) == 0);
    assert(bs_get_bit(&s, 1) == 1);
    assert(bs_get_bit(&s, 2) == 0);
    assert(bs_get_bit(&s, 3) == 1);
    PASS();
}

void test_bs_append_get(void) {
    TEST("bs_append_get");
    binary_string_t s;
    bs_init(&s);
    assert(bs_append_bit(&s, 1));
    assert(bs_append_bit(&s, 0));
    assert(bs_append_bit(&s, 1));
    assert(s.length == 3);
    assert(bs_get_bit(&s, 0) == 1);
    assert(bs_get_bit(&s, 1) == 0);
    assert(bs_get_bit(&s, 2) == 1);
    assert(bs_get_bit(&s, 3) == -1);
    PASS();
}

void test_bs_concat(void) {
    TEST("bs_concat");
    binary_string_t a, b;
    bs_from_cstr(&a, "01");
    bs_from_cstr(&b, "10");
    assert(bs_concat(&a, &b));
    assert(a.length == 4);
    assert(bs_get_bit(&a, 0) == 0);
    assert(bs_get_bit(&a, 1) == 1);
    assert(bs_get_bit(&a, 2) == 1);
    assert(bs_get_bit(&a, 3) == 0);
    PASS();
}

void test_bs_prefix(void) {
    TEST("bs_prefix");
    binary_string_t s, p;
    bs_from_cstr(&s, "010110");
    bs_prefix(&s, 3, &p);
    assert(p.length == 3);
    assert(bs_get_bit(&p, 0) == 0);
    assert(bs_get_bit(&p, 1) == 1);
    assert(bs_get_bit(&p, 2) == 0);
    PASS();
}

void test_bs_has_prefix(void) {
    TEST("bs_has_prefix");
    binary_string_t s, pref;
    bs_from_cstr(&s, "010110");
    bs_from_cstr(&pref, "010");
    assert(bs_has_prefix(&s, &pref));
    bs_from_cstr(&pref, "011");
    assert(!bs_has_prefix(&s, &pref));
    PASS();
}

void test_bs_equal(void) {
    TEST("bs_equal");
    binary_string_t a, b;
    bs_from_cstr(&a, "1010");
    bs_from_cstr(&b, "1010");
    assert(bs_equal(&a, &b));
    bs_from_cstr(&b, "1011");
    assert(!bs_equal(&a, &b));
    PASS();
}

void test_solomonoff_M_empty(void) {
    TEST("solomonoff_M(empty)");
    binary_string_t x;
    bs_init(&x);
    algprob_t m = solomonoff_M(&x, 4, 100000);
    /* M(empty) = 1 (every program's output starts with empty prefix) */
    assert(m >= 0.99);
    PASS();
}

void test_solomonoff_M_single_bit(void) {
    TEST("solomonoff_M(single_bit)");
    binary_string_t x0, x1;
    bs_from_cstr(&x0, "0");
    bs_from_cstr(&x1, "1");
    algprob_t m0 = solomonoff_M(&x0, 6, 500000);
    algprob_t m1 = solomonoff_M(&x1, 6, 500000);
    /* M(0) + M(1) <= 1 (semimeasure) */
    assert(m0 + m1 <= 1.01);
    PASS();
}

void test_semimeasure_property(void) {
    TEST("semimeasure_property");
    double total = solomonoff_verify_semimeasure(2, 6, 200000);
    assert(total <= 1.01);
    PASS();
}

void test_kraft_sum(void) {
    TEST("kraft_sum");
    double ks = solomonoff_kraft_sum(10);
    /* sum_{|p|<=10} 2^{-|p|} = 11 for all binary strings */
    assert(fabs(ks - 11.0) < 0.01);
    PASS();
}

void test_um_self_test(void) {
    TEST("um_self_test");
    assert(um_self_test());
    PASS();
}

void test_levin_Pt(void) {
    TEST("levin_Pt");
    binary_string_t x;
    bs_from_cstr(&x, "0");
    algprob_t pt = levin_Pt(&x, 4, 50000);
    assert(pt >= 0.0 && pt <= 1.0);
    PASS();
}

int main(void) {
    printf("=== Solomonoff Universal Prediction Tests ===\n\n");

    test_bs_init();
    test_bs_from_cstr();
    test_bs_append_get();
    test_bs_concat();
    test_bs_prefix();
    test_bs_has_prefix();
    test_bs_equal();
    test_solomonoff_M_empty();
    test_solomonoff_M_single_bit();
    test_semimeasure_property();
    test_kraft_sum();
    test_um_self_test();
    test_levin_Pt();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
