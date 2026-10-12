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
