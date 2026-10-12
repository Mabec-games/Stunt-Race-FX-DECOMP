; race_text.asm: words the race screen spells with sprites, one letter tile
; per sprite of the work-RAM sprite table ($0375, four bytes a sprite;
; the tile is byte 2).
; C: src/stuntracefx/hud/race_text.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:EFA3 OamClassName
; WHAT IT DOES: Spells the race class ($0D4A) on the six sprites after
; sprite $0D7B: "EXPERT" for class 1, "MASTER" for class 2 (not reached in
; the traces so far), nothing for class 0.
; HOW: The tiles come from $02:EFD6 (six bytes a name), counted down in
; direct page $F1; 8-bit stores through the data bank. P is saved and
; restored.
; Entry: D = $0000. Exit: Y = ($0D7B + 1) * 4, plus $18 after a name; A
; low byte = the class or the last tile, high byte from Y's value before
; the name; X past the name's last tile, unchanged for class 0; P
; unchanged.
%at($02EFA3)
OamClassName:
    php
    rep.b #$30
    lda.w !wram_0D7B
    inc a
    asl a
    asl a
    tay
    sep.b #$20
    lda.w !wram_0D4A
    beq .lEFD4
    cmp.b #$01
    bne .lEFBD
    ldx.w #$0006
    bra .lEFC0
.lEFBD:
    ldx.w #$0000
.lEFC0:
    lda.b #$06
    sta.b !digit_count
.lEFC4:
    lda.l !class_names,x
    sta.w !oam_buffer+2,y
    inx
    iny
    iny
    iny
    iny
    dec.b !digit_count
    bne .lEFC4
.lEFD4:
    plp
    rts
