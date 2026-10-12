; tilemap.asm: BG tilemap buffers in work RAM and their uploads.
; C: src/stuntracefx/video/tilemap.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:ED7F TilemapFill7F3497
; WHAT IT DOES: Fills Y words of the tilemap buffer at $7F:3497 + X with A.
; HOW: 16-bit stores, X stepping by 2.
; Entry: M 16-bit, X 16-bit; Y = count (0 means 65536). Exit: X past the
; last word; Y = 0; Z set, N clear.
%at($03ED7F)
TilemapFill7F3497:
    sta.l !tilemap_buffer_7f,x
    inx
    inx
    dey
    bne TilemapFill7F3497
    rts

; @routine 0B:FB1C TilemapFill7F3497B
; WHAT IT DOES: The same fill as TilemapFill7F3497, in bank $0B.
; HOW: As TilemapFill7F3497.
; Entry, exit: as TilemapFill7F3497.
%at($0BFB1C)
TilemapFill7F3497B:
    sta.l !tilemap_buffer_7f,x
    inx
    inx
    dey
    bne TilemapFill7F3497B
    rts

; @routine 03:EDC6 TilemapCopy16To7F39D7
; WHAT IT DOES: Copies 16 words from address A in the data bank to $7F:39D7.
; HOW: A word loop counted in direct page $F1.
; Entry: M 16-bit, X 16-bit, D = $0000; A = source. Exit: A = the last word;
; X = $20; Y = source + $20; $F1 = 0; Z set, N clear.
%at($03EDC6)
TilemapCopy16To7F39D7:
    tay
    ldx.w #$0000
    lda.w #$0010
    sta.b !digit_count
.lEDCF:
    lda.w 0,y
    sta.l !tilemap_buffer_39d7,x
    inx
    inx
    iny
    iny
    dec.b !digit_count
    bne .lEDCF
    rts

; @routine 03:ED89 TilemapRowFromGsuRam
; WHAT IT DOES: Copies 32 tilemap words from Super FX RAM ($70:3000 + A +
; $F1) to the buffer at $93D7 + Y in the data bank, setting bit 13 (priority)
; on each.
; HOW: A word loop counted in direct page $F3.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last word written; X
; and Y advanced by $40; $F3 = 0; Z set, N clear; C, V from the first add.
%at($03ED89)
TilemapRowFromGsuRam:
    clc
    adc.b !digit_count
    tax
    lda.w #$0020
    sta.b !dp_f3
.lED92:
    lda.l !gsu_picture,x
    ora.w #$2000
    sta.w !tilemap_buffer_93d7,y
    inx
    inx
    iny
    iny
    dec.b !dp_f3
    bne .lED92
    rts

; @routine 03:EAF3 VramFillGradient
; WHAT IT DOES: Writes 16 rows of 32 words to VRAM: row r holds A + r, A + r
; + 16, A + r + 32, ... (each word 16 more than the last).
; HOW: Nested loops writing VMDATA; the row start lives in direct page $F1,
; the step ($10) in $F3.
; Entry: M 16-bit, X 8-bit, D = $0000. Exit: A = the last word written + 16;
; X = 0, Y = 0; $F1 = A + 16; $F3 = $10; Z set, N clear; C, V from the last
; add.
%at($03EAF3)
VramFillGradient:
    sta.b !digit_count
    lda.w #$0010
    sta.b !dp_f3
    tay
.lEAFB:
    ldx.b #$20
    lda.b !digit_count
.lEAFF:
    sta.w !VMDATAL
    clc
    adc.b !dp_f3
    dex
    bne .lEAFF
    inc.b !digit_count
    dey
    bne .lEAFB
    rts

; @routine 03:DCC0 VramDmaTileFromGsuRam
; WHAT IT DOES: Copies one 32-byte tile (number A, counted from the offset
; at $00F1) from Super FX RAM to VRAM word address X.
; HOW: Channel 0 in mode 1, bank $70, 32 bytes.
; Entry: M 16-bit, X 16-bit; DB a bank where $00F1 is work RAM. Exit: A = 1;
; M 16-bit; N, Z clear; C, V from the add.
%at($03DCC0)
VramDmaTileFromGsuRam:
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w !digit_count
    sta.w !A1T0L
    stx.w !VMADDL
    lda.w #$1801
    sta.w !DMAP0
    lda.w #$0020
    sta.w !DAS0L
    sep.b #$20
    lda.b #$80
    sta.w !VMAIN
    lda.b #$70
    sta.w !A1B0
    lda.b #$01
    sta.w !MDMAEN
    rep.b #$20
    rts
