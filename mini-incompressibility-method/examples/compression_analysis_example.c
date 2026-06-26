#include "../include/incompressibility.h"
#include "../include/incompressibility_proofs.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    printf("=== Compression-Based Analysis ===\n\n");
    const char* tests[] = {
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaa",
        "abcdefghijklmnopqrstuvwxyz",
        "the quick brown fox jumps over",
        "To be or not to be that is the"
    };
    int n_tests = 4;
    for (int i = 0; i < n_tests; i++) {
        size_t len = strlen(tests[i]);
        IncompressibilityAnalysis* a = inc_analyze_string((const unsigned char*)tests[i], len);
        printf("String %d: %s\n", i+1, tests[i]);
        inc_analyze_report(a);
        LZ77Token* tokens = NULL;
        size_t n_tok = lz77_compress((const unsigned char*)tests[i], len, &tokens);
        printf("  LZ77 tokens: %zu\n\n", n_tok);
        free(tokens);
        inc_analyze_free(a);
    }
    printf("=== NID Demo ===\n");
    const unsigned char* x = (const unsigned char*)"helloworld";
    const unsigned char* y = (const unsigned char*)"helloworkd";
    double nid = inc_information_distance_estimate(x, 10, y, 10);
    printf("NID(helloworld, helloworkd) = %.4f\n", nid);
    return 0;
}
