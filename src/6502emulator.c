#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>

#include "./6502emulator.h"

// Global Variables
Memory memory[NUMBER_OF_PAGES][MAX_CAPACITY_OF_EACH_PAGE] = {0X00}; // Memory Layout
Zero_Page *zero_page = memory[0x00]; // Page 0 (Zero Page)
BYTE stack_size = 0x00; // We wanna Track the stack Size
Stack *stack_pointer = memory[0x01]; // Page 1

// Registers
Accumulator accumulator = 0x00;
Register_X X = 0x00;
Register_Y Y = 0x00;
PC program_counter = 0x00;
PSR processor_status_register = 0x00;

void s502_dump_page(BYTE *page)
{
    BYTE page_width = 16;
    BYTE page_height = 16;
    for (BYTE i = 0; i < page_width; ++i) {
        for (BYTE j = 0; j < page_height; ++j) {
            printf("0X%02X ", page[i * page_height + j]);
        }
        printf("\n");
    }
}

void s502_push_stack(BYTE value)
{
    // NOTE: Stack Operations are limited to only page one (Stack Pointer) of the 6502
    assert(stack_size < MAX_CAPACITY_OF_EACH_PAGE - 1 && "Stack Overflow");
    stack_pointer[stack_size] = value;
    stack_size++;
}

BYTE s502_pull_stack()
{
    assert(stack_size > 0 && "Stack Underflow");
    stack_size--;
    return stack_pointer[stack_size];
}

void s502_dump_memory()
{
    for (BYTE i = 0; i < NUMBER_OF_PAGES - 1; ++i) {
        s502_dump_page(memory[i]);
    }
}

// NOTE: Print Stats
void s502_print_stats()
{
    printf("Stack       : %u\n", stack_pointer[stack_size]);
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

BYTE read_memory(WORD operand)
{
    (void) operand;
    UNIMPLEMENTED("read_memory");
}

// NOTE: Implement A Simple Zero Page Operation
Accumulator s502_load_accumulator(WORD operand)
{
    if (accumulator == 0) s502_set_psr_flags(Z_BIT_FLAG);
    accumulator = read_memory(operand);
    if ((accumulator << 7) != 0) s502_set_psr_flags(N_BIT_FLAG);
    return accumulator;
}

int main(void)
{
    Address a = address[126];
    printf("%02X\n", a);
    return 0;
}

// TODO: Add Read/Write Access to Memory
// TODO: Simple 6502 Assembler Program
