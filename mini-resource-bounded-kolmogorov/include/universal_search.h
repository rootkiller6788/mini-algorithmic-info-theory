/**
 * universal_search.h - Levin Universal Search & Optimal Algorithms
 */
#ifndef UNIVERSAL_SEARCH_H
#define UNIVERSAL_SEARCH_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef bool (*SearchFunction)(const char *p, size_t p_len,
                                const char *aux, size_t aux_len,
                                char *output, size_t out_cap,
                                size_t *out_len, size_t steps);

typedef struct {
    char   *best_program;
    size_t  best_program_len;
    size_t  steps_used;
    double  optimality_ratio;
    bool    solution_found;
} LevinResult;

typedef struct {
    size_t  horizon;
    size_t  time_budget;
    double  exploration_rate;
    char   *history;
    size_t  history_len;
} AIXIAgent;

typedef struct {
    char   *string;
    size_t  length;
    double  probability;
    size_t  contributing_progs;
} UniversalSample;

LevinResult levin_search(const char *target, size_t target_len,
                          const char *aux, size_t aux_len,
                          size_t total_steps);
void levin_search_anytime(const char *target, size_t target_len,
                           const char *aux, size_t aux_len,
                           size_t total_budget, size_t step_size,
                           void (*callback)(const LevinResult*, void*),
                           void *user_data);
LevinResult levin_search_inversion(
    bool (*f)(const char*, size_t, char*, size_t, size_t*),
    const char *y, size_t y_len, size_t total_steps);
double levin_optimality_bound(size_t prog_len, size_t running_time);
size_t levin_speedup_np(size_t instance_size, double success_density);
double solomonoff_prior(const char *x, size_t len, size_t time_budget);
double solomonoff_prior_time_bounded(const char *x, size_t len, size_t t);
UniversalSample universal_distribution_sample(size_t max_len);
double universal_distribution_convergence(const char *x, size_t len,
                                           size_t t, size_t t_large);
AIXIAgent* aixt_create(size_t horizon, size_t time_budget,
                        double exploration_rate);
int aixt_choose_action(AIXIAgent *agent, int num_actions);
void aixt_update(AIXIAgent *agent, int action, int observation, double reward);
void aixt_free(AIXIAgent *agent);
double optimal_prediction(const char *history, size_t hist_len,
                           int next_value, size_t time_budget);
void levin_result_free(LevinResult *r);
void levin_result_print(const LevinResult *r);
void universal_sample_free(UniversalSample *s);

#endif
