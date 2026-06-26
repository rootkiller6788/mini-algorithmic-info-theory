/*
 * example_gan_detect.c — GAN Randomness Detection Example
 *
 * Demonstrates GAN-based detection of algorithmic structure in data.
 * Trains a GAN on random data, then uses the discriminator to detect
 * non-random patterns that classical tests might miss.
 *
 * Key idea (Goodfellow et al., 2014): GAN discriminator ≈ Martin-Löf test.
 * If discriminator accuracy > 0.5, the source has computable regularity.
 *
 * Usage: ./build/example_example_gan_detect
 * Build: make examples
 */
#include "../include/randomness.h"
#include "../include/gan_randomness.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    printf("=== GAN Randomness Detection Example ===\n\n");
    unsigned int seed = 54321;

    /* Generate training data: "true random" samples */
    int n_real = 8;
    printf("Generating %d random training samples (128 bits each)...\n", n_real);
    RandomBitString** real_samples = (RandomBitString**)malloc(
        (size_t)n_real * sizeof(RandomBitString*));
    for (int i = 0; i < n_real; i++)
        real_samples[i] = rbs_random(128, &seed);

    /* Create GAN and train on random data */
    printf("Creating GAN (128 dim, 32 latent, 64 hidden)...\n");
    GANTrained* gan = gan_create(128, 32, 64);
    gan_print_status(gan);

    printf("\nTraining GAN on random data (50 iterations)...\n");
    gan_train_on_random(gan, real_samples, n_real, 50, 0.01);
    printf("After training:\n");
    gan_print_status(gan);

    /* Generate fake sample from trained GAN */
    printf("\n--- GAN Generation ---\n");
    RandomBitString* fake = gan_generate(gan);
    printf("  Generated: "); rbs_print(fake, 64);

    /* Discriminate real vs fake */
    double d_real = gan_discriminate(gan, real_samples[0]);
    double d_fake = gan_discriminate(gan, fake);
    printf("  D(real_sample) = %.4f\n", d_real);
    printf("  D(fake_sample) = %.4f\n", d_fake);

    /* GAN detection theorem verification */
    printf("\n--- GAN Detection Theorem ---\n");
    double epsilon = gan_detection_bound(gan, real_samples, n_real,
                                          real_samples, n_real);
    printf("  Detection epsilon: %.4f\n", epsilon);
    printf("  (epsilon > 0 means discriminator found structure)\n");

    /* Universality evidence */
    printf("\n--- GAN Universality Evidence ---\n");
    double corr = gan_universality_evidence(gan, 5);
    printf("  Correlation with classical tests: %.4f\n", corr);

    /* Adversarial RNG attack demo */
    printf("\n--- Adversarial RNG Attack ---\n");
    AdversarialRNGResult* adv = adversarial_randomness_attack(gan, 3);
    adversarial_rng_result_print(adv);
    adversarial_rng_result_free(adv);

    /* GAN vs Classical benchmark */
    printf("\n--- GAN vs Classical Benchmark ---\n");
    RandomBitString** struct_samples = (RandomBitString**)malloc(
        3 * sizeof(RandomBitString*));
    for (int i = 0; i < 3; i++)
        struct_samples[i] = rbs_from_binary(
            "01010101010101010101010101010101"
            "01010101010101010101010101010101");
    DetectionBenchmark* bench = gan_vs_classical_benchmark(
        real_samples, 3, struct_samples, 3);
    detection_benchmark_print(bench);

    /* Cleanup */
    for (int i = 0; i < n_real; i++) rbs_free(real_samples[i]);
    for (int i = 0; i < 3; i++) rbs_free(struct_samples[i]);
    free(real_samples); free(struct_samples);
    rbs_free(fake);
    gan_free(gan);
    detection_benchmark_free(bench);

    printf("\n=== Example Complete ===\n");
    return 0;
}
