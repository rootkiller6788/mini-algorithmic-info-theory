# Knowledge Graph — mini-kolmogorov-complexity-k

## L1: Definitions (Complete)

| Concept | Definition | C Location |
|---------|-----------|------------|
| KString | Dynamic string type | include/kolmogorov.h |
| KProgram | A description/program for a universal TM | include/kolmogorov.h |
| KUniversalTM | Universal Turing Machine abstraction | include/kolmogorov.h |
| KComplexityProfile | Complexity estimate bundle | include/kolmogorov.h |
| KComplexityType | Plain (C), Prefix (K), Conditional (K(x\|y)) | include/kolmogorov.h |
| KAlphabet | Formal alphabet | include/string_tools.h |
| HuffmanTree | Optimal prefix code tree | include/compression.h |
| FrequencyDist | Empirical probability distribution | include/entropy.h |
| LZ77Token / LZ77State | LZ77 compression state | include/compression.h |
| LZ78Token / LZ78State | LZ78 dictionary compression | include/compression.h |
| LZWState | LZW Welch variant | include/compression.h |
| BinaryString (Lean) | Binary string type | src/kolmogorov.lean |

## L2: Core Concepts (Complete)

| Concept | Implementation |
|---------|---------------|
| Kolmogorov complexity K(x) | k_complexity_upper_bound |
| Conditional complexity K(x\|y) | k_conditional_upper |
| Pair complexity K(x,y) | k_complexity_pair_bound |
| Incompressibility (c-incompressible) | k_is_incompressible |
| Randomness deficiency δ(x) | k_randomness_deficiency_estimate |
| Invariance (description language choice) | k_invariance_constant |
| Prefix-free property | k_is_prefix_free |
| Upper semi-computability of K | k_upper_semi_computable |
| Halting weight / universal a priori probability | k_halting_weight |

## L3: Mathematical Structures (Complete)

| Structure | Implementation |
|-----------|---------------|
| Binary strings, alphabets | KString, KAlphabet |
| Self-delimiting encoding x̄ = 1^{⌊log\|x\|⌋}0⌊log\|x\|⌋x | kstr_self_delimiting |
| Pair encoding ⟨x,y⟩ | kstr_encode_pair / kstr_decode_pair |
| Tuple encoding | kstr_encode_tuple / kstr_decode_tuple |
| Lexicographic enumeration of all strings | kstr_lex_nth / kstr_lex_index |
| Kraft inequality Σ 2^{-l_i} ≤ 1 | k_kraft_sum, k_kraft_to_lengths |
| Prefix-free set characterization | k_is_prefix_free |
| Binary ↔ text conversion | kstr_to_binary, kstr_from_binary |
| Integer ↔ binary encoding | kstr_from_integer, kstr_to_integer |

## L4: Fundamental Theorems (Complete)

| Theorem | Proof/Implementation |
|---------|---------------------|
| Invariance Theorem: K_U(x) ≤ K_V(x) + c | k_invariance_constant |
| Upper Bound: K(x) ≤ \|x\| + O(1) | k_complexity_upper_bound |
| Pair encoding bound | k_complexity_pair_bound |
| Chaitin's incomputability theorem | src/kolmogorov.lean (statement) |
| Coding Theorem: K(x) = -log m(x) + O(1) | src/kolmogorov.lean (statement) |
| Symmetry of Information | k_symmetry_of_info |
| Shannon source coding lower bound | shannon_lower_bound, optimal_code_length |
| AEP (Asymptotic Equipartition Property) | is_typical, typical_set_size |

## L5: Algorithms/Methods (Complete)

| Algorithm | Source |
|-----------|--------|
| LZ77 (Ziv-Lempel 1977) sliding window | src/compression.c |
| LZ78 (Ziv-Lempel 1978) dictionary | src/compression.c |
| LZW (Welch 1984) dynamic dictionary | src/compression.c |
| Run-Length Encoding (RLE) | src/compression.c |
| Huffman coding (1952) optimal prefix | src/compression.c |
| Burrows-Wheeler Transform (1994) | src/compression.c |
| Shannon entropy computation | src/entropy.c |
| Block entropy estimation | src/entropy.c |
| Entropy rate estimation | src/entropy.c |
| Rényi entropy (any α) | src/entropy.c |
| Min-entropy H∞ | src/entropy.c |
| Jensen-Shannon divergence | src/entropy.c |
| Upper semi-computable approximation | src/kolmogorov.c |

## L6: Canonical Problems (Complete)

| Problem | Example |
|---------|---------|
| String complexity profile | examples/example_complexity_profile.c |
| Compression algorithm comparison | examples/example_compression_demo.c |
| Randomness testing via K(x) | examples/example_randomness_analysis.c |
| Prefix-free code validation | tests/test_main.c |
| String incompressibility detection | src/kolmogorov.c |

## L7: Applications (Complete)

| Application | Description | Implementation |
|-------------|-------------|----------------|
| DNA sequence complexity | GC content, codon entropy, coding region detection, complexity score | src/applications.c: dna_complexity_analyze, dna_is_coding_region, dna_gc_skew |
| NCD file clustering | Hierarchical clustering via Normalized Compression Distance | src/applications.c: ncd_distance_matrix, ncd_hierarchical_cluster, ncd_find_nearest |
| Language identification | Entropy profiles with confidence scoring, language distance matrix | src/applications.c: language_profile_build, language_identify, language_distance_matrix |
| Plagiarism / similarity detection | Multi-scale sliding window NCD+JSD similarity | src/applications.c: similarity_score, find_similar_regions |
| Randomness testing for crypto | Entropy + K incompressibility analysis | examples/example_randomness_analysis.c |

## L8: Advanced Topics (Complete)

| Topic | Status | Implementation |
|-------|--------|----------------|
| Algorithmic mutual information K(x:y) | Implemented | src/applications.c: algorithmic_mutual_information |
| Algorithmic conditional complexity | Implemented | src/applications.c: algorithmic_conditional_complexity |
| Preimage complexity bound | Implemented | src/applications.c: preimage_complexity_bound, hash_preimage_complexity |
| Sophistication (Koppel 1987) | Implemented | src/applications.c: sophistication_estimate, two_part_code_length |
| Resource-bounded K^t(x) | Implemented | src/applications.c: time_bounded_K_estimate, space_bounded_K_estimate, resource_gap |
| Prefix vs plain complexity | Implemented | src/applications.c: prefix_plain_difference, plain_complexity_estimate |
| Kraft inequality (formal proof) | Lean-verified | src/kolmogorov.lean: exampleCodeA/B verified + axiom stated |

## L9: Research Frontiers (Partial — implemented)

| Topic | Status | Implementation |
|-------|--------|----------------|
| Minimum Description Length (Rissanen 1978) | Implemented | src/applications.c: mdl_compare_models, mdl_optimal_model_order, normalized_mdl |
| Solomonoff universal induction (1964) | Implemented | src/applications.c: solomonoff_universal_probability, solomonoff_predictive_gain |
| Meta-complexity K(K(x)) | Implemented | src/applications.c: meta_complexity_estimate, meta_meta_complexity_estimate |
| Logical Depth (Bennett 1988) | Implemented | src/applications.c: logical_depth_estimate, meaningful_information |
| Meta-complexity (Lean) | Formally defined | src/kolmogorov.lean: axiom meta_complexity, axiom logical_depth |
