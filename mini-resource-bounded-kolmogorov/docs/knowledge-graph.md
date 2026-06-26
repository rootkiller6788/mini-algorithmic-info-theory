# Knowledge Graph — Resource-Bounded Kolmogorov Complexity

## L1: Definitions

| Concept | C Implementation | Lean Formalization |
|---------|------------------|-------------------|
| Kolmogorov complexity K(x) | `RBKComplexity` struct | `K_Le` inductive |
| Time-bounded K^t(x) | `rbk_K_time()` | — |
| Space-bounded K^s(x) | `rbk_K_space()` | — |
| Conditional K(x|y) | `rbk_K_conditional()` | `K_Cond_Le` inductive |
| Prefix complexity | `rbk_K_prefix()` | `PrefixFree` def |
| Structure function h_x(k) | `RBKStructureFn` struct | `StructureFunction` def |
| Randomness deficiency | `RBKRandomnessResult` struct | — |
| Universal distribution m(x) | `rbk_universal_distribution()` | — |
| Incompressibility | `rbk_incompressibility_cert()` | — |
| Program/enumeration | `RBKProgram`, `UTMContext` | `Program` structure |

## L2: Core Concepts

| Concept | Implementation |
|---------|---------------|
| K^t(x) ≤ |x| + c | `rbk_K_time()`, upper bound verification |
| K^s(x) vs K^t(x) tradeoff | `rbk_K_time_space()` |
| Conditional complexity chain rule | `rbk_K_joint()`, `rbk_mutual_information()` |
| Prefix-free programs | `rbk_K_prefix()` |
| Levin universal search | `levin_search()` |
| Solomonoff induction | `solomonoff_prior()` |
| AIXI optimal agent | `aixt_choose_action()` |
| Universal distribution sampling | `universal_distribution_sample()` |

## L3: Mathematical Structures

| Structure | Implementation |
|-----------|---------------|
| Monotonicity K^t ≤ K^{t'} for t ≤ t' | `rbk_check_monotonicity()` |
| Kraft inequality Σ2^{-l_i} ≤ 1 | `rbk_kraft_inequality()`, `resource_kraft_sum()` |
| Self-delimiting encoding | `rbk_self_delimiting_encode/decode()` |
| Pair encoding <x,y> | `rbk_pair_encode/decode()` |
| Lexicographic enumeration | `rbk_lexicographic_enum()` |
| Binary string utilities | `binary_*` functions in program_resource |

## L4: Fundamental Theorems

| Theorem | C Verification | Lean Proof |
|---------|---------------|------------|
| Invariance |K_U(x) - K_V(x)| ≤ c | `rbk_invariance_overhead()` | `invariance_theorem` |
| Upper bound K(x) ≤ |x| + O(1) | `rbk_upper_bound_size()` | `K_upper_bound` |
| Symmetry of information | `rbk_symmetry_of_info()` | `chain_rule_upper` |
| Coding theorem K(x) ≈ -log m(x) | `rbk_coding_theorem_error()` | partial |
| Chaitin diagonalization | `rbk_chaitin_diagonalize()` | — |

## L5: Algorithms/Methods

| Algorithm | Implementation |
|-----------|---------------|
| Structure function computation | `rbk_structure_function()` |
| Sufficient statistic extraction | `rbk_sufficient_statistic()` |
| Randomness deficiency test | `rbk_randomness_deficiency()` |
| MDL model selection | `rbk_mdl_select()`, `mdl_select_best_model()` |
| Compressibility ratio | `rbk_compressibility_ratio()` |
| Incompressibility certificate | `rbk_incompressibility_cert()` |
| Program counting | `rbk_count_programs_producing()`, `resource_count_producing()` |
| LZ77 compression bound | `lz77_compress_bound()` |
| LZ78 complexity | `lz78_complexity()` |
| Entropy estimation | `entropy_profile_compute()`, `block_entropy_series()` |

## L6: Canonical Problems

| Problem | Implementation |
|---------|---------------|
| NCD clustering | `ncd_compute()`, `ncd_cluster()` |
| Levin inversion f(x)=y | `levin_search_inversion()` |
| Time-space tradeoff | `rbk_space_time_tradeoff()` |
| Constructibility checking | `rbk_is_time_constructible()`, `rbk_is_space_constructible()` |
| Martingale betting | `martingale_compressibility_test()`, `martingale_multi_strategy()` |

## L7: Applications

| Application | Implementation |
|-------------|---------------|
| PRG incompressibility evaluation | `rbk_prg_incompressibility_ratio()` |
| Cryptographic weakness detection | `rbk_crypto_weakness_gap()` |
| OWF hardness estimation | `rbk_owf_hardness()` |
| Compression gap test | `compression_gap_test()` |
| Entropy determinism detection | `entropy_determinism_test()` |

## L8: Advanced Topics

| Topic | Implementation |
|-------|---------------|
| Algorithmic Information Distance | `rbk_information_distance()`, `rbk_normalized_information_distance()` |
| Meta-complexity adversary | `rbk_meta_complexity_adversary()` |
| Rate-distortion under K | `rate_distortion_K()` |
| Coding theorem verification | `coding_theorem_verify()` |
| Bayesian complexity posterior | `bayesian_complexity_posterior()` |

## L9: Research Frontiers

| Topic | Status |
|-------|--------|
| Meta-complexity (MKTP) | Defined in Lean, adversary simulation in C |
| GCT (Geometric Complexity Theory) | Documented, no implementation |
| Quantum Kolmogorov complexity | Documented, no implementation |
| Hardness of approximation for K | Documented, no implementation |