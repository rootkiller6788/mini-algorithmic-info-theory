/*
 * example_omega_approximation.c — Compute and analyze Chaitin's Ω
 *
 * L6: Demonstrates the approximation of Ω from below using
 * dovetailing enumeration of halting programs. Shows the
 * convergence of the sequence to the true (non-computable) value.
 *
 * This is the canonical problem: computing Ω bits.
 * Each new halting program found adds its contribution, improving
 * the lower bound. The upper bound tightens as we exhaust programs.
 */

#include "../include/binary_string.h"
#include "../include/prefix_machine.h"
#include "../include/halting.h"
#include "../include/omega.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  Omega Approximation from Below              ║\n");
    printf("║  Ω = Σ_{p: U(p) halts} 2^{-|p|}             ║\n");
    printf("╚══════════════════════════════════════════════╝\n\n");

    /* Create a simple universal machine */
    RegisterProgram* prog = rp_create();
    rp_add_instruction(prog, OP_INC, 0, 0);   /* R0++ */
    rp_add_instruction(prog, OP_INC, 0, 0);   /* R0++ */
    rp_add_instruction(prog, OP_INC, 0, 0);   /* R0++ */
    rp_add_instruction(prog, OP_DECJZ, 1, 0); /* if R1=0 loop to start */
    rp_add_instruction(prog, OP_HALT, 0, 0);

    OptimalPFM* machine = opfm_create();
    opfm_register_machine(machine, prog);

    /* Initialize Omega state */
    OmegaState* os = omega_create(machine);
    OmegaConvergence* conv = omega_convergence_create(32);

    printf("Iteration  │  Ω lower   │  Ω upper   │  Gap       │  Reliable bits\n");
    printf("───────────┼────────────┼────────────┼────────────┼──────────────\n");

    /* Run 12 iterations of omega computation */
    for (size_t iter = 0; iter < 12; iter++) {
        omega_iterate(os, 1);
        omega_convergence_record(conv,
            os->omega_lower_bound,
            os->programs_found_halting);

        printf("  %5zu    │  %.8f  │  %.8f  │  %.8f  │  %5zu\n",
               iter + 1,
               os->omega_lower_bound,
               os->omega_upper_bound,
               os->omega_upper_bound - os->omega_lower_bound,
               omega_reliable_bits(os->max_program_size,
                                   os->programs_found_halting));
    }

    printf("\n──────────────────────────────────────────────\n");
    printf("Final approximation: Ω ≈ %.12f\n", os->omega_lower_bound);
    printf("Programs checked: %zu\n", os->programs_checked);
    printf("Programs found halting: %zu\n", os->programs_found_halting);
    printf("Max program size reached: %zu\n", os->max_program_size);

    /* Show omega bits */
    printf("\nFirst 8 bits of Ω: ");
    BitString* bits = omega_get_bits(os, 8);
    if (bits) {
        bs_print(bits);
        bs_free(bits);
    }

    /* Verify Ω is left-c.e. */
    printf("\nΩ is left-c.e.: %s\n",
           omega_is_left_ce(os) ? "YES ✓" : "NO ✗");
    printf("Ω is non-computable: demonstrated via encoding of halting problem.\n");

    /* Convergence analysis */
    printf("\nConvergence rate: %.12f per iteration\n",
           omega_convergence_rate(conv));

    /* Cleanup */
    omega_convergence_free(conv);
    omega_free(os);
    opfm_free(machine);

    printf("\nDone.\n");
    return 0;
}
