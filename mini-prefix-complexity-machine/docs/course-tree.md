# Course Tree — Mini Prefix Complexity Machine

## Prerequisites

```
mini-complexity-foundations/
  ├── mini-p-np-np-completeness/     (basic complexity classes)
  └── mini-time-hierarchy-theorem/   (hierarchy theorems)

mini-circuit-complexity/
  └── mini-boolean-circuits-model/   (circuit model, if connecting to natural proofs)
```

## Dependency Graph

```
Information Theory Basics
  ├── Shannon entropy H(X)
  ├── Kraft inequality
  └── Source coding theorem
      │
      ▼
Prefix Codes (prefix_codes.c)
  ├── Kraft inequality verification
  ├── Shannon-Fano coding
  ├── Huffman coding (optimal)
  ├── Arithmetic coding
  ├── Golomb/Rice codes
  └── Tunstall codes
      │
      ▼
Prefix Machines (prefix_machine.c)
  ├── Prefix Turing machine model
  ├── Prefix complexity K(x)
  ├── Self-delimiting codes
  ├── Elias gamma/delta codes
  ├── Lebesgue measure on Cantor space
  └── Chaitin's Omega Ω
      │
      ▼
Monotone Complexity (monotone_complexity.c)
  ├── Monotone machine definition
  ├── Km(x) and its properties
  ├── Levin universal search
  ├── Chain rule for Km
  ├── Symmetry of information
  └── Schnorr randomness via Km
      │
      ▼
Universal Distribution (universal_distribution.c)
  ├── m(x) = Σ 2^{-|p|}
  ├── Coding Theorem: K(x) = -log m(x) + O(1)
  ├── Dominance property
  ├── Solomonoff universal prediction
  ├── Algorithmic mutual information
  └── Convergence of universal induction
```

## Forward Dependencies

This module enables:
- **mini-pcp-theorem/** — Algorithmic information in PCP proofs
- **mini-ip-pspace/** — Interactive proofs and randomness
- **mini-communication-complexity/** — Information complexity
- **mini-fine-grained-complexity/** — Resource-bounded Kolmogorov complexity
