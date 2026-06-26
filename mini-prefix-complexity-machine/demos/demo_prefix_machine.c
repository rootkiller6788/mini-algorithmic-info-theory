#include "../include/prefix_machine.h"
#include <stdio.h>
int main(void) {
    printf("=== Prefix Complexity Machine Demo ===\n\n");

    /* Prefix-free sets */
    printf("- Prefix-free sets -\n");
    PMString* pf[4];
    pf[0] = pmstr_from_cstr("0"); pf[1] = pmstr_from_cstr("10");
    pf[2] = pmstr_from_cstr("110"); pf[3] = pmstr_from_cstr("111");
    printf("  Set: {0, 10, 110, 111}\n");
    printf("  Prefix-free? %s\n", pm_is_prefix_free(pf, 4) ? "YES" : "NO");
    printf("  Kraft sum = %.4f (<=1? %s)\n",
           pm_lebesgue_measure(pf, 4),
           pm_kraft_check(pf, 4) ? "YES" : "NO");
    for (int i = 0; i < 4; i++) pmstr_free(pf[i]);

    pf[0] = pmstr_from_cstr("0"); pf[1] = pmstr_from_cstr("01");
    pf[2] = pmstr_from_cstr("10");
    printf("  Set: {0, 01, 10} -> Prefix-free? %s\n\n",
           pm_is_prefix_free(pf, 3) ? "YES" : "NO");
    for (int i = 0; i < 3; i++) pmstr_free(pf[i]);

    /* Prefix complexity */
    printf("- Prefix complexity K(x) -\n");
    const char* tests[] = {"", "a", "hello", "aaaaa", "random!", NULL};
    for (int i = 0; tests[i]; i++) {
        PMString* s = pmstr_from_cstr(tests[i]);
        printf("  K(\"%s\") <= %zu bits  (|x|=%zu)\n",
               tests[i], pm_prefix_complexity(s), s->len);
        pmstr_free(s);
    }

    /* Plain-to-prefix gap */
    printf("\n- Plain vs Prefix gap -\n");
    printf("  K(x) <= C(x) + 2 log C(x) + O(1)\n");
    for (size_t C = 10; C <= 1000; C *= 10)
        printf("  C(x)=%4zu -> K(x) <= %zu\n", C, pm_plain_to_prefix_gap(C));

    /* Self-delimiting code */
    printf("\n- Self-delimiting code -\n");
    PMString* s = pmstr_from_cstr("test");
    PMString* sd = pm_self_delimiting(s);
    printf("  Original |x|=%zu, Self-delim |xbar|=%zu (predicted: %zu)\n",
           s->len, sd->len, pm_self_delimiting_length(s->len));
    printf("  xbar bits: "); pmstr_print(sd);
    pmstr_free(s); pmstr_free(sd);

    /* Elias codes */
    printf("\n- Elias codes for n=100 -\n");
    PMString* g = pm_elias_gamma_encode(100);
    PMString* d = pm_elias_delta_encode(100);
    printf("  Gamma(100): len=%zu decoded=%zu\n",
           g->len, pm_elias_gamma_decode(g));
    printf("  Delta(100): len=%zu decoded=%zu\n",
           d->len, pm_elias_delta_decode(d));
    pmstr_free(g); pmstr_free(d);

    /* Monotone */
    printf("\n- Monotone check -\n");
    PrefixMachine* pm = pm_create(4, 2, "test-pm");
    pm_add_transition(pm, 0, 0, 0, 0, 1);
    pm_add_transition(pm, 0, 1, 1, 1, 1);
    printf("  Is monotone? %s\n", pm_is_monotone(pm) ? "YES" : "NO");
    pm_free(pm);

    printf("\nDemo complete.\n");
    return 0;
}
