#ifndef EMULATOR_6502_H_
#define EMULATOR_6502_H_

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;

typedef int8_t  i8;
typedef int16_t i16;

#define UNIMPLEMENTED(message)                                  \
    do {                                                        \
        fprintf(stderr, "%s not implement yet!!!\n", message) ; \
        exit(0);                                                \
    } while (0)
#endif // EMULATOR_6502_H_
