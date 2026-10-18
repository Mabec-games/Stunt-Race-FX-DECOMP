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
    dw PhaseNothing, PhaseNothing, ResultsPanelSteps, RacePhaseLapP2, $EA85, $EBA0, $ECF3, $E972
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
; HOW: JMP (table,X) on $0D5A; the drawing at the timer is
; RaceStartStepsDraw (07:E323).
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

; @routine 07:E323 RaceStartStepsDraw
; WHAT IT DOES: RaceStartSteps' drawing at the timer $0E55: the race time
; drawn (GsuBytes200_BFB5BB, GsuTimeDigits) at the digit size from
; .sizes by the timer.
; HOW: The size byte read as a 16-bit word, ANDed to its low byte, into
; $70:0210. The timer counts down from 7, so the word read at timer 7
; takes the table's ninth byte and its tenth ($07:E343) is never read:
; that byte is left to the raw intake as data, and the sub-state 4 code
; after it is RaceStartStepsHold.
; Entry: M 8-bit, X 8-bit; D = $0000. Exit: M 16-bit, X = the timer; as
; GsuTimeDigits leaves the rest.
%at($07E323)
    ldx.w !wram_0E54+1
    rep.b #$20
    lda.w .sizes,x
    and.w #$00FF
    sta.l !fx_draw_rows
    jsl.l GsuBytes200_BFB5BB
    jsr.w GsuTimeDigits
    rts
.sizes:
    ; a tenth byte ($00 at $07:E343) follows, which nothing reads
    db $20, $1C, $18, $14, $10, $0C, $08, $04, $00

; @routine 07:E344 RaceStartStepsHold
; WHAT IT DOES: RaceStartSteps' sub-state 4, entered from its jump table:
; waits on the timer $0E55 (not at all while $0D54 is 0), then the next
; sub-state with $0D54 = 8 and the timer 3.
; HOW: LDA/BEQ, DEC/BNE, two INCs of $0D5A, byte stores.
; Entry: M 8-bit, X 8-bit; D = $0000. Exit: A low byte = 3, or $0D54 or
; the timer as last loaded; N, Z from it.
%at($07E344)
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

; @routine 07:E3E6 RaceStartTimerStepsRts
; WHAT IT DOES: The second RTS ending RaceStartTimerSteps, which no code
; reaches: returns at once.
; HOW: RTS.
; Entry: any. Exit: everything unchanged.
%at($07E3E6)
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
    sta.w !results_split_y
    lda.b #$88
    sta.w !results_split2_y
    bra .line1Next
.line1One:
    lda.b #$28
    sta.w !results_line_y
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
    sta.w !results_split_y+4
    lda.b #$88
    sta.w !results_split2_y+4
    bra .line2Next
.line2One:
    lda.b #$28
    sta.w !results_line_y+4
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
    sta.w !results_split_y+8
    lda.b #$88
    sta.w !results_split2_y+8
    bra .line3Next
.line3One:
    lda.b #$28
    sta.w !results_line_y+8
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
    sta.w !results_split_tile
    sta.w !results_split2_tile
    sta.w !results_split_tile+4
    sta.w !results_split2_tile+4
    sta.w !results_split_tile+8
    sta.w !results_split2_tile+8
    bra .marksNext
.marksOne:
    lda.b #$A2
    sta.w !results_line_tile
    sta.w !results_line_tile+4
    sta.w !results_line_tile+8
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

; @routine 07:E556 ResultsPanelStepsRts
; WHAT IT DOES: The second RTS ending ResultsPanelSteps, which no code
; reaches: returns at once.
; HOW: RTS.
; Entry: any. Exit: everything unchanged.
%at($07E556)
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

; @routine 07:E562 RacePhaseLapP1
; WHAT IT DOES: Player Y's race phase on the split screen, for player 1:
; Super FX program $07:F441 (GsuF441Player1) and the pause by pad 1
; (OamStagePause); then the race: the time up at 9:59.99, or in modes 0 and 1
; when the time at $0E70 runs out (phase $0C, or for the split screen both
; phases $14; sound $31); the car's crash or fall (bits 12 and 0 of
; $0DEE/$0DEC + Y: phase $10 or $0E, with the split screen
; RaceFinishPhases10 / 0E, else the palette darkened and the sound reset);
; else the banner step $0E41 (+1 for player 2): 0 and $A test a lap crossed
; ($0DE8 + Y above $0DEA + Y: RaceLapCrossed), 4 and 6 grow the lap
; banners, 8 waits.
; HOW: JMP (table,X) on the banner step. The time-up at 9:59.99, the
; split-screen time up and the crash without the split screen are not
; reached in the traces so far.
; Entry: M 8-bit, X 8-bit; Y = the player; D = $0000. Exit: as the path
; leaves it.
%at($07E562)
RacePhaseLapP1:
    jsr.w GsuF441Player1
    jsr.w OamStagePause
    bra RacePhaseLap_race

; @routine 07:E56A RacePhaseLapP2
; WHAT IT DOES: As RacePhaseLapP1 for player 2 (GsuF441Player2,
; OamStagePause2).
; HOW: As RacePhaseLapP1.
; Entry: as RacePhaseLapP1. Exit: as RacePhaseLapP1.
%at($07E56A)
RacePhaseLapP2:
    jsr.w GsuF441Player2
    jsr.w OamStagePause2
    bra RacePhaseLap_race

; @routine 07:E572 RacePhaseLap
; WHAT IT DOES: As RacePhaseLapP1 for one player: the pause on START
; (PauseOnStart), then the race.
; HOW: As RacePhaseLapP1.
; Entry: as RacePhaseLapP1. Exit: as RacePhaseLapP1.
%at($07E572)
RacePhaseLap:
    jsr.w PauseOnStart
RacePhaseLap_race:
    lda.w !race_time_minutes
    cmp.b #$09
    bne .lE58A
    lda.w !race_time_seconds
    cmp.b #$59
    bne .lE58A
    lda.w !race_time_fraction
    cmp.b #$99
    beq .lE59E
.lE58A:
    lda.w !hud_mode
    cmp.b #$02
    beq .lE5CE
    cmp.b #$03
    beq .lE5CE
    rep.b #$20
    lda.w !race_time_bcd
    bne .lE5CE
    sep.b #$20
.lE59E:
    lda.w !wram_0D62
    bne .lE5B4
    lda.b #$20
    sta.w !wram_0E59+1
    lda.b #$0C
    sta.w !engine_phase_p1,y
    lda.b #$31
    jsl.l SoundPort0QueuePush
    rts
.lE5B4:
    lda.b #$20
    sta.w !wram_0E59+1
    sta.w !wram_0E58
    lda.b #$14
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
    stz.w !wram_0D53
    lda.b #$31
    jsl.l SoundPort0QueuePush
    rts
.lE5CE:
    sep.b #$20
    lda.w !player_rec_41,y
    xba
    lda.w !player_rec_40,y
    rep.b #$20
    bit.w #$0001
    beq .lE5E5
    bit.w #$1000
    bne .lE5E8
    bra .lE63C
.lE5E5:
    jmp.w .lE68A
.lE5E8:
    sep.b #$20
    lda.w !wram_0D62
    beq .lE608
    lda.b #$10
    sta.w !engine_phase_p1,y
    lda.b #$0C
    sta.w !wram_0E54+2
    lda.b #$37
    jsl.l SoundPort0QueuePush
    lda.b #$FF
    sta.w !wram_0D53
    jsr.w RaceFinishPhases10
    rts
.lE608:
    rep.b #$20
    lda.w #$FC00
    jsl.l PaletteDarkenFour
    sep.b #$20
    lda.b #$F0
    sta.w !wram_0E16+2
    stz.w !wram_0E5B
    stz.w !wram_0D5B
    jsr.w Countdown0DF8
    lda.b #$10
    sta.w !engine_phase_p1,y
    lda.b #$0E
    sta.w !wram_0E54+2
    sta.w !wram_0E4B
    jsr.w OamHide75Up
    jsl.l SoundReset
    lda.b #$37
    jsl.l SoundPort0QueuePush
    rts
.lE63C:
    sep.b #$20
    lda.w !wram_0D62
    beq .lE656
    lda.b #$0E
    sta.w !engine_phase_p1,y
    lda.b #$0C
    sta.w !wram_0E54+2
    lda.b #$FF
    sta.w !wram_0D53
    jsr.w RaceFinishPhases0E
    rts
.lE656:
    rep.b #$20
    lda.w #$801F
    jsl.l PaletteDarkenFour
    sep.b #$20
    lda.b #$F0
    sta.w !wram_0E16+2
    stz.w !wram_0E5B
    stz.w !wram_0D5B
    jsr.w Countdown0DF8
    lda.b #$0E
    sta.w !engine_phase_p1,y
    lda.b #$0C
    sta.w !wram_0E54+2
    sta.w !wram_0E4B
    jsr.w OamHide75Up
    jsl.l SoundReset
    lda.b #$37
    jsl.l SoundPort0QueuePush
    rts
.lE68A:
    sep.b #$30
    cpy.b #$00
    beq .lE696
    ldx.w !wram_0E42
    jmp.w (.banners,x)
.lE696:
    ldx.w !wram_0E41
    jmp.w (.banners,x)
.banners:
    dw .lE6A8, LapBannerNone, LapBannerGrow, LapBannerGrowLast, LapBannerWait, .lE6A8
.lE6A8:
    sep.b #$20
    lda.w !player_rec_7a,y
    cmp.w !wram_0DEA,y
    beq .lE6B4
    bcs RaceLapCrossed
.lE6B4:
    rts

; @routine 07:E6B5 RaceLapCrossed
; WHAT IT DOES: A lap crossed by player Y: with the override $0D4C the lap's
; split (BCD) kept in the course's save entry when better ($0FA3 = 1, $FF
; worse, 0 equal); else the lap count $0DEA + Y: laps 1 and 2 add the lap's
; time (RaceTimeAddD814 / D815), set the banner $0E41 + Y = 4 or 6 with
; timer $17, sound $48, LapSplit and RecordCompareTime0 / 1; lap 3 is the
; finish: the finishing panel's sprites, LapSplit 6, RaceFinishSounds, phase
; 8, OamHideForHudMode, RaceTimeKeep, $0E6C = the time (at most 99) and the
; special course's record (SpecialRecordInsert, RecordsLoadCourseLong).
; HOW: The code after RacePhaseLap's lap test; also the target of a JMP from
; the time-up phase (07:ED65). The override, the split-screen finish, player
; modes 2 and up and the finishes for both are not reached in the traces so
; far.
; Entry: M 8-bit, X 8-bit; Y = the player; D = $0000. Exit: as the path
; leaves it.
%at($07E6B5)
RaceLapCrossed:
    lda.w !digit62_override
    bne .lE6BD
    brl .lE770
.lE6BD:
    lda.b #$35
    jsl.l SoundPort0QueuePush
    lda.w !wram_0DEA
    inc a
    sta.w !wram_0DEA
    lda.w !race_time_minutes
    sta.b !digit_count
    sed
    lda.w !race_time_fraction
    sec
    sbc.w !lap_mark
    sta.w !lap_splits
    lda.w !race_time_seconds
    sbc.w !wram_0EDC
    bcs .lE6E7
    clc
    adc.b #$60
    dec.b !digit_count
.lE6E7:
    sta.w !lap_splits+1
    lda.b !digit_count
    sec
    sbc.w !wram_0EDD
    sta.w !lap_splits+2
    cld
    lda.w !race_time_fraction
    sta.w !lap_mark
    lda.w !race_time_seconds
    sta.w !wram_0EDC
    lda.w !race_time_minutes
    sta.w !wram_0EDD
    rep.b #$30
    jsl.l RecordOffsetAllCourses
    phx
    lda.l !save_record_b,x
    sta.w !packed_time_a
    rep.b #$20
    lda.w !lap_splits+2
    and.w #$00FF
    tay
    lda.w !lap_splits+1
    and.w #$00FF
    tax
    lda.w !lap_splits
    and.w #$00FF
    jsr.w TimePackBcd
    sta.w !packed_time_b
    plx
    cmp.l !save_record_b,x
    bcs .lE758
    sta.l !save_record_b,x
    phx
    jsl.l HudBestTimeDigits
    plx
    lda.w !level_columns+12
    sta.l !save_record_b+2,x
    sep.b #$20
    lda.w !wram_0D89
    sta.l !save_record_b+4,x
    jsr.w SaveSumsRecordsB
    lda.b #$01
    bra .lE764
.lE758:
    beq .lE760
    sep.b #$20
    lda.b #$FF
    bra .lE764
.lE760:
    sep.b #$20
    lda.b #$00
.lE764:
    sta.w !p1_lead
    rep.b #$20
    lda.w #$012C
    sta.w !hud_hold
    rts
.lE770:
    lda.w !wram_0DEA,y
    inc a
    sta.w !wram_0DEA,y
    cmp.b #$02
    beq .lE7B2
    bcs .lE7DC
    cmp.b #$01
    beq .lE788
    lda.b #$48
    jsl.l SoundPort0QueuePush
    rts
.lE788:
    jsl.l RaceTimeAddD814
    rep.b #$20
    lda.w #$012C
    sta.w !hud_hold
    sep.b #$20
    lda.b #$04
    sta.w !wram_0E41,y
    lda.b #$17
    sta.w !wram_0E3F,y
    lda.b #$48
    jsl.l SoundPort0QueuePush
    sep.b #$30
    ldx.b #$00
    jsr.w LapSplit
    jsl.l RecordCompareTime0
    rts
.lE7B2:
    jsl.l RaceTimeAddD815
    rep.b #$20
    lda.w #$012C
    sta.w !hud_hold
    sep.b #$20
    lda.b #$48
    jsl.l SoundPort0QueuePush
    lda.b #$06
    sta.w !wram_0E41,y
    lda.b #$17
    sta.w !wram_0E3F,y
    sep.b #$30
    ldx.b #$03
    jsr.w LapSplit
    jsl.l RecordCompareTime1
    rts
.lE7DC:
    sep.b #$20
    stz.w !wram_0E5B
    dec.w !wram_0E57
    dec.w !wram_0E4B
    lda.w !wram_0D62
    beq .lE7F5
    lda.b #$FF
    sta.w !wram_0D53
    jsr.w RaceFinishSplit
    rts
.lE7F5:
    lda.w !hud_mode
    beq .lE7FD
    brl .lE8C4
.lE7FD:
    lda.w !player_mode
    cmp.b #$04
    bcc .lE81E
    stz.w !wram_0D5B
    lda.b #$23
    sta.w !wram_0E58
    lda.b #$0A
    sta.w !engine_phase_p1,y
    jsr.w OamHideForHudMode
    jsr.w Countdown0DF8
    lda.b #$51
    jsl.l SoundPort0QueuePush
    rts
.lE81E:
    sep.b #$20
    lda.w !wram_0D24
    cmp.b #$03
    bne .lE836
    lda.b #$08
    sta.w !engine_phase_p1
    jsr.w CarFinishSound
    lda.b #$04
    jsl.l SoundPort1QueuePush
    rts
.lE836:
    rep.b #$20
    lda.w !player_mode
    and.w #$00FF
    cmp.w #$0002
    bcc .lE86D
    beq .lE859
    lda.w #$800C
    sta.w !glyph_spr_glyph
    lda.w #$840C
    sta.w !glyph_spr_glyph+2
    lda.w #$8018
    sta.w !glyph_spr_glyph+4
    bra .lE87F
.lE859:
    lda.w #$8006
    sta.w !glyph_spr_glyph
    lda.w #$8406
    sta.w !glyph_spr_glyph+2
    lda.w #$8018
    sta.w !glyph_spr_glyph+4
    bra .lE87F
.lE86D:
    lda.w #$8000
    sta.w !glyph_spr_glyph
    lda.w #$8400
    sta.w !glyph_spr_glyph+2
    lda.w #$8012
    sta.w !glyph_spr_glyph+4
.lE87F:
    lda.w #$0021
    sta.w !glyph_spr_1040
    lda.w #$001D
    sta.w !glyph_spr_1040+2
    lda.w #$0019
    sta.w !glyph_spr_1040+4
    lda.w #$0008
    sta.w !glyph_spr_1040+48
    lda.w #$001C
    sta.w !glyph_spr_1040+50
    lda.w #$002F
    sta.w !wram_1073+1
    lda.w #$00D7
    sta.w !glyph_spr_x
    lda.w #$00D3
    sta.w !glyph_spr_x+2
    lda.w #$00CF
    sta.w !glyph_spr_x+4
    sep.b #$20
    lda.w !wram_0D48
    cmp.b #$03
    bne .lE8C4
    jsl.l GlyphSpritesInit
    bra .lE8E2
.lE8C4:
    rep.b #$20
    lda.w #$FFFF
    sta.w !hud_hold
    sep.b #$20
    lda.w !wram_10CD
    beq .lE8E2
    lda.b #$08
    sta.w !engine_phase_p1
    jsr.w CarFinishSound
    lda.b #$04
    jsl.l SoundPort1QueuePush
    rts
.lE8E2:
    sep.b #$20
    lda.b #$08
    sta.w !wram_0E41,y
    sep.b #$30
    ldx.b #$06
    jsr.w LapSplit
    jsl.l RaceFinishSounds
    sep.b #$20
    lda.b #$14
    sta.w !wram_0E3F,y
    lda.b #$08
    sta.w !engine_phase_p1,y
    stz.w !panel_b_shown
    lda.b #$70
    sta.w !wram_0E58
    jsr.w OamHideForHudMode
    jsr.w RaceTimeKeep
    rep.b #$30
    lda.w !race_time_bcd
    cmp.w #$0099
    bcc .lE91B
    lda.w #$0099
.lE91B:
    sta.w !wram_0E6C
    lda.w !race_time_fraction
    ldx.w !race_time_seconds
    ldy.w !race_time_minutes
    cpy.w #$0008
    bcs .lE936
    jsr.w TimePackBcd
    jsr.w SpecialRecordInsert
    jsl.l RecordsLoadCourseLong
.lE936:
    rts

; @routine 07:E937 CarFinishSoundP2
; WHAT IT DOES: As CarFinishSound for player 2: sets the current player
; !current_player = 1, then the sound for its car ($0D8A).
; HOW: Enters CarFinishSound's compare. Not reached in the traces so far.
; Entry: any widths. Exit: as CarFinishSound.
%at($07E937)
CarFinishSoundP2:
    php
    sep.b #$30
    lda.b #$01
    sta.w !current_player
    lda.w !wram_0D8A
    bra CarFinishSound_car

; @routine 07:E944 CarFinishSound
; WHAT IT DOES: Plays the finish's sound for player 1's car (!wram_0D89): 8,
; or $15 for car 4 or more.
; HOW: PHP, SEP #$30; PLP at the end. Car 4 and up is not reached in the
; traces so far.
; Entry: any widths. Exit: P unchanged; A low byte = the sound, or as
; SoundPort0QueuePush leaves it.
%at($07E944)
CarFinishSound:
    php
    sep.b #$30
    lda.w !wram_0D89
CarFinishSound_car:
    cmp.b #$04
    bcc .first
    lda.b #$15
    bra .play
.first:
    lda.b #$08
.play:
    jsl.l SoundPort0QueuePush
    plp
    rts

; The lap banners' sizes (LapBannerGrow), by the banner's timer.
lap_banner_sizes:
    db $02, $04, $08, $0C, $0E, $10, $12, $14, $18, $18, $18, $18, $18, $18, $17, $16
    db $14, $12, $10, $0E, $0C, $08, $04, $02

; @routine 07:E9C0 LapBannerNone
; WHAT IT DOES: Nothing: banner 2 of RacePhaseLap's banners.
; HOW: RTS. Not reached in the traces so far.
; Entry: any widths. Exit: every register unchanged.
%at($07E9C0)
LapBannerNone:
    rts

; @routine 07:E9C1 LapBannerGrow
; WHAT IT DOES: Shows player Y's lap banner at the size its timer $0E3F + Y
; picks from lap_banner_sizes (GsuF0A5OrF0B4), and counts the timer down;
; at 0 clears the banner $0E41 + Y.
; HOW: The size masked 16-bit; SEP #$30 after.
; Entry: M 8-bit, X 8-bit; Y = the player. Exit: M 8-bit, X 8-bit; A low
; byte = the timer; X = the old timer.
%at($07E9C1)
LapBannerGrow:
    lda.w !wram_0E3F,y
    tax
    lda.w lap_banner_sizes,x
    rep.b #$20
    and.w #$00FF
    jsr.w GsuF0A5OrF0B4
    sep.b #$30
    lda.w !wram_0E3F,y
    dec a
    bne .keep
    lda.b #$00
    sta.w !wram_0E41,y
.keep:
    sta.w !wram_0E3F,y
    rts

; @routine 07:E9E1 LapBannerGrowLast
; WHAT IT DOES: As LapBannerGrow with GsuEEF5OrEFCD (the last lap's banner).
; HOW: As LapBannerGrow.
; Entry: as LapBannerGrow. Exit: as LapBannerGrow.
%at($07E9E1)
LapBannerGrowLast:
    lda.w !wram_0E3F,y
    tax
    lda.w lap_banner_sizes,x
    rep.b #$20
    and.w #$00FF
    jsr.w GsuEEF5OrEFCD
    sep.b #$30
    lda.w !wram_0E3F,y
    dec a
    bne .keep
    lda.b #$00
    sta.w !wram_0E41,y
.keep:
    sta.w !wram_0E3F,y
    rts

; @routine 07:EA01 LapBannerWait
; WHAT IT DOES: Counts player Y's banner timer $0E3F + Y down; at 0 clears
; the banner, sets $0E57 = $FF and plays sound 3 on port 0.
; HOW: DEC A. Not reached in the traces so far.
; Entry: M 8-bit, X 8-bit; Y = the player. Exit: A low byte = the timer.
%at($07EA01)
LapBannerWait:
    lda.w !wram_0E3F,y
    dec a
    bne .keep
    sta.w !wram_0E41,y
    lda.b #$FF
    sta.w !wram_0E57
    lda.b #$03
    jsl.l SoundPort0QueuePushRaw
    lda.b #$00
.keep:
    sta.w !wram_0E3F,y
    rts

; @routine 07:D672 RaceFinishSounds
; WHAT IT DOES: The sounds of a race's finish: by mode !hud_mode, 0:
; $0D5E = the player mode, sound 2 on port 0 for player mode 1 (then $48 on
; port 0 after the car's sound) or 3 for the others; 2: PlayerSound35;
; others: compares record time 2 with the best (RecordTimeCompare), sound 2
; on port 0 when it is worse ($0FA3 = 1), else 3; then the car's sound
; (CarFinishSound).
; HOW: PHP, REP #$30; PLP, RTL. Only mode 0 with player mode 1 is reached in
; the traces so far.
; Entry: any widths. Exit: P unchanged; A as the last sound call leaves it.
%at($07D672)
RaceFinishSounds:
    php
    rep.b #$30
    lda.w !hud_mode
    beq .mode0
    cmp.w #$0002
    beq .mode2
    ldx.w #$0006
    jsr.w RecordTimeCompare
    sep.b #$20
    lda.w !p1_lead
    cmp.b #$01
    bne .better
    lda.b #$02
    jsl.l SoundPort0QueuePushRaw
    bra .car
.better:
    lda.b #$03
    jsl.l SoundPort0QueuePushRaw
.car:
    jsr.w CarFinishSound
    plp
    rtl
.mode2:
    jsr.w PlayerSound35
    jsr.w CarFinishSound
    plp
    rtl
.mode0:
    sep.b #$20
    lda.w !player_mode
    sta.w !wram_0D5D+1
    cmp.b #$01
    beq .one
    lda.b #$03
    jsl.l SoundPort0QueuePushRaw
    jsr.w CarFinishSound
    plp
    rtl
.one:
    lda.b #$02
    jsl.l SoundPort0QueuePushRaw
    jsr.w CarFinishSound
    lda.b #$48
    jsl.l SoundPort0QueuePush
    plp
    rtl

; @routine 07:E972 PhaseGlyphsFive
; WHAT IT DOES: Draws five glyphs with Super FX program $07:F15B
; (GsuF15BFive): a phase's whole work.
; HOW: JSR, RTS.
; Entry: D = $0000. Exit: as GsuF15BFive leaves it.
%at($07E972)
PhaseGlyphsFive:
    jsr.w GsuF15BFive
    rts

; @routine 07:E976 PhaseGlyphsSix
; WHAT IT DOES: Draws six HUD glyphs (HudSixGlyphs): a phase's whole work.
; HOW: JSR, RTS.
; Entry: D = $0000. Exit: as HudSixGlyphs leaves it.
%at($07E976)
PhaseGlyphsSix:
    jsr.w HudSixGlyphs
    rts

; @routine 07:E97A RaceEndWaitRow6B
; WHAT IT DOES: Draws with BigSpritesRow6B each frame until the slide out reaches
; $1F; then $0E5F cleared, $0D22 = the slide, $0D23 = 6, $0E56 = $78, $0D55
; cleared and the race's end (RaceEndCountdownFade).
; HOW: An 8-bit compare of the long byte. Reached only by unit_race_phases.
; Entry: M 8-bit, X 8-bit. Exit: as BigSpritesRow6B leaves it, or does not return.
%at($07E97A)
RaceEndWaitRow6B:
    lda.l !slide_out
    cmp.b #$1F
    bcc .draw
    stz.w !palette_animate
    sta.w !wram_0D22
    lda.b #$06
    sta.w !wram_0D23
    lda.b #$78
    sta.w !wram_0E54+2
    stz.w !panel_b_shown
    jsr.w RaceEndCountdownFade

; @routine 07:E998 RaceEndWaitRow6BRts
; WHAT IT DOES: The RTS after RaceEndWaitRow6B's call of
; RaceEndCountdownFade, which never returns: returns at once.
; HOW: RTS.
; Entry: any. Exit: everything unchanged.
%at($07E998)
    rts
.draw:
    jsr.w BigSpritesRow6B
    rts

; @routine 07:E99D RaceEndWaitBigSprites
; WHAT IT DOES: As RaceEndWaitRow6B, drawing the five big sprites
; (BigSpritesRow5) each frame.
; HOW: As RaceEndWaitRow6B. The race's end is not reached in the traces so
; far.
; Entry: M 8-bit, X 8-bit. Exit: as BigSpritesRow5 leaves it, or does not
; return.
%at($07E99D)
RaceEndWaitBigSprites:
    lda.l !slide_out
    cmp.b #$1F
    bcc .draw
    stz.w !palette_animate
    sta.w !wram_0D22
    lda.b #$06
    sta.w !wram_0D23
    lda.b #$78
    sta.w !wram_0E54+2
    stz.w !panel_b_shown
    jsr.w RaceEndCountdownFade

; @routine 07:E9BB RaceEndWaitBigSpritesRts
; WHAT IT DOES: The RTS after RaceEndWaitBigSprites' call of
; RaceEndCountdownFade, which never returns: returns at once.
; HOW: RTS.
; Entry: any. Exit: everything unchanged.
%at($07E9BB)
    rts
.draw:
    jsr.w BigSpritesRow5
    rts

; @routine 07:EA1B ResultsWindowWait
; WHAT IT DOES: The results' window: with $10CD set, the soft reset
; (SoftResetToBootWarm) once the fade is done ($0D61 negative); else counts
; $0E58 down: at $10, or below $58 on B or START (unless the split window is
; moving), slides the window up; at 0, once the window is closed, runs the
; overlay frames and goes on to the race again (RaceRestart, with $0DD4) or
; the video reset (RaceEndVideoReset).
; HOW: 8-bit tests; the presses 16-bit. Reached only by unit_race_phases.
; Entry: M 8-bit, X 8-bit. Exit: as the path leaves it, or does not return.
%at($07EA1B)
ResultsWindowWait:
    lda.w !wram_10CD
    beq .lEA28
    lda.w !fade_level_hi
    bpl .lEA63
    jmp.w SoftResetToBootWarm
.lEA28:
    lda.w !wram_0E58
    beq .lEA6A
    cmp.b #$10
    bne .lEA3F
    lda.b #$01
    sta.w !wram_0D22
    stz.w !wram_0D23
    jsl.l WindowSlideUpSound
    bra .lEA64
.lEA3F:
    cmp.b #$58
    bcs .lEA64
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lEA64
    sep.b #$20
    lda.w !window_hdma_off
    bne .lEA63
    stz.w !wram_0E58
    lda.b #$01
    sta.w !wram_0D22
    stz.w !wram_0D23
    jsl.l WindowSlideUpSound
.lEA63:
    rts
.lEA64:
    sep.b #$20
    dec.w !wram_0E58
    rts
.lEA6A:
    lda.w !window_hdma_off
    cmp.b #$05
    bne .lEA84
    stz.w !wram_0D59
    jsr.w OverlayFramesUntilFaded
    sep.b #$20
    lda.w !sound_port1_last+2
    beq .lEA81
    jmp.w RaceRestart
.lEA81:
    jmp.w RaceEndVideoReset
.lEA84:
    rts

; @routine 07:EA85 RaceEndWindowSteps
; WHAT IT DOES: The race's end window: with $0D48 = 3 the glyph sprites grow
; (GlyphSpritesGrowAndTimes); else counts $0E58 down: at $10 slides the
; window up ($19DF cleared), at $58 sets $0D65 = $A, below $58 slides the
; three glyphs (GlyphSlideThree), at $2B with $0D65 set sets it to $10; once
; the window is closed, the overlay frames, then: with $0D24 = 3 or $0D48
; other than 1 ($0D48 one more) the race again (RaceRestart); with $0D48 = 1
; the override $0D4C = $FF, the course $0D4A + $14, car 4 and the race
; again.
; HOW: SEP #$30; A kept on the stack around GlyphSlideThree.
; Entry: any widths. Exit: as the path leaves it, or does not return.
%at($07EA85)
RaceEndWindowSteps:
    sep.b #$30
    lda.w !wram_0D48
    cmp.b #$03
    bne .lEA93
    jsl.l GlyphSpritesGrowAndTimes
    rts
.lEA93:
    lda.w !window_hdma_off
    cmp.b #$05
    beq .lEADD
    lda.w !wram_0E58
    cmp.b #$10
    bne .lEAB3
    stz.w !wram_19DF
    lda.b #$01
    sta.w !wram_0D22
    stz.w !wram_0D23
    jsl.l WindowSlideUpSound
    lda.w !wram_0E58
.lEAB3:
    cmp.b #$58
    bne .lEAC1
    lda.b #$0A
    sta.w !time_bonus_state
    lda.w !wram_0E58
    bra .lEAC3
.lEAC1:
    bcs .lEAC8
.lEAC3:
    pha
    jsr.w GlyphSlideThree
    pla
.lEAC8:
    cmp.b #$2B
    bne .lEAD9
    lda.w !time_bonus_state
    beq .lEAD9
    lda.b #$10
    sta.w !time_bonus_state
    lda.w !wram_0E58
.lEAD9:
    dec.w !wram_0E58
    rts
.lEADD:
    stz.w !wram_0D59
    jsr.w OverlayFramesUntilFaded
    sep.b #$20
    lda.w !wram_0D24
    cmp.b #$03
    beq .lEB0C
    lda.w !wram_0D48
    cmp.b #$01
    bne .lEB09
    lda.b #$FF
    sta.w !digit62_override
    lda.w !wram_0D4A
    clc
    adc.b #$14
    sta.w !course
    lda.b #$04
    sta.w !wram_0D89
    jmp.w RaceRestart
.lEB09:
    inc.w !wram_0D48
.lEB0C:
    jmp.w RaceRestart

; @routine 07:EB0F RaceEndWindowStepsRts
; WHAT IT DOES: The RTS after RaceEndWindowSteps' last JMP, which no code
; reaches: returns at once.
; HOW: RTS.
; Entry: any. Exit: everything unchanged.
%at($07EB0F)
    rts

; @routine 07:EB10 PauseResultsP2
; WHAT IT DOES: The paused results for player 2: three glyphs (GsuF15BThree);
; counts $0E58 down, at its end sets $0D2E = 1, slides the window up and sets
; $0D2F and the delay $0D5F to 8; then counts the delay down, and at its end
; sets $0D2F = $0D2D = $A and runs pad 2's pause loop (in OamStagePause2).
; HOW: 8-bit; the pause loop is a jump that leaves through the caller's
; frame.
; Entry: D = $0000. Exit: as the path leaves it.
%at($07EB10)
PauseResultsP2:
    jsr.w GsuF15BThree
    sep.b #$20
    lda.w !wram_0E58
    beq .lEB31
    dec.w !wram_0E58
    bne .lEB30
    lda.b #$01
    sta.w !wram_0D2E
    jsl.l WindowSlideUpSound
    lda.b #$08
    sta.w !wram_0D2F
    sta.w !wram_0D5D+2
.lEB30:
    rts
.lEB31:
    lda.w !wram_0D5D+2
    beq .lEB3A
    dec.w !wram_0D5D+2
    rts
.lEB3A:
    lda.b #$0A
    sta.w !wram_0D2F
    sta.w !wram_0D2D
    jmp.w OamStagePause2_lBB7C

; @routine 07:EB45 PhaseGlyphsFourB
; WHAT IT DOES: Draws with GsuF15BFourB: a phase's whole work.
; HOW: JSR, RTS. Reached only by unit_race_phases.
; Entry: D = $0000. Exit: as GsuF15BFourB leaves it.
%at($07EB45)
PhaseGlyphsFourB:
    jsr.w GsuF15BFourB
    rts

; @routine 07:EB49 PauseResultsFourB
; WHAT IT DOES: The paused results (see PauseResults), drawing with GsuF15BFourB.
; HOW: JSR, then PauseResults' code. Reached only by unit_race_phases and unit_entries_07.
; Entry: D = $0000. Exit: as the path leaves it.
%at($07EB49)
PauseResultsFourB:
    jsr.w GsuF15BFourB
    bra PauseResults_lEB51

; @routine 07:EB4E PauseResults
; WHAT IT DOES: The paused results for player 1: three glyphs (GsuF15BThree);
; counts $0E58 down, at its end sound $51, $0D2E cleared, the window slid
; up, $0D2F = 8 and the delay $0D5F = $14; then counts the delay down; at its
; end sound $30 on port 0 when the time is 9:59, $0D2F = $0D2D = $A and pad
; 1's pause loop (in OamStagePause).
; HOW: 8-bit; the pause loop is a long jump that leaves through the caller's
; frame. The 9:59 sound is not reached in the traces so far.
; Entry: D = $0000. Exit: as the path leaves it.
%at($07EB4E)
PauseResults:
    jsr.w GsuF15BThree
.lEB51:
    sep.b #$20
    lda.w !wram_0E58
    beq .lEB77
    dec.w !wram_0E58
    bne .lEB76
    lda.b #$51
    jsl.l SoundPort0QueuePush
    lda.b #$00
    sta.w !wram_0D2E
    jsl.l WindowSlideUpSound
    lda.b #$08
    sta.w !wram_0D2F
    lda.b #$14
    sta.w !wram_0D5D+2
.lEB76:
    rts
.lEB77:
    lda.w !wram_0D5D+2
    beq .lEB80
    dec.w !wram_0D5D+2
    rts
.lEB80:
    lda.w !race_time_minutes
    cmp.b #$09
    bne .lEB94
    lda.w !race_time_seconds
    cmp.b #$59
    bne .lEB94
    lda.b #$30
    jsl.l SoundPort0QueuePushRaw
.lEB94:
    lda.b #$0A
    sta.w !wram_0D2F
    sta.w !wram_0D2D
    jml.l OamStagePause_lBB16

; @routine 07:EBA0 PhaseGlyphsFourA
; WHAT IT DOES: Draws with GsuF15BFourA: a phase's whole work.
; HOW: JSR, RTS. Reached only by unit_race_phases and unit_entries_07.
; Entry: D = $0000. Exit: as GsuF15BFourA leaves it.
%at($07EBA0)
PhaseGlyphsFourA:
    jsr.w GsuF15BFourA
    rts

; @routine 07:EBA4 RaceEndWindowNine
; WHAT IT DOES: Draws with BigSpritesNine; counts $0E58 down: at 1, or below $28 on B
; or START, clears it, sets $19DF = 1 and slides the window up; at 0, once
; the window is closed, leaves the race (RaceEndLeave).
; HOW: 8-bit; the presses 16-bit. Reached only by unit_race_phases.
; Entry: D = $0000. Exit: as the path leaves it, or does not return.
%at($07EBA4)
RaceEndWindowNine:
    jsr.w BigSpritesNine
    sep.b #$20
    lda.w !wram_0E58
    beq .lEBD5
    cmp.b #$01
    beq .lEBC2
    cmp.b #$28
    bcs .lEBCF
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$9000
    beq .lEBCF
    sep.b #$20
.lEBC2:
    stz.w !wram_0E58
    lda.b #$01
    sta.w !wram_19DF
    jsl.l WindowSlideUpSound
    rts
.lEBCF:
    sep.b #$20
    dec.w !wram_0E58
    rts
.lEBD5:
    lda.w !window_hdma_off
    cmp.b #$05
    beq RaceEndLeave

; @routine 07:EBDC PhaseNothingEBDC
; WHAT IT DOES: Nothing: a phase handler with no work.
; HOW: RTS (RaceEndWindowNine's last).
; Entry: any widths. Exit: every register unchanged.
%at($07EBDC)
PhaseNothingEBDC:
    rts

; @routine 07:EBDD RaceEndLeave
; WHAT IT DOES: Leaves the race: with $0E5C set, the championship's closing
; screen (ChampionScreenRun) and the restart (SoftResetToBootWarm); else on
; in BootWarm (RaceEndToAttract).
; HOW: A long branch to RaceEndToAttract's code. The championship's screen
; is not reached in the traces so far.
; Entry: M 8-bit. Exit: never returns.
%at($07EBDD)
RaceEndLeave:
    lda.w !wram_0E5C
    bne .lEBE5
    brl RaceEndToAttract
.lEBE5:
    jsl.l ChampionScreenRun

; @routine 07:EBE9 SoftResetToBootWarm
; WHAT IT DOES: Restarts the game after the race: the screen blanked, the NMI,
; DMA and HDMA off; the sound driver's data reloaded (SoundDataLoad of $F3B5,
; $1E) and blocks $1B ($85F8) and $19 ($9A98) uploaded, each after command 5
; on APU port 1 and a delay; SoundReset, 1 to APU port 0; then BootWarm.
; HOW: Delay loops of $4000 counts of A; P saved around each step.
; Entry: any widths. Exit: never returns.
%at($07EBE9)
SoftResetToBootWarm:
    sei
    sep.b #$20
    lda.b #$80
    sta.w !INIDISP
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEC07:
    dec a
    bne .lEC07
    lda.w #$F3B5
    ldy.w #$001E
    jsl.l SoundDataLoad
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEC22:
    dec a
    bne .lEC22
    lda.w #$001B
    ldy.w #$85F8
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lEC34:
    dec a
    bne .lEC34
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lEC45:
    dec a
    bne .lEC45
    lda.w #$0019
    ldy.w #$9A98
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lEC57:
    dec a
    bne .lEC57
    plp
    jsl.l SoundReset
    sep.b #$20
    lda.b #$01
    sta.w !APUIO0
    cli
    jml.l BootWarm

; @routine 07:EC6B RaceRestart
; WHAT IT DOES: Runs the race again: the screen blanked, the NMI, DMA and HDMA
; off, then on in BootWarm before its race (the stack reset, RaceLoop).
; HOW: A long jump into BootWarm.
; Entry: any widths. Exit: never returns.
%at($07EC6B)
RaceRestart:
    sei
    sep.b #$20
    lda.b #$80
    sta.w !INIDISP
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    jml.l BootWarm_race

; @routine 07:EC80 RaceEndVideoReset
; WHAT IT DOES: HDMA off, the screen blanked, the video reset (RaceVideoInit),
; then RaceEndToAttract.
; HOW: SEP #$20, SEI; the code of RaceEndToAttract follows.
; Entry: any widths. Exit: never returns.
%at($07EC80)
RaceEndVideoReset:
    sep.b #$20
    stz.w !HDMAEN
    lda.b #$80
    sta.w !INIDISP
    sei
    jsl.l RaceVideoInit

; @routine 07:EC8F RaceEndToAttract
; WHAT IT DOES: Leaves the race: the screen blanked, the NMI, DMA and HDMA
; off, then on in BootWarm after its race (the stack reset and the attract
; race).
; HOW: As RaceRestart.
; Entry: any widths. Exit: never returns.
%at($07EC8F)
RaceEndToAttract:
    sei
    sep.b #$20
    lda.b #$80
    sta.w !INIDISP
    stz.w !NMITIMEN
    stz.w !MDMAEN
    stz.w !HDMAEN
    jml.l BootWarm_attract

; @routine 07:ECA4 TimeUpBattle
; WHAT IT DOES: The time up of a battle: the six big sprites (BigSpritesRow6);
; the first time sound $51, OamHide75Up and both timers $0E59, $0E5A = $3C;
; counts $0E5A down; once $0E59 is set, at the end of $0E5A the save's bad
; flag = $11, $0D22 = $0D23 = 6, the timer $3C, $0D55 cleared, sound $30 and
; the race's end (RaceEndCountdown).
; HOW: 8-bit; the second part counts $0E5A down twice a frame. The second
; part is reached only by unit_race_phases.
; Entry: D = $0000. Exit: as the path leaves it, or does not return.
%at($07ECA4)
TimeUpBattle:
    jsr.w BigSpritesRow6
    sep.b #$20
    lda.w !wram_0E59
    bne .lECC8
    lda.w !wram_0E59+1
    bne .lECC4
    lda.b #$51
    jsl.l SoundPort0QueuePush
    jsr.w OamHide75Up
    lda.b #$3C
    sta.w !wram_0E59+1
    sta.w !wram_0E59
.lECC4:
    dec.w !wram_0E59+1
    rts
.lECC8:
    sep.b #$20
    dec.w !wram_0E59+1
    bne .lECF0
    pha
    lda.b #$11
    sta.l !save_bad_flag
    pla
    lda.b #$06
    sta.w !wram_0D22
    sta.w !wram_0D23
    lda.b #$3C
    sta.w !wram_0E59+1
    stz.w !panel_b_shown
    lda.b #$30
    jsl.l SoundPort0QueuePushRaw
    jmp.w RaceEndCountdown
.lECF0:
    dec.w !wram_0E59+1

; @routine 07:ECF3 PhaseNothingECF3
; WHAT IT DOES: Nothing: a phase handler with no work.
; HOW: RTS (TimeUpBattle's last).
; Entry: any widths. Exit: every register unchanged.
%at($07ECF3)
PhaseNothingECF3:
    rts

; @routine 07:ECF4 TimeUpStunt
; WHAT IT DOES: Stunt Trax's time up: the six big sprites (BigSpritesRow6);
; while $0E59 is 0: with $0DDA at $10 or more, phase 6 when the time $0E70
; is not 0, else counts $0E5A down; then $0E5A = $20, $0E59 one less, sound
; $51 and OamHide75Up. Once $0E59 is set: at $0E5A = 0, $0D22 = $0D23 = 6,
; the timer $78, $0D55 cleared, sound $30 and the race's end
; (RaceEndCountdown); else counts $0E5A down.
; HOW: 8-bit. Phase 6 is reached only by unit_race_phases.
; Entry: D = $0000. Exit: as the path leaves it, or does not return.
%at($07ECF4)
TimeUpStunt:
    jsr.w BigSpritesRow6
    sep.b #$20
    lda.w !wram_0E59
    bne .lED28
    lda.w !wram_0DDA
    cmp.b #$10
    bcc .lED10
    lda.w !race_time_bcd
    bne .lED22
    dec.w !wram_0E59+1
    beq .lED10
    rts
.lED10:
    lda.b #$20
    sta.w !wram_0E59+1
    dec.w !wram_0E59
    lda.b #$51
    jsl.l SoundPort0QueuePush
    jsr.w OamHide75Up
    rts
.lED22:
    lda.b #$06
    sta.w !engine_phase_p1
    rts
.lED28:
    sep.b #$20
    lda.w !wram_0E59+1
    bne .lED48
    lda.b #$06
    sta.w !wram_0D22
    sta.w !wram_0D23
    lda.b #$78
    sta.w !wram_0E59+1
    stz.w !panel_b_shown
    lda.b #$30
    jsl.l SoundPort0QueuePushRaw
    jmp.w RaceEndCountdown
.lED48:
    dec.w !wram_0E59+1
    rts

; @routine 07:ED4C TimeUpRace
; WHAT IT DOES: A race's time up: the six big sprites (BigSpritesRow6); while
; $0E59 is 0: a lap crossed meanwhile ($0DE8 above $0DEA) goes to phase 6 and
; RaceLapCrossed; with $0DDA at $10 or more, phase 6 when the time is not 0,
; else counts $0E5A down; then $0E5A = $24, $0E59 one less, sound $51 and
; Countdown0DF8. Once $0E59 is set: at $0E5A = 0, once the window is closed,
; leaves the race (RaceEndLeave); else counts $0E5A down, at 1 setting $19DF
; = 1 and sliding the window up.
; HOW: SEP #$30; DEC A and compares. The lap crossed and phase 6 are reached
; only by unit_race_phases.
; Entry: Y = the player; D = $0000. Exit: as the path leaves it, or does not
; return.
%at($07ED4C)
TimeUpRace:
    jsr.w BigSpritesRow6
    sep.b #$30
    lda.w !wram_0E59
    bne .lED92
    lda.w !player_rec_7a
    cmp.w !wram_0DEA
    beq .lED68
    bcc .lED68
    lda.b #$06
    sta.w !engine_phase_p1
    jmp.w RaceLapCrossed
.lED68:
    lda.w !wram_0DDA
    cmp.b #$10
    bcc .lED80
    lda.w !race_time_bcd
    bne .lED7A
    dec.w !wram_0E59+1
    beq .lED80
    rts
.lED7A:
    lda.b #$06
    sta.w !engine_phase_p1
    rts
.lED80:
    lda.b #$24
    sta.w !wram_0E59+1
    dec.w !wram_0E59
    lda.b #$51
    jsl.l SoundPort0QueuePush
    jsr.w Countdown0DF8
    rts
.lED92:
    sep.b #$20
    lda.w !wram_0E59+1
    bne .lEDA3
    lda.w !window_hdma_off
    cmp.b #$05
    bne .lEDB4
    jmp.w RaceEndLeave
.lEDA3:
    dec a
    cmp.b #$01
    bne .lEDB5
    sta.w !wram_0E59+1
    lda.b #$01
    sta.w !wram_19DF
    jsl.l WindowSlideUpSound
.lEDB4:
    rts
.lEDB5:
    cmp.b #$18
    bne .lEDBD
    sta.w !wram_0E59+1
    rts
.lEDBD:
    sta.w !wram_0E59+1
    rts
