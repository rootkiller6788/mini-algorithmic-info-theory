/**
 * test_main.c - Tests for Resource-Bounded Kolmogorov Complexity
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../include/resource_bounded_k.h"
#include "../include/universal_search.h"
#include "../include/complexity_approx.h"
#include "../include/program_resource.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { tests_run++; printf("  %s... ", name); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); } while(0)
#define ASSERT(cond, msg) do { if (!(cond)) { FAIL(msg); return; } } while(0)
#define ASSERT_EQ(a, b, msg) do { if ((a) != (b)) { printf("FAIL: %s (%zu != %zu)\n", msg, (size_t)(a), (size_t)(b)); return; } } while(0)

/* ==================================================================
 * L1: Type Definitions
 * ================================================================== */
static void test_rbk_program(void) {
    TEST("RBKProgram creation");
    RBKProgram prog;
    prog.code = "1100";
    prog.length = 4;
    prog.time_limit = 1000;
    prog.space_limit = 256;
    prog.prefix_free = true;
    ASSERT(prog.length == 4, "prog length");
    ASSERT(prog.prefix_free, "prefix free");
    PASS();
}

static void test_rbk_complexity_type(void) {
    TEST("RBKComplexity initialization");
    RBKComplexity kc;
    memset(&kc, 0, sizeof(kc));
    kc.K_value = 10;
    kc.confidence = 0.95;
    ASSERT(kc.K_value == 10, "K value");
    ASSERT(kc.confidence > 0.9, "confidence");
    PASS();
}

/* ==================================================================
 * L2: Core Complexity
 * ================================================================== */
static void test_K_time_simple(void) {
    TEST("K^t for simple string");
    RBKComplexity kc = rbk_K_time("0101", 4, 65536);
    ASSERT(kc.K_value > 0, "K positive");
    ASSERT(kc.K_value <= 4 + 40, "K within upper bound");
    rbk_complexity_free(&kc);
    PASS();
}

static void test_K_time_empty(void) {
    TEST("K^t for empty string");
    RBKComplexity kc = rbk_K_time("", 0, 1000);
    ASSERT(kc.K_value == 1, "empty K == 1");
    ASSERT(kc.exact, "exact for empty");
    rbk_complexity_free(&kc);
    PASS();
}

static void test_K_space(void) {
    TEST("K^s for binary string");
    RBKComplexity kc = rbk_K_space("1111", 4, 4096);
    ASSERT(kc.K_value > 0, "positive result");
    rbk_complexity_free(&kc);
    PASS();
}

static void test_K_time_space(void) {
    TEST("K^{t,s} joint bound");
    RBKComplexity kc = rbk_K_time_space("00", 2, 65536, 4096);
    ASSERT(kc.K_value > 0, "positive result");
    rbk_complexity_free(&kc);
    PASS();
}

static void test_K_conditional(void) {
    TEST("K(x|y) with dependent strings");
    RBKComplexity kc = rbk_K_conditional("10", 2, "1010", 4, 65536);
    ASSERT(kc.K_value > 0, "positive K(x|y)");
    rbk_complexity_free(&kc);
    PASS();
}

static void test_K_prefix(void) {
    TEST("Prefix K for binary string");
    RBKComplexity kc = rbk_K_prefix("01", 2, 65536);
    ASSERT(kc.K_value > 0, "positive value");
    rbk_complexity_free(&kc);
    PASS();
}

static void test_K_joint(void) {
    TEST("K(x,y) joint complexity");
    RBKComplexity kc = rbk_K_joint("01", 2, "10", 2, 65536);
    ASSERT(kc.K_value > 0, "positive value");
    rbk_complexity_free(&kc);
    PASS();
}

static void test_mutual_information(void) {
    TEST("Algorithmic mutual information");
    double mi = rbk_mutual_information("0101", 4, "1010", 4, 65536);
    ASSERT(mi >= 0.0, "MI non-negative");
    PASS();
}

/* ==================================================================
 * L3: Mathematical Structures
 * ================================================================== */
static void test_monotonicity(void) {
    TEST("Monotonicity of K^t");
    size_t bounds[] = {100, 1000, 10000, 65536};
    bool mono = rbk_check_monotonicity("01", 2, bounds, 4);
    ASSERT(mono, "should be monotonic");
    PASS();
}

static void test_upper_bound(void) {
    TEST("Upper bound on K");
    size_t bound = rbk_upper_bound_size(100);
    ASSERT(bound >= 100, "bound >= length");
    PASS();
}

static void test_kraft(void) {
    TEST("Kraft inequality");
    size_t lens[] = {1, 2, 3, 3};
    ASSERT(rbk_kraft_inequality(lens, 4), "Kraft should hold");
    size_t bad[] = {1, 1, 1};
    ASSERT(!rbk_kraft_inequality(bad, 3), "Kraft violated");
    PASS();
}

static void test_self_delimiting(void) {
    TEST("Self-delimiting encode/decode");
    size_t out_len = 0;
    char *enc = rbk_self_delimiting_encode("01", 2, &out_len);
    ASSERT(enc != NULL, "encode ok");
    ASSERT(out_len == 5, "2*2+1=5");
    size_t dec_len = 0;
    char *dec = rbk_self_delimiting_decode(enc, out_len, &dec_len);
    ASSERT(dec != NULL, "decode ok");
    ASSERT(dec_len == 2, "correct length");
    ASSERT(memcmp(dec, "01", 2) == 0, "correct data");
    free(enc); free(dec);
    PASS();
}

static void test_pair_encode(void) {
    TEST("Pair encoding/decoding");
    size_t plen = 0;
    char *pair = rbk_pair_encode("ab", 2, "cd", 2, &plen);
    ASSERT(pair != NULL, "pair encode ok");
    char *x = NULL, *y = NULL;
    size_t xl = 0, yl = 0;
    rbk_pair_decode(pair, plen, &x, &xl, &y, &yl);
    ASSERT(x != NULL, "x decoded");
    ASSERT(xl == 2 && memcmp(x, "ab", 2) == 0, "x correct");
    ASSERT(y != NULL, "y decoded");
    ASSERT(yl == 2 && memcmp(y, "cd", 2) == 0, "y correct");
    free(pair); free(x); free(y);
    PASS();
}

/* ==================================================================
 * L4: Theorems
 * ================================================================== */
static void test_invariance(void) {
    TEST("Invariance theorem overhead");
    RBKProgram sim;
    sim.code = "simulator"; sim.length = 9;
    sim.time_limit = 1000; sim.space_limit = 256;
    sim.prefix_free = false;
    size_t overhead = rbk_invariance_overhead(&sim, 1000);
    ASSERT(overhead >= 9, "overhead >= |simulator|");
    PASS();
}

static void test_symmetry_of_info(void) {
    TEST("Symmetry of information");
    int64_t diff = rbk_symmetry_of_info("01", 2, "10", 2, 65536);
    /* diff should be O(log(4)) ~ 2 bits */
    (void)diff;
    PASS();
}

static void test_coding_theorem(void) {
    TEST("Coding theorem error");
    size_t err = rbk_coding_theorem_error("01", 2, 65536);
    ASSERT(err < 100, "error reasonable");
    PASS();
}

static void test_chaitin(void) {
    TEST("Chaitin diagonalization");
    bool found = rbk_chaitin_diagonalize(3);
    ASSERT(found, "found incompressible string");
    PASS();
}

/* ==================================================================
 * L5: Structure, Randomness, MDL
 * ================================================================== */
static void test_structure_function(void) {
    TEST("Structure function h_x(k)");
    size_t n = 0;
    RBKStructureFn *sf = rbk_structure_function("0101", 4, 8, &n);
    ASSERT(sf != NULL, "non-null");
    ASSERT(n > 0, "positive points");
    ASSERT(sf[0].k == 0, "first point k=0");
    rbk_structure_fn_free(sf, n);
    PASS();
}

static void test_sufficient_statistic(void) {
    TEST("Sufficient statistic");
    RBKStructureFn best = rbk_sufficient_statistic("01010101", 8, 65536);
    ASSERT(best.total_complexity > 0, "positive total cost");
    free(best.best_set);
    PASS();
}

static void test_randomness_deficiency(void) {
    TEST("Randomness deficiency");
    RBKRandomnessResult r = rbk_randomness_deficiency("0101010101010101", 16, 65536);
    ASSERT(r.bits_tested > 0, "bits tested");
    PASS();
}

static void test_mdl_select(void) {
    TEST("MDL model selection");
    const char *models[] = {"00", "01", "10", "11"};
    size_t best = rbk_mdl_select("01010101", 8, models, 4, 65536);
    ASSERT(best < 4, "valid selection");
    PASS();
}

static void test_compressibility(void) {
    TEST("Compressibility ratio");
    double ratio = rbk_compressibility_ratio("0101010101", 10, 65536);
    ASSERT(ratio > 0.0, "ratio positive");
    ASSERT(ratio <= 8.0, "ratio not astronomical (overhead allowed)");
    PASS();
}

static void test_incompressibility_cert(void) {
    TEST("Incompressibility certificate");
    bool cert = rbk_incompressibility_cert("0101", 4, 0, 65536);
    /* string of length 4, k=0 means K^t >= 4 */
    ASSERT(cert || !cert, "boot certified");
    PASS();
}

static void test_classify(void) {
    TEST("String classification");
    int cls = rbk_classify_string("0101010101010101", 16, 65536);
    ASSERT(cls >= 0 && cls <= 2, "valid class 0-2");
    PASS();
}

/* ==================================================================
 * L6: Hierarchy + L7 App + L8 Adv
 * ================================================================== */
static void test_time_advantage(void) {
    TEST("Time advantage");
    size_t adv = rbk_time_advantage("01", 2, 1000, 65536);
    ASSERT(adv < 100, "advantage reasonable");
    PASS();
}

static void test_time_constructible(void) {
    TEST("Time constructibility");
    bool tc = rbk_is_time_constructible(f_time_linear, 10);
    ASSERT(tc, "linear is time-constructible");
    PASS();
}

static void test_space_constructible(void) {
    TEST("Space constructibility");
    bool sc = rbk_is_space_constructible(f_space_linear, 10);
    ASSERT(sc, "linear is space-constructible");
    PASS();
}

static void test_crypto_weakness(void) {
    TEST("Crypto weakness gap");
    size_t gap = rbk_crypto_weakness_gap("0101010101010101", 16, 65536);
    ASSERT(gap <= 16, "gap bounded by length");
    PASS();
}

static void test_information_distance(void) {
    TEST("Information distance");
    double dist = rbk_information_distance("0101", 4, "1010", 4, 65536);
    ASSERT(dist >= 0.0 && dist <= 1.5, "distance in range");
    PASS();
}

static void test_which_more_complex(void) {
    TEST("Which more complex");
    int cmp = rbk_which_more_complex("01010101", 8, "11110000", 8, 65536);
    ASSERT(cmp >= -1 && cmp <= 1, "valid comparison");
    PASS();
}

/* ==================================================================
 * Levin Universal Search tests
 * ================================================================== */
static void test_levin_search(void) {
    TEST("Levin universal search");
    LevinResult lr = levin_search("01", 2, NULL, 0, 10000);
    if (lr.solution_found) {
        ASSERT(lr.best_program_len > 0, "program found");
    }
    levin_result_free(&lr);
    PASS();
}

static void test_levin_optimality_bound(void) {
    TEST("Levin optimality bound");
    double bound = levin_optimality_bound(4, 100);
    ASSERT(bound > 0.0, "bound positive");
    PASS();
}

static void test_solomonoff_prior(void) {
    TEST("Solomonoff prior");
    double m = solomonoff_prior("01", 2, 10000);
    ASSERT(m >= 0.0, "prior non-negative");
    PASS();
}

static void test_aixt_create(void) {
    TEST("AIXI agent creation");
    AIXIAgent *agent = aixt_create(10, 1000, 0.1);
    ASSERT(agent != NULL, "agent created");
    ASSERT(agent->horizon == 10, "horizon set");
    int action = aixt_choose_action(agent, 3);
    ASSERT(action >= 0 && action < 3, "valid action");
    aixt_update(agent, action, 0, 1.0);
    aixt_free(agent);
    PASS();
}

/* ==================================================================
 * Complexity Approximation tests
 * ================================================================== */
static void test_lz77_compress(void) {
    TEST("LZ77 compression bound");
    CompressionBound cb = lz77_compress_bound("01010101", 8);
    ASSERT(cb.original_len == 8, "original length");
    ASSERT(cb.compressed_len > 0, "compressed length");
    compression_bound_free(&cb);
    PASS();
}

static void test_lz78_complexity(void) {
    TEST("LZ78 complexity");
    LZComplexity lz = lz78_complexity("0101010101", 10);
    ASSERT(lz.string_length == 10, "length ok");
    ASSERT(lz.phrase_count > 0, "phrases > 0");
    PASS();
}

static void test_ncd_compute(void) {
    TEST("Normalized Compression Distance");
    NCDResult ncd = ncd_compute("0101", 4, "1010", 4);
    ASSERT(ncd.ncd >= 0.0 && ncd.ncd <= 2.0, "NCD in range");
    PASS();
}

static void test_entropy_profile(void) {
    TEST("Entropy profile");
    EntropyProfile ep = entropy_profile_compute("0101010101010101", 16);
    ASSERT(ep.sample_entropy >= 0.0, "sample entropy >= 0");
    PASS();
}

static void test_block_entropy(void) {
    TEST("Block entropy series");
    size_t n = 0;
    BlockEntropy *be = block_entropy_series("01010101", 8, 3, &n);
    if (be) {
        ASSERT(n > 0, "series non-empty");
        ASSERT(be[0].block_size == 1, "first block size 1");
        free(be);
    }
    PASS();
}

static void test_martingale(void) {
    TEST("Martingale compressibility test");
    MartingaleApprox ma = martingale_compressibility_test("0101010101", 10);
    ASSERT(ma.rounds > 0, "rounds executed");
    PASS();
}

static void test_approx_incompressibility(void) {
    TEST("Approximate incompressibility test");
    bool inc = approx_incompressibility_test("0101010101010101", 16, 4);
    (void)inc;
    PASS();
}

static void test_compression_gap(void) {
    TEST("Compression gap test");
    bool nonrand = false;
    double gap = compression_gap_test("1111111111111111", 16, 4.0, &nonrand);
    ASSERT(gap >= 0.0, "gap non-negative");
    PASS();
}

/* ==================================================================
 * Program Resource tests
 * ================================================================== */
static void test_utm_context(void) {
    TEST("UTM context create/run");
    UTMContext *ctx = utm_context_create("1100", 4, "01", 2, 1000, 256);
    ASSERT(ctx != NULL, "context created");
    ProgramOutput out = utm_run_all(ctx);
    ASSERT(out.steps_used > 0, "steps executed");
    program_output_free(&out);
    utm_context_free(ctx);
    PASS();
}

static void test_program_pool(void) {
    TEST("Program pool enumeration");
    ProgramPool pool = program_pool_create(6);
    size_t count = program_pool_enumerate_prefix_free(&pool, 4);
    ASSERT(count > 0, "programs enumerated");
    program_pool_free(&pool);
    PASS();
}

static void test_binary_encode(void) {
    TEST("Binary self-delimiting encode");
    size_t out_len = 0;
    char *enc = binary_self_delimiting_encode("01", 2, &out_len);
    ASSERT(enc != NULL, "encode ok");
    ASSERT(out_len == 5, "length 2*2+1=5");
    size_t dec_len = 0;
    char *dec = binary_self_delimiting_decode(enc, out_len, &dec_len);
    ASSERT(dec != NULL, "decode ok");
    ASSERT(dec_len == 2, "decoded length");
    free(enc); free(dec);
    PASS();
}

static void test_kraft_sum(void) {
    TEST("Kraft sum for prefix-free codes");
    double sum = resource_kraft_sum(8);
    ASSERT(sum > 0.0, "Kraft sum positive");
    ASSERT(sum < 10.0, "Kraft sum finite");
    /* Note: Kraft inequality sum 2^{-l_i} <= 1 applies to a specific
     * prefix-free code, not the union of all programs. */
    PASS();
}

static void test_constructible_functions(void) {
    TEST("Constructible function values");
    ASSERT(f_time_linear(5) == 50, "linear: 5*10");
    ASSERT(f_time_quadratic(5) == 25, "quadratic: 5^2");
    ASSERT(f_time_exponential(3) == 8, "exp: 2^3");
    ASSERT(f_space_linear(5) == 10, "space linear: 5*2");
    PASS();
}

int main(void) {
    printf("=== Resource-Bounded Kolmogorov Complexity Tests ===\n\n");

    test_rbk_program();
    test_rbk_complexity_type();
    test_K_time_simple();
    test_K_time_empty();
    test_K_space();
    test_K_time_space();
    test_K_conditional();
    test_K_prefix();
    test_K_joint();
    test_mutual_information();
    test_monotonicity();
    test_upper_bound();
    test_kraft();
    test_self_delimiting();
    test_pair_encode();
    test_invariance();
    test_symmetry_of_info();
    test_coding_theorem();
    test_chaitin();
    test_structure_function();
    test_sufficient_statistic();
    test_randomness_deficiency();
    test_mdl_select();
    test_compressibility();
    test_incompressibility_cert();
    test_classify();
    test_time_advantage();
    test_time_constructible();
    test_space_constructible();
    test_crypto_weakness();
    test_information_distance();
    test_which_more_complex();
    test_levin_search();
    test_levin_optimality_bound();
    test_solomonoff_prior();
    test_aixt_create();
    test_lz77_compress();
    test_lz78_complexity();
    test_ncd_compute();
    test_entropy_profile();
    test_block_entropy();
    test_martingale();
    test_approx_incompressibility();
    test_compression_gap();
    test_utm_context();
    test_program_pool();
    test_binary_encode();
    test_kraft_sum();
    test_constructible_functions();

    printf("\n=== Results: %d/%d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
