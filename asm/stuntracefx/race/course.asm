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

; @routine 02:EC10 RaceFadeStageDispatch
; WHAT IT DOES: Runs the handler for stage $0D48 from the table at
; $02:EC18 ($ECB8, $ECD9, $EC30, $EC73).
; HOW: ASL, TAX, JMP (table,X) with 8-bit registers; the handler returns
; to the caller.
; Entry: M 8-bit, X 8-bit. Exit: as the handler leaves it.
%at($02EC10)
RaceFadeStageDispatch:
    lda.w !wram_0D48
    asl a
    tax
    jmp.w (!race_fade_stage_handlers,x)

