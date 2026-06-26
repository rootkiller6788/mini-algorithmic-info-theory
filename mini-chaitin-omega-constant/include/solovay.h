#ifndef SOLOVAY_H
#define SOLOVAY_H

#include "semicomputable.h"
#include <stdint.h>

/* ============================================================
 * Solovay Reducibility
 *
 * α ≤_S β if there exists a computable function f and constant c
 * such that for all rational q < β, f(q) is rational with f(q) < α,
 * and α - f(q) ≤ c·(β - q).
 *
 * Intuition: any approximation to β from below can be effectively
 * translated into an approximation to α from below, with at most
 * constant slowdown.
 *
 * Ω is ≤_S-complete for left-c.e. reals: every left-c.e. real is
 * Solovay-reducible to Ω.
 *
 * Knowledge points:
 *   - Solovay reducibility definition and properties
 *   - Ω is Solovay-complete for left-c.e. reals
 *   - Solovay degrees form an upper semilattice
 *   - Random left-c.e. reals are exactly the Solovay-complete ones
 *   - Calude's characterization: α is Solovay-complete iff it's Ω-like
 *   - Dominance relation between left-c.e. reals
 * ============================================================ */

/* ---- Solovay Reduction ---- */
/* Stores a computable reduction from one left-c.e. real to another */
typedef struct {
    /* For Solovay reducibility: α ≤_S β via (c, f) where:
     * f maps approximations q < β to approximations f(q) < α
     * with α - f(q) ≤ c·(β - q)
     * Here we store c and a function pointer for f. */
    double        constant_c;
    /* The reduction function: given a bound q < β_upper,
     * returns f(q) < α_lower */
    double        (*reduction_func)(double q_upper, void* context);
    void*         context;
    int           is_valid;  /* 1 if reduction is verified */
} SolovayReduction;

SolovayReduction* solovay_reduction_create(double c,
                                            double (*func)(double, void*),
                                            void* context);
void              solovay_reduction_free(SolovayReduction* sr);
double            solovay_apply(const SolovayReduction* sr, double q);

/* ---- Solovay Completeness ---- */
/* A left-c.e. real α is Solovay-complete if for every left-c.e.
 * real β, β ≤_S α. Ω is Solovay-complete. */

/* Verify that omega_lce is Solovay-complete by showing it dominates
 * a known hard left-c.e. real (e.g., the halting problem characteristic). */
int solovay_is_complete(const LeftCEReal* alpha);

/* ---- Ω-like Numbers ---- */
/* According to Calude et al. (2002), a left-c.e. real α is
 * Solovay-complete iff there exist constants c, N such that
 * for all n ≥ N, K(α↾n) ≥ n - c.
 * i.e., α is Solovay-complete iff it's algorithmically random. */

int solovay_is_omega_like(const LeftCEReal* alpha,
                          void* machine,
                          size_t max_prog_len, size_t step_limit);

/* ---- Solovay Degree Operations ---- */
/* The join of two Solovay degrees: α ⊕ β */
LeftCEReal* solovay_join(const LeftCEReal* alpha, const LeftCEReal* beta);

/* Check if α ≤_S β (approximate verification) */
int solovay_reducible_to(const LeftCEReal* alpha, const LeftCEReal* beta);

/* ---- Solovay Reduction Examples ---- */

/* Ω_U ≤_S Ω_V for universal machines U, V.
 * The Solovay constant depends on the machines. */
SolovayReduction* solovay_between_omegas(void* omega_u, void* omega_v);

/* Every computable real is Solovay-reducible to any left-c.e. real */
int solovay_computable_reduces_to_lce(double computable_value,
                                       const LeftCEReal* lce);

/* ---- Dominance ---- */
/* α dominates β if there exists constant c such that
 * for all n, the nth approximation of α is within c times
 * the error of the nth approximation of β. */
typedef struct {
    const LeftCEReal* alpha;
    const LeftCEReal* beta;
    double   domination_constant;
    int      alpha_dominates_beta;
} DominanceResult;

DominanceResult solovay_dominance_check(const LeftCEReal* alpha,
                                         const LeftCEReal* beta);

/* ---- Hierarchy of Left-c.e. Reals ---- */
/* There exist left-c.e. reals that are NOT Solovay-complete.
 * This constructs a non-complete left-c.e. real (e.g., computable) */
LeftCEReal* solovay_noncomplete_example(void);
/* Constructs a strictly intermediate left-c.e. real */
LeftCEReal* solovay_intermediate_example(void);

#endif /* SOLOVAY_H */
