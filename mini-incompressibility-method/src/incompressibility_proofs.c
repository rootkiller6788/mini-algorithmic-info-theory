/*
 * incompressibility_proofs.c -- Canonical Proof Templates Implementation
 *
 * This file implements the classic proof templates that use the
 * incompressibility method. Each function corresponds to one
 * standard theorem proved via incompressibility in Li & Vitanyi §6.
 *
 * L4: Fundamental Theorems -- Sorting, Selection, Graph, String bounds
 * L6: Canonical Problems -- GNI, LCS, Palindrome, String matching
 * L7: Applications -- Pumping lemmas, average-case, communication complexity
 * L8: Advanced Topics -- LZ77 compression, Normalized Compression Distance
 *
 * The incompressibility method works by contradiction:
 *   1. Assume a property P fails for some input
 *   2. Use the failure to construct a short description (encoding)
 *   3. This short description contradicts Kolmogorov incompressibility
 *   4. Therefore P must hold for all c-incompressible inputs
 *
 * Reference: Li & Vitányi "An Introduction to Kolmogorov Complexity
 *   and its Applications" (4th ed, 2019), Chapter 6.
 * Reference: Hromkovic "Algorithmics for Hard Problems" §8.3
 * Reference: Sipser §6.4, Arora-Barak §6.8
 * Courses: MIT 6.841, Stanford CS254, Berkeley CS278, CMU 15-855
 *           Oxford Advanced Complexity, Cambridge Part III
 *           ETH 263-4650, Princeton COS 522, Caltech CS 154
 */

#include "../include/incompressibility.h"
#include "../include/incompressibility_proofs.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

/* ══════════════════════════════════════════════════════════════
   Utility: ilog2 (integer floor log2)
   ══════════════════════════════════════════════════════════════ */
static size_t ilog2(size_t n) {
    size_t r = 0;
    while (n > 1) { n >>= 1; r++; }
    return r;
}

/* ══════════════════════════════════════════════════════════════
   Utility: Factorial approximation (Stirling)
   ══════════════════════════════════════════════════════════════ */

/*
 * Stirling's approximation: log2(n!) ~ n*log2(n) - n*log2(e) + O(log n)
 * Used in sorting lower bounds where log2(n!) appears.
 */
static double stirling_log2_factorial(size_t n) {
    if (n <= 1) return 0.0;
    double log2n = log2((double)n);
    double log2e = 1.4426950408889634; /* log2(e) */
    return (double)n * log2n - (double)n * log2e
         + 0.5 * log2n + 0.9189385332046727;
}

/* Exact log2 of n! via summation for small n, Stirling for large. */
static double exact_log2_factorial(size_t n) {
    if (n <= 1) return 0.0;
    if (n <= 100) {
        double sum = 0.0;
        for (size_t i = 2; i <= n; i++) sum += log2((double)i);
        return sum;
    }
    return stirling_log2_factorial(n);
}

/* ══════════════════════════════════════════════════════════════
   Utility: Random incompressible string generator
   ══════════════════════════════════════════════════════════════ */

/*
 * Generate a pseudo-random byte string of given length.
 * Used as a proxy for a c-incompressible string in demonstrations.
 *
 * Knowledge point: Most strings are incompressible (K(x) >= |x| - O(1)).
 * A counting argument shows: at most 2^{n-c} out of 2^n strings have
 * K(x) < n - c. So the fraction of compressible strings is <= 2^{-c}.
 */
static void generate_random_bytes(unsigned char* buf, size_t len) {
    static int seeded = 0;
    if (!seeded) { srand((unsigned)time(NULL)); seeded = 1; }
    for (size_t i = 0; i < len; i++)
        buf[i] = (unsigned char)(rand() & 0xFF);
}

/* ══════════════════════════════════════════════════════════════
   L3: Graph representation for incompressibility proofs
   ══════════════════════════════════════════════════════════════ */

/*
 * Simple adjacency matrix representation for labeled graphs.
 * Used in incompressibility proofs about graph properties.
 *
 * Knowledge point: A labeled graph on n vertices can be represented
 * by n + n(n-1)/2 bits (vertex labels + adjacency matrix bits).
 * The number of labeled graphs on n vertices is 2^{C(n,2)}.
 */

typedef struct {
    size_t n_vertices;
    unsigned char* adj;   /* adjacency matrix, packed bits */
} SimpleGraph;

static SimpleGraph* graph_create(size_t n) {
    SimpleGraph* g = (SimpleGraph*)malloc(sizeof(SimpleGraph));
    assert(g != NULL);
    g->n_vertices = n;
    size_t byte_sz = (n * n + 7) / 8;
    g->adj = (unsigned char*)calloc(byte_sz, 1);
    assert(g->adj != NULL);
    return g;
}

static void graph_free(SimpleGraph* g) {
    if (g) { free(g->adj); free(g); }
}

static int graph_get_edge(const SimpleGraph* g, size_t u, size_t v) {
    if (u >= g->n_vertices || v >= g->n_vertices) return 0;
    size_t idx = u * g->n_vertices + v;
    return (g->adj[idx / 8] >> (int)(7 - (idx % 8))) & 1;
}

static void graph_set_edge(SimpleGraph* g, size_t u, size_t v, int val) {
    if (u >= g->n_vertices || v >= g->n_vertices) return;
    size_t idx = u * g->n_vertices + v;
    if (val)
        g->adj[idx / 8] |= (unsigned char)(1 << (int)(7 - (idx % 8)));
    else
        g->adj[idx / 8] &= (unsigned char)(~(1 << (int)(7 - (idx % 8))));
}

static void graph_random_dense(SimpleGraph* g, double p) {
    for (size_t i = 0; i < g->n_vertices; i++)
        for (size_t j = i + 1; j < g->n_vertices; j++)
            if ((double)rand() / (double)RAND_MAX < p) {
                graph_set_edge(g, i, j, 1);
                graph_set_edge(g, j, i, 1);
            }
}

/* Count edges in the graph */
static size_t graph_edge_count(const SimpleGraph* g) {
    size_t cnt = 0;
    for (size_t i = 0; i < g->n_vertices; i++)
        for (size_t j = i + 1; j < g->n_vertices; j++)
            if (graph_get_edge(g, i, j)) cnt++;
    return cnt;
}

/* ══════════════════════════════════════════════════════════════
   L3: Permutation representation
   ══════════════════════════════════════════════════════════════ */

/*
 * A permutation of [0..n-1]. Used in sorting lower bound proofs.
 * The number of permutations is n!, requiring log2(n!) bits to encode.
 */
typedef struct {
    size_t n;
    size_t* pi;       /* pi[i] = element at position i */
} Permutation;

static Permutation* perm_create(size_t n) {
    Permutation* p = (Permutation*)malloc(sizeof(Permutation));
    assert(p != NULL);
    p->n = n;
    p->pi = (size_t*)malloc(n * sizeof(size_t));
    assert(p->pi != NULL);
    for (size_t i = 0; i < n; i++) p->pi[i] = i;
    return p;
}

static void perm_free(Permutation* p) {
    if (p) { free(p->pi); free(p); }
}

/* Fisher-Yates shuffle to produce a random permutation */
static void perm_shuffle(Permutation* p) {
    for (size_t i = p->n - 1; i > 0; i--) {
        size_t j = (size_t)rand() % (i + 1);
        size_t tmp = p->pi[i];
        p->pi[i] = p->pi[j];
        p->pi[j] = tmp;
    }
}

/*
 * Lehmer code: encode a permutation into an integer in [0, n!-1].
 * The Lehmer code provides an optimal encoding: ceil(log2(n!)) bits.
 *
 * Knowledge point: The factorial number system provides a bijection
 * between permutations and integers 0..n!-1. This encoding uses
 * exactly ceil(log2(n!)) bits -- optimal for permutation encoding.
 */
static size_t perm_to_lehmer(const Permutation* p) {
    /* Compute ranking via factorial number system */
    size_t rank = 0;
    size_t* remaining = (size_t*)malloc(p->n * sizeof(size_t));
    assert(remaining != NULL);
    for (size_t i = 0; i < p->n; i++) remaining[i] = i;

    size_t fact = 1;
    for (size_t i = 1; i < p->n; i++) fact *= i;

    for (size_t pos = 0; pos < p->n - 1; pos++) {
        size_t elem = p->pi[pos];
        /* Find position of elem in remaining */
        size_t idx = 0;
        while (remaining[idx] != elem) idx++;
        rank += idx * fact;
        /* Remove elem from remaining */
        for (size_t k = idx; k < p->n - pos - 1; k++)
            remaining[k] = remaining[k + 1];
        if (p->n - pos - 1 > 0) fact /= (p->n - pos - 1);
    }
    free(remaining);
    return rank;
}

/* ══════════════════════════════════════════════════════════════
   L4: Sorting Lower Bound via Incompressibility
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Comparison Sorting Lower Bound):
 *   Any deterministic comparison-based sorting algorithm makes
 *   at least ceil(log2(n!)) comparisons in the worst case.
 *
 * Proof via Incompressibility (Li-Vitányi §6.2.1):
 *   Fix a c-incompressible permutation pi of n elements.
 *   Run the sorting algorithm on input ordered by pi.
 *   Each comparison of a[i] < a[j] reveals at most 1 bit about pi.
 *   After T comparisons, algorithm has at most T bits about pi.
 *   But K(pi) >= log2(n!) - c (c-incompressibility).
 *   Therefore T >= log2(n!) - c = Omega(n log n).
 *
 * Reference: Li-Vitányi (2019) §6.2.1, Theorem 6.2.1
 */

size_t inc_sorting_comparison_lower_bound(size_t n) {
    /*
     * Knowledge point: log2(n!) = n*log2(n) - n*log2(e) + Theta(log n)
     * by Stirling's approximation. This gives the well-known
     * Omega(n log n) lower bound for comparison-based sorting.
     */
    if (n <= 1) return 0;
    double lnfac = exact_log2_factorial(n);
    return (size_t)ceil(lnfac);
}

IncompressibilityProof* inc_proof_sorting_lower_bound(size_t n) {
    /*
     * Construct a complete incompressibility proof for the sorting
     * lower bound with n elements.
     *
     * Knowledge point: The incompressibility method provides an
     * alternative proof to the decision tree model. Its advantage
     * is that it also yields average-case bounds directly.
     */
    IncompressibilityProof* proof = incproof_create(
        "Comparison Sorting Lower Bound",
        "Comparison-based sorting requires Omega(n log n) comparisons");
    char d[256];
    double lnfac = exact_log2_factorial(n);

    snprintf(d, sizeof(d), "Assume a comparison sort uses T(n) comparisons to sort n=%zu elements", n);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, (size_t)n * ilog2(n), 0.0);

    snprintf(d, sizeof(d), "Fix a c-incompressible permutation pi of [1..%zu]. Each comparison reveals at most 1 bit about pi.", n);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (size_t)n * ilog2(n), 0.0);

    snprintf(d, sizeof(d), "After T(n) comparisons, K(pi|algo) <= T(n)+O(1). But K(pi) >= log(%zu!)-c = %.1f bits", n, lnfac);
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)ceil(lnfac), lnfac);

    snprintf(d, sizeof(d), "T(n) >= log(n!)-c-O(1) = n*log(n)-n*log(e)-O(1). Contradiction if T(n)=o(n log n).");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore T(n) >= log(n!) = Omega(n log n) comparisons required. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

int inc_verify_sorting_bound(size_t n, size_t actual_comparisons) {
    /*
     * Verify that actual_comparisons meets the lower bound.
     * Knowledge point: If actual_comparisons < lower_bound, the
     * algorithm either uses non-comparison operations or is buggy.
     */
    size_t lb = inc_sorting_comparison_lower_bound(n);
    return (actual_comparisons >= lb) ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════
   L4: Heapsort Optimality via Incompressibility
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Heapsort Optimality):
 *   Heapsort makes Theta(n log n) comparisons. Since any comparison-based
 *   sort needs Omega(n log n) comparisons, heapsort is asymptotically optimal.
 *
 * Knowledge point: Heapsort achieves the theoretical lower bound.
 * In the worst case and average case, Heapsort uses approximately
 * 2n log2(n) comparisons. Combined with the Omega(n log n) lower bound,
 * this establishes optimality up to a constant factor of 2.
 *
 * Reference: Williams (1964), Floyd (1964)
 */

IncompressibilityProof* inc_proof_heapsort_optimality(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Heapsort Asymptotic Optimality",
        "Heapsort achieves Theta(n log n), matching the comparison lower bound");
    char d[256];
    double lnfac = exact_log2_factorial(n);

    snprintf(d, sizeof(d), "Heapsort on %zu elements: build_heap = O(n) + extract phases = O(n log n)", n);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, (size_t)(2.0 * n * log2((double)n)), 0.0);

    double heap_cmp = 2.0 * (double)n * log2((double)n);
    snprintf(d, sizeof(d), "Heapsort uses at most 2n*log2(n) + O(n) comparisons ~ %.1f. Lower bound: log(n!) ~ %.1f", heap_cmp, lnfac);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (size_t)ceil(heap_cmp), 0.0);

    snprintf(d, sizeof(d), "Ratio: (2n log n)/(n log n) -> 2. Both are Theta(n log n), differing only by constant factor.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)(heap_cmp - lnfac), heap_cmp - lnfac);

    snprintf(d, sizeof(d), "If a sorting algorithm used o(n log n) comparisons, contradicts lower bound. Heapsort does not; hence optimal.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Heapsort is asymptotically optimal: Theta(n log n) matches the Omega(n log n) lower bound. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: Finding Maximum Lower Bound
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Maximum Finding Lower Bound):
 *   Finding the maximum of n distinct elements requires exactly
 *   n - 1 comparisons in the worst case.
 *
 * Incompressibility proof:
 *   Assume an algorithm uses fewer than n-1 comparisons.
 *   Then the comparison graph on n elements is a forest with > 1 tree.
 *   Elements in different trees cannot be compared; there exists
 *   at least one element whose relative order is unknown.
 *   Without knowing the order, cannot determine the maximum.
 *
 * Reference: Li-Vitányi §6.2.2
 */

size_t inc_maximum_lower_bound(size_t n) {
    /*
     * Knowledge point: The exact lower bound for finding the maximum
     * is n-1 comparisons. Unlike sorting, this bound is EXACT, not
     * asymptotic. This can be proved both by adversary argument and
     * by incompressibility method.
     */
    if (n <= 1) return 0;
    return n - 1;
}

IncompressibilityProof* inc_proof_maximum_lower_bound(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Maximum Finding Lower Bound",
        "Finding the maximum requires exactly n-1 comparisons");
    char d[256];

    snprintf(d, sizeof(d), "Assume algorithm finds max of %zu distinct elements with T < %zu comparisons", n, n-1);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n * 8, 0.0);

    snprintf(d, sizeof(d), "Comparison graph on %zu elements has T edges. With T < %zu edges, graph is a forest with >= 2 components.", n, n-1);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (n - 1) * 8, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible ordering. With < n-1 comparisons, there exist elements i, j never compared directly or indirectly via transitivity.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, n * ilog2(n), 0.0);

    snprintf(d, sizeof(d), "Cannot distinguish whether i > j or j > i without comparing. Thus cannot determine max. Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore any algorithm must perform at least n-1 comparisons to find the maximum. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: Median Finding Lower Bound
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Median Finding Lower Bound):
 *   Finding the median of n elements requires at least 3n/2 - O(1)
 *   comparisons in the worst case.
 *
 * Knowledge point: Median is harder than max (n-1 comparisons) because
 * the algorithm must resolve relations on BOTH sides of the median.
 * The constant factor 3/2 > 1.0 for max reflects the extra work needed.
 *
 * Reference: Bent & John (1985), Li-Vitányi §6.2.3
 */

size_t inc_median_lower_bound(size_t n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    return (size_t)((3 * n) / 2 - 2);
}

IncompressibilityProof* inc_proof_median_lower_bound(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Median Finding Lower Bound",
        "Finding the median requires at least 3n/2 - O(1) comparisons");
    char d[256];
    size_t lb = inc_median_lower_bound(n);

    snprintf(d, sizeof(d), "Assume algorithm finds median of %zu elements with T < %zu comparisons", n, lb);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n * 8, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible ordering. Each comparison resolves at most 1 domination relation.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, lb * 8, 0.0);

    snprintf(d, sizeof(d), "Median must dominate (n-1)/2 and be dominated by (n-1)/2. Need at least 3n/2 - 2 comparisons.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)(3*n/2), 0.0);

    snprintf(d, sizeof(d), "With fewer comparisons, cannot distinguish the true median from a near-median element. Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore at least 3n/2 - O(1) comparisons are required to find the median. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: Graph Non-Isomorphism Certificate (GNI in co-NP)
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (GNI has Polynomial Certificates):
 *   Graph Non-Isomorphism = {(G,H) : G not iso H} is in co-NP.
 *
 * Incompressibility proof (Li-Vitányi §6.3):
 *   For non-isomorphic G, H on n vertices, take a c-incompressible
 *   permutation pi. Certify G not-iso H by: "pi is the lexicographically
 *   first permutation s.t. pi(G) != H". Since G not-iso H, such pi exists.
 *   pi can be described as: description(G) + description(H) + O(log(n!))
 *   = O(n^2) + O(n log n) = O(n^2) bits. Polynomial certificate!
 *
 * Reference: Li-Vitányi §6.3, Goldreich (2008) §2.2
 */

size_t inc_gni_certificate_size_bound(size_t n_vertices) {
    if (n_vertices <= 1) return 0;
    return n_vertices * n_vertices;  /* adjacency matrix upper bound */
}

IncompressibilityProof* inc_proof_gni_certificate(size_t n_vertices) {
    IncompressibilityProof* proof = incproof_create(
        "Graph Non-Isomorphism Certificate",
        "GNI has polynomial-size certificates (co-NP membership)");
    char d[256];

    snprintf(d, sizeof(d), "Assume G,H non-isomorphic on %zu vertices require super-polynomial certificates", n_vertices);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n_vertices*n_vertices, 0.0);

    snprintf(d, sizeof(d), "Incompressible permutation pi: lexicographically first where pi(G) != H. Cert = describe(G,H) + pi-index.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, n_vertices*n_vertices, 0.0);

    snprintf(d, sizeof(d), "G,H adjacency each O(n^2) bits. pi via Lehmer code O(n log n) bits. Total = O(n^2), polynomial.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, n_vertices*n_vertices, 0.0);

    snprintf(d, sizeof(d), "Certificate size O(n^2) contradicts super-polynomial assumption.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "GNI has polynomial-size certificates, hence GNI is in co-NP. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: Graph Connectivity Lower Bound
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Graph Connectivity Edge Lower Bound):
 *   Testing graph connectivity requires Omega(n^2) edge examinations.
 *
 * Knowledge point: Even simple graph properties can require reading
 * most of the input. If an algorithm examines o(n^2) edges, a large
 * unexamined region remains where flipping edges can toggle connectivity.
 *
 * Reference: Li-Vitányi §6.4.1
 */

size_t inc_connectivity_edge_lower_bound(size_t n_vertices) {
    if (n_vertices <= 1) return 0;
    return n_vertices * (n_vertices - 1) / 4;
}

IncompressibilityProof* inc_proof_connectivity_lower_bound(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Graph Connectivity Lower Bound",
        "Testing connectivity requires Omega(n^2) edge examinations");
    char d[256];
    size_t lb = n * (n - 1) / 4;

    snprintf(d, sizeof(d), "Assume algorithm tests connectivity of %zu-vertex graph examining T < %zu edges", n, lb);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n*n, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible graph. Algorithm examines T edges, n(n-1)/2 - T unexamined.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, n*n, 0.0);

    snprintf(d, sizeof(d), "Unexamined region has >= n(n-1)/4 edges. Enough to flip connectivity by toggling edges there.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, lb, 0.0);

    snprintf(d, sizeof(d), "Construct G' identical on examined edges but with opposite connectivity. Algorithm outputs same for both.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore connectivity testing requires Omega(n^2) edge examinations. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: Connected Component Counting Lower Bound
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Component Counting):
 *   Determining the exact number of connected components requires
 *   Omega(n^2) edge examinations for dense graphs.
 *
 * Knowledge point: Component counting and connectivity testing share
 * the same asymptotic bound (Omega(n^2) for dense graphs), but component
 * counting requires a larger constant factor.
 *
 * Reference: Li-Vitányi §6.4.2
 */

size_t inc_component_counting_lower_bound(size_t n_vertices, size_t n_edges) {
    if (n_vertices <= 1) return 0;
    if (n_edges < n_vertices) return n_edges;
    size_t max_edges = n_vertices * (n_vertices - 1) / 2;
    if (n_edges > max_edges) n_edges = max_edges;
    return n_edges / 2;
}

IncompressibilityProof* inc_proof_component_counting_bound(size_t n, size_t m) {
    IncompressibilityProof* proof = incproof_create(
        "Connected Component Counting Lower Bound",
        "Counting connected components requires examining most edges");
    char d[256];
    size_t lb = inc_component_counting_lower_bound(n, m);

    snprintf(d, sizeof(d), "Assume algorithm counts components of %zu-vertex %zu-edge graph with T < %zu exams", n, m, lb);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n*n, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible graph. Unexamined region has >= %zu edges.", m-lb);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (n*(n-1))/2, 0.0);

    snprintf(d, sizeof(d), "Adding or removing edges in the unexamined region changes component count undetectably.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, lb, 0.0);

    snprintf(d, sizeof(d), "Algorithm cannot determine correct component count. Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore component counting requires Omega(n^2) edge examinations. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: String Matching Lower Bound
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (String Matching Lower Bound):
 *   Any deterministic algorithm for exact string matching must
 *   examine at least n - m + 1 characters in the worst case.
 *
 * Knowledge point: Tight bound -- naive algorithm achieves n-m+1
 * comparisons per alignment. KMP/Boyer-Moore are O(n+m) worst case
 * but may skip positions in practice while still achieving the bound.
 *
 * Reference: Li-Vitányi §6.4.3, Knuth-Morris-Pratt (1977)
 */

size_t inc_string_matching_lower_bound(size_t text_len, size_t pattern_len) {
    if (pattern_len > text_len) return text_len;
    if (pattern_len == 0) return 0;
    return text_len - pattern_len + 1;
}

IncompressibilityProof* inc_proof_string_matching_bound(size_t n, size_t m) {
    IncompressibilityProof* proof = incproof_create(
        "String Matching Lower Bound",
        "Exact string matching requires at least n-m+1 character examinations");
    char d[256];
    size_t lb = inc_string_matching_lower_bound(n, m);

    snprintf(d, sizeof(d), "Assume algorithm finds pattern(len=%zu) in text(len=%zu) with T < %zu exams", m, n, lb);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n*8, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible text T and pattern P. n-m+1 possible start positions.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, m*8, 0.0);

    snprintf(d, sizeof(d), "If algorithm skips position i, T[i..i+m-1] could equal P without detection.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, lb, 0.0);

    snprintf(d, sizeof(d), "T < n-m+1 examinations means some starting position is unchecked. Incompressible text could have the pattern. Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore at least n - m + 1 character examinations are required. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: Longest Common Subsequence (LCS) Lower Bound
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (LCS Lower Bound):
 *   Computing LCS of two strings of length n requires Omega(n^2)
 *   comparisons under the comparison model.
 *
 * Knowledge point: Classic DP runs in O(n^2). Quadratic lower bound
 * shows this is optimal (up to polylog) under the comparison model.
 * Subquadratic would contradict SETH (Abboud et al. 2015).
 *
 * Reference: Li-Vitányi §6.4.4; Abboud et al. (2015)
 */

size_t inc_lcs_lower_bound(size_t n) {
    if (n <= 1) return 1;
    return n * n / 4; /* Omega(n^2) */
}

IncompressibilityProof* inc_proof_lcs_bound(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Longest Common Subsequence Lower Bound",
        "Computing LCS of two length-n strings requires Omega(n^2) comparisons");
    char d[256];
    size_t lb = inc_lcs_lower_bound(n);

    snprintf(d, sizeof(d), "Assume algorithm computes LCS of two length-%zu strings with T < %zu comparisons", n, lb);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n*n, 0.0);

    snprintf(d, sizeof(d), "Fix two c-incompressible strings X, Y. LCS depends on O(n^2) pairwise equalities X[i]==Y[j].");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, n*n, 0.0);

    snprintf(d, sizeof(d), "Each comparison = one equality check. T comparisons leave O(n^2)-T pairs unchecked.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, lb, 0.0);

    snprintf(d, sizeof(d), "Uncompared pair (i,j): flipping X[i]==Y[j] changes LCS length, but algorithm cannot detect. Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore LCS requires Omega(n^2) comparisons. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L4: Palindrome Recognition Lower Bound
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Palindrome Recognition):
 *   Any algorithm that checks whether a string is a palindrome
 *   must examine at least ceil(n/2) characters in the worst case.
 *
 * Knowledge point: Tight bound -- naive algorithm comparing s[i]
 * with s[n-1-i] examines exactly ceil(n/2) pairs and achieves
 * the lower bound. Half the input must be examined despite symmetry.
 *
 * Incompressibility proof: Take a c-incompressible palindrome.
 * If the algorithm examines fewer than n/2 characters, some symmetric
 * pair (i, n-1-i) is entirely unexamined. Modifying both characters
 * in that pair breaks the palindrome property without detection.
 *
 * Reference: Li-Vitányi §6.4.5
 */

size_t inc_palindrome_lower_bound(size_t n) {
    return (n + 1) / 2; /* ceil(n/2) */
}

IncompressibilityProof* inc_proof_palindrome_bound(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Palindrome Recognition Lower Bound",
        "Checking if a string is a palindrome requires ceil(n/2) examinations");
    char d[256];
    size_t lb = inc_palindrome_lower_bound(n);

    snprintf(d, sizeof(d), "Assume algorithm checks palindrome for length-%zu string with T < %zu exams", n, lb);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n*8, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible palindrome. There are ceil(n/2) symmetric pairs (i, n-1-i).");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, n*8, 0.0);

    snprintf(d, sizeof(d), "Each exam covers at most its pair. T < %zu => some symmetric pair entirely unexamined.", lb);
    incproof_add_step(proof, PROOF_STEP_BOUND, d, lb, 0.0);

    snprintf(d, sizeof(d), "Change both chars in unexamined pair: palindrome status changes, but no exam sees it. Algorithm fails.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore palindrome recognition requires ceil(n/2) examinations. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L7: Pumping Lemma for Regular Languages via Incompressibility
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Pumping Lemma via Incompressibility):
 *   For any regular language L, there exists p such that any
 *   incompressible string w in L with |w| >= p can be written as
 *   w = xyz where |xy| <= p, |y| >= 1, and xy^i z in L for all i >= 0.
 *
 * Incompressibility strengthening (Li-Vitányi §6.5):
 *   Take a c-incompressible string w in L with |w| >= p.
 *   By DFA pigeonhole: while processing w, some state repeats.
 *   The substring y can be pumped. If |y| > 0, then w can be
 *   described as: position of first repeat + length(y) + rest.
 *   This is shorter than |w|, contradicting c-incompressibility
 *   unless |y| is trivially bounded by O(log|w|) + c.
 *
 * Reference: Li-Vitányi §6.5.1
 */

int inc_pumping_lemma_regular(size_t pump_length, const char* language_test) {
    /*
     * Test whether a language descriptor satisfies the regular
     * pumping lemma with the given pumping length.
     *
     * The language_test string encodes simple regex-like patterns:
     *   "a* b*"      -> regular (star-closed)
     *   "a^n b^n"    -> non-regular (counting constraint)
     *
     * Returns: 1 if the language appears to satisfy the pumping lemma
     * (seems regular), 0 if it likely violates it (non-regular).
     */
    if (!language_test || pump_length == 0) return 0;

    int has_counting = 0;
    int has_star = 0;
    const char* p = language_test;
    while (*p) {
        /* '^n' or '^' followed by digit indicates counting constraint */
        if (*p == '^' && *(p+1) != '\0' && *(p+1) != ' ') has_counting = 1;
        if (*p == '*') has_star = 1;
        p++;
    }

    /* Languages with counting constraints (a^n b^n style) are non-regular */
    if (has_counting && !has_star) return 0;
    /* Languages with only star operations and concatenation are regular */
    return 1;
}

IncompressibilityProof* inc_proof_regular_pumping(void) {
    IncompressibilityProof* proof = incproof_create(
        "Pumping Lemma (Regular) via Incompressibility",
        "Regular languages cannot have counting constraints on incompressible strings");
    char d[256];

    snprintf(d, sizeof(d), "Let L be a regular language with DFA of p states. Assume c-incompressible w in L, |w| >= p.");
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, 0, 0.0);

    snprintf(d, sizeof(d), "DFA pigeonhole: while processing w, some state repeats. That segment y can be pumped.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, 0, 0.0);

    snprintf(d, sizeof(d), "If |y| > 0, w can be described as: position of repeat + length(y) + rest. Shorter than |w|.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, 0, 0.0);

    snprintf(d, sizeof(d), "Shorter description contradicts c-incompressibility unless |y| = O(log|w|) + c.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore regular languages cannot have counting constraints on incompressible strings. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L7: Pumping Lemma for Context-Free Languages via Incompressibility
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (CFL Pumping via Incompressibility):
 *   For any CFL L, there exists p such that any incompressible w in L
 *   with |w| >= p can be written as w = uvxyz where |vxy| <= p,
 *   |vy| >= 1, and uv^i x y^i z in L for all i >= 0.
 *
 * Knowledge point: CFLs are more expressive than regular languages
 * but still cannot count three variables independently. E.g.,
 * {a^n b^n c^n : n >= 1} is NOT context-free.
 *
 * The incompressibility method provides an alternative proof of the
 * CFL pumping lemma, showing that the two synchronized pumped
 * substrings (v, y) imply structural repetition exploitable for
 * compression, thus bounding the complexity of CFL strings.
 *
 * Reference: Li-Vitányi §6.5.2; Hopcroft-Ullman §6.1
 */

int inc_pumping_lemma_cfl(size_t pump_length, const char* language_test) {
    /*
     * Test whether a language descriptor satisfies the CFL pumping lemma.
     * CFL pumping involves TWO substrings (v,y) pumped in tandem.
     * Triple counting languages (a^n b^n c^n) fail even this.
     */
    if (!language_test || pump_length == 0) return 0;

    int depth = 0;
    int max_depth = 0;
    int has_triple = 0;
    const char* p = language_test;
    while (*p) {
        if (*p == '(' || *p == '{') depth++;
        if (*p == ')' || *p == '}') depth--;
        if (depth > max_depth) max_depth = depth;
        if (*p == 'c' && *(p+1) == '^') has_triple = 1;
        p++;
    }
    if (pump_length > 1000) pump_length = 1000;

    /* Triple counting languages are not context-free */
    if (has_triple) return 0;
    /* Balanced parentheses within pumping length suggest CFL */
    return (max_depth <= (int)pump_length / 2) ? 1 : 0;
}

IncompressibilityProof* inc_proof_cfl_pumping(void) {
    IncompressibilityProof* proof = incproof_create(
        "Pumping Lemma (CFL) via Incompressibility",
        "CFLs cannot have triple counting constraints on incompressible strings");
    char d[256];

    snprintf(d, sizeof(d), "Let L be a CFL with CFG of p variables. Assume c-incompressible w in L, |w| >= 2^p.");
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, 0, 0.0);

    snprintf(d, sizeof(d), "CFG parse tree pigeonhole: a path repeats a variable. Yields decomposition uvxyz with sync v,y.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, 0, 0.0);

    snprintf(d, sizeof(d), "If |vy| > 0, describe w via: tree position + |v| + |y| + context x. O(log|w|) + O(p) encoding.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, 0, 0.0);

    snprintf(d, sizeof(d), "Short description contradicts c-incompressibility. So v,y carry almost no information about w.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "CFLs cannot have triple counting detectable on incompressible strings. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L7: Average-Case Sorting Bound via Incompressibility
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Average-Case Sorting):
 *   The average number of comparisons for any comparison-based
 *   sorting algorithm on random permutations is Omega(n log n).
 *
 * Knowledge point: Follows from Shannon's source coding theorem.
 * The entropy of a random permutation is log(n!). Average comparisons
 * must be at least this entropy. Thus average = Theta(n log n),
 * which coincides with the worst-case bound asymptotically.
 *
 * Reference: Li-Vitányi §6.2.1; Cover & Thomas §5.11
 */

size_t inc_average_case_sorting_bound(size_t n) {
    /*
     * Average-case lower bound: log2(n!) - O(1).
     * Same asymptotic form as worst case but slightly smaller constant.
     */
    if (n <= 1) return 0;
    double lnfac = exact_log2_factorial(n);
    return (size_t)(lnfac * 0.99);
}

IncompressibilityProof* inc_proof_average_sorting_bound(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Average-Case Sorting Lower Bound",
        "Average comparisons for sorting is Omega(n log n)");
    char d[256];
    double lnfac = exact_log2_factorial(n);

    snprintf(d, sizeof(d), "Assume algorithm A sorts %zu elements using T_avg(n) = o(n log n) comparisons on average.", n);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, (size_t)lnfac, 0.0);

    snprintf(d, sizeof(d), "Entropy of random permutation = log(n!) = %.1f bits. By Shannon, expected comparisons >= entropy.", lnfac);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (size_t)lnfac, 0.0);

    snprintf(d, sizeof(d), "If T_avg(n) = o(n log n), average information revealed < entropy. Cannot distinguish all permutations on average.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)(lnfac * 0.9), lnfac * 0.9);

    snprintf(d, sizeof(d), "Algorithm must output correct sorted order. Information-theoretic contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore T_avg(n) = Omega(n log n). Average and worst case coincide asymptotically. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L7: Average-Case equals Worst-Case via Incompressibility
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Average = Worst-Case for Incompressible Inputs):
 *   For many natural algorithmic problems, the average-case
 *   complexity on Kolmogorov-random inputs equals the worst-case
 *   complexity.
 *
 * Knowledge point: This meta-theorem is a powerful consequence.
 * Since most inputs are c-incompressible (fraction >= 1 - 2^{-c}),
 * any upper bound on average-case complexity implies the SAME bound
 * on worst-case complexity for incompressible inputs (i.e., for
 * almost all inputs).
 *
 * This bridges average-case and worst-case analysis:
 *   - Good average case = good for almost all inputs
 *   - Adversarial input = from measure-zero set of compressible inputs
 *
 * Reference: Li-Vitányi §6.7; Antunes & Fortnow (2009)
 */

int inc_average_equals_worst_case(size_t n, ProofStrategy strategy) {
    /*
     * Returns the fraction (in thousandths) of inputs that are
     * c-incompressible for c = 10 (K(x) >= |x| - 10).
     *
     * For c = 10: fraction >= 1 - 2^{-10} = 1 - 1/1024 ~ 0.999.
     * So average-case ~ worst-case for >99.9% of inputs.
     */
    (void)n;
    switch (strategy) {
        case PROOF_COUNTING:
            return (int)((1.0 - 1.0/1024.0) * 1000.0); /* 999/1000 */
        case PROOF_ENCODING:
            return (int)((1.0 - 1.0/512.0) * 1000.0);  /* 998/1000 */
        case PROOF_AVERAGE_CASE:
            return (int)((1.0 - 1.0/256.0) * 1000.0);  /* 996/1000 */
        case PROOF_EXPECTATION:
            return (int)((1.0 - 1.0/256.0) * 1000.0);
        default:
            return 990; /* at least 99% */
    }
}

/* ══════════════════════════════════════════════════════════════
   L7: Heapsort Average-Case Analysis
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Heapsort Average Case):
 *   The average number of comparisons for heapsort on random
 *   permutations of n elements is approximately 2n log2(n).
 *
 * Knowledge point: Heapsort's average case (~2n log n) is very close
 * to its worst case (~2n log n) — unusual property of low variance.
 * Compare: quicksort average ~1.39n log n but worst O(n^2).
 * Heapsort: always ~2n log n, both average and worst.
 *
 * Schaffer-Sedgewick formula (1993):
 *   C_avg(n) = 2n log2(n) + n*(gamma - 3 + log2(pi)) + o(n)
 * where gamma ~ 0.5772 is Euler's constant.
 *
 * Reference: Schaffer & Sedgewick (1993), "The Analysis of Heapsort"
 */

double inc_heapsort_average_comparisons(size_t n) {
    /*
     * Knowledge point: Precise asymptotic formula for heapsort
     * average comparisons, derived via complex analysis of the
     * heap shape distribution and promotion probabilities.
     */
    if (n <= 1) return 0.0;
    double log2n = log2((double)n);
    double gamma = 0.57721566490153286060;
    double log2pi = log2(3.14159265358979323846);

    double result = 2.0 * (double)n * log2n
                  + (double)n * (gamma - 3.0 + log2pi);
    return result > 0 ? result : 0.0;
}

/* ══════════════════════════════════════════════════════════════
   L7: Communication Complexity — Equality (EQ_n)
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Equality Communication Lower Bound):
 *   The deterministic communication complexity of EQ_n(x, y)
 *   = "Is x = y?" (where x, y in {0,1}^n) is n+1 bits.
 *
 * Knowledge point: Foundational result (Yao 1979). Alice has x,
 * Bob has y; they determine if x = y by exchanging bits.
 * Trivial protocol: Alice sends x (n bits), Bob compares and
 * sends 1 bit back = n+1 bits. Lower bound matches: n+1.
 *
 * In the randomized model: EQ_n has O(log n) bits (with public
 * randomness), showing an EXPONENTIAL GAP between deterministic
 * and randomized communication complexity.
 *
 * Reference: Li-Vitányi §6.6.1; Kushilevitz-Nisan (1997) §1.1
 */

size_t inc_eq_communication_lower_bound(size_t n) {
    /*
     * Knowledge point: EQ_n requires n+1 bits deterministically.
     * This is tight -- the trivial protocol matches the bound.
     */
    return n + 1;
}

IncompressibilityProof* inc_proof_eq_communication(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Equality Communication Lower Bound",
        "Deterministic EQ_n requires n+1 bits of communication");
    char d[256];

    snprintf(d, sizeof(d), "Assume protocol P solves EQ_n using T < %zu bits for inputs of length %zu", n+1, n);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible string s in {0,1}^n. Consider inputs (s,s) and (s,s') for any s' != s.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, n, 0.0);

    snprintf(d, sizeof(d), "Transcript of P on (s,s) must differ from transcript on (s,s') for each s' != s. 2^n-1 such s'.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, n, 0.0);

    snprintf(d, sizeof(d), "With T < n+1 bits, only 2^T <= 2^n possible transcripts. Cannot distinguish all 2^n s values. Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore deterministic EQ_n requires at least n+1 bits. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L7: Communication Complexity — Set Disjointness (DISJ_n)
   ══════════════════════════════════════════════════════════════ */

/*
 * THEOREM (Set Disjointness Lower Bound):
 *   The deterministic communication complexity of DISJ_n(A, B)
 *   = "Is A intersect B = empty?" (where A, B subsets of [n]) is n+1 bits.
 *
 * Knowledge point: DISJ_n is n copies of AND_2 with communication.
 * Randomized complexity is also Theta(n) — NO exponential gap
 * between deterministic and randomized (unlike EQ_n). This makes
 * DISJ_n central to circuit lower bounds via the Karchmer-Wigderson
 * connection.
 *
 * Reference: Li-Vitányi §6.6.2; Kushilevitz-Nisan (1997) §3.3
 */

size_t inc_disjointness_lower_bound(size_t n) {
    /*
     * Knowledge point: DISJ_n requires Omega(n) communication even
     * with randomization (Razborov 1990, tight bound Theta(n) by
     * Kalyanasundaram-Schnitger 1992, Razborov 1992).
     */
    return n + 1;
}

IncompressibilityProof* inc_proof_disjointness_communication(size_t n) {
    IncompressibilityProof* proof = incproof_create(
        "Set Disjointness Communication Lower Bound",
        "Deterministic DISJ_n requires n+1 bits of communication");
    char d[256];

    snprintf(d, sizeof(d), "Assume protocol P solves DISJ_n (A,B) for subsets of [%zu] using T < %zu bits", n, n+1);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, n, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible subset A. For each singleton B={i}, output tells whether i in A.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, n, 0.0);

    snprintf(d, sizeof(d), "A can be recovered from protocol behavior on all B. Each transcript reveals at most T bits.");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, n, 0.0);

    snprintf(d, sizeof(d), "T < n+1 => fewer than 2^n possible transcripts but 2^n possible A. By pigeonhole, impossible.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore DISJ_n requires at least n+1 bits in the deterministic model. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

/* ══════════════════════════════════════════════════════════════
   L6: Heapsort Comparison Counting (Canonical Problem)
   ══════════════════════════════════════════════════════════════ */

/*
 * Run heapsort and count actual comparisons to validate theory.
 *
 * Knowledge point: Implementing heapsort and measuring comparisons
 * validates the theoretical analysis. For n elements:
 *   - Heapify (Floyd's linear-time method): ~n comparisons
 *   - Extract phase: ~2n log n comparisons
 *   - Total: ~2n log n + n comparisons
 *
 * The heapify_down function implements the sift-down (percolate)
 * operation on a max-heap represented as an array.
 *
 * Reference: Williams (1964), Floyd (1964)
 */

static void heapify_down(int* arr, size_t n, size_t i, size_t* cmp_count) {
    while (1) {
        size_t largest = i;
        size_t left = 2 * i + 1;
        size_t right = 2 * i + 2;

        if (left < n) {
            (*cmp_count)++;
            if (arr[left] > arr[largest]) largest = left;
        }
        if (right < n) {
            (*cmp_count)++;
            if (arr[right] > arr[largest]) largest = right;
        }

        if (largest == i) break;

        int tmp = arr[i];
        arr[i] = arr[largest];
        arr[largest] = tmp;
        i = largest;
    }
}

size_t heapsort_count_comparisons(int* arr, size_t n) {
    /*
     * Run heapsort and return the actual number of comparisons performed.
     * This function modifies the input array (sorts it in ascending order).
     *
     * Returns: total number of element comparisons during the sort.
     */
    size_t cmp_count = 0;

    /* Phase 1: Build max-heap (Floyd's method) -- O(n) comparisons */
    for (size_t i = n / 2; i > 0; i--)
        heapify_down(arr, n, i - 1, &cmp_count);

    /* Phase 2: Extract elements -- O(n log n) comparisons */
    for (size_t i = n - 1; i > 0; i--) {
        /* Swap root (max) with last unsorted element */
        int tmp = arr[0];
        arr[0] = arr[i];
        arr[i] = tmp;
        /* Restore heap property on reduced heap */
        heapify_down(arr, i, 0, &cmp_count);
    }
    return cmp_count;
}

/* ══════════════════════════════════════════════════════════════
   L6: Graph Non-Isomorphism Certificate Construction
   ══════════════════════════════════════════════════════════════ */

/*
 * Construct a concrete GNI certificate: given two non-isomorphic
 * graphs as adjacency matrices, produce a witness permutation pi
 * such that pi(G) != H.
 *
 * Knowledge point: A GNI certificate is a permutation pi proving
 * two graphs are NOT isomorphic. The verifier computes pi(G)
 * (permute rows and columns of G by pi) and checks pi(G) != H.
 * This verification is polynomial time (O(n^2 + n^3) for matrix
 * multiplication, or O(n^2) for adjacency matrix comparison).
 *
 * The certificate can be encoded in O(n^2) bits (the permutation
 * itself or the adjacency matrix of pi(G)).
 */

static int graph_apply_permutation_equals(const SimpleGraph* g,
                                           const SimpleGraph* h,
                                           const size_t* pi, size_t n) {
    /*
     * Check if pi(G) == H by verifying edge-wise:
     *   For all i,j: G[pi(i)][pi(j)] == H[i][j]
     * Returns 1 if equal (isomorphic under pi), 0 otherwise.
     */
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            int g_val = graph_get_edge(g, pi[i], pi[j]);
            int h_val = graph_get_edge(h, i, j);
            if (g_val != h_val) return 0;
        }
    }
    return 1;
}

size_t gni_find_certificate(const SimpleGraph* g, const SimpleGraph* h,
                             size_t** cert_pi_out) {
    /*
     * Find a distinguishing permutation (certificate) for G not-iso H.
     *
     * Uses Heap's algorithm to enumerate all n! permutations.
     * Finds the first pi where pi(G) != H.
     *
     * Returns: n (number of vertices) if certificate found,
     *          0 if graphs are isomorphic (no cert needed),
     *          0 with *cert_pi_out = NULL if trivial (different sizes).
     */
    size_t n = g->n_vertices;
    if (n != h->n_vertices) {
        *cert_pi_out = NULL;
        return 0;
    }

    /* Allocate permutation array */
    size_t* pi = (size_t*)malloc(n * sizeof(size_t));
    assert(pi != NULL);
    for (size_t i = 0; i < n; i++) pi[i] = i;

    /* Heap's algorithm state */
    size_t* c = (size_t*)calloc(n, sizeof(size_t));
    assert(c != NULL);

    /* Check identity permutation first */
    if (!graph_apply_permutation_equals(g, h, pi, n)) {
        *cert_pi_out = pi;
        free(c);
        return n;
    }

    /* Heap's algorithm iteration */
    size_t i = 0;
    int found = 0;
    while (i < n && !found) {
        if (c[i] < i) {
            if (i % 2 == 0) {
                /* Even: swap first with i-th */
                size_t tmp = pi[0]; pi[0] = pi[i]; pi[i] = tmp;
            } else {
                /* Odd: swap c[i]-th with i-th */
                size_t tmp = pi[c[i]]; pi[c[i]] = pi[i]; pi[i] = tmp;
            }
            if (!graph_apply_permutation_equals(g, h, pi, n)) {
                found = 1;
                break;
            }
            c[i]++;
            i = 0;
        } else {
            c[i] = 0;
            i++;
        }
    }

    if (!found) {
        free(pi);
        free(c);
        *cert_pi_out = NULL;
        return 0;
    }

    *cert_pi_out = pi;
    free(c);
    return n;
}

/* ══════════════════════════════════════════════════════════════
   L6: LCS Computation (Dynamic Programming) — for verification
   ══════════════════════════════════════════════════════════════ */

/*
 * Compute LCS length of two strings using DP.
 * Verifies the quadratic-time algorithm and its optimality.
 *
 * Knowledge point: Standard DP for LCS:
 *   dp[i][j] = max(dp[i-1][j], dp[i][j-1],
 *                   dp[i-1][j-1] + 1 if x[i]==y[j])
 * Runs in O(nm) time. Space can be optimized to O(min(n,m))
 * using only two rows (Hirschberg 1975).
 *
 * Reference: Wagner-Fischer (1974), Hirschberg (1975)
 */

size_t lcs_length(const unsigned char* x, size_t xlen,
                  const unsigned char* y, size_t ylen) {
    if (xlen == 0 || ylen == 0) return 0;

    /* Use two-row optimization for space efficiency */
    size_t* prev = (size_t*)calloc(ylen + 1, sizeof(size_t));
    size_t* curr = (size_t*)calloc(ylen + 1, sizeof(size_t));
    assert(prev && curr);

    for (size_t i = 1; i <= xlen; i++) {
        for (size_t j = 1; j <= ylen; j++) {
            if (x[i-1] == y[j-1])
                curr[j] = prev[j-1] + 1;
            else
                curr[j] = (prev[j] > curr[j-1]) ? prev[j] : curr[j-1];
        }
        /* Swap buffers for next row */
        size_t* tmp = prev; prev = curr; curr = tmp;
    }

    size_t result = prev[ylen];
    free(prev);
    free(curr);
    return result;
}

/* ══════════════════════════════════════════════════════════════
   L6: Palindrome Check (efficient, with examination count)
   ══════════════════════════════════════════════════════════════ */

/*
 * Check if a string is a palindrome and count character examinations.
 * Verifies the lower bound: any algorithm must examine ceil(n/2) chars.
 *
 * Knowledge point: The optimal palindrome-checking algorithm:
 *   for i = 0 to floor(n/2)-1: compare s[i] with s[n-1-i]
 * This examines exactly floor(n/2) pairs = ceil(n/2) character
 * examinations in the worst case (all pairs match). This matches
 * the lower bound proved via incompressibility.
 */
int is_palindrome_with_count(const unsigned char* s, size_t n, size_t* exams) {
    *exams = 0;
    if (n <= 1) return 1;
    for (size_t i = 0; i < n / 2; i++) {
        (*exams)++;
        if (s[i] != s[n - 1 - i]) return 0;
    }
    return 1;
}

/* ══════════════════════════════════════════════════════════════
   L6: String Matching — KMP Implementation
   ══════════════════════════════════════════════════════════════ */

/*
 * Knuth-Morris-Pratt string matching: O(n+m) worst-case comparisons.
 *
 * Knowledge point: KMP preprocesses the pattern to build a prefix
 * function (failure function) pi[q] = length of longest proper prefix
 * of pattern[0..q] that is also a suffix. This allows the search to
 * skip ahead by pi[q] positions when a mismatch occurs, avoiding the
 * naive O(nm) behavior.
 *
 * The prefix function computation itself is O(m) using a two-pointer
 * technique, demonstrating that clever preprocessing can achieve
 * asymptotically optimal search.
 *
 * Reference: Knuth-Morris-Pratt (1977)
 */

/* Compute KMP prefix function (pi array / failure function) */
static void kmp_prefix_function(const unsigned char* pat, size_t m, size_t* pi) {
    /*
     * pi[q] = length of longest proper prefix of pat[0..q]
     *         that is also a suffix of pat[0..q]
     * pi[0] = 0 (proper prefix of empty string is empty)
     */
    pi[0] = 0;
    size_t k = 0;
    for (size_t q = 1; q < m; q++) {
        while (k > 0 && pat[k] != pat[q])
            k = pi[k - 1];
        if (pat[k] == pat[q])
            k++;
        pi[q] = k;
    }
}

size_t kmp_search(const unsigned char* text, size_t n,
                  const unsigned char* pat, size_t m,
                  size_t* comparisons) {
    /*
     * KMP string matching. Finds first occurrence of pat in text.
     *
     * Returns: index of first match (0-based), or (size_t)-1 if not found.
     * *comparisons: total character comparisons performed.
     * Worst case: <= 2n + m comparisons = O(n+m).
     */
    *comparisons = 0;
    if (m == 0) return 0;
    if (n < m) return (size_t)-1;

    /* Build prefix function for pattern */
    size_t* pi = (size_t*)malloc(m * sizeof(size_t));
    assert(pi != NULL);
    kmp_prefix_function(pat, m, pi);

    size_t q = 0;  /* number of characters matched */
    for (size_t i = 0; i < n; i++) {
        while (q > 0 && pat[q] != text[i]) {
            (*comparisons)++;
            q = pi[q - 1];
        }
        (*comparisons)++;
        if (pat[q] == text[i]) {
            q++;
            if (q == m) {
                free(pi);
                return i - m + 1;  /* match found at position i-m+1 */
            }
        }
    }
    free(pi);
    return (size_t)-1;  /* no match */
}

/* ══════════════════════════════════════════════════════════════
   L5: Incompressibility Method Meta-Analysis
   ══════════════════════════════════════════════════════════════ */

/*
 * META-THEOREM (Incompressibility Method Template):
 *   To prove a lower bound for property P on inputs of size n:
 *
 *   1. ASSUME: Algorithm A violates the lower bound (uses too few resources).
 *   2. FIX: A c-incompressible input x (K(x) >= |x| - c).
 *   3. ENCODE: Use A's behavior on x to construct a short description of x.
 *   4. BOUND: Show |description| < |x| - c (contradicts incompressibility).
 *   5. CONCLUDE: Algorithm A cannot exist; lower bound holds.
 *
 * This template has been applied to prove dozens of lower bounds
 * across algorithms, graph theory, formal languages, combinatorics,
 * and communication complexity.
 *
 * Reference: Li-Vitányi §6.1, "The Method"
 */

int inc_apply_method(const unsigned char* input, size_t input_len,
                     int c, int (*property_test)(const unsigned char*, size_t)) {
    /*
     * Apply the incompressibility method template to an input.
     *
     * 1. Test if input is c-incompressible
     * 2. Test if property fails on input
     * 3. If property fails AND input is incompressible, then the
     *    encoding "this input is a counterexample" would be short,
     *    contradicting incompressibility. So such counterexamples
     *    are at most 2^{|x| - c - savings} -- a tiny fraction.
     *
     * Returns:
     *   -2: property holds for c-incompressible input (expected case)
     *   -1: invalid parameters (NULL input or test function)
     *    0: input is compressible (method inconclusive)
     *    1: counterexample exists but may be spurious
     */
    if (!property_test || !input) return -1;

    int is_incomp = incstr_is_c_incompressible(input, input_len, c);
    int prop_holds = property_test(input, input_len);

    if (is_incomp && !prop_holds) {
        /* Counterexample is c-incompressible yet violates property.
         * The encoding "this is a counterexample" would compress it.
         * Count of such counterexamples is at most 1 (if savings >= |x|-c). */
        return 1;
    }

    /* Property holds on incompressible input: expected behavior. */
    return (is_incomp && prop_holds) ? -2 : 0;
}

/* ══════════════════════════════════════════════════════════════
   L8: Kolmogorov Complexity Estimation via Compression (LZ77)
   ══════════════════════════════════════════════════════════════ */

/*
 * ADVANCED TOPIC: Estimating Kolmogorov complexity using practical
 * compressors. While K(x) is uncomputable (Chaitin's theorem), we
 * can approximate it via the compressed size from real compressors.
 *
 * Theorem (Vitányi 2006): The normalized compression distance (NCD)
 * using any normal compressor approximates the normalized information
 * distance based on Kolmogorov complexity:
 *   NCD(x,y) = (C(xy) - min(C(x),C(y))) / max(C(x),C(y))
 *            ~ (K(xy) - min(K(x),K(y))) / max(K(x),K(y))
 *
 * Applications: clustering, phylogeny reconstruction, anomaly
 * detection, malware classification — all without domain-specific
 * feature engineering.
 *
 * Reference: Cilibrasi & Vitányi (2005), "Clustering by Compression"
 * Reference: Li-Vitányi §8.4
 */

/* LZ77 token: (offset, length, next_char) triple -- type declared in header */

size_t lz77_compress(const unsigned char* data, size_t len, LZ77Token** tokens_out) {
    /*
     * LZ77 compression: sliding window approach.
     *
     * For each position, find the longest match in the previous
     * window (up to 32768 bytes back). Output a token:
     *   (offset, length, next_char)
     *
     * A match of length >= 3 is worth encoding as a reference.
     * Shorter matches are output as literals (offset=0, length=0).
     *
     * Returns: number of tokens.
     * *tokens_out: array of LZ77Token, caller must free().
     *
     * Knowledge point: Every practical compression algorithm provides
     * an upper bound on K(x). The better the compressor, the tighter
     * the bound. LZ77 is the basis for gzip, zip, PNG, and many others.
     */
    if (len == 0) { *tokens_out = NULL; return 0; }

    size_t max_tokens = len + 1;
    LZ77Token* tokens = (LZ77Token*)malloc(max_tokens * sizeof(LZ77Token));
    assert(tokens != NULL);

    size_t pos = 0, n_tokens = 0;
    size_t window = len < 32768 ? len : 32768;

    while (pos < len) {
        size_t best_off = 0, best_len = 0;
        size_t start = (pos > window) ? pos - window : 0;

        /* Search sliding window for longest match */
        for (size_t i = start; i < pos; i++) {
            size_t k = 0;
            while (pos + k < len && data[i + k] == data[pos + k]) k++;
            if (k > best_len) { best_len = k; best_off = pos - i; }
        }

        if (best_len >= 3) {
            /* Encode as reference to previous substring */
            tokens[n_tokens].offset = best_off;
            tokens[n_tokens].length = best_len;
            tokens[n_tokens].next_char = (pos + best_len < len)
                ? data[pos + best_len] : 0;
            pos += best_len + 1;
        } else {
            /* Encode as literal character */
            tokens[n_tokens].offset = 0;
            tokens[n_tokens].length = 0;
            tokens[n_tokens].next_char = data[pos];
            pos++;
        }
        n_tokens++;
    }

    *tokens_out = tokens;
    return n_tokens;
}

/* ══════════════════════════════════════════════════════════════
   L8: Normalized Compression Distance (NCD)
   ══════════════════════════════════════════════════════════════ */

/*
 * Compute approximate Normalized Compression Distance between two
 * byte strings using the LZ77-based compressor.
 *
 * NCD(x,y) in [0, 1+eps]:
 *   - NCD ~ 0: strings are very similar (likely share structure)
 *   - NCD ~ 1: strings are unrelated
 *   - NCD > 1: compressor imperfection (normally bounded)
 *
 * Knowledge point: NCD is a universal similarity metric derived
 * from Kolmogorov complexity theory. It is PARAMETER-FREE (no
 * training, no features, no thresholds) and DOMAIN-AGNOSTIC
 * (works on text, DNA, music, binaries, images).
 *
 * The key insight: if two objects share information, their
 * joint compression C(xy) will be smaller than C(x) + C(y)
 * because the compressor can exploit their shared structure.
 *
 * Reference: Cilibrasi, Vitányi, de Wolf (2004)
 *            "Algorithmic Clustering of Music"
 */
double ncd_approximate(const unsigned char* x, size_t xlen,
                       const unsigned char* y, size_t ylen) {
    /* Compress each individually */
    LZ77Token* tx = NULL;
    size_t cx = lz77_compress(x, xlen, &tx);
    free(tx);

    LZ77Token* ty = NULL;
    size_t cy = lz77_compress(y, ylen, &ty);
    free(ty);

    /* Compress concatenation xy */
    unsigned char* xy = (unsigned char*)malloc(xlen + ylen);
    assert(xy != NULL);
    memcpy(xy, x, xlen);
    memcpy(xy + xlen, y, ylen);
    LZ77Token* txy = NULL;
    size_t cxy = lz77_compress(xy, xlen + ylen, &txy);
    free(txy);
    free(xy);

    /* NCD formula */
    if (cx == 0 && cy == 0) return 0.0;  /* both empty */
    size_t min_c = (cx < cy) ? cx : cy;
    size_t max_c = (cx > cy) ? cx : cy;

    if (max_c == 0) return 0.0;
    return (double)(cxy - min_c) / (double)max_c;
}

/* ══════════════════════════════════════════════════════════════
   L8: Incompressibility Ratio Analysis
   ══════════════════════════════════════════════════════════════ */

/*
 * Analyze the incompressibility ratio of a byte string.
 *
 * Knowledge point: Most strings are incompressible. The fraction
 * of strings with K(x) < |x| - c is at most 2^{-c}. So:
 *   - >99.9% of strings are 10-incompressible
 *   - >99.999% are 20-incompressible
 *   - etc.
 *
 * This is why the incompressibility method works for "almost all"
 * inputs -- the set of compressible (non-random) inputs has
 * measure zero.
 *
 * IncompressibilityAnalysis type is declared in incompressibility_proofs.h
 */

IncompressibilityAnalysis* inc_analyze_string(const unsigned char* data, size_t len) {
    IncompressibilityAnalysis* a = (IncompressibilityAnalysis*)malloc(sizeof(IncompressibilityAnalysis));
    assert(a != NULL);
    a->len = len;

    if (len == 0) {
        a->estimated_ratio = 1.0;
        for (int i = 0; i < 5; i++) a->incomp_C[i] = 1;
        a->compress_fraction = 1.0;
        return a;
    }

    /* Estimate Kolmogorov complexity */
    size_t est_K = incstr_estimate_K(data, len);
    a->estimated_ratio = (double)est_K / (double)(len * 8);

    /* Test c-incompressibility for various c values */
    int c_vals[] = {1, 5, 10, 20, 50};
    for (int i = 0; i < 5; i++) {
        a->incomp_C[i] = incstr_is_c_incompressible(data, len, c_vals[i]);
    }

    /*
     * Fraction of strings at most this compressible:
     *  2^{est_K} / 2^{len*8} = 2^{est_K - len*8}
     */
    double diff = (double)est_K - (double)(len * 8);
    a->compress_fraction = (diff >= 0) ? 1.0 : pow(2.0, diff);

    return a;
}

void inc_analyze_report(const IncompressibilityAnalysis* a) {
    if (!a) return;
    printf("Incompressibility Analysis:\n");
    printf("  Length:            %zu bytes (%zu bits)\n", a->len, a->len * 8);
    printf("  Estimated K/|x|:   %.6f\n", a->estimated_ratio);
    printf("  Incompressibility: c=1: %s | c=5: %s | c=10: %s | c=20: %s | c=50: %s\n",
           a->incomp_C[0] ? "INCOMP" : "COMPR",
           a->incomp_C[1] ? "INCOMP" : "COMPR",
           a->incomp_C[2] ? "INCOMP" : "COMPR",
           a->incomp_C[3] ? "INCOMP" : "COMPR",
           a->incomp_C[4] ? "INCOMP" : "COMPR");
    printf("  Compress fraction: %.10f (fraction of strings this compressible)\n",
           a->compress_fraction);
}

void inc_analyze_free(IncompressibilityAnalysis* a) { free(a); }

/* ══════════════════════════════════════════════════════════════
   L6: Demonstration — Run All Canonical Proofs
   ══════════════════════════════════════════════════════════════ */

/*
 * Run all canonical incompressibility proofs for a given input size
 * and print a comprehensive report. This demonstrates the method
 * across sorting, selection, graphs, strings, and communication.
 *
 * Knowledge point: This function serves as an executable summary
 * of the incompressibility method's power. By varying a single
 * parameter n, the user can see the lower bounds for 16 different
 * computational problems, all proved using the same technique.
 * This demonstrates the UNIFYING POWER of Kolmogorov complexity
 * as a proof tool.
 *
 * The demo covers:
 *   - Sorting lower bounds (worst + average + heapsort optimality)
 *   - Selection lower bounds (max, median)
 *   - String algorithms (matching, LCS, palindrome)
 *   - Graph algorithms (connectivity, component counting, GNI)
 *   - Communication complexity (equality, disjointness)
 *   - Formal languages (regular + CFL pumping)
 *   - Meta-theorem (average = worst for random inputs)
 *
 * Each proof is constructed and validated, and the key quantity
 * (lower bound in comparisons/examinations/bits) is printed.
 */
void inc_run_all_proofs_demo(size_t n) {
    if (n > 100) n = 100; /* keep output manageable for large n */

    printf("\n");
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║  Incompressibility Method — All Canonical Proofs       ║\n");
    printf("║  Parameter size n = %-36zu║\n", n);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");

    IncompressibilityProof* p;
    size_t lb;
    double dval;

    /* ---- Sorting Lower Bounds ---- */
    printf("─── Sorting & Selection Lower Bounds ───\n\n");

    lb = inc_sorting_comparison_lower_bound(n);
    printf(" 1. Comparison Sorting Worst-Case:    %zu comparisons\n", lb);
    printf("    (log2(n!) by Stirling: n*log2(n) - n*log2(e) + O(log n))\n");

    lb = inc_average_case_sorting_bound(n);
    printf(" 2. Comparison Sorting Average-Case:  %zu comparisons\n", lb);
    printf("    (Same asymptotic form; entropy of random permutation)\n");

    dval = inc_heapsort_average_comparisons(n);
    printf(" 3. Heapsort Average Comparisons:     %.1f (~2n log2 n)\n", dval);
    printf("    (Schaffer-Sedgewick 1993 formula)\n");

    lb = inc_maximum_lower_bound(n);
    printf(" 4. Finding Maximum:                  %zu comparisons\n", lb);
    printf("    (Exact bound: n-1; each comparison resolves one dominance)\n");

    lb = inc_median_lower_bound(n);
    printf(" 5. Finding Median:                   %zu comparisons\n", lb);
    printf("    (Approximate bound: 3n/2 - O(1); harder than max)\n");

    printf("\n─── String Algorithm Lower Bounds ───\n\n");

    /* ---- String Algorithms ---- */
    size_t m = n / 4 > 0 ? n / 4 : 1;
    lb = inc_string_matching_lower_bound(n, m);
    printf(" 6. String Matching (n=%zu, m=%zu):   %zu examinations\n", n, m, lb);
    printf("    (n-m+1 possible starting positions; each needs >=1 exam)\n");

    lb = inc_lcs_lower_bound(n);
    printf(" 7. LCS (two strings of length n):    %zu comparisons\n", lb);
    printf("    (Omega(n^2); DP is optimal under comparison model)\n");

    lb = inc_palindrome_lower_bound(n);
    printf(" 8. Palindrome Recognition:           %zu examinations\n", lb);
    printf("    (ceil(n/2); must check each symmetric pair)\n");

    printf("\n─── Graph Algorithm Lower Bounds ───\n\n");

    /* ---- Graph Algorithms ---- */
    lb = inc_connectivity_edge_lower_bound(n);
    printf(" 9. Graph Connectivity Testing:       %zu edge examinations\n", lb);
    printf("    (Omega(n^2) for dense graphs)\n");

    size_t m_edges = n * (n - 1) / 4;
    lb = inc_component_counting_lower_bound(n, m_edges);
    printf("10. Component Counting (%zu edges):   %zu edge examinations\n", m_edges, lb);
    printf("    (Omega(n^2); same asymptotic as connectivity)\n");

    lb = inc_gni_certificate_size_bound(n);
    printf("11. GNI Certificate Size:             O(%zu) bits\n", lb);
    printf("    (Polynomial = O(n^2); GNI is in co-NP)\n");

    printf("\n─── Communication Complexity Lower Bounds ───\n\n");

    /* ---- Communication Complexity ---- */
    lb = inc_eq_communication_lower_bound(n);
    printf("12. EQ_n (Equality):                  %zu bits\n", lb);
    printf("    (Deterministic; randomized O(log n) with pub randomness)\n");

    lb = inc_disjointness_lower_bound(n);
    printf("13. DISJ_n (Set Disjointness):        %zu bits\n", lb);
    printf("    (Deterministic; randomized also Theta(n) — no gap!)\n");

    printf("\n─── Formal Languages ───\n\n");

    /* ---- Pumping Lemmas ---- */
    int reg = inc_pumping_lemma_regular(3, "a*");
    int cfl = inc_pumping_lemma_cfl(3, "a^n b^n");
    printf("14. Regular Pumping Test (a*):        %s\n", reg ? "SATISFIES" : "VIOLATES");
    printf("15. CFL Pumping Test (a^n b^n):       %s\n", cfl ? "SATISFIES" : "VIOLATES");
    printf("    (Incompressibility gives alternative proof of pumping lemmas)\n");

    printf("\n─── Meta-Theorems ───\n\n");

    /* ---- Average = Worst ---- */
    int frac = inc_average_equals_worst_case(n, PROOF_COUNTING);
    printf("16. Average = Worst for Random:       %d/%d (%.1f%% of inputs)\n",
           frac, 1000, frac / 10.0);
    printf("    (For c=10, fraction of c-incompressible inputs >= 1 - 1/1024)\n");

    /* ---- Full Proof Demonstration (sorting) ---- */
    printf("\n─── Complete Proof Example: Sorting Lower Bound ───\n");
    p = inc_proof_sorting_lower_bound(n);
    incproof_print(p);
    incproof_free(p);

    /* ---- Full Proof Demonstration (communication) ---- */
    printf("\n─── Complete Proof Example: EQ_n Communication ───\n");
    p = inc_proof_eq_communication(n);
    incproof_print(p);
    incproof_free(p);

    /* ---- Full Proof Demonstration (graph) ---- */
    printf("\n─── Complete Proof Example: Graph Connectivity ───\n");
    p = inc_proof_connectivity_lower_bound(n);
    incproof_print(p);
    incproof_free(p);

    /* ---- Compression Analysis ---- */
    printf("\n─── Compression-Based Analysis ───\n");
    printf("All proofs rely on the same template:\n");
    printf("  ASSUME violation → ENCODE short description → CONTRADICT incompressibility\n");
    printf("This unifies lower bounds across algorithms, graphs, strings, and communication.\n");

    /* ---- Counting argument ---- */
    printf("\n─── Incompressibility Counting ───\n");
    printf("For strings of length %zu bits:\n", n * 8);
    printf("  Total strings:    2^{%zu}\n", n * 8);
    printf("  Strings with K < %zu - 10:  <= 2^{%zu}\n", n * 8, n * 8 - 10);
    printf("  Fraction c-incompressible for c=10:  >= 1 - 1/1024 ~ 99.9%%\n");
    printf("  This is why the method works for `almost all` inputs!\n");

    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  End of Demonstration — %zu lower bounds proved         ║\n", (size_t)16);
    printf("╚══════════════════════════════════════════════════════════╝\n\n");
}

/* ══════════════════════════════════════════════════════════════
   L1: Kolmogorov Complexity Counting Lemma
   ══════════════════════════════════════════════════════════════ */

/*
 * LEMMA (Counting Bound for K(x)):
 *   The number of strings x in {0,1}^n with K(x) <= n - c
 *   is at most 2^{n - c + 1}.
 *
 * This lemma is the foundation of the incompressibility method.
 * It shows that "almost all" strings are c-incompressible for
 * any fixed c. Specifically:
 *   - At most 2^{-c+1} fraction of strings are compressible
 *   - For c = 10: at most 2/1024 ~ 0.2% are compressible
 *   - For c = 20: at most 2/1048576 ~ 0.0002% are compressible
 *
 * Proof: Each string x has at most one minimal description.
 * There are at most 2^{n-c+1} descriptions of length <= n-c.
 * Each description produces at most one string. So at most
 * 2^{n-c+1} strings can have K(x) <= n-c.
 *
 * Reference: Li-Vitányi §2.2, §6.1
 */

double inc_count_compressible_fraction(size_t n, int c) {
    /*
     * Returns: upper bound on fraction of strings of length n
     * that are c-compressible (have K(x) < n - c).
     *
     * Fraction <= 2^{n-c+1} / 2^n = 2^{-c+1}
     *
     * Knowledge point: The counting argument is the simplest
     * proof that most strings are incompressible. It uses only
     * the pigeonhole principle and the fact that descriptions
     * are themselves strings.
     */
    if (n == 0) return 0.0;
    if (c <= 0) return 1.0;
    double frac = pow(2.0, (double)(-c + 1));
    return frac < 1.0 ? frac : 1.0;
}

/* ══════════════════════════════════════════════════════════════
   L8: Information Distance via Kolmogorov Complexity
   ══════════════════════════════════════════════════════════════ */

/*
 * ADVANCED TOPIC: The Information Distance E(x,y) is defined as:
 *   E(x,y) = max(K(x|y), K(y|x)) + O(log max(K(x),K(y)))
 *
 * This is a UNIVERSAL metric: it minorizes (up to an additive
 * constant) every computable distance metric. In other words,
 * if any computable feature detects similarity between x and y,
 * the information distance detects it too — automatically.
 *
 * The Normalized Information Distance (NID):
 *   NID(x,y) = max(K(x|y), K(y|x)) / max(K(x), K(y))
 *
 * This is approximated in practice by NCD (see above).
 *
 * Since K(x|y) is uncomputable, we estimate it via:
 *   ~K(x|y) = K(xy) - K(y)   (symmetry of information)
 *
 * Reference: Bennett, Gacs, Li, Vitányi, Zurek (1998)
 *            "Information Distance", IEEE Trans. Info. Theory
 */

double inc_information_distance_estimate(const unsigned char* x, size_t xlen,
                                          const unsigned char* y, size_t ylen) {
    /*
     * Estimate NID(x,y) using compression-based approximations.
     *
     * NID(x,y) = max(K(x|y), K(y|x)) / max(K(x), K(y))
     *          ~ max(C(xy)-C(y), C(xy)-C(x)) / max(C(x), C(y))
     *
     * Knowledge point: This bridges Kolmogorov complexity with
     * practical machine learning. NID/NCD has been used for:
     * - Phylogenetic tree reconstruction from DNA sequences
     * - Language classification from text corpora
     * - Music genre classification from audio files
     * - Malware family detection from binary executables
     * All without any domain-specific feature engineering.
     */
    if (xlen == 0 && ylen == 0) return 0.0;

    /* Estimate K(x), K(y), K(xy) via compression */
    LZ77Token* tmp = NULL;
    size_t cx = lz77_compress(x, xlen, &tmp); free(tmp);
    size_t cy = lz77_compress(y, ylen, &tmp); free(tmp);

    unsigned char* xy = (unsigned char*)malloc(xlen + ylen);
    assert(xy != NULL);
    memcpy(xy, x, xlen);
    memcpy(xy + xlen, y, ylen);
    size_t cxy = lz77_compress(xy, xlen + ylen, &tmp); free(tmp);
    free(xy);

    /* K(x|y) ~ K(xy) - K(y) */
    size_t kx_given_y = (cxy > cy) ? (cxy - cy) : 0;
    size_t ky_given_x = (cxy > cx) ? (cxy - cx) : 0;

    size_t max_k = (cx > cy) ? cx : cy;
    size_t max_cond = (kx_given_y > ky_given_x) ? kx_given_y : ky_given_x;

    if (max_k == 0) return 0.0;
    return (double)max_cond / (double)max_k;
}
