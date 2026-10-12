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
