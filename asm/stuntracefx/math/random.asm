; random.asm: the pseudo-random generator in direct page $E6-$E9.
; C: src/stuntracefx/math/random.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:DF7D RandomStep
; WHAT IT DOES: Steps the generator: each of $E7, $E8, $E9 becomes the
; previous result minus it, with the borrow carried along, starting from
; $E6 - $E7 - 1; finally $E6 becomes the last result minus $E6.
; HOW: A chain of 8-bit SBCs from carry clear.
; Entry: any widths, D = $0000. Exit: A low byte = the new $E6; P
; unchanged.
%at($02DF7D)
RandomStep:
    php
    sep.b #$20
    lda.b !random_e6
    clc
    sbc.b !random_e7
    sta.b !random_e7
    sbc.b !random_e8
    sta.b !random_e8
    sbc.b !random_e9
    sta.b !random_e9
    sbc.b !random_e6
    sta.b !random_e6
    plp
    rts
