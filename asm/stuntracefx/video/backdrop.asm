; backdrop.asm: the race screen's backdrop (BG2), scrolled with the view.
; Each player's half of the screen ($0D7F selects it, 0 or 1) has its own
; BG2 scroll pair: horizontal at $0D92 + half * 2, vertical at $0D8E +
; half * 2, and $0D96 + half * 2 for the raster code. The view's angle is
; at $06CD and its heading at $06CF.
; C: src/stuntracefx/video/backdrop.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7F:1278 PlayerVehicleRaised
; WHAT IT DOES: Returns 8 when bit 14 is set in the word that player X's
; vehicle record leads to, else 0: the vehicle object is $0346 for X = 0
; and $034A otherwise; its link (+$4F in the object table at $7E:2000) is
; followed twice, and the word read at that offset in bank $00. The value
; 8 is not reached in the traces so far.
; HOW: The compare of X is 8-bit; the rest 16-bit, X pushed and pulled
; back. P is saved and restored.
; Entry: any widths; X = the player (0 or not). Exit: A = 0 or 8; X's low
; byte unchanged, high byte clear; P unchanged.
%ramcode($7F1278, $058A78)
PlayerVehicleRaised:
    php
    sep.b #$10
    cpx.b #$00
    bne .l1287
    rep.b #$30
    phx
    ldx.w !wram_0346
    bra .l128D
.l1287:
    rep.b #$30
    phx
    ldx.w !wram_034A
.l128D:
    lda.l !obj_link,x
    tax
    lda.l !obj_link,x
    tax
    lda.l !bank0_base,x
    and.w #$4000
    beq .l12A6
    lda.w #$0008
    plx
    plp
    rtl
.l12A6:
    lda.w #$0000
    plx
    plp
    rtl

; @routine 7E:E924 BackdropScrollFromView
; WHAT IT DOES: Sets the backdrop scroll of the current half ($0D7F):
; horizontal = the view heading $06CF / 4; with h = the view angle $06CD
; + $18 (+ 8 when PlayerVehicleRaised), vertical = h + the half's base at
; $7E:E920, and $0D96 = h * 2 + $EF99, at most $F257 (the cap is not
; reached in the traces so far).
; HOW: 16-bit arithmetic; the doubling's carry goes into the add of
; $EF99. X is the half * 2; P is saved and restored.
; Entry: X 8-bit, DB a bank where $0D8E is work RAM. Exit: A = the
; vertical scroll; X = the half * 2; P unchanged.
%ramcode($7EE924, $01464B)
BackdropScrollFromView:
    php
    lda.l !gsu_buffer_select
    asl a
    tax
    rep.b #$20
    lda.l !view_06cf
    lsr a
    lsr a
    sta.w !bg2_scroll_x,x
    jsl.l PlayerVehicleRaised
    clc
    adc.l !wram_06CD
    clc
    adc.w #$0018
    pha
    asl a
    adc.w #$EF99
    cmp.w #$F257
    bcc .lE950
    lda.w #$F257
.lE950:
    sta.w !gradient_src_a,x
    pla
    clc
    adc.l !backdrop_y_base,x
    sta.w !bg2_scroll_y,x
    plp
    rts
