; scale.asm: fixed scalings of signed 16-bit values. C: src/stuntracefx/math/scale.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 03:CC29 ScaleQuarter
; WHAT IT DOES: Returns a quarter of a signed 16-bit value.
; HOW: Two arithmetic shifts right (CMP #$8000 / ROR A), rounding toward
; minus infinity.
; Entry: M 16-bit; A = value. Exit: A = v/4; N, Z from it; C = bit 1 of v;
; V unchanged.
%at($03CC29)
ScaleQuarter:
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    rtl

; @routine 03:CC32 ScaleNineSixteenths
; WHAT IT DOES: Returns v/2 + v/16 of a signed 16-bit value (9/16).
; HOW: Halves the value (arithmetic shift), keeps the half in $3E, halves
; it three more times and adds $3E back.
; Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E = v/2;
; N, Z, C, V from the add.
%at($03CC32)
ScaleNineSixteenths:
    cmp.w #$8000
    ror a
    sta.b !scale_scratch
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.b !scale_scratch
    rtl

; @routine 03:CC48 ScaleFiveEighths
; WHAT IT DOES: Returns v/2 + v/8 of a signed 16-bit value (5/8).
; HOW: As ScaleNineSixteenths, with two halvings after the first instead
; of three.
; Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E = v/2;
; N, Z, C, V from the add.
%at($03CC48)
ScaleFiveEighths:
    cmp.w #$8000
    ror a
    sta.b !scale_scratch
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.b !scale_scratch
    rtl

; @routine 03:CC5A ScaleThreeQuarters
; WHAT IT DOES: Returns three quarters of a signed 16-bit value.
; HOW: CMP #$8000 / ROR A is an arithmetic shift right (the sign bit comes
; back in through carry): halve, keep the half in $3E, halve again, add.
; Entry: M 16-bit, D = $0000; A = value.
; Exit: A = v/2 + v/4; $3E = v/2; C and V from the add; N and Z from A.
%at($03CC5A)
ScaleThreeQuarters:
    cmp.w #$8000
    ror a
    sta.b !scale_scratch
    cmp.w #$8000
    ror a
    clc
    adc.b !scale_scratch
    rtl

; @routine 03:CC68 ScaleSevenEighths
; WHAT IT DOES: Returns v/2 + v/4 + v/8 of a signed 16-bit value (7/8).
; HOW: Halves three times, keeping v/2 in $3E and v/4 in $40, then adds
; them to v/8 in that order.
; Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E = v/2,
; $40 = v/4; N, Z, C, V from the last add.
%at($03CC68)
ScaleSevenEighths:
    cmp.w #$8000
    ror a
    sta.b !scale_scratch
    cmp.w #$8000
    ror a
    sta.b !dp_40
    cmp.w #$8000
    ror a
    clc
    adc.b !scale_scratch
    clc
    adc.b !dp_40
    rtl

; @routine 03:CC7F ScaleFifteenSixteenths
; WHAT IT DOES: Returns v/2 + v/4 + v/8 + v/16 of a signed 16-bit value
; (15/16).
; HOW: Halves four times, keeping v/2 in $3E, v/4 in $40 and v/8 at $1F2B
; (through the data bank), then adds v/8, v/2 and v/4 to v/16 in that
; order.
; Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E, $40,
; $1F2B as above; N, Z, C, V from the last add.
%at($03CC7F)
ScaleFifteenSixteenths:
    cmp.w #$8000
    ror a
    sta.b !scale_scratch
    cmp.w #$8000
    ror a
    sta.b !dp_40
    cmp.w #$8000
    ror a
    sta.w !wram_1F2B
    cmp.w #$8000
    ror a
    clc
    adc.w !wram_1F2B
    clc
    adc.b !scale_scratch
    clc
    adc.b !dp_40
    rtl

; @routine 03:FCE0 Mode7ScaleProduct
; WHAT IT DOES: Divides the signed value A by the byte at $7F:340A and
; multiplies the quotient by the byte at $7F:3408 with the PPU's signed
; multiplier, returning the low 16 bits of the product.
; HOW: Carry clear means A is negative: its magnitude is divided and the
; quotient negated. The quotient goes to M7A (low then high byte), the
; multiplier to M7B; MPYL/MPYM is read back.
; Entry: M 16-bit; A = value; C = 1 when A is to be taken as positive.
; Exit: A = the product's low word; M 16-bit; N, Z from A; C, V unchanged.
%at($03FCE0)
Mode7ScaleProduct:
    bcc .lFCFC
    sta.w !WRDIVL
    sep.b #$20
    lda.l !scale_divisor
    sta.w !WRDIVB
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    rep.b #$20
    lda.w !RDDIVL
    bra .lFD1C
.lFCFC:
    eor.w #$FFFF
    inc a
    sta.w !WRDIVL
    sep.b #$20
    lda.l !scale_divisor
    sta.w !WRDIVB
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    rep.b #$20
    lda.w !RDDIVL
    eor.w #$FFFF
    inc a
.lFD1C:
    sep.b #$20
    sta.w !M7A
    xba
    sta.w !M7A
    lda.l !fixed_colour_step
    sta.w !M7B
    rep.b #$20
    lda.w !MPYL
    rts

; @routine 08:83EC ScaleObjectXZ
; WHAT IT DOES: ScaleDp02Dp94 on object X's X and Z position: $02 = its
; +$0C, $94 = its +$10 (bank $7E), then rescaled.
; HOW: Two long loads, then falls into ScaleDp02Dp94.
; Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: as
; ScaleDp02Dp94.
%at($0883EC)
ScaleObjectXZ:
    lda.l !obj_pos_x_long,x
    sta.b !dp_02
    lda.l !obj_pos_z_long,x
    sta.b !dp_94

; @routine 08:83F8 ScaleDp02Dp94
; WHAT IT DOES: Rescales the pair in direct page $02 and $94: $02 becomes
; ($02 >> 10, signed) + $20 with bit 0 cleared, $94 becomes (its top five
; bits >> 5, signed) + $400; returns their sum.
; HOW: Arithmetic shifts by CMP #$8000 / ROR A; the high byte of $02 >> 2
; is taken and sign-extended.
; Entry: M 16-bit, D = $0000. Exit: A = the sum; N, Z, C, V from the last
; add.
%at($0883F8)
ScaleDp02Dp94:
    lda.b !dp_02
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    xba
    bit.w #$0080
    beq .l840D
    ora.w #$FF00
    bra .l8410
.l840D:
    and.w #$00FF
.l8410:
    clc
    adc.w #$0020
    and.w #$FFFE
    sta.b !dp_02
    lda.b !dp_94
    and.w #$F800
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.w #$0400
    sta.b !dp_94
    clc
    adc.b !dp_02
    rts

; @routine 08:83E8 ScaleDp02Dp94Long
; WHAT IT DOES: ScaleDp02Dp94 for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ScaleDp02Dp94. Exit: as ScaleDp02Dp94.
%at($0883E8)
ScaleDp02Dp94Long:
    jsr.w ScaleDp02Dp94
    rtl

; @routine 03:CC0D ScaleFiveThirtySeconds
; WHAT IT DOES: Returns v/8 + v/32 of a signed 16-bit value (5/32), keeping
; v/8 at $1F2B.
; HOW: Arithmetic shifts by CMP #$8000 / ROR A (rounding toward minus
; infinity), then an add.
; Entry: M 16-bit; A = value. Exit: A = the result; $1F2B = v/8; N, Z, C, V
; from the add.
%at($03CC0D)
ScaleFiveThirtySeconds:
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_1F2B
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.w !wram_1F2B
    rtl

; @routine 03:FD32 BlendStepUp
; WHAT IT DOES: Unless the factor $7F:3408 has reached $7F:340A, steps it up
; by one and sets the four words at $7F:3432 between their bases ($7F:3442)
; and targets ($7F:343A) by it.
; HOW: (target - base) through Mode7ScaleProduct, plus the base. The return
; at the top is not reached in the traces so far.
; Entry: any widths. Exit: M, X 16-bit; A = the last word; N, Z, C, V from
; its add; or, at the top, A = the factor with Z and C set.
%at($03FD32)
BlendStepUp:
    rep.b #$30
    lda.l !fixed_colour_step
    cmp.l !scale_divisor
    bne .step
    rtl
.step:
    inc a
    sta.l !fixed_colour_step
    bra BlendWords

; @routine 03:FD46 BlendStepDown
; WHAT IT DOES: Unless the factor $7F:3408 is 0, steps it down by one and sets
; the four words at $7F:3432 by it (as BlendStepUp).
; HOW: As BlendStepUp. The return at the top is not reached in the traces so
; far.
; Entry: any widths. Exit: as BlendStepUp; or, at the top, A = 0 with Z set.
%at($03FD46)
BlendStepDown:
    rep.b #$30
    lda.l !fixed_colour_step
    bne .step
    rtl
.step:
    dec a
    sta.l !fixed_colour_step
BlendWords:
    lda.l !scaled_words_target+0
    sec
    sbc.l !scaled_words_base+0
    jsr Mode7ScaleProduct
    clc
    adc.l !scaled_words_base+0
    sta.l !scaled_words+0
    lda.l !scaled_words_target+2
    sec
    sbc.l !scaled_words_base+2
    jsr Mode7ScaleProduct
    clc
    adc.l !scaled_words_base+2
    sta.l !scaled_words+2
    lda.l !scaled_words_target+4
    sec
    sbc.l !scaled_words_base+4
    jsr Mode7ScaleProduct
    clc
    adc.l !scaled_words_base+4
    sta.l !scaled_words+4
    lda.l !scaled_words_target+6
    sec
    sbc.l !scaled_words_base+6
    jsr Mode7ScaleProduct
    clc
    adc.l !scaled_words_base+6
    sta.l !scaled_words+6
    rtl
