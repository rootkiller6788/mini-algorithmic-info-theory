/*
 * example_compression_demo.c — Demonstrates all compression algorithms
 * on diverse data to illustrate Kolmogorov complexity upper bounds.
 *
 * L6: Solves the canonical problem of string compression.
 * Shows how different algorithms approach the theoretical K(x) bound.
 */

#include "../include/kolmogorov.h"
#include "../include/compression.h"
#include "../include/entropy.h"
#include "../include/string_tools.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static void demo_algorithm(const char* name, const KString* input) {
    printf("\n--- %s ---\n", name);
    printf("Input (%zu bytes): ", input->len);
    kstr_print(input);
}

int main(void) {
    printf("╔══════════════════════════════════════════════╗\n");
    printf("║  Compression Algorithms Demo                 ║\n");
    printf("╚══════════════════════════════════════════════╝\n");

    /* Test 1: Highly repetitive data */
    KString* rep = kstr_from_cstr(
        "DOGDOGDOGDOGDOGDOGDOGDOGDOGDOGDOGDOGDOGDOGDOGDOG");
    demo_algorithm("Highly Repetitive", rep);

    /* LZ77 */
    LZ77State* lz77 = lz77_create(256, 16);
    lz77_compress(lz77, rep);
    printf("LZ77: %zu tokens, %zu bytes → %.2f%%\n",
           lz77->n_tokens, lz77_compressed_size(lz77),
           100.0 * (double)lz77_compressed_size(lz77) / (double)rep->len);
    KString* dec = lz77_decompress(lz77);
    printf("  Roundtrip: %s\n", kstr_equals(rep, dec) ? "✓ PASS" : "✗ FAIL");
    kstr_free(dec);
    lz77_free(lz77);

    /* LZ78 */
    LZ78State* lz78 = lz78_create();
    lz78_compress(lz78, rep);
    printf("LZ78: %zu tokens, %zu bytes → %.2f%%\n",
           lz78->n_tokens, lz78_compressed_size(lz78),
           100.0 * (double)lz78_compressed_size(lz78) / (double)rep->len);
    KString* dec78 = lz78_decompress(lz78);
    printf("  Roundtrip: %s\n", kstr_equals(rep, dec78) ? "✓ PASS" : "✗ FAIL");
    kstr_free(dec78);
    lz78_free(lz78);

    /* LZW */
    LZWState* lzw = lzw_create();
    lzw_compress(lzw, rep);
    printf("LZW: %zu codes, %zu bytes → %.2f%%\n",
           lzw->n_codes, lzw_compressed_size(lzw),
           100.0 * (double)lzw_compressed_size(lzw) / (double)rep->len);
    KString* dec_lzw = lzw_decompress(lzw);
    printf("  Roundtrip: %s\n", kstr_equals(rep, dec_lzw) ? "✓ PASS" : "✗ FAIL");
    kstr_free(dec_lzw);
    lzw_free(lzw);

    /* RLE */
    KString* rle_enc = rle_encode(rep);
    printf("RLE: %zu bytes → %.2f%%\n",
           rle_enc->len, 100.0 * (double)rle_enc->len / (double)rep->len);
    KString* rle_dec = rle_decode(rle_enc);
    printf("  Roundtrip: %s\n", kstr_equals(rep, rle_dec) ? "✓ PASS" : "✗ FAIL");
    kstr_free(rle_enc); kstr_free(rle_dec);

    /* Huffman */
    HuffmanTree* ht = huffman_build(rep);
    if (ht) {
        KString* enc = huffman_encode(ht, rep);
        printf("Huffman: %zu bytes → %.2f%%\n",
               enc->len, 100.0 * (double)enc->len / (double)rep->len);
        KString* hdec = huffman_decode(ht, enc);
        printf("  Roundtrip: %s\n", kstr_equals(rep, hdec) ? "✓ PASS" : "✗ FAIL");
        kstr_free(enc); kstr_free(hdec);
        huffman_free(ht);
    }

    /* BWT */
    KString* bwt_out = bwt_transform(rep);
    printf("BWT: transform completes, %zu bytes\n", bwt_out->len);
    kstr_free(bwt_out);
    kstr_free(rep);

    /* Test 2: Natural language */
    printf("\n\n────────────────────────────────────────\n");
    KString* text = kstr_from_cstr(
        "To be, or not to be, that is the question: "
        "Whether 'tis nobler in the mind to suffer "
        "The slings and arrows of outrageous fortune, "
        "Or to take arms against a sea of troubles...");
    demo_algorithm("Natural Language (Shakespeare)", text);

    double H_text = shannon_entropy_bits(text);
    double shannon_lb_bytes = optimal_code_length(text);
    printf("Shannon entropy: %.4f bits/char\n", H_text);
    printf("Optimal code ~%.1f bytes (lower bound)\n", shannon_lb_bytes);

    printf("Compression ratios on text:\n");
    printf("  LZ77   : %.3f\n", lz77_ratio(text));
    printf("  LZ78   : %.3f\n", lz78_ratio(text));
    printf("  LZW    : %.3f\n", lzw_ratio(text));
    printf("  Huffman: %.3f\n", huffman_ratio(text));
    kstr_free(text);

    /* Test 3: Random vs structured */
    printf("\n\n────────────────────────────────────────\n");
    unsigned int seed = 12345;
    KString* rnd = kstr_random(100, &seed);
    KString* alpha = kstr_from_cstr("ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKL"
                                      "MNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ");

    printf("Random data H = %.4f (should be near max)\n",
           shannon_entropy_bits(rnd));
    printf("  %d-incompressible: %s\n", 8,
           k_is_incompressible(rnd, 8) ? "yes" : "no");

    printf("Alphabet pattern H = %.4f\n", shannon_entropy_bits(alpha));
    printf("  %d-incompressible: %s\n", 8,
           k_is_incompressible(alpha, 8) ? "yes" : "no");

    kstr_free(rnd); kstr_free(alpha);

    /* Benchmark summary */
    printf("\n\n────────────────────────────────────────\n");
    printf("Compression Benchmark Summary:\n\n");
    KString* bench_data = kstr_from_cstr(
        "These are the times that try men's souls. The summer soldier "
        "and the sunshine patriot will, in this crisis, shrink from the "
        "service of their country; but he that stands it now, deserves "
        "the love and thanks of man and woman.");
    compress_benchmark_print(bench_data);
    kstr_free(bench_data);

    printf("\nDone.\n");
    return 0;
}
