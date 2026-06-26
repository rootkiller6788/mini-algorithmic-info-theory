/*
 * bench_omega.c — Performance benchmark for Omega constant computation
 *
 * Measures the runtime scaling of:
 *   - Omega iteration vs program size
 *   - Dovetailing enumeration throughput
 *   - Kolmogorov complexity search
 */

#include "../include/binary_string.h"
#include "../include/prefix_machine.h"
#include "../include/omega.h"
#include "../include/halting.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double time_diff_sec(clock_t start, clock_t end) {
    return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  Omega Constant — Performance Benchmarks     ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_INC, 0, 0);
    rp_add_instruction(prog, OP_HALT, 0, 0);

    OptimalPFM* machine = opfm_create();
    opfm_register_machine(machine, prog);

    printf("Benchmarking Omega computation...\n\n");
    printf("  Size  |  Time (s)  |  Omega  |  Programs found\n");
    printf("--------+------------+---------+----------------\n");

    for (size_t sz = 2; sz <= 10; sz++) {
        clock_t start = clock();

        OmegaState* os = omega_create(machine);
        double omega = omega_approximate_to_size(os, sz);
        size_t found = os->programs_found_halting;

        clock_t end = clock();
        double elapsed = time_diff_sec(start, end);

        printf("  %4zu  |  %8.4f  |  %.6f  |  %6zu\n", sz, elapsed, omega, found);
        omega_free(os);
    }

    printf("\nDone.\n");
    opfm_free(machine);
    return 0;
}
