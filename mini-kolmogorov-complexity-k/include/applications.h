/*
 * applications.h — L7 Applications, L8 Advanced Topics, L9 Research Frontiers
 *
 * L7: Practical applications of Kolmogorov complexity
 *   - DNA sequence complexity analysis
 *   - File clustering via Normalized Compression Distance
 *   - Language identification via entropy profiles
 *   - Plagiarism / similarity detection
 *
 * L8: Advanced Topics
 *   - Algorithmic mutual information K(x:y)
 *   - Preimage complexity K(f^{-1}(y))
 *   - Sophistication (Koppel 1987): meaningful complexity
 *   - Resource-bounded Kolmogorov K^t(x)
 *   - Prefix vs plain complexity relationship
 *
 * L9: Research Frontiers
 *   - Minimum Description Length (Rissanen 1978)
 *   - Solomonoff universal induction (Solomonoff 1964)
 *   - Meta-complexity: K(K(x))
 *   - Logical Depth (Bennett 1988)
 *
 * Reference: Li & Vitányi (2019) §4-8, Cover & Thomas §14,
 *   Rissanen (1978), Solomonoff (1964), Koppel (1987)
 * Courses: MIT 6.841 §4-5, Stanford CS254 §4, Oxford Advanced Complexity §5
 */

#ifndef APPLICATIONS_H
#define APPLICATIONS_H

#include "kolmogorov.h"
#include "string_tools.h"
#include "compression.h"
#include "entropy.h"

/* ══════════════════════════════════════════════════════════════
   L7: DNA Sequence Complexity
   ══════════════════════════════════════════════════════════════ */

typedef struct {
    double  shannon_entropy;     /* Shannon H per base */
    double  block2_entropy;      /* Dinucleotide entropy */
    double  block3_entropy;      /* Codon entropy */
    double  lz77_ratio;          /* LZ77 compressibility */
    double  lz78_ratio;          /* LZ78 compressibility */
    double  lzw_ratio;           /* LZW compressibility */
    double  huffman_ratio;       /* Huffman compressibility */
    double  estimated_K;         /* K(x) estimate (bits per base) */
    int     is_coding;           /* 1 if likely coding region */
    int     is_repetitive;       /* 1 if repeat region */
    double  gc_content;          /* GC content ratio */
    double  complexity_score;    /* Overall complexity score [0,1] */
} DNAComplexityProfile;

DNAComplexityProfile* dna_complexity_analyze(const KString* seq);
void dna_profile_free(DNAComplexityProfile* p);
void dna_profile_print(const DNAComplexityProfile* p);
int dna_is_coding_region(const KString* seq);
double dna_gc_skew(const KString* seq);

/* ══════════════════════════════════════════════════════════════
   L7: File Clustering via NCD
   ══════════════════════════════════════════════════════════════ */

typedef struct {
    int     cluster_id;
    double  distance_to_centroid;
} NCDClusterResult;

double* ncd_distance_matrix(KString** samples, int n);
int* ncd_hierarchical_cluster(KString** samples, int n, int k, double threshold);
int ncd_find_nearest(const KString* target, KString** candidates, int n,
                     double* out_distance);

/* ══════════════════════════════════════════════════════════════
   L7: Language Identification via Entropy Profiles
   ══════════════════════════════════════════════════════════════ */

typedef struct {
    char*   language_name;
    double  entropy_per_char;
    double  entropy_rate;
    double  block2_entropy;
    double  block3_entropy;
    double  top5_freq_ratio;
    double  jsd_to_english;
} LanguageProfile;

LanguageProfile* language_profile_build(const char* name, const KString* text);
void language_profile_free(LanguageProfile* p);
int language_identify(const KString* text, LanguageProfile** profiles,
                      int n_profiles, double* confidence);
double* language_distance_matrix(LanguageProfile** profiles, int n);

/* ══════════════════════════════════════════════════════════════
   L7: Plagiarism / Similarity Detection
   ══════════════════════════════════════════════════════════════ */

double similarity_score(const KString* a, const KString* b);

typedef struct {
    size_t start_a, end_a;
    size_t start_b, end_b;
    double similarity;
} SimilarRegion;

SimilarRegion* find_similar_regions(const KString* a, const KString* b,
                                     size_t window, double threshold,
                                     int* n_regions);

/* ══════════════════════════════════════════════════════════════
   L8: Algorithmic Mutual Information
   ══════════════════════════════════════════════════════════════ */

double algorithmic_mutual_information(const KString* x, const KString* y);
double algorithmic_conditional_complexity(const KString* x, const KString* y);

/* ══════════════════════════════════════════════════════════════
   L8: Preimage Complexity
   ══════════════════════════════════════════════════════════════ */

double preimage_complexity_bound(const KString* y, const KString* f_x,
                                  double K_f);
double hash_preimage_complexity(const KString* hash_output);

/* ══════════════════════════════════════════════════════════════
   L8: Sophistication (Koppel 1987)
   ══════════════════════════════════════════════════════════════ */

double sophistication_estimate(const KString* x, int c);
double two_part_code_length(const KString* x, int block_size);

/* ══════════════════════════════════════════════════════════════
   L8: Resource-Bounded Kolmogorov Complexity
   ══════════════════════════════════════════════════════════════ */

double time_bounded_K_estimate(const KString* x, int t_steps);
double space_bounded_K_estimate(const KString* x, int s_cells);
double resource_gap(const KString* x, int t_steps);

/* ══════════════════════════════════════════════════════════════
   L8: Prefix vs Plain Complexity
   ══════════════════════════════════════════════════════════════ */

double prefix_plain_difference(const KString* x);
double plain_complexity_estimate(const KString* x);

/* ══════════════════════════════════════════════════════════════
   L9: Minimum Description Length (Rissanen 1978)
   ══════════════════════════════════════════════════════════════ */

typedef struct {
    double  model_cost;
    double  data_cost;
    double  total_cost;
    int     num_params;
    double  mdl_score;
} MDLResult;

MDLResult* mdl_compare_models(const KString* x, int model1_params,
                               int model2_params, int* best_model);
int mdl_optimal_model_order(double* data, int n_data, int max_order);
double normalized_mdl(const KString* x, int num_params);

/* ══════════════════════════════════════════════════════════════
   L9: Solomonoff Universal Induction (Solomonoff 1964)
   ══════════════════════════════════════════════════════════════ */

double* solomonoff_next_symbol_dist(const KString* x);
double solomonoff_universal_probability(const KString* x);
double solomonoff_predictive_gain(const KString* x);

/* ══════════════════════════════════════════════════════════════
   L9: Meta-Complexity
   ══════════════════════════════════════════════════════════════ */

double meta_complexity_estimate(const KString* x);
double meta_meta_complexity_estimate(const KString* x);
void complexity_hierarchy_analysis(const KString* x);

/* ══════════════════════════════════════════════════════════════
   L9: Logical Depth (Bennett 1988)
   ══════════════════════════════════════════════════════════════ */

double logical_depth_estimate(const KString* x, int max_steps);
double meaningful_information(const KString* x, int c_threshold);

#endif /* APPLICATIONS_H */
