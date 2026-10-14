; race_frame.asm: a race frame on the S-CPU, and the race's start, which
; sets up the Super FX, runs the course script and steps two frames.
; C: src/stuntracefx/race/race_frame.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:B7C5 RaceFrameStep
; WHAT IT DOES: One race frame: the frames since the last one ($0306) go to
; $06D4 and $0306 starts again; the near side is set ($1F07 = 0) and
; $70:246C = $FFFF; the objects run (ObjectsRunHandlers); the camera
; record at $1A64 moves on (+$37 += $400, CameraFollowStep) and the tied
; objects face it (ObjectFaceTied); then the view (ViewFromRecord), the
; Super FX frame with the object work (FrameRunGsu8FCA) and the view
; update (ViewUpdateAndGsuAEF8). With A = 0 it then waits for the vertical
; blank to take the frame ($0357), draws the screen (GsuAB13WithParams)
; and flips it (GsuScreenFlip).
; HOW: Data bank $03 (the program bank), $7E around the camera calls; A is
; kept on the stack; P and the data bank saved and restored.
; Entry: A = 0 to show the frame, else not. Exit: A = the entry A, or as
; GsuScreenFlip leaves it; X, Y as the last call leaves them; P and the
; data bank unchanged.
%at($03B7C5)
RaceFrameStep:
    php
    rep.b #$30
    pha
    sep.b #$20
    phb
    phk
    plb
    lda.w !frames_since_tick
    sta.w !frames_taken
    stz.w !frames_since_tick
    sep.b #$20
    lda.b #$00
    sta.w !draw_side
    rep.b #$20
    lda.w #$FFFF
    sta.l !gsu_view_246c
    sep.b #$20
    rep.b #$10
    jsl.l ObjectsRunHandlers
    sep.b #$20
    rep.b #$10
    phb
    lda.b #$7E
    pha
    plb
    ldx.w #$1A64
    rep.b #$20
    lda.b !cam_angle_add+2,x
    clc
    adc.w #$0400
    sta.b !cam_angle_add+2,x
    jsl.l CameraFollowStep
    jsl.l ObjectFaceTied
    plb
    rep.b #$10
    ldx.w #$1A64
    jsl.l ViewFromRecord
    jsl.l FrameRunGsu8FCA
    jsl.l ViewUpdateAndGsuAEF8
    rep.b #$20
    pla
    bne .done
    sep.b #$20
.wait_vblank:
    lda.w !wram_0357
    beq .wait_vblank
    rep.b #$30
    jsl.l GsuAB13WithParams
    jsr.w GsuScreenFlip
    sep.b #$20
    stz.w !wram_0357
.done:
    plb
    plp
    rtl

; @routine 03:B5DF RaceStart
; WHAT IT DOES: Starts a race: the Super FX's screen mode ($0374 = 3),
; clock (CLSR = 1), config (CFGR = $20) and screen base (SCBR = $0B); not
; the menu ($0D62 = 0); the object pools (ObjectPoolInit) and view
; (ViewDefaults); the screen start $2C00 ($70:021A and direct page $48),
; $70:01C4 = 2, $70:108C = 0; Super FX program $04:8800 (GsuRun); $116A =
; 0; the course script at $05:8000 (CourseScriptRun); two race frames not
; shown (RaceFrameStep with A = 1); the camera's target ($1A64 + 2) from
; $116C; $0357 = 1; and Super FX RAM $70:2C00-$5FFF cleared.
; HOW: Through the data bank as the caller has it, with long stores to
; Super FX RAM. P saved and restored.
; Entry: D = $0000. Exit: A = 0; X = $3400; Y = $1A64; P unchanged.
%at($03B5DF)
RaceStart:
    php
    sep.b #$20
    lda.b #$03
    sta.w !gsu_scmr_shadow
    lda.b #$01
    sta.w !GSU_CLSR
    lda.b #$20
    sta.w !GSU_CFGR
    lda.b #$0B
    sta.w !GSU_SCBR
    stz.w !wram_0D62
    rep.b #$30
    lda.w #$A545
    sta.l !gsu_rec_4e
    jsl.l ObjectPoolInit
    jsl.l ViewDefaults
    rep.b #$20
    lda.w #$2C00
    sta.l !gsu_screen_start
    sta.b !gsu_screen_start_dp
    lda.w #$0002
    sta.l !gsu_word_1c4
    lda.w #$0000
    sta.l !gsu_word_108c
    sep.b #$20
    rep.b #$10
    lda.b #$04
    ldx.w #$8800
    jsl.l GsuRun
    rep.b #$20
    stz.w !race_word_116a
    sep.b #$20
    rep.b #$10
    lda.b #$05
    ldx.w #$0000
    jsl.l CourseScriptRun
    lda.b #$01
    jsl.l RaceFrameStep
    lda.b #$01
    jsl.l RaceFrameStep
    rep.b #$30
    lda.w #$0000
    asl a
    tax
    lda.l !race_target_116c,x
    ldy.w #$1A64
    sta.w !cam_target,y
    sep.b #$20
    lda.b #$01
    sta.w !wram_0357
    rep.b #$30
    lda.w #$0000
    ldx.w #$0000
.clear:
    sta.l !gsu_screen_area,x
    inx
    inx
    cpx.w #$3400
    bne .clear
    plp
    rtl

; @routine 07:A9D7 RaceFinishPhases0E
; WHAT IT DOES: Sets the race phases after a finish: sound $37; with Y not
; 0 (player 2), player 1 $12 and player 2 $0E; with Y = 0, both $14 (after
; $07:E944) when bit 0 of $0DED is set, else player 1 $0E and player 2
; $12. Then $0E58 = $30, $0E5B = $0E5F = 0 and sound 2.
; HOW: Shares its tail with RaceFinishPhases10 ($07:AA4E). The $0DED case
; is not reached in the traces so far.
; Entry: M 8-bit, X 8-bit; Y = player. Exit: A low byte = 2; M 8-bit; N, Z
; from the data bank (the sound queue's last pull).
%at($07A9D7)
RaceFinishPhases0E:
    lda.b #$37
    jsl.l SoundPort0QueuePush
    cpy.b #$00
    bne .player2
    sep.b #$20
    lda.w !player_rec_41+1
    xba
    lda.w !player_rec_40+1
    rep.b #$20
    bit.w #$0001
    bne RaceFinishPhasesBoth
    sep.b #$20
    lda.b #$12
    sta.w !engine_phase_p2
    lda.b #$0E
    sta.w !engine_phase_p1
    bra RaceFinishPhasesTail
.player2:
    lda.b #$12
    sta.w !engine_phase_p1
    lda.b #$0E
    sta.w !engine_phase_p2
    bra RaceFinishPhasesTail

; @routine 07:AA0B RaceFinishPhases10
; WHAT IT DOES: As RaceFinishPhases0E, with $10 in place of $0E: player 2
; $10 when Y is not 0, player 1 $10 otherwise.
; HOW: As RaceFinishPhases0E. The Y-not-0 and $0DED cases are not reached
; in the traces so far.
; Entry: as RaceFinishPhases0E. Exit: as RaceFinishPhases0E.
%at($07AA0B)
RaceFinishPhases10:
    lda.b #$37
    jsl.l SoundPort0QueuePush
    cpy.b #$00
    bne .player2
    sep.b #$20
    lda.w !player_rec_41+1
    xba
    lda.w !player_rec_40+1
    rep.b #$20
    bit.w #$0001
    bne RaceFinishPhasesBoth
    sep.b #$20
    lda.b #$12
    sta.w !engine_phase_p2
    lda.b #$10
    sta.w !engine_phase_p1
    bra RaceFinishPhasesTail
.player2:
    sep.b #$20                      ; not reached in the traces so far
    lda.b #$12                      ; not reached in the traces so far
    sta.w !engine_phase_p1          ; not reached in the traces so far
    lda.b #$10                      ; not reached in the traces so far
    sta.w !engine_phase_p2          ; not reached in the traces so far
    bra RaceFinishPhasesTail        ; not reached in the traces so far
RaceFinishPhasesBoth:               ; bit 0 of $0DED: not reached in the traces so far
    sep.b #$20
    jsr.w !race_e944
    lda.b #$14
    sta.w !engine_phase_p1
    sta.w !engine_phase_p2
RaceFinishPhasesTail:               ; both routines end here
    lda.b #$30
    sta.w !wram_0E58
    stz.w !wram_0E5B
    stz.w !palette_animate
    lda.b #$02
    jsl.l SoundPort0QueuePushRaw
    rts

; @routine 03:E838 RaceVarsReset
; WHAT IT DOES: Resets the race's work variables before a race: $0DA2 = -$10
; on course $0A, else 0; $0D29 = $166; the Super FX screen start $2C00
; ($70:021A and direct page $48); the HUD, time, lap and gauge values
; cleared; $10A4 = $9A, $10A6 = $6A; $0D85 = $37D7; the fade values at
; $7F:345C-$7F:3478 ($7FFF, 0, $1F, $FFFF); $0DF4/$0DF5 = 1, $0E5F = $17,
; $0D53 = $FF, $0D45 = $1C; then the V/H IRQ timer ($17, $40), NMITIMEN =
; $31 and BG3 scroll 0 / 0.
; HOW: 16-bit stores, then 8-bit from $10BA on, in the original's order;
; $0D85 is stored twice. The course $0A value is not reached in the traces
; so far.
; Entry: any widths; DB a bank where work RAM and the PPU registers are
; mapped, D = $0000. Exit: A = $0080; M 8-bit; N set, Z clear; C from
; comparing the course with $0A.
%at($03E838)
RaceVarsReset:
    rep.b #$20
    lda.w !course
    and.w #$00FF
    cmp.w #$000A
    bne .other
    lda.w #$FFF0
    bra .set
.other:
    lda.w #$0000
.set:
    sta.w !view_0da2
    lda.w #$0000
    sta.w !fade_level
    lda.w #$0166
    sta.w !wram_0D29
    stz.w !gsu_buffer_select
    lda.w #$2C00
    sta.l !gsu_screen_start
    sta.b !gsu_screen_start_dp
    stz.w !wram_0D81
    stz.w !wram_0E62
    stz.w !race_time_seconds
    stz.w !race_time_minutes
    stz.w !wram_10A2
    lda.w #$009A
    sta.w !wram_10A4
    lda.w #$006A
    sta.w !wram_10A6
    stz.w !wram_10A8
    stz.w !wram_0DA4
    stz.w !wram_0DA6
    stz.w !wram_0D9E
    stz.w !wram_0DA0
    stz.w !menu_gauge
    stz.w !speed_gauge_2_value
    lda.w #$37D7
    sta.w !wram_0D85
    sta.w !wram_0D85
    lda.w #$0000
    sta.l !wram_7EFD5B
    sta.l !wram_7EFD5D
    sta.l !wram_7EFD5F
    sta.l !wram_7EFD61
    sta.l !wram_7EFD63
    sta.l !wram_7F3460
    sta.l !wram_7F3462
    sta.l !wram_7F346C
    sta.l !wram_7F346E
    sta.l !wram_7F3464
    sta.l !wram_7F3466
    lda.w #$FFFF
    sta.l !wram_7F3474
    sta.l !wram_7F3476
    sta.l !wram_7F3478
    lda.w #$7FFF
    sta.l !wram_7F345C
    sta.l !wram_7F345E
    lda.w #$001F
    sta.l !slide_out
    sta.l !wram_7F346A
    stz.w !speed_count_p0
    stz.w !speed_count_p1
    stz.w !speed_total_p0
    stz.w !wram_0DDE
    stz.w !speed_total_p1
    stz.w !wram_0DE2
    stz.w !hud_hold
    stz.w !time_bonus_state
    sep.b #$20
    stz.w !wram_10BA
    stz.w !wram_0D54
    stz.w !near_car_pos
    stz.w !wram_0E49
    stz.w !wram_0D5C
    stz.w !wram_0D5D
    stz.w !lap_mark
    stz.w !wram_0EDC
    stz.w !wram_0EDD
    stz.w !wram_0E1A
    stz.w !engine_phase_p1
    stz.w !engine_phase_p2
    stz.w !sound_hold
    stz.w !wram_0DD9
    stz.w !wram_0DDA
    stz.w !wram_0DDB
    stz.w !player_rec_7a
    stz.w !wram_0DE9
    stz.w !wram_1A2D
    stz.w !wram_0DEA
    stz.w !wram_0DEB
    stz.w !player_rec_4e
    stz.w !wram_0DF7
    stz.w !gauge_b4_value
    stz.w !gauge_a4_value
    stz.w !player_rec_40
    stz.w !wram_0DED
    stz.w !player_rec_41
    stz.w !wram_0DEF
    lda.b #$01
    sta.w !player_mode
    sta.w !wram_0DF5
    stz.w !wram_0E07
    stz.w !wram_0E08
    stz.b !dp_00
    stz.w !wram_0D22
    stz.w !wram_0D23
    stz.w !wram_0D2F
    stz.w !wram_0D2D
    stz.w !wram_0D2E
    stz.w !wram_0D3D
    stz.w !panel_b_shown
    stz.w !wram_0E57
    stz.w !wram_0E5C
    stz.w !wram_0E59
    stz.w !fade_out
    stz.w !wram_0E54
    stz.w !wram_0E3F
    stz.w !wram_0E40
    stz.w !wram_0E41
    stz.w !wram_0E42
    stz.w !current_player
    stz.w !wram_0E4B
    stz.w !player1_held_sound
    stz.w !player2_held_sound
    stz.w !wram_0E51
    stz.w !wram_0E52
    stz.w !wram_0E53
    stz.w !wram_0E90
    stz.w !kept_timer
    stz.w !wram_0E02
    stz.w !wram_0E5B
    stz.w !wram_0D5B
    stz.w !wram_0D5A
    lda.b #$17
    sta.w !palette_animate
    sta.w !VTIMEL
    stz.w !VTIMEH
    lda.b #$40
    sta.w !HTIMEL
    stz.w !HTIMEH
    lda.b #$FF
    sta.w !wram_0D53
    lda.b #$1C
    sta.w !wram_0D45
    lda.b #$31
    sta.w !NMITIMEN
    stz.w !BG2HOFS
    stz.w !BG2HOFS
    lda.b #$80
    sta.w !BG2VOFS
    stz.w !BG2VOFS
    rtl

; @routine 03:D8B3 RaceModeSetup
; WHAT IT DOES: Sets the race up for mode $0D2B. Mode 3 with bits 0-2 of
; $70:FC0B clear only marks $10CD ($FF), takes course $1B and the pick's
; sound ($18DB into $0D8C, HandlerSoundIndex into $0D8B). Otherwise:
; $0D4A = 0 (or, in mode 3, 1 unless bit 0 of $70:FC0B is set), $10CD and
; $0E37 cleared, the pick's sound, the race flags at $0D4C, $0D4E, $10CD,
; $0D48, $0E3E, $0D63, $0D44, $0EED-$0EEF, $0D24, $0D25 cleared, the twelve
; bytes at $0F05 set to 4 in mode 0 else 0, $0E6C/$0E6E cleared; then mode
; 0 unpacks the course's records as running totals
; (RecordsUnpackCourseRunning) with $0DF8 = 2, others $0DF8 = $FF, mode 2
; first sending 5 on APU port 1 and uploading sound block $1A:F091
; (ApuUpload) between two delay loops.
; HOW: 8-bit with word stores where the original is 16-bit; the $0F05
; bytes are stored from the last down. Mode 3 with any of bits 0-2 set is
; not reached in the traces so far.
; Entry: any widths; DB a bank where work RAM is mapped, D = $0000. Exit:
; P restored; X and Y high bytes cleared; A as the last step left it.
%at($03D8B3)
RaceModeSetup:
    php
    sep.b #$20
    lda.w !hud_mode
    cmp.b #$03
    bne .other
    lda.l !save_progress
    and.b #$07
    bne .mode3_more
    lda.b #$FF
    sta.w !wram_10CD
    lda.b #$1B
    sta.w !course
    stz.w !wram_0E3E
    rep.b #$20
    lda.l !selection_handler_a
    sta.w !wram_0D8C
    jsr.w HandlerSoundIndex
    sep.b #$30
    sta.w !wram_0D8B
    plp
    rtl
.mode3_more:
    lda.l !save_progress
    bit.b #$01
    bne .one_off
    lda.b #$01
    bra .set_0d4a
.one_off:
    lda.b #$00
.set_0d4a:
    sta.w !wram_0D4A
    bra .setup
.other:
    stz.w !wram_0D4A
.setup:
    stz.w !wram_10CD
    rep.b #$20
    stz.w !scroll_value
    lda.l !selection_handler_a
    sta.w !wram_0D8C
    jsr.w HandlerSoundIndex
    sep.b #$30
    sta.w !wram_0D8B
    stz.w !digit62_override
    stz.w !hud_blank
    stz.w !wram_10CD
    stz.w !wram_0D48
    stz.w !wram_0E3E
    stz.w !wram_0D63
    stz.w !digit62_override
    stz.w !wram_0D44
    stz.w !wram_0EED
    stz.w !wram_0EEE
    stz.w !wram_0EEF
    stz.w !wram_0D24
    stz.w !wram_0D25
    ldx.b #$0B
    lda.w !hud_mode
    beq .four
    lda.b #$00
    bra .columns
.four:
    lda.b #$04
.columns:
    sta.w !level_columns,x
    dex
    bpl .columns
    rep.b #$20
    stz.w !wram_0E6C
    stz.w !wram_0E6E
    sep.b #$20
    lda.w !hud_mode
    beq .records
    cmp.b #$02
    bne .mark
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.wait1:
    dec a
    bne .wait1
    lda.w #$001A
    ldy.w #$F091
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.wait2:
    dec a
    bne .wait2
    plp
.mark:
    sep.b #$20
    lda.b #$FF
    sta.w !wram_0DF8
    plp
    rtl
.records:
    jsl.l RecordsUnpackCourseRunning
    lda.b #$02
    sta.w !wram_0DF8
    plp
    rtl

; @routine 7E:F6E9 ViewParamsByPlayers
; WHAT IT DOES: Sets the view's direct page parameters for one player or,
; with $0D62 set, for the two-player split: $2A-$2F, $30/$31, $CE-$D1,
; $34/$35 and $32/$33 ($CF, $80, $68, $40, $4000, $4EC4 for one; $8F,
; $50, $48, $28, $6666, $71C7 for two).
; HOW: Byte stores to direct page.
; Entry: any widths, D = $0000. Exit: M 8-bit; A low byte = the last byte
; ($4E or $71), N, Z from it.
%ramcode($7EF6E9, $015410)
ViewParamsByPlayers:
    sep.b #$20
    lda.w !wram_0D62
    beq ViewParamsOne
    bra ViewParamsTwo

; @routine 7E:F6F2 ViewParamsSpeedLong
; WHAT IT DOES: A long way into ObjectLinkEventTrigger2's tail, which
; nothing reaches: stores A at $1A21.
; HOW: JSR ObjectLinkEventTrigger2_speed, RTL.
; Entry: any widths; data bank $7E. Exit: M and X 8-bit (X and Y high
; bytes 0); A unchanged.
ViewParamsSpeedLong:
    jsr.w ObjectLinkEventTrigger2_speed
    rtl
ViewParamsOne:
    lda.b #$00
    sta.b !dp_2a
    lda.b #$00
    sta.b !dp_2a+1
    lda.b #$CF
    sta.b !dp_2e
    lda.b #$00
    sta.b !dp_2e+1
    lda.b #$00
    sta.b !dp_2c
    lda.b #$00
    sta.b !dp_2c+1
    lda.b #$80
    sta.b !dp_30
    lda.b #$00
    sta.b !dp_30+1
    lda.b #$68
    sta.b !dp_ce
    lda.b #$00
    sta.b !dp_ce+1
    lda.b #$40
    sta.b !dp_d0
    lda.b #$00
    sta.b !dp_d0+1
    lda.b #$00
    sta.b !dp_34
    lda.b #$40
    sta.b !dp_34+1
    lda.b #$C4
    sta.b !dp_32
    lda.b #$4E
    sta.b !dp_32+1
    rts

; @routine 7E:F737 ViewParamsBobLong
; WHAT IT DOES: A long way into ObjectLinkEventTrigger2's tail, which
; nothing reaches: starts bob 1 with A as its kind ($1A1D = A, $1A1F =
; $1A23 = 0, $1A21 = $FE).
; HOW: JSR ObjectLinkEventTrigger2_bob, RTL.
; Entry: M 8-bit (the code is 8-bit); data bank $7E. Exit: M and X 8-bit
; (X and Y high bytes 0); A low byte = $FE; N set, Z clear.
ViewParamsBobLong:
    jsr.w ObjectLinkEventTrigger2_bob
    rtl
ViewParamsTwo:
    lda.b #$00
    sta.b !dp_2a
    lda.b #$00
    sta.b !dp_2a+1
    lda.b #$8F
    sta.b !dp_2e
    lda.b #$00
    sta.b !dp_2e+1
    lda.b #$00
    sta.b !dp_2c
    lda.b #$00
    sta.b !dp_2c+1
    lda.b #$50
    sta.b !dp_30
    lda.b #$00
    sta.b !dp_30+1
    lda.b #$48
    sta.b !dp_ce
    lda.b #$00
    sta.b !dp_ce+1
    lda.b #$28
    sta.b !dp_d0
    lda.b #$00
    sta.b !dp_d0+1
    lda.b #$66
    sta.b !dp_34
    lda.b #$66
    sta.b !dp_34+1
    lda.b #$C7
    sta.b !dp_32
    lda.b #$71
    sta.b !dp_32+1
    rts

; @routine 02:E289 RaceLoop
; WHAT IT DOES: The race's frame loop (unless $10CD is set: nothing): packs
; the three colour settings $1184-$1186 into a BGR word at $0F11, sets the
; players $0D62 from the HUD mode (the table after the loop: 2 for mode 2),
; loads the race (RaceLoad), sets $0D3F = $10, clears the fade, the fade
; out and $0D40, $0D43 and $0D50, turns the NMI, the IRQ and the joypad
; read on ($4200 = $31) and interrupts on; then each frame, after the
; vertical blank ($0D52) and the Super FX's frame ($0D81): GsuEE88Ramp, every
; eighth frame the next of four sprite pieces ($10BB), $10C1 = $10C5 = 0,
; $0D7F = 1, the soft reset check, the scroll HDMA, the large sprites hidden,
; LoopLeaveWhenEnded (which leaves the loop when the race's fade out is done)
; and 02:E356.
; HOW: The packing through direct page $F1; then P and the data bank pushed
; (the data bank $02) for LoopLeaveWhenEnded to pull on the way out.
; Entry: any widths; D = $0000; called by JSL. Exit: through
; LoopLeaveWhenEnded: as the caller left P and the data bank; or, with
; $10CD set, at once.
%at($02E289)
RaceLoop:
    sep.b #$20
    lda.w !wram_10CD
    beq .lE291
    rtl
.lE291:
    rep.b #$20
    lda.w !wram_1184
    and.w #$001F
    sta.b !digit_count
    lda.w !wram_1184+1
    asl a
    asl a
    asl a
    asl a
    asl a
    and.w #$03E0
    ora.b !digit_count
    sta.b !digit_count
    lda.w !wram_1184+2
    xba
    asl a
    asl a
    and.w #$7C00
    ora.b !digit_count
    sta.w !level_columns+12
    php
    sep.b #$30
    phb
    phk
    plb
    ldx.w !hud_mode
    lda.w RaceLoop_players,x
    sta.w !wram_0D62
    jsl.l RaceLoad
    sep.b #$20
    lda.b #$10
    sta.w !wram_0D3F
    stz.w !fade_level_hi
    stz.w !fade_out
    stz.w !wram_0D3F+4
    stz.w !wram_0D3F+1
    stz.w !hud_blank+2
    lda.b #$31
    sta.w !NMITIMEN
    cli
.lE2E7:
    sep.b #$20
    lda.w !hud_blank+4
    beq .lE2E7
    stz.w !hud_blank+4
    rep.b #$20
.lE2F3:
    lda.w !wram_0D81
    beq .lE2F3
    stz.w !wram_0D81
    jsl.l GsuEE88Ramp
    sep.b #$20
    lda.w !frame_count
    and.b #$07
    cmp.b #$07
    bne .lE317
    lda.w !sprite_piece_select
    inc a
    cmp.b #$04
    bcc .lE314
    lda.b #$00
.lE314:
    sta.w !sprite_piece_select
.lE317:
    lda.b #$00
    sta.w !sprite_piece_palette
    sta.w !sprite_piece_tile
    lda.b #$01
    sta.w !gsu_buffer_select
    jsl.l SoftResetLong
    jsl.l ScrollHdmaSet
    jsl.l OamBufferHideLargeLong
    jsr.w LoopLeaveWhenEnded
    jsr.w RaceFrameTail
    bra .lE2E7
RaceLoop_players:
    db $00,$00,$02,$00,$00

; @routine 02:E33D LoopLeaveWhenEnded
; WHAT IT DOES: Ends the frame loop at $02:E289 once its fade-out has run
; its course ($0E3A set and $0D61 negative): interrupts and the vertical
; blank interrupt off, and a return from the loop itself with its saved
; data bank and P. Otherwise a plain return.
; HOW: PLA drops this routine's return address; then STZ $4200, SEI, PLB,
; PLP and RTL take the loop's frame (P and the data bank pushed after its
; JSL).
; Entry: D = $0000. Exit: M 8-bit; on the way out as the loop's caller
; left P and the data bank, A = this routine's return address.
%at($02E33D)
LoopLeaveWhenEnded:
    sep.b #$20
    lda.w !fade_out
    beq .lE349
    lda.w !fade_level_hi
    bmi .lE34A
.lE349:
    rts
.lE34A:
    rep.b #$20
    pla
    sep.b #$30
    stz.w !NMITIMEN
    sei
    plb
    plp
    rtl

; @routine 03:F0E6 RaceLoad
; WHAT IT DOES: Loads a race: video registers reset (twice), the unlocks
; checked, the sounds chosen (SoundIndicesFromHandlers) and loaded
; (SoundDataLoad $E049/$1E, ApuUpload $1B:B8A0 and $1B:D5FC, each after 5
; on APU port 1 and a delay), SoundReset, 1 on APU port 0, the Super FX
; screen mode shadow $0374 = 3, the noise tiles (VramNoiseTiles), the
; palette from $1F:8200 into $7E:FD65 and to CGRAM, the OAM buffer
; cleared, the race variables reset and two Super FX blocks put in VRAM
; (GsuCF42ToVram, Y/X $F837/$F7E6 and $F822/$F801). Then by HUD mode $0D2B
; through the table at $03:F1B2: modes 0 and 3 take their views (Y $F868
; with X $F7F2 or $F7FB; $70:3000 copied twice into $7F:93D7 and
; $7F:BBD7), then block $F83E/$F7E9 and the scroll $0E37 = $50 ($0 with
; $0D63 set); modes 1 and 2 their views ($F7F8, $F7F5) and blocks
; $F83E/$F7EC or $F83E/$F7EF with the scroll 0, mode 2 and mode 1 without
; $0D63 also clearing $0D48 (mode 1 then with scroll $50); mode 4 (the
; fifth entry) none of these. $0E35, $0E31 and $0E33 are cleared. Last,
; RaceScreenSetup.
; HOW: P and the data bank ($03) pushed; delay loops of $4000; MVN for the
; copies. Modes 3 and 4, and mode 1 with $0D63 set, are not reached in the
; traces so far. Modes above 4 would take code bytes as addresses and are
; not modelled.
; Entry: any widths. Exit: P and the data bank restored; A, X, Y as
; RaceScreenSetup leaves them.
%at($03F0E6)
RaceLoad:
    php
    sep.b #$30
    phb
    phk
    plb
    jsl.l VideoRegsReset
    jsl.l SaveUnlocksCheck
    jsl.l SoundIndicesFromHandlers
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lF105:
    dec a
    bne .lF105
    lda.w #$E049
    ldy.w #$001E
    jsl.l SoundDataLoad
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lF120:
    dec a
    bne .lF120
    lda.w #$001B
    ldy.w #$B8A0
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lF132:
    dec a
    bne .lF132
    plp
    php
    rep.b #$30
    lda.w #$0005
    sta.l !APUIO1
    lda.w #$4000
.lF143:
    dec a
    bne .lF143
    lda.w #$001B
    ldy.w #$D5FC
    jsl.l ApuUpload
    rep.b #$20
    lda.w #$4000
.lF155:
    dec a
    bne .lF155
    plp
    jsl.l SoundReset
    sep.b #$20
    lda.b #$01
    sta.w !APUIO0
    lda.b #$03
    sta.w !gsu_scmr_shadow
    jsr.w VramNoiseTiles
    rep.b #$30
    ldx.w #$01FE
.lF171:
    lda.l !race_palette,x
    sta.l !palette_buffer,x
    dex
    dex
    bpl .lF171
    jsl.l CgramDmaAll
    jsl.l OamBufferClear
    jsr.w ReturnOnly
    jsl.l VideoRegsReset
    jsl.l RaceVarsReset
    rep.b #$30
    ldy.w #$F837
    ldx.w #$F7E6
    jsl.l GsuCF42ToVram
    rep.b #$30
    ldy.w #$F822
    ldx.w #$F801
    jsl.l GsuCF42ToVram
    sep.b #$30
    lda.w !hud_mode
    asl a
    tax
    jmp.w (RaceLoad_modes,x)
RaceLoad_modes:
    dw .lF1EE, .lF2A7, .lF256, .lF1BC, .lF321
.lF1BC:
    rep.b #$30
    ldy.w #$F868
    ldx.w #$F7FB
    jsl.l GsuCF42ToVram
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$93D7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$BBD7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    bra .lF21E
.lF1EE:
    rep.b #$30
    ldy.w #$F868
    ldx.w #$F7F2
    jsl.l GsuCF42ToVram
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$93D7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$BBD7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
.lF21E:
    rep.b #$30
    ldy.w #$F83E
    ldx.w #$F7E9
    jsl.l GsuCF42ToVram
    sep.b #$20
    lda.w !wram_0D63
    beq .lF242
    rep.b #$20
    stz.w !wram_0E33+2
    stz.w !scroll_value
    stz.w !blend_set_b+10
    stz.w !wram_0E33
    jmp.w .lF321
.lF242:
    rep.b #$20
    stz.w !wram_0E33+2
    lda.w #$0050
    sta.w !scroll_value
    stz.w !blend_set_b+10
    stz.w !wram_0E33
    jmp.w .lF321
.lF256:
    rep.b #$30
    ldy.w #$F868
    ldx.w #$F7F5
    jsl.l GsuCF42ToVram
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$93D7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$BBD7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    rep.b #$30
    ldy.w #$F83E
    ldx.w #$F7EF
    jsl.l GsuCF42ToVram
    rep.b #$20
    stz.w !wram_0E33+2
    stz.w !scroll_value
    stz.w !blend_set_b+10
    stz.w !wram_0E33
    sep.b #$20
    stz.w !wram_0D48
    bra .lF321
.lF2A7:
    rep.b #$30
    ldy.w #$F868
    ldx.w #$F7F8
    jsl.l GsuCF42ToVram
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$93D7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    phb
    php
    rep.b #$30
    lda.w #$0FFF
    ldx.w #$3000
    ldy.w #$BBD7
    mvn !tilemap_7f93d7>>16,!gsu_picture>>16
    plp
    plb
    rep.b #$30
    ldy.w #$F83E
    ldx.w #$F7EC
    jsl.l GsuCF42ToVram
    rep.b #$20
    stz.w !wram_0E33+2
    stz.w !scroll_value
    stz.w !blend_set_b+10
    stz.w !wram_0E33
    sep.b #$20
    lda.w !wram_0D63
    beq .lF309
    rep.b #$20
    stz.w !wram_0E33+2
    stz.w !scroll_value
    stz.w !blend_set_b+10
    stz.w !wram_0E33
    jmp.w .lF321
.lF309:
    rep.b #$20
    stz.w !wram_0E33+2
    lda.w #$0050
    sta.w !scroll_value
    stz.w !blend_set_b+10
    stz.w !wram_0E33
    sep.b #$20
    stz.w !wram_0D48
    bra .lF321
.lF321:
    jsr.w RaceScreenSetup
    sep.b #$20
    plb
    plp
    rtl

; @routine 02:D7CD RaceFrameRun
; WHAT IT DOES: One frame of a race: takes the frames counted since the last
; ($0306, into $06D4, cleared), points the view from player 1's car
; (ViewFromRecord on the object two links on from $0346), reads the
; players' controls (PlayerControlsFrame), draws side 0 ($1F07) with Super FX
; programs $8FCA (FrameRunGsu8FCA, $70:246C-D = $FF first) and $01:BBC7
; (GsuRun), runs the objects, the view and program $AEF8, the palette fade,
; the word copy and the backdrop angles; then side 1 with the HUD
; (FrameRunGsuAB13B, $1AE8 cleared first), DebugShowErrors, the speed gauge
; (A = $70:2476, X = $2143) and the near-car sound; keeps $1AE8 at $1AEA;
; runs program $01:D307 when $70:2732 is set, else 07:DAA7; switches the
; screen buffers (GsuScreenToggle), clears $0357 and keeps time: $1F3E =
; $0306, $06D5 adds $06D4 and $06D6 counts frames, each $3C of $06D5 moving
; $06D6 to $06D7; $0306 also goes to $70:108E, with 0 at $70:108F.
; HOW: Data bank $02, its own (the low addresses reach work RAM); $00 = 2;
; 8-bit A for the counts; P and the data bank put back at the end.
; Entry: any widths; D = $0000. Exit: P and the data bank unchanged; A low
; byte 0; X, Y as the last call left them.
%at($02D7CD)
RaceFrameRun:
    php
    sep.b #$20
    rep.b #$10
    phb
    phk
    plb
    lda.w !frames_since_tick
    sta.w !frames_taken
    stz.w !frames_since_tick
    lda.b #$02
    sta.b !bank0_base
    rep.b #$30
    ldx.w !wram_0346
    lda.l !obj_link,x
    tax
    lda.l !obj_link,x
    tax
    jsl.l ViewFromRecord
    jsl.l PlayerControlsFrame
    sep.b #$20
    stz.w !draw_side
    lda.b #$FF
    sta.l !gsu_view_246c
    sta.l !gsu_view_246c+1
    jsl.l FrameRunGsu8FCA
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$01
    ldx.w #$BBC7
    jsl.l GsuRun
    sep.b #$20
    rep.b #$10
    sep.b #$20
    rep.b #$10
    jsl.l ObjectsRunHandlers
    jsl.l ViewUpdateAndGsuAEF8
    jsl.l PaletteFadeStep
    jsl.l WordCopyStep
    jsl.l BackdropAnglesFromView
    sep.b #$20
    lda.b #$01
    sta.w !draw_side
    rep.b #$20
    stz.w !spawn_counter+10
    sep.b #$20
    jsl.l FrameRunGsuAB13B
    sep.b #$20
    jsl.l DebugShowErrors
    rep.b #$30
    ldx.w #$2143
    lda.l !gsu_match_2476
    jsl.l SpeedGaugeUpdate
    jsl.l NearCarSoundUpdate
    rep.b #$20
    lda.w !spawn_counter+10
    sta.w !spawn_counter+12
    lda.l !fx_init_2732
    beq .lD87F
    sep.b #$20
    lda.b #$01
    ldx.w #$D307
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    bra .lD885
.lD87F:
    sep.b #$20
    jsl.l RaceSoundsAndHud
.lD885:
    jsr.w GsuScreenToggle
    sep.b #$20
    stz.w !wram_0357
    sep.b #$20
    lda.w !frames_since_tick
    sta.w !wram_1F3C+2
    lda.w !frames_taken+1
    clc
    adc.w !frames_taken
    sta.w !frames_taken+1
    inc.w !frames_taken+2
    lda.w !frames_taken+1
    cmp.b #$3C
    bcc .lD8BB
    lda.w !frames_taken+2
    sta.w !frames_taken+3
    stz.w !frames_taken+2
    lda.w !frames_taken+1
    sec
    sbc.b #$3C
    sta.w !frames_taken+1
.lD8BB:
    sep.b #$20
    lda.w !frames_since_tick
    sta.l !gsu_word_108c+2
    lda.b #$00
    sta.l !gsu_word_108c+3
    plb
    plp
    rtl

; @routine 0B:FB26 RaceFrameRunTwoPlayers
; WHAT IT DOES: One frame of a two-player race: takes the frames counted
; since the last ($0306, into $06D4, cleared), runs the objects, then for
; player 1 points the view from the car (ViewFromRecord on the object two
; links on from $0346), reads both players' controls (PlayerControlsFrame)
; and draws side 0 ($1F07) with Super FX programs $8FCA (FrameRunGsu8FCA,
; $70:246C-D = $FF first) and $01:BBC7, the view and program $AEF8, the
; palette fade, the word copy and the backdrop angles; for player 2 the
; view from the car at $034A, side 1 with the HUD (FrameRunGsuAB13B, $1AE8
; cleared first) and the speed gauge (A = $70:2476, X = $2143), 07:DA02,
; a screen switch (GsuScreenFlipMenu) and $0357 cleared; then the view and
; program $AEF8 again, the backdrop angles, $06CF less $140, HUD panel A
; (FrameRunGsuAB13A), the second gauge (A = $70:2478, X = $2142), 07:DA65,
; another switch and $0357 cleared; and keeps time as RaceFrameRun does.
; HOW: As RaceFrameRun: data bank $02, its own; $00 = 2; P and the data bank
; put back at the end.
; Entry: any widths; D = $0000. Exit: P and the data bank unchanged; A low
; byte 0; X, Y as the last call left them.
%at($0BFB26)
RaceFrameRunTwoPlayers:
    php
    sep.b #$20
    rep.b #$10
    phb
    phk
    plb
    lda.w !frames_since_tick
    sta.w !frames_taken
    stz.w !frames_since_tick
    lda.b #$02
    sta.b !bank0_base
    sep.b #$20
    rep.b #$10
    jsl.l ObjectsRunHandlers
    rep.b #$30
    ldx.w !wram_0346
    lda.l !obj_link,x
    tax
    lda.l !obj_link,x
    tax
    jsl.l ViewFromRecord
    jsl.l PlayerControlsFrame
    sep.b #$20
    stz.w !draw_side
    lda.b #$FF
    sta.l !gsu_view_246c
    sta.l !gsu_view_246c+1
    jsl.l FrameRunGsu8FCA
    sep.b #$20
    rep.b #$10
    sep.b #$20
    lda.b #$01
    ldx.w #$BBC7
    jsl.l GsuRun
    sep.b #$20
    rep.b #$10
    jsl.l ViewUpdateAndGsuAEF8
    rep.b #$20
    jsl.l PaletteFadeStep
    jsl.l WordCopyStep
    jsl.l BackdropAnglesFromView
    rep.b #$30
    ldx.w !wram_034A
    lda.l !obj_link,x
    tax
    lda.l !obj_link,x
    tax
    jsl.l ViewFromRecord
    sep.b #$20
    lda.b #$01
    sta.w !draw_side
    rep.b #$20
    stz.w !spawn_counter+10
    sep.b #$20
    sep.b #$20
    jsl.l FrameRunGsuAB13B
    rep.b #$30
    ldx.w #$2143
    lda.l !gsu_match_2476
    jsl.l SpeedGaugeUpdate
    sep.b #$20
    jsl.l RaceSoundsPlayer1
    jsr.w GsuScreenFlipMenu
    sep.b #$20
    stz.w !wram_0357
    jsl.l ViewUpdateAndGsuAEF8
    jsl.l BackdropAnglesFromView
    rep.b #$20
    lda.w !view_06cf
    sec
    sbc.w #$0140
    sta.w !view_06cf
    sep.b #$20
    jsl.l FrameRunGsuAB13A
    rep.b #$30
    ldx.w #$2142
    lda.l !gsu_match_2478
    jsl.l SpeedGauge2Update
    jsl.l RaceSoundsPlayer2
    sep.b #$20
    jsr.w GsuScreenFlipMenu
    sep.b #$20
    stz.w !wram_0357
    sep.b #$20
    lda.w !frames_since_tick
    sta.w !wram_1F3C+2
    lda.w !frames_taken+1
    clc
    adc.w !frames_taken
    sta.w !frames_taken+1
    inc.w !frames_taken+2
    lda.w !frames_taken+1
    cmp.b #$3C
    bcc .lFC37
    lda.w !frames_taken+2
    sta.w !frames_taken+3
    stz.w !frames_taken+2
    lda.w !frames_taken+1
    sec
    sbc.b #$3C
    sta.w !frames_taken+1
.lFC37:
    sep.b #$20
    lda.w !frames_since_tick
    sta.l !gsu_word_108c+2
    lda.b #$00
    sta.l !gsu_word_108c+3
    plb
    plp
    rtl

; @routine 7E:F21E AttractCourseSetup
; @alias 02:CF45
; WHAT IT DOES: Sets up the attract race's course: data bank 0, interrupts
; and HDMA off, the screen blanked (A = $80 after Set7EBC2BTrio), $084B and
; $084C cleared, waits for lines $6E and $64 (WaitScanline), clears the held
; buttons $0309, $1168, $08EF and $05D5, starts the view (AttractViewStart),
; the first sprite slot and the HUD (RaceHudLoad), then Super FX program
; $01:D1EB; P and the data bank put back and interrupts on.
; HOW: Run from its ROM copy at $02:CF45; the clears as the original makes
; them, $1168 seven times.
; Entry: any widths. Exit: P and the data bank as they were, then I clear;
; A, X, Y as GsuRun leaves them.
%ramcode($7EF21E, $014F45)
AttractCourseSetup:
    php
    phb
    sep.b #$20
    lda.b #$00
    pha
    plb
    sei
    sep.b #$20
    stz.w !HDMAEN
    lda.b #$80
    jsl.l Set7EBC2BTrio
    sta.w !INIDISP
    stz.w !wram_084B
    stz.w !wram_084C
    lda.b #$6E
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    lda.b #$64
    sta.w !scanline_wait
    jsl.l WaitScanline
    sep.b #$20
    rep.b #$10
    rep.b #$30
    stz.w !joy1_held
    stz.w !packed_time_a+5
    stz.w !packed_time_a+5
    stz.w !packed_time_a+5
    sep.b #$20
    stz.w !wram_08EF
    stz.w !packed_time_a+5
    stz.w !packed_time_a+5
    stz.w !packed_time_a+5
    stz.w !slot_mask
    stz.w !packed_time_a+5
    stz.w !packed_time_a+5
    stz.w !packed_time_a+5
    stz.w !packed_time_a+5
    jsl.l AttractViewStartRom
    jsl.l SpriteSlotFirstLong
    jsl.l RaceHudLoad
    sep.b #$20
    lda.b #$01
    ldx.w #$D1EB
    jsl.l GsuRun
    sep.b #$20
    rep.b #$10
    plb
    plp
    cli
    rtl

; @routine 7E:F29F AttractViewStart
; @alias 02:CFC6
; WHAT IT DOES: Starts the attract race's view: $70:01D0 = $1234, $70:01C4 =
; 1, $70:108C = the players ($0D62), direct page $A0 = $80, $05EF = $FF and
; $00 = 0, the view's parameters for the players (ViewParamsByPlayers), then
; on at 03:AE73, which returns to this routine's caller.
; HOW: Run from its ROM copy at $02:CFC6 (the JSR reaches $02:D410, the
; copy of ViewParamsByPlayers); P pushed first, a long jump to finish.
; Entry: any widths; D = $0000. Exit: as 03:AE73 leaves it.
%ramcode($7EF29F, $014FC6)
AttractViewStart:
    php
    rep.b #$20
    lda.w #$1234
    sta.l !gsu_word_1d0
    lda.w #$0001
    sta.l !gsu_word_1c4
    sep.b #$20
    lda.w !wram_0D62
    sta.l !gsu_word_108c
    lda.b #$80
    sta.b !dp_a0
    lda.b #$FF
    sta.w !wram_05EF
    stz.b !bank0_base
    sep.b #$30
    jsr.w ViewParamsByPlayersRom
    jml.l AttractViewRest
