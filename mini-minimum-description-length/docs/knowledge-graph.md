# Knowledge Graph: mini-minimum-description-length

## L1 ? Definitions

| Topic | Status | Implementation |
|-------|--------|---------------|
| MDL Principle: min_M [L(M) + L(D|M)] | Complete | `include/mdl_core.h` |
| Two-Part Code: L(M) + L(D|M) | Complete | `include/two_part.h`, `src/two_part.c` |
| NML (Normalized Maximum Likelihood) | Complete | `include/nml.h`, `src/nml.c` |
| MDLData array type | Complete | `include/mdl_core.h` struct MDLData |
| TwoPartCode struct | Complete | `include/mdl_core.h` struct TwoPartCode |
| NMLCode struct | Complete | `include/mdl_core.h` struct NMLCode |
| MDLResult (model selection) | Complete | `include/mdl_core.h` struct MDLResult |
| MDLPolynomial struct | Complete | `include/mdl_core.h` struct MDLPolynomial |
| MDLHistogram struct | Complete | `include/mdl_core.h` struct MDLHistogram |
| MDLChangePoint struct | Complete | `include/mdl_core.h` struct MDLChangePoint |
| MDLCluster struct | Complete | `include/mdl_core.h` struct MDLCluster |
| MDLARModel struct | Complete | `include/mdl_core.h` struct MDLARModel |
| MDLCriterion enum | Complete | `include/mdl_core.h` enum MDLCriterion |
| ModelCriteria struct | Complete | `include/mdl_advanced.h` struct ModelCriteria |
| PrequentialResult struct | Complete | `include/mdl_advanced.h` struct PrequentialResult |

## L2 ? Core Concepts

| Topic | Status | Implementation |
|-------|--------|---------------|
| Crude MDL (two-part codes) | Complete | `src/two_part.c` |
| Refined MDL (NML) | Complete | `src/nml.c` |
| Mixture MDL | Complete | `src/mdl_advanced.c` |
| Predictive/prequential MDL | Complete | `src/mdl_advanced.c` |
| Universal integer codes (Elias, Rissanen) | Complete | `src/mdl_core.c` |
| Optimal precision for real parameters | Complete | `src/two_part.c` twopart_encode_real |
| Occam's razor via MDL | Complete | Model selection API |
| Model selection consistency | Complete | `src/mdl_core.c` mdl_consistency_test |

## L3 ? Mathematical Structures

| Topic | Status | Implementation |
|-------|--------|---------------|
| Elias gamma code | Complete | `src/mdl_core.c` mdl_elias_gamma_code |
| Elias delta code | Complete | `src/mdl_core.c` mdl_elias_delta_code |
| Elias omega code | Complete | `src/mdl_core.c` mdl_elias_omega_code |
| Rissanen log* code | Complete | `src/mdl_core.c` mdl_rissanen_logstar_code |
| Fisher information matrix determinants | Complete | `src/nml.c` nml_fisher_information_* |
| Jeffreys prior volume | Complete | `src/nml.c` nml_parametric_complexity_fisher |
| Parametric complexity formula | Complete | `src/nml.c` nml_parametric_complexity_approx |
| NML normalization constant | Complete | `src/nml.c` |
| Vandermonde matrix (polynomial fit) | Complete | `src/mdl_core.c` solve_vandermonde |
| Yule-Walker / Levinson-Durbin (AR) | Complete | `src/mdl_core.c` mdl_ar_fit |
| Gaussian elimination with pivoting | Complete | `src/mdl_core.c` solve_vandermonde |
| Stirling approximation for log Gamma | Complete | `src/nml.c` log_gamma_approx |
| KL divergence (Gaussian, Bernoulli, Poisson) | Complete | `src/mdl_core.c` mdl_kl_divergence_* |

## L4 ? Fundamental Theorems

| Theorem | Status | Implementation |
|---------|--------|---------------|
| Rissanen's redundancy bound: (k/2)log(n/(2pi)) | Complete | `src/mdl_core.c` mdl_redundancy_bound |
| Worst-case regret bound | Complete | `src/mdl_core.c` mdl_worst_case_regret |
| NML asymptotic equivalence to Bayes-Jeffreys | Complete | `src/nml.c` nml_bayes_jeffreys_gap |
| NML minimax optimality (Shtarkov 1987) | Complete | Documented in nml.h |
| Consistency of MDL model selection | Complete | `src/mdl_core.c` mdl_consistency_test |
| Redundancy rate convergence | Complete | `src/nml.c` nml_redundancy_rate |

## L5 ? Algorithms/Methods

| Algorithm | Status | Implementation |
|-----------|--------|---------------|
| Two-part Gaussian code | Complete | `src/mdl_core.c` mdl_two_part_gaussian |
| Two-part Bernoulli code | Complete | `src/mdl_core.c` mdl_two_part_bernoulli |
| Two-part Poisson code | Complete | `src/mdl_core.c` mdl_two_part_poisson |
| Two-part Exponential code | Complete | `src/mdl_core.c` mdl_two_part_exponential |
| Two-part Gamma code | Complete | `src/mdl_core.c` mdl_two_part_gamma |
| Two-part Multinomial code | Complete | `src/mdl_core.c` mdl_two_part_multinomial |
| NML for Gaussian (all variants) | Complete | `src/nml.c` |
| NML for Bernoulli (exact for n<=1000) | Complete | `src/nml.c` nml_bernoulli_full |
| NML for Multinomial | Complete | `src/nml.c` nml_multinomial_full |
| NML for Poisson | Complete | `src/nml.c` nml_poisson_full |
| NML for Exponential | Complete | `src/nml.c` nml_exponential_full |
| NML for Linear Regression | Complete | `src/nml.c` nml_linear_regression_full |
| Prequential Gaussian analysis | Complete | `src/mdl_advanced.c` |
| Prequential Bernoulli analysis | Complete | `src/mdl_advanced.c` |
| MML87 approximate message length | Complete | `src/mdl_advanced.c` mdl_mml87_estimate |
| Mixture MDL computation | Complete | `src/mdl_advanced.c` mdl_mixture_mdl |

## L6 ? Canonical Problems

| Problem | Status | Implementation |
|---------|--------|---------------|
| Optimal polynomial degree via MDL | Complete | `src/mdl_core.c` mdl_poly_optimal_degree |
| Optimal histogram bin count via MDL | Complete | `src/mdl_core.c` mdl_histogram_optimal_bins |
| Optimal AR order via MDL | Complete | `src/mdl_core.c` mdl_ar_optimal_order |
| Optimal cluster count via MDL | Complete | `src/mdl_core.c` mdl_cluster_optimal_k |
| MDL model selection (multiple criteria) | Complete | `src/mdl_advanced.c` |
| Decision tree MDL pruning | Complete | `src/mdl_advanced.c` |
| Change point detection via DP | Complete | `src/mdl_core.c` mdl_changepoint_detect |

## L7 ? Applications

| Application | Status | Implementation |
|-------------|--------|---------------|
| AIC/BIC/MDL comparison | Complete | `src/mdl_advanced.c` mdl_criteria_compute |
| MDL for change detection (GPS signals) | Complete | `src/mdl_advanced.c` mdl_detect_breaks |
| MDL for image segmentation | Complete | `src/mdl_advanced.c` mdl_segmentation_cost |
| MDL randomness analysis | Complete | `examples/example_randomness_analysis.c` |
| MDL compression demonstration | Complete | `examples/example_compression_demo.c` |
| Model complexity profiling | Complete | `examples/example_complexity_profile.c` |

## L8 ? Advanced Topics

| Topic | Status | Implementation |
|-------|--------|---------------|
| NML statistical power analysis | Complete | `src/nml.c` nml_model_selection_power |
| MML87 (Wallace minimum message length) | Complete | `src/mdl_advanced.c` |
| MDL-MML divergence | Complete | `src/mdl_advanced.c` mdl_mdl_mml_divergence |
| Mixture MDL / model averaging | Complete | `src/mdl_advanced.c` |
| Prequential principle (Dawid 1984) | Complete | `src/mdl_advanced.c` |
| Structural break detection with greedy search | Complete | `src/mdl_advanced.c` mdl_detect_breaks |

## L9 ? Research Frontiers

| Topic | Status | Notes |
|-------|--------|-------|
| Meta-complexity and MDL | Partial | Documented, see knowledge-graph.md |
| MDL for deep learning model selection | Partial | Documented |
| Information bottleneck and MDL | Partial | Documented |
