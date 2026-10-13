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

; @routine 7E:D311 WindowSweepIfOn
; WHAT IT DOES: Unless the window HDMA state $19D8 is set or no layout is
; chosen ($19BC = $FF), while bit 4 of $0DA5 is set selects window setup
; 4 ($19A9) and steps the sweep (WindowSweep).
; HOW: The data bank is set to the code's ($7E) and restored. The sweep
; is not reached in the traces so far.
; Entry: any widths. Exit: P and the data bank restored; A as the last
; test or call left it.
%ramcode($7ED311, $013038)
WindowSweepIfOn:
    php
    phb
    phk
    plb
    sep.b #$20
    lda.w !window_hdma_off
    bne .done
    lda.l !layout_moving
    cmp.b #$FF
    beq .done
    sep.b #$20
    lda.w !slide_flags
    and.b #$10
    beq .done
    sep.b #$20
    lda.b #$04
    sta.w !windows_setup_index
    jsr.w WindowSweep
.done:
    plb
    plp
    rtl

; @routine 7E:DE5E WindowSweep
; WHAT IT DOES: Steps sweep 0 (WindowSweepStep) and, unless its position
; $1A15 is $EE, puts the position in window setup bytes $19AC, $19AE and
; $19AD and sets the window bands (WindowHdmaFFD1, or WindowHdmaFFD5 with
; $0D62 set).
; HOW: Long stores to bank 0. Not reached in the traces so far.
; Entry: any widths. Exit: M 8-bit, X 8-bit; as the last call left them.
%ramcode($7EDE5E, $013B85)
WindowSweep:
    sep.b #$30
    ldx.b #$00
    jsr.w WindowSweepStep
    lda.l !wram_1A15
    cmp.b #$EE
    beq .done
    sta.l !window_setup_bytes
    sta.l !window_setup_bytes+2
    sta.l !window_setup_bytes+1
    lda.w !wram_0D62
    bne .alt
    jsr.w WindowHdmaFFD1
    jmp.w .done
.alt:
    jsr.w WindowHdmaFFD5
.done:
    rts

; @routine 7E:DE88 WindowSweepStep
; WHAT IT DOES: Steps sweep X at half rate ($1A17 + X toggling): its
; position $1A15 + X moves by its velocity $1A19 + X, which turns round at
; $F2 and $E8; the sixteenth step ($1A1B) ends the sweep: bit 4 of $0DA5
; cleared, $1A17/$1A18/$1A1B = 0, velocities $FE, positions $F0.
; HOW: 8-bit; long accesses to bank 0 except the velocity add and the
; end's stores (data bank). Not reached in the traces so far.
; Entry: X = sweep. Exit: M 8-bit, X 8-bit; A and flags from the last
; step.
%ramcode($7EDE88, $013BAF)
WindowSweepStep:
    sep.b #$30
    lda.l !wram_1A17,x
    clc
    adc.b #$01
    cmp.b #$02
    beq .count
    sta.l !wram_1A17,x
    jmp.w .done
.count:
    lda.l !wram_1A1B
    clc
    adc.b #$01
    sta.l !wram_1A1B
    cmp.b #$10
    bne .move
    lda.b #$00
    sta.l !wram_1A1B
    lda.w !slide_flags
    and.b #$EF
    sta.w !slide_flags
    lda.b #$00
    sta.w !wram_1A17
    sta.w !wram_1A18
    sta.w !wram_1A1B
    lda.b #$FE
    sta.w !wram_1A19
    sta.w !wram_1A1A
    lda.b #$F0
    sta.w !wram_1A15
    sta.w !wram_1A16
    rts
.move:
    lda.b #$00
    sta.l !wram_1A17,x
    lda.l !wram_1A15,x
    clc
    adc.w !wram_1A19,x
    sta.l !wram_1A15,x
    cmp.b #$F2
    beq .turn
    cmp.b #$E8
    beq .turn
    jmp.w .done
.turn:
    lda.l !wram_1A19,x
    eor.b #$FF
    inc a
    sta.l !wram_1A19,x
.done:
    rts

; @routine 7E:DD43 OscillatorStep
; WHAT IT DOES: Steps oscillator X by the level in bits 5-7 of $1A11 (4
; while bit 7 is set, with $1A0E + X = $80): a change of level restarts
; its count $1A0A + X; level 0 resets it ($1A08 + X = $40, rising by 1);
; otherwise its count runs to the rate for the level (the table at
; $7E:D30D, kept in $1A10) and then $1A08 + X moves by $1A0C + X, turning
; round at $40 and $42.
; HOW: 8-bit; long accesses to bank 0 except $1A10's store and the step
; add (data bank). Bit 7 and level 0 are not reached in the traces so far.
; Entry: X = oscillator. Exit: M 8-bit, X 8-bit; A and flags from the
; last step.
%ramcode($7EDD43, $013A6A)
OscillatorStep:
    sep.b #$30
    lda.l !wram_1A11
    and.b #$80
    beq .level
    lda.b #$80
    sta.l !wram_1A0E,x
    lda.b #$04
    jmp.w .rate
.level:
    lda.l !wram_1A11
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    cmp.l !wram_1A0E,x
    beq .same
    lda.b #$00
    sta.l !wram_1A0A,x
.same:
    lda.l !wram_1A11
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta.l !wram_1A0E,x
    bne .rate
    lda.b #$00
    sta.l !wram_1A0A,x
    lda.b #$40
    sta.l !wram_1A08,x
    lda.b #$01
    sta.l !wram_1A0C,x
    jmp.w .done
.rate:
    dec a
    phx
    tax
    lda.l !oscillator_rates,x
    sta.w !wram_1A10
    plx
    lda.l !wram_1A0A,x
    clc
    adc.b #$01
    cmp.l !wram_1A10
    beq .move
    sta.l !wram_1A0A,x
    jmp.w .done
.move:
    lda.b #$00
    sta.l !wram_1A0A,x
    lda.l !wram_1A08,x
    clc
    adc.w !wram_1A0C,x
    sta.l !wram_1A08,x
    cmp.b #$40
    beq .turn
    cmp.b #$42
    beq .turn
    jmp.w .done
.turn:
    lda.l !wram_1A0C,x
    eor.b #$FF
    inc a
    sta.l !wram_1A0C,x
.done:
    rts

; @routine 7E:D33A BobWindowsRun
; WHAT IT DOES: Unless the window HDMA state $19D8 is set or no layout is
; chosen ($19BC = $FF): checks player 1's link event
; (ObjectLinkEventTrigger) and, while bob 0 runs (bit 7 of $0DA4), sets
; window setup 2 and steps it (BobWindow0); with $0D62 set the same for
; player 2 (ObjectLinkEventTrigger2, $0DA6, BobWindow1).
; HOW: The data bank is the code's ($7E) and restored. $0D62 is read at
; the width BobWindow0 left (a word after WindowHdmaFFD1). Bob 1's window
; is not reached in the traces so far.
; Entry: any widths. Exit: P and the data bank restored.
%ramcode($7ED33A, $013061)
BobWindowsRun:
    php
    phb
    phk
    plb
    sep.b #$20
    lda.w !window_hdma_off
    bne .done
    lda.l !layout_moving
    cmp.b #$FF
    beq .done
    jsr.w ObjectLinkEventTrigger
    sep.b #$30
    lda.w !wram_0DA4
    and.b #$80
    beq .player2
    sep.b #$20
    lda.l !wram_0D45
    and.b #$DF
    sta.l !wram_0D45
    lda.b #$02
    sta.w !windows_setup_index
    jsr.w BobWindow0
.player2:
    lda.w !wram_0D62
    beq .done
    sep.b #$30
    jsr.w ObjectLinkEventTrigger2
    sep.b #$20
    lda.w !wram_0DA6
    and.b #$80
    beq .done
    sep.b #$20
    lda.l !wram_0D45
    and.b #$DF
    sta.l !wram_0D45
    lda.b #$02
    sta.w !windows_setup_index
    jsr.w BobWindow1
.done:
    plb
    plp
    rtl

; @routine 7E:DD12 OscillatorWindow
; WHAT IT DOES: Sets the oscillator level $1A11 from $0DDA, steps
; oscillator 0 (OscillatorStep) and puts its value in window setup byte
; $19AD with $19AC = $20 and $19AE = $95, then sets the window bands
; (WindowHdmaFFD1, or WindowHdmaFFD5 with $0D62 set).
; HOW: Long stores to bank 0. The $0D62 case is not reached in the traces
; so far.
; Entry: any widths. Exit: M 8-bit, X 8-bit; as the last call left them.
%ramcode($7EDD12, $013A39)
OscillatorWindow:
    sep.b #$30
    lda.l !speed_sample_long
    sta.l !wram_1A11
    ldx.b #$00
    jsr.w OscillatorStep
    lda.w !wram_1A08
    sta.l !wram_19AD
    lda.b #$20
    sta.l !window_setup_bytes
    lda.b #$95
    sta.l !wram_19AE
    lda.w !wram_0D62
    bne .alt
    jsr.w WindowHdmaFFD1
    jmp.w .done
.alt:
    jsr.w WindowHdmaFFD5
.done:
    rts
