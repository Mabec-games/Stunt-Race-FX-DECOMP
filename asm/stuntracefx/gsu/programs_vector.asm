; programs_vector.asm: two more S-CPU wrappers of Super FX programs on
; the vector in direct page $02, $08, $94.
; C: src/stuntracefx/gsu/programs_vector.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:AF9D Gsu8277
; WHAT IT DOES: Runs Super FX program $01:8277 on the vector in direct
; page $02, $08, $94 and puts its three results in direct page $B7, $B9,
; $BB.
; HOW: As Gsu82CE: inputs to $70:0068, $70:002C, $70:002E; GsuRun; results
; from $70:0026, $70:0028, $70:002A. X, Y and P saved and restored.
; Entry: any widths, D = $0000. Exit: A = the third result (16 bits); X, Y
; unchanged; P unchanged but N, Z, which the final pull of X sets.
%at($03AF9D)
Gsu8277:
    phx
    phy
    php
    rep.b #$30
    lda.b !dp_02
    sta.l !gsu_in_68
    lda.b !dp_08
    sta.l !gsu_dest
    lda.b !dp_94
    sta.l !gsu_in_2e
    sep.b #$20
    lda.b #$01
    ldx.w #$8277
    jsl.l GsuRun
    rep.b #$20
    lda.l !gsu_out_26
    sta.b !dp_b7
    lda.l !gsu_out_28
    sta.b !dp_b9
    lda.l !gsu_out_2a
    sta.b !dp_bb
    plp
    ply
    plx
    rtl

; @routine 03:C9D2 VectorGsu81E3
; WHAT IT DOES: Puts direct page $04 - $02 in $02 and $05F2 - $94 in $08,
; then runs Gsu81E3Long: ObjectGsu81E3 for a point in $02, $94 instead of
; an object.
; HOW: 16-bit subtracts, $05F2 through the data bank; X and Y saved and
; restored.
; Entry: X 16-bit, D = $0000. Exit: A as Gsu81E3Long leaves it; X, Y
; unchanged; M 16-bit; N, Z from X.
%at($03C9D2)
VectorGsu81E3:
    phx
    phy
    rep.b #$20
    lda.b !dp_04
    sec
    sbc.b !dp_02
    sta.b !dp_02
    lda.w !rot_z
    sec
    sbc.b !dp_94
    sta.b !dp_08
    jsl.l Gsu81E3Long
    rep.b #$20
    ply
    plx
    rtl

; @routine 03:B079 ScaledOffsetPair
; WHAT IT DOES: Two ScaledOffsets from the base $CE over the divisor in
; direct page $94: of $02 into $76, and of $08 into $78, the second then
; plus $D0 and minus $CE. Nothing in the game calls it.
; HOW: Byte copies of the inputs to $88-$8B and of the results from $8C,
; 8-bit adds and subtracts.
; Entry: M 8-bit, X 8-bit, D = $0000; data bank $03. Exit: A = $79's new
; value; X as the second ScaledOffset left it; N, Z, C, V from the last
; subtract.
%at($03B079)
ScaledOffsetPair:
    lda.b !dp_02
    sta.b !fxrec_rank
    lda.b !dp_03
    sta.b !fxrec_rank+1
    lda.b !dp_94
    sta.b !fxrec_rank+2
    lda.b !dp_95
    sta.b !fxrec_rank+3
    jsl.l ScaledOffset
    lda.b !fxcar_8c
    sta.b !fxrec_next_car+2
    lda.b !fxcar_8c+1
    sta.b !fxrec_next_car+3
    lda.b !dp_08
    sta.b !fxrec_rank
    lda.b !dp_09
    sta.b !fxrec_rank+1
    lda.b !dp_94
    sta.b !fxrec_rank+2
    lda.b !dp_95
    sta.b !fxrec_rank+3
    jsl.l ScaledOffset
    lda.b !fxcar_8c
    sta.b !fxrec_next_car+4
    lda.b !fxcar_8c+1
    sta.b !fxrec_next_car+5
    clc
    lda.b !dp_d0
    adc.b !fxrec_next_car+4
    sta.b !fxrec_next_car+4
    lda.b !dp_d0+1
    adc.b !fxrec_next_car+5
    sta.b !fxrec_next_car+5
    sec
    lda.b !fxrec_next_car+4
    sbc.b !dp_ce
    sta.b !fxrec_next_car+4
    lda.b !fxrec_next_car+5
    sbc.b !dp_ce+1
    sta.b !fxrec_next_car+5
    rtl

; @routine 03:B0CC ScaledOffset
; WHAT IT DOES: Sets the word at direct page $8C to the word at $CE plus
; the signed word at $88 times 256 over the word at $8A (only its top
; bits count), or to $CE when $88 is 0: a scaled offset from a base,
; worked with log and exponent tables. Nothing in the game calls it.
; HOW: $8A:$8B and the magnitude of $88:$89 are each brought to one byte
; by shifting right (X counts the shifts; $8B's count is kept at $96);
; a byte $88 below $80 is normalised by shifting left. The quotient's
; byte is the difference of the two bytes' logs from the table at $8368
; in the data bank, and its value comes from the tables at $8268 (high
; byte) and $8168 (low byte), shifted by the difference of the counts.
; The sign of $89 (kept at $8F) picks add or subtract. Work bytes $88,
; $8A, $8E, $8F and $96 are left changed. 8-bit throughout.
; Entry: M 8-bit, X 8-bit, D = $0000; data bank $03. Exit: A = $8D's new
; value (or $CF's when $88 was 0); X = the shift count left; N, Z, C, V
; from the last add or subtract (from $CF's load when $88 was 0).
ScaledOffset:
    ldx.b #$00
    lda.b !fxrec_rank+3
    beq .lB0DE
    inx
    lsr a
    beq .lB0DC
.lB0D6:
    ror.b !fxrec_rank+2
    inx
    lsr a
    bne .lB0D6
.lB0DC:
    ror.b !fxrec_rank+2
.lB0DE:
    stx.b !dp_95+1
    ldx.b #$00
    lda.b !fxrec_rank+1
    sta.b !fxcar_8c+3
    bpl .lB0F5
    lda.b #$00
    sec
    sbc.b !fxrec_rank
    sta.b !fxrec_rank
    lda.b #$00
    sbc.b !fxrec_rank+1
    sta.b !fxrec_rank+1
.lB0F5:
    beq .lB106
    inx
    lsr a
    beq .lB101
.lB0FB:
    ror.b !fxrec_rank
    inx
    lsr a
    bne .lB0FB
.lB101:
    ror.b !fxrec_rank
    jmp.w .lB11D
.lB106:
    lda.b !fxrec_rank
    bmi .lB11D
    bne .lB117
    lda.b !dp_ce
    sta.b !fxcar_8c
    lda.b !dp_ce+1
    sta.b !fxcar_8c+1
    jmp.w .lB17D
.lB117:
    dex
    asl a
    bpl .lB117
    sta.b !fxrec_rank
.lB11D:
    lda.b !fxrec_rank+2
    cmp.b !fxrec_rank
    bcc .lB12B
.lB123:
    dex
    lsr a
    cmp.b !fxrec_rank
    bcs .lB123
    sta.b !fxrec_rank+2
.lB12B:
    txa
    sec
    sbc.b !dp_95+1
    sta.b !fxcar_8c+2
    ldx.b !fxrec_rank
    lda.w !log_table,x
    ldx.b !fxrec_rank+2
    sec
    sbc.w !log_table,x
    tax
    lda.w !exp_table_hi,x
    sta.b !fxcar_8c+1
    lda.w !exp_table_lo,x
    ldx.b !fxcar_8c+2
    bmi .lB154
    beq .lB15A
.lB14B:
    asl a
    rol.b !fxcar_8c+1
    dex
    bne .lB14B
    jmp.w .lB15A
.lB154:
    lsr.b !fxcar_8c+1
    ror a
    inx
    bne .lB154
.lB15A:
    sta.b !fxcar_8c
    bit.b !fxcar_8c+3
    bpl .lB170
    sec
    lda.b !dp_ce
    sbc.b !fxcar_8c
    sta.b !fxcar_8c
    lda.b !dp_ce+1
    sbc.b !fxcar_8c+1
    sta.b !fxcar_8c+1
    jmp.w .lB17D
.lB170:
    clc
    lda.b !fxcar_8c
    adc.b !dp_ce
    sta.b !fxcar_8c
    lda.b !fxcar_8c+1
    adc.b !dp_ce+1
    sta.b !fxcar_8c+1
.lB17D:
    rtl

; @routine 03:B17E ViewFromAnglesGsu8277
; WHAT IT DOES: Builds a view from the angle bytes in direct page $C5,
; $C7, $C9: the vector ($FF - $C5's low byte - $780, -$C7, $FF - $C9's
; low byte - $780) into $02, $08, $94, the high bytes of $C5 and $C9 into
; $067C and $067D; runs Gsu8277 on it and takes its results back into
; $02, $08, $94; the nine words at $064C over 128 (kept to the sign) into
; $066A-$067A; then puts the vector, six of those words and the two
; angle bytes in Super FX RAM: $70:0068, $002C, $002E, $0154-$015E,
; $0162, $0164. Nothing in the game calls it.
; HOW: 16-bit direct page and work RAM (through the data bank), long
; stores; the division is CMP #$8000, ROR seven times.
; Entry: D = $0000. Exit: M 8-bit, X 8-bit (X and Y high bytes 0); A =
; $067B and $067D (high, low); N, Z from $067D; C = bit 6 of $065C.
ViewFromAnglesGsu8277:
    rep.b #$30
    lda.b !dp_c5
    and.w #$00FF
    eor.w #$00FF
    sec
    sbc.w #$0780
    sta.b !dp_02
    lda.b !dp_c9
    and.w #$00FF
    eor.w #$00FF
    sec
    sbc.w #$0780
    sta.b !dp_94
    lda.w #$0000
    sec
    sbc.b !dp_c7
    sta.b !dp_08
    lda.b !dp_c5
    and.w #$FF00
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sep.b #$20
    sta.w !wram_0668+20
    rep.b #$20
    lda.b !dp_c9
    and.w #$FF00
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    lsr a
    sep.b #$20
    sta.w !wram_0668+21
    jsl.l Gsu8277
    rep.b #$20
    lda.b !dp_b7
    sta.b !dp_02
    lda.b !dp_b9
    sta.b !dp_08
    lda.b !dp_bb
    sta.b !dp_94
    sep.b #$20
    rep.b #$20
    lda.w !view_8325_out
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+2
    lda.w !view_8325_out+2
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+4
    lda.w !view_8325_out+4
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+6
    lda.w !view_8325_out+6
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+8
    lda.w !view_8325_out+8
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+10
    lda.w !view_8325_out+10
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+12
    lda.w !view_8325_out+12
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+14
    lda.w !view_8325_out+14
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+16
    lda.w !view_8325_out+16
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_0668+18
    sep.b #$20
    rep.b #$20
    lda.b !dp_02
    sta.l !gsu_in_68
    lda.b !dp_08
    sta.l !gsu_dest
    lda.b !dp_94
    sta.l !gsu_in_2e
    lda.w !wram_0668+2
    sta.l !fx_model_150+4
    lda.w !wram_0668+4
    sta.l !fx_model_150+6
    lda.w !wram_0668+6
    sta.l !fx_model_150+8
    lda.w !wram_0668+14
    sta.l !fx_model_150+10
    lda.w !wram_0668+16
    sta.l !fx_model_150+12
    lda.w !wram_0668+18
    sta.l !fx_model_150+14
    sep.b #$20
    rep.b #$10
    lda.w !wram_0668+20
    sta.l !fx_model_150+18
    lda.w !wram_0668+21
    sta.l !fx_model_150+20
    sep.b #$30
    rtl

; @routine 03:B367 ViewToGsuParams
; WHAT IT DOES: Puts the view in Super FX RAM: the angle words in direct
; page $C5, $C7, $C9 at $70:00D8-00DC (first the word at $1168 in all
; three when $1167 is set) and the nine words at $064C at $70:00E4-00F4.
; Nothing in the game calls it.
; HOW: 16-bit loads (work RAM through the data bank) and long stores.
; Entry: D = $0000. Exit: M 8-bit, X 8-bit (X and Y high bytes 0); A =
; the word at $065C; N, Z from it.
ViewToGsuParams:
    rep.b #$20
    lda.w !packed_time_a+4
    and.w #$00FF
    beq .lB386
    lda.w !frame_step
    sta.l !gsu_param_00d8
    lda.w !frame_step
    sta.l !gsu_param_00da
    lda.w !frame_step
    sta.l !gsu_param_00dc
.lB386:
    lda.b !dp_c5
    sta.l !gsu_param_00d8
    lda.b !dp_c7
    sta.l !gsu_param_00da
    lda.b !dp_c9
    sta.l !gsu_param_00dc
    lda.w !view_8325_out
    sta.l !gsu_out_e4
    lda.w !view_8325_out+2
    sta.l !fx_object_matrix_1
    lda.w !view_8325_out+4
    sta.l !fx_object_matrix_2
    lda.w !view_8325_out+6
    sta.l !fx_object_matrix_3
    lda.w !view_8325_out+8
    sta.l !fx_object_matrix_4
    lda.w !view_8325_out+10
    sta.l !fx_object_matrix_5
    lda.w !view_8325_out+12
    sta.l !fx_object_matrix_6
    lda.w !view_8325_out+14
    sta.l !fx_object_matrix_7
    lda.w !view_8325_out+16
    sta.l !fx_object_matrix_8
    sep.b #$30
    rtl
