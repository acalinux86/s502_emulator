#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "./s502.h"
#include "./array.h"

// DEBUG func
void opcode_info_as_cstr(Opcode_Info info)
{
    const char *opcode = s502_opcode_as_cstr(info.opcode);
    const char *mode   = s502_addr_mode_as_cstr(info.mode);
    printf("Opcode: %s, Mode: %s\n", opcode, mode);
}

Instruction fetch_instruction(CPU *cpu)
{
    Instruction inst = {0};
    Location loc = u16_to_loc(cpu->program_counter);
    u8 data = s502_read_memory(cpu, loc);
    cpu->program_counter++;

    Opcode_Info info = opcode_matrix[data];
    inst.opcode = info.opcode;
    inst.mode   = info.mode;
    switch (inst.mode) {
    case IMPL:
    case ACCU:
        break;
    case IMME: {
        Location locs = u16_to_loc(cpu->program_counter);
        inst.operand.data.data = s502_read_memory(cpu, locs);
        inst.operand.type = OPERAND_DATA;
        cpu->program_counter++;
    } break;
    case ZP:
    case ZPX:
    case ZPY:
        break;
    case REL: {
        Location loc = u16_to_loc(cpu->program_counter);
        inst.operand.data.data = s502_read_memory(cpu, loc);
        inst.operand.type = OPERAND_DATA;
        cpu->program_counter++;
    } break;
    case ABS: {
        Location locs = {0};
        locs = u16_to_loc(cpu->program_counter);
        u8 page = s502_read_memory(cpu, locs);
        cpu->program_counter++;
        locs = u16_to_loc(cpu->program_counter);
        u8 offset = s502_read_memory(cpu, locs);
        u16 abs = bytes_to_u16(offset, page);
        inst.operand.data.address.absolute = abs;
        inst.operand.type = OPERAND_ABSOLUTE;
        cpu->program_counter++;
    } break;
    case ABSX:
    case ABSY:
    case IND:
    case INDX:
    case INDY:
        break;
    }
    return inst;
}

int main(void)
{
    u8 instructions[] = {
        //Op, Mode, Addr
        0x18,
        0xA9, 0x00,
        0x6D, 0x30, 0x60,
        0x6D, 0x31, 0x60,
        0x8D, 0x32, 0x60,
        0x0,
    };

    CPU cpu = s502_cpu_init();
    cpu.memory[0x60][0x30] = 0xA;
    cpu.memory[0x60][0x31] = 0xA;

    u16 addr = 0x8080;
    for (u32 i = 0; i < ARRAY_LEN(instructions); ++i) {
        Location loc = u16_to_loc(addr + i);
        s502_write_memory(&cpu, loc, instructions[i]);
    }
    cpu.program_counter = 0x8080;

    while (1) {
        Instruction inst = fetch_instruction(&cpu);
        if (!s502_decode(&cpu, inst)) return 1;
        if (inst.opcode == BRK) break;
    }

    return 0;
}
