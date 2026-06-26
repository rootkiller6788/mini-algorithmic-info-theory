/*
 * kolmogorov.h — Kolmogorov Complexity K(x)
 *
 * Core definitions (L1, L3):
 *   K_U(x) = min { |p| : U(p) = x }
 *   where U is a fixed universal Turing machine.
 *
 * Plain Kolmogorov complexity C(x): descriptions are arbitrary strings.
 * Prefix complexity K(x): descriptions form a prefix-free set.
 *
 * Reference: Li & Vitányi §2.1-2.3, Sipser §6.4
 * Courses: MIT 6.841, Stanford CS254, Berkeley CS278, CMU 15-855
 *           Oxford Advanced Complexity, Cambridge Part III
 */

#ifndef KOLMOGOROV_H
#define KOLMOGOROV_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ── String types ──────────────────────────────────────────── */
typedef struct {
    unsigned char* data;
    size_t         len;
    size_t         cap;
} KString;

/* ── Description (program) ─────────────────────────────────── */
typedef struct {
    unsigned char* code;
    size_t         len;
} KProgram;

/* ── Universal Turing Machine ──────────────────────────────── */
typedef struct {
    int   n_states;
    int   tape_size;
    void* internal;    /* opaque state */
    char* name;
} KUniversalTM;

/* ── Complexity estimate ───────────────────────────────────── */
typedef struct {
    double   plain_upper;       /* C(x) upper bound */
    double   prefix_upper;      /* K(x) upper bound */
    double   lz77_ratio;        /* LZ77 compression ratio */
    double   entropy_estimate;  /* Shannon entropy */
    int      is_incompressible; /* 1 if likely incompressible */
} KComplexityProfile;

/* ── Enum for complexity type ──────────────────────────────── */
typedef enum {
    K_PLAIN  = 0,   /* C(x): arbitrary descriptions */
    K_PREFIX = 1,   /* K(x): prefix-free descriptions */
    K_COND   = 2    /* K(x|y): conditional complexity */
} KComplexityType;

/* ══════════════════════════════════════════════════════════════
   String API
   ══════════════════════════════════════════════════════════════ */
KString*  kstr_create(size_t init_cap);
KString*  kstr_from_cstr(const char* s);
KString*  kstr_from_data(const unsigned char* data, size_t len);
KString*  kstr_clone(const KString* s);
void      kstr_append(KString* s, unsigned char c);
void      kstr_append_str(KString* s, const KString* other);
void      kstr_append_data(KString* s, const unsigned char* data, size_t len);
int       kstr_equals(const KString* a, const KString* b);
int       kstr_compare(const KString* a, const KString* b);
void      kstr_free(KString* s);
void      kstr_print(const KString* s);
size_t    kstr_len(const KString* s);
unsigned char kstr_at(const KString* s, size_t i);

/* ══════════════════════════════════════════════════════════════
   Core Kolmogorov Complexity
   ══════════════════════════════════════════════════════════════ */

/**
 * k_complexity_upper_bound — Compute heuristic upper bound for K(x).
 *
 * Theorem (L4): K(x) ≤ |x| + O(1)
 * Proof: program "print x" has length |x| + constant overhead.
 * Return: |x| + overhead constant.
 */
size_t k_complexity_upper_bound(const KString* x);

/**
 * k_complexity_pair_bound — K(x, y) ≤ K(x) + K(y) + O(log min(K(x), K(y)))
 */
size_t k_complexity_pair_bound(size_t Kx, size_t Ky);

/**
 * k_conditional_upper — K(x | y) ≤ K(x) + O(1)
 */
size_t k_conditional_upper(size_t Kx);

/**
 * k_symmetry_of_info — |K(x) - K(x | y) + K(y) - K(y | x) - K(x, y)|
 *                      ≤ O(log n) where n = |x| + |y|
 * (Symmetry of information, proven by Levin & Kolmogorov)
 */
double k_symmetry_of_info(const KString* x, const KString* y);

/**
 * k_incompressibility_test — Heuristically test if string is
 * likely incompressible within c bits.
 * Returns 1 if string appears incompressible.
 */
int k_is_incompressible(const KString* x, int c);

/**
 * k_randomness_deficiency — δ(x) = |x| - K(x)
 * Measures how "non-random" (compressible) a string is.
 */
int k_randomness_deficiency_estimate(const KString* x);

/* ══════════════════════════════════════════════════════════════
   Invariance Theorem
   ══════════════════════════════════════════════════════════════ */

/**
 * k_invariance_bound — For any two universal TMs U, V:
 *   K_U(x) ≤ K_V(x) + c_{U,V}
 * Returns an upper bound on the additive constant c.
 */
size_t k_invariance_constant(const KUniversalTM* u, const KUniversalTM* v);

/* ══════════════════════════════════════════════════════════════
   Complexity Profile
   ══════════════════════════════════════════════════════════════ */
KComplexityProfile* k_profile_create(const KString* x);
void k_profile_free(KComplexityProfile* p);
void k_profile_print(const KComplexityProfile* p);

/* ══════════════════════════════════════════════════════════════
   Prefix-Free Set Operations
   ══════════════════════════════════════════════════════════════ */

/**
 * k_is_prefix_free — Check if a set of strings is prefix-free:
 *   No string is a proper prefix of another.
 * Kraft inequality: Σ 2^{-|x|} ≤ 1
 */
int k_is_prefix_free(KString** strings, int count);

/**
 * k_kraft_sum — Compute the Kraft sum: Σ 2^{-|s_i|}
 */
double k_kraft_sum(KString** strings, int count);

/**
 * k_kraft_to_lengths — Given a Kraft-compliant sum, generate
 * codeword lengths satisfying Kraft inequality.
 */
int* k_kraft_to_lengths(double sum, int n);

/* ══════════════════════════════════════════════════════════════
   Enumerability
   ══════════════════════════════════════════════════════════════ */

/**
 * K(x) is upper semi-computable: there exists a computable
 * function f(x, t) such that f(x, t) ≥ f(x, t+1) and
 * lim_{t→∞} f(x, t) = K(x).
 *
 * k_upper_semi_computable — t-step approximation from above.
 */
size_t k_upper_semi_computable(const KString* x, int t_max);

/**
 * k_halting_weight — Estimate the "weight" of halting programs
 * that produce x within t steps.
 */
double k_halting_weight(const KString* x, int max_steps);

#endif /* KOLMOGOROV_H */
