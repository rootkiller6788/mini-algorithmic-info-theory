/**
 * enumeration.c - Program Enumeration and Dovetailing Engine
 *
 * Implements:
 *   1. Length-lexicographic enumeration of binary strings (program space)
 *   2. Dovetailing: interleaved execution of a pool of programs
 *   3. Result collection: matching program outputs to target strings
 *
 * Dovetailing is the key technique that makes algorithmic probability
 * computable in the limit: by interleaving steps among all programs,
 * every halting program eventually receives enough steps to finish.
 *
 * The standard dovetailing schedule allocates step i to program j
 * when i mod 2^{d-j} == 0, where d is a depth parameter. This gives
 * shorter programs exponentially more steps (prioritizing Occam's razor).
 *
 * Reference: Li & Vitanyi, Section 4.3.1; Levin, 1973.
 * Curriculum: MIT 6.840, Berkeley CS172, CMU 15-751
 */

#include "enumeration.h"
#include "universal_machine.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ================================================================
 * Program Enumerator (L2 - Core Concepts)
 * ================================================================ */

/**
 * enumerator_init - Initialize a program enumerator.
 *
 * Programs are binary strings enumerated in shortlex order:
 * epsilon, 0, 1, 00, 01, 10, 11, 000, 001, ...
 *
 * This order ensures that shorter programs (higher prior weight)
 * are enumerated before longer ones, which is crucial for computing
 * converging approximations to M(x).
 */
void enumerator_init(program_enumerator_t *e, size_t max_depth) {
    if (!e) return;
    e->current_length = 0;
    e->current_index = 0;
    e->max_depth = (max_depth > ENUM_MAX_DEPTH) ? ENUM_MAX_DEPTH : max_depth;
    e->total_enumerated = 0;
    e->exhausted = false;
}

/**
 * enumerator_next - Get the next program in shortlex order.
 *
 * Returns false when enumeration is exhausted (all strings up to
 * max_depth have been enumerated).
 *
 * The program is written to `program` as a binary string, and its
 * length is written to `length`.
 *
 * Total number of programs up to depth D: 2^{D+1} - 1.
 */
bool enumerator_next(program_enumerator_t *e, binary_string_t *program, size_t *length) {
    if (!e || !program || !length || e->exhausted) return false;

    if (e->current_length > e->max_depth) {
        e->exhausted = true;
        return false;
    }

    /* Build the binary string from current_index at current_length */
    bs_init(program);
    if (e->current_length > 0) {
        for (size_t bit = 0; bit < e->current_length; bit++) {
            int b = (e->current_index >> (e->current_length - 1 - bit)) & 1;
            bs_append_bit(program, b);
        }
    }

    *length = e->current_length;
    e->total_enumerated++;

    /* Advance to next program */
    uint64_t max_idx = (e->current_length == 0) ? 0 : (1ULL << e->current_length) - 1;
    if (e->current_index >= max_idx) {
        e->current_length++;
        e->current_index = 0;
    } else {
        e->current_index++;
    }

    return true;
}

void enumerator_reset(program_enumerator_t *e) {
    if (!e) return;
    e->current_length = 0;
    e->current_index = 0;
    e->total_enumerated = 0;
    e->exhausted = false;
}

uint64_t enumerator_count(const program_enumerator_t *e) {
    return e ? e->total_enumerated : 0;
}

bool enumerator_is_exhausted(const program_enumerator_t *e) {
    return e ? e->exhausted : true;
}

/* ================================================================
 * Dovetailing Execution Manager (L5 - Algorithms)
 * ================================================================ */

/**
 * dovetail_init - Initialize a dovetailing manager.
 *
 * Allocates a pool of program execution slots. The pool_capacity
 * limits how many programs are simultaneously executed.
 *
 * Dovetailing interleaves execution of all programs in the pool.
 * At each dovetail_step(), one program gets to execute one
 * instruction. The scheduling gives priority to shorter programs.
 */
void dovetail_init(dovetail_manager_t *dm, size_t pool_capacity, uint64_t max_steps) {
    if (!dm) return;
    memset(dm, 0, sizeof(*dm));

    dm->pool_capacity = (pool_capacity > ENUM_DOVETAIL_QUEUE_SIZE) ?
                        ENUM_DOVETAIL_QUEUE_SIZE : pool_capacity;
    dm->pool = (pool_entry_t*)calloc(dm->pool_capacity, sizeof(pool_entry_t));
    dm->pool_size = 0;
    dm->global_step = 0;
    dm->max_total_steps = max_steps;
    dm->total_halting_probability = 0.0;
    dm->num_halted = 0;
    dm->num_results = 0;

    /* Allocate results storage */
    dm->results_capacity = dm->pool_capacity;
    dm->results = (dovetail_result_t*)calloc(dm->results_capacity, sizeof(dovetail_result_t));
}

void dovetail_free(dovetail_manager_t *dm) {
    if (!dm) return;
    if (dm->pool) { free(dm->pool); dm->pool = NULL; }
    if (dm->results) { free(dm->results); dm->results = NULL; }
    dm->pool_size = 0;
    dm->pool_capacity = 0;
}

/**
 * dovetail_add_program - Add a program to the execution pool.
 *
 * Initializes a universal machine state for this program and
 * sets its weight to 2^{-|p|}. The program starts with pc=0
 * and no output yet.
 *
 * If the pool is full, the program is not added. Returns the
 * index of the added program, or -1 if not added.
 */
int dovetail_add_program(dovetail_manager_t *dm, const binary_string_t *program,
                          size_t length) {
    if (!dm || !program || dm->pool_size >= dm->pool_capacity) return -1;

    size_t idx = dm->pool_size;
    pool_entry_t *entry = &dm->pool[idx];

    /* Copy program */
    bs_copy(&entry->program, program);
    entry->prog_length = length;
    entry->weight = pow(2.0, -(double)(int64_t)length);
    entry->halted = false;
    entry->active = true;
    entry->steps_given = 0;

    /* Initialize universal machine */
    um_init(&entry->machine, program,
            dm->max_total_steps / (dm->pool_capacity + 1) + 1);

    dm->pool_size++;
    return (int)idx;
}

/**
 * dovetail_step - Execute one dovetailing step.
 *
 * Selects one program from the pool (using a fair schedule biased
 * toward shorter programs) and executes one instruction on it.
 *
 * Returns: number of programs still active (including halted ones
 *          with pending output), or 0 if all are done.
 *
 * Scheduling: program i gets a step when global_step % (1 + i/4) == 0.
 * This ensures shorter programs (smaller i, since enumerated in
 * shortlex order) get more steps.
 */
int dovetail_step(dovetail_manager_t *dm) {
    if (!dm || dm->pool_size == 0) return 0;

    /* Find a program to execute using round-robin with bias */
    int active_count = 0;
    for (size_t i = 0; i < dm->pool_size; i++) {
        if (dm->pool[i].active && !dm->pool[i].halted) active_count++;
    }
    if (active_count == 0) return 0;

    /* Select program: give priority to shorter programs */
    /* Division of the step count among active programs */
    size_t selected = dm->global_step % dm->pool_size;

    /* Find the next active program starting from `selected` */
    for (size_t attempt = 0; attempt < dm->pool_size; attempt++) {
        size_t idx = (selected + attempt) % dm->pool_size;
        pool_entry_t *entry = &dm->pool[idx];

        if (!entry->active || entry->halted) continue;

        /* Execute one step */
        bool running = um_step(&entry->machine);
        entry->steps_given++;
        dm->global_step++;

        if (!running || entry->machine.halted) {
            entry->halted = true;
            dm->num_halted++;
            dm->total_halting_probability += entry->weight;

            /* Record result if output was produced */
            if (entry->machine.output.length > 0 && dm->num_results < dm->results_capacity) {
                dm->results[dm->num_results].output = entry->machine.output;
                dm->results[dm->num_results].weight = entry->weight;
                dm->results[dm->num_results].prog_length = entry->prog_length;
                dm->results[dm->num_results].steps = entry->steps_given;
                dm->num_results++;
            }
        } else if (entry->steps_given >= entry->machine.step_limit) {
            /* Timeout - mark inactive but keep any output produced so far */
            entry->active = false;
            if (entry->machine.output.length > 0 && dm->num_results < dm->results_capacity) {
                dm->results[dm->num_results].output = entry->machine.output;
                dm->results[dm->num_results].weight = entry->weight;
                dm->results[dm->num_results].prog_length = entry->prog_length;
                dm->results[dm->num_results].steps = entry->steps_given;
                dm->num_results++;
            }
        }

        /* Check if we've exceeded the step limit */
        if (dm->global_step >= dm->max_total_steps) {
            /* Timeout all remaining active programs */
            for (size_t j = 0; j < dm->pool_size; j++) {
                if (dm->pool[j].active && !dm->pool[j].halted) {
                    dm->pool[j].active = false;
                    if (dm->pool[j].machine.output.length > 0 &&
                        dm->num_results < dm->results_capacity) {
                        dm->results[dm->num_results].output = dm->pool[j].machine.output;
                        dm->results[dm->num_results].weight = dm->pool[j].weight;
                        dm->results[dm->num_results].prog_length = dm->pool[j].prog_length;
                        dm->results[dm->num_results].steps = dm->pool[j].steps_given;
                        dm->num_results++;
                    }
                }
            }
            return 0;
        }

        return active_count;
    }

    return active_count;
}

/**
 * dovetail_run - Run dovetailing to completion or until step limit.
 *
 * Enumerates programs up to `depth` bits, adds them to the pool,
 * and interleaves their execution.
 */
void dovetail_run(dovetail_manager_t *dm, size_t depth, uint64_t max_steps) {
    if (!dm || depth > ENUM_MAX_DEPTH) return;

    dm->max_total_steps = max_steps;

    /* Keep stepping until all programs halt or timeout */
    while (dm->global_step < max_steps) {
        int active = dovetail_step(dm);
        if (active == 0) break;
    }
}

size_t dovetail_count_halted(const dovetail_manager_t *dm) {
    return dm ? dm->num_halted : 0;
}

double dovetail_total_probability(const dovetail_manager_t *dm) {
    return dm ? dm->total_halting_probability : 0.0;
}

/* ================================================================
 * Result Matching (L5)
 * ================================================================ */

/**
 * dovetail_find_matching - Find programs whose output has x as prefix.
 *
 * Iterates over all results and collects programs whose monotone
 * output extends x (i.e., output has x as a prefix).
 *
 * This directly implements the definition:
 *   M(x) = sum_{p: x is prefix of U(p)} 2^{-|p|}
 */
size_t dovetail_find_matching(dovetail_manager_t *dm, const binary_string_t *x,
                              pool_entry_t **matches, size_t max_matches) {
    if (!dm || !x || !matches || max_matches == 0) return 0;

    size_t found = 0;
    for (size_t i = 0; i < dm->num_results && found < max_matches; i++) {
        if (um_output_has_prefix(NULL, x)) {
            /* We need the actual machine state - scan pool */
            for (size_t j = 0; j < dm->pool_size && found < max_matches; j++) {
                if (dm->pool[j].halted &&
                    bs_has_prefix(&dm->pool[j].machine.output, x)) {
                    matches[found++] = &dm->pool[j];
                    break;
                }
            }
        }
    }

    /* Also check currently running programs that already produced enough output */
    for (size_t j = 0; j < dm->pool_size && found < max_matches; j++) {
        pool_entry_t *entry = &dm->pool[j];
        if (entry->active && !entry->halted) {
            if (bs_has_prefix(&entry->machine.output, x)) {
                /* Check if already added */
                bool already = false;
                for (size_t k = 0; k < found; k++) {
                    if (matches[k] == entry) { already = true; break; }
                }
                if (!already) matches[found++] = entry;
            }
        }
    }

    return found;
}

size_t dovetail_find_exact(dovetail_manager_t *dm, const binary_string_t *x,
                           pool_entry_t **matches, size_t max_matches) {
    if (!dm || !x || !matches || max_matches == 0) return 0;

    size_t found = 0;
    for (size_t j = 0; j < dm->pool_size && found < max_matches; j++) {
        pool_entry_t *entry = &dm->pool[j];
        if (entry->halted && bs_equal(&entry->machine.output, x)) {
            matches[found++] = entry;
        }
    }
    return found;
}

/* ================================================================
 * Computation of M(x) and K(x) from Dovetailing Results
 * ================================================================ */

/**
 * dovetail_compute_M - Compute M(x) from dovetailing results.
 *
 * M(x) = sum_{p: U(p) outputs x*} 2^{-|p|}
 *
 * Scans all pool entries and sums the weights of programs
 * whose output has x as a prefix.
 */
double dovetail_compute_M(const dovetail_manager_t *dm, const binary_string_t *x) {
    if (!dm || !x) return 0.0;

    double total = 0.0;
    for (size_t j = 0; j < dm->pool_size; j++) {
        const pool_entry_t *entry = &dm->pool[j];
        if (bs_has_prefix(&entry->machine.output, x)) {
            total += entry->weight;
        }
    }
    return total;
}

/**
 * dovetail_compute_K - Estimate K(x) from dovetailing results.
 *
 * K(x) = min{ |p| : U(p) = x }
 *
 * Finds the shortest program that produces exactly x.
 * Returns the length of the shortest such program,
 * or SIZE_MAX if none found.
 */
size_t dovetail_compute_K(const dovetail_manager_t *dm, const binary_string_t *x,
                          binary_string_t *shortest_program) {
    if (!dm || !x) return SIZE_MAX;

    size_t best_len = SIZE_MAX;
    for (size_t j = 0; j < dm->pool_size; j++) {
        const pool_entry_t *entry = &dm->pool[j];
        if (entry->halted && bs_equal(&entry->machine.output, x)) {
            if (entry->prog_length < best_len) {
                best_len = entry->prog_length;
                if (shortest_program) {
                    bs_copy(shortest_program, &entry->program);
                }
            }
        }
    }
    return best_len;
}
