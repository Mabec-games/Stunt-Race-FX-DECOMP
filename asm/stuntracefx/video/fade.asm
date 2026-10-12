; fade.asm: the screen fade, stepped once a frame by the raster interrupt.
; The level is a 16-bit accumulator whose high byte is the INIDISP
; brightness; $0F00 is full and $8000 is forced blank. Runs from work RAM.
; C: src/stuntracefx/video/fade.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:AC4D FadeStepFixed
; WHAT IT DOES: Steps the fade by one frame: in by a fixed $98, or out.
; HOW: Fading in, adds $98 and stops at $0F00; fading out, joins FadeStep.
; Entry: M 8-bit. Exit: A = the new level; M 8-bit; flags from the last
; compare or load.
%ramcode($7EAC4D, $010974)
FadeStepFixed:
    lda.w !fade_out
    bne FadeStep_out
    rep.b #$20
    lda.w !fade_level
    clc
    adc.w #$0098
    cmp.w #$0F00
    bcc .store
    lda.w #$0F00
.store:
    sta.w !fade_level
    sep.b #$20
    rts

; @routine 7E:AC69 FadeStep
; WHAT IT DOES: Steps the fade by one frame, fading in faster or slower
; depending on the current brightness.
; HOW: Fading in, adds the 16-bit step the brightness (high byte of the
; level) selects from the table after this routine, stopping at $0F00.
; Fading out, subtracts $58, going to forced blank instead of below zero.
; Entry: M 8-bit, X 8-bit. Exit: A = the new level; X = brightness * 2 when
; fading in; M 8-bit; flags from the last compare, subtract or load.
FadeStep:
    lda.w !fade_out
    beq FadeStep_in
FadeStep_out:
    rep.b #$20
    lda.w !fade_level
    bmi FadeStep_store
    sec
    sbc.w #$0058
    bcs FadeStep_store
    lda.w #$8000
    bra FadeStep_store
FadeStep_in:
    lda.w !fade_level+1
    asl a
    tax
    rep.b #$20
    lda.w !fade_level
    clc
    adc.l FadeInSteps,x
    cmp.w #$0F00
    bcc FadeStep_store
    lda.w #$0F00
FadeStep_store:
    sta.w !fade_level
    sep.b #$20
    rts
FadeInSteps:
    dw $0008, $0080, $0100, $0180, $0180, $0180, $0280, $0280
    dw $0280, $0280, $0380, $0380, $0380, $0380, $0380, $0200
