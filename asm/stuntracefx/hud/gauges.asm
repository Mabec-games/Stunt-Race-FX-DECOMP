; gauges.asm: the HUD's two bar gauges per panel, each a strip of sprites
; with a marker (SpriteBar4WithMarker, SpriteBar6WithMarker). A panel's
; gauge values and their screen positions sit in work RAM at $0DF9-$0E15.
; C: src/stuntracefx/hud/gauges.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:EE46 HudGaugesPanelA
; WHAT IT DOES: Draws panel A's gauges: $0DFA as a four-sprite bar at
; ($0E0F, $0E11) from sprite slot $140, and $0E06 as a six-sprite bar at
; ($0E13, $0E15) from slot $16C.
; HOW: Each position is built in A with XBA and moved to X; the value is
; loaded last. Work RAM through the data bank.
; Entry: any widths. Exit: as SpriteBar6WithMarker; P as at its call: M
; 8-bit, X 16-bit, N, Z clear (from Y = $16C), C, V unchanged.
%ramcode($7EEE46, $014B6D)
HudGaugesPanelA:
    sep.b #$20
    rep.b #$10
    lda.w !gauge_a4_y
    xba
    lda.w !gauge_a4_x
    rep.b #$20
    tax
    sep.b #$20
    lda.w !gauge_a4_value
    ldy.w #$0140
    jsl.l SpriteBar4WithMarker
    sep.b #$20
    rep.b #$10
    lda.w !gauge_a6_y
    xba
    lda.w !gauge_a6_x
    rep.b #$20
    tax
    sep.b #$20
    lda.w !gauge_a6_value
    ldy.w #$016C
    jsl.l SpriteBar6WithMarker
    rts

; @routine 7E:EE7B HudGaugesPanelB
; WHAT IT DOES: When $0D55 or $0D62 is set, draws panel B's gauges: $0DF9
; as a four-sprite bar at ($0E0E, $0E10) and $0E05 as a six-sprite bar at
; ($0E12, $0E14), from sprite slots $40 and $6C when $0D62 is set, else
; $100 and $12C.
; HOW: As HudGaugesPanelA; $0D55 and $0D62 are first tested with long
; loads, then $0D62 through the data bank.
; Entry: any widths. Exit: when neither is set, A low byte = 0, M 8-bit,
; X 16-bit, Z set, N clear; otherwise as SpriteBar6WithMarker, P as at its
; call (M 8-bit, X 16-bit, N, Z from the slot).
%ramcode($7EEE7B, $014BA2)
HudGaugesPanelB:
    sep.b #$20
    rep.b #$10
    lda.l !panel_b_shown
    bne .lEE8C
    lda.l !wram_0D62
    bne .lEE8C
    rts
.lEE8C:
    lda.w !wram_0D62
    bne .lEEA5
    lda.w !gauge_b4_y
    xba
    lda.w !gauge_b4_x
    rep.b #$20
    tax
    sep.b #$20
    lda.w !gauge_b4_value
    ldy.w #$0100
    bra .lEEBB
.lEEA5:
    sep.b #$20
    rep.b #$10
    lda.w !gauge_b4_y
    xba
    lda.w !gauge_b4_x
    rep.b #$20
    tax
    sep.b #$20
    lda.w !gauge_b4_value
    ldy.w #$0040
.lEEBB:
    jsl.l SpriteBar4WithMarker
    sep.b #$20
    rep.b #$10
    lda.w !wram_0D62
    bne .lEEDC
    lda.w !gauge_b6_y
    xba
    lda.w !gauge_b6_x
    rep.b #$20
    tax
    sep.b #$20
    lda.w !gauge_b6_value
    ldy.w #$012C
    bra .lEEF2
.lEEDC:
    sep.b #$20
    rep.b #$10
    lda.w !gauge_b6_y
    xba
    lda.w !gauge_b6_x
    rep.b #$20
    tax
    sep.b #$20
    lda.w !gauge_b6_value
    ldy.w #$006C
.lEEF2:
    jsl.l SpriteBar6WithMarker
    rts

; @routine 07:C53A Gauge6BPlace
; WHAT IT DOES: Puts panel B's six-sprite gauge at Y = $5A ($0E14) and clears
; the X-position high bits of sprites 27-38 in the high table ($057B-$057E).
; HOW: Byte ANDs through the data bank (masks $BF, $AA, $AA, $EA).
; Entry: any widths. Exit: A low byte = the last byte; X = 0; X and Y high
; bytes cleared (SEP #$30); P unchanged.
%at($07C53A)
Gauge6BPlace:
    php
    sep.b #$30
    lda.b #$5A
    sta.w !gauge_b6_y
    ldx.b #$00
    bra Gauge6Place

; @routine 07:C546 Gauge6APlace
; WHAT IT DOES: As Gauge6BPlace for panel A: Y = $C2 ($0E15), sprites 91-102
; ($058B-$058E).
; HOW: As Gauge6BPlace with X = $10. Not reached in the traces so far.
; Entry: any widths. Exit: as Gauge6BPlace, X = $10.
%at($07C546)
Gauge6APlace:
    php
    sep.b #$30
    lda.b #$C2
    sta.w !gauge_a6_y
    ldx.b #$10
Gauge6Place:
    lda.w !oam_buffer_high+6,x
    and.b #$BF
    sta.w !oam_buffer_high+6,x
    lda.w !oam_buffer_high+7,x
    and.b #$AA
    sta.w !oam_buffer_high+7,x
    lda.w !oam_buffer_high+8,x
    and.b #$AA
    sta.w !oam_buffer_high+8,x
    lda.w !oam_buffer_high+9,x
    and.b #$EA
    sta.w !oam_buffer_high+9,x
    plp
    rts
