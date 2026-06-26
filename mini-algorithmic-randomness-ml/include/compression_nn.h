/*
 * compression_nn.h — Neural Network Assisted Compression
 *
 * L5-L8: Neural network models for data compression, using deep
 * learning to predict next symbols given context. This implements
 * the connection between prediction and compression:
 *
 *   E[code length] ≥ H(X)  (Shannon, 1948)
 *   Compressor → predictor  (universal prediction, Solomonoff)
 *   Good predictor → good compressor (arithmetic coding)
 *
 * Key insight: A neural network that predicts well gives a good
 * upper bound on Kolmogorov complexity via:
 *   K(x) ≤ |NN| + Σ_i -log₂ P_NN(x_i | context)
 *
 * Reference: Bellard "NNCP" (2019), Mahoney "PAQ" series
 *            Schmidhuber "Learning to Compress"
 * Courses: MIT 6.867, Stanford CS236, Berkeley CS294-158
 */

#ifndef COMPRESSION_NN_H
#define COMPRESSION_NN_H

#include "randomness.h"
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

/* ══════════════════════════════════════════════════════════════
   L1: Neural Compressor Data Structures
   ══════════════════════════════════════════════════════════════ */

/* ── Context Mixer (PAQ-style) ─────────────────────────────── */
typedef struct {
    double*  weights;        /* mixing weights per context */
    int      n_contexts;
    double   learning_rate;
    double   mem;            /* memory parameter (decay) */
} ContextMixer;

/* ── Neural predictor for next symbol ──────────────────────── */
typedef struct {
    double** contexts;       /* context vectors [n_contexts][dim] */
    double*  weights;        /* prediction weights */
    double*  biases;         /* layer biases */
    int      input_dim;
    int      hidden_dim;
    int      output_dim;     /* 256 for byte prediction, 2 for bit */
    int      context_size;   /* how many past symbols to use */
    int      trained_symbols;
} NeuralPredictor;

/* ── Neural compressor state ───────────────────────────────── */
typedef struct {
    NeuralPredictor* predictor;
    ContextMixer*    mixer;
    size_t           input_size;
    size_t           compressed_size;
    int              alphabet_size;   /* 2 (bit) or 256 (byte) */
    void*            arithmetic_state; /* arithmetic encoder state */
} NeuralCompressor;

/* ── Learned probability distribution ──────────────────────── */
typedef struct {
    double* probs;           /* probability vector [alphabet_size] */
    int     alphabet_size;
    double  entropy;         /* -Σ p_i log₂ p_i */
} LearnedDistribution;

/* ── Compression via prediction result ─────────────────────── */
typedef struct {
    size_t  original_bits;
    size_t  compressed_bits;
    double  ratio;
    double  cross_entropy;    /* bits per symbol */
    int     symbols_processed;
} NeuralCompressionResult;

/* ══════════════════════════════════════════════════════════════
   L2: Neural Prediction (core of neural compression)
   ══════════════════════════════════════════════════════════════ */

/**
 * Neural prediction learns P(next_symbol | context) via a
 * feedforward network. The output is a probability distribution
 * over the alphabet, used by the arithmetic coder.
 *
 * Cross-entropy loss = -Σ log P_NN(x_i | context_i) / n
 * This is an upper bound on the entropy rate of the source.
 */

NeuralPredictor* nnp_create(int context_size, int hidden_dim, int alphabet_size);
void nnp_update(NeuralPredictor* np, const unsigned char* context,
                unsigned char next_symbol);
void nnp_predict(const NeuralPredictor* np, const unsigned char* context,
                 double* prob_dist);
int   nnp_predict_argmax(const NeuralPredictor* np, const unsigned char* context);
double nnp_predict_prob(const NeuralPredictor* np, const unsigned char* context,
                        unsigned char symbol);
void nnp_train_online(NeuralPredictor* np, const unsigned char* data,
                      size_t length);
double nnp_cross_entropy(const NeuralPredictor* np, const unsigned char* data,
                         size_t length);
void nnp_free(NeuralPredictor* np);

/* ══════════════════════════════════════════════════════════════
   L3: Context Mixing (PAQ architecture)
   ══════════════════════════════════════════════════════════════ */

ContextMixer* cm_create(int n_contexts, double lr);
void cm_mix(const ContextMixer* cm, const double* context_probs,
            double* mixed_prob, int alphabet_size);
void cm_update(ContextMixer* cm, const double* context_probs,
               unsigned char actual_symbol, int alphabet_size);
void cm_free(ContextMixer* cm);

/* ══════════════════════════════════════════════════════════════
   L4: Prediction-Compression Equivalence Theorem
   ══════════════════════════════════════════════════════════════ */

/**
 * prediction_compression_bound — Theorem: If a predictor achieves
 * cross-entropy H_c, then there exists a compressor with rate ≤ H_c + ε.
 * Conversely, a compressor with rate R gives a predictor with
 * log-loss ≤ R.
 *
 * This function demonstrates the bound numerically.
 */
double prediction_compression_bound(double cross_entropy, double epsilon);

/* ══════════════════════════════════════════════════════════════
   L5: Neural Compression Algorithms
   ══════════════════════════════════════════════════════════════ */

/**
 * neural_compress — Compress data using neural predictor +
 * arithmetic coding. Returns compressed bitstring and stats.
 *
 * This approximates the ideal code length:
 *   L(x) ≈ Σ_i -log₂ P_NN(x_i | x_{i-c}, ..., x_{i-1})
 */
NeuralCompressionResult* neural_compress(const unsigned char* data,
                                          size_t length,
                                          int context_size,
                                          int hidden_units);

NeuralCompressionResult* neural_compress_bits(const RandomBitString* s,
                                               int context_size,
                                               int hidden_units);

/**
 * neural_decompress — Decompress using the same predictor
 * (must have identical architecture and training order).
 */
unsigned char* neural_decompress(const NeuralCompressionResult* result,
                                  int context_size, int hidden_units,
                                  size_t* out_len);

void neural_compression_result_print(const NeuralCompressionResult* r);
void neural_compression_result_free(NeuralCompressionResult* r);

/* ══════════════════════════════════════════════════════════════
   L6: Canonical Problems
   ══════════════════════════════════════════════════════════════ */

/**
 * neural_complexity_upper_bound — Estimate upper bound on K(x)
 * via neural compression:
 *   K(x) ≤ |NN_architecture| + neural_compressed_size(x)
 */
size_t neural_complexity_upper_bound(const RandomBitString* s,
                                      int context_size, int hidden_units);

/**
 * compression_ratio_benchmark — Compare classical vs neural
 * compression on a given string. Demonstrates the gap between
 * Shannon entropy, LZ-based estimates, and neural estimates of K(x).
 */
typedef struct {
    double shannon_ratio;      /* H(X) / 8  per byte */
    double lz77_ratio;
    double lz78_ratio;
    double huffman_ratio;
    double neural_ratio;       /* neural compressor ratio */
    double kolmogorov_est;     /* best estimate of K(x)/|x| */
} CompressionBenchmark;

CompressionBenchmark* compression_benchmark_ml(const unsigned char* data,
                                                size_t length);
void compression_benchmark_print(const CompressionBenchmark* b);
void compression_benchmark_free(CompressionBenchmark* b);

#endif /* COMPRESSION_NN_H */
