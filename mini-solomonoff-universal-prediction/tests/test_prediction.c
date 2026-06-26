/**
 * test_prediction.c - Tests for Solomonoff prediction
 */
#include "prediction.h"
#include "kolmogorov.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

static int tests_run = 0, tests_passed = 0;
#define TEST(n) do { tests_run++; printf("  TEST %s... ", n); } while(0)
#define PASS() do { tests_passed++; printf("PASS\n"); } while(0)

void test_predictor_init(void) {
    TEST("predictor_init");
    sequential_predictor_t sp;
    prediction_config_t config = {8, 500000, false, 0.1, 50};
    predictor_init(&sp, &config);
    assert(sp.config.depth == 8);
    assert(sp.observed.length == 0);
    PASS();
}

void test_predict_next_bit_empty(void) {
    TEST("predict_next_bit(empty)");
    prediction_t p = predict_next_bit(NULL, 6, 100000);
    /* NULL context should return zeroed prediction */
    assert(p.prob_0 == 0.0 && p.prob_1 == 0.0);
    PASS();
}

void test_predict_single_bit_context(void) {
    TEST("predict_single_bit_context");
    binary_string_t ctx;
    bs_from_cstr(&ctx, "0");
    prediction_result_t pr = solomonoff_predict(&ctx, 6, 200000);
    assert(pr.prob_next_zero >= 0.0 && pr.prob_next_zero <= 1.0);
    assert(pr.prob_next_one >= 0.0 && pr.prob_next_one <= 1.0);
    assert(fabs(pr.prob_next_zero + pr.prob_next_one - 1.0) < 0.01);
    PASS();
}

void test_sequential_predictor(void) {
    TEST("sequential_predictor");
    sequential_predictor_t sp;
    prediction_config_t config = {6, 200000, false, 0.1, 20};
    predictor_init(&sp, &config);

    /* Observe "010" */
    predictor_observe(&sp, 0);
    predictor_observe(&sp, 1);
    predictor_observe(&sp, 0);

    assert(sp.num_predictions == 3);
    assert(sp.observed.length == 3);
    PASS();
}

void test_predictor_stats(void) {
    TEST("predictor_stats");
    sequential_predictor_t sp;
    prediction_config_t config = {4, 100000, false, 0.5, 10};
    predictor_init(&sp, &config);
    predictor_observe(&sp, 0);
    predictor_observe(&sp, 1);

    double acc, loss;
    size_t total;
    predictor_stats(&sp, &acc, &loss, &total);
    assert(total == 2);
    PASS();
}

void test_model_comparison(void) {
    TEST("model_comparison");
    sequential_predictor_t sp;
    prediction_config_t config = {4, 50000, false, 0.5, 10};
    predictor_init(&sp, &config);
    predictor_observe(&sp, 0);
    predictor_observe(&sp, 0);

    model_comparison_t cmp = compare_to_bernoulli(&sp, 0.5);
    assert(cmp.n_samples == 2);
    PASS();
}

int main(void) {
    printf("=== Solomonoff Prediction Tests ===\n\n");
    test_predictor_init();
    test_predict_next_bit_empty();
    test_predict_single_bit_context();
    test_sequential_predictor();
    test_predictor_stats();
    test_model_comparison();

    printf("\n=== Results: %d/%d tests passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
