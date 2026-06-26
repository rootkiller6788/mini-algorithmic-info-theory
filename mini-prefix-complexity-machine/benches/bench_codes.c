#include "../include/prefix_machine.h"
#include "../include/prefix_codes.h"
#include <stdio.h>
#include <time.h>

static double get_time_sec(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== Benchmark: Prefix Codes ===\n\n");

    /* Huffman construction benchmarks */
    int sizes[] = {10, 50, 100, 200};
    for (int si = 0; si < 4; si++) {
        int n = sizes[si];
        double* probs = (double*)malloc((size_t)n * sizeof(double));
        double sum = 0.0;
        for (int i = 0; i < n; i++) {
            probs[i] = 1.0 / (double)(i + 1);
            sum += probs[i];
        }
        for (int i = 0; i < n; i++) probs[i] /= sum;

        double t0 = get_time_sec();
        int* lens = huffman_lengths_compute(probs, n);
        double t1 = get_time_sec();
        printf("  Huffman n=%4d: %.6f ms\n", n, (t1 - t0) * 1000.0);
        free(lens);
        free(probs);
    }

    /* Shannon-Fano benchmarks */
    printf("\n");
    for (int si = 0; si < 4; si++) {
        int n = sizes[si] * 10;
        double* probs = (double*)malloc((size_t)n * sizeof(double));
        for (int i = 0; i < n; i++) probs[i] = 1.0 / (double)n;
        double t0 = get_time_sec();
        int* lens = shannon_fano_lengths(probs, n);
        double t1 = get_time_sec();
        printf("  Shannon-Fano n=%4d: %.6f ms\n", n, (t1 - t0) * 1000.0);
        free(lens); free(probs);
    }

    /* Kraft sum benchmarks */
    printf("\n");
    for (int si = 0; si < 4; si++) {
        int n = sizes[si] * 50;
        int* lens = (int*)malloc((size_t)n * sizeof(int));
        for (int i = 0; i < n; i++) lens[i] = (i % 10) + 1;
        double t0 = get_time_sec();
        double ks = kraft_sum(lens, n);
        double t1 = get_time_sec();
        printf("  Kraft sum n=%5d: %.6f ms (sum=%.4f)\n", n, (t1 - t0) * 1000.0, ks);
        free(lens);
    }

    /* Elias code length benchmarks */
    printf("\n--- Universal Code Lengths ---\n");
    size_t test_ns[] = {0, 1, 10, 100, 1000, 10000, 100000, 1000000};
    int nt = sizeof(test_ns) / sizeof(test_ns[0]);
    printf("  %8s %6s %6s %6s %6s\n", "n", "Gamma", "Delta", "Omega", "Leven.");
    for (int i = 0; i < nt; i++) {
        size_t n = test_ns[i];
        printf("  %8zu %6zu %6zu %6zu %6zu\n", n,
               elias_gamma_length(n), elias_delta_length(n),
               elias_omega_length(n), levenshtein_code_length(n));
    }

    /* Golomb code distribution */
    printf("\n--- Golomb Code Efficiency ---\n");
    int ms[] = {1, 2, 4, 8, 16, 32};
    for (int mi = 0; mi < 6; mi++) {
        int m = ms[mi];
        int* glens = pc_golomb_lengths(100, m);
        double avg = 0.0;
        for (int n = 0; n < 100; n++) avg += (double)glens[n];
        avg /= 100.0;
        printf("  m=%2d: average length = %.2f bits/symbol\n", m, avg);
        free(glens);
    }

    printf("\nBenchmark complete.\n");
    return 0;
}
