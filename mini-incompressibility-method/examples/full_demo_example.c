#include "../include/incompressibility.h"
#include "../include/incompressibility_proofs.h"
#include <stdio.h>

int main(void) {
    printf("Incompressibility Method -- Full Demo\n");
    inc_run_all_proofs_demo(10);
    printf("\n=== Counting Bound ===\n");
    for (int c = 1; c <= 20; c += 5) {
        double frac = inc_count_compressible_fraction(100, c);
        printf("c=%-3d: incompressible fraction >= %.2f%%\n", c, (1.0-frac)*100.0);
    }
    return 0;
}
