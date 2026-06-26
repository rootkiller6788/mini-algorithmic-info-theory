#ifndef BINARY_STRING_H
#define BINARY_STRING_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

/* ============================================================
 * Binary String ADT — fundamental for prefix-free code theory
 *
 * A binary string is a finite sequence over {0,1}.
 * The set of all binary strings {0,1}* forms the domain
 * of all prefix-free machines. Key operations: prefix
 * detection, Kraft sum validation, lexicographic ordering.
 *
 * Knowledge points encoded:
 *   - Prefix-free property (no string is prefix of another)
 *   - Kraft inequality: Σ 2^{-|s_i|} ≤ 1
 *   - Binary tree embedding of prefix codes
 *   - Lexicographic ordering of programs
 *   - Self-delimiting encoding
 * ============================================================ */

#define MAX_BITSTR_LEN 256

typedef struct {
    uint8_t bits[(MAX_BITSTR_LEN + 7) / 8];
    size_t length;   /* number of bits */
} BitString;

/* ---- Lifecycle ---- */
BitString* bs_create(const uint8_t* data, size_t bit_len);
BitString* bs_copy(const BitString* src);
void       bs_free(BitString* bs);
void       bs_clear(BitString* bs);

/* ---- Accessors ---- */
int    bs_get_bit(const BitString* bs, size_t pos);
void   bs_set_bit(BitString* bs, size_t pos, int val);
size_t bs_len(const BitString* bs);
int    bs_is_empty(const BitString* bs);

/* ---- String operations ---- */
int    bs_append_bit(BitString* bs, int bit);
int    bs_append_bits(BitString* bs, const BitString* suffix);
BitString* bs_concat(const BitString* a, const BitString* b);
BitString* bs_substring(const BitString* bs, size_t start, size_t len);
BitString* bs_remove_prefix(const BitString* bs, size_t n);

/* ---- Comparison ---- */
int    bs_equal(const BitString* a, const BitString* b);
int    bs_compare_lex(const BitString* a, const BitString* b);
int    bs_is_prefix(const BitString* prefix, const BitString* str);
int    bs_has_prefix(const BitString* str, const BitString* prefix);
int    bs_common_prefix_len(const BitString* a, const BitString* b);

/* ---- Encoding (natural numbers → binary strings) ---- */
BitString* bs_from_uint64(uint64_t n);
uint64_t   bs_to_uint64(const BitString* bs, int* overflow);
BitString* bs_encode_elias_gamma(uint64_t n);
uint64_t   bs_decode_elias_gamma(const BitString* bs, size_t* bits_read);
BitString* bs_encode_elias_delta(uint64_t n);
uint64_t   bs_decode_elias_delta(const BitString* bs, size_t* bits_read);
BitString* bs_encode_elias_omega(uint64_t n);
uint64_t   bs_decode_elias_omega(const BitString* bs, size_t* bits_read);

/* ---- Display ---- */
void bs_print(const BitString* bs);
void bs_print_verbose(const BitString* bs);
char* bs_to_string(const BitString* bs);

/* ---- Prefix-free set management ---- */
typedef struct {
    BitString** strings;
    size_t      count;
    size_t      capacity;
} PrefixFreeSet;

PrefixFreeSet* pfs_create(size_t capacity);
void           pfs_free(PrefixFreeSet* set);
int            pfs_add(PrefixFreeSet* set, const BitString* s);
int            pfs_contains(const PrefixFreeSet* set, const BitString* s);
int            pfs_is_prefix_free(const PrefixFreeSet* set);
double         pfs_kraft_sum(const PrefixFreeSet* set);
int            pfs_is_maximal(const PrefixFreeSet* set);
size_t         pfs_total_strings_at_len(const PrefixFreeSet* set, size_t len);
void           pfs_print(const PrefixFreeSet* set);
void           pfs_sort_lex(PrefixFreeSet* set);

/* ---- Kraft inequality: Σ 2^{-|s|} ≤ 1 ---- */
/* Returns the Kraft sum; must be ≤ 1 for prefix-free sets */
double kraft_sum_from_lengths(const size_t* lengths, size_t count);

/* ---- Kraft-McMillan: given lengths satisfying Kraft ≤ 1,
 *      construct a prefix-free set ---- */
PrefixFreeSet* construct_prefix_free_set(const size_t* lengths, size_t count);

/* ---- Binary tree representation of prefix codes ---- */
typedef struct PrefixTreeNode {
    struct PrefixTreeNode* child[2];   /* 0 = left, 1 = right */
    int is_terminal;                   /* marks end of a codeword */
} PrefixTreeNode;

PrefixTreeNode* ptree_create(void);
void            ptree_free(PrefixTreeNode* root);
int             ptree_insert(PrefixTreeNode* root, const BitString* s);
int             ptree_lookup(const PrefixTreeNode* root, const BitString* s);
int             ptree_is_prefix_free(PrefixTreeNode* root);
size_t          ptree_kraft_sum(PrefixTreeNode* root, size_t depth);

#endif /* BINARY_STRING_H */
