#ifndef HALTING_H
#define HALTING_H

#include "binary_string.h"
#include "prefix_machine.h"
#include <stdint.h>

/* ============================================================
 * Halting Problem Detection and Enumeration
 *
 * The halting problem is undecidable (Turing 1936).
 * However, for bounded program sizes and step limits,
 * we can detect halting. This module provides:
 *
 * Knowledge points:
 *   - Undecidability of the halting problem
 *   - Dovetailing enumeration (interleaving all programs)
 *   - Busy Beaver function BB(n): max steps for halting programs of size n
 *   - Halting probability approximation
 *   - The halting set K = {p : U(p) halts} is c.e. but not computable
 *   - K is Σ⁰₁-complete (Turing-complete at level 1 of arith. hierarchy)
 * ============================================================ */

/* ---- Halting Oracle (bounded) ---- */
/* Checks if a program halts within max_steps.
 * Returns: 1 if halts, 0 if doesn't halt within limit, -1 on error. */
int halting_check(OptimalPFM* machine, const BitString* program, size_t max_steps);

/* ---- Dovetailing enumeration ---- */
/* Enumerate all halting programs up to max_program_size.
 * Uses dovetailing: interleave execution of all programs. */

typedef struct {
    BitString* program;
    uint64_t   steps_to_halt;
    uint64_t   output;
} HaltingRecord;

typedef struct {
    HaltingRecord* records;
    size_t         count;
    size_t         capacity;
} HaltingEnumeration;

HaltingEnumeration* halting_enumerate(OptimalPFM* machine,
                                       size_t max_program_size,
                                       size_t step_limit);
void halting_enum_free(HaltingEnumeration* he);
void halting_enum_print(const HaltingEnumeration* he);
void halting_enum_sort_by_program(HaltingEnumeration* he);
void halting_enum_sort_by_output(HaltingEnumeration* he);

/* ---- Bounded halting set ---- */
/* K_n = {p : |p| ≤ n and U(p) halts within f(n) steps}
 * This is computable for any computable f. */
HaltingEnumeration* halting_set_approximation(OptimalPFM* machine, size_t n, size_t step_bound);

/* ---- Busy Beaver function ---- */
/* BB(n) = max{steps(U(p)) : |p| ≤ n and U(p) halts}
 * BB(n) grows faster than any computable function. */
uint64_t busy_beaver_upper_bound(size_t n);

/* ---- Halting problem: diagonal argument ---- */
/* Demonstrates undecidability: if halting were decidable,
 * we could construct a paradoxical program. Returns 1 if
 * the hypothesized oracle gives a contradiction. */
int halting_diagonal_proof_demo(void);

/* ---- Halting probability contribution ---- */
/* Given a halting program p, its contribution to Omega is 2^{-|p|}.
 * For a set of programs, the partial sum approximates Omega from below. */
double halting_probability_contribution(const BitString* program);
double partial_omega(const HaltingEnumeration* he);
double partial_omega_up_to_size(const HaltingEnumeration* he, size_t max_size);

/* ---- Omega bit computation (Calude's theorem) ---- */
/* To compute the first n bits of Omega, we must solve the
 * halting problem for all programs of size ≤ n + c.
 * This function returns which programs must be checked. */
int omega_bit_required_checks(size_t bit_position, size_t* max_program_size, size_t* max_steps);

#endif /* HALTING_H */
