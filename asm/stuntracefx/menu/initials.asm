; initials.asm: the menu screen where player 1 enters three letters, 0-$19
; each, at $18EF, $18F1 and $18F3; the position being changed is $18ED
; (0-2). Holding up or down repeats after 13 frames, counted in $190B (up)
; and $190D (down) from the held buttons at $0309.
; C: src/stuntracefx/menu/initials.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:C34F MenuInitialsPresses
; WHAT IT DOES: Works through player 1's presses queued this frame, last
; first, as MenuChoicePressesP1 does, applying each to the initials
; (MenuInitialsStep) and then emptying the list; then blinks the marker
; and shows the letters on the kind 9 object (MenuRowBlinkSettings).
; HOW: The list offset is pushed around each step and pulled back into A;
; an empty list skips straight to the display.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A and C as
; MenuRowBlinkSettings; X = $11F9; Y = $0A; N, Z from X.
%at($0BC34F)
MenuInitialsPresses:
    stz.w !menu_joy1_pressed
    lda.l !word_list_a_size
    dec a
    dec a
    bmi .lC378
.lC35A:
    tax
    lda.l !word_list_a,x
    sta.w !menu_118f
    ora.w !menu_joy1_pressed
    sta.w !menu_joy1_pressed
    phx
    jsr.w MenuInitialsStep
    pla
    dec a
    dec a
    bpl .lC35A
    lda.w #$0000
    sta.l !word_list_a_size
.lC378:
    lda.w #$0009
    jsr.w MenuActorFind
    jsr.w MenuRowBlinkSettings
    ldx.w !actor_current
    rts

; @routine 0B:C385 MenuInitialsStep
; WHAT IT DOES: Applies one press of player 1 ($118F) to the initials:
; left and right move the position $18ED (0-2), with sound $46 and the
; position marker's palette reset (MenuRowPalette5); up and down step the
; letter at the position through 0-$19, wrapping, with sound $45, on a
; press or when the button has been held 13 frames. Left is not reached in
; the traces so far, nor a letter wrapping up.
; HOW: The letter's offset (position * 2) is kept in X; up is handled
; before down, and a handled up skips down.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last value loaded or
; stored; X = the position * 2; Y unchanged, or from the search; N, Z, C
; from the last test or compare.
%at($0BC385)
MenuInitialsStep:
    lda.w !menu_118f
    and.w #$0200
    beq .lC3A9
    lda.l !wram_18ED
    beq .lC3A9
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    jsr.w MenuRowPalette5
    lda.l !wram_18ED
    dec a
    sta.l !wram_18ED
.lC3A9:
    lda.w !menu_118f
    and.w #$0100
    beq .lC3D0
    lda.l !wram_18ED
    cmp.w #$0002
    beq .lC3D0
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    jsr.w MenuRowPalette5
    lda.l !wram_18ED
    inc a
    sta.l !wram_18ED
.lC3D0:
    lda.l !wram_18ED
    asl a
    tax
    lda.l !joy1_held
    and.w #$0800
    beq .lC3EF
    lda.l !wram_190B
    inc a
    cmp.w #$000D
    bcs .lC3FE
    sta.l !wram_190B
    bra .lC3F6
.lC3EF:
    lda.w #$0000
    sta.l !wram_190B
.lC3F6:
    lda.w !menu_118f
    and.w #$0800
    beq .lC422
.lC3FE:
    lda.w #$0000
    sta.l !wram_190B
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !wram_18EF,x
    inc a
    cmp.w #$001A
    bne .lC41C
    lda.w #$0000
.lC41C:
    sta.l !wram_18EF,x
    bra .lC46C
.lC422:
    lda.l !joy1_held
    and.w #$0400
    beq .lC43B
    lda.l !wram_190D
    inc a
    cmp.w #$000D
    bcs .lC44A
    sta.l !wram_190D
    bra .lC442
.lC43B:
    lda.w #$0000
    sta.l !wram_190D
.lC442:
    lda.w !menu_118f
    and.w #$0400
    beq .lC46C
.lC44A:
    lda.w #$0000
    sta.l !wram_190D
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !wram_18EF,x
    dec a
    cmp.w #$FFFF
    bne .lC468
    lda.w #$0019
.lC468:
    sta.l !wram_18EF,x
.lC46C:
    rts
