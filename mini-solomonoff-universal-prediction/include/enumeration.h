/**
 * enumeration.h - Program Enumeration and Dovetailing
 *
 * Core infrastructure for enumerating all programs (binary strings)
 * and interleaving their execution via dovetailing. This is the
 * computational engine behind algorithmic probability.
 *
 * Dovetailing Theorem: There exists an effective procedure that
 * enumerates all programs and interleaves their execution such that
 * every halting program eventually receives enough steps to halt.
 *
 * Reference: Li & Vitanyi, Section 4.3; Levin, 1973.
 * Curriculum: MIT 6.840, CMU 15-751
 */

#ifndef ENUMERATION_H
#define ENUMERATION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "solomonoff.h"
#include "universal_machine.h"

/* Dovetailing schedule: program i gets a step when global_step % (1 << priority_class(i)) == 0.
   priority_class(i) = floor(log2(i+1)) so shorter programs get more steps. */

#define ENUM_MAX_DEPTH           32
#define ENUM_MAX_PROGRAMS        (1 << ENUM_MAX_DEPTH)
#define ENUM_DOVETAIL_QUEUE_SIZE 4096

/* Program execution pool entry */
typedef struct {
    binary_string_t program;
    size_t prog_length;
    double weight;
    universal_machine_state_t machine;
    uint64_t steps_given;
    bool halted;
    bool active;
} pool_entry_t;

/* Enumerate all binary strings up to length max_depth in shortlex order */
typedef struct {
    size_t current_length;
    uint64_t current_index;       /* Index within current length: 0..2^len-1 */
    size_t max_depth;
    uint64_t total_enumerated;
    bool exhausted;
} program_enumerator_t;

/* Results: (output, weight) pairs for programs that produced output */
typedef struct {
    binary_string_t output;
    double weight;
    size_t prog_length;
    uint64_t steps;
} dovetail_result_t;

/* Dovetailing execution manager */
typedef struct {
    pool_entry_t *pool;
    size_t pool_size;
    size_t pool_capacity;
    uint64_t global_step;
    uint64_t max_total_steps;
    double total_halting_probability;
    size_t num_halted;
    size_t num_results;
    dovetail_result_t *results;
    size_t results_capacity;
} dovetail_manager_t;

/* ---- Program Enumerator API ---- */
void enumerator_init(program_enumerator_t *e, size_t max_depth);
bool enumerator_next(program_enumerator_t *e, binary_string_t *program, size_t *length);
void enumerator_reset(program_enumerator_t *e);
uint64_t enumerator_count(const program_enumerator_t *e);
bool enumerator_is_exhausted(const program_enumerator_t *e);

/* ---- Dovetailing API ---- */
void dovetail_init(dovetail_manager_t *dm, size_t pool_capacity, uint64_t max_steps);
void dovetail_free(dovetail_manager_t *dm);
int  dovetail_add_program(dovetail_manager_t *dm, const binary_string_t *program,
                          size_t length);
int  dovetail_step(dovetail_manager_t *dm);
void dovetail_run(dovetail_manager_t *dm, size_t depth, uint64_t max_steps);
size_t dovetail_count_halted(const dovetail_manager_t *dm);
double dovetail_total_probability(const dovetail_manager_t *dm);

/* Find programs whose output has x as a prefix */
size_t dovetail_find_matching(dovetail_manager_t *dm, const binary_string_t *x,
                              pool_entry_t **matches, size_t max_matches);

/* Find programs whose output is exactly x */
size_t dovetail_find_exact(dovetail_manager_t *dm, const binary_string_t *x,
                           pool_entry_t **matches, size_t max_matches);

/* Compute M(x) from dovetailing results */
double dovetail_compute_M(const dovetail_manager_t *dm, const binary_string_t *x);

/* Compute K(x) from dovetailing results */
size_t dovetail_compute_K(const dovetail_manager_t *dm, const binary_string_t *x,
                          binary_string_t *shortest_program);

#endif
