#ifndef SEMICOMPUTABLE_H
#define SEMICOMPUTABLE_H

#include <stdint.h>
#include "binary_string.h"
#include "prefix_machine.h"
#include "halting.h"

/* ============================================================
 * Semicomputable Real Numbers (Left-c.e. Reals)
 *
 * A real number α is left-c.e. if there exists a computable,
 * non-decreasing sequence of rationals (q_n) with lim q_n = α.
 *
 * Ω is the canonical left-c.e. real that is NOT computable.
 *
 * Knowledge points:
 *   - Definition of left-c.e. (computably enumerable from below)
 *   - Left-c.e. reals are exactly limits of computable increasing sequences
 *   - Ω is left-c.e. but not computable (and not right-c.e.)
 *   - Every left-c.e. real is the halting probability of SOME
 *     prefix-free machine (not necessarily universal)
 *   - Left-c.e. reals form a hierarchy under Solovay reducibility
 *   - Ω is complete for left-c.e. reals under Solovay reducibility
 * ============================================================ */

/* ---- Left-c.e. real number representation ---- */
/* Represented as an increasing sequence of rational approximations.
 * Each rational is a dyadic: numerator / 2^{denominator_power} */

typedef struct {
    uint64_t  numerator;
    int       denominator_power;  /* denominator = 2^p */
} DyadicRational;

DyadicRational* dr_create(uint64_t num, int denom_power);
void            dr_free(DyadicRational* dr);
double          dr_to_double(const DyadicRational* dr);
int             dr_compare(const DyadicRational* a, const DyadicRational* b);
DyadicRational* dr_add(const DyadicRational* a, const DyadicRational* b);
DyadicRational* dr_multiply_by_rational(const DyadicRational* a,
                                         uint64_t num, uint64_t den);

/* ---- Left-c.e. Real (as increasing sequence) ---- */
#define LCE_MAX_APPROXIMATIONS 1024

typedef struct {
    DyadicRational** approximations;  /* q_0 ≤ q_1 ≤ q_2 ≤ ... */
    size_t           count;
    size_t           capacity;
    int              is_converged;
    double           limit_estimate;
} LeftCEReal;

LeftCEReal* lce_create(size_t capacity);
void        lce_free(LeftCEReal* lce);
int         lce_add_approximation(LeftCEReal* lce, const DyadicRational* approx);
double      lce_current_value(const LeftCEReal* lce);
int         lce_is_non_decreasing(const LeftCEReal* lce);
double      lce_convergence_rate(const LeftCEReal* lce);

/* ---- Constructing left-c.e. reals from prefix-free machines ---- */
/* For any prefix-free machine M, the halting probability
 * Ω_M = Σ_{p: M(p) halts} 2^{-|p|} is left-c.e.
 * This constructs the sequence of approximations. */
LeftCEReal* lce_from_machine_halting(void* pfm, size_t max_prog_size, size_t steps);

/* ---- Left-c.e. vs computable ---- */
/* A real is computable iff it is both left-c.e. and right-c.e.
 * (i.e., there exist increasing and decreasing computable sequences
 * converging to it from both sides).
 * This function demonstrates that for a given left-c.e. real. */
int lce_is_computable(const LeftCEReal* lce);

/* ---- Operations on left-c.e. reals ---- */
/* Sum of two left-c.e. reals is left-c.e. */
LeftCEReal* lce_add(const LeftCEReal* a, const LeftCEReal* b);

/* Product of left-c.e. real with rational is left-c.e. */
LeftCEReal* lce_scale(const LeftCEReal* a, double factor);

/* ---- Left-c.e. real enumeration ---- */
/* Every left-c.e. real α ∈ (0,1) has a binary expansion that is
 * approximable: we can compute successive lower bounds.
 * This function extracts bits given an approximation. */
int lce_get_bit(const LeftCEReal* lce, size_t bit_position);
BitString* lce_get_bits(const LeftCEReal* lce, size_t n_bits);

/* ---- Relationship to Σ⁰₁ sets ---- */
/* A set S ⊆ ℕ is Σ⁰₁ (c.e.) iff its characteristic real
 * χ_S = Σ_{n∈S} 2^{-n-1} is left-c.e.
 * This demonstrates the correspondence. */
LeftCEReal* lce_from_ce_set(const int* set_indicator, size_t size);
void        lce_to_ce_set(const LeftCEReal* lce, int* out_set, size_t max_n);

/* ---- Omega is not right-c.e. ---- */
/* If Ω were right-c.e., then Ω would be computable.
 * But we can prove Ω is not computable by showing it encodes
 * the halting problem. This function demonstrates the argument. */
int lce_not_right_ce_demonstration(LeftCEReal* omega_lce);

#endif /* SEMICOMPUTABLE_H */
