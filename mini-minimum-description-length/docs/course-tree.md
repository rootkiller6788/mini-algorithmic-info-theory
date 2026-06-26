# Course Tree: mini-minimum-description-length

## Prerequisites

```
Information Theory (entropy, KL divergence, mutual information)
    |
Probability Theory (distributions, MLE, Fisher information)
    |
Statistical Inference (likelihood, Bayesian inference)
    |
    +---> mini-minimum-description-length
```

## Internal Dependency Tree

```
MDLData (basic data structures)
    |
    +---> Universal Integer Codes (Elias, Rissanen)
    |         |
    |         +---> Two-Part Codes (Gaussian, Bernoulli, Poisson, etc.)
    |
    +---> Fisher Information (determinants for common families)
    |         |
    |         +---> NML Codes (parametric complexity + NLL)
    |
    +---> Model Selection (MDLResult, criteria comparison)
    |
    +---> Polynomial MDL (Vandermonde + two-part cost)
    |
    +---> Histogram MDL (bin count optimization)
    |
    +---> Change Point Detection (DP segmentation)
    |
    +---> Clustering MDL (k-means + MDL cost)
    |
    +---> AR Model MDL (Levinson-Durbin + Yule-Walker)
    |
    +---> Advanced: MML, Prequential, Mixture MDL, Structural Breaks
```

## Downstream Dependencies

```
mini-minimum-description-length
    |
    +---> mini-kolmogorov-complexity-k (MDL ~= stochastic complexity)
    +---> mini-incompressibility-method (two-part codes for proofs)
    +---> mini-solomonoff-universal-prediction (universal codes)
    +---> mini-chaitin-omega-constant (NML as universal prior)
```
