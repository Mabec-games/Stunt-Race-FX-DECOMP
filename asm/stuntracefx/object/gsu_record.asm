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
