.org $8080

start:
    clc
    lda   #$00                  ; LOAD 0 into Accumulator
    adc   $0360
    adc   $0361
    sta   $0362
    brk
