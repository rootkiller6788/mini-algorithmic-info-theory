/**
 * resource_bounded_k.c - Resource-Bounded Kolmogorov Complexity
 *
 * Implements time-bounded K^t(x), space-bounded K^s(x), prefix complexity,
 * conditional complexity, structure functions, randomness deficiency,
 * incompressibility certificates, and related theorems.
 *
 * References:
 *   Li & Vitanyi, "An Introduction to Kolmogorov Complexity", 4th ed. (2019)
 *   Sipser, "A Complexity Theoretic Approach to Randomness" (1983)
 *   Ko, "On the Notion of Infinite Pseudorandom Sequences" (1986)
 *   Hutter, "Universal Artificial Intelligence" (2005)
 *   Trakhtenbrot, "A Survey of Russian Approaches to Perebor" (1984)
 */

#include "../include/resource_bounded_k.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define RBK_MAX_PROG_LEN      1024
#define RBK_MAX_TAPE_LEN      8192
#define RBK_DEFAULT_TIME      65536
#define RBK_DEFAULT_SPACE     4096
#define RBK_LOG_OVERHEAD        40

static size_t ilog2(size_t n) {
    size_t r = 0;
    while (n >>= 1) r++;
    return r;
}

static char* str_clone(const char *src, size_t len) {
    if (!src || len == 0) return NULL;
    char *dst = (char*)malloc(len + 1);
    if (!dst) return NULL;
    memcpy(dst, src, len);
    dst[len] = '\0';
    return dst;
}

static bool output_matches(const char *output, size_t out_len,
                            const char *target, size_t target_len) {
    if (out_len < target_len) return false;
    for (size_t i = 0; i < target_len; i++)
        if (output[i] != target[i]) return false;
    return true;
}

static size_t simple_utm_run(const char *program, size_t prog_len,
                              size_t time_limit, size_t space_limit,
                              char *output, size_t output_cap) {
    char *tape = (char*)calloc(space_limit + 1, 1);
    if (!tape) return 0;
    size_t head = 0, steps = 0, ip = 0;
    while (ip + 1 < prog_len && steps < time_limit) {
        char b0 = program[ip], b1 = program[ip + 1];
        ip += 2; steps++;
        if (b0 == '0' && b1 == '0') { if (head > 0) head--; }
        else if (b0 == '0' && b1 == '1') { head++; if (head >= space_limit) head--; }
        else if (b0 == '1' && b1 == '0') { tape[head] = (tape[head] == 0) ? 1 : 0; }
        else { break; }
    }
    size_t out_len = 0;
    for (size_t i = 0; i < space_limit && i < output_cap; i++) {
        output[i] = tape[i] ? '1' : '0';
        out_len++;
    }
    free(tape);
    return out_len;
}

/* ==================================================================
 * L2: Core Complexity Functions
 * ================================================================== */

RBKComplexity rbk_K_time(const char *x, size_t len, size_t t) {
    RBKComplexity result;
    memset(&result, 0, sizeof(result));
    result.K_value = len + 16; result.confidence = 1.0; result.exact = false;
    if (!x || len == 0) { result.K_value = 1; result.exact = true; return result; }
    size_t best_len = len + RBK_LOG_OVERHEAD;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= len && plen <= 256; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 1024) count = 1024;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t out_len = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, out_len, x, len) && plen < best_len) {
                best_len = plen; free(result.witness_program);
                result.witness_program = str_clone(prog, plen);
                result.witness_length = plen; result.K_value = plen; break;
            }
        }
        if (best_len <= plen) break;
    }
    result.K_value = best_len; result.time_used = t;
    result.space_used = RBK_DEFAULT_SPACE;
    if (result.witness_program) result.confidence = 0.9;
    return result;
}

RBKComplexity rbk_K_space(const char *x, size_t len, size_t s) {
    RBKComplexity result;
    memset(&result, 0, sizeof(result));
    result.K_value = len + 16; result.confidence = 1.0; result.exact = false;
    if (!x || len == 0) { result.K_value = 1; result.exact = true; return result; }
    size_t best_len = len + RBK_LOG_OVERHEAD;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= len && plen <= 256; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 512) count = 512;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t out_len = simple_utm_run(prog, plen, RBK_DEFAULT_TIME, s, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, out_len, x, len) && plen < best_len) {
                best_len = plen; free(result.witness_program);
                result.witness_program = str_clone(prog, plen);
                result.witness_length = plen; result.K_value = plen; break;
            }
        }
        if (best_len <= plen) break;
    }
    result.K_value = best_len; result.time_used = RBK_DEFAULT_TIME; result.space_used = s;
    if (result.witness_program) result.confidence = 0.9;
    return result;
}
RBKComplexity rbk_K_time_space(const char *x, size_t len, size_t t, size_t s) {
    RBKComplexity result;
    memset(&result, 0, sizeof(result));
    result.K_value = len + 16; result.confidence = 1.0; result.exact = false;
    if (!x || len == 0) { result.K_value = 1; result.exact = true; return result; }
    size_t best_len = len + RBK_LOG_OVERHEAD;
    char output[RBK_MAX_TAPE_LEN];
    size_t max_search = (t < s) ? (ilog2(t) + 1) : (ilog2(s) + 1);
    if (max_search > 1000) max_search = 1000;
    for (size_t plen = 1; plen <= len && plen <= max_search; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 512) count = 512;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen && b < 255; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t out_len = simple_utm_run(prog, plen, t, s, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, out_len, x, len) && plen < best_len) {
                best_len = plen; free(result.witness_program);
                result.witness_program = str_clone(prog, plen);
                result.witness_length = plen; result.K_value = plen; break;
            }
        }
        if (best_len <= plen) break;
    }
    result.K_value = best_len; result.time_used = t; result.space_used = s;
    if (result.witness_program) result.confidence = 0.9;
    return result;
}

RBKComplexity rbk_K_conditional(const char *x, size_t xlen,
                                 const char *y, size_t ylen, size_t t) {
    RBKComplexity result;
    memset(&result, 0, sizeof(result));
    result.K_value = xlen + 16; result.confidence = 1.0; result.exact = false;
    if (!x || xlen == 0) { result.K_value = 1; result.exact = true; return result; }
    if (!y || ylen == 0) return rbk_K_time(x, xlen, t);
    size_t best_len = xlen + RBK_LOG_OVERHEAD;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= xlen && plen <= 256; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 512) count = 512;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t out_len = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, out_len, x, xlen) && plen < best_len) {
                best_len = plen; free(result.witness_program);
                result.witness_program = str_clone(prog, plen);
                result.witness_length = plen; result.K_value = plen; break;
            }
        }
        if (best_len <= plen) break;
    }
    result.K_value = best_len; result.time_used = t;
    result.space_used = RBK_DEFAULT_SPACE;
    if (result.witness_program) result.confidence = 0.9;
    return result;
}

RBKComplexity rbk_K_prefix(const char *x, size_t len, size_t t) {
    RBKComplexity result;
    memset(&result, 0, sizeof(result));
    result.K_value = len + 16; result.confidence = 1.0; result.exact = false;
    if (!x || len == 0) { result.K_value = 1; result.exact = true; return result; }
    size_t best_len = len + RBK_LOG_OVERHEAD;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= len + 4 && plen <= 256; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 512) count = 512;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            bool pf_ok = true;
            if (plen >= 2 && !(prog[plen-2] == '1' && prog[plen-1] == '1')) pf_ok = false;
            if (pf_ok && plen >= 4) {
                for (size_t k = 2; k < plen; k += 2) {
                    if (prog[k-2] == '1' && prog[k-1] == '1') { pf_ok = false; break; }
                }
            }
            if (!pf_ok) continue;
            size_t out_len = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, out_len, x, len) && plen < best_len) {
                best_len = plen; free(result.witness_program);
                result.witness_program = str_clone(prog, plen);
                result.witness_length = plen; result.K_value = plen; break;
            }
        }
        if (best_len <= plen) break;
    }
    result.K_value = best_len; result.time_used = t;
    result.space_used = RBK_DEFAULT_SPACE;
    if (result.witness_program) result.confidence = 0.9;
    return result;
}

RBKComplexity rbk_K_joint(const char *x, size_t xlen,
                           const char *y, size_t ylen, size_t t) {
    RBKComplexity result;
    memset(&result, 0, sizeof(result));
    result.K_value = xlen + ylen + 16; result.confidence = 1.0; result.exact = false;
    if (!x && !y) { result.K_value = 1; result.exact = true; return result; }
    size_t joint_len = (x ? xlen : 0) + (y ? ylen : 0) + 1;
    char *joint = (char*)malloc(joint_len + 1);
    if (!joint) return result;
    if (x) memcpy(joint, x, xlen);
    joint[(x ? xlen : 0)] = '|';
    if (y) memcpy(joint + (x ? xlen : 0) + 1, y, ylen);
    joint[joint_len] = '\0';
    size_t best_len = joint_len + RBK_LOG_OVERHEAD;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= joint_len && plen <= 256; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 512) count = 512;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t out_len = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, out_len, joint, joint_len) && plen < best_len) {
                best_len = plen; free(result.witness_program);
                result.witness_program = str_clone(prog, plen);
                result.witness_length = plen; result.K_value = plen; break;
            }
        }
        if (best_len <= plen) break;
    }
    free(joint);
    result.K_value = best_len; result.time_used = t;
    result.space_used = RBK_DEFAULT_SPACE;
    if (result.witness_program) result.confidence = 0.9;
    return result;
}

double rbk_mutual_information(const char *x, size_t xlen,
                               const char *y, size_t ylen, size_t t) {
    if (!x || !y || xlen == 0 || ylen == 0) return 0.0;
    RBKComplexity kx = rbk_K_time(x, xlen, t);
    RBKComplexity ky = rbk_K_time(y, ylen, t);
    RBKComplexity kxy = rbk_K_joint(x, xlen, y, ylen, t);
    double mi = (double)kx.K_value + (double)ky.K_value - (double)kxy.K_value;
    rbk_complexity_free(&kx); rbk_complexity_free(&ky); rbk_complexity_free(&kxy);
    return (mi > 0.0) ? mi : 0.0;
}

/* ==================================================================
 * L3: Mathematical Structures
 * ================================================================== */

bool rbk_check_monotonicity(const char *x, size_t len,
                             const size_t *time_bounds, size_t num_bounds) {
    if (!time_bounds || num_bounds < 2) return true;
    size_t *K_vals = (size_t*)malloc(num_bounds * sizeof(size_t));
    if (!K_vals) return false;
    for (size_t i = 0; i < num_bounds; i++) K_vals[i] = rbk_K_time(x, len, time_bounds[i]).K_value;
    bool monotonic = true;
    for (size_t i = 1; i < num_bounds; i++)
        if (K_vals[i] > K_vals[i - 1]) { monotonic = false; break; }
    free(K_vals);
    return monotonic;
}

size_t rbk_upper_bound_size(size_t x_len) {
    return x_len + 2 * ilog2(x_len + 1) + RBK_LOG_OVERHEAD;
}

bool rbk_kraft_inequality(const size_t *lengths, size_t count) {
    if (!lengths || count == 0) return true;
    double sum = 0.0;
    for (size_t i = 0; i < count; i++) sum += pow(2.0, -(double)lengths[i]);
    return sum <= 1.0 + 1e-12;
}

char* rbk_self_delimiting_encode(const char *x, size_t len, size_t *out_len) {
    if (!x) { if (out_len) *out_len = 0; return NULL; }
    size_t enc_len = 2 * len + 1;
    char *enc = (char*)malloc(enc_len + 1);
    if (!enc) { if (out_len) *out_len = 0; return NULL; }
    for (size_t i = 0; i < len; i++) enc[i] = '1';
    enc[len] = '0';
    memcpy(enc + len + 1, x, len);
    enc[enc_len] = '\0';
    if (out_len) *out_len = enc_len;
    return enc;
}

char* rbk_self_delimiting_decode(const char *enc, size_t enc_len, size_t *out_len) {
    if (!enc || enc_len < 2) { if (out_len) *out_len = 0; return NULL; }
    size_t i = 0;
    while (i < enc_len && enc[i] == '1') i++;
    if (i >= enc_len || enc[i] != '0') { if (out_len) *out_len = 0; return NULL; }
    size_t data_len = i;
    if (i + 1 + data_len > enc_len) { if (out_len) *out_len = 0; return NULL; }
    char *data = (char*)malloc(data_len + 1);
    if (!data) { if (out_len) *out_len = 0; return NULL; }
    memcpy(data, enc + i + 1, data_len);
    data[data_len] = '\0';
    if (out_len) *out_len = data_len;
    return data;
}

char* rbk_pair_encode(const char *x, size_t xlen,
                       const char *y, size_t ylen, size_t *out_len) {
    if (!x && !y) { if (out_len) *out_len = 0; return NULL; }
    size_t exlen = 0, eylen = 0;
    char *ex = NULL, *ey = NULL;
    if (x) ex = rbk_self_delimiting_encode(x, xlen, &exlen);
    if (y) ey = rbk_self_delimiting_encode(y, ylen, &eylen);
    size_t total = exlen + eylen;
    char *pair = (char*)malloc(total + 1);
    if (!pair) { free(ex); free(ey); if (out_len) *out_len = 0; return NULL; }
    if (ex) { memcpy(pair, ex, exlen); free(ex); }
    if (ey) { memcpy(pair + exlen, ey, eylen); free(ey); }
    pair[total] = '\0';
    if (out_len) *out_len = total;
    return pair;
}

void rbk_pair_decode(const char *pair, size_t pair_len,
                      char **x, size_t *xlen, char **y, size_t *ylen) {
    *x = NULL; *y = NULL; *xlen = 0; *ylen = 0;
    if (!pair || pair_len < 4) return;
    char *dx = rbk_self_delimiting_decode(pair, pair_len, xlen);
    if (dx) {
        *x = dx;
        size_t consumed = 2 * (*xlen) + 1;
        if (consumed < pair_len) {
            char *dy = rbk_self_delimiting_decode(pair + consumed, pair_len - consumed, ylen);
            *y = dy;
        }
    }
}

void rbk_lexicographic_enum(size_t max_len,
                             void (*callback)(const char*, size_t, void*),
                             void *user_data) {
    if (!callback || max_len == 0) return;
    char *buf = (char*)calloc(max_len + 1, 1);
    if (!buf) return;
    size_t total = ((size_t)1) << (max_len < 10 ? max_len : 10);
    for (size_t idx = 0; idx < total; idx++) {
        for (size_t b = 0; b < max_len; b++) buf[b] = ((idx >> b) & 1) ? '1' : '0';
        size_t len = (idx == 0) ? 0 : ilog2(idx) + 1;
        callback(buf, len, user_data);
    }
    free(buf);
}

/* ==================================================================
 * L4: Fundamental Theorems
 * ================================================================== */

size_t rbk_invariance_overhead(const RBKProgram *simulator_v, size_t t) {
    if (!simulator_v) return 0;
    return simulator_v->length + 2 * ilog2(t + 1);
}

int64_t rbk_symmetry_of_info(const char *x, size_t xlen,
                              const char *y, size_t ylen, size_t t) {
    if (!x || !y) return 0;
    RBKComplexity kx = rbk_K_time(x, xlen, t);
    RBKComplexity ky_gx = rbk_K_conditional(y, ylen, x, xlen, t);
    RBKComplexity ky = rbk_K_time(y, ylen, t);
    RBKComplexity kx_gy = rbk_K_conditional(x, xlen, y, ylen, t);
    int64_t lhs = (int64_t)kx.K_value + (int64_t)ky_gx.K_value;
    int64_t rhs = (int64_t)ky.K_value + (int64_t)kx_gy.K_value;
    rbk_complexity_free(&kx); rbk_complexity_free(&ky_gx);
    rbk_complexity_free(&ky); rbk_complexity_free(&kx_gy);
    return lhs - rhs;
}

size_t rbk_coding_theorem_error(const char *x, size_t len, size_t t) {
    RBKComplexity kc = rbk_K_time(x, len, t);
    double m_val = 0.0;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= len + 8 && plen <= 128; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 256) count = 256;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t olen = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, olen, x, len)) m_val += pow(2.0, -(double)plen);
        }
    }
    size_t approx_log_m = (m_val > 0.0) ? (size_t)(-log2(m_val)) : (len + 16);
    int64_t diff = (int64_t)kc.K_value - (int64_t)approx_log_m;
    rbk_complexity_free(&kc);
    return (size_t)((diff >= 0) ? diff : -diff);
}

bool rbk_chaitin_diagonalize(size_t resource_bound) {
    size_t max_len = resource_bound + 1;
    char *candidate = (char*)malloc(max_len + 1);
    if (!candidate) return false;
    for (size_t i = 0; i < max_len; i++) candidate[i] = '0';
    candidate[max_len] = '\0';
    bool found = false;
    for (size_t idx = 0; idx < 256 && idx < ((size_t)1) << resource_bound; idx++) {
        for (size_t b = 0; b < resource_bound; b++)
            candidate[b] = ((idx >> b) & 1) ? '1' : '0';
        RBKComplexity k = rbk_K_time(candidate, resource_bound, RBK_DEFAULT_TIME);
        if (k.K_value >= resource_bound) { found = true; rbk_complexity_free(&k); break; }
        rbk_complexity_free(&k);
    }
    free(candidate);
    return found;
}

/* ==================================================================
 * L5: Analysis - Structure, Randomness, MDL
 * ================================================================== */

RBKStructureFn* rbk_structure_function(const char *x, size_t len,
                                        size_t max_k, size_t *num_points) {
    if (!x || len == 0 || max_k == 0) { if (num_points) *num_points = 0; return NULL; }
    size_t n = max_k < len + 10 ? max_k : len + 10;
    RBKStructureFn *sf = (RBKStructureFn*)malloc(n * sizeof(RBKStructureFn));
    if (!sf) { if (num_points) *num_points = 0; return NULL; }
    for (size_t k = 0; k < n; k++) {
        sf[k].k = k;
        sf[k].min_model_complexity = k;
        sf[k].data_to_model = (k < len) ? (len - k) : 0;
        sf[k].total_complexity = sf[k].min_model_complexity + sf[k].data_to_model;
        sf[k].best_set = str_clone(x, k < len ? k : len);
    }
    if (num_points) *num_points = n;
    return sf;
}

RBKStructureFn rbk_sufficient_statistic(const char *x, size_t len, size_t t) {
    RBKStructureFn best;
    memset(&best, 0, sizeof(best));
    best.k = 0; best.total_complexity = len + RBK_LOG_OVERHEAD;
    if (!x || len == 0) return best;
    for (size_t k = 0; k <= len; k++) {
        size_t kS = k + ilog2(k + 1) + 8;
        size_t kx_gS = (len > k) ? (len - k + ilog2(len - k + 1) + 8) : 0;
        size_t total = kS + kx_gS;
        if (total < best.total_complexity) {
            best.k = k; best.min_model_complexity = kS;
            best.data_to_model = kx_gS; best.total_complexity = total;
            free(best.best_set); best.best_set = str_clone(x, k);
        }
    }
    return best;
}

RBKRandomnessResult rbk_randomness_deficiency(const char *x, size_t len, size_t t) {
    RBKRandomnessResult result;
    memset(&result, 0, sizeof(result));
    result.passes_tests = true;
    if (!x || len == 0) { result.deficiency = 0.0; result.bits_tested = 0; result.martingale_limit = 1.0; return result; }
    double max_deficiency = 0.0, martingale = 1.0;
    for (size_t k = 1; k <= len && k <= 64; k++) {
        RBKComplexity pk = rbk_K_time(x, k, t);
        double deficiency = (double)k - (double)pk.K_value;
        if (deficiency > max_deficiency) max_deficiency = deficiency;
        martingale *= (pk.K_value < (k / 2)) ? 2.0 : 0.5;
        rbk_complexity_free(&pk);
    }
    result.deficiency = max_deficiency;
    result.bits_tested = (len > 64) ? 64 : len;
    result.martingale_limit = martingale;
    result.passes_tests = (max_deficiency <= 5.0);
    return result;
}

size_t rbk_mdl_select(const char *data, size_t data_len,
                       const char **models, size_t model_count, size_t t) {
    if (!data || !models || model_count == 0) return 0;
    size_t best_idx = 0; size_t best_cost = (size_t)-1;
    for (size_t i = 0; i < model_count; i++) {
        size_t mlen = strlen(models[i]);
        RBKComplexity kc = rbk_K_conditional(data, data_len, models[i], mlen, t);
        size_t cost = mlen + kc.K_value;
        rbk_complexity_free(&kc);
        if (cost < best_cost) { best_cost = cost; best_idx = i; }
    }
    return best_idx;
}

double rbk_compressibility_ratio(const char *x, size_t len, size_t t) {
    if (!x || len == 0) return 1.0;
    RBKComplexity kc = rbk_K_time(x, len, t);
    double ratio = (double)kc.K_value / (double)len;
    rbk_complexity_free(&kc);
    return ratio;
}

bool rbk_incompressibility_cert(const char *x, size_t len, size_t k, size_t t) {
    if (!x || len == 0) return true;
    size_t threshold = (len > k) ? (len - k) : 0;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen < threshold && plen <= len && plen <= 256; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 256) count = 256;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t olen = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, olen, x, len)) return false;
        }
    }
    return true;
}

size_t rbk_count_programs_producing(const char *output, size_t out_len,
                                     size_t max_prog_len, size_t t) {
    if (!output || out_len == 0) return 0;
    size_t count = 0;
    char outbuf[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= max_prog_len && plen <= 256; plen++) {
        size_t total = ((size_t)1) << plen;
        if (total > 256) total = 256;
        for (size_t idx = 0; idx < total; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t olen = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, outbuf, RBK_MAX_TAPE_LEN);
            if (output_matches(outbuf, olen, output, out_len)) count++;
        }
    }
    return count;
}

double rbk_universal_distribution(const char *x, size_t len, size_t t) {
    if (!x || len == 0) return 0.0;
    double m = 0.0;
    char output[RBK_MAX_TAPE_LEN];
    for (size_t plen = 1; plen <= 64 && plen <= len + 8; plen++) {
        size_t count = ((size_t)1) << plen;
        if (count > 256) count = 256;
        for (size_t idx = 0; idx < count; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++) prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            size_t olen = simple_utm_run(prog, plen, t, RBK_DEFAULT_SPACE, output, RBK_MAX_TAPE_LEN);
            if (output_matches(output, olen, x, len)) m += pow(2.0, -(double)plen);
        }
    }
    return m;
}

int rbk_classify_string(const char *x, size_t len, size_t t) {
    if (!x || len == 0) return 2;
    RBKComplexity kc = rbk_K_time(x, len, t);
    double ratio = (double)kc.K_value / (double)len;
    rbk_complexity_free(&kc);
    if (ratio >= 0.9) return 0;
    if (ratio >= 0.5) return 1;
    return 2;
}

/* ==================================================================
 * L6: Resource-Bound Hierarchy, L7: Crypto, L8: Advanced
 * ================================================================== */

size_t rbk_time_advantage(const char *x, size_t len, size_t t1, size_t t2) {
    RBKComplexity k1 = rbk_K_time(x, len, t1);
    RBKComplexity k2 = rbk_K_time(x, len, t2);
    int64_t diff = (int64_t)k1.K_value - (int64_t)k2.K_value;
    rbk_complexity_free(&k1); rbk_complexity_free(&k2);
    return (size_t)((diff >= 0) ? diff : -diff);
}

size_t rbk_space_time_tradeoff(const char *x, size_t len,
                                size_t k_target, size_t s) {
    if (!x || len == 0) return 0;
    size_t t_low = 1, t_high = 1000000, found_t = 0;
    while (t_low <= t_high) {
        size_t t_mid = t_low + (t_high - t_low) / 2;
        RBKComplexity kc = rbk_K_time_space(x, len, t_mid, s);
        if (kc.K_value <= k_target) { found_t = t_mid; t_high = t_mid - 1; }
        else { t_low = t_mid + 1; }
        rbk_complexity_free(&kc);
    }
    return found_t;
}

bool rbk_is_time_constructible(size_t (*f)(size_t), size_t max_n) {
    if (!f) return false;
    size_t prev = f(1);
    if (prev < 1) return false;
    for (size_t n = 2; n <= max_n && n <= 100; n++) {
        size_t curr = f(n);
        if (curr < prev || curr < n) return false;
        prev = curr;
    }
    return true;
}

bool rbk_is_space_constructible(size_t (*f)(size_t), size_t max_n) {
    if (!f) return false;
    size_t prev = 0;
    for (size_t n = 1; n <= max_n && n <= 100; n++) {
        size_t curr = f(n);
        if (curr < prev) return false;
        if (curr < ilog2(n) && n > 2) return false;
        prev = curr;
    }
    return true;
}

double rbk_prg_incompressibility_ratio(const char *keystream, size_t len, size_t t) {
    return rbk_compressibility_ratio(keystream, len, t);
}

size_t rbk_crypto_weakness_gap(const char *output, size_t len, size_t t) {
    RBKComplexity kc = rbk_K_time(output, len, t);
    size_t gap = (len > kc.K_value) ? (len - kc.K_value) : 0;
    rbk_complexity_free(&kc);
    return gap;
}

double rbk_owf_hardness(const char *y, size_t ylen,
                         const char *x_hint, size_t xlen, size_t t) {
    RBKComplexity kc = rbk_K_conditional(x_hint, xlen, y, ylen, t);
    double hardness = (xlen > 0) ? (double)kc.K_value / (double)xlen : 0.0;
    rbk_complexity_free(&kc);
    return hardness;
}

double rbk_information_distance(const char *x, size_t xlen,
                                 const char *y, size_t ylen, size_t t) {
    if (!x || !y || xlen == 0 || ylen == 0) return 0.0;
    RBKComplexity kx_gy = rbk_K_conditional(x, xlen, y, ylen, t);
    RBKComplexity ky_gx = rbk_K_conditional(y, ylen, x, xlen, t);
    RBKComplexity kx = rbk_K_time(x, xlen, t);
    RBKComplexity ky = rbk_K_time(y, ylen, t);
    size_t max_cond = (kx_gy.K_value > ky_gx.K_value) ? kx_gy.K_value : ky_gx.K_value;
    size_t max_K = (kx.K_value > ky.K_value) ? kx.K_value : ky.K_value;
    double dist = (max_K > 0) ? (double)max_cond / (double)max_K : 0.0;
    rbk_complexity_free(&kx_gy); rbk_complexity_free(&ky_gx);
    rbk_complexity_free(&kx); rbk_complexity_free(&ky);
    return dist;
}

double rbk_normalized_information_distance(const char *x, size_t xlen,
                                            const char *y, size_t ylen, size_t t) {
    return rbk_information_distance(x, xlen, y, ylen, t);
}

size_t rbk_quine_construct(char *buffer, size_t buf_cap) {
    const char *quine_str = "110011111111";
    size_t len = strlen(quine_str);
    if (buffer && buf_cap > len) { memcpy(buffer, quine_str, len); buffer[len] = '\0'; }
    return len;
}

size_t rbk_self_print_overhead(void) { return RBK_LOG_OVERHEAD; }

int rbk_which_more_complex(const char *a, size_t alen,
                            const char *b, size_t blen, size_t t) {
    RBKComplexity ka = rbk_K_time(a, alen, t);
    RBKComplexity kb = rbk_K_time(b, blen, t);
    int result = (ka.K_value > kb.K_value) ? 0 : (kb.K_value > ka.K_value) ? 1 : -1;
    rbk_complexity_free(&ka); rbk_complexity_free(&kb);
    return result;
}

size_t rbk_meta_complexity_adversary(const char *x, size_t len,
                                      size_t t_target, size_t num_rounds) {
    if (!x || len == 0 || num_rounds == 0) return 0;
    size_t advantage = 0;
    for (size_t r = 0; r < num_rounds; r++) {
        RBKComplexity kc = rbk_K_time(x, len, t_target);
        if (kc.K_value < len / 2) advantage++;
        rbk_complexity_free(&kc);
    }
    return advantage;
}

/* ==================================================================
 * Utility Functions
 * ================================================================== */

void rbk_complexity_free(RBKComplexity *c) {
    if (c && c->witness_program) { free(c->witness_program); c->witness_program = NULL; }
}

void rbk_structure_fn_free(RBKStructureFn *sf, size_t count) {
    if (sf) { for (size_t i = 0; i < count; i++) free(sf[i].best_set); free(sf); }
}

void rbk_print_complexity(const RBKComplexity *c) {
    if (!c) return;
    printf("K(x)=%zu, time=%zu, space=%zu, conf=%.2f\n",
           c->K_value, c->time_used, c->space_used, c->confidence);
}

void rbk_print_structure_fn(const RBKStructureFn *sf, size_t count) {
    if (!sf) return;
    printf("Structure Function h_x(k):\n");
    for (size_t i = 0; i < count; i++)
        printf("  k=%zu: K(S)=%zu, K(x|S)=%zu, total=%zu\n",
               sf[i].k, sf[i].min_model_complexity, sf[i].data_to_model, sf[i].total_complexity);
}

void rbk_print_randomness(const RBKRandomnessResult *r) {
    if (!r) return;
    printf("Randomness: deficiency=%.2f, passes=%s, bits=%zu, martingale=%.4f\n",
           r->deficiency, r->passes_tests ? "YES" : "NO",
           r->bits_tested, r->martingale_limit);
}
