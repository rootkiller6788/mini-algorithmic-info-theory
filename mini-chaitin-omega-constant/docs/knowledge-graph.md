# Knowledge Graph — mini-chaitin-omega-constant

## L1: Definitions (Complete ✅)

| Entry | C Implementation | Lean Definition | File |
|-------|------------------|-----------------|------|
| BitString | `typedef struct` with bits[] array | `def BinaryString := List Nat` | binary_string.h, omega.lean |
| PrefixFreeSet | `typedef struct` with strings[] | `def isPrefixFree` | binary_string.h, omega.lean |
| PrefixTreeNode | `typedef struct` binary tree | — | binary_string.h |
| RegisterProgram | `typedef struct` with RMInstruction[] | — | prefix_machine.h |
| RMState | `typedef struct` with regs[], ip | — | prefix_machine.h |
| OptimalPFM | `typedef struct` with machines[] | — | prefix_machine.h |
| TMDescription | `typedef struct` with transitions[] | — | universal_tm.h |
| TMConfig | `typedef struct` with tape[], head | — | universal_tm.h |
| UniversalTM | `typedef struct` with library[] | — | universal_tm.h |
| HaltingRecord | `typedef struct` with program, steps | — | halting.h |
| HaltingEnumeration | `typedef struct` with records[] | — | halting.h |
| ChainRuleResult | `typedef struct` | — | kolmogorov.h |
| CodingTheoremResult | `typedef struct` | — | kolmogorov.h |
| AlgorithmicStatistic | `typedef struct` | — | kolmogorov.h |
| DyadicRational | `typedef struct` with numerator, power | — | semicomputable.h |
| LeftCEReal | `typedef struct` with approximations[] | — | semicomputable.h |
| MLTestLevel | `typedef struct` with patterns[] | — | randomness.h |
| MartinLofTest | `typedef struct` with levels[] | — | randomness.h |
| Martingale | `typedef struct` with capital | — | randomness.h |
| OmegaState | `typedef struct` with halting_set | — | omega.h |
| OmegaConvergence | `typedef struct` with lower_bounds[] | — | omega.h |
| SolovayReduction | `typedef struct` with constant_c | — | solovay.h |
| DominanceResult | `typedef struct` | — | solovay.h |
| OmegaNumberCheck | `typedef struct` with characterization | — | calude.h |
| OmegaNumberFamily | `typedef struct` with omega_numbers[] | — | calude.h |

## L2: Core Concepts (Complete ✅)

| Concept | Implementation | File |
|---------|---------------|------|
| Prefix-free property | `pfs_is_prefix_free()`, `bs_is_prefix()` | binary_string.c |
| Self-delimiting encoding | Elias gamma/delta/omega codes | binary_string.c |
| Register machine model | `rm_execute_one()`, `rm_run()` | prefix_machine.c |
| Universal TM | `utm_load_program()`, `utm_run()` | universal_tm.c |
| Undecidability of halting | `halting_diagonal_proof_demo()` | halting.c |
| Dovetailing enumeration | `halting_enumerate()` | halting.c |
| Kolmogorov complexity K(x) | `kolmogorov_prefix_bound()` | kolmogorov.c |
| Plain complexity C(x) | `kolmogorov_plain_bound()` | kolmogorov.c |
| Conditional complexity K(x|y) | `kolmogorov_conditional_bound()` | kolmogorov.c |
| Incompressibility | `kolmogorov_incompressibility()` | kolmogorov.c |
| Algorithmic probability P(x) | `algorithmic_probability()` | kolmogorov.c |
| Left-c.e. reals | `lce_create()`, `lce_add_approximation()` | semicomputable.c |
| Ω = Σ 2^{-|p|} | `omega_iterate()`, `partial_omega()` | omega.c |
| Ω is not computable | `omega_noncomputable_demo()` | omega.c |
| Martin-Löf randomness | `ml_test_create()`, `ml_test_fails()` | randomness.c |
| Solovay reducibility | `solovay_reduction_create()` | solovay.c |
| Ω is Solovay-complete | `solovay_is_complete()` | solovay.c |

## L3: Mathematical Structures (Complete ✅)

| Structure | Implementation | File |
|-----------|---------------|------|
| Binary tree for prefix codes | `PrefixTreeNode`, `ptree_insert()` | binary_string.c |
| Kraft inequality structures | `kraft_sum_from_lengths()` | binary_string.c |
| Register machine (Q,Σ,δ analog) | `RMInstruction`, `RMOpcode` | prefix_machine.c |
| Turing machine formalism | `TMDescription` with δ table | universal_tm.c |
| Self-delimiting encoding | `bs_encode_elias_gamma/delta/omega()` | binary_string.c |
| Dyadic rational representation | `DyadicRational` a/2^k | semicomputable.c |
| Left-c.e. approximating sequence | `LeftCEReal` with q₀ ≤ q₁ ≤ ... | semicomputable.c |

## L4: Fundamental Theorems (Complete ✅)

| Theorem | Code/Lemma | File |
|---------|-----------|------|
| **Kraft Inequality**: Σ 2^{-|s_i|} ≤ 1 | `kraft_sum_from_lengths()`, `pfs_kraft_sum()` | binary_string.c |
| **Invariance Theorem**: K_U(x) ≤ K_V(x) + c | `utm_invariance_demo()` | universal_tm.c |
| **Chaitin Non-Computability**: Ω is not computable | `omega_noncomputable_demo()` | omega.c |
| **Ω ≡_T ∅'**: Ω encodes halting problem | `omega_encodes_halting()` | omega.c |
| **Calude's Theorem**: Ω↾n ≡_T Halting(≤n+c) | `calude_solve_halting_from_omega()`, `calude_compute_omega_from_halting()` | calude.c |
| **Coding Theorem**: K(x) = -log P(x) + O(1) | `coding_theorem_verify()` | kolmogorov.c |
| **Chain Rule**: K(x,y) ≤ K(x)+K(y|x)+O(log) | `kolmogorov_chain_rule_verify()` | kolmogorov.c |
| **ML-Randomness of Ω**: K(Ω↾n) ≥ n - O(1) | `omega_bits_are_random()`, `omega_randomness_verification()` | omega.c, randomness.c |
| **Σ⁰₁-Completeness**: Ω ∈ Σ⁰₁ \ Π⁰₁ | `calude_omega_sigma1_complete_demo()` | calude.c |
| **BB-Ω Equivalence**: BB(n) ≡_T Ω↾n | `calude_bb_from_omega()`, `calude_omega_from_bb()` | calude.c |

## L5: Algorithms/Methods (Complete ✅)

| Algorithm | Implementation | File |
|-----------|---------------|------|
| Elias Gamma Coding | `bs_encode/decode_elias_gamma()` | binary_string.c |
| Elias Delta Coding | `bs_encode/decode_elias_delta()` | binary_string.c |
| Elias Omega Coding | `bs_encode/decode_elias_omega()` | binary_string.c |
| Kraft-McMillan construction | `construct_prefix_free_set()` | binary_string.c |
| Dovetailing enumeration | `halting_enumerate()` | halting.c |
| Bounded halting detection | `halting_check()` | halting.c |
| Kolmogorov complexity search | `kolmogorov_prefix_bound()` | kolmogorov.c |
| Algorithmic probability computation | `algorithmic_probability()` | kolmogorov.c |
| Ω approximation from below | `omega_iterate()`, `omega_approximate()` | omega.c |
| Ω bit extraction | `omega_get_bit()`, `omega_get_bits()` | omega.c |
| Statistical randomness tests | `randomness_frequency_test()`, `runs_test()`, `block_frequency_test()`, `longest_run_test()` | randomness.c |
| Left-c.e. real from machine halting | `lce_from_machine_halting()` | semicomputable.c |
| Σ⁰₁ set ↔ Left-c.e. real | `lce_from_ce_set()`, `lce_to_ce_set()` | semicomputable.c |

## L6: Canonical Problems (Complete ✅)

| Problem | Implementation | File |
|---------|---------------|------|
| Compute Ω approximation | `omega_approximate_to_size()` | omega.c |
| Ω bit sequence extraction | `omega_get_bits()` | omega.c |
| Algorithmic randomness testing | `ml_universal_test_check()`, statistical tests | randomness.c |
| Kolmogorov complexity profiling | `kolmogorov_incompressibility()`, `algorithmic_sufficient_statistic()` | kolmogorov.c |
| Coding theorem verification | `coding_theorem_verify()` | kolmogorov.c |
| Halting probability from enumeration | `partial_omega()` | halting.c |
| Ω from BB and vice versa | `calude_bb_from_omega()`, `calude_omega_from_bb()` | calude.c |

## L7: Applications (Partial+ ✅)

| Application | Implementation | File |
|-------------|---------------|------|
| Randomness extraction | `randomness_extract()` | randomness.c |
| Martingale betting simulation | `martingale_bet()`, `martingale_succeeds()` | randomness.c |
| Ω oracle demo (decide halting) | `calude_omega_oracle_demo()` | calude.c |

## L8: Advanced Topics (Partial+ ✅)

| Topic | Implementation | File |
|-------|---------------|------|
| Solovay reducibility | `solovay_reduction_create()`, `solovay_dominance_check()` | solovay.c |
| Solovay completeness | `solovay_is_complete()`, `solovay_is_omega_like()` | solovay.c |
| Ω-number characterization | `calude_omega_characterization()` | calude.c |
| Omega number family enumeration | `calude_enumerate_omega_numbers()` | calude.c |
| Universal Martin-Löf test | `ml_universal_test_create()` | randomness.c |

## L9: Research Frontiers (Partial ✅)

| Topic | Reference | File |
|-------|-----------|------|
| Calude's Ω bit computation limits | Documented | calude.c |
| Omega numbers and Solovay degrees | Documented | solovay.c |
| Arithmetical hierarchy classification | Documented | calude.c |
| Meta-complexity of Ω | Statement in Lean | omega.lean |
