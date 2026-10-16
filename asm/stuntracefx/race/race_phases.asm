; race_phases.asm: the race phase handlers, run each frame for each player
; from the phase tables at $07:E232, $07:E24F and $07:E26E (phase
; !engine_phase_p1 for player 1, !engine_phase_p2 for player 2) and from the
; tables of the phase steps in player_stats.asm. They count the start, the
; race, the finish and the results panels down, through the timer $0E55,
; the sub-state $0D5A and the panel state $0D56. All run with 8-bit
; registers and data bank $07.
; C: src/stuntracefx/race/race_phases.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:E265 PlayerTwoFrameWork
; WHAT IT DOES: Runs the handler for player 2's race phase from the table at
; $07:E26E.
; HOW: SEP #$30, then JMP (table,X); the handler returns to the caller.
; Entry: any widths. Exit: as the handler leaves it.
%at($07E265)
PlayerTwoFrameWork:
    sep.b #$30
    lda.w !engine_phase_p2
    tax
    jmp.w (.table,x)
.table:
    dw PhaseNothing, PhaseNothing, ResultsPanelSteps, $E56A, $EA85, $EBA0, $ECF3, $E972
    dw $E976, $EB4E, $EB49

; @routine 07:E284 PhaseNothing
; WHAT IT DOES: Nothing: the handler of the phases with no work.
; HOW: RTS.
; Entry: any widths. Exit: every register unchanged.
%at($07E284)
PhaseNothing:
    rts

; @routine 07:E285 RaceStartCountdown
; WHAT IT DOES: Starts the race's countdown: clears $0FAE and $0FAF, the
; timer $0E55 = $20, both players' phases 2, the sub-state $0D5A, the time
; bonus ($0D67, $0D69), $19EE, $19F0 and the bonus $0E74; plays sound 5 on
; port 0.
; HOW: 8-bit stores, the clears of words 16-bit.
; Entry: M 8-bit, X 8-bit. Exit: M 16-bit; A = 5, or as
; SoundPort0QueuePushRaw leaves it.
%at($07E285)
RaceStartCountdown:
    stz.w !p1_lead+11
    stz.w !p1_lead+12
    lda.b #$20
    sta.w !wram_0E54+1
    bra RaceStartCountdownStunt_start

; @routine 07:E292 RaceStartCountdownStunt
; WHAT IT DOES: As RaceStartCountdown with the timer $0E55 = $12 and $0FAE,
; $0FAF kept: Stunt Trax's start.
; HOW: LDA #$07 then LDA #$12 (the first load has no use).
; Entry: M 8-bit, X 8-bit. Exit: as RaceStartCountdown.
%at($07E292)
RaceStartCountdownStunt:
    lda.b #$07
    lda.b #$12
    sta.w !wram_0E54+1
RaceStartCountdownStunt_start:
    lda.b #$02
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
    stz.w !wram_0D5A
    rep.b #$20
    stz.w !bonus_digit_state
    stz.w !bonus_digit_size
    stz.w !wram_19EE
    stz.w !wram_19F0
    stz.w !wram_0E74
    lda.w #$0005
    jsl.l SoundPort0QueuePushRaw
    rts

; @routine 07:E2BD RaceReadyShort
; WHAT IT DOES: Plays sound 5 on port 0; the timer $0E55 = $1E; both
; players' phases 2; clears $10CB and $10CC.
; HOW: 8-bit stores.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = 2; N, Z clear.
%at($07E2BD)
RaceReadyShort:
    lda.b #$05
    jsl.l SoundPort0QueuePushRaw
    lda.b #$1E
    sta.w !wram_0E54+1
    bra RaceReadyLong_phases

; @routine 07:E2CA RaceReadyLong
; WHAT IT DOES: As RaceReadyShort with the timer $0E55 = $36.
; HOW: As RaceReadyShort.
; Entry: M 8-bit, X 8-bit. Exit: as RaceReadyShort.
%at($07E2CA)
RaceReadyLong:
    lda.b #$05
    jsl.l SoundPort0QueuePushRaw
    lda.b #$36
    sta.w !wram_0E54+1
RaceReadyLong_phases:
    lda.b #$02
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
    stz.w !piece_counter
    stz.w !piece_counter+1
    rts

; @routine 07:E2E4 RaceStartGlyphsSteps
; WHAT IT DOES: Draws the nine-glyph word (GlyphWordAt1C28), then
; RaceStartSteps.
; HOW: JSR, SEP #$30, then RaceStartSteps' code, which follows.
; Entry: M 8-bit, X 8-bit. Exit: as RaceStartSteps.
%at($07E2E4)
RaceStartGlyphsSteps:
    jsr.w GlyphWordAt1C28
    sep.b #$30

; @routine 07:E2E9 RaceStartSteps
; WHAT IT DOES: Steps the start's sub-state $0D5A, the timer $0E55 counting
; each: 0 waits, then the timer 7; 2 draws the race time at the digit size
; from .sizes (by the timer), and at the end the timer $14, $0D54 = 4,
; $0D5B one less and the time drawn full size (GsuTimeDigitsFull); 4 waits
; (at once while $0D54 is 0), then $0D54 = 8 and the timer 3; 6 waits, then
; clears $0D56 and sets both players' phases 4.
; HOW: JMP (table,X) on $0D5A.
; Entry: M 8-bit, X 8-bit; D = $0000. Exit: as the step leaves it: M
; 16-bit, X 8-bit after a drawing.
%at($07E2E9)
RaceStartSteps:
    ldx.w !wram_0D5A
    jmp.w (.table,x)
.table:
    dw .wait, .grow, .hold, .end
.wait:
    dec.w !wram_0E54+1
    bne .waitDone
    inc.w !wram_0D5A
    inc.w !wram_0D5A
    lda.b #$07
    sta.w !wram_0E54+1
.waitDone:
    rts
.grow:
    dec.w !wram_0E54+1
    bne .growDraw
    inc.w !wram_0D5A
    inc.w !wram_0D5A
    lda.b #$14
    sta.w !wram_0E54+1
    lda.b #$04
    sta.w !wram_0D54
    dec.w !wram_0D5B
    jmp.w .full
.growDraw:
    ldx.w !wram_0E54+1
    rep.b #$20
    lda.w .sizes,x
    and.w #$00FF
    sta.l !fx_draw_rows
    jsl.l GsuBytes200_BFB5BB
    jsr.w GsuTimeDigits
    rts
.sizes:
    db $20, $1C, $18, $14, $10, $0C, $08, $04, $00, $00
.hold:
    lda.w !wram_0D54
    beq .holdNext
    dec.w !wram_0E54+1
    bne .holdDone
.holdNext:
    inc.w !wram_0D5A
    inc.w !wram_0D5A
    lda.b #$08
    sta.w !wram_0D54
    lda.b #$03
    sta.w !wram_0E54+1
.holdDone:
    rts
.end:
    dec.w !wram_0E54+1
    bne .endDone
    stz.w !panel_b_shown+1
    lda.b #$04
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
.endDone:
    rts
.full:
    rep.b #$20
    jsr.w GsuTimeDigitsFull
    rts

; @routine 07:E376 RaceStartTimer
; WHAT IT DOES: Counts the timer $0E55 down: at 0 both players' phases 4 and
; $0D56 cleared; on the way, $0D54 = 2 as it reaches $11 and 8 as it
; reaches 6.
; HOW: DEC A and CMPs on the new value.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = the timer (or 4, 2, 8 as
; stored); N, Z, C from the last compare or load.
%at($07E376)
RaceStartTimer:
    lda.w !wram_0E54+1
    bne .count
    lda.b #$04
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
    stz.w !panel_b_shown+1
    rts
.count:
    dec a
    sta.w !wram_0E54+1
    cmp.b #$1A
    bne .not1A
    rts
.not1A:
    cmp.b #$11
    bne .not11
    lda.b #$02
    sta.w !wram_0D54
.not11:
    cmp.b #$06
    bne .done
    lda.b #$08
    sta.w !wram_0D54
.done:
    rts

; @routine 07:E3A3 RaceStartTimerSteps
; WHAT IT DOES: Counts the timer $0E55 down: at 1 (its last) clears $0D56
; and sets both players' phases 4; otherwise steps the sub-state $0D5A on as
; the timer passes $35, $21 (also stepping $0D65), $19 (with $0D54 = 2) and
; 9.
; HOW: JMP (table,X) on $0D5A with A = the new timer; SEC, SBC #n, BCS. The
; last step is a bare RTS.
; Entry: M 8-bit, X 8-bit. Exit: A low byte = the timer less the step's
; value, or 4; X = $0D5A; C from the subtraction.
%at($07E3A3)
RaceStartTimerSteps:
    lda.w !wram_0E54+1
    dec a
    bne .count
    stz.w !panel_b_shown+1
    lda.b #$04
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
    rts
.count:
    sta.w !wram_0E54+1
    ldx.w !wram_0D5A
    jmp.w (.table,x)
.table:
    dw .at35, .at21, .at19, .at09, .last
.at35:
    sec
    sbc.b #$35
    bcs .done35
    inc.w !wram_0D5A
    inc.w !wram_0D5A
.done35:
    rts
.at21:
    sec
    sbc.b #$21
    bcs .done21
    inc.w !time_bonus_state
    inc.w !time_bonus_state
    inc.w !wram_0D5A
    inc.w !wram_0D5A
.done21:
    rts
    rts                             ; a second RTS no code reaches
.at19:
    sec
    sbc.b #$19
    bcs .done19
    lda.b #$02
    sta.w !wram_0D54
    inc.w !wram_0D5A
    inc.w !wram_0D5A
.done19:
    rts
.at09:
    sec
    sbc.b #$09
    bcs .last
    inc.w !wram_0D5A
    inc.w !wram_0D5A
.last:
    rts

; @routine 07:E404 ResultsPanelSteps
; WHAT IT DOES: Steps the results panel by $0D56, each step after the delay
; $0D5F has run out (PanelDelayCount): 0 starts it (delay $3C, $0D54 = 8);
; 2 waits; 4, 6, 8 show its three lines (sprite bytes $06, $0A, $0E of the
; buffer at $0400, or for the split screen of the two at $0420 and $0520;
; delay $32, sound $49); $A shows the marks ($A2 at bytes $07, $0B, $0F;
; sound $4A), sets $0E5B = 1, clears $0D53, and in mode 1 sets $0E41 = 2
; and $0E3F = $0F; $C plays sound 1 on port 0 (delay $0F); $E sets $0D54 =
; $A; $10, once $0D54 is 0, sets $0D67 = 2 and both players' phases 6.
; HOW: JMP (table,X) on $0D56. The split-screen lines are not reached in the
; traces so far.
; Entry: M 8-bit, X 8-bit; D = $0000. Exit: as the step leaves it.
%at($07E404)
ResultsPanelSteps:
    ldx.w !panel_b_shown+1
    jmp.w (.table,x)
.table:
    dw .start, .wait, .line1, .line2, .line3, .marks, .sound, .hold
    dw .end
.start:
    lda.b #$3C
    sta.w !wram_0D5D+2
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
    lda.b #$08
    sta.w !wram_0D54
    rts
.wait:
    jsr.w PanelDelayCount
    bcs .waitDone
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
    lda.b #$32
    sta.w !wram_0D5D+2
.waitDone:
    rts
.line1:
    jsr.w PanelDelayCount
    bcs .line1Done
    lda.w !wram_0D62
    beq .line1One
    lda.b #$20
    sta.w $0426
    lda.b #$88
    sta.w $0526
    bra .line1Next
.line1One:
    lda.b #$28
    sta.w $0406
.line1Next:
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
    lda.b #$32
    sta.w !wram_0D5D+2
    lda.b #$49
    jsl.l SoundPort0QueuePush
.line1Done:
    rts
.line2:
    jsr.w PanelDelayCount
    bcs .line2Done
    lda.w !wram_0D62
    beq .line2One
    lda.b #$20
    sta.w $042A
    lda.b #$88
    sta.w $052A
    bra .line2Next
.line2One:
    lda.b #$28
    sta.w $040A
.line2Next:
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
    lda.b #$32
    sta.w !wram_0D5D+2
    lda.b #$49
    jsl.l SoundPort0QueuePush
.line2Done:
    rts
.line3:
    jsr.w PanelDelayCount
    bcs .line3Done
    lda.w !wram_0D62
    beq .line3One
    lda.b #$20
    sta.w $042E
    lda.b #$88
    sta.w $052E
    bra .line3Next
.line3One:
    lda.b #$28
    sta.w $040E
.line3Next:
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
    lda.b #$32
    sta.w !wram_0D5D+2
    lda.b #$49
    jsl.l SoundPort0QueuePush
.line3Done:
    rts
.marks:
    jsr.w PanelDelayCount
    bcs .marksDone
    lda.w !wram_0D62
    beq .marksOne
    lda.b #$A2
    sta.w $0427
    sta.w $0527
    sta.w $042B
    sta.w $052B
    sta.w $042F
    sta.w $052F
    bra .marksNext
.marksOne:
    lda.b #$A2
    sta.w $0407
    sta.w $040B
    sta.w $040F
.marksNext:
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
    lda.b #$32
    sta.w !wram_0D5D+2
    lda.b #$4A
    jsl.l SoundPort0QueuePush
    lda.b #$01
    sta.w !wram_0E5B
    stz.w !wram_0D53
    lda.w !hud_mode
    cmp.b #$01
    bne .marksDone
    lda.b #$02
    sta.w !wram_0E41
    lda.b #$0F
    sta.w !wram_0E3F
.marksDone:
    rts
.sound:
    jsr.w PanelDelayCount
    bcs .soundDone
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
    lda.b #$0F
    sta.w !wram_0D5D+2
    lda.b #$01
    jsl.l SoundPort0QueuePushRaw
.soundDone:
    rts
.hold:
    jsr.w PanelDelayCount
    bcs .holdDone
    lda.b #$0A
    sta.w !wram_0D54
    inc.w !panel_b_shown+1
    inc.w !panel_b_shown+1
.holdDone:
    rts
.end:
    lda.w !wram_0D54
    bne .endDone
    lda.b #$02
    sta.w !bonus_digit_state
    lda.b #$06
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
.endDone:
    rts
    rts                             ; a second RTS no code reaches

; @routine 07:E557 PanelDelayCount
; WHAT IT DOES: Counts the panel delay $0D5F down by the frames since the
; last tick; C clear once it has run out.
; HOW: SEC, SBC.
; Entry: M 8-bit. Exit: A low byte = $0D5F; N, Z, C, V from the subtraction.
%at($07E557)
PanelDelayCount:
    lda.w !wram_0D5D+2
    sec
    sbc.w !frames_since_tick
    sta.w !wram_0D5D+2
    rts
