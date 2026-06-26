/**
 * example_prediction.c - Solomonoff Prediction Demo
 *
 * Demonstrates next-bit prediction using algorithmic probability.
 * Given a prefix "0101", predicts whether the next bit is more
 * likely 0 or 1, using the universal prior.
 */
#include "solomonoff.h"
#include "prediction.h"
#include <stdio.h>
#include <math.h>

int main(void) {
    printf("=== Solomonoff Next-Bit Prediction ===\n\n");

    const char *test_prefixes[] = {"0", "1", "01", "010", "0101", "000", "111", NULL};

    for (int i = 0; test_prefixes[i]; i++) {
        binary_string_t ctx;
        bs_from_cstr(&ctx, test_prefixes[i]);

        prediction_result_t pred = solomonoff_predict(&ctx, 8, 500000);

        printf("Context: \"%s\" (|x|=%zu)\n", test_prefixes[i], ctx.length);
        printf("  M(x0) = %.6f\n", pred.M_x0);
        printf("  M(x1) = %.6f\n", pred.M_x1);
        printf("  P(next=0 | x) = %.4f\n", pred.prob_next_zero);
        printf("  P(next=1 | x) = %.4f\n", pred.prob_next_one);
        printf("  Confidence = %.4f\n", pred.confidence);

        const char *verdict;
        if (pred.prob_next_zero > pred.prob_next_one + 0.1)
            verdict = "Predict: 0";
        else if (pred.prob_next_one > pred.prob_next_zero + 0.1)
            verdict = "Predict: 1";
        else
            verdict = "Uncertain";
        printf("  Verdict: %s\n\n", verdict);
    }

    printf("Note: M(x) is a semimeasure (sum_x M(x) <= 1).\n");
    printf("The 'missing' probability is assigned to non-halting programs.\n");

    return 0;
}
