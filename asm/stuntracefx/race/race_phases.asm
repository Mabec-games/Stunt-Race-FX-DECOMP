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
    jsl.l $03DEFD
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
    jsl.l $03DF10
    plx
    lda.w !level_columns+12
    sta.l !save_record_b+2,x
    sep.b #$20
    lda.w !wram_0D89
    sta.l !save_record_b+4,x
    jsr.w $ADDA
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
    jsr.w $A99A
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
