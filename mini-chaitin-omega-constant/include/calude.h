#ifndef CALUDE_H
#define CALUDE_H

#include "omega.h"
#include "kolmogorov.h"
#include "semicomputable.h"
#include <stdint.h>

/* ============================================================
 * Calude's Theorem and Omega Bit Computation
 *
 * Calude (2002) proved a precise relationship between bits of Ω
 * and the halting problem:
 *
 * "The first n bits of Ω exactly solve the halting problem for
 *  all programs of size ≤ n (up to an additive constant)."
 *
 * This module explores Calude's theorem and its implications.
 *
 * Knowledge points:
 *   - Calude's theorem: exact relationship between Ω and halting
 *   - Ω as an oracle: computational power
 *   - Omega numbers: characterization of Chaitin's Ω among left-c.e. reals
 *   - Cupping with Ω: what sets become computable with Ω as oracle
 *   - Omega and the arithmetical hierarchy
 * ============================================================ */

/* ---- Calude's Theorem: Main Result ---- */
/* Theorem: There exist constants c₁, c₂ such that for all n:
 * (1) The set of halting programs of size ≤ n can be computed
 *     from the first n + c₁ bits of Ω.
 * (2) The first n bits of Ω can be computed from the halting set
 *     of programs of size ≤ n + c₂.
 *
 * This means Ω↾(n+c₁) and the halting set for programs ≤ n
 * are Turing-equivalent (uniformly in n). */

/* Given bits of Omega, solve halting for programs ≤ n */
HaltingEnumeration* calude_solve_halting_from_omega(
    const BitString* omega_bits,
    size_t n_bits,
    OptimalPFM* machine,
    size_t max_program_size);

/* Given halting info for programs ≤ n, compute Omega bits */
BitString* calude_compute_omega_from_halting(
    const HaltingEnumeration* halting_info,
    size_t n_bits);

/* ---- Calude Constants ---- */
/* Compute the additive constant for a given machine:
 * how many extra bits of Omega are needed beyond n
 * to solve halting for programs of size n. */
size_t calude_constant(OptimalPFM* machine);

/* ---- Omega Number Characterization (Calude et al. 2002) ---- */
/* A left-c.e. real α ∈ (0,1) is an Ω-number (i.e., the halting
 * probability of SOME universal prefix-free machine) iff:
 * (1) α is left-c.e.
 * (2) α is Martin-Löf random
 * Equivalently: α is Solovay-complete. */

typedef struct {
    int is_left_ce;
    int is_ml_random;
    int is_omega_number;  /* 1 if both conditions hold */
    char characterization[256];
} OmegaNumberCheck;

OmegaNumberCheck calude_omega_characterization(
    const LeftCEReal* alpha,
    OptimalPFM* machine,
    size_t max_prog_len, size_t step_limit);

/* ---- Omega as Oracle ---- */
/* With Ω as oracle, one can decide Π⁰₁ (and thus Σ⁰₁) sets.
 * In fact, Ω ≡_T ∅', the Turing jump of the empty set.
 * This is Σ⁰₁-complete. */

/* Demonstrate: use Omega bits to decide the halting problem */
int calude_omega_oracle_demo(size_t n_programs);

/* ---- Cupping Theorem ---- */
/* Ω cups to ∅': Ω ⊕ A ≥_T ∅' for any set A that is not
 * K-trivial. This explores the computational power added by Ω. */

/* ---- Omega and the Arithmetical Hierarchy ---- */
/* Ω is Σ⁰₁-complete.
 * This means: every Σ⁰₁ set is many-one reducible to Ω.
 * Ω ∈ Σ⁰₁ \ Π⁰₁ (strictly in Σ⁰₁). */

int calude_omega_sigma1_complete_demo(void);

/* ---- Bit Extraction Algorithm ---- */
/* While Ω is not computable, we can extract specific bits
 * by solving enough of the halting problem.
 * This function implements the extraction algorithm. */
int calude_extract_bit(OmegaState* os, size_t bit_index, size_t effort);

/* ---- Omega and the Busy Beaver ---- */
/* Let BB(n) be the maximum number of steps taken by any
 * halting program of size ≤ n. Then:
 * Ω_n (first n bits) determines BB(n-c) for some c.
 * And BB(n) determines Ω_n (first n bits). */
uint64_t calude_bb_from_omega(const BitString* omega_bits, size_t n_bits);
BitString* calude_omega_from_bb(uint64_t bb_value, size_t n_bits);

/* ---- Enumeration of Omega Numbers ---- */
/* There are countably many Ω-numbers (one for each UTM).
 * This enumerates Ω-numbers for a family of UTMs. */
typedef struct {
    LeftCEReal** omega_numbers;
    size_t       count;
    size_t       capacity;
} OmegaNumberFamily;

OmegaNumberFamily* calude_enumerate_omega_numbers(
    OptimalPFM** machines, size_t num_machines,
    size_t max_prog_size, size_t step_limit);
void calude_family_free(OmegaNumberFamily* family);

#endif /* CALUDE_H */
