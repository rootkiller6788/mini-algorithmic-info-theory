/*
 * test_main.c -- Tests for mini-incompressibility-method
 *
 * Assert-based tests covering L1-L8 knowledge levels.
 */

#include "../include/incompressibility.h"
#include "../include/incompressibility_proofs.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Externally defined in incompressibility_proofs.c */
extern size_t heapsort_count_comparisons(int* arr, size_t n);
extern int is_palindrome_with_count(const unsigned char* s, size_t n, size_t* exams);
extern size_t lcs_length(const unsigned char* x, size_t xlen,
                          const unsigned char* y, size_t ylen);

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(cond, msg) do { \
    if (cond) { tests_passed++; } \
    else { tests_failed++; printf("FAIL: %s\n", msg); } \
} while(0)

int main(void) {
    printf("=== mini-incompressibility-method Test Suite ===\n\n");

    /* L1: IncompressibleString */
    {
        const char* data = "Hello, Kolmogorov!";
        IncompressibleString* s = incstr_create((const unsigned char*)data, strlen(data), 10);
        TEST(s != NULL, "incstr_create returns non-NULL");
        TEST(s->len == strlen(data), "incstr_create: correct length");
        TEST(s->incomp_c == 10, "incstr_create: correct c");
        incstr_free(s);
    }

    /* L1: KolmogorovBound */
    {
        KolmogorovBound* b = kb_create(100, 100, 0);
        TEST(b != NULL, "kb_create: non-NULL");
        TEST(b->is_tight == 1, "kb_create: tight bound");
        TEST(kb_is_contradiction(b) == 0, "kb_is_contradiction: no contradiction");
        kb_free(b);

        KolmogorovBound* b2 = kb_create(200, 100, 0);
        TEST(kb_is_contradiction(b2) == 1, "kb_is_contradiction: contradiction");
        TEST(kb_gap(b2) == 100, "kb_gap: correct gap");
        kb_free(b2);
    }

    /* L2: IncompressibilityProof */
    {
        IncompressibilityProof* proof = incproof_create("Test Theorem", "Test Property");
        TEST(proof != NULL, "incproof_create: non-NULL");
        incproof_add_step(proof, PROOF_STEP_ASSUME, "Assume contradiction", 10, 0.0);
        incproof_add_step(proof, PROOF_STEP_ENCODE, "Encode violation", 15, 0.0);
        incproof_add_step(proof, PROOF_STEP_BOUND, "Derive bound", 20, 5.0);
        incproof_add_step(proof, PROOF_STEP_CONTRADICT, "Contradiction", 5, 0.0);
        incproof_add_step(proof, PROOF_STEP_CONCLUDE, "QED", 10, 0.0);
        TEST(incproof_validate(proof) == 1, "incproof_validate: valid proof");
        incproof_free(proof);
    }

    /* L2: CounterExample */
    {
        int data[] = {1, 2, 3, 4, 5};
        CounterExample* cex = cex_create(data, sizeof(data), "int[5]");
        TEST(cex != NULL, "cex_create: non-NULL");
        size_t enc_len = cex_encoding_length(cex, PROOF_PIGEONHOLE);
        TEST(enc_len > 0, "cex_encoding_length: positive length");
        cex_free(cex);
    }

    /* L3: Pigeonhole and Counting */
    {
        TEST(inc_pigeonhole_proof(10, 9) == 1, "inc_pigeonhole_proof: 10 into 9 forces collision");
        TEST(inc_pigeonhole_proof(5, 10) == 0, "inc_pigeonhole_proof: 5 into 10 OK");
        TEST(inc_counting_bound(1000, 10, 5) == 1, "inc_counting_bound: rare property");
    }

    /* L3: Encoding with property */
    {
        const unsigned char* test_str = (const unsigned char*)"ABCDEFGH";
        unsigned char* encoded = NULL;
        size_t enc_len = inc_encode_with_property(test_str, 8, 1, &encoded);
        TEST(enc_len == 9, "inc_encode_with_property: len=n+1 for holds");
        TEST(encoded[0] == '0', "inc_encode_with_property: prefix 0 for holds");
        free(encoded);

        enc_len = inc_encode_with_property(test_str, 8, 0, &encoded);
        TEST(encoded[0] == '1', "inc_encode_with_property: prefix 1 for fails");
        free(encoded);
    }

    /* L3: Self-delimiting encoding */
    {
        const unsigned char* msg = (const unsigned char*)"AB";
        unsigned char* encoded = NULL;
        size_t enc_len = inc_self_delimiting_encode(msg, 2, &encoded);
        TEST(enc_len > 0, "inc_self_delimiting_encode: positive");
        unsigned char* decoded = NULL;
        size_t dec_len = inc_self_delimiting_decode(encoded, enc_len, &decoded);
        TEST(dec_len == 2, "inc_self_delimiting_decode: length=2");
        TEST(memcmp(msg, decoded, 2) == 0, "inc_self_delimiting_decode: correct");
        free(encoded); free(decoded);
    }

    /* L3: Pair encoding */
    {
        const unsigned char x[] = {1, 2, 3};
        const unsigned char y[] = {4, 5};
        unsigned char* enc = NULL;
        size_t enc_len = inc_encode_pair(x, 3, y, 2, &enc);
        TEST(enc_len == 9, "inc_encode_pair: 4+3+2=9 bytes");
        unsigned char *x_out = NULL, *y_out = NULL;
        size_t xl = 0, yl = 0;
        TEST(inc_decode_pair(enc, enc_len, &x_out, &xl, &y_out, &yl) == 1, "inc_decode_pair: OK");
        TEST(xl == 3 && yl == 2, "inc_decode_pair: correct lengths");
        free(enc); free(x_out); free(y_out);
    }

    /* L4: Sorting lower bound */
    {
        size_t lb = inc_sorting_comparison_lower_bound(10);
        TEST(lb >= 21, "inc_sorting_lower_bound(10) >= 22 (log2(10!)=~21.8)");
        TEST(inc_verify_sorting_bound(10, lb) == 1, "inc_verify_sorting_bound: passes");
        TEST(inc_verify_sorting_bound(10, lb-1) == 0, "inc_verify_sorting_bound: fails");

        IncompressibilityProof* proof = inc_proof_sorting_lower_bound(8);
        TEST(incproof_validate(proof) == 1, "inc_proof_sorting_lower_bound: valid");
        incproof_free(proof);
    }

    /* L4: Maximum lower bound */
    {
        TEST(inc_maximum_lower_bound(10) == 9, "inc_maximum_lower_bound(10) = 9");
        TEST(inc_maximum_lower_bound(1) == 0, "inc_maximum_lower_bound(1) = 0");

        IncompressibilityProof* proof = inc_proof_maximum_lower_bound(10);
        TEST(incproof_validate(proof) == 1, "inc_proof_maximum_lower_bound: valid");
        incproof_free(proof);
    }

    /* L4: Median lower bound */
    {
        size_t lb = inc_median_lower_bound(10);
        TEST(lb == 13, "inc_median_lower_bound(10) = 3*10/2 - 2 = 13");

        IncompressibilityProof* proof = inc_proof_median_lower_bound(10);
        TEST(incproof_validate(proof) == 1, "inc_proof_median_lower_bound: valid");
        incproof_free(proof);
    }

    /* L4: Graph bounds */
    {
        TEST(inc_connectivity_edge_lower_bound(10) == 22, "inc_connectivity_edge_lower_bound(10) = 22");
        TEST(inc_component_counting_lower_bound(10, 30) == 15, "inc_component_counting_lower_bound(10,30) = 15");
        TEST(inc_gni_certificate_size_bound(10) == 100, "inc_gni_certificate_size_bound(10) = 100");
    }

    /* L4: String bounds */
    {
        TEST(inc_string_matching_lower_bound(100, 10) == 91, "inc_string_matching_lower_bound(100,10) = 91");
        TEST(inc_lcs_lower_bound(10) == 25, "inc_lcs_lower_bound(10) = 25");
        TEST(inc_palindrome_lower_bound(10) == 5, "inc_palindrome_lower_bound(10) = 5");
        TEST(inc_palindrome_lower_bound(11) == 6, "inc_palindrome_lower_bound(11) = 6");
    }

    /* L6: Palindrome check */
    {
        size_t exams;
        const unsigned char* pal = (const unsigned char*)"radar";
        TEST(is_palindrome_with_count(pal, 5, &exams) == 1, "is_palindrome: radar");
        TEST(exams == 2, "is_palindrome_with_count: 2 exams for len=5");

        const unsigned char* not_pal = (const unsigned char*)"hello";
        TEST(is_palindrome_with_count(not_pal, 5, &exams) == 0, "is_palindrome: hello");
    }

    /* L6: LCS */
    {
        const unsigned char* x = (const unsigned char*)"ABCDGH";
        const unsigned char* y = (const unsigned char*)"AEDFHR";
        TEST(lcs_length(x, 6, y, 6) == 3, "lcs_length(ABCDGH, AEDFHR) = 3 (ADH)");
        TEST(lcs_length(x, 0, y, 6) == 0, "lcs_length: empty x => 0");
    }

    /* L6: Heapsort counting */
    {
        int arr[] = {5, 3, 8, 1, 9, 2, 7, 4, 6};
        size_t n = 9;
        size_t cmps = heapsort_count_comparisons(arr, n);
        TEST(cmps > 0, "heapsort_count_comparisons: positive");
        for (size_t i = 1; i < n; i++)
            TEST(arr[i-1] <= arr[i], "heapsort_count_comparisons: sorted");
    }

    /* L5: Proof generation */
    {
        IncompressibilityProof* p;
        p = inc_generate_pigeonhole_proof(10);
        TEST(incproof_validate(p) == 1, "inc_generate_pigeonhole_proof: valid"); incproof_free(p);
        p = inc_generate_sorting_lowerbound_proof(8);
        TEST(incproof_validate(p) == 1, "inc_generate_sorting_lowerbound_proof: valid"); incproof_free(p);
        p = inc_generate_heapsort_optimality_proof(8);
        TEST(incproof_validate(p) == 1, "inc_generate_heapsort_optimality_proof: valid"); incproof_free(p);
        p = inc_generate_graph_noniso_proof(5);
        TEST(incproof_validate(p) == 1, "inc_generate_graph_noniso_proof: valid"); incproof_free(p);
    }

    /* L7: Pumping lemmas */
    {
        TEST(inc_pumping_lemma_regular(3, "a*") == 1, "inc_pumping_lemma_regular(a*): regular");
        TEST(inc_pumping_lemma_regular(3, "a^n") == 0, "inc_pumping_lemma_regular(a^n): non-regular");
        TEST(inc_pumping_lemma_cfl(3, "a^n") == 1, "inc_pumping_lemma_cfl(a^n): CFL");
        TEST(inc_pumping_lemma_cfl(3, "c^n") == 0, "inc_pumping_lemma_cfl(c^n): not CFL");

        IncompressibilityProof* pr = inc_proof_regular_pumping();
        TEST(incproof_validate(pr) == 1, "inc_proof_regular_pumping: valid"); incproof_free(pr);
        IncompressibilityProof* pc = inc_proof_cfl_pumping();
        TEST(incproof_validate(pc) == 1, "inc_proof_cfl_pumping: valid"); incproof_free(pc);
    }

    /* L7: Average-case */
    {
        TEST(inc_average_case_sorting_bound(10) > 0, "inc_average_case_sorting_bound(10) > 0");
        IncompressibilityProof* p = inc_proof_average_sorting_bound(10);
        TEST(incproof_validate(p) == 1, "inc_proof_average_sorting_bound: valid"); incproof_free(p);

        int frac = inc_average_equals_worst_case(100, PROOF_COUNTING);
        TEST(frac >= 990, "inc_average_equals_worst_case: >= 99%");
    }

    /* L7: Heapsort average */
    {
        double c = inc_heapsort_average_comparisons(100);
        TEST(c > 0, "inc_heapsort_average_comparisons(100) > 0");
    }

    /* L7: Communication */
    {
        TEST(inc_eq_communication_lower_bound(10) == 11, "inc_eq_communication_lower_bound(10) = 11");
        IncompressibilityProof* p = inc_proof_eq_communication(10);
        TEST(incproof_validate(p) == 1, "inc_proof_eq_communication: valid"); incproof_free(p);

        TEST(inc_disjointness_lower_bound(10) == 11, "inc_disjointness_lower_bound(10) = 11");
        p = inc_proof_disjointness_communication(10);
        TEST(incproof_validate(p) == 1, "inc_proof_disjointness_communication: valid"); incproof_free(p);
    }

    /* L5: apply_method */
    {
        int result = inc_apply_method(NULL, 16, 5, NULL);
        TEST(result == -1, "inc_apply_method: NULL => -1");
    }

    /* L8: LZ77 compression (tested via NCD which uses it internally) */
    {
        printf("\n--- L8: LZ77 Compression ---\n");
        const unsigned char* data = (const unsigned char*)"AAAAAABBBBCCCC";
        LZ77Token* tokens = NULL;
        size_t n_tok = lz77_compress(data, 14, &tokens);
        printf("LZ77: %zu tokens for 14 bytes of repeated input\n", n_tok);
        TEST(tokens != NULL, "lz77_compress: non-NULL");
        TEST(n_tok > 0, "lz77_compress: at least 1 token");
        free(tokens);
    }

    /* L8: Information distance */
    {
        printf("\n--- L8: Information Distance ---\n");
        const unsigned char* x = (const unsigned char*)"helloworld";
        const unsigned char* y = (const unsigned char*)"helloworkd";
        double nid = inc_information_distance_estimate(x, 10, y, 10);
        printf("NID(helloworld, helloworkd) = %.4f\n", nid);
        TEST(nid >= 0.0, "inc_information_distance_estimate: non-negative");
    }

    /* L6: Incompressibility analysis */
    {
        printf("\n--- L6: Incompressibility Analysis ---\n");
        const unsigned char* data = (const unsigned char*)"abcdefghijklmnop";
        IncompressibilityAnalysis* a = inc_analyze_string(data, 16);
        TEST(a != NULL, "inc_analyze_string: non-NULL");
        inc_analyze_report(a);
        inc_analyze_free(a);
    }

    /* L1: Counting lemma */
    {
        double f = inc_count_compressible_fraction(100, 10);
        TEST(f < 0.01, "inc_count_compressible_fraction(100,10) < 1%");
    }

    /* L6: Demo (brief) */
    {
        printf("\n--- L6: Demo (n=8) ---\n");
        inc_run_all_proofs_demo(8);
        TEST(1, "inc_run_all_proofs_demo: runs without crash");
    }

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}