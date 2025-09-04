#ifndef EMULATOR_6502_H_
#define EMULATOR_6502_H_

#define MAX_OFFSET UINT8_MAX
#define MAX_PAGES  UINT8_MAX
#define ZERO_PAGE  0x00
#define STACK_PAGE 0x01

#include "./types.h"
#include "./array.h"

typedef struct Location Location;

typedef u8 (*read_memory)(void *, Location);
typedef void (*write_memory)(void *, Location, u8);

typedef struct {
    read_memory read;
    write_memory write;
    void *device;
    u16 start_addr;
    u16 end_addr;
    bool readonly;
} MMap_Entry; // Memory Map

typedef ARRAY(MMap_Entry) MMap_Entries;

typedef struct {
    u8 x; // Register x
    u8 y; // Register y
    u8 stack; // Stack pointer
    u16 program_counter; // Program Counter
    u8 status_register; // Process Status Register
    u8 accumulator; // Accumulator
    MMap_Entries entries;
} CPU;

typedef enum {
    N_BIT_FLAG = 0x80, // Negative bit flag
    V_BIT_FLAG = 0x40, // Overflow bit flag
    U_BIT_FLAG = 0x20, // UNSET BIT, intialized with the PSR
    B_BIT_FLAG = 0x10, // Break bit flag
    D_BIT_FLAG = 0x08, // Decimal mode bit flag
    I_BIT_FLAG = 0x04, // Interrupt disable bit flag
    Z_BIT_FLAG = 0x02, // Zero bit flag
    C_BIT_FLAG = 0x01, // Carry bit flag
} Status_Flags;

typedef enum {
    IMPL, // IMPLICIT | IMPLIED
    ACCU, // ACCUMULATOR
    IMME, // IMMEDIATE
    ZP, // ZERO_PAGE
    ZPX, // ZERO_PAGE_X
    ZPY, // ZERO_PAGE_Y
    REL, // RELATIVE
    ABS, // ABSOLUTE
    ABSX, // ABSOLUTE_X
    ABSY, // ABSOLUTE_Y
    IND, // INDIRECT
    INDX, // INDIRECT_X
    INDY, // INDIRECT_Y
} Addressing_Modes;

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

    // Register Transfers
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
} Opcode;

typedef u16 Absolute;

struct Location {
    u8 page;
    u8 offset;
};

typedef union {
    Location loc;
    Absolute absolute;
} Address;

typedef enum {
    OPERAND_ABSOLUTE,
    OPERAND_LOCATION,
    OPERAND_DATA,
} Operand_Type;

typedef union {
    Address address;
    u8 data;
} Operand_Data;

typedef struct {
    Operand_Type type;
    Operand_Data data;
} Operand;

typedef struct {
    Addressing_Modes mode;
    Opcode  opcode;
    Operand operand;
} Instruction;

typedef struct {
    Opcode opcode;
    Addressing_Modes mode;
} Opcode_Info;

// Opcode/Mode matrix
extern Opcode_Info opcode_matrix[UINT8_MAX + 1];

// Functions Declarations
const char *s502_addr_mode_as_cstr(Addressing_Modes mode);
const char *s502_opcode_as_cstr(Opcode opcode);
const char *s502_operand_type_as_cstr(Operand_Type type);

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof(xs[0]))

#define UNIMPLEMENTED(message)                                          \
    do {                                                                \
        fprintf(stderr, "ERROR: %s not implemented yet!!!\n", message) ;  \
        abort();                                                        \
    } while (0)

#define UNREACHABLE(message)                                        \
    do {                                                            \
        fprintf(stderr, "ERROR: `%s` unreachable!!!!\n", message);  \
        abort();                                                    \
    } while (0)

#define ILLEGAL_ADDRESSING(mode, opcode)                    \
    do {                                                    \
        fprintf(stderr, "ERROR: Invalid `%s` mode on %s\n", \
                s502_addr_mode_as_cstr(mode),               \
                s502_opcode_as_cstr(opcode));               \
        abort();                                            \
    } while (0)

#define ILLEGAL_ACCESS(mode, operand)                                   \
    do {                                                                \
        assert(s502_operand_type_as_cstr(operand));                     \
        fprintf(stderr, "ERROR: Invalid `%s` type on `%s` mode. \n",    \
                s502_operand_type_as_cstr(operand),                     \
                s502_addr_mode_as_cstr(mode));                          \
        abort();                                                        \
    } while (0)

CPU s502_cpu_init(void);

u8 s502_read_memory(void *device, Location location);
void s502_write_memory(void *device, Location location, u8 data);
u8 s502_cpu_read(CPU *cpu, u16 addr);
void s502_cpu_write(CPU *cpu, u16 addr, u8 data);

void s502_push_stack(CPU *cpu, u8 value);
u8 s502_pull_stack(CPU *cpu);

void s502_set_psr_flags(CPU *cpu, Status_Flags flags);
void s502_clear_psr_flags(CPU *cpu, Status_Flags flags);

u8 s502_fetch_operand_data(CPU *cpu, Addressing_Modes mode, Operand operand);
Location s502_fetch_operand_location(CPU *cpu, Addressing_Modes mode, Operand operand);

void s502_load_register(CPU *cpu, Instruction instruction, u8 *register_type);
void s502_store_register(CPU *cpu, Instruction instruction, u8 data);

void s502_transfer_register_to_accumulator(CPU *cpu, u8 data);
void s502_transfer_accumulator_to_register(CPU *cpu, u8 *register_type);

void s502_add_with_carry(CPU *cpu, Instruction instruction);
void s502_sub_with_carry(CPU *cpu, Instruction instruction);

void s502_compare_register_with_data(CPU *cpu, Instruction instruction, u8 register_type);

void s502_transfer_stack_to_register(CPU *cpu, u8 *data);
void s502_push_register_to_stack(CPU *cpu, u8 register_type);
void s502_pull_register_from_stack(CPU *cpu, u8 *register_type);

void s502_logical_and(CPU *cpu, Instruction instruction);
void s502_logical_xor(CPU *cpu, Instruction instruction);
void s502_logical_or(CPU *cpu, Instruction instruction);
void s502_bit_test(CPU *cpu, Instruction instruction);

void s502_branch_flag_clear(CPU *cpu, Instruction instruction, Status_Flags flag);
void s502_branch_flag_set(CPU *cpu, Instruction instruction, Status_Flags flag);

void s502_break(CPU *cpu);
bool s502_decode(CPU *cpu, Instruction instruction);


// Helper functions
void u16_to_bytes(u16 sixteen_bit, u8 *high_byte, u8 *low_byte);
u16 bytes_to_u16(u8 a, u8 b);
Location u16_to_loc(u16 sixteen_bit);
#endif // EMULATOR_6502_H_
