/*
 * incompressibility_proofs.h �� Canonical Proof Templates
 *
 * This header defines the classic proof templates that use the
 * incompressibility method. Each function corresponds to one
 * standard theorem proved via incompressibility in Li & Vit��nyi ��6.
 *
 * L4: Fundamental Theorems �� classic results proved via the method.
 * L6: Canonical Problems �� standard problems tackled with the method.
 * L7: Applications �� applying the method to various domains.
 *
 * Reference: Li & Vit��nyi ��6.1-6.5
 */

#ifndef INCOMPRESSIBILITY_PROOFS_H
#define INCOMPRESSIBILITY_PROOFS_H

#include "incompressibility.h"

/* �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T
   L4: Fundamental Theorems via Incompressibility
   �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T */

/*
 * THEOREM (Sorting Lower Bound):
 *   Any comparison-based sorting algorithm makes Omega(n log n)
 *   comparisons in the worst case.
 *
 * Proof via incompressibility (Li-Vitanyi ��6.2.1):
 *   Fix an incompressible permutation pi of n elements. Simulate
 *   the sorting algorithm; each comparison reveals at most 1 bit
 *   of information about pi. To determine pi fully, need log(n!) bits,
 *   hence Omega(n log n) comparisons.
 */

size_t inc_sorting_comparison_lower_bound(size_t n);
IncompressibilityProof* inc_proof_sorting_lower_bound(size_t n);
int inc_verify_sorting_bound(size_t n, size_t actual_comparisons);

/*
 * THEOREM (Heapsort Optimality):
 *   Heapsort uses O(n log n) comparisons, matching the lower bound.
 *   Hence heapsort is asymptotically optimal among comparison sorts.
 */

IncompressibilityProof* inc_proof_heapsort_optimality(size_t n);

/*
 * THEOREM (Finding Maximum Lower Bound):
 *   Finding the maximum of n elements requires n-1 comparisons.
 */

size_t inc_maximum_lower_bound(size_t n);
IncompressibilityProof* inc_proof_maximum_lower_bound(size_t n);

/*
 * THEOREM (Median Finding):
 *   Finding the median requires at least 3n/2 - O(1) comparisons.
 */

size_t inc_median_lower_bound(size_t n);
IncompressibilityProof* inc_proof_median_lower_bound(size_t n);

/* �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T
   L4: Graph Theory via Incompressibility
   �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T */

/*
 * THEOREM (Graph Non-Isomorphism Certificate):
 *   Graph non-isomorphism (GNI) has polynomial-size certificates.
 *   Specifically, GNI is in co-NP.
 */

IncompressibilityProof* inc_proof_gni_certificate(size_t n_vertices);
size_t inc_gni_certificate_size_bound(size_t n_vertices);

/*
 * THEOREM (Graph Connectivity):
 *   Any algorithm testing graph connectivity must examine Omega(n^2)
 *   edges in the worst case for dense graphs.
 */

size_t inc_connectivity_edge_lower_bound(size_t n_vertices);
IncompressibilityProof* inc_proof_connectivity_lower_bound(size_t n);

/*
 * THEOREM (Number of Connected Components):
 *   Determining the exact number of connected components requires
 *   examining essentially all edges.
 */

size_t inc_component_counting_lower_bound(size_t n_vertices, size_t n_edges);
IncompressibilityProof* inc_proof_component_counting_bound(size_t n, size_t m);

/* �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T
   L4: String Algorithms via Incompressibility
   �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T */

/*
 * THEOREM (String Matching Lower Bound):
 *   Any algorithm for exact string matching must examine at least
 *   n - m + 1 characters in the worst case, where n = |text|, m = |pattern|.
 */

size_t inc_string_matching_lower_bound(size_t text_len, size_t pattern_len);
IncompressibilityProof* inc_proof_string_matching_bound(size_t n, size_t m);

/*
 * THEOREM (Longest Common Subsequence):
 *   Computing the LCS of two strings requires Omega(n^2) time in the
 *   worst case under the comparison model.
 */

size_t inc_lcs_lower_bound(size_t n);
IncompressibilityProof* inc_proof_lcs_bound(size_t n);

/*
 * THEOREM (Palindrome Recognition):
 *   Recognizing whether a string is a palindrome requires n/2
 *   character examinations in the worst case.
 */

size_t inc_palindrome_lower_bound(size_t n);
IncompressibilityProof* inc_proof_palindrome_bound(size_t n);

/* �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T
   L7: Formal Language Applications
   �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T */

/*
 * THEOREM (Pumping Lemma via Incompressibility):
 *   For any regular language L, there exists p such that any
 *   incompressible string w in L with |w| >= p can be pumped.
 */

int inc_pumping_lemma_regular(size_t pump_length, const char* language_test);
IncompressibilityProof* inc_proof_regular_pumping(void);

/*
 * THEOREM (Context-Free Pumping via Incompressibility):
 *   Similar strengthening for context-free languages.
 */

int inc_pumping_lemma_cfl(size_t pump_length, const char* language_test);
IncompressibilityProof* inc_proof_cfl_pumping(void);

/* �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T
   L7: Average-Case Complexity Applications
   �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T */

/*
 * THEOREM (Average-Case Sorting):
 *   The average number of comparisons for comparison-based sorting
 *   is also Omega(n log n), not just the worst case.
 */

size_t inc_average_case_sorting_bound(size_t n);
IncompressibilityProof* inc_proof_average_sorting_bound(size_t n);

/*
 * THEOREM (Average-Case via Incompressibility):
 *   For many problems, worst-case and average-case complexity
 *   coincide when inputs are Kolmogorov-random.
 */

int inc_average_equals_worst_case(size_t n, ProofStrategy strategy);

/*
 * THEOREM (Heapsort Average Case):
 *   The average-case number of comparisons for heapsort is ~2n log n.
 */

double inc_heapsort_average_comparisons(size_t n);

/* �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T
   L7: Communication Complexity
   �T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T�T */

/*
 * THEOREM (Equality Testing Communication Lower Bound):
 *   Deterministic communication complexity of EQ_n is n+1 bits.
 */

size_t inc_eq_communication_lower_bound(size_t n);
IncompressibilityProof* inc_proof_eq_communication(size_t n);

/*
 * THEOREM (Set Disjointness):
 *   DISJ_n requires Omega(n) bits of communication.
 */

size_t inc_disjointness_lower_bound(size_t n);
IncompressibilityProof* inc_proof_disjointness_communication(size_t n);

/* ══════════════════════════════════════════════════════════════
   L5: Meta-Analysis
   ══════════════════════════════════════════════════════════════ */

int inc_apply_method(const unsigned char* input, size_t input_len,
                     int c, int (*property_test)(const unsigned char*, size_t));

/* ══════════════════════════════════════════════════════════════
   L6: Canonical Problem Implementations (for verification)
   ══════════════════════════════════════════════════════════════ */

size_t heapsort_count_comparisons(int* arr, size_t n);
int is_palindrome_with_count(const unsigned char* s, size_t n, size_t* exams);
size_t lcs_length(const unsigned char* x, size_t xlen,
                  const unsigned char* y, size_t ylen);
size_t kmp_search(const unsigned char* text, size_t n,
                  const unsigned char* pat, size_t m, size_t* comparisons);

/* ══════════════════════════════════════════════════════════════
   L6: Incompressibility Analysis
   ══════════════════════════════════════════════════════════════ */

typedef struct {
    size_t len;
    double estimated_ratio;
    int incomp_C[5];
    double compress_fraction;
} IncompressibilityAnalysis;

IncompressibilityAnalysis* inc_analyze_string(const unsigned char* data, size_t len);
void inc_analyze_report(const IncompressibilityAnalysis* a);
void inc_analyze_free(IncompressibilityAnalysis* a);

/* ══════════════════════════════════════════════════════════════
   L6: Demonstration
   ══════════════════════════════════════════════════════════════ */

void inc_run_all_proofs_demo(size_t n);

/* ══════════════════════════════════════════════════════════════
   L8: Advanced Topics -- Compression-based K(x) estimation
   ══════════════════════════════════════════════════════════════ */

/* LZ77 compression token */
typedef struct {
    size_t offset;
    size_t length;
    unsigned char next_char;
} LZ77Token;

size_t lz77_compress(const unsigned char* data, size_t len, LZ77Token** tokens_out);
double ncd_approximate(const unsigned char* x, size_t xlen,
                       const unsigned char* y, size_t ylen);
double inc_information_distance_estimate(const unsigned char* x, size_t xlen,
                                          const unsigned char* y, size_t ylen);
double inc_count_compressible_fraction(size_t n, int c);

#endif /* INCOMPRESSIBILITY_PROOFS_H */
