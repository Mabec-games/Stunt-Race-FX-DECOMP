; object_fields.asm: small object helpers of bank $03 on the fields of one or two object records.
; C: src/stuntracefx/object/object_fields.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:CBE0 ObjectStubZero
; WHAT IT DOES: Returns A = 0.
; HOW: A 16-bit load of 0, then back to M 8-bit.
; Entry: any widths. Exit: A = $0000; M 8-bit; Z set, N clear.
%at($03CBE0)
ObjectStubZero:
    rep.b #$20
    lda.w #$0000
    sep.b #$20
    rtl

; @routine 03:CBE8 ObjectStubZeroCopy
; WHAT IT DOES: The same bytes as ObjectStubZero, at another address.
; HOW: As ObjectStubZero.
; Entry: as ObjectStubZero. Exit: as ObjectStubZero.
%at($03CBE8)
ObjectStubZeroCopy:
    rep.b #$20
    lda.w #$0000
    sep.b #$20
    rtl

; @routine 03:CBF0 ObjectPlaceAtOffset
; WHAT IT DOES: Puts object Y at object X's position plus an offset: +$0C
; = X's +$0C + the word at $04, +$10 = X's +$10 + the word at $05F2, +$0E =
; X's +$0E + the word at $0A.
; HOW: Three 16-bit adds in that order, through the caller's data bank.
; Entry: M 16-bit, D = $0000; X, Y = objects. Exit: A = the last sum; N,
; Z, C, V from its add.
%at($03CBF0)
ObjectPlaceAtOffset:
    lda.w !obj_pos_x,x
    clc
    adc.b !dp_04
    sta.w !obj_pos_x,y
    lda.w !obj_pos_z,x
    clc
    adc.w !rot_z
    sta.w !obj_pos_z,y
    lda.w !obj_pos_y,x
    clc
    adc.b !actor_0a
    sta.w !obj_pos_y,y
    rtl

; @routine 03:CD8F ObjectStepByte17
; WHAT IT DOES: Steps the byte at object X's +$17 toward the byte at $1F2B
; by A: onto the target when it is less than A away, else A nearer. Carry
; set when it was already there (nothing written), clear when it moved.
; HOW: The step kept in direct page $3E. The distance is the 8-bit
; difference made positive; the direction comes from comparing the two
; bytes, by the sign of the result (BMI), not as numbers. Through the
; caller's data bank.
; Entry: M 8-bit, X 16-bit, D = $0000; A = step, X = object. Exit: there:
; A = 0, Z set, N clear, V from the subtract, C set; moved: A = the new
; byte, N, Z from it, V from the last add or subtract, C clear.
%at($03CD8F)
ObjectStepByte17:
    sta.b !dp_3e
    lda.w !obj_17,x
    sec
    sbc.w !wram_1F2B
    beq .lCDBF
    bpl .lCD9F
    eor.b #$FF
    inc a
.lCD9F:
    cmp.b !dp_3e
    bpl .lCDA8
    lda.w !wram_1F2B
    bra .lCDBA
.lCDA8:
    lda.w !obj_17,x
    cmp.w !wram_1F2B
    beq .lCDBA
    bmi .lCDB7
    sec
    sbc.b !dp_3e
    bra .lCDBA
.lCDB7:
    clc
    adc.b !dp_3e
.lCDBA:
    sta.w !obj_17,x
    clc
    rtl
.lCDBF:
    sec
    rtl

; @routine 03:D1B2 ObjectStubNothing
; WHAT IT DOES: Nothing but leave M 8-bit.
; HOW: REP #$20, SEP #$20, RTL.
; Entry: any widths. Exit: M 8-bit; all else unchanged.
%at($03D1B2)
ObjectStubNothing:
    rep.b #$20
    sep.b #$20
    rtl

; @routine 03:D1B7 ObjectStubSwapTwice
; WHAT IT DOES: Nothing but leave M 8-bit: X and Y are swapped and swapped
; back.
; HOW: PHX / TYX / PLY twice, around REP #$20, SEP #$20.
; Entry: any widths. Exit: X, Y unchanged; M 8-bit; N, Z from Y (index
; width).
%at($03D1B7)
ObjectStubSwapTwice:
    phx
    tyx
    ply
    rep.b #$20
    sep.b #$20
    phx
    tyx
    ply
    rtl

; @routine 03:D23A ObjectStopX
; WHAT IT DOES: Stops object X: its velocity (+$2E, +$30, +$32) and the
; byte at +$17 to 0.
; HOW: 16-bit STZs for the velocity, then an 8-bit STZ. Through the
; caller's data bank.
; Entry: X = object. Exit: M 8-bit; A and the flags unchanged.
%at($03D23A)
ObjectStopX:
    rep.b #$20
    stz.w !obj_vel_x,x
    stz.w !obj_vel_y,x
    stz.w !obj_vel_z,x
    sep.b #$20
    stz.w !obj_17,x
    rtl

; @routine 03:D24B ObjectStopY
; WHAT IT DOES: ObjectStopX for object Y.
; HOW: A 16-bit load of 0 stored to the velocity, then its low byte to
; +$17. Through the caller's data bank.
; Entry: Y = object. Exit: A = $0000; M 8-bit; Z set, N clear.
%at($03D24B)
ObjectStopY:
    rep.b #$20
    lda.w #$0000
    sta.w !obj_vel_x,y
    sta.w !obj_vel_y,y
    sta.w !obj_vel_z,y
    sep.b #$20
    sta.w !obj_17,y
    rtl
