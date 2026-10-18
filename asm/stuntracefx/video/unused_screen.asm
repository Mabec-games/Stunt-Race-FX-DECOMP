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
    sta.w !palette_buffer_e997,y
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
; test of !tile_row_on never runs (the test repeats one just passed); it
; runs only as TileRowBuildZero.
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

; @routine 02:D96E TileRowBuildZero
; WHAT IT DOES: TileRowBuild's LDA #0 after its second $08ED test, which
; never runs (the test repeats one just passed): the row for level 0 read
; forwards, as TileRowBuild from its row pick.
; HOW: LDA #0, BRA to the pick; then as TileRowBuild. No code reaches
; these bytes; a direct call is their only way in.
; Entry: M, X 16-bit; D = $0000. Exit: as TileRowBuild when it reaches its
; row pick.
%at($02D96E)
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
    lda.b #$18                  ; B bus $2118, VRAM data
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

%ramcode($7EF3E3, $01510A)

; @routine 7E:F3E3 UnusedScreenStart
; WHAT IT DOES: Waits for the frame flag $00 to clear, sets it to 2 and
; clears $0306; waits for $0854 to be set, runs GsuClearScreenIfSet,
; waits for $0854 to be 2 and clears it; then SpriteSlotFirstLong and the
; stop screen (CrashWhite). Nothing in the game calls it.
; HOW: Polling loops on direct page $00 (D = $0000) and on $0854 through
; the data bank; 8-bit A, 16-bit index registers.
; Entry: D = $0000. Exit: never returns.
UnusedScreenStart:
    php
    sep.b #$20
    rep.b #$10
.lF3E8:
    lda.b !dp_00
    bne .lF3E8
    lda.b #$02
    sta.b !dp_00
    stz.w !frames_since_tick
.lF3F3:
    lda.w !wram_084C+8
    beq .lF3F3
    jsl.l GsuClearScreenIfSet
.lF3FC:
    lda.w !wram_084C+8
    cmp.b #$02
    bne .lF3FC
    stz.w !wram_084C+8
    jsl.l SpriteSlotFirstLong
    jsl.l CrashWhite

; @routine 7E:F40E UnusedScreenSetup
; WHAT IT DOES: Sets up the unused screen, interrupts off: the NMI off,
; the frame flags $00 and $084C cleared, the Super FX's SCMR shadow $0374 =
; $21; HDMA off, the screen blanked and two scanline waits
; (ScreenBlankAndWait); all of VRAM cleared and the tilemap at $2C00
; filled with $01A1, then UnusedScreenTiles; the Super FX picture at
; $2C00 ($48 = $4A = $2C00, $50 = $3000, $4E = 0; SCBR and BG12NBA from
; them); mode 2 with BG1 at $2C00 and BG2 at $7300; the tile row
; (TileRowBuildLong, TileRowUploadLong); palette 1 to colours $70-$7F;
; colours $80-$FF by DMA from $7F:FA17; windows for the sprites only;
; BG1-3 and sprites on the main screen, BG1-3 on the sub; the $7E:BC2A
; values; HDMA channels 1-4 from $7E:BC28, $BC50, $B7A0, $C22E; $0316 =
; $1E and the HDMA pointers at $084D, $084F, $083E, $0840 (and $0842 =
; 0); the scroll cleared; a raster IRQ at line $8F; then, with $80 at
; $7E:BC2B-BC2F, the screen still blanked and HDMA channel 1 on, it waits
; for lines $BE and $64 and turns on the NMI, the vertical IRQ and the
; joypad read ($31), sets $05D9 = $FF and turns interrupts on.
; HOW: Registers through the data bank (16-bit stores where M is 16-bit
; write the next register too); direct page with D = $0000; reads TIMEUP,
; the Super FX status and RDNMI to acknowledge them.
; Entry: D = $0000. Exit: M 8-bit, X 16-bit; A = TIMEUP's value (high
; byte 0); X = 0, Y = $20; I clear, N, Z from A; C and V as TileRowBuild
; left them (C from bit 3 of $4F when it built nothing).
UnusedScreenSetup:
    sep.b #$30
    sei
    stz.w !NMITIMEN
    stz.b !dp_00
    stz.w !wram_084C
    lda.w !TIMEUP
    rep.b #$20
    lda.l !GSU_SFR
    lda.w #$0021
    sta.w !gsu_scmr_shadow
    jsl.l ScreenBlankAndWait
    stz.w !CGADD
    stz.w !VMADDL
    stz.w !VMADDH
    lda.w #$0080
    sta.w !INIDISP
    sta.w !VMAIN
    lda.w #$0000
    ldx.b #$7F
.lF443:
    ldy.b #$00
.lF445:
    sta.w !VMDATAL
    sta.w !VMDATAH
    dey
    bne .lF445
    dex
    bne .lF443
    rep.b #$30
    lda.w #$2C00
    sta.w !VMADDL
    lda.w #$01A1
    ldx.w #$0400
.lF45F:
    sta.w !VMDATAL
    dex
    bne .lF45F
    sep.b #$20
    rep.b #$10
    jsl.l UnusedScreenTilesRom
    rep.b #$10
    ldx.w #$0000
    stx.b !fxrec_speed+4
    ldx.w #$3000
    stx.b !fxpart_50
    ldx.w #$2C00
    stx.b !gsu_screen_start_dp
    ldx.w #$2C00
    stx.b !fxrec_speed
    lda.b !gsu_screen_start_dp+1
    lsr a
    lsr a
    sta.w !GSU_SCBR
    lda.b !fxrec_speed+5
    lsr a
    lsr a
    lsr a
    lsr a
    ora.b #$50
    sta.w !BG12NBA
    lda.b #$03
    sta.w !OBSEL
    lda.b #$2C
    sta.w !BG1SC
    lda.b #$73
    sta.w !BG2SC
    lda.b #$02
    sta.w !BGMODE
    jsl.l TileRowBuildLong
    jsl.l TileRowUploadLong
    sep.b #$20
    rep.b #$10
    lda.b #$01
    jsl.l PaletteLoad70
    lda.b #$22
    sta.w !BBAD0
    lda.b #$80
    sta.w !CGADD
    ldx.w #$FA17
    stx.w !A1T0L
    lda.b #$7F
    sta.w !A1B0
    ldx.w #$0100
    stx.w !DAS0L
    lda.b #$00
    sta.w !DMAP0
    lda.b #$01
    sta.w !MDMAEN
    stz.w !W12SEL
    stz.w !W34SEL
    lda.b #$20
    sta.w !WOBJSEL
    stz.w !WBGLOG
    lda.b #$0C
    sta.w !WOBJLOG
    stz.w !TMW
    stz.w !TSW
    stz.w !WH0
    stz.w !WH1
    lda.b #$17
    sta.w !TM
    lda.b #$07
    sta.w !TS
    jsl.l Set7EBC2AValues
    sep.b #$20
    lda.b #$00
    sta.w !DMAP1
    lda.b #$00
    sta.w !BBAD1
    ldx.w #$BC28
    stx.w !A1T1L
    lda.b #$7E
    sta.w !A1B1
    stz.w !DASB1
    lda.b #$02
    sta.w !DMAP2
    lda.b #$0F
    sta.w !BBAD2
    ldx.w #$BC50
    stx.w !A1T2L
    lda.b #$7E
    sta.w !A1B2
    stz.w !DASB2
    lda.b #$01
    sta.w !DMAP3
    lda.b #$26
    sta.w !BBAD3
    ldx.w #$B7A0
    stx.w !A1T3L
    lda.b #$7E
    sta.w !A1B3
    stz.w !DASB3
    lda.b #$02
    sta.w !DMAP4
    lda.b #$10
    sta.w !BBAD4
    ldx.w #$C22E
    stx.w !A1T4L
    lda.b #$7E
    sta.w !A1B4
    stz.w !DASB4
    lda.b #$1E
    sta.w !joy2_pressed+3
    ldx.w #$BC50
    stx.w !wram_084C+1
    ldx.w #$BF3F
    stx.w !wram_084C+3
    ldx.w #$B7A0
    stx.w !palette_buffer_0802+60
    ldx.w #$B9E4
    stx.w !palette_buffer_0802+62
    stz.w !palette_buffer_0802+64
    jsl.l ScrollRegsClear
    sep.b #$20
    ldx.w #$008F
    stx.w !VTIMEL
    ldx.w #$0000
    stx.w !HTIMEL
    lda.b #$80
    jsl.l Set7EBC2BTrio
    sta.w !INIDISP
    stz.w !wram_084C
    lda.b #$02
    sta.w !HDMAEN
    lda.b #$BE
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    lda.b #$64
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    lda.w !RDNMI
    lda.b #$31
    sta.w !NMITIMEN
    lda.b #$FF
    sta.w !slot_mask+4
    lda.l !TIMEUP
    cli
    rtl

; @routine 7E:F5DF UnusedScreenTilesLocked
; WHAT IT DOES: UnusedScreenTiles with interrupts off, then clears any
; pending IRQ and turns interrupts on.
; HOW: PHP, SEI, JSL to the ROM copy $02:D313, a long read of TIMEUP,
; CLI, PLP.
; Entry: any widths. Exit: A = UnusedScreenTiles' high byte and TIMEUP's
; value; X = $FFFF, Y = 0 (their low bytes when X is 8-bit); P unchanged.
UnusedScreenTilesLocked:
    php
    sei
    jsl.l UnusedScreenTilesRom
    lda.l !TIMEUP
    cli
    plp
    rtl

; @routine 7E:F5EC UnusedScreenTiles
; WHAT IT DOES: Writes the unused screen's tilemaps to VRAM: from $2C40,
; 16 rows of three $39A0 words, 26 words counting up by $10 and three more
; $39A0 words (row r starting at $1C00 + r); when the Super FX RAM byte
; $70:023A is set, ten single words at $2F03-$2F07 and $2F18-$2F1C; then
; 16 words of $FFFF at $1A00 and 16 at $4A00. The work-RAM copy is called
; at its ROM address $02:D313.
; HOW: Data bank $00; VMAIN = $80; 16-bit stores to VMADDL and VMDATAL.
; DB is saved and restored; the count of the inner loop is on the stack.
; Entry: any widths. Exit: M 8-bit, X 16-bit; A = $2A86, or 0 when
; $70:023A is 0; X = $FFFF, Y = 0; N, Z from DB; C set, V clear.
UnusedScreenTiles:
    sep.b #$20
    phb
    lda.b #$00
    pha
    plb
    lda.b #$80
    sta.w !VMAIN
    rep.b #$30
    lda.w #$2C40
    sta.w !VMADDL
    lda.w #$1C00
    ldy.w #$0000
.lF606:
    pha
    lda.w #$39A0
    sta.w !VMDATAL
    sta.w !VMDATAL
    sta.w !VMDATAL
    pla
    ldx.w #$0000
.lF617:
    sta.w !VMDATAL
    clc
    adc.w #$0010
    inx
    cpx.w #$001A
    bne .lF617
    pha
    lda.w #$39A0
    sta.w !VMDATAL
    sta.w !VMDATAL
    sta.w !VMDATAL
    pla
    sec
    sbc.w #$019F
    iny
    cpy.w #$0010
    bne .lF606
    lda.l !fx_noise_seed+30
    and.w #$00FF
    beq .lF6BD
    lda.w #$2F03
    sta.w !VMADDL
    lda.w #$282E
    sta.w !VMDATAL
    lda.w #$2F04
    sta.w !VMADDL
    lda.w #$2846
    sta.w !VMDATAL
    lda.w #$2F05
    sta.w !VMADDL
    lda.w #$285E
    sta.w !VMDATAL
    lda.w #$2F06
    sta.w !VMADDL
    lda.w #$2876
    sta.w !VMDATAL
    lda.w #$2F07
    sta.w !VMADDL
    lda.w #$288E
    sta.w !VMDATAL
    lda.w #$2F18
    sta.w !VMADDL
    lda.w #$2A26
    sta.w !VMDATAL
    lda.w #$2F19
    sta.w !VMADDL
    lda.w #$2A3E
    sta.w !VMDATAL
    lda.w #$2F1A
    sta.w !VMADDL
    lda.w #$2A56
    sta.w !VMDATAL
    lda.w #$2F1B
    sta.w !VMADDL
    lda.w #$2A6E
    sta.w !VMDATAL
    lda.w #$2F1C
    sta.w !VMADDL
    lda.w #$2A86
    sta.w !VMDATAL
.lF6BD:
    ldx.w #$1A00
    stx.w !VMADDL
    ldx.w #$FFFF
    ldy.w #$0010
.lF6C9:
    stx.w !VMDATAL
    dey
    bne .lF6C9
    ldx.w #$4A00
    stx.w !VMADDL
    ldx.w #$FFFF
    ldy.w #$0010
.lF6DB:
    stx.w !VMDATAL
    dey
    bne .lF6DB
    plb
    sep.b #$20
    rtl

; @routine 7E:F6E5 ObjectLinkBobStartLong
; WHAT IT DOES: A second copy of ViewParamsBobLong ($7E:F737), which
; nothing calls: starts bob 1 with A as its kind ($1A1D = A, $1A1F =
; $1A23 = 0, $1A21 = $FE).
; HOW: JSR ObjectLinkBobStart, RTL.
; Entry: M 8-bit (the code is 8-bit); data bank $7E. Exit: M and X 8-bit
; (X and Y high bytes 0); A low byte = $FE; N set, Z clear.
%ramcode($7EF6E5, $01540C)
ObjectLinkBobStartLong:
    jsr.w ObjectLinkBobStart
    rtl
