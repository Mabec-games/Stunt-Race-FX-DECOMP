; windows.asm: the PPU window and colour maths setups the frame code switches
; between. Runs from work RAM. C: src/stuntracefx/video/windows.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:DFE4 WindowsColourAddBg12
; WHAT IT DOES: Sets the windows and colour maths for adding the sub screen to
; BG1 and BG2 inside window 1.
; HOW: W12SEL $99, WOBJSEL $30, CGWSEL $22, CGADSUB $03, WBGLOG $00,
; WOBJLOG $01, TMW $17, in that order.
; Entry: any widths. Exit: A low byte = $17; N, Z clear; M 8-bit.
%ramcode($7EDFE4, $013D0B)
WindowsColourAddBg12:
    sep.b #$20
    lda.b #$99
    sta.l !W12SEL
    lda.b #$30
    sta.l !WOBJSEL
    lda.b #$22
    sta.l !CGWSEL
    lda.b #$03
    sta.l !CGADSUB
    lda.b #$00
    sta.l !WBGLOG
    lda.b #$01
    sta.l !WOBJLOG
    lda.b #$17
    sta.l !TMW
    rts

; @routine 7E:E01D WindowsColourWindowOnly
; WHAT IT DOES: Turns the BG windows off and keeps window 1 for colour only.
; HOW: W12SEL $00, W34SEL $00, WOBJSEL $20, CGWSEL $10, WBGLOG $00,
; WOBJLOG $00, TMW $03, in that order.
; Entry: any widths. Exit: A low byte = $03; N, Z clear; M 8-bit.
%ramcode($7EE01D, $013D44)
WindowsColourWindowOnly:
    sep.b #$20
    lda.b #$00
    sta.l !W12SEL
    sta.l !W34SEL
    lda.b #$20
    sta.l !WOBJSEL
    lda.b #$10
    sta.l !CGWSEL
    lda.b #$00
    sta.l !WBGLOG
    lda.b #$00
    sta.l !WOBJLOG
    lda.b #$03
    sta.l !TMW
    rts

; @routine 7E:E085 WindowsAllInverted
; WHAT IT DOES: Sets every layer's window 1 inverted and colour maths
; subtracting on all layers.
; HOW: CGADSUB $BF, W12SEL $AA, W34SEL $AA, WOBJSEL $AA, CGWSEL $10,
; WBGLOG $00, WOBJLOG $00, TMW $1F, in that order.
; Entry: any widths. Exit: A low byte = $1F; N, Z clear; M 8-bit.
%ramcode($7EE085, $013DAC)
WindowsAllInverted:
    sep.b #$20
    lda.b #$BF
    sta.l !CGADSUB
    lda.b #$AA
    sta.l !W12SEL
    sta.l !W34SEL
    lda.b #$AA
    sta.l !WOBJSEL
    lda.b #$10
    sta.l !CGWSEL
    lda.b #$00
    sta.l !WBGLOG
    sta.l !WOBJLOG
    lda.b #$1F
    sta.l !TMW
    rts
