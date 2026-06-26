#include "../include/prefix_machine.h"
#include "../include/monotone_complexity.h"
#include "../include/universal_distribution.h"
#include <stdio.h>
#include <time.h>

static double get_time_sec(void) {
    return (double)clock() / (double)CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== Benchmark: Prefix Machine & Complexity ===\n\n");

    /* Prefix complexity computation benchmarks */
    printf("--- Prefix Complexity K(x) ---\n");
    const char* test_strs[] = {
        "a", "ab", "abc", "hello", "hello world",
        "aaaaaaaaaa", "the quick brown fox", "0123456789",
        NULL
    };
    for (int i = 0; test_strs[i]; i++) {
        PMString* s = pmstr_from_cstr(test_strs[i]);
        double t0 = get_time_sec();
        for (int rep = 0; rep < 1000; rep++) {
            size_t k = pm_prefix_complexity(s);
            (void)k;
        }
        double t1 = get_time_sec();
        printf("  K(\"%s\"): %.6f us/call\n", test_strs[i],
               (t1 - t0) * 1e6 / 1000.0);
        pmstr_free(s);
    }

    /* Self-delimiting benchmarks */
    printf("\n--- Self-Delimiting Encoding ---\n");
    for (int i = 0; test_strs[i]; i++) {
        PMString* s = pmstr_from_cstr(test_strs[i]);
        double t0 = get_time_sec();
        for (int rep = 0; rep < 1000; rep++) {
            PMString* sd = pm_self_delimiting(s);
            pmstr_free(sd);
        }
        double t1 = get_time_sec();
        printf("  sd(\"%s\"): %.6f us/call\n", test_strs[i],
               (t1 - t0) * 1e6 / 1000.0);
        pmstr_free(s);
    }

    /* Universal distribution benchmarks */
    printf("\n--- Universal Distribution m(x) ---\n");
    for (int i = 0; test_strs[i] && i < 6; i++) {
        PMString* s = pmstr_from_cstr(test_strs[i]);
        double t0 = get_time_sec();
        for (int rep = 0; rep < 100; rep++) {
            UniversalDistribution* ud = ud_estimate(s, 16, 1000);
            ud_free(ud);
        }
        double t1 = get_time_sec();
        printf("  m(\"%s\"): %.6f us/call\n", test_strs[i],
               (t1 - t0) * 1e6 / 100.0);
        pmstr_free(s);
    }

    /* Monotone complexity benchmarks */
    printf("\n--- Monotone Complexity Km(x) ---\n");
    for (int i = 0; test_strs[i] && i < 6; i++) {
        PMString* s = pmstr_from_cstr(test_strs[i]);
        double t0 = get_time_sec();
        for (int rep = 0; rep < 1000; rep++) {
            size_t km = mm_complexity(s);
            (void)km;
        }
        double t1 = get_time_sec();
        printf("  Km(\"%s\"): %.6f us/call\n", test_strs[i],
               (t1 - t0) * 1e6 / 1000.0);
        pmstr_free(s);
    }

    /* Omega estimation benchmarks */
    printf("\n--- Chaitin Omega Estimation ---\n");
    int omega_lens[] = {4, 6, 8, 10, 12};
    for (int i = 0; i < 5; i++) {
        int ml = omega_lens[i];
        double t0 = get_time_sec();
        double omega = pm_chaitin_omega_estimate(ml, 500);
        double t1 = get_time_sec();
        printf("  Omega(max_len=%2d): %.10f (%.6f ms)\n",
               ml, omega, (t1 - t0) * 1000.0);
    }

    /* String operations benchmark */
    printf("\n--- PMString Operations ---\n");
    {
        double t0 = get_time_sec();
        for (int rep = 0; rep < 10000; rep++) {
            PMString* s = pmstr_from_cstr("benchmark");
            pmstr_append(s, '!');
            pmstr_free(s);
        }
        double t1 = get_time_sec();
        printf("  create+append+free: %.6f us/op\n",
               (t1 - t0) * 1e6 / 10000.0);
    }

    printf("\nBenchmark complete.\n");
    return 0;
}
