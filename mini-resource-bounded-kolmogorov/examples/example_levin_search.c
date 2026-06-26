/**
 * example_levin_search.c - Levin Universal Search demo
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/universal_search.h"
#include "../include/resource_bounded_k.h"

static void anytime_callback(const LevinResult *lr, void *user) {
    int *round = (int*)user;
    printf("  Round %d: found=%s, prog_len=%zu, steps=%zu\n",
           *round, lr->solution_found ? "YES" : "NO",
           lr->best_program_len, lr->steps_used);
    (*round)++;
}

int main(void) {
    printf("=== Levin Universal Search Demo ===\n\n");

    /* Search for a target string */
    const char *target = "0101";
    const char *aux = "11";
    printf("Target: \"%s\", Aux input: \"%s\"\n\n", target, aux);

    /* One-shot search */
    printf("One-shot Levin search:\n");
    LevinResult lr = levin_search(target, 4, aux, 2, 50000);
    levin_result_print(&lr);
    levin_result_free(&lr);

    /* Anytime search */
    printf("\nAnytime Levin search:\n");
    int round = 1;
    levin_search_anytime(target, 4, aux, 2, 50000, 10000,
                          anytime_callback, &round);

    /* Optimality bound */
    printf("\nOptimality bound for |p|=4, t=100: %.2f\n",
           levin_optimality_bound(4, 100));

    /* Solomonoff prior */
    printf("\nSolomonoff prior for \"%s\": %.6f\n",
           target, solomonoff_prior(target, 4, 10000));

    /* AIXI agent demo */
    printf("\nAIXI agent demo (3 actions, 5 steps):\n");
    AIXIAgent *agent = aixt_create(5, 1000, 0.2);
    for (int t = 0; t < 5; t++) {
        int action = aixt_choose_action(agent, 3);
        aixt_update(agent, action, t % 2, (action == 1) ? 1.0 : 0.0);
        printf("  Step %d: action=%d\n", t, action);
    }
    aixt_free(agent);

    printf("\nDone.\n");
    return 0;
}