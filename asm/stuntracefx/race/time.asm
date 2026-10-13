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

; @routine 03:EF3F RaceTimeInit
; WHAT IT DOES: Sets the four bytes at $0E68 to $99 and starts the BCD race
; time at $0E70 (word) by the mode in $0D2B: mode 0 takes the course's entry
; $0D89 of the table at $03:D813 (CourseIndex12), or with $0D4C set byte
; $0D4A of the table at $03:D810; mode 1 (unless $0D4E is set) takes entry
; ($0E3D - $0C) * 4 + $0D89 of the table at $03:D8A3; otherwise 0.
; HOW: 8-bit throughout; the high byte is cleared with STZ. The $0D4C path is
; not reached in the traces so far.
; Entry: any widths, D = $0000. Exit: A low byte = the value, N, Z from it; X
; as the path left it (8-bit); X and Y high bytes cleared; M 8-bit, X 8-bit;
; C, V as the path left them.
%at($03EF3F)
RaceTimeInit:
    sep.b #$30
    lda.b #$99
    sta.w !race_time_nines
    sta.w !race_time_nines+1
    sta.w !race_time_nines+2
    sta.w !race_time_nines+3
    lda.w !hud_mode
    beq .mode0
    cmp.b #$02
    bcs .zero
    lda.w !hud_blank
    bne .zero
    lda.w !course
    sec
    sbc.b #$0C
    asl a
    asl a
    clc
    adc.w !wram_0D89
    tax
    lda.l !mode1_values_d8a3,x
    bra .store
.mode0:
    lda.w !digit62_override
    bne .by_0d4a
    lda.w !wram_0D89
    jsr.w CourseIndex12
    lda.l !course_values_d813,x
    bra .store
.by_0d4a:
    ldx.w !wram_0D4A
    lda.l !course_values_d810,x
    bra .store
.zero:
    lda.b #$00
.store:
    sta.w !race_time_bcd
    stz.w !race_time_bcd+1
    rts
