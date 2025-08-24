#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./s502.h"

Opcode_Info opcode_matrix[MAX_U8 + 1] = {
    //-0                  -1          -2            -3              -4         -5         -6                 -7           -8          -9           -A              -B          -C          -D          -E          -F
    {BRK, IMPL},  {ORA, INDX},        {0x00},        {0x00},        {0x00}, {ORA, ZP} ,  {ASL, ZP},      {0x00},  {PHP, IMPL}, {ORA, IMME},   {ASL, ACCU},        {0x00},      {0x00},  {ORA, ABS},   {ASL, ABS},    {0x00}, // 0-
    {BPL, REL} ,  {ORA, INDY},        {0x00},        {0x00},        {0x00}, {ORA, ZPX}, {ASL, ZPX},      {0x00},  {CLC, IMPL}, {ORA, ABSY},        {0x00},        {0x00},      {0x00}, {ORA, ABSX},  {ASL, ABSX},    {0x00}, // 1-
    {JSR, ABS} ,  {AND, INDX},        {0x00},        {0x00},     {BIT, ZP}, {AND, ZP} ,  {ROL, ZP},      {0x00},  {PLP, IMPL}, {AND, IMME},   {ROL, ACCU},        {0x00},  {BIT, ABS},  {AND, ABS},   {ROL, ABS},    {0x00}, // 2-
    {BMI, REL} ,  {AND, INDY},        {0x00},        {0x00},        {0x00}, {AND, ZPX}, {ROL, ZPX},      {0x00},  {SEC, IMPL}, {AND, ABSY},        {0x00},        {0x00},      {0x00}, {AND, ABSX},  {ROL, ABSX},    {0x00}, // 3-
    {RTI, IMPL},  {EOR, INDX},        {0x00},        {0x00},        {0x00}, {EOR, ZP} ,  {LSR, ZP},      {0x00},  {PHA, IMPL}, {EOR, IMME},    {LSR, ABS},        {0x00},  {JMP, ABS},  {EOR, ABS},   {LSR, ABS},    {0x00}, // 4-
    {BVC, REL} ,  {EOR, INDY},        {0x00},        {0x00},        {0x00}, {EOR, ZPX}, {LSR, ZPX},      {0x00},  {CLI, IMPL}, {EOR, ABSY},        {0x00},        {0x00},      {0x00}, {EOR, ABSX},  {LSR, ABSX},    {0x00}, // 5-
    {RTS, IMPL},  {ADC, INDX},        {0x00},        {0x00},        {0x00}, {ADC, ZP} ,  {ROR, ZP},      {0x00},  {PLA, IMPL}, {ADC, IMME},   {ROR, ACCU},        {0x00},  {JMP, IND},  {ADC, ABS},   {ROR, ABS},    {0x00}, // 6-
    {BVS, REL} ,  {ADC, INDY},        {0x00},        {0x00},        {0x00}, {ADC, ZPX}, {ROR, ZPX},      {0x00},  {SEI, IMPL}, {ADC, ABSY},        {0x00},        {0x00},      {0x00}, {ADC, ABSX},  {ROR, ABSX},    {0x00}, // 7-
         {0x00},  {STA, INDX},        {0x00},        {0x00},     {STY, ZP}, {STA, ZP} ,  {STX, ZP},      {0x00},  {DEY, IMPL},      {0x00},   {TXA, IMPL},        {0x00},  {STY, ABS},  {STA, ABS},   {STX, ABS},    {0x00}, // 8-
    {BCC, REL} ,  {STA, INDY},        {0x00},        {0x00},    {STY, ZPX}, {STA, ZPX}, {STX, ZPY},      {0x00},  {TYA, IMPL}, {STA, ABSY},   {TXS, IMPL},        {0x00},      {0x00}, {STA, ABSX},       {0x00},    {0x00}, // 9-
    {LDY, IMME},  {LDA, INDX},   {LDX, IMME},        {0x00},     {LDY, ZP}, {LDA, ZP} ,  {LDX, ZP},      {0x00},  {TAY, IMPL}, {LDA, IMME},   {TAX, IMPL},        {0x00},  {LDY, ABS},  {LDA, ABS},   {LDX, ABS},    {0x00}, // A-
    {BCS, REL} ,  {LDA, INDY},        {0x00},        {0x00},    {LDY, ZPX}, {LDA, ZPX}, {LDX, ZPY},      {0x00},  {CLV, IMPL}, {LDA, ABSY},   {TSX, IMPL},        {0x00}, {LDY, ABSX}, {LDA, ABSX},  {LDX, ABSY},    {0x00}, // B-
    {CPY, IMME},  {CMP, INDX},        {0x00},        {0x00},     {CPY, ZP}, {CMP, ZP} ,  {DEC, ZP},      {0x00},  {INY, IMPL}, {CMP, IMME},   {DEX, IMPL},        {0x00},  {CPY, ABS},  {CMP, ABS},   {DEC, ABS},    {0x00}, // C-
    {BNE, REL} ,  {CMP, INDY},        {0x00},        {0x00},        {0x00}, {CMP, ZPX}, {DEC, ZPX},      {0x00},  {CLD, IMPL}, {CMP, ABSY},        {0x00},        {0x00},      {0x00}, {CMP, ABSX},  {DEC, ABSX},    {0x00}, // D-
    {CPX, IMME},  {SBC, INDX},        {0x00},        {0x00},     {CPX, ZP}, {SBC, ZP} ,  {INC, ZP},      {0x00},  {INX, IMPL}, {SBC, IMME},   {NOP, IMPL},        {0x00},  {CPX, ABS},  {SBC, ABS},   {INC, ABS},    {0x00}, // E-
    {BEQ, REL} ,  {SBC, INDY},        {0x00},        {0x00},        {0x00}, {SBC, ZPX}, {INC, ZPX},      {0x00},  {SED, IMPL}, {SBC, ABSY},        {0x00},        {0x00},      {0x00}, {SBC, ABSX},  {INC, ABSX},    {0x00}, // F-
};

// Global Variables
Memory memory[MAX_PAGES][MAX_OFFSET] = {0X00}; // Memory Layout
u8 stack_size = 0x00; // We wanna Track the stack Size
Stack *stack = memory[0x01]; // Page 1

// Registers
u8 accumulator = 0x00;
Register_X X = 0x00;
Register_Y Y = 0x00;
PC program_counter = 0x00;
PSR processor_status_register = 0x20; // Initialize with the unused bit

void s502_dump_page(u8 *page)
{
    u8 page_width = 16;
    u8 page_height = 16;
    for (u8 i = 0; i < page_width; ++i) {
        for (u8 j = 0; j < page_height; ++j) {
            printf("0X%02X ", page[i * page_height + j]);
        }
        printf("\n");
    }
}

void s502_push_stack(u8 value)
{
    // NOTE: Stack Operations are limited to only page one (Stack Pointer) of the 6502
    assert(stack_size < MAX_OFFSET - 1 && "Stack Overflow");
    stack[stack_size] = value;
    stack_size++;
}

u8 s502_pull_stack()
{
    assert(stack_size > 0 && "Stack Underflow");
    stack_size--;
    return stack[stack_size];
}

void s502_dump_memory()
{
    for (u8 i = 0; i < MAX_PAGES; ++i) {
        s502_dump_page(memory[i]);
    }
}

// NOTE: Print Stats
void s502_print_stats()
{
    printf("Stack       : %u\n", stack_size);
    printf("Register_X  : %u\n", X);
    printf("Register_Y  : %u\n", Y);
    printf("Accumulator : %u\n", accumulator);
    printf("PC          : %u\n", program_counter);
    printf("PSR         : %u\n", processor_status_register);
}

u8 s502_read_memory(Location location)
{
    return memory[location.page][location.offset];
}

void s502_write_memory(Location location, u8 data)
{
    memory[location.page][location.offset] = data;
}

const char *s502_addr_mode_as_cstr(Addressing_Modes mode)
{
    switch (mode) {
    case IMPL: return "IMPLICIT";
    case ACCU: return "ACCUMULATOR";
    case IMME: return "IMMEDIATE";
    case ZP:   return "ZERO_PAGE";
    case ZPX:  return "ZERO_PAGE_X";
    case ZPY:  return "ZERO_PAGE_Y";
    case REL:  return "RELATIVE";
    case ABS:  return "ABSOLUTE";
    case ABSX: return "ABSOLUTE_X";
    case ABSY: return "ABSOLUTE_Y";
    case IND:  return "INDIRECT";
    case INDX: return "INDEXED_INDIRECT";
    case INDY: return "INDIRECT_INDEXED";
    default:   return NULL;
    }
}

const char *s502_opcode_as_cstr(Opcode opcode)
{
    switch (opcode) {
    case BRK:                  return "BRK";
    case NOP:                  return "NOP";
    case RTI:                  return "RTI";
    case RTS:                  return "RTS";
    case JMP:                  return "JMP";
    case JSR:                  return "JSR";
    case ADC:                  return "ADC";
    case SBC:                  return "SBC";
    case CMP:                  return "CMP";
    case CPX:                  return "CPX";
    case CPY:                  return "CPY";
    case LDA:                  return "LDA";
    case LDY:                  return "LDY";
    case LDX:                  return "LDX";
    case STA:                  return "STA";
    case STY:                  return "STY";
    case STX:                  return "STX";
    case CLC:                  return "CLC";
    case CLV:                  return "CLV";
    case CLI:                  return "CLI";
    case CLD:                  return "CLD";
    case SEC:                  return "SEC";
    case SED:                  return "SED";
    case SEI:                  return "SEI";
    case ORA:                  return "ORA";
    case AND:                  return "AND";
    case EOR:                  return "EOR";
    case BIT:                  return "BIT";
    case TAX:                  return "TAX";
    case TAY:                  return "TAY";
    case TXA:                  return "TXA";
    case TYA:                  return "TYA";
    case TSX:                  return "TSX";
    case TXS:                  return "TXS";
    case PHA:                  return "PHA";
    case PHP:                  return "PHP";
    case PLA:                  return "PLA";
    case PLP:                  return "PLP";
    case INC:                  return "INC";
    case INX:                  return "INX";
    case INY:                  return "INY";
    case DEX:                  return "DEX";
    case DEY:                  return "DEY";
    case DEC:                  return "DEC";
    case BNE:                  return "BNE";
    case BCC:                  return "BCC";
    case BCS:                  return "BCS";
    case BEQ:                  return "BEQ";
    case BMI:                  return "BMI";
    case BPL:                  return "BPL";
    case BVC:                  return "BVC";
    case BVS:                  return "BVS";
    case ASL:                  return "ASL";
    case LSR:                  return "LSR";
    case ROL:                  return "ROL";
    case ROR:                  return "ROR";
    case ERROR_FETCH_DATA:     return "ERROR_FETCH_DATA";
    case ERROR_FETCH_LOCATION: return "ERROR_FETCH_LOCATION";
    default:                   return NULL;
    }
}
const char *s502_operand_type_as_cstr(Operand_Type type)
{
    switch (type) {
    case OPERAND_ABSOLUTE: return "OPERAND_ABSOLUTE";
    case OPERAND_LOCATION: return "OPERAND_LOCATION";
    case OPERAND_DATA:     return "OPERAND_DATA";
    default:               return NULL;
    }
}

// NOTE: Set PSR FLAGS
void s502_set_psr_flags(PSR_Flags flags)
{
    processor_status_register |= flags;
}

// NOTE: Clear PSR FLAGS
void s502_clear_psr_flags(PSR_Flags flags)
{
    processor_status_register &= flags;
}

void u16_bit_split(u16 sixteen_bit, u8 *high_byte,, u8 *low_byte)
{
    *high_byte = sixteen_bit >> 8; // higher byte
    *low_byte  = sixteen_bit & 0xFF; // low-byte
}

// a must be the higher-byte and b the lower-byte
u16 u8_bits_join(u8 a, u8 b)
{
    return (a << 8) | b;
}

Location u16_to_loc(u16 sixteen_bit)
{
    u8 high_byte, low_byte;
    u16_bit_split(sixteen_bit, &high_byte, &low_byte);
    Location loc = {
        .page = high_byte, // higher byte
        .offset = low_byte, // low-byte
    };
    return loc;
}

// Returns data for read opcodes
u8 s502_fetch_operand_data(Addressing_Modes mode, Operand operand)
{
    switch (mode) {
    case IMME: {
        // Immediate mode just loads a value into an opcode
        switch (operand.type) {
        case OPERAND_DATA: {
            return operand.data.data;
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZP: {
        switch (operand.type) {
        case OPERAND_LOCATION: {
            // if the operand doesn't contain any page, assumed that it is page zero
            Location location = operand.data.address.loc;
            assert(location.page == 0x00 && "Invalid Page Zero Address");
            return s502_read_memory(location);
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZPX: {
        // zero page X modes sums x register to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MAX_OFFSET
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            assert(location.page == 0x00 && "Invalid Page Zero Address");
            Location new_loc = { .offset = location.offset + X, .page = location.page};
            return s502_read_memory(new_loc);
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABS: {
        // uses the absolute location in operand to load access memory into accumulator
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Location location = u16_to_loc(absolute);
            return s502_read_memory(location);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X register
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + X;
            Location location = u16_to_loc(index);
            return s502_read_memory(location);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y register
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + Y;
            Location location = u16_to_loc(index);
            return s502_read_memory(location);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDX: {
        // Adds X register to the offset of the location in the instruction
        // Fetches the data pointed by the modified location
        // Uses that data as a new location to index into memory and fetches low-byte
        // Uses the modified location.offset + 1 to index into memory and fetches high-byte
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            assert(location.page == 0x00 && "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset + X, .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            Location final = {
                .offset = s502_read_memory(new_loc),   // fetch low-byte from new_loc
                .page =   s502_read_memory(new_loc_i), // fetch high-byte from new_loc + 1
            };
            return s502_read_memory(final);
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y register occurs in the final address
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            assert(location.page == 0x00 && "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset    , .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            Location final = {
                .offset = s502_read_memory(new_loc),   // fetch low-byte from new_loc
                .page =   s502_read_memory(new_loc_i), // fetch high-byte from new_loc + 1
            };
            final.offset += Y; // final Address that contains the data
            return s502_read_memory(final);
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;
    case IMPL:
    case ACCU:
    case ZPY:
    case REL:
    case IND:
    default: ILLEGAL_ADDRESSING(mode, ERROR_FETCH_DATA);
    }
    UNREACHABLE("s502_fetch_operand_data");
}

// Returns A Location for store opcodes
Location s502_fetch_operand_location(Addressing_Modes mode, Operand operand)
{
    switch (mode) {
    case ZP: {
        switch (operand.type) {
        case OPERAND_LOCATION: {
            // if the operand doesn't contain any page, assumed that it is page zero
            return operand.data.address.loc;
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZPX: {
        // zero page X modes sums x register to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MAX_OFFSET
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            assert(location.page == 0x00 && "Invalid Page Zero Address");
            return (Location) { .offset = location.offset + X, .page = location.page};
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABS: {
        // uses the absolute location in operand to load access memory into accumulator
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            return u16_to_loc(absolute);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X register
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + X;
            return u16_to_loc(index);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y register
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + Y;
            return u16_to_loc(index);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDX: {
        // Adds X register to the offset of the location in the instruction
        // Fetches the data pointed by the modified location
        // Uses that data as a new location to index into memory and fetches low-byte
        // Uses the modified location.offset + 1 to index into memory and fetches high-byte
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            assert(location.page == 0x00 && "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset + X, .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            return (Location) {
                .offset = s502_read_memory(new_loc),   // fetch low-byte from new_loc
                .page   = s502_read_memory(new_loc_i), // fetch high-byte from new_loc + 1
            };
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y register occurs in the final address
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            assert(location.page == 0x00 && "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset    , .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            Location final = {
                .offset = s502_read_memory(new_loc),   // fetch low-byte from new_loc
                .page =   s502_read_memory(new_loc_i), // fetch high-byte from new_loc + 1
            };
            final.offset += Y; // final Address that contains the data
            return final;
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case IMME:
    case IMPL:
    case ACCU:
    case ZPY:
    case REL:
    case IND:
    default: ILLEGAL_ADDRESSING(mode, ERROR_FETCH_LOCATION);
    }
    UNREACHABLE("s502_fetch_operand_location");
}

// A, Z , N = M, memory into Accumulator
void s502_load_accumulator(Instruction instruction)
{
    accumulator = s502_fetch_operand_data(instruction.mode, instruction.operand);
    if (accumulator == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (accumulator & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// X,Z,N = M
void s502_load_x_register(Instruction instruction)
{
    X = s502_fetch_operand_data(instruction.mode, instruction.operand);
    if (X == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (X & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// Y,Z,N =  M
void s502_load_y_register(Instruction instruction)
{
    Y = s502_fetch_operand_data(instruction.mode, instruction.operand);
    if (Y == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (Y & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// M = A, store accumulator into memory
void s502_store_accumulator(Instruction instruction)
{
    Location loc = s502_fetch_operand_location(instruction.mode, instruction.operand);
    s502_write_memory(loc, accumulator); // Write Accumulator to memory
}

// M = Y, store accumulator into memory
void s502_store_y_register(Instruction instruction)
{
    Location loc = s502_fetch_operand_location(instruction.mode, instruction.operand);
    s502_write_memory(loc, Y); // Write Y to memory
}

// M = X, store accumulator into memory
void s502_store_x_register(Instruction instruction)
{
    Location loc = s502_fetch_operand_location(instruction.mode, instruction.operand);
    s502_write_memory(loc, X); // Write X to memory
}

// A = X, transfer X to A, TXA
void s502_transfer_x_to_accumulator()
{
    accumulator = X;
    if (accumulator == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (accumulator & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// A = Y, transfer Y to A, TYA
void s502_transfer_y_to_accumulator()
{
    accumulator = Y;
    if (accumulator == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (accumulator & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// X = A, transfer Accumulator to X, TAX
void s502_transfer_accumulator_to_x()
{
    X = accumulator;
    if (X == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (X & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// Y = A, transfer Accumulator to Y, TAY
void s502_transfer_accumulator_to_y()
{
    Y = accumulator;
    if (Y == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (Y & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

void s502_add_with_carry(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 raw = data + accumulator + (processor_status_register & C_BIT_FLAG);
    u8 result = (u8) raw;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (raw > MAX_U8) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(V_BIT_FLAG);
    if (~(accumulator ^ data) & (accumulator ^ result) & 0x80) {
        s502_set_psr_flags(V_BIT_FLAG);
    }
    accumulator = result;
}

void s502_sub_with_carry(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 raw = accumulator + (~data) + (processor_status_register & C_BIT_FLAG);
    u8 result = (u8) raw;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (raw > MAX_U8) s502_set_psr_flags(C_BIT_FLAG);

    // NOTE: N_BIT_FLAG = 0x80
    u8 sign_a = accumulator & N_BIT_FLAG;
    u8 sign_data_inverted = (~data) & N_BIT_FLAG;
    u8 sign_result = result & N_BIT_FLAG;

    if ((sign_a == sign_data_inverted) & (sign_a != sign_result)) {
        s502_set_psr_flags(V_BIT_FLAG);
    } else {
        s502_clear_psr_flags(V_BIT_FLAG);
    }
    accumulator = result;
}

void s502_compare_accumulator_with_data(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 result = accumulator - data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (accumulator == data) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (accumulator >= data) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);
}

void s502_compare_x_register_with_data(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 result = X - data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (X == data) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (X >= data) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);
}

void s502_compare_y_register_with_data(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 result = Y - data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (Y == data) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (Y >= data) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);
}

void s502_break()
{
    s502_set_psr_flags(B_BIT_FLAG);
    u8 pc_high_byte, pc_low_byte;
    u16_byte_split(program_counter, &pc_high_byte, &pc_low_byte); // Split the Program Counter
    s502_push_stack(pc_high_byte); // Push higher-byte first
    s502_push_stack(pc_low_byte); // Push lower-byte second
    s502_push_stack(processor_status_register); // Push the Process Status register
    // TODO: Make the Interrupt vector a const variable
    program_counter = 0xFFFF; // load the Interrupt Vector into the Program Counter
    printf("Program Interrupted\n");
}

bool s502_decode(Instruction instruction)
{
    switch (instruction.opcode) {
    case LDA: s502_load_accumulator(instruction);              return true;
    case LDY: s502_load_y_register(instruction);               return true;
    case LDX: s502_load_x_register(instruction);               return true;

    case STA: s502_store_accumulator(instruction);             return true;
    case STY: s502_store_y_register(instruction);              return true;
    case STX: s502_store_x_register(instruction);              return true;

    case TXA: s502_transfer_x_to_accumulator();                return true;
    case TYA: s502_transfer_y_to_accumulator();                return true;
    case TAX: s502_transfer_accumulator_to_x();                return true;
    case TAY: s502_transfer_accumulator_to_y();                return true;

    case CLC: s502_clear_psr_flags(C_BIT_FLAG);                return true;
    case CLD: s502_clear_psr_flags(D_BIT_FLAG);                return true;
    case CLI: s502_clear_psr_flags(I_BIT_FLAG);                return true;
    case CLV: s502_clear_psr_flags(V_BIT_FLAG);                return true;
    case SEC: s502_set_psr_flags(C_BIT_FLAG);                  return true;
    case SED: s502_set_psr_flags(D_BIT_FLAG);                  return true;
    case SEI: s502_set_psr_flags(I_BIT_FLAG);                  return true;

    case ADC: s502_add_with_carry(instruction);                return true;
    case SBC: s502_sub_with_carry(instruction);                return true;
    case CMP: s502_compare_accumulator_with_data(instruction); return true;
    case CPX: s502_compare_x_register_with_data(instruction);  return true;
    case CPY: s502_compare_y_register_with_data(instruction);  return true;

    case BRK: s502_break();                                    return true;

    case NOP: UNIMPLEMENTED("NOP");
    case RTI: UNIMPLEMENTED("RTI");
    case RTS: UNIMPLEMENTED("RTS");
    case JMP: UNIMPLEMENTED("JMP");
    case JSR: UNIMPLEMENTED("JSR");
    case ORA: UNIMPLEMENTED("ORA");
    case AND: UNIMPLEMENTED("AND");
    case EOR: UNIMPLEMENTED("EOR");
    case BIT: UNIMPLEMENTED("BIT");
    case TSX: UNIMPLEMENTED("TSX");
    case TXS: UNIMPLEMENTED("TXS");
    case PHA: UNIMPLEMENTED("PHA");
    case PHP: UNIMPLEMENTED("PHP");
    case PLA: UNIMPLEMENTED("PLA");
    case PLP: UNIMPLEMENTED("PLP");
    case INC: UNIMPLEMENTED("INC");
    case INX: UNIMPLEMENTED("INX");
    case INY: UNIMPLEMENTED("INY");
    case DEX: UNIMPLEMENTED("DEX");
    case DEY: UNIMPLEMENTED("DEY");
    case DEC: UNIMPLEMENTED("DEC");
    case BNE: UNIMPLEMENTED("BNE");
    case BCC: UNIMPLEMENTED("BCC");
    case BCS: UNIMPLEMENTED("BCS");
    case BEQ: UNIMPLEMENTED("BEQ");
    case BMI: UNIMPLEMENTED("BMI");
    case BPL: UNIMPLEMENTED("BPL");
    case BVC: UNIMPLEMENTED("BVC");
    case BVS: UNIMPLEMENTED("BVS");
    case ASL: UNIMPLEMENTED("ASL");
    case LSR: UNIMPLEMENTED("LSR");
    case ROL: UNIMPLEMENTED("ROL");
    case ROR: UNIMPLEMENTED("ROR");
    case ERROR_FETCH_DATA: UNREACHABLE("ERROR_FETCH_DATA");
    case ERROR_FETCH_LOCATION: UNREACHABLE("ERROR_FETCH_LOCATION");
    default:
        return false;
    }
}
