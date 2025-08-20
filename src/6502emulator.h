#ifndef EMULATOR_6502_H_
#define EMULATOR_6502_H_

typedef uint8_t  u8;
typedef uint16_t u16;

#define MAX_CAPACITY_OF_EACH_PAGE 0x100 // 256
#define NUMBER_OF_PAGES 0x100 // 256

// Registers
typedef u8 Stack;
typedef u8 Register_X;
typedef u8 Register_Y;
typedef u8 Accumulator;
typedef u16 PC; // Program Counter
typedef u8 PSR; // Process Status Register

typedef u8 Memory;
typedef u8 Address;
typedef u8 Zero_Page;

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
    INDEXED_INDIRECT,
    INDIRECT_INDEXED,
} Addressing_Modes;

typedef enum {
    BRK,
    RTS,

    ADC,

    LDA,

    STA,
} Opcode;

typedef struct {
    u8 page;
    u8 offset;
} Operand;

typedef struct {
    Addressing_Modes mode;
    Opcode opcode;
    Operand operand;
} Instruction;

#define UNIMPLEMENTED(message)                                  \
    do {                                                        \
        fprintf(stderr, "%s not implement yet!!!\n", message) ; \
        exit(1);                                                \
    } while (0)
#endif // EMULATOR_6502_H_
