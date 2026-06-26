#include "../include/prefix_machine.h"
#include "../include/monotone_complexity.h"
#include <stdio.h>
#include <math.h>

static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; } \
    else { printf("FAIL: %s (line %d)\n", msg, __LINE__); } \
} while(0)

int main(void) {
    printf("=== test_monotone ===\n");

    /* ── Construction (use 256 for byte-wide alphabet) ──── */
    MonotoneMachine* mm = mm_create(5, 256, "test-mm");
    TEST_ASSERT(mm != NULL, "mm_create");
    TEST_ASSERT(mm->n_states == 5, "mm n_states");
    TEST_ASSERT(mm->n_symbols == 256, "mm n_symbols");

    int r1 = mm_add_transition(mm, 0, 't', 1, 't', 1);
    TEST_ASSERT(r1 == 0, "add_transition ok for symbol 't'");
    int r2 = mm_add_transition(mm, 1, 'e', 2, 'e', 1);
    TEST_ASSERT(r2 == 0, "add_transition ok for symbol 'e'");

    /* Check the transition was stored at the right slot */
    int idx = (0 * 256 + (int)'t') * 3;
    TEST_ASSERT(mm->transitions[idx] == 1, "transition stored correctly");

    /* ── Monotonicity Check ────────────────────────────── */
    int is_mono = mm_check_monotone(mm);
    TEST_ASSERT(is_mono == 0 || is_mono == 1, "mm_check_monotone valid");

    /* ── Monotone Simulation ───────────────────────────── */
    unsigned char prog[] = {'t', 'e', 's', 't'};
    PMString* sim_out = mm_simulate(mm, prog, 4, 100);
    TEST_ASSERT(1, "mm_simulate runs without crash");
    if (sim_out) pmstr_free(sim_out);
    mm_free(mm);

    /* ── Monotone Complexity ───────────────────────────── */
    PMString* x = pmstr_from_cstr("test");
    size_t km = mm_complexity(x);
    TEST_ASSERT(km > 0, "Km(x) > 0");
    TEST_ASSERT(km >= x->len, "Km(x) >= |x|");

    PMString* empty = pmstr_from_cstr("");
    size_t km_empty = mm_complexity(empty);
    TEST_ASSERT(km_empty == 0, "Km(empty) = 0");

    /* ── Conditional Complexity ────────────────────────── */
    size_t km_cond = mm_conditional_complexity(x, x);
    TEST_ASSERT(km_cond <= km, "Km(x|x) <= Km(x)");
    TEST_ASSERT(km_cond < km + 4, "Km(x|x) is small");

    /* ── Process Complexity ────────────────────────────── */
    size_t kp = mm_process_complexity(x);
    TEST_ASSERT(kp > 0, "Kp(x) > 0");

    /* ── Decision Complexity ───────────────────────────── */
    size_t kd = mm_decision_complexity(x);
    TEST_ASSERT(kd > 0, "KD(x) > 0");
    TEST_ASSERT(kd >= km, "KD(x) >= Km(x)");

    /* ── Levin Search ──────────────────────────────────── */
    size_t levin = mm_levin_search(x, 50);
    TEST_ASSERT(levin > 0, "Levin search finds bound");

    /* ── Chain Rule ────────────────────────────────────── */
    PMString* y = pmstr_from_cstr("hi");
    ChainRuleResult cr = mm_verify_chain_rule(x, y);
    TEST_ASSERT(cr.lhs > 0, "Chain rule LHS > 0");
    TEST_ASSERT(cr.rhs > 0, "Chain rule RHS > 0");
    TEST_ASSERT(cr.gap < 200, "Chain rule gap bounded");

    /* ── Invariance Constant ───────────────────────────── */
    size_t inv_c = mm_invariance_constant(4, 3, 6, 5);
    TEST_ASSERT(inv_c > 0, "Invariance constant > 0");

    /* ── Relation K vs Km ──────────────────────────────── */
    size_t K, Km;
    mm_relation_to_prefix(x, &K, &Km);
    TEST_ASSERT(K > 0, "K(x) > 0");
    TEST_ASSERT(Km > 0, "Km(x) > 0");

    /* ── Symmetry of Information ───────────────────────── */
    SymmetryResult sr = mm_symmetry_of_information(x, y);
    TEST_ASSERT(sr.left > 0.0, "symmetry left > 0");
    TEST_ASSERT(sr.right > 0.0, "symmetry right > 0");
    TEST_ASSERT(sr.diff < 200.0, "symmetry diff bounded");

    /* ── Schnorr Randomness ────────────────────────────── */
    int schnorr = mm_is_schnorr_random_prefix(x, 0.5);
    TEST_ASSERT(schnorr == 0 || schnorr == 1, "Schnorr check valid");

    /* ── Optimality Constant ───────────────────────────── */
    MonotoneMachine* mm2 = mm_create(3, 256, "simple");
    size_t opt_c = mm_optimality_constant(mm2);
    TEST_ASSERT(opt_c > 0, "Optimality constant > 0");

    /* ── Entropy Lower Bound ───────────────────────────── */
    double probs[4] = {0.4, 0.3, 0.2, 0.1};
    double elb = mm_expected_complexity_lower_bound(probs, 4);
    TEST_ASSERT(elb > 0.0, "Entropy lower bound > 0");

    /* ── Kraft Sum for Monotone ────────────────────────── */
    const PMString* strs[3];
    strs[0] = x; strs[1] = y;
    PMString* z = pmstr_from_cstr("abc");
    strs[2] = z;
    double mkraft = mm_kraft_sum_monotone(strs, 3);
    TEST_ASSERT(mkraft <= 1.0 + 1e-6, "Monotone Kraft sum <= 1");

    /* ── Conversion to Prefix Machine ──────────────────── */
    PrefixMachine* pm = mm_to_prefix_machine(mm2);
    TEST_ASSERT(pm != NULL, "mm_to_prefix_machine");
    TEST_ASSERT(pm->n_states == mm2->n_states + 1, "prefix machine has +1 state");
    pm_free(pm);
    mm_free(mm2);

    /* ── Self-Delimiting Program ───────────────────────── */
    PMString* sd_prog = mm_selfdelim_program(x);
    TEST_ASSERT(sd_prog != NULL, "selfdelim program");
    TEST_ASSERT(sd_prog->len > x->len, "program longer than string");
    pmstr_free(sd_prog);

    pmstr_free(x); pmstr_free(y); pmstr_free(z); pmstr_free(empty);

    printf("\n=== Results: %d / %d passed ===\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
