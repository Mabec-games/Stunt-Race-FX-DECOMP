; heading.asm: an object's velocity from its speed and heading with the
; S-CPU's hardware multiplier, and moving an object by such a vector.
; C: src/stuntracefx/math/heading.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:B93E HeadingFlat
; WHAT IT DOES: Object X's flat velocity from its speed (+$17) and its
; heading (+$15): $02 = speed sin, $94 = speed cos, $08 = 0.
; HOW: The tables read with the data bank 0; two signed multiplies.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A low byte = the
; z component's low byte; X, Y unchanged, X 16-bit; C set; V from the
; last negating subtract; N, Z from the caller's data bank.
%at($03B93E)
HeadingFlat:
    lda.w !obj_17,x
    sta.b !heading_speed
    phb
    stx.b !heading_save_x
    sty.b !heading_save_y
    stz.b !dp_08
    stz.b !dp_09
    lda.w !obj_15,x
    tax
    sep.b #$10
    lda.b #$00
    pha
    plb
    lda.w !rot_sin_table,x
    sta.b !heading_sin_a
    lda.w !rot_cos_table,x
    sta.b !heading_cos_a
    lda.b !heading_speed
    bmi .lB979
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lB99B
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lB9A9
.lB979:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lB98B
    sta.w !WRMPYB
    nop
    jmp.w .lB9A3
.lB98B:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lB9A9
.lB99B:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lB9A3:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lB9A9:
    tax
    ldx.b #$00
    sta.b !dp_02
    cmp.b #$00
    bpl .lB9B3
    dex
.lB9B3:
    stx.b !dp_03
    lda.b !heading_speed
    bmi .lB9CE
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lB9F0
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lB9FE
.lB9CE:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lB9E0
    sta.w !WRMPYB
    nop
    jmp.w .lB9F8
.lB9E0:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lB9FE
.lB9F0:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lB9F8:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lB9FE:
    tax
    ldx.b #$00
    sta.b !dp_94
    cmp.b #$00
    bpl .lBA08
    dex
.lBA08:
    stx.b !dp_95
    rep.b #$10
    ldx.b !heading_save_x
    ldy.b !heading_save_y
    plb
    rtl

; @routine 03:BA12 HeadingFlatMirrored
; WHAT IT DOES: As HeadingFlat, for the heading 1 - (+$15): the mirror of
; the object's heading, one step on.
; HOW: The heading negated, then INX; the INY it makes is undone when Y is
; read back.
; Entry: as HeadingFlat. Exit: as HeadingFlat.
%at($03BA12)
HeadingFlatMirrored:
    lda.w !obj_17,x
    sta.b !heading_speed
    lda.w !obj_15,x
    stx.b !heading_save_x
    sty.b !heading_save_y
    stz.b !dp_08
    stz.b !dp_09
    eor.b #$FF
    inc a
    tax
    sep.b #$10
    phb
    lda.b #$00
    pha
    plb
    inx
    iny
    lda.w !rot_sin_table,x
    sta.b !heading_sin_a
    lda.w !rot_cos_table,x
    sta.b !heading_cos_a
    lda.b !heading_speed
    bmi .lBA52
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBA74
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBA82
.lBA52:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBA64
    sta.w !WRMPYB
    nop
    jmp.w .lBA7C
.lBA64:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBA82
.lBA74:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBA7C:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBA82:
    tax
    ldx.b #$00
    sta.b !dp_02
    cmp.b #$00
    bpl .lBA8C
    dex
.lBA8C:
    stx.b !dp_03
    lda.b !heading_speed
    bmi .lBAA7
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBAC9
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBAD7
.lBAA7:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBAB9
    sta.w !WRMPYB
    nop
    jmp.w .lBAD1
.lBAB9:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBAD7
.lBAC9:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBAD1:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBAD7:
    tax
    ldx.b #$00
    sta.b !dp_94
    cmp.b #$00
    bpl .lBAE1
    dex
.lBAE1:
    stx.b !dp_95
    rep.b #$10
    ldx.b !heading_save_x
    ldy.b !heading_save_y
    plb
    rtl

; @routine 03:BAEB HeadingPitched
; WHAT IT DOES: Object X's velocity from its speed (+$17), heading (+$15)
; and pitch (+$14): $02 = (speed sin h) cos p, $94 = (speed cos h) cos p,
; $08 = speed sin p, where p is the pitch negated.
; HOW: pitched(); the high bytes of $02, $08, $94 cleared first and
; decremented for a negative product.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: as HeadingFlat (A
; low byte = the y component).
%at($03BAEB)
HeadingPitched:
    lda.w !obj_17,x
    sta.b !heading_speed
    stz.b !dp_03
    stz.b !dp_09
    stz.b !dp_95
    stx.b !heading_save_x
    sty.b !heading_save_y
    lda.w !obj_15,x
    tay
    lda.w !obj_14,x
    eor.b #$FF
    inc a
    tax
    sep.b #$10
    phb
    lda.b #$00
    pha
    plb
    lda.w !rot_sin_table,y
    sta.b !heading_sin_a
    lda.w !rot_cos_table,y
    sta.b !heading_cos_a
    lda.w !rot_sin_table,x
    sta.b !heading_sin_b
    lda.w !rot_cos_table,x
    sta.b !heading_cos_b
    lda.b !heading_speed
    bmi .lBB39
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBB5B
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBB69
.lBB39:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBB4B
    sta.w !WRMPYB
    nop
    jmp.w .lBB63
.lBB4B:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBB69
.lBB5B:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBB63:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBB69:
    tax
    sta.b !dp_02
    lda.b !dp_02
    bmi .lBB85
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBBA7
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBBB5
.lBB85:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBB97
    sta.w !WRMPYB
    nop
    jmp.w .lBBAF
.lBB97:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBBB5
.lBBA7:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBBAF:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBBB5:
    tax
    sta.b !dp_02
    cmp.b #$00
    bpl .lBBBE
    dec.b !dp_03
.lBBBE:
    lda.b !heading_speed
    bmi .lBBD7
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBBF9
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBC07
.lBBD7:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBBE9
    sta.w !WRMPYB
    nop
    jmp.w .lBC01
.lBBE9:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBC07
.lBBF9:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBC01:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBC07:
    tax
    sta.b !dp_94
    lda.b !dp_94
    bmi .lBC23
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBC45
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBC53
.lBC23:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBC35
    sta.w !WRMPYB
    nop
    jmp.w .lBC4D
.lBC35:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBC53
.lBC45:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBC4D:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBC53:
    tax
    sta.b !dp_94
    cmp.b #$00
    bpl .lBC5C
    dec.b !dp_95
.lBC5C:
    lda.b !heading_speed
    bmi .lBC75
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_b
    bmi .lBC97
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBCA5
.lBC75:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_b
    bmi .lBC87
    sta.w !WRMPYB
    nop
    jmp.w .lBC9F
.lBC87:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBCA5
.lBC97:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBC9F:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBCA5:
    tax
    sta.b !dp_08
    cmp.b #$00
    bpl .lBCAE
    dec.b !dp_09
.lBCAE:
    rep.b #$10
    ldx.b !heading_save_x
    ldy.b !heading_save_y
    plb
    rtl

; @routine 03:BCB6 HeadingPitchedVia1F7C
; WHAT IT DOES: As HeadingPitched with the heading negated instead of the
; pitch: the object's +$15 and +$14 are copied to $1F7D and $1F7C first,
; then HeadingPitchedFrom1F7C.
; HOW: The copies through the caller's data bank, then falls into
; HeadingPitchedFrom1F7C.
; Entry: as HeadingPitched. Exit: as HeadingPitched.
%at($03BCB6)
HeadingPitchedVia1F7C:
    lda.w !obj_17,x
    sta.b !heading_speed
    lda.w !obj_15,x
    sta.w !heading_turn
    lda.w !obj_14,x
    sta.w !heading_pitch

; @routine 03:BCC7 HeadingPitchedFrom1F7C
; WHAT IT DOES: The velocity for the speed in $82, heading -($1F7D) and
; pitch $1F7C: $02 = (speed sin h) cos p, $94 = (speed cos h) cos p, $08 =
; speed sin p.
; HOW: As HeadingPitched, the angles read with the data bank already 0.
; Entry: M 8-bit, X 16-bit, D = $0000; $82 = speed. Exit: as HeadingPitched.
%at($03BCC7)
HeadingPitchedFrom1F7C:
    stz.b !dp_03
    stz.b !dp_09
    stz.b !dp_95
    stx.b !heading_save_x
    sty.b !heading_save_y
    phb
    lda.b #$00
    pha
    plb
    lda.w !heading_turn
    eor.b #$FF
    inc a
    tay
    lda.w !heading_pitch
    tax
    sep.b #$10
    lda.w !rot_sin_table,y
    sta.b !heading_sin_a
    lda.w !rot_cos_table,y
    sta.b !heading_cos_a
    lda.w !rot_sin_table,x
    sta.b !heading_sin_b
    lda.w !rot_cos_table,x
    sta.b !heading_cos_b
    lda.b !heading_speed
    bmi .lBD10
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBD32
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBD40
.lBD10:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBD22
    sta.w !WRMPYB
    nop
    jmp.w .lBD3A
.lBD22:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBD40
.lBD32:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBD3A:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBD40:
    tax
    sta.b !dp_02
    lda.b !dp_02
    bmi .lBD5C
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBD7E
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBD8C
.lBD5C:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBD6E
    sta.w !WRMPYB
    nop
    jmp.w .lBD86
.lBD6E:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBD8C
.lBD7E:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBD86:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBD8C:
    tax
    sta.b !dp_02
    cmp.b #$00
    bpl .lBD95
    dec.b !dp_03
.lBD95:
    lda.b !heading_speed
    bmi .lBDAE
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBDD0
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBDDE
.lBDAE:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBDC0
    sta.w !WRMPYB
    nop
    jmp.w .lBDD8
.lBDC0:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBDDE
.lBDD0:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBDD8:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBDDE:
    tax
    sta.b !dp_94
    lda.b !dp_94
    bmi .lBDFA
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBE1C
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBE2A
.lBDFA:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_b
    bmi .lBE0C
    sta.w !WRMPYB
    nop
    jmp.w .lBE24
.lBE0C:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBE2A
.lBE1C:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBE24:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBE2A:
    tax
    sta.b !dp_94
    cmp.b #$00
    bpl .lBE33
    dec.b !dp_95
.lBE33:
    lda.b !heading_speed
    bmi .lBE4C
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_b
    bmi .lBE6E
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBE7C
.lBE4C:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_b
    bmi .lBE5E
    sta.w !WRMPYB
    nop
    jmp.w .lBE76
.lBE5E:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBE7C
.lBE6E:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBE76:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBE7C:
    tax
    sta.b !dp_08
    cmp.b #$00
    bpl .lBE85
    dec.b !dp_09
.lBE85:
    rep.b #$10
    ldx.b !heading_save_x
    ldy.b !heading_save_y
    plb
    rtl

; @routine 03:BE8D HeadingFlatQuarterTurn
; WHAT IT DOES: As HeadingFlat for the speed A and the heading $41 - (+$15)
; of object X: a quarter turn from its mirror, one step on.
; HOW: SEC / LDA #$40 / SBC (the subtract sets V), then INX.
; Entry: M 8-bit, X 16-bit, D = $0000; A = speed, X = object. Exit: as
; HeadingFlat.
%at($03BE8D)
HeadingFlatQuarterTurn:
    sta.b !heading_speed
    stx.b !heading_save_x
    sty.b !heading_save_y
    stz.b !dp_08
    stz.b !dp_09
    sec
    lda.b #$40
    sbc.w !obj_15,x
    tax
    sep.b #$10
    phb
    lda.b #$00
    pha
    plb
    inx
    lda.w !rot_sin_table,x
    sta.b !heading_sin_a
    lda.w !rot_cos_table,x
    sta.b !heading_cos_a
    lda.b !heading_speed
    bmi .lBEC9
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBEEB
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBEF9
.lBEC9:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBEDB
    sta.w !WRMPYB
    nop
    jmp.w .lBEF3
.lBEDB:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBEF9
.lBEEB:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBEF3:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBEF9:
    tax
    ldx.b #$00
    sta.b !dp_02
    cmp.b #$00
    bpl .lBF03
    dex
.lBF03:
    stx.b !dp_03
    lda.b !heading_speed
    bmi .lBF1E
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBF40
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBF4E
.lBF1E:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lBF30
    sta.w !WRMPYB
    nop
    jmp.w .lBF48
.lBF30:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBF4E
.lBF40:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBF48:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBF4E:
    tax
    ldx.b #$00
    sta.b !dp_94
    cmp.b #$00
    bpl .lBF58
    dex
.lBF58:
    stx.b !dp_95
    rep.b #$10
    ldx.b !heading_save_x
    ldy.b !heading_save_y
    plb
    rtl

; @routine 03:BF62 HeadingFlatSpeedA
; WHAT IT DOES: As HeadingFlat for the speed A and the heading (+$15) + 1
; of object X.
; HOW: As HeadingFlat, with INX.
; Entry: as HeadingFlatQuarterTurn. Exit: as HeadingFlat.
%at($03BF62)
HeadingFlatSpeedA:
    sta.b !heading_speed
    stx.b !heading_save_x
    sty.b !heading_save_y
    stz.b !dp_08
    stz.b !dp_09
    lda.w !obj_15,x
    tax
    sep.b #$10
    phb
    lda.b #$00
    pha
    plb
    inx
    lda.w !rot_sin_table,x
    sta.b !heading_sin_a
    lda.w !rot_cos_table,x
    sta.b !heading_cos_a
    lda.b !heading_speed
    bmi .lBF9B
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBFBD
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBFCB
.lBF9B:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_sin_a
    bmi .lBFAD
    sta.w !WRMPYB
    nop
    jmp.w .lBFC5
.lBFAD:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lBFCB
.lBFBD:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lBFC5:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lBFCB:
    tax
    ldx.b #$00
    sta.b !dp_02
    cmp.b #$00
    bpl .lBFD5
    dex
.lBFD5:
    stx.b !dp_03
    lda.b !heading_speed
    bmi .lBFF0
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lC012
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC020
.lBFF0:
    eor.b #$FF
    inc a
    asl a
    sta.w !WRMPYA
    lda.b !heading_cos_a
    bmi .lC002
    sta.w !WRMPYB
    nop
    jmp.w .lC01A
.lC002:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
    nop
    nop
    lda.w !RDMPYH
    jmp.w .lC020
.lC012:
    eor.b #$FF
    inc a
    sta.w !WRMPYB
    nop
    nop
.lC01A:
    lda.b #$00
    sec
    sbc.w !RDMPYH
.lC020:
    tax
    ldx.b #$00
    sta.b !dp_94
    cmp.b #$00
    bpl .lC02A
    dex
.lC02A:
    stx.b !dp_95
    rep.b #$10
    ldx.b !heading_save_x
    ldy.b !heading_save_y
    plb
    rtl

; @routine 03:C034 ObjectMoveByVectorTimes4
; WHAT IT DOES: Moves object X by four times the vector at $02, $08, $94
; (16-bit), which is left doubled twice.
; HOW: Each word shifted left twice, all three once and then again, then
; the adds of ObjectMoveByVector.
; Entry: X 16-bit, D = $0000; X = object. Exit: A = the new +$10; M 8-bit;
; N, Z, C, V from its add.
%at($03C034)
ObjectMoveByVectorTimes4:
    rep.b #$20
    asl.b !dp_02
    asl.b !dp_08
    asl.b !dp_94
    asl.b !dp_02
    asl.b !dp_08
    asl.b !dp_94
    bra ObjectMoveByVectorAdd

; @routine 03:C044 ObjectMoveByVectorTimes2
; WHAT IT DOES: Moves object X by twice the vector at $02, $08, $94, which
; is left doubled.
; HOW: Each word shifted left once, then ObjectMoveByVector.
; Entry: as ObjectMoveByVectorTimes4. Exit: as ObjectMoveByVectorTimes4.
%at($03C044)
ObjectMoveByVectorTimes2:
    rep.b #$20
    asl.b !dp_02
    asl.b !dp_08
    asl.b !dp_94

; @routine 03:C04C ObjectMoveByVector
; WHAT IT DOES: Moves object X by the vector at $02, $08, $94: adds them
; to its +$0C, +$0E and +$10.
; HOW: Three 16-bit adds, through the caller's data bank.
; Entry: as ObjectMoveByVectorTimes4. Exit: as ObjectMoveByVectorTimes4.
%at($03C04C)
ObjectMoveByVector:
    rep.b #$20
ObjectMoveByVectorAdd:
    lda.w !obj_pos_x,x
    clc
    adc.b !dp_02
    sta.w !obj_pos_x,x
    lda.w !obj_pos_y,x
    clc
    adc.b !dp_08
    sta.w !obj_pos_y,x
    lda.w !obj_pos_z,x
    clc
    adc.b !dp_94
    sta.w !obj_pos_z,x
    sep.b #$20
    rtl

; @routine 03:CD1D ObjectVelocityFromHeading
; WHAT IT DOES: Sets object X's velocity (+$2E, +$30, +$32) to the vector
; HeadingPitchedFrom1F7C makes for the speed in $82 and the angles at
; $1F7C/$1F7D.
; HOW: The vector's words copied, 16-bit, through the caller's data bank.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = the z word;
; M 8-bit; N, Z from it; C, V as HeadingPitchedFrom1F7C leaves them.
%at($03CD1D)
ObjectVelocityFromHeading:
    jsl.l HeadingPitchedFrom1F7C
    rep.b #$20
    lda.b !dp_02
    sta.w !obj_vel_x,x
    lda.b !dp_08
    sta.w !obj_vel_y,x
    lda.b !dp_94
    sta.w !obj_vel_z,x
    sep.b #$20
    rtl

; @routine 03:CD35 ObjectVelocityFromHeadingTimes2
; WHAT IT DOES: As ObjectVelocityFromHeading with each word doubled.
; HOW: One ASL A on each word before it is stored.
; Entry: as ObjectVelocityFromHeading. Exit: as ObjectVelocityFromHeading,
; C from the last shift.
%at($03CD35)
ObjectVelocityFromHeadingTimes2:
    jsl.l HeadingPitchedFrom1F7C
    rep.b #$20
    lda.b !dp_02
    asl a
    sta.w !obj_vel_x,x
    lda.b !dp_08
    asl a
    sta.w !obj_vel_y,x
    lda.b !dp_94
    asl a
    sta.w !obj_vel_z,x
    sep.b #$20
    rtl

; @routine 03:CD50 ObjectVelocityFromHeadingTimes4
; WHAT IT DOES: As ObjectVelocityFromHeading with each word times 4.
; HOW: Two ASL A on each word.
; Entry: as ObjectVelocityFromHeading. Exit: as
; ObjectVelocityFromHeadingTimes2.
%at($03CD50)
ObjectVelocityFromHeadingTimes4:
    jsl.l HeadingPitchedFrom1F7C
    rep.b #$20
    lda.b !dp_02
    asl a
    asl a
    sta.w !obj_vel_x,x
    lda.b !dp_08
    asl a
    asl a
    sta.w !obj_vel_y,x
    lda.b !dp_94
    asl a
    asl a
    sta.w !obj_vel_z,x
    sep.b #$20
    rtl

; @routine 03:CD6E ObjectVelocityFromHeadingTimes8
; WHAT IT DOES: As ObjectVelocityFromHeading with each word times 8.
; HOW: Three ASL A on each word.
; Entry: as ObjectVelocityFromHeading. Exit: as
; ObjectVelocityFromHeadingTimes2.
%at($03CD6E)
ObjectVelocityFromHeadingTimes8:
    jsl.l HeadingPitchedFrom1F7C
    rep.b #$20
    lda.b !dp_02
    asl a
    asl a
    asl a
    sta.w !obj_vel_x,x
    lda.b !dp_08
    asl a
    asl a
    asl a
    sta.w !obj_vel_y,x
    lda.b !dp_94
    asl a
    asl a
    asl a
    sta.w !obj_vel_z,x
    sep.b #$20
    rtl

; @routine 03:D1C2 ObjectVelocityLevelFromDp
; WHAT IT DOES: Sets object X's velocity to the vector for the speed in
; $94, the pitch in $02 and the heading -$40 (HeadingPitchedFrom1F7C),
; with the z velocity (+$32) cleared instead of set.
; HOW: $02 to $1F7C, $94 to $82, $40 to $1F7D (8-bit, the first and last
; through the caller's data bank), then the words at $02 and $08 copied to
; +$2E and +$30.
; Entry: M 8-bit, X 16-bit, D = $0000; X = object. Exit: A = the word at
; $08; M 8-bit; N, Z from it; C, V as HeadingPitchedFrom1F7C leaves them.
%at($03D1C2)
ObjectVelocityLevelFromDp:
    lda.b !dp_02
    sta.w !heading_pitch
    lda.b !dp_94
    sta.b !heading_speed
    lda.b #$40
    sta.w !heading_turn
    jsl.l HeadingPitchedFrom1F7C
    rep.b #$20
    lda.b !dp_02
    sta.w !obj_vel_x,x
    lda.b !dp_08
    sta.w !obj_vel_y,x
    stz.w !obj_vel_z,x
    sep.b #$20
    rtl
