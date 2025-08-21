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
    BRK,
    RTS,

    ADC,

    LDA,
    CLC,

    STA,

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

#define MAX_U8     0xFF
#define MAX_OFFSET MAX_U8
#define MAX_PAGES  MAX_U8

typedef struct {
    Opcode opcode;
    Addressing_Modes mode;
} Opcode_Info;

Opcode_Info opcode_matrix[MAX_U8] = {
    //-0                  -1          -2            -3           -4         -5         -6           -7           -8          -9           -A              -B          -C          -D          -E          -F
    {BRK, IMPL}, {ORA, INDX},        0X02,        0X03,        0X04, {ORA, ZP} , {ASL, ZP},         0X07, {PHP, IMPL}, {ORA, IMME}, {ASL, ACCU},        0X0B,        0X0C,  {ORA, ABS},  {ASL, ABS},   0X0F, // 0-
    {BPL, REL} , {ORA, INDY},        0X12,        0X13,        0X14, {ORA, ZPX}, {ASL, ZPX},        0X17, {CLC, IMPL}, {ORA, ABSY},        0X1A,        0X1B,        0X1C, {ORA, ABSX}, {ASL, ABSX},   0X1F, // 1-
    {JSR, ABS} , {AND, INDX},        0X22,        0X23, {BIT, ZP}  , {AND, ZP} , {ROL, ZP},         0X27, {PLP, IMPL}, {AND, IMME}, {ROL, ACCU},        0X2B,  {BIT, ABS},  {AND, ABS},  {ROL, ABS},   0X2F, // 2-
    {BMI, REL} , {AND, INDY},        0X32,        0X33,        0X34, {AND, ZPX}, {ROL, ZPX},        0X37, {SEC, IMPL}, {AND, ABSY},        0X3A,        0X3B,        0X3C, {AND, ABSX}, {ROL, ABSX},   0X3F, // 3-
    {RTI, IMPL}, {EOR, INDX},        0X42,        0X43,        0X44, {EOR, ZP} , {LSR, ZP},         0X47, {PHA, IMPL}, {EOR, IMME}, {LSR, ABS},         0X4B,  {JMP, ABS},  {EOR, ABS},  {LSR, ABS},   0X4F, // 4-
    {BVC, REL} , {EOR, INDY},        0X52,        0X53,        0X54, {EOR, ZPX}, {LSR, ZPX},        0X57, {CLI, IMPL}, {EOR, ABSY},        0X5A,        0X5B,        0X5C, {EOR, ABSX}, {LSR, ABSX},   0X5F, // 5-
    {RTS, IMPL}, {ADC, INDX},        0X62,        0X63,        0X64, {ADC, ZP} , {ROR, ZP},         0X67, {PLA, IMPL}, {ADC, IMME}, {ROR, ACCU},        0X6B,  {JMP, IND},  {ADC, ABS},  {ROR, ABS},   0X6F, // 6-
    {BVS, REL} , {ADC, INDY},        0X72,        0X73,        0X74, {ADC, ZPX}, {ROR, ZPX},        0X77, {SEI, IMPL}, {ADC, ABSY},        0X7A,        0X7B,        0X7C, {ADC, ABSX}, {ROR, ABSX},   0X7F, // 7-
           0X80, {STA, INDX},        0X82,        0X83, {STY, ZP}  , {STA, ZP} , {STX, ZP},         0X87, {DEY, IMPL},        0X89, {TXA, IMPL},        0X8B,  {STY, ABS},  {STA, ABS},  {STX, ABS},   0X8F, // 8-
    {BCC, REL} , {STA, INDY},        0X92,        0X93, {STY, ZPX} , {STA, ZPX}, {STX, ZPY},        0X97, {TYA, IMPL}, {STA, ABSY}, {TXS, IMPL},        0X9B,        0X9C, {STA, ABSX},        0X9E,   0X9F, // 9-
    {LDY, IMME}, {LDA, INDX}, {LDX, IMME},        0XA3, {LDY, ZP}  , {LDA, ZP} , {LDX, ZP},         0XA7, {TAY, IMPL}, {LDA, IMME}, {TAX, IMPL},        0XAB,  {LDY, ABS},  {LDA, ABS},  {LDX, ABS},   0XAF, // A-
    {BCS, REL} , {LDA, INDY},        0XB2,        0XB3, {LDY, ZPX} , {LDA, ZPX}, {LDX, ZPY},        0XB7, {CLV, IMPL}, {LDA, ABSY}, {TSX, IMPL},        0XBB, {LDY, ABSX}, {LDA, ABSX}, {LDX, ABSY},   0XBF, // B-
    {CPY, IMME}, {CMP, INDX},        0XC2,        0XC3, {CPY, ZP}  , {CMP, ZP} , {DEC, ZP},         0XC7, {INY, IMPL}, {CMP, IMME}, {DEX, IMPL},        0XCB,  {CPY, ABS},  {CMP, ABS},  {DEC, ABS},   0XCF, // C-
    {BNE, REL} , {CMP, INDY},        0XD2,        0XD3,        0XD4, {CMP, ZPX}, {DEC, ZPX},        0XD7, {CLD, IMPL}, {CMP, ABSY},        0XDA,        0XDB,        0XDC, {CMP, ABSX}, {DEC, ABSX},   0XDF, // D-
    {CPX, IMME}, {SBC, INDX},        0XE2,        0XE3, {CPX, ZP}  , {SBC, ZP} , {INC, ZP},         0XE7, {INX, IMPL}, {SBC, IMME}, {NOP, IMPL},        0XEB,  {CPX, ABS},  {SBC, ABS},  {INC, ABS},   0XEF, // E-
    {BEQ, REL} , {SBC, INDY},        0XF2,        0XF3,        0XF4, {SBC, ZPX}, {INC, ZPX},        0XF7, {SED, IMPL}, {SBC, ABSY},        0XFA,        0XFB,        0XFC, {SBC, ABSX}, {INC, ABSX},   0XFF, // F-
};

#endif // EMULATOR_6502_H_
