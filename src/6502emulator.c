#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "./6502emulator.h"

// Global Variables
Memory memory[NUMBER_OF_PAGES][MAX_CAPACITY_OF_EACH_PAGE] = {0X00}; // Memory Layout
Zero_Page *zero_page = memory[0x00]; // Page 0 (Zero Page)
u8 stack_size = 0x00; // We wanna Track the stack Size
Stack *stack = memory[0x01]; // Page 1

// Registers
u8 accumulator = 0x00;
Register_X X = 0x00;
Register_Y Y = 0x00;
PC program_counter = 0x00;
PSR processor_status_register = 0x00;

#define UNIMPLEMENTED(message)                                         \
    do {                                                               \
        fprintf(stderr, "ERROR: %s not implement yet!!!\n", message) ; \
        exit(1);                                                       \
    } while (0)

const char *s502_opcode_as_cstr(Opcode opcode);
#define ILLEGAL_ADDRESSING(mode, opcode)                            \
    do {                                                            \
        fprintf(stderr, "ERROR: Invalid `mode` mode on %s\n",       \
                s502_opcode_as_cstr(opcode));                       \
        exit(1);                                                    \
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
    assert(stack_size < MAX_CAPACITY_OF_EACH_PAGE - 1 && "Stack Overflow");
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
    for (u8 i = 0; i < NUMBER_OF_PAGES - 1; ++i) {
        s502_dump_page(memory[i]);
    }
}

// NOTE: Print Stats
void s502_print_stats()
{
    printf("Stack       : %u\n", stack[stack_size]);
    printf("Register_X  : %u\n", X);
    printf("Register_Y  : %u\n", Y);
    printf("Accumulator : %u\n", accumulator);
    printf("PC          : %u\n", program_counter);
    printf("PSR         : %u\n", processor_status_register);
}

// NOTE: Set PSR FLAGS
void s502_set_psr_flags(PSR_Flags flags)
{
    processor_status_register |= flags;
}

u8 s502_read_memory(Location location)
{
    return memory[location.page][location.offset];
}

void s502_write_memory(Location location, Data data)
{
    memory[location.page][location.offset] = data;
}

const char *s502_addr_mode_as_cstr(Addressing_Modes mode)
{
    switch (mode) {
    case IMPLICIT:         return "IMPLICIT";
    case ACCUMULATOR:      return "ACCUMULATOR";
    case IMMEDIATE:        return "IMMEDIATE";
    case ZERO_PAGE:        return "ZERO_PAGE";
    case ZERO_PAGE_X:      return "ZERO_PAGE_X";
    case ZERO_PAGE_Y:      return "ZERO_PAGE_Y";
    case RELATIVE:         return "RELATIVE";
    case ABSOLUTE:         return "ABSOLUTE";
    case ABSOLUTE_X:       return "ABSOLUTE_X";
    case ABSOLUTE_Y:       return "ABSOLUTE_Y";
    case INDIRECT:         return "INDIRECT";
    case INDEXED_INDIRECT: return "INDEXED_INDIRECT";
    case INDIRECT_INDEXED: return "INDIRECT_INDEXED";
    default:
        return NULL;
    }
}

const char *s502_opcode_as_cstr(Opcode opcode)
{
    switch (opcode) {
    case BRK: return "BRK";
    case RTS: return "RTS";
    case ADC: return "ADC";
    case LDA: return "LDA";
    case STA: return "STA";
    case CLC: return "CLC";
    default:
        return NULL;
    }
}

void s502_clear_carry_bit()
{
    s502_set_psr_flags(C_BIT_FLAG & 0);
}

void s502_store_accumulator(Operand operand)
{
    s502_write_memory(operand.location, accumulator);
    accumulator = 0x00;// clear the accumulator
}

void s502_add_with_carry(Operand operand)
{
    u8 value = s502_read_memory(operand);
    accumulator += value;
    if (value >= 255) s502_set_psr_flags(C_BIT_FLAG);
}

void s502_set_accumulator(u8 data)
{
    accumulator = data;
    if (accumulator == 0)        s502_set_psr_flags(Z_BIT_FLAG);
    if ((accumulator << 7) != 0) s502_set_psr_flags(N_BIT_FLAG);
    program_counter++;
    return accumulator;
}


// TODO: Find a Way to split a 16-bit data into page and offset
// TODO: Refactor LDA to switch-case on operand types
void s502_load_accumulator(Instruction instruction)
{
    UNIMPLEMENTED("s502_load_accumulator");
    switch (instruction.mode) {
    case IMMEDIATE: {
        // Immediate mode just loads a value into the accumulator
        s502_set_accumulator(instruction.operand.data);
    } break;

    case ZERO_PAGE: {
        // if the operand doesn't contain any page, assumed that it is page zero
        assert(instruction.operand.location.page == 0x00 && "Invalid Page Zero Address");
        u8 data = s502_read_memory(instruction.operand.location);
        s502_set_accumulator(data);
    } break;

    case ZERO_PAGE_X: {
        // zero page X modes sums x register to the zero page operand and uses it as index
        // It wraps around incase it exceeds page MAX_CAPACITY_OF_EACH_PAGE
        assert(instruction.operand.location.page == 0x00 && "Invalid Page Zero Address");
        u8 index = instruction.operand.location.offset + X;
        u8 page = instruction.operand.location.page;
        u8 data = s502_read_memory((Location) {.page = page, .offset = index});
        s502_set_accumulator(data);
    } break;

    case ABSOLUTE: {
        // uses the absolute location in operand to load access memory into accumulator
        u8 data = s502_read_memory(instruction.operand.location);
        s502_set_accumulator(data);
    } break;

    case ABSOLUTE_X: {
        // zero page x but absolute
        u8 index = instruction.operand.location.offset + X;
        if (index > 0xFF) instruction.operand.location.page++;
        u8 page = instruction.operand.location.page;
        u8 data = s502_read_memory((Location) {.page = page, .offset = index});
        s502_set_accumulator(data);
    } break;

    case ABSOLUTE_Y: {
        // absolute x but y
        u8 index = instruction.operand.location.offset + Y;
        if (index > 0xFF) instruction.operand.location.page++;
        u8 page = instruction.operand.location.page;
        u8 data = s502_read_memory((Location) {.page = page, .offset = index});
        s502_set_accumulator(data);
    } break;

        case
    default: {
        ILLEGAL_ADDRESSING(LDA);
    }
    }
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
    case LDA:
        s502_load_accumulator(instruction); break;
    default:
        ILLEGAL_ADDRESSING(instruction.mode, instruction.opcode);
    }
}

int main(void)
{
    // populate Memory
    s502_write_memory((Operand){.page = 0x00, .offset = 0x65}, 0x0A);
    s502_write_memory((Operand){.page = 0x00, .offset = 0x66}, 0x0A);
    s502_clear_carry_bit();
    s502_load_accumulator((Operand){.page = 0x00, .offset = 0x00});
    s502_add_with_carry((Operand){.page = 0x00, .offset = 0x65});
    s502_add_with_carry((Operand){.page = 0x00, .offset = 0x66});
    s502_store_accumulator((Operand){.page = 0x00, .offset = 0x67});
    s502_dump_page(zero_page);
    return 0;
}
