; scale.asm: fixed scalings of signed 16-bit values. C: src/stuntracefx/math/scale.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 03:CC5A ScaleThreeQuarters
; WHAT IT DOES: Returns three quarters of a signed 16-bit value.
; HOW: CMP #$8000 / ROR A is an arithmetic shift right (the sign bit comes
; back in through carry): halve, keep the half in $3E, halve again, add.
; Entry: M 16-bit, D = $0000; A = value.
; Exit: A = v/2 + v/4; $3E = v/2; C and V from the add; N and Z from A.
%at($03CC5A)
ScaleThreeQuarters:
    cmp.w #$8000
    ror a
    sta.b !scale_scratch
    cmp.w #$8000
    ror a
    clc
    adc.b !scale_scratch
    rtl
