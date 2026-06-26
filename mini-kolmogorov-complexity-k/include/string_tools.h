/*
 * string_tools.h — String operations and transformations
 *
 * L3 Mathemetical Structures:
 *   Binary strings, alphabets, encodings.
 *   All strings are finite sequences over {0,1} or extended alphabets.
 *
 * Reference: Li & Vitányi §1.2, Cover & Thomas §1
 * Courses: MIT 6.045, Stanford CS254
 */

#ifndef STRING_TOOLS_H
#define STRING_TOOLS_H

#include "kolmogorov.h"

/* ── Alphabet operations ───────────────────────────────────── */
typedef struct {
    unsigned char* symbols;
    int            size;
    char*          name;
} KAlphabet;

KAlphabet* kalph_create_binary(void);
KAlphabet* kalph_create_ascii_printable(void);
KAlphabet* kalph_create_from_chars(const char* chars, int n);
KAlphabet* kalph_create_numeric(int base); /* digits 0..base-1 */
void       kalph_free(KAlphabet* a);
int        kalph_contains(const KAlphabet* a, unsigned char c);
int        kalph_index(const KAlphabet* a, unsigned char c);

/* ── Binary string conversions ─────────────────────────────── */
KString*  kstr_to_binary(const KString* s);
KString*  kstr_from_binary(const KString* bin);
KString*  kstr_from_integer(int64_t n);   /* binary representation */
int64_t   kstr_to_integer(const KString* s);

/* ── Encoding pairs / tuples ────────────────────────────────── */
/**
 * Encoding of ordered pair ⟨x, y⟩ with length:
 *   |⟨x, y⟩| = |x| + |y| + 2 log |x| + O(1)
 * This is used in the Invariance Theorem and Symmetry of Information.
 */
KString*  kstr_encode_pair(const KString* x, const KString* y);
int       kstr_decode_pair(const KString* enc, KString** x_out, KString** y_out);
KString*  kstr_encode_tuple(KString** xs, int n);
int       kstr_decode_tuple(const KString* enc, KString*** out, int n);

/* ── Self-delimiting encoding ───────────────────────────────── */
/**
 * Self-delimiting code: prefix x̄ = 1^{⌊log |x|⌋}0⌊log |x|⌋x
 * Length: |x̄| = |x| + 2 log |x| + 1
 */
KString*  kstr_self_delimiting(const KString* x);

/* ── String distances / information measures ────────────────── */
size_t    kstr_hamming_distance(const KString* a, const KString* b);
size_t    kstr_levenshtein_distance(const KString* a, const KString* b);
double    kstr_normalized_compression_distance(const KString* x, const KString* y);

/* ── Lexicographic enumeration ──────────────────────────────── */
/**
 * Generate all strings up to given length in lexicographic order.
 * Returns the string at index i (0-based).
 */
KString*  kstr_lex_nth(size_t i);
size_t    kstr_lex_index(const KString* s);

/* ── Prefix and substring operations ────────────────────────── */
int       kstr_is_prefix(const KString* prefix, const KString* s);
int       kstr_is_suffix(const KString* suffix, const KString* s);
KString*  kstr_substring(const KString* s, size_t start, size_t end);
int       kstr_contains(const KString* haystack, const KString* needle);

/* ── Random string generation ───────────────────────────────── */
KString*  kstr_random(size_t len, unsigned int* seed);

/* ── Binary sequence properties ─────────────────────────────── */
int       kstr_is_palindrome(const KString* s);
size_t    kstr_longest_repeated_substring(const KString* s);
int       kstr_count_ones(const KString* s);
int       kstr_count_runs(const KString* s);

#endif /* STRING_TOOLS_H */
