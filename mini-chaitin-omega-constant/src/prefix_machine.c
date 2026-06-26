#include "prefix_machine.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* ============================================================
 * Prefix-Free Machine — Complete Implementation
 *
 * Encodes knowledge points:
 *   L1: RegisterProgram, RMState, OptimalPFM (typedef struct)
 *   L2: Prefix-free property in machine domain
 *   L3: Self-delimiting program encoding
 *   L4: Enumeration theorem for prefix-free machines
 *   L5: Optimal PFM construction and simulation
 *   L6: Register machine model as concrete prefix-free TM
 *
 * Reference: Chaitin (1975), Li & Vitányi (2019), Minsky (1967)
 * ============================================================ */

/* ── Register Program Lifecycle ───────────────────────────── */

RegisterProgram* rp_create(void) {
    RegisterProgram* rp = (RegisterProgram*)calloc(1, sizeof(RegisterProgram));
    if (!rp) return NULL;
    rp->program_cap = 128;
    rp->program = (RMInstruction*)calloc(rp->program_cap, sizeof(RMInstruction));
    if (!rp->program) { free(rp); return NULL; }
    rp->program_len = 0;
    return rp;
}

void rp_free(RegisterProgram* rp) {
    if (!rp) return;
    free(rp->program);
    free(rp);
}

int rp_add_instruction(RegisterProgram* rp, RMOpcode op, uint8_t reg, uint16_t jump) {
    if (!rp) return -1;
    if (rp->program_len >= rp->program_cap) {
        size_t new_cap = rp->program_cap * 2;
        RMInstruction* new_prog = (RMInstruction*)realloc(rp->program,
                                            new_cap * sizeof(RMInstruction));
        if (!new_prog) return -1;
        rp->program = new_prog;
        rp->program_cap = new_cap;
    }
    rp->program[rp->program_len].opcode = op;
    rp->program[rp->program_len].reg = reg;
    rp->program[rp->program_len].jump_addr = jump;
    rp->program_len++;
    return 0;
}

void rp_print(const RegisterProgram* rp) {
    if (!rp) { printf("(null program)\n"); return; }
    printf("RegisterProgram [%zu instructions]:\n", rp->program_len);
    const char* opnames[] = {"HALT", "INC", "DECJZ", "NEXT"};
    for (size_t i = 0; i < rp->program_len; i++) {
        const RMInstruction* ins = &rp->program[i];
        if (ins->opcode == OP_DECJZ) {
            printf("  [%2zu] %s R%d -> %u\n",
                   i, opnames[ins->opcode], ins->reg, ins->jump_addr);
        } else {
            printf("  [%2zu] %s R%d\n",
                   i, opnames[ins->opcode], ins->reg);
        }
    }
}

/* ── RM State Lifecycle ───────────────────────────────────── */

RMState* rm_state_create(void) {
    RMState* state = (RMState*)calloc(1, sizeof(RMState));
    return state;
}

void rm_state_free(RMState* state) {
    free(state);
}

void rm_state_reset(RMState* state) {
    if (!state) return;
    memset(state->regs, 0, sizeof(state->regs));
    state->ip = 0;
    state->steps = 0;
    state->halted = 0;
    state->error = 0;
}

/* ── Execution ────────────────────────────────────────────── */
/* Knowledge point L5: Register machine execution semantics.
 * The register machine is a minimal prefix-free computation model.
 * Instructions: INC R_i, DECJZ R_i j, HALT */

int rm_execute_one(RMState* state, const RegisterProgram* prog) {
    if (!state || !prog) return -1;
    if (state->halted || state->error) return 0;
    if (state->ip >= prog->program_len) {
        state->halted = 1;
        return 0;
    }
    const RMInstruction* ins = &prog->program[state->ip];
    state->steps++;
    switch (ins->opcode) {
    case OP_HALT:
        state->halted = 1;
        break;
    case OP_INC:
        if (ins->reg >= 8) { state->error = 1; return -1; }
        state->regs[ins->reg]++;
        state->ip++;
        break;
    case OP_DECJZ:
        if (ins->reg >= 8) { state->error = 1; return -1; }
        if (state->regs[ins->reg] == 0) {
            state->ip = ins->jump_addr;
        } else {
            state->regs[ins->reg]--;
            state->ip++;
        }
        break;
    case OP_NEXT:
        state->ip++;
        break;
    default:
        state->error = 1;
        return -1;
    }
    return 0;
}

int rm_run(RMState* state, const RegisterProgram* prog, size_t max_steps) {
    if (!state || !prog) return -1;
    while (!state->halted && !state->error && state->steps < max_steps) {
        if (rm_execute_one(state, prog) != 0) return -1;
    }
    return state->halted ? 1 : 0;
}

int rm_run_with_input(RMState* state, const RegisterProgram* prog,
                      const uint64_t* inputs, size_t num_inputs,
                      size_t max_steps) {
    if (!state || !prog) return -1;
    rm_state_reset(state);
    for (size_t i = 0; i < num_inputs && i < 8; i++)
        state->regs[i] = inputs[i];
    return rm_run(state, prog, max_steps);
}

/* ── Prefix-Free Encoding of Programs ─────────────────────── */
/* Knowledge point L3: Self-delimiting encoding.
 * Format: Elias-gamma(instruction_count) then each instruction
 * encoded in fixed width. This ensures no proper prefix of
 * a valid program encoding is itself a valid program encoding. */

BitString* pfm_encode_program(const RegisterProgram* prog) {
    if (!prog) return NULL;
    BitString* result = bs_create(NULL, 0);
    if (!result) return NULL;
    /* Header: Elias-gamma(prog_len) */
    BitString* len_enc = bs_encode_elias_gamma((uint64_t)prog->program_len);
    if (!len_enc) { bs_free(result); return NULL; }
    bs_append_bits(result, len_enc);
    bs_free(len_enc);
    /* Each instruction: 2 bits opcode + 3 bits reg + 12 bits jump = 17 bits */
    for (size_t i = 0; i < prog->program_len; i++) {
        const RMInstruction* ins = &prog->program[i];
        /* Opcode: 2 bits */
        bs_append_bit(result, (ins->opcode >> 1) & 1);
        bs_append_bit(result, ins->opcode & 1);
        /* Register: 3 bits */
        for (int b = 2; b >= 0; b--)
            bs_append_bit(result, (ins->reg >> b) & 1);
        /* Jump address: 12 bits */
        for (int b = 11; b >= 0; b--)
            bs_append_bit(result, (ins->jump_addr >> b) & 1);
    }
    return result;
}

RegisterProgram* pfm_decode_program(const BitString* bs, size_t* bits_read) {
    if (!bs) return NULL;
    size_t consumed = 0;
    uint64_t inst_count = bs_decode_elias_gamma(bs, &consumed);
    if (inst_count > 1024) { if (bits_read) *bits_read = consumed; return NULL; }
    RegisterProgram* prog = rp_create();
    if (!prog) { if (bits_read) *bits_read = consumed; return NULL; }
    for (uint64_t i = 0; i < inst_count; i++) {
        if (consumed + 17 > bs->length) {
            rp_free(prog);
            if (bits_read) *bits_read = consumed;
            return NULL;
        }
        /* Read opcode: 2 bits */
        uint8_t op = (uint8_t)((bs_get_bit(bs, consumed) << 1) |
                                bs_get_bit(bs, consumed + 1));
        consumed += 2;
        /* Read register: 3 bits */
        uint8_t reg = 0;
        for (int b = 0; b < 3; b++, consumed++)
            reg = (uint8_t)((reg << 1) | bs_get_bit(bs, consumed));
        /* Read jump: 12 bits */
        uint16_t jump = 0;
        for (int b = 0; b < 12; b++, consumed++)
            jump = (uint16_t)((jump << 1) | bs_get_bit(bs, consumed));
        rp_add_instruction(prog, (RMOpcode)op, reg, jump);
    }
    if (bits_read) *bits_read = consumed;
    return prog;
}

/* ── Prefix-Free Set Check for Programs ──────────────────── */

int pfm_is_prefix_free_set(RegisterProgram** progs, size_t count) {
    if (!progs || count <= 1) return 1;
    /* Encode each program and check prefix-free property */
    BitString** encoded = (BitString**)malloc(count * sizeof(BitString*));
    if (!encoded) return 0;
    for (size_t i = 0; i < count; i++) {
        encoded[i] = pfm_encode_program(progs[i]);
        if (!encoded[i]) {
            for (size_t j = 0; j < i; j++) bs_free(encoded[j]);
            free(encoded);
            return 0;
        }
    }
    /* Check pairwise */
    int is_pf = 1;
    for (size_t i = 0; i < count && is_pf; i++) {
        for (size_t j = i + 1; j < count && is_pf; j++) {
            if (bs_is_prefix(encoded[i], encoded[j]) ||
                bs_is_prefix(encoded[j], encoded[i]))
                is_pf = 0;
        }
    }
    for (size_t i = 0; i < count; i++) bs_free(encoded[i]);
    free(encoded);
    return is_pf;
}

/* ── Optimal Prefix-Free Machine ──────────────────────────── */
/* Knowledge point L4: Optimal prefix-free machine.
 * An optimal PFM V satisfies: for any PFM U, ∃c ∀x: K_V(x) ≤ K_U(x) + c.
 * Construction: V's domain is a universal prefix-free set that
 * encodes (index_of_U, program_for_U) in a self-delimiting way.
 *
 * Knowledge point L5: Simulation via indexed machine library.
 * The optimal machine interleaves multiple sub-machines,
 * each accessed by a self-delimiting index prefix. */

OptimalPFM* opfm_create(void) {
    OptimalPFM* opfm = (OptimalPFM*)calloc(1, sizeof(OptimalPFM));
    if (!opfm) return NULL;
    opfm->num_machines = 0;
    opfm->state = rm_state_create();
    if (!opfm->state) { free(opfm); return NULL; }
    return opfm;
}

void opfm_free(OptimalPFM* opfm) {
    if (!opfm) return;
    for (size_t i = 0; i < opfm->num_machines; i++)
        rp_free(opfm->machines[i]);
    rm_state_free(opfm->state);
    free(opfm);
}

int opfm_register_machine(OptimalPFM* opfm, const RegisterProgram* prog) {
    if (!opfm || !prog) return -1;
    if (opfm->num_machines >= 16) return -1;
    /* Copy the program */
    RegisterProgram* copy = rp_create();
    if (!copy) return -1;
    for (size_t i = 0; i < prog->program_len; i++) {
        const RMInstruction* ins = &prog->program[i];
        rp_add_instruction(copy, ins->opcode, ins->reg, ins->jump_addr);
    }
    opfm->machines[opfm->num_machines++] = copy;
    return 0;
}

int opfm_set_input(OptimalPFM* opfm, const BitString* input) {
    if (!opfm || !input) return -1;
    /* Parse input: first decode the machine index using Elias-gamma,
     * then the rest is the program for that sub-machine */
    size_t consumed = 0;
    uint64_t machine_idx = bs_decode_elias_gamma(input, &consumed);
    if (machine_idx >= opfm->num_machines) return -1;
    /* Remaining bits encode the program for the selected machine */
    size_t remaining = input->length - consumed;
    if (remaining > 0) {
        BitString* prog_bs = bs_substring(input, consumed, remaining);
        if (!prog_bs) return -1;
        size_t prog_read = 0;
        RegisterProgram* prog = pfm_decode_program(prog_bs, &prog_read);
        bs_free(prog_bs);
        if (!prog) return -1;
        /* Use this as an extra machine and run it */
        if (opfm->num_machines < 16) {
            opfm->machines[opfm->num_machines++] = prog;
        } else {
            rp_free(prog);
            return -1;
        }
    }
    opfm->current = opfm->machines[machine_idx];
    rm_state_reset(opfm->state);
    return 0;
}

int opfm_run(OptimalPFM* opfm, size_t max_steps) {
    if (!opfm || !opfm->current) return -1;
    return rm_run(opfm->state, opfm->current, max_steps);
}

int opfm_halted(const OptimalPFM* opfm) {
    return opfm && opfm->state ? opfm->state->halted : 0;
}

uint64_t opfm_output(const OptimalPFM* opfm) {
    if (!opfm || !opfm->state || !opfm->state->halted) return 0;
    return opfm->state->regs[0];
}