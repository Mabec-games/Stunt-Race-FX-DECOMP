; slide.asm: a 16-bit value at $19F6 that slides toward a target while bit 7
; of $0DA5 is set; its high byte is copied to $7F:3468.
; C: src/stuntracefx/system/slide.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:E0E8 SlideStep
; WHAT IT DOES: Moves the sliding value one step toward its target and
; publishes its high byte; ends the slide when the target is reached going
; down.
; HOW: Direction by the sign of $19FC: adds or subtracts the step $19F8.
; Reaching the target exactly: going up, nothing is stored; going down, the
; slide flag is cleared, $7F:3470 = $FFFF and the high byte published.
; Otherwise the new value is stored and its high byte published.
; Entry: any widths. Exit: A depends on the path (see the abi wrapper);
; P unchanged.
%ramcode($7EE0E8, $013E0F)
SlideStep:
    php
    phb
    phk
    plb
    sep.b #$20
    lda.l !slide_flags
    and.b #$80
    beq .lE146
    rep.b #$20
    lda.w !slide_direction
    bmi .lE10F
    lda.w !slide_value
    clc
    adc.w !slide_step
    cmp.w !slide_target
    beq .lE123
    sta.w !slide_value
    jmp.w .lE13D
.lE10F:
    rep.b #$20
    lda.w !slide_value
    sec
    sbc.w !slide_step
    cmp.w !slide_target
    beq .lE123
    sta.w !slide_value
    jmp.w .lE13D
.lE123:
    lda.w !slide_direction
    bpl .lE146
    sep.b #$20
    lda.l !slide_flags
    and.b #$7F
    sta.l !slide_flags
    rep.b #$20
    lda.w #$FFFF
    sta.l !slide_done
.lE13D:
    sep.b #$20
    lda.w !slide_value+$1
    sta.l !slide_out
.lE146:
    plb
    plp
    rtl
