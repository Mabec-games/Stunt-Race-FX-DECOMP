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

; @routine 0B:C225 MenuInitialsFrame
; WHAT IT DOES: The initials entry screen's frame: dims the fixed colour
; (MenuFixedColourDown), shows a race frame (RaceFrameStep, A = 0), sets
; $18FD, applies player 1's presses (MenuInitialsPresses), then takes the
; buttons pressed ($118B, cleared): start or B ($9000) ends the menu
; ($11EB = 1, sound $46, the current record freed); Y ($4000) goes back:
; kind 5's +$10 = 0, kinds 6 and 7 to animations $AE and $B8, kinds 8 and
; 9 +$10 = $FFFF, this record's handler $BA4E with its slide restarted,
; kind 4 to animation $8A, $1191 cleared and sound $47; select ($2000)
; with selection $18C3 at 0 confirms: sound $46, kinds 4, 8 and 9 +$10 =
; $FFFF, kinds 10-14 +$10 = 0, handler $BFB8, $1191 cleared.
; HOW: Y keeps the buttons; record X is $11F9 as MenuInitialsPresses
; leaves it. Kind 4's animation change is not reached in the traces so
; far.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: M 16-bit; A, X, Y and flags
; as the path's last step left them.
%at($0BC225)
MenuInitialsFrame:
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    jsr.w MenuInitialsPresses
    ldy.w !menu_joy1_pressed
    stz.w !menu_joy1_pressed
    tya
    and.w #$9000
    bne .exit
    tya
    and.w #$4000
    beq .select
    brl .back
.select:
    tya
    and.w #$2000
    bne .confirm
    rts
.confirm:
    lda.l !wram_18C3
    cmp.w #$0000
    bne .done
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$0004
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0008
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0009
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000A
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000B
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000C
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000D
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000E
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$BFB8
    sta.b !actor_handler,x
    lda.w #$0000
    sta.l !word_list_a_size
.done:
    rts
.exit:
    lda.w #$0001
    sta.w !menu_exit
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
    rts
.back:
    ldx.w !actor_current
    lda.w #$0005
    ldy.w #$0000
    jsr.w MenuActorsSet10
    phx
    lda.w #$0006
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00AE
    beq .anim6
    lda.w #$00AE
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.anim6:
    lda.w #$0007
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00B8
    beq .anim7
    lda.w #$00B8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.anim7:
    plx
    lda.w #$0008
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0009
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$BA4E
    sta.b !actor_handler,x
    stz.b !actor_0a,x
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$008A
    beq .anim4
    lda.w #$008A
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.anim4:
    lda.w #$0000
    sta.l !word_list_a_size
    sep.b #$20
    lda.b #$47
    jsl.l SoundPort0QueuePush
    rep.b #$20
    rts

; @routine 0B:BA4E MenuSettingsFrame
; WHAT IT DOES: The settings screen's frame for actor record X. While its
; count +$0A runs it is counted down (sound $3F, raw, at $13, $0E, $09 and
; $04; at 0 kind 4 to animation $8A). Then the fixed colour is dimmed
; twice (MenuFixedColourDown), a race frame shown (RaceFrameStep, A = 0),
; $18FD = 1, player 1's presses applied (MenuChoicePressesP1) and the
; buttons pressed ($118B, cleared) taken: Y ($4000) goes back to the
; initials (handler $C872, count $80, sound $47, an actor from $027E,
; kinds 4 and 5 +$10 = $FFFF, kinds 6 and 7 to animations $C2 and $C8);
; B or Start ($9000), unless $18F9 is set, takes the choice $18C3: 2 ends
; (kind 4 to animation $90, kinds 4 and 5 +$10 = $FFFF, sound $46, handler
; $BC9A, $1905 = 1, $1907 = $28, colour 0 = $464A, $1193 = 0, $E0 queued
; for HDMAEN), others go on (sound $46, $190B = $190D = 0, kind 5 +$10 =
; $FFFF, kinds 6 and 7 as above, kinds 8 and 9 +$10 = 0, handler $C225,
; $1191 = 0); Select ($2000) with choice 0 sound $46, kinds 4 and 5 +$10 =
; $FFFF, kinds 6 and 7 as above, kinds 10-14 +$10 = 0, handler $BEEF.
; HOW: Y keeps the buttons; X is $11F9 again after the calls. The count
; is not reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the record. Exit: M 16-bit; A,
; X, Y and flags as the path's last step left them.
%at($0BBA4E)
MenuSettingsFrame:
    lda.b !actor_0a,x
    beq .lBA8D
    dec a
    sta.b !actor_0a,x
    cmp.w #$0013
    beq .lBA69
    cmp.w #$000E
    beq .lBA69
    cmp.w #$0009
    beq .lBA69
    cmp.w #$0004
    bne .lBA73
.lBA69:
    sep.b #$20
    lda.b #$3F
    jsl.l SoundPort0QueuePushRaw
    rep.b #$20
.lBA73:
    lda.b !actor_0a,x
    bne .lBA8D
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$008A
    beq .lBA8D
    lda.w #$008A
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBA8D:
    ldx.w !actor_current
    jsr.w MenuFixedColourDown
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    ldx.w !actor_current
    jsr.w MenuChoicePressesP1
    ldx.w !actor_current
    ldy.w !menu_joy1_pressed
    stz.w !menu_joy1_pressed
    tya
    and.w #$4000
    beq .lBABC
    brl .lBC39
.lBABC:
    lda.l !wram_18F9
    bne .lBACB
    tya
    and.w #$9000
    beq .lBACB
    brl .lBB5B
.lBACB:
    tya
    and.w #$2000
    bne .lBAD2
    rts
.lBAD2:
    lda.l !wram_18C3
    cmp.w #$0000
    beq .lBADE
    brl .lBB5A
.lBADE:
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$0004
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0005
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    phx
    lda.w #$0006
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00C2
    beq .lBB11
    lda.w #$00C2
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBB11:
    lda.w #$0007
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00C8
    beq .lBB27
    lda.w #$00C8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBB27:
    plx
    lda.w #$000A
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000B
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000C
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000D
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000E
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$BEEF
    sta.b !dp_08,x
.lBB5A:
    rts
.lBB5B:
    lda.l !wram_18C3
    cmp.w #$0002
    beq .lBBCF
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$0000
    sta.l !wram_190B
    sta.l !wram_190D
    lda.w #$0005
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    phx
    lda.w #$0006
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00C2
    beq .lBB99
    lda.w #$00C2
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBB99:
    lda.w #$0007
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00C8
    beq .lBBAF
    lda.w #$00C8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBBAF:
    plx
    lda.w #$0008
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0009
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$C225
    sta.b !dp_08,x
    lda.w #$0000
    sta.l !word_list_a_size
    rts
.lBBCF:
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$0090
    beq .lBBE5
    lda.w #$0090
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBBE5:
    lda.w #$0004
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0005
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    ldx.w !actor_current
    lda.w #$BC9A
    sta.b !dp_08,x
    lda.w #$0001
    sta.l !menu_1905
    lda.w #$0028
    sta.l !menu_1907
    lda.w #$464A
    sta.l !palette_buffer
    lda.w #$0000
    sta.l !word_list_b_size
    sep.b #$20
    lda.b #$E0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$420C
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    rts
.lBC39:
    ldx.w !actor_current
    lda.w #$C872
    sta.b !dp_08,x
    lda.w #$0080
    sta.b !actor_0a,x
    sep.b #$20
    lda.b #$47
    jsl.l SoundPort0QueuePush
    rep.b #$20
    ldx.w #$027E
    jsr.w MenuActorSpawn
    lda.w #$0004
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0005
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    phx
    lda.w #$0006
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00C2
    beq .lBC7F
    lda.w #$00C2
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBC7F:
    lda.w #$0007
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00C8
    beq .lBC95
    lda.w #$00C8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBC95:
    plx
    ldx.w !actor_current
    rts
