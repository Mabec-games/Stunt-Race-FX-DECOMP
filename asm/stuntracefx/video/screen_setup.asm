; screen_setup.asm: PPU, window, HDMA and Super FX screen settings written
; when a screen is set up. Register stores go through the data bank, which
; the callers keep in the I/O banks.
; C: src/stuntracefx/video/screen_setup.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:EB0E VideoRegsReset
; WHAT IT DOES: Puts the PPU in a known state: mode 3 with the BG1/BG2 maps
; at $7A/$5A and characters at $60, colour 0 black, windows, colour maths,
; IRQ timers, interrupts and DMA all off; then resets the Super FX screen
; base and buffer start.
; HOW: Register stores in a fixed order. COLDATA is stored with X (16-bit,
; so SETINI gets X's high byte) before SETINI is cleared. The buffer start
; $2C00 goes to $70:021A and direct page $48.
; Entry: X 16-bit, D = $0000. Exit: A = $2C00; P unchanged.
%at($03EB0E)
VideoRegsReset:
    php
    sep.b #$20
    rep.b #$10
    lda.b #$03
    sta.w !BGMODE
    stz.w !MOSAIC
    lda.b #$03
    sta.w !OBSEL
    lda.b #$7A
    sta.w !BG1SC
    lda.b #$5A
    sta.w !BG2SC
    lda.b #$60
    sta.w !BG12NBA
    stz.w !CGADD
    stz.w !CGDATA
    stz.w !CGDATA
    stz.w !TS
    stz.w !TMW
    stz.w !TSW
    stz.w !W12SEL
    stz.w !W34SEL
    stz.w !WOBJSEL
    stz.w !WBGLOG
    stz.w !WOBJLOG
    stz.w !CGWSEL
    stz.w !CGADSUB
    stx.w !COLDATA
    stz.w !SETINI
    stz.w !HTIMEL
    stz.w !HTIMEH
    stz.w !VTIMEL
    stz.w !VTIMEH
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    lda.b #$0B
    sta.w !GSU_SCBR
    rep.b #$20
    lda.w #$2C00
    sta.l !gsu_screen_start
    sta.b !gsu_screen_start_dp
    plp
    rtl

; @routine 03:F32A RaceScreenSetup
; WHAT IT DOES: Sets up the race screen: the Super FX buffer selection, the
; flags at $7F:3411-$7F:3431, HDMA channel 1 (from $7F:3410 to WH0-WH1 via
; mode 1) and channel 2 (the scroll table at $10AA to BG1VOFS via mode 2,
; which it fills from $0E33 and $0E37), the windows and colour maths, BG
; mode $0B with its maps, colour 1 black and every BG scroll at 0.
; HOW: Straight-line stores in a fixed order; work RAM below $2000 and the
; registers are reached through the data bank.
; Entry: D = $0000. Exit: A = $1001; X, Y high bytes clear; M 8-bit, X
; 8-bit; N, Z clear.
%at($03F32A)
RaceScreenSetup:
    rep.b #$20
    lda.w #$0000
    sta.w !gsu_buffer_select
    lda.w #$FFFF
    sta.w !wram_0D81
    sep.b #$30
    lda.b #$01
    sta.l !window_table_7f+1
    sta.l !window_table_7f+4
    sta.l !window_table_7f+6
    sta.l !window_table_7f+9
    sta.l !window_table_7f+12
    sta.l !window_table_7f+21
    sta.l !window_table_7f+24
    sta.l !window_table_7f+27
    sta.l !window_table_7f+30
    sta.l !window_table_7f+31
    lda.b #$00
    sta.l !window_table_7f+2
    sta.l !window_table_7f+5
    sta.l !window_table_7f+32
    sta.l !window_table_7f+33
    stz.w !colour_math_mode
    lda.b #$01
    sta.w !DMAP1
    lda.b #$26
    sta.w !BBAD1
    lda.b #$7F
    sta.w !A1B1
    rep.b #$20
    lda.w #$3410
    sta.w !A1T1L
    sep.b #$20
    lda.b #$30
    sta.w !WOBJSEL
    lda.b #$07
    sta.w !TMW
    lda.b #$02
    sta.w !TM
    lda.b #$03
    sta.w !W34SEL
    lda.b #$20
    sta.w !CGWSEL
    sep.b #$20
    lda.b #$38
    sta.w !scroll_hdma
    lda.w !wram_0E33
    sta.w !scroll_hdma+1
    lda.w !wram_0E33+1
    sta.w !scroll_hdma+2
    lda.b #$4F
    sta.w !scroll_hdma+3
    lda.w !scroll_value
    sta.w !scroll_hdma+4
    lda.w !scroll_value+1
    sta.w !scroll_hdma+5
    lda.b #$01
    sta.w !scroll_hdma+6
    lda.w !scroll_value
    sta.w !scroll_hdma+7
    lda.w !scroll_value+1
    sta.w !scroll_hdma+8
    lda.b #$01
    sta.w !scroll_hdma+9
    lda.w !scroll_value
    sta.w !scroll_hdma+10
    lda.w !scroll_value+1
    sta.w !scroll_hdma+11
    lda.b #$01
    sta.w !scroll_hdma+12
    lda.w !scroll_value
    sta.w !scroll_hdma+13
    lda.w !scroll_value+1
    sta.w !scroll_hdma+14
    lda.b #$00
    sta.w !scroll_hdma+15
    lda.b #$02
    sta.w !DMAP2
    lda.b #$10
    sta.w !BBAD2
    lda.b #$00
    sta.w !A1B2
    rep.b #$20
    lda.w #$10AA
    sta.w !A1T2L
    sep.b #$20
    lda.b #$02
    sta.w !wram_0D45
    lda.b #$F0
    sta.w !wram_0E91
    sta.w !wram_0E92
    lda.b #$0B
    sta.w !BGMODE
    lda.b #$03
    sta.w !OBSEL
    lda.b #$00
    sta.w !BG1SC
    lda.b #$52
    sta.w !BG2SC
    lda.b #$20
    sta.w !BG12NBA
    lda.b #$12
    sta.w !TM
    lda.b #$01
    sta.w !CGADD
    stz.w !CGDATA
    stz.w !CGDATA
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
    rts

; @routine 03:EC6D VideoStartUploads
; WHAT IT DOES: With interrupts off, clears direct page $00, acknowledges the
; Super FX, sets the S-CPU's screen mode value ($0374) to 3 and runs the two
; DMA transfers described at $F873 (to VRAM $0000) and $F87A (to $7C00).
; HOW: NMITIMEN = 0 and SEI; reads $1073 and the Super FX status (16-bit,
; so $3031 too); VMAIN = $80; DmaFromDescriptor twice. P (with the
; interrupt flag) is restored on return.
; Entry: any widths, D = $0000. Exit: A = $F801; X = $80; Y = $FF; P
; unchanged.
%at($03EC6D)
VideoStartUploads:
    php
    sep.b #$30
    sei
    stz.w !NMITIMEN
    stz.b !dp_00
    lda.w !wram_1073
    rep.b #$20
    lda.l !GSU_SFR
    lda.w #$0003
    sta.w !gsu_scmr_shadow
    ldx.b #$80
    stx.w !VMAIN
    lda.w #$0000
    sta.w !VMADDL
    lda.w #$F873
    jsr.w DmaFromDescriptor
    lda.w #$7C00
    sta.w !VMADDL
    lda.w #$F87A
    jsr.w DmaFromDescriptor
    plp
    rtl

; @routine 03:D996 RaceVideoInit
; WHAT IT DOES: RaceVideoSetup, first clearing $0EED-$0EEF and the words at
; $0E6C and $0E6E when $0D24 is 3 or more.
; HOW: Data bank $03 (the program bank) around the call, so the clears go to
; work RAM through it.
; Entry: M 8-bit, D = $0000. Exit: as RaceVideoSetup; the data bank
; unchanged, N and Z from it (as RaceVideoSetup's swapped return leaves P
; otherwise).
%at($03D996)
RaceVideoInit:
    phb
    phk
    plb
    sep.b #$20
    lda.w !wram_0D24
    cmp.b #$03
    bcc .setup
    stz.w !wram_0EED                ; not reached in the traces so far
    stz.w !wram_0EED+1              ; not reached in the traces so far
    stz.w !wram_0EED+2              ; not reached in the traces so far
    rep.b #$20                      ; not reached in the traces so far
    stz.w !wram_0E6C                ; not reached in the traces so far
    stz.w !wram_0E6E                ; not reached in the traces so far
.setup:
    jsl.l RaceVideoSetup
    plb
    rtl

; @routine 03:D9B9 RaceVideoSetup
; WHAT IT DOES: Sets up the screen for the course in $0E3D: resets the PPU
; (VideoRegsReset), starts the uploads (VideoStartUploads), sets the sound
; indices, has the Super FX draw the course's pictures into VRAM, loads the
; course palettes (PaletteCourseLoad, then CgramDmaAll) and gradient colours.
; Then, unless $0D62 is set: the course's pictures, the HUD graphics for the
; HUD mode $0D2B, the framed panels (ScreenFrameAndPictures) and the course
; tilemap built from the Super FX picture into $7F:93D7 (and a second one
; into $7F:BBD7 when the course has one). With $0D62 set: the menu's course
; graphics (MenuCourseGraphics), $7F:9BD7 cleared ($2001 bytes), the Super
; FX picture copied to $7F:93D7 ($800 bytes) and two DMA transfers of it to
; VRAM $7C00 and $6600.
; HOW: Data bank $03 for the course tables (three bytes per course: $F4F9,
; $F54D, $F5A1, $F5F5; $F807 is read and not used). HUD modes 0-3 go through
; the table at $03:DA53: mode 0 (and 3 with $10CD set) draws $9228 and $FAE8
; and copies $80 words of the picture row for $0D8B's low nibble to VRAM
; $5B40; mode 1 (unless $0D4E is set, which goes on as mode 3) draws $FA38,
; sends $180 bytes to VRAM $5AC0 and draws $FB90; mode 3 does the same with
; $100 bytes from $70:3180 and $FC20; mode 2 draws nothing more. Tilemap
; rows come from TilemapRowFromGsuRam, five per step of $40 in direct page
; $F1 up to $800. The two block copies are MVN (byte by byte, read then
; write). The $0D62 path ends with its pulls swapped (PLP takes the data
; bank byte, PLB the status byte), so it returns with P = the caller's data
; bank and the data bank = the caller's P. Mode 3's own drawing, the third
; course picture and the second tilemap are not reached in the traces so
; far.
; Entry: D = $0000; X as VideoRegsReset takes it. Exit: A = 0 (the second
; tilemap: $800); X = $800; Y = course * 3 (the second tilemap: $5000); P
; and the data bank unchanged. With $0D62 set: A = $0401; X = $CF42; Y =
; $9BD7; P and the data bank swapped as above.
%at($03D9B9)
RaceVideoSetup:
    php
    sep.b #$20
    phb
    phk
    plb
    rep.b #$20
    jsl.l VideoRegsReset
    jsl.l VideoStartUploads
    jsl.l SoundIndicesFromHandlers
    rep.b #$30
    ldy.w #$F80D
    ldx.w #$F7DA
    jsl.l GsuCF42ToVram
    rep.b #$30
    lda.w !course
    asl a
    clc
    adc.w !course
    and.w #$00FF
    pha
    jsr.w PaletteCourseLoad
    jsl.l CgramDmaAll
    rep.b #$30
    plx
    phx
    lda.w !course_table_f807_w,x
    sep.b #$20
    plx
    phx
    jsr.w GradientBuildColours
    sep.b #$20
    lda.w !wram_0D62
    beq .draw
    ply
    jmp.w .menu
.draw:
    rep.b #$30
    pla
    pha
    clc
    adc.w #!course_picture_a_w
    tax
    ldy.w #$F853
    jsl.l GsuCF42ToVram
    rep.b #$30
    plx
    phx
    lda.w !course_picture_c_w,x
    beq .no_third
    txa                             ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.w #!course_picture_c_w      ; not reached in the traces so far
    tax                             ; not reached in the traces so far
    ldy.w #$F85A                    ; not reached in the traces so far
    jsl.l GsuCF42ToVram             ; not reached in the traces so far
.no_third:
    rep.b #$30
    ldy.w #$F814
    ldx.w #$F7DD
    jsl.l GsuCF42ToVram
    rep.b #$30
    lda.w #$D970
    ldx.w #$001D
    jsl.l GsuCF42To3000
    jsl.l CoursePictureToVram
    sep.b #$30
    lda.w !hud_mode
    asl a
    tax
    jmp.w (.modes,x)
.modes:
    dw .mode0, .mode1, .mode2, .mode3
.mode3:
    lda.w !wram_10CD
    beq .mode3_draw
    brl .mode0
.mode3_draw:
    rep.b #$30                      ; not reached in the traces so far
    ldy.w #$F830                    ; not reached in the traces so far
    ldx.w #$F7D4                    ; not reached in the traces so far
    jsl.l GsuCF42ToVram             ; not reached in the traces so far
    rep.b #$30                      ; not reached in the traces so far
    ldx.w #$0018                    ; not reached in the traces so far
    lda.w #$FA38                    ; not reached in the traces so far
    jsl.l GsuCF42To3000             ; not reached in the traces so far
    rep.b #$30                      ; not reached in the traces so far
    php                             ; not reached in the traces so far
    rep.b #$20                      ; not reached in the traces so far
    lda.w #$1801                    ; not reached in the traces so far
    sta.l !DMAP0                    ; not reached in the traces so far
    lda.w #$3180                    ; not reached in the traces so far
    sta.l !A1T0L                    ; not reached in the traces so far
    lda.w #$5AC0                    ; not reached in the traces so far
    sta.l !VMADDL                   ; not reached in the traces so far
    lda.w #$0100                    ; not reached in the traces so far
    sta.l !DAS0L                    ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$80                      ; not reached in the traces so far
    sta.l !VMAIN                    ; not reached in the traces so far
    lda.b #$70                      ; not reached in the traces so far
    sta.l !A1B0                     ; not reached in the traces so far
    lda.b #$01                      ; not reached in the traces so far
    sta.l !MDMAEN                   ; not reached in the traces so far
    plp                             ; not reached in the traces so far
    rep.b #$30                      ; not reached in the traces so far
    ldx.w #$0018                    ; not reached in the traces so far
    lda.w #$FC20                    ; not reached in the traces so far
    jsl.l GsuCF42To3000             ; not reached in the traces so far
    jmp.w .mode2                    ; not reached in the traces so far
.mode1:
    sep.b #$20
    lda.w !hud_blank
    bne .mode3
    rep.b #$30
    ldy.w #$F830
    ldx.w #$F7D4
    jsl.l GsuCF42ToVram
    rep.b #$30
    ldx.w #$0018
    lda.w #$FA38
    jsl.l GsuCF42To3000
    rep.b #$30
    php
    rep.b #$20
    lda.w #$1801
    sta.l !DMAP0
    lda.w #$3000
    sta.l !A1T0L
    lda.w #$5AC0
    sta.l !VMADDL
    lda.w #$0180
    sta.l !DAS0L
    sep.b #$20
    lda.b #$80
    sta.l !VMAIN
    lda.b #$70
    sta.l !A1B0
    lda.b #$01
    sta.l !MDMAEN
    plp
    rep.b #$30
    ldx.w #$0018
    lda.w #$FB90
    jsl.l GsuCF42To3000
    bra .mode2
.mode0:
    rep.b #$30
    ldy.w #$F829
    ldx.w #$F7D1
    jsl.l GsuCF42ToVram
    rep.b #$30
    lda.w #$9228
    ldx.w #$001D
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w !wram_0D8B
    and.w #$000F
    xba
    tax
    ldy.w #$0080
    lda.w #$5B40
    sta.w !VMADDL
.mode0_words:
    lda.l !gsu_picture,x
    sta.w !VMDATAL
    inx
    inx
    dey
    bne .mode0_words
    rep.b #$30
    ldx.w #$0018
    lda.w #$FAE8
    jsl.l GsuCF42To3000
.mode2:
    rep.b #$30
    jsr.w ScreenFrameAndPictures
    rep.b #$30
    ply
    phy
    lda.w !course_picture_b_w+2,y
    tax
    lda.w !course_picture_b_w,y
    jsl.l GsuCF42To3000
    sep.b #$20
    lda.b #$7F
    phb
    pha
    plb
    rep.b #$30
    ldy.w #$0000
    stz.b !tilemap_row_offset
.rows:
    lda.w #$0000
    jsr.w TilemapRowFromGsuRam
    lda.w #$0800
    jsr.w TilemapRowFromGsuRam
    lda.w #$1000
    jsr.w TilemapRowFromGsuRam
    lda.w #$1800
    jsr.w TilemapRowFromGsuRam
    lda.w #$0000
    jsr.w TilemapRowFromGsuRam
    lda.b !tilemap_row_offset
    clc
    adc.w #$0040
    sta.b !tilemap_row_offset
    cmp.w #$0800
    bcc .rows
    sep.b #$20
    plb
    rep.b #$30
    ply
    lda.w !course_picture_d_w+2,y
    and.w #$00FF
    beq .done
    tax                             ; not reached in the traces so far
    lda.w !course_picture_d_w,y     ; not reached in the traces so far
    jsl.l GsuCF42To3000             ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$7F                      ; not reached in the traces so far
    phb                             ; not reached in the traces so far
    pha                             ; not reached in the traces so far
    plb                             ; not reached in the traces so far
    rep.b #$30                      ; not reached in the traces so far
    ldy.w #$2800                    ; not reached in the traces so far
    stz.b !tilemap_row_offset       ; not reached in the traces so far
.rows_second:
    lda.w #$0000                    ; not reached in the traces so far
    jsr.w TilemapRowFromGsuRam      ; not reached in the traces so far
    lda.w #$0800                    ; not reached in the traces so far
    jsr.w TilemapRowFromGsuRam      ; not reached in the traces so far
    lda.w #$1000                    ; not reached in the traces so far
    jsr.w TilemapRowFromGsuRam      ; not reached in the traces so far
    lda.w #$1800                    ; not reached in the traces so far
    jsr.w TilemapRowFromGsuRam      ; not reached in the traces so far
    lda.w #$0000                    ; not reached in the traces so far
    jsr.w TilemapRowFromGsuRam      ; not reached in the traces so far
    lda.b !tilemap_row_offset       ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.w #$0040                    ; not reached in the traces so far
    sta.b !tilemap_row_offset       ; not reached in the traces so far
    cmp.w #$0800                    ; not reached in the traces so far
    bcc .rows_second                ; not reached in the traces so far
    plb                             ; not reached in the traces so far
.done:
    sep.b #$20
    plb
    plp
    rtl
.menu:
    jsl.l MenuCourseGraphics
    phb
    php
    rep.b #$30
    lda.w #$0000
    sta.l !tilemap_7f9bd7
    lda.w #$1FFF
    ldx.w #!tilemap_7f9bd7
    ldy.w #!tilemap_7f9bd7+1
    mvn !tilemap_7f9bd7>>16,!tilemap_7f9bd7>>16
    plp
    plb
    rep.b #$30
    ldy.w #$F84C
    ldx.w #$F7E3
    jsl.l GsuCF42ToVram
    rep.b #$30
    phb
    php
    rep.b #$30
    lda.w #$07FF
    ldx.w #!gsu_picture
    ldy.w #!tilemap_7f93d7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    rep.b #$30
    lda.w #$C770
    ldx.w #$0018
    jsl.l GsuCF42To3000
    rep.b #$30
    php
    rep.b #$20
    lda.w #$1801
    sta.l !DMAP0
    lda.w #$3000
    sta.l !A1T0L
    lda.w #$7C00
    sta.l !VMADDL
    lda.w #$0800
    sta.l !DAS0L
    sep.b #$20
    lda.b #$80
    sta.l !VMAIN
    lda.b #$70
    sta.l !A1B0
    lda.b #$01
    sta.l !MDMAEN
    plp
    rep.b #$30
    php
    rep.b #$20
    lda.w #$1801
    sta.l !DMAP0
    lda.w #$3600
    sta.l !A1T0L
    lda.w #$6600
    sta.l !VMADDL
    lda.w #$0400
    sta.l !DAS0L
    sep.b #$20
    lda.b #$80
    sta.l !VMAIN
    lda.b #$70
    sta.l !A1B0
    lda.b #$01
    sta.l !MDMAEN
    plp
    plp
    plb
    rtl

; @routine 03:DD1B RaceHudLoad
; WHAT IT DOES: Loads a race's sound and HUD graphics: the sound reset, the
; car $0D89 = 4 when $0D4C is set in HUD mode 0 (else the sounds chosen,
; SoundIndicesFromHandlers), the course's sound (CourseSoundLoad); the
; Super FX picture $1D:8078, and by HUD mode $0D2B (the table at $03:DD56)
; tiles of it to VRAM: mode 0 the tiles $18E5, $18E7 and $18E9 (from
; $30A0) at $62F0, $63E0 and $63F0, then as mode 1; mode 1 nothing more
; unless $0D4E is set, which goes on as mode 3; mode 2 tile $18E5 (from
; $3000) at $62F0; mode 3 $20 bytes from $70:3080 to $63F0. Then tile
; $18E3 (from $3000) at $62E0, the Super FX blocks $F845/$F804 to VRAM
; and $18:D920 drawn, $3000 bytes of it copied to $7F:63D7, the sound reset
; again, the race time set up (RaceTimeInit), the Super FX's speed $70:0050
; from the course's word at $03:F799; then on to $03:DECA, or $03:E6FA
; with $0D62 set, which pull P and the data bank pushed here.
; HOW: P and the data bank ($03) pushed; MVN for the copy; the last part
; is a jump (jump_routine_pushed with the two bytes). HUD modes above 3
; would take code bytes as addresses and are not modelled.
; Entry: any widths; D = $0000. Exit: as the code at $03:DECA or $03:E6FA
; leaves it.
%at($03DD1B)
RaceHudLoad:
    php
    sep.b #$20
    phb
    phk
    plb
    jsl.l SoundReset
    sep.b #$20
    lda.w !digit62_override
    beq .lDD38
    lda.w !hud_mode
    bne .lDD38
    lda.b #$04
    sta.w !wram_0D89
    bra .lDD3C
.lDD38:
    jsl.l SoundIndicesFromHandlers
.lDD3C:
    jsl.l CourseSoundLoad
    rep.b #$30
    lda.w #$8078
    ldx.w #$001D
    jsl.l GsuCF42To3000
    rep.b #$30
    lda.w !hud_mode
    asl a
    tax
    jmp.w (RaceHudLoad_modes,x)
RaceHudLoad_modes:
    dw .lDDA4, .lDDCE, .lDD94, .lDD5E
.lDD5E:
    php
    rep.b #$20
    lda.w #$1801
    sta.l !DMAP0
    lda.w #$3080
    sta.l !A1T0L
    lda.w #$63F0
    sta.l !VMADDL
    lda.w #$0020
    sta.l !DAS0L
    sep.b #$20
    lda.b #$80
    sta.l !VMAIN
    lda.b #$70
    sta.l !A1B0
    lda.b #$01
    sta.l !MDMAEN
    plp
    bra .lDDD5
.lDD94:
    lda.w #$3000
    sta.b !digit_count
    lda.w !wram_18E5
    ldx.w #$62F0
    jsr.w VramDmaTileFromGsuRam
    bra .lDDD5
.lDDA4:
    lda.w #$30A0
    sta.b !digit_count
    lda.w !wram_18E5
    ldx.w #$62F0
    jsr.w VramDmaTileFromGsuRam
    lda.w #$30A0
    sta.b !digit_count
    lda.w !wram_18E7
    ldx.w #$63E0
    jsr.w VramDmaTileFromGsuRam
    lda.w #$30A0
    sta.b !digit_count
    lda.w !wram_18E9
    ldx.w #$63F0
    jsr.w VramDmaTileFromGsuRam
.lDDCE:
    sep.b #$20
    lda.w !hud_blank
    bne .lDD5E
.lDDD5:
    rep.b #$30
    lda.w #$3000
    sta.b !digit_count
    lda.w !selection_derived
    ldx.w #$62E0
    jsr.w VramDmaTileFromGsuRam
    rep.b #$30
    ldy.w #$F845
    ldx.w #$F804
    jsl.l GsuCF42ToVram
    rep.b #$30
    lda.w #$D920
    ldx.w #$0018
    jsl.l GsuCF42To3000
    phb
    php
    rep.b #$30
    lda.w #$2FFF
    ldx.w #$3000
    ldy.w #$63D7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    sep.b #$20
    jsl.l SoundReset
    sep.b #$20
    jsr.w RaceTimeInit
    rep.b #$30
    lda.w !course
    and.w #$00FF
    asl a
    tax
    lda.l !course_gsu_speeds,x
    sta.l !gsu_speed_50
    sep.b #$20
    lda.w !wram_0D62
    bne .lDE36
    brl RaceHudLoadRace
.lDE36:
    jmp.w RaceHudLoadMenu
