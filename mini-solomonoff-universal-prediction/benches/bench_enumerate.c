/**
 * bench_enumerate.c - Benchmark: Program Enumeration and Dovetailing
 *
 * Measures performance of:
 *   1. Program enumeration throughput (programs/second)
 *   2. Dovetailing execution speed (steps/second)
 *   3. M(x) computation time vs depth
 */
#include "solomonoff.h"
#include "enumeration.h"
#include <stdio.h>
#include <time.h>

static double time_ms(clock_t start, clock_t end) {
    return (double)(end - start) * 1000.0 / CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== Enumeration & Dovetailing Benchmarks ===\n\n");

    /* Benchmark 1: Enumeration throughput */
    printf("1. Program enumeration throughput:\n");
    for (size_t depth = 4; depth <= 12; depth += 4) {
        program_enumerator_t e;
        enumerator_init(&e, depth);

        binary_string_t prog;
        size_t plen;
        size_t count = 0;

        clock_t start = clock();
        while (enumerator_next(&e, &prog, &plen)) count++;
        clock_t end = clock();

        double ms = time_ms(start, end);
        printf("   depth=%2zu: %8zu programs in %8.2f ms (%.0f prog/s)\n",
               depth, count, ms, count / (ms / 1000.0));
    }

    /* Benchmark 2: Dovetailing speed */
    printf("\n2. Dovetailing execution speed:\n");
    for (size_t depth = 4; depth <= 8; depth += 2) {
        dovetail_manager_t dm;
        size_t pool_cap = (depth <= 6) ? (1U << depth) : 256;
        dovetail_init(&dm, pool_cap, 500000);

        program_enumerator_t enumerator;
        enumerator_init(&enumerator, depth);

        binary_string_t prog;
        size_t plen;
        size_t added = 0;
        while (enumerator_next(&enumerator, &prog, &plen) && added < pool_cap) {
            dovetail_add_program(&dm, &prog, plen);
            added++;
        }

        clock_t start = clock();
        dovetail_run(&dm, depth, 500000);
        clock_t end = clock();

        double ms = time_ms(start, end);
        printf("   depth=%2zu: %zu programs, %llu steps, %.2f ms\n",
               depth, added, (unsigned long long)dm.global_step, ms);

        dovetail_free(&dm);
    }

    /* Benchmark 3: M(x) computation time */
    printf("\n3. M(x) computation time:\n");
    binary_string_t x;
    bs_from_cstr(&x, "01");
    for (size_t depth = 4; depth <= 10; depth += 2) {
        clock_t start = clock();
        algprob_t Mx = solomonoff_M_fast(&x, depth, 50000);
        clock_t end = clock();

        double ms = time_ms(start, end);
        printf("   depth=%2zu: M(01) = %.6f, %.2f ms\n", depth, Mx, ms);
    }

    printf("\nDone.\n");
    return 0;
}
