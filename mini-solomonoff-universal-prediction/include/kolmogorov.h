/**
 * kolmogorov.h - Kolmogorov Complexity: Plain, Prefix, and Conditional
 *
 * K_U(x) = min{ |p| : U(p) = x }
 *
 * Reference: Li & Vitanyi, 4th ed, 2019.
 * Curriculum: MIT 6.840/18.400, Berkeley CS172, Cambridge Part III
 */

#ifndef KOLMOGOROV_H
#define KOLMOGOROV_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>
#include "solomonoff.h"

size_t kolmogorov_K(const binary_string_t *x, size_t depth, uint64_t max_steps,
                    binary_string_t *shortest_program);
size_t kolmogorov_K_upper(binary_string_t *constructed_program,
                          const binary_string_t *x);
size_t kolmogorov_K_pair(const binary_string_t *x,
                         const binary_string_t *y,
                         size_t depth, uint64_t max_steps);
size_t kolmogorov_conditional_K(const binary_string_t *x,
                                const binary_string_t *y,
                                size_t depth, uint64_t max_steps);
bool kolmogorov_is_incompressible(const binary_string_t *x, size_t c,
                                  size_t depth, uint64_t max_steps);
int kolmogorov_randomness_deficiency(const binary_string_t *x,
                                     size_t depth, uint64_t max_steps);
size_t kolmogorov_test_randomness(const binary_string_t *x,
                                  size_t depth, uint64_t max_steps);
size_t prefix_complexity_K(const binary_string_t *x, size_t depth,
                           uint64_t max_steps,
                           binary_string_t *shortest_prefix_program);
double prefix_complexity_verify_kraft(size_t n, size_t depth, uint64_t max_steps);
double chaitin_omega_lower(size_t depth, uint64_t max_steps);
double algorithmic_mutual_info(const binary_string_t *x,
                               const binary_string_t *y,
                               size_t depth, uint64_t max_steps);
double algorithmic_distance(const binary_string_t *x,
                            const binary_string_t *y,
                            size_t depth, uint64_t max_steps);
double normalized_compression_distance(const binary_string_t *x,
                                       const binary_string_t *y,
                                       size_t depth, uint64_t max_steps);
void coding_theorem_verify(size_t n, size_t depth, uint64_t max_steps,
                           double *max_diff, double *avg_diff, size_t *num_checked);
double levin_coding_theorem_constant(size_t n, size_t depth, uint64_t max_steps);

#endif
