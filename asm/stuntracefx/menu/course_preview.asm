; course_preview.asm: the menu objects that run the course preview, a race
; frame (RaceFrameStep) drawn each menu frame behind the course select.
; C: src/stuntracefx/menu/course_preview.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 0B:BC9A MenuPreviewIdle
; WHAT IT DOES: The course preview while the menu waits: two fixed colour
; steps down, a race frame shown, $18FD = 1; unless $1905 is set, makes
; $BCD2 the object's handler, repeats the choice (MenuChoiceRepeat), sets
; $1A66 from the previous selection ($18C9, MenuSet1A66) and the pair's
; animations (MenuSetPairAnimations), and shows another race frame.
; HOW: Calls in turn; the object is the current one ($11F9).
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as the last call leaves it.
%at($0BBC9A)
MenuPreviewIdle:
    jsr.w MenuFixedColourDown
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    lda.l !menu_1905
    beq .not_held
    rts
.not_held:
    ldx.w !actor_current
    lda.w #$BCD2
    sta.b !actor_handler,x
    jsr.w MenuChoiceRepeat
    lda.l !selection_prev
    jsr.w MenuSet1A66
    jsr.w MenuSetPairAnimations
    lda.w #$0000
    jsl.l RaceFrameStep
    rts

; @routine 0B:BCF1 MenuPreviewStart
; WHAT IT DOES: The first frame of the course preview's countdown: two
; fixed colour steps down, a race frame shown, $18FD = 1; unless $1905 is
; set, makes MenuPreviewCountdown ($BD55) the object's handler with a count
; of 20 (+$0A), starts animation $D8 on the kind 4 object (unless it has
; it), clears +$10 of groups 4 and 5 (MenuActorsSet10 with Y = 0) and asks
; for HDMA channels 6 and 7 ($C0 to HDMAEN on the frame write list).
; HOW: Calls in turn; the object is the current one ($11F9).
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: as FrameByteWritePush
; leaves it, with X the kind 4 object and M 16-bit.
%at($0BBCF1)
MenuPreviewStart:
    jsr.w MenuFixedColourDown
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    lda.l !menu_1905
    beq .not_held
    rts
.not_held:
    ldx.w !actor_current
    lda.w #$BD55
    sta.b !actor_handler,x
    lda.w #$0014
    sta.b !actor_0a,x
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$00D8
    beq .animated
    lda.w #$00D8
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.animated:
    lda.w #$0004
    ldy.w #$0000
    jsr.w MenuActorsSet10
    lda.w #$0005
    ldy.w #$0000
    jsr.w MenuActorsSet10
    sep.b #$20
    lda.b #$C0
    sep.b #$20
    rep.b #$10
    phx
    ldx.w #!HDMAEN
    jsl.l FrameByteWritePush
    plx
    rep.b #$20
    rts

; @routine 0B:BD55 MenuPreviewCountdown
; WHAT IT DOES: The course preview's countdown: steps the object's count
; (+$0A) down, with sound $3F at 19, 14, 9 and 4, and at 0 starts
; animation $90 on the kind 4 object; then a fixed colour step down, a race
; frame shown and $18FD = 1, and player 2's choice keys
; (MenuChoicePressesP2): with $9000 pressed it ends the menu ($11EB = 1,
; $18FD = 0, sound $46, the object freed); with $4000 it sets groups 4 and
; 5 to $FFFF (MenuActorsSet10), plays sound $47, makes $BE29 the object's
; handler, sets $1905 = 1, $1907 = $28 and $7E:FD65 = $464A, and asks for
; HDMA channels 5, 6 and 7 ($E0).
; HOW: The count compares through 16-bit A; the object is reloaded from
; $11F9 after the kind 4 search. The $4000 case is not reached in the
; traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; X = the object. Exit: as the last
; call leaves it, M 16-bit.
%at($0BBD55)
MenuPreviewCountdown:
    lda.b !actor_0a,x
    beq .frame
    dec a
    sta.b !actor_0a,x
    cmp.w #$0013
    beq .tick_sound
    cmp.w #$000E
    beq .tick_sound
    cmp.w #$0009
    beq .tick_sound
    cmp.w #$0004
    bne .counted
.tick_sound:
    sep.b #$20
    lda.b #$3F
    jsl.l SoundPort0QueuePushRaw
    rep.b #$20
.counted:
    lda.b !actor_0a,x
    bne .frame
    lda.w #$0004
    jsr.w MenuActorFind
    lda.b !actor_anim,x
    cmp.w #$0090
    beq .frame
    lda.w #$0090
    sta.b !actor_anim,x
    stz.b !actor_anim_pos,x
    stz.b !actor_anim_timer,x
.frame:
    ldx.w !actor_current
    jsr.w MenuFixedColourDown
    lda.w #$0000
    jsl.l RaceFrameStep
    lda.w #$0001
    sta.l !wram_18FD
    ldx.w !actor_current
    jsr.w MenuChoicePressesP2
    ldy.w !menu_joy2_pressed
    tya
    and.w #$4000
    bne .keep_preview
    tya
    and.w #$9000
    bne .leave
    rts
.leave:
    lda.w #$0001
    sta.w !menu_exit
    lda.w #$0000
    sta.l !wram_18FD
    sep.b #$20
    lda.b #$46
    jsl.l SoundPort0QueuePush
    rep.b #$20
    ldy.w !actor_current_slot
    jsr.w MenuActorFree
    rts
.keep_preview:
    lda.w #$0004                    ; not reached in the traces so far
    ldy.w #$FFFF                    ; not reached in the traces so far
    jsr.w MenuActorsSet10           ; not reached in the traces so far
    lda.w #$0005                    ; not reached in the traces so far
    ldy.w #$FFFF                    ; not reached in the traces so far
    jsr.w MenuActorsSet10           ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$47                      ; not reached in the traces so far
    jsl.l SoundPort0QueuePush       ; not reached in the traces so far
    rep.b #$20                      ; not reached in the traces so far
    ldx.w !actor_current            ; not reached in the traces so far
    lda.w #$BE29                    ; not reached in the traces so far
    sta.b !actor_handler,x          ; not reached in the traces so far
    lda.w #$0001                    ; not reached in the traces so far
    sta.l !menu_1905_long           ; not reached in the traces so far
    lda.w #$0028                    ; not reached in the traces so far
    sta.l !menu_1907_long           ; not reached in the traces so far
    lda.w #$464A                    ; not reached in the traces so far
    sta.l !palette_word_fd65        ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$E0                      ; not reached in the traces so far
    sep.b #$20                      ; not reached in the traces so far
    rep.b #$10                      ; not reached in the traces so far
    phx                             ; not reached in the traces so far
    ldx.w #!HDMAEN                  ; not reached in the traces so far
    jsl.l FrameByteWritePush        ; not reached in the traces so far
    plx                             ; not reached in the traces so far
    rep.b #$20                      ; not reached in the traces so far
    rts                             ; not reached in the traces so far
