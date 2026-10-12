; scale.asm: fixed scalings of signed 16-bit values. C: src/stuntracefx/math/scale.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch 65816

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
