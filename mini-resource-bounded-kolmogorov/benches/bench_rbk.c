/**
 * bench_rbk.c - Performance benchmark for resource-bounded Kolmogorov
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/resource_bounded_k.h"
#include "../include/universal_search.h"
#include "../include/complexity_approx.h"
#include "../include/program_resource.h"

static double time_ms(clock_t start, clock_t end) {
    return (double)(end - start) * 1000.0 / (double)CLOCKS_PER_SEC;
}

int main(void) {
    printf("=== RBK Benchmarks ===\n\n");
    clock_t start, end;

    const char *s = "0101010101";
    size_t n = 10;

    /* K_time benchmark */
    start = clock();
    RBKComplexity kc = rbk_K_time(s, n, 65536);
    end = clock();
    printf("K^t (n=%zu): %zu bits, %.2f ms\n", n, kc.K_value, time_ms(start, end));
    rbk_complexity_free(&kc);

    /* K_prefix benchmark */
    start = clock();
    kc = rbk_K_prefix(s, n, 65536);
    end = clock();
    printf("K_prefix (n=%zu): %zu bits, %.2f ms\n", n, kc.K_value, time_ms(start, end));
    rbk_complexity_free(&kc);

    /* Structure function benchmark */
    start = clock();
    size_t pts = 0;
    RBKStructureFn *sf = rbk_structure_function(s, n, 15, &pts);
    end = clock();
    printf("Structure fn (max_k=15): %zu pts, %.2f ms\n", pts, time_ms(start, end));
    rbk_structure_fn_free(sf, pts);

    /* LZ77 compression benchmark */
    start = clock();
    CompressionBound cb = lz77_compress_bound(s, n);
    end = clock();
    printf("LZ77 compress: %zu -> %zu bytes, %.2f ms\n",
           cb.original_len, cb.compressed_len, time_ms(start, end));
    compression_bound_free(&cb);

    /* LZ78 complexity benchmark */
    start = clock();
    LZComplexity lz = lz78_complexity(s, n);
    end = clock();
    printf("LZ78 complexity: %zu phrases, %.2f ms\n",
           lz.phrase_count, time_ms(start, end));

    /* Levin search benchmark */
    start = clock();
    LevinResult lr = levin_search(s, n, NULL, 0, 50000);
    end = clock();
    printf("Levin search: found=%s, %.2f ms\n",
           lr.solution_found ? "YES" : "NO", time_ms(start, end));
    levin_result_free(&lr);

    /* Program pool benchmark */
    start = clock();
    ProgramPool pool = program_pool_create(6);
    size_t count = program_pool_enumerate_prefix_free(&pool, 5);
    end = clock();
    printf("Program pool (len<=5): %zu programs, %.2f ms\n",
           count, time_ms(start, end));
    program_pool_free(&pool);

    /* Entropy profile benchmark */
    start = clock();
    EntropyProfile ep = entropy_profile_compute(s, n);
    end = clock();
    printf("Entropy profile: SampEn=%.4f, %.2f ms\n",
           ep.sample_entropy, time_ms(start, end));

    printf("\nDone.\n");
    return 0;
}