; lap.asm: lap splits. The race clock ($0E63 hundredths, $0E64 seconds, $0E66
; minutes, all binary-coded decimal) is marked at each lap in $0EDB-$0EDD;
; a lap's time is the clock minus the mark.
; C: src/stuntracefx/race/lap.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:D5C7 LapSplit
; WHAT IT DOES: Records a lap: the clock minus the last mark goes into the
; three bytes at $0EDE + X (hundredths, seconds, minutes), the clock itself
; into $0EF0 + X, the mark moves to the clock, and the clock's digits are
; spread one per byte into $0F69 (minutes) and $0F6A-$0F6D (seconds tens and
; units, hundredths tens and units).
; HOW: Decimal-mode subtraction byte by byte; a seconds borrow adds 60 and
; takes a minute from the copy of the minutes in direct page $F1.
; Entry: M 8-bit, X 8-bit, D = $0000; X = the lap's record offset. Exit: A
; = the minutes; N, Z from them; C = bit 3 of the seconds; V from the
; minutes subtraction; D clear; X, Y unchanged.
%at($07D5C7)
LapSplit:
    sed
    lda.w !race_time_minutes
    sta.b !lap_minutes
    lda.w !race_time_fraction
    sec
    sbc.w !lap_mark
    sta.w !lap_splits,x
    lda.w !race_time_seconds
    sbc.w !lap_mark+1
    bcs .seconds
    clc
    adc.b #$60
    dec.b !lap_minutes
.seconds:
    sta.w !lap_splits+1,x
    lda.b !lap_minutes
    sec
    sbc.w !lap_mark+2
    sta.w !lap_splits+2,x
    lda.w !race_time_fraction
    sta.w !lap_mark
    sta.w !lap_times,x
    pha
    and.b #$0F
    sta.w !lap_digits+4
    pla
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_digits+3
    lda.w !race_time_seconds
    sta.w !lap_mark+1
    sta.w !lap_times+1,x
    pha
    and.b #$0F
    sta.w !lap_digits+2
    pla
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !lap_digits+1
    lda.w !race_time_minutes
    sta.w !lap_mark+2
    sta.w !lap_times+2,x
    sta.w !lap_digits
    cld
    rts
