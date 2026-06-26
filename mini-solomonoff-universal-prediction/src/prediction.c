/**
 * prediction.c - Solomonoff Universal Prediction Implementation
 *
 * Implements sequential prediction using algorithmic probability:
 *
 * P(x_{n+1} = b | x_1...x_n) = M(x_1...x_n b) / sum_{c} M(x_1...x_n c)
 *
 * Key results:
 *   1. M predicts any computable measure mu with rapid convergence.
 *   2. The total number of errors is bounded by O(K(mu)).
 *   3. The cumulative log loss is close to optimal.
 *
 * Reference: Solomonoff (1978), Hutter (2005), Li & Vitanyi (2019).
 * Curriculum: MIT 6.841, CMU 15-751, Cambridge Part III.
 */

#include "prediction.h"
#include "enumeration.h"
#include "universal_machine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ================================================================
 * Predictor State Management
 * ================================================================ */

void predictor_init(sequential_predictor_t *sp, const prediction_config_t *config) {
    if (!sp) return;
    memset(sp, 0, sizeof(*sp));
    if (config) {
        sp->config = *config;
    } else {
        sp->config.depth = 12;
        sp->config.max_steps = 1000000;
        sp->config.use_prefix_free = false;
        sp->config.confidence_threshold = 0.1;
        sp->config.top_k = 100;
    }
    bs_init(&sp->observed);
}

void predictor_reset(sequential_predictor_t *sp) {
    if (!sp) return;
    bs_init(&sp->observed);
    memset(&sp->last_prediction, 0, sizeof(sp->last_prediction));
    sp->num_predictions = 0;
    sp->num_correct = 0;
    sp->cumulative_loss = 0.0;
    sp->running_M = 1.0;
}

/**
 * predictor_predict_next - Predict next bit using current context.
 *
 * Core prediction routine:
 *   1. Compute M(observed·0) and M(observed·1)
 *   2. Normalize to get probabilities
 *   3. Select most probable bit if confidence exceeds threshold
 *
 * The prediction is stored in sp->last_prediction and returned.
 */
prediction_t predictor_predict_next(sequential_predictor_t *sp) {
    prediction_t pred;
    memset(&pred, 0, sizeof(pred));

    if (!sp) return pred;

    /* Get algorithmic probability for both continuations */
    binary_string_t x0, x1;
    bs_init(&x0);
    bs_init(&x1);
    bs_concat(&x0, &sp->observed);
    bs_append_bit(&x0, 0);
    bs_concat(&x1, &sp->observed);
    bs_append_bit(&x1, 1);

    algprob_t M0 = solomonoff_M(&x0, sp->config.depth, sp->config.max_steps);
    algprob_t M1 = solomonoff_M(&x1, sp->config.depth, sp->config.max_steps);

    double total = M0 + M1;
    if (total < SOLOMONOFF_EPSILON) {
        pred.prob_0 = 0.5;
        pred.prob_1 = 0.5;
        pred.predicted_bit = -1;
        pred.confidence = 0.0;
    } else {
        pred.prob_0 = M0 / total;
        pred.prob_1 = M1 / total;
        double diff = fabs(pred.prob_0 - pred.prob_1);
        pred.confidence = diff;

        if (diff >= sp->config.confidence_threshold) {
            pred.predicted_bit = (pred.prob_1 > pred.prob_0) ? 1 : 0;
        } else {
            pred.predicted_bit = -1;  /* Uncertain */
        }
    }

    pred.num_programs_used = 0;  /* Not tracked in simple mode */
    pred.total_prob_mass = total;
    pred.steps_used = 0;
    pred.convergence_estimate = (sp->num_predictions > 0) ?
        fabs(pred.prob_1 - 0.5) / sqrt((double)sp->num_predictions + 1.0) : 0.5;

    sp->last_prediction = pred;
    return pred;
}

/**
 * predictor_observe - Feed an observed bit and update running M.
 *
 * After observing bit b, we update:
 *   - observed sequence grows by one bit
 *   - running_M shrinks (roughly M(observed) decreases as sequence grows)
 *   - cumulative_loss accumulates -log P(b|context)
 *   - prediction accuracy statistics update
 */
void predictor_observe(sequential_predictor_t *sp, int bit) {
    if (!sp || (bit != 0 && bit != 1)) return;

    /* Update accuracy */
    if (sp->last_prediction.predicted_bit >= 0) {
        if (sp->last_prediction.predicted_bit == bit) sp->num_correct++;
    }

    /* Update log loss */
    double prob_bit = (bit == 1) ? sp->last_prediction.prob_1 : sp->last_prediction.prob_0;
    if (prob_bit > SOLOMONOFF_EPSILON) {
        sp->cumulative_loss += -log2(prob_bit);
    } else {
        sp->cumulative_loss += 10.0;  /* Penalty for zero-probability event */
    }

    /* Extend observed sequence */
    bs_append_bit(&sp->observed, bit);
    sp->num_predictions++;

    /* Update running M */
    sp->running_M = solomonoff_M(&sp->observed, sp->config.depth, sp->config.max_steps);
}

void predictor_predict_k(sequential_predictor_t *sp, size_t k,
                          double *prob_sequence, size_t seq_len) {
    if (!sp || !prob_sequence || seq_len < k) return;

    binary_string_t sim = sp->observed;
    for (size_t i = 0; i < k && i < seq_len; i++) {
        binary_string_t x0, x1;
        bs_init(&x0); bs_init(&x1);
        bs_concat(&x0, &sim); bs_append_bit(&x0, 0);
        bs_concat(&x1, &sim); bs_append_bit(&x1, 1);

        algprob_t M0 = solomonoff_M(&x0, sp->config.depth, sp->config.max_steps);
        algprob_t M1 = solomonoff_M(&x1, sp->config.depth, sp->config.max_steps);
        double total = M0 + M1;

        if (total < SOLOMONOFF_EPSILON)
            prob_sequence[i] = 0.5;
        else
            prob_sequence[i] = M1 / total;

        /* Assume most likely continuation for simulation */
        int guess = (M1 >= M0) ? 1 : 0;
        bs_append_bit(&sim, guess);
    }
}

double predictor_current_M(const sequential_predictor_t *sp) {
    return sp ? sp->running_M : 0.0;
}

void predictor_stats(const sequential_predictor_t *sp,
                     double *accuracy, double *avg_log_loss,
                     size_t *total_predictions) {
    if (!sp) return;
    if (accuracy) {
        *accuracy = (sp->num_predictions > 0) ?
            (double)sp->num_correct / (double)sp->num_predictions : 0.0;
    }
    if (avg_log_loss) {
        *avg_log_loss = (sp->num_predictions > 0) ?
            sp->cumulative_loss / (double)sp->num_predictions : 0.0;
    }
    if (total_predictions) *total_predictions = sp->num_predictions;
}

/* ================================================================
 * Batch Prediction
 * ================================================================ */

/**
 * predict_next_bit - One-shot prediction for arbitrary context.
 *
 * Given a binary string context, predict the next bit using
 * algorithmic probability with the specified depth and step limits.
 */
prediction_t predict_next_bit(const binary_string_t *context,
                               size_t depth, uint64_t max_steps) {
    prediction_t pred;
    memset(&pred, 0, sizeof(pred));

    if (!context) return pred;

    prediction_result_t result = solomonoff_predict(context, depth, max_steps);
    pred.prob_0 = result.prob_next_zero;
    pred.prob_1 = result.prob_next_one;
    pred.confidence = result.confidence;

    if (pred.confidence > 0.1)
        pred.predicted_bit = (pred.prob_1 > pred.prob_0) ? 1 : 0;
    else
        pred.predicted_bit = -1;

    pred.total_prob_mass = result.M_x0 + result.M_x1;
    pred.convergence_estimate = 1.0 / sqrt((double)(context->length + 1));

    return pred;
}

/**
 * predict_continuation - Predict multiple future bits.
 *
 * Greedy multi-step prediction: at each step, predict the most
 * probable next bit, then use it as if observed for the next step.
 *
 * This gives the "most algorithmically plausible" continuation.
 * For optimal multi-step prediction, one should sum over all
 * 2^k possible continuations weighted by M, but that is
 * exponentially expensive.
 */
size_t predict_continuation(const binary_string_t *prefix,
                             size_t max_additional_bits,
                             int *continuation, size_t cont_capacity,
                             size_t depth, uint64_t max_steps) {
    if (!prefix || !continuation || cont_capacity < max_additional_bits)
        return 0;

    binary_string_t ctx;
    bs_init(&ctx);
    bs_concat(&ctx, prefix);

    for (size_t k = 0; k < max_additional_bits; k++) {
        prediction_result_t pred = solomonoff_predict(&ctx, depth, max_steps);
        int next_bit = (pred.prob_next_one >= pred.prob_next_zero) ? 1 : 0;
        continuation[k] = next_bit;
        bs_append_bit(&ctx, next_bit);
    }
    return max_additional_bits;
}

/* ================================================================
 * Model Comparison
 * ================================================================ */

model_comparison_t compare_to_bernoulli(const sequential_predictor_t *sp,
                                         double bernoulli_p) {
    model_comparison_t cmp;
    memset(&cmp, 0, sizeof(cmp));

    if (!sp || bernoulli_p < 0.0 || bernoulli_p > 1.0) return cmp;

    cmp.n_samples = sp->num_predictions;
    cmp.M_log_loss = sp->cumulative_loss;

    /* Compute Bernoulli log loss: -sum log P(x_i) = -[count_1*log(p) + count_0*log(1-p)] */
    double log_p = log2(bernoulli_p);
    double log_1mp = log2(1.0 - bernoulli_p);

    if (bernoulli_p <= 0.0 || bernoulli_p >= 1.0) {
        /* Degenerate Bernoulli: infinite loss on mismatch */
        cmp.baseline_log_loss = 1e10;
        cmp.relative_improvement = 1e10;
        return cmp;
    }

    /* Count 1s in observed sequence */
    size_t count_1 = bs_popcount(&sp->observed);
    size_t count_0 = sp->observed.length - count_1;

    cmp.baseline_log_loss = -(count_1 * log_p + count_0 * log_1mp);
    cmp.relative_improvement = cmp.baseline_log_loss - cmp.M_log_loss;

    return cmp;
}

/* ================================================================
 * Convergence Tracking
 * ================================================================ */

size_t convergence_study(const binary_string_t *sequence,
                          double true_bias,
                          size_t min_depth, size_t max_depth, size_t step,
                          uint64_t max_steps,
                          convergence_record_t *records, size_t max_records) {
    if (!sequence || !records || max_records == 0) return 0;

    size_t count = 0;
    for (size_t d = min_depth; d <= max_depth && count < max_records; d += step) {
        /* Use the last bit of sequence as target, rest as context */
        binary_string_t context;
        bs_prefix(sequence, sequence->length > 0 ? sequence->length - 1 : 0, &context);

        prediction_result_t pred = solomonoff_predict(&context, d, max_steps);

        records[count].depth = d;
        records[count].predicted_prob = pred.prob_next_one;
        records[count].true_prob = true_bias;
        records[count].abs_error = fabs(pred.prob_next_one - true_bias);
        records[count].squared_error = records[count].abs_error * records[count].abs_error;
        count++;
    }
    return count;
}

/* ================================================================
 * Ensemble Analysis
 * ================================================================ */

/**
 * prediction_ensemble - Collect programs that contribute to prediction.
 *
 * For each program up to depth whose output matches the context,
 * determine whether it predicts next bit = 0 or 1.
 *
 * This reveals the "voting" behavior of the algorithmic ensemble:
 * which programs (theories) favor which continuation.
 */
size_t prediction_ensemble(const binary_string_t *context,
                            size_t depth, uint64_t max_steps,
                            ensemble_member_t *members, size_t max_members) {
    if (!context || !members || max_members == 0) return 0;

    dovetail_manager_t dm;
    size_t pool_cap = (depth <= 10) ? (1U << depth) : SOLOMONOFF_POOL_SIZE;
    dovetail_init(&dm, pool_cap, max_steps);

    program_enumerator_t enumerator;
    enumerator_init(&enumerator, depth);

    binary_string_t prog;
    size_t plen;
    size_t added = 0;
    while (enumerator_next(&enumerator, &prog, &plen) && added < pool_cap) {
        dovetail_add_program(&dm, &prog, plen);
        added++;
    }
    dovetail_run(&dm, depth, max_steps);

    size_t count = 0;
    for (size_t j = 0; j < dm.pool_size && count < max_members; j++) {
        pool_entry_t *entry = &dm.pool[j];
        if (!entry->halted) continue;

        /* Check if output has context as prefix */
        if (bs_has_prefix(&entry->machine.output, context)) {
            members[count].program = entry->program;
            members[count].length = entry->prog_length;
            members[count].weight = entry->weight;
            members[count].output = entry->machine.output;

            /* Check what the program predicts for the next bit */
            if (entry->machine.output.length > context->length) {
                int next_bit = bs_get_bit(&entry->machine.output, context->length);
                members[count].produces_0 = (next_bit == 0);
                members[count].produces_1 = (next_bit == 1);
            } else {
                members[count].produces_0 = false;
                members[count].produces_1 = false;
            }

            members[count].contribution = entry->weight;
            count++;
        }
    }

    dovetail_free(&dm);
    return count;
}
