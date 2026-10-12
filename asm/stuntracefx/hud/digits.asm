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
    sta.w !oam_buffer+26,x
    inc a
    sta.w !oam_buffer+30,x
    lda.w !race_time_seconds
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.w !oam_buffer+18,x
    inc a
    sta.w !oam_buffer+22,x
    lda.w !race_time_fraction
    and.b #$0F
    asl a
    sta.w !oam_buffer+10,x
    inc a
    sta.w !oam_buffer+14,x
    lda.w !race_time_fraction
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.w !oam_buffer+2,x
    inc a
    sta.w !oam_buffer+6,x
    lda.w !race_time_minutes
    and.b #$0F
    asl a
    sta.w !oam_buffer+34,x
    inc a
    sta.w !oam_buffer+38,x
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
    sta.w !oam_buffer+362
    inc a
    sta.w !oam_buffer+366
    lda.l !hud_digits_19ec
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    sta.w !oam_buffer+354
    inc a
    sta.w !oam_buffer+358
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
    sta.w !oam_buffer+2,x
    inc a
    sta.w !oam_buffer+6,x
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
    sta.w !oam_buffer+10,x
    inc a
    sta.w !oam_buffer+14,x
    pla
    asl a
    sta.w !oam_buffer+18,x
    inc a
    sta.w !oam_buffer+22,x
    rts

; @routine 7E:E7AE HudDrawValueA
; WHAT IT DOES: Draws the number at $0DDA as three digits at sprite offset
; $114.
; HOW: HudDrawNumber3, through the shared tail at $7E:E7D0.
; Entry: any widths, D = $0000. Exit: as HudDrawNumber3; X = $14, Y high
; byte clear; M 8-bit, X 8-bit.
%ramcode($7EE7AE, $0144D5)
HudDrawValueA:
    sep.b #$20
    lda.w !wram_0DDA
    rep.b #$10
    ldx.w #$0114
    bra HudDrawValueTail

; @routine 7E:E7BA HudDrawValueB
; WHAT IT DOES: Draws the number at $0DDA as three digits at sprite offset
; $28.
; HOW: As HudDrawValueA.
; Entry: as HudDrawValueA. Exit: as HudDrawValueA, X = $28.
%ramcode($7EE7BA, $0144E1)
HudDrawValueB:
    sep.b #$20
    lda.w !wram_0DDA
    rep.b #$10
    ldx.w #$0028
    bra HudDrawValueTail

; @routine 7E:E7C6 HudDrawValueC
; WHAT IT DOES: Draws the number at $0DDB as three digits at sprite offset
; $128.
; HOW: As HudDrawValueA; its last three instructions are the shared tail.
; Entry: as HudDrawValueA. Exit: as HudDrawValueA, X = $28.
%ramcode($7EE7C6, $0144ED)
HudDrawValueC:
    sep.b #$20
    lda.w !wram_0DDB
    rep.b #$10
    ldx.w #$0128
HudDrawValueTail:                   ; shared with HudDrawValueA and HudDrawValueB
    jsr.w HudDrawNumber3
    sep.b #$10
    rts

; @routine 09:C072 HudCountUp19EC
; WHAT IT DOES: Marks object X (bit 0 of +$25), adds one to the BCD count at
; $19EC and queues sound $3C for the current player.
; HOW: An 8-bit decimal-mode add; SoundPort0QueuePush.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = $3C; D clear;
; C, V from the decimal add; N, Z from the data bank.
%at($09C072)
HudCountUp19EC:
    lda.w !obj_25,x
    ora.b #$01
    sta.w !obj_25,x
    sed
    sep.b #$20
    lda.w !hud_digits_19ec
    clc
    adc.b #$01
    sta.w !hud_digits_19ec
    cld
    lda.b #$3C
    jsl.l SoundPort0QueuePush
    rts

; @routine 7E:E7FC HudDigitSprites62
; WHAT IT DOES: Shows the digit in $0DF4 on sprites 62 and 63 (tile 2d on
; top, 2d + 1 below), or the digit 1 when $0D4C is set (no script has
; reached that case yet).
; HOW: Byte stores to the two sprites' tile bytes ($046F, $0473) through
; the data bank.
; Entry: M 8-bit. Exit: A low byte = 2d + 1; N, Z from it; C = bit 7 of d.
%ramcode($7EE7FC, $014523)
HudDigitSprites62:
    lda.w !digit62_override
    beq .lE805
    lda.b #$01                      ; not reached in the traces so far
    bra .lE808
.lE805:
    lda.w !digit62_value
.lE808:
    asl a
    sta.w !oam_buffer+250
    inc a
    sta.w !oam_buffer+254
    rts
