# Mini Prefix Complexity Machine

Prefix (self-delimiting) Kolmogorov complexity, monotone complexity, universal a priori probability, and optimal prefix codes.

## Module Status: COMPLETE ✅

- **L1 Definitions**: Complete (10 core definitions)
- **L2 Core Concepts**: Complete (10 concepts)
- **L3 Math Structures**: Complete (6 formal structures)
- **L4 Fundamental Laws**: Complete (11 theorems)
- **L5 Algorithms/Methods**: Complete (12 algorithms)
- **L6 Canonical Problems**: Complete (6 problems with demos/examples)
- **L7 Applications**: Complete (4 applications)
- **L8 Advanced Topics**: Partial (4/8 topics implemented)
- **L9 Research Frontiers**: Partial (documented)

**Score: 16/18** (COMPLETE threshold ≥ 16)

## Core Definitions

| Term | Symbol | Definition |
|------|--------|------------|
| Prefix complexity | K(x) | min{\|p\| : U(p) = x, U is prefix TM} |
| Monotone complexity | Km(x) | min{\|p\| : M(p) outputs x as prefix} |
| Universal a priori probability | m(x) | Σ_{p:U(p)=x} 2^{-\|p\|} |
| Chaitin's Omega | Ω | Σ_{p:U(p) halts} 2^{-\|p\|} |
| Prefix code | — | No codeword is prefix of another |
| Self-delimiting code | x̄ | 1^{\|n\|}0 n_binary x |

## Core Theorems

| Theorem | Formula |
|---------|---------|
| Coding Theorem | K(x) = -log m(x) + O(1) |
| Kraft Inequality | Σ 2^{-l_i} ≤ 1 (necessary & sufficient for prefix code) |
| McMillan's Theorem | Any uniquely decodable code satisfies Kraft |
| Invariance Theorem | \|K_U(x) - K_V(x)\| ≤ c_{U,V} |
| Chain Rule (Km) | Km(xy) ≤ Km(x) + Km(y\|x) + O(1) |
| Plain→Prefix Gap | K(x) ≤ C(x) + 2 log C(x) + O(1) |
| K vs Km | K(x) ≤ Km(x) ≤ K(x) + O(1) |
| Dominance | ∃c>0 ∀x: m(x) ≥ c·μ(x) for any computable semimeasure μ |
| Solomonoff Convergence | Σ_n (M(·\|x^n) - μ(·\|x^n))² < ∞ (μ-a.s.) |

## Core Algorithms

| Algorithm | Year | Complexity |
|-----------|------|------------|
| Huffman coding | 1952 | O(n log n) |
| Shannon-Fano coding | 1948 | O(n log n) |
| Arithmetic coding | 1979 | O(n) |
| Levin universal search | 1973 | O(2^{K(x)}) |
| Elias delta code | 1975 | O(log n) bits |
| Elias gamma code | 1975 | O(log n) bits |
| Golomb coding | 1966 | O(log n) bits for geometric dist. |
| Rice coding | 1979 | O(log n) bits (Golomb with m=2^k) |
| Tunstall coding | 1967 | O(alphabet^k) |
| Solomonoff prediction | 1964 | Not computable (ideal) |

## Classic Problems

1. **Optimal prefix code construction** — Given probabilities, find minimum expected length code
2. **Universal prediction** — Predict next bit of computable sequence
3. **Description complexity estimation** — Bound K(x) via program enumeration
4. **Omega estimation** — Approximate Chaitin's constant from below
5. **Coding Theorem verification** — Check K(x) = -log m(x) + O(1) numerically
6. **Monotone machine simulation** — Run programs on prefix-monotone machines

## Nine-School Curriculum Mapping

| School | Course | Topics |
|--------|--------|--------|
| MIT | 6.841 Advanced Complexity | Kolmogorov, algorithmic information |
| Stanford | CS254 Computational Complexity | Solomonoff induction, prefix complexity |
| Berkeley | CS172/CS278 | Kolmogorov complexity, randomness |
| CMU | 15-455/15-855 | Algorithmic information theory |
| Princeton | COS 522/551 | Prefix complexity, Levin search |
| Caltech | CS 151/154 | Omega, algorithmic information |
| Cambridge | Part II/III Complexity | Prefix machines, monotone complexity |
| Oxford | Computational Complexity | Algorithmic mutual information |
| ETH | 263-4650/252-0400 | Formal prefix machines |

## Building

```bash
cd build
make          # build all (demos, examples, tests, benches)
make test     # build and run tests
make demos    # build demos
make examples # build examples
make benches  # build benchmarks
make clean    # remove built files
```

## File Structure

```
mini-prefix-complexity-machine/
├── README.md              ← This file
├── include/
│   ├── prefix_machine.h       Prefix TM, K(x), self-delimiting, Omega
│   ├── prefix_codes.h         Kraft, Huffman, arithmetic, Golomb/Rice
│   ├── monotone_complexity.h  Km(x), chain rule, Levin search
│   └── universal_distribution.h m(x), Solomonoff, mutual info
├── src/
│   ├── prefix_machine.c       (394 lines)
│   ├── prefix_codes.c         (736 lines)
│   ├── monotone_complexity.c  (872 lines)
│   └── universal_distribution.c (660 lines)
├── tests/                4 test files (assert-based)
├── examples/             3 example programs
├── demos/                3 interactive demos
├── benches/              2 performance benchmarks
├── docs/                 5 documentation files
└── build/
    └── Makefile
```

## Key References

- Li & Vitányi, "An Introduction to Kolmogorov Complexity and Its Applications" (2008)
- Cover & Thomas, "Elements of Information Theory" (2006)
- Chaitin, "A Theory of Program Size Formally Identical to Information Theory" (1975)
- Levin, "Universal Sequential Search Problems" (1973)
- Solomonoff, "A Formal Theory of Inductive Inference" (1964)
- Huffman, "A Method for the Construction of Minimum-Redundancy Codes" (1952)
- Kraft, "A Device for Quantizing, Grouping, and Coding Amplitude Modulated Pulses" (1949)
