#ifndef OMEGA_H
#define OMEGA_H

#include "binary_string.h"
#include "prefix_machine.h"
#include "halting.h"
#include "kolmogorov.h"
#include <stdint.h>

/* ============================================================
 * Chaitin's Omega Constant — Core Module
 *
 * Ω_U = Σ_{p: U(p) halts} 2^{-|p|}
 *
 * The halting probability of a prefix-free universal machine U.
 * This is the central object of algorithmic information theory.
 *
 * Key Properties:
 *   - Ω ∈ (0,1) is a well-defined real number
 *   - Ω is left-c.e. (computably enumerable from below)
 *   - Ω is NOT computable (its binary expansion is uncomputable)
 *   - Ω is Martin-Löf random (algorithmically random)
 *   - Ω is Turing-equivalent to the halting problem ∅'
 *   - Ω is Σ⁰₁-complete
 *   - For different universal machines U,V, Ω_U and Ω_V are Solovay-equivalent
 *
 * Knowledge points:
 *   - Definition and existence of Ω
 *   - Omega as limit of computable increasing sequence
 *   - Omega approximation algorithms
 *   - Omega digit extraction
 *   - Omega randomness verification
 *   - Omega and Chaitin's incompleteness
 * ============================================================ */

/* ---- Omega State ---- */
/* Central structure for omega computation */
typedef struct {
    OptimalPFM* machine;              /* the underlying optimal prefix-free machine */
    HaltingEnumeration* halting_set;   /* discovered halting programs */
    double   omega_lower_bound;       /* current approximation from below */
    double   omega_upper_bound;       /* current bound from above */
    size_t   programs_checked;        /* total programs examined */
    size_t   programs_found_halting;  /* halting programs found */
    size_t   max_program_size;        /* current search depth */
    size_t   step_limit;              /* steps per program */
    int      converged;               /* whether approximation stabilized */
    BitString* bit_representation;    /* binary digits of omega approximation */
    size_t   bits_computed;           /* number of bits computed reliably */
} OmegaState;

/* ---- Lifecycle ---- */
OmegaState* omega_create(OptimalPFM* machine);
void        omega_free(OmegaState* os);
void        omega_reset(OmegaState* os);

/* ---- Core Omega Computation ---- */

/* Compute Omega_U = Σ_{p:U(p) halts} 2^{-|p|} with increasing accuracy.
 * Each call extends the dovetailing search and improves the lower bound.
 * Returns the new lower bound. */
double omega_iterate(OmegaState* os, size_t iterations);

/* Run omega approximation until convergence to within epsilon */
double omega_approximate(OmegaState* os, double epsilon, size_t max_iterations);

/* Run omega approximation searching programs up to given size */
double omega_approximate_to_size(OmegaState* os, size_t max_program_size);

/* ---- Omega Properties ---- */

/* Verify that 0 < Ω < 1 */
int omega_bounds_check(const OmegaState* os);

/* Verify Ω is left-c.e.: returns 1 if the sequence of approximations
 * is non-decreasing and converges to a limit. */
int omega_is_left_ce(const OmegaState* os);

/* Verify Ω is non-computable: demonstrate that no algorithm can
 * compute Ω to arbitrary precision. Returns a demonstration of
 * non-computability by showing Ω encodes the halting problem. */
int omega_noncomputable_demo(OmegaState* os);

/* Show that Ω ≡_T ∅' (Turing-equivalent to the halting problem).
 * Given the first n bits of Ω, we can solve the halting problem
 * for programs of length ≤ n. */
int omega_encodes_halting(OmegaState* os, size_t n_bits);

/* ---- Omega Bit Extraction ---- */

/* Extract the nth bit of Ω (approximation).
 * Requires solving halting for programs up to size n + O(1). */
int omega_get_bit(OmegaState* os, size_t bit_position);

/* Get the first n bits of Ω as a BitString */
BitString* omega_get_bits(OmegaState* os, size_t n);

/* Verify that K(Ω↾n) ≥ n - O(1), i.e., the first n bits of Ω
 * are algorithmically incompressible (Martin-Löf random). */
int omega_bits_are_random(OmegaState* os, size_t n);

/* ---- Omega for Different Universal Machines ---- */

/* Two universal machines U, V yield Omegas that are Solovay-equivalent:
 * Ω_V ≤ c·Ω_U for some rational c > 0. (Solovay reducibility)
 * This function computes the Solovay constant between two approximations. */
double omega_solovay_constant(const OmegaState* os1, const OmegaState* os2);

/* ---- Omega Approximation Quality ---- */

/* The error after checking programs up to size n is ≤ 2^{-n} */
double omega_approximation_error_bound(size_t max_program_size);

/* Number of bits of Ω that are reliable given current search depth */
size_t omega_reliable_bits(size_t max_program_size, size_t halting_count);

/* ---- Omega and Algorithmic Probability ---- */

/* Ω = Σ_x P_U(x) where P_U(x) = Σ_{p:U(p)=x} 2^{-|p|}
 * Verify this identity. */
int omega_equals_total_algorithmic_probability(OmegaState* os,
                                                size_t max_output_size,
                                                size_t step_limit);

/* ---- Omega for Register Machine Model ---- */

/* Compute an approximation of Ω for the small register machine model.
 * This is a concrete, computable approximation that demonstrates all
 * theoretical properties in a finite setting. */
double omega_register_machine(size_t max_program_size, size_t step_limit);

/* ---- Omega Convergence Monitor ---- */

typedef struct {
    double* lower_bounds;
    size_t* programs_found;
    size_t  count;
    size_t  capacity;
} OmegaConvergence;

OmegaConvergence* omega_convergence_create(size_t capacity);
void              omega_convergence_free(OmegaConvergence* oc);
int               omega_convergence_record(OmegaConvergence* oc,
                                            double bound, size_t found);
void              omega_convergence_print(const OmegaConvergence* oc);
double            omega_convergence_rate(const OmegaConvergence* oc);

#endif /* OMEGA_H */
