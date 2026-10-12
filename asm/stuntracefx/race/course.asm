; course.asm: the course number $0E3D, from the menu's selections.
; C: src/stuntracefx/race/course.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:EB4D CourseFromSelectionLong
; WHAT IT DOES: CourseFromSelection for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as CourseFromSelection. Exit: as CourseFromSelection.
%at($02EB4D)
CourseFromSelectionLong:
    jsr.w CourseFromSelection
    rtl

; @routine 02:EB51 CourseFromSelection
; WHAT IT DOES: Sets the course number $0E3D: !wram_0D4A * 4 + $0D48, or $0D4A
; + $14 when $0D4C is set (not reached in the traces so far).
; HOW: 8-bit arithmetic through the data bank.
; Entry: M 8-bit. Exit: A low byte = the course; N, Z, C, V from the add.
%at($02EB51)
CourseFromSelection:
    lda.w !digit62_override
    beq .lEB60
    lda.w !wram_0D4A
    clc
    adc.b #$14
    sta.w !course
    rts
.lEB60:
    lda.w !wram_0D4A
    asl a
    asl a
    clc
    adc.w !wram_0D48
    sta.w !course
    rts
