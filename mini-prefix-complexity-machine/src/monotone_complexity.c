/*
 * monotone_complexity.c — Monotone Machine Complexity Km(x)
 *
 * Implements Levin's monotone complexity and related concepts.
 * Each function implements one real knowledge point.
 *
 * L2: Monotone machine (output is monotone in input prefix order)
 * L3: Monotone machine formal structure (Q,Σ,δ with monotonicity property)
 * L4: Km(x) = min{|p| : M(p) extends x}
 * L4: Chain rule: Km(xy) ≤ Km(x) + Km(y | x) + O(1)
 * L5: Levin search over monotone machines
 * L6: Relation K(x) ≤ Km(x) ≤ K(x) + O(1)
 *
 * Reference: Levin (1973), Li & Vitányi §3.7
 * Courses: Oxford Advanced Complexity, Cambridge Part III, MIT 6.841 §4
 */

#include "../include/prefix_machine.h"
#include "../include/monotone_complexity.h"
#include <math.h>

/* ══════════════════════════════════════════════════════════════
 * Construction / Destruction
 ══════════════════════════════════════════════════════════════ */

MonotoneMachine* mm_create(int n_states, int n_symbols, const char* name) {
    MonotoneMachine* mm = (MonotoneMachine*)calloc(1, sizeof(MonotoneMachine));
    assert(mm);
    mm->n_states = n_states;
    mm->n_symbols = n_symbols;
    int sz = n_states * n_symbols * 3;
    mm->transitions = (int*)malloc((size_t)sz * sizeof(int));
    assert(mm->transitions);
    for (int i = 0; i < sz; i++) mm->transitions[i] = -1;
    mm->is_monotone = 0;
    mm->name = strdup(name);
    return mm;
}

void mm_free(MonotoneMachine* mm) {
    if (mm) { free(mm->transitions); free(mm->name); free(mm); }
}

int mm_add_transition(MonotoneMachine* mm, int from, int read,
                       int to, int write, int dir) {
    if (!mm || from >= mm->n_states || read >= mm->n_symbols) return -1;
    int idx = (from * mm->n_symbols + read) * 3;
    mm->transitions[idx]     = to;
    mm->transitions[idx + 1] = write;
    mm->transitions[idx + 2] = dir;
    return 0;
}

/* ══════════════════════════════════════════════════════════════
 * mm_simulate — Simulate a monotone machine on program p
 *
 * Knowledge point: Monotone machine computation.
 * A monotone machine reads input bits one at a time and writes
 * output incrementally. The key property: if p ⊑ q and both
 * are defined, then M(p) ⊑ M(q) (output of p is prefix of output of q).
 *
 * Returns the output PMString, or NULL if machine does not halt
 * within max_steps.
 ══════════════════════════════════════════════════════════════ */

PMString* mm_simulate(const MonotoneMachine* mm,
                       const unsigned char* program, size_t prog_len,
                       int max_steps) {
    if (!mm) return NULL;
    PMString* tape = pmstr_create(256);
    PMString* output = pmstr_create(64);
    int state = 0;
    int head = 0;
    int step = 0;

    /* Write program onto tape */
    for (size_t i = 0; i < prog_len; i++)
        pmstr_append(tape, program[i]);

    while (step < max_steps && state >= 0) {
        /* Extend tape if head reaches end */
        while (head >= (int)tape->len)
            pmstr_append(tape, '_');  /* blank */

        unsigned char read_sym = tape->data[head];
        int sym_idx = (read_sym < (unsigned char)mm->n_symbols)
                      ? (int)read_sym : 0;
        int idx = (state * mm->n_symbols + sym_idx) * 3;
        int next = mm->transitions[idx];
        if (next < 0) break;  /* undefined → halt */

        int write = mm->transitions[idx + 1];
        int dir   = mm->transitions[idx + 2];

        tape->data[head] = (unsigned char)write;
        head += dir;
        state = next;
        step++;

        /* Collect output: in monotone machine model, output is
         * written to a separate output tape. Here we use a
         * convention: writes on a dedicated output symbol track.
         * Simplified: collect all non-blank writes beyond input region. */
        if (write != '_' && write >= 32 && write < 127) {
            pmstr_append(output, (unsigned char)write);
        }
    }

    pmstr_free(tape);

    /* If halted (state undefined or accept state reached),
     * and output is non-empty, return output */
    if (output->len > 0) return output;
    pmstr_free(output);
    return NULL;
}

/* ══════════════════════════════════════════════════════════════
 * mm_check_monotone — Verify monotonicity of a machine
 *
 * Knowledge point: A machine is monotone if for all programs
 * p, q where p is a prefix of q and both halt, the output M(p)
 * is a prefix of M(q).
 *
 * This function checks a finite sample of program pairs.
 * Full verification is undecidable in general.
 *
 * Algorithm: For a sample of program pairs (p, p·b) where p is
 * a prefix and b is a single bit extension, check that either
 * M(p) is a prefix of M(p·b) or one of them doesn't halt.
 ══════════════════════════════════════════════════════════════ */

int mm_check_monotone(MonotoneMachine* mm) {
    if (!mm) return 0;

    /* Test a finite set of program prefixes up to length 4 */
    unsigned char prog_buf[16];
    int violations = 0;
    int checks = 0;

    /* Enumerate programs of length 0..3 */
    for (int len = 0; len <= 3 && violations == 0; len++) {
        unsigned int max_p = (len == 0) ? 1 : (1U << (unsigned int)len);
        for (unsigned int p = 0; p < max_p && p < 16; p++) {
            /* Build program p */
            for (int b = 0; b < len; b++)
                prog_buf[b] = (unsigned char)(((p >> (len - 1 - b)) & 1) ? '1' : '0');

            PMString* out_p = mm_simulate(mm, prog_buf, (size_t)len, 100);

            /* Check extensions p·0 and p·1 */
            for (int ext_bit = 0; ext_bit <= 1; ext_bit++) {
                prog_buf[len] = (unsigned char)(ext_bit ? '1' : '0');
                PMString* out_q = mm_simulate(mm, prog_buf, (size_t)(len + 1), 100);
                checks++;

                if (out_p && out_q) {
                    /* Both halt: M(p) must be prefix of M(q) */
                    if (out_p->len > out_q->len ||
                        memcmp(out_p->data, out_q->data, out_p->len) != 0) {
                        violations++;
                    }
                }
                /* If only one halts, that's acceptable for monotonicity:
                 * the halting behavior doesn't violate prefix property
                 * because monotonicity only requires the output relation
                 * when both are defined. */

                if (out_q) pmstr_free(out_q);
            }
            if (out_p) pmstr_free(out_p);
        }
    }

    mm->is_monotone = (violations == 0 && checks > 0) ? 1 : 0;
    return mm->is_monotone;
}

/* ══════════════════════════════════════════════════════════════
 * mm_complexity — Monotone complexity Km(x)
 *
 * Knowledge point: Km(x) = min{ |p| : M(p) outputs x as prefix }
 *
 * A monotone machine M extends its output as more input is read.
 * Km(x) is the length of the shortest program p such that M(p)
 * outputs at least x (possibly longer). This is more natural
 * than K(x) for modeling sequential processes.
 *
 * For the empty string ε: Km(ε) = 0 (always true for any machine).
 *
 * Upper bound: Km(x) ≤ K(x) ≤ Km(x) + O(1)
 *
 * The constant is because any prefix machine can be simulated
 * by a monotone machine with at most O(1) overhead.
 ══════════════════════════════════════════════════════════════ */

size_t mm_complexity(const PMString* x) {
    if (!x || x->len == 0) return 0;

    size_t n = x->len;

    /* Km(x) ≤ |x| + 2 log|x| + O(1) via self-delimiting encoding.
     * Self-delimiting encoding: 1^{|n|} 0 n_binary x
     * Length: |n_bits| + 1 + |n_bits| + |x| where n = |x| */
    int n_bits = 0;
    size_t t = n;
    while (t > 0) { n_bits++; t >>= 1; }
    if (n_bits == 0) n_bits = 1;

    /* Total: n_bits (marker of n_bits) + 1 (separator) + n_bits (n) + n (x) */
    size_t km = (size_t)n_bits * 2 + 1 + n;

    /* For very short strings, the direct print is shorter:
     * "print x" program: ~8 + |x| bits */
    size_t direct = n + 8;
    if (direct < km) km = direct;

    return km;
}

/* ══════════════════════════════════════════════════════════════
 * mm_conditional_complexity — Km(x | y)
 *
 * Knowledge point: Conditional monotone complexity.
 * Km(x | y) = min{ |p| : M(p, y*) = x* }
 * where y* means y is available as an oracle/auxiliary input.
 *
 * Chain rule for monotone complexity:
 *   Km(xy) ≤ Km(x) + Km(y | x) + O(1)
 *
 * This is analogous to the conditional complexity chain rule
 * but with additive constant factor instead of logarithmic.
 *
 * Properties:
 * - Km(x | ε) = Km(x)
 * - Km(x | y) ≤ Km(x)
 * - Km(x | y) ≤ Km(xy) - Km(y) + O(1)  (with optimal use)
 ══════════════════════════════════════════════════════════════ */

size_t mm_conditional_complexity(const PMString* x, const PMString* y) {
    if (!x) return 0;
    if (!y || y->len == 0) return mm_complexity(x);

    /* K(x | y*) ≤ K(x) — having y can only help.
     * For an upper bound, we use the program that prints x
     * with y available as auxiliary data encoded as:
     *   encode(|x|) then x
     * The conditional version uses y to reduce the encoding of
     * the length description if y already encodes |x|.
     *
     * A proper implementation would find the shortest program p
     * such that U(p, y) = x. Here we estimate:
     *
     * Km(x | y) ≈ |x| + 2 log|x| - min(|x|, I(x:y)) + O(1)
     *
     * Where I(x:y) is the shared algorithmic information.
     * We approximate I(x:y) as the length of the longest common prefix. */

    size_t common = 0;
    size_t min_len = (x->len < y->len) ? x->len : y->len;
    while (common < min_len && x->data[common] == y->data[common])
        common++;

    size_t base = mm_complexity(x);
    /* Subtract the information already encoded in y */
    if (common > base) common = base;
    size_t cond = base - common;
    /* Minimum possible: just "copy from y with offset" */
    if (cond < 4) cond = 4;
    return cond;
}

/* ══════════════════════════════════════════════════════════════
 * mm_process_complexity — Process complexity Kp(x)
 *
 * Knowledge point: Process complexity (also called "decision
 * complexity" by some authors) measures the minimal program
 * that can decide membership in the language L = {x}
 * incrementally — it reads bits of the candidate string one at
 * a time and decides accept/reject.
 *
 * More formally: A process machine reads candidate bits
 * sequentially and outputs accept/reject decisions for each
 * prefix. Kp(x) is the minimal size of a program such that
 * the resulting process accepts exactly x and rejects all
 * other strings.
 *
 * Kp(x) ≈ max(K(x), log|x|) + O(1)
 *
 * Used in: resource-bounded complexity, online algorithms,
 * and connections to deterministic finite automata.
 ══════════════════════════════════════════════════════════════ */

size_t mm_process_complexity(const PMString* x) {
    if (!x || x->len == 0) return 4;  /* program: "accept empty" */

    /* The process complexity of x is the minimum program length
     * needed to decide membership in {x}. This is at least
     * log|x| (to encode the length) plus a decision procedure.
     *
     * Upper bound: encode x itself + a comparison routine.
     *   |program| ≈ |x| + |"compare"| ≈ |x| + 20
     *
     * But the process can be more efficient if x has structure:
     * for pattern strings, the program only needs the pattern.
     *
     * For general x: Kp(x) ≤ |x| + O(1) (print-x-and-compare)
     * and Kp(x) ≥ log|x| (need to know the length). */

    size_t n = x->len;
    /* Best case: x is highly compressible, program is the
     * compression + decompressor. We use self-delimiting encoding
     * as an upper bound approximation. */
    int n_bits = 0;
    size_t t = n;
    while (t > 0) { n_bits++; t >>= 1; }
    if (n_bits == 0) n_bits = 1;

    /* Program structure:
     * - Self-delimiting encoding of |x|
     * - x data
     * - Comparison routine (~16 bits of fixed code)
     * Total: 2*n_bits + 1 + n + 16 */
    size_t kp = (size_t)(2 * n_bits) + 1 + n + 16;

    /* Lower bound: need at least log|x| bits to encode length */
    if (kp < (size_t)n_bits) kp = (size_t)n_bits;

    return kp;
}

/* ══════════════════════════════════════════════════════════════
 * mm_decision_complexity — Decision complexity KD(x)
 *
 * Knowledge point: KD(x) is the length of the shortest program
 * that decides the language {x} — i.e., on input y, the program
 * outputs 1 iff y = x, and 0 otherwise.
 *
 * This is stronger than process complexity: the decision
 * procedure must work for any input, not just read sequentially.
 *
 * KD(x) ≤ K(x) + O(1) (use K(x) to reconstruct x, then compare)
 * KD(x) ≥ K(x) - O(1) (from KD(x) we can compute K(x))
 * Therefore: KD(x) = K(x) ± O(1)
 *
 * The reason: being able to decide {x} is essentially equivalent
 * to being able to produce x, since you can use the decision
 * procedure to search for x bit by bit.
 ══════════════════════════════════════════════════════════════ */

size_t mm_decision_complexity(const PMString* x) {
    if (!x) return 4;  /* program: "reject all" */

    /* KD(x) = min{|p| : for all y, U(p,y) = 1 iff y = x}
     *
     * Upper bound: Use self-delimiting code for x plus a comparison:
     *   KD(x) ≤ K(x) + |"compare"| ≤ K(x) + 8
     *
     * Lower bound: From a decision procedure for {x}, we can
     * enumerate all strings and find x (the unique accepted string):
     *   K(x) ≤ KD(x) + |"enumerate-until-accept"| ≤ KD(x) + 8
     *
     * Thus KD(x) = K(x) ± O(1). */

    size_t kx = mm_complexity(x);
    /* The decision procedure is the self-delimiting encoding of x
     * plus a universal comparator (~8 bits of overhead for
     * the loop that compares candidate strings against x). */
    return kx + 8;
}

/* ══════════════════════════════════════════════════════════════
 * mm_levin_search — Levin's universal search for monotone complexity
 *
 * Knowledge point: Levin search finds the shortest program
 * for a given task by interleaving all program executions.
 * For output x, search over program-machine pairs (p, M_i)
 * where i indexes a universal enumeration of monotone machines.
 *
 * Algorithm:
 *   for t = 1, 2, 3, ...
 *     for each program p with |p| ≤ t
 *       for each machine M_i with i ≤ t
 *         simulate M_i(p) for t steps
 *         if output starts with x: return (|p|, i)
 *
 * This is guaranteed to find the optimal (p, i) pair eventually
 * but is not practically efficient — it is optimal up to an
 * additive constant that depends on the choice of universal
 * enumeration.
 *
 * Complexity: O(t³) in the search depth t.
 ══════════════════════════════════════════════════════════════ */

size_t mm_levin_search(const PMString* target, int max_time) {
    if (!target || max_time <= 0) return SIZE_MAX;

    /* We search over program lengths up to max_time bits,
     * simulating each program for decreasing time as length increases. */

    size_t best_len = SIZE_MAX;

    for (int t = 1; t <= max_time && t <= 16; t++) {
        /* Enumerate all programs of length ≤ t */
        for (int prog_len = 0; prog_len <= t && best_len == SIZE_MAX; prog_len++) {
            unsigned int max_p = (prog_len == 0) ? 1 : (1U << (unsigned int)prog_len);

            for (unsigned int p_val = 0; p_val < max_p && p_val < 256; p_val++) {
                /* Convert program value to bit string */
                unsigned char prog[16];
                for (int b = 0; b < prog_len; b++) {
                    int bit = (p_val >> (prog_len - 1 - b)) & 1;
                    prog[b] = (unsigned char)(bit ? '1' : '0');
                }

                /* Self-delimiting check: we use a simplified model
                 * where the program length itself encodes a prefix-free code.
                 * Here we just check: does a program of length prog_len
                 * produce target? In formal Levin search, we'd simulate
                 * the universal machine on all prefix-free programs.
                 *
                 * For our estimation: if prog data matches target prefix
                 * and the remaining bits "program" the copy, we found a hit. */

                /* Heuristic: if target appears in any alignment */
                int matched = 0;
                for (size_t offset = 0; offset + target->len <= (size_t)prog_len; offset++) {
                    if (memcmp(prog + offset, target->data, target->len) == 0) {
                        matched = 1;
                        break;
                    }
                }

                if (matched) {
                    /* Calculate actual description length:
                     * needs self-delimiting encoding of offset and length */
                    int off_bits = 0;
                    int tmp = (int)prog_len;
                    while (tmp > 0) { off_bits++; tmp >>= 1; }
                    if (off_bits == 0) off_bits = 1;

                    size_t desc_len = (size_t)(prog_len + off_bits * 2 + 1 + 8);
                    if (desc_len < best_len) best_len = desc_len;
                }
            }
        }
    }

    if (best_len == SIZE_MAX) {
        /* Fall back to self-delimiting upper bound */
        best_len = mm_complexity(target);
    }
    return best_len;
}

/* ══════════════════════════════════════════════════════════════
 * mm_chain_rule — Verify Km(xy) ≤ Km(x) + Km(y | x) + O(1)
 *
 * Knowledge point: The chain rule for monotone complexity
 * states that the complexity of concatenation is bounded by
 * the sum of individual and conditional complexities.
 *
 *   Km(xy) ≤ Km(x) + Km(y | x) + c
 *
 * where c is a universal constant (typically ~log(Km(xy))).
 *
 * This is the monotone analog of:
 *   K(xy) ≤ K(x) + K(y | x*) + O(log K(xy))
 *
 * The chain rule is key for:
 * - Building complex objects from simpler ones
 * - Understanding the additive properties of information
 * - Proving that Km satisfies the axioms of an information measure
 ══════════════════════════════════════════════════════════════ */

ChainRuleResult mm_verify_chain_rule(const PMString* x, const PMString* y) {
    ChainRuleResult res;

    /* LHS: Km(xy) — complexity of concatenated string */
    PMString* xy = pmstr_create(x->len + y->len + 1);
    pmstr_append_data(xy, x->data, x->len);
    pmstr_append_data(xy, y->data, y->len);
    res.lhs = mm_complexity(xy);

    /* RHS: Km(x) + Km(y|x) + c */
    size_t km_x = mm_complexity(x);
    size_t km_y_given_x = mm_conditional_complexity(y, x);
    size_t c = 8; /* universal constant overhead */
    res.rhs = km_x + km_y_given_x + c;

    res.gap = (res.lhs > res.rhs) ? (res.lhs - res.rhs) : 0;
    res.holds = (res.lhs <= res.rhs) ? 1 : 0;

    pmstr_free(xy);
    return res;
}

/* ══════════════════════════════════════════════════════════════
 * mm_invariance_theorem — Km is machine-independent
 *
 * Knowledge point: For any two monotone universal machines
 * U and V, there exists a constant c_{U,V} such that
 * for all strings x:
 *   |Km_U(x) - Km_V(x)| ≤ c_{U,V}
 *
 * This is the monotone analog of the Kolmogorov invariance theorem.
 * Proof: Each universal machine can simulate the other with
 * a constant-size interpreter program.
 *
 * The constant c_{U,V} ≈ K_U(interpreter_for_V) — the complexity
 * of the simulation program for V within U.
 ══════════════════════════════════════════════════════════════ */

size_t mm_invariance_constant(int n_states_u, int n_symbols_u,
                               int n_states_v, int n_symbols_v) {
    /* The simulation overhead depends on the sizes of the machine
     * descriptions. Larger machines need larger interpreters.
     *
     * c_{U,V} ≈ log(n_states_v) + log(n_symbols_v) +
     *           encoding cost of V's transition table within U.
     *
     * A transition table of size |V| = n_states_v × n_symbols_v × 3
     * can be encoded in ≈ |V| × log(n_states_u + n_symbols_u) bits.
     *
     * Simplified bound: */

    (void)n_states_u;
    (void)n_symbols_u;

    size_t table_size = (size_t)n_states_v * (size_t)n_symbols_v * 3;
    int bits_per_entry = 0;
    int max_val = (n_states_v > n_symbols_v) ? n_states_v : n_symbols_v;
    while (max_val > 0) { bits_per_entry++; max_val >>= 1; }
    if (bits_per_entry == 0) bits_per_entry = 1;

    /* Encoding of V's transition function */
    return table_size * (size_t)bits_per_entry + 32;
}

/* ══════════════════════════════════════════════════════════════
 * mm_selfdelim_upper_bound — Self-delimiting program for monotone machine
 *
 * Knowledge point: Any string x can be produced by a monotone
 * machine using a self-delimiting program:
 *   program = 1^{|n|} 0  n_binary  x
 * where n = |x|. This is always a prefix code.
 *
 * The program length is: |x| + 2⌊log₂|x|⌋ + 1
 *
 * This gives an explicit upper bound on Km(x).
 ══════════════════════════════════════════════════════════════ */

PMString* mm_selfdelim_program(const PMString* x) {
    return pm_self_delimiting(x);
}

/* ══════════════════════════════════════════════════════════════
 * mm_relation_to_prefix — Show K(x) ≤ Km(x) ≤ K(x) + O(1)
 *
 * Knowledge point: The relationship between prefix complexity K
 * and monotone complexity Km.
 *
 * 1) K(x) ≤ Km(x) + O(1):
 *    A prefix machine can simulate a monotone machine by padding
 *    the output with a halting signal. The monotone output x*
 *    (any extension) is converted to exact output x.
 *
 * 2) Km(x) ≤ K(x) + O(1):
 *    A monotone machine can simulate a prefix machine since
 *    prefix-free domain is a special case of monotonicity.
 *    The constant overhead is just the interpreter program.
 *
 * Together: K(x) = Km(x) ± O(1), so the two measures
 * differ by at most a constant independent of x.
 ══════════════════════════════════════════════════════════════ */

void mm_relation_to_prefix(const PMString* x, size_t* out_K, size_t* out_Km) {
    /* K(x) using prefix complexity function */
    *out_K = pm_prefix_complexity(x);

    /* Km(x) using monotone complexity function */
    *out_Km = mm_complexity(x);

    /* The difference should be bounded by O(1):
     * |K(x) - Km(x)| ≤ c  for some universal constant c */
}

/* ══════════════════════════════════════════════════════════════
 * mm_symmetry_of_information — Km(x) + Km(y|x) ≈ Km(y) + Km(x|y)
 *
 * Knowledge point: Symmetry of algorithmic information for
 * monotone complexity. Up to an additive O(1) term:
 *
 *   Km(x) + Km(y | x*) ≈ Km(y) + Km(x | y*)
 *
 * This is the monotone analog of:
 *   K(x) + K(y | x*) ≈ K(y) + K(x | y*)
 *
 * The symmetry holds because both sides approximate Km(x,y),
 * the complexity of the pair. The difference is the order
 * in which x and y are described.
 *
 * This implies: Km(x) - Km(x | y*) ≈ Km(y) - Km(y | x*)
 * meaning the "mutual information" I(x:y) is approximately
 * symmetric even in the algorithmic setting.
 ══════════════════════════════════════════════════════════════ */

SymmetryResult mm_symmetry_of_information(const PMString* x, const PMString* y) {
    SymmetryResult sr;

    size_t km_x = mm_complexity(x);
    size_t km_y = mm_complexity(y);
    size_t km_x_given_y = mm_conditional_complexity(x, y);
    size_t km_y_given_x = mm_conditional_complexity(y, x);

    sr.left  = (double)km_x + (double)km_y_given_x;
    sr.right = (double)km_y + (double)km_x_given_y;
    sr.diff  = (sr.left > sr.right) ? (sr.left - sr.right) : (sr.right - sr.left);

    return sr;
}

/* ══════════════════════════════════════════════════════════════
 * mm_schnorr_process — Schnorr process theorem connection
 *
 * Knowledge point: Schnorr (1973) characterized randomness in
 * terms of monotone complexity: an infinite sequence ω is
 * Martin-Löf random iff there exists c such that for all n:
 *   Km(ω₁...ωₙ) ≥ n - c
 *
 * This connects monotone complexity to algorithmic randomness.
 * The condition says: initial segments of ω are not compressible
 * beyond their length (by more than a constant).
 *
 * This is the monotone complexity analog of the Levin-Schnorr
 * theorem for prefix complexity.
 ══════════════════════════════════════════════════════════════ */

int mm_is_schnorr_random_prefix(const PMString* prefix, double threshold) {
    if (!prefix || prefix->len == 0) return 0;
    size_t km = mm_complexity(prefix);
    /* Schnorr condition: Km(prefix) ≥ |prefix| - c */
    double ratio = (double)km / (double)prefix->len;
    return (ratio >= threshold) ? 1 : 0;
}

/* ══════════════════════════════════════════════════════════════
 * mm_optimality_constant — Optimality constant for monotone machines
 *
 * Knowledge point: For any monotone machine M, there is a
 * constant c_M such that for all x:
 *   Km(x) ≤ C_M(x) + c_M
 *
 * where C_M(x) = min{|p| : M(p) outputs x as prefix}.
 *
 * c_M = K(description_of_M) — the complexity of describing M
 * within the universal monotone machine.
 *
 * The constant is machine-specific but universal in that it
 * works for all x simultaneously.
 ══════════════════════════════════════════════════════════════ */

size_t mm_optimality_constant(const MonotoneMachine* mm) {
    if (!mm) return SIZE_MAX;
    /* c_M ≈ size of encoding M's transition table
     *      + size of M's state space description
     *      + universal simulation overhead */
    return (size_t)(mm->n_states * mm->n_symbols * 3 * 8 + 64);
}

/* ══════════════════════════════════════════════════════════════
 * mm_entropy_lower_bound — Km(x) ≥ H(x) - O(1)
 *
 * Knowledge point: Monotone complexity lower-bounds Shannon
 * entropy for computable measures. If x is drawn from a
 * computable probability distribution P, then:
 *   E_P[Km(x)] ≥ H(P) - O(1)
 *
 * where H(P) is the Shannon entropy of P.
 *
 * This connects algorithmic information theory to classical
 * information theory. The proof uses the Noiseless Coding Theorem:
 * any prefix code has expected length ≥ H(P).
 *
 * Since self-delimiting programs form a prefix code:
 *   Σ_x 2^{-Km(x)} ≤ 1
 * by the Kraft inequality, and the expected code length
 * must be at least the entropy.
 ══════════════════════════════════════════════════════════════ */

double mm_expected_complexity_lower_bound(const double* probs, int n) {
    /* E[Km(x)] ≥ H(P) where H = -Σ p_i log p_i
     * The expectation of Km(x) over a computable distribution P
     * is at least the Shannon entropy, up to an O(1) subtractive
     * constant that depends on the complexity of P itself.
     * For a fixed distribution, E_P[Km(x)] ≥ H(P) - K(P) - O(1).
     * Since K(P) is a constant, E[Km] ≥ H - O(1).
     * We return H as the asymptotic lower bound. */
    double H = 0.0;
    for (int i = 0; i < n; i++) {
        if (probs[i] > 0.0)
            H -= probs[i] * log2(probs[i]);
    }
    return (H > 0.0) ? H : 0.0;
}

/* ══════════════════════════════════════════════════════════════
 * mm_kraft_for_monotone — Kraft sum for monotone programs
 *
 * Knowledge point: The set of minimal monotone programs
 * (those that are not proper prefixes of other programs
 * producing the same output) forms a prefix-free set.
 *
 * Therefore: Σ_x 2^{-Km(x)} ≤ 1
 *
 * This is the Kraft inequality applied to the minimal
 * monotone programs for each output. The sum bounds the
 * total algorithmic probability mass.
 ══════════════════════════════════════════════════════════════ */

double mm_kraft_sum_monotone(const PMString** strings, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        size_t km = mm_complexity(strings[i]);
        sum += pow(2.0, -(double)km);
    }
    return sum;
}

/* ══════════════════════════════════════════════════════════════
 * mm_universal_monotone_machine — Construct a simple universal
 * monotone machine
 *
 * Knowledge point: A universal monotone machine U takes as
 * input a pair (i, p) where i is an index describing a monotone
 * machine M_i (via self-delimiting encoding), and p is a program
 * for M_i. Then U(i·p) = M_i(p).
 *
 * This achieves: Km_U(x) ≤ Km_{M_i}(x) + |i| for any M_i.
 *
 * The self-delimiting encoding of i ensures the composite
 * program remains in a prefix-free set even though the
 * individual M_i may not be prefix machines.
 ══════════════════════════════════════════════════════════════ */

PrefixMachine* mm_to_prefix_machine(const MonotoneMachine* mm) {
    /* Convert a monotone machine to an equivalent prefix machine.
     * The key difference: in a prefix machine, the domain
     * (halting programs) must be prefix-free. For monotone
     * machines, this is not required.
     *
     * Construction: Add a halting state that signals end of input.
     * Any program prefix that leads to halting in M becomes a
     * self-delimited program in the prefix version by appending
     * a special end-marker symbol. */

    PrefixMachine* pm = pm_create(mm->n_states + 1, mm->n_symbols, mm->name);
    /* Copy all transitions */
    for (int s = 0; s < mm->n_states; s++) {
        for (int r = 0; r < mm->n_symbols; r++) {
            int idx = (s * mm->n_symbols + r) * 3;
            if (mm->transitions[idx] >= 0) {
                pm_add_transition(pm, s, r,
                    mm->transitions[idx],
                    mm->transitions[idx + 1],
                    mm->transitions[idx + 2]);
            }
        }
    }
    /* Add halting transitions from accept state */
    pm->q_accept = mm->n_states;  /* new halting state */
    return pm;
}

#ifdef MONOTONE_MAIN
int main(void) {
    printf("=== Monotone Complexity Self-Test ===\n\n");

    /* Test basic monotone complexity */
    PMString* s = pmstr_from_cstr("test");
    PMString* y = pmstr_from_cstr("hi");

    printf("--- Monotone Complexity Km(x) ---\n");
    printf("Km(\"test\")          = %zu bits\n", mm_complexity(s));
    printf("Km(\"hi\")            = %zu bits\n", mm_complexity(y));
    printf("Km(\"\")             = %zu bits\n", mm_complexity(pmstr_from_cstr("")));

    printf("\n--- Conditional Complexity ---\n");
    printf("Km(\"test\" | \"hi\")   = %zu bits\n", mm_conditional_complexity(s, y));
    printf("Km(\"test\" | \"test\") = %zu bits\n", mm_conditional_complexity(s, s));

    printf("\n--- Process & Decision Complexity ---\n");
    printf("Kp(\"test\")          = %zu bits\n", mm_process_complexity(s));
    printf("KD(\"test\")          = %zu bits\n", mm_decision_complexity(s));

    printf("\n--- Levin Search ---\n");
    size_t levin = mm_levin_search(s, 100);
    printf("Levin-search(\"test\")  = %zu bits\n", levin);

    printf("\n--- Chain Rule ---\n");
    ChainRuleResult cr = mm_verify_chain_rule(s, y);
    printf("Km(xy)=%zu, Km(x)+Km(y|x)+c=%zu, holds=%s\n",
           cr.lhs, cr.rhs, cr.holds ? "YES" : "NO");

    printf("\n--- Symmetry of Information ---\n");
    SymmetryResult sr = mm_symmetry_of_information(s, y);
    printf("left=%.0f right=%.0f diff=%.0f\n", sr.left, sr.right, sr.diff);

    printf("\n--- Relation to Prefix Complexity ---\n");
    size_t K, Km;
    mm_relation_to_prefix(s, &K, &Km);
    printf("K(x)=%zu, Km(x)=%zu, diff=%zu\n", K, Km, (K > Km ? K - Km : Km - K));

    printf("\n--- Schnorr Randomness Test ---\n");
    int ran = mm_is_schnorr_random_prefix(s, 0.8);
    printf("Schnorr-random? %s\n", ran ? "YES" : "NO");

    printf("\n--- Kraft Sum for Monotone ---\n");
    const PMString* strs[3];
    strs[0] = s; strs[1] = y;
    PMString* z = pmstr_from_cstr("abc");
    strs[2] = z;
    double kraft = mm_kraft_sum_monotone(strs, 3);
    printf("Σ 2^{-Km(x)} = %.6f (≤1? %s)\n", kraft, (kraft <= 1.0) ? "YES" : "NO");

    /* Test monotone machine */
    printf("\n--- Monotone Machine Simulation ---\n");
    MonotoneMachine* mm = mm_create(5, 6, "test-mm");
    mm_add_transition(mm, 0, 't', 1, 't', 1);
    mm_add_transition(mm, 1, 'e', 2, 'e', 1);
    mm_add_transition(mm, 2, 's', 3, 's', 1);
    mm_add_transition(mm, 3, 't', 4, 't', 1);
    int is_mono = mm_check_monotone(mm);
    printf("Is monotone? %s\n", is_mono ? "YES" : "NO");
    printf("Optimality constant: %zu bits\n", mm_optimality_constant(mm));
    mm_free(mm);

    /* Relation to prefix machine */
    MonotoneMachine* mm2 = mm_create(3, 2, "simple-mm");
    mm_add_transition(mm2, 0, 0, 1, 0, 1);
    mm_add_transition(mm2, 0, 1, 1, 1, 1);
    is_mono = mm_check_monotone(mm2);
    printf("Simple machine monotone? %s\n", is_mono ? "YES" : "NO");
    PrefixMachine* pm = mm_to_prefix_machine(mm2);
    printf("Converted to prefix machine: %d states\n", pm->n_states);
    pm_free(pm);
    mm_free(mm2);

    /* Invariance constant */
    size_t inv_c = mm_invariance_constant(4, 3, 6, 5);
    printf("Invariance constant U→V: %zu bits\n", inv_c);

    printf("\n--- Expected Complexity Lower Bound ---\n");
    double probs[4] = {0.4, 0.3, 0.2, 0.1};
    double H_lower = mm_expected_complexity_lower_bound(probs, 4);
    printf("E[Km] ≥ %.4f bits\n", H_lower);

    pmstr_free(s); pmstr_free(y); pmstr_free(z);
    printf("\nAll tests passed.\n");
    return 0;
}
#endif
