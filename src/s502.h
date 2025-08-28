#ifndef EMULATOR_6502_H_
#define EMULATOR_6502_H_

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t  i32;

#define MAX_U8     (UINT8_MAX)
#define MAX_OFFSET MAX_U8
#define MAX_PAGES  MAX_U8
#define STACK_PAGE 0x01

typedef struct {
    u8 x; // Register x
    u8 y; // Register y
    u8 stack; // Stack pointer
    u16 program_counter; // Program Counter
    u8 status_register; // Process Status Register
    u8 accumulator; // Accumulator

    // +1 to make it 256
    // 256 pages
    // 256 offsets in each page
    u8 memory[MAX_PAGES + 1][MAX_OFFSET + 1]; // Memory Layout
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
} PSR_Flags;

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

typedef struct {
    u8 page;
    u8 offset;
} Location;

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
extern Opcode_Info opcode_matrix[MAX_U8 + 1];

// CPU
extern CPU cpu;

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

void s502_cpu_init();

void s502_dump_page(u8 *page);
void s502_dump_memory();
void s502_print_stats();

void s502_push_stack(u8 value);
u8 s502_pull_stack();

u8 s502_read_memory(Location location);
void s502_write_memory(Location location, u8 data);

void s502_set_psr_flags(PSR_Flags flags);
void s502_clear_psr_flags(PSR_Flags flags);

u8 s502_fetch_operand_data(Addressing_Modes mode, Operand operand);
Location s502_fetch_operand_location(Addressing_Modes mode, Operand operand);

void s502_load_accumulator(Instruction instruction);
void s502_load_x_register(Instruction instruction);
void s502_load_y_register(Instruction instruction);

void s502_store_accumulator(Instruction instruction);
void s502_store_y_register(Instruction instruction);
void s502_store_x_register(Instruction instruction);

void s502_transfer_x_to_accumulator();
void s502_transfer_y_to_accumulator();

void s502_transfer_accumulator_to_x();
void s502_transfer_accumulator_to_y();

void s502_add_with_carry(Instruction instruction);
void s502_sub_with_carry(Instruction instruction);
void s502_compare_accumulator_with_data(Instruction instruction);
void s502_compare_x_register_with_data(Instruction instruction);
void s502_compare_y_register_with_data(Instruction instruction);

void s502_transfer_stack_to_x();
void s502_transfer_x_to_stack();
void s502_transfer_accumulator_to_stack();
void s502_transfer_status_register_to_stack();

void s502_pull_accumulator_from_stack();
void s502_pull_status_register_from_stack();

void s502_logical_and(Instruction instruction);
void s502_logical_xor(Instruction instruction);
void s502_logical_or(Instruction instruction);
void s502_bit_test(Instruction instruction);

void s502_branch_carry_clear(Instruction instruction);
void s502_branch_carry_set(Instruction instruction);
void s502_branch_zero_set(Instruction instruction);
void s502_branch_zero_clear(Instruction instruction);
void s502_branch_negative_set(Instruction instruction);
void s502_branch_negative_clear(Instruction instruction);
void s502_branch_overflow_set(Instruction instruction);
void s502_branch_overflow_clear(Instruction instruction);

void s502_break();
bool s502_decode(Instruction instruction);


// Helper functions
void u16_to_bytes(u16 sixteen_bit, u8 *high_byte, u8 *low_byte);
u16 bytes_to_u16(u8 a, u8 b);
Location u16_to_loc(u16 sixteen_bit);
#endif // EMULATOR_6502_H_
