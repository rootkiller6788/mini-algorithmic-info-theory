#ifndef UNIVERSAL_TM_H
#define UNIVERSAL_TM_H

#include "binary_string.h"
#include <stdint.h>

/* ============================================================
 * Universal Turing Machine
 *
 * A UTM can simulate any other Turing machine given its description.
 * The existence of UTMs is fundamental: it enables the definition
 * of Kolmogorov complexity and Ω (both depend on a fixed UTM).
 *
 * Knowledge points:
 *   - Universal Turing machine definition
 *   - Encoding arbitrary TMs for the UTM
 *   - Invariance theorem for Kolmogorov complexity
 *   - Standard encoding schemes
 *   - Self-interpreter (UTM interpreting itself)
 * ============================================================ */

#define UTM_TAPE_SIZE    1024
#define UTM_MAX_STATES   64
#define UTM_MAX_SYMBOLS  16

/* ---- Turing Machine Description ---- */
typedef struct {
    uint8_t num_states;
    uint8_t num_symbols;     /* tape alphabet size (includes blank=0) */
    uint8_t start_state;
    uint8_t halt_state;
    /* transition[state][symbol] = (next_state, write_symbol, move_dir)
     * move_dir: 0=left, 1=right, 2=stay
     * Packed: (next_state << 16) | (write_symbol << 8) | move_dir */
    uint32_t* transitions;   /* flat array: [state * num_symbols + symbol] */
} TMDescription;

TMDescription* tm_create(uint8_t states, uint8_t symbols);
void           tm_free(TMDescription* tm);
int            tm_set_transition(TMDescription* tm, uint8_t state, uint8_t symbol,
                                  uint8_t next_state, uint8_t write, uint8_t move);
void           tm_print(const TMDescription* tm);

/* ---- TM Configuration (instantaneous description) ---- */
typedef struct {
    uint8_t    tape[UTM_TAPE_SIZE];
    size_t     head_pos;
    uint8_t    state;
    size_t     steps;
    int        halted;
    int        error;
} TMConfig;

TMConfig* tm_config_create(void);
void      tm_config_free(TMConfig* config);
void      tm_config_reset(TMConfig* config);
int       tm_config_set_input(TMConfig* config, const BitString* input);

/* ---- TM Simulation ---- */
int  tm_step(TMConfig* config, const TMDescription* tm);
int  tm_run(TMConfig* config, const TMDescription* tm, size_t max_steps);
void tm_config_print(const TMConfig* config, const TMDescription* tm);

/* ---- Universal TM ---- */
/* UTM takes (TM_encoding, input) and simulates.
 * The encoding must be self-delimiting so UTM knows where
 * the TM description ends and the input begins. */

/* Encode a TM description as a self-delimiting bit string */
BitString* utm_encode_tm(const TMDescription* tm);
TMDescription* utm_decode_tm(const BitString* bs, size_t* bits_read);

/* The UTM itself: given a program (encoded TM + input),
 * simulate and return result. */
typedef struct {
    TMDescription** library;    /* library of known TMs */
    size_t          library_size;
    TMConfig*       config;
    TMDescription*  current_tm;
} UniversalTM;

UniversalTM* utm_create(void);
void         utm_free(UniversalTM* utm);
int          utm_load_program(UniversalTM* utm, const BitString* program);
int          utm_run(UniversalTM* utm, size_t max_steps);
int          utm_halted(const UniversalTM* utm);
BitString*   utm_read_output(const UniversalTM* utm);

/* ---- Standard TM Encodings ---- */
BitString* utm_standard_encode(const TMDescription* tm);
TMDescription* utm_standard_decode(const BitString* bs);

/* ---- Small Universal Turing Machines ---- */
/* Known small UTMs from the literature (Minsky, Rogozhin, etc.).
 * These are concrete, minimal UTMs with few states and symbols. */

/* Create Minsky's (4,7) UTM: 4 states, 7 symbols */
TMDescription* utm_minsky_4_7(void);

/* Create Rogozhin's (4,6) UTM */
TMDescription* utm_rogozhin_4_6(void);

/* Create a minimal (2,18) semi-weak UTM */
TMDescription* utm_minimal_2_18(void);

/* ---- Invariance Theorem Demo ---- */
/* Given two UTMs U and V, for any x: |K_U(x) - K_V(x)| ≤ c_{U,V}.
 * This function demonstrates the additive constant relationship. */
int utm_invariance_demo(UniversalTM* u1, UniversalTM* u2,
                        const BitString* test_string, size_t step_limit);

#endif /* UNIVERSAL_TM_H */
