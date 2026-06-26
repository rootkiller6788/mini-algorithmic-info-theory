/*
 * gan_randomness.h — GAN-Based Detection of Algorithmic Structure
 *
 * L7-L9: Generative Adversarial Networks for detecting non-random
 * structure in data. A GAN trained on random data learns to generate
 * "fake random" numbers. A discriminator trained to distinguish
 * true randomness from GAN output can then detect hidden structure.
 *
 * Key idea (Goodfellow et al., 2014): The GAN framework naturally
 * models the difference between "truly random" and "structured but
 * looks random" data. A perfect discriminator would detect any
 * computable regularity — linking GANs to Martin-Löf tests.
 *
 * Connection to algorithmic randomness:
 *   GAN discriminator ≈ sequential test
 *   If data is ML-random, no discriminator can beat 50% accuracy.
 *   If data has structure, discriminator can exploit it.
 *
 * Reference: Goodfellow et al. "GANs" (2014)
 *            Arjovsky et al. "Wasserstein GAN" (2017)
 *            Goldblum et al. "GAN Detection of Generated Text"
 * Courses: MIT 6.867, Stanford CS236, Berkeley CS294-158
 */

#ifndef GAN_RANDOMNESS_H
#define GAN_RANDOMNESS_H

#include "randomness.h"
#include <stdlib.h>
#include <math.h>

/* ══════════════════════════════════════════════════════════════
   L1: GAN Data Structures
   ══════════════════════════════════════════════════════════════ */

/* ── Generator network ─────────────────────────────────────── */
typedef struct {
    double* w1, *b1;      /* hidden layer 1 */
    double* w2, *b2;      /* hidden layer 2 */
    double* w_out, *b_out; /* output layer */
    int     latent_dim;
    int     hidden_dim;
    int     output_dim;    /* bit string length */
} GANGenerator;

/* ── Discriminator network ─────────────────────────────────── */
typedef struct {
    double* w1, *b1;
    double* w2, *b2;
    double* w_out, *b_out;
    int     input_dim;
    int     hidden_dim;
} GANDiscriminator;

/* ── GAN training state ────────────────────────────────────── */
typedef struct {
    GANGenerator*    gen;
    GANDiscriminator* disc;
    int              input_dim;
    int              latent_dim;
    int              hidden_dim;
    int              trained_iters;
    double           gen_loss;
    double           disc_loss;
    double           wasserstein_distance;  /* WGAN metric */
} GANTrained;

/* ── Discriminator-based randomness test ───────────────────── */
typedef struct {
    GANDiscriminator* disc;
    double            accuracy_on_random;
    double            accuracy_on_structured;
    double            detection_threshold;
    int               is_calibrated;
} GANRandomnessDetector;

/* ── GAN-generated sample ──────────────────────────────────── */
typedef struct {
    RandomBitString* sample;
    double           discriminator_score;  /* D(x) — probability "real" */
    int              is_gan_generated;     /* 1 if from generator */
} GANSample;

/* ══════════════════════════════════════════════════════════════
   L2: GAN Training for Randomness
   ══════════════════════════════════════════════════════════════ */

/**
 * gan_create — Initialize a GAN with specified architecture.
 * Latent noise → Generator → Fake data ⋮ Real random data → Discriminator.
 */
GANTrained* gan_create(int input_dim, int latent_dim, int hidden_dim);

/**
 * gan_train_on_random — Train the GAN using:
 *   - Real data: cryptographic random number generator output
 *   - Fake data: generator output from latent noise
 * After training, the discriminator learns subtle patterns
 * that distinguish "truly random" from "algorithmically generated".
 */
void gan_train_on_random(GANTrained* gan,
                          RandomBitString** real_samples, int n_real,
                          int iterations, double lr);

/**
 * gan_train_wasserstein — WGAN training with gradient penalty.
 * More stable training, better gradient signal.
 */
void gan_train_wasserstein(GANTrained* gan,
                            RandomBitString** real_samples, int n_real,
                            int iterations, double lr, double gp_weight);

/**
 * gan_generate — Generate a fake bit string from latent noise.
 */
RandomBitString* gan_generate(GANTrained* gan);

/**
 * gan_discriminate — Get discriminator's judgment:
 *   D(x) ≈ P(x is real random), range [0, 1]
 */
double gan_discriminate(GANTrained* gan, const RandomBitString* x);

void gan_print_status(const GANTrained* gan);
void gan_free(GANTrained* gan);

/* ══════════════════════════════════════════════════════════════
   L3: GAN Architecture Variants
   ══════════════════════════════════════════════════════════════ */

/**
 * dcgan_randomness — Deep Convolutional GAN for 2D randomness
 * detection. Treats data as 2D image for spatial correlation.
 */
GANTrained* dcgan_randomness_create(int width, int height, int latent_dim);
void dcgan_randomness_train(GANTrained* gan,
                             RandomBitString** real_samples, int n,
                             int epochs, double lr);

/**
 * conditional_gan_randomness — cGAN conditioned on entropy level,
 * for fine-grained control over generation randomness.
 */
GANTrained* cgan_randomness_create(int input_dim, int latent_dim,
                                    int n_conditions);
void cgan_randomness_train(GANTrained* gan,
                            RandomBitString** samples, double* conditions,
                            int n, int epochs, double lr);

/* ══════════════════════════════════════════════════════════════
   L4: GAN-Randomness Theorems
   ══════════════════════════════════════════════════════════════ */

/**
 * gan_detection_theorem — Formalize: If a GAN discriminator can
 * achieve accuracy > 0.5 + ε on distinguishing true randomness
 * from an algorithmic source, then the source is NOT ML-random.
 *
 * Returns: estimated ε lower bound for the trained discriminator.
 */
double gan_detection_bound(GANTrained* gan,
                            RandomBitString** real_samples, int n_real,
                            RandomBitString** test_samples, int n_test);

/**
 * gan_universality_conjecture — Hypothesis: As GAN capacity → ∞,
 * the optimal discriminator approximates a universal Martin-Löf test.
 * This function provides experimental evidence by comparing GAN
 * detection with classical randomness test results.
 */
double gan_universality_evidence(GANTrained* gan, int n_trials);

/* ══════════════════════════════════════════════════════════════
   L5: GAN-Based Randomness Detection
   ══════════════════════════════════════════════════════════════ */

/**
 * gan_randomness_detector_train — Train a discriminator to be a
 * dedicated randomness detector. Uses adversarial training on
 * known-random and known-nonrandom data.
 */
GANRandomnessDetector* gan_randomness_detector_train(
    RandomBitString** random_samples, int n_random,
    RandomBitString** structured_samples, int n_structured,
    int hidden_dim, int epochs, double lr);

/**
 * gan_randomness_detector_score — Score a sample:
 *   High score = appears random, Low score = suspicious structure.
 */
double gan_randomness_detector_score(
    const GANRandomnessDetector* det, const RandomBitString* s);

/**
 * gan_randomness_detector_is_random — Binary classification.
 */
int gan_randomness_detector_is_random(
    const GANRandomnessDetector* det, const RandomBitString* s,
    double threshold);

void gan_randomness_detector_print(const GANRandomnessDetector* det);
void gan_randomness_detector_free(GANRandomnessDetector* det);

/* ══════════════════════════════════════════════════════════════
   L6: Canonical Problems
   ══════════════════════════════════════════════════════════════ */

/**
 * detect_hidden_structure — The canonical GAN randomness problem:
 * Given a data source that "looks random" but was generated by a
 * known algorithm (π digits, LCG, LFSR, etc.), can a GAN detect
 * it better than classical tests?
 */
typedef struct {
    char*   source_type;           /* "π digits", "LCG", "LFSR", etc. */
    double  classical_hit_rate;    /* fraction detected by NIST tests */
    double  gan_hit_rate;          /* fraction detected by GAN */
    double  gan_advantage;         /* gan - classical */
    int     n_samples_tested;
} StructureDetectionComparison;

StructureDetectionComparison* detect_hidden_structure_gan(
    const RandomBitString* s, int n_trials);
void structure_detection_print(const StructureDetectionComparison* c);
void structure_detection_free(StructureDetectionComparison* c);

/**
 * gan_vs_classical_benchmark — Comprehensive benchmark comparing
 * GAN-based detection against classical randomness test suites.
 */
typedef struct {
    double  gan_auc;                /* GAN detector AUC */
    double  nist_auc;               /* NIST battery AUC */
    double  autoencoder_auc;        /* Autoencoder anomaly AUC */
    double  neural_classifier_auc;  /* ML classifier AUC */
    double  ensemble_auc;           /* Combined AUC */
    int     n_benchmark_samples;
} DetectionBenchmark;

DetectionBenchmark* gan_vs_classical_benchmark(
    RandomBitString** random_samples, int n_random,
    RandomBitString** structured_samples, int n_structured);
void detection_benchmark_print(const DetectionBenchmark* b);
void detection_benchmark_free(DetectionBenchmark* b);

/* ══════════════════════════════════════════════════════════════
   L7-L9: Applications & Frontiers
   ══════════════════════════════════════════════════════════════ */

/**
 * adversarial_randomness_attack — Test whether a GAN can generate
 * data that fools classical randomness tests (adversarial RNG).
 * If successful, this demonstrates the gap between statistical
 * and algorithmic randomness tests.
 */
typedef struct {
    int     nist_passed;           /* number of NIST tests passed */
    int     gan_detector_fooled;   /* 1 if GAN detector fooled */
    double  kolmogorov_estimate;   /* estimated K(x)/|x| */
    int     is_martin_lof_random;  /* 1 if passes ML test */
} AdversarialRNGResult;

AdversarialRNGResult* adversarial_randomness_attack(
    GANTrained* gan, int n_tests);
void adversarial_rng_result_print(const AdversarialRNGResult* r);
void adversarial_rng_result_free(AdversarialRNGResult* r);

#endif /* GAN_RANDOMNESS_H */
