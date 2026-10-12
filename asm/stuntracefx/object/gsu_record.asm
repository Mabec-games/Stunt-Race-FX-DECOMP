; gsu_record.asm: each object's record in Super FX RAM, which the renderer
; reads. An object's +$22 (bank $7E, through the data bank) holds the
; record's address in bank $70, or 0 when it has none. Fields set here:
; +$0C/+$0E/+$10 the position, +$16 flags, +$1A a mode word.
; C: src/stuntracefx/object/gsu_record.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 08:CCBE GsuRecordMode2
; WHAT IT DOES: Sets object X's record mode word (+$1A) to 2.
; HOW: A 16-bit store at $70:001A + the record address, when there is one.
; Entry: X 16-bit; X = object. Exit: A = 2 (0 with no record); M 8-bit; Z
; set with no record, else N, Z from X.
%at($08CCBE)
GsuRecordMode2:
    rep.b #$20
    lda.w !obj_22,x
    beq .lCCCF
    phx
    tax
    lda.w #$0002
    sta.l !gsu_rec_mode,x
    plx
.lCCCF:
    sep.b #$20
    rtl

; @routine 08:CCF1 GsuRecordSetFlags
; WHAT IT DOES: Sets the bits in direct page $02 in object X's record flags
; (+$16).
; HOW: 16-bit OR in place, when the object has a record.
; Entry: X 16-bit, D = $0000; X = object. Exit: A = the new flags (0 with no
; record); M 8-bit; Z set with no record, else N, Z from X.
%at($08CCF1)
GsuRecordSetFlags:
    rep.b #$20
    lda.w !obj_22,x
    beq .lCD05
    phx
    tax
    lda.l !gsu_rec_flags,x
    ora.b !dp_02
    sta.l !gsu_rec_flags,x
    plx
.lCD05:
    sep.b #$20
    rtl

; @routine 08:CCA3 GsuRecordFlag10
; WHAT IT DOES: In object X's record flags (+$16), sets bit 4 and clears
; bit 8.
; HOW: 16-bit OR and AND in place, when the object has a record.
; Entry: X 16-bit; X = object. Exit: A = the new flags (0 with no record);
; M 8-bit; Z set with no record, else N, Z from X.
%at($08CCA3)
GsuRecordFlag10:
    rep.b #$20
    lda.w !obj_22,x
    beq .lCCBB
    phx
    tax
    lda.l !gsu_rec_flags,x
    ora.w #$0010
    and.w #$FEFF
    sta.l !gsu_rec_flags,x
    plx
.lCCBB:
    sep.b #$20
    rtl

; @routine 08:CCD2 GsuRecordPosition
; WHAT IT DOES: Copies the position in direct page $02, $08, $94 into object
; X's record (+$0C, +$0E, +$10).
; HOW: Three 16-bit stores, when the object has a record.
; Entry: X 16-bit, D = $0000; X = object. Exit: A = the third word (0 with
; no record); M 8-bit; Z set with no record, else N, Z from X.
%at($08CCD2)
GsuRecordPosition:
    rep.b #$20
    lda.w !obj_22,x
    beq .lCCEE
    phx
    tax
    lda.b !dp_02
    sta.l !gsu_rec_pos_x,x
    lda.b !dp_08
    sta.l !gsu_rec_pos_y,x
    lda.b !dp_94
    sta.l !gsu_rec_pos_z,x
    plx
.lCCEE:
    sep.b #$20
    rtl

; @routine 08:E3EE GsuRecordWord88
; WHAT IT DOES: Returns the word at +$88 of the Super FX record of the
; object that object X's +$06 names, minus one.
; HOW: Y = the object's +$06; the record address is that object's +$22.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A = the word - 1 (16-bit
; value, M 8-bit); Y = the +$06 object; X unchanged; N, Z from X.
%at($08E3EE)
GsuRecordWord88:
    ldy.w !obj_06,x
    phx
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_88,x
    dec a
    sep.b #$20
    plx
    rts

; @routine 08:E401 GsuRecordWord58
; WHAT IT DOES: Returns the word at +$58 of the Super FX record of object X's
; +$06 object, with carry set when bits 4 and 10 of the record's +$42 are
; both clear.
; HOW: Y = the object's +$06; the record address is that object's +$22.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A = the word (M 8-bit); Y =
; the +$06 object; X unchanged; N, Z from X; C as above.
%at($08E401)
GsuRecordWord58:
    ldy.w !obj_06,x
    phx
    rep.b #$20
    lda.w !obj_22,y
    tax
    lda.l !gsu_rec_42,x
    and.w #$0410
    beq .lE41D
    lda.l !gsu_rec_58,x
    sep.b #$20
    plx
    clc
    rts
.lE41D:
    lda.l !gsu_rec_58,x
    sep.b #$20
    plx
    sec
    rts

; @routine 08:D070 GsuRecordFlag4
; WHAT IT DOES: Sets bit 2 of object X's record word at +$1C and returns 0;
; an object without a record gets $FF in its +$36 and returns $FF.
; HOW: The record test reads both bytes of +$22 (8-bit OR).
; Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = 0 or $FF; X
; unchanged; Y = X when there is a record; M 8-bit; N, Z from A.
%at($08D070)
GsuRecordFlag4:
    lda.w !obj_22,x
    ora.w !obj_22+1,x
    bne .lD07F
    lda.b #$FF
    sta.w !obj_36,x
    bra .lD099
.lD07F:
    rep.b #$20
    lda.w !obj_22,x
    beq .lD099
    txy
    tax
    lda.l !gsu_rec_1c,x
    ora.w #$0004
    sta.l !gsu_rec_1c,x
    tyx
    sep.b #$20
    lda.b #$00
    rtl
.lD099:
    sep.b #$20
    lda.b #$FF
    rtl

; @routine 08:D83D GsuRecordVelocity
; WHAT IT DOES: Copies object X's velocity (+$2E, +$30, +$32), times 8, into
; its record (+$2E, +$30, +$32) and returns 0; an object without a record
; returns $FF.
; HOW: Three left shifts of each word.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A low byte = 0 (high byte
; from the last word) or A = $00FF; Y = X when there is a record; M 8-bit;
; N, Z from A; C from the last shift when there is a record.
%at($08D83D)
GsuRecordVelocity:
    rep.b #$20
    lda.w !obj_22,x
    beq .lD86A
    txy
    tax
    lda.w !obj_vel_x,y
    asl a
    asl a
    asl a
    sta.l !gsu_rec_vel_x,x
    lda.w !obj_vel_y,y
    asl a
    asl a
    asl a
    sta.l !gsu_rec_vel_y,x
    lda.w !obj_vel_z,y
    asl a
    asl a
    asl a
    sta.l !gsu_rec_vel_z,x
    tyx
    sep.b #$20
    lda.b #$00
    rtl
.lD86A:
    sep.b #$20
    lda.b #$FF
    rtl

; @routine 08:B863 GsuRecordWords88
; WHAT IT DOES: For each object in the list at $7E:FFAE (count = half the
; word at $7E:FFB8), copies the word at +$88 of its Super FX record into
; the list at $7E:FFA6.
; HOW: The record address is each object's +$22 (bank $7E); direct page $02
; counts the entries.
; Entry: any widths, D = $0000. Exit: A = the last word; X, Y unchanged; P
; unchanged except N, Z, which the final pull of X sets from X.
%at($08B863)
GsuRecordWords88:
    phx
    phy
    php
    rep.b #$30
    lda.l !object_list_b_len
    cmp.w #$8000
    ror a
    sta.b !dp_02
    ldx.w #$0000
.lB875:
    lda.l !object_list_b,x
    phx
    tax
    lda.l !obj_22_long,x
    tax
    lda.l !gsu_rec_88,x
    plx
    sta.l !wram_7EFFA6,x
    inx
    inx
    dec.b !dp_02
    bne .lB875
    plp
    ply
    plx
    rtl

; @routine 08:B2E9 GsuRecordStep6A
; WHAT IT DOES: While $0D62 is set, advances object X's record word at +$6A
; by 1 (when the record's +$88 is 1) or 3, up to $180, unless bit 10 of the
; record's +$42 is set.
; HOW: The step is kept at $1F2B; the limit test is a signed compare.
; Entry: M 8-bit, X 16-bit; X = object. Exit: $0D62 clear: A low byte = 0,
; Z set. Otherwise A = the last word read or stored (M 8-bit); X unchanged;
; N, Z from X; C, V from the limit compare and the add when it stepped.
%at($08B2E9)
GsuRecordStep6A:
    lda.w !wram_0D62
    beq .lB32A
    phx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_42,x
    bit.w #$0400
    bne .lB327
    lda.w #$0001
    sta.w !wram_1F2B
    lda.l !gsu_rec_88,x
    cmp.w #$0001
    beq .lB313
    lda.w #$0003
    sta.w !wram_1F2B
.lB313:
    lda.l !gsu_rec_6a,x
    clc
    adc.w !wram_1F2B
    cmp.w #$0180
    bmi .lB323
    lda.w #$0180
.lB323:
    sta.l !gsu_rec_6a,x
.lB327:
    sep.b #$20
    plx
.lB32A:
    rts

; @routine 08:DC6D ObjectVelocityFromRecordOf
; WHAT IT DOES: Sets object X's velocity (+$2E, +$30, +$32) to an eighth of
; the values at +$2E, +$30, +$32 of object Y's Super FX record.
; HOW: Each value is shifted right three times keeping its sign.
; Entry: M 8-bit, X 16-bit; X = the object to set, Y = the object whose
; record is read. Exit: A = the Z velocity (M 8-bit); X, Y unchanged; N, Z
; from X; C from the last shift.
%at($08DC6D)
ObjectVelocityFromRecordOf:
    phx
    phy
    rep.b #$20
    lda.w !obj_22,y
    txy
    tax
    lda.l !gsu_rec_vel_x,x
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !obj_vel_x,y
    lda.l !gsu_rec_vel_y,x
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !obj_vel_y,y
    lda.l !gsu_rec_vel_z,x
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !obj_vel_z,y
    sep.b #$20
    ply
    plx
    rtl

; @routine 08:D86F GsuRecordFromObject
; WHAT IT DOES: Copies object X's state into its Super FX record: +$53,
; +$55, +$57 to +$0C, +$0E, +$10; +$20 to +$14; its position (+$0C, +$0E,
; +$10) to +$1E, +$20, +$22; its +$15 byte, as the high byte, to +$24; and
; the low 7 bits of +$5A to +$12.
; HOW: 16-bit copies in that order, when the object has a record.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A = the last value (0 with no
; record); X = Y = the object; M 8-bit; N, Z from X.
%at($08D86F)
GsuRecordFromObject:
    rep.b #$20
    txy
    lda.w !obj_22,x
    beq .lD8BE
    tax
    lda.w !obj_53,y
    sta.l !gsu_rec_pos_x,x
    lda.w !obj_55,y
    sta.l !gsu_rec_pos_y,x
    lda.w !obj_57,y
    sta.l !gsu_rec_pos_z,x
    lda.w !obj_20,y
    sta.l !gsu_rec_14,x
    lda.w !obj_pos_x,y
    sta.l !gsu_rec_1e,x
    lda.w !obj_pos_y,y
    sta.l !gsu_rec_20,x
    lda.w !obj_pos_z,y
    sta.l !gsu_rec_22,x
    lda.w !obj_15,y
    and.w #$00FF
    xba
    sta.l !gsu_rec_24,x
    lda.w !obj_5a,y
    and.w #$007F
    sta.l !gsu_rec_12,x
.lD8BE:
    sep.b #$20
    tyx
    rtl

; @routine 08:D5DF ObjectVelocityFromRecordScaled
; WHAT IT DOES: As ObjectVelocityFromRecord, with each value then scaled by
; three quarters: X and Z velocity = 3/32 of the record values of the
; object named by object X's +$27.
; HOW: Shift right three times keeping the sign, then ScaleThreeQuarters.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = the Z velocity
; (M 8-bit); X, Y = the object; $3E = the last half; N, Z from X; C, V from
; the last scaling add.
%at($08D5DF)
ObjectVelocityFromRecordScaled:
    ldy.w !obj_27,x
    rep.b #$20
    lda.w !obj_22,y
    txy
    tax
    lda.l !gsu_rec_vel_x,x
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    jsl.l ScaleThreeQuarters
    sta.w !obj_vel_x,y
    lda.l !gsu_rec_vel_z,x
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    jsl.l ScaleThreeQuarters
    sta.w !obj_vel_z,y
    tyx
    sep.b #$20
    rts
