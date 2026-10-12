; time.asm: the race time, kept in binary-coded decimal; $0E70 is a 16-bit
; BCD value the course tables add to.
; C: src/stuntracefx/race/time.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:EF9F RaceTimeAddD815
; WHAT IT DOES: Adds to the BCD value at $0E70 the BCD byte of the table at
; $03:D815 for the course and the entry at $0D89 (12 bytes per course,
; three per entry).
; HOW: CourseIndex12, then a 16-bit ADC in decimal mode.
; Entry: M 8-bit, X 8-bit, D = $0000. Exit: A = the new value (M 8-bit); X
; = the index; $F1 = entry * 3; D clear; N, Z, C, V from the decimal add.
%at($03EF9F)
RaceTimeAddD815:
    lda.w !wram_0D89
    jsr.w CourseIndex12
    lda.l !course_values_d815,x
RaceTimeAddTail:                    ; shared with RaceTimeAddD814
    rep.b #$20
    sed
    and.w #$00FF
    clc
    adc.w !race_time_bcd
    sta.w !race_time_bcd
    cld
    sep.b #$20
    rtl

; @routine 03:EF93 RaceTimeAddD814
; WHAT IT DOES: As RaceTimeAddD815, from the table at $03:D814.
; HOW: As RaceTimeAddD815, sharing its tail.
; Entry: as RaceTimeAddD815. Exit: as RaceTimeAddD815.
%at($03EF93)
RaceTimeAddD814:
    lda.w !wram_0D89
    jsr.w CourseIndex12
    lda.l !course_values_d814,x
    bra RaceTimeAddTail
