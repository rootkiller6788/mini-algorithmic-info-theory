/*
 * example_detect_prng.c — PRNG Detection Example
 *
 * Demonstrates ML-based detection of pseudorandom number generators
 * vs true random sources using statistical and ML features.
 *
 * Usage: ./build/example_example_detect_prng
 * Build: make examples
 */
#include "../include/randomness.h"
#include "../include/randomness_ml.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("=== PRNG Detection Example ===\n\n");
    unsigned int seed = 12345;

    /* Generate "true random" data using LCG (for demonstration) */
    printf("Generating test data (2048 bits)...\n");
    RandomBitString* sample = rbs_random(2048, &seed);
    printf("Sample: "); rbs_print(sample, 64);

    /* Run classical NIST battery */
    printf("\n--- NIST SP 800-22 Statistical Tests ---\n");
    NISTTestBattery* nist = nist_test_battery_run(sample);
    nist_test_battery_print(nist);
    nist_test_battery_free(nist);

    /* Run ML-based PRNG detection */
    printf("\n--- ML-Based PRNG Detection ---\n");
    PRNGDetectionResult* result = detect_pseudorandom_ml(sample);
    prng_detection_result_print(result);
    prng_detection_result_free(result);

    /* Comprehensive randomness profile */
    printf("\n--- Comprehensive Randomness Profile ---\n");
    RandomnessProfile* profile = randomness_profile_create(sample);
    randomness_profile_print(profile);
    if (profile) {
        RandomnessType t = randomness_profile_classify(profile);
        const char* type_names[] = {
            "ML-Random", "Schnorr-Random", "Computably-Random",
            "Stochastic", "Weakly-Random", "Non-Random"
        };
        printf("  Classification: %s\n", type_names[t]);
    }
    randomness_profile_free(profile);

    rbs_free(sample);
    printf("\n=== Example Complete ===\n");
    return 0;
}
