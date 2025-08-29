#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "./s502.h"

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
    u8 data = s502_cpu_read(cpu, cpu->program_counter);
    cpu->program_counter++;

    Opcode_Info info = opcode_matrix[data];
    inst.opcode = info.opcode;
    inst.mode   = info.mode;
    switch (inst.mode) {
    case IMPL:
    case ACCU:
        break;
    case IMME: {
        inst.operand.data.data = s502_cpu_read(cpu, cpu->program_counter);
        inst.operand.type = OPERAND_DATA;
        cpu->program_counter++;
    } break;
    case ZP:
    case ZPX:
    case ZPY:
        break;
    case REL: {
        inst.operand.data.data = s502_cpu_read(cpu, cpu->program_counter);
        inst.operand.type = OPERAND_DATA;
        cpu->program_counter++;
    } break;
    case ABS: {
        u8 page = s502_cpu_read(cpu, cpu->program_counter);
        cpu->program_counter++;
        u8 offset = s502_cpu_read(cpu, cpu->program_counter);
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
        0x6D, 0x00, 0x00,
        0x6D, 0x01, 0x00,
        0x8D, 0x02, 0x00,
        0x0,
    };

    CPU cpu = s502_cpu_init();
    u8 system_ram[2048] = {0};
    u8 system_rom[1024*32] = {0};

    MMap_Entry ram = {
        .device = system_ram,
        .read = s502_read_memory,
        .write = s502_write_memory,
        .readonly = false,
        .start_addr = bytes_to_u16(ZERO_PAGE, ZERO_PAGE),
        .end_addr = bytes_to_u16(STACK_PAGE, UINT8_MAX),
    };

    MMap_Entry rom = {
        .read = s502_read_memory,
        .device = system_rom,
        .readonly = true,
        .start_addr = 0x8000,
        .end_addr = 0xFFFF,
    };

    array_append(&cpu.entries, ram);
    array_append(&cpu.entries, rom);
    s502_cpu_write(&cpu, bytes_to_u16(0x00, 0x00), 0xA);
    s502_cpu_write(&cpu, bytes_to_u16(0x00, 0x01), 0xA);

    u16 addr = bytes_to_u16(0x01, 0xB);
    for (u32 i = 0; i < ARRAY_LEN(instructions); ++i) {
        s502_cpu_write(&cpu, addr + i, instructions[i]);
    }
    cpu.program_counter = bytes_to_u16(0x01, 0xB);

    while (1) {
        Instruction inst = fetch_instruction(&cpu);
        if (!s502_decode(&cpu, inst)) return 1;
        if (inst.opcode == BRK) break;
    }

    return 0;
}
