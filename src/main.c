#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "./mos.h"

Instruction mos_fetch_instruction(CPU *cpu)
{
    Instruction inst = {0};
    uint8_t data = mos_cpu_read(cpu, cpu->pc);
    cpu->pc++;

    Opcode_Info info = opcode_matrix[data];
    inst.opcode = info.opcode;
    inst.mode   = info.mode;
    switch (inst.mode) {
    case IMPL:
    case ACCU:
        break;
    case IMME: {
        inst.operand.data.data = mos_cpu_read(cpu, cpu->pc);
        inst.operand.type = OPERAND_DATA;
        cpu->pc++;
    } break;
    case ZP:
    case ZPX:
    case ZPY:
        break;
    case REL: {
        inst.operand.data.data = mos_cpu_read(cpu, cpu->pc);
        inst.operand.type = OPERAND_DATA;
        cpu->pc++;
    } break;
    case ABS: {
        uint8_t offset = mos_cpu_read(cpu, cpu->pc);
        cpu->pc++;
        uint8_t page = mos_cpu_read(cpu, cpu->pc);
        uint16_t abs = mos_bytes_to_uint16_t(page, offset);
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

typedef ARRAY(uint8_t) U8s;

int main(void)
{
    U8s instructions = {0};

    array_append(&instructions, 0x18); // CLC

    array_append(&instructions, 0xA9); // LDA
    array_append(&instructions, 0x00);

    array_append(&instructions, 0x6D); // ADC
    array_append(&instructions, 0x00);
    array_append(&instructions, 0x00);

    array_append(&instructions, 0x6D); // ADC
    array_append(&instructions, 0x01);
    array_append(&instructions, 0x00);

    array_append(&instructions, 0x8D); // STA
    array_append(&instructions, 0x02);
    array_append(&instructions, 0x00);

    array_append(&instructions, 0x00); // BRK

    CPU cpu = mos_cpu_init();
    uint8_t system_ram[0xFFFF] = {0};

    MMap_Entry ram = {
        .device = system_ram,
        .read = mos_read_memory,
        .write = mos_write_memory,
        .readonly = false,
        .start_addr = 0X0000,
        .end_addr = 0XFFFF,
    };

    array_append(&cpu.entries, ram);
    mos_cpu_write(&cpu, mos_bytes_to_uint16_t(0x00, 0x00), 0xA);
    mos_cpu_write(&cpu, mos_bytes_to_uint16_t(0x00, 0x01), 0xA);

    uint16_t addr = mos_bytes_to_uint16_t(0x01, 0xB);
    for (uint32_t i = 0; i < instructions.count; ++i) {
        mos_cpu_write(&cpu, addr + i, instructions.items[i]);
    }
    cpu.pc = mos_bytes_to_uint16_t(0x01, 0xB);

    while (1) {
        Instruction inst = mos_fetch_instruction(&cpu);
        if (!mos_decode(&cpu, inst)) return 1;
        if (inst.opcode == BRK) break;
    }
    printf("Result: %u\n", mos_cpu_read(&cpu, mos_bytes_to_uint16_t(0x00, 0x02)));
    printf("PC: 0x%x\n", cpu.pc);
    return 0;
}
