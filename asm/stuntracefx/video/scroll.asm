; scroll.asm: BG scroll registers set by the raster interrupt. Runs from work
; RAM. C: src/stuntracefx/video/scroll.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:B28C Bg2ScrollFromTable
; WHAT IT DOES: Sets BG2's scroll from the pair for a screen band.
; HOW: Writes the horizontal pair at $0D92+X to BG2HOFS and the vertical pair
; at $0D8E+X to BG2VOFS, low byte first.
; Entry: M 8-bit, DB a bank where these are work RAM; X = band offset.
; Exit: A low byte = the vertical high byte; N, Z from it.
%ramcode($7EB28C, $010FB3)
Bg2ScrollFromTable:
    lda.w !bg2_scroll_x,x
    sta.w !BG2HOFS
    lda.w !bg2_scroll_x+1,x
    sta.w !BG2HOFS
    lda.w !bg2_scroll_y,x
    sta.w !BG2VOFS
    lda.w !bg2_scroll_y+1,x
    sta.w !BG2VOFS
    rts

; @routine 7E:B2A5 ScrollCopyToBank7F
; WHAT IT DOES: Copies two 16-bit scroll values ($0D96, $0D98) to their
; places in bank $7F ($7F:348D, $7F:3493).
; HOW: Two 16-bit copies.
; Entry: any widths, DB a bank where $0D96 is work RAM. Exit: A = the second
; value; N, Z from it; M 16-bit.
ScrollCopyToBank7F:
    rep.b #$20
    lda.w !scroll_pair_a
    sta.l !scroll_pair_a_7f
    lda.w !scroll_pair_b
    sta.l !scroll_pair_b_7f
    rts

; @routine 03:FED9 ScrollHdmaSet
; WHAT IT DOES: Fills the scroll HDMA table at $10AA: the scroll value from
; $0E37 into its first and fourth entries' data and $0130 into the second
; and third.
; HOW: Word stores to $10AE, $10B7, $10B1, $10B4, in that order.
; Entry: any widths. Exit: A = $0130 (16-bit); P unchanged.
%at($03FED9)
ScrollHdmaSet:
    php
    rep.b #$20
    lda.l !scroll_value
    sta.l !scroll_hdma+$4
    sta.l !scroll_hdma+$D
    lda.w #$0130
    sta.l !scroll_hdma+$7
    sta.l !scroll_hdma+$A
    plp
    rtl
