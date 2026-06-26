# Knowledge Graph: Solomonoff Universal Prediction

## L1 - Definitions
- algorithmic probability M(x), binary_string_t, program_t
- Kolmogorov complexity K(x), prefix complexity
- semimeasure, universal monotone machine
- prediction_result_t, convergence_point_t

## L2 - Core Concepts
- Occam's razor formalization
- Convergence of M to true distribution
- Semimeasure property (sum M(x) <= 1)
- Dominance over computable semimeasures
- Prediction-compression duality

## L3 - Math Structures
- Binary string representation (MSB-first packing)
- Universal monotone machine (4-register + memory + output tape)
- Program enumeration tree (shortlex order)
- Probability semimeasure space
- Elias Gamma prefix-free encoding

## L4 - Fundamental Theorems
1. Coding Theorem: K(x) = -log2 M(x) + O(1)
2. Invariance Theorem: |K_U(x) - K_V(x)| <= c_UV
3. Solomonoff Convergence Theorem
4. Kraft Inequality for prefix complexity
5. Chain Rule: K(x,y) = K(x) + K(y|x*) + O(log)
6. Symmetry of Information

## L5 - Algorithms/Methods
- Program enumeration (shortlex)
- Dovetailing execution
- Levin's time-bounded Pt
- Next-bit prediction via M ratios
- Kolmogorov estimation via shortest program
- MDL (Minimum Description Length)
- Prefix-free program encoding

## L6 - Canonical Problems
- Next-bit prediction
- Sequence completion
- Incompressibility testing
- Compression ratio estimation

## L7 - Applications
- Time series prediction (financial, sensor, network)
- Anomaly detection (intrusion, fraud, medical)
- Binary classification (spam, malware, bio-sequence)
- Complexity profiling (genome analysis)
- Change point detection (EEG, climate, speech)
- Randomness testing (RNG quality, steganography)
- Pattern discovery (scientific laws, program synthesis)
- Universal compression baseline

## L8 - Advanced Topics
- Chaitin's Omega (halting probability)
- Normalized Information Distance (NID)
- Algorithmic Mutual Information I(x:y)
- Ensemble analysis (top-k program contributions)
- Partial: speed prior, universal search

## L9 - Research Frontiers
- AIXI / Universal AI (Hutter, 2005)
- Resource-bounded Solomonoff induction
- Practical approximation via compression (NCD)
