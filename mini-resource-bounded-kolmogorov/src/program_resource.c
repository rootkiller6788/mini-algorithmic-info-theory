/**
 * program_resource.c - Program Enumeration and Resource Management
 *
 * Manages program enumeration under resource bounds for Kolmogorov
 * complexity computations. Provides UTM simulation, prefix-free code
 * generation, program counting, and binary string utilities.
 *
 * References:
 *   Li & Vitanyi, "An Introduction to Kolmogorov Complexity", 4th ed. (2019)
 *   Zvonkin & Levin, "The Complexity of Finite Objects" (1970)
 *   Chaitin, "A Theory of Program Size Formally Identical to Information Theory" (1975)
 */

#include "../include/program_resource.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static size_t ilog2(size_t n) {
    size_t r = 0;
    while (n >>= 1) r++;
    return r;
}

/* ==================================================================
 * L2: UTM Emulation
 * ================================================================== */

UTMContext* utm_context_create(const char *program, size_t prog_len,
                                const char *input, size_t input_len,
                                size_t time_limit, size_t space_limit) {
    UTMContext *ctx = (UTMContext*)malloc(sizeof(UTMContext));
    if (!ctx) return NULL;
    memset(ctx, 0, sizeof(UTMContext));
    ctx->program = (char*)program;
    ctx->prog_len = prog_len;
    ctx->input = (char*)input;
    ctx->input_len = input_len;
    ctx->time_limit = time_limit;
    ctx->space_limit = space_limit < 8192 ? space_limit : 8192;
    ctx->tape = (char*)calloc(ctx->space_limit + 1, 1);
    if (!ctx->tape) { free(ctx); return NULL; }
    for (size_t i = 0; i < input_len && i < ctx->space_limit; i++)
        ctx->tape[i] = (input[i] == '1') ? 1 : 0;
    ctx->tape_head = input_len;
    ctx->tape_len = ctx->space_limit;
    return ctx;
}

bool utm_step_one(UTMContext *ctx) {
    if (!ctx || ctx->halted || ctx->time_exceeded || ctx->space_exceeded)
        return false;
    if (ctx->steps_executed >= ctx->time_limit)
        { ctx->time_exceeded = true; return false; }
    size_t ip = ctx->steps_executed * 2;
    if (ip + 1 >= ctx->prog_len) { ctx->halted = true; return false; }
    char b0 = ctx->program[ip], b1 = ctx->program[ip + 1];
    ctx->steps_executed++;
    if (b0 == '0' && b1 == '0') { if (ctx->tape_head > 0) ctx->tape_head--; }
    else if (b0 == '0' && b1 == '1') {
        ctx->tape_head++;
        if (ctx->tape_head >= ctx->space_limit)
            { ctx->tape_head--; ctx->space_exceeded = true; return false; }
    } else if (b0 == '1' && b1 == '0') { ctx->tape[ctx->tape_head] ^= 1; }
    else { ctx->halted = true; return false; }
    return true;
}

ProgramOutput utm_run_all(UTMContext *ctx) {
    ProgramOutput result;
    memset(&result, 0, sizeof(result));
    result.terminated = false; result.prefix_ok = true;
    if (!ctx) return result;
    while (utm_step_one(ctx)) { }
    result.steps_used = ctx->steps_executed;
    result.space_used = ctx->tape_head + 1;
    result.terminated = ctx->halted;
    size_t out_len = ctx->space_limit < 4096 ? ctx->space_limit : 4096;
    result.output = (char*)malloc(out_len + 1);
    if (result.output) {
        for (size_t i = 0; i < out_len; i++)
            result.output[i] = ctx->tape[i] ? '1' : '0';
        result.output_len = out_len;
        result.output[out_len] = '\0';
    }
    return result;
}

bool utm_check_prefix_free(const char *program, size_t prog_len,
                            ProgramPool *valid_programs) {
    if (!valid_programs || prog_len == 0) return true;
    for (size_t sub = 1; sub < prog_len && sub <= valid_programs->max_length; sub++) {
        for (size_t i = 0; i < valid_programs->count; i++) {
            if (valid_programs->lengths[i] == sub) {
                bool match = true;
                for (size_t j = 0; j < sub; j++)
                    if (program[j] != valid_programs->programs[i][j])
                        { match = false; break; }
                if (match) return false;
            }
        }
    }
    return true;
}

void utm_context_free(UTMContext *ctx) {
    if (ctx) { free(ctx->tape); free(ctx); }
}

/* ==================================================================
 * L3: Program Enumeration
 * ================================================================== */

ProgramPool program_pool_create(size_t max_length) {
    ProgramPool pool;
    memset(&pool, 0, sizeof(pool));
    pool.max_length = max_length;
    pool.capacity = (max_length < 10) ? ((size_t)1 << (max_length + 1)) : 1024;
    pool.programs = (char**)calloc(pool.capacity, sizeof(char*));
    pool.lengths = (size_t*)calloc(pool.capacity, sizeof(size_t));
    pool.count = 0; pool.prefix_free_pool = true;
    return pool;
}

size_t program_pool_enumerate_prefix_free(ProgramPool *pool, size_t max_length) {
    if (!pool) return 0;
    pool->max_length = max_length; pool->count = 0;
    for (size_t plen = 1; plen <= max_length && plen <= 12; plen++) {
        size_t total = ((size_t)1) << plen;
        if (total > 256) total = 256;
        for (size_t idx = 0; idx < total && pool->count < pool->capacity; idx++) {
            char prog[64];
            for (size_t b = 0; b < plen; b++)
                prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            bool is_pf = true;
            for (size_t i = 0; i < pool->count; i++) {
                size_t min_len = pool->lengths[i] < plen ? pool->lengths[i] : plen;
                bool match = true;
                for (size_t j = 0; j < min_len; j++)
                    if (prog[j] != pool->programs[i][j]) { match = false; break; }
                if (match) { is_pf = false; break; }
            }
            if (!is_pf) continue;
            pool->programs[pool->count] = strdup(prog);
            pool->lengths[pool->count] = plen;
            pool->count++;
        }
    }
    return pool->count;
}

void program_pool_lexicographic_enumerate(
    size_t max_length,
    void (*callback)(const char *program, size_t prog_len, void *user_data),
    void *user_data) {
    if (!callback || max_length == 0) return;
    size_t total = ((size_t)1) << (max_length < 10 ? max_length : 10);
    char *buf = (char*)malloc(max_length + 1);
    if (!buf) return;
    for (size_t idx = 0; idx < total; idx++) {
        for (size_t b = 0; b < max_length; b++)
            buf[b] = ((idx >> b) & 1) ? '1' : '0';
        size_t len = (idx == 0) ? 0 : ilog2(idx) + 1;
        callback(buf, len, user_data);
    }
    free(buf);
}

size_t program_pool_count_at_length(size_t n, bool prefix_free) {
    if (n > 20) n = 20;
    if (!prefix_free) return ((size_t)1) << n;
    if (n == 0) return 1;
    return ((size_t)1) << (n - 1);
}

bool program_pool_next(LexEnumState *state) {
    if (!state || state->done) return false;
    size_t carry = 1;
    for (size_t i = 0; i < state->max_len && carry; i++) {
        if (state->current[i] == '0') { state->current[i] = '1'; carry = 0; }
        else { state->current[i] = '0'; carry = 1; }
    }
    state->current_len = (state->total_enumerated == 0) ? 0
                         : ilog2(state->total_enumerated) + 1;
    if (state->current_len > state->max_len) state->current_len = state->max_len;
    state->total_enumerated++;
    if (state->total_enumerated >= ((size_t)1) << state->max_len) state->done = true;
    return !state->done;
}

LexEnumState lex_enum_init(size_t max_len) {
    LexEnumState state;
    memset(&state, 0, sizeof(state));
    state.max_len = max_len < 10 ? max_len : 10;
    state.current = (char*)calloc(state.max_len + 1, 1);
    state.current_len = 0; state.total_enumerated = 0; state.done = false;
    if (state.current) state.current[0] = '0';
    return state;
}

void program_pool_free(ProgramPool *pool) {
    if (pool) {
        for (size_t i = 0; i < pool->count; i++) free(pool->programs[i]);
        free(pool->programs); free(pool->lengths);
        memset(pool, 0, sizeof(*pool));
    }
}

void program_output_free(ProgramOutput *out) {
    if (out && out->output) { free(out->output); out->output = NULL; }
}

/* ==================================================================
 * L4: Resource Constraint Utilities
 *
 * Upper bound on K^t(x): K(x) <= |x| + 2log|x| + c
 * via self-printing construction.
 *
 * Time/Space constructibility: can a TM compute f(n) in O(f(n))?
 *
 * Simulation overhead: simulating one UTM on another.
 * ================================================================== */

size_t resource_upper_bound_Kt(const char *x, size_t len) {
    if (!x) return 0;
    return len + 2 * ilog2(len + 1) + 40;
}

bool resource_is_time_constructible(size_t (*f)(size_t), size_t max_n) {
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

bool resource_is_space_constructible(size_t (*f)(size_t), size_t max_n) {
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

size_t f_time_linear(size_t n) { return n * 10; }
size_t f_time_quadratic(size_t n) { return n * n; }
size_t f_time_exponential(size_t n) {
    size_t r = 1;
    for (size_t i = 0; i < n && i < 10; i++) r *= 2;
    return r;
}
size_t f_space_linear(size_t n) { return n * 2; }
size_t f_space_log(size_t n) { return ilog2(n + 1) + 1; }

double resource_simulation_overhead(size_t steps) {
    /* Overhead of simulating one UTM on another: O(log steps) factor.
     * Specifically, the overhead is multiplicative: t' = O(t log t). */
    if (steps == 0) return 1.0;
    return log2((double)steps + 1.0);
}

/* ==================================================================
 * L5: Program Counting / Universal Distribution
 *
 * m(x) = sum_{p: U(p)=x} 2^{-|p|}
 *
 * Kraft inequality: sum_{p in prefix-free set} 2^{-|p|} <= 1
 * ================================================================== */

size_t resource_count_producing(const char *output, size_t out_len,
                                 size_t max_prog_len, size_t time_limit) {
    if (!output || out_len == 0) return 0;
    size_t count = 0;
    char outbuf[4096];
    for (size_t plen = 1; plen <= max_prog_len && plen <= 256; plen++) {
        size_t total = ((size_t)1) << plen;
        if (total > 256) total = 256;
        for (size_t idx = 0; idx < total; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++)
                prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            /* Simple UTM run */
            char tape[1024]; memset(tape, 0, sizeof(tape));
            size_t head = 0, steps = 0;
            for (size_t ip = 0; ip + 1 < plen && steps < time_limit; ip += 2) {
                steps++;
                if (prog[ip] == '0' && prog[ip+1] == '0') { if (head > 0) head--; }
                else if (prog[ip] == '0' && prog[ip+1] == '1') { head++; if (head >= 1024) head--; }
                else if (prog[ip] == '1' && prog[ip+1] == '0') tape[head] ^= 1;
                else break;
            }
            size_t olen = 0;
            for (size_t i = 0; i < 1024 && i < 4096; i++) {
                outbuf[i] = tape[i] ? '1' : '0';
                olen++;
            }
            if (olen >= out_len) {
                bool match = true;
                for (size_t i = 0; i < out_len; i++)
                    if (outbuf[i] != output[i]) { match = false; break; }
                if (match) count++;
            }
        }
    }
    return count;
}

double resource_universal_distribution(const char *x, size_t len,
                                        size_t max_prog_len, size_t time_limit) {
    if (!x || len == 0) return 0.0;
    double m = 0.0;
    char outbuf[4096];
    for (size_t plen = 1; plen <= max_prog_len && plen <= 64; plen++) {
        size_t total = ((size_t)1) << plen;
        if (total > 256) total = 256;
        for (size_t idx = 0; idx < total; idx++) {
            char prog[256];
            for (size_t b = 0; b < plen; b++)
                prog[b] = ((idx >> b) & 1) ? '1' : '0';
            prog[plen] = '\0';
            char tape[1024]; memset(tape, 0, sizeof(tape));
            size_t head = 0, steps = 0;
            for (size_t ip = 0; ip + 1 < plen && steps < time_limit; ip += 2) {
                steps++;
                if (prog[ip] == '0' && prog[ip+1] == '0') { if (head > 0) head--; }
                else if (prog[ip] == '0' && prog[ip+1] == '1') { head++; if (head >= 1024) head--; }
                else if (prog[ip] == '1' && prog[ip+1] == '0') tape[head] ^= 1;
                else break;
            }
            size_t olen = 0;
            for (size_t i = 0; i < 1024 && i < 4096; i++) {
                outbuf[i] = tape[i] ? '1' : '0';
                olen++;
            }
            if (olen >= len) {
                bool match = true;
                for (size_t i = 0; i < len; i++)
                    if (outbuf[i] != x[i]) { match = false; break; }
                if (match) m += pow(2.0, -(double)plen);
            }
        }
    }
    return m;
}

double resource_kraft_sum(size_t max_len) {
    double total = 0.0;
    for (size_t n = 1; n <= max_len && n <= 20; n++) {
        size_t count = program_pool_count_at_length(n, true);
        total += (double)count * pow(2.0, -(double)n);
    }
    return total;
}

double resource_solomonoff_next_bit(const char *history, size_t hist_len,
                                     size_t max_prog_len, size_t time_limit) {
    if (!history || hist_len == 0) return 0.5;
    /* P(next=1 | history) = m(history·1) / m(history)
     * Approximate using resource_universal_distribution. */
    char full0[512], full1[512];
    size_t cp = hist_len < 255 ? hist_len : 255;
    memcpy(full0, history, cp); full0[cp] = '0'; full0[cp+1] = '\0';
    memcpy(full1, history, cp); full1[cp] = '1'; full1[cp+1] = '\0';
    double m_hist = resource_universal_distribution(history, cp, max_prog_len, time_limit);
    double m_full1 = resource_universal_distribution(full1, cp + 1, max_prog_len, time_limit);
    if (m_hist <= 0.0) return 0.5;
    double prob = m_full1 / m_hist;
    return (prob > 1.0) ? 1.0 : ((prob < 0.0) ? 0.0 : prob);
}

/* ==================================================================
 * L6: Binary String Utilities
 *
 * Self-delimiting encoding: x -> 1^{|x|}0x
 * Pair encoding: <x,y> = encode(x)||encode(y)
 * ================================================================== */

char* binary_self_delimiting_encode(const char *x, size_t len, size_t *out_len) {
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

char* binary_self_delimiting_decode(const char *enc, size_t enc_len,
                                     size_t *out_len) {
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

char* binary_pair_encode(const char *x, size_t xlen,
                          const char *y, size_t ylen, size_t *out_len) {
    if (!x && !y) { if (out_len) *out_len = 0; return NULL; }
    size_t exlen = 0, eylen = 0;
    char *ex = NULL, *ey = NULL;
    if (x) ex = binary_self_delimiting_encode(x, xlen, &exlen);
    if (y) ey = binary_self_delimiting_encode(y, ylen, &eylen);
    size_t total = exlen + eylen;
    char *pair = (char*)malloc(total + 1);
    if (!pair) { free(ex); free(ey);
        if (out_len) *out_len = 0; return NULL; }
    if (ex) { memcpy(pair, ex, exlen); free(ex); }
    if (ey) { memcpy(pair + exlen, ey, eylen); free(ey); }
    pair[total] = '\0';
    if (out_len) *out_len = total;
    return pair;
}

bool binary_pair_decode(const char *pair, size_t pair_len,
                         char **x, size_t *xlen, char **y, size_t *ylen) {
    *x = NULL; *y = NULL; *xlen = 0; *ylen = 0;
    if (!pair || pair_len < 4) return false;
    char *dx = binary_self_delimiting_decode(pair, pair_len, xlen);
    if (dx) {
        *x = dx;
        size_t consumed = 2 * (*xlen) + 1;
        if (consumed < pair_len) {
            char *dy = binary_self_delimiting_decode(pair + consumed,
                                                      pair_len - consumed, ylen);
            *y = dy;
            return true;
        }
    }
    return (*x != NULL);
}

size_t binary_ilog2(size_t n) {
    size_t r = 0;
    while (n >>= 1) r++;
    return r;
}

void binary_print(const char *x, size_t len, const char *label) {
    if (!x) return;
    printf("%s [%zu bits]: ", label ? label : "data", len);
    for (size_t i = 0; i < len && i < 128; i++) putchar(x[i]);
    if (len > 128) printf("...");
    printf("\n");
}

void resource_profile_print(const ResourceProfile *rp) {
    if (!rp) return;
    printf("Resource Profile: tested=%zu, outputs=%zu, time=%zu, space=%zu\n",
           rp->programs_tested, rp->outputs_collected,
           rp->total_time_used, rp->total_space_used);
    printf("  timeouts=%zu, space_violations=%zu, accept=%.2f%%\n",
           rp->timeouts, rp->space_violations, rp->acceptance_ratio * 100.0);
}
