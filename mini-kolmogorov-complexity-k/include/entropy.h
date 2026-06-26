/*
 * entropy.h — Shannon Entropy and Information Measures
 *
 * L3 Math Structures: probability distributions, entropy, mutual information
 * L5 Algorithms: entropy estimation, block entropy, entropy rate
 *
 * H(X) = -Σ p(x) log₂ p(x)          (Shannon entropy)
 * H(X|Y) = H(X,Y) - H(Y)            (conditional entropy)
 * I(X;Y) = H(X) + H(Y) - H(X,Y)    (mutual information)
 *
 * Connection to Kolmogorov complexity (L4):
 *   K(x) ≈ H(x) + O(1) for typical sequences from ergodic sources.
 *
 * Reference: Shannon (1948), Cover & Thomas §2
 * Courses: MIT 6.441 (Info Theory), Stanford EE376A, Berkeley EE229A
 */

#ifndef ENTROPY_H
#define ENTROPY_H

#include "kolmogorov.h"
#include <stdlib.h>
#include <math.h>

/* ══════════════════════════════════════════════════════════════
   Frequency distribution
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    double* probs;       /* probability mass function */
    int     n_symbols;   /* alphabet size */
    double  entropy;     /* cached entropy value */
} FrequencyDist;

FrequencyDist* freqdist_create(int n_symbols);
void  freqdist_compute_from_string(FrequencyDist* fd, const KString* s);
void  freqdist_compute_block(FrequencyDist* fd, const KString* s, int block_size);
void  freqdist_free(FrequencyDist* fd);

/* ══════════════════════════════════════════════════════════════
   Entropy Measures
   ══════════════════════════════════════════════════════════════ */

/**
 * shannon_entropy — H = -Σ p_i log₂ p_i
 */
double shannon_entropy_bits(const KString* s);
double shannon_entropy_nats(const KString* s);

/**
 * empirical_entropy_per_symbol — Estimated entropy rate.
 * Uses maximum likelihood estimation from observed frequencies.
 */
double empirical_entropy(const KString* s);

/**
 * block_entropy — H(X_1, ..., X_k) = -Σ P(x_1..x_k) log P(x_1..x_k)
 */
double block_entropy(const KString* s, int block_size);

/**
 * conditional_entropy_empirical — H(Y|X) estimated from paired data
 */
double conditional_entropy(const KString* x, const KString* y);

/**
 * mutual_information_empirical — I(X;Y) from paired data
 */
double mutual_information(const KString* x, const KString* y);

/**
 * entropy_rate — Estimate the entropy rate per symbol:
 *   lim_{n→∞} H(X_1,...,X_n)/n
 * Using increasing block sizes.
 */
double entropy_rate_estimate(const KString* s, int max_block);

/* ══════════════════════════════════════════════════════════════
   Entropy coding bounds
   ══════════════════════════════════════════════════════════════ */

/**
 * Source Coding Theorem (Shannon 1948):
 *   For any source X, there exists a code with expected length L
 *   satisfying H(X) ≤ L < H(X) + 1.
 *
 * shannon_lower_bound — Returns H(X) as lower bound in bits.
 */
double shannon_lower_bound(const KString* s);

/**
 * optimal_code_length_lower — For an individual string x,
 *   any uniquely decodable code has total length ≥ |x|·H
 *   (for i.i.d. sources).
 */
double optimal_code_length(const KString* s);

/* ══════════════════════════════════════════════════════════════
   Normalized measures
   ══════════════════════════════════════════════════════════════ */

/**
 * normalized_entropy — H / log₂(|Σ|), where |Σ| is alphabet size.
 * Range: [0, 1]
 * 1 = maximum entropy (uniform random), 0 = deterministic.
 */
double normalized_entropy(const KString* s, int alphabet_size);

/**
 * redundancy — 1 - (H / H_max)
 */
double redundancy(const KString* s, int alphabet_size);

/* ══════════════════════════════════════════════════════════════
   Jensen-Shannon Divergence
   ══════════════════════════════════════════════════════════════ */

/**
 * jensen_shannon_divergence — Symmetric divergence between two texts.
 *   JSD(P||Q) = ½ D_KL(P||M) + ½ D_KL(Q||M), M = ½(P+Q)
 * Used in text comparison, segment boundary detection.
 */
double jensen_shannon_divergence(const KString* a, const KString* b);

/* ══════════════════════════════════════════════════════════════
   Typical Sequences (Asymptotic Equipartition)
   ══════════════════════════════════════════════════════════════ */

/**
 * is_typical — Check if string is in the typical set A_ε^{(n)}.
 * A string is ε-typical if |-(1/n)·log P(x) - H| ≤ ε
 */
int is_typical(const KString* s, double epsilon);

/**
 * typical_set_size — Approximate size of typical set:
 *   |A_ε^{(n)}| ≈ 2^{nH}
 */
size_t typical_set_size(const KString* s, double epsilon);

/* ══════════════════════════════════════════════════════════════
   Complexity vs Entropy — Bridging L4 and L5
   ══════════════════════════════════════════════════════════════ */

/**
 * kolmogorov_entropy_gap — Estimate |K(x)/|x| - H(x)|
 * For typical sequences, this gap → 0 as |x| → ∞.
 */
double kolmogorov_entropy_gap(const KString* s);

/* ══════════════════════════════════════════════════════════════
   Min-Entropy / Rényi Entropy
   ══════════════════════════════════════════════════════════════ */

/**
 * min_entropy — H_∞ = -log₂ max p(x)
 * Maximum predictability / worst-case randomness.
 */
double min_entropy(const KString* s);

/**
 * renyi_entropy — H_α = (1/(1-α)) log₂ Σ p_i^α
 * α = 0: Hartley entropy (log N)
 * α = 1: Shannon entropy (limit)
 * α = 2: Collision entropy
 * α = ∞: Min-entropy
 */
double renyi_entropy(const KString* s, double alpha);

#endif /* ENTROPY_H */
