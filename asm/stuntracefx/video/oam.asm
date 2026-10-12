; oam.asm: the work-RAM copy of the sprite table (OAM), copied to the PPU each
; frame. Each sprite is four bytes: X, Y, tile, attributes.
; C: src/stuntracefx/video/oam.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
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
