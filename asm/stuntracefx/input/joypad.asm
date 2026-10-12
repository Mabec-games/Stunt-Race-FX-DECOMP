; joypad.asm: reading the two joypads from the auto-read registers. Runs from
; work RAM. C: src/stuntracefx/input/joypad.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch 65816

; @routine 7E:B1AE JoypadRead
; WHAT IT DOES: Reads both joypads: the buttons held now, and the buttons
; pressed since the last read.
; HOW: Per pad, newly pressed = (now XOR held last time) AND now, stored to
; the pressed word and its copy; then now becomes the held state.
; Entry: any M, X 8-bit. Exit: A = joypad 1 now; P unchanged.
%ramcode($7EB1AE, $010ED5)
JoypadRead:
    php
    sep.b #$10
    rep.b #$20
    lda.l !JOY1L
    pha
    eor.w !joy1_held
    and.l !JOY1L
    sta.w !joy1_pressed
    sta.w !joy1_pressed_copy
    lda.l !JOY2L
    pha
    eor.w !joy2_held
    and.l !JOY2L
    sta.w !joy2_pressed
    sta.w !joy2_pressed_copy
    pla
    sta.w !joy2_held
    pla
    sta.w !joy1_held
    plp
    rts

; @routine 7E:B1E1 JoypadAccumulate
; WHAT IT DOES: Folds this frame's joypad state into the frame code's
; accumulated presses and held buttons.
; HOW: Per pad, presses since its own last sample are ORed into the pressed
; copy. Then both pads' presses are taken against joypad 2's held state and
; written to joypad 2's pressed word (the second overwrites the first), and
; both held states have now ORed in.
; Entry: any widths. Exit: A = joypad 1 now OR its held state; P unchanged.
JoypadAccumulate:
    php
    rep.b #$20
    lda.w !JOY1L
    pha
    eor.w !joy1_last
    and.w !JOY1L
    ora.w !joy1_pressed_copy
    sta.w !joy1_pressed_copy
    pla
    sta.w !joy1_last
    lda.w !JOY2L
    pha
    eor.w !joy2_last
    and.w !JOY2L
    ora.w !joy2_pressed_copy
    sta.w !joy2_pressed_copy
    pla
    sta.w !joy2_last
    lda.w !JOY1L
    pha
    eor.w !joy2_held
    and.w !JOY1L
    sta.w !joy2_pressed
    lda.w !JOY2L
    pha
    eor.w !joy2_held
    and.w !JOY2L
    sta.w !joy2_pressed
    pla
    ora.w !joy2_held
    sta.w !joy2_held
    pla
    ora.w !joy1_held
    sta.w !joy1_held
    plp
    rts
