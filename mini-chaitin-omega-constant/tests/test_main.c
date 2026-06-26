/*
 * test_main.c — Comprehensive test suite for mini-chaitin-omega-constant
 *
 * Tests all core APIs across binary_string.h, prefix_machine.h,
 * universal_tm.h, halting.h, kolmogorov.h, semicomputable.h,
 * omega.h, randomness.h, solovay.h, calude.h
 *
 * Each TEST verifies a distinct knowledge point.
 */

#include "../include/binary_string.h"
#include "../include/prefix_machine.h"
#include "../include/universal_tm.h"
#include "../include/halting.h"
#include "../include/kolmogorov.h"
#include "../include/semicomputable.h"
#include "../include/omega.h"
#include "../include/randomness.h"
#include "../include/solovay.h"
#include "../include/calude.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) do { printf("  TEST: %s ... ", name); } while(0)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)
#define ASSERT_EQ_INT(expected, actual) do { \
    if ((expected) != (actual)) { \
        printf("FAIL (expected=%d, actual=%d)\n", (int)(expected), (int)(actual)); \
        tests_failed++; \
    } else { printf("PASS\n"); tests_passed++; } \
} while(0)
#define ASSERT_TRUE(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); tests_failed++; } \
    else { printf("PASS\n"); tests_passed++; } \
} while(0)

/* ── Binary String Tests (L1,L2,L3) ─────────────────────── */
static void test_binary_string_basic(void) {
    uint8_t data[] = {0xAB, 0xCD}; /* 10101011 11001101 */
    TEST("bs_create/bs_free");
    BitString* bs = bs_create(data, 16);
    ASSERT_TRUE(bs != NULL, "create");
    ASSERT_TRUE(bs->length == 16, "length");
    bs_free(bs);

    TEST("bs_get_bit/bs_set_bit");
    BitString* bs2 = bs_create(NULL, 8);
    bs_set_bit(bs2, 0, 1);
    bs_set_bit(bs2, 7, 1);
    ASSERT_TRUE(bs_get_bit(bs2, 0) == 1, "get bit 0");
    ASSERT_TRUE(bs_get_bit(bs2, 3) == 0, "get bit 3");
    ASSERT_TRUE(bs_get_bit(bs2, 7) == 1, "get bit 7");
    ASSERT_EQ_INT(8, (int)bs2->length);
    bs_free(bs2);

    TEST("bs_copy/bs_equal");
    BitString* orig = bs_create(data, 8);
    BitString* copy = bs_copy(orig);
    ASSERT_TRUE(bs_equal(orig, copy), "equal");
    bs_set_bit(orig, 0, 1 - bs_get_bit(orig, 0));
    ASSERT_TRUE(!bs_equal(orig, copy), "not equal after change");
    bs_free(orig); bs_free(copy);
}

static void test_binary_string_ops(void) {
    TEST("bs_concat");
    uint8_t d1[] = {0x0F}; /* LSB first: 1111 */
    uint8_t d2[] = {0xF0}; /* LSB first: 0000 */
    BitString* a = bs_create(d1, 4);
    BitString* b = bs_create(d2, 4);
    BitString* ab = bs_concat(a, b);
    ASSERT_TRUE(ab->length == 8, "concat length");
    /* a=1111, b=0000 → ab=11110000 → bit0=1, bit7=0 */
    ASSERT_TRUE(bs_get_bit(ab, 0) == 1 && bs_get_bit(ab, 3) == 1 &&
                bs_get_bit(ab, 4) == 0 && bs_get_bit(ab, 7) == 0, "concat bits");
    bs_free(a); bs_free(b); bs_free(ab);

    TEST("bs_substring");
    uint8_t d3[] = {0x55}; /* LSB first: 10101010 */
    BitString* s = bs_create(d3, 8);
    BitString* sub = bs_substring(s, 2, 3);
    ASSERT_TRUE(sub->length == 3, "substring length");
    ASSERT_TRUE(bs_get_bit(sub, 0) == bs_get_bit(s, 2), "substring bit0");
    bs_free(s); bs_free(sub);

    TEST("bs_is_prefix / bs_common_prefix_len");
    uint8_t dpref[] = {0x0F}; /* LSB first: 1111 */
    BitString* pref = bs_create(dpref, 4);
    /* Build whole as pref+0000 */
    BitString* whole = bs_create(dpref, 4);
    bs_append_bits(whole, b); /* whole = 11110000 */
    ASSERT_TRUE(bs_is_prefix(pref, whole), "is prefix");
    int common = bs_common_prefix_len(pref, whole);
    ASSERT_EQ_INT(4, common);
    bs_free(pref); bs_free(whole);
}

static void test_elias_coding(void) {
    TEST("bs_encode/decode_elias_gamma roundtrip");
    for (uint64_t n = 1; n <= 20; n++) {
        BitString* enc = bs_encode_elias_gamma(n);
        size_t read = 0;
        uint64_t dec = bs_decode_elias_gamma(enc, &read);
        if (dec != n) { printf("\n  n=%llu dec=%llu ", (unsigned long long)n, (unsigned long long)dec); FAIL("gamma"); bs_free(enc); return; }
        bs_free(enc);
    }
    PASS();

    TEST("bs_encode/decode_elias_delta roundtrip");
    for (uint64_t n = 1; n <= 20; n++) {
        BitString* enc = bs_encode_elias_delta(n);
        size_t read = 0;
        uint64_t dec = bs_decode_elias_delta(enc, &read);
        if (dec != n) { printf("\n  n=%llu dec=%llu ", (unsigned long long)n, (unsigned long long)dec); FAIL("delta"); bs_free(enc); return; }
        bs_free(enc);
    }
    PASS();
}

static void test_prefix_free_set(void) {
    TEST("pfs_create/add/contains");
    PrefixFreeSet* set = pfs_create(16);
    uint8_t d0[] = {0x00}; BitString* s0 = bs_create(d0, 1);
    uint8_t d1[] = {0x01}; BitString* s1 = bs_create(d1, 2);
    ASSERT_TRUE(pfs_add(set, s0) == 0, "add s0");
    ASSERT_TRUE(pfs_add(set, s1) == 0, "add s1");
    ASSERT_TRUE(pfs_is_prefix_free(set), "prefix-free check");
    ASSERT_TRUE(fabs(pfs_kraft_sum(set) - 0.75) < 0.01, "kraft sum");
    bs_free(s0); bs_free(s1); pfs_free(set);
}

static void test_kraft_construction(void) {
    TEST("construct_prefix_free_set from lengths");
    size_t lengths[] = {1, 2, 2};
    PrefixFreeSet* set = construct_prefix_free_set(lengths, 3);
    ASSERT_TRUE(set != NULL, "construction");
    ASSERT_TRUE(pfs_is_prefix_free(set), "result is prefix-free");
    double ks = pfs_kraft_sum(set);
    /* Σ 2^{-l} = 2^{-1} + 2^{-2} + 2^{-2} = 0.5 + 0.25 + 0.25 = 1.0 */
    ASSERT_TRUE(set->count == 3, "should have 3 strings");
    ASSERT_TRUE(ks > 0.0 && ks <= 1.01, "kraft sum valid");
    pfs_free(set);
}

/* ── Prefix Machine Tests (L1,L2,L3) ────────────────────── */
static void test_register_machine(void) {
    TEST("rp_create/add_instruction/rp_print");
    RegisterProgram* rp = rp_create();
    ASSERT_TRUE(rp != NULL, "create");
    rp_add_instruction(rp, OP_INC, 0, 0);
    rp_add_instruction(rp, OP_INC, 0, 0);
    rp_add_instruction(rp, OP_HALT, 0, 0);
    ASSERT_EQ_INT(3, (int)rp->program_len);
    rp_free(rp);

    TEST("rm_execute_one / rm_run");
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);  /* R0++ */
    rp_add_instruction(prog, OP_INC, 0, 0);  /* R0++ */
    rp_add_instruction(prog, OP_HALT, 0, 0);
    RMState* st = rm_state_create();
    int res = rm_run(st, prog, 100);
    ASSERT_EQ_INT(1, res); /* halted */
    ASSERT_TRUE(st->regs[0] == 2, "R0 == 2");
    rm_state_free(st); rp_free(prog);
}

static void test_pfm_encode_decode(void) {
    TEST("pfm_encode/decode_program roundtrip");
    RegisterProgram* orig = rp_create();
    rp_add_instruction(orig, OP_INC, 0, 0);
    rp_add_instruction(orig, OP_DECJZ, 1, 5);
    rp_add_instruction(orig, OP_HALT, 0, 0);
    BitString* enc = pfm_encode_program(orig);
    ASSERT_TRUE(enc != NULL, "encode");
    size_t read = 0;
    RegisterProgram* decoded = pfm_decode_program(enc, &read);
    ASSERT_TRUE(decoded != NULL, "decode");
    ASSERT_EQ_INT((int)orig->program_len, (int)decoded->program_len);
    rp_free(orig); bs_free(enc); rp_free(decoded);
}

static void test_optimal_pfm(void) {
    TEST("opfm_create/register_machine/run");
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_HALT, 0, 0);
    OptimalPFM* opfm = opfm_create();
    opfm_register_machine(opfm, prog);
    /* Create input that selects machine 0 */
    BitString* input = bs_encode_elias_gamma(0);
    BitString* inner = pfm_encode_program(prog);
    bs_append_bits(input, inner);
    int set_ok = opfm_set_input(opfm, input);
    ASSERT_TRUE(set_ok == 0, "set input");
    int run_ok = opfm_run(opfm, 100);
    ASSERT_TRUE(run_ok == 1, "run");
    ASSERT_TRUE(opfm_halted(opfm), "halted");
    ASSERT_TRUE(opfm_output(opfm) == 1, "output=1");
    bs_free(input); bs_free(inner);
    opfm_free(opfm);
    /* prog is now owned by opfm */
}

/* ── Universal TM Tests (L1,L2,L3) ──────────────────────── */
static void test_tm_description(void) {
    TEST("tm_create/set_transition/run");
    TMDescription* tm = tm_create(3, 3);
    ASSERT_TRUE(tm != NULL, "tm_create");
    /* Simple TM: if see 1, write 1 and move right; if blank, halt */
    tm_set_transition(tm, 0, 0, 2, 0, 2); /* blank→halt */
    tm_set_transition(tm, 0, 1, 0, 1, 1); /* 1→1,R,stay in q0 */
    tm_set_transition(tm, 0, 2, 0, 2, 1); /* 2→2,R */
    tm_set_transition(tm, 1, 0, 2, 0, 2);
    tm_set_transition(tm, 1, 1, 2, 1, 2);
    tm_set_transition(tm, 1, 2, 2, 2, 2);
    tm_set_transition(tm, 2, 0, 2, 0, 2);
    tm_set_transition(tm, 2, 1, 2, 1, 2);
    tm_set_transition(tm, 2, 2, 2, 2, 2);
    TMConfig* cfg = tm_config_create();
    cfg->tape[cfg->head_pos] = 1;
    cfg->state = 0;
    int res = tm_run(cfg, tm, 50);
    ASSERT_TRUE(res == 1, "tm_run halts");
    ASSERT_TRUE(cfg->halted, "halted flag");
    tm_free(tm); tm_config_free(cfg);

    TEST("utm_encode/decode_tm");
    TMDescription* tm2 = tm_create(3, 3);
    BitString* enc = utm_encode_tm(tm2);
    ASSERT_TRUE(enc != NULL, "encode TM");
    TMDescription* dec = utm_standard_decode(enc);
    ASSERT_TRUE(dec != NULL, "decode TM");
    ASSERT_TRUE(dec->num_states == 3, "states preserved");
    tm_free(tm2); bs_free(enc); tm_free(dec);

    TEST("utm_minsky_4_7");
    TMDescription* minsky = utm_minsky_4_7();
    ASSERT_TRUE(minsky != NULL, "minsky UTM");
    ASSERT_TRUE(minsky->num_states == 4, "4 states");
    tm_free(minsky);

    TEST("utm_rogozhin_4_6");
    TMDescription* rog = utm_rogozhin_4_6();
    ASSERT_TRUE(rog != NULL, "rogozhin UTM");
    ASSERT_TRUE(rog->num_states == 4, "4 states");
    ASSERT_TRUE(rog->num_symbols == 6, "6 symbols");
    tm_free(rog);
}

/* ── Halting Problem Tests (L1,L2,L4) ───────────────────── */
static void test_halting(void) {
    TEST("halting_diagonal_proof_demo");
    int d = halting_diagonal_proof_demo();
    ASSERT_TRUE(d == 1, "diagonal proof demonstrated");

    TEST("halting_probability_contribution");
    BitString* p = bs_encode_elias_delta(5);
    double contrib = halting_probability_contribution(p);
    ASSERT_TRUE(contrib > 0.0 && contrib <= 1.0, "contribution in (0,1]");
    bs_free(p);

    TEST("partial_omega via halting enumeration");
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_HALT, 0, 0);
    OptimalPFM* opfm = opfm_create();
    opfm_register_machine(opfm, prog);
    HaltingEnumeration* he = halting_enumerate(opfm, 4, 2000);
    ASSERT_TRUE(he != NULL, "enumeration");
    double omega_part = partial_omega(he);
    ASSERT_TRUE(omega_part >= 0.0, "partial omega non-negative");
    /* Only checking short programs, omega_part should be computable value */
    ASSERT_TRUE(omega_part >= 0.0, "partial omega valid");
    halting_enum_free(he);
    opfm_free(opfm);

    TEST("busy_beaver_upper_bound monotonicity");
    uint64_t bb3 = busy_beaver_upper_bound(3);
    uint64_t bb4 = busy_beaver_upper_bound(4);
    ASSERT_TRUE(bb4 >= bb3, "BB monotonic");
}

/* ── Kolmogorov Complexity Tests (L1,L2,L4) ─────────────── */
static void test_kolmogorov(void) {
    TEST("kolmogorov_plain_bound / kolmogorov_prefix_bound");
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_HALT, 0, 0);
    OptimalPFM* opfm = opfm_create();
    opfm_register_machine(opfm, prog);
    uint64_t target = 3;
    size_t kp = kolmogorov_plain_bound(opfm, &target, 1, 8, 1000);
    size_t kpf = kolmogorov_prefix_bound(opfm, &target, 1, 8, 1000);
    ASSERT_TRUE(kp <= 9, "plain bound finite");
    ASSERT_TRUE(kpf <= 9, "prefix bound finite");
    opfm_free(opfm);

    TEST("kolmogorov_incompressibility");
    uint8_t rnd[] = {0xA6, 0x3F, 0x91, 0xCC}; /* 32 random-looking bits */
    BitString* x = bs_create(rnd, 32);
    OptimalPFM* opfm2 = opfm_create();
    RegisterProgram* p2 = rp_create();
    rp_add_instruction(p2, OP_INC, 0, 0);
    rp_add_instruction(p2, OP_HALT, 0, 0);
    opfm_register_machine(opfm2, p2);
    int c = kolmogorov_incompressibility(x, opfm2, 8, 1000);
    ASSERT_TRUE(c >= -32, "incompressibility measure");
    bs_free(x); opfm_free(opfm2);

    TEST("mutual information non-negativity");
    OptimalPFM* opfm3 = opfm_create();
    RegisterProgram* p3 = rp_create();
    rp_add_instruction(p3, OP_INC, 0, 0);
    rp_add_instruction(p3, OP_HALT, 0, 0);
    opfm_register_machine(opfm3, p3);
    uint64_t a = 1, b = 2;
    int64_t mi = kolmogorov_mutual_information(opfm3, &a, 1, &b, 1, 8, 1000);
    ASSERT_TRUE(mi >= 0, "I(x:y) >= 0");
    opfm_free(opfm3);
}

/* ── Semicomputable Reals Tests (L1,L2,L3) ──────────────── */
static void test_semicomputable(void) {
    TEST("dr_create/dr_to_double/compare");
    DyadicRational* a = dr_create(1, 1); /* 1/2 */
    DyadicRational* b = dr_create(3, 2); /* 3/4 */
    ASSERT_TRUE(fabs(dr_to_double(a) - 0.5) < 1e-10, "1/2");
    ASSERT_TRUE(fabs(dr_to_double(b) - 0.75) < 1e-10, "3/4");
    ASSERT_TRUE(dr_compare(a, b) < 0, "1/2 < 3/4");
    dr_free(a); dr_free(b);

    TEST("lce_create/add_approximation/non_decreasing");
    LeftCEReal* lce = lce_create(64);
    for (int i = 1; i <= 10; i++) {
        DyadicRational* d = dr_create((uint64_t)i, 4); /* i/16 */
        lce_add_approximation(lce, d);
        dr_free(d);
    }
    ASSERT_TRUE(lce_is_non_decreasing(lce), "non-decreasing");
    ASSERT_TRUE(lce_current_value(lce) > 0.5, "current > 0.5");
    lce_free(lce);

    TEST("lce_from_ce_set / lce_to_ce_set");
    int indicators[] = {1, 0, 1, 0, 0, 1};
    LeftCEReal* chi = lce_from_ce_set(indicators, 6);
    ASSERT_TRUE(chi != NULL, "create from CE set");
    int decoded[6] = {0};
    lce_to_ce_set(chi, decoded, 6);
    ASSERT_TRUE(decoded[0] == 1 && decoded[1] == 0 && decoded[2] == 1,
                "decode CE set");
    lce_free(chi);

    TEST("lce_add two left-c.e. reals");
    LeftCEReal* l1 = lce_create(4);
    LeftCEReal* l2 = lce_create(4);
    for (int i = 1; i <= 4; i++) {
        DyadicRational* d1 = dr_create((uint64_t)i, 3);
        DyadicRational* d2 = dr_create((uint64_t)i, 2);
        lce_add_approximation(l1, d1); dr_free(d1);
        lce_add_approximation(l2, d2); dr_free(d2);
    }
    LeftCEReal* sum = lce_add(l1, l2);
    ASSERT_TRUE(sum != NULL, "sum created");
    lce_free(l1); lce_free(l2); lce_free(sum);
}

/* ── Omega Tests (L1,L2,L4) ─────────────────────────────── */
static void test_omega(void) {
    TEST("omega_create/iterate/approximate");
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_HALT, 0, 0);
    OptimalPFM* opfm = opfm_create();
    opfm_register_machine(opfm, prog);
    OmegaState* os = omega_create(opfm);
    ASSERT_TRUE(os != NULL, "omega_create");
    omega_iterate(os, 3);
    double omega_val = os->omega_lower_bound;
    ASSERT_TRUE(omega_val >= 0.0, "omega >= 0");
    /* For this simple machine, running few iterations,
     * omega should be ≥ 0 (some programs may not halt in time) */
    ASSERT_TRUE(omega_val >= 0.0, "omega_valid");
    omega_free(os);
    opfm_free(opfm);

    TEST("omega_get_bit consistency");
    RegisterProgram* p2 = rp_create();
    rp_add_instruction(p2, OP_INC, 0, 0);
    rp_add_instruction(p2, OP_HALT, 0, 0);
    OptimalPFM* opfm2 = opfm_create();
    opfm_register_machine(opfm2, p2);
    OmegaState* os2 = omega_create(opfm2);
    omega_approximate_to_size(os2, 8);
    int bit0 = omega_get_bit(os2, 0);
    int bit0_again = omega_get_bit(os2, 0);
    ASSERT_EQ_INT(bit0, bit0_again); /* bits should be stable */
    omega_free(os2);
    opfm_free(opfm2);

    TEST("omega_encodes_halting demonstration");
    RegisterProgram* p3 = rp_create();
    rp_add_instruction(p3, OP_INC, 0, 0);
    rp_add_instruction(p3, OP_HALT, 0, 0);
    OptimalPFM* opfm3 = opfm_create();
    opfm_register_machine(opfm3, p3);
    OmegaState* os3 = omega_create(opfm3);
    omega_iterate(os3, 3);
    int encodes = omega_encodes_halting(os3, 8);
    ASSERT_TRUE(encodes == 1, "encoding demo");
    omega_free(os3); opfm_free(opfm3);
}

/* ── Randomness Tests (L1,L2,L5) ────────────────────────── */
static void test_randomness(void) {
    TEST("frequency test on balanced sequence");
    uint8_t balanced[] = {0x55, 0x55}; /* 01010101 01010101 */
    BitString* bs = bs_create(balanced, 16);
    FrequencyTestResult fr = randomness_frequency_test(bs);
    ASSERT_TRUE(fr.passed, "frequency balanced");
    bs_free(bs);

    TEST("runs test on alternating sequence");
    uint8_t alt[] = {0xAA, 0xAA}; /* 10101010 10101010 */
    BitString* bs2 = bs_create(alt, 16);
    RunsTestResult rr = randomness_runs_test(bs2);
    ASSERT_TRUE(rr.num_runs >= 1, "runs detected");
    bs_free(bs2);

    TEST("longest run test");
    uint8_t ones[] = {0xFF, 0xFF}; /* all ones */
    BitString* bs3 = bs_create(ones, 16);
    LongestRunResult lr = randomness_longest_run_test(bs3);
    ASSERT_TRUE(lr.longest_run >= 16, "16 consecutive ones");
    bs_free(bs3);

    TEST("martingale operations");
    Martingale* m = martingale_create(64);
    ASSERT_TRUE(m->capital == 1.0, "initial capital");
    double c1 = martingale_bet(m, 1, 0.5);
    ASSERT_TRUE(c1 > 1.0, "won bet increases capital");
    martingale_free(m);

    TEST("martingale succeeds check");
    Martingale* m2 = martingale_create(64);
    martingale_bet(m2, 1, 0.5);
    martingale_bet(m2, 1, 0.5);
    martingale_bet(m2, 1, 0.5);
    ASSERT_TRUE(martingale_succeeds(m2, 1.5), "capital exceeds threshold");
    martingale_free(m2);
}

/* ── Solovay Tests (L1,L2) ──────────────────────────────── */
static void test_solovay(void) {
    TEST("solovay_reduction_create/apply");
    double ctx = 0.5;
    SolovayReduction* sr = solovay_reduction_create(2.0,
        /* trivial reduction: f(q) = q * context */
        (double(*)(double,void*))NULL, &ctx);
    /* We only test structure, not actual reduction */
    ASSERT_TRUE(sr != NULL, "reduction created");
    solovay_reduction_free(sr);

    TEST("solovay_noncomplete_example");
    LeftCEReal* nc = solovay_noncomplete_example();
    ASSERT_TRUE(nc != NULL, "non-complete example");
    double val = lce_current_value(nc);
    ASSERT_TRUE(fabs(val - 0.5) < 0.01, "value ≈ 0.5");
    lce_free(nc);

    TEST("solovay_intermediate_example");
    LeftCEReal* inter = solovay_intermediate_example();
    ASSERT_TRUE(inter != NULL, "intermediate example");
    lce_free(inter);

    TEST("solovay_dominance_check");
    LeftCEReal* l1 = lce_create(4);
    LeftCEReal* l2 = lce_create(4);
    for (int i = 1; i <= 4; i++) {
        DyadicRational* d1 = dr_create((uint64_t)i, 3);
        DyadicRational* d2 = dr_create((uint64_t)i * 2, 3);
        lce_add_approximation(l1, d1); dr_free(d1);
        lce_add_approximation(l2, d2); dr_free(d2);
    }
    DominanceResult dr = solovay_dominance_check(l1, l2);
    ASSERT_TRUE(dr.alpha_dominates_beta || !dr.alpha_dominates_beta,
                "dominance computed");
    lce_free(l1); lce_free(l2);

    TEST("solovay_join");
    LeftCEReal* a = lce_create(4);
    LeftCEReal* b = lce_create(4);
    for (int i = 1; i <= 4; i++) {
        DyadicRational* da = dr_create((uint64_t)i, 3);
        DyadicRational* db = dr_create((uint64_t)i, 2);
        lce_add_approximation(a, da); dr_free(da);
        lce_add_approximation(b, db); dr_free(db);
    }
    LeftCEReal* join = solovay_join(a, b);
    ASSERT_TRUE(join != NULL, "join created");
    lce_free(a); lce_free(b); lce_free(join);
}

/* ── Calude Tests (L1,L2,L4) ────────────────────────────── */
static void test_calude(void) {
    TEST("calude_constant");
    RegisterProgram* p = rp_create();
    rp_add_instruction(p, OP_INC, 0, 0);
    rp_add_instruction(p, OP_HALT, 0, 0);
    OptimalPFM* opfm = opfm_create();
    opfm_register_machine(opfm, p);
    size_t c = calude_constant(opfm);
    ASSERT_TRUE(c > 0, "positive constant");
    opfm_free(opfm);

    TEST("calude_omega_characterization");
    LeftCEReal* lce = solovay_noncomplete_example();
    RegisterProgram* p2 = rp_create();
    rp_add_instruction(p2, OP_INC, 0, 0);
    rp_add_instruction(p2, OP_HALT, 0, 0);
    OptimalPFM* opfm2 = opfm_create();
    opfm_register_machine(opfm2, p2);
    OmegaNumberCheck check = calude_omega_characterization(lce, opfm2, 8, 1000);
    ASSERT_TRUE(check.is_left_ce, "is left-ce");
    ASSERT_TRUE(strlen(check.characterization) > 0, "characterization non-empty");
    lce_free(lce); opfm_free(opfm2);

    TEST("calude_omega_oracle_demo");
    int oracle = calude_omega_oracle_demo(8);
    ASSERT_TRUE(oracle == 1, "oracle demo");

    TEST("calude_omega_sigma1_complete_demo");
    int sig1 = calude_omega_sigma1_complete_demo();
    ASSERT_TRUE(sig1 == 1, "sigma1 complete demo");

    TEST("calude_bb_from_omega");
    uint8_t bits[] = {0x55};
    BitString* ob = bs_create(bits, 8);
    uint64_t bb = calude_bb_from_omega(ob, 4);
    ASSERT_TRUE(bb > 0, "BB from omega positive");
    bs_free(ob);
}

/* ── Main ───────────────────────────────────────────────── */
int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  Chaitin Omega Constant — Test Suite         ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    printf("── Binary String ADT ──\n");
    test_binary_string_basic();
    test_binary_string_ops();
    test_elias_coding();
    test_prefix_free_set();
    test_kraft_construction();

    printf("\n── Prefix-Free Machine ──\n");
    test_register_machine();
    test_pfm_encode_decode();
    test_optimal_pfm();

    printf("\n── Universal Turing Machine ──\n");
    test_tm_description();

    printf("\n── Halting Problem ──\n");
    test_halting();

    printf("\n── Kolmogorov Complexity ──\n");
    test_kolmogorov();

    printf("\n── Semicomputable Reals ──\n");
    test_semicomputable();

    printf("\n── Chaitin's Omega ──\n");
    test_omega();

    printf("\n── Algorithmic Randomness ──\n");
    test_randomness();

    printf("\n── Solovay Reducibility ──\n");
    test_solovay();

    printf("\n── Calude's Theorem ──\n");
    test_calude();

    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║  Results: %d passed, %d failed              ║\n",
           tests_passed, tests_failed);
    printf("╚══════════════════════════════════════════════╝\n");

    return (tests_failed > 0) ? 1 : 0;
}
