; object_find.asm: searches of the object lists that nothing in the game
; calls; the companion of src/stuntracefx/object/object_find.c.
; C: src/stuntracefx/object/object_find.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch 65816

; @routine 03:C08F ObjectAtPoint
; WHAT IT DOES: Finds the first object in the used list with bit 0 of +$24
; set whose box (half sizes +$27 in X, +$29 in Z) holds the point in
; direct page $02 (X) and $08 (Z); X = that object, or 0.
; HOW: For each object, |+$0C - $02| - +$27 and then |+$10 - $08| - +$29
; must both be negative; the half size is put in direct page $04. The
; skip to the next object is a JML within the routine.
; Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X = the
; object or 0; Y = the last link read; A, N, V, C from the last subtract
; (Z from X when none is found).
%at($03C08F)
ObjectAtPoint:
    ldx.w !object_list_0325
    beq .lC0D2
.lC094:
    lda.w !obj_24,x
    and.w #$0001
    bne .lC0A0
    jml.l .lC0CC
.lC0A0:
    lda.w !obj_27,x
    sta.b !dp_04
    lda.w !obj_pos_x,x
    sec
    sbc.b !dp_02
    bpl .lC0B1
    eor.w #$FFFF
    inc a
.lC0B1:
    sec
    sbc.b !dp_04
    bpl .lC0CC
    lda.w !obj_29,x
    sta.b !dp_04
    lda.w !obj_pos_z,x
    sec
    sbc.b !dp_08
    bpl .lC0C7
    eor.w #$FFFF
    inc a
.lC0C7:
    sec
    sbc.b !dp_04
    bmi .lC0D5
.lC0CC:
    ldy.w !obj_base,x
    tyx
    bne .lC094
.lC0D2:
    ldx.w #$0000
.lC0D5:
    rtl

; @routine 03:C0D6 ObjectNextOfKind
; WHAT IT DOES: Steps the search kept at $1F30 to the next object of kind
; A (any kind when A is 0), skipping the object X; Y = the object found
; and $1F30 = the one after it. When the search has ended ($1F30 = 0) Y =
; 0 and $1F30 stays 0. With a kind and no match left, Y = 0 and $1F30 =
; object 0's link.
; HOW: X is kept in direct page $3E; the list is walked by +$00 from
; $1F30, comparing +$04 with A.
; Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: A, X
; unchanged; Y as above; N, Z from X; C from the last compare.
ObjectNextOfKind:
    stx.b !scale_scratch
    ldx.w !wram_1F2D+3
    beq .lC0FC
    cmp.w #$0000
    beq .lC105
    txy
.lC0E3:
    cmp.w !obj_04,x
    beq .lC0EE
.lC0E8:
    ldy.w !obj_base,x
    tyx
    bne .lC0E3
.lC0EE:
    cpx.b !scale_scratch
    beq .lC0E8
    ldy.w !obj_base,x
    sty.w !wram_1F2D+3
    txy
    ldx.b !scale_scratch
    rtl
.lC0FC:
    ldy.w #$0000
    sty.w !wram_1F2D+3
    ldx.b !scale_scratch
    rtl
.lC105:
    ldy.w !obj_base,x
    beq .lC0FC
    tyx
    cpx.b !scale_scratch
    beq .lC105
    stx.w !wram_1F2D+3
    ldx.b !scale_scratch
    rtl

; @routine 03:C115 ObjectNextFlagged
; WHAT IT DOES: As ObjectNextOfKind, but the object must also have one of
; the flag bits in direct page $02 set in its +$24 byte; with A = 0 any
; kind does. Y = the object found and $1F30 = the one after it.
; HOW: X kept in direct page $3E; A kept in $1F2B around the 8-bit flag
; test. The ended case shares ObjectNextOfKind's code.
; Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X unchanged;
; Y as above; A = the kind with the flag test's byte in its low byte (as
; entered when the search ended); M 16-bit; N, Z from X.
ObjectNextFlagged:
    cmp.w #$0000
    beq .lC14E
    stx.b !scale_scratch
    ldx.w !wram_1F2D+3
    beq ObjectNextOfKind_lC0FC
    txy
.lC122:
    cmp.w !obj_04,x
    beq .lC12D
.lC127:
    ldy.w !obj_base,x
    tyx
    bne .lC122
.lC12D:
    cpx.b !scale_scratch
    beq .lC127
    sta.w !wram_1F2B
    sep.b #$20
    lda.w !obj_24,x
    and.b !dp_02
    rep.b #$20
    bne .lC144
    lda.w !wram_1F2B
    bra .lC127
.lC144:
    ldy.w !obj_base,x
    sty.w !wram_1F2D+3
    txy
    ldx.b !scale_scratch
    rtl
.lC14E:
    stx.b !scale_scratch
    ldx.w !wram_1F2D+3
    beq ObjectNextOfKind_lC0FC
    txy
    sep.b #$20
.lC158:
    cpx.b !scale_scratch
    beq .lC163
    lda.w !obj_24,x
    and.b !dp_02
    bne .lC169
.lC163:
    ldy.w !obj_base,x
    tyx
    bne .lC158
.lC169:
    ldy.w !obj_base,x
    sty.w !wram_1F2D+3
    txy
    ldx.b !scale_scratch
    rep.b #$20
    rtl

; @routine 03:C175 ObjectNearestOfKind
; WHAT IT DOES: From the search point $1F30, finds the object of kind A
; (any kind when A is 0) other than X nearest to X by
; ObjectDistanceApprox, counting only distances from $42 up to below $3E;
; Y = that object (0 if none), $3E = its distance. The search is used up:
; $1F30 = object 0's link (any kind: 0).
; HOW: X kept in direct page $04, the kind in $0A, the best object in $40;
; the compares are by the sign of the difference.
; Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X unchanged;
; Y as above; A = the last distance (or the kind); M 16-bit; N, Z from X.
ObjectNearestOfKind:
    stx.b !dp_04
    ldx.w !wram_1F2D+3
    bne .lC17F
    brl .lC1BB
.lC17F:
    cmp.w #$0000
    beq .lC1C1
    sta.b !actor_0a
    stz.b !dp_40
.lC188:
    cpx.b !dp_04
    beq .lC1AA
    lda.w !obj_04,x
    cmp.b !actor_0a
    bne .lC1AA
    ldy.b !dp_04
    jsl.l ObjectDistanceApprox
    rep.b #$20
    lda.w !wram_035E+6
    cmp.b !scale_scratch
    bpl .lC1AA
    cmp.b !fxrec_sides
    bmi .lC1AA
    sta.b !scale_scratch
    stx.b !dp_40
.lC1AA:
    ldy.w !obj_base,x
    tyx
    bne .lC188
    ldy.w !obj_base,x
    sty.w !wram_1F2D+3
    ldy.b !dp_40
    ldx.b !dp_04
    rtl
.lC1BB:
    ldy.w #$0000
    ldx.b !dp_04
    rtl
.lC1C1:
    stz.b !dp_40
.lC1C3:
    cpx.b !dp_04
    beq .lC1DE
    ldy.b !dp_04
    jsl.l ObjectDistanceApprox
    rep.b #$20
    lda.w !wram_035E+6
    cmp.b !scale_scratch
    bpl .lC1DE
    cmp.b !fxrec_sides
    bmi .lC1DE
    sta.b !scale_scratch
    stx.b !dp_40
.lC1DE:
    ldy.w !obj_base,x
    tyx
    bne .lC1C3
    sty.w !wram_1F2D+3
    ldy.b !dp_40
    ldx.b !dp_04
    rtl

; @routine 03:C1EC ObjectFirstWithin
; WHAT IT DOES: From the search point $1F30, finds the first object of
; kind A (any kind when A is 0) other than X whose ObjectDistanceApprox
; from X is from $42 up to below $3E; Y = that object and $1F30 = the one
; after it. With none, Y = 0 and $1F30 = object 0's link.
; HOW: X kept in direct page $04, the kind in $0A ($40 is cleared and not
; used); the compares are by the sign of the difference. The ended case
; shares ObjectNearestOfKind's code.
; Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: X unchanged;
; Y as above; A = the last distance (or the kind); M 16-bit; N, Z from X.
ObjectFirstWithin:
    stx.b !dp_04
    ldx.w !wram_1F2D+3
    beq ObjectNearestOfKind_lC1BB
    cmp.w #$0000
    beq .lC22A
    sta.b !actor_0a
    stz.b !dp_40
.lC1FC:
    cpx.b !dp_04
    beq .lC21A
    lda.w !obj_04,x
    cmp.b !actor_0a
    bne .lC21A
    ldy.b !dp_04
    jsl.l ObjectDistanceApprox
    rep.b #$20
    lda.w !wram_035E+6
    cmp.b !scale_scratch
    bpl .lC21A
    cmp.b !fxrec_sides
    bpl .lC220
.lC21A:
    ldy.w !obj_base,x
    tyx
    bne .lC1FC
.lC220:
    ldy.w !obj_base,x
    sty.w !wram_1F2D+3
    txy
    ldx.b !dp_04
    rtl
.lC22A:
    cpx.b !dp_04
    beq .lC241
    ldy.b !dp_04
    jsl.l ObjectDistanceApprox
    rep.b #$20
    lda.w !wram_035E+6
    cmp.b !scale_scratch
    bpl .lC241
    cmp.b !fxrec_sides
    bpl .lC247
.lC241:
    ldy.w !obj_base,x
    tyx
    bne .lC22A
.lC247:
    ldy.w !obj_base,x
    sty.w !wram_1F2D+3
    txy
    ldx.b !dp_04
    rtl

; @routine 03:C251 ObjectFirstOfKind
; WHAT IT DOES: X = the first object of kind A in the used list, or 0.
; HOW: Walks the list from $0325 by +$00, comparing +$04 with A. P is
; saved and restored.
; Entry: M 16-bit, X 16-bit; data bank $7E. Exit: A unchanged; X as
; above; Y = the object or 0; P unchanged.
ObjectFirstOfKind:
    php
    ldy.w !object_list_0325
.lC255:
    tyx
    cmp.w !obj_04,x
    beq .lC263
    ldy.w !obj_base,x
    bne .lC255
    ldx.w #$0000
.lC263:
    plp
    rtl

; @routine 03:C265 ObjectCountOfKind
; WHAT IT DOES: Counts the objects of kind A in the used list into
; $1F2B, and returns the count.
; HOW: Walks the list from $0325 by +$00, comparing +$04 with A; INC of
; the word $1F2B per match. P is saved and restored.
; Entry: M 16-bit, X 16-bit; data bank $7E. Exit: A = the count; X = the
; last object; Y = 0; P unchanged.
ObjectCountOfKind:
    php
    ldy.w !object_list_0325
    stz.w !wram_1F2B
.lC26C:
    tyx
    cmp.w !obj_04,x
    bne .lC275
    inc.w !wram_1F2B
.lC275:
    ldy.w !obj_base,x
    bne .lC26C
    lda.w !wram_1F2B
    plp
    rtl

; @routine 03:CA19 ObjectDistanceApprox
; WHAT IT DOES: Sets $0364 to an estimate of the distance in X and Z
; between objects Y and X: with dx, dz their halved absolute differences
; in direct page $02 and $08, it is ((2 (dx + dz) + max) * 9 / 2) / 8,
; about 1.1 times the true distance, which no square root would give
; faster.
; HOW: 16-bit subtracts and absolute values; the halvings are CMP #$8000,
; ROR; times 9/2 as a / 2 + a * 4. Work: direct page $02 and $08.
; Entry: X 16-bit, D = $0000; data bank $7E; X, Y = objects. Exit: M
; 8-bit; A = the estimate (both bytes); X, Y unchanged; N, Z, C from the
; last halving, V from the last add.
%at($03CA19)
ObjectDistanceApprox:
    rep.b #$20
    lda.w !obj_pos_x,y
    sec
    sbc.w !obj_pos_x,x
    bpl .lCA28
    eor.w #$FFFF
    inc a
.lCA28:
    sta.b !dp_02
    lda.w !obj_pos_z,y
    sec
    sbc.w !obj_pos_z,x
    bpl .lCA37
    eor.w #$FFFF
    inc a
.lCA37:
    sta.b !dp_08
    rep.b #$20
    lda.b !dp_02
    cmp.w #$8000
    ror a
    sta.b !dp_02
    lda.b !dp_08
    cmp.w #$8000
    ror a
    sta.b !dp_08
    lda.b !dp_08
    clc
    adc.b !dp_02
    asl a
    sta.w !wram_035E+6
    lda.b !dp_08
    cmp.b !dp_02
    bmi .lCA5E
    lda.b !dp_08
    bra .lCA60
.lCA5E:
    lda.b !dp_02
.lCA60:
    clc
    adc.w !wram_035E+6
    sta.b !dp_02
    asl a
    asl a
    sta.b !dp_08
    lda.b !dp_02
    cmp.w #$8000
    ror a
    clc
    adc.b !dp_08
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    cmp.w #$8000
    ror a
    sta.w !wram_035E+6
    sep.b #$20
    rtl

; @routine 03:CA85 ObjectDistanceXY
; WHAT IT DOES: Sets $1A62 to |dx| + |dy| between objects Y and X (their
; +$0C and +$0E), leaving dx in direct page $02 and dy in $08.
; HOW: 16-bit subtracts; |dx| as 0 - dx; dy added or subtracted by its
; sign.
; Entry: X 16-bit, D = $0000; data bank $7E; X, Y = objects. Exit: M
; 8-bit; A = the sum (both bytes); X, Y unchanged; N, Z, C, V from the
; last add or subtract.
ObjectDistanceXY:
    rep.b #$20
    lda.w !obj_pos_x,y
    sec
    sbc.w !obj_pos_x,x
    sta.b !dp_02
    bpl .lCA98
    lda.w #$0000
    sec
    sbc.b !dp_02
.lCA98:
    sta.w !current_player+5
    lda.w !obj_pos_y,y
    sec
    sbc.w !obj_pos_y,x
    sta.b !dp_08
    bmi .lCAAC
    clc
    adc.w !current_player+5
    bra .lCAB2
.lCAAC:
    sec
    lda.w !current_player+5
    sbc.b !dp_08
.lCAB2:
    sta.w !current_player+5
    sep.b #$20
    rtl

; @routine 03:C81F ObjectPointGsu81E3
; WHAT IT DOES: ObjectPointDistanceXZ, then Super FX program $01:81E3 on
; the dx, dz it leaves (Gsu81E3Long); returns the result's high byte.
; Nothing in the game calls it.
; HOW: X and Y pushed and pulled around the calls; XBA, AND #$00FF.
; Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit, X 16-bit; A
; = the high byte of Gsu81E3Long's result (high byte 0); X, Y unchanged;
; N, Z from X.
%at($03C81F)
ObjectPointGsu81E3:
    phx
    phy
    jsl.l ObjectPointDistanceXZ
    jsl.l Gsu81E3Long
    rep.b #$20
    xba
    and.w #$00FF
    sep.b #$20
    rep.b #$10
    ply
    plx
    rtl

; @routine 03:C836 ObjectOffsetDistanceXZ
; WHAT IT DOES: Sets $0364 to |dx| + |dz| from object X to object Y moved
; by direct page $04 in X and by $05F2 in Z, leaving dx in $02 and dz in
; $08. Nothing in the game calls it.
; HOW: 16-bit adds and subtracts, as ObjectDistanceXY.
; Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = the sum;
; X, Y unchanged; N, Z, C, V from the last add or subtract.
ObjectOffsetDistanceXZ:
    rep.b #$20
    lda.w !obj_pos_x,y
    clc
    adc.b !dp_04
    sec
    sbc.w !obj_pos_x,x
    sta.b !dp_02
    bpl .lC84C
    lda.w #$0000
    sec
    sbc.b !dp_02
.lC84C:
    sta.w !wram_035E+6
    lda.w !obj_pos_z,y
    clc
    adc.w !rot_z
    sec
    sbc.w !obj_pos_z,x
    sta.b !dp_08
    bmi .lC864
    clc
    adc.w !wram_035E+6
    bra .lC86A
.lC864:
    sec
    lda.w !wram_035E+6
    sbc.b !dp_08
.lC86A:
    sta.w !wram_035E+6
    sep.b #$20
    rtl

; @routine 03:C870 ObjectPointDistanceXZ
; WHAT IT DOES: Sets $0364 to |dx| + |dz| from object X to the point in
; direct page $04 (X) and $05F2 (Z), leaving dx in $02 and dz in $08.
; Nothing in the game calls it but ObjectPointGsu81E3.
; HOW: 16-bit subtracts, as ObjectDistanceXY; $05F2 through the data
; bank.
; Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = the sum;
; X, Y unchanged; N, Z, C, V from the last add or subtract.
ObjectPointDistanceXZ:
    rep.b #$20
    lda.b !dp_04
    sec
    sbc.w !obj_pos_x,x
    sta.b !dp_02
    bpl .lC882
    lda.w #$0000
    sec
    sbc.b !dp_02
.lC882:
    sta.w !wram_035E+6
    lda.w !rot_z
    sec
    sbc.w !obj_pos_z,x
    sta.b !dp_08
    bmi .lC896
    clc
    adc.w !wram_035E+6
    bra .lC89C
.lC896:
    sec
    lda.w !wram_035E+6
    sbc.b !dp_08
.lC89C:
    sta.w !wram_035E+6
    sep.b #$20
    rtl

; @routine 03:C8A2 ObjectPointDistanceXY
; WHAT IT DOES: Sets $0364 to |dx| + |dy| from object X to the point in
; direct page $04 (X) and $0A (Y), leaving dx in $02 and dy in $08.
; Nothing in the game calls it.
; HOW: 16-bit subtracts, as ObjectDistanceXY.
; Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = the sum;
; X, Y unchanged; N, Z, C, V from the last add or subtract.
ObjectPointDistanceXY:
    rep.b #$20
    lda.b !dp_04
    sec
    sbc.w !obj_pos_x,x
    sta.b !dp_02
    bpl .lC8B4
    lda.w #$0000
    sec
    sbc.b !dp_02
.lC8B4:
    sta.w !wram_035E+6
    lda.b !actor_0a
    sec
    sbc.w !obj_pos_y,x
    sta.b !dp_08
    bmi .lC8C7
    clc
    adc.w !wram_035E+6
    bra .lC8CD
.lC8C7:
    sec
    lda.w !wram_035E+6
    sbc.b !dp_08
.lC8CD:
    sta.w !wram_035E+6
    sep.b #$20
    rtl

; @routine 03:C8D3 ObjectVelocityAddFrom
; WHAT IT DOES: Adds object Y's velocity (+$2E, +$30, +$32) to object
; X's, keeping Y's in direct page $02, $08, $94. Nothing in the game
; calls it.
; HOW: X and Y swapped through direct page $3E (twice); 16-bit adds.
; Entry: X 16-bit, D = $0000; data bank $7E. Exit: M 8-bit; A = X's new
; +$32; X, Y unchanged; $3E = Y; N, Z, C, V from the last add.
ObjectVelocityAddFrom:
    rep.b #$20
    stx.b !scale_scratch
    tyx
    ldy.b !scale_scratch
    lda.w !obj_vel_x,x
    sta.b !dp_02
    lda.w !obj_vel_y,x
    sta.b !dp_08
    lda.w !obj_vel_z,x
    sta.b !dp_94
    stx.b !scale_scratch
    tyx
    ldy.b !scale_scratch
    lda.w !obj_vel_x,x
    clc
    adc.b !dp_02
    sta.w !obj_vel_x,x
    lda.w !obj_vel_y,x
    clc
    adc.b !dp_08
    sta.w !obj_vel_y,x
    lda.w !obj_vel_z,x
    clc
    adc.b !dp_94
    sta.w !obj_vel_z,x
    sep.b #$20
    rtl

; @routine 03:C90C ObjectPairGsu81E3
; WHAT IT DOES: Runs Super FX program $01:81E3 (Gsu81E3Long) on the X and
; Z differences from object X to object Y, put in direct page $02 and
; $08. Nothing in the game calls it.
; HOW: X and Y pushed and pulled around it; subtracts in the entry width.
; Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit;
; A as Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
ObjectPairGsu81E3:
    phx
    phy
    lda.w !obj_pos_x,y
    sec
    sbc.w !obj_pos_x,x
    sta.b !dp_02
    lda.w !obj_pos_z,y
    sec
    sbc.w !obj_pos_z,x
    sta.b !dp_08
    jsl.l Gsu81E3Long
    rep.b #$30
    ply
    plx
    rtl

; @routine 03:C929 ObjectPairPitchGsu81E3
; WHAT IT DOES: Runs Super FX program $01:81E3 (Gsu81E3Long) on the
; height difference from object X to object Y (direct page $02) and
; their ObjectDistanceApprox (direct page $08). Nothing in the game calls
; it.
; HOW: X and Y pushed and pulled around it; 16-bit subtract.
; Entry: X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit; A as
; Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
ObjectPairPitchGsu81E3:
    phx
    phy
    jsl.l ObjectDistanceApprox
    rep.b #$20
    lda.w !wram_035E+6
    sta.b !dp_08
    lda.w !obj_pos_y,y
    sec
    sbc.w !obj_pos_y,x
    sta.b !dp_02
    jsl.l Gsu81E3Long
    rep.b #$30
    ply
    plx
    rtl

; @routine 03:C948 ObjectPairOffsetGsu81E3
; WHAT IT DOES: As ObjectPairGsu81E3 with object Y moved by direct page
; $04 in X and $05F2 in Z. Nothing in the game calls it.
; HOW: The differences in the entry width, then the offsets added in 16
; bits; X and Y pushed and pulled around it.
; Entry: M 16-bit, X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit;
; A as Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
ObjectPairOffsetGsu81E3:
    phx
    phy
    lda.w !obj_pos_x,y
    sec
    sbc.w !obj_pos_x,x
    sta.b !dp_02
    lda.w !obj_pos_z,y
    sec
    sbc.w !obj_pos_z,x
    sta.b !dp_08
    rep.b #$20
    lda.b !dp_02
    clc
    adc.b !dp_04
    sta.b !dp_02
    lda.b !dp_08
    clc
    adc.w !rot_z
    sta.b !dp_08
    jsl.l Gsu81E3Long
    rep.b #$30
    ply
    plx
    rtl

; @routine 03:C976 ObjectPairOffsetPitchGsu81E3
; WHAT IT DOES: Runs Super FX program $01:81E3 (Gsu81E3Long) on the
; height difference from object X to object Y less direct page $0A
; (direct page $02) and ObjectOffsetDistanceXZ's sum (direct page $08).
; Nothing in the game calls it.
; HOW: X and Y pushed and pulled around it; 16-bit subtracts.
; Entry: X 16-bit, D = $0000; data bank $7E. Exit: M, X 16-bit; A as
; Gsu81E3Long leaves it; X, Y unchanged; N, Z from X.
ObjectPairOffsetPitchGsu81E3:
    phx
    phy
    jsl.l ObjectOffsetDistanceXZ
    rep.b #$20
    lda.w !wram_035E+6
    sta.b !dp_08
    lda.w !obj_pos_y,y
    sec
    sbc.w !obj_pos_y,x
    sec
    sbc.b !actor_0a
    sta.b !dp_02
    jsl.l Gsu81E3Long
    rep.b #$30
    ply
    plx
    rtl
