/**
 * universal_machine.c - Universal Monotone Machine Implementation
 *
 * Implements a 4-register universal monotone machine U.
 *
 * "Monotone" means: the output tape is write-only, and once a bit
 * is written, it cannot be modified. This models the type of machine
 * required for Solomonoff's M(x) definition.
 *
 * Instruction set (4-bit opcodes with optional operands):
 *   0x0 NOP          - No operation
 *   0x1 INC Rn       - Rn += 1
 *   0x2 DEC Rn       - Rn -= 1 (saturate at 0)
 *   0x3 LD Rn, [Rm]  - Rn = MEM[Rm]
 *   0x4 ST [Rn], Rm  - MEM[Rn] = Rm & 0xFF
 *   0x5 OUT Rn       - Output (Rn & 1) to output tape
 *   0x6 JNZ Rn, off  - If Rn != 0: PC += off (signed 8-bit)
 *   0x7 HALT         - Stop execution
 *   0x8 XOR Rn, Rm   - Rn ^= Rm
 *   0x9 SHL Rn, imm  - Rn <<= imm
 *   0xA CMP Rn, Rm   - Set flags based on Rn - Rm
 *   0xB CPY Rn, Rm   - Rn = Rm
 *   0xC LDK Rn, imm  - Rn = imm (4-bit immediate)
 *   0xD OUTB Rn      - Output 8 bits from Rn (LSB first)
 *   0xE ADDI Rn, imm - Rn += imm
 *   0xF SYS n        - System call (extensible)
 *
 * Reference: Minsky, "Computation: Finite and Infinite Machines", 1967.
 * Curriculum: MIT 6.045, CMU 15-251
 */

#include "universal_machine.h"
#include <string.h>
#include <stdlib.h>

/* Forward declaration */
static void um_set_flag(um_state_t *m, uint8_t flag, bool value);

void um_init(um_state_t *m, const binary_string_t *program, uint64_t step_limit) {
    if (!m || !program) return;
    memset(m, 0, sizeof(*m));
    bs_copy(&m->program, program);
    m->pc = 0;
    m->halted = false;
    m->timeout = false;
    memset(m->reg, 0, sizeof(m->reg));
    m->flags = 0;
    memset(m->mem, 0, sizeof(m->mem));
    m->mem_ptr = 0;
    bs_init(&m->output);
    m->output_bit_count = 0;
    m->step_count = 0;
    m->step_limit = step_limit;
    m->instructions_decoded = 0;
}

void um_reset(um_state_t *m) {
    if (!m) return;
    binary_string_t prog = m->program;
    uint64_t limit = m->step_limit;
    um_init(m, &prog, limit);
}

/**
 * um_decode_instruction - Decode variable-length instruction.
 *
 * Instructions are encoded in 4-bit nibbles:
 *   Nibble 0: [opcode(4)]
 *   For opcodes needing operands, subsequent nibbles encode registers
 *   and immediates.
 *
 * Encoding scheme:
 *   NOP, HALT: 1 nibble (4 bits)
 *   INC, DEC, OUT: 2 nibbles [opcode(4) | reg(2) | 00]
 *   LD, ST, XOR, CPY, CMP: 3 nibbles
 *   JNZ: 4 nibbles [op|ra|00] [rb|00|00] [offset(8)]
 *   SHL, ADDI, LDK: 3 nibbles [op|ra|00] [rb/imm(4)|0000]
 *   OUTB: 2 nibbles [op|ra|0000]
 *   SYS: 2 nibbles [op|sysno(4)]
 */

size_t um_decode_instruction(const binary_string_t *program, size_t bit_offset,
                              um_instruction_t *instr) {
    if (!program || !instr) return 0;
    memset(instr, 0, sizeof(*instr));
    instr->valid = false;

    if (bit_offset + 4 > program->length) return 0;

    /* Read opcode (4 bits starting at bit_offset) */
    uint8_t raw = 0;
    for (int i = 0; i < 4; i++) {
        int bit = bs_get_bit(program, bit_offset + i);
        if (bit < 0) return 0;
        raw = (raw << 1) | bit;
    }
    instr->opcode = (um_opcode_t)raw;
    instr->raw = raw;
    size_t consumed = 4;
    bit_offset += 4;

    switch (instr->opcode) {
    case UM_NOP:
    case UM_HALT:
        instr->valid = true;
        break;

    case UM_INC:
    case UM_DEC:
    case UM_OUT:
        if (bit_offset + 4 > program->length) return 0;
        {   uint8_t reg = 0;
            for (int i = 0; i < 2; i++) {
                int bit = bs_get_bit(program, bit_offset + i);
                if (bit < 0) return 0;
                reg = (reg << 1) | bit;
            }
            instr->reg_a = reg;
            consumed += 4;
            instr->valid = true;
        }
        break;

    case UM_LD:
    case UM_ST:
    case UM_XOR:
    case UM_CPY:
    case UM_CMP:
        if (bit_offset + 8 > program->length) return 0;
        {   uint8_t ra = 0, rb = 0;
            for (int i = 0; i < 2; i++) {
                ra = (ra << 1) | bs_get_bit(program, bit_offset + i);
                rb = (rb << 1) | bs_get_bit(program, bit_offset + 4 + i);
            }
            instr->reg_a = ra;
            instr->reg_b = rb;
            consumed += 8;
            instr->valid = true;
        }
        break;

    case UM_JNZ:
        if (bit_offset + 12 > program->length) return 0;
        {   uint8_t ra = 0;
            for (int i = 0; i < 2; i++)
                ra = (ra << 1) | bs_get_bit(program, bit_offset + i);
            int8_t off = 0;
            for (int i = 0; i < 8; i++) {
                int bit = bs_get_bit(program, bit_offset + 4 + i);
                if (bit < 0) return 0;
                off = (off << 1) | bit;
            }
            instr->reg_a = ra;
            instr->offset = off;
            consumed += 12;
            instr->valid = true;
        }
        break;

    case UM_SHL:
    case UM_ADDI:
    case UM_LDK:
        if (bit_offset + 8 > program->length) return 0;
        {   uint8_t ra = 0, imm = 0;
            for (int i = 0; i < 2; i++)
                ra = (ra << 1) | bs_get_bit(program, bit_offset + i);
            for (int i = 0; i < 4; i++)
                imm = (imm << 1) | bs_get_bit(program, bit_offset + 4 + i);
            instr->reg_a = ra;
            instr->immediate = imm;
            consumed += 8;
            instr->valid = true;
        }
        break;

    case UM_OUTB:
        if (bit_offset + 4 > program->length) return 0;
        {   uint8_t ra = 0;
            for (int i = 0; i < 2; i++)
                ra = (ra << 1) | bs_get_bit(program, bit_offset + i);
            instr->reg_a = ra;
            consumed += 4;
            instr->valid = true;
        }
        break;

    case UM_SYS:
        if (bit_offset + 4 > program->length) return 0;
        {   uint8_t n = 0;
            for (int i = 0; i < 4; i++) {
                int bit = bs_get_bit(program, bit_offset + i);
                if (bit < 0) return 0;
                n = (n << 1) | bit;
            }
            instr->immediate = n;
            consumed += 4;
            instr->valid = true;
        }
        break;
    }

    return consumed;
}

um_instruction_t um_decode(const um_state_t *m) {
    um_instruction_t instr;
    memset(&instr, 0, sizeof(instr));
    if (!m || m->halted) return instr;

    um_decode_instruction(&m->program, m->pc, &instr);
    return instr;
}

/**
 * um_step - Execute one instruction.
 *
 * Returns true if execution should continue, false on HALT or error.
 * The monotone property is enforced: output bits are only appended,
 * never modified.
 */
bool um_step(um_state_t *m) {
    if (!m || m->halted || m->timeout) return false;
    if (m->step_count >= m->step_limit) {
        m->timeout = true;
        return false;
    }
    if (m->pc >= m->program.length) {
        m->halted = true;
        return false;
    }

    um_instruction_t instr;
    size_t consumed = um_decode_instruction(&m->program, m->pc, &instr);
    if (!instr.valid || consumed == 0) {
        m->halted = true;
        return false;
    }

    m->pc += consumed;
    m->instructions_decoded++;
    m->step_count++;

    uint32_t *reg = m->reg;
    uint8_t ra = instr.reg_a;
    uint8_t rb = instr.reg_b;

    /* Ensure register indices are in bounds */
    if (ra >= UM_NUM_REGISTERS) ra = 0;
    if (rb >= UM_NUM_REGISTERS) rb = 0;

    switch (instr.opcode) {
    case UM_NOP:
        break;

    case UM_INC:
        reg[ra]++;
        um_set_flag(m, UM_FLAG_ZERO, reg[ra] == 0);
        um_set_flag(m, UM_FLAG_OVERFLOW, reg[ra] == 0 && reg[ra] == 0);
        break;

    case UM_DEC:
        if (reg[ra] > 0) reg[ra]--;
        um_set_flag(m, UM_FLAG_ZERO, reg[ra] == 0);
        break;

    case UM_LD:
        if (reg[rb] < UM_MEMORY_SIZE)
            reg[ra] = m->mem[reg[rb]];
        break;

    case UM_ST:
        if (reg[ra] < UM_MEMORY_SIZE)
            m->mem[reg[ra]] = (uint8_t)(reg[rb] & 0xFF);
        break;

    case UM_OUT:
        /* Monotone output: append one bit */
        bs_append_bit(&m->output, reg[ra] & 1);
        m->output_bit_count++;
        break;

    case UM_JNZ:
        if (reg[ra] != 0) {
            /* Apply signed offset to PC */
            if (instr.offset < 0 && m->pc < (size_t)(-instr.offset)) {
                m->pc = 0;
            } else {
                m->pc = (size_t)((int64_t)m->pc + instr.offset);
            }
        }
        break;

    case UM_HALT:
        m->halted = true;
        return false;

    case UM_XOR:
        reg[ra] ^= reg[rb];
        um_set_flag(m, UM_FLAG_ZERO, reg[ra] == 0);
        break;

    case UM_SHL:
        reg[ra] <<= (instr.immediate & 0x1F);
        um_set_flag(m, UM_FLAG_ZERO, reg[ra] == 0);
        break;

    case UM_CMP:
        {   uint32_t diff = reg[ra] - reg[rb];
            um_set_flag(m, UM_FLAG_ZERO, diff == 0);
            um_set_flag(m, UM_FLAG_CARRY, reg[ra] < reg[rb]);
            um_set_flag(m, UM_FLAG_NEGATIVE, (diff & 0x80000000) != 0);
        }
        break;

    case UM_CPY:
        reg[ra] = reg[rb];
        break;

    case UM_LDK:
        reg[ra] = instr.immediate & 0xF;
        break;

    case UM_OUTB:
        /* Output 8 bits of the register, LSB first */
        for (int i = 0; i < 8; i++) {
            bs_append_bit(&m->output, (reg[ra] >> i) & 1);
            m->output_bit_count++;
        }
        break;

    case UM_ADDI:
        reg[ra] += (instr.immediate & 0xF);
        um_set_flag(m, UM_FLAG_ZERO, reg[ra] == 0);
        break;

    case UM_SYS:
        switch (instr.immediate) {
        case 0:
            /* SYS 0: Output register 0 as 8 bits */
            for (int i = 0; i < 8; i++) {
                bs_append_bit(&m->output, (reg[0] >> i) & 1);
                m->output_bit_count++;
            }
            break;
        case 1:
            /* SYS 1: Copy memory block to output
               R0 = start address, R1 = length */
            {   uint32_t addr = reg[0];
                uint32_t len = reg[1];
                if (len > SOLOMONOFF_MAX_OUTPUT_LENGTH - m->output_bit_count)
                    len = SOLOMONOFF_MAX_OUTPUT_LENGTH - m->output_bit_count;
                for (uint32_t i = 0; i < len && (addr + i) < UM_MEMORY_SIZE; i++) {
                    uint8_t byte = m->mem[addr + i];
                    for (int b = 0; b < 8; b++) {
                        bs_append_bit(&m->output, (byte >> b) & 1);
                        m->output_bit_count++;
                    }
                }
            }
            break;
        default:
            break;  /* Unknown syscall: NOP */
        }
        break;
    }

    return true;
}

uint64_t um_run(um_state_t *m) {
    if (!m) return 0;
    while (um_step(m)) {
        /* continue */
    }
    return m->step_count;
}

const binary_string_t *um_get_output(const um_state_t *m) {
    return m ? &m->output : NULL;
}

bool um_output_has_prefix(const um_state_t *m, const binary_string_t *x) {
    if (!m || !x) return false;
    return bs_has_prefix(&m->output, x);
}

bool um_output_equals(const um_state_t *m, const binary_string_t *x) {
    if (!m || !x) return false;
    return bs_equal(&m->output, x);
}

/* ================================================================
 * Program Construction Utilities (L5)
 * ================================================================ */

/**
 * um_build_print_program - Build a program that outputs a literal string.
 *
 * Strategy: LDK R0, bit0; OUT R0; LDK R0, bit1; OUT R0; ... HALT
 *
 * Each bit requires ~12 bits of encoding (LDK + OUT).
 * Total program length = 12*|data| + 4 (HALT).
 *
 * This gives the trivial upper bound: K(x) <= 12*|x| + 4.
 * A smarter encoding would achieve K(x) <= |x| + O(1).
 */
size_t um_build_print_program(binary_string_t *program, const binary_string_t *data) {
    if (!program || !data) return 0;
    bs_init(program);

    for (size_t i = 0; i < data->length; i++) {
        int bit = bs_get_bit(data, i);
        if (bit < 0) break;

        /* LDK R0, bit: load immediate (0 or 1) into R0 */
        um_encode_instruction(program, 0, UM_LDK, 0, 0, (uint8_t)bit);

        /* OUT R0: output LSB of R0 */
        um_encode_instruction(program, 0, UM_OUT, 0, 0, 0);
    }

    /* HALT */
    um_encode_instruction(program, 0, UM_HALT, 0, 0, 0);

    return program->length;
}

size_t um_build_repeat_program(binary_string_t *program, const binary_string_t *pattern,
                                size_t repeat_count) {
    if (!program || !pattern || pattern->length == 0) return 0;
    bs_init(program);

    /* Unroll: print the pattern repeat_count times using LDK+OUT pairs */
    for (size_t r = 0; r < repeat_count && r < 128; r++) {
        for (size_t i = 0; i < pattern->length; i++) {
            int bit = bs_get_bit(pattern, i);
            if (bit < 0) break;
            um_encode_instruction(program, 0, UM_LDK, 0, 0, (uint8_t)bit);
            um_encode_instruction(program, 0, UM_OUT, 0, 0, 0);
        }
    }
    um_encode_instruction(program, 0, UM_HALT, 0, 0, 0);
    return program->length;
}

size_t um_build_xor_program(binary_string_t *program,
                             const binary_string_t *a, const binary_string_t *b) {
    if (!program || !a || !b) return 0;
    bs_init(program);

    size_t len = (a->length < b->length) ? a->length : b->length;
    for (size_t i = 0; i < len; i++) {
        int ba = bs_get_bit(a, i), bb = bs_get_bit(b, i);
        if (ba < 0 || bb < 0) break;
        int result_bit = ba ^ bb;
        um_encode_instruction(program, 0, UM_LDK, 0, 0, (uint8_t)result_bit);
        um_encode_instruction(program, 0, UM_OUT, 0, 0, 0);
    }
    um_encode_instruction(program, 0, UM_HALT, 0, 0, 0);
    return program->length;
}

/**
 * um_make_prefix_free - Wrap a raw program in self-delimiting encoding.
 *
 * Encodes: prefix_free(p) = Elias_Gamma(|p|) . p
 *
 * Elias Gamma encoding of n:
 *   Write floor(log2(n)) zeros, then the binary representation of n.
 *   E.g., n=1 -> "1", n=2 -> "010", n=3 -> "011", n=4 -> "00100".
 *
 * This ensures the set of programs is prefix-free, which is required
 * for prefix Kolmogorov complexity and the Kraft inequality.
 */
size_t um_make_prefix_free(binary_string_t *result, const binary_string_t *raw_program) {
    if (!result || !raw_program) return 0;
    bs_init(result);

    size_t n = raw_program->length;

    /* Elias Gamma encoding of n */
    if (n == 0) {
        bs_append_bit(result, 0);  /* Encode 0 as "0" */
    } else {
        /* Find floor(log2(n)) */
        size_t n2 = n;
        int bits = 0;
        while (n2 > 0) { n2 >>= 1; bits++; }

        /* Write (bits-1) zeros */
        for (int i = 0; i < bits - 1; i++) {
            bs_append_bit(result, 0);
        }
        /* Write binary representation of n */
        for (int i = bits - 1; i >= 0; i--) {
            bs_append_bit(result, (n >> i) & 1);
        }
    }

    /* Append the raw program */
    bs_concat(result, raw_program);

    return result->length;
}

/* ================================================================
 * Machine Analysis and Self-Test
 * ================================================================ */

uint64_t um_count_programs(size_t length) {
    if (length > 63) return UINT64_MAX;
    return 1ULL << length;
}

uint64_t um_estimate_halting(size_t max_length, uint64_t max_steps) {
    uint64_t count = 0;
    /* Sample a few programs to estimate halting ratio */
    for (size_t len = 0; len <= max_length && len <= 6; len++) {
        uint64_t num_progs = (len == 0) ? 1 : (1ULL << len);
        for (uint64_t i = 0; i < num_progs && i < 10; i++) {
            binary_string_t prog;
            bs_init(&prog);
            for (size_t b = 0; b < len; b++) {
                bs_append_bit(&prog, (i >> (len - 1 - b)) & 1);
            }
            um_state_t m;
            um_init(&m, &prog, max_steps);
            um_run(&m);
            if (m.halted) count++;
        }
    }
    return count;
}

/**
 * um_self_test - Verify machine correctness with known programs.
 *
 * Tests:
 *   1. "0100 0011 0111" (INC R0 x3 + HALT) -> R0 = 3
 *   2. Print program for "01" -> output = "01"
 *   3. HALT instruction terminates immediately
 *   4. Empty program halts (runs past program end)
 */
bool um_self_test(void) {
    /* Test 1: HALT terminates immediately */
    {
        binary_string_t prog;
        /* Encode HALT: opcode 0111 */
        bs_init(&prog);
        um_encode_instruction(&prog, 0, UM_HALT, 0, 0, 0);
        um_state_t m;
        um_init(&m, &prog, 100);
        um_run(&m);
        if (!m.halted) return false;
        if (m.step_count != 1) return false;
    }

    /* Test 2: NOP + HALT (execution continues after NOP) */
    {
        binary_string_t prog;
        bs_init(&prog);
        um_encode_instruction(&prog, 0, UM_NOP, 0, 0, 0);
        um_encode_instruction(&prog, 0, UM_HALT, 0, 0, 0);
        um_state_t m;
        um_init(&m, &prog, 100);
        um_run(&m);
        if (!m.halted) return false;
        if (m.step_count != 2) return false;
    }

    /* Test 3: Print a known string via build_print_program */
    {
        binary_string_t data;
        bs_from_cstr(&data, "01");
        binary_string_t prog;
        um_build_print_program(&prog, &data);
        um_state_t m;
        um_init(&m, &prog, 100000);
        um_run(&m);
        if (!m.halted) return false;
        if (!bs_equal(&m.output, &data)) return false;
    }

    /* Test 4: Print empty string */
    {
        binary_string_t empty;
        bs_init(&empty);
        binary_string_t prog;
        um_build_print_program(&prog, &empty);
        um_state_t m;
        um_init(&m, &prog, 1000);
        um_run(&m);
        if (!m.halted) return false;
        if (m.output.length != 0) return false;
    }

    /* Test 5: INC R0 x3 via encoded instructions */
    {
        binary_string_t prog;
        bs_init(&prog);
        /* INC R0: opcode INC=0x1, reg_a=0 */
        um_encode_instruction(&prog, 0, UM_INC, 0, 0, 0);
        um_encode_instruction(&prog, 0, UM_INC, 0, 0, 0);
        um_encode_instruction(&prog, 0, UM_INC, 0, 0, 0);
        um_encode_instruction(&prog, 0, UM_HALT, 0, 0, 0);
        um_state_t m;
        um_init(&m, &prog, 1000);
        um_run(&m);
        if (!m.halted) return false;
        if (m.reg[0] != 3) return false;
    }

    /* Test 6: LDK + OUT produces expected output */
    {
        binary_string_t prog;
        bs_init(&prog);
        /* LDK R0, 1 (bit=1) -> OUT R0 -> HALT */
        um_encode_instruction(&prog, 0, UM_LDK, 0, 0, 1);  /* R0 = 1 */
        um_encode_instruction(&prog, 0, UM_OUT, 0, 0, 0);  /* output LSB of R0 */
        um_encode_instruction(&prog, 0, UM_HALT, 0, 0, 0);
        um_state_t m;
        um_init(&m, &prog, 1000);
        um_run(&m);
        if (!m.halted) return false;
        /* R0=1, output LSB = 1 */
        if (m.output.length != 1) return false;
        if (bs_get_bit(&m.output, 0) != 1) return false;
    }

    /* Test 7: XOR self = 0 */
    {
        binary_string_t prog;
        bs_init(&prog);
        um_encode_instruction(&prog, 0, UM_LDK, 0, 0, 5);   /* R0 = 5 */
        um_encode_instruction(&prog, 0, UM_CPY, 1, 0, 0);   /* R1 = R0 */
        um_encode_instruction(&prog, 0, UM_XOR, 0, 1, 0);   /* R0 = R0 ^ R1 = 0 */
        um_encode_instruction(&prog, 0, UM_HALT, 0, 0, 0);
        um_state_t m;
        um_init(&m, &prog, 1000);
        um_run(&m);
        if (!m.halted) return false;
        if (m.reg[0] != 0) return false;
    }

    /* Test 8: CMP sets zero flag */
    {
        binary_string_t prog;
        bs_init(&prog);
        um_encode_instruction(&prog, 0, UM_LDK, 0, 0, 7);   /* R0 = 7 */
        um_encode_instruction(&prog, 0, UM_LDK, 1, 0, 7);   /* R1 = 7 */
        um_encode_instruction(&prog, 0, UM_CMP, 0, 1, 0);   /* compare R0 with R1 */
        um_encode_instruction(&prog, 0, UM_HALT, 0, 0, 0);
        um_state_t m;
        um_init(&m, &prog, 1000);
        um_run(&m);
        if (!m.halted) return false;
        if (!(m.flags & UM_FLAG_ZERO)) return false;
    }

    return true;
}

/* ================================================================
 * Flag Helpers
 * ================================================================ */

static void um_set_flag(um_state_t *m, uint8_t flag, bool value) {
    if (!m) return;
    if (value)
        m->flags |= flag;
    else
        m->flags &= ~flag;
}

size_t um_encode_instruction(binary_string_t *buf, size_t bit_offset,
                              um_opcode_t op, uint8_t ra, uint8_t rb, uint8_t imm) {
    if (!buf) return 0;
    /* Encode opcode as 4 bits */
    /* ... this is a convenience function for program construction */
    /* For simplicity, we append bits directly to buf */
    size_t start_len = buf->length;

    /* Opcode */
    for (int i = 3; i >= 0; i--) bs_append_bit(buf, (op >> i) & 1);

    switch (op) {
    case UM_NOP: case UM_HALT: break;
    case UM_INC: case UM_DEC: case UM_OUT:
        for (int i = 1; i >= 0; i--) bs_append_bit(buf, (ra >> i) & 1);
        bs_append_bit(buf, 0); bs_append_bit(buf, 0); break;
    case UM_LD: case UM_ST: case UM_XOR: case UM_CPY: case UM_CMP:
        for (int i = 1; i >= 0; i--) bs_append_bit(buf, (ra >> i) & 1);
        for (int i = 1; i >= 0; i--) bs_append_bit(buf, (rb >> i) & 1);
        bs_append_bit(buf, 0); bs_append_bit(buf, 0);
        bs_append_bit(buf, 0); bs_append_bit(buf, 0); break;
    case UM_JNZ:
        for (int i = 1; i >= 0; i--) { bs_append_bit(buf, (ra >> i) & 1); }
        bs_append_bit(buf, 0); bs_append_bit(buf, 0);
        for (int i = 7; i >= 0; i--) { bs_append_bit(buf, (imm >> i) & 1); }
        break;
    case UM_SHL: case UM_ADDI: case UM_LDK:
        for (int i = 1; i >= 0; i--) { bs_append_bit(buf, (ra >> i) & 1); }
        bs_append_bit(buf, 0); bs_append_bit(buf, 0);
        for (int i = 3; i >= 0; i--) { bs_append_bit(buf, (imm >> i) & 1); }
        break;
    case UM_OUTB:
        for (int i = 1; i >= 0; i--) { bs_append_bit(buf, (ra >> i) & 1); }
        bs_append_bit(buf, 0); bs_append_bit(buf, 0);
        bs_append_bit(buf, 0); bs_append_bit(buf, 0); break;
    case UM_SYS:
        for (int i = 3; i >= 0; i--) { bs_append_bit(buf, (imm >> i) & 1); }
        break;
    }

    return buf->length - start_len;
}
