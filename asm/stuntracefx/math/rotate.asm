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
