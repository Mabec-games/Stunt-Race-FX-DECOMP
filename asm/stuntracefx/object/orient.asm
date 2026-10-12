; orient.asm: turning a record to face, or to match, the object it is tied
; to. The record (X) holds flags at +$00, the tied object's offset in the
; object table at $7E:2000 at +$02, a position at +$08/+$0A/+$0C and three
; angles at +$0E/+$10/+$12. The tied object's angles are the bytes at its
; +$14/+$15/+$16.
; C: src/stuntracefx/object/orient.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 02:DE45 ObjectFaceTied
; WHAT IT DOES: Turns record X toward the object it is tied to (+$02).
; With flag bit 0: the turn and the pitch toward it come from the Super FX
; (ObjectGsu81E3 with the record's position in direct page $04/$0A and
; $05F2; ObjectGsu80BCThen81E3): +$10 = the first + $8000, +$0E = the
; second (with flag bit 6, a quarter of the way there each call), and +$12
; eases halfway toward a quarter of -(the object's +$16 * 256). Otherwise,
; with flag bit 14, each angle eases halfway toward -(the object's angle
; byte * 256). Then +$12 is cleared unless flag bit 15 is set.
; HOW: The record's flags and +$12 at the end are read as direct page
; offsets indexed by X (D = $0000); everything else through the data bank
; with Y = the record and X = the object. P is saved and restored.
; Entry: D = $0000; X = the record. Exit: A = the flags AND $8000; X
; unchanged; Y = the record when either flag was set, else unchanged; P
; unchanged.
%at($02DE45)
ObjectFaceTied:
    php
    rep.b #$30
    lda.b !tie_flags,x
    and.w #$0001
    bne .lDE52
    brl .lDEBD
.lDE52:
    rep.b #$30
    txy
    lda.b !tie_object,x
    tax
    lda.w !tie_x,y
    sta.b !dp_04
    lda.w !tie_y,y
    sta.b !dp_0a
    lda.w !tie_z,y
    sta.w !rot_z
    jsl.l ObjectGsu81E3
    clc
    adc.w #$8000
    sta.w !tie_angle_10,y
    lda.w !tie_flags,y
    and.w #$0040
    bne .lDE81
    jsl.l ObjectGsu80BCThen81E3
    bra .lDE95
.lDE81:
    jsl.l ObjectGsu80BCThen81E3
    sec
    sbc.w !tie_angle_0e,y
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    clc
    adc.w !tie_angle_0e,y
.lDE95:
    sta.w !tie_angle_0e,y
    lda.w !obj_16,x
    and.w #$00FF
    xba
    eor.w #$FFFF
    inc a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sec
    sbc.w !tie_angle_12,y
    cmp.w #$8000
    ror a
    clc
    adc.w !tie_angle_12,y
    sta.w !tie_angle_12,y
    tyx
    bra .lDF19
.lDEBD:
    rep.b #$20
    lda.b !tie_flags,x
    and.w #$4000
    beq .lDF19
    txy
    lda.b !tie_object,x
    tax
    lda.w !obj_14,x
    and.w #$00FF
    xba
    eor.w #$FFFF
    inc a
    sec
    sbc.w !tie_angle_0e,y
    cmp.w #$8000
    ror a
    clc
    adc.w !tie_angle_0e,y
    sta.w !tie_angle_0e,y
    lda.w !obj_15,x
    and.w #$00FF
    xba
    eor.w #$FFFF
    inc a
    sec
    sbc.w !tie_angle_10,y
    cmp.w #$8000
    ror a
    clc
    adc.w !tie_angle_10,y
    sta.w !tie_angle_10,y
    lda.w !obj_16,x
    and.w #$00FF
    xba
    eor.w #$FFFF
    inc a
    sec
    sbc.w !tie_angle_12,y
    cmp.w #$8000
    ror a
    clc
    adc.w !tie_angle_12,y
    sta.w !tie_angle_12,y
    tyx
.lDF19:
    lda.b !tie_flags,x
    and.w #$8000
    bne .lDF22
    stz.b !tie_angle_12,x
.lDF22:
    plp
    rtl

; @routine 02:DAD6 ViewFromRecord
; WHAT IT DOES: Sets the view from record X: its position to direct page
; $C5/$C7/$C9, its angles to $0664/$0666/$0668, the tied object's +$22 to
; Super FX RAM $70:247A; the view's area origin $1EF5-$1EF9 from the
; position (ScaleDp02Dp94Long on X and Z), and the fixed area origin
; $1EFB-$1EFF from the tied object's X and Z (+$0C, +$10), or a copy of
; the view's when flag bit 13 is set (not reached in the traces so far).
; HOW: The record's fields as direct page offsets indexed by X (D =
; $0000); the object's in bank $7E; the outputs through the data bank.
; Entry: D = $0000; X = the record. Exit: A = the last value stored; X
; unchanged; M 16-bit, X 16-bit; N, Z from X (from A with bit 13); C, V
; from the last ScaleDp02Dp94Long.
%at($02DAD6)
ViewFromRecord:
    rep.b #$30
    lda.b !tie_x,x
    sta.b !dp_c5
    lda.b !tie_y,x
    sta.b !dp_c7
    lda.b !tie_z,x
    sta.b !dp_c9
    lda.b !tie_angle_0e,x
    sta.w !wram_0664
    lda.b !tie_angle_10,x
    sta.w !wram_0666
    lda.b !tie_angle_12,x
    sta.w !wram_0668
    phx
    lda.b !tie_object,x
    tax
    lda.l !obj_22_long,x
    sta.l !gsu_tied_22
    plx
    phx
    lda.b !dp_c5
    sta.b !dp_02
    lda.b !dp_c9
    sta.b !dp_94
    jsl.l ScaleDp02Dp94Long
    sta.w !view_area_origin
    lda.b !dp_02
    sta.w !view_area_origin+2
    lda.b !dp_94
    sta.w !view_area_origin+4
    plx
    lda.b !tie_flags,x
    and.w #$2000
    beq .lDB36
    lda.w !view_area_origin
    sta.w !fixed_area_origin
    lda.w !view_area_origin+2
    sta.w !fixed_area_origin+2
    lda.w !view_area_origin+4
    sta.w !fixed_area_origin+4
    bra .lDB58
.lDB36:
    phx
    lda.b !tie_object,x
    tax
    lda.l !obj_pos_x_long,x
    sta.b !dp_02
    lda.l !obj_pos_z_long,x
    sta.b !dp_94
    jsl.l ScaleDp02Dp94Long
    sta.w !fixed_area_origin
    lda.b !dp_02
    sta.w !fixed_area_origin+2
    lda.b !dp_94
    sta.w !fixed_area_origin+4
    plx
.lDB58:
    rtl
