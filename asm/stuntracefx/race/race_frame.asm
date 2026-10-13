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
ViewParamsSpeedLong:                    ; 7E:F6F2, not reached in the traces so far
    jsr.w ObjectLinkEventTrigger2_speed
    rtl
ViewParamsOne:
    lda.b #$00
    sta.b $2A
    lda.b #$00
    sta.b $2B
    lda.b #$CF
    sta.b $2E
    lda.b #$00
    sta.b $2F
    lda.b #$00
    sta.b $2C
    lda.b #$00
    sta.b $2D
    lda.b #$80
    sta.b $30
    lda.b #$00
    sta.b $31
    lda.b #$68
    sta.b $CE
    lda.b #$00
    sta.b $CF
    lda.b #$40
    sta.b $D0
    lda.b #$00
    sta.b $D1
    lda.b #$00
    sta.b $34
    lda.b #$40
    sta.b $35
    lda.b #$C4
    sta.b $32
    lda.b #$4E
    sta.b $33
    rts
ViewParamsBobLong:                      ; 7E:F737, not reached in the traces so far
    jsr.w ObjectLinkEventTrigger2_bob
    rtl
ViewParamsTwo:
    lda.b #$00
    sta.b $2A
    lda.b #$00
    sta.b $2B
    lda.b #$8F
    sta.b $2E
    lda.b #$00
    sta.b $2F
    lda.b #$00
    sta.b $2C
    lda.b #$00
    sta.b $2D
    lda.b #$50
    sta.b $30
    lda.b #$00
    sta.b $31
    lda.b #$48
    sta.b $CE
    lda.b #$00
    sta.b $CF
    lda.b #$28
    sta.b $D0
    lda.b #$00
    sta.b $D1
    lda.b #$66
    sta.b $34
    lda.b #$66
    sta.b $35
    lda.b #$C7
    sta.b $32
    lda.b #$71
    sta.b $33
    rts

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

