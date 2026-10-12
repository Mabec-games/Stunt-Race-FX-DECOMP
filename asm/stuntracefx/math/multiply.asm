; multiply.asm: the PPU's signed multiplier (Mode 7 registers) as a fast
; 16 x 8 multiply. Runs from work RAM. C: src/stuntracefx/math/multiply.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:E6BA PpuMultiplierSetLowByteTimes8
; WHAT IT DOES: Loads the multiplier's 16-bit factor with the low byte of A
; times 8.
; HOW: Moves the low byte to the high byte and shifts right 5, then writes
; the result to M7A low then high.
; Entry: M 16-bit. Exit: A = the factor with its bytes swapped; N, Z from its
; high byte; C clear; M 16-bit.
%ramcode($7EE6BA, $0143E1)
PpuMultiplierSetLowByteTimes8:
    xba
    and.w #$FF00
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sep.b #$20
    sta.l !M7A
    xba
    sta.l !M7A
    rep.b #$20
    rts

; @routine 7E:E7A1 PpuMultiply
; WHAT IT DOES: Multiplies the multiplier's 16-bit factor by the low byte of
; A (both signed) and returns the product's upper 16 bits.
; HOW: Writes the low byte to M7B, then reads MPYM and MPYH.
; Entry: M 16-bit. Exit: A = product >> 8; N, Z from it; M 16-bit.
%ramcode($7EE7A1, $0144C8)
PpuMultiply:
    sep.b #$20
    sta.l !M7B
    rep.b #$20
    lda.l !MPYM
    rts
