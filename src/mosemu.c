#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "./mos.h"

MOS_Instruction mos_fetch_instruction(MOS_Cpu *cpu)
{
    MOS_Instruction inst = {0};
    uint8_t data = mos_cpu_read(cpu, cpu->pc);
    cpu->pc++;

    MOS_OpcodeInfo info = opcode_matrix[data];
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
        cpu->pc++;
        uint16_t abs = mos_bytes_to_uint16_t(page, offset);
        inst.operand.data.address = abs;
        inst.operand.type = OPERAND_ADDRESS;
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
        0x18, // CLC
        0xA9, // LDA
        0x00,
        0x6D, // ADC
        0x00,
        0x00,
        0x6D, // ADC
        0x01,
        0x00,
        0x0A, // ASL, A
        0x8D, // STA
        0x02,
        0x00,
        0x00, // BRK
    };

    MOS_Cpu cpu = mos_cpu_init();
    uint8_t system_ram[0xFFFF] = {0};

    MOS_MMap ram = {
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
    for (uint32_t i = 0; i < MOS_ARRAY_LEN(instructions); ++i) {
        mos_cpu_write(&cpu, addr + i, instructions[i]);
        printf("Inst: 0x%02X\n", instructions[i]);
    }
    cpu.pc = addr;
    printf("PC: 0x%02X\n", cpu.pc);
    while (1) {
        MOS_Instruction inst = mos_fetch_instruction(&cpu);
        if (!mos_decode(&cpu, inst)) return 1;
        if (inst.opcode == BRK) break;
    }

    uint8_t dat = mos_cpu_read(&cpu, mos_bytes_to_uint16_t(0x00, 0x02));
    printf("Result: 0X%X\n", dat);

    uint16_t pc = cpu.pc;
    printf("PC: 0x%02X\n", pc);

    array_delete(&cpu.entries);
    return 0;
}
