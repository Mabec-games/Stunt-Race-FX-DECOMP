; checksum.asm: the checksum of the battery-backed save data in Super FX RAM.
; C: src/stuntracefx/save/checksum.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:F02B SaveChecksum
; WHAT IT DOES: Computes the save data's checksum and stores it twice
; ($70:FC14 and $70:FC15).
; HOW: An 8-bit sum with carry of the nine bytes $70:FC06-$70:FC0E, added in
; the order FC0B, FC06, FC07, FC0C, FC0D, FC0E, FC08, FC09, FC0A, each add
; taking the previous carry.
; Entry: any widths. Exit: A low byte = the checksum; P unchanged.
%at($03F02B)
SaveChecksum:
    php
    sep.b #$20
    clc
    lda.l !save_bytes+5
    adc.l !save_bytes
    adc.l !save_bytes+1
    adc.l !save_bytes+6
    adc.l !save_bytes+7
    adc.l !save_bytes+8
    adc.l !save_bytes+2
    adc.l !save_bytes+3
    adc.l !save_bytes+4
    sta.l !save_checksum
    sta.l !save_checksum_copy
    plp
    rtl
