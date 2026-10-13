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
; HOW: JMP (table,X); the handler returns to the caller.
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
