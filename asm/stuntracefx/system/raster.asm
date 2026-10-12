; raster.asm: waiting on the PPU's beam inside the raster interrupt.
; Runs from work RAM (copied there with the rest of main_wram, see
; config/stuntracefx/ramcode.csv). C: src/stuntracefx/system/raster.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"

arch 65816

; @routine 7E:A2FD WaitHBlank
; WHAT IT DOES: Waits until the PPU is in horizontal blank, so the raster
; interrupt can change PPU registers between two lines.
; HOW: Reads HVBJOY until its H-blank bit (bit 6) is set.
; Entry: M 8-bit, X 8-bit. Exit: A = the last HVBJOY read; N from it; Z clear.
%ramcode($7EA2FD, $010024)
WaitHBlank:
    lda.w !HVBJOY
    bit.b #$40
    beq WaitHBlank
    rts
