; sprite_text.asm: lines of text drawn as sprites. A line is a count byte
; then one byte per character; a byte with bit 7 set is a space (it moves
; the pen without using a sprite). Lines are found through the word table
; at $07:8000, indexed by $0E3D.
; C: src/stuntracefx/video/sprite_text.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:AA60 OamTextLineLong
; WHAT IT DOES: OamTextLine for callers in other banks.
; HOW: Sets the data bank to $07 around the call.
; Entry: M 8-bit, D = $0000. Exit: as OamTextLine; DB unchanged.
%at($07AA60)
OamTextLineLong:
    php
    sep.b #$20
    phb
    phk
    plb
    jsr.w OamTextLine
    sep.b #$20
    plb
    plp
    rtl

; @routine 07:AA6E OamTextLine
; WHAT IT DOES: Draws line $0E3D as sprites from sprite $F2 on, centred on
; X position $F3: each character's sprite gets X = the pen (stepping 8),
; Y = $F4, tile = the character + $F1 and attributes = $F5.
; HOW: The pen starts at $F3 - 4 * count; $F2 then counts the characters
; down and $F3 is the pen. A space steps the pen and the count without a
; sprite, and is not followed by the end-of-line test, so a line never ends
; in a space. Line and sprites are read and written through the data bank.
; Entry: M 8-bit, D = $0000. Exit: A = the line pointer's high byte and
; $F5; X past the line; Y past the last sprite (4 bytes each), both cut to
; 8 bits when X is 8-bit; P unchanged.
%at($07AA6E)
OamTextLine:
    php
    rep.b #$30
    lda.b !text_count
    and.w #$00FF
    asl a
    asl a
    tay
    lda.w !text_line
    asl a
    and.w #$00FF
    tax
    lda.l !text_lines,x
    tax
    sep.b #$20
    lda.w 0,x
    sta.b !text_count
    inx
    asl a
    asl a
    sec
    sbc.b !text_pen
    eor.b #$FF
    inc a
    sta.b !text_pen
.char:
    lda.w 0,x
    bpl .sprite
    inx
    lda.b !text_pen
    clc
    adc.b #$08
    sta.b !text_pen
    dec.b !text_count
    bra .char
.sprite:
    clc
    adc.b !text_tile
    sta.w !oam_buffer+2,y
    inx
    lda.b !text_pen
    sta.w !oam_buffer,y
    clc
    adc.b #$08
    sta.b !text_pen
    lda.b !text_y
    sta.w !oam_buffer+1,y
    lda.b !text_attr
    sta.w !oam_buffer+3,y
    iny
    iny
    iny
    iny
    dec.b !text_count
    bne .char
    plp
    rts
