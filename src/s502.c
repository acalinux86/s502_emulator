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

CPU cpu = {0};

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
    cpu.memory[0x01][cpu.stack--] = value;
}

u8 s502_pull_stack()
{
    return cpu.memory[0x01][cpu.stack++];
}

void s502_dump_memory()
{
    for (u8 i = 0; i < MAX_PAGES; ++i) {
        s502_dump_page(cpu.memory[i]);
    }
}

// NOTE: Print Stats
void s502_print_stats()
{
    printf("Stack       : %u\n", cpu.stack);
    printf("X           : %u\n", cpu.x);
    printf("Y           : %u\n", cpu.y);
    printf("Accumulator : %u\n", cpu.accumulator);
    printf("PC          : %u\n", cpu.program_counter);
    printf("PSR         : %u\n", cpu.status_register);
}

u8 s502_read_memory(Location location)
{
    return cpu.memory[location.page][location.offset];
}

void s502_write_memory(Location location, u8 data)
{
    cpu.memory[location.page][location.offset] = data;
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
    cpu.status_register |= flags;
}

// NOTE: Clear PSR FLAGS
void s502_clear_psr_flags(PSR_Flags flags)
{
    cpu.status_register &= (~flags);
}

void u16_byte_split(u16 sixteen_bit, u8 *high_byte, u8 *low_byte)
{
    *high_byte = sixteen_bit >> 8; // higher byte
    *low_byte  = sixteen_bit & 0xFF; // low-byte
}

// a must be the higher-byte and b the lower-byte
u16 u8_bytes_join(u8 a, u8 b)
{
    return (a << 8) | b;
}

Location u16_to_loc(u16 sixteen_bit)
{
    u8 high_byte, low_byte;
    u16_byte_split(sixteen_bit, &high_byte, &low_byte);
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
            Location new_loc = { .offset = location.offset + cpu.x, .page = location.page};
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
            Absolute index = absolute + cpu.x;
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
            Absolute index = absolute + cpu.y;
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
            Location new_loc   = {.offset = location.offset + cpu.x, .page = location.page};
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
            final.offset += cpu.y; // final Address that contains the data
            return s502_read_memory(final);
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case REL: {
        switch (operand.type) {
        case OPERAND_DATA: {
            return s502_read_memory(operand.data.data);
        } break;
        case OPERAND_ABSOLUTE:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(operand.mode, operand.type);
    } break;

    case IMPL:
    case ACCU:
    case ZPY:
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
            return (Location) { .offset = location.offset + cpu.x, .page = location.page};
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
            Absolute index = absolute + cpu.x;
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
            Absolute index = absolute + cpu.y;
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
            Location new_loc   = {.offset = location.offset + cpu.x, .page = location.page};
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
            final.offset += cpu.y; // final Address that contains the data
            return final;
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case REL:
    case IMME:
    case IMPL:
    case ACCU:
    case ZPY:
    case IND:
    default: ILLEGAL_ADDRESSING(mode, ERROR_FETCH_LOCATION);
    }
    UNREACHABLE("s502_fetch_operand_location");
}

// A, Z , N = M, memory into Accumulator
void s502_load_accumulator(Instruction instruction)
{
    cpu.accumulator = s502_fetch_operand_data(instruction.mode, instruction.operand);
    if (cpu.accumulator == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (cpu.accumulator & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// X,Z,N = M
void s502_load_x_register(Instruction instruction)
{
    cpu.x = s502_fetch_operand_data(instruction.mode, instruction.operand);
    if (cpu.x == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (cpu.x & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// Y,Z,N =  M
void s502_load_y_register(Instruction instruction)
{
    cpu.y = s502_fetch_operand_data(instruction.mode, instruction.operand);
    if (cpu.y == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (cpu.y & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// M = A, store accumulator into memory
void s502_store_accumulator(Instruction instruction)
{
    Location loc = s502_fetch_operand_location(instruction.mode, instruction.operand);
    s502_write_memory(loc, cpu.accumulator); // Write Accumulator to memory
}

// M = Y, store accumulator into memory
void s502_store_y_register(Instruction instruction)
{
    Location loc = s502_fetch_operand_location(instruction.mode, instruction.operand);
    s502_write_memory(loc, cpu.y); // Write Y to memory
}

// M = X, store accumulator into memory
void s502_store_x_register(Instruction instruction)
{
    Location loc = s502_fetch_operand_location(instruction.mode, instruction.operand);
    s502_write_memory(loc, cpu.x); // Write X to memory
}

// A = X, transfer X to A, TXA
void s502_transfer_x_to_accumulator()
{
    cpu.accumulator = cpu.x;
    if (cpu.accumulator == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (cpu.accumulator & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// A = Y, transfer Y to A, TYA
void s502_transfer_y_to_accumulator()
{
    cpu.accumulator = cpu.y;
    if (cpu.accumulator == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (cpu.accumulator & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// X = A, transfer Accumulator to X, TAX
void s502_transfer_accumulator_to_x()
{
    cpu.x = cpu.accumulator;
    if (cpu.x == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (cpu.x & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

// Y = A, transfer Accumulator to Y, TAY
void s502_transfer_accumulator_to_y()
{
    cpu.y = cpu.accumulator;
    if (cpu.y == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (cpu.y & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }
}

void s502_add_with_carry(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 raw = data + cpu.accumulator + (cpu.status_register & C_BIT_FLAG);
    u8 result = (u8) raw;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (raw > MAX_U8) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(V_BIT_FLAG);
    if (~(cpu.accumulator ^ data) & (cpu.accumulator ^ result) & 0x80) {
        s502_set_psr_flags(V_BIT_FLAG);
    }
    cpu.accumulator = result;
}

void s502_sub_with_carry(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 raw = cpu.accumulator + (~data) + (cpu.status_register & C_BIT_FLAG);
    u8 result = (u8) raw;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (raw > MAX_U8) s502_set_psr_flags(C_BIT_FLAG);

    // NOTE: N_BIT_FLAG = 0x80
    u8 sign_a = cpu.accumulator & N_BIT_FLAG;
    u8 sign_data_inverted = (~data) & N_BIT_FLAG;
    u8 sign_result = result & N_BIT_FLAG;

    if ((sign_a == sign_data_inverted) & (sign_a != sign_result)) {
        s502_set_psr_flags(V_BIT_FLAG);
    } else {
        s502_clear_psr_flags(V_BIT_FLAG);
    }
    cpu.accumulator = result;
}

void s502_compare_accumulator_with_data(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 result = cpu.accumulator - data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (cpu.accumulator == data) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (cpu.accumulator >= data) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);
}

void s502_compare_x_register_with_data(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 result = cpu.x - data;
    s502_clear_psr_flags(Z_BIT_FLAG);
    if (cpu.x == data) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (cpu.x >= data) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);
}

void s502_compare_y_register_with_data(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u16 result = cpu.y - data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (cpu.y == data) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(C_BIT_FLAG);
    if (cpu.y >= data) s502_set_psr_flags(C_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);
}

void s502_transfer_stack_to_x()
{
    cpu.x = cpu.memory[0x01][cpu.stack];

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (cpu.x == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (cpu.x & N_BIT_FLAG) s502_set_psr_flags(N_BIT_FLAG);
}

void s502_transfer_x_to_stack()
{
    s502_push_stack(cpu.x);
}

void s502_transfer_accumulator_to_stack()
{
    s502_push_stack(cpu.accumulator);
}

void s502_transfer_status_register_to_stack()
{
    s502_push_stack(cpu.status_register);
}

void s502_pull_accumulator_from_stack()
{
    cpu.accumulator = s502_pull_stack();
}

void s502_pull_status_register_from_stack()
{
    cpu.status_register = s502_pull_stack();
}

void s502_logical_and(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u8 result = cpu.accumulator & data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_clear_psr_flags(N_BIT_FLAG);

    cpu.accumulator = result;
}

void s502_logical_xor(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u8 result = cpu.accumulator ^ data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_clear_psr_flags(N_BIT_FLAG);

    cpu.accumulator = result;
}

void s502_logical_or(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u8 result = cpu.accumulator | data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_clear_psr_flags(N_BIT_FLAG);

    cpu.accumulator = result;
}

void s502_bit_test(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    u8 result = cpu.accumulator & data;

    s502_clear_psr_flags(Z_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(Z_BIT_FLAG);

    s502_clear_psr_flags(N_BIT_FLAG);
    s502_clear_psr_flags(V_BIT_FLAG);
    u8 m6 = data & (1 << 6);
    u8 m7 = data & (1 << 7);
    cpu.status_register |= m6;
    cpu.status_register |= m7;
}

void s502_break()
{
    s502_set_psr_flags(B_BIT_FLAG);
    u8 pc_high_byte, pc_low_byte;
    u16_byte_split(cpu.program_counter, &pc_high_byte, &pc_low_byte); // Split the Program Counter
    s502_push_stack(pc_high_byte); // Push higher-byte first
    s502_push_stack(pc_low_byte); // Push lower-byte second
    s502_push_stack(cpu.status_register); // Push the Process Status register
    // TODO: Make the Interrupt vector a const variable
    cpu.program_counter = cpu.memory[MAX_PAGES][MAX_OFFSET]; // load the Interrupt Vector into the Program Counter
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

    case TSX: s502_transfer_stack_to_x();                      return true;
    case TXS: s502_transfer_x_to_stack();                      return true;
    case PHA: s502_transfer_accumulator_to_stack();            return true;
    case PHP: s502_transfer_status_register_to_stack();        return true;
    case PLA: s502_pull_accumulator_from_stack();              return true;
    case PLP: s502_pull_status_register_from_stack();          return true;

    case ORA: s502_logical_or(instruction);                    return true;
    case AND: s502_logical_and(instruction);                   return true;
    case EOR: s502_logical_xor(instruction);                   return true;
    case BIT: s502_bit_test(instruction);                      return true;

    case BRK: s502_break();                                    return true;

    case NOP: UNIMPLEMENTED("NOP");
    case RTI: UNIMPLEMENTED("RTI");
    case RTS: UNIMPLEMENTED("RTS");
    case JMP: UNIMPLEMENTED("JMP");
    case JSR: UNIMPLEMENTED("JSR");


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
