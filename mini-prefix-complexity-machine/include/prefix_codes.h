/*
 * prefix_codes.h — Prefix code theory and algorithms
 *
 * L3: Kraft inequality, McMillan's theorem, optimal prefix codes
 * L5: Shannon-Fano coding, Huffman coding, arithmetic coding
 *
 * A prefix code is a set of codewords where no codeword is a prefix
 * of another. Equivalent to the codewords forming a prefix-free set.
 *
 * Kraft Inequality (1949): A prefix code with codeword lengths l₁,...,lₙ
 * exists iff Σ 2^{-l_i} ≤ 1.
 *
 * McMillan's Theorem (1956): Any uniquely decodable code satisfies Σ 2^{-l_i} ≤ 1,
 * so prefix codes are as efficient as any uniquely decodable code.
 *
 * Reference: Cover & Thomas §5, Li & Vitányi §3.1
 * Courses: MIT 6.441, Stanford EE376A, Berkeley EE229A
 */

#ifndef PREFIX_CODES_H
#define PREFIX_CODES_H

#include "prefix_machine.h"

/* ══════════════════════════════════════════════════════════════
   Code Tree Representation
   ══════════════════════════════════════════════════════════════ */

typedef struct PCTreeNode {
    int              symbol;   /* -1 for internal nodes */
    struct PCTreeNode* left;   /* 0 */
    struct PCTreeNode* right;  /* 1 */
} PCTreeNode;

typedef struct {
    PCTreeNode* root;
    int         n_symbols;
    char**      codewords;    /* bit string for each symbol */
    size_t*     lengths;      /* length of each codeword */
} PrefixCode;

PrefixCode* pc_create(int n_symbols);
void        pc_free(PrefixCode* code);
PrefixCode* pc_from_lengths(const int* lengths, int n);
int         pc_is_valid(const PrefixCode* code);
int         pc_encode(const PrefixCode* code, int symbol, char* out, size_t cap);
int         pc_decode(const PrefixCode* code, const char* bitstream, int len,
                      int* out_symbol, int* consumed);

/* ── Information-Theoretic Quantities ──────────────────────── */
double pc_shannon_entropy(const double* probs, int n);
double pc_expected_length(const double* probs, const int* lengths, int n);
double pc_fano_bound(double entropy, int n_symbols);

/* ══════════════════════════════════════════════════════════════
   Kraft Inequality
   ══════════════════════════════════════════════════════════════ */

double kraft_sum(const int* lengths, int n);
int    kraft_satisfied(const int* lengths, int n);
int*   kraft_lengths_from_probs(const double* probs, int n);

/* ══════════════════════════════════════════════════════════════
   Shannon-Fano Coding (1948)
   ══════════════════════════════════════════════════════════════ */
int*   shannon_fano_lengths(const double* probs, int n);
PrefixCode* shannon_fano_code(const double* probs, int n);

/* ══════════════════════════════════════════════════════════════
   Huffman Coding (1952) — optimal prefix code
   ══════════════════════════════════════════════════════════════ */
int*   huffman_lengths_compute(const double* probs, int n);
PrefixCode* huffman_code_compute(const double* probs, int n);
double huffman_expected_length(const double* probs, const int* lengths, int n);
double huffman_redundancy(const double* probs, const int* lengths, int n);

/* ══════════════════════════════════════════════════════════════
   Arithmetic Coding
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    unsigned int low;
    unsigned int high;
    unsigned int range;
    int          pending_bits;
} ArithmeticEncoder;

ArithmeticEncoder* arith_create(void);
void arith_encode_symbol(ArithmeticEncoder* ae, int symbol,
                          const double* cdf, int n_symbols);
void arith_flush(ArithmeticEncoder* ae, PMString* output);
int  arith_decode_symbol(unsigned int* value, const double* cdf, int n_symbols);
void arith_free(ArithmeticEncoder* ae);

/* ══════════════════════════════════════════════════════════════
   Universal Codes (for integers)
   ══════════════════════════════════════════════════════════════ */
size_t elias_delta_length(size_t n);
size_t elias_gamma_length(size_t n);
size_t elias_omega_length(size_t n);
size_t levenshtein_code_length(size_t n);

/* ══════════════════════════════════════════════════════════════
   Golomb & Rice Codes
   ══════════════════════════════════════════════════════════════ */
int*   pc_golomb_lengths(int max_n, int m);
size_t rice_code_length(size_t n, int k);
int*   pc_rice_lengths(int max_n, int k);

/* ══════════════════════════════════════════════════════════════
   McMillan's Theorem
   ══════════════════════════════════════════════════════════════ */
int pc_mcmillan_check(const int* lengths, int n);

/* ══════════════════════════════════════════════════════════════
   Kraft Construction
   ══════════════════════════════════════════════════════════════ */
PrefixCode* pc_kraft_construct(const int* lengths, int n);

/* ══════════════════════════════════════════════════════════════
   Tunstall Code (fixed-length output, variable-length input)
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    char*  block;
    size_t block_len;
    double prob;
} TunstallLeaf;

TunstallLeaf* pc_tunstall_build(const double* probs, int alphabet_size,
                                 int n_leaves);
void pc_tunstall_free(TunstallLeaf* leaves, int n);

/* ══════════════════════════════════════════════════════════════
   Block Coding
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    int*   block_lengths;
    int    block_size;
    int    n_blocks;
    double bits_per_symbol;
} BlockCodeResult;

BlockCodeResult* pc_block_code_analyze(const double* probs, int n_syms, int k);
void pc_block_code_free(BlockCodeResult* bcr);

#endif /* PREFIX_CODES_H */
