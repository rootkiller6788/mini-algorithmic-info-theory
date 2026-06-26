#include "../include/incompressibility.h"
#include "../include/incompressibility_proofs.h"
#include <stdio.h>

int main(void) {
    printf("=== Communication Complexity via Incompressibility ===\n");
    printf("Yao-style communication lower bounds via K-complexity.\n\n");
    for (size_t n = 4; n <= 64; n *= 2) {
        size_t eq = inc_eq_communication_lower_bound(n);
        size_t dj = inc_disjointness_lower_bound(n);
        printf("n=%-4zu  EQ_n: %zu bits  DISJ_n: %zu bits\n", n, eq, dj);
    }
    printf("\nKey: EQ_n has exp gap (det O(n), rand O(log n)).\n");
    printf("DISJ_n has NO gap (both Theta(n)).\n");
    printf("\n=== EQ_n Proof ===\n");
    IncompressibilityProof* p = inc_proof_eq_communication(8);
    incproof_print(p);
    incproof_free(p);
    return 0;
}
