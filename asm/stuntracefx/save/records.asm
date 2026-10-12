; records.asm: five-byte records in the save data (Super FX RAM $70:FC4A and
; $70:FC5E onwards).
; C: src/stuntracefx/save/records.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:ADCC SaveRecordSumA
; WHAT IT DOES: Returns the 8-bit sum of the five bytes at $70:FC4A + X.
; HOW: Five additions, each with carry cleared first.
; Entry: M 8-bit, X 8-bit; X = offset. Exit: A low byte = the sum; X + 5;
; Y = 0; Z set, N clear; C, V from the last add.
%at($07ADCC)
SaveRecordSumA:
    ldy.b #$05
    lda.b #$00
.lADD0:
    clc
    adc.l !save_record_a,x
    inx
    dey
    bne .lADD0
    rts

; @routine 07:ADF9 SaveRecordSumB
; WHAT IT DOES: Returns the 8-bit sum of the five bytes at $70:FC5E + X.
; HOW: As SaveRecordSumA.
; Entry: as SaveRecordSumA. Exit: as SaveRecordSumA.
%at($07ADF9)
SaveRecordSumB:
    ldy.b #$05
    lda.b #$00
.lADFD:
    clc
    adc.l !save_record_b,x
    inx
    dey
    bne .lADFD
    rts
