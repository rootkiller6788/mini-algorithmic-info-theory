# Course Tree — mini-algorithmic-randomness-ml

## Prerequisites (dependencies on other modules)

```
mini-algorithmic-randomness-ml
├── Requires: mini-complexity-foundations (Turing machines, decidability)
│   └── Used for: no_halting_detector_demonstration
├── Requires: mini-boolean-circuits-model (binary computation model)
│   └── Used for: bit string operations, binary matrix rank test
├── Requires: mini-p-np-np-completeness (complexity classes)
│   └── Used for: PAC learnability bound, VC-dimension of randomness
└── Requires: mini-cook-levin-theorem (NP-completeness)
    └── Used for: understanding incomputability of K(x)
```

## Internal Dependency Tree

```
randomness.h (core definitions)
├── randomness_core.c
│   ├── Bit string utilities
│   ├── Martin-Löf tests
│   ├── Martingale implementation
│   ├── Statistical tests (NIST SP 800-22)
│   └── Randomness profiling
│
├── randomness_ml.h (ML for randomness)
│   └── randomness_ml.c
│       ├── Neural network (feedforward + backprop)
│       ├── Feature extraction (24 features)
│       ├── Universal approximation demo
│       ├── ML classifier (random/non-random)
│       ├── Autoencoder anomaly detection
│       ├── Kolmogorov neural estimator
│       └── PRNG detection
│
├── compression_nn.h (neural compression)
│   └── compression_nn.c
│       ├── Neural predictor (online learning)
│       ├── Context mixer (PAQ architecture)
│       ├── Neural compression
│       ├── K(x) upper bound
│       └── Compression benchmark
│
└── gan_randomness.h (GAN detection)
    └── gan_randomness.c
        ├── GAN architecture (generator + discriminator)
        ├── Standard GAN training
        ├── WGAN training
        ├── GAN discrimination
        ├── DCGAN / cGAN variants
        ├── GAN detection theorem
        ├── Hidden structure detection
        ├── Detection benchmark
        └── Adversarial RNG attack
```

## Linear Progression (recommended reading order)
1. `include/randomness.h` → Martin-Löf random definitions
2. `src/randomness_core.c` → Core algorithms and tests
3. `include/randomness_ml.h` → ML data structures
4. `src/randomness_ml.c` → ML implementations
5. `include/compression_nn.h` → Neural compression structures
6. `src/compression_nn.c` → Neural compression algorithms
7. `include/gan_randomness.h` → GAN structures
8. `src/gan_randomness.c` → GAN training and detection
9. `tests/test_randomness.c` → Integration tests

