/**
 * universal_machine.h - Universal Monotone Machine U
 *
 * Implements a simple universal monotone machine. "Monotone" means
 * the output tape is write-only with head only moving right:
 * output can grow but never be erased or overwritten.
 *
 * Architecture: 4 general-purpose registers, 256-byte working memory,
 * write-only monotone output tape, 16-instruction set encoded as binary.
 *
 * Instruction set (4-bit opcodes, some extended with operands):
 *   0x0 NOP, 0x1 INC Rn, 0x2 DEC Rn, 0x3 LD Rn [Rm], 0x4 ST [Rn] Rm,
 *   0x5 OUT Rn, 0x6 JNZ Rn off, 0x7 HALT, 0x8 XOR Rn Rm,
 *   0x9 SHL Rn imm, 0xA CMP Rn Rm, 0xB CPY Rn Rm,
 *   0xC LDK Rn imm, 0xD OUTB Rn, 0xE ADDI Rn imm, 0xF SYS n
 *
 * This machine is Turing-complete (can simulate any TM).
 *
 * Reference: Rojas, "Konrad Zuse's Legacy", IEEE Annals, 1997.
 * Curriculum: MIT 6.045, CMU 15-251, Cambridge Part II
 */

#ifndef UNIVERSAL_MACHINE_H
#define UNIVERSAL_MACHINE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "solomonoff.h"

#define UM_NUM_REGISTERS     4
#define UM_MEMORY_SIZE       256
#define UM_FLAG_ZERO         0x01
#define UM_FLAG_CARRY        0x02
#define UM_FLAG_OVERFLOW     0x04
#define UM_FLAG_NEGATIVE     0x08

typedef enum {
    UM_NOP  = 0x0, UM_INC  = 0x1, UM_DEC  = 0x2, UM_LD   = 0x3,
    UM_ST   = 0x4, UM_OUT  = 0x5, UM_JNZ  = 0x6, UM_HALT = 0x7,
    UM_XOR  = 0x8, UM_SHL  = 0x9, UM_CMP  = 0xA, UM_CPY  = 0xB,
    UM_LDK  = 0xC, UM_OUTB = 0xD, UM_ADDI = 0xE, UM_SYS  = 0xF
} um_opcode_t;

typedef struct {
    um_opcode_t opcode;
    uint8_t reg_a, reg_b;
    uint8_t immediate;
    int8_t offset;
    uint8_t raw;
    bool valid;
} um_instruction_t;

struct um_state_t;
typedef struct um_state_t um_state_t;

struct um_state_t {
    binary_string_t program;
    size_t pc;
    bool halted, timeout;
    uint32_t reg[UM_NUM_REGISTERS];
    uint8_t flags;
    uint8_t mem[UM_MEMORY_SIZE];
    uint8_t mem_ptr;
    binary_string_t output;
    size_t output_bit_count;
    uint64_t step_count, step_limit;
    size_t instructions_decoded;
};

/* Core machine operations */
void um_init(um_state_t *m, const binary_string_t *program, uint64_t step_limit);
void um_reset(um_state_t *m);
um_instruction_t um_decode(const um_state_t *m);
bool um_step(um_state_t *m);
uint64_t um_run(um_state_t *m);
const binary_string_t *um_get_output(const um_state_t *m);
bool um_output_has_prefix(const um_state_t *m, const binary_string_t *x);
bool um_output_equals(const um_state_t *m, const binary_string_t *x);

/* Program construction */
size_t um_build_print_program(binary_string_t *program, const binary_string_t *data);
size_t um_build_repeat_program(binary_string_t *program, const binary_string_t *pattern,
                                size_t repeat_count);
size_t um_build_xor_program(binary_string_t *program,
                             const binary_string_t *a, const binary_string_t *b);
size_t um_make_prefix_free(binary_string_t *result, const binary_string_t *raw_program);

/* Instruction encoding */
size_t um_encode_instruction(binary_string_t *buf, size_t bit_offset,
                              um_opcode_t op, uint8_t ra, uint8_t rb, uint8_t imm);
size_t um_decode_instruction(const binary_string_t *program, size_t bit_offset,
                              um_instruction_t *instr);

/* Machine analysis */
uint64_t um_count_programs(size_t length);
uint64_t um_estimate_halting(size_t max_length, uint64_t max_steps);
bool um_self_test(void);

#endif /* UNIVERSAL_MACHINE_H */
