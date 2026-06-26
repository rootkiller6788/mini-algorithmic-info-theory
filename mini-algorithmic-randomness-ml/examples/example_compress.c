/*
 * example_compress.c — Neural Compression Example
 *
 * Demonstrates neural network-based data compression, comparing
 * classical (Shannon, LZ, Huffman) vs neural approaches.
 *
 * Key insight: A good predictor → good compressor.
 * Cross-entropy of neural predictor ≈ achievable compression rate.
 *
 * Usage: ./build/example_example_compress
 * Build: make examples
 */
#include "../include/randomness.h"
#include "../include/compression_nn.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("=== Neural Compression Example ===\n\n");

    /* Create test data: text-like pattern vs random */
    const char* text = "Hello World! This is a test of neural compression. "
                       "Algorithmic information theory meets machine learning. "
                       "Martin-Lof randomness can be approximated by GANs.";
    size_t len = strlen(text);
    printf("Input text (%zu bytes): %s\n", len, text);

    /* Neural compression */
    printf("\n--- Neural Compression ---\n");
    NeuralCompressionResult* r = neural_compress(
        (const unsigned char*)text, len, 4, 8);
    neural_compression_result_print(r);

    /* Complexity upper bound */
    printf("\n--- Kolmogorov Complexity Upper Bound ---\n");
    RandomBitString* bits = rbs_from_binary(
        "0100100001100101011011000110110001101111"); /* "Hello" in ASCII binary */
    size_t bound = neural_complexity_upper_bound(bits, 4, 8);
    printf("  K(x) <= %zu bits (|x|=%zu, ratio=%.4f)\n",
           bound, bits->len,
           bits->len > 0 ? (double)bound / (double)bits->len : 0.0);

    /* Compression benchmark: compare all methods */
    printf("\n--- Compression Benchmark (all methods) ---\n");
    CompressionBenchmark* b = compression_benchmark_ml(
        (const unsigned char*)text, len);
    compression_benchmark_print(b);

    neural_compression_result_free(r);
    compression_benchmark_free(b);
    rbs_free(bits);

    printf("\n=== Example Complete ===\n");
    return 0;
}
