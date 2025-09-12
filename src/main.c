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
    uint8_t data = s502_cpu_read(cpu, cpu->pc);
    cpu->pc++;

    Opcode_Info info = opcode_matrix[data];
    inst.opcode = info.opcode;
    inst.mode   = info.mode;
    switch (inst.mode) {
    case IMPL:
    case ACCU:
        break;
    case IMME: {
        inst.operand.data.data = s502_cpu_read(cpu, cpu->pc);
        inst.operand.type = OPERAND_DATA;
        cpu->pc++;
    } break;
    case ZP:
    case ZPX:
    case ZPY:
        break;
    case REL: {
        inst.operand.data.data = s502_cpu_read(cpu, cpu->pc);
        inst.operand.type = OPERAND_DATA;
        cpu->pc++;
    } break;
    case ABS: {
        uint8_t page = s502_cpu_read(cpu, cpu->pc);
        cpu->pc++;
        uint8_t offset = s502_cpu_read(cpu, cpu->pc);
        uint16_t abs = bytes_to_uint16_t(offset, page);
        inst.operand.data.address.absolute = abs;
        inst.operand.type = OPERAND_ABSOLUTE;
        cpu->pc++;
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
    uint8_t instructions[] = {
        //Op, Mode, Addr
        0x18,
        0xA9, 0x00,
        0x6D, 0x00, 0x00,
        0x6D, 0x01, 0x00,
        0x8D, 0x02, 0x00,
        0x0,
    };

    CPU cpu = s502_cpu_init();
    uint8_t system_ram[0xFFFF] = {0};

    MMap_Entry ram = {
        .device = system_ram,
        .read = s502_read_memory,
        .write = s502_write_memory,
        .readonly = false,
        .start_addr = 0X0000,
        .end_addr = 0XFFFF,
    };

    array_append(&cpu.entries, ram);
    s502_cpu_write(&cpu, bytes_to_uint16_t(0x00, 0x00), 0xA);
    s502_cpu_write(&cpu, bytes_to_uint16_t(0x00, 0x01), 0xA);

    uint16_t addr = bytes_to_uint16_t(0x01, 0xB);
    for (uint32_t i = 0; i < ARRAY_LEN(instructions); ++i) {
        s502_cpu_write(&cpu, addr + i, instructions[i]);
    }
    cpu.pc = bytes_to_uint16_t(0x01, 0xB);

    while (1) {
        Instruction inst = fetch_instruction(&cpu);
        if (!s502_decode(&cpu, inst)) return 1;
        if (inst.opcode == BRK) break;
    }
    printf("Result: %u\n", s502_cpu_read(&cpu, bytes_to_uint16_t(0x00, 0x02)));
    return 0;
}
