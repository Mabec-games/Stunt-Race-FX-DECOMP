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

; @routine 7F:10CD JoypadPortCheck
; WHAT IT DOES: Checks joypad port A (0 or 1) for a device on its second
; data line: with the latch set, eight reads of that line all 1 and then,
; with the latch clear, eight reads not all 1 set $18FB to 1. The second
; half is not reached in the traces so far.
; HOW: The serial bits are shifted into !joy_serial_bits (ROL); the latch is
; JOYSER0 bit 0 and is left clear. P is saved and restored.
; Entry: any widths; A low byte = the port. Exit: A low byte = 0 (A = 0
; when $18FB was set); X = the port, Y = $FF (8-bit values); P unchanged.
%ramcode($7F10CD, $0588CD)
JoypadPortCheck:
    php
    sep.b #$30
    tax
    lda.b #$01
    sta.w !JOYSER0
    stz.w !joy_serial_bits
    ldy.b #$07
.l10DB:
    lda.w !JOYSER0,x
    and.b #$02
    beq .l10E8
    sec
    rol.w !joy_serial_bits
    bra .l10EC
.l10E8:
    clc
    rol.w !joy_serial_bits
.l10EC:
    dey
    bpl .l10DB
    lda.w !joy_serial_bits
    cmp.b #$FF
    bne .l1124
    lda.b #$00
    sta.w !JOYSER0
    stz.w !joy_serial_bits
    ldy.b #$07
.l1100:
    lda.w !JOYSER0,x
    and.b #$02
    beq .l110D
    sec
    rol.w !joy_serial_bits
    bra .l1111
.l110D:
    clc
    rol.w !joy_serial_bits
.l1111:
    dey
    bpl .l1100
    lda.w !joy_serial_bits
    cmp.b #$FF
    beq .l1124
    rep.b #$20
    lda.w #$0001
    sta.l !wram_18FB
.l1124:
    sep.b #$20
    lda.b #$00
    sta.w !JOYSER0
    rep.b #$20
    plp
    rts

; @routine 7E:A7A3 FrameSoundAndJoypads
; WHAT IT DOES: The frame's sound output (SoundPortsSend) and joypad read
; (JoypadRead), then adds this frame's presses ($0311, $0313) to the ones
; the menus collect at $118B and $118D.
; HOW: 16-bit ORs through the data bank.
; Entry: as JoypadRead. Exit: A = $118D; M 16-bit; N, Z from it.
%ramcode($7EA7A3, $0104CA)
FrameSoundAndJoypads:
    jsr.w SoundPortsSend
    jsr.w JoypadRead
    rep.b #$20
    lda.w !menu_joy1_pressed
    ora.w !joy1_pressed
    sta.w !menu_joy1_pressed
    lda.w !menu_joy2_pressed
    ora.w !joy2_pressed
    sta.w !menu_joy2_pressed
    rts
