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

; @routine 7E:DFCA WindowsSetupSelect
; WHAT IT DOES: Applies the window and colour maths setup chosen by $19A9:
; 0 WindowsColourAddBg12, 2 ColourMathSub83Windowed, 4 colour maths $03
; with the windows for colour only, 6 colour maths $81 with BG1 alone
; inside the window, 8 WindowsAllInverted.
; HOW: An indexed indirect call through the five-entry table at $7E:DFDA,
; with the data bank set to $7E. Only 0, 2 and 8 are seen in play.
; Entry: any widths; $19A9 = 0, 2, 4, 6 or 8. Exit: A low byte as the
; setup leaves it; X = $19A9 (high byte clear); P and the data bank
; unchanged.
%ramcode($7EDFCA, $013CF1)
WindowsSetupSelect:
    php
    phb
    phk
    plb
    sep.b #$30
    lda.w !windows_setup_index
    tax
    jsr.w (!windows_setup_table,x)
    plb
    plp
    rtl

; @routine 7E:DF61 BobStep
; WHAT IT DOES: Steps bob X (0 or 1) at half rate: on every second call
; ($1A1E + X toggling) its count $1A22 + X goes up; at 10 the count restarts
; and bit 7 of $0DA4 (bob 0) or $0DA6 (bob 1) is cleared; otherwise its
; position $1A1C + X moves by its velocity $1A20 + X, which turns round at $1C
; and $16.
; HOW: 8-bit; long accesses to bank 0 except the velocity add and the flag
; bytes (data bank). Bob 1's flag is not reached in the traces so far.
; Entry: X = bob. Exit: M 8-bit, X 8-bit (high bytes cleared); A low byte and
; flags from the last step taken.
%ramcode($7EDF61, $013C88)
BobStep:
    sep.b #$30
    lda.l !wram_1A1E,x
    clc
    adc.b #$01
    cmp.b #$02
    beq .count
    sta.l !wram_1A1E,x
    jmp.w .done
.count:
    lda.l !wram_1A22,x
    clc
    adc.b #$01
    sta.l !wram_1A22,x
    cmp.b #$0A
    bne .move
    lda.b #$00
    sta.l !wram_1A22,x
    cpx.b #$01
    beq .bob1
    lda.w !wram_0DA4
    and.b #$7F
    sta.w !wram_0DA4
    bra .flagged
.bob1:
    lda.w !wram_0DA6
    and.b #$7F
    sta.w !wram_0DA6
.flagged:
    rts
.move:
    lda.b #$00
    sta.l !wram_1A1E,x
    lda.l !wram_1A1C,x
    clc
    adc.w !wram_1A20,x
    sta.l !wram_1A1C,x
    cmp.b #$1C
    beq .turn
    cmp.b #$16
    beq .turn
    jmp.w .done
.turn:
    lda.l !wram_1A20,x
    eor.b #$FF
    inc a
    sta.l !wram_1A20,x
.done:
    rts

; @routine 7E:DEFE BobWindow0
; WHAT IT DOES: Steps bob 0 (BobStep) and, unless it is at $18, sets window
; setup bytes $19AC-$19AE from its position ($20, (position | $40) - 2,
; position | $80) and the window bands (WindowHdmaFFD1, or WindowHdmaFFD5
; with $0D62 set).
; HOW: Long accesses to bank 0; $0D62 through the data bank. The $0D62
; case is not reached in the traces so far.
; Entry: any widths. Exit: M 8-bit, X 8-bit; as the last call left them.
%ramcode($7EDEFE, $013C25)
BobWindow0:
    sep.b #$30
    ldx.b #$00
    jsr.w BobStep
    sep.b #$30
    lda.l !wram_1A1C
    cmp.b #$18
    beq .done
    pha
    ora.b #$80
    sta.l !wram_19AE
    pla
    ora.b #$40
    sec
    sbc.b #$02
    sta.l !wram_19AD
    lda.b #$20
    sta.l !window_setup_bytes
    lda.w !wram_0D62
    bne .alt
    jsr.w WindowHdmaFFD1
    jmp.w .done
.alt:
    jsr.w WindowHdmaFFD5
.done:
    rts

; @routine 7E:DF35 BobWindow1
; WHAT IT DOES: As BobWindow0 for bob 1: setup bytes $19AF-$19B1 and the
; bands WindowHdmaFFD9.
; HOW: As BobWindow0. Not reached in the traces so far.
; Entry: any widths. Exit: as BobWindow0.
%ramcode($7EDF35, $013C5C)
BobWindow1:
    sep.b #$30
    ldx.b #$01
    jsr.w BobStep
    sep.b #$30
    lda.l !wram_1A1D
    cmp.b #$18
    beq .done
    pha
    ora.b #$80
    sta.l !wram_19B1
    pla
    ora.b #$40
    sec
    sbc.b #$02
    sta.l !wram_19B0
    lda.b #$20
    sta.l !wram_19AF
    jsr.w WindowHdmaFFD9
.done:
    rts
