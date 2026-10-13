; tilemap.asm: BG tilemap buffers in work RAM and their uploads.
; C: src/stuntracefx/video/tilemap.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

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

; @routine 03:EA76 VramFrameRows2B0
; WHAT IT DOES: Writes the frame rows with tile base $02B0 at the current
; VRAM address.
; HOW: frame_rows; A is saved and restored around it.
; Entry: M 16-bit, D = $0000; the VRAM address and VMAIN set. Exit: A
; unchanged; X = 0; M 16-bit, X 16-bit; N, Z from A; C, V from the last add.
%at($03EA76)
VramFrameRows2B0:
    pha
    rep.b #$30
    lda.w #$02B0
    bra FrameRows

; @routine 03:EA7E VramFrameRows1B0
; WHAT IT DOES: As VramFrameRows2B0, with tile base $01B0.
; HOW: As VramFrameRows2B0.
; Entry: as VramFrameRows2B0. Exit: as VramFrameRows2B0.
%at($03EA7E)
VramFrameRows1B0:
    pha
    rep.b #$30
    lda.w #$01B0
    bra FrameRows

; @routine 03:EA86 VramPictureRows2B0
; WHAT IT DOES: Writes the Super FX picture as tilemap words with tile base
; $02B0 at the current VRAM address.
; HOW: picture_rows; A is saved and restored around it.
; Entry: M 16-bit, D = $0000; the VRAM address and VMAIN set. Exit: A
; unchanged; X = $180; M 16-bit, X 16-bit; N, Z from A; C set; V from the
; last add.
%at($03EA86)
VramPictureRows2B0:
    pha
    rep.b #$30
    ldx.w #$0000
    lda.w #$02B0
    bra PictureRows

; @routine 03:EA91 VramPictureRows1B0
; WHAT IT DOES: As VramPictureRows2B0, with tile base $01B0.
; HOW: As VramPictureRows2B0.
; Entry: as VramPictureRows2B0. Exit: as VramPictureRows2B0.
%at($03EA91)
VramPictureRows1B0:
    pha
    rep.b #$30
    ldx.w #$0000
    lda.w #$01B0
    bra PictureRows

; The frame rows (frame_rows in the C): 32 words of tile $2001 + base, then
; 35 of $2002 + base, or $2010 + base in HUD modes 1 and 3 while $10CD is 0.
; A = base on entry; the caller's A is on the stack.
FrameRows:
    sta.b !dp_f3
    ldx.w #$0020
.lEAA1:
    lda.w #$2001
    clc
    adc.b !dp_f3
    sta.w !VMDATAL
    dex
    bne .lEAA1
    ldx.w #$0023
.lEAB0:
    lda.w !hud_mode
    and.w #$00FF
    cmp.w #$0001
    beq .lEAC5
    cmp.w #$0003
    beq .lEAC5
.lEAC0:
    lda.w #$2002
    bra .lEAD0
.lEAC5:
    lda.w !wram_10CD
    and.w #$00FF
    bne .lEAC0
    lda.w #$2010
.lEAD0:
    clc
    adc.b !dp_f3
    sta.w !VMDATAL
    dex
    bne .lEAB0
    pla
    rts

; The picture rows (picture_rows in the C): the 192 words of the Super FX
; picture, each plus the base in A; the caller's A is on the stack.
PictureRows:
    sta.b !dp_f3
    ldx.w #$0000
.lEAE0:
    lda.l !gsu_picture,x
    clc
    adc.b !dp_f3
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$0180
    bcc .lEAE0
    pla
    rts

; @routine 02:FB27 TilemapRowFrom7FBBD7
; WHAT IT DOES: Copies four words from $7F:BBD7 + X to the tilemap buffer at
; $7F:93D7 + X, and moves X on by $40 (one row of 32 words).
; HOW: Four 16-bit long copies.
; Entry: M 16-bit, X 16-bit; X = byte offset. Exit: A = X = the offset + $40;
; N, Z, C, V from the add.
%at($02FB27)
TilemapRowFrom7FBBD7:
    lda.l !tilemap_7fbbd7,x
    sta.l !tilemap_7f93d7,x
    lda.l !tilemap_7fbbd7+2,x
    sta.l !tilemap_7f93d7+2,x
    lda.l !tilemap_7fbbd7+4,x
    sta.l !tilemap_7f93d7+4,x
    lda.l !tilemap_7fbbd7+6,x
    sta.l !tilemap_7f93d7+6,x
    txa
    clc
    adc.w #$0040
    tax
    rts

; @routine 02:FAFE TilemapRowFromFB4E
; WHAT IT DOES: Copies four words from $FB4E + Y in the data bank to the
; tilemap buffer at $7F:93D7 + X; Y moves on by 8, X by $40.
; HOW: Four 16-bit copies.
; Entry: M 16-bit, X 16-bit; X = byte offset, Y = source offset. Exit: A = X
; = the offset + $40; N, Z, C, V from that add.
%at($02FAFE)
TilemapRowFromFB4E:
    lda.w !tilemap_rows_fb4e,y
    sta.l !tilemap_7f93d7,x
    lda.w !tilemap_rows_fb4e+2,y
    sta.l !tilemap_7f93d7+2,x
    lda.w !tilemap_rows_fb4e+4,y
    sta.l !tilemap_7f93d7+4,x
    lda.w !tilemap_rows_fb4e+6,y
    sta.l !tilemap_7f93d7+6,x
    tya
    clc
    adc.w #$0008
    tay
    txa
    clc
    adc.w #$0040
    tax
    rts

; @routine 07:CEDF TilemapPaletteCycle
; WHAT IT DOES: Every fourth frame, steps the phase at $0DD5 (0 to 5) and
; sets the palette field of Y tilemap entries in the buffer at $7F:93D6,
; starting at entry offset X, to the phase's palette: from the table at
; $07:CF2C when the HUD mode word is 2, else from $07:CF26.
; HOW: Skips unless the low two bits of the frame counter are 0. The
; palette bits go through direct page $F1; each entry's attribute byte keeps
; its bits other than 2-4.
; Entry: M 8-bit, X 16-bit, D = $0000; X = entry offset (bytes), Y = count
; (1 or more). Exit: when skipped, A low byte = the frame counter's low two
; bits; N, Z from them; X, Y unchanged. Otherwise: A = the table byte after
; the phase's and the last attribute byte; X past the last entry; Y = 0; C
; set when the HUD mode word is 2 or more; Z set, N clear. M 8-bit.
%at($07CEDF)
TilemapPaletteCycle:
    sep.b #$20
    lda.w !frame_count
    and.b #$03
    beq .step
    rts
.step:
    lda.w !palette_phase
    inc a
    cmp.b #$06
    bcc .phase
    lda.b #$00
.phase:
    sta.w !palette_phase
    rep.b #$20
    and.w #$00FF
    phx
    tax
    lda.w !hud_mode
    cmp.w #$0002
    beq .b
    lda.l !palette_cycle_a,x
    bra .bits
.b:
    lda.l !palette_cycle_b,x
.bits:
    sep.b #$20
    sta.b !palette_bits
    plx

; @routine 07:CF14 TilemapPaletteFill
; WHAT IT DOES: Sets the palette field (bits 2-4 of the attribute byte) of
; Y tilemap entries in the buffer at $7F:93D6, from entry offset X, to the
; bits in direct page $F1.
; HOW: Read, mask with $E3, OR in $F1, write back; X steps by 2. It is the
; tail of TilemapPaletteCycle, also called on its own.
; Entry: M 8-bit, X 16-bit, D = $0000; X = entry offset (bytes), Y = count
; (1 or more), $F1 = the palette bits. Exit: A low byte = the last
; attribute byte; X past the last entry; Y = 0; Z set, N clear.
TilemapPaletteFill:
    lda.l !cycle_tilemap+1,x
    and.b #$E3
    ora.b !palette_bits
    sta.l !cycle_tilemap+1,x
    inx
    inx
    dey
    bne TilemapPaletteFill
    rts

; @routine 03:EA2E ScreenFrameAndPictures
; WHAT IT DOES: Writes the two framed picture panels to VRAM: at $7800 the
; frame rows (base $02B0), the gradient from $2000 and at $7A40 the picture
; (base $02B0); at $7C00 the frame rows (base $01B0), the gradient from
; $2010 and at $7E40 the picture (base $01B0).
; HOW: VMAIN = $80; VramFrameRows, VramFillGradient and VramPictureRows for
; each panel, setting the VRAM address between them.
; Entry: D = $0000. Exit: A = $7E40; X = $180; P unchanged.
%at($03EA2E)
ScreenFrameAndPictures:
    php
    sep.b #$10
    rep.b #$20
    lda.w #$7800
    sta.w !VMADDL
    ldx.b #$80
    stx.w !VMAIN
    jsr.w VramFrameRows2B0
    sep.b #$10
    rep.b #$20
    lda.w #$2000
    jsr.w VramFillGradient
    rep.b #$20
    lda.w #$7A40
    sta.w !VMADDL
    jsr.w VramPictureRows2B0
    lda.w #$7C00
    sta.w !VMADDL
    jsr.w VramFrameRows1B0
    lda.w #$2010
    sep.b #$10
    rep.b #$20
    jsr.w VramFillGradient
    rep.b #$20
    lda.w #$7E40
    sta.w !VMADDL
    jsr.w VramPictureRows1B0
    plp
    rts

; @routine 03:F47B VramNoiseTiles
; WHAT IT DOES: Fills $400 words of VRAM from address 0 with random tiles
; $20-$3F (from RandomWordStep); first stores $8000 as a word at BG4VOFS,
; which leaves BG4VOFS 0 and sets VMAIN to $80.
; HOW: A 16-bit store to $2114/$2115, then VMDATA writes; $0D18 is the
; random word.
; Entry: any widths. Exit: A = the last word written; X = 0; M 16-bit, X
; 16-bit; Z set, N clear.
%at($03F47B)
VramNoiseTiles:
    rep.b #$30
    lda.w #$8000
    sta.w !BG4VOFS
    lda.w #$0000
    sta.w !VMADDL
    ldx.w #$0400
.lF48C:
    jsl.l RandomWordStep
    lda.w !random_word
    and.w #$001F
    ora.w #$0020
    sta.w !VMDATAL
    dex
    bne .lF48C
    rts

; @routine 7E:B24C TilemapRowsTo5C00
; WHAT IT DOES: Writes 17 tilemap rows of 26 entries from the table at
; $7F:93D7 to VRAM from $5C00 (one 32-entry row apart), then sets BG2's
; scroll from screen band 0.
; HOW: VMAIN $80; for each row the VRAM address, then 26 words to VMDATA
; from $7F:93D7 plus the row offset in direct page $F9 (steps of $40 up to
; $440); the row address rides on the stack. Then Bg2ScrollFromTable with
; X = 0. The PPU is reached through the data bank.
; Entry: X 16-bit, D = $0000. Exit: A low byte = BG2's vertical scroll high
; byte, high byte $5E; X = Y = 0; M 8-bit; N, Z from A low byte; C set, V
; clear.
%ramcode($7EB24C, $010F73)
TilemapRowsTo5C00:
    sep.b #$20
    lda.b #$80
    sta.w !VMAIN
    rep.b #$30
    stz.b !dp_f9
    lda.w #$5C00
    pha
.lB25B:
    pla
    sta.w !VMADDL
    clc
    adc.w #$0020
    pha
    ldx.b !dp_f9
    ldy.w #$001A
.lB269:
    lda.l !tilemap_7f93d7,x
    sta.w !VMDATAL
    inx
    inx
    dey
    bne .lB269
    lda.b !dp_f9
    clc
    adc.w #$0040
    sta.b !dp_f9
    cmp.w #$0440
    bne .lB25B
    pla
    sep.b #$20
    ldx.w #$0000
    jsr.w Bg2ScrollFromTable
    rts

; @routine 02:FA91 TilemapLevelColumns
; WHAT IT DOES: Draws twelve 4-row columns into the tilemap buffer, each
; filled to the level (0-4, more counts as 4) in $0F05-$0F10: the filled
; rows come from $02:FB4E, the empty ones above them from $7F:BBD7.
; HOW: For column i the buffer offset is the word at $02:FB6E + 2i; level n
; takes n rows from TilemapRowFromFB4E starting at source offset (4 - n) *
; 8, then 4 - n rows from TilemapRowFrom7FBBD7. Data bank 2 inside.
; Entry: any widths. Exit: A = the last offset + $40 (16-bit); X = its low
; byte, Y = $0C (with the 8-bit index registers P restores); P and the data
; bank unchanged.
%at($02FA91)
TilemapLevelColumns:
    php
    phb
    phk
    plb
    rep.b #$30
    ldy.w #$0000
.lFA9A:
    tya
    asl a
    tax
    lda.w !level_column_offsets,x
    tax
    lda.w !level_columns,y
    phy
    and.w #$00FF
    beq .lFAE8
    cmp.w #$0002
    bcc .lFAE0
    beq .lFAD5
    cmp.w #$0003
    beq .lFAC7
    ldy.w #$0000
    jsr.w TilemapRowFromFB4E
    jsr.w TilemapRowFromFB4E
    jsr.w TilemapRowFromFB4E
    jsr.w TilemapRowFromFB4E
    bra .lFAF4
.lFAC7:
    ldy.w #$0008
    jsr.w TilemapRowFromFB4E
    jsr.w TilemapRowFromFB4E
    jsr.w TilemapRowFromFB4E
    bra .lFAF1
.lFAD5:
    ldy.w #$0010
    jsr.w TilemapRowFromFB4E
    jsr.w TilemapRowFromFB4E
    bra .lFAEE
.lFAE0:
    ldy.w #$0018
    jsr.w TilemapRowFromFB4E
    bra .lFAEB
.lFAE8:
    jsr.w TilemapRowFrom7FBBD7
.lFAEB:
    jsr.w TilemapRowFrom7FBBD7
.lFAEE:
    jsr.w TilemapRowFrom7FBBD7
.lFAF1:
    jsr.w TilemapRowFrom7FBBD7
.lFAF4:
    ply
    iny
    cpy.w #$000C
    bcc .lFA9A
    plb
    plp
    rts

; @routine 7E:E8F4 TilemapRowToE3D7
; WHAT IT DOES: Copies 32 tilemap entries from $93D7 + X to $E3D7 + Y, or
; from the alternate buffer at $BBD7 + X when bit 14 of $0DA4 is set (no
; script has reached that case yet).
; HOW: A word loop counted down in direct page $F3; buffers through the
; data bank, $0DA4 with a long load.
; Entry: M 16-bit, X 16-bit, D = $0000; X = source offset, Y = destination
; offset. Exit: A = the last word copied; X, Y + $40; N clear, Z set; C, V
; unchanged.
%ramcode($7EE8F4, $01461B)
TilemapRowToE3D7:
    lda.w #$0020
    sta.b !row_count
    lda.l !wram_0DA4
    bit.w #$4000
    bne .alt
.lE902:
    lda.w !tilemap_buffer_93d7,x
    sta.w !tilemap_row_e3d7,y
    inx
    inx
    iny
    iny
    dec.b !row_count
    bne .lE902
    rts
.alt:                               ; not reached in the traces so far
    lda.w !tilemap_alt_bbd7,x
    sta.w !tilemap_row_e3d7,y
    inx
    inx
    iny
    iny
    dec.b !row_count
    bne .alt
    rts

; @routine 7E:E815 BackdropRowsUpdate
; WHAT IT DOES: Updates the scrolling backdrop for the Super FX buffer
; $0D7F selects: sets $0D96 to twice the view angle $06CD plus $42 (at most
; $300), BG2's scroll pair for that buffer from the angle and $06CF, and
; copies 17 rows of 32 entries from the backdrop map at $7F:93D7 into that
; buffer's tilemap ($7F:E3D7, or + $440 for buffer 1), filling
; with $20C0 once the map ends ($2800 bytes).
; HOW: ObjectLinkFlag4000Step first. The map row is ((($06CD + $0DA2) / 8)
; AND $3F) * $140 by the PPU multiplier ($0140 in M7A, the row in M7B and
; M7C), plus ($06CF / 8) AND $FE; rows go through TilemapRowToE3D7 with the
; data bank at $7F and step $140 (17 counted in direct page $F1). The
; vertical scroll is (angle AND 7) - $20 plus the word at $7E:E811 + 2 *
; buffer, the horizontal (($06CF / 2) AND 7) - $18. The clamp of $0D96 has
; not been reached by any script yet.
; Entry: any widths, D = $0000. Exit: A = the next map offset, or $20C0
; after a fill; X, Y past the copy (low bytes when P restores 8-bit index
; registers); P and the data bank unchanged.
%ramcode($7EE815, $01453C)
BackdropRowsUpdate:
    php
    rep.b #$30
    lda.l !gsu_buffer_select
    asl a
    tax
    jsl.l ObjectLinkFlag4000Step
    lda.l !wram_06CD
    pha
    asl a
    adc.w #$0042
    cmp.w #$0300
    bcc .lE833
    lda.w #$0300                    ; not reached in the traces so far
.lE833:
    sta.w !gradient_src_a
    pla
    and.w #$0007
    sec
    sbc.w #$0020
    clc
    adc.l !backdrop_scroll_base,x
    sta.w !bg2_scroll_y,x
    lda.w !view_06cf
    lsr a
    and.w #$0007
    sec
    sbc.w #$0018
    sta.w !bg2_scroll_x,x
    sep.b #$20
    lda.b #$40
    sta.l !M7A
    lda.b #$01
    sta.l !M7A
    rep.b #$20
    lda.w !wram_06CD
    clc
    adc.w !view_0da2
    lsr a
    lsr a
    lsr a
    and.w #$003F
    sta.l !M7B
    lda.l !MPYL
    sta.b !digit_count
    lda.w !view_06cf
    lsr a
    lsr a
    lsr a
    and.w #$00FE
    clc
    adc.b !digit_count
    tax
    sep.b #$20
    phb
    lda.b #$7F
    pha
    plb
    rep.b #$20
    lda.w #$0011
    sta.b !digit_count
    lda.l !gsu_buffer_select
    bne .lE8C8
    ldy.w #$0000
.lE89F:
    cpx.w #$2800
    bcs .lE8B6
    jsr.w TilemapRowToE3D7
    txa
    clc
    adc.w #$0100
    tax
    dec.b !digit_count
    bne .lE89F
    sep.b #$20
    plb
    plp
    rts
.lE8B6:
    lda.w #$20C0
.lE8B9:
    sta.w !tilemap_row_e3d7,y
    iny
    iny
    cpy.w #$0440
    bcc .lE8B9
    sep.b #$20
    plb
    plp
    rts
.lE8C8:
    ldy.w #$0440
.lE8CB:
    cpx.w #$2800
    bcs .lE8E2
    jsr.w TilemapRowToE3D7
    txa
    clc
    adc.w #$0100
    tax
    dec.b !digit_count
    bne .lE8CB
    sep.b #$20
    plb
    plp
    rts
.lE8E2:
    lda.w #$20C0
.lE8E5:
    sta.w !tilemap_row_e3d7,y
    iny
    iny
    cpy.w #$0880
    bcc .lE8E5
    sep.b #$20
    plb
    plp
    rts

; @routine 03:D306 BackdropAnglesFromView
; WHAT IT DOES: Sets the backdrop's scroll from the view position: $06CD
; (the angle BackdropRowsUpdate scrolls by) = -($0664 / 64), kept below $E8
; and at least -$38, plus $98, modulo $200; $06CF = ($0666 / 32) AND $7FF.
; HOW: Arithmetic shifts as CMP #$8000 / ROR A; the two limits are tested
; with CMP and the sign of the difference, as the original does. Work RAM
; through the data bank. The upper limit is not reached in the traces so
; far.
; Entry: any widths. Exit: A = the $06CF value; X, Y unchanged; P
; unchanged except N, Z, which the final pull of Y sets from Y.
%at($03D306)
BackdropAnglesFromView:
    phy
    php
    rep.b #$30
    lda.w !view_position
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
    cmp.w #$8000
    ror a
    eor.w #$FFFF
    inc a
    cmp.w #$00E8
    bmi .low
    lda.w #$00E8                    ; not reached in the traces so far
.low:
    cmp.w #$FFC8
    bpl .wrap
    lda.w #$FFC8
.wrap:
    clc
    adc.w #$0098
    and.w #$01FF
    sta.w !view_angle
    lda.w !view_position+2
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
    and.w #$07FF
    sta.w !view_06cf
    sep.b #$20
    plp
    ply
    rtl

; @routine 07:C293 TilemapPaletteCycle87
; WHAT IT DOES: TilemapPaletteCycle on nine entries from entry offset $87.
; HOW: Sets X and Y and calls it.
; Entry: M 8-bit, D = $0000. Exit: as TilemapPaletteCycle, with X 16-bit;
; when skipped, X = $87 and Y = 9.
%at($07C293)
TilemapPaletteCycle87:
    rep.b #$10
    ldx.w #$0087
    ldy.w #$0009
    jsr.w TilemapPaletteCycle
    rts

; @routine 07:C29F TilemapPaletteCycle107
; WHAT IT DOES: TilemapPaletteCycle on 13 entries from entry offset $107.
; HOW: Sets X and Y and calls it.
; Entry: M 8-bit, D = $0000. Exit: as TilemapPaletteCycle, with X 16-bit;
; when skipped, X = $107 and Y = 13.
%at($07C29F)
TilemapPaletteCycle107:
    rep.b #$10
    ldx.w #$0107
    ldy.w #13
    jsr.w TilemapPaletteCycle
    rts

; @routine 07:C2AB TilemapPaletteCycle187
; WHAT IT DOES: TilemapPaletteCycle on 10 entries from entry offset $187.
; HOW: Sets X and Y and calls it.
; Entry: M 8-bit, D = $0000. Exit: as TilemapPaletteCycle, with X 16-bit;
; when skipped, X = $187 and Y = 10.
%at($07C2AB)
TilemapPaletteCycle187:
    rep.b #$10
    ldx.w #$0187
    ldy.w #10
    jsr.w TilemapPaletteCycle
    rts

; @routine 07:C2B7 TilemapRow87Palette5
; WHAT IT DOES: Gives 9 tilemap entries from offset $87 palette 5, then
; the fourth of them ($7F:9464) palette 4.
; HOW: TilemapPaletteFill with $F1 = $14, then one masked store.
; Entry: M 8-bit, D = $0000. Exit: A low byte = the marked attribute byte;
; N, Z from it; X = $0099; Y = 0; X 16-bit.
%at($07C2B7)
TilemapRow87Palette5:
    rep.b #$10
    ldx.w #$0087
    ldy.w #9
    lda.b #$14
    sta.b !palette_bits
    jsr.w TilemapPaletteFill
    lda.l !cycle_tilemap+142
    and.b #$E3
    ora.b #$10
    sta.l !cycle_tilemap+142
    rts

; @routine 07:C2D3 TilemapRow107Palette5
; WHAT IT DOES: Gives 13 tilemap entries from offset $107 palette 5, then
; the seventh of them ($7F:94EA) palette 4.
; HOW: TilemapPaletteFill with $F1 = $14, then one masked store.
; Entry: M 8-bit, D = $0000. Exit: A low byte = the marked attribute byte;
; N, Z from it; X = $0121; Y = 0; X 16-bit.
%at($07C2D3)
TilemapRow107Palette5:
    rep.b #$10
    ldx.w #$0107
    ldy.w #13
    lda.b #$14
    sta.b !palette_bits
    jsr.w TilemapPaletteFill
    lda.l !cycle_tilemap+276
    and.b #$E3
    ora.b #$10
    sta.l !cycle_tilemap+276
    rts

; @routine 07:C2EF TilemapRow187Palette5
; WHAT IT DOES: Gives 10 tilemap entries from offset $187 palette 5, then
; the seventh of them ($7F:956A) palette 4.
; HOW: As TilemapRow107Palette5.
; Entry: M 8-bit, D = $0000. Exit: A low byte = the marked attribute byte;
; N, Z from it; X = $019B; Y = 0; X 16-bit.
%at($07C2EF)
TilemapRow187Palette5:
    rep.b #$10
    ldx.w #$0187
    ldy.w #10
    lda.b #$14
    sta.b !palette_bits
    jsr.w TilemapPaletteFill
    lda.l !cycle_tilemap+404
    and.b #$E3
    ora.b #$10
    sta.l !cycle_tilemap+404
    rts

; @routine 07:CE97 TilemapRow187Palette4
; WHAT IT DOES: Gives 9 tilemap entries from offset $187 palette 4, then
; the fourth of them ($7F:9564) palette 0.
; HOW: TilemapPaletteFill with $F1 = $10, then one masked store.
; Entry: M 8-bit, X 16-bit, D = $0000. Exit: A low byte = the marked
; attribute byte; N, Z from it; X = $0199; Y = 0.
%at($07CE97)
TilemapRow187Palette4:
    ldx.w #$0187
    ldy.w #9
    lda.b #$10
    sta.b !palette_bits
    jsr.w TilemapPaletteFill
    lda.l !cycle_tilemap+398
    and.b #$E3
    sta.l !cycle_tilemap+398
    rts

; @routine 07:CEAF TilemapRow207Palette4
; WHAT IT DOES: Gives 13 tilemap entries from offset $207 palette 4, then
; the seventh of them ($7F:95EA) palette 0.
; HOW: As TilemapRow187Palette4.
; Entry: M 8-bit, X 16-bit, D = $0000. Exit: A low byte = the marked
; attribute byte; N, Z from it; X = $0221; Y = 0.
%at($07CEAF)
TilemapRow207Palette4:
    ldx.w #$0207
    ldy.w #13
    lda.b #$10
    sta.b !palette_bits
    jsr.w TilemapPaletteFill
    lda.l !cycle_tilemap+532
    and.b #$E3
    sta.l !cycle_tilemap+532
    rts

; @routine 07:CEC7 TilemapRow287Palette4
; WHAT IT DOES: Gives 10 tilemap entries from offset $287 palette 4, then
; the seventh of them ($7F:966A) palette 0.
; HOW: As TilemapRow187Palette4.
; Entry: M 8-bit, X 16-bit, D = $0000. Exit: A low byte = the marked
; attribute byte; N, Z from it; X = $029B; Y = 0.
%at($07CEC7)
TilemapRow287Palette4:
    ldx.w #$0287
    ldy.w #10
    lda.b #$10
    sta.b !palette_bits
    jsr.w TilemapPaletteFill
    lda.l !cycle_tilemap+660
    and.b #$E3
    sta.l !cycle_tilemap+660
    rts

; @routine 07:CFA3 DigitTiles1184
; WHAT IT DOES: Writes the three digits at $1184-$1186 as tilemap words
; at $7F:9481-$7F:9486: tile $CA + digit, priority bit set ($2000).
; HOW: Each digit read 8-bit, then a 16-bit add and OR.
; Entry: any widths. Exit: A = $20CA + the last digit; M 8-bit; N, Z from
; the OR; C, V from the add.
%at($07CFA3)
DigitTiles1184:
    sep.b #$20
    lda.l !wram_1184+0
    rep.b #$20
    and.w #$00FF
    clc
    adc.w #$00CA
    ora.w #$2000
    sta.l !digit_tiles_9481+0
    sep.b #$20
    lda.l !wram_1184+1
    rep.b #$20
    and.w #$00FF
    clc
    adc.w #$00CA
    ora.w #$2000
    sta.l !digit_tiles_9481+2
    sep.b #$20
    lda.l !wram_1184+2
    rep.b #$20
    and.w #$00FF
    clc
    adc.w #$00CA
    ora.w #$2000
    sta.l !digit_tiles_9481+4
    sep.b #$20
    rts

; @routine 07:D029 TimesKeepAndAdvance
; WHAT IT DOES: Keeps $0EE7-$0EE9 at $0EEA-$0EEC, clears $0E3F, moves the
; stage $0D59 on, sets $0D23 = 2, clears $19DF and starts the window
; closing ($19D8 = 2).
; HOW: 8-bit stores through the data bank.
; Entry: any widths. Exit: A low byte = 2; M 8-bit; N, Z clear.
%at($07D029)
TimesKeepAndAdvance:
    sep.b #$20
    lda.w !wram_0EE7
    sta.w !wram_0EEA
    lda.w !wram_0EE7+1
    sta.w !wram_0EEA+1
    lda.w !wram_0EE7+2
    sta.w !wram_0EEA+2
    lda.b #$00
    sta.w !wram_0E3F
    inc.w !wram_0D59
    lda.b #$02
    sta.w !wram_0D23
    sep.b #$20
    stz.w !wram_19DF
    lda.b #$02
    sta.w !window_hdma_off
    rts
