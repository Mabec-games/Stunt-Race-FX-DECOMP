; oam.asm: the work-RAM copy of the sprite table (OAM), copied to the PPU each
; frame. Each sprite is four bytes: X, Y, tile, attributes.
; C: src/stuntracefx/video/oam.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

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
