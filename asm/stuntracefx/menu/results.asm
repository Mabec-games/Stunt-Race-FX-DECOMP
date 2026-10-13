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
