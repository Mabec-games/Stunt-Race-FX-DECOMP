; oam.asm: the work-RAM copy of the sprite table (OAM), copied to the PPU each
; frame. Each sprite is four bytes: X, Y, tile, attributes.
; C: src/stuntracefx/video/oam.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:C4D2 OamHideRange
; WHAT IT DOES: Hides a run of sprites by moving them below the screen.
; HOW: Stores Y = $F0 into each sprite's Y byte, starting at the sprite whose
; table offset is X, for the given count.
; Entry: M 8-bit, X 16-bit; X = first sprite * 4, Y = count (1 or more).
; Exit: A low byte = $F0; X past the last sprite; Y = 0; Z set, N clear.
%at($07C4D2)
OamHideRange:
    lda.b #$F0
.next:
    sta.l !oam_buffer+1,x
    inx
    inx
    inx
    inx
    dey
    bne .next
    rts

; @routine 7E:ADFD OamWriteSprites36To46
; WHAT IT DOES: Copies sprites 36 to 46 from the work-RAM table straight to
; OAM.
; HOW: OAM word address $48 (sprite 36), then the 44 bytes from table offset
; $90 written to OAMDATA one by one. Runs from work RAM.
; Entry: M 8-bit, DB a bank where the table is work RAM. Exit: X = $BC
; (16-bit); Y = 0; A low byte = the last byte written; Z set, N clear.
%ramcode($7EADFD, $010B24)
OamWriteSprites36To46:
    rep.b #$10
    ldx.w #$0090
    lda.b #$48
    sta.w !OAMADDL
    stz.w !OAMADDH
    ldy.w #$002C
.next:
    lda.w !oam_buffer,x
    sta.w !OAMDATA
    inx
    dey
    bne .next
    rts

; @routine 7E:B005 SpriteYOnScreen
; WHAT IT DOES: Returns a sprite Y coordinate, or $F0 (hidden) when it is $B0
; or more, below the part of the screen the sprites may cover.
; HOW: Compares with $B0. Runs from work RAM.
; Entry: M 8-bit; A = Y. Exit: A low byte = the Y to use; C set when it was
; replaced; N, Z from the compare, or from $F0.
%ramcode($7EB005, $010D2C)
SpriteYOnScreen:
    cmp.b #$B0
    bcc .keep
    lda.b #$F0
.keep:
    rts

; @routine 7F:099C SpriteStrip4
; WHAT IT DOES: Places a horizontal strip of four sprites, 16 pixels apart,
; from the position in direct page $02, with the tiles of the set A selects.
; HOW: The set is (A / 2) rounded down to a multiple of 8 bytes in the table
; after SpriteStrip4; each sprite takes the next X and tile word. Direct page
; $02 is restored. Runs from work RAM bank $7F.
; Entry: M 16-bit, X 16-bit, D = $0000; A = set selector, Y = first sprite
; offset. Exit: A = the position; X = the table offset + 8; Y + 16; P
; unchanged.
%ramcode($7F099C, $05819C)
SpriteStrip4:
    php
    rep.b #$30
    lsr a
    and.w #$00F8
    tax
    lda.b !dp_02
    pha
    lda.w #$0003
    sta.b !dp_08
.next:
    lda.b !dp_02
    sta.w !oam_buffer,y
    clc
    adc.w #$0010
    sta.b !dp_02
    lda.l SpriteStrip4Tiles,x
    sta.w !oam_buffer+2,y
    inx
    inx
    iny
    iny
    iny
    iny
    dec.b !dp_08
    bpl .next
    pla
    sta.b !dp_02
    plp
    rts
SpriteStrip4Tiles:

; @routine 7F:0A35 SpriteStrip6
; WHAT IT DOES: Places a horizontal strip of six sprites, 8 pixels apart, from
; the position in direct page $02, with the tiles of the set A selects.
; HOW: The set is (A AND $38) * 2 bytes into the table after SpriteStrip6.
; Entry: as SpriteStrip4. Exit: A = the position; X = the table offset + 12;
; Y + 24; P unchanged.
%ramcode($7F0A35, $058235)
SpriteStrip6:
    php
    rep.b #$30
    and.w #$0038
    asl a
    tax
    lda.b !dp_02
    pha
    lda.w #$0005
    sta.b !dp_08
.next:
    lda.b !dp_02
    sta.w !oam_buffer,y
    clc
    adc.w #$0008
    sta.b !dp_02
    lda.l SpriteStrip6Tiles,x
    sta.w !oam_buffer+2,y
    inx
    inx
    iny
    iny
    iny
    iny
    dec.b !dp_08
    bpl .next
    pla
    sta.b !dp_02
    plp
    rts
SpriteStrip6Tiles:

; @routine 03:ECD1 OamBufferClear
; WHAT IT DOES: Moves every sprite in the sprite table copy off screen at
; X = Y = $F0 and clears the high table (small size, X bit 8 clear).
; HOW: Word stores of $F0F0 to the X/Y pair of entries 127 down to 0, then
; word stores of zero over the 32-byte high table from the end.
; Entry: any widths. Exit: A = 0; X = $FFFE ($FE when X is 8-bit); P
; unchanged.
%at($03ECD1)
OamBufferClear:
    php
    rep.b #$30
    lda.w #$F0F0
    ldx.w #$01FC
.lECDA:
    sta.l !oam_buffer,x
    dex
    dex
    dex
    dex
    bpl .lECDA
    lda.w #$0000
    ldx.w #$001E
.lECEA:
    sta.l !oam_buffer_high,x
    dex
    dex
    bpl .lECEA
    plp
    rtl

; @routine 03:FEF9 OamBufferHideLarge
; WHAT IT DOES: Moves every sprite below the screen (Y = $F0) and sets the
; high table to $AA: large size, X bit 8 clear, for all 128.
; HOW: Byte stores of $F0 to each entry's Y, then $AA to each high table
; byte, both counting up.
; Entry: any widths. Exit: A low byte = $AA; X = $20; P
; unchanged.
%at($03FEF9)
OamBufferHideLarge:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$F0
    ldx.w #$0000
.lFF03:
    sta.w !oam_buffer+1,x
    inx
    inx
    inx
    inx
    cpx.w #$0200
    bcc .lFF03
    ldx.w #$0000
    lda.b #$AA
.lFF14:
    sta.w !oam_buffer_high,x
    inx
    cpx.w #$0020
    bcc .lFF14
    plp
    rts

; @routine 03:E9FE OamFromList
; WHAT IT DOES: Writes sprite entries from the list at $F883 + Y in the data
; bank into the sprite table copy at $0375 + X, until an $FF byte.
; HOW: Each list item is either $FE, one byte, which hides the entry by
; storing $FE as its Y, or four bytes (X, Y, tile, attributes) copied as
; they are. X steps four per entry.
; Entry: M 8-bit, X 16-bit; X = sprite table offset, Y = list offset. Exit:
; X, Y past the last entry and item; A low byte = $FF; Z, C set, N clear.
%at($03E9FE)
OamFromList:
    lda.w !oam_lists,y
    cmp.b #$FF
    beq .lEA2D
    cmp.b #$FE
    bne .lEA0E
    sta.w !oam_buffer+1,x
    bra .lEA26
.lEA0E:
    sta.w !oam_buffer,x
    lda.w !oam_lists+1,y
    sta.w !oam_buffer+1,x
    lda.w !oam_lists+2,y
    sta.w !oam_buffer+2,x
    lda.w !oam_lists+3,y
    sta.w !oam_buffer+3,x
    iny
    iny
    iny
.lEA26:
    iny
    inx
    inx
    inx
    inx
    bra OamFromList
.lEA2D:
    rts

; @routine 02:FB8A OamSprite0AddY
; WHAT IT DOES: Moves sprite 0 of the sprite table copy down by A lines.
; HOW: An 8-bit add to its Y byte ($0376) through the data bank.
; Entry: M 8-bit; A = lines. Exit: A low byte = the new Y; N, Z, C, V from
; the add.
%at($02FB8A)
OamSprite0AddY:
    clc
    adc.w !oam_buffer+1
    sta.w !oam_buffer+1
    rts

; @routine 02:FB92 OamAttr36x4
; WHAT IT DOES: Gives 4 sprites from entry A attribute byte $36.
; HOW: Byte stores to each entry's attributes ($0378 + 4n) through the data
; bank; the loop is OamAttr38x7's.
; Entry: M 16-bit, X 16-bit; A = first entry. Exit: A = (A * 4) with $36 as
; its low byte; X past the last entry; Y = 0; M 16-bit; Z set, N clear; C
; from the doubling.
%at($02FB92)
OamAttr36x4:
    asl a
    asl a
    tax
    ldy.w #$0004
    sep.b #$20
    lda.b #$36
    bra OamAttrFillLoop

; @routine 02:FBC2 OamAttr38x6
; WHAT IT DOES: Gives 6 sprites from entry A attribute byte $38.
; HOW: As OamAttr36x4.
; Entry: as OamAttr36x4. Exit: as OamAttr36x4, low byte $38.
%at($02FBC2)
OamAttr38x6:
    asl a
    asl a
    tax
    ldy.w #$0006
    sep.b #$20
    lda.b #$38
    bra OamAttrFillLoop

; @routine 02:FBCE OamAttr38x7
; WHAT IT DOES: Gives 7 sprites from entry A attribute byte $38.
; HOW: As OamAttr36x4; the loop at $02:FBD8 is shared with the two above.
; Entry: as OamAttr36x4. Exit: as OamAttr36x4, low byte $38.
%at($02FBCE)
OamAttr38x7:
    asl a
    asl a
    tax
    ldy.w #$0007
    sep.b #$20
    lda.b #$38
OamAttrFillLoop:                    ; shared with OamAttr36x4, 36x6, 38x4 and 38x6
    sta.w !oam_buffer+3,x
    inx
    inx
    inx
    inx
    dey
    bne OamAttrFillLoop
    rep.b #$20
    rts

; @routine 02:FC83 OamHideEntries3And9
; WHAT IT DOES: Hides the sprites at entries 3 and 9 after offset Y.
; HOW: Stores $F000 (X = 0, Y = $F0) to their position words ($0381 + Y,
; $0399 + Y) through the data bank.
; Entry: M 16-bit; Y = byte offset. Exit: A = $F000; N set, Z clear.
%at($02FC83)
OamHideEntries3And9:
    lda.w #$F000
    sta.w !oam_buffer+12,y
    sta.w !oam_buffer+36,y
    rts

; @routine 02:FC8D OamHideEntries0To2
; WHAT IT DOES: Hides the three sprites from offset Y.
; HOW: As OamHideEntries3And9, at $0375, $0379 and $037D + Y.
; Entry: as OamHideEntries3And9. Exit: as OamHideEntries3And9.
%at($02FC8D)
OamHideEntries0To2:
    lda.w #$F000
    sta.w !oam_buffer,y
    sta.w !oam_buffer+4,y
    sta.w !oam_buffer+8,y
    rts

; @routine 04:E317 OamSetPalette
; WHAT IT DOES: Sets the palette bits (1-3) of the attribute byte of X
; sprites from offset Y to the bits in A.
; HOW: The bits are kept in direct page $F1; each attribute byte ($0378 + Y,
; through the data bank) keeps its other bits.
; Entry: X 16-bit, D = $0000; A low byte = palette bits, X = count (0 means
; 65536), Y = byte offset. Exit: A low byte = the last attribute; X = 0; Y
; past the last sprite; M 8-bit; Z set, N clear.
%at($04E317)
OamSetPalette:
    sep.b #$20
    sta.b !digit_count
.lE31B:
    lda.w !oam_buffer+3,y
    and.b #$F1
    ora.b !digit_count
    sta.w !oam_buffer+3,y
    iny
    iny
    iny
    iny
    dex
    bne .lE31B
    rts

; @routine 07:C33A OamBlock52Left
; WHAT IT DOES: Places sprites 52 and 53 at X = $50 and $72 and gives
; sprites 54-56 the attribute bytes $0C, $0C, $06.
; HOW: Byte stores through the data bank (oam_block_52).
; Entry: M 8-bit. Exit: A low byte = $06; X = $10 (16-bit); N, Z clear; C,
; V from the add.
%at($07C33A)
OamBlock52Left:
    rep.b #$10
    ldx.w #$0010
    lda.b #$50
    sta.w !oam_sprite52_x-16,x
    clc
    adc.b #$22
    sta.w !oam_sprite53_x-16,x
    lda.b #$0C
    sta.w !oam_sprite54_attr-16,x
    sta.w !oam_sprite55_attr-16,x
    lda.b #$06
    sta.w !oam_sprite56_attr-16,x
    rts

; @routine 07:C36A OamBlock52Right
; WHAT IT DOES: Places sprites 52 and 53 at X = $87 and $A1 and gives
; sprites 54-56 the attribute bytes $06, $06, $0C.
; HOW: As OamBlock52Left.
; Entry: as OamBlock52Left. Exit: as OamBlock52Left, A low byte = $0C.
%at($07C36A)
OamBlock52Right:
    rep.b #$10
    ldx.w #$0010
    lda.b #$87
    sta.w !oam_sprite52_x-16,x
    clc
    adc.b #$1A
    sta.w !oam_sprite53_x-16,x
    lda.b #$06
    sta.w !oam_sprite54_attr-16,x
    sta.w !oam_sprite55_attr-16,x
    lda.b #$0C
    sta.w !oam_sprite56_attr-16,x
    rts

; @routine 07:C572 OamColumn8
; WHAT IT DOES: Fills eight sprites from offset X: position words from
; direct page $F1 (stepping $10), tile words from $F3 (stepping 2).
; HOW: 16-bit stores; $F1 and $F3 are left past the last values.
; Entry: M 16-bit, X 16-bit, D = $0000; X = byte offset. Exit: A = the last
; $F3; X past the last sprite; Y = 0; Z set, N clear; C, V from the last
; add.
%at($07C572)
OamColumn8:
    ldy.w #$0008
.lC575:
    lda.b !digit_count
    sta.w !oam_buffer,x
    clc
    adc.w #$0010
    sta.b !digit_count
    lda.b !dp_f3
    sta.w !oam_buffer+2,x
    clc
    adc.w #$0002
    sta.b !dp_f3
    inx
    inx
    inx
    inx
    dey
    bne .lC575
    rts

; @routine 07:C50E OamHighBits58
; WHAT IT DOES: Sets $0E15 to $F8 and turns on size or X-high bits in high
; table bytes $16-$19 (sprites 88-103): $40, $55, $55, $15.
; HOW: Byte ORs through the data bank.
; Entry: any widths. Exit: A low byte = the last byte; X = $10 (high byte
; clear); Y high byte clear; P unchanged.
%at($07C50E)
OamHighBits58:
    php
    sep.b #$30
    lda.b #$F8
    sta.w !wram_0E15
    ldx.b #$10
OamHighBitsOr:                          ; OamHighBits18 joins here with X = 0
    lda.w !oam_high_16-16,x
    ora.b #$40
    sta.w !oam_high_16-16,x
    lda.w !oam_high_16-15,x
    ora.b #$55
    sta.w !oam_high_16-15,x
    lda.w !oam_high_16-14,x
    ora.b #$55
    sta.w !oam_high_16-14,x
    lda.w !oam_high_16-13,x
    ora.b #$15
    sta.w !oam_high_16-13,x
    plp
    rts

; @routine 03:FEF5 OamBufferHideLargeLong
; WHAT IT DOES: OamBufferHideLarge for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as OamBufferHideLarge. Exit: as OamBufferHideLarge.
%at($03FEF5)
OamBufferHideLargeLong:
    jsr.w OamBufferHideLarge
    rtl

; @routine 04:E2EC OamPaletteFromTableE30F
; WHAT IT DOES: Sets the palette bits of 22 sprites from entry $0D79 to the
; byte of the table at $04:E30F for $0D48's low byte.
; HOW: OamSetPalette with X = $16, Y = $0D79 * 4.
; Entry: any widths, D = $0000. Exit: A = the last attribute byte (high
; byte clear); X = 0; Y past the last sprite; P unchanged.
%at($04E2EC)
OamPaletteFromTableE30F:
    php
    rep.b #$30
    lda.w !wram_0D79
    asl a
    asl a
    tay
    lda.w !wram_0D48
    and.w #$00FF
    tax
    lda.l !palette_bits_e30f,x
    and.w #$00FF
    ldx.w #$0016
    jsr.w OamSetPalette
    plp
    rtl

; @routine 04:E23F OamPaletteFromTableE26A
; WHAT IT DOES: Sets the palette bits of 22 sprites from entry $0D7B + 13 to
; $0A when $10D8 is 0, otherwise to the byte of the table at $04:E26A for
; ($10D8 - 1)'s low byte.
; HOW: OamSetPalette with X = $16.
; Entry: any widths, D = $0000. Exit: as OamPaletteFromTableE30F.
%at($04E23F)
OamPaletteFromTableE26A:
    php
    rep.b #$30
    lda.w !wram_0D7B
    clc
    adc.w #$000D
    asl a
    asl a
    tay
    lda.w !wram_10D8
    bne .lE256
    lda.w #$000A
    bra .lE262
.lE256:
    dec a
    and.w #$00FF
    tax
    lda.l !palette_bits_e26a,x
    and.w #$00FF
.lE262:
    ldx.w #$0016
    jsr.w OamSetPalette
    plp
    rtl

; @routine 04:E26E OamPaletteFromTableE26AByMode
; WHAT IT DOES: Sets the palette bits of 18 sprites from entry $0D79 to the
; byte of the table at $04:E26A for $0D48's low byte.
; HOW: OamSetPalette with X = $12.
; Entry: any widths, D = $0000. Exit: A = the last attribute byte (high
; byte clear); X = 0; Y past the last sprite; P unchanged.
%at($04E26E)
OamPaletteFromTableE26AByMode:
    php
    rep.b #$30
    lda.w !wram_0D79
    asl a
    asl a
    tay
    lda.w !wram_0D48
    and.w #$00FF
    tax
    lda.l !palette_bits_e26a,x
    and.w #$00FF
    ldx.w #$0012
    jsr.w OamSetPalette
    plp
    rtl

; @routine 04:E28D OamPalettesE28D
; WHAT IT DOES: Sets sprite palette bits in three runs from entry $0D75: 18
; sprites from the table at $04:E30B for $0D48's low byte, then 6 sprites
; from entry + $16 and 6 from entry + $1C from the table at $04:E313 for
; the bytes at $0D89 and $0D8A.
; HOW: OamSetPalette three times.
; Entry: any widths, D = $0000. Exit: A = the last attribute byte (high
; byte clear); X = 0; Y past the last sprite; P unchanged.
%at($04E28D)
OamPalettesE28D:
    php
    rep.b #$30
    lda.w !wram_0D75
    asl a
    asl a
    tay
    lda.w !wram_0D48
    and.w #$00FF
    tax
    lda.l !palette_bits_e30b,x
    and.w #$00FF
    ldx.w #$0012
    jsr.w OamSetPalette
    rep.b #$30
    lda.w !wram_0D75
    clc
    adc.w #$0016
    asl a
    asl a
    tay
    lda.w !wram_0D89
    and.w #$00FF
    tax
    lda.l !palette_bits_e313,x
    and.w #$00FF
    ldx.w #$0006
    jsr.w OamSetPalette
    rep.b #$30
    lda.w !wram_0D75
    clc
    adc.w #$001C
    asl a
    asl a
    tay
    lda.w !wram_0D8A
    and.w #$00FF
    tax
    lda.l !palette_bits_e313,x
    and.w #$00FF
    ldx.w #$0006
    jsr.w OamSetPalette
    plp
    rtl


; @routine 07:C45D OamHide36And96
; WHAT IT DOES: Hides sprites 36-56 and 96-111, and clears the size and
; X-high bits of sprites 96-111 (high table bytes $18-$1B).
; HOW: OamHideRange twice, then four byte stores of 0 through the data bank.
; Entry: any widths. Exit: A low byte = 0; X = $01C0 ($C0 when X is 8-bit);
; Y = 0; P unchanged.
%at($07C45D)
OamHide36And96:
    php
    sep.b #$20
    rep.b #$10
    ldx.w #4*36
    ldy.w #21
    jsr.w OamHideRange
    ldx.w #4*96
    ldy.w #16
    jsr.w OamHideRange
    lda.b #$00
    sta.w !oam_buffer_high+24
    sta.w !oam_buffer_high+25
    sta.w !oam_buffer_high+26
    sta.w !oam_buffer_high+27
    plp
    rts

; @routine 07:C4A4 OamHide75Up
; WHAT IT DOES: Sets $0E0C and $0E14 to $F8 and hides sprites 75-127; when
; the HUD mode ($0D2B) is nonzero sprites 87-95 are left alone.
; HOW: OamHideRange from sprite 75: 53 sprites, or 12 then 32 from sprite
; 96 when the mode is set.
; Entry: any widths. Exit: A low byte = $F0; X = $0200 (0 when X is
; 8-bit); Y = 0; P unchanged.
%at($07C4A4)
OamHide75Up:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$F8
    sta.w !wram_0E0C
    sta.w !wram_0E14
    lda.w !hud_mode
    beq .all
    ldx.w #4*75
    ldy.w #12
    jsr.w OamHideRange
    ldx.w #4*96
    ldy.w #32
    bra .hide
.all:
    ldx.w #4*75
    ldy.w #53
.hide:
    jsr.w OamHideRange
    plp
    rts

; @routine 07:C4F1 OamHighBits58Park112
; WHAT IT DOES: OamHighBits58, then sets the X-high bit of sprites 112-127
; (high table bytes $1C-$1F = $55), which puts them off the left edge.
; HOW: Two word stores of $5555 to $0591 and $0593 through the data bank.
; Entry: any widths. Exit: A = $5555; X = $10 (high byte clear); Y high
; byte clear; P unchanged.
%at($07C4F1)
OamHighBits58Park112:
    php
    jsr.w OamHighBits58
    rep.b #$20
    lda.w #$5555
    sta.w !oam_buffer_high+28
    sta.w !oam_buffer_high+30
    plp
    rts

; @routine 07:C502 OamHighBits18
; WHAT IT DOES: Sets $0E14 to $F8 and turns on size or X-high bits in high
; table bytes 6-9 (sprites 24-39): $40, $55, $55, $15.
; HOW: Enters OamHighBits58's OR sequence with X = 0 instead of $10.
; Entry: any widths. Exit: A low byte = the last byte; X = 0; Y high byte
; clear; P unchanged.
%at($07C502)
OamHighBits18:
    php
    sep.b #$30
    lda.b #$F8
    sta.w !wram_0E14
    ldx.b #$00
    bra OamHighBitsOr

; @routine 07:C388 OamLargeColumns
; WHAT IT DOES: Lays out three columns of eight large sprites: sprites 40-47
; at $4840 with tiles from $0660, 96-103 at $5840 from $0680, 104-111 at
; $6840 from $06A0; positions step down $10 and tile words by 2. Sets the
; size bit of all 24 (high table bytes 10, 11 and 24-27 = $AA).
; HOW: OamColumn8 three times with its inputs in direct page $F1 and $F3,
; then byte stores through the data bank.
; Entry: any widths, D = $0000. Exit: A = $06AA; X = $01C0 ($C0 when X is
; 8-bit); Y = 0; P unchanged.
%at($07C388)
OamLargeColumns:
    php
    rep.b #$30
    lda.w #$4840
    sta.b !column_pos
    lda.w #$0660
    sta.b !column_tile
    ldx.w #4*40
    jsr.w OamColumn8
    lda.w #$5840
    sta.b !column_pos
    lda.w #$0680
    sta.b !column_tile
    ldx.w #4*96
    jsr.w OamColumn8
    lda.w #$6840
    sta.b !column_pos
    lda.w #$06A0
    sta.b !column_tile
    ldx.w #4*104
    jsr.w OamColumn8
    sep.b #$20
    lda.b #$AA
    sta.w !oam_buffer_high+10
    lda.b #$AA
    sta.w !oam_buffer_high+11
    lda.b #$AA
    sta.w !oam_buffer_high+24
    lda.b #$AA
    sta.w !oam_buffer_high+25
    lda.b #$AA
    sta.w !oam_buffer_high+26
    lda.b #$AA
    sta.w !oam_buffer_high+27
    plp
    rts

; @routine 07:C3DD OamPlaceRows75And112
; WHAT IT DOES: Sets sprite Y positions: sprites 75-86 to $93, 112-113 to
; $70, 114-115 to $80, and, unless $10CD is set, 116-127 to $8C or $94
; (119, 121, 123, 125 and 127 at $94; the rest at $8C). Clears the size and
; X-high bits of sprites 40-47 and 96-111 (high table bytes 10, 11, 24-27).
; HOW: Byte stores through the data bank.
; Entry: any widths. Exit: A low byte = 0; X, Y unchanged; P unchanged.
%at($07C3DD)
OamPlaceRows75And112:
    php
    sep.b #$20
    lda.w !wram_10CD
    bne .rows
    lda.b #$8C
    sta.w !oam_buffer+(4*116)+1
    sta.w !oam_buffer+(4*117)+1
    sta.w !oam_buffer+(4*118)+1
    sta.w !oam_buffer+(4*120)+1
    sta.w !oam_buffer+(4*122)+1
    sta.w !oam_buffer+(4*124)+1
    sta.w !oam_buffer+(4*126)+1
    lda.b #$94
    sta.w !oam_buffer+(4*119)+1
    sta.w !oam_buffer+(4*121)+1
    sta.w !oam_buffer+(4*123)+1
    sta.w !oam_buffer+(4*125)+1
    sta.w !oam_buffer+(4*127)+1
.rows:
    lda.b #$93
    sta.w !oam_buffer+(4*75)+1
    sta.w !oam_buffer+(4*76)+1
    sta.w !oam_buffer+(4*77)+1
    sta.w !oam_buffer+(4*78)+1
    sta.w !oam_buffer+(4*79)+1
    sta.w !oam_buffer+(4*80)+1
    sta.w !oam_buffer+(4*81)+1
    sta.w !oam_buffer+(4*82)+1
    sta.w !oam_buffer+(4*83)+1
    sta.w !oam_buffer+(4*84)+1
    sta.w !oam_buffer+(4*85)+1
    sta.w !oam_buffer+(4*86)+1
    lda.b #$70
    sta.w !oam_buffer+(4*112)+1
    sta.w !oam_buffer+(4*113)+1
    lda.b #$80
    sta.w !oam_buffer+(4*114)+1
    sta.w !oam_buffer+(4*115)+1
    lda.b #$00
    sta.w !oam_buffer_high+10
    sta.w !oam_buffer_high+11
    lda.b #$00
    sta.w !oam_buffer_high+24
    sta.w !oam_buffer_high+25
    lda.b #$00
    sta.w !oam_buffer_high+26
    sta.w !oam_buffer_high+27
    plp
    rts

; @routine 7F:094E SpriteBar4WithMarker
; WHAT IT DOES: Shows a value of 0-63 (larger values are shown as 63) as a
; four-sprite strip at position X with the tiles SpriteStrip4 picks for it,
; plus a marker sprite at X + value (X + value - 16, with a different tile,
; from 16 up) in the sprite slot Y.
; HOW: Value to direct page $04, position to $02; SpriteStrip4 fills slots
; Y + 4 on; then the marker: tile word $3622 below 16, $3C20 from 16.
; Entry: M 8-bit, X 16-bit, D = $0000; A = value, X = position (X, Y
; pair), Y = marker slot offset in the sprite table. Exit: A = the marker's
; tile word; X as SpriteStrip4 leaves it; Y unchanged; P unchanged.
%ramcode($7F094E, $05814E)
SpriteBar4WithMarker:
    php
    sep.b #$20
    rep.b #$10
    cmp.b #$3F
    bcc .l0959
    lda.b #$3F
.l0959:
    sta.b !dp_04
    rep.b #$20
    stx.b !dp_02
    phy
    tya
    clc
    adc.w #$0004
    tay
    sep.b #$20
    lda.b !dp_04
    rep.b #$20
    and.w #$00FF
    pha
    jsr.w SpriteStrip4
    pla
    ply
    cmp.w #$0010
    bcc .l098C
    clc
    adc.b !dp_02
    sec
    sbc.w #$0010
    sta.w !oam_buffer,y
    lda.w #$3C20
    sta.w !oam_buffer+2,y
    plp
    rtl
.l098C:
    rep.b #$20
    clc
    adc.b !dp_02
    sta.w !oam_buffer,y
    lda.w #$3622
    sta.w !oam_buffer+2,y
    plp
    rtl

; @routine 7F:09ED SpriteBar6WithMarker
; WHAT IT DOES: Shows a value as a six-sprite strip at position X with the
; tiles SpriteStrip6 picks for it, plus a marker sprite at X + value
; (X + value - 8, with a different tile, from 8 up) in the sprite slot Y.
; HOW: As SpriteBar4WithMarker without the limit: SpriteStrip6 fills slots
; Y + 4 on; the marker's tile word is $3C1E below 8, $3C1F from 8.
; Entry: as SpriteBar4WithMarker. Exit: A = the marker's tile word; X as
; SpriteStrip6 leaves it; Y unchanged; P unchanged.
%ramcode($7F09ED, $0581ED)
SpriteBar6WithMarker:
    php
    sep.b #$20
    rep.b #$10
    sta.b !dp_04
    rep.b #$20
    stx.b !dp_02
    phy
    tya
    clc
    adc.w #$0004
    tay
    sep.b #$20
    lda.b !dp_04
    rep.b #$20
    and.w #$00FF
    pha
    jsr.w SpriteStrip6
    pla
    ply
    cmp.w #$0008
    bcc .l0A25
    clc
    adc.b !dp_02
    sec
    sbc.w #$0008
    sta.w !oam_buffer,y
    lda.w #$3C1F
    sta.w !oam_buffer+2,y
    plp
    rtl
.l0A25:
    rep.b #$20
    clc
    adc.b !dp_02
    sta.w !oam_buffer,y
    lda.w #$3C1E
    sta.w !oam_buffer+2,y
    plp
    rtl

; @routine 02:FA19 MetaspriteDraw
; WHAT IT DOES: Draws metasprite X into the sprite table from slot Y on:
; each piece's X and Y offsets are added to the origin at $0D3B/$0D3C and
; its tile and attribute bytes copied; a piece whose fifth byte has bit 0
; clear is made small (its size bit in the high table cleared).
; HOW: The group table at $04:EDCB (4 bytes per metasprite: first piece,
; piece count) and 5-byte pieces at $04:EE4B; the count runs down in direct
; page $F1 (8-bit, after a 16-bit store), the size masks $FD, $F7, $DF, $7F
; come from $02:FA8D through direct page $F3. Origin and sprite table go
; through the data bank.
; Entry: M 16-bit, X 16-bit, D = $0000; X = metasprite, Y = first slot.
; Exit: A = the last piece's fifth byte (or the high-table byte after its
; mask), high byte from the start of the group (or the last mask word);
; X, Y past the pieces; M 16-bit; N clear, Z set; C, V from the last Y
; add, C from bit 3 of the slot offset when the last piece was made small.
%at($02FA19)
MetaspriteDraw:
    txa
    asl a
    asl a
    tax
    tya
    asl a
    asl a
    tay
    lda.l !metasprite_groups+2,x
    sta.b !metasprite_count
    lda.l !metasprite_groups,x
    tax
    sep.b #$20
.lFA2E:
    lda.l !metasprite_pieces,x
    clc
    adc.w !metasprite_origin_x
    sta.w !oam_buffer,y
    lda.l !metasprite_pieces+1,x
    clc
    adc.w !metasprite_origin_y
    sta.w !oam_buffer+1,y
    lda.l !metasprite_pieces+2,x
    sta.w !oam_buffer+2,y
    lda.l !metasprite_pieces+3,x
    sta.w !oam_buffer+3,y
    lda.l !metasprite_pieces+4,x
    bit.b #$01
    bne .lFA7D
    phy
    phx
    rep.b #$20
    tya
    lsr a
    lsr a
    pha
    lsr a
    lsr a
    tax
    pla
    and.w #$0003
    tay
    lda.w !oam_size_masks,y
    sta.b !oam_size_mask
    sep.b #$20
    lda.l !oam_buffer_high,x
    and.b !oam_size_mask
    sta.l !oam_buffer_high,x
    plx
    ply
.lFA7D:
    inx
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    dec.b !metasprite_count
    bne .lFA2E
    rep.b #$20
    rts

; @routine 02:F94F FlashingMetaspriteDraw
; WHAT IT DOES: Resets the high sprite table to large sprites ($AA in
; $0575-$0595) and, when its position at $0D37/$0D38 is non-zero, draws
; metasprite 8 of the bank $1C set from sprite 0 with its palettes cycled:
; the phase in $0D39 steps 0, 1, 2 every fourth frame; phase 0 keeps each
; piece's palette, phases 1 and 2 swap the three palettes around.
; HOW: $0D35 = 8; the set's group table at $1C:E5AC (first piece, count)
; and 5-byte pieces at $1C:E62C as MetaspriteDraw reads them, the count
; running down in direct page $F1. Work RAM through the data bank.
; Entry: any widths, D = $0000. Exit: A = the last piece's fifth byte, high
; byte from its group's first piece (A = 0 and X = $20 when not drawn); X,
; Y past the pieces (low bytes with the 8-bit index registers P restores);
; P unchanged.
%at($02F94F)
FlashingMetaspriteDraw:
    php
    rep.b #$30
    lda.w #$0008
    sta.w !metasprite_set_sel
FlashingMetaspriteDraw_set:             ; FlashingMetaspriteDraw0 joins here
    ldx.w #$0000
    lda.w #$AAAA
.lF95E:
    sta.w !oam_buffer_high,x
    inx
    cpx.w #$0020
    bne .lF95E
    lda.w !flash_sprite_x
    bne .lF96E
    plp
    rts
.lF96E:
    lda.w !frame_count
    and.w #$0003
    bne .lF985
    lda.w !flash_phase
    inc a
    cmp.w #$0003
    bcc .lF982
    lda.w #$0000
.lF982:
    sta.w !flash_phase
.lF985:
    ldy.w #$0000
    lda.w !metasprite_set_sel
    asl a
    asl a
    tax
    lda.l !metasprite_groups_1c+2,x
    sta.b !metasprite_count
    lda.l !metasprite_groups_1c,x
    tax
.lF999:
    sep.b #$20
    lda.l !metasprite_pieces_1c,x
    clc
    adc.w !flash_sprite_x
    sta.w !oam_buffer,y
    lda.l !metasprite_pieces_1c+1,x
    clc
    adc.w !flash_sprite_y
    sta.w !oam_buffer+1,y
    lda.l !metasprite_pieces_1c+2,x
    sta.w !oam_buffer+2,y
    lda.w !flash_phase
    cmp.b #$01
    bcc .lF9FD
    beq .lF9DF
    lda.l !metasprite_pieces_1c+3,x
    bit.b #$08
    bne .lF9D9
    bit.b #$02
    bne .lF9D3
    and.b #$F1
    ora.b #$06
    bra .lFA01
.lF9D3:
    and.b #$F1
    ora.b #$08
    bra .lFA01
.lF9D9:
    and.b #$F1
    ora.b #$04
    bra .lFA01
.lF9DF:
    lda.l !metasprite_pieces_1c+3,x
    bit.b #$08
    bne .lF9F7
    bit.b #$02
    bne .lF9F1
    and.b #$F1
    ora.b #$08
    bra .lFA01
.lF9F1:
    and.b #$F1
    ora.b #$04
    bra .lFA01
.lF9F7:
    and.b #$F1
    ora.b #$06
    bra .lFA01
.lF9FD:
    lda.l !metasprite_pieces_1c+3,x
.lFA01:
    sta.w !oam_buffer+3,y
    lda.l !metasprite_pieces_1c+4,x
    bit.b #$01
    inx
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    dec.b !metasprite_count
    bne .lF999
    plp
    rts

; @routine 02:F926 FlashingMetaspriteDraw0
; WHAT IT DOES: FlashingMetaspriteDraw with metasprite 0 of the bank $1C
; set.
; HOW: $0D35 = 0, then a jump into FlashingMetaspriteDraw after its own
; store.
; Entry: as FlashingMetaspriteDraw. Exit: as FlashingMetaspriteDraw.
%at($02F926)
FlashingMetaspriteDraw0:
    php
    rep.b #$30
    lda.w #$0000
    sta.w !metasprite_set_sel
    jmp.w FlashingMetaspriteDraw_set

; @routine 02:FB9E OamAttr36x6
; WHAT IT DOES: Gives 6 sprites from entry A attribute byte $36.
; HOW: As OamAttr36x4.
; Entry: as OamAttr36x4. Exit: as OamAttr36x4.
%at($02FB9E)
OamAttr36x6:
    asl a
    asl a
    tax
    ldy.w #$0006
    sep.b #$20
    lda.b #$36
    bra OamAttrFillLoop

; @routine 02:FBB6 OamAttr38x4
; WHAT IT DOES: Gives 4 sprites from entry A attribute byte $38.
; HOW: As OamAttr36x4.
; Entry: as OamAttr36x4. Exit: as OamAttr36x4, low byte $38.
%at($02FBB6)
OamAttr38x4:
    asl a
    asl a
    tax
    ldy.w #$0004
    sep.b #$20
    lda.b #$38
    bra OamAttrFillLoop

; @routine 07:C4E0 OamHighBits18Park48
; WHAT IT DOES: OamHighBits18, then sets the X-high bit of sprites 48-63
; (high table bytes 12-15 = $55), which puts them off the left edge.
; HOW: Two word stores of $5555 to $0581 and $0583 through the data bank.
; Entry: any widths. Exit: A = $5555; X = 0; Y high byte clear; P
; unchanged.
%at($07C4E0)
OamHighBits18Park48:
    php
    jsr.w OamHighBits18
    rep.b #$20
    lda.w #$5555
    sta.w !oam_buffer_high+12
    sta.w !oam_buffer_high+14
    plp
    rts

; @routine 07:B985 SpritePieces3
; WHAT IT DOES: Writes three sprites of a piece set to the sprite buffer
; from entry offset Y: the set is the word at $1C:F09E + ((the byte at
; $07:B981 + $10BB's low byte) + $10C3) * 4 (low byte); each piece (five
; bytes from $1C:F11E + the set) adds its X, Y and tile to $10BD, $10BF and
; $10C1, and gives its attribute, with the palette bits (1-3) from $10C5
; when that is not 0.
; HOW: Direct page $F1 counts the pieces down. $10C5 is read again for the
; palette when it is set.
; Entry: M 16-bit, X 16-bit, D = $0000; Y = sprite buffer offset. Exit:
; A = the set's high byte and the last attribute; X = the set + 15; Y
; past the third sprite; M 8-bit; N clear, Z set ($F1 counted to 0); C, V from the last
; tile add.
%at($07B985)
SpritePieces3:
    lda.w !sprite_piece_select
    and.w #$00FF
    tax
    lda.l !piece_set_bias,x
    and.w #$00FF
    clc
    adc.w !sprite_piece_frame
    asl a
    asl a
    and.w #$00FF
    tax
    lda.l !piece_sets,x
    tax
    sep.b #$20
    lda.b #$03
    sta.b !sprite_piece_count
.piece:
    lda.w !sprite_piece_x
    clc
    adc.l !piece_bytes,x
    sta.w !oam_buffer,y
    lda.w !sprite_piece_y
    clc
    adc.l !piece_bytes+1,x
    sta.w !oam_buffer+1,y
    lda.w !sprite_piece_tile
    clc
    adc.l !piece_bytes+2,x
    sta.w !oam_buffer+2,y
    lda.w !sprite_piece_palette
    bne .own_palette
    lda.l !piece_bytes+3,x
    bra .attr
.own_palette:
    lda.l !piece_bytes+3,x
    and.b #$F1
    ora.w !sprite_piece_palette
.attr:
    sta.w !oam_buffer+3,y
    inx
    inx
    inx
    inx
    inx
    iny
    iny
    iny
    iny
    dec.b !sprite_piece_count
    bne .piece
    rtl
