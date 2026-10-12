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

; @routine 02:FBE5 SpriteTextAt895E
; WHAT IT DOES: Draws text line $0E3D as sprites from slot $70 at X $89,
; Y $5E, attributes $36, and makes those sprites small.
; HOW: X in A's high byte, Y as $89 - $2B, then the shared body at
; $02:FC1F (OamTextLineLong, then $058E-$0595 cleared).
; Entry: X 16-bit, D = $0000. Exit: A = 0; X, Y as OamTextLine leaves
; them; M 16-bit; Z, C, V set, N clear.
%at($02FBE5)
SpriteTextAt895E:
    sep.b #$20
    lda.b #$89
    xba
    lda.b #$89
    sec
    sbc.b #$2B
    bra SpriteTextAt_y

; @routine 02:FBF1 SpriteTextAt8152
; WHAT IT DOES: As SpriteTextAt895E at X $81, Y $52.
; HOW: Y as $89 - $37, then the shared body at $02:FC1C.
; Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
%at($02FBF1)
SpriteTextAt8152:
    sep.b #$20
    lda.b #$81
    xba
    lda.b #$89
    bra SpriteTextAt_body

; @routine 02:FBFA SpriteTextAt8152B
; WHAT IT DOES: The same as SpriteTextAt8152 (a second copy of its entry).
; HOW: As SpriteTextAt8152.
; Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
%at($02FBFA)
SpriteTextAt8152B:
    sep.b #$20
    lda.b #$81
    xba
    lda.b #$89
    bra SpriteTextAt_body

; @routine 02:FC03 SpriteTextAt8952
; WHAT IT DOES: As SpriteTextAt895E at X $89, Y $52. Nothing calls this
; entry in the traces so far.
; HOW: As SpriteTextAt8152.
; Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
%at($02FC03)
SpriteTextAt8952:
    sep.b #$20
    lda.b #$89
    xba
    lda.b #$89
    bra SpriteTextAt_body

; @routine 02:FC0C SpriteTextAt8152C
; WHAT IT DOES: A third copy of SpriteTextAt8152's entry. Nothing calls it
; in the traces so far.
; HOW: As SpriteTextAt8152.
; Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
%at($02FC0C)
SpriteTextAt8152C:
    sep.b #$20
    lda.b #$81
    xba
    lda.b #$89
    bra SpriteTextAt_body

; @routine 02:FC15 SpriteTextAt8952B
; WHAT IT DOES: A second copy of SpriteTextAt8952's entry, falling into the
; shared body. Nothing calls it in the traces so far.
; HOW: As SpriteTextAt8152.
; Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
%at($02FC15)
SpriteTextAt8952B:
    sep.b #$20
    lda.b #$89
    xba
    lda.b #$89
SpriteTextAt_body:                  ; the entries above join here
    sec
    sbc.b #$37
SpriteTextAt_y:                     ; SpriteTextAt895E joins here
    sta.b !text_y
    xba
    sta.b !text_pen
    lda.b #$70
    sta.b !text_count
    lda.b #$36
    sta.b !text_attr
    stz.b !text_tile
    jsl.l OamTextLineLong
    rep.b #$20
    lda.w #$0000
    sta.w !oam_buffer_high+25
    sta.w !oam_buffer_high+27
    sta.w !oam_buffer_high+29
    sta.w !oam_buffer_high+31
    rts
