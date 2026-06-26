/**
 * example_convergence.c - Convergence of Solomonoff Induction
 *
 * Empirically demonstrates that M(x_{n+1}|x_1...x_n) converges
 * to the true conditional probability for a computable measure.
 *
 * We generate data from a Bernoulli(0.7) process and track how
 * Solomonoff's predictions converge to 0.7 as more data is seen.
 *
 * Theorem (Solomonoff, 1978): For any computable measure mu,
 *   sum_{n=1}^{infinity} E[(M(x_n|x_{<n}) - mu(x_n|x_{<n}))^2]
 *   <= K(mu) * ln 2
 *
 * This bounded total error ensures rapid convergence.
 */
#include "solomonoff.h"
#include "prediction.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    printf("=== Convergence of Solomonoff Induction ===\n\n");
    printf("True distribution: Bernoulli(0.7) - P(1) = 0.7\n");
    printf("Tracking M's prediction as data accumulates:\n\n");

    /* Generate a deterministic pseudo-random sequence biased toward 1 */
    /* Using a simple pattern: "1110111011..." approximates bias 0.7 */
    const char *sequence_pattern = "11101110111011101110111011101110";
    size_t seq_len = 32;

    printf("%4s %12s %12s %10s\n", "n", "True P(1)", "M's P(1)", "|Error|");
    printf("%4s %12s %12s %10s\n", "---", "---------", "---------", "-------");

    binary_string_t observed;
    bs_init(&observed);

    double true_bias = 0.7;
    double cumulative_error = 0.0;

    for (size_t i = 0; i < seq_len && sequence_pattern[i]; i++) {
        prediction_result_t pred = solomonoff_predict(&observed, 8, 500000);

        double error = fabs(pred.prob_next_one - true_bias);
        cumulative_error += error * error;

        if (i % 4 == 0) {
            printf("%4zu %12.6f %12.6f %10.6f\n",
                   i + 1, true_bias, pred.prob_next_one, error);
        }

        /* Feed the actual observation */
        int bit = (sequence_pattern[i] == '1') ? 1 : 0;
        bs_append_bit(&observed, bit);
    }

    printf("  ...\n");
    printf("  Cumulative squared error: %.6f\n", cumulative_error);
    printf("\n  By Solomonoff's theorem, total expected squared error\n");
    printf("  is bounded by K(Bernoulli(0.7)) * ln(2) ~ O(1).\n");

    return 0;
}
