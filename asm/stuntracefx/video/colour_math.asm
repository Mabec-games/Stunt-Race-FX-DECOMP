; colour_math.asm: the colour maths settings the raster interrupt copies to the
; PPU: $0D47 for CGADSUB, and the fixed colour's intensity at $7F:340D.
; C: src/stuntracefx/video/colour_math.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:FE57 ColourMathA3
; WHAT IT DOES: Sets the colour maths mode to $A3 (subtract, half, BG1 and
; BG2).
; HOW: Stores $A3 to $0D47.
; Entry: any widths. Exit: A low byte = $A3; M 8-bit; N set, Z clear.
%at($03FE57)
ColourMathA3:
    sep.b #$20
    lda.b #$A3
    sta.w !colour_math_mode
    rts

; @routine 03:FCC9 ColourMathA7Start
; WHAT IT DOES: Sends sound $04 on port 1, sets the colour maths mode to $A7
; and the fixed colour step at $7F:3408 to 0.
; HOW: SoundPort1QueuePush, then two stores.
; Entry: any widths. Exit: A = 0; M 16-bit, X 8-bit (X, Y high bytes clear);
; Z set, N clear.
%at($03FCC9)
ColourMathA7Start:
    sep.b #$30
    lda.b #$04
    jsl.l SoundPort1QueuePush
    lda.b #$A7
    sta.w !colour_math_mode
    rep.b #$20
    lda.w #$0000
    sta.l !fixed_colour_step
    rtl

; @routine 03:FE6B FixedColourFromStep
; WHAT IT DOES: Sets the fixed colour's intensity from the step at $7F:3408,
; through the table after this routine.
; HOW: $7F:340D = table[$7F:3408].
; Entry: any widths. Exit: A low byte = the intensity; X = the step; M and X
; 8-bit; N, Z from A.
%at($03FE6B)
FixedColourFromStep:
    sep.b #$30
    lda.l !fixed_colour_step
    tax
    lda.l !fixed_colour_table,x
    sta.l !fixed_colour
    rtl

; @routine 7E:E011 ColourMathSub83Windowed
; WHAT IT DOES: Sets colour maths to $83 (subtract, on BG1 and BG2) and the
; windows to colour only.
; HOW: CGADSUB = $83 (a long store), then WindowsColourWindowOnly.
; Entry: any widths. Exit: A low byte = $03; N, Z clear; M 8-bit.
%ramcode($7EE011, $013D38)
ColourMathSub83Windowed:
    sep.b #$20
    lda.b #$83
    sta.l !CGADSUB
    jsr.w WindowsColourWindowOnly
    rts

; @routine 03:FE53 ColourMathA3Long
; WHAT IT DOES: ColourMathA3 for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ColourMathA3. Exit: as ColourMathA3.
%at($03FE53)
ColourMathA3Long:
    jsr.w ColourMathA3
    rtl

; @routine 03:FE63 ColourMathOff
; WHAT IT DOES: Sets the colour maths mode at $0D47 to 0 (none).
; HOW: One byte store through the data bank.
; Entry: any widths. Exit: A low byte = 0; M 8-bit; Z set, N clear.
%at($03FE63)
ColourMathOff:
    sep.b #$20
    lda.b #$00
    sta.w !colour_math_mode
    rts

; @routine 03:FE5F ColourMathOffLong
; WHAT IT DOES: ColourMathOff for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ColourMathOff. Exit: as ColourMathOff.
%at($03FE5F)
ColourMathOffLong:
    jsr.w ColourMathOff
    rtl
