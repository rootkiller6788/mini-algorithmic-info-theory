/*
 * bench_compression.c — Performance benchmarks for compression algorithms
 * and Kolmogorov complexity estimation.
 */

#include "../include/kolmogorov.h"
#include "../include/compression.h"
#include "../include/entropy.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double time_diff_sec(clock_t start, clock_t end) {
    return (double)(end - start) / (double)CLOCKS_PER_SEC;
}

int main(void) {
    printf("╔══════════════════════════════════════════╗\n");
    printf("║  Compression Algorithm Benchmarks        ║\n");
    printf("╚══════════════════════════════════════════╝\n\n");

    /* Generate test data */
    size_t N = 50000;
    KString* data = kstr_create(N);
    unsigned int seed = 12345;
    for (size_t i = 0; i < N; i++) {
        seed = seed * 1103515245U + 12345U;
        kstr_append(data, (unsigned char)((seed >> 16) & 0xFF));
    }
    printf("Test data: %zu bytes\n", N);

    /* LZ77 benchmark */
    clock_t t1 = clock();
    LZ77State* lz77 = lz77_create(4096, 18);
    lz77_compress(lz77, data);
    clock_t t2 = clock();
    printf("LZ77:  %zu tokens, %zu bytes, %.2f ms\n",
           lz77->n_tokens, lz77_compressed_size(lz77),
           time_diff_sec(t1, t2) * 1000.0);
    KString* dec = lz77_decompress(lz77);
    printf("  Verify: %s\n", kstr_equals(data, dec) ? "PASS" : "FAIL");
    kstr_free(dec);
    lz77_free(lz77);

    /* LZ78 benchmark */
    t1 = clock();
    LZ78State* lz78 = lz78_create();
    lz78_compress(lz78, data);
    t2 = clock();
    printf("LZ78:  %zu tokens, %zu bytes, %.2f ms\n",
           lz78->n_tokens, lz78_compressed_size(lz78),
           time_diff_sec(t1, t2) * 1000.0);
    KString* dec78 = lz78_decompress(lz78);
    printf("  Verify: %s\n", kstr_equals(data, dec78) ? "PASS" : "FAIL");
    kstr_free(dec78);
    lz78_free(lz78);

    /* LZW benchmark */
    t1 = clock();
    LZWState* lzw = lzw_create();
    lzw_compress(lzw, data);
    t2 = clock();
    printf("LZW:   %zu codes, %zu bytes, %.2f ms\n",
           lzw->n_codes, lzw_compressed_size(lzw),
           time_diff_sec(t1, t2) * 1000.0);
    KString* dec_lzw = lzw_decompress(lzw);
    printf("  Verify: %s\n", kstr_equals(data, dec_lzw) ? "PASS" : "FAIL");
    kstr_free(dec_lzw);
    lzw_free(lzw);

    /* RLE benchmark */
    t1 = clock();
    KString* rle = rle_encode(data);
    t2 = clock();
    printf("RLE:   %zu bytes → %zu, %.2f ms\n",
           data->len, rle->len, time_diff_sec(t1, t2) * 1000.0);
    kstr_free(rle);

    /* Huffman benchmark */
    t1 = clock();
    HuffmanTree* ht = huffman_build(data);
    t2 = clock();
    printf("Huffman build: %.2f ms\n", time_diff_sec(t1, t2) * 1000.0);
    if (ht) {
        clock_t t3 = clock();
        KString* enc = huffman_encode(ht, data);
        clock_t t4 = clock();
        printf("Huffman encode: %.2f ms, %zu bytes → %zu\n",
               time_diff_sec(t3, t4) * 1000.0, data->len, enc->len);
        KString* hdec = huffman_decode(ht, enc);
        printf("  Verify: %s\n", kstr_equals(data, hdec) ? "PASS" : "FAIL");
        kstr_free(enc); kstr_free(hdec);
        huffman_free(ht);
    }

    /* Entropy benchmark */
    t1 = clock();
    double H = shannon_entropy_bits(data);
    t2 = clock();
    printf("\nShannon entropy: %.4f bits, %.2f ms\n",
           H, time_diff_sec(t1, t2) * 1000.0);

    /* Incompressibility */
    t1 = clock();
    int incomp = k_is_incompressible(data, 32);
    t2 = clock();
    printf("Incompressibility test: %s, %.2f ms\n",
           incomp ? "yes" : "no", time_diff_sec(t1, t2) * 1000.0);

    kstr_free(data);
    return 0;
}
