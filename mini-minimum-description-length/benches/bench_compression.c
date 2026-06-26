/*
 * bench_compression.c - MDL Compression Speed Benchmark
 *
 * Benchmarks key MDL operations for performance profiling.
 *
 * Run: make bench
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "../include/mdl_core.h"
#include "../include/nml.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static double wall_time(void) {
    return (double)clock() / CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== MDL Performance Benchmarks ===\n\n");
    double t;

    int n = 10000;
    MDLData* d = mdl_data_create((size_t)n);
    for (int i = 0; i < n; i++)
        mdl_data_append(d, 5.0 + 2.0 * ((double)rand()/RAND_MAX - 0.5));

    t = wall_time();
    double c1 = mdl_two_part_gaussian(d);
    t = wall_time() - t;
    printf("Two-part Gaussian (n=%d):\n", n);
    printf("  Cost: %.4f bits, Time: %.4f sec\n", c1, t);

    t = wall_time();
    double c2 = nml_gaussian_full(d);
    t = wall_time() - t;
    printf("NML Gaussian (n=%d):\n  Cost: %.4f bits, Time: %.4f sec\n", n, c2, t);

    t = wall_time();
    double c3 = mdl_data_mean(d);
    t = wall_time() - t;
    printf("Data mean (n=%d):\n  Value: %.4f, Time: %.6f sec\n", n, c3, t);

    int n_models = 100;
    MDLResult* r = mdl_select_create(n_models);
    t = wall_time();
    for (int i = 0; i < n_models; i++)
        mdl_select_set_cost(r, i, 100.0 + (double)i * 0.5);
    int best = mdl_select_best(r);
    t = wall_time() - t;
    printf("Model selection (%d models):\n", n_models);
    printf("  Best: %d, Time: %.6f sec\n", best, t);
    mdl_select_free(r);

    int n_cp = 100;
    MDLData* cp = mdl_data_create((size_t)n_cp);
    for (int i = 0; i < n_cp/2; i++)
        mdl_data_append(cp, 1.0 + 0.1*((double)rand()/RAND_MAX-0.5));
    for (int i = n_cp/2; i < n_cp; i++)
        mdl_data_append(cp, 5.0 + 0.1*((double)rand()/RAND_MAX-0.5));

    t = wall_time();
    MDLChangePoint* cpt = mdl_changepoint_detect(cp, 3);
    t = wall_time() - t;
    printf("Change point detection (n=%d):\n", n_cp);
    printf("  Found: %d changes, Time: %.4f sec\n",
           cpt ? cpt->n_changes : 0, t);
    mdl_changepoint_free(cpt);

    mdl_data_free(d);
    mdl_data_free(cp);
    printf("\n=== Benchmarks complete ===\n");
    return 0;
}
