; time_bonus.asm: the race time on screen and the time bonus counted into it,
; drawn by the Super FX digit program $07:F15B. $0E70 is the race time (BCD,
; see time.asm) and $0E74 a bonus (BCD) counted into it; $0D6B steps an
; animation whose digit size comes from a table.
; C: src/stuntracefx/race/time_bonus.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 07:919D GsuTimeDigitsGrow
; WHAT IT DOES: Draws the race time (GsuTimeDigits) at the digit size of
; animation step $0D6B, from the table at $07:918F ($08, $10, $18, $1C,
; then $20), with the renderer bytes 0, $BF, $B5, $BB.
; HOW: The size word to $70:0210, GsuBytes200_BFB5BB, then the code of
; GsuTimeDigits, which follows.
; Entry: M 16-bit, X 16-bit, D = $0000; DB = the bank of the digit table.
; Exit: as GsuTimeDigits.
%at($07919D)
GsuTimeDigitsGrow:
    ldx.w !wram_0D6B
    lda.l !digit_size_grow,x
    bra GsuTimeDigitsSize

; @routine 07:91A6 GsuTimeDigitsFull
; WHAT IT DOES: As GsuTimeDigitsGrow, at the full digit size $20.
; HOW: Enters GsuTimeDigitsGrow's tail with A = $20.
; Entry: as GsuTimeDigitsGrow. Exit: as GsuTimeDigits.
GsuTimeDigitsFull:
    lda.w #$0020
GsuTimeDigitsSize:
    sta.l !gsu_digit_size
    jsl.l GsuBytes200_BFB5BB

; @routine 07:91B1 GsuTimeDigits
; WHAT IT DOES: Draws the race time at $0E70 (BCD) with the Super FX digit
; program: the tens and units digits at X $AC, Y 8, and when the time is
; $100 or more the hundreds digit first at X $9E.
; HOW: $70:020A = $AC, $70:020C = 8, $70:020E = $10; each digit sets
; $70:0216 from the table at $8E3E (GsuParam216A) and runs program
; $07:F15B (GsuRun). The digit size at $70:0210 is the caller's. The
; hundreds digit is not reached in the traces so far.
; Entry: M 16-bit, X 16-bit, D = $0000; DB = the bank of the digit table.
; Exit: A = the units digit's table word's high byte and the screen mode
; value; X = $F15B; Y unchanged; M 16-bit, X 16-bit; C clear; N, Z from the
; data bank; V unchanged.
GsuTimeDigits:
    lda.w #$00AC
    sta.l !gsu_digit_x
    lda.w #$0008
    sta.l !gsu_digit_y
    lda.w #$0010
    sta.l !gsu_digit_step
    lda.w !race_time_bcd
    cmp.w #$0100
    bcc .tens
    xba                             ; not reached in the traces so far
    and.w #$000F
    jsr.w GsuParam216A
    lda.w #$009E
    sta.l !gsu_digit_x
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    lda.w #$00AC
    sta.l !gsu_digit_x
    lda.w !race_time_bcd
.tens:
    lsr a
    lsr a
    lsr a
    lsr a
    and.w #$000F
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w !race_time_bcd
    and.w #$000F
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F15B
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts

; @routine 07:922F GsuBonusDigitsGrow
; WHAT IT DOES: Sets the digit size for animation step $0D6B from the table
; at $07:918F; then, when $0E6C is nonzero, draws the two digits of $0E74
; with the renderer bytes 0, $BF, $B7, $B6.
; HOW: The size word to $70:0210; GsuBytes200_BFB7B6 and the code of
; GsuDigitPair0E74, which follows.
; Entry: M 16-bit, X 16-bit, D = $0000; DB = the bank of the digit table.
; Exit: when $0E6C is 0, A = 0, X = $0D6B, Z set, N clear; otherwise as
; GsuDigitPair0E74.
%at($07922F)
GsuBonusDigitsGrow:
    ldx.w !wram_0D6B
    lda.l !digit_size_grow,x
    bra GsuBonusDigitsSize

; @routine 07:9238 GsuBonusDigitsBounce
; WHAT IT DOES: As GsuBonusDigitsGrow, with the size from the table at
; $07:92AD ($08, $10, $14, $18, $18, $14, $10, $08).
; HOW: Enters GsuBonusDigitsGrow's tail. No caller is known; not reached
; in the traces so far.
; Entry: as GsuBonusDigitsGrow. Exit: as GsuBonusDigitsGrow.
GsuBonusDigitsBounce:
    ldx.w !wram_0D6B
    lda.l !digit_size_bounce,x
    bra GsuBonusDigitsSize

; @routine 07:9241 GsuBonusDigitsFull
; WHAT IT DOES: As GsuBonusDigitsGrow, at the full digit size $20.
; HOW: Enters GsuBonusDigitsGrow's tail with A = $20. No caller is known;
; not reached in the traces so far.
; Entry: as GsuBonusDigitsGrow. Exit: when $0E6C is 0, A = 0, X
; unchanged, Z set, N clear; otherwise as GsuDigitPair0E74.
GsuBonusDigitsFull:
    lda.w #$0020
GsuBonusDigitsSize:
    sta.l !gsu_digit_size
    lda.w !wram_0E6C
    bne .draw
    rts
.draw:                              ; continues in GsuDigitPair0E74
    jsl.l GsuBytes200_BFB7B6

; @routine 07:8E90 TimeBonusTally
; WHAT IT DOES: One frame of the time bonus animation on the race screen,
; a state machine in $0D65 (0, 2, 4, ... 24): the digits of the race time
; and of the bonus at $0E74 (shown when $0E6C is nonzero) grow in, $0E74 is
; counted into the time ($0E7C at a time, by the size of the bonus) with
; sounds, the digits shrink away; states 18-24 animate a second set of
; pieces. $0D6B steps the animation (by 2), $0D6D and $0D6F place it.
; HOW: Sets $0D6F to 6 and jumps through the table at $07:8E9F by $0D65.
; 0: idle. 2: start (step -2, $0E74 = $0E6C). 4: grow the time; at step 8
; choose $0E7C (1 below $32, 2 below $48, 3 below $64, else 4) with sound
; $0B. 6: count, until $0D5A reaches 8, then add the rest, step $0D5B
; down and sound $27. 8: shrink the bonus digits ($0D6D = $8F less the
; table at $07:9806), then idle. 10-16: the same for a time taken back
; into $0E74 (3 at a time, 1 below 9, up to $99) with the bonus digit
; bytes from TimeDigitBytes and the mirrored frame. 18: start the
; second animation when $19F0 is set ($0E74 = $28, sound $0B). 20: grow
; it to step 10. 22: hold it. 24: shrink it, sound $27 each step.
; Counting runs in decimal mode, so a sound queued from it computes its
; ring slot in decimal. States 10-16 are not reached in the traces so
; far, nor the bonus paths ($0E74 or $0E6C nonzero), the counting step of
; state 6, the idle end of state 18 or the advance of state 20. The 18
; bytes at $07:917D, a copy of the drawing tail of state 22 that nothing
; jumps to, stay in the raw intake.
; Entry: any widths; DB = a bank where the work RAM and the digit tables
; are reached. Exit: P unchanged; A and X as the last call or store left
; them; Y as the drawing calls leave it.
%at($078E90)
TimeBonusTally:
    php
    rep.b #$30
    lda.w #$0006
    sta.w !wram_0D6F
    ldx.w !time_bonus_state
    jmp.w (!time_bonus_states,x)

; The state handlers; the table between them stays in the raw intake. Each
; returns with the PLP that matches TimeBonusTally's PHP.
%at($078EB9)
.start:                             ; state 2
    lda.w #$0006
    sta.w !wram_0D6F
    inc.w !time_bonus_state
    inc.w !time_bonus_state
    lda.w #$FFFE
    sta.w !wram_0D6B
    lda.w !wram_0E6C
    sta.w !wram_0E74
.idle:                              ; state 0
    plp
    rts
.grow:                              ; state 4
    lda.w !wram_0D6B
    cmp.w #$0008
    bne .growStep
    inc.w !time_bonus_state
    inc.w !time_bonus_state
    lda.w !wram_0E74
    beq .setStep
    pha                             ; not reached in the traces so far
    lda.w #$000B
    jsl.l SoundPort0QueuePushRaw
    pla
    cmp.w #$0016
    bcs .step32
    lda.w #$0001
    bra .setStep
.step32:
    cmp.w #$0032
    bcs .step48
    lda.w #$0001
    bra .setStep
.step48:
    cmp.w #$0048
    bcs .step64
    lda.w #$0002
    bra .setStep
.step64:
    cmp.w #$0064
    bcs .step4
    lda.w #$0003
    bra .setStep
.step4:
    lda.w #$0004
.setStep:
    sta.w !time_bonus_step
    jmp.w .drawTime
.growStep:
    inc a
    inc a
    sta.w !wram_0D6B
    bra .drawTime
.count:                             ; state 6
    lda.w !wram_0D5A
    and.w #$00FF
    cmp.w #$0008
    bcc .countStep
    sed
    lda.w !race_time_bcd
    clc
    adc.w !wram_0E74
    sta.w !race_time_bcd
    stz.w !wram_0E74
    cld
    dec.w !wram_0D5B
    inc.w !time_bonus_state
    inc.w !time_bonus_state
    lda.w !wram_0E6C
    beq .counted
    lda.w #$0027                    ; not reached in the traces so far
    jsl.l SoundPort0QueuePushRaw
.counted:
    bra .drawTime
.countStep:
    sed
    lda.w !wram_0E74
    beq .countDone
    sec                             ; not reached in the traces so far
    sbc.w !time_bonus_step
    bcs .countPart
    lda.w !race_time_bcd
    clc
    adc.w !wram_0E74
    sta.w !race_time_bcd
    lda.w #$0000
    sta.w !wram_0E74
    bra .countDone
.countPart:
    sta.w !wram_0E74
    lda.w !race_time_bcd
    clc
    adc.w !time_bonus_step
    sta.w !race_time_bcd
.countDone:
    cld
    bra .drawTime
.shrink:                            ; state 8
    lda.w !wram_0D6B
    beq .shrunk
    dec a
    dec a
    sta.w !wram_0D6B
    tax
    lda.w #$008F
    sec
    sbc.l !fa5a_six_dx,x
    sta.w !wram_0D6D
    jsr.w GsuBonusDigitsGrow
    bra .drawFrame
.shrunk:
    stz.w !time_bonus_state
    plp
    rts
.drawTime:
    lda.w #$0030
    sta.w !wram_0D6D
    jsr.w GsuTimeDigitsGrow
    lda.w !wram_0E6C
    beq .noBonus
    jsr.w GsuBonusDigitsGrow        ; not reached in the traces so far
.noBonus:
    lda.w #$0037
    sta.w !wram_0D6D
.drawFrame:
    lda.w !wram_0E6C
    beq .pieces
    jsl.l GsuBytes200_BFB7B6        ; not reached in the traces so far
    jsr.w GsuFA5AThree
    jsr.w GsuFA5AFromTables
    jsr.w GsuFA5ASix
    jsr.w GsuFA35FromTables
    plp
    rts
.pieces:
    jsr.w GsuPiecesStep10CB
    plp
    rts
.takeStart:                         ; state 10, not reached in the traces so far
    lda.w #$0006
    sta.w !wram_0D6F
    inc.w !time_bonus_state
    inc.w !time_bonus_state
    lda.w #$FFFE
    sta.w !wram_0D6B
    lda.w #$0000
    sta.w !wram_0E74
    lda.w !race_time_bcd
    beq .takeNone
    sta.w !time_taken_copy
    sep.b #$20
    lda.b #$00
    sta.w !wram_0D5B
    plp
    rts
.takeNone:
    stz.w !time_bonus_state
    sep.b #$20
    lda.b #$00
    sta.w !wram_0D5B
    plp
    rts
.takeGrow:                          ; state 12, not reached in the traces so far
    lda.w !wram_0D6B
    cmp.w #$0008
    bne .takeGrowStep
    inc.w !time_bonus_state
    inc.w !time_bonus_state
    lda.w #$0040
    jsl.l SoundPort0QueuePushRaw
    bra .drawBonus
.takeGrowStep:
    inc a
    inc a
    sta.w !wram_0D6B
    tax
    lda.w #$008F
    sec
    sbc.l !fa5a_six_dx,x
    sta.w !wram_0D6D
.drawBonus:
    jsr.w TimeDigitBytes
    ldx.w !wram_0D6B
    lda.l !digit_size_grow,x
    sta.l !gsu_digit_size
    jsr.w GsuDigitPair0E74
    jmp.w .drawTimeFull
.take:                              ; state 14, not reached in the traces so far
    lda.w !race_time_bcd
    bne .takeSome
    bra .drawBonus
.takeSome:
    lda.w !wram_0E74
    cmp.w #$0099
    bcs .drawBonus
    sed
    lda.w !race_time_bcd
    cmp.w #$0009
    bcs .take3
    sec
    sbc.w #$0001
    sta.w !race_time_bcd
    cmp.w #$0000
    bne .took1
    lda.w #$0044
    jsl.l SoundPort0QueuePushRaw
.took1:
    lda.w !wram_0E74
    clc
    adc.w #$0001
    bra .took
.take3:
    sec
    sbc.w #$0003
    sta.w !race_time_bcd
    lda.w !wram_0E74
    clc
    adc.w #$0003
.took:
    cmp.w #$0099
    bcc .tookSet
    lda.w #$0044
    jsl.l SoundPort0QueuePushRaw
    lda.w #$0099
.tookSet:
    sta.w !wram_0E74
    cld
    bra .drawBonus
.takeShrink:                        ; state 16, not reached in the traces so far
    lda.w !wram_0D6B
    beq .takeShrunk
    dec a
    dec a
    sta.w !wram_0D6B
    lda.w #$0037
    sta.w !wram_0D6D
    jsr.w TimeDigitBytes
    ldx.w !wram_0D6B
    lda.l !digit_size_grow,x
    sta.l !gsu_digit_size
    jsr.w GsuDigitPair0E74
    jsr.w GsuTimeDigitsGrow
    bra .drawMirrored
.takeShrunk:
    stx.w !time_bonus_state
    plp
    rts
.drawTimeFull:
    jsr.w TimeDigitBytes
    lda.w #$0020
    sta.l !gsu_digit_size
    jsr.w TimeDigitBytes
    jsr.w GsuTimeDigits
.drawMirrored:
    jsl.l GsuBytes200_BFB7B6
    jsr.w GsuFA5AThreeShifted
    jsr.w GsuFA5AFromTables
    jsr.w GsuFA5ASixLeft
    jsr.w GsuFA35FromTables
    plp
    rts

; @routine 07:90EE TimeDigitBytes
; WHAT IT DOES: Chooses the renderer bytes at $70:0200 for the time
; digits: 0, $BF, $BA, $B9 when $0D4C is nonzero; otherwise by $0E76: $BF,
; $B5, $BB from $20, $BF, $BC, $BD from $10, else $BF, $B7, $B6.
; HOW: Compares and one of the GsuBytes200 setters. Not reached in the
; traces so far.
; Entry: M 16-bit, X 16-bit; DB = a bank where $0D4C and $0E76 are work
; RAM. Exit: A = the word read with its low byte the last renderer byte;
; X, Y unchanged; with $0D4C nonzero N, Z from it; otherwise N, Z, C from
; the last compare ($0E76 - $20 or - $10).
TimeDigitBytes:
    lda.w !digit62_override
    beq .byTime
    jsl.l GsuBytes200_BFBAB9
    rts
.byTime:
    lda.w !time_taken_copy
    cmp.w #$0020
    bcc .below20
    jsl.l GsuBytes200_BFB5BB
    bra .done
.below20:
    cmp.w #$0010
    bcc .below10
    jsl.l GsuBytes200_BFBCBD
    bra .done
.below10:
    jsl.l GsuBytes200_BFB7B6
.done:
    rts

; TimeBonusTally's states 18-24, after TimeDigitBytes.
%at($079116)
TimeBonusSecond:
.start:                             ; state 18
    lda.w !wram_19F0
    bne .begin
    stz.w !time_bonus_state            ; not reached in the traces so far
    plp
    rts
.begin:
    lda.w #$0030
    sta.w !wram_0D6F
    inc.w !time_bonus_state
    inc.w !time_bonus_state
    lda.w #$000B
    jsl.l SoundPort0QueuePushRaw
    stz.w !wram_0D6B
    lda.w #$0028
    sta.w !wram_0E74
    plp
    rts
.grow:                              ; state 20
    lda.w !wram_0D6B
    cmp.w #$0008
    bne .growStep
    inc.w !time_bonus_state            ; not reached in the traces so far
    inc.w !time_bonus_state
.growStep:
    inc a
    inc a
    sta.w !wram_0D6B
.draw:                              ; state 22
    lda.w #$0030
    sta.w !wram_0D6D
    jsr.w GsuFA5AThree
    jsr.w GsuFA5AFromTables
    jsr.w GsuFA5ASix
    jsr.w GsuFA1BFromTables
    plp
    rts
.shrink:                            ; state 24
    lda.w !wram_0D6B
    bne .shrinkStep
    stz.w !time_bonus_state
    plp
    rts
.shrinkStep:
    dec a
    dec a
    sta.w !wram_0D6B
    lda.w #$0027
    jsl.l SoundPort0QueuePushRaw
    bra .draw

; @routine 07:8BB3 TimeBonusTransfer
; WHAT IT DOES: While the byte at $0D5B is set, moves the time bonus at
; $19F0 into the race time at $0E70 (3 a frame, 1 below 5, $17 at a time
; from $18 up), showing what is left at $0E74 and starting TimeBonusTally's
; second animation (state 18) when it is idle; with no bonus, counts $0E74
; up towards $19EE and sends a running TimeBonusTally to state 24. Then
; one step of a small animation in $0D67 that grows the bonus digits ($0D69
; up by 4 to $20) and draws them.
; HOW: Decimal mode for the arithmetic. $0D67 indexes the table at
; $07:8C43: 0 nothing, 2 grow, 4 draw, 6 shrink (which steps $0D69 up, not
; down, and is not reached in the traces so far). Drawing is
; GsuBytes200_BFBAB9 and GsuDigitPair0E74 at size $0D69. The $17 step is
; not reached in the traces so far.
; Entry: D = $0000; DB = a bank where $0D5B-$19F0 are work RAM and the
; digit table is at $8E3E. Exit: when $0D5B is 0, A low byte 0, M 8-bit,
; Z set; otherwise M 16-bit, X 16-bit, D clear, and A, X and the flags as
; the last step leaves them (GsuDigitPair0E74 when it draws).
%at($078BB3)
TimeBonusTransfer:
    sep.b #$20
    lda.w !wram_0D5B
    bne .active
    rts
.active:
    rep.b #$30
    sed
    lda.w !wram_19F0
    bne .transfer
    stz.w !time_bonus_step
    lda.w !time_bonus_state
    beq .countUp
    lda.w #$0018
    sta.w !time_bonus_state
    beq .dispatch
.countUp:
    lda.w !wram_19EE
    cmp.w !wram_0E74
    beq .dispatch
    lda.w !wram_0E74
    clc
    adc.w #$0001
    sta.w !wram_0E74
    bra .dispatch
.transfer:
    cmp.w #$0018
    bcc .below18
    sec                             ; not reached in the traces so far
    sbc.w #$0017
    sta.w !wram_19F0
    sta.w !wram_0E74
    lda.w !race_time_bcd
    clc
    adc.w #$0017
    sta.w !race_time_bcd
    bra .start
.below18:
    cmp.w #$0005
    bcs .by3
    sec
    sbc.w #$0001
    sta.w !wram_19F0
    sta.w !wram_0E74
    lda.w !race_time_bcd
    clc
    adc.w #$0001
    sta.w !race_time_bcd
    bra .start
.by3:
    sec
    sbc.w #$0003
    sta.w !wram_19F0
    sta.w !wram_0E74
    lda.w !race_time_bcd
    clc
    adc.w #$0003
    sta.w !race_time_bcd
.start:
    lda.w !time_bonus_state
    bne .dispatch
    lda.w #$0012
    sta.w !time_bonus_state
.dispatch:
    cld
    ldx.w !bonus_digit_state
    jmp.w (!bonus_digit_states,x)

; TimeBonusTransfer's digit states; the table before them stays in the raw
; intake.
%at($078C4B)
.grow:                              ; state 2
    lda.w !bonus_digit_size
    cmp.w #$0020
    bcc .larger
    inc.w !bonus_digit_state
    inc.w !bonus_digit_state
    bra .draw
.larger:
    lda.w !bonus_digit_size
    clc
    adc.w #$0004
    sta.w !bonus_digit_size
    bra .draw
.shrink:                            ; state 6, not reached in the traces so far
    lda.w !bonus_digit_size
    bne .smaller
    stz.w !bonus_digit_state
    rts
.smaller:
    inc.w !bonus_digit_size
.draw:                              ; state 4
    jsl.l GsuBytes200_BFBAB9
    lda.w !bonus_digit_size
    sta.l !gsu_digit_size
    jsr.w GsuDigitPair0E74
.idle:                              ; state 0
    rts

; @routine 07:8C82 TimeLeftDigits
; WHAT IT DOES: Draws the race time at $0E70 (BCD) with Super FX program
; $07:F6DF while the byte at $0D5B is set and player 1's engine phase
; ($0DD6) is not $0C: two digits at X $AC, Y 8 (renderer bytes $BF, $BC,
; $BD below $20, else $BF, $B5, $BB); below 10 one large digit with
; program $07:FAF2, placed and scaled from the table at $07:8DDA by the
; high four bits of $0E69, with sound $42 (for player 1) each time it
; changes. $0E78 keeps the value last drawn.
; HOW: GsuParam216A per digit and GsuRun. The large digit's table entry is
; four words: $70:020E and $70:0210, $70:0212 and $70:0214, X, Y. A time
; of 0 only clears $0E78; times of $100 and more draw three digits from X
; $9E. Neither is reached in the traces so far.
; Entry: D = $0000; DB = a bank where the work RAM and the tables at $8DDA
; and $8E3E are reached. Exit: when $0D5B is 0, A low byte 0, M 8-bit, Z
; set; when the phase is $0C, A low byte $0C, M 8-bit, Z and C set;
; otherwise M 16-bit, X 16-bit, and A, X and the flags as the last
; instruction leaves them (after a draw: A = the table word's high byte
; and the screen mode value, X = the program, N and Z from the data bank).
%at($078C82)
TimeLeftDigits:
    sep.b #$20
    lda.w !wram_0D5B
    bne .active
    rts
.active:
    lda.w !engine_phase_p1
    cmp.b #$0C
    bne .shown
    rts
.shown:
    rep.b #$30
    lda.w !race_time_bcd
    beq .zero
    cmp.w #$0100
    bcc .below100
    jmp.w .three                    ; not reached in the traces so far
.below100:
    cmp.w #$0010
    bcs .two
    jmp.w .one
.zero:
    lda.w !time_left_shown          ; not reached in the traces so far
    beq .zeroDone
    stz.w !time_left_shown
.zeroDone:
    rts
.two:
    sta.w !time_left_shown
    lsr a
    lsr a
    lsr a
    lsr a
    jsr.w GsuParam216A
    lda.w !time_left_shown
    cmp.w #$0020
    bcs .twoHigh
    jsl.l GsuBytes200_BFBCBD
    bra .twoDraw
.twoHigh:
    jsl.l GsuBytes200_BFB5BB
.twoDraw:
    lda.w #$00AC
    sta.l !gsu_digit_x
    lda.w #$0008
    sta.l !gsu_digit_y
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w !race_time_bcd
    and.w #$000F
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts
.three:
    sta.w !time_left_shown          ; not reached in the traces so far
    xba
    and.w #$000F
    jsr.w GsuParam216A
    jsl.l GsuBytes200_BFB5BB
    lda.w #$009E
    sta.l !gsu_digit_x
    lda.w #$0008
    sta.l !gsu_digit_y
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w #$00AC
    sta.l !gsu_digit_x
    lda.w !time_left_shown
    and.w #$00FF
    lsr a
    lsr a
    lsr a
    lsr a
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rep.b #$30
    lda.w !time_left_shown
    and.w #$000F
    jsr.w GsuParam216A
    sep.b #$20
    rep.b #$10
    ldx.w #$F6DF
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts
.one:
    cmp.w !time_left_shown
    beq .oneDraw
    sta.w !time_left_shown
    lda.w #$0042
    jsl.l SoundPort0QueuePush
.oneDraw:
    lda.w !race_time_bcd
    jsr.w GsuParam216A
    rep.b #$30
    lda.w !wram_0E69
    and.w #$00FF
    lsr a
    lsr a
    lsr a
    lsr a
    asl a
    asl a
    asl a
    tax
    lda.w !time_digit_large,x
    sta.l !gsu_digit_step
    sta.l !gsu_digit_size
    lda.w !time_digit_large+2,x
    sta.l !gsu_in_212
    sta.l !gsu_in_214
    lda.w !time_digit_large+4,x
    sta.l !gsu_digit_x
    lda.w !time_digit_large+6,x
    sta.l !gsu_digit_y
    jsl.l GsuBytes200_BFB7B6
    sep.b #$20
    rep.b #$10
    ldx.w #$FAF2
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    rts
