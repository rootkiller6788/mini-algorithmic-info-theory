# SKILL.md — Chaitin's Omega Constant (Ω)

## Knowledge Domain
Algorithmic Information Theory — Chaitin's Omega constant: the halting probability of a prefix-free universal Turing machine.

## Core Concepts Implemented

### 1. Binary Strings and Prefix-Free Sets
- Binary string ADT with lexicographic ordering
- Prefix relation detection
- Prefix-free set construction and validation
- Kraft inequality: Σ 2^{-|s|} ≤ 1
- Maximal prefix-free sets
- Binary tree representation of prefix codes

### 2. Self-Delimiting Turing Machines
- Definition and encoding of self-delimiting programs
- Prefix-free Turing machine simulation
- Program-data boundary detection
- Self-delimiting encoding of natural numbers N → {0,1}*
- Elias delta, gamma, omega codes

### 3. Universal Turing Machine
- Encoding of arbitrary TMs for a fixed UTM
- UTM step-by-step simulation
- Dovetailing: interleaved execution of all programs
- Enumeration of all halting programs
- Halting problem oracle (bounded, approximate)

### 4. Kolmogorov Complexity
- Plain Kolmogorov complexity C(x)
- Prefix-free Kolmogorov complexity K(x)
- Invariance theorem: K₁(x) ≤ K₂(x) + O(1)
- Conditional complexity K(x|y)
- Chain rule: K(x,y) ≤ K(x) + K(y|x) + O(1)
- Incompressibility method
- Algorithmic sufficient statistic

### 5. Halting Probability Ω
- Definition: Ω_U = Σ_{p: U(p) halts} 2^{-|p|}
- Ω is a left-c.e. real number (semicomputable from below)
- Ω is algorithmically random (Martin-Löf random)
- Ω is incompressible: K(Ω↾n) ≥ n - O(1)
- Ω encodes the halting problem: Turing-equivalent to ∅'
- Ω is Σ⁰₁-complete
- Different universal machines yield "Solovay-equivalent" Omegas

### 6. Omega Approximation
- Dovetailing algorithm for lower bounds
- Convergence bounds: Ω - Ω_n ≤ 2^{-n}
- Time-bounded approximations
- Register machine omega for small machines
- Binary digit extraction (non-computable but approximable)

### 7. Algorithmic Randomness
- Martin-Löf randomness definition
- Universal Martin-Löf test
- Schnorr's theorem: ML-random ⇔ no c.e. martingale succeeds
- Ω is 1-random (Martin-Löf random)
- Randomness of finite sequences: deficiency of randomness
- Randomness tests: frequency, runs, universal test

### 8. Solovay Reducibility
- Domination of left-c.e. reals
- Ω-like numbers: Solovay-complete left-c.e. reals
- Calude's characterization of Ω numbers
- Solovay's completeness criterion

### 9. Chaitin's Incompleteness Theorem
- Formal system cannot prove K(x) > n for n beyond a constant
- Information-theoretic incompleteness
- LISP program-size complexity example
- Bound on provable complexity

### 10. Calude's Theorem
- Computing the first n bits of Ω requires solving the halting problem for programs up to size n + c
- The halting problem for programs ≤ n bits is Turing-reducible to Ω↾n
- Exact relationship between bits of Ω and halting information

### 11. Applications and Implications
- Ω as a random oracle: computational power
- Ω and the Busy Beaver function relation
- Ω in inductive inference
- Algorithmic probability P(x) = Σ_{p:U(p)=x} 2^{-|p|}
- Relation P(x) ≈ 2^{-K(x)} (Coding Theorem)

## Implementation Architecture
- `include/` — 10 header files defining all data types and interfaces
- `src/` — 10 implementation files with all algorithms
- `tests/` — 6 test files with unit tests
- `examples/` — 7 example programs demonstrating key concepts
- `benches/` — 2 benchmark programs for performance
- `demos/` — 1 interactive demonstration
- `docs/` — 3 documentation files
- `build/` — Makefile build system

## Key Functions (each = one knowledge point)
Each function encodes exactly one theorem, property, or algorithm from the theory
of Chaitin's omega constant.
