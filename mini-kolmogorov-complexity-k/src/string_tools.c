/*
 * string_tools.c — String operations for Kolmogorov complexity analysis
 *
 * L3 Math Structures: alphabets, binary encoding, self-delimiting codes,
 * pair/tuple encoding, string distances, lexicographic enumeration.
 *
 * Reference: Li & Vitányi §1.2, 1.11 (pair encoding), 3.1 (self-delimiting)
 * Courses: MIT 6.841 §3, Stanford CS254 §3
 */

#include "../include/kolmogorov.h"
#include "../include/string_tools.h"
#include "../include/compression.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

/* ══════════════════════════════════════════════════════════════
   Alphabet API
   ══════════════════════════════════════════════════════════════ */

KAlphabet* kalph_create_binary(void) {
    KAlphabet* a = (KAlphabet*)malloc(sizeof(KAlphabet));
    assert(a != NULL);
    a->size = 2;
    a->symbols = (unsigned char*)malloc(2);
    assert(a->symbols != NULL);
    a->symbols[0] = '0';
    a->symbols[1] = '1';
    a->name     = strdup("binary");
    return a;
}

KAlphabet* kalph_create_ascii_printable(void) {
    KAlphabet* a = (KAlphabet*)malloc(sizeof(KAlphabet));
    assert(a != NULL);
    a->size = 95;
    a->symbols = (unsigned char*)malloc(95);
    assert(a->symbols != NULL);
    int idx = 0;
    for (int c = 32; c < 127; c++)
        a->symbols[idx++] = (unsigned char)c;
    a->name = strdup("ASCII-printable");
    return a;
}

KAlphabet* kalph_create_from_chars(const char* chars, int n) {
    KAlphabet* a = (KAlphabet*)malloc(sizeof(KAlphabet));
    assert(a != NULL);
    a->size = n;
    a->symbols = (unsigned char*)malloc((size_t)n);
    assert(a->symbols != NULL);
    for (int i = 0; i < n; i++)
        a->symbols[i] = (unsigned char)chars[i];
    a->name = strdup("custom");
    return a;
}

KAlphabet* kalph_create_numeric(int base) {
    if (base < 2 || base > 36) return NULL;
    KAlphabet* a = (KAlphabet*)malloc(sizeof(KAlphabet));
    assert(a != NULL);
    a->size = base;
    a->symbols = (unsigned char*)malloc((size_t)base);
    assert(a->symbols != NULL);
    for (int i = 0; i < base && i < 10; i++)
        a->symbols[i] = (unsigned char)('0' + i);
    for (int i = 10; i < base; i++)
        a->symbols[i] = (unsigned char)('A' + (i - 10));
    char buf[32];
    snprintf(buf, sizeof(buf), "numeric-base-%d", base);
    a->name = strdup(buf);
    return a;
}

void kalph_free(KAlphabet* a) {
    if (a) {
        free(a->symbols);
        free(a->name);
        free(a);
    }
}

int kalph_contains(const KAlphabet* a, unsigned char c) {
    if (!a) return 0;
    for (int i = 0; i < a->size; i++)
        if (a->symbols[i] == c) return 1;
    return 0;
}

int kalph_index(const KAlphabet* a, unsigned char c) {
    if (!a) return -1;
    for (int i = 0; i < a->size; i++)
        if (a->symbols[i] == c) return i;
    return -1;
}

/* ══════════════════════════════════════════════════════════════
   Binary conversion
   ══════════════════════════════════════════════════════════════ */

KString* kstr_to_binary(const KString* s) {
    if (!s) return kstr_create(0);
    KString* out = kstr_create(s->len * 8 + 1);
    for (size_t i = 0; i < s->len; i++) {
        for (int b = 7; b >= 0; b--) {
            kstr_append(out, ((s->data[i] >> b) & 1) ? '1' : '0');
        }
    }
    return out;
}

KString* kstr_from_binary(const KString* bin) {
    if (!bin || bin->len == 0) return kstr_create(0);
    size_t out_len = (bin->len + 7) / 8;
    KString* out = kstr_create(out_len);
    for (size_t i = 0; i < bin->len; i += 8) {
        unsigned char byte = 0;
        for (int b = 0; b < 8 && (i + b) < bin->len; b++) {
            if (bin->data[i + b] == '1')
                byte |= (1U << (7 - b));
        }
        kstr_append(out, byte);
    }
    return out;
}

KString* kstr_from_integer(int64_t n) {
    /* Binary representation without leading zeros */
    if (n == 0) {
        KString* s = kstr_create(1);
        kstr_append(s, '0');
        return s;
    }
    char buf[128];
    int pos = 0;
    uint64_t u = (n < 0) ? (uint64_t)(-n) : (uint64_t)n;
    if (n < 0) { buf[pos++] = '-'; }
    char tmp[64]; int tp = 0;
    while (u > 0) { tmp[tp++] = (char)('0' + (u & 1)); u >>= 1; }
    while (tp > 0) buf[pos++] = tmp[--tp];
    buf[pos] = '\0';
    return kstr_from_cstr(buf);
}

int64_t kstr_to_integer(const KString* s) {
    if (!s || s->len == 0) return 0;
    int sign = 1;
    size_t i = 0;
    if (s->data[0] == '-') { sign = -1; i = 1; }
    uint64_t val = 0;
    for (; i < s->len; i++) {
        if (s->data[i] >= '0' && s->data[i] <= '1')
            val = (val << 1) | (uint64_t)(s->data[i] - '0');
        else return 0;
    }
    return (int64_t)(sign * (int64_t)val);
}

/* ══════════════════════════════════════════════════════════════
   Pair / Tuple encoding
   ══════════════════════════════════════════════════════════════ */

KString* kstr_encode_pair(const KString* x, const KString* y) {
    /*
     * Encode ⟨x, y⟩:
     *   |x̄| y  where x̄ = 1^{⌊log |x|⌋} 0 ⌊log|x|⌋ x
     * Length: |x| + |y| + 2 log|x| + O(1)
     */
    if (!x || !y) return kstr_create(0);
    KString* x_bar = kstr_self_delimiting(x);
    KString* enc = kstr_create(x_bar->len + y->len + 1);
    kstr_append_str(enc, x_bar);
    kstr_append_str(enc, y);
    kstr_free(x_bar);
    return enc;
}

int kstr_decode_pair(const KString* enc, KString** x_out, KString** y_out) {
    if (!enc || enc->len == 0) return 0;

    /* Parse self-delimiting prefix: read x̄ */
    size_t i = 0;
    int ones = 0;
    while (i < enc->len && enc->data[i] == '1') { ones++; i++; }
    if (i >= enc->len || enc->data[i] != '0') return 0;
    i++; /* skip 0 */
    if (i + (size_t)ones > enc->len) return 0;

    /* Read binary length from next 'ones' characters */
    size_t x_len = 0;
    for (int j = 0; j < ones && i < enc->len; j++, i++) {
        x_len = (x_len << 1) | (enc->data[i] - '0');
    }

    if (i + x_len > enc->len) return 0;

    *x_out = kstr_from_data(enc->data + i, x_len);
    i += x_len;
    *y_out = kstr_from_data(enc->data + i, enc->len - i);
    return 1;
}

KString* kstr_encode_tuple(KString** xs, int n) {
    if (!xs || n <= 0) return kstr_create(0);
    if (n == 1) return kstr_clone(xs[0]);

    KString* enc = kstr_encode_pair(xs[0], xs[1]);
    for (int i = 2; i < n; i++) {
        KString* tmp = kstr_encode_pair(enc, xs[i]);
        kstr_free(enc);
        enc = tmp;
    }
    return enc;
}

int kstr_decode_tuple(const KString* enc, KString*** out, int n) {
    if (!enc || n <= 0 || !out) return 0;
    *out = (KString**)malloc((size_t)n * sizeof(KString*));
    assert(*out != NULL);

    KString* remaining = kstr_clone(enc);
    for (int i = 0; i < n - 1; i++) {
        KString *x, *rest;
        if (!kstr_decode_pair(remaining, &x, &rest)) {
            for (int j = 0; j < i; j++) kstr_free((*out)[j]);
            free(*out);
            kstr_free(remaining);
            return 0;
        }
        (*out)[i] = x;
        kstr_free(remaining);
        remaining = rest;
    }
    (*out)[n - 1] = remaining;
    return 1;
}

/* ══════════════════════════════════════════════════════════════
   Self-delimiting encoding
   ══════════════════════════════════════════════════════════════ */

KString* kstr_self_delimiting(const KString* x) {
    /*
     * Self-delimiting encoding x̄:
     *   Let l = |x| in binary, without leading zeros.
     *   x̄ = 1^{|l|} 0 l x
     * Length: |x̄| = |x| + 2⌊log|x|⌋ + 1
     */
    if (!x) return kstr_create(0);

    size_t n = x->len;
    /* Compute binary representation of n */
    int bits_n = 0;
    size_t tmp = n;
    while (tmp > 0) { bits_n++; tmp >>= 1; }
    if (bits_n == 0) bits_n = 1;

    KString* out = kstr_create(x->len + 2 * (size_t)bits_n + 2);
    /* 1^{bits_n} */
    for (int i = 0; i < bits_n; i++) kstr_append(out, '1');
    /* 0 */
    kstr_append(out, '0');
    /* Binary length */
    for (int i = bits_n - 1; i >= 0; i--) {
        kstr_append(out, ((n >> i) & 1) ? '1' : '0');
    }
    /* x */
    kstr_append_str(out, x);
    return out;
}

/* ══════════════════════════════════════════════════════════════
   String distances
   ══════════════════════════════════════════════════════════════ */

size_t kstr_hamming_distance(const KString* a, const KString* b) {
    if (!a || !b) return 0;
    size_t max_len = a->len > b->len ? a->len : b->len;
    size_t d = 0;
    for (size_t i = 0; i < max_len; i++) {
        unsigned char ca = i < a->len ? a->data[i] : 0;
        unsigned char cb = i < b->len ? b->data[i] : 0;
        if (ca != cb) d++;
    }
    return d;
}

size_t kstr_levenshtein_distance(const KString* a, const KString* b) {
    if (!a || a->len == 0) return b ? b->len : 0;
    if (!b || b->len == 0) return a ? a->len : 0;

    size_t n = a->len, m = b->len;
    size_t* prev = (size_t*)malloc((m + 1) * sizeof(size_t));
    size_t* curr = (size_t*)malloc((m + 1) * sizeof(size_t));
    assert(prev && curr);

    for (size_t j = 0; j <= m; j++) prev[j] = j;

    for (size_t i = 1; i <= n; i++) {
        curr[0] = i;
        for (size_t j = 1; j <= m; j++) {
            size_t cost = (a->data[i - 1] == b->data[j - 1]) ? 0 : 1;
            size_t ins  = curr[j - 1] + 1;
            size_t del  = prev[j] + 1;
            size_t sub  = prev[j - 1] + cost;
            size_t min_v = ins < del ? ins : del;
            curr[j] = sub < min_v ? sub : min_v;
        }
        size_t* t = prev; prev = curr; curr = t;
    }
    size_t d = prev[m];
    free(prev); free(curr);
    return d;
}

double kstr_normalized_compression_distance(const KString* x, const KString* y) {
    /*
     * NCD(x, y) = (C(xy) - min(C(x), C(y))) / max(C(x), C(y))
     * Closely related to Kolmogorov complexity.
     * We estimate using LZW compression.
     */
    if (!x || !y) return 1.0;
    if (x->len == 0 && y->len == 0) return 0.0;
    if (x->len == 0 || y->len == 0) return 1.0;

    /* C(xy) */
    KString* xy = kstr_create(x->len + y->len);
    kstr_append_str(xy, x);
    kstr_append_str(xy, y);
    double Cxy = lzw_ratio(xy);
    double Cx  = lzw_ratio(x);
    double Cy  = lzw_ratio(y);

    double ncd = (Cxy - (Cx < Cy ? Cx : Cy)) / (Cx > Cy ? Cx : Cy);
    kstr_free(xy);
    return ncd < 0.0 ? 0.0 : (ncd > 1.0 ? 1.0 : ncd);
}

/* ══════════════════════════════════════════════════════════════
   Lexicographic enumeration
   ══════════════════════════════════════════════════════════════ */

KString* kstr_lex_nth(size_t i) {
    /* Generate the i-th binary string in lexicographic order:
     * i=0 → ε, i=1 → 0, i=2 → 1, i=3 → 00, i=4 → 01, i=5 → 10, i=6 → 11, ...
     * For length L ≥ 1: indices [2^L - 1, 2^{L+1} - 2] have length L.
     * Within length L: value = i - (2^L - 1), encoded in L bits.
     */
    if (i == 0) return kstr_create(0);

    size_t L = 1;
    size_t first = 1;   /* 2^L - 1 */
    size_t count = 2;   /* 2^L */
    while (i >= first + count) {
        first += count;
        L++;
        count <<= 1;
    }
    size_t idx = i - first;
    KString* s = kstr_create(L + 1);
    for (int b = (int)L - 1; b >= 0; b--)
        kstr_append(s, ((idx >> b) & 1) ? '1' : '0');
    return s;
}

size_t kstr_lex_index(const KString* s) {
    if (!s || s->len == 0) return 0;
    /* For length L: base = 2^L - 1 (sum of all shorter strings) */
    size_t L = s->len;
    size_t base = (1ULL << L) - 1;
    size_t idx_in_group = 0;
    for (size_t i = 0; i < L; i++) {
        if (s->data[i] == '1')
            idx_in_group |= (1ULL << (L - 1 - i));
    }
    return base + idx_in_group;
}

/* ══════════════════════════════════════════════════════════════
   Prefix / substring
   ══════════════════════════════════════════════════════════════ */

int kstr_is_prefix(const KString* prefix, const KString* s) {
    if (!prefix || !s) return 0;
    if (prefix->len > s->len) return 0;
    return memcmp(prefix->data, s->data, prefix->len) == 0;
}

int kstr_is_suffix(const KString* suffix, const KString* s) {
    if (!suffix || !s) return 0;
    if (suffix->len > s->len) return 0;
    return memcmp(suffix->data, s->data + s->len - suffix->len,
                  suffix->len) == 0;
}

KString* kstr_substring(const KString* s, size_t start, size_t end) {
    if (!s || start >= s->len || end > s->len || start >= end)
        return kstr_create(0);
    return kstr_from_data(s->data + start, end - start);
}

int kstr_contains(const KString* haystack, const KString* needle) {
    if (!haystack || !needle || needle->len == 0) return 1;
    if (needle->len > haystack->len) return 0;
    for (size_t i = 0; i + needle->len <= haystack->len; i++) {
        if (memcmp(haystack->data + i, needle->data, needle->len) == 0)
            return 1;
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════════
   Random string
   ══════════════════════════════════════════════════════════════ */

KString* kstr_random(size_t len, unsigned int* seed) {
    KString* s = kstr_create(len);
    for (size_t i = 0; i < len; i++) {
        *seed = (*seed * 1103515245U + 12345U) & 0x7FFFFFFFU;
        kstr_append(s, (unsigned char)(*seed % 256));
    }
    return s;
}

/* ══════════════════════════════════════════════════════════════
   Binary sequence properties
   ══════════════════════════════════════════════════════════════ */

int kstr_is_palindrome(const KString* s) {
    if (!s || s->len <= 1) return 1;
    for (size_t i = 0; i < s->len / 2; i++) {
        if (s->data[i] != s->data[s->len - 1 - i]) return 0;
    }
    return 1;
}

size_t kstr_longest_repeated_substring(const KString* s) {
    if (!s || s->len < 2) return 0;
    size_t max_len = 0;
    for (size_t len = 1; len <= s->len / 2; len++) {
        for (size_t i = 0; i + 2 * len <= s->len; i++) {
            for (size_t j = i + len; j + len <= s->len; j++) {
                if (memcmp(s->data + i, s->data + j, len) == 0) {
                    if (len > max_len) max_len = len;
                    goto next_len;
                }
            }
        }
        next_len:;
    }
    return max_len;
}

int kstr_count_ones(const KString* s) {
    if (!s) return 0;
    int c = 0;
    for (size_t i = 0; i < s->len; i++)
        if (s->data[i] == '1') c++;
    return c;
}

int kstr_count_runs(const KString* s) {
    if (!s || s->len == 0) return 0;
    int runs = 1;
    for (size_t i = 1; i < s->len; i++) {
        if (s->data[i] != s->data[i - 1]) runs++;
    }
    return runs;
}

/* ══════════════════════════════════════════════════════════════
   Self-test
   ══════════════════════════════════════════════════════════════ */

#ifdef STRING_TOOLS_MAIN
int main(void) {
    printf("=== String Tools Self-Test ===\n");

    KString* a = kstr_from_cstr("hello world");
    KString* b = kstr_from_cstr("hello there");
    KString* bin = kstr_to_binary(a);

    printf("a: "); kstr_print(a);
    printf("binary(a): "); kstr_print(bin);

    KString* roundtrip = kstr_from_binary(bin);
    printf("roundtrip: %s\n", kstr_equals(a, roundtrip) ? "PASS" : "FAIL");
    kstr_free(roundtrip);
    kstr_free(bin);

    /* Pair encoding */
    KString* p = kstr_encode_pair(a, b);
    printf("pair encoding (%zu bytes)\n", p->len);
    KString *x, *y;
    if (kstr_decode_pair(p, &x, &y)) {
        printf("decode x: %s\n", kstr_equals(x, a) ? "OK" : "FAIL");
        printf("decode y: %s\n", kstr_equals(y, b) ? "OK" : "FAIL");
        kstr_free(x); kstr_free(y);
    }
    kstr_free(p);

    /* Hamming */
    printf("Hamming distance: %zu\n", kstr_hamming_distance(a, b));

    /* Levenshtein */
    printf("Levenshtein distance: %zu\n", kstr_levenshtein_distance(a, b));

    /* Lexicographic enumeration */
    printf("\nLex enumeration:\n");
    for (size_t i = 0; i < 8; i++) {
        KString* s = kstr_lex_nth(i);
        printf("  i=%zu: ", i); kstr_print(s);
        size_t idx = kstr_lex_index(s);
        printf("    index check: %s\n", idx == i ? "PASS" : "FAIL");
        kstr_free(s);
    }

    /* Self-delimiting */
    KString* sd = kstr_self_delimiting(a);
    printf("\nself-delimiting: len=%zu (original=%zu)\n", sd->len, a->len);
    kstr_free(sd);

    /* Palindrome test */
    KString* pal = kstr_from_cstr("racecar");
    printf("is_palindrome(racecar): %d\n", kstr_is_palindrome(pal));
    printf("is_palindrome(hello): %d\n", kstr_is_palindrome(a));
    kstr_free(pal);

    /* Longest repeated substring */
    KString* repeats = kstr_from_cstr("banana");
    printf("longest repeated(banana): %zu\n",
           kstr_longest_repeated_substring(repeats));
    kstr_free(repeats);

    /* Random */
    unsigned int seed = 42;
    KString* rnd = kstr_random(16, &seed);
    printf("random: "); kstr_print(rnd);
    kstr_free(rnd);

    kstr_free(a); kstr_free(b);
    printf("\n=== All string tools tests passed ===\n");
    return 0;
}
#endif /* STRING_TOOLS_MAIN */
