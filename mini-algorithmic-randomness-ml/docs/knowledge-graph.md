# Knowledge Graph — mini-algorithmic-randomness-ml

## L1: Definitions
- **Martin-Löf Randomness** (1966): An infinite binary sequence is ML-random iff it passes all effective sequential tests. `include/randomness.h` — `MLTest`, `RandomnessTest`
- **Schnorr Randomness** (1971): ML-randomness characterized by computable martingales. `RandomMartingale`
- **Kolmogorov Complexity** K(x): Length of shortest program outputting x. `KolmogorovNeuralEstimator`
- **GAN** (Goodfellow et al., 2014): Generative Adversarial Network. `GANTrained`, `GANGenerator`, `GANDiscriminator`
- **Neural Predictor**: Learns P(next_symbol | context). `NeuralPredictor`
- **Autoencoder**: Learns low-dimensional manifold of data. `AutoencoderAnomalyDetector`

## L2: Core Concepts
- Effective sequential tests as computably enumerable sequences of open sets
- Martingale characterization: capital unbounded on non-random sequences
- Three-way equivalence: ML-random ⇔ Schnorr-random ⇔ Incompressible
- GAN discriminator as effective randomness test (Goldblum et al., 2019)
- Cross-entropy as upper bound on entropy rate (Shannon, 1948)
- Universal approximation → neural randomness tests (Hornik et al., 1989)

## L3: Mathematical Structures
- **Sequential test** U: c.e. sequence of Σ⁰₁ sets with λ(U_m) ≤ 2^{-m}
- **Supermartingale** d: 2^{<ω} → ℝ^+ with d(σ) = (d(σ0)+d(σ1))/2
- **Neural network** as function approximator: ReLU, Sigmoid, Tanh activations
- **GAN minimax game**: min_G max_D V(D,G) = E[log D(x)] + E[log(1-D(G(z)))]
- **Arithmetic coding** state: low, high, pending bits
- **Context mixing** (PAQ architecture): P_mixed = Σ w_i · P_i

## L4: Fundamental Theorems
- **Martin-Löf Universal Test Theorem** (1966): ∃ universal sequential test
- **Equivalence Theorem**: ML-random ⇔ Schnorr-random ⇔ K-incompressible
- **No Halting Detector** (Turing, 1936): No general randomness decider
- **Universal Approximation** (Hornik et al., 1989): 2-layer NN ≈ any Borel function
- **Prediction-Compression Equivalence** (Solomonoff, 1964): code_length ≈ -log₂ P
- **VC-Dimension of Randomness** (Goldreich, 2008): C_RAND has ∞ VC-dim → not PAC-learnable
- **GAN Detection Theorem**: Discriminator accuracy > 0.5+ε → source is not ML-random

## L5: Algorithms/Methods
- **NIST SP 800-22 Test Battery**: 15 statistical randomness tests
- **Backpropagation** (Rumelhart et al., 1986): Gradient descent for neural networks
- **Xavier/Glorot Initialization** (2010): weight init for deep networks
- **Online Gradient Descent**: Neural predictor weight updates
- **Context Mixing** (Mahoney, 2002): PAQ-style model blending
- **Autoencoder Training**: Reconstruction-based anomaly detection
- **WGAN Training** (Arjovsky et al., 2017): Wasserstein distance + gradient penalty

## L6: Canonical Problems
- **Randomness Profiling**: Comprehensive randomness characterization → `RandomnessProfile`
- **PRNG Detection**: Distinguish algorithmic from true random → `PRNGDetectionResult`
- **Neural K(x) Estimation**: Approximate incomputable Kolmogorov complexity
- **Hidden Structure Detection**: GAN vs classical test comparison → `StructureDetectionComparison`
- **Adversarial RNG Attack**: Generate pseudo-random that fools statistical tests

## L7: Applications
- **RNG Certification**: PRNG detection for cryptographic randomness validation
- **Data Compression**: Neural prediction-based compression (PAQ/NNCP lineage)
- **Anomaly Detection**: Autoencoder-based non-randomness detection
- **Model Verification**: GAN discriminator as adversarial robustness check

## L8: Advanced Topics
- **GAN-Randomness Connection**: Discriminator → Martin-Löf test approximation
- **Wasserstein GAN**: More stable training via Earth Mover distance
- **Conditional GAN**: Entropy-conditioned random generation
- **Adversarial Randomness**: Fooling statistical tests with learned generators
- **Compression Benchmarks**: Shannon vs LZ77 vs LZ78 vs Huffman vs Neural

## L9: Research Frontiers
- **GAN Universality Conjecture**: lim_{capacity→∞} discriminator ≈ universal ML test
- **Neural K-Complexity Bounds**: |NN| + compressed_size(x) ≥ K(x) (conjectured tight)
- **Deep Randomness Theory**: Bridging algorithmic information theory and deep learning
