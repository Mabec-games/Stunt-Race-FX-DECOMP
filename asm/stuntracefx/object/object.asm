; object.asm: the object table in work RAM bank $7E. Objects are $62-byte
; records from $7E:2000; free records form a list linked through their first
; word. Fields used here: +$0C/+$0E/+$10 position (16-bit each), +$14-$16
; three bytes copied as a group, +$1A a word, +$2E/+$30/+$32 velocity,
; +$4F the link to a related object.
; C: src/stuntracefx/object/object.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:89A8 ObjectLinkedSet1A
; WHAT IT DOES: Stores A in field +$1A of the object that X's object links to.
; HOW: Reads the link at +$4F, then stores there (bank $7E, long addressing).
; Entry: M 16-bit, X 16-bit; A = value, X = object. Exit: A unchanged; X =
; the linked object; N, Z from A.
%at($0389A8)
ObjectLinkedSet1A:
    pha
    lda.l !obj_link,x
    tax
    pla
    sta.l !obj_1a,x
    rtl

; @routine 03:D214 ObjectCopy14To
; WHAT IT DOES: Copies object X's three bytes at +$14-$16 to object Y.
; HOW: Three byte copies, in the data bank.
; Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A low byte =
; the last byte; N, Z from it.
%at($03D214)
ObjectCopy14To:
    lda.w !obj_14,x
    sta.w !obj_14,y
    lda.w !obj_15,x
    sta.w !obj_15,y
    lda.w !obj_16,x
    sta.w !obj_16,y
    rtl

; @routine 03:D227 ObjectCopy14From
; WHAT IT DOES: Copies object Y's three bytes at +$14-$16 to object X.
; HOW: As ObjectCopy14To, the other way.
; Entry: as ObjectCopy14To. Exit: as ObjectCopy14To.
%at($03D227)
ObjectCopy14From:
    lda.w !obj_14,y
    sta.w !obj_14,x
    lda.w !obj_15,y
    sta.w !obj_15,x
    lda.w !obj_16,y
    sta.w !obj_16,x
    rtl

; @routine 03:D1E6 ObjectCopyPositionTo
; WHAT IT DOES: Copies object X's position to object Y.
; HOW: Three 16-bit copies (+$0C, +$0E, +$10), in the data bank.
; Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A = the
; last word; N, Z from it (16-bit); M 8-bit.
%at($03D1E6)
ObjectCopyPositionTo:
    rep.b #$20
    lda.w !obj_pos_x,x
    sta.w !obj_pos_x,y
    lda.w !obj_pos_y,x
    sta.w !obj_pos_y,y
    lda.w !obj_pos_z,x
    sta.w !obj_pos_z,y
    sep.b #$20
    rtl

; @routine 03:D1FD ObjectCopyPositionFrom
; WHAT IT DOES: Copies object Y's position to object X.
; HOW: As ObjectCopyPositionTo, the other way.
; Entry: as ObjectCopyPositionTo. Exit: as ObjectCopyPositionTo.
%at($03D1FD)
ObjectCopyPositionFrom:
    rep.b #$20
    lda.w !obj_pos_x,y
    sta.w !obj_pos_x,x
    lda.w !obj_pos_y,y
    sta.w !obj_pos_y,x
    lda.w !obj_pos_z,y
    sta.w !obj_pos_z,x
    sep.b #$20
    rtl

; @routine 03:C06C ObjectApplyVelocity
; WHAT IT DOES: Moves object X by its velocity.
; HOW: Adds +$2E, +$30, +$32 to +$0C, +$0E, +$10 (16-bit), in the data bank.
; Entry: M 8-bit, X 16-bit; DB = the object table's bank. Exit: A = the new
; +$10; flags from the last add; M 8-bit.
%at($03C06C)
ObjectApplyVelocity:
    rep.b #$20
    lda.w !obj_pos_x,x
    clc
    adc.w !obj_vel_x,x
    sta.w !obj_pos_x,x
    lda.w !obj_pos_y,x
    clc
    adc.w !obj_vel_y,x
    sta.w !obj_pos_y,x
    lda.w !obj_pos_z,x
    clc
    adc.w !obj_vel_z,x
    sta.w !obj_pos_z,x
    sep.b #$20
    rtl

; @routine 03:D3DE ObjectPoolInit
; WHAT IT DOES: Empties the object table: links all $136 records into the
; free list and clears the counters at $0329-$032E.
; HOW: Starting at offset 1, each record's first word gets the next record's
; offset ($62 on); the last gets 0. Data bank $7E inside.
; Entry: any widths. Exit: A = 1 + $136 * $62; X = the last record; Y = 0;
; P unchanged.
%at($03D3DE)
ObjectPoolInit:
    php
    sep.b #$20
    phb
    lda.b #$7E
    pha
    plb
    rep.b #$30
    stz.w !wram_0329
    stz.w !wram_032B
    lda.w #$0001
    sta.w !wram_032D
    ldy.w #$0136
.lD3F7:
    tax
    clc
    adc.w #$0062
    sta.w !obj_base,x
    dey
    bne .lD3F7
    stz.w !obj_base,x
    sep.b #$20
    plb
    plp
    rtl

; @routine 7F:12AC ObjectLinkFlag4000Step
; WHAT IT DOES: When the object two links on from the one at $0346 has bit
; 14 set in its first word, adds 8 to $06CD.
; HOW: Follows the link at +$4F twice (bank $7E), reads the record's first
; word in bank 0.
; Entry: any widths. Exit: A = the masked word, or the new $06CD; X, P
; unchanged.
%ramcode($7F12AC, $058AAC)
ObjectLinkFlag4000Step:
    php
    rep.b #$30
    phx
    ldx.w !wram_0346
    lda.l !obj_link,x
    tax
    lda.l !obj_link,x
    tax
    lda.l !bank0_base,x
    and.w #$4000
    beq .l12D2
    lda.l !wram_06CD
    clc
    adc.w #$0008
    sta.l !wram_06CD
.l12D2:
    plx
    plp
    rtl

; @routine 08:B830 ObjectFlag40IfMatch
; WHAT IT DOES: When A equals the word at $70:2476, sets bit 6 of field
; +$25 of the object at $0346.
; HOW: 16-bit compare, then an 8-bit OR through the data bank.
; Entry: M 16-bit, X 16-bit; A = value. Exit: no match: A unchanged; M
; 16-bit; N, Z, C from the compare. Match: A low byte = the new field; X =
; the object; M 8-bit; N, Z from it; C set.
%at($08B830)
ObjectFlag40IfMatch:
    rep.b #$20
    cmp.l !gsu_match_2476
    bne .lB845
    ldx.w !wram_0346
    sep.b #$20
    lda.w !obj_25,x
    ora.b #$40
    sta.w !obj_25,x
.lB845:
    rts

; @routine 08:E896 ObjectTableE8AF
; WHAT IT DOES: Returns the byte of the table at $08:E8AF for (A + object
; X's +$15 + $1F4C) / 8.
; HOW: 8-bit adds, three right shifts.
; Entry: M 8-bit, X 16-bit; X = object, A = value. Exit: A low byte = the
; table byte; X, Y unchanged; N, Z from X; C from the last shift; V from
; the second add.
%at($08E896)
ObjectTableE8AF:
    clc
    adc.w !obj_15,x
    clc
    adc.w !wram_1F4C
    phx
    phy
    sep.b #$10
    lsr a
    lsr a
    lsr a
    tax
    lda.l !table_08e8af,x
    rep.b #$10
    ply
    plx
    rts

; @routine 08:C03C ObjectTableC056
; WHAT IT DOES: Sets $1F5A to the byte of the table at $08:C056 for (object
; Y's +$15 - $1F5A) / 4.
; HOW: An 8-bit subtract, two right shifts.
; Entry: M 8-bit, X 16-bit; Y = object. Exit: A low byte = the new $1F5A;
; X, Y unchanged; N, Z from Y; C from the last shift; V from the subtract.
%at($08C03C)
ObjectTableC056:
    lda.w !obj_15,y
    sec
    sbc.w !wram_1F5A
    phy
    phx
    sep.b #$10
    lsr a
    lsr a
    tax
    lda.l !table_08c056,x
    sta.w !wram_1F5A
    rep.b #$10
    plx
    ply
    rts

; @routine 08:B75B ObjectListAppend
; WHAT IT DOES: Appends object X to the zero-ended list of up to 32 words at
; $7E:FFBD, whose length in bytes is at $7E:FFFD; a full list is left alone.
; HOW: Stores X at the end, steps the length by 2 and stores the 0 that
; ends the list.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A = 0, or $40 when full (M
; 8-bit); X, Y unchanged; N, Z from X; C from the full-length compare.
%at($08B75B)
ObjectListAppend:
    phx
    phy
    rep.b #$20
    txy
    lda.l !object_list_len
    cmp.w #$0040
    beq .lB77D
    tax
    tya
    sta.l !object_list,x
    inx
    inx
    txa
    sta.l !object_list_len
    lda.w #$0000
    sta.l !object_list,x
.lB77D:
    sep.b #$20
    ply
    plx
    rtl

; @routine 08:8364 ObjectBucketInsert
; WHAT IT DOES: Puts object X at the head of the list for its bucket: the
; bucket number is the object's +$40, the list heads are words at $7F:0000
; + the bucket, and +$3C/+$3E link to the next and previous objects.
; HOW: The old head becomes the object's next (+$3C, its previous cleared)
; and gets the object as its previous; the object becomes the head. Direct
; page $40 holds the object while the old head is updated. Data bank $7E
; inside.
; Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: A = the object;
; X, Y unchanged; N, Z from the caller's data bank.
%at($088364)
ObjectBucketInsert:
    phb
    phy
    phx
    sep.b #$20
    lda.b #$7E
    pha
    plb
    rep.b #$20
    lda.w !obj_bucket,x
    stz.w !obj_bucket_prev,x
    txy
    tax
    lda.l !object_buckets,x
    sta.w !obj_bucket_next,y
    beq .l8389
    sty.b !dp_40
    tay
    lda.b !dp_40
    sta.w !obj_bucket_prev,y
    tay
.l8389:
    tya
    sta.l !object_buckets,x
    plx
    ply
    plb
    rtl

; @routine 08:D5AB ObjectVelocityFromRecord
; WHAT IT DOES: Sets object X's X and Z velocity (+$2E, +$32) to an eighth
; of the values at +$2E and +$32 of the Super FX record of the object named
; by its +$27.
; HOW: Each value is shifted right three times keeping its sign.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A = the Z velocity (M 8-bit);
; X, Y = the object; N, Z from X; C from the last shift.
%at($08D5AB)
ObjectVelocityFromRecord:
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
    sta.w !obj_vel_x,y
    lda.l !gsu_rec_vel_z,x
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !obj_vel_z,y
    tyx
    sep.b #$20
    rts

; @routine 08:8392 ObjectBucketRemove
; WHAT IT DOES: Takes object X out of its bucket's list (see
; ObjectBucketInsert): its previous object, or the bucket's head when it has
; none, is linked to its next, and its next gets its previous.
; HOW: Data bank $7E inside; the head is $7F:0000 + the object's +$40.
; Entry: X 16-bit; X = object. Exit: A = the previous object (or the next,
; when there is no next or no previous); X, Y unchanged; M 16-bit; N, Z
; from the caller's data bank.
%at($088392)
ObjectBucketRemove:
    phb
    phy
    phx
    sep.b #$20
    lda.b #$7E
    pha
    plb
    rep.b #$20
    txy
    lda.w !obj_bucket_prev,x
    beq .l83B3
    tax
    lda.w !obj_bucket_next,y
    sta.w !obj_bucket_next,x
    beq .l83C4
    tay
    txa
    sta.w !obj_bucket_prev,y
    bra .l83C4
.l83B3:
    lda.w !obj_bucket,x
    tax
    lda.w !obj_bucket_next,y
    sta.l !object_buckets,x
    beq .l83C4
    tax
    stz.w !obj_bucket_prev,x
.l83C4:
    plx
    ply
    plb
    rtl

; @routine 08:B48E ObjectSet5DByKind
; WHAT IT DOES: Sets object X's +$5D to $9163, $9335 or $9507 for A = 1, 2 or
; 3, and to $8F91 otherwise.
; HOW: Compares in turn; one 16-bit store through the data bank.
; Entry: M 8-bit, X 16-bit; X = object, A = kind. Exit: A = the value (M
; 8-bit); N set, Z clear; C from the last compare.
%at($08B48E)
ObjectSet5DByKind:
    cmp.b #$01
    beq .lB4A5
    cmp.b #$02
    beq .lB4B0
    cmp.b #$03
    beq .lB4BB
    rep.b #$20
    lda.w #$8F91
    sta.w !obj_5d,x
    sep.b #$20
    rts
.lB4A5:
    rep.b #$20
    lda.w #$9163
    sta.w !obj_5d,x
    sep.b #$20
    rts
.lB4B0:
    rep.b #$20
    lda.w #$9335
    sta.w !obj_5d,x
    sep.b #$20
    rts
.lB4BB:
    rep.b #$20
    lda.w #$9507
    sta.w !obj_5d,x
    sep.b #$20
    rts

; @routine 08:B8F9 ObjectPlaceFromTable
; WHAT IT DOES: Sets up the object in entry A (from 1) of the list at
; $7E:FFAE: its +$27 from $1F2D, its +$49 and +$4A from the bytes at $1F2B
; and $1F4C, and, unless bit 0 of $1AEE is set, its position (+$0C, +$0E,
; +$10) and +$15 byte from entry X (from 1) of the 8-byte table at
; $7E:FF66.
; HOW: Data bank $7E inside; word copies, then byte copies.
; Entry: M 16-bit, X 16-bit; A low byte = list entry, X low byte = table
; entry. Exit: A low byte = the byte from $1F4C; X = the table offset; Y =
; the object; P unchanged.
%at($08B8F9)
ObjectPlaceFromTable:
    php
    sep.b #$20
    rep.b #$10
    phb
    pha
    lda.b #$7E
    pha
    plb
    pla
    rep.b #$20
    phx
    and.w #$00FF
    dec a
    asl a
    tax
    lda.l !object_list_b,x
    tay
    plx
    txa
    and.w #$00FF
    dec a
    asl a
    asl a
    asl a
    tax
    lda.w !wram_1F2D
    sta.w !obj_27,y
    lda.w !wram_1AEE
    and.w #$0001
    bne .lB945
    lda.w !place_table,x
    sta.w !obj_pos_x,y
    lda.w !place_table+2,x
    sta.w !obj_pos_y,y
    lda.w !place_table+4,x
    sta.w !obj_pos_z,y
    sep.b #$20
    lda.w !place_table+6,x
    sta.w !obj_15,y
.lB945:
    sep.b #$20
    lda.w !wram_1F2B
    sta.w !obj_49,y
    lda.w !wram_1F4C
    sta.w !obj_4a,y
    plb
    plp
    rtl

; @routine 7E:D397 ObjectLinkEventTrigger
; WHAT IT DOES: Unless bit 7 of $0DA4 is already set: when the object two
; links on from the one at $0346 has bit 14 set in its first word and
; $1A24 is non-zero, clears $1A24, sets $0DA4 to $80 and starts the values
; at $1A1C-$1A22 ($1A, 0, $FE, 0).
; HOW: Links through +$4F; everything through the data bank.
; Entry: any widths. Exit: M 8-bit, X 8-bit (X, Y high bytes clear); A and
; N, Z as left by the test that ended it (see the code).
%ramcode($7ED397, $0130BE)
ObjectLinkEventTrigger:
    sep.b #$20
    lda.w !wram_0DA4
    and.b #$80
    bne .lD3DA
    rep.b #$30
    ldx.w !wram_0346
    ldy.w !obj_link_w,x
    lda.w !obj_link_w,y
    tay
    lda.w 0,y
    and.w #$4000
    beq .lD3DA
    rep.b #$20
    lda.w !wram_1A24
    beq .lD3DA
    lda.w #$0000
    sta.w !wram_1A24
    sep.b #$20
    lda.b #$80
    sta.w !wram_0DA4
    lda.b #$1A
    sta.w !wram_1A1C
    lda.b #$00
    sta.w !wram_1A1E
    sta.w !wram_1A22
    lda.b #$FE
    sta.w !wram_1A20
.lD3DA:
    sep.b #$30
    rts

; @routine 03:CB25 ObjectInit
; WHAT IT DOES: Clears object Y's record from +$04 on (47 words), puts it in
; bucket 0's list, and sets its +$36 to 0 and its +$59 to $69.
; HOW: Data bank $7E inside; ObjectBucketInsert; X and Y are swapped while
; it works and swapped back.
; Entry: M 8-bit, X 16-bit; Y = object. Exit: A = $69 with the object's
; high byte; X, Y unchanged; M 8-bit; N, Z from the caller's data bank.
%at($03CB25)
ObjectInit:
    phb
    lda.b #$7E
    pha
    plb
    phx
    tyx
    ply
    rep.b #$20
    lda.w #$0000
    phy
    phx
    ldy.w #$002F
.lCB37:
    sta.w !obj_04,x
    inx
    inx
    dey
    bne .lCB37
    plx
    ply
    lda.w #$0000
    sta.w !obj_bucket,x
    jsl.l ObjectBucketInsert
    sep.b #$20
    lda.b #$00
    sta.w !obj_36,x
    lda.b #$69
    sta.w !obj_59,x
    phx
    tyx
    ply
    plb
    rtl

; @routine 08:B8B4 ObjectPlaceAll
; WHAT IT DOES: Sets up every object in the list at $7E:FFA6 (half the
; count at $7E:FFB8 of them) with ObjectPlaceFromTable: object n (from 1)
; takes table entry $7E:FFA6[n], and its $1F2B, $1F2D and $1F4C words from
; the lists at $18D3, $18DB and $18E3.
; HOW: Direct page $02 counts down from the count halved (arithmetic shift),
; $08 counts the entry up from 1; the three words are stored through the
; data bank before each call.
; Entry: any widths, D = $0000. Exit: A = the last ObjectPlaceFromTable A;
; X, Y, P restored, then N, Z from X (pulled at its entry width).
%at($08B8B4)
ObjectPlaceAll:
    phx
    phy
    php
    rep.b #$30
    lda.l !object_list_b_len
    cmp.w #$8000
    ror a
    sta.b !dp_02
    lda.w #$0001
    sta.b !dp_08
    ldx.w #$0000
.lB8CB:
    lda.l !selection_out,x
    sta.w !wram_1F2B
    lda.l !selection_out_b,x
    sta.w !wram_1F2D
    lda.l !selection_derived,x
    sta.w !wram_1F4C
    lda.l !wram_7EFFA6,x
    phx
    tax
    lda.b !dp_08
    jsl.l ObjectPlaceFromTable
    plx
    inx
    inx
    inc.b !dp_08
    dec.b !dp_02
    bne .lB8CB
    plp
    ply
    plx
    rtl

; @routine 08:E760 ObjectCountStep5F
; WHAT IT DOES: Steps object X's counter +$49 up by one; while it has not
; passed the limit at $1F58 its +$5F rises by $1F5A, after that +$5F falls
; by $1F5A and the routine reports when the counter has reached $1F59.
; HOW: 8-bit compares (BEQ/BMI on the difference); returns with carry set
; only when the counter equals $1F59 on the falling side.
; Entry: M 8-bit, X 16-bit; X = object. Exit: C set when done; rising: A
; low byte = the new +$5F, N, Z, V from the add; falling: A low byte = the
; counter, N, Z from the compare with $1F59, V from the subtract.
%at($08E760)
ObjectCountStep5F:
    inc.w !obj_49,x
    lda.w !obj_49,x
    cmp.w !wram_1F58
    beq .lE771
    bmi .lE771
    jml.l ObjectCountStep5F_falling
.lE771:
    lda.w !obj_5f,x
    clc
    adc.w !wram_1F5A
    sta.w !obj_5f,x
    jmp.w ObjectCountStep5F_clear
ObjectCountStep5F_falling:          ; reached by the long jump above
    lda.w !obj_5f,x
    sec
    sbc.w !wram_1F5A
    sta.w !obj_5f,x
    lda.w !obj_49,x
    cmp.w !wram_1F59
    bne ObjectCountStep5F_clear
    jml.l ObjectCountStep5F_done
ObjectCountStep5F_clear:            ; both branches return carry clear here
    clc
    rts
ObjectCountStep5F_done:             ; reached by the long jump above
    sec
    rts

; @routine 08:B5FA ObjectNearFocus
; WHAT IT DOES: Switches object X between near and far by its distance
; from the object at $1A64 (|dX| + |dZ| against $1194). Near: clears bit 3
; of +$24, zeroes +$5F and sets +$5D by its kind (+$49) with
; ObjectSet5DByKind. Far: sets bit 3 of +$24, zeroes +$3A, sets +$5F to
; $32, +$5D to $99A3 + 2 * kind, and puts a direction ((the $1A64 object's
; +$11 + its own +$15) / 8, low two bits clear, bit 7 set) above the low
; two bits of +$18.
; HOW: 16-bit differences made positive by EOR/INC, summed, compared; the
; rest in 8-bit through the data bank. $1F2B keeps |dX|.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: Y = $1A64; M
; 8-bit; near: as ObjectSet5DByKind (V from the distance add); far: A = the
; +$5D value, N, Z, C, V from its add.
%at($08B5FA)
ObjectNearFocus:
    rep.b #$20
    ldy.w #!menu_1a64
    lda.w 8,y
    sec
    sbc.w !obj_pos_x,x
    bpl .lB60C
    eor.w #$FFFF
    inc a
.lB60C:
    sta.w !wram_1F2B
    lda.w 12,y
    sec
    sbc.w !obj_pos_z,x
    bpl .lB61C
    eor.w #$FFFF
    inc a
.lB61C:
    clc
    adc.w !wram_1F2B
    cmp.w #$1194
    sep.b #$20
    bpl .lB63C
    lda.w !obj_24,x
    and.b #$F7
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_5f,x
    lda.w !obj_49,x
    jsr.w ObjectSet5DByKind
    bra .lB67F
.lB63C:
    lda.w !obj_18,x
    and.b #$03
    sta.w !obj_18,x
    ldy.w #!menu_1a64
    lda.w 17,y
    clc
    adc.w !obj_15,x
    lsr a
    lsr a
    lsr a
    and.b #$FC
    ora.b #$80
    ora.w !obj_18,x
    sta.w !obj_18,x
    lda.w !obj_24,x
    ora.b #$08
    sta.w !obj_24,x
    lda.b #$00
    sta.w !obj_3a,x
    lda.b #$32
    sta.w !obj_5f,x
    lda.w !obj_49,x
    rep.b #$20
    and.w #$00FF
    asl a
    clc
    adc.w #$99A3
    sta.w !obj_5d,x
    sep.b #$20
.lB67F:
    rtl

; @routine 08:83CC ObjectRebucket
; WHAT IT DOES: Recomputes object X's bucket from its X and Z position
; (ScaleObjectXZ) and, when it changed, moves the object from its old
; bucket's list to the new one's.
; HOW: Compared with +$40 (bank $7E); ObjectBucketRemove, the new +$40,
; ObjectBucketInsert. P is saved and restored around it.
; Entry: any M, X 16-bit, D = $0000; X = object. Exit: A = the bucket when
; unchanged, else the object; X, Y, P unchanged; $02 and $94 as
; ScaleDp02Dp94 leaves them.
%at($0883CC)
ObjectRebucket:
    php
    rep.b #$20
    jsr.w ScaleObjectXZ
    cmp.l !obj_bucket_long,x
    beq .l83E6
    pha
    jsl.l ObjectBucketRemove
    pla
    sta.l !obj_bucket_long,x
    jsl.l ObjectBucketInsert
.l83E6:
    plp
    rtl

; @routine 08:B5B3 ObjectRecordFlag70Step
; WHAT IT DOES: While $0D53 is clear and object X's countdown +$48 is
; running, counts it down; otherwise sets bit 2 of the word at +$70 of the
; object's Super FX record when $0D53 is set, clears it when not.
; HOW: 8-bit tests through the data bank, 16-bit read-modify-write of the
; record word. An object with no record branches to $08:B5F6, a JSL to the
; stop screen CrashWhite ($00:8014) that would fall into ObjectNearFocus;
; no script reaches that path.
; Entry: M 8-bit, X 16-bit; X = object. Exit: M 8-bit; X, Y unchanged.
; Counting: A low byte = the count before the step, N, Z from the new
; count. Otherwise: A = the new record word, N, Z from X.
%at($08B5B3)
ObjectRecordFlag70Step:
    sep.b #$20
    lda.w !wram_0D53
    bne ObjectRecordFlag70Step_update
    lda.w !obj_48,x
    bne .lB5C3
    jml.l ObjectRecordFlag70Step_update
.lB5C3:
    dec.w !obj_48,x
    jmp.w ObjectRecordFlag70Step_done
ObjectRecordFlag70Step_update:      ; reached by the long jump above
    rep.b #$20
    phx
    lda.w !obj_22,x
    beq ObjectRecordFlag70Step_stop
    tax
    lda.w !wram_0D53
    and.w #$00FF
    beq .lB5E7
    lda.l !gsu_rec_70,x
    ora.w #$0004
    sta.l !gsu_rec_70,x
    bra .lB5F2
.lB5E7:
    lda.l !gsu_rec_70,x
    and.w #$FFFB
    sta.l !gsu_rec_70,x
.lB5F2:
    plx
    sep.b #$20
ObjectRecordFlag70Step_done:
    rtl
ObjectRecordFlag70Step_stop:        ; no record: never reached by a script
    jsl.l CrashWhite                ; falls into ObjectNearFocus

; @routine 08:B4C6 ObjectLinkedHandlerByState
; WHAT IT DOES: Once per object (bit 3 of +$25 marks it done), gives
; object X's linked object (+$4F) a new handler in bank 08 by the state
; byte at $0DD6 + Y: 8 sets bits 3 of the record's +$64 and 5 of its +$70
; and picks $91EB, $93D7 or $92DF by the record's +$88 (1, 4, other);
; $0A or $0C clears bits 4, 5 and 10 of the record's +$42 and, when $0E59
; is set, picks $94A1; other states pick $951B when $19ED is 8. Each pick
; also runs GsuRecordWords88 unless bit 0 of $1AEE is set, and marks the
; object done.
; HOW: 8-bit compares; the handler is the linked object's +$1A (address)
; and +$1C (bank). Only the $0A/$0C path is reached by a script.
; Entry: M 8-bit, X 16-bit; X = object, Y = the state index. Exit: M
; 8-bit; X unchanged; Y = the linked object when a handler was picked; A,
; N, Z, C as the path leaves them; V unchanged.
%at($08B4C6)
ObjectLinkedHandlerByState:
    sep.b #$20
    lda.w !obj_25,x
    and.b #$08
    beq .lB4D3
    jml.l ObjectLinkedHandlerByState_done
.lB4D3:
    lda.w !wram_0DD6,y
    cmp.b #$08
    beq .lB532
    cmp.b #$0C
    beq .lB501
    cmp.b #$0A
    beq .lB501
    lda.w !wram_19ED
    cmp.b #$08
    beq .lB4EC
    jmp.w ObjectLinkedHandlerByState_done
.lB4EC:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$951B
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    jmp.w .lB59A
.lB501:
    phx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_42,x
    and.w #$FBCF
    sta.l !gsu_rec_42,x
    sep.b #$20
    plx
    lda.w !wram_0E59
    bne .lB51E
    brl ObjectLinkedHandlerByState_done
.lB51E:
    ldy.w !obj_link_w,x
    rep.b #$20
    lda.w #$94A1
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    bra .lB59A
.lB532:
    phx
    rep.b #$20
    lda.w !obj_22,x
    tax
    lda.l !gsu_rec_64,x
    ora.w #$0008
    sta.l !gsu_rec_64,x
    lda.l !gsu_rec_70,x
    ora.w #$0020
    sta.l !gsu_rec_70,x
    lda.l !gsu_rec_88,x
    sep.b #$20
    sta.w !wram_1F58
    plx
    ldy.w !obj_link_w,x
    lda.w !wram_1F58
    cmp.b #$01
    beq .lB589
    cmp.b #$04
    beq .lB578
    rep.b #$20
    lda.w #$92DF
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    bra .lB59A
.lB578:
    rep.b #$20
    lda.w #$93D7
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    bra .lB59A
.lB589:
    rep.b #$20
    lda.w #$91EB
    sta.w !obj_1a_w,y
    sep.b #$20
    lda.b #$08
    sta.w !obj_1c,y
    bra .lB59A
.lB59A:
    rep.b #$20
    lda.w !wram_1AEE
    and.w #$0001
    sep.b #$20
    bne .lB5AA
    jsl.l GsuRecordWords88
.lB5AA:
    lda.w !obj_25,x
    ora.b #$08
    sta.w !obj_25,x
ObjectLinkedHandlerByState_done:    ; reached by the long jump above
    rtl

; @routine 08:8525 ObjectAttachLinked
; WHAT IT DOES: Links object X to object Y: X's +$4F = Y, Y's +$06 = X, and
; the block Y's +$4F points at gets X at +$02, kind 5 at +$00, its +$32
; from the table at $09:8090 and its other fields (+$26-+$30, +$34-+$3A)
; cleared. The table entry is the word at $70:2396, plus 2 when Y's +$1A
; is $8E2A, else plus 4, 8 or 6 for $0D2B = 1, 2 or 3.
; HOW: The block is addressed through Y as a full address in the data
; bank. Only the $0D2B choices are reached by a script.
; Entry: M 8-bit, X 16-bit; X = object, Y = the other object. Exit: A = 0
; (M 8-bit), Z set; X unchanged; Y = the block; $1F4C = Y's +$1A, $1F4E =
; the block; C (and V after an add) from the last compare or add.
%at($088525)
ObjectAttachLinked:
    rep.b #$20
    tya
    sta.w !obj_link_w,x
    lda.w !obj_link_w,y
    sta.w !wram_1F4E
    lda.w !obj_1a_w,y
    sta.w !wram_1F4C
    txa
    sta.w !obj_06,y
    ldy.w !wram_1F4E
    txa
    sta.w 2,y
    phx
    lda.l !attach_entry
    tax
    lda.w !wram_1F4C
    cmp.w #$8E2A
    beq .l857F
    lda.w !hud_mode
    and.w #$00FF
    cmp.w #$0001
    beq .l8577
    cmp.w #$0002
    beq .l8567
    cmp.w #$0003
    beq .l856F
    bra .l8585
.l8567:
    txa
    clc
    adc.w #$0008
    tax
    bra .l8585
.l856F:
    txa
    clc
    adc.w #$0006
    tax
    bra .l8585
.l8577:
    txa
    clc
    adc.w #$0004
    tax
    bra .l8585
.l857F:
    txa
    clc
    adc.w #$0002
    tax
.l8585:
    lda.l !attach_values,x
    sta.w 50,y
    plx
    lda.w #$0000
    sta.w 53,y
    sta.w 55,y
    sta.w 57,y
    sta.w 38,y
    sta.w 40,y
    sta.w 42,y
    sta.w 44,y
    sta.w 46,y
    sta.w 48,y
    lda.w #$0005
    sta.w 0,y
    sep.b #$20
    lda.b #$00
    sta.w 52,y
    rtl

; @routine 7E:F0E1 ObjectGroupsActivate
; WHAT IT DOES: For each entry in the list at $1AF5 (4 bytes each, ended by
; a zero first word), takes the object bucket its second word selects and
; sets the state byte (+$36) of every object in it to 3, skipping objects
; whose state is negative, and, when the entry's code is $23, objects with
; bit 4 of +$24 set.
; HOW: The bucket number (second word AND $07FE) indexes the bucket heads
; at $7F:0000; objects chain through +$3C (next in bucket). $1F2B keeps the list position and
; $1F2D the entry's code; X is reloaded from and stored back to direct page
; $3E around each chain. Data bank $7E inside.
; Entry: X 16-bit, D = $0000. Exit: A = 0; X = the list position of the
; terminating entry; Y = 0 when any chain was walked, else unchanged (low
; bytes only when P restores 8-bit index registers); P and the data bank
; unchanged.
%ramcode($7EF0E1, $014E08)
ObjectGroupsActivate:
    php
    phb
    sep.b #$20
    lda.b #$7E
    pha
    plb
    rep.b #$30
    stz.w !wram_1F2B
.lF0EE:
    ldx.w !wram_1F2B
    lda.w !group_trigger_list,x
    bne .lF0F9
    brl .lF13E
.lF0F9:
    sta.w !wram_1F2D
    lda.w !group_trigger_list+2,x
    inx
    inx
    inx
    inx
    stx.w !wram_1F2B
    and.w #$07FE
    tax
    lda.l !object_buckets,x
    beq .lF0EE
    tay
    ldx.b !scale_scratch
.lF113:
    sep.b #$20
    lda.w !wram_1F2D
    cmp.b #$23
    bne .lF127
    lda.w !obj_24,y
    and.b #$10
    beq .lF127
    jml.l .lF131
.lF127:
    lda.w !obj_36,y
    bmi .lF131
    lda.b #$03
    sta.w !obj_36,y
.lF131:
    rep.b #$20
    lda.w !obj_bucket_next,y
    tay
    bne .lF113
    stx.b !scale_scratch
    jmp.w .lF0EE
.lF13E:
    plb
    plp
    rts

; @routine 7E:C58D BucketTriggersFromArea
; WHAT IT DOES: Builds the trigger list at $1AF5 (read by
; ObjectGroupsActivate) from a rectangle of map cells: for each cell inside
; the window and not $2E, an entry of the cell's code and the bucket
; number $1F01 + the cell's column * 2 + its row * $40; the list ends with
; a zero entry.
; HOW: The record at A (data bank) holds the columns, rows, first column
; and first row as bytes, then the cells row by row. Direct page $A5 is
; -first column * 2 and $94 the first row * $40 (signed, counting down a
; row at a time); $02 steps the column by 2, $04 and $05F2 count the
; columns and rows (16-bit; $1F2B keeps the column count). A cell is in
; the window when $02 + $1F03 is below $40 and $94 + $1F05 below $800.
; Entry: M 16-bit, X 16-bit, D = $0000; A = record address. Exit: A = the
; final $94; X = the list size in bytes; Y = A + the cell count; N clear, Z
; set; C, V from the last row step.
%ramcode($7EC58D, $0122B4)
BucketTriggersFromArea:
    tay
    ldx.w #$0000
    lda.w 3,y
    and.w #$00FF
    xba
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.b !area_row
    lda.w 1,y
    and.w #$00FF
    sta.w !area_rows_left
    lda.w 2,y
    and.w #$00FF
    eor.w #$FFFF
    inc a
    asl a
    sta.b !area_col0
    lda.w 0,y
    and.w #$00FF
    sta.w !wram_1F2B
.lC5C1:
    lda.b !area_col0
    sta.b !dp_02
    lda.w !wram_1F2B
    sta.b !dp_04
.lC5CA:
    lda.b !dp_02
    clc
    adc.w !area_window_x
    and.w #$FFC0
    bne .lC600
    lda.b !area_row
    clc
    adc.w !area_window_y
    and.w #$F800
    bne .lC600
    lda.w 4,y
    and.w #$00FF
    cmp.w #$002E
    beq .lC600
    sta.w !group_trigger_list,x
    lda.w !bucket_origin
    clc
    adc.b !dp_02
    clc
    adc.b !area_row
    sta.w !group_trigger_list+2,x
    txa
    clc
    adc.w #$0004
    tax
.lC600:
    iny
    inc.b !dp_02
    inc.b !dp_02
    dec.b !dp_04
    bne .lC5CA
    lda.b !area_row
    sec
    sbc.w #$0040
    sta.b !area_row
    dec.w !area_rows_left
    bne .lC5C1
    stz.w !group_trigger_list,x
    stz.w !group_trigger_list+2,x
    rts

; @routine 7E:C520 BucketTriggersFromView
; WHAT IT DOES: Builds the trigger list (BucketTriggersFromArea) from the
; area record that the high byte of the view's Y position ($0667) selects,
; with the window and bucket origin from $1EF5-$1EFA.
; HOW: Copies $1EF5, $1EF7, $1EF9 to $1F01, $1F03, $1F05; the record
; address is the word at $7E:C61D + ((($0667 >> 3) + 1) AND $FE). Data
; bank $7E inside.
; Entry: D = $0000. Exit: as BucketTriggersFromArea, with M 8-bit and N, Z
; from the data bank pulled back.
%ramcode($7EC520, $012247)
BucketTriggersFromView:
    sep.b #$20
    phb
    lda.b #$7E
    pha
    plb
    rep.b #$30
    lda.w !view_area_origin
    sta.w !bucket_origin
    lda.w !view_area_origin+2
    sta.w !area_window_x
    lda.w !view_area_origin+4
    sta.w !area_window_y
    lda.w !view_pos_y_hi
    and.w #$00FF
    lsr a
    lsr a
    lsr a
    inc a
    and.w #$00FE
    tax
    lda.w !area_records,x
    jsr.w BucketTriggersFromArea
    sep.b #$20
    plb
    rts

; @routine 7E:C553 BucketTriggersFixedArea
; WHAT IT DOES: Builds the trigger list (BucketTriggersFromArea) from the
; area record at $7E:C661 when $0D62 is set or bit 7 of $1AEC is, else from
; the one at $7E:C696, with the window and bucket origin from $1EFB-$1F00.
; HOW: Copies $1EFB, $1EFD, $1EFF to $1F01, $1F03, $1F05. Data bank $7E
; inside.
; Entry: X 16-bit, D = $0000. Exit: as BucketTriggersFromArea, with M 8-bit
; and N, Z from the data bank pulled back.
%ramcode($7EC553, $01227A)
BucketTriggersFixedArea:
    sep.b #$20
    phb
    lda.b #$7E
    pha
    plb
    rep.b #$20
    lda.w !fixed_area_origin
    sta.w !bucket_origin
    lda.w !fixed_area_origin+2
    sta.w !area_window_x
    lda.w !fixed_area_origin+4
    sta.w !area_window_y
    lda.w !wram_0D62
    and.w #$00FF
    bne .lC57E
    lda.w !wram_1AEC
    and.w #$0080
    beq .lC583
.lC57E:
    lda.w #$C661
    bra .lC586
.lC583:
    lda.w #$C696
.lC586:
    jsr.w BucketTriggersFromArea
    sep.b #$20
    plb
    rts

; @routine 03:CB5C ObjectAlloc
; WHAT IT DOES: Takes an object record from the free list ($032D) and links
; it into the used list (next at +$00, previous at +$02) after the last one
; taken ($032B), or at the head ($0329) when there is none; it becomes the
; last one taken. Carry set when it got one, clear when the free list was
; empty.
; HOW: Data bank $7E inside, 16-bit; P saved and restored, then the carry
; set or cleared. With an empty free list it ends at $03:CBAA. The empty
; case and a following record (a non-zero next) are not reached by a
; script.
; Entry: any widths. Exit: P as on entry but C; got one: X = the record,
; Y = its next, A = the previous last (or the old head when there was none,
; or X when there is a next); none: X = Y = the last taken, A's low byte
; = $7E.
%at($03CB5C)
ObjectAlloc:
    php
    sep.b #$20
    phb
    lda.b #$7E
    pha
    plb
    rep.b #$30
    ldx.w !wram_032B
    txy
    ldx.w !wram_032D
    bne .lCB73
    tyx
    jmp.w ObjectAlloc_none
.lCB73:
    lda.w !obj_base,x
    sta.w !wram_032D
    tya
    bne .lCB8A
    lda.w !wram_0329
    sta.w !obj_base,x
    stz.w !obj_02,x
    stx.w !wram_0329
    bra .lCB98
.lCB8A:
    lda.w !obj_base,y
    sta.w !obj_base,x
    txa
    sta.w !obj_base,y
    tya
    sta.w !obj_02,x
.lCB98:
    stx.w !wram_032B
    ldy.w !obj_base,x
    beq .lCBA4
    txa
    sta.w !obj_02,y
.lCBA4:
    sep.b #$20
    plb
    plp
    sec
    rtl
ObjectAlloc_none:                   ; the free list was empty
    sep.b #$20
    plb
    plp
    clc
    rtl

; @routine 03:CCBB ObjectSpawn
; WHAT IT DOES: Unless the count at $0CEE has reached $2E, takes a new
; object (ObjectAlloc), clears it (ObjectInit), and sets its +$36 to 3 and
; its +$04 to $1F2B. Carry set and Y = the object when it got one; carry
; clear and Y = 0 otherwise.
; HOW: X is kept in direct page $3E across the calls; the limit is a signed
; 8-bit compare (BPL).
; Entry: M 8-bit, X 16-bit, D = $0000. Exit: X unchanged; M 8-bit; got one:
; A = $1F2B, N, Z from it; none: Y = 0, N, Z from X.
%at($03CCBB)
ObjectSpawn:
    stx.b !dp_3e
    lda.w !object_count
    cmp.b #$2E
    bpl .lCCCA
    jsl.l ObjectAlloc
    bcs .lCCD1
.lCCCA:
    ldy.w #$0000
    ldx.b !dp_3e
    clc
    rtl
.lCCD1:
    txy
    ldx.b !dp_3e
    sep.b #$20
    jsl.l ObjectInit
    lda.b #$03
    sta.w !obj_36,y
    rep.b #$20
    lda.w !wram_1F2B
    sta.w !obj_04,y
    sep.b #$20
    sec
    rtl

; @routine 03:B8A1 ObjectFreeLong
; WHAT IT DOES: ObjectFree for callers in other banks.
; HOW: JSR to it, then RTL.
; Entry: as ObjectFree. Exit: as ObjectFree.
%at($03B8A1)
ObjectFreeLong:
    jsr.w ObjectFree
    rtl

; @routine 03:B8A5 ObjectFree
; WHAT IT DOES: Gives object X back: takes it out of its bucket
; (ObjectBucketRemove) and out of the used list (fixing the head $0329 or
; its previous record's next, and its next record's previous or the last
; taken $032B), then puts it at the head of the free list ($032D).
; HOW: 16-bit, through the caller's data bank; X, Y and P are saved and
; restored. Freeing the head of the used list is not reached by a script
; (it also clears the last taken, and writes +$02 of the new head even when
; there is none).
; Entry: M 8-bit, X 16-bit; X = object. Exit: A = the old free head (M
; 8-bit); X, Y, P unchanged except N, Z, which the final pull of X sets
; from X.
%at($03B8A5)
ObjectFree:
    phx
    phy
    php
    jsl.l ObjectBucketRemove
    rep.b #$20
    ldy.w !obj_02,x
    bne .lB8C4
    ldy.w !obj_base,x
    sty.w !wram_0329
    lda.w #$0000
    sta.w !obj_02,y
    stz.w !wram_032B
    bra .lB8D8
.lB8C4:
    phx
    lda.w !obj_base,x
    tax
    sta.w !obj_base,y
    beq .lB8D4
    tya
    sta.w !obj_02,x
    bra .lB8D7
.lB8D4:
    sty.w !wram_032B
.lB8D7:
    plx
.lB8D8:
    lda.w !wram_032D
    sta.w !obj_base,x
    stx.w !wram_032D
    sep.b #$20
    plp
    ply
    plx
    rts

; @routine 03:CCA1 ObjectRelease
; WHAT IT DOES: Frees object X (ObjectFreeLong), unless it is the object
; at $034C, which instead counts one up in $035C.
; HOW: A 16-bit compare of X; the count is an 8-bit increment.
; Entry: M 8-bit, X 16-bit; X = object. Exit: X, Y unchanged; C from the
; compare; the object at $034C: N, Z from the new count; otherwise as
; ObjectFree.
%at($03CCA1)
ObjectRelease:
    cpx.w !wram_034C
    bne .lCCAB
    inc.w !wram_035C
    bra .lCCAF
.lCCAB:
    jsl.l ObjectFreeLong
.lCCAF:
    rtl

; @routine 08:AB46 ObjectCallHandler
; WHAT IT DOES: Runs object Y's handler, the routine at bank +$1C, address
; +$1A, with X = Y = the object, keeping $1F58 and X, Y across it.
; HOW: Pushes a return address ($08:AB67, so the handler's RTL lands on
; $08:AB68), then the handler's bank and address - 1, and RTLs into it.
; Entry: M 8-bit, X 16-bit; Y = object. The handler is entered with A =
; its address - 1 (N, Z from that), M 8-bit, X = Y = the object, and must
; return with M 8-bit. Exit: A low byte = $1F58, A high byte as the handler
; left it; X, Y unchanged; N, Z from X; C, V as the handler left them.
%at($08AB46)
ObjectCallHandler:
    phx
    phy
    lda.w !wram_1F58
    pha
    tyx
    phy
    phx
    lda.b #ObjectCallHandler_return>>16
    pha
    rep.b #$20
    lda.w #ObjectCallHandler_return-1
    pha
    sep.b #$20
    lda.w !obj_1c,x
    pha
    rep.b #$20
    lda.w !obj_1a_w,x
    dec a
    pha
    sep.b #$20
    rtl
ObjectCallHandler_return:           ; the handler's RTL lands here
    plx
    ply
    pla
    sta.w !wram_1F58
    ply
    plx
    rts

; @routine 03:CAB8 ObjectRunHandlerA
; WHAT IT DOES: Runs object X's first handler: the long address at +$1A
; (word) / +$1C (bank), unless the word is 0.
; HOW: Keeps the bank at $0356, the word at $0354 and the object at $034C,
; pushes $03:CAE7 as the handler's return and enters the handler with an RTL
; (A = its address - 1, M 8-bit).
; Entry: M 8-bit, X 16-bit; X = object. Exit: A and Y as the handler leaves
; them (A = 0 with no handler); X, P unchanged.
%at($03CAB8)
ObjectRunHandlerA:
    php
    phx
    lda.w !obj_1c,x
    sta.w !handler_bank
    rep.b #$20
    lda.w !obj_1a_w,x
    beq ObjectRunHandlerA_return
    sta.w !handler_addr
    stx.w !handler_object
    sep.b #$20
    lda.b #ObjectRunHandlerA_return>>16
    pha
    rep.b #$20
    lda.w #ObjectRunHandlerA_return-1
    pha
    sep.b #$20
    lda.w !handler_bank
    pha
    rep.b #$20
    lda.w !handler_addr
    dec a
    pha
    sep.b #$20
    rtl
ObjectRunHandlerA_return:           ; the handler's RTL lands here
    plx
    plp
    rtl

; @routine 03:CAEB ObjectRunHandlerB
; WHAT IT DOES: Runs object X's second handler: the long address at +$1D
; (word) / +$1F (bank), unless the word is 0 or the object has a Super FX
; record (+$22).
; HOW: As ObjectRunHandlerA, returning through $03:CB1F.
; Entry: M 8-bit, X 16-bit; X = object. Exit: A and Y as the handler leaves
; them (A = 0 with no handler, the record address when there is one); M
; 8-bit; X, P unchanged.
%at($03CAEB)
ObjectRunHandlerB:
    php
    phx
    lda.w !obj_1f,x
    sta.w !handler_bank
    rep.b #$20
    lda.w !obj_1d,x
    beq .lCB20
    sta.w !handler_addr
    lda.w !obj_22,x
    bne .lCB20
    stx.w !handler_object
    sep.b #$20
    lda.b #$03
    pha
    rep.b #$20
    lda.w #$CB1F
    pha
    sep.b #$20
    lda.w !handler_bank
    pha
    rep.b #$20
    lda.w !handler_addr
    dec a
    pha
    sep.b #$20
    rtl
.lCB20:
    sep.b #$20
    plx
    plp
    rtl

; @routine 03:CCB0 ObjectReleaseY
; WHAT IT DOES: ObjectRelease for the object in Y, called from other banks.
; HOW: Swaps X and Y around a long call to ObjectRelease (through the
; stack), so its exit X and Y come back swapped too.
; Entry: X 16-bit; Y = object. Exit: X = ObjectRelease's exit Y, Y = its
; exit X; A as it leaves it; N, Z from Y.
%at($03CCB0)
ObjectReleaseY:
    phx
    tyx
    ply
    jsl.l ObjectRelease
    phx
    tyx
    ply
    rtl

; @routine 03:D388 ObjectsRunHandlers
; WHAT IT DOES: The objects' frame: counts the frame in $05E9 (16-bit), then
; for each object in the used list (from $0329) steps its countdown at +$36:
; a negative one restarts at 3 and runs the second handler
; (ObjectRunHandlerB); one above 1 counts down and runs the first handler
; (ObjectRunHandlerA); one that reaches 0 from 1 clears +$22, +$23 and
; itself. An object a handler asked to free ($035C nonzero) is freed
; (ObjectFreeLong) after its next link is read.
; HOW: Data bank $7E for the walk; $035C is cleared before each object. The
; handlers are entered with X = the object, Y = the object (the caller's Y
; for the first) and the countdown's N, Z, C and V as the caller left them.
; Entry: X 16-bit; any M. Exit: A low byte = the last byte read; X = Y = 0;
; M 8-bit, X 16-bit; N, Z from the restored data bank; C, V as the handlers
; leave them.
%at($03D388)
ObjectsRunHandlers:
    sep.b #$20
    inc.w !wram_05E9
    bne .counted
    inc.w !wram_05E9+1
.counted:
    rep.b #$10
    phb
    lda.b #$7E
    pha
    plb
    ldx.w !wram_0329
.object:
    stz.w !wram_035C
    lda.w !obj_36,x
    beq .next
    bmi .restart
    dec a
    bne .count
    stz.w !obj_22,x
    stz.w !obj_22+1,x
    stz.w !obj_36,x
    bra .next
.restart:
    lda.b #$03
    sta.w !obj_36,x
    jsl.l ObjectRunHandlerB
    bra .ran
.count:
    sta.w !obj_36,x
    jsl.l ObjectRunHandlerA
.ran:
    lda.w !wram_035C
    bne .free
.next:
    ldy.w !obj_base,x
.link:
    tyx
    bne .object
    bra .done
.free:
    ldy.w !obj_base,x
    jsl.l ObjectFreeLong
    bra .link
.done:
    plb
    rtl
