#include "../include/prefix_machine.h"
#include "../include/universal_distribution.h"
#include <stdio.h>

static void analyze_string(const char* label, const char* str) {
    PMString* s = pmstr_from_cstr(str);
    size_t K_est = pm_prefix_complexity(s);
    UniversalDistribution* ud = ud_estimate(s, 16, 1000);
    double m_x = ud->m_x;
    double K_from_m = -log2(m_x > 0 ? m_x : 1e-300);
    printf("  %-12s |x|=%2zu  K_est=%3zu  m(x)=%12.6e  -log m=%7.3f\n",
           label, s->len, K_est, m_x, K_from_m);
    ud_free(ud);
    pmstr_free(s);
}

int main(void) {
    printf("=== The Coding Theorem Demo ===\n\n");
    printf("  K(x) = -log m(x) + O(1)\n");
    printf("  m(x) = sum_{p:U(p)=x} 2^{-|p|}\n\n");

    printf("%-14s %s\n", "String", "|x|  K_est    m(x)           -log m");
    printf("-------------------------------------------------\n");
    analyze_string("empty",  "");
    analyze_string("a",      "a");
    analyze_string("ab",     "ab");
    analyze_string("hello",  "hello");
    analyze_string("aaaaa",  "aaaaa");
    analyze_string("random", "x7k2p");

    printf("\nInterpretation:\n");
    printf("  Simple strings have larger m(x) -> smaller K(x).\n");
    printf("  Random strings have smaller m(x) -> larger K(x).\n");
    printf("  m(x) multiplicatively dominates all computable semimeasures.\n");
    printf("\nCoding Theorem bound: pm_coding_theorem_bound(m) = -log2(m).\n");

    double test_m = 0.001;
    printf("  For m=%.4f: bound = %.4f bits\n", test_m, pm_coding_theorem_bound(test_m));

    printf("\nDemo complete.\n");
    return 0;
}
