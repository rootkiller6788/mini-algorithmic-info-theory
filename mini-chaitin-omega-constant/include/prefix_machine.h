#ifndef PREFIX_MACHINE_H
#define PREFIX_MACHINE_H

#include "binary_string.h"
#include <stdint.h>

/* ============================================================
 * Prefix-Free Turing Machine
 *
 * A prefix-free machine is a TM M such that if M(p) halts,
 * then for any proper prefix p' of p, M(p') does NOT halt.
 * Equivalently: the domain of M is a prefix-free set.
 *
 * Self-delimiting programs: the machine knows where the
 * program ends without an explicit delimiter.
 *
 * Knowledge points:
 *   - Definition and simulation of prefix-free TMs
 *   - Register machine model (simple prefix-free UTM)
 *   - Enumeration theorem for prefix-free machines
 *   - Optimal prefix-free machine (additively optimal)
 * ============================================================ */

/* Maximum steps before we give up on halting (for bounded sim) */
#define PFM_MAX_STEPS   10000000
#define PFM_MAX_PROGRAM 64
#define PFM_MAX_MEMORY  256

/* ---- Register Machine (kind of prefix-free TM) ---- */
/* A simple register machine with instructions encoded in binary.
 * Self-delimiting property: program format includes length prefix. */

typedef enum {
    OP_HALT  = 0,
    OP_INC   = 1,   /* increment register and go to next */
    OP_DECJZ = 2,   /* if register=0 jump, else decrement */
    OP_NEXT  = 3,   /* unconditional next */
} RMOpcode;

typedef struct {
    RMOpcode opcode;
    uint8_t  reg;       /* register index */
    uint16_t jump_addr; /* jump target (for DECJZ) */
} RMInstruction;

typedef struct {
    RMInstruction* program;
    size_t         program_len;
    size_t         program_cap;
} RegisterProgram;

RegisterProgram* rp_create(void);
void             rp_free(RegisterProgram* rp);
int              rp_add_instruction(RegisterProgram* rp, RMOpcode op, uint8_t reg, uint16_t jump);
void             rp_print(const RegisterProgram* rp);

/* ---- Register Machine Execution State ---- */
typedef struct {
    uint64_t regs[8];    /* 8 registers */
    size_t   ip;          /* instruction pointer */
    size_t   steps;       /* step counter */
    int      halted;      /* halting flag */
    int      error;       /* error flag (e.g., out of bounds) */
} RMState;

RMState* rm_state_create(void);
void     rm_state_free(RMState* state);
void     rm_state_reset(RMState* state);

/* ---- Execution ---- */
int  rm_execute_one(RMState* state, const RegisterProgram* prog);
int  rm_run(RMState* state, const RegisterProgram* prog, size_t max_steps);
int  rm_run_with_input(RMState* state, const RegisterProgram* prog,
                       const uint64_t* inputs, size_t num_inputs,
                       size_t max_steps);

/* ---- Prefix-free encoding of programs ---- */
/* A program is a self-delimiting binary string.
 * We encode the register machine program as:
 *   Elias-gamma(length) ++ program_bits
 * This ensures no proper prefix is a valid program. */

BitString* pfm_encode_program(const RegisterProgram* prog);
RegisterProgram* pfm_decode_program(const BitString* bs, size_t* bits_read);

/* ---- Check if a set of programs is prefix-free ---- */
int pfm_is_prefix_free_set(RegisterProgram** progs, size_t count);

/* ---- Optimal prefix-free machine ---- */
/* An optimal prefix-free machine V has the property:
 * For any prefix-free machine U, there exists constant c such that
 *   K_V(x) ≤ K_U(x) + c   for all x.
 * We simulate a small optimal machine by interpreting input as
 *   (index_of_machine, program_for_that_machine)
 * encoded in a self-delimiting way. */

typedef struct {
    RegisterProgram* machines[16];  /* up to 16 registered machines */
    size_t           num_machines;
    RegisterProgram* current;
    RMState*         state;
} OptimalPFM;

OptimalPFM* opfm_create(void);
void        opfm_free(OptimalPFM* opfm);
int         opfm_register_machine(OptimalPFM* opfm, const RegisterProgram* prog);
int         opfm_set_input(OptimalPFM* opfm, const BitString* input);
int         opfm_run(OptimalPFM* opfm, size_t max_steps);
int         opfm_halted(const OptimalPFM* opfm);
uint64_t    opfm_output(const OptimalPFM* opfm);

#endif /* PREFIX_MACHINE_H */
