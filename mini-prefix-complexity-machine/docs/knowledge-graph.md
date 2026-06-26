# Knowledge Graph — Mini Prefix Complexity Machine

## L1: Definitions

| Definition | C Implementation | Status |
|-----------|-----------------|--------|
| Prefix machine (domain prefix-free) | `PrefixMachine` struct | Complete |
| Prefix complexity K(x) = min{\|p\| : U(p)=x} | `pm_prefix_complexity()` | Complete |
| Monotone machine (prefix-monotone output) | `MonotoneMachine` struct | Complete |
| Monotone complexity Km(x) | `mm_complexity()` | Complete |
| Universal a priori probability m(x) | `UniversalDistribution` struct | Complete |
| Chaitin's Omega Ω = Σ 2^{-\|p\|} | `pm_chaitin_omega_estimate()` | Complete |
| Self-delimiting code | `pm_self_delimiting()` | Complete |
| Prefix code (no codeword is prefix of another) | `PrefixCode` struct | Complete |
| Kraft inequality Σ 2^{-l_i} ≤ 1 | `kraft_sum()`, `kraft_satisfied()` | Complete |
| Solomonoff universal prior M | `ud_solomonoff_prediction()` | Complete |

## L2: Core Concepts

| Concept | Implementation | Status |
|---------|---------------|--------|
| Prefix-free set property | `pm_is_prefix_free()` | Complete |
| Plain vs prefix complexity gap | `pm_plain_to_prefix_gap()` | Complete |
| Coding Theorem K(x) = -log m(x) + O(1) | `pm_coding_theorem_bound()`, `ud_algorithmic_entropy()` | Complete |
| Algorithmic mutual information I(x:y) | `ud_algorithmic_mutual_info()` | Complete |
| Chain rule Km(xy) ≤ Km(x) + Km(y\|x) + O(1) | `mm_verify_chain_rule()` | Complete |
| Invariance theorem | `mm_invariance_constant()` | Complete |
| Symmetry of information | `mm_symmetry_of_information()` | Complete |
| Dominance property m(x) ≥ c·μ(x) | `ud_dominance_constant()` | Complete |
| Conditional complexity K(x\|y) | `mm_conditional_complexity()`, `ud_conditional_probability()` | Complete |
| Optimal prefix codes | Huffman, Shannon-Fano implementations | Complete |

## L3: Mathematical Structures

| Structure | Representation | Status |
|-----------|---------------|--------|
| Prefix Turing machine (Q,Σ,Γ,δ) | `PrefixMachine` with transition table | Complete |
| Monotone machine (Q,Σ,δ) | `MonotoneMachine` with transition table | Complete |
| Code tree (binary tree) | `PCTreeNode` with left/right | Complete |
| Computation configuration | `PMConfig` struct | Complete |
| Universal enumeration | `mm_levin_search()` search over program space | Complete |
| Semimeasure space | `UniversalDistribution` with lower/upper bounds | Complete |

## L4: Fundamental Theorems

| Theorem | Verification | Status |
|---------|-------------|--------|
| Kraft inequality (1949) | `kraft_satisfied()`, `pc_mcmillan_check()` | Complete |
| McMillan's theorem (1956) | `pc_mcmillan_check()` | Complete |
| Coding Theorem (Levin 1974) | `ud_algorithmic_entropy()`, `pm_coding_theorem_bound()` | Complete |
| Invariance theorem | `mm_invariance_constant()` | Complete |
| Chain rule for Km | `mm_verify_chain_rule()` | Complete |
| Symmetry of information | `mm_symmetry_of_information()` | Complete |
| Solomonoff convergence | `ud_convergence_bound()` | Complete |
| K(x) ≤ C(x) + 2 log C(x) + O(1) | `pm_plain_to_prefix_gap()` | Complete |
| K(x) ≤ Km(x) ≤ K(x) + O(1) | `mm_relation_to_prefix()` | Complete |
| Schnorr randomness via Km | `mm_is_schnorr_random_prefix()` | Complete |
| Data processing inequality | `ud_data_processing_bound()` | Complete |

## L5: Algorithms/Methods

| Algorithm | Implementation | Status |
|-----------|---------------|--------|
| Huffman coding (1952) | `huffman_lengths_compute()`, `huffman_code_compute()` | Complete |
| Shannon-Fano coding (1948) | `shannon_fano_code()` | Complete |
| Arithmetic coding | `ArithmeticEncoder` with encode/decode | Complete |
| Levin universal search | `mm_levin_search()` | Complete |
| Universal code construction (Elias) | `pm_elias_gamma_encode()`, `pm_elias_delta_encode()` | Complete |
| Golomb coding | `pc_golomb_lengths()` | Complete |
| Rice coding | `rice_code_length()`, `pc_rice_lengths()` | Complete |
| Tunstall coding | `pc_tunstall_build()` | Complete |
| Block Huffman coding | `pc_block_code_analyze()` | Complete |
| Kraft construction (canonical codes) | `pc_kraft_construct()`, `pc_from_lengths()` | Complete |
| m(x) estimation | `ud_estimate()` | Complete |
| Solomonoff prediction | `ud_solomonoff_prediction()` | Complete |

## L6: Canonical Problems

| Problem | Example/Demo | Status |
|---------|-------------|--------|
| Optimal prefix code construction | `example_selfdelimiting.c` | Complete |
| Universal prediction | `example_universal_dist.c` | Complete |
| Monotone complexity estimation | `example_monotone.c` | Complete |
| Chaitin's Omega estimation | `demo_chaitin_omega.c` | Complete |
| Coding Theorem verification | `demo_coding_theorem.c` | Complete |
| Prefix machine simulation | `demo_prefix_machine.c` | Complete |

## L7: Applications

| Application | Implementation | Status |
|------------|---------------|--------|
| Solomonoff universal induction | `ud_solomonoff_prediction()` | Complete |
| Algorithmic entropy estimation | `ud_algorithmic_entropy()` | Complete |
| Minimal description length principle | `mm_levin_search()` | Complete |
| Prefix codes in data compression | Huffman, Shannon-Fano, arithmetic coding | Complete |

## L8: Advanced Topics

| Topic | Implementation | Status |
|-------|---------------|--------|
| Schnorr randomness characterization | `mm_is_schnorr_random_prefix()` | Complete |
| Levin search optimality | `mm_levin_search()` with time bounds | Complete |
| Algorithmic information convergence | `ud_convergence_bound()` | Partial |
| Monotone vs prefix complexity | `mm_relation_to_prefix()`, `mm_to_prefix_machine()` | Complete |

## L9: Research Frontiers

| Frontier | Status |
|----------|--------|
| Resource-bounded Kolmogorov complexity | Documented only |
| Algorithmic randomness beyond Martin-Löf | Documented only |
| Quantum Kolmogorov complexity | Documented only |
| Meta-complexity and the Minimum Circuit Size Problem | Documented only |
