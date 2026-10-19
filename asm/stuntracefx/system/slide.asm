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
    lda.w !slide_value+1
    sta.l !slide_out
.lE146:
    plb
    plp
    rtl

; @routine 7E:E190 SlideOutWobble
; WHAT IT DOES: Once the sliding value's high byte ($19F7) is $0F, every
; second call moves the published byte $7F:3468 by $1A04 and turns $1A04
; round when that byte reaches $0F or $0B: a wobble between the two.
; Nothing reaches it (no JSL, jump or table; it follows DataBankTest's
; RTL), so a direct call is its only way in.
; HOW: SEP #$20; $1A05 counts calls to 2 and is cleared; CLC, ADC; EOR #$FF
; and INC to negate. Ends PLB, RTL: the data bank its caller pushed is
; pulled.
; Entry: the caller's data bank pushed above the return (PHB); data bank
; with $19F7, $1A04, $1A05. Exit: A low byte = the last value read or
; made; M 8-bit; data bank pulled, N, Z from it; C, V from the last compare
; or add.
%ramcode($7EE190, $013EB7)
SlideOutWobble:
    sep.b #$20
    lda.w !slide_value+1
    cmp.b #$0F
    bne .done
    lda.w !slide_wobble_count
    clc
    adc.b #$01
    sta.w !slide_wobble_count
    cmp.b #$02
    bne .done
    stz.w !slide_wobble_count
    lda.l !slide_out
    clc
    adc.w !slide_wobble_step
    sta.l !slide_out
    cmp.b #$0F
    beq .turn
    cmp.b #$0B
    beq .turn
    jmp.w .done
.turn:
    lda.w !slide_wobble_step
    eor.b #$FF
    inc a
    sta.w !slide_wobble_step
.done:
    plb
    rtl
