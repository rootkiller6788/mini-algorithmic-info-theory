/*
 * example_monotone.c — Monotone machine complexity example
 *
 * L2: Monotone machine definition & properties
 * L4: Chain rule: Km(xy) ≤ Km(x) + Km(y|x) + O(1)
 * L5: Levin search
 *
 * Demonstrates monotone complexity estimation and
 * the relationship between K(x) and Km(x).
 */
#include "../include/prefix_machine.h"
#include "../include/monotone_complexity.h"
#include "../include/universal_distribution.h"
#include <stdio.h>
#include <math.h>

static void analyze_monotone(const char* label, const char* str) {
    PMString* s = pmstr_from_cstr(str);
    size_t km = mm_complexity(s);
    size_t kp = mm_process_complexity(s);
    size_t kd = mm_decision_complexity(s);
    size_t K = pm_prefix_complexity(s);
    size_t levin = mm_levin_search(s, 100);

    printf("  %-12s |x|=%2zu  Km=%3zu  K=%3zu  Kp=%3zu  KD=%3zu  Levin=%3zu\n",
           label, s->len, km, K, kp, kd, levin);

    pmstr_free(s);
}

int main(void) {
    printf("=== Monotone Complexity Example ===\n\n");

    printf("--- Monotone Complexity Km(x) ---\n");
    printf("  Km(x) = min{|p| : M(p) outputs x as prefix}\n\n");
    analyze_monotone("empty", "");
    analyze_monotone("a", "a");
    analyze_monotone("ab", "ab");
    analyze_monotone("hello", "hello");
    analyze_monotone("aaaaaa", "aaaaaa");

    printf("\n  Note: K(x) ≈ Km(x). The difference is at most O(1).\n");

    /* ── Chain Rule ────────────────────────────────────────── */
    printf("\n--- Chain Rule Verification ---\n");
    printf("  Km(xy) ≤ Km(x) + Km(y|x) + O(1)\n\n");

    PMString* x = pmstr_from_cstr("hello");
    PMString* y = pmstr_from_cstr("world");
    ChainRuleResult cr = mm_verify_chain_rule(x, y);
    PMString* xy = pmstr_create(x->len + y->len + 1);
    pmstr_append_data(xy, x->data, x->len);
    pmstr_append_data(xy, y->data, y->len);

    printf("  Km(\"helloworld\") = %zu\n", cr.lhs);
    printf("  Km(\"hello\") + Km(\"world\"|\"hello\") + O(1) = %zu\n", cr.rhs);
    printf("  Chain rule holds? %s\n", cr.holds ? "YES" : "NO");
    pmstr_free(xy);

    /* ── Symmetry of Information ───────────────────────────── */
    printf("\n--- Symmetry of Algorithmic Information ---\n");
    printf("  Km(x) + Km(y|x) ≈ Km(y) + Km(x|y)\n\n");

    SymmetryResult sr = mm_symmetry_of_information(x, y);
    printf("  Left side  = %.0f\n", sr.left);
    printf("  Right side = %.0f\n", sr.right);
    printf("  Difference = %.0f (should be O(1))\n", sr.diff);

    /* ── Process vs Decision Complexity ────────────────────── */
    printf("\n--- Process vs Decision Complexity ---\n");
    const char* tests[] = {"0", "01", "010", "0101", "abc", NULL};
    for (int i = 0; tests[i]; i++) {
        PMString* t = pmstr_from_cstr(tests[i]);
        size_t kp = mm_process_complexity(t);
        size_t kd = mm_decision_complexity(t);
        printf("  \"%s\": Kp=%zu  KD=%zu\n", tests[i], kp, kd);
        pmstr_free(t);
    }

    /* ── Schnorr Randomness Test ───────────────────────────── */
    printf("\n--- Schnorr Randomness (via Km) ---\n");
    const char* r_tests[] = {"aaaaa", "01010", "x7k2p", "abcde", NULL};
    for (int i = 0; r_tests[i]; i++) {
        PMString* t = pmstr_from_cstr(r_tests[i]);
        int ran = mm_is_schnorr_random_prefix(t, 0.7);
        printf("  \"%s\": %srandom (Km/|x| = %.3f)\n",
               r_tests[i], ran ? "" : "not ",
               (double)mm_complexity(t) / (double)t->len);
        pmstr_free(t);
    }

    /* ── Monotone Machine Simulation ───────────────────────── */
    printf("\n--- Simple Monotone Machine ---\n");
    MonotoneMachine* mm = mm_create(5, 6, "example-mm");
    mm_add_transition(mm, 0, 'h', 1, 'h', 1);
    mm_add_transition(mm, 1, 'e', 2, 'e', 1);
    mm_add_transition(mm, 2, 'l', 3, 'l', 1);
    mm_add_transition(mm, 3, 'l', 4, 'l', 1);
    int is_mono = mm_check_monotone(mm);
    printf("  Machine created, monotone? %s\n", is_mono ? "YES" : "NO");
    printf("  Optimality constant: %zu bits\n", mm_optimality_constant(mm));

    unsigned char prog[] = {'h', 'e', 'l', 'l'};
    PMString* out = mm_simulate(mm, prog, 4, 50);
    if (out) {
        printf("  M(\"hell\") output: ");
        for (size_t i = 0; i < out->len; i++) putchar(out->data[i]);
        putchar('\n');
        pmstr_free(out);
    } else {
        printf("  M(\"hell\") did not produce output in time limit.\n");
    }
    mm_free(mm);

    pmstr_free(x); pmstr_free(y);

    printf("\n--- Invariance Theorem ---\n");
    size_t inv = mm_invariance_constant(5, 6, 8, 4);
    printf("  c_{U,V} = %zu bits (simulation overhead between machines)\n", inv);

    printf("\nExample complete.\n");
    return 0;
}
