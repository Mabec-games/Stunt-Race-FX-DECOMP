; debug_numbers.asm: numbers printed with DebugDrawChar in hexadecimal,
; decimal and binary, and their digit tables.
; C: src/stuntracefx/system/debug_numbers.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:D499 DebugPrintHexWord
; WHAT IT DOES: Meant to print A as four hexadecimal digits, the high byte
; first. The second pair printed is not A's low byte: the first
; DebugPrintHexByte leaves the high byte of the text position in A's high
; byte, which the second XBA brings down.
; HOW: XBA, DebugPrintHexByte, XBA, DebugPrintHexByte; A pushed and pulled
; (one byte).
; Entry: M 8-bit, D = $0000; A = the word. Exit: A low byte unchanged, its
; high byte as the second print leaves it; N, Z from the low byte.
%at($03D499)
DebugPrintHexWord:
    pha
    xba
    jsl.l DebugPrintHexByte
    xba
    jsl.l DebugPrintHexByte
    pla
    rtl

; @routine 03:D4A6 DebugPrintDecimalByte
; WHAT IT DOES: DebugDecimalByte for callers in other banks, with every
; register kept.
; HOW: long_wrapper.
; Entry: as DebugPrintHexByte. Exit: as DebugPrintHexByte.
%at($03D4A6)
DebugPrintDecimalByte:
    phb
    pha
    phx
    phy
    php
    sep.b #$20
    rep.b #$10
    sta.w !debug_byte
    lda.b #$00
    pha
    plb
    lda.w !debug_byte
    jsr.w DebugDecimalByte
    plp
    ply
    plx
    pla
    plb
    rtl

; @routine 03:D4C2 DebugPrintSignedByte
; WHAT IT DOES: DebugSignedByte for callers in other banks, with every
; register kept.
; HOW: long_wrapper.
; Entry: as DebugPrintHexByte. Exit: as DebugPrintHexByte.
%at($03D4C2)
DebugPrintSignedByte:
    phb
    pha
    phx
    phy
    php
    sep.b #$20
    rep.b #$10
    sta.w !debug_byte
    lda.b #$00
    pha
    plb
    lda.w !debug_byte
    jsr.w DebugSignedByte
    plp
    ply
    plx
    pla
    plb
    rtl

; @routine 03:D4DE DebugPrintHexByte
; WHAT IT DOES: DebugHexByte for callers in other banks, with every
; register kept.
; HOW: long_wrapper.
; Entry: A low byte = the byte; D = $0000. Exit: X, Y, P unchanged but N,
; Z, which the final PLB sets from the data bank; A unchanged with M
; 16-bit, its low byte only with M 8-bit.
%at($03D4DE)
DebugPrintHexByte:
    phb
    pha
    phx
    phy
    php
    sep.b #$20
    rep.b #$10
    sta.w !debug_byte
    lda.b #$00
    pha
    plb
    lda.w !debug_byte
    jsr.w DebugHexByte
    plp
    ply
    plx
    pla
    plb
    rtl

; @routine 03:D4FA DebugPrintDecimalWord
; WHAT IT DOES: Prints A as a signed decimal number: character $0D first
; when it is negative, then the digits of its magnitude without leading
; zeros (character $10 + digit), or one $10 for 0.
; HOW: The magnitude kept at $42; each digit counted in $3E by taking off
; the power of ten from the word table at $03:D57A (10000 to 1) while the
; result stays positive; Y says a digit has been drawn. 16-bit, the data
; bank 0; every register and P saved.
; Entry: any widths; A = the value; D = $0000. Exit: every register and P
; unchanged.
%at($03D4FA)
DebugPrintDecimalWord:
    php
    phb
    rep.b #$30
    phx
    phy
    pha
    sta.b !debug_value
    sep.b #$20
    lda.b #$00
    pha
    plb
    rep.b #$20
    lda.b !debug_value
    bpl .lD51E
    eor.w #$FFFF
    inc a
    sta.b !debug_value
    sep.b #$20
    rep.b #$10
    lda.b #$0D
    jsr.w DebugDrawChar
.lD51E:
    rep.b #$20
    ldx.w #$0000
    stz.b !debug_digit
    ldy.w #$0000
.lD528:
    lda.b !debug_value
    sec
    sbc.l DebugPowersOfTen,x
    bpl .lD574
    clc
    adc.l DebugPowersOfTen,x
    sta.b !debug_value
    lda.b !debug_digit
    cmp.w #$0000
    bne .lD544
    cpy.w #$0000
    beq .lD555
.lD544:
    ldy.w #$0001
    sep.b #$20
    phx
    phy
    lda.b !debug_digit
    clc
    adc.b #$10
    jsr.w DebugDrawChar
    ply
    plx
.lD555:
    rep.b #$20
    stz.b !debug_digit
    inx
    inx
    cpx.w #$000A
    bne .lD528
    cpy.w #$0000
    bne .lD56E
    sep.b #$20
    lda.b #$10
    jsr.w DebugDrawChar
    rep.b #$20
.lD56E:
    pla
    ply
    plx
    plb
    plp
    rtl
.lD574:
    sta.b !debug_value
    inc.b !debug_digit
    bra .lD528

; @routine 03:D584 DebugSignedByte
; WHAT IT DOES: Prints the signed byte in A: character $24 and the
; magnitude when it is negative, else character $29 and the value
; (DebugDecimalByte).
; HOW: The byte kept on the stack across the sign's draw.
; Entry: as DebugDecimalByte. Exit: as DebugDecimalByte.
%at($03D584)
DebugSignedByte:
    pha
    cmp.b #$00
    bpl .lD595
    lda.b #$24
    jsr.w DebugDrawChar
    pla
    eor.b #$FF
    inc a
    jmp.w DebugDecimalByte
.lD595:
    lda.b #$29
    jsr.w DebugDrawChar
    pla
    jmp.w DebugDecimalByte

; @routine 03:D59E DebugDecimalByte
; WHAT IT DOES: Prints the byte in A (0-255) in decimal: the hundreds
; digit only when there is one, the tens digit only when not 0 or after a
; hundreds digit, then the units digit. The digits are drawn as the values
; 0-9 (not the font's digit characters at $10).
; HOW: 200 and 100 taken off by compares; the rest looked up in the
; two-digit decimal table at $03:D5ED. The hundreds digit and the rest are
; kept on the stack across the draws.
; Entry: M 8-bit, X 16-bit; A = the byte. Exit: A, X, Y as the last
; DebugDrawChar leaves them; N, Z from the units digit; C from the last
; test of the tens (bit 3 of the pair, or no hundreds digit); V from the
; subtract of 200 or 100 when there was one.
%at($03D59E)
DebugDecimalByte:
    cmp.b #$C8
    bcc .lD5AA
    sec
    sbc.b #$C8
    pha
    lda.b #$02
    bra .lD5B9
.lD5AA:
    cmp.b #$64
    bcc .lD5B6
    sec
    sbc.b #$64
    pha
    lda.b #$01
    bra .lD5B9
.lD5B6:
    pha
    lda.b #$FF
.lD5B9:
    pha
    cmp.b #$FF
    beq .lD5C1
    jsr.w DebugDrawChar
.lD5C1:
    pla
    rep.b #$20
    and.w #$00FF
    tay
    sep.b #$20
    pla
    rep.b #$20
    and.w #$00FF
    tax
    sep.b #$20
    lda.l DebugDecimalPairs,x
    pha
    lsr a
    lsr a
    lsr a
    lsr a
    bne .lD5E3
    cpy.w #$00FF
    beq .lD5E6
.lD5E3:
    jsr.w DebugDrawChar
.lD5E6:
    pla
    and.b #$0F
    jsr.w DebugDrawChar
    rts

; @routine 03:D782 DebugHexByte
; WHAT IT DOES: Prints the byte in A as two hexadecimal digits.
; HOW: Each nibble indexes the ASCII digits at $03:D7B1 (a word read, its
; low byte kept), less $20 for the font. 16-bit inside; P saved.
; Entry: A low byte = the byte. Exit: P unchanged; A, X, Y as the second
; DebugDrawChar leaves them.
%at($03D782)
DebugHexByte:
    php
    rep.b #$30
    pha
    and.w #$00F0
    lsr a
    lsr a
    lsr a
    lsr a
    tax
    lda.l DebugHexDigits,x
    and.w #$00FF
    sec
    sbc.w #$0020
    jsr.w DebugDrawChar
    pla
    and.w #$000F
    tax
    lda.l DebugHexDigits,x
    and.w #$00FF
    sec
    sbc.w #$0020
    jsr.w DebugDrawChar
    plp
    rts

; @routine 03:D7C1 DebugPrintBinaryByte
; WHAT IT DOES: Prints the byte in A as eight binary digits, the highest
; bit first.
; HOW: Each pair of bits, taken from the low end of $40 by two LSRs,
; becomes a byte $00, $01, $10 or $11 on the stack; the four are then
; printed with DebugHexByte, the last pushed (the highest bits) first. 8-bit
; registers; P saved.
; Entry: A = the byte; D = $0000. Exit: P unchanged; A, X, Y as the last
; DebugDrawChar leaves them (X, Y 8 bits wide when P says so).
%at($03D7C1)
DebugPrintBinaryByte:
    php
    sep.b #$30
    sta.b !debug_bits
    lda.b #$00
    lsr.b !debug_bits
    php
    lsr.b !debug_bits
    rol a
    asl a
    asl a
    asl a
    plp
    rol a
    pha
    lda.b #$00
    lsr.b !debug_bits
    php
    lsr.b !debug_bits
    rol a
    asl a
    asl a
    asl a
    plp
    rol a
    pha
    lda.b #$00
    lsr.b !debug_bits
    php
    lsr.b !debug_bits
    rol a
    asl a
    asl a
    asl a
    plp
    rol a
    pha
    lda.b #$00
    lsr.b !debug_bits
    php
    lsr.b !debug_bits
    rol a
    asl a
    asl a
    asl a
    plp
    rol a
    pha
    pla
    jsr.w DebugHexByte
    pla
    jsr.w DebugHexByte
    pla
    jsr.w DebugHexByte
    pla
    jsr.w DebugHexByte
    plp
    rtl

; The powers of ten DebugPrintDecimalWord takes off, largest first.
%at($03D57A)
DebugPowersOfTen:
    dw 10000, 1000, 100, 10, 1

; Each byte 0-99 as two decimal digits in its nibbles, for DebugDecimalByte.
%at($03D5ED)
DebugDecimalPairs:
    db $00, $01, $02, $03, $04, $05, $06, $07, $08, $09
    db $10, $11, $12, $13, $14, $15, $16, $17, $18, $19
    db $20, $21, $22, $23, $24, $25, $26, $27, $28, $29
    db $30, $31, $32, $33, $34, $35, $36, $37, $38, $39
    db $40, $41, $42, $43, $44, $45, $46, $47, $48, $49
    db $50, $51, $52, $53, $54, $55, $56, $57, $58, $59
    db $60, $61, $62, $63, $64, $65, $66, $67, $68, $69
    db $70, $71, $72, $73, $74, $75, $76, $77, $78, $79
    db $80, $81, $82, $83, $84, $85, $86, $87, $88, $89
    db $90, $91, $92, $93, $94, $95, $96, $97, $98, $99

; The ASCII hexadecimal digits for DebugHexByte.
%at($03D7B1)
DebugHexDigits:
    db "0123456789ABCDEF"
