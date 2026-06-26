# Course Tree — mini-chaitin-omega-constant

## Prerequisites

```
mini-theory-of-computation/
├── 0. mini-complexity-foundations/
│   ├── mini-cook-levin-theorem/          (NP-completeness)
│   ├── mini-p-np-np-completeness/        (P vs NP)
│   └── mini-reductions-completeness/     (reduction theory)
│
└── 9. mini-algorithmic-info-theory/
    ├── mini-kolmogorov-complexity-k/      ★ direct prerequisite
    ├── mini-prefix-complexity-machine/    ★ direct prerequisite
    ├── mini-incompressibility-method/     (counting arguments)
    ├── mini-algorithmic-randomness-ml/    ★ co-requisite
    ├── mini-solomonoff-universal-prediction/
    ├── mini-minimum-description-length/
    ├── mini-resource-bounded-kolmogorov/
    └── mini-chaitin-omega-constant/      ← THIS MODULE
```

## Dependency Graph

```
Kolmogorov Complexity K(x) ────┐
Prefix-free Machines ──────────┤
Turing Machines ───────────────┤
Halting Problem ───────────────┼──→ Chaitin Ω
Algorithmic Randomness ────────┤
Left-c.e. Reals ───────────────┤
Solovay Reducibility ──────────┘
```

## What This Module Provides To

- **mini-algorithmic-randomness-ml**: Ω is the canonical ML-random real
- **mini-solomonoff-universal-prediction**: Ω relates to universal distribution
- **mini-minimum-description-length**: Coding theorem connection
- **mini-resource-bounded-kolmogorov**: Ω provides lower bounds
