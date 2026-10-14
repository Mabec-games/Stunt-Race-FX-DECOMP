; player_stats.asm: the per-player race statistics the HUD and the results
; show, taken each frame from the player car's Super FX record, and the
; race phase dispatchers that run after them.
; C: src/stuntracefx/race/player_stats.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:DDCE SpeedSampleAdd
; WHAT IT DOES: Turns speed A into a speedometer sample, |A| * $3C / 256
; kept to $FF, stores it as player Y's sample ($0DDA + Y), and adds it to
; that player's 32-bit total (player 0: $0DDC-$0DDF, else $0DE0-$0DE3),
; counting the samples in $0DE4 (player 0) or $0DE6. A total that would
; pass $FFFFFFFF stays at $FFFFFFFF and the sample is not counted (for
; player 1 only the low word is set to $FFFF, stored twice).
; HOW: The product comes from the PPU multiplier ($211B twice, $211C, then
; $2135-$2136). The clamp and the overflow cases are not reached in the
; traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; A = speed (signed), Y = player.
; Exit: A = the total's high word; M 16-bit; X, Y unchanged. Counted: N, Z
; from the new count, C clear, V from the high word's add. Kept at the
; limit: A = $FFFF, N, C set, Z, V clear.
%at($07DDCE)
SpeedSampleAdd:
    cmp.w #$8000
    bcc .positive
    eor.w #$FFFF
    inc a
.positive:
    sep.b #$20
    sta.w !M7A
    xba
    sta.w !M7A
    lda.b #$3C
    sta.w !M7B
    rep.b #$20
    lda.w !MPYM
    cmp.w #$00FF
    bcc .sample
    lda.w #$00FF                    ; not reached in the traces so far
.sample:
    sep.b #$20
    sta.w !speed_sample,y
    cpy.w #$0000
    bne .player1
    rep.b #$20
    and.w #$00FF
    clc
    adc.w !speed_total_p0
    sta.w !speed_total_p0
    lda.w !speed_total_p0+2
    adc.w #$0000
    bcc .count0
    lda.w #$FFFF                    ; not reached in the traces so far
    sta.w !speed_total_p0           ; not reached in the traces so far
    sta.w !speed_total_p0+2         ; not reached in the traces so far
    rts                             ; not reached in the traces so far
.count0:
    sta.w !speed_total_p0+2
    inc.w !speed_count_p0
    rts
.player1:
    rep.b #$20
    and.w #$00FF
    clc
    adc.w !speed_total_p1
    sta.w !speed_total_p1
    lda.w !speed_total_p1+2
    adc.w #$0000
    bcc .count1
    lda.w #$FFFF                    ; not reached in the traces so far
    sta.w !speed_total_p1           ; not reached in the traces so far
    sta.w !speed_total_p1           ; not reached; stores the low word again
    rts                             ; not reached in the traces so far
.count1:
    sta.w !speed_total_p1+2
    inc.w !speed_count_p1
    rts

; @routine 07:DD60 PlayerStatsFromRecord
; WHAT IT DOES: Unless $0E57 is set, copies player Y's figures from its
; car's Super FX record X: the bytes of +$40-$43 to $0DEC, $0DEE, $0DF0 and
; $0DF2 (+ Y), +$4E to $0DF6, +$82 / 8 to $0DF9 and its gauge position
; ($30 + 2 * that, kept to $3F first) to $0DFB, a speedometer sample of
; +$4A (SpeedSampleAdd), +$6A / 8 to $0E05, +$7A to $0DE8 and the low
; nibble of +$88 to $0DF4. Then GsuRecordBits60.
; HOW: Long reads of Super FX RAM in that order, byte stores through the
; data bank.
; Entry: M 8-bit, X 16-bit, D = $0000; X = record, Y = player. Exit: A as
; GsuRecordBits60 leaves it; X, Y unchanged; M 8-bit; skipped: N, Z from
; $0E57; else N, Z from the +$88 nibble, C from the last shift of +$6A,
; V as SpeedSampleAdd leaves it.
%at($07DD60)
PlayerStatsFromRecord:
    lda.w !wram_0E57
    bne .skip
    lda.l !gsu_rec_40,x
    sta.w !player_rec_40,y
    lda.l !gsu_rec_41,x
    sta.w !player_rec_41,y
    lda.l !gsu_rec_42,x
    sta.w !player_rec_42,y
    lda.l !gsu_rec_43,x
    sta.w !player_rec_43,y
    lda.l !gsu_rec_4e,x
    sta.w !player_rec_4e,y
    rep.b #$20
    lda.l !gsu_rec_82,x
    lsr a
    lsr a
    lsr a
    sep.b #$20
    sta.w !gauge_b4_value,y
    cmp.b #$3F
    bcc .gauge
    lda.b #$3F
.gauge:
    asl a
    adc.b #$30
    sta.w !kept_sub,y
    rep.b #$20
    lda.l !gsu_speeds,x
    jsr.w SpeedSampleAdd
    lda.l !gsu_in_6a,x
    lsr a
    lsr a
    lsr a
    sep.b #$20
    sta.w !kept_enable,y
    lda.l !gsu_rec_7a,x
    and.b #$FF
    sta.w !player_rec_7a,y
    lda.l !gsu_rec_88,x
    and.b #$0F
    sta.w !player_mode,y
.skip:
    jsl.l GsuRecordBits60
    rts

; @routine 07:E22B PhaseDispatchE232
; WHAT IT DOES: Runs the handler for race phase $0DD6 from the table at
; $07:E232.
; HOW: JMP (table,X); the handler returns to the caller, or through its
; caller's frame (jump_routine).
; Entry: any widths. Exit: as the handler leaves it.
%at($07E22B)
PhaseDispatchE232:
    lda.w !engine_phase_p1
    tax
    jmp.w (!phase_handlers_e232,x)

; @routine 07:E246 PhaseDispatchE24F
; WHAT IT DOES: Runs the handler for race phase $0DD6 from the table at
; $07:E24F, with 8-bit registers.
; HOW: SEP #$30, then JMP (table,X).
; Entry: any widths. Exit: as the handler leaves it.
%at($07E246)
PhaseDispatchE24F:
    sep.b #$30
    lda.w !engine_phase_p1
    tax
    jmp.w (!phase_handlers_e24f,x)

; @routine 07:DD47 PlayerStatsAndPaletteAnimate
; WHAT IT DOES: Player 0's figures from its record (PlayerStatsFromRecord),
; then $0E5F = $FF.
; HOW: P saved and restored.
; Entry: any widths, D = $0000. Exit: A low byte = $FF; X = the record;
; Y = 0; P unchanged.
%at($07DD47)
PlayerStatsAndPaletteAnimate:
    php
    rep.b #$30
    ldy.w #$0000
    lda.l !gsu_match_2476
    tax
    sep.b #$20
    jsr.w PlayerStatsFromRecord
    sep.b #$20
    lda.b #$FF
    sta.w !palette_animate
    plp
    rtl

; @routine 07:DCEC PlayerStatsThenPhaseE232
; WHAT IT DOES: Player 0's figures from its record, then the race phase
; handler from the table at $07:E232 with Y = 0.
; HOW: PlayerStatsFromRecord, SEP #$10, then PhaseDispatchE232.
; Entry: any widths, D = $0000. Exit: as the phase handler leaves it.
%at($07DCEC)
PlayerStatsThenPhaseE232:
    rep.b #$30
    ldy.w #$0000
    lda.l !gsu_match_2476
    tax
    sep.b #$20
    jsr.w PlayerStatsFromRecord
    sep.b #$10
    ldy.b #$00
    jsr.w PhaseDispatchE232
    rts

; @routine 07:DD03 PlayerStatsThenPhaseDD35
; WHAT IT DOES: Player 0's figures from its record; then, when $10CD is set
; and the race phase $0DD6 is 8 or more, starts the fade out ($0E3A = 1)
; and, while bit 7 of $0D61 is set, goes on to $07:EBE9; otherwise runs the
; race phase handler from the table at $07:DD35.
; HOW: PlayerStatsFromRecord, SEP #$10, then the checks; the handlers and
; $07:EBE9 are entered by JMP and return to the caller.
; Entry: any widths, D = $0000. Exit: fading with bit 7 of $0D61 clear: A
; low byte = $0D61, N, Z from it, C set; otherwise as the handler leaves it.
%at($07DD03)
PlayerStatsThenPhaseDD35:
    rep.b #$30
    ldy.w #$0000
    lda.l !gsu_match_2476
    tax
    sep.b #$20
    jsr.w PlayerStatsFromRecord
    sep.b #$10
    lda.w !wram_10CD
    beq .phase
    lda.w !engine_phase_p1
    cmp.b #$08
    bcc .phase
    lda.b #$01
    sta.w !fade_out
    lda.w !fade_level_hi
    bpl .done
    jmp.w !race_fade_tail
.done:
    rts
.phase:
    lda.w !engine_phase_p1
    tax
    jmp.w (!phase_handlers_dd35,x)

; @routine 07:DA02 RaceSoundsPlayer1
; WHAT IT DOES: Player 1's share of a two-player race frame: the soft reset
; check, both players' stats from their records (PlayerStatsFromRecord, Y =
; 0 with X = $70:2476, Y = 1 with X = $70:2478), player 1's engine sounds
; ($1A5D = 0), the race phase (PhaseDispatchE24F); then $0D1A cleared. Its
; X-press block (RaceXPressPlayer1) is never run: the test loads 0 after its
; BIT, so the branch always passes it.
; HOW: Data bank $07, its own; P and the data bank put back.
; Entry: any widths. Exit: P and the data bank unchanged; A = 0.
%at($07DA02)
RaceSoundsPlayer1:
    php
    phb
    phk
    plb
    sep.b #$30
    jsr.w SoftResetCheck
    rep.b #$30
    ldy.w #$0000
    lda.l !gsu_match_2476
    tax
    sep.b #$20
    jsr.w PlayerStatsFromRecord
    rep.b #$30
    ldy.w #$0001
    lda.l !gsu_match_2478
    tax
    sep.b #$20
    jsr.w PlayerStatsFromRecord
    sep.b #$10
    ldy.b #$00
    sty.w !current_player
    jsl.l EngineSoundsPlayer1
    jsr.w PhaseDispatchE24F
    rep.b #$20
    lda.w !joy1_pressed_copy
    bit.w #$0040
    lda.w #$0000
    beq RaceXPressPlayer1_end

; @routine 07:DA44 RaceXPressPlayer1
; WHAT IT DOES: RaceSoundsPlayer1's X-press block, which nothing reaches:
; the palette's first colour $1CC4, $0D22 = $0D23 = $0C, $0D5C = $FF, the
; sprites parked (OamHighBits18Park48); then $0D1A cleared and the frame's
; end.
; HOW: As inside RaceSoundsPlayer1; ends pulling the data bank and P its
; start pushed.
; Entry: M 16-bit, X 8-bit, data bank $07; the data bank and P on the
; stack above the return address. Exit: as RaceSoundsPlayer1.
%at($07DA44)
RaceXPressPlayer1:
    lda.w #$1CC4
    sta.l !palette_buffer
    sep.b #$20
    lda.b #$0C
    sta.w !wram_0D22
    sta.w !wram_0D23
    lda.b #$FF
    sta.w !wram_0D5C
    jsr.w OamHighBits18Park48
    rep.b #$20
RaceXPressPlayer1_end:
    stz.w !joy1_pressed_copy
    plb
    plp
    rtl

; @routine 07:DA65 RaceSoundsPlayer2
; WHAT IT DOES: Player 2's share of a two-player race frame: player 2's
; engine sounds ($1A5D = 1) and 07:E265; then $0D1C cleared. Its X-press
; block (RaceXPressPlayer2) is never run, as RaceSoundsPlayer1's.
; HOW: As RaceSoundsPlayer1.
; Entry: any widths. Exit: P and the data bank unchanged; A = 0.
%at($07DA65)
RaceSoundsPlayer2:
    php
    phb
    phk
    plb
    sep.b #$30
    ldy.b #$01
    sty.w !current_player
    jsl.l EngineSoundsPlayer2
    sep.b #$10
    jsr.w PlayerTwoFrameWork
    rep.b #$20
    lda.w !joy2_pressed_copy
    bit.w #$0040
    lda.w #$0000
    beq RaceXPressPlayer2_end

; @routine 07:DA86 RaceXPressPlayer2
; WHAT IT DOES: RaceSoundsPlayer2's X-press block, which nothing reaches:
; the palette's first colour $1CC4, $0D22 = $0D23 = $0E, $0D5D = 1, the
; sprites parked (OamHighBits58Park112); then $0D1C cleared and the frame's
; end.
; HOW: As RaceXPressPlayer1.
; Entry: as RaceXPressPlayer1. Exit: as RaceSoundsPlayer2.
%at($07DA86)
RaceXPressPlayer2:
    lda.w #$1CC4
    sta.l !palette_buffer
    sep.b #$20
    lda.b #$0E
    sta.w !wram_0D22
    sta.w !wram_0D23
    lda.b #$01
    sta.w !wram_0D5D
    jsr.w OamHighBits58Park112
    rep.b #$20
RaceXPressPlayer2_end:
    stz.w !joy2_pressed_copy
    plb
    plp
    rtl

; @routine 07:DE46 PlayerOnePhaseStep
; WHAT IT DOES: Player 1's race phase each frame: the stats from the record
; (PlayerStatsFromRecord, Y = 0, X = $70:2476), then the handler for the
; phase $0DD6 from the table after the dispatch: 0, 2 and 4 into 07:E265's
; handlers ($E285, $E2E4, $E404), 6 and 8 PlayerOnePhaseRace, 10
; PlayerOnePhaseTally, 12 07:ECA4, 14 PlayerOnePhaseEnd.
; HOW: JMP (abs,X) through the table of eight words at $07:DE5E; M and X
; 8-bit for the dispatch.
; Entry: any widths; data bank $07. Exit: as the handler leaves it.
%at($07DE46)
PlayerOnePhaseStep:
    rep.b #$30
    ldy.w #$0000
    lda.l !gsu_match_2476
    tax
    sep.b #$20
    jsr.w PlayerStatsFromRecord
    sep.b #$30
    lda.w !engine_phase_p1
    tax
    jmp.w (PlayerOnePhaseStep_handlers,x)
PlayerOnePhaseStep_handlers:
    dw $E285,$E2E4,$E404,$DE6E,$DE6E,$DFD2,$ECA4,$E044

; @routine 07:DE6E PlayerOnePhaseRace
; WHAT IT DOES: The race phase: the pause check (PauseOnStart); while the
; race time $0E70 runs, a new lap ($0DE8 against $0DEA) plays sound $35 and
; counts $0DF8 and the lead $0FAE up. At time 0: sounds $02 (raw) and $15,
; the sprites hidden for the HUD mode, phase $0A ($0DD6, $0D53), $0D5B and
; $0E5B cleared, and the result's glyph sprites laid out: positions $0FB0
; and $0FE0 + X for three rows of 7, 8 and 9 glyphs (y $34, $4C, $60, x
; $38) from the glyphs at $07:DFA2, the two digits of $0E74 as glyphs
; $1400 + 2n at $1016 and $1014, and the lead's words by $0FAE (0, 1, more).
; HOW: 8-bit A for the flags, 16-bit for the sprites; ends with RTS.
; Entry: M 8-bit, X 8-bit; data bank $07. Exit: A, X, Y as the last step
; left them.
%at($07DE6E)
PlayerOnePhaseRace:
    jsr.w PauseOnStart
    rep.b #$20
    lda.w !race_time_bcd
    beq PlayerOnePhase_lDE92
    sep.b #$20
    lda.w !player_rec_7a
    cmp.w !wram_0DEA
    beq PlayerOnePhase_lDE91
    sta.w !wram_0DEA
    lda.b #$35
    jsl.l SoundPort0QueuePush
    inc.w !wram_0DF8
    inc.w !p1_lead+11
PlayerOnePhase_lDE91:
    rts
PlayerOnePhase_lDE92:
    sep.b #$20
    lda.b #$02
    jsl.l SoundPort0QueuePushRaw
    lda.b #$15
    jsl.l SoundPort0QueuePush
    jsr.w OamHideForHudMode
    lda.b #$0A
    sta.w !engine_phase_p1
    sta.w !wram_0D53
    stz.w !wram_0D5B
    stz.w !wram_0E5B
    rep.b #$20
    lda.w #$0004
    sta.w !glyph_spr_x
    lda.w #$0008
    sta.w !glyph_spr_y
    rep.b #$30
    ldx.w #$0002
    ldy.w #$0007
PlayerOnePhase_lDEC7:
    lda.w #$0034
    sta.w !glyph_spr_y,x
    lda.w #$0038
    sta.w !glyph_spr_x,x
    lda.l PlayerOnePhase_glyphs-2,x
    sta.w !glyph_spr_glyph,x
    inx
    inx
    dey
    bne PlayerOnePhase_lDEC7
    ldy.w #$0008
PlayerOnePhase_lDEE2:
    lda.w #$004C
    sta.w !glyph_spr_y,x
    lda.w #$0038
    sta.w !glyph_spr_x,x
    lda.l PlayerOnePhase_glyphs-2,x
    sta.w !glyph_spr_glyph,x
    inx
    inx
    dey
    bne PlayerOnePhase_lDEE2
    ldy.w #$0009
PlayerOnePhase_lDEFD:
    lda.w #$0060
    sta.w !glyph_spr_y,x
    lda.w #$0038
    sta.w !glyph_spr_x,x
    lda.l PlayerOnePhase_glyphs-2,x
    sta.w !glyph_spr_glyph,x
    inx
    inx
    dey
    bne PlayerOnePhase_lDEFD
    lda.w !wram_0E74
    and.w #$000F
    asl a
    clc
    adc.w #$1400
    sta.w !glyph_spr_glyph+6
    lda.w !wram_0E74
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$000F
    asl a
    clc
    adc.w #$1400
    sta.w !glyph_spr_glyph+4
    lda.w !p1_lead+11
    beq PlayerOnePhase_lDF5F
    cmp.w #$0002
    bcs PlayerOnePhase_lDF71
    lda.w #$9610
    sta.w !glyph_spr_glyph+18
    lda.w #$960E
    sta.w !glyph_spr_glyph+20
    lda.w #$941C
    sta.w !glyph_spr_glyph+22
    lda.w #$0000
    sta.w !glyph_spr_glyph+30
    lda.w #$980E
    sta.w !glyph_spr_1040
    bra PlayerOnePhase_lDF71
PlayerOnePhase_lDF5F:
    lda.w #$960E
    sta.w !glyph_spr_glyph+18
    lda.w #$9610
    sta.w !glyph_spr_glyph+20
    lda.w #$0000
    sta.w !glyph_spr_glyph+22
PlayerOnePhase_lDF71:
    rts
PlayerOnePhase_limits:
    dw $0054,$0060,$0068,$0074,$007C,$0084,$008C,$0054
    dw $0060,$0068,$0070,$007C,$0084,$008C,$0094,$0060
    dw $0068,$0070,$0078,$0080,$008C,$0094,$009C,$00A4
PlayerOnePhase_glyphs:
    dw $9A00,$9400,$9400,$9618,$941C,$9418,$9808,$9A00
    dw $961A,$9800,$9610,$960A,$9414,$9612,$9618,$941C
    dw $9802,$961A,$9616,$9414,$9418,$9414,$9616,$9618

; @routine 07:DFD2 PlayerOnePhaseTally
; WHAT IT DOES: The result's tally phase: slides the glyph sprites' x
; ($0FB0 + 2..$30) right by $0B up to their limits at $07:DF72; once the
; first passes $AC, sets the time $0E70 from $0E74, $0E58 = $40, phase $0E,
; $0D65 = $0C, $0D6F = 6, $0D6B = $FFFE and $0E74 from $0E6C. Then draws:
; $70:0210 = $20, GsuBytes200_BFBAB9, the digit pair at the first x and
; every glyph sprite (GlyphSpritesDrawAll).
; HOW: 16-bit registers; the limits read with long loads.
; Entry: M 8-bit, X 8-bit; data bank $07. Exit: as GlyphSpritesDrawAll.
%at($07DFD2)
PlayerOnePhaseTally:
    rep.b #$30
    ldx.w #$0002
PlayerOnePhase_lDFD7:
    lda.w !glyph_spr_x,x
    clc
    adc.w #$000B
    cmp.l PlayerOnePhase_limits-2,x
    bcc PlayerOnePhase_lDFE8
    lda.l PlayerOnePhase_limits-2,x
PlayerOnePhase_lDFE8:
    sta.w !glyph_spr_x,x
    inx
    inx
    cpx.w #$0032
    bcc PlayerOnePhase_lDFD7
    lda.w !glyph_spr_x
    clc
    adc.w #$000B
    cmp.w #$00AC
    bcc PlayerOnePhase_lE02C
    sep.b #$20
    lda.w !wram_0E74
    sta.w !race_time_bcd
    lda.b #$40
    sta.w !wram_0E58
    lda.b #$0E
    sta.w !engine_phase_p1
    lda.b #$0C
    sta.w !time_bonus_state
    rep.b #$20
    lda.w #$0006
    sta.w !wram_0D6F
    lda.w #$FFFE
    sta.w !wram_0D6B
    lda.w !wram_0E6C
    sta.w !wram_0E74
    lda.w #$00AC
PlayerOnePhase_lE02C:
    sta.w !glyph_spr_x
    lda.w #$0020
    sta.l !fx_draw_rows
    jsl.l GsuBytes200_BFBAB9
    lda.w !glyph_spr_x
    jsr.w GsuDigitPair0E74At
    jsr.w GlyphSpritesDrawAll
    rts

; @routine 07:E044 PlayerOnePhaseEnd
; WHAT IT DOES: The last phase: counts $0E58 down (sound $04 on port 1 at
; $0A; at 0 $19DF = 1 and WindowSlideUpSound); once it is 0 and the window
; slide is done ($19D8 = 5), keeps $0E74 at $0E6C, clears $0D4C, sets $0D48
; = 2, $0D89 from $0D8B and $18DB from $0D8C, and goes on at 07:EC6B.
; Otherwise draws the glyph sprites (GlyphSpritesDrawAll).
; HOW: 8-bit A; a long store to $18DB; a JMP to finish.
; Entry: M 8-bit, X 8-bit; data bank $07. Exit: as GlyphSpritesDrawAll or
; 07:EC6B leaves it.
%at($07E044)
PlayerOnePhaseEnd:
    lda.w !wram_0E58
    beq PlayerOnePhase_lE063
    cmp.b #$0A
    bne PlayerOnePhase_lE053
    lda.b #$04
    jsl.l SoundPort1QueuePush
PlayerOnePhase_lE053:
    dec.w !wram_0E58
    bne PlayerOnePhase_lE061
    lda.b #$01
    sta.w !wram_19DF
    jsl.l WindowSlideUpSound
PlayerOnePhase_lE061:
    bra PlayerOnePhase_lE06A
PlayerOnePhase_lE063:
    lda.w !window_hdma_off
    cmp.b #$05
    beq PlayerOnePhase_lE06E
PlayerOnePhase_lE06A:
    jsr.w GlyphSpritesDrawAll
    rts
PlayerOnePhase_lE06E:
    lda.w !wram_0E74
    sta.w !wram_0E6C
    stz.w !digit62_override
    lda.b #$02
    sta.w !wram_0D48
    lda.w !wram_0D8B
    sta.w !wram_0D89
    rep.b #$20
    lda.w !wram_0D8C
    sta.l !selection_handler_a
    jmp.w WindowDigitStepEC6B

; @routine 07:E08E PlayerOnePhaseReturn
; WHAT IT DOES: A lone RTS after PlayerOnePhaseEnd's jump, which nothing
; reaches.
; HOW: Returns.
; Entry: any. Exit: nothing changes.
%at($07E08E)
PlayerOnePhaseReturn:
    rts
