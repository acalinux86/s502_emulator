#include "./mos.h"

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

// DEBUG:
MOS_Cpu mos_cpu_init(void)
{
    MOS_Cpu cpu = {0};
    cpu.regx = 0; cpu.regy = 0; cpu.racc = 0;
    cpu.pc = 0;   cpu.psr = U_BIT_FLAG; cpu.sp = 0xFF;
    array_new(&cpu.entries);
    return cpu;
}

uint8_t mos_read_memory(void *device, uint16_t location)
{
    uint8_t *ram = (uint8_t*)device;
    return ram[location];
}

void mos_write_memory(void *device, uint16_t location, uint8_t data)
{
    uint8_t *ram = (uint8_t*) device;
    ram[location] = data;
}

uint8_t mos_cpu_read(MOS_Cpu *cpu, uint16_t addr)
{
    for (uint8_t i = 0; i < cpu->entries.count; ++i) {
        MOS_MMap *entry = &cpu->entries.items[i];
        if (addr >= entry->start_addr && addr <= entry->end_addr) {
            return entry->read(entry->device, addr);
        }
    }

    fprintf(stderr, "ERROR: Address Read From Unmapped Memory: 0x%04x\n", addr);
    return 0;
}

void mos_cpu_write(MOS_Cpu *cpu, uint16_t addr, uint8_t data)
{
    for (uint8_t i = 0; i < cpu->entries.count; ++i) {
        MOS_MMap *entry = &cpu->entries.items[i];
        if (!entry->readonly) {
            if (addr >= entry->start_addr && addr <= entry->end_addr) {
                entry->write(entry->device, addr, data);
                return; // successful write, return early
            } else {
                fprintf(stderr, "ERROR: Address Written to Unmapped Memory: 0x%04x\n", addr);
                exit(1);
            }
        } else {
            fprintf(stderr, "MOS_Cpu FAULT: Memory Readonly\n");
            exit(1);
        }
    }
    fprintf(stderr, "ERROR: Address Written to Unmapped Memory: 0x%04x\n", addr);
    exit(1);
}

void mos_push_stack(MOS_Cpu *cpu, uint8_t value)
{
    // NOTE: Stack Operations are limited to only page one (Stack Pointer) of the 6502
    mos_cpu_write(cpu, mos_bytes_to_uint16_t(MOS_STACK_PAGE, cpu->sp), value);
    cpu->sp--;
}

uint8_t mos_pull_stack(MOS_Cpu *cpu)
{
    cpu->sp++;
    return mos_cpu_read(cpu, mos_bytes_to_uint16_t(MOS_STACK_PAGE, cpu->sp));
}

// NOTE: Set PSR FLAGS
void mos_set_psr_flags(MOS_Cpu *cpu, Status_Flags flags)
{
    cpu->psr |= flags;
}

// NOTE: Clear PSR FLAGS
void mos_clear_psr_flags(MOS_Cpu *cpu, Status_Flags flags)
{
    cpu->psr &= (~flags);
}

uint16_t mos_zero_page(uint16_t location)
{
    uint8_t high_byte, low_byte;
    mos_uint16_t_to_bytes(location, &high_byte, &low_byte);
    MOS_ASSERT(high_byte == 0x00, "Invalid Page Zero Address");
    return location;
}

uint16_t mos_zero_page_x(MOS_Cpu *cpu, uint16_t location)
{
    uint8_t high_byte, low_byte;
    mos_uint16_t_to_bytes(location, &high_byte, &low_byte);
    MOS_ASSERT(high_byte == 0x00 , "Invalid Page Zero Address");
    return mos_bytes_to_uint16_t(high_byte, low_byte + cpu->regx);
}

uint16_t mos_absolute_x(MOS_Cpu *cpu, uint16_t location)
{
    return location + cpu->regx;
}

uint16_t mos_absolute_y(MOS_Cpu *cpu, uint16_t location)
{
    return location + cpu->regy;
}

uint16_t mos_indirect_x(MOS_Cpu *cpu, uint16_t location)
{
    uint8_t high_byte, low_byte;
    mos_uint16_t_to_bytes(location, &high_byte, &low_byte);

    uint16_t new_loc = mos_bytes_to_uint16_t(high_byte , low_byte + cpu->regx);
    mos_uint16_t_to_bytes(new_loc, &high_byte, &low_byte);

    uint16_t new_loc_i = mos_bytes_to_uint16_t(high_byte , low_byte + 1);

    // fetch low-byte from new_loc, fetch high-byte from new_loc + 1
    return mos_bytes_to_uint16_t(mos_cpu_read(cpu, new_loc_i), mos_cpu_read(cpu, new_loc));
}

uint16_t mos_indirect_y(MOS_Cpu *cpu, uint16_t location)
{
    uint8_t high_byte, low_byte;
    mos_uint16_t_to_bytes(location, &high_byte, &low_byte);

    uint16_t new_loc = mos_bytes_to_uint16_t(high_byte , low_byte + 1);
    uint8_t offset = mos_cpu_read(cpu, location);   // fetch low-byte from location
    uint8_t page =   mos_cpu_read(cpu, new_loc); // fetch high-byte from location + 1
    uint16_t base_addr = mos_bytes_to_uint16_t(page, offset);
    return base_addr + cpu->regy; // final address
}

// Returns data for read opcodes
uint8_t mos_fetch_operand_data(MOS_Cpu *cpu, MOS_AddressingModes mode, MOS_Operand operand)
{
    switch (mode) {
    case IMME: {
        // Immediate mode just loads a value into an opcode
        switch (operand.type) {
        case OPERAND_DATA: {
            return operand.data.data;
        }
        case OPERAND_ADDRESS:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZP: {
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            // if the operand doesn't contain any page, assumed that it is page zero
            uint16_t location = mos_zero_page(operand.data.address);
            return mos_cpu_read(cpu, location);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZPX: {
        // zero page X modes sums x reg to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MOS_MAX_OFFSET
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            uint16_t new_loc = mos_zero_page_x(cpu, operand.data.address);
            return mos_cpu_read(cpu, new_loc);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABS: {
        // uses the absolute location in operand to load access memory into accumulator
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            return mos_cpu_read(cpu, operand.data.address);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X reg
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            uint16_t index = mos_absolute_x(cpu, operand.data.address);
            return mos_cpu_read(cpu, index);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y reg
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            uint16_t index = mos_absolute_y(cpu, operand.data.address);
            return mos_cpu_read(cpu, index);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDX: {
        // Adds X reg to the offset of the location in the instruction
        // Fetches the data pointed by the modified location
        // Uses that data as a new location to index into memory and fetches low-byte
        // Uses the modified location.offset + 1 to index into memory and fetches high-byte
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            uint16_t final = mos_indirect_x(cpu, operand.data.address);
            return mos_cpu_read(cpu, final);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y reg occurs in the final address
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            uint16_t final_location = mos_indirect_y(cpu, operand.data.address);
            return mos_cpu_read(cpu, final_location);
        }
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
        case OPERAND_ADDRESS:
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

// Returns A MOS_Location for store opcodes
uint16_t mos_fetch_operand_location(MOS_Cpu *cpu, MOS_AddressingModes mode, MOS_Operand operand)
{
    switch (mode) {
    case ZP: {
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            // if the operand doesn't contain any page, assumed that it is page zero
            return operand.data.address;
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ZPX: {
        // zero page X modes sums x reg to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MOS_MAX_OFFSET
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            return mos_zero_page_x(cpu, operand.data.address);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABS: {
        // uses the absolute location in operand to load memory into accumulator
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            return operand.data.address;
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSX: {
        // absolute but += X reg
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            return mos_absolute_x(cpu, operand.data.address);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case ABSY: {
        // absolute but += Y reg
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            return mos_absolute_y(cpu, operand.data.address);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDX: {
        // Adds X reg to the offset of the location in the instruction
        // Fetches the data pointed by the modified location
        // Uses that data as a new location to index into memory and fetches low-byte
        // Uses the modified location.offset + 1 to index into memory and fetches high-byte
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            return mos_indirect_x(cpu, operand.data.address);
        }
        case OPERAND_DATA:
        default: MOS_ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    case INDY: {
        // same as index indirect but the addition of the Y reg occurs in the final address
        switch (operand.type) {
        case OPERAND_ADDRESS: {
            uint16_t location = operand.data.address;
            return mos_indirect_y(cpu, location);
        }
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
void mos_load_reg(MOS_Cpu *cpu, MOS_Instruction instruction, uint8_t *reg_type)
{
    *reg_type = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*reg_type == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*reg_type & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

// M = A | X | Y, store A | X | Y into memory
void mos_store_reg(MOS_Cpu *cpu, MOS_Instruction instruction, uint8_t data)
{
    uint16_t loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    mos_cpu_write(cpu, loc, data);  // Write Accumulator to memory
}

// A = X | Y, transfer X | Y to A, TXA | TYA
void mos_transfer_reg_to_accumulator(MOS_Cpu *cpu, uint8_t data)
{
    cpu->racc = data;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->racc == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (cpu->racc & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

// X | Y = A, transfer Accumulator to X | Y, TAX | TAY
void mos_transfer_accumulator_to_reg(MOS_Cpu *cpu, uint8_t *reg_type)
{
    *reg_type = cpu->racc;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*reg_type == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*reg_type & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_add_with_carry(MOS_Cpu *cpu, MOS_Instruction instruction)
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

void mos_sub_with_carry(MOS_Cpu *cpu, MOS_Instruction instruction)
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

void mos_compare_reg_with_data(MOS_Cpu *cpu, MOS_Instruction instruction, uint8_t reg_type)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint16_t result = reg_type - data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | C_BIT_FLAG | N_BIT_FLAG);
    if (reg_type == data) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (reg_type >= data) mos_set_psr_flags(cpu, C_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_transfer_stack_to_reg(MOS_Cpu *cpu, uint8_t *data)
{
    *data = mos_cpu_read(cpu, mos_bytes_to_uint16_t(MOS_STACK_PAGE, cpu->sp));
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (*data == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (*data & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

// A | SR
void mos_push_reg_to_stack(MOS_Cpu *cpu, uint8_t reg_type)
{
    mos_push_stack(cpu, reg_type);
}

// A or SR
void mos_pull_reg_from_stack(MOS_Cpu *cpu, uint8_t *reg_type)
{
    *reg_type = mos_pull_stack(cpu);
}

void mos_logical_and(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc & data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_logical_xor(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc ^ data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_logical_or(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc | data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_bit_test(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = cpu->racc & data;

    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG | V_BIT_FLAG);
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (data & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    if (data & V_BIT_FLAG) mos_set_psr_flags(cpu, V_BIT_FLAG);
}

void mos_branch_flag_clear(MOS_Cpu *cpu, MOS_Instruction instruction, Status_Flags flag)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    if (cpu->psr & flag) {
        cpu->pc += data;
    }
}

void mos_branch_flag_set(MOS_Cpu *cpu, MOS_Instruction instruction, Status_Flags flag)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    if (!(cpu->psr & flag)) {
        cpu->pc += data;
    }
}

void mos_decrement_regx(MOS_Cpu *cpu)
{
    cpu->regx--;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->regx == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (cpu->regx & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_decrement_regy(MOS_Cpu *cpu)
{
    cpu->regy--;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->regy == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (cpu->regy & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_decrement(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint16_t loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    uint8_t data = mos_cpu_read(cpu, loc);
    mos_cpu_write(cpu, loc, --data); // Decrement
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (data == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (data & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_increment_regx(MOS_Cpu *cpu)
{
    cpu->regx++;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->regx == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (cpu->regx & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_increment_regy(MOS_Cpu *cpu)
{
    cpu->regy++;
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->regy == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (cpu->regy & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

void mos_increment(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint16_t loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    uint8_t data = mos_cpu_read(cpu, loc);
    mos_cpu_write(cpu, loc, ++data); // Increment
    mos_clear_psr_flags(cpu, Z_BIT_FLAG | N_BIT_FLAG);
    if (data == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (data & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
}

// asl on accumulator
void mos_arithmetic_shift_left_racc(MOS_Cpu *cpu)
{
    uint8_t result = cpu->racc << 1; // mult
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->racc & N_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 7 (neg bit)
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

// asl on memory
void mos_arithmetic_shift_left_memory(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = data << 1; // mult
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (data & N_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 7 (neg bit)
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    uint16_t loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    mos_cpu_write(cpu, loc, result);
}

void mos_logical_shift_right_racc(MOS_Cpu *cpu)
{
    uint8_t result = cpu->racc >> 1; // div
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->racc & C_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 0
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_logical_shift_right_memory(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = data >> 1; // div
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (data & C_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 0 (carry bit)
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    uint16_t loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    mos_cpu_write(cpu, loc, result);
}

void mos_rotate_left_racc(MOS_Cpu *cpu)
{
    uint8_t result = cpu->racc << 1;
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->racc & N_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 7 (neg bit)
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_rotate_left_memory(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = data << 1;
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (data & N_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 7 (neg bit)
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    uint16_t loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    mos_cpu_write(cpu, loc, result);
}

void mos_rotate_right_racc(MOS_Cpu *cpu)
{
    uint8_t result = cpu->racc >> 1;
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (cpu->racc & C_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 0 (carry bit)
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    cpu->racc = result;
}

void mos_rotate_right_memory(MOS_Cpu *cpu, MOS_Instruction instruction)
{
    uint8_t data = mos_fetch_operand_data(cpu, instruction.mode, instruction.operand);
    uint8_t result = data >> 1;
    mos_clear_psr_flags(cpu, C_BIT_FLAG | Z_BIT_FLAG | N_BIT_FLAG);
    if (data & C_BIT_FLAG) mos_set_psr_flags(cpu, C_BIT_FLAG); // Set to the contents of old bit 0 (carry bit)
    if (result == 0) mos_set_psr_flags(cpu, Z_BIT_FLAG);
    if (result & N_BIT_FLAG) mos_set_psr_flags(cpu, N_BIT_FLAG);
    uint16_t loc = mos_fetch_operand_location(cpu, instruction.mode, instruction.operand);
    mos_cpu_write(cpu, loc, result);
}

void mos_break(MOS_Cpu *cpu)
{
    mos_clear_psr_flags(cpu, B_BIT_FLAG);
    mos_set_psr_flags(cpu, B_BIT_FLAG);
    uint8_t pc_high_byte, pc_low_byte;
    mos_uint16_t_to_bytes(cpu->pc, &pc_high_byte, &pc_low_byte); // Split the Program Counter
    mos_push_stack(cpu, pc_high_byte); // Push higher-byte first
    mos_push_stack(cpu, pc_low_byte); // Push lower-byte second
    mos_push_stack(cpu, cpu->psr); // Push the Process Status reg
    // TODO: Make the Interrupt vector a const variable
    cpu->pc = mos_cpu_read(cpu, mos_bytes_to_uint16_t(MOS_MAX_PAGES, MOS_MAX_OFFSET)); // load the Interrupt Vector into the Program Counter
    printf("Program Interrupted\n");
}

bool mos_decode(MOS_Cpu *cpu, MOS_Instruction instruction)
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

    case INC: mos_increment(cpu, instruction);                        return true;
    case DEC: mos_decrement(cpu, instruction);                        return true;
    case INX: mos_increment_regx(cpu);                                return true;
    case INY: mos_increment_regy(cpu);                                return true;
    case DEX: mos_decrement_regx(cpu);                                return true;
    case DEY: mos_decrement_regy(cpu);                                return true;

    case ASL: {
        if (instruction.mode == ACCU) {
            mos_arithmetic_shift_left_racc(cpu);
            return true;
        } else {
            mos_arithmetic_shift_left_memory(cpu, instruction);
            return true;
        }
    }
    case LSR: {
        if (instruction.mode == ACCU) {
            mos_logical_shift_right_racc(cpu);
            return true;
        } else {
            mos_logical_shift_right_memory(cpu, instruction);
            return true;
        }
    }
    case ROL: {
        if (instruction.mode == ACCU) {
            mos_rotate_left_racc(cpu);
            return true;
        } else {
            mos_rotate_left_memory(cpu, instruction);
            return true;
        }
    }
    case ROR: {
        if (instruction.mode == ACCU) {
            mos_rotate_right_racc(cpu);
            return true;
        } else {
            mos_rotate_right_memory(cpu, instruction);
            return true;
        }
    }

    case BRK: mos_break(cpu);                                         return true;

    case NOP: {
        // Nothing
        return true;
    };

    case RTI: MOS_UNIMPLEMENTED("RTI");
    case RTS: MOS_UNIMPLEMENTED("RTS");
    case JMP: MOS_UNIMPLEMENTED("JMP");
    case JSR: MOS_UNIMPLEMENTED("JSR");

    case ERROR_FETCH_DATA:
    case ERROR_FETCH_LOCATION:
    default:
        MOS_UNREACHABLE("decode");
        return false;
    }
}

const char *mos_addr_mode_as_cstr(MOS_AddressingModes mode)
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

const char *mos_opcode_as_cstr(MOS_Opcode opcode)
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

const char *mos_operand_type_as_cstr(MOS_OperandType type)
{
    switch (type) {
    case OPERAND_ADDRESS:  return "OPERAND_ADDRESS";
    case OPERAND_DATA:     return "OPERAND_DATA";
    default:               return NULL;
    }
}

MOS_OpcodeInfo opcode_matrix[UINT8_MAX + 1] = {
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
