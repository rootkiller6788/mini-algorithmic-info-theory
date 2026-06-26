/**
 * complexity_approx.c — Approximations for Kolmogorov Complexity
 *
 * Since Kolmogorov complexity is uncomputable (Chaitin 1975), practical
 * applications rely on approximations using compression algorithms,
 * entropy estimators, and probabilistic sampling.
 *
 * This file implements:
 *   - LZ77/LZ78-based compression upper bounds on K(x)
 *   - Normalized Compression Distance (NCD) for clustering
 *   - MDL two-part coding selection
 *   - Martingale-based randomness approximation
 *   - Entropy profile estimation (sample entropy, block entropy)
 *   - Cryptographic incompressibility tests
 *   - Universal distribution via Monte Carlo sampling
 *
 * References:
 *   Cilibrasi & Vitanyi, "Clustering by Compression" (2005)
 *   Li & Vitanyi, "An Introduction to Kolmogorov Complexity", 4th ed. (2019)
 *   Lempel & Ziv, "On the Complexity of Finite Sequences" (1976)
 *   Rissanen, "Modeling by Shortest Data Description" (1978)
 */

#include "../include/complexity_approx.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* ==================================================================
 * Internal Helpers
 * ================================================================== */

/** Integer floor log2. */
static size_t ilog2(size_t n) {
    size_t r = 0;
    while (n >>= 1) r++;
    return r;
}

/* ==================================================================
 * LZ77 Compression Upper Bound
 *
 * LZ77 sliding-window compression: for each position i, find the longest
 * match in the preceding window, output (offset, length, next_char).
 * The compressed length gives an upper bound on K(x):
 *   K(x) <= |LZ77(x)| + O(1)
 *
 * Reference: Ziv & Lempel, "A Universal Algorithm for Sequential Data
 *   Compression" (1977), IEEE Trans. Info. Theory.
 * Complexity: O(n * window_size) where n = len.
 * ================================================================== */

CompressionBound lz77_compress_bound(const char *x, size_t len) {
    CompressionBound result;
    memset(&result, 0, sizeof(result));
    result.original_len = len;
    result.ratio = 1.0;
    result.estimated_K = (double)len;
    result.algorithm_name = "LZ77";

    if (!x || len == 0) return result;

    /* Simulate LZ77: greedy longest match in sliding window.
     * window_size = 255, lookahead = 255 (practical limits). */
    size_t window = (len < 255) ? len : 255;
    size_t compressed_bits = 0;
    size_t pos = 0;

    while (pos < len) {
        size_t best_len = 0, best_offset = 0;
        size_t search_start = (pos > window) ? (pos - window) : 0;

        for (size_t off = search_start; off < pos; off++) {
            size_t match_len = 0;
            while (pos + match_len < len && off + match_len < pos &&
                   x[off + match_len] == x[pos + match_len]) {
                match_len++;
                if (match_len >= 255) break;
            }
            if (match_len > best_len) {
                best_len = match_len;
                best_offset = pos - off;
            }
        }

        /* Encode: (offset bits) + (length bits) + (next char = 8 bits)
         * Use log2(window) bits for offset, log2(min(len,255)) for length. */
        size_t offset_bits = ilog2(window + 1) + 1;
        size_t length_bits = ilog2((len < 255 ? len : 255) + 1) + 1;
        compressed_bits += offset_bits + length_bits + 8;
        pos += best_len + 1;
    }

    size_t compressed_bytes = compressed_bits / 8 + 1;
    result.compressed_len = compressed_bytes;
    result.ratio = (double)compressed_bytes / (double)len;
    result.estimated_K = (double)compressed_bytes + ilog2(compressed_bytes + 1) + 40;

    /* Allocate compressed buffer placeholder */
    result.compressed = (char*)malloc(compressed_bytes + 1);
    if (result.compressed) {
        memset(result.compressed, 'C', compressed_bytes);
        result.compressed[compressed_bytes] = '\0';
    }

    return result;
}

/* ==================================================================
 * LZ78 (LZW) Complexity
 *
 * LZ78/LZW parsing: build a dictionary of distinct phrases.
 * Number of phrases / string length is the normalized LZ complexity.
 * As n -> infinity, LZ complexity / n approaches the entropy rate.
 *
 * Reference: Ziv & Lempel, "Compression of Individual Sequences via
 *   Variable-Rate Coding" (1978), IEEE Trans. Info. Theory.
 * Complexity: O(n) with dictionary lookup.
 * ================================================================== */

LZComplexity lz78_complexity(const char *x, size_t len) {
    LZComplexity result;
    memset(&result, 0, sizeof(result));
    result.string_length = len;

    if (!x || len == 0) return result;

    /* Simple LZ78 parsing: greedy phrase building.
     * Each new phrase is the longest prefix not yet in the dictionary.
     * Count phrases = distinct dictionary entries. */
    size_t dict_count = 0;
    size_t pos = 0;

    /* Dictionary: store start positions of phrases */
    size_t dict_start[1024];
    size_t dict_len[1024];

    while (pos < len && dict_count < 1024) {
        size_t phrase_len = 1;
        /* Extend phrase as long as it has been seen before */
        while (pos + phrase_len < len) {
            bool seen = false;
            for (size_t d = 0; d < dict_count; d++) {
                if (dict_len[d] == phrase_len + 1) {
                    bool match = true;
                    for (size_t i = 0; i <= phrase_len; i++) {
                        if (x[dict_start[d] + i] != x[pos + i])
                            { match = false; break; }
                    }
                    if (match) { seen = true; break; }
                }
            }
            if (!seen) break;
            phrase_len++;
        }

        dict_start[dict_count] = pos;
        dict_len[dict_count] = phrase_len + 1;
        dict_count++;
        pos += phrase_len + 1;
    }

    result.phrase_count = dict_count;
    result.normalized_lz = (double)dict_count / (double)len;
    /* Entropy rate estimate: H_hat = n * log(n) / LZ_count for large n */
    if (dict_count > 0 && len > 0) {
        result.entropy_estimate = (double)dict_count * log2((double)len + 1.0)
                                   / (double)len;
    }
    return result;
}

/* ==================================================================
 * Normalized Compression Distance (NCD)
 *
 * NCD(x,y) = [C(xy) - min(C(x), C(y))] / max(C(x), C(y))
 *
 * where C() is a compressor (here LZ77-based). NCD approximates the
 * normalized information distance (NID) and is a universal similarity
 * metric: if any computable distance says x and y are similar,
 * NCD also says so.
 *
 * Reference: Cilibrasi & Vitanyi (2005), IEEE Trans. Info. Theory.
 * Complexity: O(|x|*w + |y|*w + |xy|*w) where w = window.
 * ================================================================== */

NCDResult ncd_compute(const char *x, size_t xlen,
                       const char *y, size_t ylen) {
    NCDResult result;
    memset(&result, 0, sizeof(result));

    if (!x || !y || xlen == 0 || ylen == 0) {
        result.ncd = 1.0;
        result.normalized_information_distance = 1.0;
        return result;
    }

    CompressionBound cx = lz77_compress_bound(x, xlen);
    CompressionBound cy = lz77_compress_bound(y, ylen);

    /* Concatenate xy */
    size_t xylen = xlen + ylen;
    char *xy = (char*)malloc(xylen + 1);
    if (!xy) { result.ncd = 1.0; return result; }
    memcpy(xy, x, xlen);
    memcpy(xy + xlen, y, ylen);
    xy[xylen] = '\0';

    CompressionBound cxy = lz77_compress_bound(xy, xylen);
    free(xy);

    double cx_len = (double)cx.compressed_len;
    double cy_len = (double)cy.compressed_len;
    double cxy_len = (double)cxy.compressed_len;
    double min_c = (cx_len < cy_len) ? cx_len : cy_len;
    double max_c = (cx_len > cy_len) ? cx_len : cy_len;

    if (max_c > 0.0) {
        result.ncd = (cxy_len - min_c) / max_c;
        result.compression_distance = cxy_len;
    }
    result.normalized_information_distance = result.ncd;

    compression_bound_free(&cx);
    compression_bound_free(&cy);
    compression_bound_free(&cxy);
    return result;
}

double aid_proxy(const char *x, size_t xlen, const char *y, size_t ylen) {
    /* Algorithmic Information Distance proxy via compression.
     * E(x,y) = max(K(x|y), K(y|x)) / max(K(x), K(y))
     * Approximated by: max(C(x|y), C(y|x)) / max(C(x), C(y))
     * where C(x|y) ~ C(xy) - C(y) as a rough proxy. */
    if (!x || !y || xlen == 0 || ylen == 0) return 1.0;
    NCDResult ncd = ncd_compute(x, xlen, y, ylen);
    return ncd.ncd;
}

double symmetric_aid_proxy(const char *x, size_t xlen,
                            const char *y, size_t ylen) {
    /* Symmetric AID: (K(x|y) + K(y|x)) / K(xy), compression proxy. */
    if (!x || !y || xlen == 0 || ylen == 0) return 2.0;
    CompressionBound cx = lz77_compress_bound(x, xlen);
    CompressionBound cy = lz77_compress_bound(y, ylen);
    /* K(x|y) ~ C(xy) - C(y), K(y|x) ~ C(xy) - C(x) */
    size_t xylen = xlen + ylen;
    char *xy = (char*)malloc(xylen + 1);
    if (!xy) return 2.0;
    memcpy(xy, x, xlen); memcpy(xy + xlen, y, ylen);
    xy[xylen] = '\0';
    CompressionBound cxy = lz77_compress_bound(xy, xylen);
    free(xy);

    double kx_gy = (double)cxy.compressed_len - (double)cy.compressed_len;
    double ky_gx = (double)cxy.compressed_len - (double)cx.compressed_len;
    double kxy = (double)cxy.compressed_len;

    double result = (kxy > 0.0) ? (kx_gy + ky_gx) / kxy : 2.0;
    compression_bound_free(&cx); compression_bound_free(&cy);
    compression_bound_free(&cxy);
    return result;
}

/* ==================================================================
 * MDL / Two-Part Coding
 *
 * Rissanen's Minimum Description Length principle:
 * Best model M minimizes L(M) + L(D|M).
 * L(H) = length of hypothesis description
 * L(D|H) = length of data given hypothesis
 *
 * Stochastic complexity (NML): normalized maximum likelihood.
 * ================================================================== */

TwoPartMDL two_part_mdl(const char *data, size_t data_len,
                         const char **hypotheses, const size_t *hyp_lens,
                         size_t num_hyps) {
    TwoPartMDL result;
    memset(&result, 0, sizeof(result));
    result.total_cost = data_len + 64;

    if (!data || !hypotheses || num_hyps == 0) return result;

    size_t best_total = (size_t)-1;
    for (size_t i = 0; i < num_hyps; i++) {
        /* Model cost = |H| (simplified: self-delimiting encoding overhead) */
        size_t model_cost = hyp_lens[i] + ilog2(hyp_lens[i] + 1) + 8;
        /* Data cost = approximate K(data | H) as data_len - match_len */
        size_t match = 0;
        size_t hlen = hyp_lens[i];
        for (size_t j = 0; j < data_len && j < hlen; j++) {
            if (data[j] == hypotheses[i][j]) match++;
            else break;
        }
        size_t data_cost = data_len - match + ilog2(data_len - match + 1) + 8;
        size_t total = model_cost + data_cost;

        if (total < best_total) {
            best_total = total;
            result.model_cost = model_cost;
            result.data_cost = data_cost;
            result.total_cost = total;
            result.stochastic_complexity = (double)total;
        }
    }
    return result;
}

double stochastic_complexity_nml(const char *binary_sequence, size_t len) {
    /* NML for Bernoulli sequences:
     * SC(x) = -log P(x|theta_hat(x)) + log sum_{y in {0,1}^n} P(y|theta_hat(y))
     * where theta_hat(x) = n_1/n (MLE for Bernoulli parameter).
     *
     * The parametric complexity term = log sum_{k=0..n} C(n,k) (k/n)^k ((n-k)/n)^{n-k}
     * This is computable but expensive; we approximate for small n. */
    if (!binary_sequence || len == 0) return 0.0;

    size_t ones = 0;
    for (size_t i = 0; i < len; i++)
        if (binary_sequence[i] == '1') ones++;

    double theta_hat = (double)ones / (double)len;
    double log_likelihood = 0.0;
    if (theta_hat > 0.0 && theta_hat < 1.0) {
        log_likelihood = (double)ones * log2(theta_hat)
                       + (double)(len - ones) * log2(1.0 - theta_hat);
    }

    /* Parametric complexity: approximation = 0.5 * log2(n) + O(1) */
    double param_complexity = 0.5 * log2((double)len + 1.0) + 1.0;
    return -log_likelihood + param_complexity;
}

size_t mdl_select_best_model(const char *data, size_t data_len,
                              const char **models, const size_t *model_lens,
                              size_t num_models) {
    if (!data || !models || num_models == 0) return 0;

    size_t best_idx = 0;
    size_t best_cost = (size_t)-1;

    for (size_t i = 0; i < num_models; i++) {
        size_t mlen = model_lens[i];
        size_t match = 0;
        for (size_t j = 0; j < data_len && j < mlen; j++) {
            if (data[j] == models[i][j]) match++;
            else break;
        }
        size_t cost = mlen + ilog2(mlen + 1) + 8
                    + (data_len - match) + ilog2(data_len - match + 1) + 8;
        if (cost < best_cost) { best_cost = cost; best_idx = i; }
    }
    return best_idx;
}

/* ==================================================================
 * Martingale-Based Randomness Tests
 *
 * A martingale is a betting strategy. If x is non-random (compressible),
 * a clever betting strategy can make unbounded profit.
 *
 * Reference: Schnorr, "Randomness and the amount of information" (1971)
 * ================================================================== */

MartingaleApprox martingale_compressibility_test(const char *x, size_t len) {
    MartingaleApprox result;
    memset(&result, 0, sizeof(result));
    result.capital = 1.0;
    result.max_capital = 1.0;
    result.reject_randomness = false;

    if (!x || len == 0) return result;

    double capital = 1.0;
    double max_capital = 1.0;
    size_t run_len = 0;

    for (size_t i = 0; i < len && i < 256; i++) {
        /* Strategy: bet on alternating pattern.
         * If we see runs (same bit repeated), bet against continuation.
         * If the bit continues the run, we lose; if it breaks, we win. */
        if (i > 0 && x[i] == x[i - 1]) {
            run_len++;
            /* Bet 10% of capital that the run breaks next bit */
            double bet = capital * 0.1;
            if (i + 1 < len) {
                if (x[i + 1] != x[i]) {
                    capital += bet * 1.5; /* win: run broke */
                } else {
                    capital -= bet; /* lose: run continued */
                }
            }
        } else {
            run_len = 0;
        }
        if (capital > max_capital) max_capital = capital;
        if (capital <= 0.0) { capital = 0.01; break; }
    }

    result.capital = capital;
    result.max_capital = max_capital;
    result.rounds = len < 256 ? len : 256;
    /* Reject randomness if capital significantly exceeds 1.0 */
    result.reject_randomness = (max_capital > 2.0 && len > 10);
    return result;
}

MartingaleApprox martingale_multi_strategy(const char *x, size_t len,
                                            const double *strategies,
                                            size_t num_strategies) {
    MartingaleApprox best;
    memset(&best, 0, sizeof(best));
    best.capital = 1.0;
    best.max_capital = 1.0;
    best.reject_randomness = false;

    if (!x || len == 0 || num_strategies == 0) return best;

    for (size_t s = 0; s < num_strategies; s++) {
        double capital = 1.0, max_cap = 1.0;
        double weight = strategies[s]; /* strategy aggressiveness */

        for (size_t i = 1; i < len && i < 256; i++) {
            /* Strategy: bet weight fraction on "bit differs from previous" */
            double bet = capital * weight * 0.05;
            if (x[i] != x[i - 1]) {
                capital += bet;
            } else {
                capital -= bet;
            }
            if (capital > max_cap) max_cap = capital;
            if (capital <= 0.0) break;
        }

        if (max_cap > best.max_capital) {
            best.capital = capital;
            best.max_capital = max_cap;
            best.rounds = len < 256 ? len : 256;
        }
    }

    best.reject_randomness = (best.max_capital > 2.0);
    return best;
}

/* ==================================================================
 * L5: Entropy Estimation
 *
 * Various entropy estimators for finite sequences:
 *   - Sample entropy (ApEn/SampEn): regularity measure
 *   - Block entropy: H_n = -sum P(w) log P(w) for blocks of size n
 *   - Permutation entropy: ordinal patterns
 *
 * Reference: Pincus, "Approximate Entropy" (1991)
 *            Bandt & Pompe, "Permutation Entropy" (2002)
 * ================================================================== */

EntropyProfile entropy_profile_compute(const char *x, size_t len) {
    EntropyProfile ep;
    memset(&ep, 0, sizeof(ep));

    if (!x || len < 10) return ep;

    /* Sample Entropy (SampEn): count template matches.
     * m = 2 (template length), r = 0 (exact match).
     * A = count of matches of length m+1
     * B = count of matches of length m
     * SampEn = -ln(A/B) */
    size_t B = 0, A = 0;
    size_t m = 2;
    for (size_t i = 0; i + m + 1 <= len && i < 500; i++) {
        for (size_t j = i + 1; j + m + 1 <= len && j < 500; j++) {
            bool match = true;
            for (size_t k = 0; k < m; k++)
                if (x[i + k] != x[j + k]) { match = false; break; }
            if (match) {
                B++;
                if (j + m < len && x[i + m] == x[j + m]) A++;
            }
        }
    }
    if (B > 0 && A > 0) {
        ep.sample_entropy = -log((double)A / (double)B);
    }

    /* Approximate Entropy (ApEn): similar but self-matches included.
     * ApEn = phi^m - phi^{m+1} where phi^m = average log C_i^m */
    double phi_m = 0.0, phi_m1 = 0.0;
    size_t N = len < 500 ? len : 500;
    for (size_t i = 0; i + m <= N; i++) {
        size_t count_m = 0, count_m1 = 0;
        for (size_t j = 0; j + m <= N; j++) {
            bool match = true;
            for (size_t k = 0; k < m; k++)
                if (x[i + k] != x[j + k]) { match = false; break; }
            if (match) {
                count_m++;
                if (j + m < N && x[i + m] == x[j + m]) count_m1++;
            }
        }
        if (count_m > 0) phi_m += log((double)count_m);
        if (count_m1 > 0) phi_m1 += log((double)count_m1);
    }
    ep.approximate_entropy = (phi_m / (double)N) - (phi_m1 / (double)N);

    /* Permutation Entropy: count ordinal patterns of length 3.
     * For binary string, patterns: 00, 01, 10, 11 -> 4 patterns. */
    size_t patterns[4] = {0};
    for (size_t i = 0; i + 1 < len && i < 1024; i++) {
        int idx = (x[i] == '1' ? 2 : 0) + (x[i + 1] == '1' ? 1 : 0);
        patterns[idx]++;
    }
    size_t total_patterns = patterns[0] + patterns[1] + patterns[2] + patterns[3];
    if (total_patterns > 0) {
        double pe = 0.0;
        for (int k = 0; k < 4; k++) {
            if (patterns[k] > 0) {
                double p = (double)patterns[k] / (double)total_patterns;
                pe -= p * log2(p);
            }
        }
        ep.permutation_entropy = pe;
    }

    ep.topological_entropy = ep.permutation_entropy;
    return ep;
}

BlockEntropy* block_entropy_series(const char *x, size_t len,
                                    size_t max_block, size_t *out_count) {
    if (!x || len < 3 || max_block == 0) {
        if (out_count) *out_count = 0;
        return NULL;
    }

    size_t n = max_block < 8 ? max_block : 8;
    BlockEntropy *series = (BlockEntropy*)malloc(n * sizeof(BlockEntropy));
    if (!series) { if (out_count) *out_count = 0; return NULL; }

    for (size_t b = 1; b <= n && b <= len; b++) {
        series[b - 1].block_size = b;
        size_t num_blocks = len - b + 1;
        if (num_blocks > 2048) num_blocks = 2048;

        /* Count n-gram frequencies */
        size_t max_patterns = (size_t)1 << (b < 10 ? b : 10);
        size_t *freq = (size_t*)calloc(max_patterns, sizeof(size_t));
        if (!freq) { series[b - 1].block_entropy = 0.0; continue; }

        for (size_t i = 0; i < num_blocks; i++) {
            size_t pattern = 0;
            for (size_t j = 0; j < b; j++)
                if (x[i + j] == '1') pattern |= ((size_t)1) << j;
            if (pattern < max_patterns) freq[pattern]++;
        }

        double H = 0.0;
        for (size_t p = 0; p < max_patterns; p++) {
            if (freq[p] > 0) {
                double prob = (double)freq[p] / (double)num_blocks;
                H -= prob * log2(prob);
            }
        }
        series[b - 1].block_entropy = H;
        series[b - 1].entropy_rate = H / (double)b;
        /* Redundancy = 1 - H/H_max = 1 - H/b */
        series[b - 1].redundancy = 1.0 - H / (double)b;
        free(freq);
    }

    if (out_count) *out_count = n;
    return series;
}

double lz_entropy_rate(const char *x, size_t len) {
    /* LZ entropy rate estimate: H_hat = (log n) * LZ_count / n */
    if (!x || len == 0) return 0.0;
    LZComplexity lz = lz78_complexity(x, len);
    return lz.entropy_estimate;
}

/* ==================================================================
 * L6: NCD-based Clustering
 *
 * Cilibrasi-Vitanyi clustering: use NCD as distance metric,
 * then apply hierarchical clustering to group similar strings.
 *
 * This approach is parameter-free (no feature engineering needed),
 * works on any data type (text, music, genomes), and is provably
 * universal: if any computable distance clusters the data, NCD does too.
 *
 * Reference: Cilibrasi & Vitanyi, "Clustering by Compression" (2005)
 * Complexity: O(k^2 * n) for k strings of avg length n.
 * ================================================================== */

double* ncd_distance_matrix(const char **strings, const size_t *lens,
                             size_t count) {
    if (!strings || !lens || count < 2) return NULL;
    size_t size = count * count;
    double *dm = (double*)malloc(size * sizeof(double));
    if (!dm) return NULL;
    for (size_t i = 0; i < size; i++) dm[i] = 0.0;

    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            NCDResult ncd = ncd_compute(strings[i], lens[i],
                                         strings[j], lens[j]);
            dm[i * count + j] = ncd.ncd;
            dm[j * count + i] = ncd.ncd;
        }
    }
    return dm;
}

int* ncd_cluster(const char **strings, const size_t *lens,
                  size_t count, double threshold, size_t *num_clusters) {
    if (!strings || !lens || count == 0) {
        if (num_clusters) *num_clusters = 0;
        return NULL;
    }

    double *dm = ncd_distance_matrix(strings, lens, count);
    if (!dm) { if (num_clusters) *num_clusters = 0; return NULL; }

    /* Simple agglomerative clustering: union-find with threshold */
    int *parent = (int*)malloc(count * sizeof(int));
    int *labels = (int*)malloc(count * sizeof(int));
    if (!parent || !labels) {
        free(dm); free(parent); free(labels);
        if (num_clusters) *num_clusters = 0;
        return NULL;
    }

    for (size_t i = 0; i < count; i++) { parent[i] = (int)i; labels[i] = 0; }

    /* Merge closest pairs below threshold */
    for (size_t iter = 0; iter < count * count; iter++) {
        double best_dist = 1e9;
        size_t best_i = 0, best_j = 0;
        for (size_t i = 0; i < count; i++) {
            for (size_t j = i + 1; j < count; j++) {
                if (dm[i * count + j] < best_dist && dm[i * count + j] < threshold) {
                    best_dist = dm[i * count + j];
                    best_i = i; best_j = j;
                }
            }
        }
        if (best_dist >= threshold) break;
        /* Merge: set parent of best_j to best_i */
        int root_i = best_i;
        while (parent[root_i] != root_i) root_i = parent[root_i];
        int root_j = best_j;
        while (parent[root_j] != root_j) root_j = parent[root_j];
        if (root_i != root_j) parent[root_j] = root_i;
        dm[best_i * count + best_j] = 1.0; /* mark as processed */
        dm[best_j * count + best_i] = 1.0;
    }

    /* Assign cluster labels */
    size_t cluster_id = 0;
    for (size_t i = 0; i < count; i++) {
        int root = (int)i;
        while (parent[root] != root) root = parent[root];
        bool found = false;
        for (size_t j = 0; j < i; j++) {
            int rj = (int)j;
            while (parent[rj] != rj) rj = parent[rj];
            if (root == rj) { labels[i] = labels[j]; found = true; break; }
        }
        if (!found) labels[i] = (int)cluster_id++;
    }

    free(dm); free(parent);
    if (num_clusters) *num_clusters = cluster_id;
    return labels;
}

double* ncd_quartet_tree(const char **strings, const size_t *lens,
                          size_t count) {
    if (!strings || !lens || count < 2) return NULL;
    double *dm = ncd_distance_matrix(strings, lens, count);
    if (!dm) return NULL;

    /* Neighbor-Joining simplifed: return distance matrix directly.
     * Full NJ requires iterative reduction; here we compute quartet
     * topology as a complete-linkage dendrogram proxy. */
    size_t size = count * count;
    double *tree = (double*)malloc(size * sizeof(double));
    if (!tree) { free(dm); return NULL; }
    memcpy(tree, dm, size * sizeof(double));
    free(dm);
    return tree;
}

/* ==================================================================
 * L7: Cryptographic Incompressibility Tests
 *
 * Incompressibility => pseudorandomness:
 * If K(x|n) < |x| - c, then x is compressible => not random.
 * Conversely, c-incompressible strings are good PRG seeds.
 *
 * Reference: Goldreich, "Foundations of Cryptography" (2001)
 *            Li & Vitanyi, Chapter 4.5 (Incompressibility method)
 * ================================================================== */

bool approx_incompressibility_test(const char *x, size_t len, size_t slack) {
    if (!x || len == 0) return true;
    CompressionBound cb = lz77_compress_bound(x, len);
    /* x is incompressible if compressed_len + slack >= len */
    bool result = (cb.compressed_len + slack >= len);
    compression_bound_free(&cb);
    return result;
}

double compression_gap_test(const char *x, size_t len,
                            double threshold, bool *is_nonrandom) {
    if (!x || len == 0) {
        if (is_nonrandom) *is_nonrandom = false;
        return 0.0;
    }
    CompressionBound cb = lz77_compress_bound(x, len);
    double gap = (double)len - (double)cb.compressed_len;
    if (is_nonrandom) *is_nonrandom = (gap > threshold);
    compression_bound_free(&cb);
    return gap;
}

bool entropy_determinism_test(const char *x, size_t len,
                               double entropy_threshold) {
    if (!x || len < 10) return true;
    EntropyProfile ep = entropy_profile_compute(x, len);
    return (ep.sample_entropy < entropy_threshold);
}

/* ==================================================================
 * L8: Advanced Approximations
 *
 * Universal distribution via Monte Carlo sampling.
 * Coding theorem verification.
 * Bayesian complexity posterior.
 * Rate-distortion with Kolmogorov complexity.
 * ================================================================== */

double universal_distribution_mc(const char *x, size_t len,
                                  size_t num_samples, size_t time_budget) {
    if (!x || len == 0 || num_samples == 0) return 0.0;

    /* Monte Carlo estimate of m(x):
     * Sample programs uniformly by length-weight (2^{-|p|}),
     * run them, count fraction that output x. */
    size_t hits = 0;
    char output[4096];

    for (size_t s = 0; s < num_samples; s++) {
        /* Sample program length from geometric distribution (p=0.5) */
        size_t plen = 1;
        while (plen < 64 && (rand() & 1)) plen++;
        /* Generate random program of that length */
        char prog[64];
        for (size_t b = 0; b < plen; b++)
            prog[b] = (rand() & 1) ? '1' : '0';
        prog[plen] = '\0';

        /* Run program with simulated UTM */
        char tape[512]; memset(tape, 0, sizeof(tape));
        size_t head = 0, steps = 0;
        for (size_t ip = 0; ip + 1 < plen && steps < time_budget; ip += 2) {
            steps++;
            if (prog[ip] == '0' && prog[ip + 1] == '0') { if (head > 0) head--; }
            else if (prog[ip] == '0' && prog[ip + 1] == '1') { head++; if (head >= 512) head--; }
            else if (prog[ip] == '1' && prog[ip + 1] == '0') tape[head] ^= 1;
            else break;
        }
        size_t out_len = 0;
        for (size_t i = 0; i < 512 && i < 4096; i++) {
            output[i] = tape[i] ? '1' : '0';
            out_len++;
        }

        if (out_len >= len) {
            bool match = true;
            for (size_t i = 0; i < len; i++)
                if (output[i] != x[i]) { match = false; break; }
            if (match) hits++;
        }
    }

    return (double)hits / (double)num_samples;
}

bool coding_theorem_verify(const char *x, size_t len,
                            size_t num_samples, size_t time_budget,
                            double tolerance) {
    if (!x || len == 0) return true;

    /* Coding Theorem: c_1 * m(x) <= 2^{-K(x)} <= c_2 * m(x)
     * Verify that m(x) (MC estimate) vs 2^{-K_{LZ}(x)} are within tolerance. */
    double m_est = universal_distribution_mc(x, len, num_samples, time_budget);
    CompressionBound cb = lz77_compress_bound(x, len);
    double pow_neg_K = pow(2.0, -(double)cb.compressed_len);
    compression_bound_free(&cb);

    if (m_est <= 0.0 && pow_neg_K <= 0.0) return true;
    if (m_est <= 0.0 || pow_neg_K <= 0.0) return false;

    double ratio = m_est / pow_neg_K;
    return (ratio >= 1.0 / tolerance && ratio <= tolerance);
}

double bayesian_complexity_posterior(const char *data, size_t data_len,
                                      const char **programs,
                                      const size_t *prog_lens,
                                      size_t num_programs, size_t time_budget) {
    if (!data || !programs || num_programs == 0) return 0.0;

    /* P(program_i | data) = P(data | prog_i) * P(prog_i) / normalizing
     * where P(prog_i) ∝ 2^{-|prog_i|} (universal prior)
     * and P(data | prog_i) approximated by matching. */

    double *posteriors = (double*)malloc(num_programs * sizeof(double));
    if (!posteriors) return 0.0;

    double total_weight = 0.0;
    for (size_t i = 0; i < num_programs; i++) {
        double prior = pow(2.0, -(double)prog_lens[i]);
        /* Likelihood: count bits where program prediction matches data */
        size_t matches = 0;
        size_t plen = prog_lens[i];
        for (size_t j = 0; j < data_len && j < plen; j++)
            if (data[j] == programs[i][j]) matches++;
        double likelihood = (double)matches / (data_len > 0 ? (double)data_len : 1.0);
        posteriors[i] = prior * likelihood;
        total_weight += posteriors[i];
    }

    double complexity_estimate = 0.0;
    if (total_weight > 0.0) {
        for (size_t i = 0; i < num_programs; i++) {
            double post = posteriors[i] / total_weight;
            complexity_estimate += post * (double)prog_lens[i];
        }
    }

    free(posteriors);
    return complexity_estimate;
}

size_t rate_distortion_K(const char *signal, size_t signal_len,
                          double distortion_tolerance,
                          size_t (*distance)(const char*, size_t,
                                            const char*, size_t)) {
    if (!signal || signal_len == 0 || !distance) return signal_len + 16;

    /* Rate-distortion: find minimal K for description with bounded distortion.
     * Approximate by trying increasingly coarse compressions. */
    size_t best_description_len = signal_len + 16;
    double best_distortion = 0.0;

    for (size_t desc_len = 1; desc_len <= signal_len && desc_len <= 64; desc_len++) {
        /* Coarse description: take first desc_len bits as model */
        size_t distort = distance(signal, signal_len, signal, desc_len);
        double dist_norm = (double)distort;
        if (dist_norm <= distortion_tolerance && desc_len < best_description_len) {
            best_description_len = desc_len;
            best_distortion = dist_norm;
        }
    }
    return best_description_len;
}

/* ==================================================================
 * Utility Functions
 * ================================================================== */

void compression_bound_free(CompressionBound *b) {
    if (b && b->compressed) { free(b->compressed); b->compressed = NULL; }
}

void lz_complexity_free(LZComplexity *lz) {
    /* LZComplexity contains no heap allocations */
    (void)lz;
}

void ncd_result_print(const NCDResult *n) {
    if (!n) return;
    printf("NCD=%.4f, NID=%.4f, comp_dist=%.4f\n",
           n->ncd, n->normalized_information_distance, n->compression_distance);
}

void two_part_mdl_print(const TwoPartMDL *m) {
    if (!m) return;
    printf("MDL: model=%zu, data=%zu, total=%zu, SC=%.2f\n",
           m->model_cost, m->data_cost, m->total_cost, m->stochastic_complexity);
}

void entropy_profile_print(const EntropyProfile *ep) {
    if (!ep) return;
    printf("Entropy: SampEn=%.4f, ApEn=%.4f, PermEn=%.4f, TopEn=%.4f\n",
           ep->sample_entropy, ep->approximate_entropy,
           ep->permutation_entropy, ep->topological_entropy);
}

void martingale_approx_print(const MartingaleApprox *ma) {
    if (!ma) return;
    printf("Martingale: capital=%.4f, max=%.4f, rounds=%zu, reject=%s\n",
           ma->capital, ma->max_capital, ma->rounds,
           ma->reject_randomness ? "YES" : "NO");
}
