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
; Entry: any widths. Exit: X and Y high bytes cleared (the routine switches
; to 8-bit index registers); A and P unchanged.
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
    sta.l !palette_saved,x
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

; @routine 0B:F50C MenuHdmaSetup
; WHAT IT DOES: Points HDMA channels 7, 5 and 4 at TM (one byte per line,
; tables at $7F:1315, $7F:131A, $7F:131F) and channel 6 at WH0-WH1 (two
; bytes, table at $7F:1324). Enabling them is left to the caller.
; HOW: Mode, B-bus register, table address (16-bit store) and bank for each
; channel, in the order 7, 6, 5, 4; the registers are reached through the
; data bank.
; Entry: any widths. Exit: A low byte = $7F; X = $131F; P unchanged.
%at($0BF50C)
MenuHdmaSetup:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$00
    sta.w !DMAP7
    lda.b #$2C
    sta.w !BBAD7
    ldx.w #$1315
    stx.w !A1T7L
    lda.b #$7F
    sta.w !A1B7
    lda.b #$01
    sta.w !DMAP6
    lda.b #$26
    sta.w !BBAD6
    ldx.w #$1324
    stx.w !A1T6L
    lda.b #$7F
    sta.w !A1B6
    lda.b #$00
    sta.w !DMAP5
    lda.b #$2C
    sta.w !BBAD5
    ldx.w #$131A
    stx.w !A1T5L
    lda.b #$7F
    sta.w !A1B5
    lda.b #$00
    sta.w !DMAP4
    lda.b #$2C
    sta.w !BBAD4
    ldx.w #$131F
    stx.w !A1T4L
    lda.b #$7F
    sta.w !A1B4
    plp
    rts

; @routine 0B:F5E6 MenuHdmaSetupB
; WHAT IT DOES: Points HDMA channel 7 at BG2HOFS (indirect, table $7F:133A,
; data bank 0), 6 at CGWSEL ($7F:1341), 5 at TMW ($7F:1348) and 4 at WH0
; ($7F:1353), sets $7F:1354 to $FF and the positions at $18BB/$18BF to 0 and
; $18BD/$18C1 to $A8.
; HOW: Register stores through the data bank in channel order 7, 6, 5, 4,
; then the bank $7F byte and four 16-bit long stores.
; Entry: any widths. Exit: A = $00A8; X = $1353; P unchanged.
%at($0BF5E6)
MenuHdmaSetupB:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$42
    sta.w !DMAP7
    lda.b #$0F
    sta.w !BBAD7
    ldx.w #$133A
    stx.w !A1T7L
    lda.b #$7F
    sta.w !A1B7
    lda.b #$00
    sta.w !DASB7
    lda.b #$00
    sta.w !DMAP6
    lda.b #$30
    sta.w !BBAD6
    ldx.w #$1341
    stx.w !A1T6L
    lda.b #$7F
    sta.w !A1B6
    lda.b #$00
    sta.w !DMAP5
    lda.b #$2E
    sta.w !BBAD5
    ldx.w #$1348
    stx.w !A1T5L
    lda.b #$7F
    sta.w !A1B5
    lda.b #$00
    sta.w !DMAP4
    lda.b #$26
    sta.w !BBAD4
    ldx.w #$1353
    stx.w !A1T4L
    lda.b #$7F
    sta.w !A1B4
    lda.b #$FF
    sta.l !menu_hdma_1354
    rep.b #$20
    lda.w #$0000
    sta.l !menu_pos_18bb
    sta.l !menu_sine_y
    lda.w #$00A8
    sta.l !menu_pos_18bd
    sta.l !menu_sine_y_min
    plp
    rts

; @routine 0B:E303 MenuPaletteScale
; WHAT IT DOES: Writes Y + 1 colours into the palette buffer from X on, each
; the saved colour ($7E:FF65 + X) with its red, green and blue scaled by
; the factor at $11DF / 32.
; HOW: Three 8 x 8 hardware multiplies per colour (component x factor, the
; product's bits 5-9 kept); $11E3 holds the colour and $11E1 the result
; as it is built (red, then green, then blue).
; Entry: X 16-bit; X = byte offset of the first colour, Y = count - 1.
; Exit: A = the last colour; X past it; Y = $FFFF; P unchanged.
%at($0BE303)
MenuPaletteScale:
    php
    rep.b #$30
.lE306:
    lda.l !palette_saved,x
    sta.w !scratch_11e3
    and.w #$001F
    sep.b #$20
    sta.w !WRMPYA
    lda.w !scratch_11df
    sta.w !WRMPYB
    rep.b #$20
    nop
    nop
    nop
    nop
    lda.w !RDMPYL
    asl a
    asl a
    asl a
    xba
    and.w #$001F
    sta.w !scratch_11e1
    lda.w !scratch_11e3
    and.w #$03E0
    asl a
    asl a
    asl a
    xba
    sep.b #$20
    sta.w !WRMPYA
    lda.w !scratch_11df
    sta.w !WRMPYB
    rep.b #$20
    nop
    nop
    nop
    nop
    lda.w !RDMPYL
    and.w #$03E0
    ora.w !scratch_11e1
    sta.w !scratch_11e1
    lda.w !scratch_11e3
    and.w #$7C00
    lsr a
    lsr a
    xba
    and.w #$001F
    sep.b #$20
    sta.w !WRMPYA
    lda.w !scratch_11df
    sta.w !WRMPYB
    rep.b #$20
    nop
    nop
    nop
    nop
    lda.w !RDMPYL
    and.w #$03E0
    lsr a
    lsr a
    lsr a
    xba
    ora.w !scratch_11e1
    sta.w !scratch_11e1
    sta.l !palette_buffer,x
    inx
    inx
    dey
    bmi .lE38E
    brl .lE306
.lE38E:
    plp
    rtl

; @routine 0B:F6EA MenuPictureInsetGrid
; WHAT IT DOES: Raises the tile numbers of the $380-word picture at
; $70:3000 by $100 into a copy at $70:3800, and in the picture itself puts
; an 11-row by 13-column block of tiles numbered from $11E1 (priority set),
; rows one tile apart and columns $0B apart.
; HOW: Rows of 32 words from word $180: 4 raised, 13 replaced in the picture
; (the copy still gets the raised original), 15 raised; $11DF counts the
; rows and $11E1 the first tile of each row. The words before and after
; the block are only raised.
; Entry: any widths. Exit: A = the last word; X = $700; Y = $FFFF; P
; unchanged.
%at($0BF6EA)
MenuPictureInsetGrid:
    php
    rep.b #$30
    ldx.w #$0000
    ldy.w #$017F
.lF6F3:
    lda.l !gsu_picture,x
    clc
    adc.w #$0100
    sta.l !gsu_picture,x
    sta.l !gsu_picture_copy,x
    inx
    inx
    dey
    bpl .lF6F3
    lda.w #$000A
    sta.w !scratch_11df
.lF70E:
    ldy.w #$0003
.lF711:
    lda.l !gsu_picture,x
    clc
    adc.w #$0100
    sta.l !gsu_picture,x
    sta.l !gsu_picture_copy,x
    inx
    inx
    dey
    bpl .lF711
    lda.w !scratch_11e1
    ora.w #$2000
    ldy.w #$000C
.lF72F:
    pha
    lda.l !gsu_picture,x
    clc
    adc.w #$0100
    sta.l !gsu_picture_copy,x
    pla
    sta.l !gsu_picture,x
    clc
    adc.w #$000B
    inx
    inx
    dey
    bpl .lF72F
    ldy.w #$000E
.lF74D:
    lda.l !gsu_picture,x
    clc
    adc.w #$0100
    sta.l !gsu_picture,x
    sta.l !gsu_picture_copy,x
    inx
    inx
    dey
    bpl .lF74D
    inc.w !scratch_11e1
    dec.w !scratch_11df
    bpl .lF70E
    ldy.w #$009F
.lF76D:
    lda.l !gsu_picture,x
    clc
    adc.w #$0100
    sta.l !gsu_picture,x
    sta.l !gsu_picture_copy,x
    inx
    inx
    dey
    bpl .lF76D
    plp
    rts

; @routine 0B:F94A MenuCourseGraphics
; WHAT IT DOES: Draws the course screen's graphics into VRAM: the course's
; two pictures (tables $03:F4F9 and $03:F5A1, three bytes per course) to
; $4C00 ($1800 bytes) and $3C00 ($2000 bytes), picture $FB84 to both $1680
; and $3680 ($600 bytes each) and picture $FFAC to $1C00 ($800 bytes).
; HOW: Each picture is drawn by the Super FX (GsuCF42To3000) and written a
; word at a time to VMDATA.
; Entry: any widths, D = $0000. Exit: A = the last word; X = $800; P
; unchanged.
%at($0BF94A)
MenuCourseGraphics:
    php
    rep.b #$30
    lda.w !course
    asl a
    clc
    adc.w !course
    and.w #$00FF
    tax
    lda.l !course_picture_a,x
    pha
    lda.l !course_picture_a+2,x
    and.w #$00FF
    tax
    pla
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w #$4C00
    sta.w !VMADDL
    ldx.w #$0000
.lF976:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$1800
    bne .lF976
    rep.b #$30
    lda.w !course
    asl a
    clc
    adc.w !course
    and.w #$00FF
    tax
    lda.l !course_picture_b,x
    pha
    lda.l !course_picture_b+2,x
    and.w #$00FF
    tax
    pla
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w #$3C00
    sta.w !VMADDL
    ldx.w #$0000
.lF9AF:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$2000
    bne .lF9AF
    rep.b #$30
    lda.w #$FB84
    ldx.w #$0017
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w #$1680
    sta.w !VMADDL
    ldx.w #$0000
.lF9D4:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$0600
    bne .lF9D4
    rep.b #$30
    lda.w #$3680
    sta.w !VMADDL
    ldx.w #$0000
.lF9ED:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$0600
    bne .lF9ED
    rep.b #$30
    lda.w #$FFAC
    ldx.w #$0017
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w #$1C00
    sta.w !VMADDL
    ldx.w #$0000
.lFA12:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$0800
    bne .lFA12
    sep.b #$20
    plp
    rtl
