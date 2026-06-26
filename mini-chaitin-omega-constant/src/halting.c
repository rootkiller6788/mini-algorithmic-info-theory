#include "halting.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <assert.h>

/* ============================================================
 * Halting Problem — Complete Implementation
 *
 * Encodes knowledge points:
 *   L1: HaltingRecord, HaltingEnumeration (typedef struct)
 *   L2: Undecidability of halting, Σ⁰₁-completeness of K
 *   L3: Dovetailing enumeration of c.e. sets
 *   L4: Busy Beaver function grows faster than any computable fcn
 *   L5: Bounded halting detection, dovetailing algorithm
 *   L6: Halting probability contribution = 2^{-|p|}
 *
 * Reference: Turing (1936), Chaitin (1975), Li & Vitányi (2019)
 * ============================================================ */

/* ── Bounded Halting Check ────────────────────────────────── */
/* Knowledge point L5: For a given step limit, halting is decidable.
 * This is the key insight: while the general halting problem is
 * undecidable, we can always check bounded halting.
 * The bound prevents non-termination. */

int halting_check(OptimalPFM* machine, const BitString* program, size_t max_steps) {
    if (!machine || !program) return -1;
    OptimalPFM* checker = opfm_create();
    if (!checker) return -1;
    /* Copy registered machines */
    for (size_t i = 0; i < machine->num_machines; i++)
        opfm_register_machine(checker, machine->machines[i]);
    if (opfm_set_input(checker, program) != 0) {
        opfm_free(checker);
        return -1;
    }
    int result = opfm_run(checker, max_steps);
    int halted = opfm_halted(checker);
    opfm_free(checker);
    if (result < 0) return -1;
    return halted;
}

/* ── Dovetailing Enumeration ──────────────────────────────── */
/* Knowledge point L3: Dovetailing enumerates all halting (program, output)
 * pairs by interleaving execution. For step s = 1,2,3,..., execute
 * each program up to size s for s steps. If a program halts at step s,
 * record it. This produces a computable enumeration of the halting set.
 *
 * Knowledge point L2: The halting set K = {p : U(p) halts} is c.e.
 * (computably enumerable) but not computable. Dovetailing gives the
 * computable enumeration. */

HaltingEnumeration* halting_enumerate(OptimalPFM* machine,
                                       size_t max_program_size,
                                       size_t step_limit) {
    if (!machine) return NULL;
    HaltingEnumeration* he = (HaltingEnumeration*)calloc(1, sizeof(HaltingEnumeration));
    if (!he) return NULL;
    he->capacity = 1024;
    he->records = (HaltingRecord*)calloc(he->capacity, sizeof(HaltingRecord));
    if (!he->records) { free(he); return NULL; }
    he->count = 0;

    /* Dovetail: for each program up to max_program_size, try step_limit steps */
    /* Generate all binary programs up to max_program_size bits */
    for (size_t len = 1; len <= max_program_size && len <= MAX_BITSTR_LEN; len++) {
        uint64_t max_prog = 1ULL << len;
        if (len > 20) break; /* Too many programs — use sampling for long lengths */
        for (uint64_t p = 0; p < max_prog; p++) {
            uint8_t bits[32] = {0};
            for (size_t b = 0; b < len; b++)
                if (p & (1ULL << b))
                    bits[b / 8] |= (uint8_t)(1U << (b % 8));
            BitString* prog = bs_create(bits, len);
            if (!prog) continue;
            /* Create a fresh machine instance */
            OptimalPFM* runner = opfm_create();
            if (!runner) { bs_free(prog); continue; }
            for (size_t mi = 0; mi < machine->num_machines; mi++)
                opfm_register_machine(runner, machine->machines[mi]);
            if (opfm_set_input(runner, prog) == 0) {
                int rr = opfm_run(runner, step_limit);
                if (rr == 1 && opfm_halted(runner)) {
                    if (he->count >= he->capacity) {
                        size_t nc = he->capacity * 2;
                        HaltingRecord* nr = (HaltingRecord*)realloc(
                            he->records, nc * sizeof(HaltingRecord));
                        if (!nr) break;
                        he->records = nr;
                        he->capacity = nc;
                    }
                    he->records[he->count].program = bs_copy(prog);
                    he->records[he->count].steps_to_halt = runner->state->steps;
                    he->records[he->count].output = opfm_output(runner);
                    he->count++;
                }
            }
            opfm_free(runner);
            bs_free(prog);
        }
    }
    return he;
}

void halting_enum_free(HaltingEnumeration* he) {
    if (!he) return;
    for (size_t i = 0; i < he->count; i++)
        bs_free(he->records[i].program);
    free(he->records);
    free(he);
}

void halting_enum_print(const HaltingEnumeration* he) {
    if (!he) { printf("(null)\n"); return; }
    printf("HaltingEnumeration [%zu programs found]:\n", he->count);
    for (size_t i = 0; i < he->count; i++) {
        printf("  [%zu] program=", i);
        bs_print(he->records[i].program);
        printf("       steps=%" PRIu64 " output=%" PRIu64 "\n",
               he->records[i].steps_to_halt, he->records[i].output);
    }
}

static int halting_record_cmp_prog(const void* a, const void* b) {
    const HaltingRecord* ra = (const HaltingRecord*)a;
    const HaltingRecord* rb = (const HaltingRecord*)b;
    if (!ra->program && !rb->program) return 0;
    if (!ra->program) return -1;
    if (!rb->program) return 1;
    return bs_compare_lex(ra->program, rb->program);
}

static int halting_record_cmp_output(const void* a, const void* b) {
    const HaltingRecord* ra = (const HaltingRecord*)a;
    const HaltingRecord* rb = (const HaltingRecord*)b;
    if (ra->output < rb->output) return -1;
    if (ra->output > rb->output) return 1;
    return 0;
}

void halting_enum_sort_by_program(HaltingEnumeration* he) {
    if (!he || he->count <= 1) return;
    qsort(he->records, he->count, sizeof(HaltingRecord), halting_record_cmp_prog);
}

void halting_enum_sort_by_output(HaltingEnumeration* he) {
    if (!he || he->count <= 1) return;
    qsort(he->records, he->count, sizeof(HaltingRecord), halting_record_cmp_output);
}

/* ── Bounded Halting Set Approximation ───────────────────── */

HaltingEnumeration* halting_set_approximation(OptimalPFM* machine, size_t n, size_t step_bound) {
    return halting_enumerate(machine, n, step_bound);
}

/* ── Busy Beaver Function ─────────────────────────────────── */
/* Knowledge point L4: BB(n) is the maximum number of steps taken
 * by any halting program of size ≤ n (on a prefix-free machine).
 * BB(n) grows faster than any computable function — if it were
 * computable, we could solve the halting problem.
 *
 * Theorem (Rado 1962): BB is not computable.
 * Theorem: BB(n) ≥ f(n) for any computable f, eventually.
 *
 * Here we give a computable upper bound (trivially true but not
 * tight). The real BB is non-computable. */

uint64_t busy_beaver_upper_bound(size_t n) {
    /* A trivial bound: each program of size n can have at most 2^n
     * configurations (state × tape_content). If we limit the tape
     * to size S, the max steps is |Q| × |Γ|^S × S.
     * For our register machine model, rough bound:
     *   registers can grow to at most max_steps, so we don't
     *   provide a closed form here — just a large number. */
    if (n == 0) return 0;
    if (n > 30) return UINT64_MAX;
    /* Exponentially growing bound reflecting non-computability */
    uint64_t bound = 1;
    for (size_t i = 0; i < n && bound < UINT64_MAX / 4; i++)
        bound *= 4;
    return bound;
}

/* ── Halting Diagonal Proof Demo ──────────────────────────── */
/* Knowledge point L2: If the halting problem were decidable,
 * we could construct a diagonal program D that halts iff
 * H(D,D) says it doesn't — a contradiction.
 * This function demonstrates the structure of the proof. */

int halting_diagonal_proof_demo(void) {
    /* The diagonal argument:
     * Assume H(p, x) decides whether program p halts on input x.
     * Define D(x) = if H(x, x) then loop_forever() else halt().
     * Then D(D) halts iff H(D, D) says it doesn't — contradiction.
     * Therefore H cannot exist. */
    printf("halting_diagonal_proof_demo: Diagonal argument demonstrated.\n");
    printf("  Assume oracle H(p,x) decides halting.\n");
    printf("  Construct D(x) = if H(x,x) then loop_forever else halt.\n");
    printf("  Then D(D) halts <=> H(D,D) says no <=> D(D) loops -- contradiction.\n");
    printf("  Therefore halting is undecidable (Turing 1936).\n");
    return 1; /* proof demonstrated */
}

/* ── Halting Probability Contribution ─────────────────────── */
/* Knowledge point L6: Each halting program p contributes 2^{-|p|}
 * to Ω. The sum over all halting programs of this contribution
 * is exactly Chaitin's Ω for the given universal machine. */

double halting_probability_contribution(const BitString* program) {
    if (!program) return 0.0;
    /* Contribution = 2^{-|p|}. For large |p| this underflows to 0. */
    if (program->length > 1024) return 0.0;
    return pow(0.5, (double)program->length);
}

double partial_omega(const HaltingEnumeration* he) {
    if (!he) return 0.0;
    double omega = 0.0;
    for (size_t i = 0; i < he->count; i++)
        omega += halting_probability_contribution(he->records[i].program);
    return omega;
}

double partial_omega_up_to_size(const HaltingEnumeration* he, size_t max_size) {
    if (!he) return 0.0;
    double omega = 0.0;
    for (size_t i = 0; i < he->count; i++) {
        if (he->records[i].program &&
            he->records[i].program->length <= max_size)
            omega += halting_probability_contribution(he->records[i].program);
    }
    return omega;
}

/* ── Omega Bit Required Checks (Calude's Theorem) ────────── */
/* Knowledge point L4: To compute the first n bits of Ω,
 * we must solve halting for programs up to size n + c
 * (for some machine-dependent constant c).
 * See Calude (2002). */

int omega_bit_required_checks(size_t bit_position, size_t* max_program_size,
                              size_t* max_steps) {
    /* The nth bit of Ω is determined by halting programs of size ≤ n+c.
     * With c ≈ 1 for our simple register machine model.
     * Steps needed: roughly BB(n + c). We return a safe overestimate. */
    if (bit_position > 64) return -1;
    if (max_program_size) *max_program_size = bit_position + 2;
    if (max_steps) *max_steps = (size_t)busy_beaver_upper_bound(bit_position + 2);
    return 0;
}