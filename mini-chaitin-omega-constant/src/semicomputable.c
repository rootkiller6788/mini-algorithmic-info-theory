#include "semicomputable.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

/* ============================================================
 * Semicomputable Reals (Left-c.e. Reals) — Complete Implementation
 *
 * Encodes knowledge points:
 *   L1: DyadicRational, LeftCEReal (typedef struct)
 *   L2: Left-c.e. reals as limits of computable increasing sequences
 *   L3: Rational approximation via dyadics (numerator / 2^k)
 *   L4: Ω is left-c.e. but not right-c.e. (not computable)
 *   L5: Construction from prefix-free machine halting
 *   L6: Correspondence between left-c.e. reals and Σ⁰₁ sets
 *
 * Reference: Soare (1987), Downey & Hirschfeldt (2010), Calude (2002)
 * ============================================================ */

/* ── Dyadic Rationals ─────────────────────────────────────── */
/* Knowledge point L3: A dyadic rational is of the form
 * a / 2^b where a ∈ ℤ, b ∈ ℕ. This representation is
 * closed under addition and comparison is exact. */

DyadicRational* dr_create(uint64_t num, int denom_power) {
    DyadicRational* dr = (DyadicRational*)malloc(sizeof(DyadicRational));
    if (!dr) return NULL;
    dr->numerator = num;
    dr->denominator_power = denom_power;
    return dr;
}

void dr_free(DyadicRational* dr) {
    free(dr);
}

double dr_to_double(const DyadicRational* dr) {
    if (!dr) return 0.0;
    return (double)dr->numerator / pow(2.0, (double)dr->denominator_power);
}

int dr_compare(const DyadicRational* a, const DyadicRational* b) {
    if (!a || !b) return (a == b) ? 0 : (!a ? -1 : 1);
    /* Compare a/2^{da} vs b/2^{db}:
     * a/2^{da} < b/2^{db} iff a * 2^{db} < b * 2^{da} */
    int max_power = (a->denominator_power > b->denominator_power)
                    ? a->denominator_power : b->denominator_power;
    /* Scale both to same power */
    uint64_t a_scaled = a->numerator;
    uint64_t b_scaled = b->numerator;
    if (a->denominator_power < max_power)
        a_scaled <<= (max_power - a->denominator_power);
    else
        b_scaled <<= (max_power - b->denominator_power);
    if (a_scaled < b_scaled) return -1;
    if (a_scaled > b_scaled) return 1;
    return 0;
}

DyadicRational* dr_add(const DyadicRational* a, const DyadicRational* b) {
    if (!a || !b) return NULL;
    int max_power = (a->denominator_power > b->denominator_power)
                    ? a->denominator_power : b->denominator_power;
    uint64_t a_s = a->numerator;
    uint64_t b_s = b->numerator;
    if (a->denominator_power < max_power)
        a_s <<= (max_power - a->denominator_power);
    if (b->denominator_power < max_power)
        b_s <<= (max_power - b->denominator_power);
    uint64_t sum = a_s + b_s;
    /* Check overflow */
    if (sum < a_s) return NULL;
    return dr_create(sum, max_power);
}

DyadicRational* dr_multiply_by_rational(const DyadicRational* a,
                                         uint64_t num, uint64_t den) {
    if (!a || den == 0) return NULL;
    /* (a_num / 2^a_den) * (num / den) = (a_num * num) / (2^a_den * den) */
    /* Approximate as dyadic: keep denominator power of 2 */
    uint64_t prod_num = a->numerator * num;
    /* Adjust denominator: we need a power of 2, so scale */
    int new_den_power = a->denominator_power;
    /* Approximate: reduce fraction to dyadic */
    while (den > 1 && new_den_power < 60) {
        if (den % 2 == 0) { den /= 2; }
        else { prod_num /= den; break; }
    }
    return dr_create(prod_num, new_den_power);
}

/* ── Left-c.e. Real Lifecycle ────────────────────────────── */

LeftCEReal* lce_create(size_t capacity) {
    LeftCEReal* lce = (LeftCEReal*)calloc(1, sizeof(LeftCEReal));
    if (!lce) return NULL;
    lce->capacity = (capacity > 0) ? capacity : 128;
    lce->approximations = (DyadicRational**)calloc(lce->capacity,
                                                     sizeof(DyadicRational*));
    if (!lce->approximations) { free(lce); return NULL; }
    lce->count = 0;
    lce->is_converged = 0;
    lce->limit_estimate = 0.0;
    return lce;
}

void lce_free(LeftCEReal* lce) {
    if (!lce) return;
    for (size_t i = 0; i < lce->count; i++)
        dr_free(lce->approximations[i]);
    free(lce->approximations);
    free(lce);
}

int lce_add_approximation(LeftCEReal* lce, const DyadicRational* approx) {
    if (!lce || !approx) return -1;
    /* Ensure non-decreasing sequence */
    if (lce->count > 0) {
        int cmp = dr_compare(approx, lce->approximations[lce->count - 1]);
        if (cmp < 0) return -1; /* must be non-decreasing */
    }
    if (lce->count >= lce->capacity) {
        size_t new_cap = lce->capacity * 2;
        DyadicRational** new_arr = (DyadicRational**)realloc(
            lce->approximations, new_cap * sizeof(DyadicRational*));
        if (!new_arr) return -1;
        lce->approximations = new_arr;
        lce->capacity = new_cap;
    }
    lce->approximations[lce->count] = dr_create(approx->numerator,
                                                  approx->denominator_power);
    if (!lce->approximations[lce->count]) return -1;
    lce->count++;
    lce->limit_estimate = dr_to_double(approx);
    return 0;
}

double lce_current_value(const LeftCEReal* lce) {
    if (!lce || lce->count == 0) return 0.0;
    return dr_to_double(lce->approximations[lce->count - 1]);
}

int lce_is_non_decreasing(const LeftCEReal* lce) {
    if (!lce || lce->count <= 1) return 1;
    for (size_t i = 1; i < lce->count; i++) {
        if (dr_compare(lce->approximations[i], lce->approximations[i-1]) < 0)
            return 0;
    }
    return 1;
}

double lce_convergence_rate(const LeftCEReal* lce) {
    if (!lce || lce->count <= 1) return 0.0;
    /* Rate = avg change per step */
    double first = dr_to_double(lce->approximations[0]);
    double last  = dr_to_double(lce->approximations[lce->count - 1]);
    return (last - first) / (double)lce->count;
}

/* ── Left-c.e. Real from Prefix-Free Machine ──────────────── */
/* Knowledge point L4: For any prefix-free machine M,
 * Ω_M = Σ_{p: M(p) halts} 2^{-|p|} is left-c.e.
 * Proof: enumerate programs in order; as each is found to halt,
 * add its contribution, producing a non-decreasing computable
 * sequence converging to Ω_M. */

LeftCEReal* lce_from_machine_halting(void* pfm_v, size_t max_prog_size, size_t steps) {
    OptimalPFM* machine = (OptimalPFM*)pfm_v;
    (void)pfm_v;
    if (!machine) return NULL;
    LeftCEReal* lce = lce_create(256);
    if (!lce) return NULL;

    HaltingEnumeration* he = halting_enumerate(machine, max_prog_size, steps);
    if (!he) { lce_free(lce); return NULL; }

    /* Build cumulative sum as approximations */
    double cumulative = 0.0;
    for (size_t i = 0; i < he->count; i++) {
        if (he->records[i].program) {
            cumulative += pow(0.5, (double)he->records[i].program->length);
            /* Convert to dyadic rational */
            /* Approximation: round to 2^{-40} */
            int denom = 40;
            uint64_t num = (uint64_t)(cumulative * pow(2.0, (double)denom) + 0.5);
            DyadicRational* dr = dr_create(num, denom);
            if (dr) {
                lce_add_approximation(lce, dr);
                dr_free(dr);
            }
        }
    }
    halting_enum_free(he);
    return lce;
}

/* ── Is This Left-c.e. Real Computable? ──────────────────── */
/* Knowledge point L2: A real is computable iff it is both
 * left-c.e. and right-c.e. Equivalently: there exist computable
 * sequences converging from above and below.
 * Ω is left-c.e. but NOT right-c.e. (hence not computable). */

int lce_is_computable(const LeftCEReal* lce) {
    if (!lce) return 0;
    /* Check if the sequence has converged (difference between
     * consecutive approximations approaches 0 rapidly).
     * For truly computable reals, we can compute both bounds.
     * For Ω, the sequence never converges fast enough. */
    if (lce->count < 2) return 0;
    double last_diff = 0.0;
    for (size_t i = 1; i < lce->count && i < 10; i++) {
        double prev = dr_to_double(lce->approximations[i-1]);
        double cur  = dr_to_double(lce->approximations[i]);
        if (cur - prev > last_diff * 10) return 0; /* not converging fast */
        last_diff = cur - prev;
    }
    /* Heuristic: small convergence → likely computable */
    return (lce->count > 5 && lce_convergence_rate(lce) < 1e-6) ? 1 : 0;
}

/* ── Operations on Left-c.e. Reals ───────────────────────── */

LeftCEReal* lce_add(const LeftCEReal* a, const LeftCEReal* b) {
    if (!a || !b) return NULL;
    size_t max_count = (a->count > b->count) ? a->count : b->count;
    LeftCEReal* result = lce_create(max_count);
    if (!result) return NULL;
    for (size_t i = 0; i < max_count; i++) {
        const DyadicRational* da = (i < a->count) ? a->approximations[i]
                                                    : a->approximations[a->count - 1];
        const DyadicRational* db = (i < b->count) ? b->approximations[i]
                                                    : b->approximations[b->count - 1];
        DyadicRational* sum = dr_add(da, db);
        if (sum) { lce_add_approximation(result, sum); dr_free(sum); }
    }
    return result;
}

LeftCEReal* lce_scale(const LeftCEReal* a, double factor) {
    if (!a) return NULL;
    LeftCEReal* result = lce_create(a->count);
    if (!result) return NULL;
    for (size_t i = 0; i < a->count; i++) {
        double val = dr_to_double(a->approximations[i]) * factor;
        int denom = 40;
        uint64_t num = (uint64_t)(val * pow(2.0, (double)denom) + 0.5);
        DyadicRational* dr = dr_create(num, denom);
        if (dr) { lce_add_approximation(result, dr); dr_free(dr); }
    }
    return result;
}

/* ── Bit Extraction from Left-c.e. Real ───────────────────── */
/* Knowledge point L5: Given a left-c.e. real α with current
 * approximation q ≤ α (from below), we can determine bit n
 * if q is close enough to α. Specifically: if q is within
 * 2^{-n-1} of α, then the nth bit is determined. */

int lce_get_bit(const LeftCEReal* lce, size_t bit_position) {
    if (!lce || lce->count == 0) return 0;
    double current = lce_current_value(lce);
    /* If current is close enough and the gap is < threshold,
     * we know the bit. Otherwise we need better approximation. */
    double fractional = current - floor(current);
    /* Extract bit: multiply by 2^{position+1}, take integer part mod 2 */
    double scaled = fractional * pow(2.0, (double)(bit_position + 1));
    return ((uint64_t)scaled) & 1;
}

BitString* lce_get_bits(const LeftCEReal* lce, size_t n_bits) {
    if (!lce) return NULL;
    BitString* bs = bs_create(NULL, 0);
    if (!bs) return NULL;
    for (size_t i = 0; i < n_bits; i++)
        bs_append_bit(bs, lce_get_bit(lce, i));
    return bs;
}

/* ── Σ⁰₁ Set ↔ Left-c.e. Real Correspondence ─────────────── */
/* Knowledge point L6: A set S ⊆ ℕ is c.e. (Σ⁰₁) iff its
 * characteristic real χ_S = Σ_{n∈S} 2^{-n-1} is left-c.e.
 * This establishes the equivalence between computable
 * enumerability of sets and left-c.e.-ness of reals. */

LeftCEReal* lce_from_ce_set(const int* set_indicator, size_t size) {
    if (!set_indicator) return NULL;
    LeftCEReal* lce = lce_create(size);
    if (!lce) return NULL;
    double cumulative = 0.0;
    for (size_t i = 0; i < size; i++) {
        if (set_indicator[i]) {
            cumulative += pow(0.5, (double)(i + 1));
            int denom = 40;
            uint64_t num = (uint64_t)(cumulative * pow(2.0, (double)denom) + 0.5);
            DyadicRational* dr = dr_create(num, denom);
            if (dr) { lce_add_approximation(lce, dr); dr_free(dr); }
        }
    }
    return lce;
}

void lce_to_ce_set(const LeftCEReal* lce, int* out_set, size_t max_n) {
    if (!lce || !out_set) return;
    memset(out_set, 0, max_n * sizeof(int));
    double value = lce_current_value(lce);
    /* Decode: if bit n is 1, then n ∈ S */
    for (size_t i = 0; i < max_n; i++) {
        double bit_val = value * pow(2.0, (double)(i + 1));
        if ((uint64_t)(bit_val + 1e-10) & 1) {
            out_set[i] = 1;
            value -= pow(0.5, (double)(i + 1));
        }
    }
}

/* ── Ω is not Right-c.e. Demonstration ───────────────────── */
/* Knowledge point L4: If Ω were right-c.e., there would be a
 * computable non-increasing sequence converging to Ω from above.
 * Combined with Ω being left-c.e., this would make Ω computable.
 * But Ω encodes the halting problem, so it cannot be computable.
 * Therefore Ω is not right-c.e. */

int lce_not_right_ce_demonstration(LeftCEReal* omega_lce) {
    if (!omega_lce) return 0;
    printf("lce_not_right_ce_demonstration: Ω is left-c.e. but not right-c.e.\n");
    printf("  If Ω were right-c.e., then Ω would be computable\n");
    printf("  (both-sided approximation → computable).\n");
    printf("  But Ω ≡_T K (the halting set), which is non-computable.\n");
    printf("  Therefore Ω is not right-c.e.\n");
    printf("  Current Ω approximation from below: %.10f\n",
           lce_current_value(omega_lce));
    return 1;
}