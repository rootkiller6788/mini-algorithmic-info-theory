/*
 * incompressibility.c — Core Incompressibility Method Implementation
 *
 * Implements: string incompressibility testing, proof construction,
 * counterexample analysis, Kolmogorov bounds, encoding/decoding,
 * and the proof generation engine.
 *
 * The incompressibility method (Li & Vitányi §6) is a proof technique
 * using Kolmogorov complexity to establish combinatorial and
 * computational lower bounds via contradiction.
 *
 * Reference: Li & Vitányi "An Introduction to Kolmogorov Complexity
 *   and its Applications" (4th ed, 2019), Chapter 6.
 * Companion: Sipser §6.4, Arora-Barak §6.8
 */

#include "../include/incompressibility.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>

/* ══════════════════════════════════════════════════════════════
   Utility: integer log2 and byte-level Shannon entropy
   ══════════════════════════════════════════════════════════════ */

static size_t ilog2(size_t n) {
    size_t r = 0;
    while (n > 1) { n >>= 1; r++; }
    return r;
}

static double entropy_byte(const unsigned char* data, size_t len) {
    int freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[data[i]]++;
    double H = 0.0;
    for (int i = 0; i < 256; i++) {
        if (freq[i] > 0) {
            double p = (double)freq[i] / (double)len;
            H -= p * log2(p);
        }
    }
    return H;
}

/*
 * LZ-like compression ratio: estimate compressibility by
 * finding repeated substrings. Returns fraction of original
 * size (1.0 = incompressible, <1.0 = compressible).
 */
static double lz_estimate_ratio(const unsigned char* data, size_t len) {
    if (len < 4) return 1.0;
    size_t saved = 0;
    size_t window = len < 256 ? len : 256;
    for (size_t i = 0; i < len; ) {
        size_t best_len = 0;
        for (size_t j = (i >= window) ? i - window : 0; j < i; j++) {
            size_t k = 0;
            while (i + k < len && data[j + k] == data[i + k]) k++;
            if (k > best_len) best_len = k;
        }
        if (best_len >= 3) {
            saved += best_len - 2;
            i += best_len;
        } else {
            i++;
        }
    }
    if (saved >= len) return 0.05;
    return 1.0 - (double)saved / (double)len;
}

/* ══════════════════════════════════════════════════════════════
   L2: IncompressibleString API
   ══════════════════════════════════════════════════════════════ */

IncompressibleString* incstr_create(const unsigned char* data, size_t len, int c) {
    IncompressibleString* s = (IncompressibleString*)malloc(sizeof(IncompressibleString));
    assert(s != NULL);
    s->data = (unsigned char*)malloc(len + 1);
    assert(s->data != NULL);
    memcpy(s->data, data, len);
    s->data[len] = '\0';
    s->len = len;
    s->incomp_c = c;
    s->estimated_K = incstr_estimate_K(data, len);
    s->is_incompressible = (s->estimated_K + (size_t)c >= len * 8) ? 1 : 0;
    if (len == 0) s->is_incompressible = 1;
    return s;
}

void incstr_free(IncompressibleString* s) {
    if (s) { free(s->data); free(s); }
}

size_t incstr_estimate_K(const unsigned char* data, size_t len) {
    /*
     * Estimate K(x) using heuristics:
     *   K(x) <= |x| + O(1)       (trivial upper bound)
     *   K(x) ~= H(x) * |x| / 8   (entropy heuristic, in bytes)
     *   K(x) ~= lz_ratio * |x|   (compression heuristic)
     *
     * We return the minimum of these estimates (as bits).
     */
    if (len == 0) return 0;
    double H = entropy_byte(data, len);
    double lz = lz_estimate_ratio(data, len);

    size_t trivial_bits = len * 8 + 32;
    size_t entropy_bits = (size_t)ceil(H * (double)len) + 16;
    size_t lz_bits = (size_t)(lz * (double)len * 8.0) + 16;

    size_t est = trivial_bits;
    if (entropy_bits < est) est = entropy_bits;
    if (lz_bits < est) est = lz_bits;
    return est;
}

int incstr_is_c_incompressible(const unsigned char* data, size_t len, int c) {
    /*
     * Definition: x is c-incompressible if K(x) >= |x| - c.
     *
     * Since K is incomputable (Chaitin's theorem), we estimate.
     * We test with multiple heuristics; if NONE compresses by >= c bits,
     * the string is declared c-incompressible.
     */
    if (len == 0) return 1;
    size_t est_K = incstr_estimate_K(data, len);
    size_t bit_len = len * 8;
    return (est_K + (size_t)c >= bit_len) ? 1 : 0;
}

void incstr_print_report(const IncompressibleString* s) {
    if (!s) { printf("(null)\n"); return; }
    printf("IncompressibleString report:\n");
    printf("  Length: %zu bytes (%zu bits)\n", s->len, s->len * 8);
    printf("  Estimated K(x): %zu bits\n", s->estimated_K);
    printf("  Incompressibility c: %d\n", s->incomp_c);
    printf("  c-incompressible: %s\n", s->is_incompressible ? "YES" : "NO");
    double ratio = s->len > 0 ? (double)s->estimated_K / (double)(s->len * 8) : 1.0;
    printf("  K/|x| ratio: %.4f\n", ratio);
    if (s->len <= 64) {
        printf("  Data: ");
        for (size_t i = 0; i < s->len; i++)
            printf("%c", s->data[i] >= 32 && s->data[i] < 127 ? s->data[i] : '.');
        printf("\n");
    }
}

/* ══════════════════════════════════════════════════════════════
   L2: IncompressibilityProof API
   ══════════════════════════════════════════════════════════════ */

IncompressibilityProof* incproof_create(const char* theorem_name,
                                         const char* property) {
    IncompressibilityProof* p = (IncompressibilityProof*)malloc(sizeof(IncompressibilityProof));
    assert(p != NULL);
    p->theorem_name = theorem_name ? strdup(theorem_name) : strdup("Unnamed");
    p->property = property ? strdup(property) : strdup("Unspecified");
    p->capacity = 16;
    p->n_steps = 0;
    p->steps = (ProofStep*)malloc((size_t)p->capacity * sizeof(ProofStep));
    assert(p->steps != NULL);
    memset(p->steps, 0, (size_t)p->capacity * sizeof(ProofStep));
    p->constant_overhead = 32;
    p->is_valid = 0;
    return p;
}

void incproof_add_step(IncompressibilityProof* proof,
                        ProofStepType type,
                        const char* desc,
                        size_t desc_length,
                        double bound) {
    assert(proof != NULL);
    if (proof->n_steps >= proof->capacity) {
        proof->capacity *= 2;
        proof->steps = (ProofStep*)realloc(proof->steps,
                            (size_t)proof->capacity * sizeof(ProofStep));
        assert(proof->steps != NULL);
    }
    ProofStep* s = &proof->steps[proof->n_steps++];
    s->type = type;
    strncpy(s->description, desc, 255);
    s->description[255] = '\0';
    s->description_length = desc_length;
    s->bound = bound;
}

int incproof_validate(IncompressibilityProof* proof) {
    /*
     * A valid incompressibility proof requires 5 steps:
     *   1. ASSUME     — assume there exists a counterexample
     *   2. ENCODE     — construct a short encoding using the violation
     *   3. BOUND      — derive a K(x) upper bound from the encoding
     *   4. CONTRADICT — contradiction with c-incompressibility
     *   5. CONCLUDE   — conclude that the property holds
     */
    if (!proof || proof->n_steps < 5) return 0;
    int has_assume = 0, has_encode = 0, has_bound = 0;
    int has_contradict = 0, has_conclude = 0;
    for (int i = 0; i < proof->n_steps; i++) {
        switch (proof->steps[i].type) {
            case PROOF_STEP_ASSUME:     has_assume = 1;     break;
            case PROOF_STEP_ENCODE:     has_encode = 1;     break;
            case PROOF_STEP_BOUND:      has_bound = 1;      break;
            case PROOF_STEP_CONTRADICT: has_contradict = 1; break;
            case PROOF_STEP_CONCLUDE:   has_conclude = 1;   break;
        }
    }
    proof->is_valid = (has_assume && has_encode && has_bound
                       && has_contradict && has_conclude);
    return proof->is_valid;
}

void incproof_print(const IncompressibilityProof* proof) {
    if (!proof) return;
    printf("========================================\n");
    printf("Incompressibility Proof\n");
    printf("========================================\n");
    printf("Theorem: %s\n", proof->theorem_name);
    printf("Property: %s\n", proof->property);
    printf("Validity: %s\n", proof->is_valid ? "VALID" : "INVALID");
    printf("Constant overhead: %zu bits\n", proof->constant_overhead);
    printf("Steps: %d\n", proof->n_steps);
    printf("----------------------------------------\n");
    const char* type_names[] = {"ASSUME","ENCODE","BOUND","CONTRADICT","CONCLUDE"};
    for (int i = 0; i < proof->n_steps; i++) {
        printf("  [%d] %-12s | len=%3zu | bound=%7.1f | %s\n",
               i + 1, type_names[proof->steps[i].type],
               proof->steps[i].description_length,
               proof->steps[i].bound,
               proof->steps[i].description);
    }
    printf("========================================\n");
}

void incproof_free(IncompressibilityProof* proof) {
    if (proof) {
        free(proof->theorem_name);
        free(proof->property);
        free(proof->steps);
        free(proof);
    }
}

/* ══════════════════════════════════════════════════════════════
   L2: CounterExample API
   ══════════════════════════════════════════════════════════════ */

CounterExample* cex_create(void* object, size_t size, const char* type) {
    CounterExample* c = (CounterExample*)malloc(sizeof(CounterExample));
    assert(c != NULL);
    c->object = malloc(size);
    assert(c->object != NULL);
    memcpy(c->object, object, size);
    c->object_size = size;
    c->object_type = type ? strdup(type) : strdup("unknown");
    c->kolmogorov_bound = incstr_estimate_K((const unsigned char*)object, size);
    c->encoding_length = size * 8 + 32;
    c->is_genuine_counterexample = 0;
    return c;
}

size_t cex_encoding_length(CounterExample* cex, ProofStrategy strategy) {
    /*
     * Compute encoding length needed to describe the counterexample
     * assuming the property violation is exploited for compression.
     * Different strategies yield different savings.
     */
    if (!cex) return 0;
    size_t base_bits = cex->object_size * 8;
    switch (strategy) {
        case PROOF_PIGEONHOLE:
            return base_bits - ilog2(cex->object_size) + 16;
        case PROOF_COUNTING:
            return base_bits - ilog2(base_bits) + 32;
        case PROOF_ENCODING:
            return base_bits / 2 + ilog2(cex->object_size) + 32;
        case PROOF_DIAGONALIZATION:
            return base_bits + 64;
        case PROOF_AVERAGE_CASE:
            return base_bits - ilog2(cex->object_size) * 2 + 32;
        case PROOF_EXPECTATION:
            return base_bits + 48;
        case PROOF_COMPRESSION:
            return (size_t)((double)base_bits * lz_estimate_ratio(
                (const unsigned char*)cex->object, cex->object_size)) + 16;
        case PROOF_RECURSIVE:
            return base_bits / 4 + 64;
        default:
            return base_bits + 32;
    }
}

int cex_is_genuine(CounterExample* cex, int incompressibility_c) {
    /*
     * A counterexample is genuine iff the shortest encoding found
     * via ANY strategy is still >= |x| - c (the incompressibility bound).
     * If some strategy gives a shorter encoding, the counterexample
     * violates incompressibility and cannot exist — hence no genuine
     * counterexample exists.
     */
    if (!cex) return 0;
    size_t min_encoding = cex->object_size * 8 + 256;
    for (int s = PROOF_PIGEONHOLE; s <= PROOF_RECURSIVE; s++) {
        size_t el = cex_encoding_length(cex, (ProofStrategy)s);
        if (el < min_encoding) min_encoding = el;
    }
    size_t incomp_bound = cex->object_size * 8 - (size_t)incompressibility_c;
    cex->is_genuine_counterexample = (min_encoding >= incomp_bound) ? 1 : 0;
    return cex->is_genuine_counterexample;
}

void cex_free(CounterExample* cex) {
    if (cex) { free(cex->object); free(cex->object_type); free(cex); }
}

/* ══════════════════════════════════════════════════════════════
   L2: KolmogorovBound API
   ══════════════════════════════════════════════════════════════ */

KolmogorovBound* kb_create(size_t lower, size_t upper, size_t constant) {
    KolmogorovBound* b = (KolmogorovBound*)malloc(sizeof(KolmogorovBound));
    assert(b != NULL);
    b->lower_bound = lower;
    b->upper_bound = upper;
    b->constant_term = constant;
    b->is_tight = (lower == upper) ? 1 : 0;
    return b;
}

int kb_is_contradiction(const KolmogorovBound* b) {
    if (!b) return 0;
    return (b->lower_bound > b->upper_bound) ? 1 : 0;
}

int64_t kb_gap(const KolmogorovBound* b) {
    if (!b) return 0;
    return (int64_t)b->lower_bound - (int64_t)b->upper_bound;
}

void kb_free(KolmogorovBound* b) { free(b); }

/* ══════════════════════════════════════════════════════════════
   L3: Pigeonhole Principle via Incompressibility
   ══════════════════════════════════════════════════════════════ */

int inc_pigeonhole_proof(int n_items, int n_boxes) {
    /*
     * Classical pigeonhole: if n_items > n_boxes, some box has >= 2 items.
     *
     * Incompressibility proof (Li-Vitanyi §6.1):
     *   Assume no box has >= 2 items. Then all items are in distinct boxes.
     *   Each of the n_items items can be described by its box index.
     *   That needs log(n_boxes) bits per item.
     *   But this would give K(item) <= log(n_boxes) + O(1), contradicting
     *   the fact that most items of length > log(n_boxes) are incompressible.
     */
    if (n_items <= n_boxes) return 0;

    IncompressibilityProof* proof = incproof_create(
        "Pigeonhole Principle",
        "If n+1 items placed in n boxes, some box has >= 2 items");

    incproof_add_step(proof, PROOF_STEP_ASSUME,
        "Assume all boxes contain at most 1 item",
        (size_t)n_items * (size_t)ilog2((size_t)n_boxes), 0.0);

    size_t bits_per = ilog2((size_t)n_boxes) + 1;
    incproof_add_step(proof, PROOF_STEP_ENCODE,
        "Encode each item by its box index: log(boxes) + 1 bits",
        (size_t)n_items * bits_per + 32, 0.0);

    incproof_add_step(proof, PROOF_STEP_BOUND,
        "K(each item) <= log(n_boxes) + O(1)",
        bits_per, (double)bits_per);

    incproof_add_step(proof, PROOF_STEP_CONTRADICT,
        "But incompressible items require K(x) >= |x| - c; contradiction for long items",
        0, 0.0);

    incproof_add_step(proof, PROOF_STEP_CONCLUDE,
        "Pigeonhole principle holds: some box must have >= 2 items. QED.",
        0, 0.0);

    incproof_validate(proof);
    incproof_free(proof);
    return 1;
}

int inc_counting_bound(size_t universe_size, size_t property_count,
                        int incompressibility_c) {
    /*
     * Counting bound: if at most m out of N objects have property P,
     * then an incompressible object avoids P with high probability.
     *
     * An object with P can be encoded by its index among P-objects,
     * saving log(N/m) bits. If this saving exceeds the incompressibility
     * constant c, then no c-incompressible object can have P.
     */
    if (universe_size == 0 || property_count == 0) return 0;
    double fraction = (double)property_count / (double)universe_size;
    double savings = -log2(fraction);
    return (savings > (double)incompressibility_c) ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════
   L3: Encoding / Decoding with Property
   ══════════════════════════════════════════════════════════════ */

size_t inc_encode_with_property(const unsigned char* x, size_t len,
                                 int property_holds,
                                 unsigned char** encoded_out) {
    /*
     * If property P holds for x:  prefix '0' + x.  Length: |x| + 1.
     * If property P fails for x: prefix '1' + compressed form.
     *   Compressed form exploits the rarity of violation to save bits.
     */
    size_t out_len;
    if (property_holds) {
        out_len = len + 1;
        *encoded_out = (unsigned char*)malloc(out_len);
        assert(*encoded_out != NULL);
        (*encoded_out)[0] = '0';
        memcpy(*encoded_out + 1, x, len);
    } else {
        /* Violation: encode the "surprising" part */
        out_len = (len < 8) ? len + 2 : len / 2 + 2;
        *encoded_out = (unsigned char*)malloc(out_len);
        assert(*encoded_out != NULL);
        (*encoded_out)[0] = '1';
        size_t copy_len = out_len - 2;
        memcpy(*encoded_out + 1, x, copy_len);
        (*encoded_out)[out_len - 1] = 0xFF; /* marker */
    }
    return out_len;
}

int inc_decode_from_property(const unsigned char* encoded, size_t enc_len,
                              unsigned char** decoded_out, size_t* dec_len) {
    if (!encoded || enc_len < 2) return 0;
    if (encoded[0] == '0') {
        /* Property holds: direct copy */
        *dec_len = enc_len - 1;
        *decoded_out = (unsigned char*)malloc(*dec_len);
        assert(*decoded_out != NULL);
        memcpy(*decoded_out, encoded + 1, *dec_len);
        return 1;
    } else if (encoded[0] == '1') {
        /* Property fails: lossy reconstruction */
        *dec_len = (enc_len - 2) * 2;
        if (*dec_len == 0) *dec_len = 1;
        *decoded_out = (unsigned char*)calloc(*dec_len, 1);
        assert(*decoded_out != NULL);
        size_t copy_len = enc_len - 2;
        memcpy(*decoded_out, encoded + 1, copy_len);
        return 1;
    }
    return 0;
}

/* ══════════════════════════════════════════════════════════════
   L3: Self-Delimiting Encodings for Prefix-Free Sets
   ══════════════════════════════════════════════════════════════ */

size_t inc_self_delimiting_encode(const unsigned char* x, size_t len,
                                   unsigned char** out) {
    /*
     * Construct a self-delimiting code:
     *   '0' -> '00', '1' -> '11', terminator -> '01'.
     * Length: 2*|x|*8 + 2 bits.
     * This code is prefix-free: satisfies Kraft inequality sum 2^{-|c(x)|} <= 1.
     */
    size_t bit_len = 2 * len * 8 + 2;
    size_t byte_len = (bit_len + 7) / 8;
    *out = (unsigned char*)calloc(byte_len, 1);
    assert(*out != NULL);

    size_t bit_pos = 0;
    for (size_t i = 0; i < len; i++) {
        for (int b = 7; b >= 0; b--) {
            int bit = (x[i] >> b) & 1;
            size_t byte_idx = bit_pos / 8;
            int bit_off = 7 - (int)(bit_pos % 8);
            (*out)[byte_idx] |= (unsigned char)(bit << bit_off);
            bit_pos++;
            byte_idx = bit_pos / 8;
            bit_off = 7 - (int)(bit_pos % 8);
            (*out)[byte_idx] |= (unsigned char)(bit << bit_off);
            bit_pos++;
        }
    }
    /* Terminator '01' -- write 0 then 1 */
    { bit_pos++; }  /* skip 0 bit (byte already zeroed by calloc) */
    { size_t bi = bit_pos / 8; int bo = 7 - (int)(bit_pos % 8);
      (*out)[bi] |= (unsigned char)(1 << bo); bit_pos++; }

    return byte_len;
}

size_t inc_self_delimiting_decode(const unsigned char* enc, size_t enc_len,
                                   unsigned char** out) {
    if (!enc || enc_len == 0) { *out = NULL; return 0; }
    size_t max_bits = enc_len * 8;
    size_t max_out = max_bits / 2 + 1;
    *out = (unsigned char*)calloc(max_out, 1);
    assert(*out != NULL);

    size_t bit_pos = 0, out_byte = 0;
    int out_bit = 7;

    while (bit_pos + 1 < max_bits) {
        int b0 = (int)(enc[bit_pos / 8] >> (7 - (int)(bit_pos % 8))) & 1;
        bit_pos++;
        if (bit_pos >= max_bits) break;
        int b1 = (int)(enc[bit_pos / 8] >> (7 - (int)(bit_pos % 8))) & 1;
        bit_pos++;

        if (b0 == 0 && b1 == 1) break; /* terminator */
        if (b0 == 0 && b1 == 0) {
            if (out_bit < 0) { out_byte++; out_bit = 7; }
            out_bit--; /* write 0 (already zeroed) */
        } else if (b0 == 1 && b1 == 1) {
            if (out_bit < 0) { out_byte++; out_bit = 7; }
            (*out)[out_byte] |= (unsigned char)(1 << out_bit);
            out_bit--;
        }
    }
    return out_byte + (out_bit < 7 ? 1 : 0);
}

/* ══════════════════════════════════════════════════════════════
   L3: Pair (x, y) Encoding
   ══════════════════════════════════════════════════════════════ */

size_t inc_encode_pair(const unsigned char* x, size_t xlen,
                        const unsigned char* y, size_t ylen,
                        unsigned char** out) {
    /*
     * Encode pair as: [4-byte |x|] [x] [y]
     * Length: 4 + |x| + |y| bytes = |x| + |y| + O(1) (constant 4 bytes).
     */
    size_t out_len = 4 + xlen + ylen;
    *out = (unsigned char*)malloc(out_len);
    assert(*out != NULL);
    (*out)[0] = (unsigned char)((xlen >> 24) & 0xFF);
    (*out)[1] = (unsigned char)((xlen >> 16) & 0xFF);
    (*out)[2] = (unsigned char)((xlen >> 8) & 0xFF);
    (*out)[3] = (unsigned char)(xlen & 0xFF);
    memcpy(*out + 4, x, xlen);
    memcpy(*out + 4 + xlen, y, ylen);
    return out_len;
}

int inc_decode_pair(const unsigned char* enc, size_t enc_len,
                     unsigned char** x_out, size_t* x_len,
                     unsigned char** y_out, size_t* y_len) {
    if (!enc || enc_len < 4) return 0;
    *x_len = ((size_t)enc[0] << 24) | ((size_t)enc[1] << 16)
           | ((size_t)enc[2] << 8)  |  (size_t)enc[3];
    if (4 + *x_len > enc_len) return 0;
    *y_len = enc_len - 4 - *x_len;
    *x_out = (unsigned char*)malloc(*x_len);
    *y_out = (unsigned char*)malloc(*y_len);
    assert(*x_out && *y_out);
    memcpy(*x_out, enc + 4, *x_len);
    memcpy(*y_out, enc + 4 + *x_len, *y_len);
    return 1;
}

/* ══════════════════════════════════════════════════════════════
   L5: Proof Generation Engine
   ══════════════════════════════════════════════════════════════ */

IncompressibilityProof* inc_generate_pigeonhole_proof(int n) {
    IncompressibilityProof* proof = incproof_create(
        "Pigeonhole Principle (via Incompressibility)",
        "n+1 pigeons in n holes forces a collision");
    char d[256];

    snprintf(d, sizeof(d), "Assume %d pigeons in %d holes with NO collision", n+1, n);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, (size_t)n * 8, 0.0);

    size_t hole_bits = ilog2((size_t)n) + 1;
    snprintf(d, sizeof(d), "Each pigeon encoded by hole number: %zu bits (no collision => unique holes)", hole_bits);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (size_t)(n+1) * hole_bits, 0.0);

    double log_fact = n > 1 ? log2(tgamma((double)n + 2.0)) : 0.0;
    snprintf(d, sizeof(d), "K(pigeon arrangement) <= %zu*%zu = %zu bits. True entropy = log((n+1)!) = %.1f",
             (size_t)(n+1), hole_bits, (size_t)(n+1)*hole_bits, log_fact);
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)ceil(log_fact), log_fact);

    snprintf(d, sizeof(d), "For c-incompressible permutation, K >= log((n+1)!) - c. But encoding uses fewer bits: contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Therefore %d pigeons in %d holes forces a collision. QED.", n+1, n);
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

IncompressibilityProof* inc_generate_sorting_lowerbound_proof(int n) {
    IncompressibilityProof* proof = incproof_create(
        "Sorting Lower Bound",
        "Comparison-based sorting requires Omega(n log n) comparisons");
    char d[256];
    double lnfac = n > 1 ? log2(tgamma((double)n + 1.0)) : 0.0;

    snprintf(d, sizeof(d), "Assume exists sorting algorithm using T(n) comparisons for all inputs of size %d", n);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, (size_t)n * 8, 0.0);

    snprintf(d, sizeof(d), "Fix c-incompressible permutation pi of %d elements. Run algorithm, record T(n) comparison outcomes.", n);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (size_t)n * ilog2((size_t)n), 0.0);

    snprintf(d, sizeof(d), "From T(n) outcomes we can reconstruct pi. So K(pi) <= T(n) + O(1). Need T(n) >= log(n!) = %.1f", lnfac);
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)ceil(lnfac), (double)n * log2((double)n));

    snprintf(d, sizeof(d), "But pi is c-incompressible: K(pi) >= log(n!) - c. Hence T(n) >= log(n!) - c - O(1) = Omega(n log n). Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Any comparison-based sorting requires T(n) >= log(n!) - O(1) = Omega(n log n). QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

IncompressibilityProof* inc_generate_heapsort_optimality_proof(int n) {
    IncompressibilityProof* proof = incproof_create(
        "Heapsort Optimality",
        "Heapsort is asymptotically optimal among comparison sorts");
    char d[256];
    double lnfac = n > 1 ? log2(tgamma((double)n + 1.0)) : 0.0;

    snprintf(d, sizeof(d), "Assume a sorting algorithm uses o(n log n) comparisons for size %d", n);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, (size_t)n * 8, 0.0);

    snprintf(d, sizeof(d), "Sorting %d elements conveys log(%d!) = %.1f bits of information. Each comparison <= 1 bit.", n, n, lnfac);
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (size_t)ceil(lnfac), 0.0);

    double heap_comp = 2.0 * (double)n * log2((double)n);
    snprintf(d, sizeof(d), "Heapsort: <= 2n log n + O(n) comparisons = %.1f. Lower bound: n log n - O(n) = %.1f. Ratio = 2.", heap_comp, lnfac);
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)heap_comp, heap_comp);

    snprintf(d, sizeof(d), "o(n log n) algorithm violates incompressibility lower bound. Contradiction.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Heapsort achieves Theta(n log n): asymptotically optimal. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

IncompressibilityProof* inc_generate_graph_noniso_proof(int n_vertices) {
    IncompressibilityProof* proof = incproof_create(
        "Graph Non-Isomorphism Certificate",
        "GNI has polynomial-size certificates (is in co-NP)");
    char d[256];

    snprintf(d, sizeof(d), "Assume two non-isomorphic graphs G, H on %d vertices need exponentially large certificates", n_vertices);
    incproof_add_step(proof, PROOF_STEP_ASSUME, d, (size_t)(n_vertices * n_vertices), 0.0);

    snprintf(d, sizeof(d), "Take incompressible G, H. Certificate = lexicographically first pi where pi(G) != H. Can be encoded in O(n^2) bits.");
    incproof_add_step(proof, PROOF_STEP_ENCODE, d, (size_t)(n_vertices * ilog2((size_t)n_vertices)), 0.0);

    snprintf(d, sizeof(d), "The short encoding of pi contradicts incompressibility unless pi's description is O(n^2).");
    incproof_add_step(proof, PROOF_STEP_BOUND, d, (size_t)(n_vertices * n_vertices / 8), 0.0);

    snprintf(d, sizeof(d), "pi's description in O(n^2) bits means GNI has polynomial-size certificates. Contradiction to assumption.");
    incproof_add_step(proof, PROOF_STEP_CONTRADICT, d, 0, 0.0);

    snprintf(d, sizeof(d), "Graph Non-Isomorphism is in co-NP: polynomial certificates exist. QED.");
    incproof_add_step(proof, PROOF_STEP_CONCLUDE, d, 0, 0.0);

    incproof_validate(proof);
    return proof;
}

