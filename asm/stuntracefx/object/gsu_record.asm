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

; @routine 08:CBCC GsuRecordNew424Piece
; WHAT IT DOES: As GsuRecordNew424, with +$0A = $907A: the record of a
; flying piece or a firework shell (bank 09's course objects).
; HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CBCC)
GsuRecordNew424Piece:
    phx
    phy
    lda.b #$04
    ldx.w #$8C0A
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$907A
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

; @routine 08:CC4A GsuRecordNewCamera
; WHAT IT DOES: As GsuRecordNew424, from Super FX program $04:8C6C, with
; +$0A = $916D: the record of a replay camera's follower.
; HOW: Runs Super FX program $04:8C6C; then the shared tail at $08:CC7C.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CC4A)
GsuRecordNewCamera:
    phx
    phy
    lda.b #$04
    ldx.w #$8C6C
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$916D
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

; @routine 08:B72E GsuRecordFlags8ASet
; WHAT IT DOES: Sets the bits in A's low byte in the word at +$8A of the
; Super FX record Y.
; HOW: The record in X (pushed and pulled around it); 16-bit long
; accesses.
; Entry: X 16-bit; Y = record; A low byte = bits. Exit: A = the new word;
; M 8-bit; X, Y unchanged; N, Z from X.
%at($08B72E)
GsuRecordFlags8ASet:
    phx
    phy
    tyx
    rep.b #$20
    and.w #$00FF
    ora.l !gsu_rec_8a,x
    sta.l !gsu_rec_8a,x
    sep.b #$20
    ply
    plx
    rtl

; @routine 08:B743 GsuRecordFlags8AClear
; WHAT IT DOES: Clears the bits in A's low byte in the word at +$8A of the
; Super FX record Y.
; HOW: As GsuRecordFlags8ASet, with EOR #$00FF and AND.
; Entry: X 16-bit; Y = record; A low byte = bits. Exit: A = the new word;
; M 8-bit; X, Y unchanged; N, Z from X.
%at($08B743)
GsuRecordFlags8AClear:
    phx
    phy
    tyx
    rep.b #$20
    and.w #$00FF
    eor.w #$00FF
    and.l !gsu_rec_8a,x
    sta.l !gsu_rec_8a,x
    sep.b #$20
    ply
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

; @routine 08:CF0B ObjectSyncAngles
; WHAT IT DOES: As ObjectSyncToRecord without the position: the object is
; rebucketed, its record marked (GsuRecordFlag4) and given its heading
; (+$15 as the high byte of +$24) and +$12 (+$5A's low 7 bits), $1F3C =
; the record's +$18.
; HOW: As ObjectSyncToRecord, ending in the tail at $08:CE60 (or $08:CE69
; with no record).
; Entry: as ObjectSyncFromRecord. Exit: as ObjectSyncFromRecord, except
; that with a record and $1F3C zero A = the record's +$18.
%at($08CF0B)
ObjectSyncAngles:
    jsl.l ObjectRebucket
    jsl.l GsuRecordFlag4
    rep.b #$20
    txy
    lda.w !obj_22,y
    bne .record
    brl ObjectSync_none
.record:
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
    tyx
    sep.b #$20
    jmp.w ObjectSync_tail

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

; @routine 08:C5A5 PlayerControlsFrame
; WHAT IT DOES: The players' controls for a race frame: for player 1, and
; player 2 when $0D62 is set, puts the buttons held this frame ($0309 or
; $030D) and last frame ($1F23 or $1F25) at $1AF0 and $1AF2, the player at
; $1A5D, and runs the car's controls (CarControlsToRecord on the object at
; $0346 or $034A) and its linked object's handler (ObjectLinkedHandlerByState
; with Y = the player); then keeps this frame's buttons as last frame's and
; clears $0309 and $030D.
; HOW: Data bank $7E, the caller's put back at the end; 8-bit A except for
; the button words.
; Entry: any widths. Exit: A = player 2's buttons (read before $030D is
; cleared); M 8-bit, X 16-bit; X, Y as the last handler left them; N, Z from
; the caller's data bank.
%at($08C5A5)
PlayerControlsFrame:
    sep.b #$20
    rep.b #$10
    phb
    lda.b #$7E
    pha
    plb
    rep.b #$20
    lda.w !joy1_held
    sta.w !seq_input
    lda.w !gsu_ab13_polls+2
    sta.w !seq_input+2
    sep.b #$20
    stz.w !current_player
    ldx.w !wram_0346
    jsl.l CarControlsToRecord
    ldy.w #$0000
    jsl.l ObjectLinkedHandlerByState
    lda.w !wram_0D62
    beq .lC5F7
    rep.b #$20
    lda.w !joy2_held
    sta.w !seq_input
    lda.w !gsu_ab13_polls+4
    sta.w !seq_input+2
    sep.b #$20
    lda.b #$01
    sta.w !current_player
    ldx.w !wram_034A
    jsl.l CarControlsToRecord
    ldy.w #$0001
    jsl.l ObjectLinkedHandlerByState
.lC5F7:
    rep.b #$20
    lda.w !joy1_held
    sta.w !gsu_ab13_polls+2
    lda.w !joy2_held
    sta.w !gsu_ab13_polls+4
    stz.w !joy1_held
    stz.w !joy2_held
    sep.b #$20
    plb
    rtl

; @routine 08:C60F CarControlsToRecord
; WHAT IT DOES: Turns a player's buttons ($1AF0; last frame's at $1AF2) into
; car X's controls in its Super FX record (+$22; none: nothing). With bit 2
; of the car's +$25 set it only marks the record: +$40 bit 9, and +$42 kept
; to $D800 unless it has bit 12. Otherwise, unless the record's +$1C has bit
; 4: SELECT newly pressed runs the button sequence (RecordButtonSequence) and
; sets bit 0 of the linked object's (+$4F) +$25, and bit 1 with L and R held;
; in a two-player race ($0D62) the car's +$29 counts up to a random limit
; $6E-$71 ($1F2B), and is reset to $FFFF by B, Y, A or X once it is past 9;
; at the limit record +$64 gets bit 3 and the linked object bits 0 and 1.
; Then, with $0D53 clear and +$42 bit 12 clear: +$40 loses bit 9, +$42 keeps
; only $D800, and the buttons held set +$42 bits: LEFT 0 (else RIGHT 1), L 2
; (else R 3), A $2040 (and $20 below speed $32 at +$4A), Y $400 when +$6A is
; not 0 (else sound $5E and the player's flag $10, at $1A5E or $1A60 by
; $1A5D, once), B $10; X newly pressed 7 when +$5E is 0 and +$40 bit 8 is
; clear. Without Y the player's flag $10 is cleared. With +$40 bit 8 set,
; DOWN takes $600 from +$26 and UP adds it.
; HOW: 16-bit A for the button and record words, 8-bit for the +$25 flags;
; the record through long accesses to bank $70, the rest through the data
; bank. Most tests branch over a long jump to the next one.
; Entry: data bank $7E; X = the car's object. Exit: X, Y = the object; M
; 8-bit, X 16-bit; N, Z from X; A, C and V as the last test or sum left
; them.
%at($08C60F)
CarControlsToRecord:
    sep.b #$20
    rep.b #$10
    stx.w !draw_side+17
    lda.w !obj_25,x
    and.b #$04
    bne .lC621
    jml.l .lC64A
.lC621:
    rep.b #$20
    phx
    txy
    lda.w !obj_22,x
    beq .lC646
    tax
    lda.l !gsu_rec_40,x
    ora.w #$0200
    sta.l !gsu_rec_40,x
    lda.l !gsu_rec_42,x
    bit.w #$1000
    bne .lC646
    and.w #$D800
    sta.l !gsu_rec_42,x
.lC646:
    plx
    sep.b #$20
    rtl
.lC64A:
    rep.b #$20
    phx
    txy
    lda.w !obj_22,x
    bne .lC656
    brl .lC8FB
.lC656:
    tax
    lda.l !gsu_rec_1c,x
    and.w #$0010
    beq .lC663
    brl .lC8FB
.lC663:
    lda.w !seq_input
    bit.w #$2000
    bne .lC66F
    jml.l .lC6AE
.lC66F:
    jsr.w RecordButtonSequence
    lda.w !seq_input+2
    bit.w #$2000
    beq .lC67E
    jml.l .lC6AE
.lC67E:
    phy
    lda.w !obj_link_w,y
    tay
    sep.b #$20
    lda.w !obj_25,y
    ora.b #$01
    sta.w !obj_25,y
    lda.w !seq_input
    bit.b #$20
    bne .lC698
    jml.l .lC6AB
.lC698:
    lda.w !seq_input
    bit.b #$10
    bne .lC6A3
    jml.l .lC6AB
.lC6A3:
    lda.w !obj_25,y
    ora.b #$02
    sta.w !obj_25,y
.lC6AB:
    rep.b #$20
    ply
.lC6AE:
    lda.w !wram_0D62
    and.w #$00FF
    bne .lC6B9
    brl .lC73E
.lC6B9:
    jsl.l RandomStepLong
    and.w #$0003
    clc
    adc.w #$006E
    sta.w !wram_1F2B
    lda.w !obj_29,y
    bmi .lC73E
    cmp.w !wram_1F2B
    bpl .lC714
    inc a
    sta.w !obj_29,y
    cmp.w #$000A
    bmi .lC73E
    lda.w !seq_input
    bit.w #$8000
    beq .lC6E6
    jml.l .lC70C
.lC6E6:
    lda.w !seq_input
    bit.w #$4000
    beq .lC6F2
    jml.l .lC70C
.lC6F2:
    lda.w !seq_input
    bit.w #$0080
    beq .lC6FE
    jml.l .lC70C
.lC6FE:
    lda.w !seq_input
    bit.w #$0040
    beq .lC70A
    jml.l .lC70C
.lC70A:
    bra .lC73E
.lC70C:
    lda.w #$FFFF
    sta.w !obj_29,y
    bra .lC73E
.lC714:
    lda.l !gsu_rec_64,x
    bit.w #$0008
    bne .lC73E
    ora.w #$0008
    sta.l !gsu_rec_64,x
    phy
    lda.w !obj_link_w,y
    tay
    sep.b #$20
    lda.w !obj_25,y
    ora.b #$01
    sta.w !obj_25,y
    lda.w !obj_25,y
    ora.b #$02
    sta.w !obj_25,y
    rep.b #$20
    ply
.lC73E:
    lda.w !wram_0D53
    and.w #$00FF
    beq .lC749
    brl .lC8FB
.lC749:
    lda.l !gsu_rec_40,x
    and.w #$FDFF
    sta.l !gsu_rec_40,x
    lda.l !gsu_rec_42,x
    bit.w #$1000
    beq .lC760
    brl .lC8FB
.lC760:
    lda.l !gsu_rec_42,x
    and.w #$D800
    sta.l !gsu_rec_42,x
    lda.w !seq_input
    bit.w #$0200
    bne .lC777
    jml.l .lC784
.lC777:
    lda.l !gsu_rec_42,x
    ora.w #$0001
    sta.l !gsu_rec_42,x
    bra .lC79B
.lC784:
    lda.w !seq_input
    bit.w #$0100
    bne .lC790
    jml.l .lC79B
.lC790:
    lda.l !gsu_rec_42,x
    ora.w #$0002
    sta.l !gsu_rec_42,x
.lC79B:
    lda.w !seq_input
    bit.w #$0020
    bne .lC7A7
    jml.l .lC7B4
.lC7A7:
    lda.l !gsu_rec_42,x
    ora.w #$0004
    sta.l !gsu_rec_42,x
    bra .lC7CB
.lC7B4:
    lda.w !seq_input
    bit.w #$0010
    bne .lC7C0
    jml.l .lC7CB
.lC7C0:
    lda.l !gsu_rec_42,x
    ora.w #$0008
    sta.l !gsu_rec_42,x
.lC7CB:
    lda.w !seq_input
    bit.w #$0080
    bne .lC7D7
    jml.l .lC7F7
.lC7D7:
    lda.l !gsu_rec_42,x
    ora.w #$2040
    sta.l !gsu_rec_42,x
    lda.l !gsu_speeds,x
    sec
    sbc.w #$0032
    bpl .lC7F7
    lda.l !gsu_rec_42,x
    ora.w #$0020
    sta.l !gsu_rec_42,x
.lC7F7:
    lda.w !seq_input
    bit.w #$4000
    bne .lC803
    jml.l .lC84C
.lC803:
    lda.l !gsu_in_6a,x
    beq .lC816
    lda.l !gsu_rec_42,x
    ora.w #$0400
    sta.l !gsu_rec_42,x
    bra .lC84C
.lC816:
    lda.w !current_player
    and.w #$00FF
    beq .lC823
    lda.w !current_player+3
    bra .lC826
.lC823:
    lda.w !current_player+1
.lC826:
    bit.w #$0010
    bne .lC84A
    pha
    lda.w #$005E
    jsl.l SoundPort0QueuePush
    pla
    ora.w #$0010
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC846
    pla
    sta.w !current_player+3
    bra .lC84A
.lC846:
    pla
    sta.w !current_player+1
.lC84A:
    bra .lC879
.lC84C:
    lda.w !current_player
    and.w #$00FF
    beq .lC859
    lda.w !current_player+3
    bra .lC85C
.lC859:
    lda.w !current_player+1
.lC85C:
    bit.w #$0010
    beq .lC879
    pha
    pla
    and.w #$FFEF
    pha
    lda.w !current_player
    and.w #$00FF
    beq .lC875
    pla
    sta.w !current_player+3
    bra .lC879
.lC875:
    pla
    sta.w !current_player+1
.lC879:
    lda.w !seq_input
    bit.w #$0040
    bne .lC885
    jml.l .lC8AB
.lC885:
    lda.w !seq_input+2
    bit.w #$0040
    beq .lC891
    jml.l .lC8AB
.lC891:
    lda.l !gsu_rec_5e,x
    bne .lC8AB
    lda.l !gsu_rec_40,x
    and.w #$0100
    bne .lC8AB
    lda.l !gsu_rec_42,x
    ora.w #$0080
    sta.l !gsu_rec_42,x
.lC8AB:
    lda.w !seq_input
    bit.w #$8000
    bne .lC8B7
    jml.l .lC8C2
.lC8B7:
    lda.l !gsu_rec_42,x
    ora.w #$0010
    sta.l !gsu_rec_42,x
.lC8C2:
    lda.l !gsu_rec_40,x
    and.w #$0100
    beq .lC8FB
    lda.w !seq_input
    bit.w #$0400
    bne .lC8D7
    jml.l .lC8E3
.lC8D7:
    lda.l !gsu_out_26,x
    sec
    sbc.w #$0600
    sta.l !gsu_out_26,x
.lC8E3:
    lda.w !seq_input
    bit.w #$0800
    bne .lC8EF
    jml.l .lC8FB
.lC8EF:
    lda.l !gsu_out_26,x
    clc
    adc.w #$0600
    sta.l !gsu_out_26,x
.lC8FB:
    plx
    sep.b #$20
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

; @routine 08:CD08 GsuRecordSet2AHigh
; WHAT IT DOES: Sets the word at +$2A of object X's record to A's low byte
; as the high byte (and keeps that word in $1F36), when the object has a
; record.
; HOW: AND #$FF / XBA, then a long store.
; Entry: M 8-bit, X 16-bit; X = object, A low byte = value. Exit: M 8-bit;
; X unchanged; record: A = the word, N, Z from X; no record: A = 0, Z set.
%at($08CD08)
GsuRecordSet2AHigh:
    rep.b #$20
    and.w #$00FF
    xba
    sta.w !wram_1F36
    rep.b #$20
    lda.w !obj_22,x
    beq .lCD22
    phx
    tax
    lda.w !wram_1F36
    sta.l !gsu_rec_2a,x
    plx
.lCD22:
    sep.b #$20
    rtl

; @routine 08:CD25 ObjectFlagsRecordSetup20
; WHAT IT DOES: As ObjectFlagsRecordSetup, with bit 5 of object X's +$24
; set first. Nothing in the game calls it.
; HOW: An 8-bit OR into +$24, then falls into ObjectFlagsRecordSetup.
; Entry: as ObjectFlagsRecordSetup. Exit: as ObjectFlagsRecordSetup.
%at($08CD25)
ObjectFlagsRecordSetup20:
    lda.w !obj_24,x
    ora.b #$20
    sta.w !obj_24,x

; @routine 08:CD2D ObjectFlagsRecordSetup
; WHAT IT DOES: Sets up an object whose record takes the object's flags:
; +$27 = 0, handler $08:CD7E (ObjectRecordFlag4Step), +$1D = $08:CD55
; (ObjectFlagsRecord), then its record (ObjectFlagsRecord). Entered as an
; object's handler.
; HOW: Stores (16-bit then 8-bit), then falls into ObjectFlagsRecord.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as
; ObjectFlagsRecord.
%at($08CD2D)
ObjectFlagsRecordSetup:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_27,x
    sep.b #$20
    rep.b #$20
    lda.w #ObjectRecordFlag4Step
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #ObjectRecordFlag4Step>>16
    sta.w !obj_1c,x
    rep.b #$20
    lda.w #ObjectFlagsRecord
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #ObjectFlagsRecord>>16
    sta.w !obj_1f,x

; @routine 08:CD55 ObjectFlagsRecord
; WHAT IT DOES: Gives object X a record from the second pool
; (GsuRecordNew422), ORs the object's +$06 word and then its +$27 word into
; the record's flags (+$16), and marks it (ObjectRecordFlag4Step). With no
; room in the pool it returns at once. Also the +$1D pointer
; ObjectFlagsRecordSetup leaves.
; HOW: BPL on GsuRecordNew422's result (BRL to an RTL when negative); the
; object in Y and the record in X while ORing, two read-OR-write passes
; over +$16.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: no room: as
; GsuRecordNew422 (A = $00FF, N set); otherwise as GsuRecordFlag4.
%at($08CD55)
ObjectFlagsRecord:
    jsl.l GsuRecordNew422
    bpl .lCD5E
    brl ObjectRecordFlag4Step_lCD83
.lCD5E:
    rep.b #$20
    txy
    lda.w !obj_22,x
    tax
    lda.w !obj_06,y
    ora.l !gsu_rec_flags,x
    sta.l !gsu_rec_flags,x
    lda.l !gsu_rec_flags,x
    ora.w !obj_27,y
    sta.l !gsu_rec_flags,x
    tyx
    sep.b #$20

; @routine 08:CD7E ObjectRecordFlag4Step
; WHAT IT DOES: A handler that only marks object X's record
; (GsuRecordFlag4) each frame; also the tail other steps jump to.
; HOW: JSL to GsuRecordFlag4.
; Entry: as GsuRecordFlag4. Exit: as GsuRecordFlag4.
%at($08CD7E)
ObjectRecordFlag4Step:
    jsl.l GsuRecordFlag4
    rtl
.lCD83:
    rtl


; @routine 08:CD84 ObjectRecordMarkSetup
; WHAT IT DOES: Sets up an object that keeps a Super FX record and only
; marks it: handler $08:CDA4 (ObjectRecordMarkStep), ObjectStubZero, a
; record (GsuRecordNew422), then +$09 bit 0 set. No course in the scripts
; places it.
; HOW: Stores and calls in turn; a failed record is not checked.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; A low byte = the
; new +$09, N, Z from it.
%at($08CD84)
ObjectRecordMarkSetup:
    rep.b #$20
    lda.w #$CDA4
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x
    jsl.l ObjectStubZero
    jsl.l GsuRecordNew422
    lda.w !obj_09,x
    ora.b #$01
    sta.w !obj_09,x
    rtl

; @routine 08:CDA4 ObjectRecordMarkStep
; WHAT IT DOES: A handler that only rebuckets object X and marks its
; record (GsuRecordFlag4) each frame.
; HOW: ObjectRebucket, GsuRecordFlag4.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as GsuRecordFlag4.
%at($08CDA4)
ObjectRecordMarkStep:
    jsl.l ObjectRebucket
    jsl.l GsuRecordFlag4
    rtl

; @routine 08:CDAD ObjectSyncFallStep
; WHAT IT DOES: A handler: ObjectSyncFromRecordFall.
; HOW: JSL to it.
; Entry: as ObjectSyncFromRecordFall. Exit: as ObjectSyncFromRecordFall.
%at($08CDAD)
ObjectSyncFallStep:
    jsl.l ObjectSyncFromRecordFall
    rtl

; @routine 08:CDB2 ObjectSpinSetup
; WHAT IT DOES: Sets up a spinning object: +$1D = $08:CDD0
; (ObjectSpinRecord), handler $08:CDDE (ObjectSpinStep), then its record
; (ObjectSpinRecord). No course in the scripts places it.
; HOW: Stores, then falls into ObjectSpinRecord.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08CDB2)
ObjectSpinSetup:
    rep.b #$20
    lda.w #$CDD0
    sta.w !obj_1d,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1f,x
    rep.b #$20
    lda.w #$CDDE
    sta.w !obj_1a_w,x
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,x

; @routine 08:CDD0 ObjectSpinRecord
; WHAT IT DOES: Gives the spinning object its Super FX record
; (GsuRecordNew424) in mode 2 with $20 in the high byte of +$2A, then its
; first step (ObjectSpinStep). Also its +$1D pointer.
; HOW: GsuRecordNew424, GsuRecordMode2, LDA #$20, GsuRecordSet2AHigh;
; falls into ObjectSpinStep.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord.
%at($08CDD0)
ObjectSpinRecord:
    jsl.l GsuRecordNew424
    jsl.l GsuRecordMode2
    lda.b #$20
    jsl.l GsuRecordSet2AHigh

; @routine 08:CDDE ObjectSpinStep
; WHAT IT DOES: The spinning object's handler: its heading +$15 turns by
; one a frame and goes to its record's +$24 (low byte), then the object
; follows its record (ObjectSyncFromRecord).
; HOW: An 8-bit add to +$15 (CLC, ADC, which sets V); with a record, a
; 16-bit read of +$15 masked to its low byte stored at the record's +$24.
; Entry: M 8-bit, X 16-bit; X = object. Exit: as ObjectSyncFromRecord, V
; from the add.
%at($08CDDE)
ObjectSpinStep:
    lda.w !obj_15,x
    clc
    adc.b #$01
    sta.w !obj_15,x
    txy
    rep.b #$20
    lda.w !obj_22,y
    beq .lCDFB
    tax
    lda.w !obj_15,y
    and.w #$00FF
    sta.l !gsu_in_24,x
    tyx
.lCDFB:
    sep.b #$20
    jsl.l ObjectSyncFromRecord
    rtl


; @routine 08:CE79 ObjectSyncFromRecordFall
; WHAT IT DOES: As ObjectSyncFromRecord for an object whose record is
; known to be there, with its Y speed (+$30) added to the height taken
; back from the record, and +$24 bit 1 neither cleared nor set.
; HOW: Rebucket, GsuRecordFlag4; the object in Y and the record in X while
; copying; $1F3C = the record's +$18 after the heading and +$12; ends in
; ObjectSyncFromRecord's tail at $08:CE60. The record address is not
; tested.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: X = the object; M
; 8-bit; carry clear; V from the height's add; Y = $1F3C; A low byte = 1
; when it is not zero (N, Z from that), else A = the record's +$22 and Z
; set.
%at($08CE79)
ObjectSyncFromRecordFall:
    jsl.l ObjectRebucket
    jsl.l GsuRecordFlag4
    rep.b #$20
    txy
    lda.w !obj_22,y
    tax
    lda.w !obj_15,y
    and.w #$00FF
    xba
    sta.l !gsu_in_24,x
    lda.w !obj_5a,y
    and.w #$007F
    sta.l !gsu_rec_12,x
    lda.l !gsu_rec_18,x
    sta.w !wram_1F3C
    lda.l !gsu_rec_1e,x
    sta.w !obj_pos_x,y
    lda.l !gsu_in_20,x
    clc
    adc.w !obj_vel_y,y
    sta.w !obj_pos_y,y
    lda.l !gsu_in_22,x
    sta.w !obj_pos_z,y
    tyx
    sep.b #$20
    jmp.w ObjectSync_tail


; @routine 08:CBFF GsuRecordNew424Zero
; WHAT IT DOES: As GsuRecordNew424Alt, with +$0A = 0. Nothing in the game
; calls it.
; HOW: Runs Super FX program $04:8C0A; then the shared tail at $08:CC7C.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CBFF)
GsuRecordNew424Zero:
    phx
    phy
    lda.b #$04
    ldx.w #$8C0A
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$0000
    sta.w !wram_1F4C
    sep.b #$20
    bra GsuRecordNew_tail


; @routine 08:CC31 GsuRecordNew424B
; WHAT IT DOES: As GsuRecordNew424, from Super FX program $04:8C3B, with
; +$0A = $9185. Nothing in the game calls it.
; HOW: Runs Super FX program $04:8C3B; then the shared tail at $08:CC7C.
; Entry: as GsuRecordNew422. Exit: as GsuRecordNew422.
%at($08CC31)
GsuRecordNew424B:
    phx
    phy
    lda.b #$04
    ldx.w #$8C3B
    jsl.l GsuRun
    ply
    plx
    rep.b #$20
    lda.w #$9185
    sta.w !wram_1F4C
    sep.b #$20
    bra GsuRecordNew_tail
