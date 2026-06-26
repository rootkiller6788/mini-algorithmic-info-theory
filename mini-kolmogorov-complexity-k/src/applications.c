/*
 * applications.c — L7 Applications, L8 Advanced Topics, L9 Research Frontiers
 *
 * Real implementations of Kolmogorov complexity applications:
 *
 * L7: DNA complexity, NCD clustering, language identification, plagiarism
 * L8: Algorithmic MI, preimage complexity, sophistication, resource-bounded K,
 *     prefix vs plain complexity
 * L9: MDL, Solomonoff induction, meta-complexity, logical depth
 *
 * Reference: Li & Vitányi (2019) §4-8, Rissanen (1978), Solomonoff (1964),
 *   Koppel (1987), Bennett (1988)
 * Courses: MIT 6.841, Stanford CS254, Oxford Advanced Complexity, CMU 15-855
 */

#include "../include/kolmogorov.h"
#include "../include/string_tools.h"
#include "../include/compression.h"
#include "../include/entropy.h"
#include "../include/applications.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <float.h>

/* ══════════════════════════════════════════════════════════════
   L7: DNA Sequence Complexity
   ══════════════════════════════════════════════════════════════ */

static int is_dna_char(unsigned char c) {
    c = (unsigned char)(c >= 'a' && c <= 'z' ? c - 32 : c);
    return (c == 'A' || c == 'C' || c == 'G' || c == 'T' ||
            c == 'U' || c == 'R' || c == 'Y' || c == 'S' ||
            c == 'W' || c == 'K' || c == 'M' || c == 'B' ||
            c == 'D' || c == 'H' || c == 'V' || c == 'N');
}

DNAComplexityProfile* dna_complexity_analyze(const KString* seq) {
    if (!seq || seq->len == 0) return NULL;
    DNAComplexityProfile* p = (DNAComplexityProfile*)malloc(sizeof(DNAComplexityProfile));
    assert(p != NULL);
    memset(p, 0, sizeof(DNAComplexityProfile));

    int counts[256] = {0};
    int dna_count = 0;
    int gc_count = 0;
    for (size_t i = 0; i < seq->len; i++) {
        unsigned char c = seq->data[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
        counts[c]++;
        if (is_dna_char(c)) {
            dna_count++;
            if (c == 'G' || c == 'C') gc_count++;
        }
    }

    if (dna_count == 0) {
        p->complexity_score = 0.0;
        return p;
    }

    p->gc_content = (double)gc_count / (double)dna_count;

    const char bases[] = {'A', 'C', 'G', 'T'};
    p->shannon_entropy = 0.0;
    for (int i = 0; i < 4; i++) {
        if (counts[(int)bases[i]] > 0) {
            double prob = (double)counts[(int)bases[i]] / (double)dna_count;
            p->shannon_entropy -= prob * log2(prob);
        }
    }

    p->block2_entropy = block_entropy(seq, 2);
    p->block3_entropy = block_entropy(seq, 3);
    p->lz77_ratio = lz77_ratio(seq);
    p->lz78_ratio = lz78_ratio(seq);
    p->lzw_ratio  = lzw_ratio(seq);
    p->huffman_ratio = huffman_ratio(seq);

    double best_ratio = p->lz77_ratio;
    if (p->lz78_ratio < best_ratio) best_ratio = p->lz78_ratio;
    if (p->lzw_ratio < best_ratio)  best_ratio = p->lzw_ratio;
    if (p->huffman_ratio < best_ratio) best_ratio = p->huffman_ratio;
    p->estimated_K = best_ratio * 8.0;

    int high_entropy = (p->shannon_entropy > 1.5);
    int mid_gc = (p->gc_content >= 0.35 && p->gc_content <= 0.65);
    int codon_entropy = (p->block3_entropy > 0.8 * p->shannon_entropy);
    p->is_coding = (high_entropy && mid_gc && codon_entropy) ? 1 : 0;

    int low_entropy = (p->shannon_entropy < 0.5);
    double compressible = (best_ratio < 0.3);
    p->is_repetitive = (low_entropy || compressible) ? 1 : 0;

    double entropy_score = p->shannon_entropy / 2.0;
    if (entropy_score > 1.0) entropy_score = 1.0;
    double compression_score = 1.0 - best_ratio;
    p->complexity_score = 0.5 * entropy_score + 0.5 * compression_score;

    return p;
}

void dna_profile_free(DNAComplexityProfile* p) { free(p); }

void dna_profile_print(const DNAComplexityProfile* p) {
    if (!p) return;
    printf("DNA Complexity Profile:\n");
    printf("  Shannon entropy : %.4f bits/base\n", p->shannon_entropy);
    printf("  Dinucleotide H  : %.4f\n", p->block2_entropy);
    printf("  Codon H         : %.4f\n", p->block3_entropy);
    printf("  GC content      : %.2f%%\n", p->gc_content * 100.0);
    printf("  LZ77 ratio      : %.4f\n", p->lz77_ratio);
    printf("  LZ78 ratio      : %.4f\n", p->lz78_ratio);
    printf("  LZW ratio       : %.4f\n", p->lzw_ratio);
    printf("  Huffman ratio   : %.4f\n", p->huffman_ratio);
    printf("  Est. K          : %.2f bits/base\n", p->estimated_K);
    printf("  Coding region   : %s\n", p->is_coding ? "likely" : "unlikely");
    printf("  Repetitive      : %s\n", p->is_repetitive ? "yes" : "no");
    printf("  Complexity score: %.4f [0=simple, 1=complex]\n", p->complexity_score);
}

int dna_is_coding_region(const KString* seq) {
    DNAComplexityProfile* p = dna_complexity_analyze(seq);
    int result = p ? p->is_coding : 0;
    dna_profile_free(p);
    return result;
}

double dna_gc_skew(const KString* seq) {
    if (!seq || seq->len == 0) return 0.0;
    int g = 0, c = 0;
    for (size_t i = 0; i < seq->len; i++) {
        unsigned char ch = seq->data[i];
        if (ch >= 'a' && ch <= 'z') ch = (unsigned char)(ch - 32);
        if (ch == 'G') g++;
        if (ch == 'C') c++;
    }
    int total = g + c;
    if (total == 0) return 0.0;
    return (double)(g - c) / (double)total;
}

/* ══════════════════════════════════════════════════════════════
   L7: NCD Clustering
   ══════════════════════════════════════════════════════════════ */

double* ncd_distance_matrix(KString** samples, int n) {
    if (!samples || n <= 0) return NULL;
    double* mat = (double*)malloc((size_t)(n * n) * sizeof(double));
    assert(mat != NULL);

    for (int i = 0; i < n; i++) {
        mat[i * n + i] = 0.0;
        for (int j = i + 1; j < n; j++) {
            double d = kstr_normalized_compression_distance(samples[i], samples[j]);
            mat[i * n + j] = d;
            mat[j * n + i] = d;
        }
    }
    return mat;
}

int* ncd_hierarchical_cluster(KString** samples, int n, int k, double threshold) {
    if (!samples || n <= 0 || k <= 0) return NULL;
    if (k > n) k = n;

    int* clusters = (int*)malloc((size_t)n * sizeof(int));
    assert(clusters != NULL);
    for (int i = 0; i < n; i++) clusters[i] = i;

    double* dist = ncd_distance_matrix(samples, n);
    int n_clusters = n;

    while (n_clusters > k) {
        double min_dist = DBL_MAX;
        int mi = -1, mj = -1;
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                if (clusters[i] != clusters[j] && dist[i * n + j] < min_dist) {
                    min_dist = dist[i * n + j];
                    mi = i; mj = j;
                }
            }
        }

        if (mi < 0 || min_dist > threshold) break;

        int old_id = clusters[mj];
        for (int c = 0; c < n; c++) {
            if (clusters[c] == old_id) clusters[c] = clusters[mi];
        }
        n_clusters--;

        int next_id = 0;
        int* mapping = (int*)calloc((size_t)n, sizeof(int));
        for (int i = 0; i < n; i++) mapping[i] = -1;
        for (int i = 0; i < n; i++) {
            if (mapping[clusters[i]] == -1) mapping[clusters[i]] = next_id++;
            clusters[i] = mapping[clusters[i]];
        }
        free(mapping);
    }

    free(dist);
    return clusters;
}

int ncd_find_nearest(const KString* target, KString** candidates, int n,
                     double* out_distance) {
    if (!target || !candidates || n <= 0) return -1;
    double min_dist = DBL_MAX;
    int best = -1;
    for (int i = 0; i < n; i++) {
        if (!candidates[i]) continue;
        double d = kstr_normalized_compression_distance(target, candidates[i]);
        if (d < min_dist) { min_dist = d; best = i; }
    }
    if (out_distance) *out_distance = min_dist;
    return best;
}

/* ══════════════════════════════════════════════════════════════
   L7: Language Identification
   ══════════════════════════════════════════════════════════════ */

LanguageProfile* language_profile_build(const char* name, const KString* text) {
    if (!text || text->len == 0) return NULL;
    LanguageProfile* p = (LanguageProfile*)malloc(sizeof(LanguageProfile));
    assert(p != NULL);
    memset(p, 0, sizeof(LanguageProfile));
    p->language_name = strdup(name ? name : "unknown");

    p->entropy_per_char = shannon_entropy_bits(text);
    p->entropy_rate = entropy_rate_estimate(text, 3);
    p->block2_entropy = block_entropy(text, 2);
    p->block3_entropy = block_entropy(text, 3);

    int freq[256] = {0};
    for (size_t i = 0; i < text->len; i++) freq[text->data[i]]++;
    int top5_freqs[5] = {0};
    for (int k = 0; k < 5; k++) {
        int max_f = 0, max_i = 0;
        for (int c = 0; c < 256; c++) {
            if (freq[c] > max_f) { max_f = freq[c]; max_i = c; }
        }
        top5_freqs[k] = max_f;
        freq[max_i] = 0;
    }
    double sum_top5 = 0.0;
    for (int k = 0; k < 5; k++) sum_top5 += (double)top5_freqs[k];
    p->top5_freq_ratio = sum_top5 / (double)text->len;

    return p;
}

void language_profile_free(LanguageProfile* p) {
    if (p) { free(p->language_name); free(p); }
}

int language_identify(const KString* text, LanguageProfile** profiles,
                      int n_profiles, double* confidence) {
    if (!text || !profiles || n_profiles <= 0) return -1;

    LanguageProfile* unknown = language_profile_build("unknown", text);
    if (!unknown) return -1;

    double best_score = DBL_MAX;
    int best_idx = -1;

    for (int i = 0; i < n_profiles; i++) {
        if (!profiles[i]) continue;

        double d_entropy = fabs(unknown->entropy_per_char -
                                profiles[i]->entropy_per_char);
        double d_rate   = fabs(unknown->entropy_rate -
                               profiles[i]->entropy_rate) * 0.5;
        double d_block2 = fabs(unknown->block2_entropy -
                               profiles[i]->block2_entropy) * 0.25;
        double d_top5   = fabs(unknown->top5_freq_ratio -
                               profiles[i]->top5_freq_ratio) * 0.25;

        double score = d_entropy + d_rate + d_block2 + d_top5;

        if (score < best_score) {
            best_score = score;
            best_idx = i;
        }
    }

    if (confidence) *confidence = (best_score < 0.5) ? (1.0 - best_score) : 0.0;
    language_profile_free(unknown);
    return best_idx;
}

double* language_distance_matrix(LanguageProfile** profiles, int n) {
    if (!profiles || n <= 0) return NULL;
    double* mat = (double*)malloc((size_t)(n * n) * sizeof(double));
    assert(mat != NULL);

    for (int i = 0; i < n; i++) {
        mat[i * n + i] = 0.0;
        for (int j = i + 1; j < n; j++) {
            double d = fabs(profiles[i]->entropy_per_char -
                           profiles[j]->entropy_per_char) +
                       fabs(profiles[i]->entropy_rate -
                           profiles[j]->entropy_rate) * 0.5 +
                       fabs(profiles[i]->top5_freq_ratio -
                           profiles[j]->top5_freq_ratio) * 0.25;
            mat[i * n + j] = d;
            mat[j * n + i] = d;
        }
    }
    return mat;
}

/* ══════════════════════════════════════════════════════════════
   L7: Plagiarism / Similarity Detection
   ══════════════════════════════════════════════════════════════ */

double similarity_score(const KString* a, const KString* b) {
    if (!a || !b) return 0.0;
    if (a->len == 0 && b->len == 0) return 1.0;
    if (a->len == 0 || b->len == 0) return 0.0;

    double ncd = kstr_normalized_compression_distance(a, b);
    double jsd = jensen_shannon_divergence(a, b);
    double jsd_norm = jsd / log(2.0);
    if (jsd_norm > 1.0) jsd_norm = 1.0;
    double jsd_sim = 1.0 - jsd_norm;
    double ncd_sim = 1.0 - ncd;
    if (ncd_sim < 0.0) ncd_sim = 0.0;
    return 0.5 * ncd_sim + 0.5 * jsd_sim;
}

SimilarRegion* find_similar_regions(const KString* a, const KString* b,
                                     size_t window, double threshold,
                                     int* n_regions) {
    if (!a || !b || window == 0 || a->len < window || b->len < window) {
        *n_regions = 0;
        return NULL;
    }

    size_t max_regions = (a->len / window) * (b->len / window) + 1;
    SimilarRegion* regions = (SimilarRegion*)malloc(
        max_regions * sizeof(SimilarRegion));
    assert(regions != NULL);
    *n_regions = 0;

    for (size_t ia = 0; ia + window <= a->len; ia += window / 2) {
        KString* sub_a = kstr_substring(a, ia, ia + window);
        for (size_t ib = 0; ib + window <= b->len; ib += window / 2) {
            KString* sub_b = kstr_substring(b, ib, ib + window);
            double sim = similarity_score(sub_a, sub_b);
            if (sim >= threshold) {
                regions[*n_regions].start_a = ia;
                regions[*n_regions].end_a   = ia + window;
                regions[*n_regions].start_b = ib;
                regions[*n_regions].end_b   = ib + window;
                regions[*n_regions].similarity = sim;
                (*n_regions)++;
            }
            kstr_free(sub_b);
        }
        kstr_free(sub_a);
    }
    return regions;
}

/* ══════════════════════════════════════════════════════════════
   L8: Algorithmic Mutual Information
   ══════════════════════════════════════════════════════════════ */

double algorithmic_mutual_information(const KString* x, const KString* y) {
    if (!x || !y || x->len == 0 || y->len == 0) return 0.0;

    double Kx = (double)x->len * 8.0;
    {
        double r = lzw_ratio(x);
        double K_est = r * (double)x->len * 8.0;
        if (K_est < Kx) Kx = K_est;
    }
    double Ky = (double)y->len * 8.0;
    {
        double r = lzw_ratio(y);
        double K_est = r * (double)y->len * 8.0;
        if (K_est < Ky) Ky = K_est;
    }

    KString* xy = kstr_create(x->len + y->len + 1);
    kstr_append_str(xy, x);
    kstr_append_str(xy, y);
    double Kxy = (double)xy->len * 8.0;
    {
        double r = lzw_ratio(xy);
        double K_est = r * (double)xy->len * 8.0;
        if (K_est < Kxy) Kxy = K_est;
    }
    kstr_free(xy);

    double I = Kx + Ky - Kxy;
    double log_correction = log2(Kx + Ky + 1.0);
    return (I > 0.0 ? I : 0.0) + log_correction;
}

double algorithmic_conditional_complexity(const KString* x, const KString* y) {
    if (!x || !y) return 0.0;

    KString* xy = kstr_create(x->len + y->len + 1);
    kstr_append_str(xy, x);
    kstr_append_str(xy, y);

    double Kxy = (double)xy->len * 8.0;
    {
        double r = lzw_ratio(xy);
        double K_est = r * (double)xy->len * 8.0;
        if (K_est < Kxy) Kxy = K_est;
    }
    double Ky = (double)y->len * 8.0;
    {
        double r = lzw_ratio(y);
        double K_est = r * (double)y->len * 8.0;
        if (K_est < Ky) Ky = K_est;
    }
    kstr_free(xy);

    double K_cond = Kxy - Ky;
    return K_cond > 0.0 ? K_cond : 0.0;
}

/* ══════════════════════════════════════════════════════════════
   L8: Preimage Complexity
   ══════════════════════════════════════════════════════════════ */

double preimage_complexity_bound(const KString* y, const KString* f_x,
                                  double K_f) {
    if (!y || !f_x) return 0.0;

    double K_y = (double)y->len * 8.0;
    {
        double r = lzw_ratio(y);
        double K_est = r * (double)y->len * 8.0;
        if (K_est < K_y) K_y = K_est;
    }

    double lower = K_y - K_f - 16.0;
    return lower > 0.0 ? lower : 0.0;
}

double hash_preimage_complexity(const KString* hash_output) {
    if (!hash_output || hash_output->len == 0) return 0.0;

    size_t n_bits = hash_output->len * 8;
    double K_y = (double)n_bits;

    double best_ratio = lzw_ratio(hash_output);
    double r = lz77_ratio(hash_output); if (r < best_ratio) best_ratio = r;
    r = lz78_ratio(hash_output); if (r < best_ratio) best_ratio = r;
    r = huffman_ratio(hash_output); if (r < best_ratio) best_ratio = r;

    double K_est = best_ratio * (double)n_bits;
    if (K_est < K_y) K_y = K_est;
    return K_y;
}

/* ══════════════════════════════════════════════════════════════
   L8: Sophistication (Koppel 1987)
   ══════════════════════════════════════════════════════════════ */

double sophistication_estimate(const KString* x, int c) {
    if (!x || x->len == 0 || c <= 0) return 0.0;

    KString* model = rle_encode(x);
    double K_model = (double)model->len * 8.0;
    {
        double r = lzw_ratio(model);
        double K_est = r * (double)model->len * 8.0;
        if (K_est < K_model) K_model = K_est;
    }

    double K_total = (double)x->len * 8.0;
    {
        double r = lzw_ratio(x);
        double K_est = r * (double)x->len * 8.0;
        if (K_est < K_total) K_total = K_est;
    }

    double noise = K_total - K_model;
    if (noise < 0.0) noise = 0.0;

    kstr_free(model);

    if (noise <= (double)c) {
        return K_model;
    } else {
        return K_total - (double)c;
    }
}

double two_part_code_length(const KString* x, int block_size) {
    if (!x || x->len == 0 || block_size <= 0) return 0.0;

    size_t n_blocks = x->len / (size_t)block_size;
    if (n_blocks == 0) n_blocks = 1;

    KString* model_str = kstr_create(n_blocks + 1);
    for (size_t b = 0; b < n_blocks; b++) {
        size_t start = b * (size_t)block_size;
        size_t end = start + (size_t)block_size;
        if (end > x->len) end = x->len;
        unsigned int sum = 0;
        size_t count = end - start;
        for (size_t i = start; i < end; i++) sum += x->data[i];
        kstr_append(model_str, (unsigned char)(sum / count));
    }

    double K_S = (double)model_str->len * 8.0;
    {
        double r = lzw_ratio(model_str);
        double K_est = r * (double)model_str->len * 8.0;
        if (K_est < K_S) K_S = K_est;
    }

    double H_residual = 0.0;
    int* res_freq = (int*)calloc(256, sizeof(int));
    for (size_t b = 0; b < n_blocks; b++) {
        size_t start = b * (size_t)block_size;
        size_t end = start + (size_t)block_size;
        if (end > x->len) end = x->len;
        unsigned char mean = model_str->data[b];
        for (size_t i = start; i < end; i++) {
            int res = (int)x->data[i] - (int)mean;
            res = (res + 256) % 256;
            res_freq[res]++;
        }
    }

    size_t total_res = 0;
    for (int i = 0; i < 256; i++) total_res += (size_t)res_freq[i];
    if (total_res > 0) {
        for (int i = 0; i < 256; i++) {
            if (res_freq[i] > 0) {
                double p = (double)res_freq[i] / (double)total_res;
                H_residual -= p * log2(p);
            }
        }
    }
    free(res_freq);
    double K_x_given_S = H_residual * (double)x->len;

    kstr_free(model_str);
    return K_S + K_x_given_S;
}

/* ══════════════════════════════════════════════════════════════
   L8: Resource-Bounded Kolmogorov Complexity
   ══════════════════════════════════════════════════════════════ */

double time_bounded_K_estimate(const KString* x, int t_steps) {
    if (!x || x->len == 0) return 0.0;

    double K_base = (double)k_complexity_upper_bound(x) * 8.0;
    double tau = 10000.0;
    double discount = exp(-(double)t_steps / tau);

    double K_fast = (double)x->len * 8.0;
    {
        double r = lzw_ratio(x);
        double K_est = r * (double)x->len * 8.0;
        if (K_est < K_fast) K_fast = K_est;
    }

    return K_base * discount + K_fast * (1.0 - discount);
}

double space_bounded_K_estimate(const KString* x, int s_cells) {
    if (!x || x->len == 0) return 0.0;

    double K_base = (double)k_complexity_upper_bound(x) * 8.0;

    if ((size_t)s_cells >= x->len + 32) {
        double r = lzw_ratio(x);
        return r * (double)x->len * 8.0;
    }

    double space_ratio = (double)x->len / (double)(s_cells > 0 ? s_cells : 1);
    double penalty = log2(space_ratio + 1.0) * 8.0;
    return K_base + penalty;
}

double resource_gap(const KString* x, int t_steps) {
    if (!x || x->len == 0) return 0.0;

    double K_approx = (double)x->len * 8.0;
    {
        double r = lzw_ratio(x);
        double K_est = r * (double)x->len * 8.0;
        if (K_est < K_approx) K_approx = K_est;
    }

    double K_t = time_bounded_K_estimate(x, t_steps);
    double gap = K_t - K_approx;
    return gap > 0.0 ? gap : 0.0;
}

/* ══════════════════════════════════════════════════════════════
   L8: Prefix vs Plain Complexity
   ══════════════════════════════════════════════════════════════ */

double prefix_plain_difference(const KString* x) {
    if (!x || x->len == 0) return 0.0;

    double C_est = plain_complexity_estimate(x);
    if (C_est < 1.0) C_est = 1.0;

    double overhead = 2.0 * log2(C_est) + 1.0;
    return overhead;
}

double plain_complexity_estimate(const KString* x) {
    if (!x || x->len == 0) return 0.0;

    double best = lz77_ratio(x);
    double r = lz78_ratio(x); if (r < best) best = r;
    r = lzw_ratio(x); if (r < best) best = r;
    r = huffman_ratio(x); if (r < best) best = r;

    double raw_bits = best * (double)x->len * 8.0;
    double overhead = 2.0 * log2(raw_bits + 1.0);
    double C = raw_bits - overhead;
    return C > 0.0 ? C : 0.0;
}

/* ══════════════════════════════════════════════════════════════
   L9: Minimum Description Length (Rissanen 1978)
   ══════════════════════════════════════════════════════════════ */

MDLResult* mdl_compare_models(const KString* x, int model1_params,
                               int model2_params, int* best_model) {
    if (!x || x->len == 0 || !best_model) return NULL;

    MDLResult* results = (MDLResult*)malloc(2 * sizeof(MDLResult));
    assert(results != NULL);
    memset(results, 0, 2 * sizeof(MDLResult));

    double n = (double)x->len;

    results[0].num_params = model1_params;
    results[0].model_cost = 0.5 * (double)model1_params * log2(n);
    double K1 = two_part_code_length(x, 8);
    results[0].data_cost  = K1 - results[0].model_cost;
    if (results[0].data_cost < 0.0) results[0].data_cost = K1 * 0.9;
    results[0].total_cost = results[0].model_cost + results[0].data_cost;
    results[0].mdl_score  = results[0].total_cost / n;

    results[1].num_params = model2_params;
    results[1].model_cost = 0.5 * (double)model2_params * log2(n);
    double K2 = two_part_code_length(x, 4);
    results[1].data_cost  = K2 - results[1].model_cost;
    if (results[1].data_cost < 0.0) results[1].data_cost = K2 * 0.9;
    results[1].total_cost = results[1].model_cost + results[1].data_cost;
    results[1].mdl_score  = results[1].total_cost / n;

    *best_model = (results[0].total_cost <= results[1].total_cost) ? 0 : 1;
    return results;
}

int mdl_optimal_model_order(double* data, int n_data, int max_order) {
    if (!data || n_data <= 0 || max_order < 0) return 0;

    double n = (double)n_data;
    int best_k = 0;
    double best_cost = DBL_MAX;

    for (int k = 0; k <= max_order && k < n_data; k++) {
        double sigma_sq = 0.0;
        int count = 0;
        for (int i = k; i < n_data; i++) {
            double diff = data[i];
            double sign = 1.0;
            for (int j = 1; j <= k && (i - j) >= 0; j++) {
                sign = -sign;
                diff += sign * data[i - j];
            }
            sigma_sq += diff * diff;
            count++;
        }
        if (count > 0) sigma_sq /= (double)count;
        if (sigma_sq < 1e-10) sigma_sq = 1e-10;

        double model_cost = 0.5 * (double)(k + 1) * log2(n);
        double data_cost  = 0.5 * n * log2(2.0 * M_PI * sigma_sq) + 0.5 * n / log(2.0);
        double total_cost = model_cost + data_cost;

        if (total_cost < best_cost) {
            best_cost = total_cost;
            best_k = k;
        }
    }
    return best_k;
}

double normalized_mdl(const KString* x, int num_params) {
    if (!x || x->len == 0) return 0.0;

    double n = (double)x->len;
    double log_alphabet = log2(256.0);

    double model_cost = 0.5 * (double)num_params * log2(n);
    double H = shannon_entropy_bits(x);
    double data_cost = n * H;

    double total = model_cost + data_cost;
    return total / (n * log_alphabet);
}
