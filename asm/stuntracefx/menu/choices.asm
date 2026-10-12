; choices.asm: moving the menu's choices with the joypads. The frame code
; appends each frame's newly pressed buttons to two word lists (player 1
; at $1195, size $1191; player 2 at $11B5, size $1193); the menu works
; through them here, left and right stepping a choice ($18CB for player 1,
; $18C9 for player 2) through the tables at $0B:C81F (left) and $0B:C827
; (right). Choice 3 is offered only when the saved progress byte at
; $70:FC0B has bit 2 set (for player 1 also when the mode $18C3 is 3).
; C: src/stuntracefx/menu/choices.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:C64C MenuChoicePressesP1
; WHAT IT DOES: Works through player 1's presses queued this frame, last
; first: each is stored at $118F, ORed into $118B, and applied to the
; choice (MenuChoiceStepP1); then empties the list. $118B is cleared
; first.
; HOW: The list offset is pushed around each step and pulled back into A.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: list empty: A = -2; N set,
; Z clear. Otherwise A = 0; Z set, N clear; X, Y, C from the last step.
%at($0BC64C)
MenuChoicePressesP1:
    stz.w !menu_joy1_pressed
    lda.l !word_list_a_size
    dec a
    dec a
    bmi .lC675
.lC657:
    tax
    lda.l !word_list_a,x
    sta.w !menu_118f
    ora.w !menu_joy1_pressed
    sta.w !menu_joy1_pressed
    phx
    jsr.w MenuChoiceStepP1
    pla
    dec a
    dec a
    bpl .lC657
    lda.w #$0000
    sta.l !word_list_a_size
.lC675:
    rts

; @routine 0B:C676 MenuChoiceStepP1
; WHAT IT DOES: Applies one press of player 1 ($118F) to the choice at
; $18CB: left steps it through $0B:C81F, right through $0B:C827, each with
; sound $45; a choice of 3 that is not allowed becomes 2 (left) or 1
; (right). The new choice is shown (MenuSet1A66, MenuSetPairAnimations).
; Left is handled before right. The mode test is not reached in the
; traces so far.
; HOW: 16-bit tests of the press; the choice and its tables are read with
; long addressing; X is pushed around the table read.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: right not pressed: A = 0; Z
; set, N clear; X, Y, C as left left them (as MenuSetPairAnimations, or
; unchanged). Right pressed: as MenuSetPairAnimations.
%at($0BC676)
MenuChoiceStepP1:
    lda.w !menu_118f
    and.w #$0200
    beq .lC6CC
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !selection
    asl a
    phx
    tax
    lda.l !choice_left,x
    plx
    sta.l !selection
    lda.l !save_progress
    and.w #$00FF
    cmp.w #$0000
    beq .lC6B2
    and.w #$0004
    bne .lC6C2
    lda.l !wram_18C3
    cmp.w #$0003
    beq .lC6C2
.lC6B2:
    lda.l !selection
    cmp.w #$0003
    bne .lC6C2
    lda.w #$0002
    sta.l !selection
.lC6C2:
    lda.l !selection
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
.lC6CC:
    lda.w !menu_118f
    and.w #$0100
    beq .lC722
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !selection
    asl a
    phx
    tax
    lda.l !choice_right,x
    plx
    sta.l !selection
    lda.l !save_progress
    and.w #$00FF
    cmp.w #$0000
    beq .lC708
    and.w #$0004
    bne .lC718
    lda.l !wram_18C3
    cmp.w #$0003
    beq .lC718
.lC708:
    lda.l !selection
    cmp.w #$0003
    bne .lC718
    lda.w #$0001
    sta.l !selection
.lC718:
    lda.l !selection
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
.lC722:
    rts

; @routine 0B:C760 MenuChoicePressesP2
; WHAT IT DOES: As MenuChoicePressesP1 for player 2: the presses at $11B5
; (size $1193), ORed into !menu_joy2_pressed, applied by MenuChoiceStepP2.
; HOW: As MenuChoicePressesP1.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoicePressesP1.
%at($0BC760)
MenuChoicePressesP2:
    stz.w !menu_joy2_pressed
    lda.l !word_list_b_size
    dec a
    dec a
    bmi .lC789
.lC76B:
    tax
    lda.l !word_list_b,x
    sta.w !menu_118f
    ora.w !menu_joy2_pressed
    sta.w !menu_joy2_pressed
    phx
    jsr.w MenuChoiceStepP2
    pla
    dec a
    dec a
    bpl .lC76B
    lda.w #$0000
    sta.l !word_list_b_size
.lC789:
    rts

; @routine 0B:C78A MenuChoiceStepP2
; WHAT IT DOES: As MenuChoiceStepP1 for player 2's choice at $18C9; choice
; 3 is allowed only by bit 2 of $70:FC0B, and the repeat value $18D3 is
; refreshed (MenuChoiceRepeat) before the choice is shown. Not reached in
; the traces so far, except with neither left nor right pressed.
; HOW: As MenuChoiceStepP1; the choice is pushed around MenuChoiceRepeat.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoiceStepP1.
%at($0BC78A)
MenuChoiceStepP2:
    lda.w !menu_118f
    and.w #$0200
    beq .lC7D4
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !selection_prev
    asl a
    phx
    tax
    lda.l !choice_left,x
    plx
    sta.l !selection_prev
    lda.l !save_progress
    and.w #$0004
    bne .lC7C5
    lda.l !selection_prev
    cmp.w #$0003
    bne .lC7C5
    lda.w #$0002
    sta.l !selection_prev
.lC7C5:
    lda.l !selection_prev
    pha
    jsr.w MenuChoiceRepeat
    pla
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
.lC7D4:
    lda.w !menu_118f
    and.w #$0100
    beq .lC81E
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !selection_prev
    asl a
    phx
    tax
    lda.l !choice_right,x
    plx
    sta.l !selection_prev
    lda.l !save_progress
    and.w #$0004
    bne .lC80F
    lda.l !selection_prev
    cmp.w #$0003
    bne .lC80F
    lda.w #$0001
    sta.l !selection_prev
.lC80F:
    lda.l !selection_prev
    pha
    jsr.w MenuChoiceRepeat
    pla
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
.lC81E:
    rts

; @routine 0B:B9C2 MenuChoicesFromSave
; WHAT IT DOES: Restores the menu's choices from the saved settings in
; Super FX RAM for the menu A: menu 0 sets $18CB from bits 0-1 of
; $70:FC0C, menu 1 from bits 4-5 of it, menu 2 sets $18CB and $18C9 from
; bits 0-1 and 4-5 of $70:FC0E; any other menu sets $18CB from bits 0-1 of
; $70:FC0D, or to 1 when the progress byte $70:FC0B is 0. Each value is
; mapped through the word table at $0B:B9BA. Menus other than 0-2 are not
; reached in the traces so far.
; HOW: 16-bit long reads; P is saved and restored.
; Entry: A = the menu (16 bits are compared). Exit: A = the last value
; stored; X = the last saved value * 2, unchanged for the default of 1; P
; unchanged.
%at($0BB9C2)
MenuChoicesFromSave:
    php
    rep.b #$30
    cmp.w #$0000
    beq .lB9FC
    cmp.w #$0001
    beq .lBA0F
    cmp.w #$0002
    beq .lBA26
    lda.l !save_progress
    and.w #$00FF
    cmp.w #$0000
    beq .lB9F3
    lda.l !save_settings_b
    and.w #$0003
    asl a
    tax
    lda.l !choice_from_save,x
    sta.l !selection
    bra .lBA4C
.lB9F3:
    lda.w #$0001
    sta.l !selection
    bra .lBA4C
.lB9FC:
    lda.l !save_settings_a
    and.w #$0003
    asl a
    tax
    lda.l !choice_from_save,x
    sta.l !selection
    bra .lBA4C
.lBA0F:
    lda.l !save_settings_a
    and.w #$0030
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    tax
    lda.l !choice_from_save,x
    sta.l !selection
    bra .lBA4C
.lBA26:
    lda.l !save_settings_c
    and.w #$0003
    asl a
    tax
    lda.l !choice_from_save,x
    sta.l !selection
    lda.l !save_settings_c
    and.w #$0030
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    tax
    lda.l !choice_from_save,x
    sta.l !selection_prev
.lBA4C:
    plp
    rts

; @routine 0B:BEEF MenuSettingsHandler
; WHAT IT DOES: The handler of the settings screen: clears the fast
; animation flag $18FD, raises the fixed colour (MenuFixedColourUp) and
; applies the presses (MenuSettingsInput). When B, Y, Select or Start
; ($F000) was pressed it leaves: kinds 4 and 5 get +$10 = 0 (sound $46
; between them), kinds 6 and 7 animations $AE and $B8, kinds $0A-$0E +$10
; = $FFFF; player 1's press list is emptied; the object moves on to
; handler $BA4E with +$0A = 0 and +$1E = 1; kind 4 gets animation $8A, and
; the choice $18CB is shown again with $18D3 = 0.
; HOW: MenuActorsSet10 and MenuActorFind; X is pushed around the two
; searches. The kind 4 animation change is not reached in the traces so
; far.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: staying: A = 0;
; Y = $118B; Z set, N clear; X, C, V as MenuSettingsInput. Leaving: as
; MenuSetPairAnimations.
%at($0BBEEF)
MenuSettingsHandler:
    lda.w #$0000
    sta.l !wram_18FD
    jsr.w MenuFixedColourUp
    jsr.w MenuSettingsInput
    ldy.w !menu_joy1_pressed
    tya
    and.w #$F000
    bne .lBF06
    rts
.lBF06:
    lda.w #$0004
    ldy.w #$0000
    jsr.w MenuActorsSet10
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$0005
    ldy.w #$0000
    jsr.w MenuActorsSet10
    phx
    lda.w #$0006
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00AE
    beq .lBF39
    lda.w #$00AE
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBF39:
    lda.w #$0007
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00B8
    beq .lBF4F
    lda.w #$00B8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBF4F:
    plx
    lda.w #$000A
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000B
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000C
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000D
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000E
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0000
    sta.l !word_list_a_size
    lda.w #$BA4E
    sta.b !actor_handler,x
    stz.b !actor_0a,x
    lda.w #$0001
    sta.b !actor_1e,x
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$008A
    beq .lBFA6
    lda.w #$008A
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lBFA6:
    lda.w #$0000
    sta.l !selection_out
    lda.l !selection
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
    rts

; @routine 0B:BFB8 MenuSettingsHandler2
; WHAT IT DOES: As MenuSettingsHandler for the second settings screen,
; without clearing $18FD: when B, Y, Select or Start was pressed, kind 4
; gets +$10 = 0 (then sound $46), $190B and $190D are cleared, kinds 8
; and 9 get +$10 = 0 and kinds $0A-$0E $FFFF; the object moves on to
; handler $C225 with +$1E = 1, player 1's press list is emptied and the
; choice $18CB is shown again with $18D3 = 0.
; HOW: MenuActorsSet10 for each kind.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
; MenuSettingsHandler.
%at($0BBFB8)
MenuSettingsHandler2:
    jsr.w MenuFixedColourUp
    jsr.w MenuSettingsInput
    ldy.w !menu_joy1_pressed
    tya
    and.w #$F000
    bne .lBFC8
    rts
.lBFC8:
    lda.w #$0004
    ldy.w #$0000
    jsr.w MenuActorsSet10
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$0000
    sta.l !wram_190B
    sta.l !wram_190D
    lda.w #$0008
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0009
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$000A
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000B
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000C
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000D
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$000E
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$C225
    sta.b !actor_handler,x
    lda.w #$0001
    sta.b !actor_1e,x
    lda.w #$0000
    sta.l !word_list_a_size
    lda.w #$0000
    sta.l !selection_out
    lda.l !selection
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
    rts

; @routine 0B:C048 MenuSettingsInput
; WHAT IT DOES: Applies player 1's presses ($118B) on the settings screen:
; left and right move the row $18EB (0-2) with sound $46 and move the row
; marker (MenuSettingsRowMarker); up and down step the row's setting
; $18CD[row] through 0-2, wrapping, with sound $45. Then shows the
; settings (MenuSettingsShow, MenuChoiceAnimations). Left and up are not
; reached in the traces so far.
; HOW: Four 16-bit tests of the presses, in the order left ($0200), right
; ($0100), up ($0800), down ($0400).
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as MenuChoiceAnimations.
%at($0BC048)
MenuSettingsInput:
    lda.w !menu_joy1_pressed
    and.w #$0200
    beq .lC068
    lda.l !wram_18EB
    beq .lC068
    dec a
    sta.l !wram_18EB
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    jsr.w MenuSettingsRowMarker
.lC068:
    lda.w !menu_joy1_pressed
    and.w #$0100
    beq .lC08B
    lda.l !wram_18EB
    cmp.w #$0002
    beq .lC08B
    inc a
    sta.l !wram_18EB
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    jsr.w MenuSettingsRowMarker
.lC08B:
    lda.w !menu_joy1_pressed
    and.w #$0800
    beq .lC0B4
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !wram_18EB
    asl a
    tax
    lda.l !selection_1,x
    dec a
    cmp.w #$FFFF
    bne .lC0B0
    lda.w #$0002
.lC0B0:
    sta.l !selection_1,x
.lC0B4:
    lda.w !menu_joy1_pressed
    and.w #$0400
    beq .lC0DD
    sep.b #$20
    lda.b #$45
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !wram_18EB
    asl a
    tax
    lda.l !selection_1,x
    inc a
    cmp.w #$0003
    bne .lC0D9
    lda.w #$0000
.lC0D9:
    sta.l !selection_1,x
.lC0DD:
    ldx.w !actor_current
    jsr.w MenuSettingsShow
    jsr.w MenuChoiceAnimations
    rts

; @routine 0B:C0E7 MenuSettingsShow
; WHAT IT DOES: Shows the settings: derives the values that follow from
; the choices (SelectionTables18CB), starts each choice's animation
; (MenuChoiceAnimations), and gives the kind 3 object animation $36, $3C
; or $30 for the value 0, 1 or 2 of the setting in the current row
; ($18CD[$18EB]). Value 0 is not reached in the traces so far.
; HOW: The value is kept at $11DF and the kind listed for the row at
; $0B:C15E at $11E3 (unused here); MenuActorFind for kind 3.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the last value loaded;
; X = $11F9; Y from the last search; N, Z from X; C from the last compare;
; V from MenuChoiceAnimations.
%at($0BC0E7)
MenuSettingsShow:
    jsl.l SelectionTables18CBLong
    jsr.w MenuChoiceAnimations
    lda.l !wram_18EB
    asl a
    tax
    lda.l !selection_1,x
    sta.w !scratch_11df
    lda.l !menu_choice_kinds,x
    sta.w !scratch_11e3
    lda.w !scratch_11df
    cmp.w #$0000
    bne .lC122
    lda.w #$0003
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$0036
    beq .lC120
    lda.w #$0036
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC120:
    bra .lC15A
.lC122:
    cmp.w #$0001
    bne .lC13F
    lda.w #$0003
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$003C
    beq .lC13D
    lda.w #$003C
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC13D:
    bra .lC15A
.lC13F:
    cmp.w #$0002
    bne .lC15A
    lda.w #$0003
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$0030
    beq .lC15A
    lda.w #$0030
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC15A:
    ldx.w !actor_current
    rts

; @routine 0B:C1C8 MenuSettingsRowMarker
; WHAT IT DOES: Moves the settings row marker (the kind $0D object) to the
; row $18EB: its height +$02 = row * 32 + $34, and its animation $64, $5A
; or $6E for rows 0, 1 and 2. Rows 0 and 1 are not reached in the traces
; so far.
; HOW: MenuActorFind for kind $0D; the animation is started only when +$14
; does not already hold it.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the height; X = $11F9; Y
; from the search; N, Z from X; C, V from the add.
%at($0BC1C8)
MenuSettingsRowMarker:
    lda.w #$000D
    jsr.w MenuActorFind
    lda.l !wram_18EB
    bne .lC1E6
    lda.b !actor_anim,x
    cmp.w #$0064
    beq .lC1E4
    lda.w #$0064
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC1E4:
    bra .lC212
.lC1E6:
    cmp.w #$0001
    bne .lC1FD
    lda.b !actor_anim,x
    cmp.w #$005A
    beq .lC1FB
    lda.w #$005A
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC1FB:
    bra .lC212
.lC1FD:
    cmp.w #$0002
    bne .lC212
    lda.b !actor_anim,x
    cmp.w #$006E
    beq .lC212
    lda.w #$006E
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lC212:
    lda.l !wram_18EB
    asl a
    asl a
    asl a
    asl a
    asl a
    clc
    adc.w #$0034
    sta.b !actor_02,x
    ldx.w !actor_current
    rts

; @routine 0B:B952 MenuModeSelect
; WHAT IT DOES: The handler of the mode menu: moves the selection $18C3
; (MenuChoiceUpDown); on A or Start ($9000) plays sound $46 and confirms.
; Mode 3 sets both choices to 1, and with no saved progress ($70:FC0B)
; leaves the menu at once ($11EB = 1) and frees this object. Otherwise the
; object moves on to handler $C5EF (+$0A = 0), spawn list $270 is made
; (MenuActorSpawn), the choices are restored from the save for the mode
; (MenuChoicesFromSave), $18D3 = 0 and the choice $18CB is shown.
; HOW: MenuActorFree with Y = $11FB, this object's list slot.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: no press: A =
; 0; Y = $118B; Z set, N clear. Left: A = the list entry moved; X = $FFFF;
; Y = $11FB; N set, Z clear. Otherwise: A, Y, C as MenuSetPairAnimations;
; X = $11F9; N, Z from X.
%at($0BB952)
MenuModeSelect:
    jsr.w MenuChoiceUpDown
    lda.w !menu_joy1_pressed
    and.w #$9000
    beq .lB9B9
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.l !wram_18C3
    cmp.w #$0003
    bne .lB991
    lda.w #$0001
    sta.l !selection
    sta.l !selection_prev
    lda.l !save_progress
    and.w #$00FF
    bne .lB991
    lda.w #$0001
    sta.w !menu_exit
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
    rts
.lB991:
    lda.w #$C5EF
    sta.b !actor_handler,x
    stz.b !actor_0a,x
    ldx.w #$0270
    jsr.w MenuActorSpawn
    lda.l !wram_18C3
    jsr.w MenuChoicesFromSave
    lda.w #$0000
    sta.l !selection_out
    lda.l !selection
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
    ldx.w !actor_current
.lB9B9:
    rts
