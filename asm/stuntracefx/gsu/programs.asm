; programs.asm: S-CPU wrappers that set a Super FX program's inputs in Super
; FX RAM, run it with GsuRun, and collect what it leaves there.
;
; Program $01:CF42 draws into a buffer whose address it takes at $70:002C
; (with inputs at $70:0068, $70:006A and a zero at $70:00A2); $01:80BC and
; $01:82CE take a vector and return results; $01:8325 fills nine words at
; $70:00E4; $07:EE88 reads an eight-word ramp at $70:3C00.
; C: src/stuntracefx/gsu/programs.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:EBEF GsuCF42ToC000
; WHAT IT DOES: Runs Super FX program $01:CF42 drawing to $C000, with inputs
; A and X.
; HOW: $70:0068 = A, $70:002C = $C000, $70:00A2 = 0, $70:006A = X, then
; GsuRun.
; Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = X's high byte and the
; screen mode value; X = $CF42; M 8-bit; N, Z from the data bank.
%at($03EBEF)
GsuCF42ToC000:
    sta.l !gsu_in_68
    lda.w #$C000
    bra GsuCF42_dest

; @routine 03:EBF8 GsuCF42To4000
; WHAT IT DOES: As GsuCF42ToC000, drawing to $4000.
; HOW: As GsuCF42ToC000.
; Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
%at($03EBF8)
GsuCF42To4000:
    sta.l !gsu_in_68
    lda.w #$4000
    bra GsuCF42_dest

; @routine 03:EC01 GsuCF42To3000
; WHAT IT DOES: As GsuCF42ToC000, drawing to $3000.
; HOW: As GsuCF42ToC000.
; Entry: as GsuCF42ToC000. Exit: as GsuCF42ToC000.
%at($03EC01)
GsuCF42To3000:
    sta.l !gsu_in_68
    lda.w #$3000
GsuCF42_dest:                       ; GsuCF42ToC000 and GsuCF42To4000 join here
    sta.l !gsu_dest
    lda.w #$0000
    sta.l !gsu_in_a2
    txa
    sta.l !gsu_in_6a
    sep.b #$20
    lda.b #$01
    ldx.w #$CF42
    jsl.l GsuRun
    rtl

; @routine 03:EB83 GsuCF42ToVram
; WHAT IT DOES: Runs program $01:CF42 drawing to $70:3000 and copies the
; result to VRAM by DMA.
; HOW: The inputs come from the record at X in the data bank (word at +0 to
; $70:0068, byte at +2 to $70:006A); after GsuRun, the transfer comes from
; the record at Y: VMAIN, the VRAM address, DMA mode and B-bus register, and
; the size, with the source fixed at $70:3000.
; Entry: M 16-bit, X 16-bit; X = input record, Y = transfer record. Exit: A =
; 1; X = Y; M 8-bit, X 16-bit; N, Z clear.
%at($03EB83)
GsuCF42ToVram:
    phy
    lda.w 0,x
    sta.l !gsu_in_68
    lda.w #$3000
    sta.l !gsu_dest
    lda.w #$0000
    sta.l !gsu_in_a2
    sep.b #$20
    lda.w 2,x
    sta.l !gsu_in_6a
    lda.b #$01
    ldx.w #$CF42
    jsl.l GsuRun
    sep.b #$20
    rep.b #$10
    plx
    lda.w 0,x
    sta.w !VMAIN
    lda.w 1,x
    sta.w !VMADDL
    lda.w 2,x
    sta.w !VMADDH
    lda.w 3,x
    sta.w !DMAP0
    lda.w 4,x
    sta.w !BBAD0
    lda.b #$00
    sta.w !A1T0L
    lda.b #$30
    sta.w !A1T0H
    lda.b #$70
    sta.w !A1B0
    lda.w 5,x
    sta.w !DAS0L
    lda.w 6,x
    sta.w !DAS0H
    lda.b #$01
    sta.w !MDMAEN
    rtl

; @routine 03:C9EE ObjectGsu80BC
; WHAT IT DOES: Runs Super FX program $01:80BC on object X's offset from a
; point and returns its result.
; HOW: $70:0068 = direct page $04 - the object's +$0C; $70:002E = $05F2 -
; its +$10 (both through the data bank); GsuRun; the result is $70:006A.
; Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: A = the result;
; X unchanged; M 16-bit, X 16-bit; N, Z from A; C, V from the second
; subtract.
%at($03C9EE)
ObjectGsu80BC:
    lda.b !dp_04
    sec
    sbc.w !obj_pos_x,x
    sta.l !gsu_in_68
    lda.w !rot_z
    sec
    sbc.w !obj_pos_z,x
    sta.l !gsu_in_2e
    phx
    sep.b #$20
    lda.b #$01
    ldx.w #$80BC
    jsl.l GsuRun
    rep.b #$20
    rep.b #$10
    plx
    lda.l !gsu_in_6a
    rtl

; @routine 03:AFD7 Gsu82CE
; WHAT IT DOES: Runs Super FX program $01:82CE on the vector in direct page
; $02, $08, $94 and puts its three results in direct page $B7, $B9, $BB.
; HOW: Inputs to $70:0068, $70:002C, $70:002E; GsuRun; results from
; $70:0026, $70:0028, $70:002A.
; Entry: any widths, D = $0000. Exit: A = the third result; X, Y, P
; unchanged.
%at($03AFD7)
Gsu82CE:
    phx
    phy
    php
    rep.b #$30
    lda.b !dp_02
    sta.l !gsu_in_68
    lda.b !dp_08
    sta.l !gsu_dest
    lda.b !dp_94
    sta.l !gsu_in_2e
    sep.b #$20
    lda.b #$01
    ldx.w #$82CE
    jsl.l GsuRun
    rep.b #$20
    lda.l !gsu_out_26
    sta.b !dp_b7
    lda.l !gsu_out_28
    sta.b !dp_b9
    lda.l !gsu_out_2a
    sta.b !dp_bb
    plp
    ply
    plx
    rtl

; @routine 03:B011 Gsu8325
; WHAT IT DOES: Runs Super FX program $01:8325 on the three words at $065E
; and copies the nine words it leaves at $70:00E4 to $0608.
; HOW: Inputs to $70:0020-$70:0024; GsuRun; nine word copies. Work RAM is
; reached through the data bank.
; Entry: X 16-bit, D = $0000. Exit: A = the last word; X = $25, Y high byte
; clear; M 8-bit, X 8-bit; N, Z from A (16-bit).
%at($03B011)
Gsu8325:
    rep.b #$20
    lda.w !wram_065E
    sta.l !gsu_in_20
    lda.w !wram_065E+2
    sta.l !gsu_in_22
    lda.w !wram_065E+4
    sta.l !gsu_in_24
    sep.b #$20
    rep.b #$10
    lda.b #$01
    ldx.w #$8325
    jsl.l GsuRun
    rep.b #$20
    lda.l !gsu_out_e4
    sta.w !gsu_8325_out
    lda.l !gsu_out_e4+2
    sta.w !gsu_8325_out+2
    lda.l !gsu_out_e4+4
    sta.w !gsu_8325_out+4
    lda.l !gsu_out_e4+6
    sta.w !gsu_8325_out+6
    lda.l !gsu_out_e4+8
    sta.w !gsu_8325_out+8
    lda.l !gsu_out_e4+10
    sta.w !gsu_8325_out+10
    lda.l !gsu_out_e4+12
    sta.w !gsu_8325_out+12
    lda.l !gsu_out_e4+14
    sta.w !gsu_8325_out+14
    lda.l !gsu_out_e4+16
    sta.w !gsu_8325_out+16
    sep.b #$30
    rtl

; @routine 03:FE8D GsuEE88Ramp
; WHAT IT DOES: Writes the ramp $80, $90, ... $F0 to $70:3C00 and runs Super
; FX program $07:EE88.
; HOW: Eight word stores, then GsuRun.
; Entry: any widths. Exit: A = the screen mode value (high byte clear); X =
; $88, Y high byte clear; M 16-bit, X 8-bit; N, Z from the data bank.
%at($03FE8D)
GsuEE88Ramp:
    rep.b #$20
    lda.w #$0080
    sta.l !gsu_ramp
    lda.w #$0090
    sta.l !gsu_ramp+2
    lda.w #$00A0
    sta.l !gsu_ramp+4
    lda.w #$00B0
    sta.l !gsu_ramp+6
    lda.w #$00C0
    sta.l !gsu_ramp+8
    lda.w #$00D0
    sta.l !gsu_ramp+10
    lda.w #$00E0
    sta.l !gsu_ramp+12
    lda.w #$00F0
    sta.l !gsu_ramp+14
    sep.b #$20
    rep.b #$10
    ldx.w #$EE88
    lda.b #$07
    jsl.l GsuRun
    rep.b #$20
    sep.b #$10
    rtl
