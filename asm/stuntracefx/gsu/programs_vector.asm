; programs_vector.asm: two more S-CPU wrappers of Super FX programs on
; the vector in direct page $02, $08, $94.
; C: src/stuntracefx/gsu/programs_vector.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:AF9D Gsu8277
; WHAT IT DOES: Runs Super FX program $01:8277 on the vector in direct
; page $02, $08, $94 and puts its three results in direct page $B7, $B9,
; $BB.
; HOW: As Gsu82CE: inputs to $70:0068, $70:002C, $70:002E; GsuRun; results
; from $70:0026, $70:0028, $70:002A. X, Y and P saved and restored.
; Entry: any widths, D = $0000. Exit: A = the third result (16 bits); X, Y
; unchanged; P unchanged but N, Z, which the final pull of X sets.
%at($03AF9D)
Gsu8277:
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
    ldx.w #$8277
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

; @routine 03:C9D2 VectorGsu81E3
; WHAT IT DOES: Puts direct page $04 - $02 in $02 and $05F2 - $94 in $08,
; then runs Gsu81E3Long: ObjectGsu81E3 for a point in $02, $94 instead of
; an object.
; HOW: 16-bit subtracts, $05F2 through the data bank; X and Y saved and
; restored.
; Entry: X 16-bit, D = $0000. Exit: A as Gsu81E3Long leaves it; X, Y
; unchanged; M 16-bit; N, Z from X.
%at($03C9D2)
VectorGsu81E3:
    phx
    phy
    rep.b #$20
    lda.b !dp_04
    sec
    sbc.b !dp_02
    sta.b !dp_02
    lda.w !rot_z
    sec
    sbc.b !dp_94
    sta.b !dp_08
    jsl.l Gsu81E3Long
    rep.b #$20
    ply
    plx
    rtl
