/**
 * universal_search.c - Levin Universal Search & Optimal Algorithms
 *
 * Implements Levin's universal search (LUS): given a problem defined by
 * a computable predicate, Levin search enumerates all programs p in order
 * of increasing |p| * log(time(p)), finding a solution with only a constant
 * factor overhead over the optimal algorithm.
 *
 * Key ideas:
 *   - Total time <= O(2^{|p*|} * t(p*)) where p* is optimal
 *   - Anytime variant: progressively better solutions
 *   - Inversion: given f and y, find x such that f(x) = y
 *   - Solomonoff prior: universal probability distribution
 *   - AIXI agent: optimal RL via Kolmogorov complexity
 *
 * References:
 *   Levin, "Universal Sequential Search Problems" (1973)
 *   Solomonoff, "A Formal Theory of Inductive Inference" (1964)
 *   Hutter, "Universal Artificial Intelligence" (2005)
 *   Li & Vitanyi, "An Introduction to Kolmogorov Complexity", 4th ed. (2019)
 */

#include "../include/universal_search.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==================================================================
 * Internal Helpers
 * ================================================================== */

static size_t ilog2(size_t n) {
    size_t r = 0;
    while (n >>= 1) r++;
    return r;
}

static char* str_dup(const char *src, size_t len) {
    if (!src || len == 0) return NULL;
    char *dst = (char*)malloc(len + 1);
    if (!dst) return NULL;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

/**
 * Simple UTM execution. Binary instruction set:
 *   00 = LEFT, 01 = RIGHT, 10 = FLIP, 11 = HALT.
 * Input is written to the tape before execution begins.
 * Returns length of output written to 'output' buffer.
 */
static size_t utm_execute(const char *program, size_t prog_len,
                           const char *input, size_t input_len,
                           size_t steps, char *output, size_t output_cap) {
    char tape[4096];
    memset(tape, 0, sizeof(tape));
    for (size_t i = 0; i < input_len && i < 4096; i++)
        tape[i] = (input[i] == '1') ? 1 : 0;
    size_t head = input_len;
    size_t step_count = 0, ip = 0;
    while (ip + 1 < prog_len && step_count < steps) {
        char b0 = program[ip], b1 = program[ip + 1];
        ip += 2; step_count++;
        if (b0 == '0' && b1 == '0') { if (head > 0) head--; }
        else if (b0 == '0' && b1 == '1') { head++; if (head >= 4096) head--; }
        else if (b0 == '1' && b1 == '0') { tape[head] = (tape[head] == 0) ? 1 : 0; }
        else { break; }
    }
    size_t out_len = 0;
    for (size_t i = 0; i < 4096 && i < output_cap; i++) {
        output[i] = tape[i] ? '1' : '0';
        out_len++;
    }
    return out_len;
}

/* ==================================================================
 * L2: Levin Universal Search
 *
 * Levin search enumerates programs p_i in order of increasing
 * |p_i| + log(t_i). For a problem with verifier V, Levin search
 * finds a solution using only O(2^{|p*|}) overhead over optimal p*.
 *
 * Lemma (Levin, 1973): If algorithm A solves search in time t(n),
 * then Levin search solves it in time O(2^{|A|} * t(n)).
 * ================================================================== */

LevinResult levin_search(const char *target, size_t target_len,
                          const char *aux, size_t aux_len,
                          size_t total_steps) {
    LevinResult result;
    memset(&result, 0, sizeof(result));
    result.optimality_ratio = 1.0;
    result.solution_found = false;
    if (!target || target_len == 0) return result;

    char output[4096];
    size_t steps_used = 0;
    size_t best_prog_len = (size_t)-1;

    /* Phase-based search: each phase allocates more time per program.
     * Programs up to length 'phase' get 2^phase steps each. */
    for (size_t phase = 1; steps_used < total_steps && phase <= 12; phase++) {
        size_t max_len = phase < 10 ? phase : 10;
        for (size_t plen = 1; plen <= max_len && steps_used < total_steps; plen++) {
            size_t budget_per_prog = ((size_t)1) << phase;
            if (budget_per_prog > 1024) budget_per_prog = 1024;
            size_t count = ((size_t)1) << plen;
            if (count > 128) count = 128;

            for (size_t idx = 0; idx < count && steps_used < total_steps; idx++) {
                char prog[128];
                for (size_t b = 0; b < plen; b++)
                    prog[b] = ((idx >> b) & 1) ? '1' : '0';
                prog[plen] = '\0';

                size_t aux_use = aux_len < 512 ? aux_len : 512;
                size_t out_len = utm_execute(prog, plen, aux, aux_use,
                                              budget_per_prog, output, 4096);
                if (out_len >= target_len) {
                    bool match = true;
                    for (size_t i = 0; i < target_len; i++)
                        if (output[i] != target[i]) { match = false; break; }
                    if (match && plen < best_prog_len) {
                        best_prog_len = plen;
                        free(result.best_program);
                        result.best_program = str_dup(prog, plen);
                        result.best_program_len = plen;
                        result.solution_found = true;
                    }
                }
                steps_used += budget_per_prog;
            }
        }
        steps_used += ((size_t)1) << phase;
    }

    result.steps_used = steps_used;
    if (result.solution_found && best_prog_len > 0 && best_prog_len < (size_t)-1) {
        result.optimality_ratio = (double)steps_used /
            ((double)best_prog_len * (double)(ilog2(steps_used + 1) + 1));
    }
    return result;
}

void levin_search_anytime(const char *target, size_t target_len,
                           const char *aux, size_t aux_len,
                           size_t total_budget, size_t step_size,
                           void (*callback)(const LevinResult*, void*),
                           void *user_data) {
    if (!callback) return;
    LevinResult cumulative;
    memset(&cumulative, 0, sizeof(cumulative));
    cumulative.solution_found = false;
    cumulative.optimality_ratio = 1.0;

    for (size_t budget = step_size; budget <= total_budget; budget += step_size) {
        LevinResult partial = levin_search(target, target_len,
                                            aux, aux_len, budget);
        if (partial.solution_found && (!cumulative.solution_found ||
            partial.best_program_len < cumulative.best_program_len)) {
            free(cumulative.best_program);
            cumulative.best_program = partial.best_program;
            partial.best_program = NULL;
            cumulative.best_program_len = partial.best_program_len;
            cumulative.solution_found = true;
            cumulative.steps_used = partial.steps_used;
        } else {
            levin_result_free(&partial);
        }
        cumulative.steps_used = budget;
        callback(&cumulative, user_data);
    }
    free(cumulative.best_program);
}

LevinResult levin_search_inversion(
    bool (*f)(const char*, size_t, char*, size_t, size_t*),
    const char *y, size_t y_len, size_t total_steps) {
    LevinResult result;
    memset(&result, 0, sizeof(result));
    result.optimality_ratio = 1.0;
    result.solution_found = false;
    if (!f || !y || y_len == 0) return result;

    char output[4096], best_x[256];
    size_t steps_used = 0, best_x_len = 0;
    size_t best_prog_len = (size_t)-1;

    for (size_t phase = 1; steps_used < total_steps && phase <= 12; phase++) {
        size_t max_len = phase < 10 ? phase : 10;
        for (size_t plen = 1; plen <= max_len && steps_used < total_steps; plen++) {
            size_t budget_per_prog = ((size_t)1) << phase;
            if (budget_per_prog > 1024) budget_per_prog = 1024;
            size_t count = ((size_t)1) << plen;
            if (count > 64) count = 64;
            for (size_t idx = 0; idx < count && steps_used < total_steps; idx++) {
                char prog[128];
                for (size_t b = 0; b < plen; b++)
                    prog[b] = ((idx >> b) & 1) ? '1' : '0';
                prog[plen] = '\0';
                size_t out_len = utm_execute(prog, plen, y, y_len,
                                              budget_per_prog, output, 4096);
                size_t check_len = 0;
                char check_output[4096];
                if (f(output, out_len, check_output, 4096, &check_len)) {
                    bool match = true;
                    if (check_len >= y_len) {
                        for (size_t i = 0; i < y_len; i++)
                            if (check_output[i] != y[i]) { match = false; break; }
                    } else { match = false; }
                    if (match && plen < best_prog_len) {
                        best_prog_len = plen;
                        size_t cp = out_len < 256 ? out_len : 255;
                        memcpy(best_x, output, cp);
                        best_x_len = cp; best_x[best_x_len] = '\0';
                        result.solution_found = true;
                    }
                }
                steps_used += budget_per_prog;
            }
        }
    }
    if (result.solution_found) {
        result.best_program = str_dup(best_x, best_x_len);
        result.best_program_len = best_x_len;
    }
    result.steps_used = steps_used;
    return result;
}

double levin_optimality_bound(size_t prog_len, size_t running_time) {
    /* Optimality ratio bound: total time <= (2^{prog_len} / d) * running_time
     * where d is the success density of programs that solve the problem.
     * Returns the multiplicative factor. */
    if (prog_len == 0 || running_time == 0) return 1.0;
    return pow(2.0, (double)prog_len) / (double)running_time;
}

size_t levin_speedup_np(size_t instance_size, double success_density) {
    /* Levin's speedup for NP search problems:
     * If fraction d of programs solve an NP problem, Levin search
     * runs in O(2^n / d) time vs O(2^n) brute force.
     * Returns estimated steps. */
    if (success_density <= 0.0 || instance_size == 0) return 0;
    double brute = pow(2.0, (double)instance_size);
    double speedup = brute / success_density;
    return (size_t)(speedup < brute ? speedup : brute);
}

/* ==================================================================
 * L4-L5: Solomonoff Prior & Universal Distribution
 *
 * Solomonoff's universal prior: m(x) = sum_{p: U(p)=x} 2^{-|p|}
 * This is the universal a priori probability that a random program
 * produces x. It dominates all computable semimeasures.
 *
 * Solomonoff's completeness theorem: For any computable measure mu,
 * m(x) >= c * mu(x) for some constant c > 0 (depending on mu).
 * ================================================================== */

double solomonoff_prior(const char *x, size_t len, size_t time_budget) {
    /* m(x) = sum_{p: U(p)=x} 2^{-|p|} approximated with time budget.
     * Enumerate programs up to given length, run each for time_budget
     * steps, accumulate 2^{-|p|} for those that output x. */
    if (!x || len == 0) return 0.0;
    double m = 0.0;
    char output[4096];
    for (size_t plen = 1; plen <= 64 && plen <= len + 8; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 256) count = 256;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++)
                prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t out_len = utm_execute(prog, plen, NULL, 0,
                                          time_budget, output, 4096);
            bool match = (out_len >= len);
            if (match) {
                for (size_t i = 0; i < len; i++)
                    if (output[i] != x[i]) { match = false; break; }
            }
            if (match) m += pow(2.0, -(double)plen);
        }
    }
    return m;
}

double solomonoff_prior_time_bounded(const char *x, size_t len, size_t t) {
    /* time-bounded Solomonoff prior with explicit bound t per program.
     * Identical to solomonoff_prior with time_budget = t. Kept separate
     * for API clarity. */
    return solomonoff_prior(x, len, t);
}

UniversalSample universal_distribution_sample(size_t max_len) {
    /* Sample from the universal distribution by generating random
     * programs and running them. Returns a sample string with its
     * probability estimate.
     *
     * Process: enumerate programs up to max_len, weight each by 2^{-|p|},
     * normalize to a probability distribution, sample one. */
    UniversalSample sample;
    memset(&sample, 0, sizeof(sample));
    if (max_len == 0 || max_len > 10) max_len = 8;

    double total_weight = 0.0;
    char output[4096];
    /* First pass: compute total weight */
    for (size_t plen = 1; plen <= max_len; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 64) count = 64;
        total_weight += (double)count * pow(2.0, -(double)plen);
    }
    if (total_weight <= 0.0) return sample;

    /* Sample using weighted random selection */
    double r = (double)rand() / (double)RAND_MAX * total_weight;
    double cumulative = 0.0;
    for (size_t plen = 1; plen <= max_len; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 64) count = 64;
        double w = pow(2.0, -(double)plen);
        for (size_t idx = 0; idx < count; idx++) {
            cumulative += w;
            if (cumulative >= r) {
                char prog[128];
                for (size_t b = 0; b < plen; b++)
                    prog[b] = ((idx >> b) & 1) ? '1' : '0';
                prog[plen] = '\0';
                size_t out_len = utm_execute(prog, plen, NULL, 0,
                                              1024, output, 4096);
                sample.string = str_dup(output, out_len);
                sample.length = out_len;
                sample.probability = w / total_weight;
                sample.contributing_progs = 1;
                return sample;
            }
        }
    }
    return sample;
}

double universal_distribution_convergence(const char *x, size_t len,
                                           size_t t, size_t t_large) {
    /* Measures convergence rate: |m^{t_large}(x) - m^t(x)|.
     * As t increases, the approximation approaches true m(x).
     * Returns absolute difference. */
    double m_t = solomonoff_prior(x, len, t);
    double m_t_large = solomonoff_prior(x, len, t_large);
    return fabs(m_t_large - m_t);
}

/* ==================================================================
 * L7: AIXI Agent — Optimal Reinforcement Learning
 *
 * AIXI is the optimal agent for general reinforcement learning
 * based on Solomonoff induction and sequential decision theory.
 *
 * AIXI at step k chooses action a_k that maximizes:
 *   sum_{future} [reward(future) * m(history + action + observation)]
 *
 * where m is the Solomonoff prior (universal distribution).
 *
 * Reference: Hutter, "Universal Artificial Intelligence" (2005)
 * Complexity: Incomputable in general; approximations are essential.
 * ================================================================== */

AIXIAgent* aixt_create(size_t horizon, size_t time_budget,
                        double exploration_rate) {
    AIXIAgent *agent = (AIXIAgent*)malloc(sizeof(AIXIAgent));
    if (!agent) return NULL;
    agent->horizon = horizon;
    agent->time_budget = time_budget;
    agent->exploration_rate = exploration_rate;
    agent->history = NULL;
    agent->history_len = 0;
    return agent;
}

int aixt_choose_action(AIXIAgent *agent, int num_actions) {
    /* AIXI action selection:
     * For each action a, estimate future expected reward using
     * Solomonoff prediction on (history + a + anticipated observation).
     * Choose action maximizing expected reward.
     * This is an approximation — true AIXI is incomputable. */
    if (!agent || num_actions <= 0) return 0;

    int best_action = 0;
    double best_value = -1e9;

    for (int a = 0; a < num_actions; a++) {
        /* Approximate: use Solomonoff prior to predict if this action
         * leads to favorable outcomes. Simple heuristic: actions that
         * have worked before (in history) get higher value. */
        double value = 0.0;

        /* Exploration bonus (randomized, scale by exploration_rate) */
        value += ((double)rand() / (double)RAND_MAX) * agent->exploration_rate;

        /* Exploitation: Solomonoff-based prediction.
         * Encode (history || action) as a string and estimate m(history||action)
         * as a proxy for the likelihood of good outcomes. */
        char buf[512];
        size_t buf_len = 0;
        if (agent->history) {
            size_t cp = agent->history_len < 256 ? agent->history_len : 256;
            memcpy(buf, agent->history, cp);
            buf_len = cp;
        }
        buf[buf_len++] = (char)('0' + a);
        double prior = solomonoff_prior(buf, buf_len, agent->time_budget);
        value += prior * 10.0; /* scale for practical use */

        if (value > best_value) {
            best_value = value;
            best_action = a;
        }
    }
    return best_action;
}

void aixt_update(AIXIAgent *agent, int action, int observation, double reward) {
    /* Update the agent's history with (action, observation, reward).
     * The history grows as: h_{t+1} = h_t || (a_t, o_t, r_t) */
    if (!agent) return;
    char entry[64];
    int entry_len = snprintf(entry, sizeof(entry), "a%d_o%d_r%.2f|",
                             action, observation, reward);
    if (entry_len <= 0) return;

    char *new_hist = (char*)malloc(agent->history_len + (size_t)entry_len + 1);
    if (!new_hist) return;
    if (agent->history) {
        memcpy(new_hist, agent->history, agent->history_len);
        free(agent->history);
    }
    memcpy(new_hist + agent->history_len, entry, (size_t)entry_len);
    agent->history_len += (size_t)entry_len;
    new_hist[agent->history_len] = '\0';
    agent->history = new_hist;
}

void aixt_free(AIXIAgent *agent) {
    if (agent) {
        free(agent->history);
        free(agent);
    }
}

double optimal_prediction(const char *history, size_t hist_len,
                           int next_value, size_t time_budget) {
    /* Solomonoff prediction: P(next=1 | history) = m(history·1) / m(history)
     * where m is the universal distribution.
     *
     * Returns estimated probability of next_value given history.
     * This is the optimal predictor — no computable predictor
     * asymptotically outperforms it. */
    if (!history) return 0.5;

    /* Build history + prediction */
    char full[512];
    size_t cp = hist_len < 256 ? hist_len : 256;
    memcpy(full, history, cp);
    full[cp] = (char)('0' + next_value);
    size_t full_len = cp + 1;

    double m_hist = solomonoff_prior(history, cp, time_budget);
    double m_full = solomonoff_prior(full, full_len, time_budget);

    if (m_hist <= 0.0) return 0.5;
    double prob = m_full / m_hist;
    return (prob > 1.0) ? 1.0 : ((prob < 0.0) ? 0.0 : prob);
}

/* ==================================================================
 * Utility Functions
 * ================================================================== */

void levin_result_free(LevinResult *r) {
    if (r && r->best_program) {
        free(r->best_program);
        r->best_program = NULL;
    }
}

void levin_result_print(const LevinResult *r) {
    if (!r) return;
    printf("Levin Result: found=%s, prog_len=%zu, steps=%zu, ratio=%.2f\n",
           r->solution_found ? "YES" : "NO",
           r->best_program_len, r->steps_used, r->optimality_ratio);
    if (r->best_program) printf("  program=\"%s\"\n", r->best_program);
}

void universal_sample_free(UniversalSample *s) {
    if (s && s->string) {
        free(s->string);
        s->string = NULL;
    }
}
