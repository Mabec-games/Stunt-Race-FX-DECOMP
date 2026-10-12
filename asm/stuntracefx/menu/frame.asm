; frame.asm: the menu's frame handshake. The menu code sets bit 0 of $11D8
; when it has finished a frame; the menu's vertical blank code clears $11D8
; once it has applied the frame's updates.
; C: src/stuntracefx/menu/frame.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:E390 MenuWaitFrame
; WHAT IT DOES: Marks the frame finished and waits for the vertical blank
; code to take it.
; HOW: Sets bit 0 of $11D8 (16-bit, through the data bank) and polls it
; until it is zero.
; Entry: any widths. Exit: A = 0; P unchanged.
%at($0BE390)
MenuWaitFrame:
    php
    rep.b #$20
    lda.w !menu_frame_wait
    ora.w #$0001
    sta.w !menu_frame_wait
.lE39C:
    lda.w !menu_frame_wait
    bne .lE39C
    plp
    rts
