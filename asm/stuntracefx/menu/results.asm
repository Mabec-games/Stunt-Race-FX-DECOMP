; results.asm: the objects of the screens shown after a championship race
; (the standings), run once a frame through their +$08 like the other menu
; objects (handlers.c): X = the object, M 16-bit, X 16-bit, D = $0000, so
; the object's fields are direct page offsets indexed by X. Fields used
; here: +$08 (the handler), +$0A (a count), +$10, +$12 (the sprite group),
; +$14 (the animation) and +$22 (a step). $0DD6 is the race phase (8 once
; the championship is over) and $0FAC the leader's place.
; C: src/stuntracefx/menu/results.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:D8A9 ResultsWaitThenRise
; WHAT IT DOES: Counts object X's +$0A down; when it runs out, the object
; goes on to ResultsRise (+$08 = $D8B5) with +$0A = 0.
; HOW: DEC and the sign.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting: N,
; Z from the count; else A = $D8B5, N set, Z clear.
%at($0BD8A9)
ResultsWaitThenRise:
    dec.b !actor_0a,x
    bpl .lD8B4
    lda.w #$D8B5
    sta.b !dp_08,x
    stz.b !actor_0a,x
.lD8B4:
    rts

; @routine 0B:D8B5 ResultsRise
; WHAT IT DOES: Raises the standings panel: +$0A goes up by 4 to $66 (kept
; at $1921), the scroll $11EF is $66 less it; at $66 the object goes on to
; ResultsOpenTop (+$08 = $D8FA, +$0A = 0) and HDMA channels 3, 5, 6 and 7
; are asked for ($E8 for $420C on the frame's write list).
; HOW: The add is held at $66 with CMP and the carry; A is kept on the
; stack around the scroll.
; Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; rising: A =
; +$0A, N, Z, C from the compare with $66; done: X = the object, N, Z from
; it, A low byte $E8.
%at($0BD8B5)
ResultsRise:
    rep.b #$30
    lda.b !actor_0a,x
    clc
    adc.w #$0004
    cmp.w #$0066
    bcc .lD8C5
    lda.w #$0066
.lD8C5:
    sta.b !actor_0a,x
    sta.l !wram_1921
    pha
    lda.w #$0066
    sec
    sbc.b !actor_0a,x
    sta.w !menu_scroll_y
    pla
    cmp.w #$0066
    beq .lD8DC
    rts
.lD8DC:
    lda.w #$D8FA
    sta.b !dp_08,x
    lda.w #$0000
    sta.b !actor_0a,x
    sep.b #$20
    lda.b #$E8
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$420C
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    rts

; @routine 0B:D8FA ResultsOpenTop
; WHAT IT DOES: Opens the window's top band: +$0A up by 8, its low byte to
; $7F:130D; at $100 the object goes on to ResultsOpenBottom (+$08 = $D91B,
; +$0A = 0).
; HOW: An 8-bit long store between SEP and REP.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit; A
; = +$0A (0 when done); N, Z, C from the compare with $100, or from 0.
%at($0BD8FA)
ResultsOpenTop:
    lda.b !actor_0a,x
    clc
    adc.w #$0008
    sta.b !actor_0a,x
    cmp.w #$0100
    beq .lD910
    sep.b #$20
    sta.l !window_lines_1301+12
    rep.b #$20
    rts
.lD910:
    lda.w #$D91B
    sta.b !dp_08,x
    lda.w #$0000
    sta.b !actor_0a,x
    rts

; @routine 0B:D91B ResultsOpenBottom
; WHAT IT DOES: As ResultsOpenTop for the bottom band ($7F:130F); at $100
; the object goes on to ResultsWaitButton (+$08 = $DA8A, +$0A = 0), HDMA
; channels 5, 6 and 7 are asked for ($E0 for $420C) with the window mask
; $212E cleared, and, when the leader's place $0FAC is not 0 and the
; championship is over ($0DD6 = 8), the kind $18 object (MenuActorFind)
; starts ResultsLeaderBlink (+$08 = $DA33, +$0A = +$22 = 0).
; HOW: Two bytes on the frame's write list; the tests through 8-bit values
; read as words and masked.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: M 16-bit;
; opening: as ResultsOpenTop; done: A, X and the flags from the last step
; (X = the kind $18 object when it was started).
%at($0BD91B)
ResultsOpenBottom:
    lda.b !actor_0a,x
    clc
    adc.w #$0008
    sta.b !actor_0a,x
    cmp.w #$0100
    beq .lD931
    sep.b #$20
    sta.l !window_lines_1301+14
    rep.b #$20
    rts
.lD931:
    lda.w #$DA8A
    sta.b !dp_08,x
    lda.w #$0000
    sta.b !actor_0a,x
    sep.b #$20
    lda.b #$E0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$420C
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #$212E
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    lda.l !p1_lead+9
    and.w #$00FF
    cmp.w #$0000
    beq .lD984
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    bne .lD984
    lda.w #$0018
    jsr.w MenuActorFind
    lda.w #$DA33
    sta.b !dp_08,x
    stz.b !actor_0a,x
    stz.b !actor_22,x
.lD984:
    rts

; @routine 0B:D985 ResultsPlaceWait
; WHAT IT DOES: The place marker for place +$22: when the leader's place
; $0FAC is it and the championship is over ($0DD6 = 8), copies its sprite
; group (MenuActorCopyGroup) and starts ResultsLeaderBlink (+$08 = $DA33,
; +$22 = +$0A = 0); otherwise MenuWaitThenAnimate.
; HOW: Byte compares through words masked with $FF; X reloaded from $11F9
; after the copy.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: started: X =
; $11F9; A = $DA33; N, Z from it; otherwise as MenuWaitThenAnimate.
%at($0BD985)
ResultsPlaceWait:
    lda.l !p1_lead+9
    and.w #$00FF
    cmp.b !actor_22,x
    bne .lD9AC
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    bne .lD9AC
    jsr.w MenuActorCopyGroup
    ldx.w !actor_current
    lda.w #$DA33
    sta.b !dp_08,x
    stz.b !actor_22,x
    stz.b !actor_0a,x
    rts
.lD9AC:
    jmp.w MenuWaitThenAnimate

; @routine 0B:D9AF ResultsPlaceCar
; WHAT IT DOES: The car picture for place +$22: the car's kind (the low two
; bits of the byte at $70:FC6F + ResultsCarIndex) picks its animation from
; the table at $0B:DA17 for the leader's place ($0FAC) or $0B:DA0F for the
; others, stored at +$14; then MenuWaitThenAnimate.
; HOW: The table read through X, then X reloaded from $11F9.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
; MenuWaitThenAnimate.
%at($0BD9AF)
ResultsPlaceCar:
    lda.l !p1_lead+9
    and.w #$00FF
    cmp.b !actor_22,x
    bne .lD9D2
    jsr.w ResultsCarIndex
    lda.l !save_course_names+2,x
    and.w #$0003
    asl a
    tax
    lda.l !results_car_anim_leader,x
    ldx.w !actor_current
    sta.b !actor_anim,x
    jmp.w MenuWaitThenAnimate
.lD9D2:
    jsr.w ResultsCarIndex
    lda.l !save_course_names+2,x
    and.w #$0003
    asl a
    tax
    lda.l !results_car_anim,x
    ldx.w !actor_current
    sta.b !actor_anim,x
    jmp.w MenuWaitThenAnimate

; @routine 0B:D9EA ResultsCarIndex
; WHAT IT DOES: X = the index of the car for place +$22 - 1 in the course
; $0D4A's results: course * 9 + (place - 1) * 3.
; HOW: Shifts and adds; $11DF and $11E1 hold the parts.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = A = the
; index; N, Z, C, V from the last add.
%at($0BD9EA)
ResultsCarIndex:
    lda.l !wram_0D4A
    and.w #$00FF
    sta.w !scratch_11df
    asl a
    asl a
    asl a
    clc
    adc.w !scratch_11df
    sta.w !scratch_11e1
    lda.b !actor_22,x
    dec a
    sta.w !scratch_11df
    asl a
    clc
    adc.w !scratch_11df
    clc
    adc.w !scratch_11e1
    tax
    rts

; @routine 0B:DA1F ResultsLeaderBlinkResume
; WHAT IT DOES: Starts the leader's place marker blinking where it was:
; copies its sprite group (MenuActorCopyGroup), takes the count and step
; back from $190F and $1911, makes ResultsLeaderBlink its handler and runs
; it.
; HOW: Falls into ResultsLeaderBlink.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
; ResultsLeaderBlink.
%at($0BDA1F)
ResultsLeaderBlinkResume:
    jsr.w MenuActorCopyGroup
    lda.l !wram_190D+2
    sta.b !actor_0a,x
    lda.l !wram_190D+4
    sta.b !actor_22,x
    lda.w #$DA33
    sta.b !dp_08,x

; @routine 0B:DA33 ResultsLeaderBlink
; WHAT IT DOES: The leader's place marker blinks: every sixth frame (+$0A
; run down from 5) its step +$22 flips between 0 and 1, which picks the
; palette bits $200 or $600 (table $0B:DA86) for its group
; (ResultsGroupPalette); the count and step are kept at $190F and $1911.
; HOW: The palette word through $11DF; X reloaded from $11F9.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
; ResultsGroupPalette.
%at($0BDA33)
ResultsLeaderBlink:
    dec.b !actor_0a,x
    bpl .lDA44
    lda.w #$0005
    sta.b !actor_0a,x
    lda.b !actor_22,x
    inc a
    and.w #$0001
    sta.b !actor_22,x
.lDA44:
    lda.b !actor_22,x
    asl a
    tax
    lda.l !results_blink_palettes,x
    sta.w !scratch_11df
    ldx.w !actor_current
    lda.b !actor_0a,x
    sta.l !wram_190D+2
    lda.b !actor_22,x
    sta.l !wram_190D+4
    jsr.w ResultsGroupPalette
    rts

; @routine 0B:DA62 ResultsGroupPalette
; WHAT IT DOES: Gives every sprite of object X's group the palette bits
; $11DF: each attribute word (the group's sprite count at $7F:1356 + +$12,
; its entries from $7F:135A) has bits 9-11 replaced.
; HOW: AND #$F1FF, ORA $11DF per entry, 8 bytes apart; X kept on the stack.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X as on
; entry, N, Z from it (PLX); Y = $FFFF; A = the last entry offset + 8;
; C, V from the last add.
%at($0BDA62)
ResultsGroupPalette:
    phx
    lda.b !actor_12,x
    tax
    lda.l !menu_sprite_groups,x
    tay
    inx
    inx
.lDA6D:
    lda.l !menu_sprite_attr,x
    and.w #$F1FF
    ora.w !scratch_11df
    sta.l !menu_sprite_attr,x
    txa
    clc
    adc.w #$0008
    tax
    dey
    bpl .lDA6D
    plx
    rts

; @routine 0B:DA8A ResultsWaitButton
; WHAT IT DOES: Waits for A, B or Start: then, when the championship is
; over ($0DD6 = 8), the panel goes down again (ResultsLower: +$08 = $DAD1,
; +$0A = $66), otherwise the screen ends ($11EB = 1, the object freed,
; reached by a unit test, unit_menus); sound
; $46 either way.
; HOW: The leader's place $0FAC is read and masked but not used.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: no press: Y
; = the presses, A = 0, Z set; else M 16-bit, A low byte $46 and the
; flags as SoundPort0QueuePush leaves them.
%at($0BDA8A)
ResultsWaitButton:
    ldy.w !menu_joy1_pressed
    tya
    and.w #$9000
    beq .done
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    bne .end_screen
    lda.w !p1_lead+9
    and.w #$00FF
    lda.w #$DAD1
    sta.b !dp_08,x
    lda.w #$0066
    sta.b !actor_0a,x
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    rts
.end_screen:
    lda.w #$0001
    sta.w !menu_exit
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
.done:
    rts

; @routine 0B:DC6D ResultsRiseAgain
; WHAT IT DOES: As ResultsRise, then goes on to ResultsWaitEnd (+$08 =
; $DC9F, +$0A = 0) with no HDMA change.
; HOW: As ResultsRise.
; Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; rising: as
; ResultsRise; done: A = 0, Z set, N clear.
%at($0BDC6D)
ResultsRiseAgain:
    rep.b #$30
    lda.b !actor_0a,x
    clc
    adc.w #$0004
    cmp.w #$0066
    bcc .lDC7D
    lda.w #$0066
.lDC7D:
    sta.b !actor_0a,x
    sta.l !wram_1921
    pha
    lda.w #$0066
    sec
    sbc.b !actor_0a,x
    sta.w !menu_scroll_y
    pla
    cmp.w #$0066
    beq .lDC94
    rts
.lDC94:
    lda.w #$DC9F
    sta.b !dp_08,x
    lda.w #$0000
    sta.b !actor_0a,x
    rts

; @routine 0B:DC9F ResultsWaitEnd
; WHAT IT DOES: Waits for A, B or Start, then ends the screen ($11EB = 1),
; frees the object (MenuActorFree) and plays sound $46.
; HOW: As ResultsWaitButton's other case.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
; ResultsWaitButton.
%at($0BDC9F)
ResultsWaitEnd:
    ldy.w !menu_joy1_pressed
    tya
    and.w #$9000
    beq .lDCBE
    lda.w #$0001
    sta.w !menu_exit
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
.lDCBE:
    rts

; @routine 0B:DD01 ResultsOverOrHide
; WHAT IT DOES: When the championship is over ($0DD6 = 8),
; MenuWaitThenAnimate; otherwise the object is hidden (+$10 = $FFFF) and
; does nothing more (+$08 = MenuActorNoStep; reached by a unit test,
; unit_menus).
; HOW: The other case is the code after the JMP ($0B:DD10), reloading X
; from $11F9.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as
; MenuWaitThenAnimate; hidden: X = $11F9, A = $DF7B, N set, Z clear.
%at($0BDD01)
ResultsOverOrHide:
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    bne .hide
    jmp.w MenuWaitThenAnimate
.hide:
    ldx.w !actor_current
    lda.w #$FFFF
    sta.b !actor_10,x
    lda.w #MenuActorNoStep
    sta.b !dp_08,x
    rts
; @routine 0B:DAD1 ResultsLower
; WHAT IT DOES: Lowers the standings panel: +$0A goes down by 4 to 0 (kept
; at $1921), the scroll $11EF is $66 less it; at 0 the object goes on to
; ResultsRiseAgain (+$08 = $DC6D, +$0A = 0), $1925 = $5F, the objects of
; kinds $14-$17 are hidden and those of kinds $1A-$25 shown
; (MenuActorsSet10 with $FFFF and 0), and for the leader's place $0FAC
; (1-3; places 2 and 3 reached by unit tests,
; unit_menus) the kind $18 object gets animation $262, $268 or $26E,
; +$02 = $88 and ResultsLeaderBlinkResume, the kind $19 object animation
; $250, $256 or $25C, +$02 = $58 and MenuWaitThenAnimate.
; HOW: An animation is restarted (+$16 = +$18 = 0) only when it changes.
; The place tests are byte compares through words masked with $FF.
; Entry: X = the object, D = $0000. Exit: M 16-bit, X 16-bit; lowering: A
; = +$0A, N, Z, C from the compare with 0; done: X = the kind $19 object
; (or as MenuActorFind left it), A and the flags from the last step.
%at($0BDAD1)
ResultsLower:
    rep.b #$30
    lda.b !actor_0a,x
    sec
    sbc.w #$0004
    bpl .lDADE
    lda.w #$0000
.lDADE:
    sta.b !actor_0a,x
    sta.l !wram_1921
    pha
    lda.w #$0066
    sec
    sbc.b !actor_0a,x
    sta.w !menu_scroll_y
    pla
    cmp.w #$0000
    beq .lDAF5
    rts
.lDAF5:
    lda.w #$DC6D
    sta.b !dp_08,x
    lda.w #$0000
    sta.b !actor_0a,x
    lda.w #$005F
    sta.l !wram_1925
    lda.w #$0014
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0015
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0016
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$0017
    ldy.w #$FFFF
    jsr.w MenuActorsSet10
    lda.w #$001A
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$001B
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$001C
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$001D
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$001E
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$001F
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0020
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0021
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0022
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0023
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0024
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0025
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0018
    jsr.w MenuActorFind
    lda.l !p1_lead+9
    and.w #$00FF
    beq .lDC01
    cmp.w #$0001
    bne .lDBC6
    lda.b !actor_anim,x
    cmp.w #$0262
    beq .lDBBA
    lda.w #$0262
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lDBBA:
    lda.w #$0088
    sta.b !dp_02,x
    lda.w #$DA1F
    sta.b !dp_08,x
    bra .lDC01
.lDBC6:
    cmp.w #$0002
    bne .lDBE7
    lda.b !actor_anim,x
    cmp.w #$0268
    beq .lDBDB
    lda.w #$0268
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lDBDB:
    lda.w #$0088
    sta.b !dp_02,x
    lda.w #$DA1F
    sta.b !dp_08,x
    bra .lDC01
.lDBE7:
    lda.b !actor_anim,x
    cmp.w #$026E
    beq .lDBF7
    lda.w #$026E
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lDBF7:
    lda.w #$0088
    sta.b !dp_02,x
    lda.w #$DA1F
    sta.b !dp_08,x
.lDC01:
    lda.w #$0019
    jsr.w MenuActorFind
    lda.l !p1_lead+9
    and.w #$00FF
    beq .lDC6C
    cmp.w #$0001
    bne .lDC31
    lda.b !actor_anim,x
    cmp.w #$0250
    beq .lDC25
    lda.w #$0250
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lDC25:
    lda.w #$0058
    sta.b !dp_02,x
    lda.w #$D5EC
    sta.b !dp_08,x
    bra .lDC6C
.lDC31:
    cmp.w #$0002
    bne .lDC52
    lda.b !actor_anim,x
    cmp.w #$0256
    beq .lDC46
    lda.w #$0256
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lDC46:
    lda.w #$0058
    sta.b !dp_02,x
    lda.w #$D5EC
    sta.b !dp_08,x
    bra .lDC6C
.lDC52:
    lda.b !actor_anim,x
    cmp.w #$025C
    beq .lDC62
    lda.w #$025C
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lDC62:
    lda.w #$0058
    sta.b !dp_02,x
    lda.w #$D5EC
    sta.b !dp_08,x
.lDC6C:
    rts

; @routine 0B:DCBF ResultsTimeDigits
; WHAT IT DOES: When the championship is over ($0DD6 = 8), shows the time
; $0EEF, $0EEE, $0EED (two digits each) on object X's sprite group and
; stops it (+$08 = MenuActorNoStep); otherwise hides it (+$10 = $FFFF;
; reached by unit tests, unit_menus).
; HOW: MenuActorCopyGroup, then the digits from the group's first entry
; ($11E1); X reloaded from $11F9.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9; A
; = $DF7B; N set, Z clear.
%at($0BDCBF)
ResultsTimeDigits:
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    bne .lDCF3
    jsr.w MenuActorCopyGroup
    lda.b !actor_12,x
    inc a
    inc a
    sta.w !scratch_11e1
    lda.l !wram_0EEF
    jsr.w ResultsBcdPair
    lda.l !wram_0EEE
    jsr.w ResultsBcdPair
    lda.l !wram_0EED
    jsr.w ResultsBcdPair
    ldx.w !actor_current
    lda.w #$DF7B
    sta.b !dp_08,x
    rts
.lDCF3:
    ldx.w !actor_current
    lda.w #$FFFF
    sta.b !actor_10,x
    lda.w #$DF7B
    sta.b !dp_08,x
    rts

; @routine 0B:DD1E ResultsInitials
; WHAT IT DOES: Shows the initials of the car for place +$22 (three 5-bit
; letters in the word at $70:FC6D + ResultsCarIndex) on object X's sprite
; group, then starts ResultsLeaderBlink when it is the leader's place and
; the championship is over, otherwise stops (MenuActorNoStep).
; HOW: MenuActorCopyGroup; the word shifted right 5 at a time through
; $11DF; ResultsLetterTiles from the group's first entry ($11E5).
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9;
; A and the flags from the last step.
%at($0BDD1E)
ResultsInitials:
    jsr.w MenuActorCopyGroup
    lda.b !actor_12,x
    inc a
    inc a
    sta.w !scratch_11e5
    jsr.w ResultsCarIndex
    lda.l !save_course_names,x
    sta.w !scratch_11df
    and.w #$001F
    jsr.w ResultsLetterTiles
    lda.w !scratch_11df
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !scratch_11df
    and.w #$001F
    jsr.w ResultsLetterTiles
    lda.w !scratch_11df
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sta.w !scratch_11df
    and.w #$001F
    jsr.w ResultsLetterTiles
    ldx.w !actor_current
    lda.l !p1_lead+9
    and.w #$00FF
    cmp.b !actor_22,x
    bne .lDD81
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    bne .lDD81
    ldx.w !actor_current
    lda.w #$DA33
    sta.b !dp_08,x
    stz.b !actor_22,x
    stz.b !actor_0a,x
    rts
.lDD81:
    lda.w #$DF7B
    sta.b !dp_08,x
    rts

; @routine 0B:DD87 ResultsLetterTiles
; WHAT IT DOES: Puts letter A (0-31) in the next two sprites of the group
; at $11E5: its tile from the table at $0B:DDD3 and the tile $10 on below
; it.
; HOW: The tile through $11E3; X kept on the stack.
; Entry: M 16-bit, X 16-bit; A = the letter. Exit: X as on entry, N, Z
; from it; A = the new $11E5; C, V from its add.
%at($0BDD87)
ResultsLetterTiles:
    phx
    asl a
    tax
    lda.l !results_letter_tiles,x
    sta.w !scratch_11e3
    ldx.w !scratch_11e5
    lda.l !menu_sprite_attr,x
    and.w #$FE00
    ora.w !scratch_11e3
    sta.l !menu_sprite_attr,x
    lda.w !scratch_11e5
    clc
    adc.w #$0008
    sta.w !scratch_11e5
    lda.w !scratch_11e3
    clc
    adc.w #$0010
    sta.w !scratch_11e3
    ldx.w !scratch_11e5
    lda.l !menu_sprite_attr,x
    and.w #$FE00
    ora.w !scratch_11e3
    sta.l !menu_sprite_attr,x
    lda.w !scratch_11e5
    clc
    adc.w #$0008
    sta.w !scratch_11e5
    plx
    rts

; @routine 0B:DE0B ResultsLapTimes
; WHAT IT DOES: Shows the time of place +$22 (the bytes at $0F78, $0F77 and
; $0F76 + it, two digits each) on object X's sprite group; a place with no
; time ($FF) first blanks its line (ResultsNoTime). Then starts
; ResultsLeaderBlink when +$20 is the leader's place and the championship
; is over, otherwise stops (MenuActorNoStep).
; HOW: MenuActorCopyGroup; ResultsBcdPair from the group's first entry
; ($11E1); the place and its first byte kept on the stack.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: X = $11F9;
; A and the flags from the last step.
%at($0BDE0B)
ResultsLapTimes:
    jsr.w MenuActorCopyGroup
    lda.b !actor_12,x
    inc a
    inc a
    sta.w !scratch_11e1
    lda.b !actor_22,x
    tax
    lda.l !record_times+2,x
    pha
    phx
    and.w #$00FF
    cmp.w #$00FF
    bne .lDE29
    jsr.w ResultsNoTime
.lDE29:
    plx
    pla
    jsr.w ResultsBcdPair
    lda.l !record_times+1,x
    jsr.w ResultsBcdPair
    lda.l !record_times,x
    jsr.w ResultsBcdPair
    ldx.w !actor_current
    lda.l !p1_lead+9
    and.w #$00FF
    cmp.b !actor_20,x
    bne .lDE63
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    bne .lDE63
    ldx.w !actor_current
    lda.w #$DA33
    sta.b !dp_08,x
    stz.b !actor_22,x
    stz.b !actor_0a,x
    rts
.lDE63:
    lda.w #$DF7B
    sta.b !dp_08,x
    rts

; @routine 0B:DE69 ResultsBcdPair
; WHAT IT DOES: Puts the two decimal digits of the byte A (high, then low)
; in the group at $11E1 (ResultsDigitTiles).
; HOW: The byte kept at $11E5.
; Entry: M 16-bit, X 16-bit; A low byte = the digits. Exit: as
; ResultsDigitTiles.
%at($0BDE69)
ResultsBcdPair:
    and.w #$00FF
    sta.w !scratch_11e5
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$000F
    jsr.w ResultsDigitTiles
    lda.w !scratch_11e5
    and.w #$000F
    jsr.w ResultsDigitTiles
    rts

; @routine 0B:DE83 ResultsDigitTiles
; WHAT IT DOES: Puts digit A (0-9; 15 for a blank, tile $94) in the next
; two sprites of the group at $11E1: tile A + $50 and the one $10 on.
; HOW: As ResultsLetterTiles with the tile computed.
; Entry: M 16-bit, X 16-bit; A = the digit. Exit: X as on entry, N, Z
; from it; A = the new $11E1; C, V from its add.
%at($0BDE83)
ResultsDigitTiles:
    phx
    cmp.w #$000F
    bne .lDE8C
    lda.w #$0044
.lDE8C:
    clc
    adc.w #$0050
    sta.w !scratch_11e3
    ldx.w !scratch_11e1
    lda.l !menu_sprite_attr,x
    and.w #$FE00
    ora.w !scratch_11e3
    sta.l !menu_sprite_attr,x
    lda.w !scratch_11e1
    clc
    adc.w #$0008
    sta.w !scratch_11e1
    lda.w !scratch_11e3
    clc
    adc.w #$0010
    sta.w !scratch_11e3
    ldx.w !scratch_11e1
    lda.l !menu_sprite_attr,x
    and.w #$FE00
    ora.w !scratch_11e3
    sta.l !menu_sprite_attr,x
    lda.w !scratch_11e1
    clc
    adc.w #$0008
    sta.w !scratch_11e1
    plx
    rts

; @routine 0B:DED5 ResultsNoTime
; WHAT IT DOES: For a place with no time: by the current object's kind
; ($20, $21 or another; $20 reached by a unit test, unit_menus) blanks
; the initials of the kind $23, $24 or $25 object (ResultsBlankLetters)
; and hides the kind $1D, $1E or $1F object (+$10 = $FFFF, +$02 = $200,
; MenuActorNoStep).
; HOW: $11DF and $11E1 kept on the stack.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: X = the hidden object; A =
; $11DF; N, Z from it.
%at($0BDED5)
ResultsNoTime:
    lda.w !scratch_11df
    pha
    lda.w !scratch_11e1
    pha
    ldx.w !actor_current
    lda.b !actor_kind,x
    cmp.w #$0020
    beq .lDF3A
    cmp.w #$0021
    beq .lDF13
    lda.w #$0025
    jsr.w MenuActorFind
    jsr.w ResultsBlankLetters
    lda.w #$001F
    jsr.w MenuActorFind
    lda.w #$FFFF
    sta.b !actor_10,x
    lda.w #$0200
    sta.b !dp_02,x
    lda.w #$DF7B
    sta.b !dp_08,x
    pla
    sta.w !scratch_11e1
    pla
    sta.w !scratch_11df
    rts
.lDF13:
    lda.w #$0024
    jsr.w MenuActorFind
    jsr.w ResultsBlankLetters
    lda.w #$001E
    jsr.w MenuActorFind
    lda.w #$FFFF
    sta.b !actor_10,x
    lda.w #$0200
    sta.b !dp_02,x
    lda.w #$DF7B
    sta.b !dp_08,x
    pla
    sta.w !scratch_11e1
    pla
    sta.w !scratch_11df
    rts
.lDF3A:
    lda.w #$0023
    jsr.w MenuActorFind
    jsr.w ResultsBlankLetters
    lda.w #$001D
    jsr.w MenuActorFind
    lda.w #$FFFF
    sta.b !actor_10,x
    lda.w #$0200
    sta.b !dp_02,x
    lda.w #$DF7B
    sta.b !dp_08,x
    pla
    sta.w !scratch_11e1
    pla
    sta.w !scratch_11df
    rts

; @routine 0B:DF61 ResultsBlankLetters
; WHAT IT DOES: Puts three blank letters (26) in object X's sprite group.
; HOW: ResultsLetterTiles from the group's first entry ($11E5).
; Entry: M 16-bit, X 16-bit; X = the object. Exit: as ResultsLetterTiles.
%at($0BDF61)
ResultsBlankLetters:
    lda.b !actor_12,x
    inc a
    inc a
    sta.w !scratch_11e5
    lda.w #$001A
    jsr.w ResultsLetterTiles
    lda.w #$001A
    jsr.w ResultsLetterTiles
    lda.w #$001A
    jsr.w ResultsLetterTiles
    rts

; @routine 0B:DFDB ResultsPlace0
; WHAT IT DOES: Place 0's points object (see place_points in the C):
; counts +$0A down, then shows, with ResultsPlace0Animate as its handler,
; and shows car 0's points or animates by $0D48 (the hiding below it can
; never run: the BCC after CMP #0 never branches).
; HOW: As the other places, with 0.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting: N,
; Z from the count; else as ResultsPoints, ResultsPhaseAnim or the hiding
; store (A = $FFFF).
%at($0BDFDB)
ResultsPlace0:
    dec.b !actor_0a,x
    bpl .lE005
    stz.b !actor_10,x
    lda.w #$E018
    sta.b !dp_08,x
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    beq ResultsPlace0_points
    lda.l !wram_0D48
    and.w #$00FF
    cmp.w #$0000
    beq .lE002
    bcc ResultsPlace0Hide
    bne ResultsPlace0_points
.lE002:
    jsr.w ResultsPhaseAnim
.lE005:
    rts

; @routine 0B:E006 ResultsPlace0Hide
; WHAT IT DOES: ResultsPlace0's hiding store, which nothing reaches (the
; BCC after CMP #0 before it never branches): +$10 = $FFFF.
; HOW: A 16-bit direct page store indexed by the object.
; Entry: M 16-bit, D = $0000; X = the object. Exit: A = $FFFF; N set, Z
; clear.
ResultsPlace0Hide:
    lda.w #$FFFF
    sta.b !actor_10,x
    rts
ResultsPlace0_points:
    lda.w #$0000
    jsr.w ResultsPoints
    lda.w #$E0DF
    sta.b !dp_08,x
    rts

; @routine 0B:E018 ResultsPlace0Animate
; WHAT IT DOES: Place 0's points object's frame while animating:
; MenuActorAnimateFrame.
; HOW: JSR, RTS.
; Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
%at($0BE018)
ResultsPlace0Animate:
    jsr.w MenuActorAnimateFrame
    rts

; @routine 0B:E01C ResultsPlace1
; WHAT IT DOES: As ResultsPlace0 for place 1 (handler ResultsPlace1Animate).
; HOW: As ResultsPlace0, with 1.
; Entry: as ResultsPlace0. Exit: as ResultsPlace0.
%at($0BE01C)
ResultsPlace1:
    dec.b !actor_0a,x
    bpl .lE046
    stz.b !actor_10,x
    lda.w #$E059
    sta.b !dp_08,x
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    beq .lE04D
    lda.l !wram_0D48
    and.w #$00FF
    cmp.w #$0001
    beq .lE043
    bcc .lE047
    bne .lE04D
.lE043:
    jsr.w ResultsPhaseAnim
.lE046:
    rts
.lE047:
    lda.w #$FFFF
    sta.b !actor_10,x
    rts
.lE04D:
    lda.w #$0001
    jsr.w ResultsPoints
    lda.w #$E0DF
    sta.b !dp_08,x
    rts

; @routine 0B:E059 ResultsPlace1Animate
; WHAT IT DOES: As ResultsPlace0Animate, for place 1.
; HOW: JSR, RTS.
; Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
%at($0BE059)
ResultsPlace1Animate:
    jsr.w MenuActorAnimateFrame
    rts

; @routine 0B:E05D ResultsPlace2
; WHAT IT DOES: As ResultsPlace0 for place 2 (handler ResultsPlace2Animate).
; HOW: As ResultsPlace0, with 2.
; Entry: as ResultsPlace0. Exit: as ResultsPlace0.
%at($0BE05D)
ResultsPlace2:
    dec.b !actor_0a,x
    bpl .lE087
    stz.b !actor_10,x
    lda.w #$E09A
    sta.b !dp_08,x
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    beq .lE08E
    lda.l !wram_0D48
    and.w #$00FF
    cmp.w #$0002
    beq .lE084
    bcc .lE088
    bne .lE08E
.lE084:
    jsr.w ResultsPhaseAnim
.lE087:
    rts
.lE088:
    lda.w #$FFFF
    sta.b !actor_10,x
    rts
.lE08E:
    lda.w #$0002
    jsr.w ResultsPoints
    lda.w #$E0DF
    sta.b !dp_08,x
    rts

; @routine 0B:E09A ResultsPlace2Animate
; WHAT IT DOES: As ResultsPlace0Animate, for place 2.
; HOW: JSR, RTS.
; Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
%at($0BE09A)
ResultsPlace2Animate:
    jsr.w MenuActorAnimateFrame
    rts

; @routine 0B:E09E ResultsPlace3
; WHAT IT DOES: As ResultsPlace0 for place 3 (handler ResultsPlace3Animate).
; HOW: As ResultsPlace0, with 3.
; Entry: as ResultsPlace0. Exit: as ResultsPlace0.
%at($0BE09E)
ResultsPlace3:
    dec.b !actor_0a,x
    bpl .lE0C8
    stz.b !actor_10,x
    lda.w #$E0DB
    sta.b !dp_08,x
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0008
    beq .lE0CF
    lda.l !wram_0D48
    and.w #$00FF
    cmp.w #$0003
    beq .lE0C5
    bcc .lE0C9
    bne .lE0CF
.lE0C5:
    jsr.w ResultsPhaseAnim
.lE0C8:
    rts
.lE0C9:
    lda.w #$FFFF
    sta.b !actor_10,x
    rts
.lE0CF:
    lda.w #$0003
    jsr.w ResultsPoints
    lda.w #$E0DF
    sta.b !dp_08,x
    rts

; @routine 0B:E0DB ResultsPlace3Animate
; WHAT IT DOES: As ResultsPlace0Animate, for place 3.
; HOW: JSR, RTS.
; Entry: as MenuActorAnimateFrame. Exit: as MenuActorAnimateFrame.
%at($0BE0DB)
ResultsPlace3Animate:
    jsr.w MenuActorAnimateFrame
    rts

; @routine 0B:E0E0 ResultsPoints
; WHAT IT DOES: Shows the points of car A (five digits: the low digit of
; $0EFB + 3A, then the bytes $0EFA and $0EF9 + 3A) on object X's sprite
; group.
; HOW: MenuActorCopyGroup with A kept on the stack, then at $11DF; the
; digits from the group's first entry ($11E1).
; Entry: M 16-bit, X 16-bit, D = $0000; A = the car; X = the object. Exit:
; X = $11F9, N, Z from it; A and C, V from the last step.
%at($0BE0E0)
ResultsPoints:
    pha
    jsr.w MenuActorCopyGroup
    pla
    sta.w !scratch_11df
    lda.b !actor_12,x
    inc a
    inc a
    sta.w !scratch_11e1
    lda.w !scratch_11df
    asl a
    clc
    adc.w !scratch_11df
    tax
    lda.l !lap_times+11,x
    and.w #$000F
    jsr.w ResultsPointsTile
    lda.l !lap_times+10,x
    jsr.w ResultsPointsPair
    lda.l !lap_times+9,x
    jsr.w ResultsPointsPair
    ldx.w !actor_current
    rts

; @routine 0B:E114 ResultsPointsPair
; WHAT IT DOES: Puts the two decimal digits of the byte A (high, then low)
; in the group at $11E1 as single tiles 0-9 (ResultsPointsTile).
; HOW: The byte kept at $11E5.
; Entry: M 16-bit, X 16-bit; A low byte = the digits. Exit: as
; ResultsPointsTile.
%at($0BE114)
ResultsPointsPair:
    and.w #$00FF
    sta.w !scratch_11e5
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$000F
    jsr.w ResultsPointsTile
    lda.w !scratch_11e5
    and.w #$000F
    jsr.w ResultsPointsTile
    rts

; @routine 0B:E12E ResultsPointsTile
; WHAT IT DOES: Puts tile A in the next sprite of the group at $11E1.
; HOW: The tile through $11E3; X kept on the stack.
; Entry: M 16-bit, X 16-bit; A = the tile. Exit: X as on entry, N, Z from
; it; A = the new $11E1; C, V from its add.
%at($0BE12E)
ResultsPointsTile:
    phx
    sta.w !scratch_11e3
    ldx.w !scratch_11e1
    lda.l !menu_sprite_attr,x
    and.w #$FE00
    ora.w !scratch_11e3
    sta.l !menu_sprite_attr,x
    lda.w !scratch_11e1
    clc
    adc.w #$0008
    sta.w !scratch_11e1
    plx
    rts

; @routine 0B:E14F ResultsPhaseAnim
; WHAT IT DOES: Gives object X the animation for the race phase $0DD6: $1A4
; for 6, $17C for $0A, $19A for $0C, $190 for $0E, $186 for $10; none for
; the others. Reached by unit tests (unit_menus).
; HOW: A compare chain; the animation is restarted (+$16 = +$18 = 0) only
; when it changes.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: A = the
; animation (or the phase), N, Z, C from the last compare.
%at($0BE14F)
ResultsPhaseAnim:
    lda.l !engine_phase_p1
    and.w #$00FF
    cmp.w #$0006
    beq .lE170
    cmp.w #$000A
    beq .lE181
    cmp.w #$000C
    beq .lE192
    cmp.w #$000E
    beq .lE1A3
    cmp.w #$0010
    beq .lE1B4
    rts
.lE170:
    lda.b !actor_anim,x
    cmp.w #$01A4
    beq .lE180
    lda.w #$01A4
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lE180:
    rts
.lE181:
    lda.b !actor_anim,x
    cmp.w #$017C
    beq .lE191
    lda.w #$017C
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lE191:
    rts
.lE192:
    lda.b !actor_anim,x
    cmp.w #$019A
    beq .lE1A2
    lda.w #$019A
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lE1A2:
    rts
.lE1A3:
    lda.b !actor_anim,x
    cmp.w #$0190
    beq .lE1B3
    lda.w #$0190
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lE1B3:
    rts
.lE1B4:
    lda.b !actor_anim,x
    cmp.w #$0186
    beq .lE1C4
    lda.w #$0186
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lE1C4:
    rts

