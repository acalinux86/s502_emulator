#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "./6502emulator.h"
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

// NOTE: The Table Below shows the relative address of each page
Address address[MAX_CAPACITY_OF_EACH_PAGE] = {
    0X00, 0X01, 0X02, 0X03, 0X04, 0X05, 0X06, 0X07, 0X08, 0X09, 0X0A, 0X0B, 0X0C, 0X0D, 0X0E, 0X0F,
    0X10, 0X11, 0X12, 0X13, 0X14, 0X15, 0X16, 0X17, 0X18, 0X19, 0X1A, 0X1B, 0X1C, 0X1D, 0X1E, 0X1F,
    0X20, 0X21, 0X22, 0X23, 0X24, 0X25, 0X26, 0X27, 0X28, 0X29, 0X2A, 0X2B, 0X2C, 0X2D, 0X2E, 0X2F,
    0X30, 0X31, 0X32, 0X33, 0X34, 0X35, 0X36, 0X37, 0X38, 0X39, 0X3A, 0X3B, 0X3C, 0X3D, 0X3E, 0X3F,
    0X40, 0X41, 0X42, 0X43, 0X44, 0X45, 0X46, 0X47, 0X48, 0X49, 0X4A, 0X4B, 0X4C, 0X4D, 0X4E, 0X4F,
    0X50, 0X51, 0X52, 0X53, 0X54, 0X55, 0X56, 0X57, 0X58, 0X59, 0X5A, 0X5B, 0X5C, 0X5D, 0X5E, 0X5F,
    0X60, 0X61, 0X62, 0X63, 0X64, 0X65, 0X66, 0X67, 0X68, 0X69, 0X6A, 0X6B, 0X6C, 0X6D, 0X6E, 0X6F,
    0X70, 0X71, 0X72, 0X73, 0X74, 0X75, 0X76, 0X77, 0X78, 0X79, 0X7A, 0X7B, 0X7C, 0X7D, 0X7E, 0X7F,
    0X80, 0X81, 0X82, 0X83, 0X84, 0X85, 0X86, 0X87, 0X88, 0X89, 0X8A, 0X8B, 0X8C, 0X8D, 0X8E, 0X8F,
    0X90, 0X91, 0X92, 0X93, 0X94, 0X95, 0X96, 0X97, 0X98, 0X99, 0X9A, 0X9B, 0X9C, 0X9D, 0X9E, 0X9F,
    0XA0, 0XA1, 0XA2, 0XA3, 0XA4, 0XA5, 0XA6, 0XA7, 0XA8, 0XA9, 0XAA, 0XAB, 0XAC, 0XAD, 0XAE, 0XAF,
    0XB0, 0XB1, 0XB2, 0XB3, 0XB4, 0XB5, 0XB6, 0XB7, 0XB8, 0XB9, 0XBA, 0XBB, 0XBC, 0XBD, 0XBE, 0XBF,
    0XC0, 0XC1, 0XC2, 0XC3, 0XC4, 0XC5, 0XC6, 0XC7, 0XC8, 0XC9, 0XCA, 0XCB, 0XCC, 0XCD, 0XCE, 0XCF,
    0XD0, 0XD1, 0XD2, 0XD3, 0XD4, 0XD5, 0XD6, 0XD7, 0XD8, 0XD9, 0XDA, 0XDB, 0XDC, 0XDD, 0XDE, 0XDF,
    0XE0, 0XE1, 0XE2, 0XE3, 0XE4, 0XE5, 0XE6, 0XE7, 0XE8, 0XE9, 0XEA, 0XEB, 0XEC, 0XED, 0XEE, 0XEF,
    0XF0, 0XF1, 0XF2, 0XF3, 0XF4, 0XF5, 0XF6, 0XF7, 0XF8, 0XF9, 0XFA, 0XFB, 0XFC, 0XFD, 0XFE, 0XFF,
};

// Global Variables

Memory memory[NUMBER_OF_PAGES][MAX_CAPACITY_OF_EACH_PAGE] = {0X00}; // Memory Layout
Zero_Page *zero_page = memory[0x00]; // Page 0 (Zero Page)
u8 stack_size = 0x00; // We wanna Track the stack Size
Stack *stack_pointer = memory[0x01]; // Page 1

// Registers
Accumulator accumulator = 0x00;
Register_X X = 0x00;
Register_Y Y = 0x00;
PC program_counter = 0x00;
PSR processor_status_register = 0x00;

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
    stack_pointer[stack_size] = value;
    stack_size++;
}

u8 s502_pull_stack()
{
    assert(stack_size > 0 && "Stack Underflow");
    stack_size--;
    return stack_pointer[stack_size];
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

u8 read_memory(u8 operand)
{
    (void) operand;
    UNIMPLEMENTED("read_memory");
}

// NOTE: Implement A Simple Zero Page Operation
Accumulator s502_load_accumulator(u8 operand)
{
    if (accumulator == 0) s502_set_psr_flags(Z_BIT_FLAG);
    accumulator = read_memory(operand);
    if ((accumulator << 7) != 0) s502_set_psr_flags(N_BIT_FLAG);
    return accumulator;
}

int main(void)
{
    s502_print_stats();
    accumulator = s502_load_accumulator(0x10);
    s502_print_stats();
    return 0;
}

// TODO: Add Read/Write Access to Memory
// TODO: Simple 6502 Assembler Program
