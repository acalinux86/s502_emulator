#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./s502.h"

#define S502_ASSERT(condition, message)                     \
    do {                                                    \
        if (!(condition)) {                                 \
            fprintf(stderr, "CPU FAULT: %s\n", message);    \
        }                                                   \
    } while(0)

Opcode_Info opcode_matrix[UINT8_MAX + 1] = {
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

// Helper Functions
void uint16_t_to_bytes(uint16_t sixteen_bit, uint8_t *high_byte, uint8_t *low_byte)
{
    *high_byte = sixteen_bit >> 8; // higher byte
    *low_byte  = sixteen_bit & 0xFF; // low-byte
}

// a must be the higher-byte and b the lower-byte
uint16_t bytes_to_uint16_t(uint8_t a, uint8_t b)
{
    return (a << 8) | b;
}

Location uint16_t_to_loc(uint16_t sixteen_bit)
{
    uint8_t high_byte, low_byte;
    uint16_t_to_bytes(sixteen_bit, &high_byte, &low_byte);
    Location loc = {
        .page = high_byte, // higher byte
        .offset = low_byte, // low-byte
    };
    return loc;
}

uint16_t loc_to_uint16_t(Location loc)
{
    return bytes_to_uint16_t(loc.page, loc.offset);
}

// DEBUG:
CPU s502_cpu_init(void)
{
    CPU cpu = {0};
    cpu.regx = 0;
    cpu.regy = 0;
    cpu.racc = 0;
    cpu.pc = 0;
    cpu.pc = U_BIT_FLAG;
    cpu.sp = 0xFF;
    array_new(&cpu.entries);
    return cpu;
}

uint8_t s502_read_memory(void *device, Location location)
{
    uint8_t *ram = (uint8_t*)device;
    return ram[bytes_to_uint16_t(location.page, location.offset)];
}

void s502_write_memory(void *device, Location location, uint8_t data)
{
    uint8_t *ram = (uint8_t*) device;
    ram[bytes_to_uint16_t(location.page, location.offset)] = data;
}

uint8_t s502_cpu_read(CPU *cpu, uint16_t addr)
{
    for (uint8_t i = 0; i < cpu->entries.count; ++i) {
        MMap_Entry *entry = &cpu->entries.items[i];
        if (addr >= entry->start_addr && addr <= entry->end_addr) {
            return entry->read(entry->device, uint16_t_to_loc(addr));
        }
    }

    fprintf(stderr, "ERROR: Address Read From Unmapped Memory: 0x%04x\n", addr);
    return 0;
}

void s502_cpu_write(CPU *cpu, uint16_t addr, uint8_t data)
{
    for (uint8_t i = 0; i < cpu->entries.count; ++i) {
        MMap_Entry *entry = &cpu->entries.items[i];
        if (!entry->readonly) {
            if (addr >= entry->start_addr && addr <= entry->end_addr) {
                entry->write(entry->device, uint16_t_to_loc(addr), data);
            }
        } else {
            fprintf(stderr, "CPU FAULT: Memory Readonly\n");
            exit(1);
        }
    }
}

void s502_push_stack(CPU *cpu, uint8_t value)
{
    // NOTE: Stack Operations are limited to only page one (Stack Pointer) of the 6502
    s502_cpu_write(cpu, bytes_to_uint16_t(STACK_PAGE, cpu->sp), value);
    cpu->sp--;
}

uint8_t s502_pull_stack(CPU *cpu)
{
    cpu->sp++;
    return s502_cpu_read(cpu, bytes_to_uint16_t(STACK_PAGE, cpu->sp));
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
void s502_set_psr_flags(CPU *cpu, Status_Flags flags)
{
    cpu->psr |= flags;
}

// NOTE: Clear PSR FLAGS
void s502_clear_psr_flags(CPU *cpu, Status_Flags flags)
{
    cpu->psr &= (~flags);
}

// Returns data for read opcodes
uint8_t s502_fetch_operand_data(CPU *cpu, Addressing_Modes mode, Operand operand)
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
            S502_ASSERT(location.page == 0x00, "Invalid Page Zero Address");
            return s502_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZPX: {
        // zero page X modes sums x reg to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MAX_OFFSET
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            S502_ASSERT(location.page == 0x00 , "Invalid Page Zero Address");
            Location new_loc = { .offset = location.offset + cpu->regx, .page = location.page};
            return s502_cpu_read(cpu, loc_to_uint16_t(new_loc));
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
            Location location = uint16_t_to_loc(absolute);
            return s502_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regx;
            Location location = uint16_t_to_loc(index);
            return s502_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regy;
            Location location = uint16_t_to_loc(index);
            return s502_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDX: {
        // Adds X reg to the offset of the location in the instruction
        // Fetches the data pointed by the modified location
        // Uses that data as a new location to index into memory and fetches low-byte
        // Uses the modified location.offset + 1 to index into memory and fetches high-byte
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            S502_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset + cpu->regx, .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            Location final = {
                .offset = s502_cpu_read(cpu, loc_to_uint16_t(new_loc)),   // fetch low-byte from new_loc
                .page =   s502_cpu_read(cpu, loc_to_uint16_t(new_loc_i)), // fetch high-byte from new_loc + 1
            };
            return s502_cpu_read(cpu, loc_to_uint16_t(final));
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y reg occurs in the final address
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            S502_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset    , .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            uint8_t offset = s502_cpu_read(cpu, loc_to_uint16_t(new_loc));   // fetch low-byte from new_loc
            uint8_t page =   s502_cpu_read(cpu, loc_to_uint16_t(new_loc_i)); // fetch high-byte from new_loc + 1
            uint16_t base_addr = bytes_to_uint16_t(page, offset);
            uint16_t final = base_addr + cpu->regy;
            return s502_cpu_read(cpu, final);
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case REL: {
        switch (operand.type) {
        case OPERAND_DATA: {
            // NOTE: Relative Addressing only always provides an offset
            // with which to be added to the PC
            return operand.data.data;
        } break;
        case OPERAND_ABSOLUTE:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case IMPL:
    case ACCU:
    case ZPY:
    case IND:
    default:
        ILLEGAL_ADDRESSING(mode, ERROR_FETCH_DATA);
    }
    UNREACHABLE("s502_fetch_operand_data");
}

// Returns A Location for store opcodes
Location s502_fetch_operand_location(CPU *cpu, Addressing_Modes mode, Operand operand)
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
        // zero page X modes sums x reg to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MAX_OFFSET
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            S502_ASSERT(location.page == 0x00 , "Invalid Page Zero Address");
            return (Location) { .offset = location.offset + cpu->regx, .page = location.page};
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
            return uint16_t_to_loc(absolute);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regx;
            return uint16_t_to_loc(index);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regy;
            return uint16_t_to_loc(index);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDX: {
        // Adds X reg to the offset of the location in the instruction
        // Fetches the data pointed by the modified location
        // Uses that data as a new location to index into memory and fetches low-byte
        // Uses the modified location.offset + 1 to index into memory and fetches high-byte
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            S502_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset + cpu->regx, .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            return (Location) {
                .offset = s502_cpu_read(cpu, loc_to_uint16_t(new_loc)),   // fetch low-byte from new_loc
                .page   = s502_cpu_read(cpu, loc_to_uint16_t(new_loc_i)), // fetch high-byte from new_loc + 1
            };
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y reg occurs in the final address
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            S502_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset    , .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            Location final = {
                .offset = s502_cpu_read(cpu, loc_to_uint16_t(new_loc)),   // fetch low-byte from new_loc
                .page =   s502_cpu_read(cpu, loc_to_uint16_t(new_loc_i)), // fetch high-byte from new_loc + 1
            };
            final.offset += cpu->regy; // final Address that contains the data
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

// Reg , Z , N = M, memory into Reg
void s502_load_reg(CPU *cpu, Instruction instruction, uint8_t *reg_type)
{
    *reg_type = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*reg_type == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*reg_type & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
}

// M = A | X | Y, store A | X | Y into memory
void s502_store_reg(CPU *cpu, Instruction instruction, uint8_t data)
{
    Location loc = s502_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    s502_cpu_write(cpu, loc_to_uint16_t(loc), data);  // Write Accumulator to memory
}

// A = X | Y, transfer X | Y to A, TXA | TYA
void s502_transfer_reg_to_accumulator(CPU *cpu, uint8_t data)
{
    cpu->racc = data;
    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->racc == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (cpu->racc & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
}

// X | Y = A, transfer Accumulator to X | Y, TAX | TAY
void s502_transfer_accumulator_to_reg(CPU *cpu, uint8_t *reg_type)
{
    *reg_type = cpu->racc;
    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*reg_type == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*reg_type & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
}

void s502_add_with_carry(CPU *cpu, Instruction instruction)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint16_t raw = data + cpu->racc + (cpu->psr & C_BIT_FLAG);
    uint8_t result = (uint8_t) raw;

    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG | C_BIT_FLAG | V_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
    if (raw > UINT8_MAX) s502_set_psr_flags(cpu, C_BIT_FLAG);
    if (~(cpu->racc ^ data) & (cpu->racc ^ result) & 0x80) {
        s502_set_psr_flags(cpu, V_BIT_FLAG);
    }
    cpu->racc = result;
}

void s502_sub_with_carry(CPU *cpu, Instruction instruction)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint16_t raw = cpu->racc + (~data) + (cpu->psr & C_BIT_FLAG);
    uint8_t result = (uint8_t) raw;

    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG | C_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
    if (raw > UINT8_MAX) s502_set_psr_flags(cpu, C_BIT_FLAG);

    // NOTE: N_BIT_FLAG = 0x80
    uint8_t sign_a = cpu->racc & N_BIT_FLAG;
    uint8_t sign_data_inverted = (~data) & N_BIT_FLAG;
    uint8_t sign_result = result & N_BIT_FLAG;

    if ((sign_a == sign_data_inverted) & (sign_a != sign_result)) {
        s502_set_psr_flags(cpu, V_BIT_FLAG);
    } else {
        s502_clear_psr_flags(cpu, V_BIT_FLAG);
    }
    cpu->racc = result;
}

void s502_compare_reg_with_data(CPU *cpu, Instruction instruction, uint8_t reg_type)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint16_t result = reg_type - data;

    s502_clear_psr_flags(cpu, Z_BIT_FLAG | C_BIT_FLAG | N_BIT_FLAG);
    if (reg_type == data) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (reg_type >= data) s502_set_psr_flags(cpu, C_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
}

void s502_transfer_stack_to_reg(CPU *cpu, uint8_t *data)
{
    *data = s502_cpu_read(cpu, bytes_to_uint16_t(STACK_PAGE, cpu->sp));
    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*data == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*data & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
}

// A | SR
void s502_push_reg_to_stack(CPU *cpu, uint8_t reg_type)
{
    s502_push_stack(cpu, reg_type);
}

// A or SR
void s502_pull_reg_from_stack(CPU *cpu, uint8_t *reg_type)
{
    *reg_type = s502_pull_stack(cpu);
}

void s502_logical_and(CPU *cpu, Instruction instruction)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc & data;

    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_clear_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void s502_logical_xor(CPU *cpu, Instruction instruction)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc ^ data;

    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_clear_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void s502_logical_or(CPU *cpu, Instruction instruction)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc | data;

    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) s502_clear_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void s502_bit_test(CPU *cpu, Instruction instruction)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc & data;

    s502_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG | V_BIT_FLAG);
    if (result == 0) s502_set_psr_flags(cpu, Z_BIT_FLAG);
    if (data & N_BIT_FLAG) s502_set_psr_flags(cpu, N_BIT_FLAG);
    if (data & V_BIT_FLAG) s502_set_psr_flags(cpu, V_BIT_FLAG);
}

void s502_branch_flag_clear(CPU *cpu, Instruction instruction, Status_Flags flag)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    if (cpu->psr & flag) {
        cpu->pc += (data);
    }
}

void s502_branch_flag_set(CPU *cpu, Instruction instruction, Status_Flags flag)
{
    uint8_t data = s502_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    if (!(cpu->psr & flag)) {
        cpu->pc += (data);
    }
}

void s502_break(CPU *cpu)
{
    s502_set_psr_flags(cpu, B_BIT_FLAG);
    uint8_t pc_high_byte, pc_low_byte;
    uint16_t_to_bytes(cpu->pc, &pc_high_byte, &pc_low_byte); // Split the Program Counter
    s502_push_stack(cpu, pc_high_byte); // Push higher-byte first
    s502_push_stack(cpu, pc_low_byte); // Push lower-byte second
    s502_push_stack(cpu, cpu->psr); // Push the Process Status reg
    // TODO: Make the Interrupt vector a const variable
    cpu->pc = s502_cpu_read(cpu, bytes_to_uint16_t(MAX_PAGES, MAX_OFFSET)); // load the Interrupt Vector into the Program Counter
    printf("Program Interrupted\n");
}

bool s502_decode(CPU *cpu, Instruction instruction)
{
    switch (instruction.opcode) {
    case LDA: s502_load_reg(cpu, instruction, &cpu->racc);             return true;
    case LDY: s502_load_reg(cpu, instruction, &cpu->regy);             return true;
    case LDX: s502_load_reg(cpu, instruction, &cpu->regx);             return true;

    case STA: s502_store_reg(cpu, instruction, cpu->racc);             return true;
    case STY: s502_store_reg(cpu, instruction, cpu->regy);             return true;
    case STX: s502_store_reg(cpu, instruction, cpu->regx);             return true;

    case TXA: s502_transfer_reg_to_accumulator(cpu, cpu->regx);        return true;
    case TYA: s502_transfer_reg_to_accumulator(cpu, cpu->regy);        return true;
    case TAX: s502_transfer_accumulator_to_reg(cpu, &cpu->regx);       return true;
    case TAY: s502_transfer_accumulator_to_reg(cpu, &cpu->regy);       return true;

    case CLC: s502_clear_psr_flags(cpu, C_BIT_FLAG);                        return true;
    case CLD: s502_clear_psr_flags(cpu, D_BIT_FLAG);                        return true;
    case CLI: s502_clear_psr_flags(cpu, I_BIT_FLAG);                        return true;
    case CLV: s502_clear_psr_flags(cpu, V_BIT_FLAG);                        return true;
    case SEC: s502_set_psr_flags(cpu, C_BIT_FLAG);                          return true;
    case SED: s502_set_psr_flags(cpu, D_BIT_FLAG);                          return true;
    case SEI: s502_set_psr_flags(cpu, I_BIT_FLAG);                          return true;

    case ADC: s502_add_with_carry(cpu, instruction);                        return true;
    case SBC: s502_sub_with_carry(cpu, instruction);                        return true;
    case CMP: s502_compare_reg_with_data(cpu, instruction, cpu->racc); return true;
    case CPX: s502_compare_reg_with_data(cpu, instruction, cpu->regx); return true;
    case CPY: s502_compare_reg_with_data(cpu, instruction, cpu->regy); return true;

    case TSX: s502_transfer_stack_to_reg(cpu, &cpu->regx);             return true;
    case TXS: s502_push_reg_to_stack(cpu, cpu->regx);                  return true;
    case PHA: s502_push_reg_to_stack(cpu, cpu->racc);                  return true;
    case PHP: s502_push_reg_to_stack(cpu, cpu->psr);                   return true;
    case PLA: s502_pull_reg_from_stack(cpu, &cpu->racc);               return true;
    case PLP: s502_pull_reg_from_stack(cpu, &cpu->psr);                return true;

    case ORA: s502_logical_or(cpu, instruction);                            return true;
    case AND: s502_logical_and(cpu, instruction);                           return true;
    case EOR: s502_logical_xor(cpu, instruction);                           return true;
    case BIT: s502_bit_test(cpu, instruction);                              return true;

    case BNE: s502_branch_flag_clear(cpu, instruction, Z_BIT_FLAG);         return true;
    case BCC: s502_branch_flag_clear(cpu, instruction, C_BIT_FLAG);         return true;
    case BPL: s502_branch_flag_clear(cpu, instruction, N_BIT_FLAG);         return true;
    case BVC: s502_branch_flag_clear(cpu, instruction, V_BIT_FLAG);         return true;
    case BEQ: s502_branch_flag_set(cpu, instruction, Z_BIT_FLAG);           return true;
    case BCS: s502_branch_flag_set(cpu, instruction, C_BIT_FLAG);           return true;
    case BMI: s502_branch_flag_set(cpu, instruction, N_BIT_FLAG);           return true;
    case BVS: s502_branch_flag_set(cpu, instruction, V_BIT_FLAG);           return true;

    case BRK: s502_break(cpu);                                              return true;

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

    case ASL: UNIMPLEMENTED("ASL");
    case LSR: UNIMPLEMENTED("LSR");
    case ROL: UNIMPLEMENTED("ROL");
    case ROR: UNIMPLEMENTED("ROR");
    case ERROR_FETCH_DATA:
    case ERROR_FETCH_LOCATION:
    default:
        UNREACHABLE("decode");
        return false;
    }
}
