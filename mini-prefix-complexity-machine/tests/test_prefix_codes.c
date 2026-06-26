/*
 * test_prefix_codes.c — Tests for prefix code algorithms
 *
 * L3: Kraft, McMillan
 * L5: Shannon-Fano, Huffman, Arithmetic coding
 */
#include "../include/prefix_machine.h"
#include "../include/prefix_codes.h"
#include <stdio.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; } \
    else { printf("FAIL: %s (line %d)\n", msg, __LINE__); } \
} while(0)

int main(void) {
    printf("=== test_prefix_codes ===\n");

    /* ── PrefixCode create/free ─────────────────────────── */
    PrefixCode* pc = pc_create(4);
    TEST_ASSERT(pc != NULL, "pc_create");
    TEST_ASSERT(pc->n_symbols == 4, "pc n_symbols");
    pc_free(pc);

    /* ── Kraft ──────────────────────────────────────────── */
    int lens[4] = {2, 2, 2, 2};
    TEST_ASSERT(kraft_satisfied(lens, 4) == 1, "Kraft satisfied (4x2)");
    double ks = kraft_sum(lens, 4);
    TEST_ASSERT(fabs(ks - 1.0) < 1e-9, "Kraft sum = 1.0");

    int lens_bad[3] = {1, 1, 1};
    TEST_ASSERT(kraft_satisfied(lens_bad, 3) == 0, "Kraft violated (3x1)");

    /* ── Shannon-Fano ───────────────────────────────────── */
    double probs[4] = {0.4, 0.3, 0.2, 0.1};
    int* sf_len = shannon_fano_lengths(probs, 4);
    TEST_ASSERT(sf_len != NULL, "SF lengths");
    TEST_ASSERT(kraft_satisfied(sf_len, 4) == 1, "SF Kraft satisfied");
    double SF_L = pc_expected_length(probs, sf_len, 4);
    double H = pc_shannon_entropy(probs, 4);
    TEST_ASSERT(SF_L >= H, "SF: E[L] >= H");
    TEST_ASSERT(SF_L < H + 2.0, "SF: E[L] < H + 2");
    free(sf_len);

    PrefixCode* sf_code = shannon_fano_code(probs, 4);
    TEST_ASSERT(sf_code != NULL, "SF code non-null");
    TEST_ASSERT(pc_is_valid(sf_code) == 1, "SF code valid");
    pc_free(sf_code);

    /* ── Huffman ────────────────────────────────────────── */
    int* huf_len = huffman_lengths_compute(probs, 4);
    TEST_ASSERT(huf_len != NULL, "Huffman lengths");
    TEST_ASSERT(kraft_satisfied(huf_len, 4) == 1, "Huffman Kraft satisfied");
    double Huf_L = pc_expected_length(probs, huf_len, 4);
    TEST_ASSERT(Huf_L >= H, "Huffman: E[L] >= H");
    TEST_ASSERT(Huf_L < H + 1.0, "Huffman: E[L] < H + 1");
    double redundancy = huffman_redundancy(probs, huf_len, 4);
    TEST_ASSERT(redundancy >= 0.0, "Huffman redundancy >= 0");
    TEST_ASSERT(redundancy < 1.0, "Huffman redundancy < 1");

    double expected_L = huffman_expected_length(probs, huf_len, 4);
    TEST_ASSERT(fabs(expected_L - Huf_L) < 1e-9, "expected_length consistent");
    free(huf_len);

    PrefixCode* huf_code = huffman_code_compute(probs, 4);
    TEST_ASSERT(huf_code != NULL, "Huffman code non-null");
    TEST_ASSERT(pc_is_valid(huf_code) == 1, "Huffman code valid");

    /* ── Encode/Decode ──────────────────────────────────── */
    char out_buf[64];
    int len0 = pc_encode(huf_code, 0, out_buf, sizeof(out_buf));
    TEST_ASSERT(len0 > 0, "encode symbol 0");

    int dec_sym = -1, dec_consumed = 0;
    int r = pc_decode(huf_code, out_buf, len0, &dec_sym, &dec_consumed);
    TEST_ASSERT(r == 0, "decode success");
    TEST_ASSERT(dec_sym == 0, "decode correct symbol");
    TEST_ASSERT(dec_consumed == len0, "decode consumed bits");
    pc_free(huf_code);

    /* ── McMillan ───────────────────────────────────────── */
    TEST_ASSERT(pc_mcmillan_check(lens, 4) == 1, "McMillan satisfied");
    TEST_ASSERT(pc_mcmillan_check(lens_bad, 3) == 0, "McMillan violated");

    /* ── Kraft Construction ─────────────────────────────── */
    int klens[3] = {2, 2, 3};
    PrefixCode* kc = pc_kraft_construct(klens, 3);
    TEST_ASSERT(kc != NULL, "Kraft construct");
    TEST_ASSERT(pc_is_valid(kc) == 1, "Kraft construct valid");
    pc_free(kc);

    /* ── pc_from_lengths ────────────────────────────────── */
    PrefixCode* pfl = pc_from_lengths(klens, 3);
    TEST_ASSERT(pfl != NULL, "pc_from_lengths");
    TEST_ASSERT(pc_is_valid(pfl) == 1, "pc_from_lengths valid");
    for (int i = 0; i < 3; i++)
        TEST_ASSERT(pfl->lengths[i] == (size_t)klens[i], "lengths match");
    pc_free(pfl);

    /* ── Golomb ─────────────────────────────────────────── */
    int* golomb = pc_golomb_lengths(20, 4);
    TEST_ASSERT(golomb != NULL, "Golomb lengths");
    { double ks2 = 0.0; for (int i = 0; i < 20; i++) ks2 += pow(2.0, -(double)golomb[i]);
      TEST_ASSERT(ks2 <= 1.0 + 1e-9, "Golomb Kraft"); }
    free(golomb);

    /* ── Rice ───────────────────────────────────────────── */
    int* rice = pc_rice_lengths(20, 2);
    TEST_ASSERT(rice != NULL, "Rice lengths");
    free(rice);

    size_t rl = rice_code_length(42, 3);
    TEST_ASSERT(rl > 0, "Rice code length > 0");

    /* ── Universal Codes ────────────────────────────────── */
    for (size_t n = 0; n <= 100; n++) {
        size_t dl = elias_delta_length(n);
        size_t gl = elias_gamma_length(n);
        size_t ol = elias_omega_length(n);
        size_t ll = levenshtein_code_length(n);
        TEST_ASSERT(dl > 0, "delta length > 0");
        TEST_ASSERT(gl > 0, "gamma length > 0");
        TEST_ASSERT(ol > 0, "omega length > 0");
        TEST_ASSERT(ll > 0, "levenshtein length > 0");
    }

    /* ── Tunstall ───────────────────────────────────────── */
    TunstallLeaf* tl = pc_tunstall_build(probs, 2, 4);
    TEST_ASSERT(tl != NULL, "Tunstall build");
    pc_tunstall_free(tl, 4);

    /* ── Block Coding ───────────────────────────────────── */
    BlockCodeResult* bcr = pc_block_code_analyze(probs, 4, 2);
    TEST_ASSERT(bcr != NULL, "Block code analyze");
    TEST_ASSERT(bcr->n_blocks == 16, "Block count = 4^2");
    TEST_ASSERT(bcr->bits_per_symbol > 0.0, "bits per symbol > 0");
    pc_block_code_free(bcr);

    /* ── Arithmetic Coding ──────────────────────────────── */
    ArithmeticEncoder* ae = arith_create();
    TEST_ASSERT(ae != NULL, "arith_create");
    double cdf[5] = {0.4, 0.7, 0.9, 1.0, 1.0};
    arith_encode_symbol(ae, 1, cdf, 4);
    PMString* arith_out = pmstr_create(64);
    arith_flush(ae, arith_out);
    pmstr_free(arith_out);
    unsigned int val = 0;
    int sym = arith_decode_symbol(&val, cdf, 4);
    TEST_ASSERT(sym >= 0 && sym < 4, "arith decode valid");
    arith_free(ae);

    /* ── Fano Bound ─────────────────────────────────────── */
    double fano = pc_fano_bound(H, 4);
    TEST_ASSERT(fano == H + 1.0, "Fano bound = H+1");

    printf("\n=== Results: %d / %d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
