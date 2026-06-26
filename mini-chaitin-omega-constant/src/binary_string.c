#include "binary_string.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <inttypes.h>

/* ============================================================
 * Binary String ADT — Complete Implementation
 *
 * Encodes the following knowledge points:
 *   L1: BitString typedef, PrefixFreeSet, PrefixTreeNode
 *   L2: Prefix-free property, self-delimiting encoding
 *   L3: Binary tree structure for prefix codes
 *   L4: Kraft inequality validation
 *   L5: Elias gamma/delta/omega universal code construction
 *   L6: Prefix code generation from length specifications
 *
 * Reference: Li & Vitányi (2019), Cover & Thomas (2006)
 * ============================================================ */

/* ── Lifecycle ────────────────────────────────────────────── */

BitString* bs_create(const uint8_t* data, size_t bit_len) {
    if (bit_len > MAX_BITSTR_LEN) return NULL;
    BitString* bs = (BitString*)calloc(1, sizeof(BitString));
    if (!bs) return NULL;
    bs->length = bit_len;
    if (data && bit_len > 0) {
        size_t bytes = (bit_len + 7) / 8;
        memcpy(bs->bits, data, bytes);
        /* Clear trailing bits in last partial byte */
        if (bit_len % 8 != 0) {
            bs->bits[bytes - 1] &= (uint8_t)((1 << (bit_len % 8)) - 1);
        }
    }
    return bs;
}

BitString* bs_copy(const BitString* src) {
    if (!src) return NULL;
    return bs_create(src->bits, src->length);
}

void bs_free(BitString* bs) {
    if (bs) { free(bs); }
}

void bs_clear(BitString* bs) {
    if (!bs) return;
    memset(bs->bits, 0, sizeof(bs->bits));
    bs->length = 0;
}

/* ── Accessors ────────────────────────────────────────────── */

int bs_get_bit(const BitString* bs, size_t pos) {
    if (!bs || pos >= bs->length) return -1;
    size_t byte_idx = pos / 8;
    size_t bit_idx  = pos % 8;
    return (bs->bits[byte_idx] >> bit_idx) & 1;
}

void bs_set_bit(BitString* bs, size_t pos, int val) {
    if (!bs || pos >= MAX_BITSTR_LEN) return;
    size_t byte_idx = pos / 8;
    size_t bit_idx  = pos % 8;
    if (val) {
        bs->bits[byte_idx] |= (uint8_t)(1U << bit_idx);
    } else {
        bs->bits[byte_idx] &= (uint8_t)(~(1U << bit_idx));
    }
    if (pos >= bs->length) {
        bs->length = pos + 1;
    }
}

size_t bs_len(const BitString* bs) {
    return bs ? bs->length : 0;
}

int bs_is_empty(const BitString* bs) {
    return (!bs || bs->length == 0);
}

/* ── String Operations ───────────────────────────────────── */

int bs_append_bit(BitString* bs, int bit) {
    if (!bs || bs->length >= MAX_BITSTR_LEN) return -1;
    bs_set_bit(bs, bs->length, bit);
    return 0;
}

int bs_append_bits(BitString* bs, const BitString* suffix) {
    if (!bs || !suffix) return -1;
    if (bs->length + suffix->length > MAX_BITSTR_LEN) return -1;
    for (size_t i = 0; i < suffix->length; i++) {
        bs_append_bit(bs, bs_get_bit(suffix, i));
    }
    return 0;
}

BitString* bs_concat(const BitString* a, const BitString* b) {
    if (!a || !b) return NULL;
    size_t total_len = a->length + b->length;
    if (total_len > MAX_BITSTR_LEN) return NULL;
    BitString* result = bs_create(NULL, total_len);
    if (!result) return NULL;
    for (size_t i = 0; i < a->length; i++)
        bs_set_bit(result, i, bs_get_bit(a, i));
    for (size_t i = 0; i < b->length; i++)
        bs_set_bit(result, a->length + i, bs_get_bit(b, i));
    return result;
}

BitString* bs_substring(const BitString* bs, size_t start, size_t len) {
    if (!bs || start + len > bs->length) return NULL;
    BitString* sub = bs_create(NULL, len);
    if (!sub) return NULL;
    for (size_t i = 0; i < len; i++)
        bs_set_bit(sub, i, bs_get_bit(bs, start + i));
    return sub;
}

BitString* bs_remove_prefix(const BitString* bs, size_t n) {
    if (!bs || n > bs->length) return NULL;
    return bs_substring(bs, n, bs->length - n);
}

/* ── Comparison ──────────────────────────────────────────── */

int bs_equal(const BitString* a, const BitString* b) {
    if (!a || !b) return (a == b);
    if (a->length != b->length) return 0;
    size_t bytes = (a->length + 7) / 8;
    if (memcmp(a->bits, b->bits, bytes) != 0) return 0;
    /* Check trailing bits in last byte */
    if (a->length % 8 != 0) {
        uint8_t mask = (uint8_t)((1 << (a->length % 8)) - 1);
        if ((a->bits[bytes - 1] & mask) != (b->bits[bytes - 1] & mask))
            return 0;
    }
    return 1;
}

int bs_compare_lex(const BitString* a, const BitString* b) {
    if (!a || !b) return 0;
    size_t min_len = (a->length < b->length) ? a->length : b->length;
    for (size_t i = 0; i < min_len; i++) {
        int ba = bs_get_bit(a, i);
        int bb = bs_get_bit(b, i);
        if (ba < bb) return -1;
        if (ba > bb) return 1;
    }
    if (a->length < b->length) return -1;
    if (a->length > b->length) return 1;
    return 0;
}

int bs_is_prefix(const BitString* prefix, const BitString* str) {
    if (!prefix || !str) return 0;
    if (prefix->length > str->length) return 0;
    for (size_t i = 0; i < prefix->length; i++) {
        if (bs_get_bit(prefix, i) != bs_get_bit(str, i)) return 0;
    }
    return 1;
}

int bs_has_prefix(const BitString* str, const BitString* prefix) {
    return bs_is_prefix(prefix, str);
}

int bs_common_prefix_len(const BitString* a, const BitString* b) {
    if (!a || !b) return 0;
    size_t max_check = (a->length < b->length) ? a->length : b->length;
    size_t i;
    for (i = 0; i < max_check; i++) {
        if (bs_get_bit(a, i) != bs_get_bit(b, i)) break;
    }
    return (int)i;
}

/* ── Natural Number Encoding ─────────────────────────────── */

BitString* bs_from_uint64(uint64_t n) {
    BitString* bs = bs_create(NULL, 0);
    if (!bs) return NULL;
    if (n == 0) {
        bs_append_bit(bs, 0);
        return bs;
    }
    /* Build bits MSB first, then reverse */
    uint8_t temp_bits[64];
    int count = 0;
    while (n > 0) {
        temp_bits[count++] = (uint8_t)(n & 1);
        n >>= 1;
    }
    /* store LSB first */
    for (int i = 0; i < count; i++) {
        bs_append_bit(bs, temp_bits[i]);
    }
    return bs;
}

uint64_t bs_to_uint64(const BitString* bs, int* overflow) {
    if (!bs) { if (overflow) *overflow = 1; return 0; }
    if (overflow) *overflow = 0;
    if (bs->length > 64) { if (overflow) *overflow = 1; }
    uint64_t result = 0;
    size_t limit = (bs->length > 64) ? 64 : bs->length;
    for (size_t i = 0; i < limit; i++) {
        if (bs_get_bit(bs, i)) result |= (1ULL << i);
    }
    return result;
}

/* ── Elias Gamma Encoding ────────────────────────────────── */
/* Elias gamma: encodes n as floor(log₂ n) zeros, then binary of n.
 * Length: 2⌊log₂ n⌋ + 1 bits.
 *
 * Knowledge point: Universal code — length depends only on n,
 * optimal for distributions where P(n) ~ 1/n².
 * Reference: Elias (1975) */

BitString* bs_encode_elias_gamma(uint64_t n) {
    BitString* result = bs_create(NULL, 0);
    if (!result) return NULL;
    if (n == 0) { bs_append_bit(result, 1); return result; }
    n += 1; /* encode n+1 so 0 encodes to 1 */
    /* Count bits needed — floor(log2 n) */
    int bit_count = 0;
    uint64_t temp = n;
    while (temp > 0) { bit_count++; temp >>= 1; }
    /* Write (bit_count - 1) zeros */
    for (int i = 0; i < bit_count - 1; i++)
        bs_append_bit(result, 0);
    /* Write binary of n (MSB first is 1, which terminates zeros) */
    for (int i = bit_count - 1; i >= 0; i--)
        bs_append_bit(result, (int)((n >> i) & 1));
    return result;
}

uint64_t bs_decode_elias_gamma(const BitString* bs, size_t* bits_read) {
    if (!bs) { if (bits_read) *bits_read = 0; return 0; }
    size_t zero_count = 0;
    size_t pos = 0;
    while (pos < bs->length && bs_get_bit(bs, pos) == 0) {
        zero_count++; pos++;
    }
    if (pos >= bs->length) { if (bits_read) *bits_read = pos; return 0; }
    /* Read the 1 as MSB, then zero_count more bits */
    uint64_t value = 0;
    for (size_t i = 0; i <= zero_count && pos < bs->length; i++, pos++) {
        value = (value << 1) | (uint64_t)bs_get_bit(bs, pos);
    }
    if (bits_read) *bits_read = pos;
    return (value > 0) ? value - 1 : 0;
}

/* ── Elias Delta Encoding ────────────────────────────────── */
/* Elias delta: uses gamma to encode the length, then the binary.
 * Length ≈ log₂ n + 2 log₂ log₂ n + 1.
 * Better than gamma for large n.
 * Reference: Elias (1975) */

BitString* bs_encode_elias_delta(uint64_t n) {
    BitString* result = bs_create(NULL, 0);
    if (!result) return NULL;
    n += 1;
    int bit_count = 0;
    uint64_t temp = n;
    while (temp > 0) { bit_count++; temp >>= 1; }
    /* Encode bit_count with gamma */
    BitString* gamma_len = bs_encode_elias_gamma((uint64_t)(bit_count - 1));
    if (!gamma_len) { bs_free(result); return NULL; }
    bs_append_bits(result, gamma_len);
    bs_free(gamma_len);
    /* Append binary of n without the leading 1 */
    for (int i = bit_count - 2; i >= 0; i--)
        bs_append_bit(result, (int)((n >> i) & 1));
    return result;
}

uint64_t bs_decode_elias_delta(const BitString* bs, size_t* bits_read) {
    if (!bs) { if (bits_read) *bits_read = 0; return 0; }
    size_t consumed = 0;
    uint64_t bit_count_minus_1 = bs_decode_elias_gamma(bs, &consumed);
    size_t bit_count = (size_t)bit_count_minus_1 + 1;
    if (bit_count == 0 || consumed + (bit_count - 1) > bs->length) {
        if (bits_read) *bits_read = consumed;
        return 0;
    }
    uint64_t value = 1; /* leading 1 */
    for (size_t i = 1; i < bit_count; i++) {
        value = (value << 1) | (uint64_t)bs_get_bit(bs, consumed + i - 1);
    }
    if (bits_read) *bits_read = consumed + bit_count - 1;
    return (value > 0) ? value - 1 : 0;
}

/* ── Elias Omega Encoding ────────────────────────────────── */
/* Elias omega: recursively encodes the length.
 * Repeatedly encodes length until length == 1.
 * Terminates each group with a 0 bit (except the last).
 * Reference: Elias (1975), Li & Vitányi (2019) */

BitString* bs_encode_elias_omega(uint64_t n) {
    BitString* result = bs_create(NULL, 0);
    if (!result) return NULL;
    n += 1;
    /* Build groups from innermost out */
    uint64_t groups[16];
    int num_groups = 0;
    uint64_t current = n;
    while (current > 1) {
        groups[num_groups++] = current;
        /* floor(log₂ current) */
        int bits = 0;
        uint64_t t = current;
        while (t > 0) { bits++; t >>= 1; }
        current = (uint64_t)(bits - 1);
    }
    /* Write innermost first: just "0" for the final group */
    bs_append_bit(result, 0);
    /* Write each group: binary of k, then 0 terminator */
    for (int g = num_groups - 1; g >= 0; g--) {
        uint64_t k = groups[g];
        int bits = 0;
        uint64_t t = k;
        while (t > 0) { bits++; t >>= 1; }
        /* Write binary from MSB */
        for (int i = bits - 1; i >= 0; i--)
            bs_append_bit(result, (int)((k >> i) & 1));
        if (g > 0) bs_append_bit(result, 0);
    }
    return result;
}

uint64_t bs_decode_elias_omega(const BitString* bs, size_t* bits_read) {
    if (!bs) { if (bits_read) *bits_read = 0; return 0; }
    size_t pos = 0;
    uint64_t value = 1;
    while (pos < bs->length) {
        if (bs_get_bit(bs, pos) == 0) {
            pos++;
            /* value is the next length */
            if (value == 1) break; /* final group */
            /* Read 'value' bits */
            if (pos + value > bs->length) break;
            uint64_t next = bs_get_bit(bs, pos) ? 1 : 0;
            for (uint64_t i = 1; i < value; i++) {
                next = (next << 1) | (uint64_t)bs_get_bit(bs, pos + i);
            }
            /* The next bit should be 0 as terminator of this group */
            pos += value;
            value = next;
        } else {
            break;
        }
    }
    if (bits_read) *bits_read = pos;
    return (value > 0) ? value - 1 : 0;
}

/* ── Display ─────────────────────────────────────────────── */

void bs_print(const BitString* bs) {
    if (!bs) { printf("(null)\n"); return; }
    if (bs->length == 0) { printf("(empty)\n"); return; }
    for (size_t i = 0; i < bs->length; i++) {
        printf("%d", bs_get_bit(bs, i));
    }
    printf("\n");
}

void bs_print_verbose(const BitString* bs) {
    if (!bs) { printf("BitString: (null)\n"); return; }
    printf("BitString [%zu bits]: ", bs->length);
    bs_print(bs);
}

char* bs_to_string(const BitString* bs) {
    if (!bs) return strdup("(null)");
    if (bs->length == 0) return strdup("");
    char* str = (char*)malloc(bs->length + 1);
    if (!str) return NULL;
    for (size_t i = 0; i < bs->length; i++)
        str[i] = (char)('0' + bs_get_bit(bs, i));
    str[bs->length] = '\0';
    return str;
}

/* ── Prefix-Free Set Management ──────────────────────────── */
/* Knowledge point L2: A set S ⊆ {0,1}* is prefix-free if no
 * string in S is a proper prefix of any other string in S.
 * The Kraft inequality Σ 2^{-|s|} ≤ 1 is necessary and sufficient
 * for existence of a prefix-free set with given lengths.
 * Reference: Kraft (1949), McMillan (1956) */

PrefixFreeSet* pfs_create(size_t capacity) {
    PrefixFreeSet* set = (PrefixFreeSet*)calloc(1, sizeof(PrefixFreeSet));
    if (!set) return NULL;
    set->capacity = (capacity > 0) ? capacity : 16;
    set->strings = (BitString**)calloc(set->capacity, sizeof(BitString*));
    if (!set->strings) { free(set); return NULL; }
    set->count = 0;
    return set;
}

void pfs_free(PrefixFreeSet* set) {
    if (!set) return;
    for (size_t i = 0; i < set->count; i++)
        bs_free(set->strings[i]);
    free(set->strings);
    free(set);
}

int pfs_add(PrefixFreeSet* set, const BitString* s) {
    if (!set || !s) return -1;
    /* Check prefix-free property with existing strings */
    for (size_t i = 0; i < set->count; i++) {
        if (bs_is_prefix(set->strings[i], s) || bs_is_prefix(s, set->strings[i]))
            return -1; /* violates prefix-free property */
    }
    if (set->count >= set->capacity) {
        size_t new_cap = set->capacity * 2;
        BitString** new_arr = (BitString**)realloc(set->strings,
                                                     new_cap * sizeof(BitString*));
        if (!new_arr) return -1;
        set->strings = new_arr;
        set->capacity = new_cap;
    }
    set->strings[set->count] = bs_copy(s);
    if (!set->strings[set->count]) return -1;
    set->count++;
    return 0;
}

int pfs_contains(const PrefixFreeSet* set, const BitString* s) {
    if (!set || !s) return 0;
    for (size_t i = 0; i < set->count; i++)
        if (bs_equal(set->strings[i], s)) return 1;
    return 0;
}

int pfs_is_prefix_free(const PrefixFreeSet* set) {
    if (!set) return 1;
    for (size_t i = 0; i < set->count; i++) {
        for (size_t j = i + 1; j < set->count; j++) {
            if (bs_is_prefix(set->strings[i], set->strings[j]) ||
                bs_is_prefix(set->strings[j], set->strings[i]))
                return 0;
        }
    }
    return 1;
}

double pfs_kraft_sum(const PrefixFreeSet* set) {
    if (!set) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < set->count; i++) {
        if (set->strings[i])
            sum += pow(0.5, (double)set->strings[i]->length);
    }
    return sum;
}

int pfs_is_maximal(const PrefixFreeSet* set) {
    /* A prefix-free set is maximal (complete) if no string can be added
     * without violating the prefix-free property.
     * For a maximal prefix-free set, Kraft sum = 1. */
    if (!set) return 0;
    return (fabs(pfs_kraft_sum(set) - 1.0) < 1e-10);
}

size_t pfs_total_strings_at_len(const PrefixFreeSet* set, size_t len) {
    if (!set) return 0;
    size_t count = 0;
    for (size_t i = 0; i < set->count; i++)
        if (set->strings[i] && set->strings[i]->length == len) count++;
    return count;
}

void pfs_print(const PrefixFreeSet* set) {
    if (!set) { printf("(null)\n"); return; }
    printf("PrefixFreeSet [%zu strings, Kraft=%.6f]:\n",
           set->count, pfs_kraft_sum(set));
    for (size_t i = 0; i < set->count; i++) {
        printf("  [%zu] ", i);
        bs_print(set->strings[i]);
    }
}

void pfs_sort_lex(PrefixFreeSet* set) {
    if (!set || set->count <= 1) return;
    /* Insertion sort by lexicographic order */
    for (size_t i = 1; i < set->count; i++) {
        BitString* key = set->strings[i];
        int64_t j = (int64_t)i - 1;
        while (j >= 0 && bs_compare_lex(set->strings[j], key) > 0) {
            set->strings[j + 1] = set->strings[j];
            j--;
        }
        set->strings[j + 1] = key;
    }
}

/* ── Kraft Sum from Lengths ──────────────────────────────── */
/* Knowledge point L4: Kraft inequality. Given codeword lengths
 * l₁, ..., lₙ, there exists a prefix-free set with these lengths
 * iff Σ 2^{-l_i} ≤ 1. */

double kraft_sum_from_lengths(const size_t* lengths, size_t count) {
    if (!lengths || count == 0) return 0.0;
    double sum = 0.0;
    for (size_t i = 0; i < count; i++)
        sum += pow(0.5, (double)lengths[i]);
    return sum;
}

/* ── Construct Prefix-Free Set from Lengths ──────────────── */
/* Knowledge point L5: Kraft-McMillan construction. Given lengths
 * satisfying Kraft ≤ 1, build a concrete prefix-free set using
 * the binary tree method. */

PrefixFreeSet* construct_prefix_free_set(const size_t* lengths, size_t count) {
    if (!lengths || count == 0) return NULL;
    /* Verify Kraft inequality */
    if (kraft_sum_from_lengths(lengths, count) > 1.0 + 1e-10) return NULL;
    PrefixFreeSet* set = pfs_create(count);
    if (!set) return NULL;
    /* Sort lengths for greedy construction */
    size_t* sorted_idx = (size_t*)malloc(count * sizeof(size_t));
    if (!sorted_idx) { pfs_free(set); return NULL; }
    for (size_t i = 0; i < count; i++) sorted_idx[i] = i;
    /* Simple bubble sort by length */
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (lengths[sorted_idx[i]] > lengths[sorted_idx[j]]) {
                size_t tmp = sorted_idx[i];
                sorted_idx[i] = sorted_idx[j];
                sorted_idx[j] = tmp;
            }
        }
    }
    /* Greedy codeword assignment: maintain set of "used" prefixes */
    PrefixFreeSet* used = pfs_create(count * 2);
    if (!used) { free(sorted_idx); pfs_free(set); return NULL; }
    for (size_t i = 0; i < count; i++) {
        size_t len = lengths[sorted_idx[i]];
        /* Find an unused codeword of this length */
        BitString* cw = bs_create(NULL, len);
        if (!cw) break;
        /* Try all bit patterns of length len */
        uint64_t max_pat = 1ULL << len;
        int found = 0;
        for (uint64_t pat = 0; pat < max_pat; pat++) {
            for (size_t b = 0; b < len; b++)
                bs_set_bit(cw, b, (int)((pat >> b) & 1));
            cw->length = len;
            /* Check if any used string is a prefix of cw, or vice versa */
            int conflict = 0;
            for (size_t u = 0; u < used->count; u++) {
                if (bs_is_prefix(used->strings[u], cw) ||
                    bs_is_prefix(cw, used->strings[u])) {
                    conflict = 1; break;
                }
            }
            if (!conflict) { found = 1; break; }
        }
        if (found) {
            pfs_add(used, cw);
            pfs_add(set, cw);
        }
        bs_free(cw);
    }
    pfs_free(used);
    free(sorted_idx);
    return set;
}

/* ── Prefix Tree Operations ──────────────────────────────── */
/* Knowledge point L3: Binary tree representation of prefix codes.
 * Each codeword corresponds to a leaf in a binary tree. Internal
 * nodes can be terminals (if code is not maximal). */

PrefixTreeNode* ptree_create(void) {
    PrefixTreeNode* node = (PrefixTreeNode*)calloc(1, sizeof(PrefixTreeNode));
    return node;
}

void ptree_free(PrefixTreeNode* root) {
    if (!root) return;
    ptree_free(root->child[0]);
    ptree_free(root->child[1]);
    free(root);
}

int ptree_insert(PrefixTreeNode* root, const BitString* s) {
    if (!root || !s) return -1;
    PrefixTreeNode* cur = root;
    for (size_t i = 0; i < s->length; i++) {
        int bit = bs_get_bit(s, i);
        if (bit < 0 || bit > 1) return -1;
        if (!cur->child[bit]) {
            cur->child[bit] = ptree_create();
            if (!cur->child[bit]) return -1;
        }
        cur = cur->child[bit];
        /* If we pass through a terminal, this violates prefix-free */
        if (cur->is_terminal && i < s->length - 1) return -1;
    }
    /* If node already has children, adding terminal here is invalid */
    if (cur->child[0] || cur->child[1]) return -1;
    cur->is_terminal = 1;
    return 0;
}

int ptree_lookup(const PrefixTreeNode* root, const BitString* s) {
    if (!root || !s) return 0;
    const PrefixTreeNode* cur = root;
    for (size_t i = 0; i < s->length; i++) {
        int bit = bs_get_bit(s, i);
        if (bit < 0 || bit > 1) return 0;
        if (!cur->child[bit]) return 0;
        cur = cur->child[bit];
    }
    return cur->is_terminal;
}

int ptree_is_prefix_free(PrefixTreeNode* root) {
    if (!root) return 1;
    /* If internal node is terminal, its children must not exist */
    if (root->is_terminal && (root->child[0] || root->child[1]))
        return 0;
    /* Recurse */
    int left_ok = root->child[0] ? ptree_is_prefix_free(root->child[0]) : 1;
    int right_ok = root->child[1] ? ptree_is_prefix_free(root->child[1]) : 1;
    return left_ok && right_ok;
}

size_t ptree_kraft_sum_helper(PrefixTreeNode* node, size_t depth);

size_t ptree_kraft_sum(PrefixTreeNode* root, size_t depth) {
    if (!root) return 0;
    /* We return an encoded integer: the sum of 2^{max_depth - depth}
     * at each terminal node. This avoids floating point. */
    return ptree_kraft_sum_helper(root, depth);
}

__attribute__((unused))
static size_t ptree_max_depth(PrefixTreeNode* node, size_t cur_depth) {
    if (!node) return cur_depth;
    size_t max_d = cur_depth;
    if (node->child[0]) {
        size_t d0 = ptree_max_depth(node->child[0], cur_depth + 1);
        if (d0 > max_d) max_d = d0;
    }
    if (node->child[1]) {
        size_t d1 = ptree_max_depth(node->child[1], cur_depth + 1);
        if (d1 > max_d) max_d = d1;
    }
    if (node->is_terminal && cur_depth > max_d) max_d = cur_depth;
    return max_d;
}

size_t ptree_kraft_sum_helper(PrefixTreeNode* node, size_t depth) {
    if (!node) return 0;
    if (node->is_terminal) {
        /* Return the numerator contribution assuming denominator 2^max_depth */
        return (size_t)1;
    }
    size_t sum = 0;
    if (node->child[0]) sum += ptree_kraft_sum_helper(node->child[0], depth + 1);
    if (node->child[1]) sum += ptree_kraft_sum_helper(node->child[1], depth + 1);
    return sum;
}
