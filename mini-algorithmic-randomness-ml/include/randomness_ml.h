/*
 * randomness_ml.h — Machine Learning for Algorithmic Randomness
 *
 * L7-L9: ML-based estimation of Kolmogorov complexity, neural network
 * randomness classifiers, autoencoder anomaly detection, GAN detection
 * of structured data masquerading as random.
 *
 * Key idea: While K(x) is incomputable (Chaitin, 1969), neural networks
 * can learn to estimate it from data, providing practical upper bounds
 * that converge to K(x) for large enough networks.
 *
 * Reference: Schmidhuber "Deep Learning & Algorithmic Information Theory"
 *            Zenil et al. "Numerical Evaluation of Algorithmic Complexity"
 *            Goldblum et al. "Detecting AI-Generated Randomness"
 * Courses: MIT 6.867 (ML), Stanford CS229, Berkeley CS294-158
 */

#ifndef RANDOMNESS_ML_H
#define RANDOMNESS_ML_H

#include "randomness.h"
#include <stdlib.h>
#include <math.h>

/* ══════════════════════════════════════════════════════════════
   L1: ML Data Structures
   ══════════════════════════════════════════════════════════════ */

/* ── Neural network layer ──────────────────────────────────── */
typedef struct {
    double* weights;       /* W[l][i,j] weight matrix (flattened) */
    double* biases;        /* b[l][i] */
    double* output;        /* activation output cache */
    double* grad_w;        /* weight gradient */
    double* grad_b;        /* bias gradient */
    int     n_in;
    int     n_out;
    int     activation;    /* 0=ReLU, 1=Sigmoid, 2=Tanh, 3=Linear */
} NLayer;

/* ── Feed-forward neural network ───────────────────────────── */
typedef struct {
    NLayer*  layers;
    int      n_layers;
    double   learning_rate;
    double   momentum;
    int      batch_size;
    int      trained_epochs;
    double   train_loss;
    double   val_loss;
} NeuralNet;

/* ── Feature vector for randomness ML ──────────────────────── */
typedef struct {
    double*  features;
    int      n_features;
    char**   feature_names;
} RandomFeatures;

/* ── ML randomness classifier ──────────────────────────────── */
typedef struct {
    NeuralNet*      net;
    char*           model_name;
    int             n_classes;       /* 2: random/non-random, or N types */
    double          accuracy;
    double          precision;
    double          recall;
    double          f1_score;
    double*         confusion_matrix; /* flattened n_classes×n_classes */
    int             is_trained;
} MLRandomnessClassifier;

/* ── Autoencoder for anomaly (non-randomness) detection ────── */
typedef struct {
    NeuralNet* encoder;
    NeuralNet* decoder;
    int        latent_dim;
    double     reconstruction_error_threshold;
    double     mean_train_error;
    double     std_train_error;
} AutoencoderAnomalyDetector;

/* ── K-complexity neural estimator ─────────────────────────── */
typedef struct {
    NeuralNet* net;
    int        input_window;   /* context window in bits */
    double     mean_absolute_error;
    int        trained_samples;
} KolmogorovNeuralEstimator;

/* ══════════════════════════════════════════════════════════════
   L2: Core ML Concepts
   ══════════════════════════════════════════════════════════════ */

/**
 * Neural network construction and training for randomness tasks.
 * These feedforward nets learn to approximate the incomputable
 * Kolmogorov complexity function K(x) from finite samples.
 */

NeuralNet* nn_create(int* layer_sizes, int n_layers, double lr);
void nn_forward(NeuralNet* net, const double* input, double* output);
void nn_backward(NeuralNet* net, const double* input, const double* target);
void nn_train(NeuralNet* net, double** inputs, double** targets,
              int n_samples, int epochs);
void nn_train_batch(NeuralNet* net, double** inputs, double** targets,
                    int start, int batch_size);
double nn_predict_single(NeuralNet* net, const double* input);
void nn_free(NeuralNet* net);

/* ══════════════════════════════════════════════════════════════
   L3: Feature Extraction for Randomness ML
   ══════════════════════════════════════════════════════════════ */

/**
 * random_features_extract — Extract ML features from a bit string:
 *   - Entropy features (Shannon, Rényi, min-entropy)
 *   - Statistical features (moments, autocorrelation)
 *   - Compression features (LZ77 ratio, Huffman ratio)
 *   - Spectral features (FFT magnitudes)
 *   - Run-length features (max run, run distribution moments)
 */
RandomFeatures* random_features_extract(const RandomBitString* s);
void random_features_print(const RandomFeatures* f);
void random_features_free(RandomFeatures* f);

/**
 * random_features_to_vector — Convert features to double array
 * for neural network input.
 */
double* random_features_to_vector(const RandomFeatures* f, int* out_len);

/* ══════════════════════════════════════════════════════════════
   L4: Universality Theorems for ML Randomness
   ══════════════════════════════════════════════════════════════ */

/**
 * ml_universal_approximation_theorem — Verify that a 2-layer ReLU
 * network can approximate any randomness test function.
 * Hornik et al. (1989): neural nets are universal approximators.
 * Consequence: ∃ NN that approximates K(x) to arbitrary precision.
 */
int ml_universal_approximation_demo(int hidden_units, double epsilon);

/**
 * pac_randomness_learnability — Check whether randomness can be
 * PAC-learned. Theorem: No — randomness is not PAC-learnable
 * because the class of random sequences has VC-dimension ∞.
 * This function demonstrates the limitation.
 */
int pac_randomness_learnability_bound(int n_samples);

/* ══════════════════════════════════════════════════════════════
   L5: ML Algorithms for Randomness
   ══════════════════════════════════════════════════════════════ */

/**
 * ml_randomness_classifier_train — Train a neural network to
 * classify data as random vs non-random based on extracted features.
 * Training data: known-random (crypto RNG) vs known-structured
 * (patterned, compressed, algorithmic sources).
 */
MLRandomnessClassifier* ml_randomness_classifier_train(
    RandomBitString** samples, int* labels, int n_samples,
    int hidden_units, int epochs);

/**
 * ml_randomness_classifier_predict — Predict randomness probability
 * for a new sample.
 */
double ml_randomness_classifier_predict(
    const MLRandomnessClassifier* clf, const RandomBitString* s);

/**
 * ml_randomness_classifier_evaluate — Compute accuracy, precision,
 * recall, F1 on test set.
 */
void ml_randomness_classifier_evaluate(
    MLRandomnessClassifier* clf,
    RandomBitString** test_samples, int* test_labels, int n_test);

void ml_randomness_classifier_print(const MLRandomnessClassifier* clf);
void ml_randomness_classifier_free(MLRandomnessClassifier* clf);

/* ══════════════════════════════════════════════════════════════
   L5: Autoencoder Anomaly Detection
   ══════════════════════════════════════════════════════════════ */

/**
 * autoencoder_train — Train an autoencoder on "random" data.
 * High reconstruction error on non-random data = anomaly.
 */
AutoencoderAnomalyDetector* autoencoder_train(
    RandomBitString** normal_samples, int n_samples,
    int input_dim, int latent_dim, int epochs);

/**
 * autoencoder_anomaly_score — Score how anomalous (non-random)
   a sample is. Higher = more likely non-random.
 */
double autoencoder_anomaly_score(
    const AutoencoderAnomalyDetector* det, const RandomBitString* s);

/**
 * autoencoder_is_random — Threshold-based classification.
 */
int autoencoder_is_random(
    const AutoencoderAnomalyDetector* det, const RandomBitString* s);

void autoencoder_print(const AutoencoderAnomalyDetector* det);
void autoencoder_free(AutoencoderAnomalyDetector* det);

/* ══════════════════════════════════════════════════════════════
   L5: Neural Kolmogorov Complexity Estimation
   ══════════════════════════════════════════════════════════════ */

/**
 * kolmogorov_neural_estimate — Train a neural network on
 * (string, K_estimate) pairs and use it to estimate K for
 * new strings. This learns the "texture" of complexity.
 */
KolmogorovNeuralEstimator* kolmogorov_neural_train(
    RandomBitString** samples, size_t* k_estimates, int n_samples,
    int window, int hidden_units, int epochs);

/**
 * kolmogorov_neural_predict — Estimate K(x) using trained network.
 */
double kolmogorov_neural_predict(
    const KolmogorovNeuralEstimator* est, const RandomBitString* s);

void kolmogorov_neural_print(const KolmogorovNeuralEstimator* est);
void kolmogorov_neural_free(KolmogorovNeuralEstimator* est);

/* ══════════════════════════════════════════════════════════════
   L6-L7: Canonical Problems & Applications
   ══════════════════════════════════════════════════════════════ */

/**
 * ml_randomness_suite — Run the full ML-enhanced randomness
 * analysis suite. Combines classical tests, ML classifier,
 * autoencoder detection, and neural K-estimation.
 */
RandomnessProfile* ml_randomness_suite(const RandomBitString* s);

/**
 * detect_pseudorandom_ml — Distinguish cryptographic PRNG from
 * true RNG using ML features. Key application in RNG certification.
 */
typedef struct {
    int    is_prng;         /* 1 if classified as PRNG */
    double confidence;
    double ensemble_votes;  /* agreement across classifiers */
    char*  evidence;        /* human-readable explanation */
} PRNGDetectionResult;

PRNGDetectionResult* detect_pseudorandom_ml(const RandomBitString* s);
void prng_detection_result_print(const PRNGDetectionResult* r);
void prng_detection_result_free(PRNGDetectionResult* r);

#endif /* RANDOMNESS_ML_H */
