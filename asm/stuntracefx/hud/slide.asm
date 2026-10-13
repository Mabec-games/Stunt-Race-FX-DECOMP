; slide.asm: HUD panels that slide on and off the screen. Runs from work RAM.
; C: src/stuntracefx/hud/slide.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:AECE HudSlideIn
; WHAT IT DOES: Moves the sliding HUD panel 4 pixels further in, stopping at
; $10; a negative offset just moves.
; HOW: Adds 4; when the result is not negative and is $10 or more, uses $10.
; Entry: M 8-bit. Exit: A low byte = the new offset; flags from the add, the
; compare or the load.
%ramcode($7EAECE, $010BF5)
HudSlideIn:
    lda.w !hud_slide
    clc
    adc.b #$04
    bmi .store
    cmp.b #$10
    bcc .store
    lda.b #$10
.store:
    sta.w !hud_slide
    rts

; @routine 7E:AE18 HudPanelASlide
; WHAT IT DOES: In stage 8 and above ($0D54) slides panel A in (stage 8,
; HudSlideIn) or out (above, HudSlideOut, the stage restarting when the
; slide was at $D0) and places its sprites (Y offset $F8, sprite offset
; $20).
; HOW: The shared tail at $7E:AFA7.
; Entry: any widths, D = $0000. Exit: P restored; X = $20 plus as the
; tail left it; A as it left it.
%ramcode($7EAE18, $010B3F)
HudPanelASlide:
    php
    sep.b #$20
    rep.b #$10
    ldx.w #$0020
    lda.b #$F8
    sta.w !hud_panel_y
    lda.w !wram_0D54
    cmp.b #$08
    beq HudPanelSlideIn
    bcs HudPanelSlideOut
    plp
    rts

; @routine 7E:AE30 HudPanelBSlide
; WHAT IT DOES: In stage 8 and above places panel B's sprites (Y offset
; $4A, sprite offset $120) without moving the slide.
; HOW: As HudPanelASlide.
; Entry: as HudPanelASlide. Exit: as HudPanelASlide.
%ramcode($7EAE30, $010B57)
HudPanelBSlide:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$4A
    sta.w !hud_panel_y
    ldx.w #$0120
    lda.w !wram_0D54
    cmp.b #$08
    beq HudPanelPlace
    bcs HudPanelPlace2
    plp
    rts
HudPanelSlideIn:
    jsr.w HudSlideIn
HudPanelPlace:
    jmp.w HudSpritesY
HudPanelSlideOut:
    lda.w !hud_slide
    cmp.b #$D0
    bne .out
    stz.w !wram_0D54
.out:
    jsr.w HudSlideOut
HudPanelPlace2:
    jmp.w HudSpritesY

; @routine 7E:AE5E HudPanelsStep
; WHAT IT DOES: The HUD panels' stage $0D54: 2 moves the map marker $0E16
; up by 4 to $AC and places its column; 4 moves gauge B's X $0E12 right by
; 4 to $1F and places its row; 6 moves panel B's Y $0E0B down by 4 to $2A
; ($0E0C = $7C there) and places it; 8 slides in, 10 slides out (with the
; panel A sprites, $F9 = 0, X = 0); 0 does nothing.
; HOW: JMP ($AE67,X) on the stage; the placing tails at $7E:AF09, $AF5C,
; $AEF2 and $AFA7. The marker clamp at $AC is not reached in the traces
; so far.
; Entry: any widths, D = $0000. Exit: P restored.
%ramcode($7EAE5E, $010B85)
HudPanelsStep:
    php
    sep.b #$30
    ldx.w !wram_0D54
    jmp.w (.stages,x)
.stages:
    dw .stage0, .stage2, .stage4, .stage6, .stage8, .stage10
.stage0:
    plp
    rts
.stage2:
    lda.w !wram_0E16
    sec
    sbc.b #$04
    cmp.b #$AC
    bcs .marker
    lda.b #$AC
.marker:
    sta.w !wram_0E16
    jmp.w HudMarkerY
.stage4:
    lda.w !gauge_b6_x
    clc
    adc.b #$04
    bmi .gauge
    cmp.b #$1F
    bcc .gauge
    lda.b #$1F
.gauge:
    sta.w !gauge_b6_x
    jmp.w HudGaugeBX
.stage6:
    lda.w !map_b_y_w
    clc
    adc.b #$04
    bmi .panel
    cmp.b #$2A
    bcc .panel
    lda.b #$7C
    sta.w !wram_0E0C
    lda.b #$2A
.panel:
    sta.w !map_b_y_w
    jmp.w HudPanelBY
.stage8:
    jsr.w HudSlideIn
    rep.b #$10
    ldx.w #$0000
    stx.b !hud_panel_y
    jmp.w HudSpritesY
.stage10:
    jsr.w HudSlideOut
    rep.b #$10
    ldx.w #$0000
    stx.b !hud_panel_y
    jmp.w HudSpritesY

; @routine 7E:AEE0 HudSlideOut
; WHAT IT DOES: Moves the HUD slide $0D57 back by 4, down to $D0 (once it
; is negative).
; HOW: 8-bit subtract and compare through the data bank.
; Entry: M 8-bit. Exit: A low byte = the new slide; flags from it.
%ramcode($7EAEE0, $010C07)
HudSlideOut:
    lda.w !hud_slide
    sec
    sbc.b #$04
    bpl .store
    cmp.b #$D0
    bcs .store
    lda.b #$D0
.store:
    sta.w !hud_slide
    rts
HudPanelBY:                             ; jumped to by HudPanelsStep, ends its PLP
    lda.w !map_b_y_w
    sec
    sbc.b #$0D
    sta.w !oam_buffer+$1C0
    sta.w !oam_buffer+$1C8
    clc
    adc.b #$10
    sta.w !oam_buffer+$1C4
    sta.w !oam_buffer+$1CC
    plp
    rts
HudMarkerY:                             ; jumped to by HudPanelsStep, ends its PLP
    lda.w !wram_0E16
    sta.w !oam_buffer+$1F8
    sta.w !oam_buffer+$1FC
    clc
    adc.b #$08
    bcc .m0
    lda.b #$FF
.m0:
    sta.w !oam_buffer+$1D0
    clc
    adc.b #$06
    bcc .m1
    lda.b #$FF
.m1:
    sta.w !oam_buffer+$1E8
    sta.w !oam_buffer+$1EC
    clc
    adc.b #$08
    bcc .m2
    lda.b #$FF
.m2:
    sta.w !oam_buffer+$1F0
    sta.w !oam_buffer+$1F4
    clc
    adc.b #$08
    bcc .m3
    lda.b #$FF
.m3:
    sta.w !oam_buffer+$1D4
    clc
    adc.b #$08
    bcc .m4
    lda.b #$FF
.m4:
    sta.w !oam_buffer+$1D8
    sta.w !oam_buffer+$1DC
    clc
    adc.b #$08
    bcc .m5
    lda.b #$FF
.m5:
    sta.w !oam_buffer+$1E0
    sta.w !oam_buffer+$1E4
    plp
    rts
HudGaugeBX:                             ; jumped to by HudPanelsStep, ends its PLP
    lda.w !gauge_b6_x
    bmi .g0
    sta.w !oam_buffer+$12C
.g0:
    sec
    sbc.b #$08
    bmi .g1
    sta.w !oam_buffer+$148
.g1:
    clc
    adc.b #$10
    bmi .g2
    sta.w !oam_buffer+$130
.g2:
    clc
    adc.b #$08
    bmi .g3
    sta.w !oam_buffer+$134
.g3:
    clc
    adc.b #$08
    bmi .g4
    sta.w !oam_buffer+$138
.g4:
    adc.b #$08
    sta.w !oam_buffer+$140
    adc.b #$08
    sta.w !oam_buffer+$144
    sta.w !oam_buffer+$13C
    adc.b #$08
    sta.w !oam_buffer+$14C
    adc.b #$04
    sta.w !oam_buffer+$150
    adc.b #$08
    sta.w !oam_buffer+$154
    adc.b #$08
    sta.w !oam_buffer+$158
    plp
    rts
HudSpritesY:                            ; jumped to by the panel slides, ends their PLP
    sep.b #$20
    rep.b #$10
    lda.w !hud_slide
    clc
    adc.b !hud_panel_y
    pha
    pha
    jsr.w SpriteYOnScreen
    sta.w !oam_buffer+$9C,x
    sta.w !oam_buffer+$AC,x
    pla
    clc
    adc.b #$10
    pha
    jsr.w SpriteYOnScreen
    sta.w !oam_buffer+$A0,x
    sta.w !oam_buffer+$B0,x
    pla
    clc
    adc.b #$10
    pha
    jsr.w SpriteYOnScreen
    sta.w !oam_buffer+$A4,x
    sta.w !oam_buffer+$B4,x
    pla
    clc
    adc.b #$10
    jsr.w SpriteYOnScreen
    sta.w !oam_buffer+$A8,x
    sta.w !oam_buffer+$B8,x
    pla
    pha
    adc.b #$0A
    jsr.w SpriteYOnScreen
    sta.w !oam_buffer+$90,x
    pla
    pha
    adc.b #$18
    jsr.w SpriteYOnScreen
    sta.w !oam_buffer+$94,x
    pla
    clc
    adc.b #$26
    jsr.w SpriteYOnScreen
    sta.w !oam_buffer+$98,x
    plp
    rts
