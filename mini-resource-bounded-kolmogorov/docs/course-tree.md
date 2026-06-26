# Course Tree — Resource-Bounded Kolmogorov Complexity

## Prerequisite Dependencies

```
                    TOC (Theory of Computation)
                         |
          +--------------+--------------+
          |              |              |
   Computability    Complexity     Automata
          |              |
          +-------K(x) defined-------+
                          |
               Resource-Bounded K
               (this module)
                          |
          +------+--------+--------+--------+
          |      |        |        |        |
       K^t(x)  K^s(x)  Prefix  Conditional  Joint
          |      |        |        |        |
          +------+---+----+--------+--------+
                     |
        +-----------+-----------+-----------+
        |           |           |           |
    Structure   Randomness    Coding    Incompressibility
    Function    Deficiency    Theorem   Certificates
        |           |           |           |
        +-----------+-----------+-----------+
                     |
           +--------+--------+
           |                 |
    Levin Search      Solomonoff Prior
           |                 |
    AIXI  Agent      Universal Distribution
           |
    Meta-Complexity (L8-L9)
```

## What This Module Depends On

1. **Theory of Computation** (Turing machines, halting problem) — prerequisite
2. **Basic Complexity Theory** (P, NP, polynomial time) — prerequisite
3. **Information Theory** (entropy, coding) — prerequisite
4. **Computability** (recursive functions, UTM) — prerequisite

## What Depends On This Module

1. **Algorithmic Information Theory** — direct extension
2. **Cryptography** — OWF via incompressibility
3. **Machine Learning** — MDL, Solomonoff induction
4. **Clustering / Data Mining** — NCD-based methods
5. **Theoretical AI** — AIXI, universal intelligence
6. **Meta-Complexity** — Complexity of computing K
7. **Computational Randomness** — Martin-Lof tests
8. **Derandomization** — Incompressibility method for lower bounds

## Learning Path

```
1. K(x) definition (L1)
   └─> Why K is uncomputable (Chaitin diagonalization)

2. K^t(x) — making K computable (L2)
   └─> Exhaustive search + upper bounds

3. Prefix-free codes (L3)
   └─> Kraft inequality → Coding theorem

4. Structure function (L5)
   └─> Algorithmic sufficient statistics

5. Levin search (L2)
   └─> Optimal algorithms up to constant factor

6. Applications (L7)
   └─> Cryptography, clustering, AI

7. Meta-complexity (L8-L9)
   └─> Complexity of computing complexity
```