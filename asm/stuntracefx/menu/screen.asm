; screen.asm: menu screen setup: scroll registers, the palette buffer and its
; saved copy, sprite lists and pictures drawn by the Super FX.
; C: src/stuntracefx/menu/screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:F4B2 MenuScrollReset
; WHAT IT DOES: Sets the horizontal and vertical scroll of BG1-BG3 to 0.
; HOW: Two zero writes to each scroll register (low then high byte), the
; horizontal ones first.
; Entry: any widths. Exit: every register unchanged.
%at($0BF4B2)
MenuScrollReset:
    php
    sep.b #$30
    stz.w !BG1HOFS
    stz.w !BG1HOFS
    stz.w !BG2HOFS
    stz.w !BG2HOFS
    stz.w !BG3HOFS
    stz.w !BG3HOFS
    stz.w !BG1VOFS
    stz.w !BG1VOFS
    stz.w !BG2VOFS
    stz.w !BG2VOFS
    stz.w !BG3VOFS
    stz.w !BG3VOFS
    plp
    rts

; @routine 0B:F4DB MenuPaletteLoad
; WHAT IT DOES: Copies a full 256-colour palette into the palette buffer
; from the long address in direct page $FB.
; HOW: 256 word copies.
; Entry: D = $0000. Exit: A = the last colour; X = Y = $200; P unchanged.
%at($0BF4DB)
MenuPaletteLoad:
    php
    rep.b #$30
    ldx.w #$0000
    ldy.w #$0000
.lF4E4:
    lda.b [!apu_stream_ptr],y
    sta.l !palette_buffer,x
    iny
    iny
    inx
    inx
    cpx.w #$0200
    bne .lF4E4
    plp
    rts

; @routine 0B:F4F5 MenuPaletteSave
; WHAT IT DOES: Saves the palette buffer to the copy that follows it
; ($7E:FF65).
; HOW: 256 word copies.
; Entry: any widths. Exit: A = the last colour; X = $200; P unchanged.
%at($0BF4F5)
MenuPaletteSave:
    php
    rep.b #$30
    ldx.w #$0000
.lF4FB:
    lda.l !palette_buffer,x
    sta.l !palette_kept+$3E,x
    inx
    inx
    cpx.w #$0200
    bne .lF4FB
    plp
    rts

; @routine 0B:FAE5 MenuOamFromList
; WHAT IT DOES: Copies words from the list at $FCA2 + Y (data bank) into the
; sprite table copy at $0375 + X, until a $FFFF word.
; HOW: A word loop stepping X and Y by 2.
; Entry: X = sprite table offset, Y = list offset. Exit: X, Y past the last
; word copied and the end mark; A = $FFFF; M 16-bit, X 16-bit; Z, C set, N
; clear.
%at($0BFAE5)
MenuOamFromList:
    rep.b #$30
.lFAE7:
    lda.w !menu_oam_lists,y
    cmp.w #$FFFF
    beq .lFAF8
    sta.w !oam_buffer,x
    inx
    inx
    iny
    iny
    bra .lFAE7
.lFAF8:
    rts

; @routine 0B:E3D2 MenuPictureToVram
; WHAT IT DOES: Has the Super FX draw a picture into $70:3000 (program
; $01:CF42 with inputs A and X) and writes it to VRAM at the current
; address, a word at a time, up to the byte count at $11D5.
; HOW: GsuCF42To3000, VMAIN = $80, then VMDATA writes until X reaches the
; count (read each time through the data bank).
; Entry: D = $0000; A, X = the picture's inputs. Exit: A = the last word; X
; = the count; P unchanged.
%at($0BE3D2)
MenuPictureToVram:
    php
    rep.b #$30
    jsl.l GsuCF42To3000
    sep.b #$20
    rep.b #$10
    lda.b #$80
    sta.w !VMAIN
    rep.b #$20
    ldx.w #$0000
.lE3E7:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w !menu_picture_size
    bne .lE3E7
    plp
    rts
