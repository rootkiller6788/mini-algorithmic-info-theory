# Course Tree — mini-kolmogorov-complexity-k

## Prerequisites

```
Turing Machines (L1-L3)
  ├── Universal Turing Machine (L2)
  ├── Computability theory (halting problem, decidable/undecidable)
  └── Strings, alphabets, encodings (L3)

Information Theory (L3-L5)
  ├── Shannon entropy
  ├── Kraft inequality
  ├── Prefix-free codes
  └── Source coding theorem
```

## Dependents (modules that build on this)

```
mini-kolmogorov-complexity-k (THIS MODULE)
  ├── mini-prefix-complexity-machine
  │   └── Prefix-free machines, monotone complexity
  ├── mini-incompressibility-method
  │   └── Proof technique using incompressibility
  ├── mini-minimum-description-length
  │   └── Statistical MDL principle
  ├── mini-solomonoff-universal-prediction
  │   └── Universal induction via K
  ├── mini-chaitin-omega-constant
  │   └── Halting probability Ω
  ├── mini-algorithmic-randomness-ml
  │   └── Martin-Löf randomness theory
  └── mini-resource-bounded-kolmogorov
      └── Time/space-bounded K complexity
```

## Internal Dependencies

```
kolmogorov.h
  ├── string_tools.h (pair encoding, self-delimiting, alphabets)
  ├── compression.h (LZ77/78/ZW, RLE, Huffman, BWT)
  └── entropy.h (Shannon, Rényi, block entropy, JSD)
```
