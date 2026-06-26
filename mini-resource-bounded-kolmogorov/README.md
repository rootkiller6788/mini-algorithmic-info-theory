# mini-resource-bounded-kolmogorov

**Resource-Bounded Kolmogorov Complexity** — mini theory of algorithmic information under computational resource constraints.

## Module Status: COMPLETE ✅

- **L1-L6**: Complete
- **L7**: Complete (3 cryptographic/protocol applications)
- **L8**: Complete (2 advanced topics: AID, meta-complexity)
- **L9**: Partial (meta-complexity adversary documented, research frontiers outlined)

**Code**: `include/` + `src/` = 3410 lines | 49/49 tests pass | `make` compiles clean

---

## Nine-Layer Knowledge Coverage

| Level | Name | Status | Key Artifacts |
|-------|------|--------|---------------|
| **L1** | Definitions | Complete | `RBKProgram`, `RBKComplexity`, `RBKStructureFn`, `RBKRandomnessResult`, `LevinResult`, `AIXIAgent` (6+ structs) |
| **L2** | Core Concepts | Complete | `K^t(x)`, `K^s(x)`, `K(x|y)`, prefix K, joint K, mutual info, Levin search, Solomonoff prior |
| **L3** | Math Structures | Complete | Monotonicity, Kraft inequality, self-delimiting encoding, pair coding, lexicographic enumeration |
| **L4** | Fundamental Laws | Complete | Invariance Theorem, Symmetry of Info, Coding Theorem, Chaitin diagonalization (+ Lean proofs) |
| **L5** | Algorithms/Methods | Complete | Structure function, sufficient statistic, randomness deficiency, MDL, compressibility ratio, incompressibility cert |
| **L6** | Canonical Problems | Complete | Time advantage, space-time tradeoff, constructibility, NCD clustering, Levin inversion |
| **L7** | Applications | Complete | PRG evaluation, OWF hardness, cryptographic weakness detection |
| **L8** | Advanced Topics | Complete | Algorithmic Information Distance, NID, meta-complexity adversary |
| **L9** | Research Frontiers | Partial | Meta-complexity (MKTP) documented; GCT/quantum complexity in docs |

## Core Definitions (L1)

- **Kolmogorov Complexity** K(x): length of shortest program p with U(p) = x
- **Time-Bounded** K^t(x): min{|p| : U(p) outputs x in ≤ t steps}
- **Space-Bounded** K^s(x): min{|p| : U(p) outputs x in ≤ s cells}
- **Conditional** K(x|y): min{|p| : U(p, y) = x}
- **Prefix Complexity**: self-delimiting programs only
- **Structure Function** h_x(k): min{K(x|S) : K(S) ≤ k}
- **Randomness Deficiency** d(x|μ): max_k [k - K(x_{1:k} | |x|)]
- **Universal Distribution** m(x): Σ_{p:U(p)=x} 2^{-|p|}

## Core Theorems (L4)

| Theorem | Formula | Lean |
|---------|---------|------|
| Invariance | |K_U(x) - K_V(x)| ≤ c_{U,V} | ✓ |
| Upper Bound | K(x) ≤ |x| + 2 log |x| + O(1) | ✓ |
| Symmetry of Info | K(x) + K(y|x) = K(y) + K(x|y) + O(log n) | ✓ |
| Coding Theorem | K(x) = -log m(x) + O(1) | partial |
| Kraft Inequality | Σ 2^{-l_i} ≤ 1 for prefix codes | ✓ |
| Chaitin's Diagonalization | ∃x with K(x) > B(|x|) for any computable B | ✓ |

## Core Algorithms (L2, L5)

1. **K^t(x) computation** — Exhaustive search with time bound t
2. **Levin Universal Search** — Optimal algorithm discovery (≤ O(2^{|p*|}) overhead)
3. **Levin Inversion** — Given f and y, find x with f(x)=y
4. **Structure Function** h_x(k) — Algorithmic sufficient statistic
5. **MDL Selection** — argmin_H[L(H) + L(D|H)]
6. **Incompressibility Certificate** — Verify K^t(x) ≥ |x|-k
7. **Solomonoff Prior** — m(x) via program enumeration
8. **AIXI Agent** — Optimal RL via Kolmogorov complexity

## Canonical Problems (L6)

1. Time-bounded K estimation for pseudorandomness testing
2. Levin search for NP witnesses
3. NCD-based clustering (Cilibrasi-Vitanyi)
4. Cryptographic PRG evaluation via compressibility
5. Algorithmic Information Distance computation

## Nine-School Course Mapping

| School | Course | Mapped Topics |
|--------|--------|---------------|
| **MIT** | 6.045 Automata · 6.841 Adv Complexity | Kolmogorov complexity foundations, resource bounds |
| **Stanford** | CS254 Computational Complexity | Algorithmic information, incompressibility method |
| **Berkeley** | CS172 Computability & Complexity | Kolmogorov complexity, prefix codes |
| **CMU** | 15-455/855 Complexity Theory | Resource-bounded K, meta-complexity |
| **Princeton** | COS 522 Computational Complexity | Kolmogorov complexity in crypto, OWF hardness |
| **Caltech** | CS 151/154 Complexity | Algorithmic randomness, martingales |
| **Cambridge** | Part II/III Complexity Theory | Kolmogorov complexity, Levin search |
| **Oxford** | Computational Complexity | Information distance, universal search |
| **ETH** | 263-4650 Adv Complexity · 252-0400 Logic | Resource-bounded K, formalization |

## References

| Text | Author | Year |
|------|--------|------|
| An Introduction to Kolmogorov Complexity | Li & Vitanyi | 2019 |
| Universal Artificial Intelligence | Hutter | 2005 |
| A Complexity Theoretic Approach to Randomness | Sipser | 1983 |
| Clustering by Compression | Cilibrasi & Vitanyi | 2005 |
| A Theory of Program Size | Chaitin | 1975 |
| Universal Sequential Search Problems | Levin | 1973 |

## Build

```
make          # run tests (49/49 pass)
make examples # build examples
make bench    # benchmark
```

## File Inventory

```
include/                   780 lines
  resource_bounded_k.h     203  — Core API: K^t, K^s, structure, randomness
  universal_search.h        68  — Levin search, AIXI, Solomonoff
  complexity_approx.h      261  — LZ, NCD, MDL, entropy, martingale
  program_resource.h       248  — UTM, enumeration, prefix-free, binary utils

src/                      2630 lines
  resource_bounded_k.c     742  — Core implementation
  universal_search.c       503  — Levin search, AIXI, Solomonoff
  complexity_approx.c      910  — LZ77/78, NCD, MDL, entropy, martingale
  program_resource.c       475  — UTM, enumeration, prefix-free, binary
  resource_bounded_k.lean  158  — Formal definitions & theorems

tests/                      (49 tests, all pass)
examples/                   (3 end-to-end demos)
docs/                       (5 knowledge documents)
```