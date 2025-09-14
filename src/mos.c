#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./mos.h"

#define MOS_ASSERT(condition, message)                     \
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
void mos_uint16_t_to_bytes(uint16_t sixteen_bit, uint8_t *high_byte, uint8_t *low_byte)
{
    *high_byte = sixteen_bit >> 8; // higher byte
    *low_byte  = sixteen_bit & 0xFF; // low-byte
}

// a must be the higher-byte and b the lower-byte
uint16_t mos_bytes_to_uint16_t(uint8_t a, uint8_t b)
{
    return (a << 8) | b;
}

Location mos_uint16_t_to_loc(uint16_t sixteen_bit)
{
    uint8_t high_byte, low_byte;
    mos_uint16_t_to_bytes(sixteen_bit, &high_byte, &low_byte);
    Location loc = {
        .page = high_byte, // higher byte
        .offset = low_byte, // low-byte
    };
    return loc;
}

uint16_t loc_to_uint16_t(Location loc)
{
    return mos_bytes_to_uint16_t(loc.page, loc.offset);
}

// DEBUG:
CPU mos_cpu_init(void)
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

uint8_t mos_read_memory(void *device, Location location)
{
    uint8_t *ram = (uint8_t*)device;
    return ram[mos_bytes_to_uint16_t(location.page, location.offset)];
}

void mos_write_memory(void *device, Location location, uint8_t data)
{
    uint8_t *ram = (uint8_t*) device;
    ram[mos_bytes_to_uint16_t(location.page, location.offset)] = data;
}

uint8_t mos_cpu_read(CPU *cpu, uint16_t addr)
{
    for (uint8_t i = 0; i < cpu->entries.count; ++i) {
        MMap_Entry *entry = &cpu->entries.items[i];
        if (addr >= entry->start_addr && addr <= entry->end_addr) {
            return entry->read(entry->device, mos_uint16_t_to_loc(addr));
        }
    }

    fprintf(stderr, "ERROR: Address Read From Unmapped Memory: 0x%04x\n", addr);
    return 0;
}

void mos_cpu_write(CPU *cpu, uint16_t addr, uint8_t data)
{
    for (uint8_t i = 0; i < cpu->entries.count; ++i) {
        MMap_Entry *entry = &cpu->entries.items[i];
        if (!entry->readonly) {
            if (addr >= entry->start_addr && addr <= entry->end_addr) {
                entry->write(entry->device, mos_uint16_t_to_loc(addr), data);
            }
        } else {
            fprintf(stderr, "CPU FAULT: Memory Readonly\n");
            exit(1);
        }
    }
}

void mos_push_stack(CPU *cpu, uint8_t value)
{
    // NOTE: Stack Operations are limited to only page one (Stack Pointer) of the 6502
    mos_cpu_write(cpu, mos_bytes_to_uint16_t(STACK_PAGE, cpu->sp), value);
    cpu->sp--;
}

uint8_t mos_pull_stack(CPU *cpu)
{
    cpu->sp++;
    return mos_cpu_read(cpu, mos_bytes_to_uint16_t(STACK_PAGE, cpu->sp));
}


const char *mos_addr_mode_as_cstr(Addressing_Modes mode)
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

const char *mos_opcode_as_cstr(Opcode opcode)
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

const char *mos_operand_type_as_cstr(Operand_Type type)
{
    switch (type) {
    case OPERAND_ABSOLUTE: return "OPERAND_ABSOLUTE";
    case OPERAND_LOCATION: return "OPERAND_LOCATION";
    case OPERAND_DATA:     return "OPERAND_DATA";
    default:               return NULL;
    }
}

// NOTE: Set PSR FLAGS
void mos_set_psr_flags(CPU *cpu, Status_Flags flags)
{
    cpu->psr |= flags;
}

// NOTE: Clear PSR FLAGS
void mos_clear_psr_flags(CPU *cpu, Status_Flags flags)
{
    cpu->psr &= (~flags);
}

// Returns data for read opcodes
uint8_t mos_fetch_operand_data(CPU *cpu, Addressing_Modes mode, Operand operand)
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
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZP: {
        switch (operand.type) {
        case OPERAND_LOCATION: {
            // if the operand doesn't contain any page, assumed that it is page zero
            Location location = operand.data.address.loc;
            MOS_ASSERT(location.page == 0x00, "Invalid Page Zero Address");
            return mos_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZPX: {
        // zero page X modes sums x reg to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MAX_OFFSET
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            MOS_ASSERT(location.page == 0x00 , "Invalid Page Zero Address");
            Location new_loc = { .offset = location.offset + cpu->regx, .page = location.page};
            return mos_cpu_read(cpu, loc_to_uint16_t(new_loc));
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABS: {
        // uses the absolute location in operand to load access memory into accumulator
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Location location = mos_uint16_t_to_loc(absolute);
            return mos_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regx;
            Location location = mos_uint16_t_to_loc(index);
            return mos_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regy;
            Location location = mos_uint16_t_to_loc(index);
            return mos_cpu_read(cpu, loc_to_uint16_t(location));
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
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
            MOS_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset + cpu->regx, .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            Location final = {
                .offset = mos_cpu_read(cpu, loc_to_uint16_t(new_loc)),   // fetch low-byte from new_loc
                .page =   mos_cpu_read(cpu, loc_to_uint16_t(new_loc_i)), // fetch high-byte from new_loc + 1
            };
            return mos_cpu_read(cpu, loc_to_uint16_t(final));
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y reg occurs in the final address
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            MOS_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset    , .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            uint8_t offset = mos_cpu_read(cpu, loc_to_uint16_t(new_loc));   // fetch low-byte from new_loc
            uint8_t page =   mos_cpu_read(cpu, loc_to_uint16_t(new_loc_i)); // fetch high-byte from new_loc + 1
            uint16_t base_addr = mos_bytes_to_uint16_t(page, offset);
            uint16_t final = base_addr + cpu->regy;
            return mos_cpu_read(cpu, final);
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
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
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case IMPL:
    case ACCU:
    case ZPY:
    case IND:
    default:
        MOS_ILLEGAL_ADDRESSING(mode, ERROR_FETCH_DATA);
    }
    MOS_UNREACHABLE("mos_fetch_operand_data");
}

// Returns A Location for store opcodes
Location mos_fetch_operand_location(CPU *cpu, Addressing_Modes mode, Operand operand)
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
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZPX: {
        // zero page X modes sums x reg to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MAX_OFFSET
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            MOS_ASSERT(location.page == 0x00 , "Invalid Page Zero Address");
            return (Location) { .offset = location.offset + cpu->regx, .page = location.page};
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABS: {
        // uses the absolute location in operand to load access memory into accumulator
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            return mos_uint16_t_to_loc(absolute);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regx;
            return mos_uint16_t_to_loc(index);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y reg
        switch (operand.type) {
        case OPERAND_ABSOLUTE: {
            Absolute absolute = operand.data.address.absolute;
            Absolute index = absolute + cpu->regy;
            return mos_uint16_t_to_loc(index);
        }
        case OPERAND_DATA:
        case OPERAND_LOCATION:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
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
            MOS_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset + cpu->regx, .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            return (Location) {
                .offset = mos_cpu_read(cpu, loc_to_uint16_t(new_loc)),   // fetch low-byte from new_loc
                .page   = mos_cpu_read(cpu, loc_to_uint16_t(new_loc_i)), // fetch high-byte from new_loc + 1
            };
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y reg occurs in the final address
        switch (operand.type) {
        case OPERAND_LOCATION: {
            Location location = operand.data.address.loc;
            MOS_ASSERT(location.page == 0x00 , "Invalid Zero Page Address");
            Location new_loc   = {.offset = location.offset    , .page = location.page};
            Location new_loc_i = {.offset = new_loc.offset  + 1, .page = new_loc.page};
            Location final = {
                .offset = mos_cpu_read(cpu, loc_to_uint16_t(new_loc)),   // fetch low-byte from new_loc
                .page =   mos_cpu_read(cpu, loc_to_uint16_t(new_loc_i)), // fetch high-byte from new_loc + 1
            };
            final.offset += cpu->regy; // final Address that contains the data
            return final;
        }
        case OPERAND_ABSOLUTE:
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case REL:
    case IMME:
    case IMPL:
    case ACCU:
    case ZPY:
    case IND:
    default: MOS_ILLEGAL_ADDRESSING(mode, ERROR_FETCH_LOCATION);
    }
    MOS_UNREACHABLE("mos_fetch_operand_location");
}

// Reg , Z , N = M, memory into Reg
void mos_load_reg(CPU *cpu, Instruction instruction, uint8_t *reg_type)
{
    *reg_type = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*reg_type == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*reg_type & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

// M = A | X | Y, store A | X | Y into memory
void mos_store_reg(CPU *cpu, Instruction instruction, uint8_t data)
{
    Location loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    mos_cpu_write(cpu, loc_to_uint16_t(loc), data);  // Write Accumulator to memory
}

// A = X | Y, transfer X | Y to A, TXA | TYA
void mos_transfer_reg_to_accumulator(CPU *cpu, uint8_t data)
{
    cpu->racc = data;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->racc == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (cpu->racc & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

// X | Y = A, transfer Accumulator to X | Y, TAX | TAY
void mos_transfer_accumulator_to_reg(CPU *cpu, uint8_t *reg_type)
{
    *reg_type = cpu->racc;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*reg_type == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*reg_type & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_add_with_carry(CPU *cpu, Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint16_t raw = data + cpu->racc + (cpu->psr & C_BIT_FLAG);
    uint8_t result = (uint8_t) raw;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG | C_BIT_FLAG | V_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    if (raw > UINT8_MAX) mos_set_psr_flags(cpu, C_BIT_FLAG);
    if (~(cpu->racc ^ data) & (cpu->racc ^ result) & 0x80) {
        mos_set_psr_flags(cpu, V_BIT_FLAG);
    }
    cpu->racc = result;
}

void mos_sub_with_carry(CPU *cpu, Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint16_t raw = cpu->racc + (~data) + (cpu->psr & C_BIT_FLAG);
    uint8_t result = (uint8_t) raw;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG | C_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    if (raw > UINT8_MAX) mos_set_psr_flags(cpu, C_BIT_FLAG);

    // NOTE: N_BIT_FLAG = 0x80
    uint8_t sign_a = cpu->racc & N_BIT_FLAG;
    uint8_t sign_data_inverted = (~data) & N_BIT_FLAG;
    uint8_t sign_result = result & N_BIT_FLAG;

    if ((sign_a == sign_data_inverted) & (sign_a != sign_result)) {
        mos_set_psr_flags(cpu, V_BIT_FLAG);
    } else {
        mos_clear_psr_flags(cpu, V_BIT_FLAG);
    }
    cpu->racc = result;
}

void mos_compare_reg_with_data(CPU *cpu, Instruction instruction, uint8_t reg_type)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint16_t result = reg_type - data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | C_BIT_FLAG | N_BIT_FLAG);
    if (reg_type == data) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (reg_type >= data) mos_set_psr_flags(cpu, C_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_transfer_stack_to_reg(CPU *cpu, uint8_t *data)
{
    *data = mos_cpu_read(cpu, mos_bytes_to_uint16_t(STACK_PAGE, cpu->sp));
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*data == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*data & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

// A | SR
void mos_push_reg_to_stack(CPU *cpu, uint8_t reg_type)
{
    mos_push_stack(cpu, reg_type);
}

// A or SR
void mos_pull_reg_from_stack(CPU *cpu, uint8_t *reg_type)
{
    *reg_type = mos_pull_stack(cpu);
}

void mos_logical_and(CPU *cpu, Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc & data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_clear_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_logical_xor(CPU *cpu, Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc ^ data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_clear_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_logical_or(CPU *cpu, Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc | data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_clear_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_bit_test(CPU *cpu, Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc & data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG | V_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (data & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    if (data & V_BIT_FLAG) mos_set_psr_flags(cpu, V_BIT_FLAG);
}

void mos_branch_flag_clear(CPU *cpu, Instruction instruction, Status_Flags flag)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    if (cpu->psr & flag) {
        cpu->pc += (data);
    }
}

void mos_branch_flag_set(CPU *cpu, Instruction instruction, Status_Flags flag)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    if (!(cpu->psr & flag)) {
        cpu->pc += (data);
    }
}

void mos_decrement(CPU *cpu, Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    (void) data;
}

void mos_break(CPU *cpu)
{
    mos_set_psr_flags(cpu, B_BIT_FLAG);
    uint8_t pc_high_byte, pc_low_byte;
    mos_uint16_t_to_bytes(cpu->pc, &pc_high_byte, &pc_low_byte); // Split the Program Counter
    mos_push_stack(cpu, pc_high_byte); // Push higher-byte first
    mos_push_stack(cpu, pc_low_byte); // Push lower-byte second
    mos_push_stack(cpu, cpu->psr); // Push the Process Status reg
    // TODO: Make the Interrupt vector a const variable
    cpu->pc = mos_cpu_read(cpu, mos_bytes_to_uint16_t(MAX_PAGES, MAX_OFFSET)); // load the Interrupt Vector into the Program Counter
    printf("Program Interrupted\n");
}

bool mos_decode(CPU *cpu, Instruction instruction)
{
    switch (instruction.opcode) {
    case LDA: mos_load_reg(cpu, instruction, &cpu->racc);             return true;
    case LDY: mos_load_reg(cpu, instruction, &cpu->regy);             return true;
    case LDX: mos_load_reg(cpu, instruction, &cpu->regx);             return true;

    case STA: mos_store_reg(cpu, instruction, cpu->racc);             return true;
    case STY: mos_store_reg(cpu, instruction, cpu->regy);             return true;
    case STX: mos_store_reg(cpu, instruction, cpu->regx);             return true;

    case TXA: mos_transfer_reg_to_accumulator(cpu, cpu->regx);        return true;
    case TYA: mos_transfer_reg_to_accumulator(cpu, cpu->regy);        return true;
    case TAX: mos_transfer_accumulator_to_reg(cpu, &cpu->regx);       return true;
    case TAY: mos_transfer_accumulator_to_reg(cpu, &cpu->regy);       return true;

    case CLC: mos_clear_psr_flags(cpu, C_BIT_FLAG);                   return true;
    case CLD: mos_clear_psr_flags(cpu, D_BIT_FLAG);                   return true;
    case CLI: mos_clear_psr_flags(cpu, I_BIT_FLAG);                   return true;
    case CLV: mos_clear_psr_flags(cpu, V_BIT_FLAG);                   return true;
    case SEC: mos_set_psr_flags(cpu, C_BIT_FLAG);                     return true;
    case SED: mos_set_psr_flags(cpu, D_BIT_FLAG);                     return true;
    case SEI: mos_set_psr_flags(cpu, I_BIT_FLAG);                     return true;

    case ADC: mos_add_with_carry(cpu, instruction);                   return true;
    case SBC: mos_sub_with_carry(cpu, instruction);                   return true;
    case CMP: mos_compare_reg_with_data(cpu, instruction, cpu->racc); return true;
    case CPX: mos_compare_reg_with_data(cpu, instruction, cpu->regx); return true;
    case CPY: mos_compare_reg_with_data(cpu, instruction, cpu->regy); return true;

    case TSX: mos_transfer_stack_to_reg(cpu, &cpu->regx);             return true;
    case TXS: mos_push_reg_to_stack(cpu, cpu->regx);                  return true;
    case PHA: mos_push_reg_to_stack(cpu, cpu->racc);                  return true;
    case PHP: mos_push_reg_to_stack(cpu, cpu->psr);                   return true;
    case PLA: mos_pull_reg_from_stack(cpu, &cpu->racc);               return true;
    case PLP: mos_pull_reg_from_stack(cpu, &cpu->psr);                return true;

    case ORA: mos_logical_or(cpu, instruction);                       return true;
    case AND: mos_logical_and(cpu, instruction);                      return true;
    case EOR: mos_logical_xor(cpu, instruction);                      return true;
    case BIT: mos_bit_test(cpu, instruction);                         return true;

    case BNE: mos_branch_flag_clear(cpu, instruction, Z_BIT_FLAG);    return true;
    case BCC: mos_branch_flag_clear(cpu, instruction, C_BIT_FLAG);    return true;
    case BPL: mos_branch_flag_clear(cpu, instruction, N_BIT_FLAG);    return true;
    case BVC: mos_branch_flag_clear(cpu, instruction, V_BIT_FLAG);    return true;
    case BEQ: mos_branch_flag_set(cpu, instruction, Z_BIT_FLAG);      return true;
    case BCS: mos_branch_flag_set(cpu, instruction, C_BIT_FLAG);      return true;
    case BMI: mos_branch_flag_set(cpu, instruction, N_BIT_FLAG);      return true;
    case BVS: mos_branch_flag_set(cpu, instruction, V_BIT_FLAG);      return true;

    case INC:                                                         return true;
    case INX:                                                         return true;
    case INY:                                                         return true;
    case DEX:                                                         return true;
    case DEY:                                                         return true;
    case DEC:                                                         return true;

    case BRK: mos_break(cpu);                                         return true;

    case NOP: MOS_UNIMPLEMENTED("NOP");
    case RTI: MOS_UNIMPLEMENTED("RTI");
    case RTS: MOS_UNIMPLEMENTED("RTS");
    case JMP: MOS_UNIMPLEMENTED("JMP");
    case JSR: MOS_UNIMPLEMENTED("JSR");

    case ASL: MOS_UNIMPLEMENTED("ASL");
    case LSR: MOS_UNIMPLEMENTED("LSR");
    case ROL: MOS_UNIMPLEMENTED("ROL");
    case ROR: MOS_UNIMPLEMENTED("ROR");
    case ERROR_FETCH_DATA:
    case ERROR_FETCH_LOCATION:
    default:
        MOS_UNREACHABLE("decode");
        return false;
    }
}
