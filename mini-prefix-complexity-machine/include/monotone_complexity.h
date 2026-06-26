/*
 * monotone_complexity.h — Monotone machine complexity Km(x)
 *
 * L2: A monotone machine M has the property that if p and q are programs
 * and p is a prefix of q, then M(p) is a prefix of M(q) (when both defined).
 *
 * Km(x) = min{ |p| : M(p) = x* }  (x as prefix of output)
 * Km satisfies: Km(xy) ≤ Km(x) + Km(y | x) + O(1)
 *
 * Relation to prefix complexity: K(x) ≤ Km(x) ≤ K(x) + O(1)
 *
 * Reference: Levin (1973), Li & Vitányi §3.7
 * Courses: Oxford Advanced Complexity, Cambridge Part III, MIT 6.841 §4
 */

#ifndef MONOTONE_COMPLEXITY_H
#define MONOTONE_COMPLEXITY_H

#include "prefix_machine.h"

/* ── Monotone Machine Structure ─────────────────────────────── */
typedef struct {
    int   n_states;
    int*  transitions;    /* (from, read) -> (to, write, dir) */
    int   n_symbols;
    int   is_monotone;
    char* name;
} MonotoneMachine;

/* ── Construction ───────────────────────────────────────────── */
MonotoneMachine* mm_create(int n_states, int n_symbols, const char* name);
void  mm_free(MonotoneMachine* mm);
int   mm_add_transition(MonotoneMachine* mm, int from, int read,
                         int to, int write, int dir);

/* ── Monotonicity Verification ──────────────────────────────── */
/**
 * mm_check_monotone — Verify monotonicity property by sampling
 * finite program pairs. Returns 1 if the sampled behavior is monotone.
 * Full verification is undecidable (by Rice's theorem).
 */
int   mm_check_monotone(MonotoneMachine* mm);

/**
 * mm_simulate — Simulate monotone machine on program p.
 * Returns output PMString or NULL if no halt within max_steps.
 */
PMString* mm_simulate(const MonotoneMachine* mm,
                       const unsigned char* program, size_t prog_len,
                       int max_steps);

/* ── Monotone Complexity ───────────────────────────────────── */
/**
 * mm_complexity — Km(x) = min{|p| : M(p) outputs x as prefix}
 * Upper bound via self-delimiting encoding: |x| + 2⌊log|x|⌋ + 1
 */
size_t mm_complexity(const PMString* x);

/**
 * mm_conditional_complexity — Km(x|y)
 * Chain rule: Km(xy) ≤ Km(x) + Km(y|x) + O(1)
 */
size_t mm_conditional_complexity(const PMString* x, const PMString* y);

/* ── Process & Decision Complexity ──────────────────────────── */
/**
 * mm_process_complexity — Kp(x): minimal program to decide
 * {x} incrementally, reading input bit by bit.
 */
size_t mm_process_complexity(const PMString* x);

/**
 * mm_decision_complexity — KD(x): minimal program to decide {x}
 * for arbitrary (random-access) input.
 * KD(x) = K(x) ± O(1)
 */
size_t mm_decision_complexity(const PMString* x);

/* ── Levin Search ───────────────────────────────────────────── */
/**
 * mm_levin_search — Universal search for optimal monotone program.
 * Returns upper bound on Km(x) by interleaving all program executions.
 * Optimal up to additive O(1) constant.
 */
size_t mm_levin_search(const PMString* target, int max_time);

/* ── Chain Rule ─────────────────────────────────────────────── */
typedef struct {
    size_t lhs;    /* Km(xy) */
    size_t rhs;    /* Km(x) + Km(y|x) + c */
    size_t gap;    /* max(0, lhs - rhs) if violated */
    int    holds;  /* 1 if chain rule satisfied */
} ChainRuleResult;

ChainRuleResult mm_verify_chain_rule(const PMString* x, const PMString* y);

/* ── Invariance Theorem ─────────────────────────────────────── */
size_t mm_invariance_constant(int n_states_u, int n_symbols_u,
                               int n_states_v, int n_symbols_v);

/* ── Self-Delimiting Program Construction ───────────────────── */
PMString* mm_selfdelim_program(const PMString* x);

/* ── Relation between K and Km ──────────────────────────────── */
void mm_relation_to_prefix(const PMString* x, size_t* out_K, size_t* out_Km);

/* ── Symmetry of Information ────────────────────────────────── */
typedef struct {
    double left;   /* Km(x) + Km(y|x) */
    double right;  /* Km(y) + Km(x|y) */
    double diff;   /* |left - right| */
} SymmetryResult;

SymmetryResult mm_symmetry_of_information(const PMString* x, const PMString* y);

/* ── Schnorr Randomness ─────────────────────────────────────── */
int mm_is_schnorr_random_prefix(const PMString* prefix, double threshold);

/* ── Optimality Constant ────────────────────────────────────── */
size_t mm_optimality_constant(const MonotoneMachine* mm);

/* ── Entropy Lower Bound ────────────────────────────────────── */
double mm_expected_complexity_lower_bound(const double* probs, int n);

/* ── Kraft Sum for Monotone Programs ────────────────────────── */
double mm_kraft_sum_monotone(const PMString** strings, int n);

/* ── Conversion to Prefix Machine ───────────────────────────── */
PrefixMachine* mm_to_prefix_machine(const MonotoneMachine* mm);

#endif /* MONOTONE_COMPLEXITY_H */
