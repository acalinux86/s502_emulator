#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./6502emulator.h"

// Global Variables
Memory memory[MAX_PAGES][MAX_OFFSET] = {0X00}; // Memory Layout
u8 stack_size = 0x00; // We wanna Track the stack Size
Stack *stack = memory[0x01]; // Page 1

// Registers
u8 accumulator = 0x00;
Register_X X = 0x00;
Register_Y Y = 0x00;
PC program_counter = 0x00;
PSR processor_status_register = 0x00;

const char *s502_addr_mode_as_cstr(Addressing_Modes mode);
const char *s502_opcode_as_cstr(Opcode opcode);
const char *s502_operand_type_as_cstr(Operand_Type type);

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof(xs[0]))

#define UNIMPLEMENTED(message)                                         \
    do {                                                               \
        fprintf(stderr, "ERROR: %s not implement yet!!!\n", message) ; \
        exit(1);                                                       \
    } while (0)

#define UNREACHABLE(message)                                        \
    do {                                                            \
        fprintf(stderr, "ERROR: `%s` unreachable!!!!\n", message);  \
        exit(1);                                                    \
    } while (0)

#define ILLEGAL_ADDRESSING(mode, opcode)                    \
    do {                                                    \
        fprintf(stderr, "ERROR: Invalid `%s` mode on %s\n", \
                s502_addr_mode_as_cstr(mode),               \
                s502_opcode_as_cstr(opcode));               \
        exit(1);                                            \
    } while (0)

#define ILLEGAL_ACCESS(mode, operand)                                   \
    do {                                                                \
        assert(s502_operand_type_as_cstr(operand));                     \
        fprintf(stderr, "ERROR: Invalid `%s` type on `%s` mode. \n",    \
                s502_operand_type_as_cstr(operand),                     \
                s502_addr_mode_as_cstr(mode));                          \
    } while (0)

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
    case RTS:                  return "RTS";
    case ADC:                  return "ADC";
    case LDA:                  return "LDA";
    case STA:                  return "STA";
    case CLC:                  return "CLC";
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

void load_accumulator(u8 data)
{
    accumulator = data;
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

Location u16_to_loc(u16 sixteen_bit)
{
    // -- little-endian
    return (Location) {
        .page = sixteen_bit >> 8, // higher byte
        .offset = sixteen_bit & 0xFF, // low-byte
    };
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
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

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
        default: ILLEGAL_ACCESS(mode, operand.type);
        }
    } break;

    default: ILLEGAL_ADDRESSING(mode, ERROR_FETCH_LOCATION);
    }
    UNREACHABLE("s502_fetch_operand_location");
}

// M = A, memory into Accumulator
void s502_load_accumulator(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    load_accumulator(data);
}

// A = M, store accumulator into memory
void s502_store_accumulator(Instruction instruction)
{
    Location loc = s502_fetch_operand_location(instruction.mode, instruction.operand);
    s502_write_memory(loc, accumulator);
}

void add_with_carry(u8 data)
{
    u16 raw = data + accumulator + (processor_status_register & C_BIT_FLAG);
    u8 result = (u8) raw;
    if (result == 0) {
        s502_set_psr_flags(Z_BIT_FLAG);
    } else {
        s502_clear_psr_flags(Z_BIT_FLAG);
    }

    if (raw > MAX_U8) {
        s502_set_psr_flags(V_BIT_FLAG);
    } else {
        s502_clear_psr_flags(V_BIT_FLAG);
    }

    if (result & N_BIT_FLAG) {
        s502_set_psr_flags(N_BIT_FLAG);
    } else {
        s502_clear_psr_flags(N_BIT_FLAG);
    }

    accumulator = result;
}

void s502_add_with_carry(Instruction instruction)
{
    u8 data = s502_fetch_operand_data(instruction.mode, instruction.operand);
    add_with_carry(data);
}

void s502_break()
{
    s502_set_psr_flags(B_BIT_FLAG);
    program_counter = 0xFFFF;
    printf("Program Interrupted\n");
}

bool s502_decode(Instruction instruction)
{
    switch (instruction.opcode) {
    case CLC: s502_clear_psr_flags(C_BIT_FLAG);    return true;
    case LDA: s502_load_accumulator(instruction);  return true;
    case ADC: s502_add_with_carry(instruction);    return true;
    case STA: s502_store_accumulator(instruction); return true;
    default:                                       return false;
    }
}

int main(void)
{
    s502_write_memory(u16_to_loc(0x1234), 0xA);
    s502_write_memory(u16_to_loc(0x1235), 0xB);

    Instruction instruction1 = {
        .mode = IMPL,
        .opcode = CLC,
    };

    Instruction instruction2 = {
        .mode = IMME,
        .opcode = LDA,
        .operand = {
            .type = OPERAND_DATA,
            .data = {
                .data = 0x00,
            }
        }
    };

    Instruction instruction3 = {
        .mode = ABS,
        .opcode = ADC,
        .operand = {
            .type = OPERAND_ABSOLUTE,
            .data = {
                .address = {
                    .absolute = 0x1234
                },
            }
        }
    };


    Instruction instruction4 = {
        .mode = ABS,
        .opcode = ADC,
        .operand = {
            .type = OPERAND_ABSOLUTE,
            .data = {
                .address = {
                    .absolute = 0x1235
                },
            }
        }
    };

    Instruction instruction5 = {
        .mode = ZP,
        .opcode = STA,
        .operand = {
            .type = OPERAND_LOCATION,
            .data = {
                .address = {
                    .loc = {.page = 0x00, .offset = 0xFF},
                },
            }
        }
    };

    Instruction instructions[] = {
        instruction1,
        instruction2,
        instruction3,
        instruction4,
        instruction5
    };

    u8 n = 0;

    while (n < ARRAY_LEN(instructions)) {
        if (!s502_decode(instructions[n])) {
            fprintf(stderr, "Failed to decode\n");
            return 1;
        }
        n++;
    }

    s502_print_stats();
    s502_dump_page(memory[0x00]);
    return 0;
}
