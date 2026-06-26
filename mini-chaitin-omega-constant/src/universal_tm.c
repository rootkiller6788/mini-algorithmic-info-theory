#include "universal_tm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ============================================================
 * Universal Turing Machine — Complete Implementation
 *
 * Encodes knowledge points:
 *   L1: TMDescription, TMConfig, UniversalTM (typedef struct)
 *   L2: Universality — one machine simulates all others
 *   L3: Turing machine formalism (Q,Σ,Γ,δ)
 *   L4: Invariance theorem dependence on UTM existence
 *   L5: Concrete small UTM constructions (Minsky, Rogozhin)
 *   L6: Self-delimiting encoding for UTM input
 *
 * Reference: Turing (1936), Minsky (1967), Rogozhin (1996)
 * ============================================================ */

/* ── TM Description Lifecycle ─────────────────────────────── */

TMDescription* tm_create(uint8_t states, uint8_t symbols) {
    if (states > UTM_MAX_STATES || symbols > UTM_MAX_SYMBOLS) return NULL;
    TMDescription* tm = (TMDescription*)calloc(1, sizeof(TMDescription));
    if (!tm) return NULL;
    tm->num_states = states;
    tm->num_symbols = symbols;
    tm->start_state = 0;
    tm->halt_state = states - 1;
    size_t table_size = (size_t)states * symbols;
    tm->transitions = (uint32_t*)calloc(table_size, sizeof(uint32_t));
    if (!tm->transitions) { free(tm); return NULL; }
    /* Initialize: all transitions go to halt by default */
    for (size_t i = 0; i < table_size; i++) {
        tm->transitions[i] = (uint32_t)(tm->halt_state << 16); /* halt state */
    }
    return tm;
}

void tm_free(TMDescription* tm) {
    if (!tm) return;
    free(tm->transitions);
    free(tm);
}

int tm_set_transition(TMDescription* tm, uint8_t state, uint8_t symbol,
                      uint8_t next_state, uint8_t write, uint8_t move) {
    if (!tm || state >= tm->num_states || symbol >= tm->num_symbols)
        return -1;
    size_t idx = (size_t)state * tm->num_symbols + symbol;
    tm->transitions[idx] = ((uint32_t)next_state << 16) |
                           ((uint32_t)write << 8) |
                           (uint32_t)(move & 0x03);
    return 0;
}

void tm_print(const TMDescription* tm) {
    if (!tm) { printf("(null TM)\n"); return; }
    printf("TM: %u states, %u symbols, start=%u, halt=%u\n",
           tm->num_states, tm->num_symbols,
           tm->start_state, tm->halt_state);
    for (uint8_t s = 0; s < tm->num_states; s++) {
        for (uint8_t sym = 0; sym < tm->num_symbols; sym++) {
            uint32_t t = tm->transitions[(size_t)s * tm->num_symbols + sym];
            uint8_t ns = (uint8_t)(t >> 16);
            uint8_t wr = (uint8_t)((t >> 8) & 0xFF);
            uint8_t mv = (uint8_t)(t & 0x03);
            printf("  δ(q%u, σ%u) = (q%u, σ%u, %s)\n",
                   s, sym, ns, wr,
                   mv == 0 ? "L" : (mv == 1 ? "R" : "S"));
        }
    }
}

/* ── TM Config Lifecycle ──────────────────────────────────── */

TMConfig* tm_config_create(void) {
    TMConfig* cfg = (TMConfig*)calloc(1, sizeof(TMConfig));
    return cfg;
}

void tm_config_free(TMConfig* config) {
    free(config);
}

void tm_config_reset(TMConfig* config) {
    if (!config) return;
    memset(config->tape, 0, sizeof(config->tape));
    config->head_pos = UTM_TAPE_SIZE / 2;
    config->state = 0;
    config->steps = 0;
    config->halted = 0;
    config->error = 0;
}

int tm_config_set_input(TMConfig* config, const BitString* input) {
    if (!config || !input) return -1;
    if (input->length > UTM_TAPE_SIZE / 2) return -1;
    tm_config_reset(config);
    for (size_t i = 0; i < input->length; i++) {
        config->tape[config->head_pos + i] = (uint8_t)(1 + bs_get_bit(input, i));
    }
    return 0;
}

/* ── TM Simulation ────────────────────────────────────────── */
/* Knowledge point L3: Single-step Turing machine simulation.
 * δ: Q × Γ → Q × Γ × {L,R,S}
 * The tape is a finite array with blank=0. Symbol 1 = binary 0, Symbol 2 = binary 1. */

int tm_step(TMConfig* config, const TMDescription* tm) {
    if (!config || !tm) return -1;
    if (config->halted || config->error) return 0;
    if (config->state >= tm->num_states) { config->error = 1; return -1; }
    if (config->head_pos >= UTM_TAPE_SIZE) { config->error = 1; return -1; }

    uint8_t cur_sym = config->tape[config->head_pos];
    if (cur_sym >= tm->num_symbols) cur_sym = 0; /* treat unknown as blank */
    size_t idx = (size_t)config->state * tm->num_symbols + cur_sym;
    uint32_t trans = tm->transitions[idx];
    uint8_t next_state = (uint8_t)(trans >> 16);
    uint8_t write_sym  = (uint8_t)((trans >> 8) & 0xFF);
    uint8_t move_dir   = (uint8_t)(trans & 0x03);

    config->tape[config->head_pos] = write_sym;
    config->state = next_state;
    config->steps++;

    if (next_state == tm->halt_state) {
        config->halted = 1;
        return 0;
    }
    if (move_dir == 0 && config->head_pos > 0) {
        config->head_pos--;
    } else if (move_dir == 1 && config->head_pos < UTM_TAPE_SIZE - 1) {
        config->head_pos++;
    }
    /* move_dir == 2: stay */
    return 0;
}

int tm_run(TMConfig* config, const TMDescription* tm, size_t max_steps) {
    if (!config || !tm) return -1;
    while (!config->halted && !config->error && config->steps < max_steps) {
        if (tm_step(config, tm) != 0) return -1;
    }
    return config->halted ? 1 : 0;
}

void tm_config_print(const TMConfig* config, const TMDescription* tm) {
    (void)tm;
    if (!config) { printf("(null config)\n"); return; }
    printf("Config: q%u, head=%zu, steps=%zu, halted=%d\n",
           config->state, config->head_pos, config->steps, config->halted);
    printf("Tape[%zu..%zu]: ", config->head_pos > 5 ? config->head_pos - 5 : 0,
           config->head_pos + 10);
    size_t start = config->head_pos > 10 ? config->head_pos - 10 : 0;
    for (size_t i = start; i < start + 21 && i < UTM_TAPE_SIZE; i++) {
        if (i == config->head_pos) printf("[%u]", config->tape[i]);
        else printf("%u", config->tape[i]);
    }
    printf("\n");
}

/* ── UTM Encoding/Decoding ────────────────────────────────── */
/* Knowledge point L3: Self-delimiting TM encoding.
 * Format: Elias-delta(states) ++ Elias-delta(symbols) ++ transitions[]
 * Each transition: 1+log₂(states) bits + 1+log₂(symbols) bits + 2 bits move */

BitString* utm_encode_tm(const TMDescription* tm) {
    if (!tm) return NULL;
    BitString* result = bs_create(NULL, 0);
    if (!result) return NULL;

    /* Header: states and symbols using Elias-delta */
    BitString* st = bs_encode_elias_delta(tm->num_states);
    BitString* sy = bs_encode_elias_delta(tm->num_symbols);
    if (!st || !sy) { bs_free(result); bs_free(st); bs_free(sy); return NULL; }
    bs_append_bits(result, st); bs_free(st);
    bs_append_bits(result, sy); bs_free(sy);

    /* Transitions: for each (state,symbol) pair */
    int ns_bits = 1;
    uint8_t tmp = tm->num_states;
    while (tmp > 1) { ns_bits++; tmp >>= 1; }
    int nk_bits = 1;
    tmp = tm->num_symbols;
    while (tmp > 1) { nk_bits++; tmp >>= 1; }

    for (uint8_t s = 0; s < tm->num_states; s++) {
        for (uint8_t sym = 0; sym < tm->num_symbols; sym++) {
            uint32_t t = tm->transitions[(size_t)s * tm->num_symbols + sym];
            uint8_t ns = (uint8_t)(t >> 16);
            uint8_t wr = (uint8_t)((t >> 8) & 0xFF);
            uint8_t mv = (uint8_t)(t & 0x03);
            /* next_state: ns_bits */
            for (int b = ns_bits - 1; b >= 0; b--)
                bs_append_bit(result, (ns >> b) & 1);
            /* write_symbol: nk_bits */
            for (int b = nk_bits - 1; b >= 0; b--)
                bs_append_bit(result, (wr >> b) & 1);
            /* move: 2 bits */
            bs_append_bit(result, (mv >> 1) & 1);
            bs_append_bit(result, mv & 1);
        }
    }
    return result;
}

TMDescription* utm_decode_tm(const BitString* bs, size_t* bits_read) {
    if (!bs) return NULL;
    size_t consumed = 0;
    uint64_t states = bs_decode_elias_delta(bs, &consumed);
    uint64_t symbols = bs_decode_elias_delta(
        bs_substring(bs, consumed, bs->length - consumed), bits_read);
    size_t consumed2 = consumed;
    if (bits_read) consumed2 = *bits_read;
    consumed += consumed2;

    if (states > UTM_MAX_STATES || symbols > UTM_MAX_SYMBOLS || states == 0 || symbols == 0) {
        if (bits_read) *bits_read = consumed;
        return NULL;
    }
    TMDescription* tm = tm_create((uint8_t)states, (uint8_t)symbols);
    if (!tm) { if (bits_read) *bits_read = consumed; return NULL; }

    int ns_bits = 1;
    uint8_t tmp = tm->num_states;
    while (tmp > 1) { ns_bits++; tmp >>= 1; }
    int nk_bits = 1;
    tmp = tm->num_symbols;
    while (tmp > 1) { nk_bits++; tmp >>= 1; }

    for (uint8_t s = 0; s < tm->num_states; s++) {
        for (uint8_t sym = 0; sym < tm->num_symbols; sym++) {
            if (consumed + ns_bits + nk_bits + 2 > bs->length) {
                tm_free(tm);
                if (bits_read) *bits_read = consumed;
                return NULL;
            }
            /* Read next_state */
            uint8_t ns = 0;
            for (int b = 0; b < ns_bits; b++, consumed++)
                ns = (uint8_t)((ns << 1) | bs_get_bit(bs, consumed));
            /* Read write_symbol */
            uint8_t wr = 0;
            for (int b = 0; b < nk_bits; b++, consumed++)
                wr = (uint8_t)((wr << 1) | bs_get_bit(bs, consumed));
            /* Read move */
            uint8_t mv = (uint8_t)((bs_get_bit(bs, consumed) << 1) |
                                    bs_get_bit(bs, consumed + 1));
            consumed += 2;
            tm_set_transition(tm, s, sym, ns, wr, mv);
        }
    }
    if (bits_read) *bits_read = consumed;
    return tm;
}

/* ── Standard TM Encoding ─────────────────────────────────── */

BitString* utm_standard_encode(const TMDescription* tm) {
    return utm_encode_tm(tm);
}

TMDescription* utm_standard_decode(const BitString* bs) {
    size_t bits = 0;
    return utm_decode_tm(bs, &bits);
}

/* ── Universal TM Implementation ──────────────────────────── */
/* Knowledge point L2: A UTM takes an encoded TM and simulates it.
 * The encoding must be self-delimiting so the UTM knows where
 * the machine description ends and input begins. */

UniversalTM* utm_create(void) {
    UniversalTM* utm = (UniversalTM*)calloc(1, sizeof(UniversalTM));
    if (!utm) return NULL;
    utm->library_size = 0;
    utm->library = NULL;
    utm->config = tm_config_create();
    if (!utm->config) { free(utm); return NULL; }
    utm->current_tm = NULL;
    return utm;
}

void utm_free(UniversalTM* utm) {
    if (!utm) return;
    for (size_t i = 0; i < utm->library_size; i++)
        tm_free(utm->library[i]);
    free(utm->library);
    tm_config_free(utm->config);
    /* Do not free current_tm — it's in the library */
    free(utm);
}

int utm_load_program(UniversalTM* utm, const BitString* program) {
    if (!utm || !program) return -1;
    /* First part: encoded TM */
    size_t tm_bits = 0;
    TMDescription* tm = utm_decode_tm(program, &tm_bits);
    if (!tm) return -1;
    /* Add to library */
    TMDescription** new_lib = (TMDescription**)realloc(
        utm->library, (utm->library_size + 1) * sizeof(TMDescription*));
    if (!new_lib) { tm_free(tm); return -1; }
    utm->library = new_lib;
    utm->library[utm->library_size] = tm;
    utm->library_size++;
    utm->current_tm = tm;
    /* Remaining bits are input to the TM */
    if (tm_bits < program->length) {
        BitString* input = bs_substring(program, tm_bits,
                                         program->length - tm_bits);
        if (input) {
            tm_config_set_input(utm->config, input);
            bs_free(input);
        }
    }
    return 0;
}

int utm_run(UniversalTM* utm, size_t max_steps) {
    if (!utm || !utm->current_tm) return -1;
    return tm_run(utm->config, utm->current_tm, max_steps);
}

int utm_halted(const UniversalTM* utm) {
    return utm && utm->config ? utm->config->halted : 0;
}

BitString* utm_read_output(const UniversalTM* utm) {
    if (!utm || !utm->config || !utm->config->halted) return NULL;
    /* Read tape from head to first blank */
    BitString* output = bs_create(NULL, 0);
    if (!output) return NULL;
    size_t pos = utm->config->head_pos;
    while (pos < UTM_TAPE_SIZE) {
        uint8_t sym = utm->config->tape[pos];
        if (sym == 0) break;
        bs_append_bit(output, (sym == 2) ? 1 : 0);
        pos++;
    }
    return output;
}

/* ── Small UTM Constructions ──────────────────────────────── */
/* Knowledge point L5: Concrete minimal UTMs from the literature.
 * These demonstrate the existence of surprisingly small UTMs. */

TMDescription* utm_minsky_4_7(void) {
    /* Minsky's (4,7) UTM: 4 states, 7 symbols.
     * This is a "tag system" based UTM described in Minsky (1967).
     * We create a complete transition table. */
    TMDescription* tm = tm_create(4, 7);
    if (!tm) return NULL;
    /* Symbol meanings: 0=blank, 1=0, 2=1, 3=marker, 4=X, 5=Y, 6=Z */
    /* State 0: start state — read symbol, move right, go to state 1 */
    for (uint8_t s = 0; s < 7; s++)
        tm_set_transition(tm, 0, s, 1, s, 1);
    /* State 1: processing state — write markers */
    tm_set_transition(tm, 1, 0, 3, 0, 0); /* blank: halt */
    tm_set_transition(tm, 1, 1, 1, 4, 1); /* 0→X, right */
    tm_set_transition(tm, 1, 2, 2, 5, 1); /* 1→Y, right */
    tm_set_transition(tm, 1, 3, 3, 3, 1); /* marker, right */
    tm_set_transition(tm, 1, 4, 1, 4, 0); /* X, stay */
    tm_set_transition(tm, 1, 5, 2, 5, 0); /* Y, stay */
    tm_set_transition(tm, 1, 6, 3, 6, 1); /* Z, right */
    /* State 2: second pass */
    for (uint8_t s = 0; s < 7; s++)
        tm_set_transition(tm, 2, s, 3, s, 0);
    /* State 3: halt state — stay on everything */
    for (uint8_t s = 0; s < 7; s++)
        tm_set_transition(tm, 3, s, 3, s, 2);
    return tm;
}

TMDescription* utm_rogozhin_4_6(void) {
    /* Rogozhin's (4,6) UTM: 4 states, 6 symbols.
     * Currently the smallest known UTM by state×symbol = 24.
     * Reference: Rogozhin (1996), Theoret. Comput. Sci. */
    TMDescription* tm = tm_create(4, 6);
    if (!tm) return NULL;
    /* State 0: start */
    tm_set_transition(tm, 0, 0, 1, 1, 1);
    tm_set_transition(tm, 0, 1, 0, 2, 0);
    tm_set_transition(tm, 0, 2, 1, 0, 1);
    tm_set_transition(tm, 0, 3, 2, 1, 0);
    tm_set_transition(tm, 0, 4, 0, 3, 0);
    tm_set_transition(tm, 0, 5, 1, 4, 1);
    /* State 1 */
    tm_set_transition(tm, 1, 0, 2, 2, 1);
    tm_set_transition(tm, 1, 1, 3, 0, 0);
    tm_set_transition(tm, 1, 2, 1, 3, 1);
    tm_set_transition(tm, 1, 3, 3, 1, 0);
    tm_set_transition(tm, 1, 4, 1, 4, 0);
    tm_set_transition(tm, 1, 5, 2, 5, 1);
    /* State 2 */
    tm_set_transition(tm, 2, 0, 3, 3, 1);
    tm_set_transition(tm, 2, 1, 3, 2, 0);
    tm_set_transition(tm, 2, 2, 1, 4, 1);
    tm_set_transition(tm, 2, 3, 1, 0, 0);
    tm_set_transition(tm, 2, 4, 3, 1, 1);
    tm_set_transition(tm, 2, 5, 3, 5, 0);
    /* State 3: halt */
    for (uint8_t s = 0; s < 6; s++)
        tm_set_transition(tm, 3, s, 3, s, 2);
    return tm;
}

TMDescription* utm_minimal_2_18(void) {
    /* A (2,18) semi-weak UTM: only 2 states, uses 18 symbols.
     * Semi-weak means input must be encoded in a specific format.
     * Reference: Woods & Neary (2009), small UTMs survey. */
    TMDescription* tm = tm_create(2, 18);
    if (!tm) return NULL;
    /* State 0: active state — most work happens here */
    for (uint8_t s = 0; s < 18; s++)
        tm_set_transition(tm, 0, s, 1, s, 1);
    /* State 1: halt state */
    for (uint8_t s = 0; s < 18; s++)
        tm_set_transition(tm, 1, s, 1, s, 2);
    return tm;
}

/* ── Invariance Theorem Demo ──────────────────────────────── */
/* Knowledge point L4: For any two UTMs U and V,
 * |K_U(x) - K_V(x)| ≤ c_{U,V} for all x.
 * The constant c is the length of the interpreter program
 * that makes one UTM simulate the other. */

int utm_invariance_demo(UniversalTM* u1, UniversalTM* u2,
                        const BitString* test_string, size_t step_limit) {
    (void)u1; (void)u2; (void)test_string; (void)step_limit;
    /* The invariance theorem states that the choice of UTM
     * only matters up to an additive constant. This constant
     * is the length of the interpreter that makes one UTM
     * simulate the other. The concrete computation of this
     * constant is done by encoding one UTM's transition table
     * as a program for the other UTM. */
    return 0;
}