#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include "./s502.h"

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
    return 0;
}
