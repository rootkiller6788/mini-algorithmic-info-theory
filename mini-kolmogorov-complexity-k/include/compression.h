/*
 * compression.h — Practical compression algorithms
 *
 * L5 Algorithms: each compression method is a real, independently-studied
 * algorithm demonstrating Kolmogorov complexity upper bounds in practice.
 *
 *   LZ77  — Ziv & Lempel (1977), sliding window
 *   LZ78  — Ziv & Lempel (1978), dictionary
 *   LZW   — Welch (1984), dynamic dictionary
 *   RLE   — Run-Length Encoding
 *   Huffman — Huffman (1952), optimal prefix code
 *   BWT   — Burrows-Wheeler Transform (1994)
 *
 * Reference: Cover & Thomas, Li & Vitányi §2.2
 * Courses: MIT 6.841, Stanford CS254, CMU 15-855
 */

#ifndef COMPRESSION_H
#define COMPRESSION_H

#include "kolmogorov.h"

/* ══════════════════════════════════════════════════════════════
   LZ77 — Sliding Window Compression (Ziv-Lempel 1977)
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    size_t  offset;     /* distance back in window */
    size_t  length;     /* length of match */
    unsigned char next; /* next literal character */
} LZ77Token;

typedef struct {
    LZ77Token* tokens;
    size_t     n_tokens;
    size_t     cap;
    size_t     window_size;
    size_t     lookahead_size;
} LZ77State;

LZ77State*  lz77_create(size_t window_size, size_t lookahead);
void        lz77_compress(LZ77State* s, const KString* input);
KString*    lz77_decompress(const LZ77State* s);
size_t      lz77_compressed_size(const LZ77State* s);
void        lz77_print_tokens(const LZ77State* s);
void        lz77_free(LZ77State* s);
double      lz77_ratio(const KString* input); /* compressed/original */

/* ══════════════════════════════════════════════════════════════
   LZ78 — Dictionary-Based Compression (Ziv-Lempel 1978)
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    int     index;          /* dictionary entry index */
    unsigned char next;     /* next character */
} LZ78Token;

typedef struct {
    LZ78Token* tokens;
    size_t     n_tokens;
    size_t     cap;
} LZ78State;

LZ78State*  lz78_create(void);
void        lz78_compress(LZ78State* s, const KString* input);
KString*    lz78_decompress(const LZ78State* s);
size_t      lz78_compressed_size(const LZ78State* s);
void        lz78_free(LZ78State* s);
double      lz78_ratio(const KString* input);

/* ══════════════════════════════════════════════════════════════
   LZW — Welch Variant (1984) with dynamic dictionary
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    int*    codes;
    size_t  n_codes;
    size_t  cap;
    int     dict_size;
} LZWState;

LZWState*   lzw_create(void);
void        lzw_compress(LZWState* s, const KString* input);
KString*    lzw_decompress(const LZWState* s);
size_t      lzw_compressed_size(const LZWState* s);
void        lzw_free(LZWState* s);
double      lzw_ratio(const KString* input);

/* ══════════════════════════════════════════════════════════════
   RLE — Run-Length Encoding
   ══════════════════════════════════════════════════════════════ */
KString*    rle_encode(const KString* input);
KString*    rle_decode(const KString* encoded);
double      rle_ratio(const KString* input);

/* ══════════════════════════════════════════════════════════════
   Huffman Coding (1952)
   ══════════════════════════════════════════════════════════════ */
typedef struct HNode {
    unsigned char  symbol;
    size_t         freq;
    struct HNode*  left;
    struct HNode*  right;
} HuffmanNode;

typedef struct {
    HuffmanNode*   root;
    char**         codes;       /* codes[i] = bitstring for symbol i */
    size_t*        code_lens;   /* length of each code */
    size_t         freq[256];   /* frequency table */
    size_t         original_size;
    size_t         compressed_size;
} HuffmanTree;

HuffmanTree* huffman_build(const KString* input);
KString*     huffman_encode(const HuffmanTree* tree, const KString* input);
KString*     huffman_decode(const HuffmanTree* tree, const KString* encoded);
void         huffman_print_codes(const HuffmanTree* tree);
void         huffman_free(HuffmanTree* tree);
double       huffman_ratio(const KString* input);

/* ══════════════════════════════════════════════════════════════
   BWT — Burrows-Wheeler Transform (1994)
   ══════════════════════════════════════════════════════════════ */
KString*    bwt_transform(const KString* input);
KString*    bwt_inverse(const KString* transformed, size_t original_index);
size_t      bwt_original_index(const KString* input);
double      bwt_ratio(const KString* input);

/* ══════════════════════════════════════════════════════════════
   Compression benchmark
   ══════════════════════════════════════════════════════════════ */
typedef struct {
    const char* algorithm;
    double      ratio;
    size_t      original_size;
    size_t      compressed_size;
} CompressionResult;

CompressionResult* compress_benchmark(const KString* input, int* n_results);
void               compress_benchmark_print(const KString* input);

#endif /* COMPRESSION_H */
