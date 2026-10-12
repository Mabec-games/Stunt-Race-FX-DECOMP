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
