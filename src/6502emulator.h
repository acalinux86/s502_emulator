#ifndef EMULATOR_6502_H_
#define EMULATOR_6502_H_

typedef uint8_t  u8;
typedef uint16_t u16;

// Registers
typedef u8 Stack;
typedef u8 Register_X;
typedef u8 Register_Y;
typedef u16 PC; // Program Counter
typedef u8 PSR; // Process Status Register

typedef u8 Memory;

typedef enum {
    N_BIT_FLAG = 0x80, // Negative bit flag
    V_BIT_FLAG = 0x40, // Overflow bit flag
    B_BIT_FLAG = 0x10, // Break bit flag
    D_BIT_FLAG = 0x08, // Decimal mode bit flag
    I_BIT_FLAG = 0x04, // Interrupt disable bit flag
    Z_BIT_FLAG = 0x02, // Zero bit flag
    C_BIT_FLAG = 0x01, // Carry bit flag
} PSR_Flags;

typedef enum {
    IMPLICIT,
    ACCUMULATOR,
    IMMEDIATE,
    ZERO_PAGE,
    ZERO_PAGE_X,
    ZERO_PAGE_Y,
    RELATIVE,
    ABSOLUTE,
    ABSOLUTE_X,
    ABSOLUTE_Y,
    INDIRECT,
    INDEXED_INDIRECT, // a pointer to the address of the low byte held in zero page
    INDIRECT_INDEXED,
} Addressing_Modes;

typedef enum {
    BRK,
    RTS,

    ADC,

    LDA,
    CLC,

    STA,
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

#endif // EMULATOR_6502_H_
