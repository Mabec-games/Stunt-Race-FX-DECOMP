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

; @routine 0B:E2E1 MenuRunFrames
; WHAT IT DOES: Runs menu frames until $11DA equals $11DB: each frame waits
; for the vertical blank (MenuWaitFrame), runs the objects
; (MenuActorsRun), builds the sprite table (MenuOamBuild) and asks for
; every frame update (bits 0-4 of $11D7).
; HOW: 8-bit A for the flags and the compare; P saved and restored.
; Entry: any widths. Exit: A low byte = $11DA, high byte as MenuOamBuild
; left it; X, Y as MenuOamBuild left them; P unchanged.
%at($0BE2E1)
MenuRunFrames:
    php
    sep.b #$20
.lE2E4:
    jsr.w MenuWaitFrame
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$1F
    sta.w !frame_update_flags
    lda.w !wram_11DA
    cmp.w !wram_11DB
    bne .lE2E4
    plp
    rts
