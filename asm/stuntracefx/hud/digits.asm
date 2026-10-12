; digits.asm: numbers drawn as sprite digits. Each digit is two 8x8 sprite
; tiles stacked: tile 2d on top, 2d+1 below; the routines write only the tile
; bytes of entries already placed in the work-RAM sprite table. Run from work
; RAM. C: src/stuntracefx/hud/digits.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:B00C HudDrawTime
; WHAT IT DOES: Draws the race time with sprite digits, unless the HUD is
; blanked; also counts down the HUD's hold timer.
; HOW: Unless $0D2B is 1, the 16-bit hold timer at $0E5D counts down while it
; is not zero, and the time is drawn whenever it is zero; otherwise only when
; $0D4E is set. Five digits (fraction, seconds, minutes) go to the five
; sprite pairs from the table offset at $0E60.
; Entry: X 8-bit. Exit: P unchanged; A low byte = the last tile written, or
; $0D4E when nothing was drawn.
%ramcode($7EB00C, $010D33)
HudDrawTime:
    php
    sep.b #$20
    rep.b #$10
    lda.w !hud_mode
    cmp.b #$01
    beq .check_blank
    ldx.w !hud_hold
    beq .draw
    dex
    stx.w !hud_hold
.check_blank:
    lda.w !hud_blank
    bne .draw
    plp
    rts
.draw:
    ldx.w !hud_time_sprite
    lda.w !race_time_seconds
    and.b #$0F
    asl a
    sta.w !oam_buffer+$1A,x
    inc a
    sta.w !oam_buffer+$1E,x
    lda.w !race_time_seconds
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.w !oam_buffer+$12,x
    inc a
    sta.w !oam_buffer+$16,x
    lda.w !race_time_fraction
    and.b #$0F
    asl a
    sta.w !oam_buffer+$0A,x
    inc a
    sta.w !oam_buffer+$0E,x
    lda.w !race_time_fraction
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.w !oam_buffer+$02,x
    inc a
    sta.w !oam_buffer+$06,x
    lda.w !race_time_minutes
    and.b #$0F
    asl a
    sta.w !oam_buffer+$22,x
    inc a
    sta.w !oam_buffer+$26,x
    plp
    rts

; @routine 7E:E7D6 HudDrawDigits19EC
; WHAT IT DOES: Draws the two-digit BCD value at $19EC with sprite digits
; (sprite pairs at table offsets $160 and $168), unless the HUD is blanked.
; HOW: Does nothing when $0D4E is set; else ones digit, then tens digit.
; Entry: X 8-bit. Exit: A low byte = the last tile written, or $0D4E; C clear
; after drawing; N, Z from A; M 8-bit.
%ramcode($7EE7D6, $0144FD)
HudDrawDigits19EC:
    sep.b #$20
    lda.w !hud_blank
    bne .done
    lda.l !hud_digits_19ec
    and.b #$0F
    asl a
    sta.w !oam_buffer+$16A
    inc a
    sta.w !oam_buffer+$16E
    lda.l !hud_digits_19ec
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.w !oam_buffer+$162
    inc a
    sta.w !oam_buffer+$166
.done:
    rts

; @routine 7E:E95E HudDrawNumber3
; WHAT IT DOES: Draws an 8-bit number as three sprite digits (hundreds, tens,
; ones) into three sprite pairs starting at table offset X.
; HOW: Counts hundreds by subtracting 100, then tens by subtracting 10, in
; direct page $F1; each digit's tiles go to its pair.
; Entry: M 8-bit, X 16-bit, D = $0000; A = number, X = first sprite offset.
; Exit: A low byte = the ones digit's lower tile; $F1 = the tens; C clear; V
; from the last subtraction; N, Z from A.
%ramcode($7EE95E, $014685)
HudDrawNumber3:
    stz.b !digit_count
.hundreds:
    cmp.b #100
    bcc .hundreds_done
    sbc.b #100
    inc.b !digit_count
    bra .hundreds
.hundreds_done:
    pha
    lda.b !digit_count
    asl a
    sta.w !oam_buffer+$02,x
    inc a
    sta.w !oam_buffer+$06,x
    pla
    stz.b !digit_count
.tens:
    cmp.b #10
    bcc .tens_done
    sbc.b #10
    inc.b !digit_count
    bra .tens
.tens_done:
    pha
    lda.b !digit_count
    asl a
    sta.w !oam_buffer+$0A,x
    inc a
    sta.w !oam_buffer+$0E,x
    pla
    asl a
    sta.w !oam_buffer+$12,x
    inc a
    sta.w !oam_buffer+$16,x
    rts
