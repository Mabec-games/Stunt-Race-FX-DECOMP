; handlers.asm: handlers of the menu screens' objects (the records described
; in actors.c), each run once a frame through the record's +$08 with X =
; the record, M 16-bit, X 16-bit and D = $0000, so the record's fields are
; direct page offsets indexed by X. Fields used here: +$0A (a step count),
; +$14/+$16/+$18 (the animation, see MenuActorAnimate) and +$1E (a frame
; count).
; C: src/stuntracefx/menu/handlers.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:D5EC MenuWaitThenAnimate
; WHAT IT DOES: The handler of an object that waits +$1E + 1 frames and
; then only animates: once the count is negative the handler becomes
; MenuActorAnimateOnly ($D5F6), which it runs at once.
; HOW: A memory decrement of +$1E; on the last frame it stores $D5F6 in
; +$08 and falls into MenuActorAnimateOnly. The waiting return is not
; reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A and X
; unchanged; N, Z from the count. Done: as MenuActorAnimateFrame.
%at($0BD5EC)
MenuWaitThenAnimate:
    dec.b !actor_1e,x
    bmi .lD5F1
    rts
.lD5F1:
    lda.w #$D5F6
    sta.b !actor_handler,x

; @routine 0B:D5FA MenuWaitThenSequence
; WHAT IT DOES: The handler of an object that waits +$1E + 1 frames and
; then watches for the button sequence: once the count is negative the
; handler becomes MenuButtonSequence ($D606) with no steps matched, and
; runs at once.
; HOW: A memory decrement of +$1E; on the last frame +$08 = $D606, +$0A =
; 0, then falls into MenuButtonSequence.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: waiting: A and X
; unchanged; N, Z from the count. Done: as MenuButtonSequence.
%at($0BD5FA)
MenuWaitThenSequence:
    dec.b !actor_1e,x
    bmi .lD5FF
    rts
.lD5FF:
    lda.w #$D606
    sta.b !actor_handler,x
    stz.b !actor_0a,x

; @routine 0B:D606 MenuButtonSequence
; WHAT IT DOES: The handler that animates the object and watches the
; buttons newly pressed ($118B) for an eight-step sequence (the masks at
; $0B:D66D; +$0A = steps matched). A press of X ($0800) restarts it and
; clears $1913. A press off the sequence restarts it and is handled as a
; selection (MenuSelectConfirm). The eighth step plays sound $46, turns on
; the colour maths (MenuSequenceColourMath), moves the handler on to
; $D71B and starts animation $152 unless it is already playing; that step
; is not reached in the traces so far.
; HOW: MenuActorAnimateFrame, then a 16-bit test of $118B against the
; step's mask (kept at $11DF); a matched press is consumed (stz $118B).
; $118B is read once more, unused, before the mask is fetched.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: no press: A = 0;
; Z set, N clear; X, C, V as MenuActorAnimateFrame. Off the sequence: as
; MenuSelectConfirm. Matched: A = the steps matched, X = $11F9, P from
; the compare with 8. Eighth step: A = +$14 when it was $152 (P from the
; compare), else A = $152 (N, Z clear, C from the compare); X = $11F9.
%at($0BD606)
MenuButtonSequence:
    jsr.w MenuActorAnimateFrame
    lda.w !menu_joy1_pressed
    beq .lD66C
    and.w #$0800
    beq .lD61C
    stz.b !actor_0a,x
    lda.w #$0000
    sta.l !wram_1913
.lD61C:
    lda.w !menu_joy1_pressed
    lda.b !actor_0a,x
    asl a
    tax
    lda.l !button_sequence,x
    sta.w !scratch_11df
    ldx.w !actor_current
    lda.w !menu_joy1_pressed
    and.w !scratch_11df
    bne .lD63B
    stz.b !actor_0a,x
    jsr.w MenuSelectConfirm
    rts
.lD63B:
    stz.w !menu_joy1_pressed
    lda.b !actor_0a,x
    inc a
    sta.b !actor_0a,x
    cmp.w #$0008
    bne .lD66C
    jsr.w MenuSequenceColourMath
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    lda.w #$D71B
    sta.b !actor_handler,x
    stz.b !actor_0a,x
    lda.b !actor_anim,x
    cmp.w #$0152
    beq .lD66C
    lda.w #$0152
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lD66C:
    rts

; @routine 0B:D67D MenuSequenceColourMath
; WHAT IT DOES: The reward for the menu's button sequence: queues writes
; for the next vertical blank that turn on HDMA channels 4 and 6 and set up
; colour subtraction of a full-intensity fixed colour on BG1-3 and the
; backdrop, through colour window 1, with the BG window masks cleared. Not
; reached in the traces so far.
; HOW: Eight FrameByteWritePush calls: HDMAEN $50, TSW $07, COLDATA $FF,
; CGADSUB $A7, CGWSEL $20, W12SEL 0, W34SEL 0, WOBJSEL $30; X is pushed
; around each.
; Entry: X 16-bit or 8-bit; D any. Exit: A low byte = $30, A high byte
; unchanged; X unchanged; M 16-bit, X 16-bit; N, Z from X; C, V unchanged.
%at($0BD67D)
MenuSequenceColourMath:
    sep.b #$20
    lda.b #$50
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    lda.b #$07
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!TSW
    jsl.l FrameByteWritePush
    plx
    lda.b #$FF
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!COLDATA
    jsl.l FrameByteWritePush
    plx
    lda.b #$A7
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGADSUB
    jsl.l FrameByteWritePush
    plx
    lda.b #$20
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!CGWSEL
    jsl.l FrameByteWritePush
    plx
    lda.b #$00
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!W12SEL
    jsl.l FrameByteWritePush
    plx
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!W34SEL
    jsl.l FrameByteWritePush
    plx
    lda.b #$30
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!WOBJSEL
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    rts

; @routine 0B:CB28 MenuChoiceObject48Start
; WHAT IT DOES: First frame of the choice object for the fourth choice:
; copies its sprite group (MenuActorCopyGroup). While the saved progress
; byte ($70:FC0B) is 0 it shows animation $FA with frame 0 of the frames
; at table offset $48 and becomes MenuChoiceObject48 ($CB74); otherwise
; it becomes MenuChoiceObject38Start ($CB3D), a path not reached in the
; traces so far.
; HOW: The animation is started only when +$14 is not already $FA (+$16
; and +$18 cleared); then $11E3 = $48 and MenuActorUploadFrames with A = 0.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = the new
; handler; N set, Z clear; X = $11F9. Locked: Y = $FFFF; C, V as
; MenuActorUploadFrames. Open: Y unchanged; C, V as MenuActorCopyGroup.
%at($0BCB28)
MenuChoiceObject48Start:
    lda.l !save_progress
    and.w #$00FF
    cmp.w #$0000
    beq MenuChoiceObject48Locked
    jsr.w MenuActorCopyGroup
    lda.w #$CB3D
    sta.b !actor_handler,x
    rts

; @routine 0B:CB3D MenuChoiceObject38Start
; WHAT IT DOES: The open fourth choice's first frame after
; MenuChoiceObject48Start: makes MenuChoiceObject38 ($CB42) the handler
; and runs it. Not reached in the traces so far.
; HOW: +$08 = $CB42, then falls into MenuChoiceObject38.
; Entry: as MenuChoiceObject38. Exit: as MenuChoiceObject38.
%at($0BCB3D)
MenuChoiceObject38Start:
    lda.w #$CB42
    sta.b !actor_handler,x

; @routine 0B:CB42 MenuChoiceObject38
; WHAT IT DOES: As MenuChoiceObject00 (actors.c), the frames starting at
; offset $38: the fourth choice once it is open. Not reached in the traces
; so far.
; HOW: MenuChoiceObjectUpdate, $11E3 = $38, MenuChoiceObjectFrames with A
; = $38.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: as
; MenuChoiceObjectFrames.
%at($0BCB42)
MenuChoiceObject38:
    jsr.w MenuChoiceObjectUpdate
    lda.w #$0038
    sta.w !scratch_11e3
    jsr.w MenuChoiceObjectFrames
    rts

; MenuChoiceObject48Start continued: the fourth choice while it is locked.
; MenuChoiceObject38Start and MenuChoiceObject38 sit between the two parts.
%at($0BCB4F)
MenuChoiceObject48Locked:
    lda.b !actor_anim,x
    cmp.w #$00FA
    beq .lCB5F
    lda.w #$00FA
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.lCB5F:
    jsr.w MenuActorCopyGroup
    lda.w #$0048
    sta.w !scratch_11e3
    lda.w #$0000
    jsr.w MenuActorUploadFrames
    lda.w #$CB74
    sta.b !actor_handler,x
    rts

; @routine 0B:CE8A MenuBg1RiseAway
; WHAT IT DOES: The handler of the BG1 object leaving the screen upwards
; after a selection: its speed (+$22) grows by 1 every second frame, its
; position (+$20) moves by the speed, BG1's vertical scroll follows it
; (-position) and +$06 = position + $4D. Once the position is below -$58
; it plays sound $13 and hands over to MenuDropAway ($CF27). The path for
; a position that is not negative is not reached in the traces so far.
; HOW: MenuActorAnimateFrame first; +$0A toggles between 0 and 1. The
; scroll is queued for the next vertical blank with two
; FrameByteWritePush calls to BG1VOFS, high byte first.
; Entry: M 16-bit, X 16-bit, D = $0000; X = record. Exit: A = +$06; N, Z,
; C, V from the add; M 16-bit, X 16-bit; X as MenuActorAnimateFrame left
; it.
%at($0BCE8A)
MenuBg1RiseAway:
    jsr.w MenuActorAnimateFrame
    lda.b !actor_0a,x
    inc a
    and.w #$0001
    sta.b !actor_0a,x
    bne .lCE9F
    lda.b !actor_22,x
    clc
    adc.w #$0001
    sta.b !actor_22,x
.lCE9F:
    lda.b !actor_20,x
    clc
    adc.b !actor_22,x
    sta.b !actor_20,x
    lda.b !actor_20,x
    bmi .lCEAD
    brl .lCEF7
.lCEAD:
    cmp.w #$FFA8
    bcc .lCEB5
    brl .lCEF7
.lCEB5:
    lda.w #$CF27
    sta.b !actor_handler,x
    sep.b #$20
    lda.b #$13
    jsl.l SoundPort0QueuePushRaw
    rep.b #$20
    sep.b #$20
    rep.b #$20
    lda.w #$0000
    sec
    sbc.b !actor_20,x
    sep.b #$20
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    lda.b !actor_20,x
    clc
    adc.w #$004D
    sta.b !actor_06,x
    rts
.lCEF7:
    lda.w #$0000
    sec
    sbc.b !actor_20,x
    sep.b #$20
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    xba
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!BG1VOFS
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    lda.b !actor_20,x
    clc
    adc.w #$004D
    sta.b !actor_06,x
    rts

; @routine 0B:DF7B MenuActorNoStep
; WHAT IT DOES: Nothing: the frame handler of an object that does nothing
; on its own.
; HOW: RTS.
; Entry: any. Exit: every register and flag unchanged.
%at($0BDF7B)
MenuActorNoStep:
    rts

; @routine 0B:DF7C MenuSelectionFramesCycle
; WHAT IT DOES: Object X's frame: every seventh frame (its count +$0A run
; down from 6) steps its frame +$22 through 0-3 and queues that frame of the
; picture for the selection $18CB to VRAM: $80 bytes from $70:3000 + the
; frame's offset (0, $200, $400, $200; table $0B:DFCB) + the selection's
; ($80, $100, 0, $180; table $0B:DFD3), to VRAM word $15C0.
; HOW: The frame offset is kept at $11DF; the request goes through the
; words at $16B1-$16B7 (data bank) and VramDmaQueueAdd; X is reloaded from
; $11F9.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: counting:
; A unchanged, N, Z from the count; a step: X = $11F9 with N, Z from it,
; A as VramDmaQueueAdd leaves it, C and V from the address add.
%at($0BDF7C)
MenuSelectionFramesCycle:
    dec.b !actor_0a,x
    bpl .counting
    lda.w #$0006
    sta.b !actor_0a,x
    lda.b !actor_22,x
    inc a
    and.w #$0003
    sta.b !actor_22,x
    asl a
    tax
    lda.l !menu_cycle_frame_offsets,x
    sta.w !scratch_11df
    lda.l !selection
    asl a
    tax
    lda.l !menu_cycle_selection_offsets,x
    clc
    adc.w !scratch_11df
    clc
    adc.w #$3000
    rep.b #$30
    sta.w !vram_queue_staged+2
    lda.w #$0080
    sta.w !vram_queue_staged
    lda.w #$0070
    sta.w !vram_queue_staged+4
    lda.w #$15C0
    sta.w !vram_queue_staged+6
    jsl.l VramDmaQueueAdd
    rep.b #$20
    rep.b #$10
    ldx.w !actor_current
.counting:
    rts

; @routine 0B:E0DF MenuActorNoStepE0DF
; WHAT IT DOES: Nothing: as MenuActorNoStep, a second copy.
; HOW: RTS.
; Entry: any. Exit: every register and flag unchanged.
%at($0BE0DF)
MenuActorNoStepE0DF:
    rts
