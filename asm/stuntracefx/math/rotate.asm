; rotate.asm: rotating a 2D vector by an 8-bit angle with the S-CPU's
; hardware multiplier.
; C: src/stuntracefx/math/rotate.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:C27F RotateXZ
; WHAT IT DOES: Rotates the vector (x, z) in direct page $02 and $94 by the
; angle A: x' = x cos + z sin goes to direct page $04, z' = z cos - x sin to
; $05F2, each sign-extended to 16 bits.
; HOW: cos and sin of -A come from the byte tables at $00:973E and $00:96FE
; (kept at $0625 and $0624); four signed multiplies via smul, added in 8
; bits. The data bank is 0 inside.
; Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = the high byte of
; z'; A high byte, X, Y unchanged; P unchanged except N, Z, which the final
; pull of Y sets from Y.
%at($03C27F)
RotateXZ:
    phy
    phx
    php
    phb
    sep.b #$10
    eor.b #$FF
    inc a
    tax
    lda.b #$00
    pha
    plb
    lda.w !rot_cos_table,x
    sta.w !rot_cos
    lda.w !rot_sin_table,x
    sta.w !rot_sin
    lda.b !dp_02
    bmi .lC2B3
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC2D6
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC2E4
.lC2B3:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC2C6
    sta.w !WRMPYB
    nop
    jmp.w .lC2DE
.lC2C6:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC2E4
.lC2D6:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC2DE:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC2E4:
    tax
    stx.b !dp_04
    lda.b !dp_94
    bmi .lC301
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC324
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC332
.lC301:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC314
    sta.w !WRMPYB
    nop
    jmp.w .lC32C
.lC314:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC332
.lC324:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC32C:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC332:
    tax
    txa
    clc
    adc.b !dp_04
    sta.b !dp_04
    bit.b #$80
    beq .lC343
    lda.b #$FF
    sta.b !dp_05
    bra .lC345
.lC343:
    stz.b !dp_05
.lC345:
    lda.b !dp_02
    bmi .lC35F
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC382
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC390
.lC35F:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC372
    sta.w !WRMPYB
    nop
    jmp.w .lC38A
.lC372:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC390
.lC382:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC38A:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC390:
    tax
    txa
    eor.b #$FF
    inc a
    sta.w !rot_z
    lda.b !dp_94
    bmi .lC3B2
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC3D5
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC3E3
.lC3B2:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC3C5
    sta.w !WRMPYB
    nop
    jmp.w .lC3DD
.lC3C5:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC3E3
.lC3D5:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC3DD:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC3E3:
    tax
    txa
    clc
    adc.w !rot_z
    sta.w !rot_z
    bit.b #$80
    beq .lC3F7
    lda.b #$FF
    sta.w !rot_z_high
    bra .lC3FA
.lC3F7:
    stz.w !rot_z_high
.lC3FA:
    plb
    plp
    plx
    ply
    rtl

; @routine 03:C3FF RotateXZFrom08
; WHAT IT DOES: As RotateXZ, for the vector (x, z) in direct page $08 and
; $94 and the angle A itself (not negated): x' = x cos + z sin goes to
; direct page $0A, z' = z cos - x sin to $05F2, each sign-extended to 16
; bits.
; HOW: cos and sin of A from the byte tables at $00:973E and $00:96FE
; (kept at $0625 and $0624); four signed multiplies via smul, added in 8
; bits. The data bank is 0 inside.
; Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = the high byte of
; z'; A high byte, X, Y unchanged; P unchanged except N, Z, which the final
; pull of X sets from X.
%at($03C3FF)
RotateXZFrom08:
    phx
    phy
    php
    phb
    sep.b #$10
    tax
    lda.b #$00
    pha
    plb
    lda.w !rot_cos_table,x
    sta.w !rot_cos
    lda.w !rot_sin_table,x
    sta.w !rot_sin
    lda.b !dp_08
    bmi .lC430
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC453
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC461
.lC430:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC443
    sta.w !WRMPYB
    nop
    jmp.w .lC45B
.lC443:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC461
.lC453:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC45B:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC461:
    tax
    stx.b !dp_0a
    lda.b !dp_94
    bmi .lC47E
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC4A1
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC4AF
.lC47E:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC491
    sta.w !WRMPYB
    nop
    jmp.w .lC4A9
.lC491:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC4AF
.lC4A1:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC4A9:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC4AF:
    tax
    txa
    clc
    adc.b !dp_0a
    sta.b !dp_0a
    bit.b #$80
    beq .lC4C0
    lda.b #$FF
    sta.b !dp_0b
    bra .lC4C2
.lC4C0:
    stz.b !dp_0b
.lC4C2:
    lda.b !dp_08
    bmi .lC4DC
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC4FF
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC50D
.lC4DC:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC4EF
    sta.w !WRMPYB
    nop
    jmp.w .lC507
.lC4EF:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC50D
.lC4FF:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC507:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC50D:
    tax
    txa
    eor.b #$FF
    inc a
    sta.w !rot_z
    lda.b !dp_94
    bmi .lC52F
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC552
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC560
.lC52F:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC542
    sta.w !WRMPYB
    nop
    jmp.w .lC55A
.lC542:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC560
.lC552:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC55A:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC560:
    tax
    txa
    clc
    adc.w !rot_z
    sta.w !rot_z
    bit.b #$80
    beq .lC574
    lda.b #$FF
    sta.w !rot_z_high
    bra .lC577
.lC574:
    stz.w !rot_z_high
.lC577:
    plb
    plp
    ply
    plx
    rtl

; @routine 03:C57C RotateXY
; WHAT IT DOES: Rotates the vector (x, y) in direct page $02 and $08 by the
; angle A: x' = x cos + y sin goes to direct page $04, y' = y cos - x sin to
; direct page $0A, each sign-extended to 16 bits.
; HOW: As RotateXZFrom08 (cos and sin of A, four signed multiplies via
; smul, added in 8 bits, data bank 0 inside).
; Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = the high byte of
; y'; A high byte, X, Y unchanged; P unchanged except N, Z, which the final
; pull of Y sets from Y.
%at($03C57C)
RotateXY:
    phy
    phx
    php
    phb
    sep.b #$10
    tax
    lda.b #$00
    pha
    plb
    lda.w !rot_cos_table,x
    sta.w !rot_cos
    lda.w !rot_sin_table,x
    sta.w !rot_sin
    lda.b !dp_02
    bmi .lC5AD
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC5D0
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC5DE
.lC5AD:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC5C0
    sta.w !WRMPYB
    nop
    jmp.w .lC5D8
.lC5C0:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC5DE
.lC5D0:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC5D8:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC5DE:
    tax
    stx.b !dp_04
    lda.b !dp_08
    bmi .lC5FB
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC61E
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC62C
.lC5FB:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC60E
    sta.w !WRMPYB
    nop
    jmp.w .lC626
.lC60E:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC62C
.lC61E:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC626:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC62C:
    tax
    txa
    clc
    adc.b !dp_04
    sta.b !dp_04
    bit.b #$80
    beq .lC63D
    lda.b #$FF
    sta.b !dp_05
    bra .lC63F
.lC63D:
    stz.b !dp_05
.lC63F:
    lda.b !dp_02
    bmi .lC659
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC67C
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC68A
.lC659:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_sin
    bmi .lC66C
    sta.w !WRMPYB
    nop
    jmp.w .lC684
.lC66C:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC68A
.lC67C:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC684:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC68A:
    tax
    txa
    eor.b #$FF
    inc a
    sta.b !dp_0a
    lda.b !dp_08
    bmi .lC6AB
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC6CE
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC6DC
.lC6AB:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.w !rot_cos
    bmi .lC6BE
    sta.w !WRMPYB
    nop
    jmp.w .lC6D6
.lC6BE:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC6DC
.lC6CE:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC6D6:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC6DC:
    tax
    txa
    clc
    adc.b !dp_0a
    sta.b !dp_0a
    bit.b #$80
    beq .lC6ED
    lda.b #$FF
    sta.b !dp_0b
    bra .lC6EF
.lC6ED:
    stz.b !dp_0b
.lC6EF:
    plb
    plp
    plx
    ply
    rtl

; @routine 02:DF95 MulWordByteLong
; WHAT IT DOES: MulWordByte for callers in other banks.
; HOW: JSR, RTL.
; Entry: M 8-bit, X 8-bit, D = $0000. Exit: as MulWordByte.
%at($02DF95)
MulWordByteLong:
    jsr.w MulWordByte
    rtl

; @routine 02:DF99 MulWordByte
; WHAT IT DOES: Multiplies the signed word in direct page $67 by the signed
; byte at $69 times 2, keeping the product's top 16 of 24 bits: $6A = v *
; 2s / 256. Nothing in the game calls it.
; HOW: The hardware multiplier, on magnitudes: 2|s| times the low byte,
; then (after reading the high byte of that product) times the high byte;
; the two products are added, or subtracted from 0 when the signs differ.
; The multiplier's registers through the data bank; $6C-$6D are cleared
; and used as the zero to add to.
; Entry: M 8-bit, X 8-bit, D = $0000. Exit: M 8-bit; A = the product
; (both bytes); Y = the first product's high byte; N, Z, C, V from the
; last add or subtract.
MulWordByte:
    stz.b !mul_zero
    stz.b !mul_zero+1
    stz.b !mul_product+1
    lda.b !mul_byte
    bmi .neg_s
    asl a
    sta.w !WRMPYA
    lda.b !mul_word+1
    bmi .pos_s_neg_v
    lda.b !mul_word
    sta.w !WRMPYB
    nop
    nop
    lda.b !mul_word+1
    jmp.w .add
.pos_s_neg_v:
    lda.b #$00
    sec
    sbc.b !mul_word
    sta.w !WRMPYB
    lda.b #$00
    sbc.b !mul_word+1
    jmp.w .sub
.neg_s:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !mul_word+1
    bmi .neg_s_neg_v
    lda.b !mul_word
    sta.w !WRMPYB
    nop
    nop
    nop
    lda.b !mul_word+1
.sub:
    ldy.w !RDMPYH
    sta.w !WRMPYB
    sty.b !mul_product
    rep.b #$20
    lda.b !mul_zero
    sec
    sbc.w !RDMPYL
    sec
    sbc.b !mul_product
    jmp.w .store
.neg_s_neg_v:
    lda.b #$00
    sec
    sbc.b !mul_word
    sta.w !WRMPYB
    nop
    nop
    lda.b #$00
    sbc.b !mul_word+1
.add:
    ldy.w !RDMPYH
    sta.w !WRMPYB
    sty.b !mul_product
    rep.b #$21
    lda.b !mul_zero
    adc.w !RDMPYL
    clc
    adc.b !mul_product
.store:
    sta.b !mul_product
    sep.b #$20
    rts

; @routine 03:C6F4 RotateXZWide
; WHAT IT DOES: Rotates the 16-bit vector (x, z) in direct page $02 and
; $94 by the angle A: z' = x sin + z cos goes to $05F2, x' = x cos - z
; sin to direct page $04, at 16-bit precision (RotateXZ works in 8 bits).
; Nothing in the game calls it.
; HOW: sin and cos of A from the byte tables at $00:96FE and $00:973E
; (kept at $05F6 and $05F7); four MulWordByteLong products, added and
; subtracted a byte at a time. X and Y are kept in direct page $3E and
; $40; P and DB are saved and restored; the data bank is 0 inside.
; Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = x''s high
; byte; X, Y, DB, P unchanged (X and Y reloaded from $3E and $40).
%at($03C6F4)
RotateXZWide:
    stx.b !scale_scratch
    sty.b !dp_40
    php
    phb
    sep.b #$10
    tax
    lda.b #$00
    pha
    plb
    lda.w !rot_sin_table,x
    sta.w !rot_wide_sin
    lda.w !rot_cos_table,x
    sta.w !rot_wide_cos
    lda.b !dp_02
    sta.b !mul_word
    lda.b !dp_03
    sta.b !mul_word+1
    lda.w !rot_wide_sin
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    sta.w !rot_z
    lda.b !mul_product+1
    sta.w !rot_z+1
    lda.b !dp_94
    sta.b !mul_word
    lda.b !dp_95
    sta.b !mul_word+1
    lda.w !rot_wide_cos
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    clc
    adc.w !rot_z
    sta.w !rot_z
    lda.b !mul_product+1
    adc.w !rot_z+1
    sta.w !rot_z+1
    lda.b !dp_94
    sta.b !mul_word
    lda.b !dp_95
    sta.b !mul_word+1
    lda.w !rot_wide_sin
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    sta.b !dp_04
    lda.b !mul_product+1
    sta.b !dp_05
    lda.b !dp_02
    sta.b !mul_word
    lda.b !dp_03
    sta.b !mul_word+1
    lda.w !rot_wide_cos
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    sec
    sbc.b !dp_04
    sta.b !dp_04
    lda.b !mul_product+1
    sbc.b !dp_05
    sta.b !dp_05
    plb
    plp
    ldy.b !dp_40
    ldx.b !scale_scratch
    rtl

; @routine 03:C788 RotateXZWideFrom08
; WHAT IT DOES: As RotateXZWide for the vector (x, z) in direct page $08
; and $94 and the angle -A: z' to $05F2, x' to direct page $0A. Nothing
; in the game calls it.
; HOW: As RotateXZWide, the angle negated first (EOR #$FF, INC).
; Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = x''s high
; byte; X, Y, DB, P unchanged (X and Y reloaded from $3E and $40).
RotateXZWideFrom08:
    stx.b !scale_scratch
    sty.b !dp_40
    php
    phb
    sep.b #$10
    eor.b #$FF
    inc a
    tax
    lda.b #$00
    pha
    plb
    lda.w !rot_sin_table,x
    sta.w !rot_wide_sin
    lda.w !rot_cos_table,x
    sta.w !rot_wide_cos
    lda.b !dp_08
    sta.b !mul_word
    lda.b !dp_09
    sta.b !mul_word+1
    lda.w !rot_wide_sin
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    sta.w !rot_z
    lda.b !mul_product+1
    sta.w !rot_z+1
    lda.b !dp_94
    sta.b !mul_word
    lda.b !dp_95
    sta.b !mul_word+1
    lda.w !rot_wide_cos
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    clc
    adc.w !rot_z
    sta.w !rot_z
    lda.b !mul_product+1
    adc.w !rot_z+1
    sta.w !rot_z+1
    lda.b !dp_94
    sta.b !mul_word
    lda.b !dp_95
    sta.b !mul_word+1
    lda.w !rot_wide_sin
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    sta.b !actor_0a
    lda.b !mul_product+1
    sta.b !dp_0b
    lda.b !dp_08
    sta.b !mul_word
    lda.b !dp_09
    sta.b !mul_word+1
    lda.w !rot_wide_cos
    sta.b !mul_byte
    jsl.l MulWordByteLong
    lda.b !mul_product
    sec
    sbc.b !actor_0a
    sta.b !actor_0a
    lda.b !mul_product+1
    sbc.b !dp_0b
    sta.b !dp_0b
    plb
    plp
    ldy.b !dp_40
    ldx.b !scale_scratch
    rtl
