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

; @routine 09:FBB2 GsuRecordBits60
; WHAT IT DOES: Keeps bits 5-6 of the word at +$40 of the two records whose
; addresses are at $70:2476 and $70:2478, at $1A24 and $1A26.
; HOW: The data bank is $09 inside (so $1A24 is work RAM).
; Entry: any widths. Exit: A = the second value (16-bit); X, P unchanged.
%at($09FBB2)
GsuRecordBits60:
    php
    phx
    phb
    phk
    plb
    rep.b #$30
    lda.l !gsu_match_2476
    tax
    lda.l !gsu_rec_40,x
    and.w #$0060
    sta.w !wram_1A24
    lda.l !gsu_match_2478
    tax
    lda.l !gsu_rec_40,x
    and.w #$0060
    sta.w !wram_1A26
    plb
    plx
    plp
    rtl

; @routine 08:CB98 GsuRecordNew422
; WHAT IT DOES: Gives object X a new Super FX record from the second pool
; (mode $0422, linked into the list at $70:2398), with +$0A = 0, filled from
; the object.
; HOW: Runs Super FX program $04:8B64, which takes $2A bytes from the pool
; whose free list is at $70:1D70 and sets the record's flags and mode; then
; the shared tail at $08:CC7C ($1F4C = the +$0A word).
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = 0 when the
; object got a record, $00FF when the pool had no room; N, Z from A's low
; byte; M 8-bit; X unchanged; Y = X with a record, else unchanged; $1F4C =
; the +$0A word.
%at($08CB98)
GsuRecordNew422:
    phx
    phy
    lda.b #$04
    ldx.w #$8B64
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F4C
    sep.b #$20
    brl GsuRecordNew_tail

; @routine 08:CBB2 GsuRecordNew42D
; WHAT IT DOES: Gives object X a new Super FX record from the first pool
; (mode $042D, flags $20), with +$0A = $90E8, filled from the object.
; HOW: Runs Super FX program $04:8C95, which takes $34 bytes from the pool
; whose free list is at $70:1098; then the shared tail at $08:CC7C.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CBB2)
GsuRecordNew42D:
    phx
    phy
    lda.b #$04
    ldx.w #$8C95
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$90E8
    sta.w !wram_1F4C
    sep.b #$20
    jmp.w GsuRecordNew_tail

; @routine 08:CBE6 GsuRecordNew424
; WHAT IT DOES: Gives object X a new Super FX record from the first pool
; (mode $0424, flags 8, +$2A = $40), with +$0A = $910A, filled from the
; object.
; HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CBE6)
GsuRecordNew424:
    phx
    phy
    lda.b #$04
    ldx.w #$8C0A
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$910A
    sta.w !wram_1F4C
    sep.b #$20
    bra GsuRecordNew_tail

; @routine 08:CC18 GsuRecordNew424Alt
; WHAT IT DOES: As GsuRecordNew424, with +$0A = $90E8.
; HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CC18)
GsuRecordNew424Alt:
    phx
    phy
    lda.b #$04
    ldx.w #$8C0A
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$90E8
    sta.w !wram_1F4C
    sep.b #$20
    bra GsuRecordNew_tail

; @routine 08:CC63 GsuRecordNew426
; WHAT IT DOES: Gives object X a new Super FX record from the first pool
; (mode $0426, flags 8, +$2A = $40), with +$0A = $910A, filled from the
; object.
; HOW: Runs Super FX program $04:8B33; then falls into the shared tail at
; $08:CC7C, which the four routines above branch to.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CC63)
GsuRecordNew426:
    phx
    phy
    lda.b #$04
    ldx.w #$8B33
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$910A
    sta.w !wram_1F4C
    sep.b #$20
    bra GsuRecordNew_tail
GsuRecordNew_tail:                  ; the four routines above join here
    rep.b #$20
    lda.l !gsu_record_arg
    sta.w !obj_22,x
    bne .lCC8A
    brl .lCC9E
.lCC8A:
    phx
    txy
    tax
    lda.w !wram_1F4C
    sta.l !gsu_rec_0a,x
    plx
    sep.b #$20
    jsl.l GsuRecordFromObject
    lda.b #$00
    rtl
.lCC9E:
    sep.b #$20
    lda.b #$FF
    rtl

; @routine 08:B80A GsuRecordLower82
; WHAT IT DOES: Lowers the word at +$82 of the Super FX record Y by an
; amount scaled from A, not below 0; when Y is the record at $70:2476, sets
; bit 6 of +$25 of the object at $0346.
; HOW: $70:238E = Y and $70:2390 = A as the high byte are the inputs of
; Super FX program $04:BF75 (the amount times 2 times $200 by FMULT,
; subtracted); ObjectFlag40IfMatch before it runs.
; Entry: M 8-bit, X 16-bit; A low byte = amount, Y = record. Exit: A = Y's
; high byte and the screen mode value; X, Y unchanged; M 8-bit, X 16-bit;
; N, Z from X; C from ObjectFlag40IfMatch's compare.
%at($08B80A)
GsuRecordLower82:
    phx
    rep.b #$20
    and.w #$00FF
    xba
    sta.l !gsu_in_amount
    tya
    sta.l !gsu_record_arg
    jsr.w ObjectFlag40IfMatch
    sep.b #$20
    lda.b #$04
    ldx.w #$BF75
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    sep.b #$20
    plx
    rtl

; @routine 08:B7D6 GsuRecordRaise6A
; WHAT IT DOES: Unless bit 0 of +$40 of the Super FX record Y is set,
; raises its word at +$6A by an amount scaled from A, to at most $180; when
; Y is the record at $70:2476, sets bit 6 of +$25 of the object at $0346.
; HOW: As GsuRecordLower82, with Super FX program $04:BF51 (the amount
; times 2 times $180 by FMULT, added and clamped).
; Entry: M 8-bit, X 16-bit; A low byte = amount, Y = record. Exit: bit set:
; A = 1; else as GsuRecordLower82. X, Y unchanged; M 8-bit, X 16-bit; N, Z
; from X; C unchanged when skipped, else from ObjectFlag40IfMatch.
%at($08B7D6)
GsuRecordRaise6A:
    phx
    rep.b #$20
    and.w #$00FF
    xba
    sta.l !gsu_in_amount
    tya
    sta.l !gsu_record_arg
    tax
    lda.l !gsu_rec_40,x
    and.w #$0001
    bne .lB806
    lda.l !gsu_record_arg
    jsr.w ObjectFlag40IfMatch
    sep.b #$20
    lda.b #$04
    ldx.w #$BF51
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
.lB806:
    sep.b #$20
    plx
    rtl

; @routine 08:BF94 ObjectTurnFromRecord
; WHAT IT DOES: Takes object Y's angles from its Super FX record X (+$29 to
; +$16, +$25 to +$15) and sets its turn values for drawing: +$5D = +$29 +
; 8 + a lean step, and +$18 (and +$19 when $0D62 is set) = $80 + the
; direction byte from the table at $08:C056 relative to the view object at
; $1A64 (and the one at $1A9F).
; HOW: The lean step is (+$16 + the view's +$13) / 2 with its low two bits
; cleared toward zero, limited to $F8..8 by signed compares, negated when
; +$15 is more than $40 either way from the view's heading; ObjectTableC056
; turns the negated view heading into the direction byte. The view
; objects are read through the direct page (bank 0).
; Entry: M 16-bit, X 16-bit, D = $0000; X = record, Y = object. Exit: M
; 8-bit; X, Y unchanged; A high byte from +$5D; A low byte and N, Z: $0D62
; clear: 0, Z set; else the +$19 value, N set; C, V from ObjectTableC056.
%at($08BF94)
ObjectTurnFromRecord:
    sep.b #$20
    lda.l !gsu_rec_29,x
    sta.w !obj_16,y
    lda.l !gsu_rec_25,x
    sta.w !obj_15,y
    phx
    ldx.w #!menu_1a64
    lda.b 17,x
    eor.b #$FF
    inc a
    sta.w !wram_1F5A
    lda.b 19,x
    sta.w !wram_1F2B
    plx
    lda.w !obj_16,y
    clc
    adc.w !wram_1F2B
    cmp.b #$80
    ror a
    bpl .lBFCA
    eor.b #$FF
    inc a
    and.b #$FC
    eor.b #$FF
    inc a
.lBFCA:
    and.b #$FC
    cmp.b #$08
    bmi .lBFD2
    lda.b #$08
.lBFD2:
    cmp.b #$F8
    bpl .lBFD8
    lda.b #$F8
.lBFD8:
    sta.w !wram_1F58
    lda.w !obj_15,y
    sec
    sbc.w !wram_1F5A
    bpl .lBFE7
    eor.b #$FF
    inc a
.lBFE7:
    cmp.b #$40
    bmi .lBFF4
    lda.w !wram_1F58
    eor.b #$FF
    inc a
    sta.w !wram_1F58
.lBFF4:
    rep.b #$20
    lda.w !wram_1F58
    bit.w #$0080
    beq .lC003
    ora.w #$FF00
    bra .lC006
.lC003:
    and.w #$00FF
.lC006:
    clc
    adc.w #$0008
    clc
    adc.w !obj_29,y
    sta.w !obj_5d,y
    sep.b #$20
    jsr.w ObjectTableC056
    lda.w !wram_1F5A
    ora.b #$80
    sta.w !obj_18,y
    lda.w !wram_0D62
    beq .lC03B
    phx
    ldx.w #!view_b
    lda.b 17,x
    eor.b #$FF
    inc a
    sta.w !wram_1F5A
    plx
    jsr.w ObjectTableC056
    lda.w !wram_1F5A
    ora.b #$80
    sta.w !obj_19,y
.lC03B:
    rts

; @routine 08:CE02 ObjectSyncFromRecord
; WHAT IT DOES: Rebuckets object X, marks its record (GsuRecordFlag4), and
; takes its position back from the record (+$1E/+$20/+$22 to +$0C/+$0E/
; +$10), after giving the record its heading (+$15 as the high byte of
; +$24) and +$12 (+$5A's low 7 bits); bit 1 of +$24 follows bit 1 of the
; record's +$1C. Returns whether the record's +$18 is set.
; HOW: The object is in Y and the record in X while copying; $1F3C keeps
; the record's +$18; ends in the tail at $08:CE60 (shared with
; ObjectSyncToRecord) or, with no record, $08:CE69.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: X = the object; M
; 8-bit; V unchanged. Record: carry clear; Y = $1F3C; A low byte = 1 when
; it is non-zero (N, Z from that), else A = the new +$24 when the bit was
; set, 0 when not (Z set). No record: A = 0, Y = X, $1F3C = 0, Z and carry
; set.
%at($08CE02)
ObjectSyncFromRecord:
    jsl.l ObjectRebucket
    jsl.l GsuRecordFlag4
    lda.w !obj_24,x
    and.b #$FD
    sta.w !obj_24,x
    rep.b #$20
    txy
    lda.w !obj_22,y
    beq ObjectSync_none
    tax
    lda.l !gsu_rec_18,x
    sta.w !wram_1F3C
    lda.w !obj_15,y
    and.w #$00FF
    xba
    sta.l !gsu_rec_24,x
    lda.w !obj_5a,y
    and.w #$007F
    sta.l !gsu_rec_12,x
    lda.l !gsu_rec_1e,x
    sta.w !obj_pos_x,y
    lda.l !gsu_rec_20,x
    sta.w !obj_pos_y,y
    lda.l !gsu_rec_22,x
    sta.w !obj_pos_z,y
    lda.l !gsu_rec_1c,x
    tyx
    and.w #$0002
    sep.b #$20
    beq ObjectSync_tail
    lda.w !obj_24,x
    ora.b #$02
    sta.w !obj_24,x
ObjectSync_tail:                    ; ObjectSyncToRecord joins here too
    ldy.w !wram_1F3C
    beq .lCE67
    lda.b #$01
.lCE67:
    clc
    rtl
ObjectSync_none:                    ; no record (ObjectSyncToRecord joins here too)
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F3C
    sep.b #$20
    sep.b #$20
    lda.b #$00
    sec
    rtl

; @routine 08:CEC3 ObjectSyncToRecord
; WHAT IT DOES: As ObjectSyncFromRecord, the other way: after the heading
; and +$12, the object's position goes to the record (+$0C/+$0E/+$10 to
; +$1E/+$20/+$22); +$24 is left alone.
; HOW: As ObjectSyncFromRecord, jumping to its tail at $08:CE60 (or
; $08:CE69 with no record).
; Entry: as ObjectSyncFromRecord. Exit: as ObjectSyncFromRecord, except
; that with a record and $1F3C zero A = the object's +$10.
%at($08CEC3)
ObjectSyncToRecord:
    jsl.l ObjectRebucket
    jsl.l GsuRecordFlag4
    rep.b #$20
    txy
    lda.w !obj_22,y
    beq ObjectSync_none
    tax
    lda.w !obj_15,y
    and.w #$00FF
    xba
    sta.l !gsu_rec_24,x
    lda.w !obj_5a,y
    and.w #$007F
    sta.l !gsu_rec_12,x
    lda.l !gsu_rec_18,x
    sta.w !wram_1F3C
    lda.w !obj_pos_x,y
    sta.l !gsu_rec_1e,x
    lda.w !obj_pos_y,y
    sta.l !gsu_rec_20,x
    lda.w !obj_pos_z,y
    sta.l !gsu_rec_22,x
    tyx
    sep.b #$20
    jmp.w ObjectSync_tail

; @routine 08:D01E RecordContactFlags
; WHAT IT DOES: Adds to the flags in $1F38 what a record's contact word
; ($1F3A, its +$64) and flags ($1F36, its +$16) say: when the contact word
; is non-zero, 4 plus its lowest set bit of bits 0-2 (or the whole word
; when none of them is set); 8 for flags bit 0; $80 for flags bit 11.
; HOW: BIT #imm tests (Z only), ORs into $1F38 through the data bank.
; Entry: M 16-bit, X 16-bit. Exit: A = $1F38; N, Z from it; C, V
; unchanged.
%at($08D01E)
RecordContactFlags:
    lda.w !wram_1F3A
    beq .lD04A
    bit.w #$0001
    beq .lD02D
    lda.w #$0001
    bra .lD041
.lD02D:
    bit.w #$0002
    beq .lD037
    lda.w #$0002
    bra .lD041
.lD037:
    bit.w #$0004
    beq .lD041
    lda.w #$0004
    bra .lD041
.lD041:
    ora.w #$0004
    ora.w !wram_1F38
    sta.w !wram_1F2D+11
.lD04A:
    lda.w !wram_1F36
    bit.w #$0001
    beq .lD05B
    lda.w !wram_1F2D+11
    ora.w #$0008
    sta.w !wram_1F2D+11
.lD05B:
    lda.w !wram_1F2D+9
    bit.w #$0800
    beq .lD06C
    lda.w !wram_1F38
    ora.w #$0080
    sta.w !wram_1F38
.lD06C:
    lda.w !wram_1F2D+11
    rts

; @routine 08:B402 GsuRecordLand
; WHAT IT DOES: When $70:2482 is non-zero and object X's record has bit 6
; of +$2C and bit 4 of +$8A set, and not bit 8 of +$40: stops the record
; (+$2E, +$30, +$32, +$36, +$34, +$38 zeroed) and, unless +$82 is already
; $200, sets +$82 to $200 and in +$40 clears bits 5-6 and sets bits 0 and
; 12.
; HOW: 16-bit tests and stores on the record (the object's +$22, used
; without a zero test). The stopping part is not reached by any script.
; Entry: M 8-bit, X 16-bit; X = object. Exit: X unchanged; M 8-bit; N, Z
; from X; A = the last value tested or written; C set by the +$82 compare
; when that is reached, else unchanged.
%at($08B402)
GsuRecordLand:
    rep.b #$20
    phx
    lda.l !gsu_record_ready
    beq .lB463
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_2c,x
    and.w #$0040
    beq .lB463
    lda.l !gsu_rec_8a,x
    and.w #$0010
    beq .lB463
    lda.l !gsu_rec_40,x
    and.w #$0100
    bne .lB463
    lda.w #$0000
    sta.l !gsu_rec_vel_x,x
    sta.l !gsu_rec_vel_y,x
    sta.l !gsu_rec_vel_z,x
    sta.l !gsu_rec_36,x
    sta.l !gsu_rec_34,x
    sta.l !gsu_rec_38,x
    lda.l !gsu_rec_82,x
    cmp.w #$0200
    beq .lB463
    lda.w #$0200
    sta.l !gsu_rec_82,x
    lda.l !gsu_rec_40,x
    and.w #$FF9F
    ora.w #$1001
    sta.l !gsu_rec_40,x
.lB463:
    plx
    sep.b #$20
    rts

; @routine 08:CF41 RecordNextContact
; WHAT IT DOES: Walks the contact list from the entry at $1F3C (entries
; are records linked through +$0E) to the next one with bit 1 of +$10
; set; for it, takes the touched record (its +$0C) and makes $1F3A that
; record's contact word (+$64, when bit 1 of its +$16 is set, else 0),
; $1F36 its flags (+$16), $1F38 their RecordContactFlags, and leaves $1F3C
; at the following entry. Carry set when one was found.
; HOW: 16-bit walk with X; Y = the touched record.
; Entry: M 8-bit, X 16-bit. Exit: M 8-bit; X unchanged. Found: carry set,
; A = $1F38, Y = the touched record, N, Z from X. Not found: carry clear,
; A = 0, Y unchanged; N, Z from X after a walk, Z set when $1F3C was 0.
%at($08CF41)
RecordNextContact:
    rep.b #$20
    lda.w !wram_1F3C
    beq .lCF8E
    phx
.lCF49:
    tax
    lda.l !contact_flags,x
    and.w #$0002
    bne .lCF5C
    lda.l !contact_next,x
    bne .lCF49
    plx
    bra .lCF8E
.lCF5C:
    lda.l !contact_record,x
    tay
    lda.l !contact_next,x
    sta.w !wram_1F3C
    tyx
    stz.w !wram_1F3A
    lda.l !gsu_rec_flags,x
    bit.w #$0002
    beq .lCF7C
    lda.l !gsu_rec_64,x
    sta.w !wram_1F3A
.lCF7C:
    lda.l !gsu_rec_flags,x
    sta.w !wram_1F36
    stz.w !wram_1F38
    jsr.w RecordContactFlags
    plx
    sep.b #$20
    sec
    rtl
.lCF8E:
    sep.b #$20
    clc
    rtl

; @routine 08:CF92 RecordContactSummary
; WHAT IT DOES: Sums up every contact of object X's record into $1F38:
; $10 when bit 0 of the record's +$1C is set ($80 more when bit 5 of
; $70:26F2 is too), bit 5 of its +$2C, then RecordContactFlags over the
; OR of the contact words ($1F3A) and flags ($1F36) of every touched record
; (+$0C) in the contact list from its +$18 whose entries have bit 1 of +$10.
; HOW: 16-bit walk with X through +$0E; Y keeps the entry while X is the
; touched record. With no record it returns 0 at once.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X unchanged; C, V
; unchanged. Record: A = $1F38, N, Z from it; Y = the last entry with bit 1
; set (unchanged when none). No record: A = 0, Z set.
%at($08CF92)
RecordContactSummary:
    rep.b #$20
    lda.w !obj_22,x
    bne .lCF9C
    brl .lD019
.lCF9C:
    phx
    tax
    stz.w !wram_1F38
    lda.l !gsu_rec_1c,x
    and.w #$0001
    beq .lCFC5
    lda.w !wram_1F38
    ora.w #$0010
    sta.w !wram_1F38
    lda.l !gsu_control_26f2
    and.w #$0020
    beq .lCFC5
    lda.w !wram_1F38
    ora.w #$0080
    sta.w !wram_1F38
.lCFC5:
    lda.l !gsu_rec_2c,x
    and.w #$0020
    ora.w !wram_1F38
    sta.w !wram_1F38
    stz.w !wram_1F36
    stz.w !wram_1F3A
    lda.l !gsu_rec_18,x
.lCFDC:
    tax
    beq .lD012
    lda.l !contact_flags,x
    and.w #$0002
    beq .lD00C
    lda.l !contact_record,x
    txy
    tax
    lda.l !gsu_rec_flags,x
    bit.w #$0002
    beq .lD005
    lda.l !gsu_rec_64,x
    ora.w !wram_1F3A
    sta.w !wram_1F3A
    lda.l !gsu_rec_flags,x
.lD005:
    ora.w !wram_1F36
    sta.w !wram_1F36
    tyx
.lD00C:
    lda.l !contact_next,x
    bra .lCFDC
.lD012:
    plx
    jsr.w RecordContactFlags
    sep.b #$20
    rtl
.lD019:
    sep.b #$20
    lda.b #$00
    rtl

; @routine 08:C8FF RecordButtonSequence
; WHAT IT DOES: Watches the buttons in $1AF0 for a four-press sequence: bit
; 6, then bit 7, then bit 14, each within 5 frames of the last, then bit 11
; between 12 and 16 frames later. The full sequence plays sound $48 and
; gives record X bit 13 of +$40, $180 at +$6A and $1000 at +$400. Too slow,
; too early or done: back to the start.
; HOW: $1A33 counts frames and $1A35 is the stage, both reset together;
; the limits are signed compares (BPL/BMI). A stage above 3 gives the
; reward at once. Only stages 0 and 1 are reached by a script.
; Entry: M 16-bit, X 16-bit; X = record. Exit: X, Y unchanged; A and N, Z,
; C as the last load or compare on the path taken; V unchanged.
%at($08C8FF)
RecordButtonSequence:
    inc.w !seq_count
    lda.w !seq_stage
    beq .lC938
    cmp.w #$0001
    beq .lC94E
    cmp.w #$0002
    beq .lC964
    cmp.w #$0003
    beq .lC97A
.reward:
    lda.w #$0048
    jsl.l SoundPort0QueuePush
    lda.l !gsu_rec_40,x
    ora.w #$2000
    sta.l !gsu_rec_40,x
    lda.w #$0180
    sta.l !gsu_rec_6a,x
    lda.w #$1000
    sta.l !gsu_rec_400,x
    bra .lC999
.lC938:
    lda.w !seq_count
    cmp.w #$0005
    bpl .lC999
    lda.w !seq_input
    bit.w #$0040
    beq .lC94C
    jml.l RecordButtonSequence_next
.lC94C:
    bra RecordButtonSequence_done
.lC94E:
    lda.w !seq_count
    cmp.w #$0005
    bpl .lC999
    lda.w !seq_input
    bit.w #$0080
    beq .lC962
    jml.l RecordButtonSequence_next
.lC962:
    bra RecordButtonSequence_done
.lC964:
    lda.w !seq_count
    cmp.w #$0005
    bpl .lC999
    lda.w !seq_input
    bit.w #$4000
    beq .lC978
    jml.l RecordButtonSequence_next
.lC978:
    bra RecordButtonSequence_done
.lC97A:
    lda.w !seq_count
    cmp.w #$0011
    bpl .lC999
    lda.w !seq_input
    bit.w #$0800
    bne .lC98E
    jml.l RecordButtonSequence_done
.lC98E:
    lda.w !seq_count
    cmp.w #$000C
    bmi .lC999
    jmp.w .reward
.lC999:
    stz.w !seq_count
    stz.w !seq_stage
RecordButtonSequence_done:          ; reached by the long jump above
    rts
RecordButtonSequence_next:          ; a press in time: the next stage
    stz.w !seq_count
    inc.w !seq_stage
    bra RecordButtonSequence_done

; @routine 08:C9A9 ObjectPoseFromRecord
; WHAT IT DOES: Takes object X's pose back from its Super FX record:
; position (+$1E/+$20/+$22 to +$0C/+$0E/+$10) and the angle bytes +$27,
; +$29, +$25 to +$14, +$16, +$15; copies the record's +$40 to its +$90.
; Before that clears bits 5 and 6 of the object's +$25, and when its +$36
; is 1 makes it 2 and sets bit 10 of the record's flags (+$16), else
; clears that bit.
; HOW: The record is the object's +$22 (no zero test); X is the record and
; Y the object while copying.
; Entry: M 8-bit, X 16-bit; X = object. Exit: X unchanged; Y = X; M 8-bit;
; A = the high byte of its +$22 and the +$25 byte; N, Z from X; C from the
; +$36 compare.
%at($08C9A9)
ObjectPoseFromRecord:
    sep.b #$20
    lda.w !obj_25,x
    and.b #$DF
    sta.w !obj_25,x
    lda.w !obj_25,x
    and.b #$BF
    sta.w !obj_25,x
    phx
    lda.w !obj_36,x
    cmp.b #$01
    bne .lC9DB
    lda.b #$02
    sta.w !obj_36,x
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_flags,x
    ora.w #$0400
    sta.l !gsu_rec_flags,x
    bra .lC9EC
.lC9DB:
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_flags,x
    and.w #$FBFF
    sta.l !gsu_rec_flags,x
.lC9EC:
    plx
    sep.b #$20
    phx
    txy
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_40,x
    sta.l !gsu_rec_90,x
    lda.l !gsu_rec_1e,x
    sta.w !obj_pos_x,y
    lda.l !gsu_rec_20,x
    sta.w !obj_pos_y,y
    lda.l !gsu_rec_22,x
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.l !gsu_rec_27,x
    sta.w !obj_14,y
    lda.l !gsu_rec_29,x
    sta.w !obj_16,y
    lda.l !gsu_rec_25,x
    sta.w !obj_15,y
    plx
    rtl
