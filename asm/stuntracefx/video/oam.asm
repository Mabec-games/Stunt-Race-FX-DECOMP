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
OamAttrFillLoop:                    ; shared with OamAttr36x4 and OamAttr38x6
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
