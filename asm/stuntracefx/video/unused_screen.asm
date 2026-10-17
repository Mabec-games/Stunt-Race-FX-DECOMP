; unused_screen.asm: a screen set-up in the work-RAM code at $7E:F33C-F6E8
; and the bank 02 helpers it calls, none of which the game calls: a mode 2
; screen with the Super FX picture at $2C00, HDMA from $7E:BC28-C22E and a
; raster IRQ, palettes from !palettes_a145, and a row of 32 tilemap words
; built at !tile_row and sent to VRAM $2FA0.
; C: src/stuntracefx/video/unused_screen.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 7E:F77C ScrollRegsClear
; WHAT IT DOES: Scrolls the four backgrounds to 0, 0. Called at its ROM
; address $02:D4A3; its first three bytes are the last of the work-RAM
; code's copy, so the routine is named by that copy's address.
; HOW: Two zero stores to each of BG1HOFS-BG4VOFS ($210D-$2114: low byte,
; then high), through the data bank. P is saved and restored.
; Entry: any widths. Exit: every register unchanged.
%at($02D4A3)
ScrollRegsClear:
    php
    sep.b #$20
    stz.w !BG1HOFS
    stz.w !BG1HOFS
    stz.w !BG1VOFS
    stz.w !BG1VOFS
    stz.w !BG2HOFS
    stz.w !BG2HOFS
    stz.w !BG2VOFS
    stz.w !BG2VOFS
    stz.w !BG3HOFS
    stz.w !BG3HOFS
    stz.w !BG3VOFS
    stz.w !BG3VOFS
    stz.w !BG4HOFS
    stz.w !BG4HOFS
    stz.w !BG4VOFS
    stz.w !BG4VOFS
    plp
    rtl

; @routine 02:D4D8 PaletteLoad70
; WHAT IT DOES: Loads palette A (16 colours from the table at $03:A145)
; into CGRAM colours $70-$7F.
; HOW: X = A's low byte times 32 (XBA, three LSRs); CGADD = $70, then 32
; long reads from $03:A145,X written to CGDATA. CGADD and CGDATA through
; the data bank. P is saved and restored.
; Entry: any widths. Exit: A = the palette's offset's high byte and the
; last byte read; X = the offset + $20, Y = $20 (their low bytes when X
; is 8-bit); P unchanged.
%at($02D4D8)
PaletteLoad70:
    php
    rep.b #$30
    and.w #$00FF
    xba
    lsr a
    lsr a
    lsr a
    tax
    sep.b #$20
    lda.b #$70
    sta.w !CGADD
    ldy.w #$0000
.loop:
    lda.l !palettes_a145,x
    sta.w !CGDATA
    lda.l !palettes_a145+1,x
    sta.w !CGDATA
    inx
    inx
    iny
    iny
    cpy.w #$0020
    bne .loop
    plp
    rtl

; @routine 02:D506 PaletteToBuffers
; WHAT IT DOES: Copies palette A (16 colours from the table at $03:A145)
; into the colour buffers at $7E:E997 and $7E:0802, with interrupts off.
; HOW: X = A's low byte times 32; data bank $7E; 16 word reads from
; $03:A145,X, each stored to both buffers; then TIMEUP is read (clearing
; a pending IRQ) and interrupts are enabled. DB and P are saved and
; restored.
; Entry: any widths. Exit: A = the last colour's high byte and TIMEUP's
; value; X = the offset + $20, Y = $20 (their low bytes when X is 8-bit);
; DB, P unchanged.
PaletteToBuffers:
    php
    phb
    rep.b #$30
    and.w #$00FF
    xba
    lsr a
    lsr a
    lsr a
    tax
    sei
    sep.b #$20
    lda.b #!palette_buffer_e997>>16
    pha
    plb
    ldy.w #$0000
    rep.b #$20
.loop:
    lda.l !palettes_a145,x
    sta.w !palette_buffer_e997&$FFFF,y
    sta.w !palette_buffer_0802,y
    inx
    inx
    iny
    iny
    cpy.w #$0020
    bne .loop
    sep.b #$20
    lda.l !TIMEUP
    rep.b #$20
    cli
    plb
    plp
    rtl

; @routine 02:D5FD ColoursRedDouble
; WHAT IT DOES: Doubles the red of the 128 colours at $0722 (raising any
; red below 3 to 3 first, capping at 31) and clears the word at $0800.
; HOW: Data bank $00; per colour, green and blue (AND $7FE0) go to $3E,
; the colour is read again for red, and red is put back ORed with $3E.
; The count is kept on the stack. P, Y, X and DB are saved and restored.
; Entry: any widths; D = $0000. Exit: A = 0 (both bytes); X, Y, DB, P
; unchanged.
%at($02D5FD)
ColoursRedDouble:
    php
    phy
    phx
    phb
    sep.b #$20
    lda.b #$00
    pha
    plb
    rep.b #$30
    ldy.w #$0000
    lda.w #$0080
.loop:
    pha
    lda.w !colour_list,y
    and.w #$7FE0
    sta.b !scale_scratch
    lda.w !colour_list,y
    and.w #$001F
    cmp.w #$0003
    beq .raise
    bcs .double
.raise:
    ora.w #$0003
.double:
    asl a
    cmp.w #$001F
    bcc .keep
    lda.w #$001F
.keep:
    and.w #$001F
    ora.b !scale_scratch
    sta.w !colour_list,y
    iny
    iny
    pla
    dec a
    bne .loop
    lda.w #$0000
    sta.w !colour_list_end
    plb
    plx
    ply
    plp
    rtl

; @routine 02:D64A ColoursClear
; WHAT IT DOES: Clears the 224 bytes at $0722-$0801.
; HOW: 8-bit zero stores from X = $DF down to 0, through the data bank.
; X is saved and restored; M is left 8-bit.
; Entry: X 16-bit. Exit: A low byte = 0; X unchanged; M 8-bit; N, Z from
; X.
ColoursClear:
    phx
    sep.b #$20
    lda.b #$00
    ldx.w #$00DF
.loop:
    sta.w !colour_list,x
    dex
    bpl .loop
    plx
    rtl

; @routine 02:D66B Set7EBC2AValues
; WHAT IT DOES: Stores $5F at $7E:BC2A, $2D at $7E:BC2C and $32 at
; $7E:BC2E.
; HOW: Three long byte stores. P is saved and restored.
; Entry: any widths. Exit: A low byte = $32; P unchanged.
%at($02D66B)
Set7EBC2AValues:
    php
    sep.b #$20
    lda.b #$5F
    sta.l !wram_7EBC2B-1
    lda.b #$2D
    sta.l !wram_7EBC2B+1
    lda.b #$32
    sta.l !wram_7EBC2D+1
    plp
    rtl

; @routine 02:D682 ScreenBlankAndWait
; WHAT IT DOES: Turns HDMA off and blanks the screen, stores $80 at
; $7E:BC2B, $BC2D and $BC2F, then waits for scanline $96 and then $8C.
; HOW: HDMAEN = 0 and INIDISP = $80 through the data bank, Set7EBC2BTrio,
; and WaitScanline twice with !scanline_wait = $96, then $8C. P is saved
; and restored.
; Entry: any widths. Exit: A low byte = $8C; P unchanged.
ScreenBlankAndWait:
    php
    sep.b #$20
    rep.b #$10
    stz.w !HDMAEN
    lda.b #$80
    sta.w !INIDISP
    jsl.l Set7EBC2BTrio
    lda.b #$96
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    lda.b #$8C
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    plp
    rtl

; @routine 02:D6AF HardwareRegsInit02Long
; WHAT IT DOES: HardwareRegsInit02 for callers in other banks.
; HOW: JSR, RTL.
; Entry: any widths. Exit: as HardwareRegsInit02.
HardwareRegsInit02Long:
    jsr.w HardwareRegsInit02
    rtl

; @routine 02:D6B3 HardwareRegsInit02
; WHAT IT DOES: Puts the video and CPU registers in a known state, as
; HardwareRegsInit does, except that BG3HOFS is written once.
; HOW: 8-bit registers, data bank $00 (left set); A = 0 for most stores,
; X and Y for the others. P is saved and restored.
; Entry: any widths. Exit: A low byte = 0; X = $FF, Y = $30 (high bytes
; 0); DB = $00; P unchanged.
HardwareRegsInit02:
    php
    sep.b #$30
    lda.b #$00
    pha
    plb
    lda.b #$8F
    sta.w !INIDISP
    lda.b #$00
    sta.w !OBSEL
    sta.w !OAMADDL
    sta.w !OAMADDH
    sta.w !BGMODE
    sta.w !MOSAIC
    sta.w !BG1SC
    sta.w !BG2SC
    sta.w !BG3SC
    sta.w !BG4SC
    sta.w !BG12NBA
    sta.w !BG34NBA
    sta.w !BG1HOFS
    sta.w !BG1HOFS
    sta.w !BG1VOFS
    sta.w !BG1VOFS
    sta.w !BG2HOFS
    sta.w !BG2HOFS
    sta.w !BG2VOFS
    sta.w !BG2VOFS
    sta.w !BG3HOFS
    sta.w !BG3VOFS
    sta.w !BG3VOFS
    sta.w !BG4HOFS
    sta.w !BG4HOFS
    sta.w !BG4VOFS
    sta.w !BG4VOFS
    ldx.b #$80
    stx.w !VMAIN
    sta.w !VMADDL
    sta.w !VMADDH
    sta.w !M7SEL
    ldx.b #$01
    sta.w !M7A
    stx.w !M7A
    sta.w !M7B
    sta.w !M7B
    sta.w !M7C
    sta.w !M7C
    sta.w !M7D
    stx.w !M7D
    sta.w !M7X
    sta.w !M7X
    sta.w !M7Y
    sta.w !M7Y
    sta.w !CGADD
    sta.w !CGDATA
    sta.w !W12SEL
    sta.w !W34SEL
    sta.w !WOBJSEL
    sta.w !WH0
    sta.w !WH1
    sta.w !WH2
    sta.w !WH3
    sta.w !WBGLOG
    sta.w !WOBJLOG
    sta.w !TM
    sta.w !TS
    sta.w !TMW
    ldy.b #$30
    sty.w !CGWSEL
    sta.w !CGADSUB
    ldx.b #$E0
    stx.w !COLDATA
    sta.w !SETINI
    sta.w !NMITIMEN
    ldx.b #$FF
    stx.w !WRIO
    sta.w !WRMPYA
    sta.w !WRMPYB
    sta.w !WRDIVL
    sta.w !WRDIVH
    sta.w !WRDIVB
    sta.w !HTIMEL
    sta.w !HTIMEH
    sta.w !VTIMEL
    sta.w !VTIMEH
    sta.w !MDMAEN
    sta.w !HDMAEN
    sta.w !MEMSEL
    plp
    rts

; @routine 02:D957 TileRowBuildLong
; WHAT IT DOES: TileRowBuild for callers in other banks.
; HOW: JSR, RTL.
; Entry: D = $0000. Exit: as TileRowBuild.
%at($02D957)
TileRowBuildLong:
    jsr.w TileRowBuild
    rtl

; @routine 02:D95B TileRowBuild
; WHAT IT DOES: Unless !tile_row_on is 0, builds the 32 tilemap words at
; !tile_row from one of eight 32-word rows in bank 0 picked by the high
; byte of the word at !tile_row_level (the table of row addresses at
; !tile_row_table, by its low three bits), each word plus the word at
; !tile_row_add. A negative level is negated first and its row is read
; backwards. Nothing is built when the row is the one recorded at
; !tile_row_source; otherwise that records it and !tile_row_built is set
; to 1 (it is cleared first in every case). The LDA #0 after the second
; test of !tile_row_on can never run (the test repeats one just passed).
; HOW: 16-bit registers (left so); work RAM through the data bank; the
; rows through direct page indexed by X (D = $0000, bank 0).
; Entry: D = $0000. Exit: M, X 16-bit. Not built (!tile_row_on = 0): A =
; 0; Z set, N clear. Same row: A = the row address, X = the table offset;
; Z, C set, N clear. Built: A = 1; X = the row address + $40 (forwards)
; or - 2 (backwards), Y = $40; N, Z clear, C set, V from the last add.
TileRowBuild:
    rep.b #$30
    stz.w !tile_row_built
    lda.w !tile_row_on
    and.w #$00FF
    bne .on
    rts
.on:
    and.w #$00FF
    bne .level
    lda.w #$0000
    bra .forwards
.level:
    lda.w !tile_row_level
    bmi .backwards
.forwards:
    xba
    and.w #$0007
    asl a
    tax
    lda.l !tile_row_table,x
    cmp.w !tile_row_source
    bne .build
    rts
.build:
    sta.w !tile_row_source
    tax
    ldy.w #$0000
.copy:
    lda.b !bank0_base,x
    clc
    adc.w !tile_row_add
    sta.w !tile_row,y
    iny
    iny
    inx
    inx
    cpy.w #$0040
    bne .copy
    lda.w #$0001
    sta.w !tile_row_built
    rts
.backwards:
    eor.w #$FFFF
    inc a
    xba
    and.w #$0007
    asl a
    tax
    lda.l !tile_row_table,x
    cmp.w !tile_row_source
    bne .build_back
    rts
.build_back:
    sta.w !tile_row_source
    clc
    adc.w #$003E
    tax
    ldy.w #$0000
.copy_back:
    lda.b !bank0_base,x
    clc
    adc.w !tile_row_add
    sta.w !tile_row,y
    dex
    dex
    iny
    iny
    cpy.w #$0040
    bne .copy_back
    lda.w #$0001
    sta.w !tile_row_built
    rts

; @routine 02:D9E0 TileRowUploadLong
; WHAT IT DOES: TileRowUpload for callers in other banks.
; HOW: 8-bit A, 16-bit index registers around the JSR; P is saved and
; restored.
; Entry: any widths. Exit: A low byte = 1; X = $0040 (its low byte when
; X is 8-bit); P unchanged.
TileRowUploadLong:
    php
    sep.b #$20
    rep.b #$10
    jsr.w TileRowUpload
    plp
    rtl

; @routine 02:D9EA TileRowUpload
; WHAT IT DOES: Sends the 32 tilemap words at !tile_row to VRAM $2FA0.
; HOW: DMA channel 0, mode 1 to VMDATA ($2118) with VMAIN = $80, 64 bytes
; from $00:0866; the registers through the data bank.
; Entry: M 8-bit, X 16-bit. Exit: A low byte = 1; X = $0040.
TileRowUpload:
    lda.b #$80
    sta.w !VMAIN
    lda.b #!VMDATAL&$FF
    sta.w !BBAD0
    ldx.w #!tile_row
    stx.w !A1T0L
    stz.w !A1B0
    ldx.w #$2FA0
    stx.w !VMADDL
    ldx.w #$0040
    stx.w !DAS0L
    lda.b #$01
    sta.w !DMAP0
    lda.b #$01
    sta.w !MDMAEN
    rts
