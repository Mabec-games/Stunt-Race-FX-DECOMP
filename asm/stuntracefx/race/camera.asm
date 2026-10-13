; camera.asm: the race camera, a record in bank 0 that follows an object
; (its target) with a lag set by mode bits.
; C: src/stuntracefx/race/camera.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:DB71 CameraFollowStep
; WHAT IT DOES: One frame of the race camera X (a record in bank 0): its
; mode bits ($05FA, from +$00) and target ($1F33, from +$02) are taken;
; with bit 1 it does nothing more. With bit 2 its speed lead (+$26-$2A) is
; minus the target's speed (+$4A of its Super FX record) scaled by 1/2 and
; 1/8, else 0. It copies +$1A-$1E to direct page $02, $08, $94; then sets
; its turn (+$16) and pitch (+$14) from the target's angles +$15 and +$14:
; at once when bits 10-12 are 0, otherwise easing the turn by the
; difference (whole in mode 0, a half or a quarter in mode 1 by the speed
; of the record at $70:247A, a quarter in mode 2, 5/64 or 5/128 in modes
; 3 and 4 through ScaleFiveThirtySeconds) and copying it to $0660, and the
; pitch to $065E (a quarter); with bit 8 all three are 0. Then the view
; angles ($065E-$0662, plus +$35-$39, $065E negated) go to the Super FX
; (Gsu8325), the nine words $0608-$0618 to $064C-$065C, and Gsu82CE runs.
; Last, its position (+$08-$0C) follows the target's (+$0C-$10 plus $B7,
; $B9, $BB): at once, or with bit 4 the height by a half or a quarter of
; the difference, or with bit 3 all three by a quarter, an eighth and a
; quarter. With bits 2 and 3 of +$42 of the record at $70:247A set and
; bit 9 clear, the turn eases by half the difference straight away.
; HOW: Signed halvings as CMP #$8000 / ROR A, most of them rounded toward
; zero; camera fields through direct page (D = 0), the target's through
; the data bank. Bits 3 and 8, modes 0 and 3, the bit-9 case and the
; rounding of the speed lead are not reached in the traces so far.
; Entry: X 16-bit, D = $0000; X = camera. Exit: X = camera; M 16-bit,
; X 16-bit; with bit 1: A = 2, N, Z from it; otherwise A = the last
; position word, N, Z, C, V from its add, Y as the last step left it.
%at($02DB71)
CameraFollowStep:
    phx
    rep.b #$30
    phx
    lda.b !cam_target,x
    tax
    stx.w !cam_target_obj
    plx
    lda.b !cam_flags,x
    sta.w !cam_mode
    lda.w !cam_mode
    and.w #$0002
    beq .active
    brl .done
.active:
    rep.b #$20
    lda.w !cam_mode
    and.w #$0004
    bne .speed_lead
    brl .no_lead
.speed_lead:
    phx
    ldx.w !cam_target_obj
    lda.l !obj_22_long,x
    tax
    lda.l !gsu_speeds,x
    bpl .speed_abs
    eor.w #$FFFF
    inc a
.speed_abs:
    plx
    cmp.w #$8000
    ror a
    bpl .lead_half
    adc.w #$0000                    ; not reached in the traces so far
.lead_half:
    sta.b !cam_tmp_c
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    bpl .lead_eighth
    adc.w #$0000                    ; not reached in the traces so far
.lead_eighth:
    sta.b !cam_tmp_b
    stz.b !cam_tmp_a
    lda.b !cam_tmp_a
    eor.w #$FFFF
    inc a
    sta.b !cam_lead,x
    lda.b !cam_tmp_b
    eor.w #$FFFF
    inc a
    sta.b !cam_lead+2,x
    lda.b !cam_tmp_c
    eor.w #$FFFF
    inc a
    sta.b !cam_lead+4,x
    bra .lead_done
.no_lead:
    stz.b !cam_lead,x
    stz.b !cam_lead+2,x
    stz.b !cam_lead+4,x
.lead_done:
    lda.b !cam_base,x
    sta.b !cam_tmp_a
    lda.b !cam_base+2,x
    sta.b !cam_tmp_b
    lda.b !cam_base+4,x
    sta.b !cam_tmp_c
    lda.w !cam_mode
    bit.w #$0100
    beq .angles
    stz.w !view_pitch               ; not reached in the traces so far
    stz.w !view_turn                ; not reached in the traces so far
    stz.w !view_roll                ; not reached in the traces so far
    jmp.w .to_super_fx              ; not reached in the traces so far
.angles:
    bit.w #$1C00
    bne .eased
    ldy.w !cam_target_obj
    lda.w !obj_15,y
    xba
    and.w #$FF00
    eor.w #$FFFF
    inc a
    sta.b !cam_turn,x
    sta.w !view_turn
    lda.w !obj_14,y
    xba
    and.w #$FF00
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.b !cam_pitch,x
    sta.w !view_pitch
    stz.w !view_roll
    jmp.w .to_super_fx
.eased:
    ldy.w !cam_target_obj
    phx
    lda.l !gsu_record_247a
    tax
    lda.l !gsu_rec_42,x
    plx
    and.w #$000C
    beq .lag
    lda.w !cam_mode
    and.w #$0200
    bne .lag
    lda.w !obj_15,y                 ; not reached in the traces so far
    xba                             ; not reached in the traces so far
    and.w #$FF00                    ; not reached in the traces so far
    eor.w #$FFFF                    ; not reached in the traces so far
    inc a                           ; not reached in the traces so far
    sec                             ; not reached in the traces so far
    sbc.b !cam_turn,x               ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.b !cam_turn,x               ; not reached in the traces so far
    sta.b !cam_turn,x               ; not reached in the traces so far
    sta.w !view_turn                ; not reached in the traces so far
    jmp.w .pitch                    ; not reached in the traces so far
.lag:
    lda.w !obj_15,y
    xba
    and.w #$FF00
    eor.w #$FFFF
    inc a
    sec
    sbc.b !cam_turn,x
    sta.w !cam_lag
    lda.w !cam_mode
    and.w #$1C00
    cmp.w #$0000
    beq .lag_whole
    cmp.w #$0400
    beq .lag_by_speed
    cmp.w #$0800
    beq .lag_quarter
    cmp.w #$0C00
    beq .lag_scaled_half
    cmp.w #$1000
    beq .lag_scaled_quarter
.lag_whole:
    lda.w !cam_lag                  ; not reached in the traces so far
    bra .add_lag                    ; not reached in the traces so far
.lag_by_speed:
    phx
    lda.l !gsu_record_247a
    tax
    lda.l !gsu_rec_6c,x
    bpl .lag_speed_abs
    eor.w #$FFFF
    inc a
.lag_speed_abs:
    plx
    cmp.w #$7EF4
    bpl .lag_half
    lda.w !cam_lag
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    bpl .lag_slow
    adc.w #$0000
.lag_slow:
    bra .add_lag
.lag_half:
    lda.w !cam_lag
    cmp.w #$8000
    ror a
    bpl .lag_fast
    adc.w #$0000
.lag_fast:
    bra .add_lag
.lag_quarter:
    lda.w !cam_lag
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    bpl .lag_quartered
    adc.w #$0000
.lag_quartered:
    bra .add_lag
.lag_scaled_half:
    lda.w !cam_lag                  ; not reached in the traces so far
    jsl.l ScaleFiveThirtySeconds    ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    bpl .lag_scaled_halved          ; not reached in the traces so far
    adc.w #$0000                    ; not reached in the traces so far
.lag_scaled_halved:
    bra .add_lag                    ; not reached in the traces so far
.lag_scaled_quarter:
    lda.w !cam_lag
    jsl.l ScaleFiveThirtySeconds
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    bpl .add_lag
    adc.w #$0000
.add_lag:
    clc
    adc.b !cam_turn,x
    sta.b !cam_turn,x
    sta.w !view_turn
.pitch:
    lda.w !obj_14,y
    xba
    and.w #$FF00
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    bpl .pitch_set
    adc.w #$0000
.pitch_set:
    sta.b !cam_pitch,x
    sta.w !view_pitch
    stz.w !view_roll
.to_super_fx:
    phx
    lda.w !view_pitch
    clc
    adc.b !cam_angle_add,x
    eor.w #$FFFF
    inc a
    sta.w !view_pitch
    lda.w !view_turn
    clc
    adc.b !cam_angle_add+2,x
    sta.w !view_turn
    lda.w !view_roll
    clc
    adc.b !cam_angle_add+4,x
    sta.w !view_roll
    sep.b #$20
    jsl.l Gsu8325
    rep.b #$20
    lda.w !view_matrix_out+0
    sta.w !view_matrix_copy+0
    lda.w !view_matrix_out+2
    sta.w !view_matrix_copy+2
    lda.w !view_matrix_out+4
    sta.w !view_matrix_copy+4
    lda.w !view_matrix_out+6
    sta.w !view_matrix_copy+6
    lda.w !view_matrix_out+8
    sta.w !view_matrix_copy+8
    lda.w !view_matrix_out+10
    sta.w !view_matrix_copy+10
    lda.w !view_matrix_out+12
    sta.w !view_matrix_copy+12
    lda.w !view_matrix_out+14
    sta.w !view_matrix_copy+14
    lda.w !view_matrix_out+16
    sta.w !view_matrix_copy+16
    sep.b #$20
    jsl.l Gsu82CE
    rep.b #$30
    plx
    lda.w !cam_mode
    bit.w #$0008
    beq .follow
    brl .smooth                     ; not reached in the traces so far
.follow:
    bit.w #$0010
    bne .follow_height
    ldy.w !cam_target_obj
    lda.w !obj_pos_x,y
    clc
    adc.b !cam_offset_x
    sta.b !cam_pos,x
    lda.w !obj_pos_y,y
    clc
    adc.b !cam_offset_y
    sta.b !cam_pos+2,x
    lda.w !obj_pos_z,y
    clc
    adc.b !cam_offset_z
    sta.b !cam_pos+4,x
    bra .done
.follow_height:
    ldy.w !cam_target_obj
    lda.w !obj_pos_x,y
    clc
    adc.b !cam_offset_x
    sta.b !cam_pos,x
    lda.w !obj_pos_y,y
    clc
    adc.b !cam_offset_y
    sec
    sbc.b !cam_pos+2,x
    bmi .height_half
    cmp.w #$8000
    ror a
.height_half:
    cmp.w #$8000
    ror a
    clc
    adc.b !cam_pos+2,x
    sta.b !cam_pos+2,x
    lda.w !obj_pos_z,y
    clc
    adc.b !cam_offset_z
    sta.b !cam_pos+4,x
    bra .done
.smooth:
    lda.w !obj_pos_x,y                ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.b !cam_offset_x            ; not reached in the traces so far
    sec                             ; not reached in the traces so far
    sbc.b !cam_pos+0,x              ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.b !cam_pos+0,x              ; not reached in the traces so far
    sta.b !cam_pos+0,x              ; not reached in the traces so far
    lda.w !obj_pos_y,y                ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.b !cam_offset_y            ; not reached in the traces so far
    sec                             ; not reached in the traces so far
    sbc.b !cam_pos+2,x              ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.b !cam_pos+2,x              ; not reached in the traces so far
    sta.b !cam_pos+2,x              ; not reached in the traces so far
    lda.w !obj_pos_z,y                ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.b !cam_offset_z            ; not reached in the traces so far
    sec                             ; not reached in the traces so far
    sbc.b !cam_pos+4,x              ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    cmp.w #$8000                    ; not reached in the traces so far
    ror a                           ; not reached in the traces so far
    clc                             ; not reached in the traces so far
    adc.b !cam_pos+4,x              ; not reached in the traces so far
    sta.b !cam_pos+4,x              ; not reached in the traces so far
    bra .done                       ; not reached in the traces so far
.done:
    plx
    rtl
