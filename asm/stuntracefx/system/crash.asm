; crash.asm: the stop screen. Each entry stores a backdrop colour in $1A2E
; and a code in X, then all of them share one body: interrupts and HDMA off,
; the screen blanked and rebuilt as BG1 alone with that colour as colour 0,
; HDMA channel 1 feeding BG1HOFS from the table at $00:812E; then BG1 is
; scrolled down step by step until its vertical scroll ($0719) reaches the
; code ($0717), and the CPU stops in a loop forever.
; C: src/stuntracefx/system/crash.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 00:8014 CrashWhite
; WHAT IT DOES: Stops the game on the stop screen with a white backdrop,
; code $AA.
; HOW: $1A2E = $7FFF, X = $AA, then the shared body at $00:805C (which
; scrolls BG1 to the code between delays of 20000 loop counts, writes 3 to
; APU port 0 at each step, and ends in a loop to itself).
; Entry: any widths; D = $0000. Exit: never returns.
%at($008014)
CrashWhite:
    rep.b #$30
    lda.w #$7FFF
    sta.w !crash_colour
    ldx.w #$00AA
    bra CrashBody

; @routine 00:8048 CrashGreen
; WHAT IT DOES: Stops the game on the stop screen with a green backdrop,
; code $E2: what ObjectSpawnFromScript does when no object is free. Not
; reached in the traces so far.
; HOW: As CrashWhite, with $1A2E = $03E0 and X = $E2.
; Entry: any widths; D = $0000. Exit: never returns.
%at($008048)
CrashGreen:
    rep.b #$30
    lda.w #$03E0
    sta.w !crash_colour
    ldx.w #$00E2
    bra CrashBody

%at($00805C)
CrashBody:                          ; every stop screen entry joins here
    stx.w !crash_code
    sep.b #$20
    stz.w !NMITIMEN
    stz.w !HDMAEN
    sei
    stz.w !TM
    stz.w !TS
    lda.b #$00
    pha
    plb
    sep.b #$20
    rep.b #$10
    lda.b #$80
    sta.w !INIDISP
    stz.w !CGADD
    lda.w !crash_colour
    sta.w !CGDATA
    lda.w !crash_colour+1
    sta.w !CGDATA
    lda.b #$40
    sta.w !BG1SC
    lda.b #$00
    sta.w !BG12NBA
    stz.w !W12SEL
    stz.w !W34SEL
    stz.w !WOBJSEL
    stz.w !WBGLOG
    stz.w !WOBJLOG
    stz.w !TMW
    stz.w !TSW
    stz.w !WH0
    stz.w !WH1
    lda.b #$01
    sta.w !BGMODE
    lda.b #$01
    sta.w !TM
    stz.w !TS
    stz.w !CGADSUB
    stz.w !COLDATA
    stz.w !DMAP1
    lda.b #$00
    sta.w !BBAD1
    lda.b #$00
    sta.w !A1B1
    ldx.w #!crash_hofs_table
    stx.w !A1T1L
    stz.w !DASB1
    stz.w !BG1VOFS
    stz.w !BG1VOFS
    lda.b #$F9
    sta.w !BG1HOFS
    lda.b #$FF
    sta.w !BG1HOFS
    lda.b #$0F
    sta.w !INIDISP
    lda.b #$02
    sta.w !HDMAEN
    rep.b #$10
.step:
    ldy.w #20000
.delay:
    dey
    bne .delay
    rep.b #$20
    lda.w !crash_code
    cmp.w !crash_scroll
    beq .stop
    sec
    sbc.w !crash_scroll
    lsr a
    lsr a
    lsr a
    bne .move
    lda.w #$0001
.move:
    clc
    adc.w !crash_scroll
    sta.w !crash_scroll
    sep.b #$20
    lda.w !crash_scroll
    sta.w !BG1VOFS
    lda.w !crash_scroll+1
    sta.w !BG1VOFS
    lda.b #$03
    sta.w !APUIO0
    bra .step
.stop:
    bra .stop
