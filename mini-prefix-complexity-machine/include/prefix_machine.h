/*
 * prefix_machine.h — Prefix Turing Machines and Prefix Complexity K(x)
 *
 * A prefix machine is a TM whose domain (set of halting inputs) is prefix-free:
 *   If p and q are halting programs, then p is not a proper prefix of q.
 *
 * L1: Prefix complexity K(x) = min{ |p| : U(p) = x, U is a prefix TM }
 * L3: Prefix-free sets, Kraft inequality, self-delimiting codes
 * L4: K(x) ≤ C(x) + O(log C(x)), K(x) ≥ C(x) - O(1)
 *
 * Reference: Li & Vitányi §3.1-3.3, Chaitin (1975)
 * Courses: MIT 6.841, Stanford CS254, Berkeley CS278
 */

#ifndef PREFIX_MACHINE_H
#define PREFIX_MACHINE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

/* ── Basic String Type ──────────────────────────────────────── */
typedef struct {
    unsigned char* data;
    size_t         len;
    size_t         cap;
} PMString;

PMString* pmstr_create(size_t cap);
PMString* pmstr_from_cstr(const char* s);
PMString* pmstr_from_data(const unsigned char* d, size_t len);
PMString* pmstr_clone(const PMString* s);
void      pmstr_append(PMString* s, unsigned char c);
void      pmstr_append_data(PMString* s, const unsigned char* d, size_t len);
int       pmstr_equals(const PMString* a, const PMString* b);
void      pmstr_free(PMString* s);
void      pmstr_print(const PMString* s);
size_t    pmstr_len(const PMString* s);

/* ── Prefix Machine ─────────────────────────────────────────── */
typedef struct {
    int   n_states;
    int   q0, q_accept;
    int*  transitions; /* flat array: [from][read] → {next, write, dir} */
    int   n_symbols;
    char* name;
} PrefixMachine;

/* ── Computation State ──────────────────────────────────────── */
typedef struct {
    int        state;
    PMString*  tape;
    int        head_pos;
    int        step_count;
} PMConfig;

/* ══════════════════════════════════════════════════════════════
   Prefix Machine Construction
   ══════════════════════════════════════════════════════════════ */

PrefixMachine* pm_create(int n_states, int n_symbols, const char* name);
int  pm_add_transition(PrefixMachine* pm, int from, int read,
                        int to, int write, int dir);
void pm_free(PrefixMachine* pm);

/* ══════════════════════════════════════════════════════════════
   Prefix-Free Checking
   ══════════════════════════════════════════════════════════════ */

/**
 * pm_is_prefix_free_domain — Check if a set of programs forms
 * a prefix-free set. Returns 1 if prefix-free.
 */
int pm_is_prefix_free(PMString** programs, int n);

/**
 * pm_prefix_complexity — Prefix Kolmogorov complexity upper bound:
 *   K(x) = min{ |p| : U(p) = x, domain(U) is prefix-free }
 * Uses self-delimiting encoding of x plus a print loop.
 */
size_t pm_prefix_complexity(const PMString* x);

/**
 * pm_plain_to_prefix_gap — K(x) ≤ C(x) + 2 log C(x) + O(1)
 * The additive gap between plain and prefix complexity.
 */
size_t pm_plain_to_prefix_gap(size_t C_x);

/**
 * pm_coding_theorem — The Coding Theorem:
 *   K(x) = -log m(x) + O(1)
 * where m(x) = Σ_{p:U(p)=x} 2^{-|p|} is the universal a priori probability.
 * This computes m(x) by exhaustive search (up to some limit).
 */
double pm_universal_probability(const PMString* x, int max_prog_len);
double pm_coding_theorem_bound(double m_x);

/* ══════════════════════════════════════════════════════════════
   Self-delimiting codes (deeper treatment)
   ══════════════════════════════════════════════════════════════ */

/**
 * pm_self_delimiting — Encode x as x̄ = 1^{|n|} 0 n x
 * where n = |x| in binary. Length: |x| + 2⌊log|x|⌋ + 1
 * This is a prefix code: no codeword is prefix of another.
 */
PMString* pm_self_delimiting(const PMString* x);
size_t    pm_self_delimiting_length(size_t len);

/**
 * pm_elias_delta — Elias delta code: length ≈ |x| + log|x| + 2 log log|x| + 1
 * More efficient than simple self-delimiting for large x.
 */
PMString* pm_elias_delta_encode(size_t n);
size_t    pm_elias_delta_decode(const PMString* code);

/**
 * pm_elias_gamma — Elias gamma code: length ≈ 2⌊log n⌋ + 1
 */
PMString* pm_elias_gamma_encode(size_t n);
size_t    pm_elias_gamma_decode(const PMString* code);

/**
 * pm_lebesgue_measure — μ(S) = Σ_{x∈S} 2^{-|x|}
 * For a prefix-free set S, μ(S) ≤ 1 (Kraft inequality).
 */
double pm_lebesgue_measure(PMString** strings, int n);

/**
 * pm_kraft_check — Returns 1 if Kraft inequality holds (Σ 2^{-|s_i|} ≤ 1)
 */
int pm_kraft_check(PMString** strings, int n);

/* ══════════════════════════════════════════════════════════════
   Optimal Prefix Codes
   ══════════════════════════════════════════════════════════════ */

/**
 * pm_shannon_fano — Build a Shannon-Fano prefix code for given probabilities.
 * Returns codeword lengths satisfying Kraft with Σ 2^{-l_i} ≤ 1
 * and expected length L ≤ H + 1.
 */
int* pm_shannon_fano_lengths(const double* probs, int n);

/**
 * pm_huffman_lengths — Build Huffman code lengths (optimal prefix code).
 * Σ 2^{-l_i} ≤ 1, expected length L ≤ H + 1.
 */
int* pm_huffman_lengths(const double* probs, int n);

/* ══════════════════════════════════════════════════════════════
   Monotone Machines (deeper topic)
   ══════════════════════════════════════════════════════════════ */

/**
 * A monotone machine has the property: if p ⊆ q (p is prefix of q)
 * and both halt, then U(p) ⊆ U(q) (outputs are consistent prefixes).
 *
 * Monotone complexity Km(x) satisfies: Km(x) ≤ K(x) ≤ Km(x) + O(1)
 */

/**
 * pm_is_monotone — Check if machine's behavior is monotone
 * (simplified heuristic check)
 */
int pm_is_monotone(const PrefixMachine* pm);

/**
 * pm_monotone_complexity — Km(x) upper bound
 */
size_t pm_monotone_complexity(const PMString* x);

/* ══════════════════════════════════════════════════════════════
   Chaitin's Omega (connection to prefix machines)
   ══════════════════════════════════════════════════════════════ */

/**
 * Ω = Σ_{p: U(p) halts} 2^{-|p|}
 * Computed approximately by summing contributions of halting programs.
 */
double pm_chaitin_omega_estimate(int max_len, int max_steps);

/**
 * pm_omega_halting_probability — Probability that a random infinite
 * sequence has a halting prefix. Related to Ω.
 */
double pm_omega_prefix_halting_prob(int max_len);

#endif /* PREFIX_MACHINE_H */
