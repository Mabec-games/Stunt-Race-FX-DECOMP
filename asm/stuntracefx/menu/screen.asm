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

; @routine 0B:C9DD MenuSpritePalettesScale
; WHAT IT DOES: Scales sprite palettes 5 and 1 (colours 208-215 and
; 144-151) from their saved copy by A / 32.
; HOW: MenuPaletteScale twice, from byte offsets $1A0 and $120, with the
; factor stored at $11DF before each.
; Entry: M 16-bit, X 16-bit; A = factor (0-32). Exit: A = the last colour;
; X = $11F9; Y = $FFFF; N, Z from X.
%at($0BC9DD)
MenuSpritePalettesScale:
    sta.w !scratch_11df
    pha
    ldy.w #$0007
    ldx.w #$01A0
    jsl.l MenuPaletteScale
    pla
    sta.w !scratch_11df
    ldy.w #$0007
    ldx.w #$0120
    jsl.l MenuPaletteScale
    ldx.w !actor_current
    rts

; @routine 0B:CFAB MenuCrossFadeStep
; WHAT IT DOES: One frame of a 64-frame cross-fade, as object X's handler:
; colours 64-127 are scaled from their saved copy by f / 32 and colours
; 12-15 by (32 - f) / 32, f from the table at $0B:CFE5 for the frame
; count +$0A; after the last frame the object's handler becomes $CE89.
; HOW: The count is incremented first; at $40 only the handler is changed.
; The table is read at the count with bit 0 cleared; the second factor is
; ((f - 1) EOR $1F) + 1. Both go through $11DF to MenuPaletteScale.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: last frame: A =
; $CE89; N set, Z clear; C set. Otherwise: A = the last colour; X = $0020;
; Y = $FFFF; N, Z from the second factor; C clear.
%at($0BCFAB)
MenuCrossFadeStep:
    lda.b !actor_0a,x
    inc a
    cmp.w #$0040
    beq .lCFDF
    sta.b !actor_0a,x
    lsr a
    asl a
    tax
    lda.l !menu_fade_factors,x
    sta.w !scratch_11df
    ldy.w #$003F
    ldx.w #$0080
    jsl.l MenuPaletteScale
    lda.w !scratch_11df
    dec a
    eor.w #$001F
    inc a
    sta.w !scratch_11df
    ldy.w #$0003
    ldx.w #$0018
    jsl.l MenuPaletteScale
    rts
.lCFDF:
    lda.w #$CE89
    sta.b !actor_handler,x
    rts

; @routine 0B:F6B6 MenuPictureDC8CGrid0
; WHAT IT DOES: Draws picture $DC8C and lays over it the 11 x 13 tile grid
; numbered from tile 0.
; HOW: GsuCF42To3000 with $DC8C and $1C, $11E1 = 0, MenuPictureInsetGrid;
; P saved and restored around it.
; Entry: X 16-bit. Exit: A = the last picture word; X = $0700; Y = $FFFF;
; P unchanged.
%at($0BF6B6)
MenuPictureDC8CGrid0:
    php
    rep.b #$30
    lda.w #$DC8C
    ldx.w #$001C
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w #$0000
    sta.w !scratch_11e1
    jsr.w MenuPictureInsetGrid
    plp
    rts

; @routine 0B:F6D0 MenuPictureDC8CGrid58
; WHAT IT DOES: As MenuPictureDC8CGrid0, the grid numbered from tile $58.
; HOW: As MenuPictureDC8CGrid0.
; Entry: X 16-bit. Exit: as MenuPictureDC8CGrid0.
%at($0BF6D0)
MenuPictureDC8CGrid58:
    php
    rep.b #$30
    lda.w #$DC8C
    ldx.w #$001C
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w #$0058
    sta.w !scratch_11e1
    jsr.w MenuPictureInsetGrid
    plp
    rts

; @routine 0B:D14A MenuCrossFadeOut
; WHAT IT DOES: The cross-fade of MenuCrossFadeStep run backwards, as object
; X's handler: +$0A counts down by 2 and picks the factors; when it would
; go below 0, sets the screen up for the menu's graphics loader and makes
; MenuLoadPictureCC78 ($D1F7) the handler: $80 bytes from $0B:D56C queued
; to VRAM $7000, and for the next vertical blank BG1 tiles at $0000 and
; BG2 tiles at $7000, the BG2 map at $7C00 (64 x 32), mode 3, only BG2 on
; the main screen, HDMA off.
; HOW: cross_fade with the new count; the setup through VramDmaQueueAdd and
; FrameByteWritePush.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: fading: as
; MenuCrossFadeStep. Done: A = $D1F7; X unchanged; N set, Z clear; M
; 16-bit, X 16-bit; C, V unchanged.
%at($0BD14A)
MenuCrossFadeOut:
    lda.b !actor_0a,x
    dec a
    dec a
    bmi .lD17C
    sta.b !actor_0a,x
    lsr a
    asl a
    tax
    lda.l !menu_fade_factors,x
    sta.w !scratch_11df
    ldy.w #$003F
    ldx.w #$0080
    jsl.l MenuPaletteScale
    lda.w !scratch_11df
    dec a
    eor.w #$001F
    inc a
    sta.w !scratch_11df
    ldy.w #$0003
    ldx.w #$0018
    jsl.l MenuPaletteScale
    rts
.lD17C:
    sep.b #$20
    rep.b #$30
    lda.w #$0080
    sta.l !vram_queue_staged
    lda.w #$000B
    sta.l !vram_queue_staged+4
    lda.w #$D56C
    sta.l !vram_queue_staged+2
    lda.w #$7000
    sta.l !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    sep.b #$20
    rep.b #$10
    lda.b #$70
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG12NBA
    jsl.l FrameByteWritePush
    plx
    lda.b #$7D
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG2SC
    jsl.l FrameByteWritePush
    plx
    lda.b #$03
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BGMODE
    jsl.l FrameByteWritePush
    plx
    lda.b #$02
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TM
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    rep.b #$30
    lda.w #$D1F7
    sta.b !actor_handler,x
    rts

; @routine 0B:F33C MenuScreenOpen0
; WHAT IT DOES: Opens the menu screen built from spawn list 0: with the
; display off, sets mode 1 (BG3 priority) with BG1-BG3 maps at $6000,
; $6400 and $7C00, tiles at 0, $2000 and $7000 (BG3 and BG4), colour math
; and the sub screen off; resets the sound and the scroll; loads the
; palette from $1C:989C (colour 0 then black) and $720 bytes of tiles from
; $1C:DC8C to VRAM $4000; clears the menu's lists and state; makes the
; objects of spawn list 0, runs them once and builds the sprite table; then
; turns on the IRQ at line $E0 and automatic joypad reading, plays sound
; $39, waits three frames and shows only sprites, with $11DA = $11DB =
; $0F.
; HOW: DB = 0 throughout; the display and interrupts are written directly,
; the frame updates are requested through $11D7 = $0C. The palette source
; goes through direct page $FB.
; Entry: D = $0000. Exit: A = $0010; X, Y as MenuOamBuild left them; P and
; DB unchanged.
%at($0BF33C)
MenuScreenOpen0:
    php
    phb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    sep.b #$20
    rep.b #$10
    lda.b #$80
    sta.w !INIDISP
    stz.w !HDMAEN
    stz.w !MDMAEN
    stz.w !wram_11DA
    stz.w !NMITIMEN
    lda.b #$09
    sta.w !BGMODE
    lda.b #$60
    sta.w !BG1SC
    lda.b #$65
    sta.w !BG2SC
    lda.b #$7C
    sta.w !BG3SC
    lda.b #$20
    sta.w !BG12NBA
    lda.b #$77
    sta.w !BG34NBA
    stz.w !CGADSUB
    stz.w !CGWSEL
    stz.w !TS
    jsl.l SoundReset
    jsr.w MenuScrollReset
    rep.b #$20
    lda.w #$989C
    sta.b !apu_stream_ptr
    lda.w #$001C
    sta.b !apu_stream_bank
    sep.b #$20
    rep.b #$10
    jsr.w MenuPaletteLoad
    rep.b #$30
    lda.w #$0000
    sta.l !palette_buffer
    lda.w #$4000
    sta.w !VMADDL
    ldx.w #$0000
.lF3AE:
    lda.l !menu_tiles_dc8c,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$0720
    bne .lF3AE
    rep.b #$30
    stz.w !byte_writes_size
    stz.w !vram_queue_size
    lda.w #$0006
    sta.w !gsu_buffer_offset
    lda.w #$0000
    sta.l !wram_18FD
    lda.w #$0001
    sta.l !wram_1903
    lda.w #$0000
    sta.l !menu_18b9
    lda.w #$1234
    sta.w !menu_1189
    stz.w !menu_118f
    stz.w !menu_exit
    lda.w #$0000
    sta.w !menu_slide_pos
    sta.w !menu_scroll_y
    jsl.l MenuActorPoolInit
    ldx.w #$0000
    jsl.l MenuActorSpawnList
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.b #$1E
    sta.w !wram_0D3F
    lda.b #$E0
    sta.w !VTIMEL
    stz.w !VTIMEH
    lda.b #$0C
    sta.w !frame_update_flags
    lda.b #$21
    sta.w !NMITIMEN
    jsr.w MenuWaitFrame
    sep.b #$20
    lda.b #$39
    jsl.l SoundPort0QueuePush
    jsr.w MenuWaitFrame
    jsr.w MenuWaitFrame
    sep.b #$20
    lda.b #$0F
    sta.w !wram_11DA
    sta.w !wram_11DB
    lda.b #$10
    sta.w !TM
    plb
    plp
    rts

; @routine 0B:EE24 MenuScreenOpen6
; WHAT IT DOES: Opens the menu screen built from spawn list 6: with the
; display off, sets mode 1 (BG3 priority) with BG1-BG3 maps at $6000,
; $6400 and $7C00, tiles at 0, $2000 and $7000, colour math adding BG2 (on
; the sub screen) to BG1, no fixed colour and no window masks; loads three
; sound blocks (SoundDataLoad $F3B5 with input $1E, then $1B:85F8 and
; $19:9A98), resets the sound and sends 1 on port 0; resets the scroll;
; loads the palette from $1C:989C, saves it, and scales colours 64-127 to
; 1/32 and 12-15 to full; sets up the menu HDMA; draws seven pictures from
; bank $1C into VRAM ($DC38 at $4000, $C9DC at $6800, $BABC at 0, $BE58 at
; $6000, $C664 at $2000, $C818 at $6400, $BECC at $7C00) and copies $40
; bytes from $1C:BE58 to $7000; clears the menu's lists, state and input;
; then menu_screen_start with spawn list 6 and BG1-BG3 and sprites on the
; main screen.
; HOW: DB = 0 throughout. Each sound upload is preceded by port 1 = 5 and
; a delay loop of $4000 steps (the last two followed by another).
; Entry: D = $0000. Exit: A = $0017; X, Y as MenuOamBuild left them; P and
; DB unchanged.
%at($0BEE24)
MenuScreenOpen6:
    php
    phb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    lda.b #$80
    sta.w !INIDISP
    stz.w !HDMAEN
    stz.w !MDMAEN
    stz.w !wram_11DA
    stz.w !NMITIMEN
    lda.b #$09
    sta.w !BGMODE
    lda.b #$60
    sta.w !BG1SC
    lda.b #$65
    sta.w !BG2SC
    lda.b #$7C
    sta.w !BG3SC
    lda.b #$20
    sta.w !BG12NBA
    lda.b #$77
    sta.w !BG34NBA
    lda.b #$01
    sta.w !CGADSUB
    lda.b #$02
    sta.w !CGWSEL
    lda.b #$02
    sta.w !TS
    stz.w !COLDATA
    stz.w !TSW
    stz.w !TMW
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEE82:
    dec a
    bne .lEE82
    lda.w #$F3B5
    ldy.w #$001E
    jsl.l SoundDataLoad
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEE9D:
    dec a
    bne .lEE9D
    lda.w #$001B
    ldy.w #$85F8
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lEEAF:
    dec a
    bne .lEEAF
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEEC0:
    dec a
    bne .lEEC0
    lda.w #$0019
    ldy.w #$9A98
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lEED2:
    dec a
    bne .lEED2
    plp
    jsl.l SoundReset
    sep.b #$20
    lda.b #$01
    sta.w !APUIO0
    jsr.w MenuScrollReset
    rep.b #$30
    rep.b #$20
    lda.w #$989C
    sta.b !apu_stream_ptr
    lda.w #$001C
    sta.b !apu_stream_bank
    rep.b #$20
    rep.b #$10
    jsr.w MenuPaletteLoad
    jsr.w MenuPaletteSave
    lda.w #$0001
    sta.w !scratch_11df
    ldy.w #$003F
    ldx.w #$0080
    jsl.l MenuPaletteScale
    lda.w #$0020
    sta.w !scratch_11df
    ldy.w #$0003
    ldx.w #$0018
    jsl.l MenuPaletteScale
    jsr.w MenuHdmaSetup
    rep.b #$30
    ldx.w #$4000
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_pic_dc38_size
    xba
    sta.w !menu_picture_size
    lda.w #$DC38
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$6800
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_pic_c9dc_size
    xba
    sta.w !menu_picture_size
    lda.w #$C9DC
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w #$7000
    sta.w !VMADDL
    ldx.w #$0000
.lEF70:
    lda.l !menu_pic_be58,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$0040
    bne .lEF70
    rep.b #$30
    ldx.w #$0000
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_pic_babc_size
    xba
    sta.w !menu_picture_size
    lda.w #$BABC
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$6000
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_pic_be58_size
    xba
    sta.w !menu_picture_size
    lda.w #$BE58
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$2000
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_pic_c664_size
    xba
    sta.w !menu_picture_size
    lda.w #$C664
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$6400
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_pic_c818_size
    xba
    sta.w !menu_picture_size
    lda.w #$C818
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$7C00
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_pic_becc_size
    xba
    sta.w !menu_picture_size
    lda.w #$BECC
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    stz.w !byte_writes_size
    stz.w !vram_queue_size
    lda.w #$0006
    sta.w !gsu_buffer_offset
    lda.w #$0000
    sta.l !wram_18FD
    stz.w !menu_joy1_pressed
    stz.w !joy1_pressed
    lda.w #$0000
    sta.l !wram_1903
    sta.l !menu_18b9
    lda.w #$1234
    sta.w !menu_1189
    stz.w !menu_118f
    stz.w !menu_exit
    lda.w #$0000
    sta.w !menu_slide_pos
    sta.w !menu_scroll_y
    jsl.l MenuActorPoolInit
    ldx.w #$0006
    jsl.l MenuActorSpawnList
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.b #$1E
    sta.w !wram_0D3F
    lda.b #$E0
    sta.w !VTIMEL
    stz.w !VTIMEH
    lda.b #$0C
    sta.w !frame_update_flags
    lda.b #$21
    sta.w !NMITIMEN
    jsr.w MenuWaitFrame
    lda.b #$17
    sta.w !TM
    plb
    plp
    rts

; @routine 0B:EDE7 PictureCellAttributes
; WHAT IT DOES: Gives the 2 x 2 cell at X in the Super FX picture map
; ($70:3000 + X, + 2, + $40, + $42) the attribute bits $11DF over each
; tile number (its low 9 bits kept).
; HOW: AND #$01FF, ORA $11DF per word; P saved and restored.
; Entry: X 16-bit; X = the cell. Exit: P restored; A = the last word.
%at($0BEDE7)
PictureCellAttributes:
    php
    rep.b #$30
    lda.l !gsu_picture,x
    and.w #$01FF
    ora.w !scratch_11df
    sta.l !gsu_picture,x
    lda.l !gsu_picture+2,x
    and.w #$01FF
    ora.w !scratch_11df
    sta.l !gsu_picture+2,x
    lda.l !gsu_picture+64,x
    and.w #$01FF
    ora.w !scratch_11df
    sta.l !gsu_picture+64,x
    lda.l !gsu_picture+66,x
    and.w #$01FF
    ora.w !scratch_11df
    sta.l !gsu_picture+66,x
    plp
    rts

; @routine 0B:EA3B ChampionScreenOpen
; WHAT IT DOES: Opens the championship's closing screen: with the display
; off, mode 3 with BG1 and BG2 maps at $6000 and $6400, no colour math, the
; windows for sprites only; three sound blocks (SoundDataLoad $F884 with
; input $1D, ApuUpload $1B:85F8 and $19:9A98), the sound reset and 1 on
; port 0; the scroll reset. When the phase $0DD6 is 8 (won) the class
; $0D4A (0-2) sets its bit (1, 2 or 4) in $70:FC0B; the save's checksum
; renewed. The palette from $04:EBCB loaded and saved, the window HDMA set
; up; pictures drawn by the Super FX: $1D:9068 copied to $7F:A7D7 ($2000
; bytes), $1C:DC8C to VRAM $6000 (each word + $100), $18:C770, $18:998C
; and $1C:989C to VRAM $4000, $2000 and 0; $04:E87B coloured for the
; class, row by row from the tables at $0B:ED9F, $0BEDD7 and $0BEDCF, the
; two cells at $0628 and $052E given the selection's attributes ($0B:EDDF)
; and written to VRAM $6400 ($1000 bytes); then $04:EBCB drawn. The menu's
; lists and state cleared ($1905 = 3, $11EF = $66, $1919 = class * 32,
; $1927 = 0 when won, else $20), the objects of spawn list $36 made, run
; and drawn; HDMA channels 4-7, $0D3F = $20, the IRQ at line $E0, frame
; updates 2 and 3, a frame waited and BG1 and BG2 on the main screen.
; HOW: DB = 0 throughout; P and DB pushed. Each sound load is preceded by
; port 1 = 5 and a delay. Classes 1 and 2 when won, and the phase other
; than 8, are not reached in the traces so far.
; Entry: any widths; D = $0000. Exit: P and DB restored; A, X, Y as
; MenuWaitFrame leaves them.
%at($0BEA3B)
ChampionScreenOpen:
    php
    phb
    sep.b #$20
    rep.b #$10
    lda.b #$00
    pha
    plb
    lda.b #$80
    sta.w !INIDISP
    stz.w !HDMAEN
    stz.w !MDMAEN
    stz.w !wram_11DA
    stz.w !NMITIMEN
    lda.b #$03
    sta.w !BGMODE
    lda.b #$60
    sta.w !BG1SC
    lda.b #$65
    sta.w !BG2SC
    lda.b #$00
    sta.w !BG12NBA
    lda.b #$00
    stz.w !CGADSUB
    stz.w !CGWSEL
    stz.w !TS
    stz.w !COLDATA
    stz.w !TSW
    lda.b #$03
    sta.w !WOBJSEL
    lda.b #$00
    sta.w !WH0
    lda.b #$00
    sta.w !WH1
    lda.b #$10
    sta.w !TMW
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEA9C:
    dec a
    bne .lEA9C
    lda.w #$F884
    ldy.w #$001D
    jsl.l SoundDataLoad
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEAB7:
    dec a
    bne .lEAB7
    lda.w #$001B
    ldy.w #$85F8
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lEAC9:
    dec a
    bne .lEAC9
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEADA:
    dec a
    bne .lEADA
    lda.w #$0019
    ldy.w #$9A98
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lEAEC:
    dec a
    bne .lEAEC
    plp
    jsl.l SoundReset
    sep.b #$20
    lda.b #$01
    sta.w !APUIO0
    jsr.w MenuScrollReset
    sep.b #$20
    lda.l !engine_phase_p1
    cmp.b #$08
    bne .lEB3A
    lda.l !wram_0D4A
    cmp.b #$00
    bne .lEB1C
    lda.l !save_progress
    ora.b #$01
    sta.l !save_progress
    bra .lEB3A
.lEB1C:
    cmp.b #$01
    bne .lEB2C
    lda.l !save_progress
    ora.b #$02
    sta.l !save_progress
    bra .lEB3A
.lEB2C:
    cmp.b #$02
    bne .lEB3A
    lda.l !save_progress
    ora.b #$04
    sta.l !save_progress
.lEB3A:
    jsl.l SaveChecksum
    rep.b #$30
    rep.b #$20
    lda.w #$EBCB
    sta.b !apu_stream_ptr
    lda.w #$0004
    sta.b !apu_stream_bank
    rep.b #$20
    rep.b #$10
    jsr.w MenuPaletteLoad
    jsr.w MenuPaletteSave
    jsr.w WindowHdmaChannelsSetup
    rep.b #$30
    lda.w #$9068
    ldx.w #$001D
    jsl.l GsuCF42To3000
    rep.b #$30
    ldx.w #$0000
.lEB6A:
    lda.l !gsu_picture,x
    sta.l !menu_picture_a,x
    inx
    inx
    cpx.w #$2000
    bne .lEB6A
    rep.b #$30
    lda.w #$DC8C
    ldx.w #$001C
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w #$6000
    sta.w !VMADDL
    ldx.w #$0000
.lEB90:
    lda.l !gsu_picture,x
    clc
    adc.w #$0100
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$0800
    bne .lEB90
    rep.b #$30
    ldx.w #$4000
    stx.w !VMADDL
    rep.b #$30
    lda.l !champion_picture_a-2
    xba
    sta.w !menu_picture_size
    lda.w #$C770
    ldx.w #$0018
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$2000
    stx.w !VMADDL
    rep.b #$30
    lda.l !champion_picture_b-2
    xba
    sta.w !menu_picture_size
    lda.w #$998C
    ldx.w #$0018
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    ldx.w #$0000
    stx.w !VMADDL
    rep.b #$30
    lda.l !menu_picture_palette-2
    xba
    sta.w !menu_picture_size
    lda.w #$989C
    ldx.w #$001C
    rep.b #$20
    rep.b #$10
    jsr.w MenuPictureToVram
    rep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w #$E87B
    ldx.w #$0004
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.l !wram_0D4A
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    tax
    ldy.w #$0003
.lEC28:
    lda.l !champion_row_colours,x
    sta.w !scratch_11e1
    lda.l !champion_row_colours+2,x
    sta.w !scratch_11e3
    phx
    phy
    tya
    asl a
    tax
    lda.l !champion_row_cells,x
    sta.w !scratch_11e5
    phx
    phy
    ldy.w #$0004
.lEC47:
    phy
    ldx.w !scratch_11e5
    ldy.w #$000B
.lEC4E:
    lda.l !gsu_picture,x
    and.w #$03FF
    ora.w !scratch_11e3
    sta.l !gsu_picture,x
    inx
    inx
    dey
    bpl .lEC4E
    ply
    lda.w !scratch_11e5
    clc
    adc.w #$0040
    sta.w !scratch_11e5
    dey
    bpl .lEC47
    ply
    plx
    tya
    asl a
    tax
    lda.l !champion_row_marks,x
    tax
    ldy.w #$0003
    lda.w !scratch_11e1
    ora.w !scratch_11e3
.lEC82:
    pha
    sta.l !gsu_picture,x
    clc
    adc.w #$0010
    sta.l !gsu_picture+64,x
    clc
    adc.w #$0010
    sta.l !gsu_picture+128,x
    pla
    inc a
    inx
    inx
    dey
    bpl .lEC82
    ply
    plx
    inx
    inx
    inx
    inx
    dey
    bpl .lEC28
    lda.l !selection
    and.w #$0003
    asl a
    tax
    lda.l !champion_selection_attrs,x
    sta.w !scratch_11df
    ldx.w #$0628
    jsr.w PictureCellAttributes
    ldx.w #$052E
    jsr.w PictureCellAttributes
    lda.w #$6400
    sta.w !VMADDL
    ldx.w #$0000
.lECCC:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    cpx.w #$1000
    bne .lECCC
    rep.b #$30
    lda.w #$EBCB
    ldx.w #$0004
    jsl.l GsuCF42To3000
    rep.b #$30
    stz.w !byte_writes_size
    stz.w !vram_queue_size
    lda.w #$0006
    sta.w !gsu_buffer_offset
    lda.w #$0000
    sta.l !wram_18FD
    stz.w !menu_joy1_pressed
    stz.w !joy1_pressed
    lda.w #$0000
    sta.l !wram_1903
    sta.l !menu_18b9
    lda.w #$0003
    sta.l !menu_1905
    lda.w #$0000
    sta.l !wram_1921
    sta.l !wram_1925
    lda.w #$0066
    sta.l !menu_scroll_y
    lda.l !wram_0D4A
    and.w #$00FF
    asl a
    asl a
    asl a
    asl a
    asl a
    sta.l !wram_1913+6
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    beq .lED4A
    lda.w #$0020
    sta.l !wram_1927
    bra .lED51
.lED4A:
    lda.w #$0000
    sta.l !wram_1927
.lED51:
    lda.w #$1234
    sta.w !menu_1189
    stz.w !menu_118f
    stz.w !menu_exit
    lda.w #$0000
    sta.w !menu_slide_pos
    jsl.l MenuActorPoolInit
    ldx.w #$0036
    jsl.l MenuActorSpawnList
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    sep.b #$20
    lda.b #$F0
    sta.w !HDMAEN
    lda.b #$20
    sta.w !wram_0D3F
    lda.b #$E0
    sta.w !VTIMEL
    stz.w !VTIMEH
    lda.b #$0C
    sta.w !frame_update_flags
    lda.b #$21
    sta.w !NMITIMEN
    jsr.w MenuWaitFrame
    lda.b #$03
    sta.w !TM
    plb
    plp
    rts

; @routine 0B:ADD4 ChampionScreenRun
; WHAT IT DOES: Runs the championship's closing screen: opens it
; (ChampionScreenOpen), waits a frame, sets the fade target to full
; (ApproachTargetFull); then each frame (MenuWaitFrame) runs the objects,
; builds the sprite table, clears both players' presses ($118B, $118D) and
; asks for frame updates 0-3 ($11D7), until the menu asks to leave
; ($11EB); then the fade target to 0 (ApproachTargetZero) and the menu
; frames run out (MenuRunFrames).
; HOW: 16-bit, with 8-bit for the $11D7 update.
; Entry: D = $0000. Exit: as MenuRunFrames leaves it.
%at($0BADD4)
ChampionScreenRun:
    rep.b #$30
    jsr.w ChampionScreenOpen
    jsr.w MenuWaitFrame
    jsr.w ApproachTargetFull
.lADDF:
    jsr.w MenuWaitFrame
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    rep.b #$20
    stz.w !menu_joy1_pressed
    stz.w !menu_joy2_pressed
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    rep.b #$30
    lda.w !menu_exit
    beq .lADDF
    jsr.w ApproachTargetZero
    jsr.w MenuRunFrames
    rtl

; @routine 0B:AEF2 TitleScreenRun
; WHAT IT DOES: The title screen: opens it (MenuScreenOpen6), $10DA = 0,
; waits a frame and sets the fade target to full, then its loop
; (TitleScreenLoop) with the idle count $1913 at 0.
; HOW: 16-bit registers; $10DA and $1913 by long accesses.
; Entry: any widths; D = $0000. Exit: as TitleScreenLoop leaves it.
%at($0BAEF2)
TitleScreenRun:
    rep.b #$30
    jsr.w MenuScreenOpen6
    sep.b #$20
    lda.b #$00
    sta.l !wram_10DA
    rep.b #$20
    jsr.w MenuWaitFrame
    jsr.w ApproachTargetFull
    rep.b #$30
    lda.w #$0000
    sta.l !wram_1913

; @routine 0B:AF10 TitleScreenLoop
; WHAT IT DOES: The title screen's frames: each runs the objects and builds
; the sprites; while object kind $12 is idle (handler $D606, +$0A at 0) the
; count $1913 goes up, and at $400 the screen fades out, $10DA = $FF and the
; next of three attract demos runs ($1915 counts 0, 1, 2: DemoRunFirst,
; DemoRunSecond, DemoRunFourth); a demo returning C set leaves with A =
; $FFFF, C clear restarts the session (BootWarm). Both players' presses are
; cleared and frame updates 0-3 asked for each frame; the menu's exit
; ($11EB) leaves with A = 0.
; HOW: The idle object through direct page + X; a branch back for the next
; frame. BootWarm resets the stack, so it runs as original code.
; Entry: M 16-bit, X 16-bit; D = $0000. Exit: A = 0 or $FFFF; M 16-bit, X
; 16-bit; or BootWarm, which never returns.
%at($0BAF10)
TitleScreenLoop:
    jsr.w MenuWaitFrame
    jsl.l MenuActorsRun
    jsl.l MenuOamBuild
    rep.b #$20
    lda.w #$0012
    jsr.w MenuActorFind
    lda.b !dp_08,x
    cmp.w #$D606
    bne .lAF88
    lda.b !actor_0a,x
    bne .lAF88
    lda.l !wram_1913
    inc a
    sta.l !wram_1913
    cmp.w #$0400
    bne .lAF88
    jsr.w ApproachTargetZero
    jsr.w MenuRunFrames
    sep.b #$20
    lda.b #$FF
    sta.l !wram_10DA
    rep.b #$20
    lda.l !wram_1913+2
    inc a
    and.w #$0003
    cmp.w #$0003
    bne .lAF5C
    lda.w #$0000
.lAF5C:
    sta.l !wram_1913+2
    cmp.w #$0000
    beq .lAF74
    cmp.w #$0001
    beq .lAF7E
    jsl.l DemoRunFourth
    bcs .lAFA6
    jml.l BootWarm
.lAF74:
    jsl.l DemoRunFirst
    bcs .lAFA6
    jml.l BootWarm
.lAF7E:
    jsl.l DemoRunSecond
    bcs .lAFA6
    jml.l BootWarm
.lAF88:
    stz.w !menu_joy1_pressed
    stz.w !menu_joy2_pressed
    sep.b #$20
    lda.w !frame_update_flags
    ora.b #$0F
    sta.w !frame_update_flags
    rep.b #$30
    lda.w !menu_exit
    bne .lAFA2
    brl TitleScreenLoop
.lAFA2:
    lda.w #$0000
    rtl
.lAFA6:
TitleScreenLoop_quit:
    rep.b #$30
    lda.w #$FFFF
    rtl


; @routine 0B:F443 MenuScrollZeroBg12
; WHAT IT DOES: Queues writes for the next vertical blank that put BG1 and
; BG2 back at scroll 0 (both bytes of BG1HOFS, BG2HOFS, BG1VOFS and
; BG2VOFS). Nothing in the game calls it.
; HOW: Eight FrameByteWritePush calls with 0; X is pushed around each.
; SEP #$30 first, so X and Y keep only their low bytes.
; Entry: any widths. Exit: A low byte = 0, A high byte unchanged; X and Y
; their low bytes (high bytes 0); P unchanged.
%at($0BF443)
MenuScrollZeroBg12:
    php
    sep.b #$30
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1HOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1HOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG2HOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG2HOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG2VOFS
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG2VOFS
    jsl.l FrameByteWritePush
    plx
    plp
    rts
