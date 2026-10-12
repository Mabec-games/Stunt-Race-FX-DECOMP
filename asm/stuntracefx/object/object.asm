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
