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
