#include "../include/incompressibility.h"
#include "../include/incompressibility_proofs.h"
#include <stdio.h>
#include <math.h>

int main(void) {
    printf("=== Sorting Lower Bound via Incompressibility ===\n");
    printf("Proves that comparison-based sorting needs Omega(n log n).\n\n");
    for (size_t n = 2; n <= 16; n *= 2) {
        size_t lb = inc_sorting_comparison_lower_bound(n);
        double logn = log2((double)n);
        printf("n=%-4zu  log2(n!)=%-4zu  n*log2(n)=%6.1f  ratio=%.3f\n",
               n, lb, (double)n * logn, (double)lb / ((double)n * logn));
    }
    printf("\nAs n->inf, log2(n!) / (n log n) -> 1 (Stirling).\n");
    IncompressibilityProof* proof = inc_proof_sorting_lower_bound(8);
    incproof_print(proof);
    incproof_free(proof);
    return 0;
}
