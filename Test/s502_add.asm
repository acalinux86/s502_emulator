.org $8080

start:
    clc
    lda   #$10000                  ; LOAD 0 into Accumulator
    ; This is also a commented line
    adc   $0360
    adc   $0361
    sta   $0362
    brk
