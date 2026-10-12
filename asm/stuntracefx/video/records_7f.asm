; records_7f.asm: two tables of 4-byte display records in bank $7F, at
; $7F:43D7 and $7F:47FF (with more blocks at $7F:45B7-$7F:4BB7), filled
; from position lists in bank $09 that end with $FF.
; C: src/stuntracefx/video/records_7f.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 09:EFFB Records7FFromF634
; WHAT IT DOES: For each pair (x, y) in the list at $F634 (data bank), up to
; the $FF that ends it, writes a record ($10, x - $28, y - $28, $A0) at
; $7F:43D7 and one ($60, x + $28, y + $28, $F0) at $7F:47FF, four bytes on
; each time.
; HOW: 8-bit arithmetic; the stores in the original's order.
; Entry: M 8-bit. Exit: A low byte = $FF; X, Y past the last record and
; pair; M 8-bit, X 16-bit; Z, C set, N clear; V from the last add.
%at($09EFFB)
Records7FFromF634:
    sep.b #$20
    rep.b #$10
    ldx.w #$0000
    ldy.w #$F634
.lF005:
    lda.w !bank0_base,y
    cmp.b #$FF
    beq .lF04C
    sec
    sbc.b #$28
    sta.l !records_7f_a+1,x
    clc
    adc.b #$50
    sta.l !records_7f_b+1,x
    lda.b #$10
    sta.l !records_7f_a,x
    lda.b #$60
    sta.l !records_7f_b,x
    lda.w !bank0_base+1,y
    sec
    sbc.b #$28
    sta.l !records_7f_a+2,x
    clc
    adc.b #$50
    sta.l !records_7f_b+2,x
    lda.b #$A0
    sta.l !records_7f_a+3,x
    lda.b #$F0
    sta.l !records_7f_b+3,x
    inx
    inx
    inx
    inx
    iny
    iny
    jmp.w .lF005
.lF04C:
    rts

; @routine 09:EF52 Records7FFromF170
; WHAT IT DOES: Writes a record ($18, x, y, $E7) at $7F:43D7 for each pair
; in the list at $F170 (data bank) up to its $FF; then copies eight 32-byte
; blocks from $09:F88B-$09:F96B to $7F:43D7, $45B7, $45D7, $47B7, $47D7,
; $49B7, $49D7, $4BB7, and copies $40 bytes from $09:F849 to $7F:5187 by
; DMA channel 0 through WMDATA.
; HOW: The blocks are copied a byte at a time, all eight per index; the DMA
; enable is a 16-bit store, so HDMAEN gets 0.
; Entry: M 8-bit. Exit: A = 1; X = 9; Y = the low byte of the end of the
; list; M 16-bit, X 8-bit; N, Z clear.
%at($09EF52)
Records7FFromF170:
    sep.b #$20
    rep.b #$10
    ldx.w #$0000
    ldy.w #$F170
.lEF5C:
    lda.w !bank0_base,y
    cmp.b #$FF
    beq .lEF83
    sta.l !records_7f_a+1,x
    lda.b #$18
    sta.l !records_7f_a,x
    lda.w !bank0_base+1,y
    sta.l !records_7f_a+2,x
    lda.b #$E7
    sta.l !records_7f_a+3,x
    inx
    inx
    inx
    inx
    iny
    iny
    jmp.w .lEF5C
.lEF83:
    sep.b #$30
    ldx.b #$00
.lEF87:
    lda.l !records_blocks,x
    sta.l !records_7f_a,x
    lda.l !records_blocks+32,x
    sta.l !records_7f_45b7,x
    lda.l !records_blocks+64,x
    sta.l !records_7f_45d7,x
    lda.l !records_blocks+96,x
    sta.l !records_7f_47b7,x
    lda.l !records_blocks+128,x
    sta.l !records_7f_47d7,x
    lda.l !records_blocks+160,x
    sta.l !records_7f_49b7,x
    lda.l !records_blocks+192,x
    sta.l !records_7f_49d7,x
    lda.l !records_blocks+224,x
    sta.l !records_7f_4bb7,x
    inx
    cpx.b #$20
    bne .lEF87
    ldx.b #$7F
    stx.w !WMADDH
    rep.b #$20
    lda.w #$5187
    sta.w !WMADDL
    ldx.b #$00
    stx.w !DMAP0
    ldx.b #$80
    stx.w !BBAD0
    lda.w #$F849
    sta.w !A1T0L
    ldx.b #$09
    stx.w !A1B0
    lda.w #$0040
    sta.w !DAS0L
    lda.w #$0001
    sta.w !MDMAEN
    rts
