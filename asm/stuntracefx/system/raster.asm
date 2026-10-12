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

; @routine 02:D7AB WaitScanline
; WHAT IT DOES: Waits until the low byte of the PPU's vertical counter
; equals the byte at $06E3; clears $06E4 first.
; HOW: Latches the counters (SLHV) and reads OPVCT twice (low, high) each
; time round; the compare is 8-bit.
; Entry: X 16-bit. Exit: A, X, P unchanged.
%at($02D7AB)
WaitScanline:
    php
    rep.b #$30
    phx
    pha
    sep.b #$20
    stz.w !scanline_wait_high
.lD7B5:
    sep.b #$20
    lda.w !SLHV
    lda.w !OPVCT
    xba
    lda.w !OPVCT
    xba
    cmp.w !scanline_wait
    bne .lD7B5
    rep.b #$30
    pla
    plx
    plp
    rtl
