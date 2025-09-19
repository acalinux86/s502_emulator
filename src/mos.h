#ifndef MOS_6502_EMULATOR_H_
#define MOS_6502_EMULATOR_H_

#define MOS_MAX_OFFSET UINT8_MAX
#define MOS_MAX_PAGES  UINT8_MAX
#define MOS_ZERO_PAGE  0x00
#define MOS_STACK_PAGE 0x01

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./array.h"

typedef uint8_t (*read_memory)(void *, uint16_t);
typedef void    (*write_memory)(void *, uint16_t, uint8_t);

typedef struct {
    void *device;
    read_memory  read;
    write_memory write;
    uint16_t start_addr;
    uint16_t end_addr;
    bool readonly;
} MOS_MMap; // Memory Map

typedef ARRAY(MOS_MMap) MOS_MMaps;

typedef struct {
    uint8_t  regx; // Reg x
    uint8_t  regy; // Reg y
    uint8_t  racc; // Accumulator
    uint8_t  sp;   // Stack pointer
    uint16_t pc;   // Program Counter
    uint8_t  psr;  // Process Status Reg
    MOS_MMaps entries;
} MOS_Cpu;

typedef enum _status_flags {
    N_BIT_FLAG = 0x80, // Negative bit flag
    V_BIT_FLAG = 0x40, // Overflow bit flag
    U_BIT_FLAG = 0x20, // UNSET BIT, intialized with the PSR
    B_BIT_FLAG = 0x10, // Break bit flag
    D_BIT_FLAG = 0x08, // Decimal mode bit flag
    I_BIT_FLAG = 0x04, // Interrupt disable bit flag
    Z_BIT_FLAG = 0x02, // Zero bit flag
    C_BIT_FLAG = 0x01, // Carry bit flag
} MOS_StatusFlags;

typedef enum {
    IMPL, // IMPLICIT | IMPLIED
    ACCU, // ACCUMULATOR
    IMME, // IMMEDIATE
    ZP,   // ZERO_PAGE
    ZPX,  // ZERO_PAGE_X
    ZPY,  // ZERO_PAGE_Y
    REL,  // RELATIVE
    ABS,  // ABSOLUTE
    ABSX, // ABSOLUTE_X
    ABSY, // ABSOLUTE_Y
    IND,  // INDIRECT
    INDX, // INDIRECT_X
    INDY, // INDIRECT_Y
} MOS_AddressingModes;

typedef enum {
    // System Functions
    BRK,
    NOP,
    RTI,

    // Jump and calls
    RTS,
    JMP,
    JSR,

    // Arithmetic
    ADC,
    SBC,
    CMP,
    CPX,
    CPY,

    // Load Operations
    LDA,
    LDY,
    LDX,

    // Store Operations
    STA,
    STY,
    STX,

    // Clear Status Flag Changes
    CLC,
    CLV,
    CLI,
    CLD,

    // Set Status Flag Changes
    SEC,
    SED,
    SEI,

    // Logical
    ORA,
    AND,
    EOR,
    BIT,

    // Reg Transfers
    TAX,
    TAY,
    TXA,
    TYA,

    // Stack Operations
    TSX,
    TXS,
    PHA,
    PHP,
    PLA,
    PLP,

    // Increments
    INC,
    INX,
    INY,

    // Decrements
    DEX,
    DEY,
    DEC,

    // Branches
    BNE,
    BCC,
    BCS,
    BEQ,
    BMI,
    BPL,
    BVC,
    BVS,

    // Shifts
    ASL,
    LSR,
    ROL,
    ROR,

    // Error Values
    ERROR_FETCH_DATA,
    ERROR_FETCH_LOCATION,
} MOS_Opcode;

typedef enum {
    OPERAND_ADDRESS,
    OPERAND_DATA,
} MOS_OperandType;

typedef union {
    uint16_t address;
    uint8_t data;
} MOS_OperandData;

typedef struct {
    MOS_OperandType type;
    MOS_OperandData data;
} MOS_Operand;

typedef struct {
    MOS_AddressingModes mode;
    MOS_Opcode  opcode;
    MOS_Operand operand;
} MOS_Instruction;

typedef struct {
    MOS_Opcode opcode;
    MOS_AddressingModes mode;
} MOS_OpcodeInfo;

// Opcode/Mode matrix
extern MOS_OpcodeInfo opcode_matrix[UINT8_MAX + 1];

// Functions Declarations
const char *mos_addr_mode_as_cstr(MOS_AddressingModes mode);
const char *mos_opcode_as_cstr(MOS_Opcode opcode);
const char *mos_operand_type_as_cstr(MOS_OperandType type);

#define MOS_ARRAY_LEN(xs) (sizeof(xs) / sizeof(xs[0]))


#define MOS_ASSERT(condition, message)                      \
    do {                                                    \
        if (!(condition)) {                                 \
            fprintf(stderr, "[CPU FAULT]: %s\n", message);  \
        }                                                   \
    } while(0)

#define MOS_UNIMPLEMENTED(message)                                         \
    do {                                                                   \
        fprintf(stderr, "[ERROR]: %s not implemented yet!!!\n", message);  \
        abort();                                                           \
    } while (0)

#define MOS_UNREACHABLE(message)                                        \
    do {                                                                \
        fprintf(stderr, "[ERROR]: `%s` unreachable!!!!\n", message);    \
        abort();                                                        \
    } while (0)

#define MOS_ILLEGAL_ADDRESSING(mode, opcode)                    \
    do {                                                        \
        fprintf(stderr, "[ERROR]: Invalid `%s` mode on %s\n",   \
                mos_addr_mode_as_cstr(mode),                    \
                mos_opcode_as_cstr(opcode));                    \
        abort();                                                \
    } while (0)

#define MOS_ILLEGAL_ACCESS(mode, operand)                               \
    do {                                                                \
        assert(mos_operand_type_as_cstr(operand));                      \
        fprintf(stderr, "[ERROR]: Invalid `%s` type on `%s` mode. \n",  \
                mos_operand_type_as_cstr(operand),                      \
                mos_addr_mode_as_cstr(mode));                           \
        abort();                                                        \
    } while (0)

MOS_Cpu mos_cpu_init(void);

uint8_t mos_read_memory(void *device, uint16_t location);
uint8_t mos_cpu_read(MOS_Cpu *cpu, uint16_t addr);
void mos_write_memory(void *device, uint16_t location, uint8_t data);
void mos_cpu_write(MOS_Cpu *cpu, uint16_t addr, uint8_t data);

void mos_push_stack(MOS_Cpu *cpu, uint8_t value);
uint8_t mos_pull_stack(MOS_Cpu *cpu);

void mos_set_psr_flags(MOS_Cpu *cpu, MOS_StatusFlags flags);
void mos_clear_psr_flags(MOS_Cpu *cpu, MOS_StatusFlags flags);

uint8_t  mos_fetch_operand_data(MOS_Cpu *cpu, MOS_AddressingModes mode, MOS_Operand operand);
uint16_t mos_fetch_operand_location(MOS_Cpu *cpu, MOS_AddressingModes mode, MOS_Operand operand);

void mos_load_reg(MOS_Cpu *cpu, MOS_Instruction instruction, uint8_t *reg_type);
void mos_store_reg(MOS_Cpu *cpu, MOS_Instruction instruction, uint8_t data);

void mos_transfer_reg_to_accumulator(MOS_Cpu *cpu, uint8_t data);
void mos_transfer_accumulator_to_reg(MOS_Cpu *cpu, uint8_t *reg_type);

void mos_add_with_carry(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_sub_with_carry(MOS_Cpu *cpu, MOS_Instruction instruction);

void mos_compare_reg_with_data(MOS_Cpu *cpu, MOS_Instruction instruction, uint8_t reg_type);

void mos_transfer_stack_to_reg(MOS_Cpu *cpu, uint8_t *data);
void mos_push_reg_to_stack(MOS_Cpu *cpu, uint8_t reg_type);
void mos_pull_reg_from_stack(MOS_Cpu *cpu, uint8_t *reg_type);

void mos_logical_and(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_logical_xor(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_logical_or(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_bit_test(MOS_Cpu *cpu, MOS_Instruction instruction);

void mos_branch_flag_clear(MOS_Cpu *cpu, MOS_Instruction instruction, MOS_StatusFlags flag);
void mos_branch_flag_set(MOS_Cpu *cpu, MOS_Instruction instruction, MOS_StatusFlags flag);

void mos_decrement(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_increment(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_decrement_regx(MOS_Cpu *cpu);
void mos_decrement_regy(MOS_Cpu *cpu);
void mos_increment_regx(MOS_Cpu *cpu);
void mos_increment_regy(MOS_Cpu *cpu);

void mos_arithmetic_shift_left_racc(MOS_Cpu *cpu);
void mos_logical_shift_right_racc(MOS_Cpu *cpu);
void mos_rotate_left_racc(MOS_Cpu *cpu);
void mos_rotate_right_racc(MOS_Cpu *cpu);

void mos_arithmetic_shift_left_memory(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_logical_shift_right_memory(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_rotate_left_memory(MOS_Cpu *cpu, MOS_Instruction instruction);
void mos_rotate_right_memory(MOS_Cpu *cpu, MOS_Instruction instruction);

void mos_break(MOS_Cpu *cpu);
bool mos_decode(MOS_Cpu *cpu, MOS_Instruction instruction);

void mos_uint16_t_to_bytes(uint16_t sixteen_bit, uint8_t *high_byte, uint8_t *low_byte);
uint16_t mos_bytes_to_uint16_t(uint8_t a, uint8_t b);

#endif // MOS_6502_EMULATOR_H_
