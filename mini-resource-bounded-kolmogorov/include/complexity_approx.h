/**
 * complexity_approx.h — Approximations for Kolmogorov Complexity
 *
 * Since Kolmogorov complexity is uncomputable, we study approximations
 * under resource bounds, compression-based upper bounds, and
 * probabilistic estimates. This header defines the API for:
 *
 *   - Compression-based upper bound estimators (LZ, gzip-style)
 *   - Approximate entropy (sample entropy, topological entropy proxies)
 *   - Normalized compression distance (NCD) for clustering
 *   - Algorithmic information distance (AID) approximation
 *   - Two-part/MDL coding estimates
 *   - Martingale-based randomness approximations
 *   - Empirical entropy and Lempel-Ziv complexity
 *
 * References:
 *   Cilibrasi & Vitanyi, "Clustering by Compression" (2005)
 *   Li & Vitanyi, "An Introduction to Kolmogorov Complexity", 4th ed. (2019)
 *   Lempel & Ziv, "On the Complexity of Finite Sequences" (1976)
 *   Rissanen, "Modeling by Shortest Data Description" (1978)
 */

#ifndef COMPLEXITY_APPROX_H
#define COMPLEXITY_APPROX_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/* ==================================================================
 * L1: Approximate Complexity Result Types
 * ================================================================== */

/** Compression-based complexity upper bound.
 *  A concrete compressor (e.g. LZ77, LZ78) gives K(x) <= |C(x)| + O(1).
 */
typedef struct {
    size_t  compressed_len;
    size_t  original_len;
    double  ratio;
    double  estimated_K;
    char   *compressed;
    char   *algorithm_name;
} CompressionBound;

/** Lempel-Ziv 76/78 complexity (empirical).
 *  LZ complexity is the number of distinct phrases in the parsing.
 */
typedef struct {
    size_t  phrase_count;
    size_t  string_length;
    double  normalized_lz;
    double  entropy_estimate;
} LZComplexity;

/** Normalized Compression Distance (NCD) between two strings.
 *  NCD(x,y) = [C(xy) - min(C(x),C(y))] / max(C(x),C(y))
 */
typedef struct {
    double  ncd;
    double  normalized_information_distance;
    double  compression_distance;
} NCDResult;

/** Two-part MDL coding result. */
typedef struct {
    size_t  model_cost;
    size_t  data_cost;
    size_t  total_cost;
    double  stochastic_complexity;
} TwoPartMDL;

/** Martingale-based randomness approximation.
 *  Computes upper bounds on randomness deficiency via betting strategies.
 */
typedef struct {
    double  capital;
    double  max_capital;
    size_t  rounds;
    bool    reject_randomness;
} MartingaleApprox;

/** Sample entropy (approximate entropy ApEn / SampEn). */
typedef struct {
    double  sample_entropy;
    double  approximate_entropy;
    double  permutation_entropy;
    double  topological_entropy;
} EntropyProfile;

/** Block entropy estimate using n-gram frequencies.
 *  H_n = -sum_{w in Sigma^n} P(w) log P(w)
 */
typedef struct {
    size_t  block_size;
    double  block_entropy;
    double  entropy_rate;
    double  redundancy;
} BlockEntropy;

/* ==================================================================
 * L2: Core Approximation Functions
 * ================================================================== */

/** LZ77-style compression complexity estimation.
 *  Computes an upper bound on K(x) by simulating LZ77 compression.
 *  O(n log n) for input of length n. */
CompressionBound lz77_compress_bound(const char *x, size_t len);

/** LZ78 (LZW) parsing to compute normalized LZ complexity. */
LZComplexity lz78_complexity(const char *x, size_t len);

/** Normalized Compression Distance using an internal compressor.
 *  Reference: Cilibrasi & Vitanyi, 2005 — IEEE Trans. Info. Theory. */
NCDResult ncd_compute(const char *x, size_t xlen,
                       const char *y, size_t ylen);

/** Algorithmic information distance proxy via compression.
 *  E1(x,y) = max(K(x|y), K(y|x)) / max(K(x), K(y))
 *  approximated using LZ compression bounds. */
double aid_proxy(const char *x, size_t xlen,
                 const char *y, size_t ylen);

/** Symmetric AID = (K(x|y)+K(y|x)) / K(xy), compression proxy. */
double symmetric_aid_proxy(const char *x, size_t xlen,
                            const char *y, size_t ylen);

/* ==================================================================
 * L3: MDL / Two-Part Coding
 * ================================================================== */

/** Two-part MDL for a hypothesis H and data D.
 *  L(H,D) = L(H) + L(D|H) , minimized over hypothesis class. */
TwoPartMDL two_part_mdl(const char *data, size_t data_len,
                         const char **hypotheses, const size_t *hyp_lens,
                         size_t num_hyps);

/** Stochastic complexity (Rissanen's NML — Normalized Maximum Likelihood).
 *  SC(x) = -log P(x|theta_hat) + log sum_y P(y|theta_hat(y))
 *  approximated for Bernoulli sequences. */
double stochastic_complexity_nml(const char *binary_sequence, size_t len);

/** Minimum Description Length selector: argmin_H L(H)+L(D|H). */
size_t mdl_select_best_model(const char *data, size_t data_len,
                              const char **models, const size_t *model_lens,
                              size_t num_models);

/* ==================================================================
 * L4: Martingale-Based Randomness Tests
 * ================================================================== */

/** Construct a martingale betting strategy on prefix lengths.
 *  If the string is compressible, the martingale detects it.
 *  Reference: Schnorr, "Randomness and the amount of information" (1971). */
MartingaleApprox martingale_compressibility_test(const char *x, size_t len);

/** Multi-martingale test: runs several betting strategies in parallel.
 *  Returns the maximum capital achieved across all strategies. */
MartingaleApprox martingale_multi_strategy(const char *x, size_t len,
                                            const double *strategies,
                                            size_t num_strategies);

/* ==================================================================
 * L5: Entropy Estimation
 * ================================================================== */

/** Compute empirical entropy profile for a binary string.
 *  Includes sample entropy, approximate entropy, permutation entropy. */
EntropyProfile entropy_profile_compute(const char *x, size_t len);

/** Block entropy estimate: computes H_n for n=1..max_block.
 *  Returns array of BlockEntropy structures. */
BlockEntropy* block_entropy_series(const char *x, size_t len,
                                    size_t max_block, size_t *out_count);

/** Lempel-Ziv estimated entropy rate.
 *  H_hat = (log n) * LZ_count / n  (for large n). */
double lz_entropy_rate(const char *x, size_t len);

/* ==================================================================
 * L6: NCD-based Clustering / Applications
 * ================================================================== */

/** Compute NCD distance matrix for a set of strings.
 *  Returns flattened upper-triangular distance matrix. */
double* ncd_distance_matrix(const char **strings, const size_t *lens,
                             size_t count);

/** Cilibrasi-Vitanyi style clustering via compression.
 *  Outputs cluster assignments (integer labels) for each string. */
int* ncd_cluster(const char **strings, const size_t *lens,
                  size_t count, double threshold, size_t *num_clusters);

/** Compute the quartet tree from NCD distances (neighbor-joining).
 *  Returns adjacency matrix for the inferred tree. */
double* ncd_quartet_tree(const char **strings, const size_t *lens,
                          size_t count);

/* ==================================================================
 * L7: Cryptographic Incompressibility Tests
 * ================================================================== */

/** Incompressibility-based pseudorandomness test.
 *  If K(x|n) < |x| - c, then x is compressible → not random.
 *  Using LZ upper bound as proxy for K. */
bool approx_incompressibility_test(const char *x, size_t len, size_t slack);

/** Distinguish pseudorandom from true random using compression gap.
 *  Gap = |x| - C(x) where C is LZ-based upper bound on K.
 *  Returns estimated gap and whether it exceeds threshold. */
double compression_gap_test(const char *x, size_t len,
                            double threshold, bool *is_nonrandom);

/** Entropy-based determinism detection.
 *  Sample entropy close to 0 → deterministic structure detected. */
bool entropy_determinism_test(const char *x, size_t len,
                               double entropy_threshold);

/* ==================================================================
 * L8: Advanced Approximations
 * ================================================================== */

/** Universal distribution approximation via Monte Carlo sampling.
 *  Samples programs uniformly, runs them for bounded time,
 *  estimates m(x) = probability a random program outputs x. */
double universal_distribution_mc(const char *x, size_t len,
                                  size_t num_samples, size_t time_budget);

/** Coding theorem verification: c * m(x) >= 2^{-K(x)}.
 *  Checks consistency of the coding theorem using approximations. */
bool coding_theorem_verify(const char *x, size_t len,
                            size_t num_samples, size_t time_budget,
                            double tolerance);

/** Bayesian complexity estimation with prior over programs.
 *  P(program|data) ∝ P(data|program) * 2^{-|program|}. */
double bayesian_complexity_posterior(const char *data, size_t data_len,
                                      const char **programs,
                                      const size_t *prog_lens,
                                      size_t num_programs, size_t time_budget);

/** Lossy compression with rate-distortion under Kolmogorov complexity.
 *  Finds minimal K for description satisfying distortion constraint.
 *  Reference: Vereshchagin & Vitanyi, "Rate Distortion and Denoising" (2004). */
size_t rate_distortion_K(const char *signal, size_t signal_len,
                          double distortion_tolerance,
                          size_t (*distance)(const char*, size_t,
                                            const char*, size_t));

/* ==================================================================
 * Utility Functions
 * ================================================================== */

void compression_bound_free(CompressionBound *b);
void lz_complexity_free(LZComplexity *lz);
void ncd_result_print(const NCDResult *n);
void two_part_mdl_print(const TwoPartMDL *m);
void entropy_profile_print(const EntropyProfile *ep);
void martingale_approx_print(const MartingaleApprox *ma);

#endif /* COMPLEXITY_APPROX_H */
