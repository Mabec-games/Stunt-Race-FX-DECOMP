; matrix.asm: 3x3 matrices of signed words in Super FX RAM, nine elements
; row by row, in 1.15 fixed point (FMULT keeps the high word of the
; product and ROL doubles it, so each product is (a * b) >> 15). The
; current one is at $00E4: rows at $E4, $EA, $F0.
; C: src/stuntracefx/render/matrix.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch superfx

; @routine 01:972B FxMatrixTranspose
; WHAT IT DOES: Transposes the 3x3 matrix at R9 in place.
; HOW: Swaps the three pairs across the diagonal: elements 1 and 3, then
; 2 and 6 together with 5 and 7, loading each with LDW and storing them
; back crossed with STW, R9 stepping between them.
; Entry: R9 = the matrix; R11 = return address. Exit: R9 = the matrix + 4
; (element 2); R1 = the old element 5, R2 = the old element 7, R3 = the
; old element 6; the last RAM address R9; OV, S, CY, Z from the last SUB #6.
; *old5, *old7, *old6 = those three, as the registers keep them.
%at($01972B)
FxMatrixTranspose:
    inc r9
    inc r9
    to r1
    ldw (r9)                    ; element 1
    inc r9
    inc r9
    to r2
    ldw (r9)                    ; element 2
    inc r9
    inc r9
    to r3
    ldw (r9)                    ; element 3
    from r1
    stw (r9)
    with r9
    sub #4
    from r3
    stw (r9)
    with r9
    add #8
    to r1
    ldw (r9)                    ; element 5
    inc r9
    inc r9
    to r3
    ldw (r9)                    ; element 6
    from r2
    stw (r9)
    inc r9
    inc r9
    to r2
    ldw (r9)                    ; element 7
    from r1
    stw (r9)
    with r9
    sub #4
    from r2
    stw (r9)
    with r9
    sub #6
    from r3
    stw (r9)
    jmp r11
    nop

; @routine 01:978B FxMatrixApply
; WHAT IT DOES: Multiplies the row vector (R1, R2, R3) by the current
; matrix and stores the three results as words at R9.
; HOW: CACHE; for each column, three FMULTs by the column's elements (LMS
; into R6), each doubled by ROL, summed in R5 then R0 and stored with STW;
; R9 steps 2 after each.
; Entry: R1, R2, R3 = the vector, R9 = where to store; R11 = return
; address. Exit: R9 = R9 + 6; R0 = the last result, R5 = its first two
; terms, R6 = the matrix's last element; the last RAM address the last
; result's; OV and CY from the last ADD R5, S and Z from R9.
; Returns the last result; *partial and *m22 = R5 and R6 as left.
%at($01978B)
FxMatrixApply:
    cache
    lms r6,(!gsu_out_e4-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+6)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+12)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    lms r6,(!gsu_out_e4-!gsu_ram+2)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+8)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+14)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    lms r6,(!gsu_out_e4-!gsu_ram+4)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+10)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+16)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    jmp r11
    nop

; @routine 01:975B FxMatrixApplyVectors
; WHAT IT DOES: Multiplies the three vectors at $0116, $011C and $0122 by
; the current matrix, storing the nine results as words from R9 on.
; HOW: Keeps R11 in R8, then for each vector loads it into R1, R2, R3 with
; LMS and calls FxMatrixApply, which steps R9; restores R11 and returns.
; Entry: R9 = where to store; R11 = return address. Exit: R9 = R9 + 18;
; R1, R2, R3 = the third vector; R8 = R11; R0, R5, R6, the last RAM address
; and the flags as the last FxMatrixApply leaves them.
%at($01975B)
FxMatrixApplyVectors:
    move r8,r11
    lms r1,(!gsu_vectors-!gsu_ram)
    lms r2,(!gsu_vectors-!gsu_ram+2)
    lms r3,(!gsu_vectors-!gsu_ram+4)
    link #4
    iwt r15,#FxMatrixApply
    nop
    lms r1,(!gsu_vectors-!gsu_ram+6)
    lms r2,(!gsu_vectors-!gsu_ram+8)
    lms r3,(!gsu_vectors-!gsu_ram+10)
    link #4
    iwt r15,#FxMatrixApply
    nop
    lms r1,(!gsu_vectors-!gsu_ram+12)
    lms r2,(!gsu_vectors-!gsu_ram+14)
    lms r3,(!gsu_vectors-!gsu_ram+16)
    link #4
    iwt r15,#FxMatrixApply
    nop
    move r11,r8
    jmp r11
    nop

; @routine 01:82CE FxMatrixRotateProgram
; WHAT IT DOES: A program the S-CPU starts: multiplies the current matrix
; by the column vector ($0068, $002C, $002E) and stores the result at
; $0026, $0028, $002A.
; HOW: CACHE; loads the vector into R1, R2, R3 with LMS; for each row, three
; FMULTs by the row's elements, each doubled by ROL, summed in R5 then R0
; and stored with SMS. Then STOP.
; Entry: started by the S-CPU. Exit (at the STOP): R1, R2, R3 = the
; vector; R0 = the last result, R5 = its first two terms, R6 = the
; matrix's last element; the last RAM address $002A; OV, S, CY, Z from the
; last ADD R5.
; Returns the last result; *partial and *m22 = R5 and R6 as left.
%at($0182CE)
FxMatrixRotateProgram:
    cache
    lms r1,(!gsu_in_68-!gsu_ram)
    lms r2,(!gsu_dest-!gsu_ram)
    lms r3,(!gsu_in_2e-!gsu_ram)
    lms r6,(!gsu_out_e4-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+2)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+4)
    from r3
    fmult
    rol
    add r5
    sms (!gsu_out_26-!gsu_ram),r0
    lms r6,(!gsu_out_e4-!gsu_ram+6)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+8)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+10)
    from r3
    fmult
    rol
    add r5
    sms (!gsu_out_28-!gsu_ram),r0
    lms r6,(!gsu_out_e4-!gsu_ram+12)
    from r1
    fmult
    to r5
    rol
    lms r6,(!gsu_out_e4-!gsu_ram+14)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!gsu_out_e4-!gsu_ram+16)
    from r3
    fmult
    rol
    add r5
    sms (!gsu_out_2a-!gsu_ram),r0
    stop
    nop

; @routine 01:8325 FxMatrixFromAnglesProgram
; WHAT IT DOES: The Super FX program that builds the rotation matrix for the
; angles at $0020-$0024 into $00E4 (FxMatrixFromAngles01).
; HOW: R9 = $00E4, the call, then STOP.
; Entry: started by the S-CPU. Exit (at the STOP): as FxMatrixFromAngles01,
; R9 = $00F4, R11 = $832D.
%at($018325)
FxMatrixFromAnglesProgram:
    iwt r9,#!gsu_out_e4-!gsu_ram
    link #4
    iwt r15,#FxMatrixFromAngles01
    nop
    stop
    nop

; @routine 01:832F FxMatrixFromAngles01
; WHAT IT DOES: Builds the rotation matrix for the three angles at $0020
; (Z), $0022 (X) and $0024 (Y) and stores its nine words at R9.
; HOW: Sets the RAM and ROM banks to 0. Each sine comes from the table at
; $00:983E: the angle's high byte picks two neighbouring words (GETB,
; GETBH), and the low byte << 7 times their difference (FMULT, ROL) is
; added to the first; each cosine is the sine of the angle + $4000. With
; products (a * b) >> 15 (FMULT, ROL) the rows are: sx sy sz + cx cy,
; sx cy sz - cx sy, sx cz; sy cz, cy cz, -sz; cx sy sz - sx cy,
; cx cy sz + sx sy, cx cz.
; Entry: R9 = where to store; R11 = return address. Exit: R9 = R9 + 16;
; R12, R8 = sin and cos of Z; R4, R3 = of X; R7, R5 = of Y; R1 = sx cy,
; R2 = cx cy, R13 = sx sy, R14 = cx sy; R6 = cos Z; R0 = the last element;
; the RAM and ROM banks 0; the last RAM address the last element's; CY,
; S, Z from its ROL; OV from the ADD R13 of the element before.
; R14 and the ROM buffer move as on the Super FX.
%at($01832F)
FxMatrixFromAngles01:
    sub r0
    ramb
    romb
    lms r1,(!gsu_in_20-!gsu_ram)
    iwt r6,#!fx_sine_table
    from r1
    hib
    add r0
    to r14
    add r6
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    getb
    inc r14
    getbh
    to r6
    sub r2
    from r1
    lob
    swap
    lsr
    fmult
    rol
    to r12
    add r2
    iwt r14,#$4000
    with r1
    add r14
    iwt r6,#!fx_sine_table
    from r1
    hib
    add r0
    to r14
    add r6
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    getb
    inc r14
    getbh
    to r6
    sub r2
    from r1
    lob
    swap
    lsr
    fmult
    rol
    to r8
    add r2
    lms r1,(!gsu_in_22-!gsu_ram)
    iwt r6,#!fx_sine_table
    from r1
    hib
    add r0
    to r14
    add r6
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    getb
    inc r14
    getbh
    to r6
    sub r2
    from r1
    lob
    swap
    lsr
    fmult
    rol
    to r4
    add r2
    iwt r14,#$4000
    with r1
    add r14
    iwt r6,#!fx_sine_table
    from r1
    hib
    add r0
    to r14
    add r6
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    getb
    inc r14
    getbh
    to r6
    sub r2
    from r1
    lob
    swap
    lsr
    fmult
    rol
    to r3
    add r2
    lms r1,(!gsu_in_24-!gsu_ram)
    iwt r6,#!fx_sine_table
    from r1
    hib
    add r0
    to r14
    add r6
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    getb
    inc r14
    getbh
    to r6
    sub r2
    from r1
    lob
    swap
    lsr
    fmult
    rol
    to r7
    add r2
    iwt r14,#$4000
    with r1
    add r14
    iwt r6,#!fx_sine_table
    from r1
    hib
    add r0
    to r14
    add r6
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    getb
    inc r14
    getbh
    to r6
    sub r2
    from r1
    lob
    swap
    lsr
    fmult
    rol
    to r5
    add r2
    move r6,r5
    from r4
    fmult
    to r1
    rol
    from r3
    fmult
    to r2
    rol
    move r6,r7
    from r4
    fmult
    to r13
    rol
    from r3
    fmult
    to r14
    rol
    move r6,r12
    from r13
    fmult
    rol
    add r2
    stw (r9)
    inc r9
    inc r9
    from r1
    fmult
    rol
    sub r14
    stw (r9)
    inc r9
    inc r9
    move r6,r8
    from r4
    fmult
    rol
    stw (r9)
    inc r9
    inc r9
    from r7
    fmult
    rol
    stw (r9)
    inc r9
    inc r9
    from r5
    fmult
    rol
    stw (r9)
    inc r9
    inc r9
    from r12
    not
    inc r0
    stw (r9)
    inc r9
    inc r9
    move r6,r12
    from r14
    fmult
    rol
    sub r1
    stw (r9)
    inc r9
    inc r9
    from r2
    fmult
    rol
    add r13
    stw (r9)
    inc r9
    inc r9
    move r6,r8
    from r3
    fmult
    rol
    stw (r9)
    jmp r11
    nop

; @routine 01:981A FxSegmentCrossZ
; WHAT IT DOES: Finds where the segment from (R1, R2, R3) to (R4, R5, R6)
; crosses z = 0: R0 = 0 when both ends have z >= 0, $FFFF when both are
; negative; otherwise R0 = 1 with the end whose z was negative replaced by
; the crossing point (the other end kept in its registers).
; HOW: The z >= 0 end is saved at $9A/$9C/$9E (SMS) and put back at the
; end (LMS); the ends are brought to the midpoint (ADD, DIV2) toward the
; crossing until the midpoint's z is 0. When the first end is the one
; with z >= 0 the ends are swapped while searching.
; Entry: R1-R6 = the ends; R11 = return address. Exit: as above; R1-R6 as
; the search left them; the last RAM address $9E; S, Z from the last
; DIV2 (or the end tests).
%at($01981A)
FxSegmentCrossZ:
    cache
    moves r3,r3
    bmi .first_negative
    nop
    moves r6,r6
    bmi .second_negative
    nop
    sub r0
    jmp r11
    nop
.first_negative:
    moves r6,r6
    bpl .first_only
    nop
    ibt r0,#$FF
    jmp r11
    nop
.second_negative:
    sms ($9A),r1
    sms ($9C),r2
    sms ($9E),r3
    move r0,r1
    move r1,r4
    move r4,r0
    move r0,r2
    move r2,r5
    move r5,r0
    move r0,r3
    move r3,r6
    move r6,r0
.search_a:
    from r3
    add r6
    div2
    beq .found_a
    nop
    bmi .neg_a
    nop
    move r6,r0
    from r1
    add r4
    to r4
    div2
    from r2
    add r5
    to r5
    div2
    bra .search_a
    nop
.neg_a:
    move r3,r0
    from r1
    add r4
    to r1
    div2
    from r2
    add r5
    to r2
    div2
    bra .search_a
    nop
.found_a:
    move r6,r0
    from r1
    add r4
    to r4
    div2
    from r2
    add r5
    to r5
    div2
    lms r1,($9A)
    lms r2,($9C)
    lms r3,($9E)
    ibt r0,#$01
    jmp r11
    nop
.first_only:
    sms ($9A),r4
    sms ($9C),r5
    sms ($9E),r6
.search_b:
    from r3
    add r6
    div2
    beq .found_b
    nop
    bmi .neg_b
    nop
    move r6,r0
    from r1
    add r4
    to r4
    div2
    from r2
    add r5
    to r5
    div2
    bra .search_b
    nop
.neg_b:
    move r3,r0
    from r1
    add r4
    to r1
    div2
    from r2
    add r5
    to r2
    div2
    bra .search_b
    nop
.found_b:
    move r3,r0
    from r1
    add r4
    to r1
    div2
    from r2
    add r5
    to r2
    div2
    lms r4,($9A)
    lms r5,($9C)
    lms r6,($9E)
    ibt r0,#$01
    jmp r11
    nop

; @routine 01:C4AA FxClipYPlane
; WHAT IT DOES: Clips the segment from (R1, R2, R3) to (R4, R5, R7)
; against the plane z = y * R6 * 8 (FMULT, ADC, two ADDs): R0 = 0 when
; both ends have z at or above it, $FFFF when both are below; otherwise
; R0 = 1 with the end below replaced by the crossing, found by halving
; (at most 12 steps), its attributes halved along with it: the first
; end's at $8A/$8E, the second's at $8C/$90 (the moved end's written
; back there).
; HOW: The end kept is saved at $9A-$9E and put back; R8, R9, R12, R13
; and the return address and R14 are kept on the R10 stack, R10 itself at
; $A0.
; Entry: R1-R7 as above, R6 = the slope; R10 = stack; R11 = return
; address. Exit: as above; R8, R9, R12-R14 restored; S, Z from R10; the
; last RAM address R10.
%at($01C4AA)
FxClipYPlane:
    cache
    from r11
    stw (r10)
    inc r10
    inc r10
    from r14
    stw (r10)
    inc r10
    inc r10
    from r2
    fmult
    adc r0
    add r0
    add r0
    from r3
    sub r0
    bmi .lC4D3
    nop
    from r5
    fmult
    adc r0
    add r0
    add r0
    from r7
    sub r0
    bmi .lC4EE
    nop
    dec r10
    dec r10
    to r14
    ldw (r10)
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC4D3:
    from r5
    fmult
    adc r0
    add r0
    add r0
    from r7
    sub r0
    bmi .lC4E2
    nop
    iwt r15,#.lC5B4
    nop
.lC4E2:
    nop
    dec r10
    dec r10
    to r14
    ldw (r10)
    ibt r0,#$FF
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC4EE:
    sms ($9A),r1
    sms ($9C),r2
    sms ($9E),r3
    from r8
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r12
    stw (r10)
    inc r10
    inc r10
    from r13
    stw (r10)
    inc r10
    inc r10
    sms ($A0),r10
    ibt r10,#$0C
    with r1
    to r0
    with r4
    to r1
    with r0
    to r4
    with r2
    to r0
    with r5
    to r2
    with r0
    to r5
    with r3
    to r0
    with r7
    to r3
    with r0
    to r7
    lms r8,($8C)
    lms r9,($90)
    lms r12,($8A)
    lms r13,($8E)
.lC52A:
    from r2
    add r5
    to r14
    div2
    from r3
    add r7
    to r11
    div2
    from r14
    fmult
    adc r0
    add r0
    add r0
    from r11
    sub r0
    beq .lC574
    nop
    bmi .lC55B
    dec r10
    beq .lC574
    nop
    with r11
    to r7
    from r1
    add r4
    to r4
    div2
    with r14
    to r5
    from r8
    add r12
    to r12
    div2
    from r9
    add r13
    to r13
    div2
    bra .lC52A
    nop
.lC55B:
    beq .lC574
    nop
    with r11
    to r3
    from r1
    add r4
    to r1
    div2
    with r14
    to r2
    from r8
    add r12
    to r8
    div2
    from r9
    add r13
    to r9
    div2
    bra .lC52A
    nop
.lC574:
    with r11
    to r7
    from r1
    add r4
    to r4
    div2
    with r14
    to r5
    from r8
    add r12
    to r12
    div2
    from r9
    add r13
    to r13
    div2
    lms r1,($9A)
    lms r2,($9C)
    lms r3,($9E)
    sms ($8C),r12
    sms ($90),r13
    lms r10,($A0)
    dec r10
    dec r10
    to r13
    ldw (r10)
    dec r10
    dec r10
    to r12
    ldw (r10)
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r14
    ldw (r10)
    ibt r0,#$01
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC5B4:
    sms ($9A),r4
    sms ($9C),r5
    sms ($9E),r7
    from r8
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r12
    stw (r10)
    inc r10
    inc r10
    from r13
    stw (r10)
    inc r10
    inc r10
    sms ($A0),r10
    ibt r10,#$0C
    lms r8,($8A)
    lms r9,($8E)
    lms r12,($8C)
    lms r13,($90)
.lC5DE:
    from r2
    add r5
    to r14
    div2
    from r3
    add r7
    to r11
    div2
    from r14
    fmult
    adc r0
    add r0
    add r0
    from r11
    sub r0
    beq .lC628
    nop
    bmi .lC60F
    dec r10
    beq .lC628
    nop
    with r11
    to r7
    from r1
    add r4
    to r4
    div2
    with r14
    to r5
    from r8
    add r12
    to r12
    div2
    from r9
    add r13
    to r13
    div2
    bra .lC5DE
    nop
.lC60F:
    beq .lC628
    nop
    with r11
    to r3
    from r1
    add r4
    to r1
    div2
    with r14
    to r2
    from r8
    add r12
    to r8
    div2
    from r9
    add r13
    to r9
    div2
    bra .lC5DE
    nop
.lC628:
    with r11
    to r3
    from r1
    add r4
    to r1
    div2
    with r14
    to r2
    from r8
    add r12
    to r8
    div2
    from r9
    add r13
    to r9
    div2
    lms r4,($9A)
    lms r5,($9C)
    lms r7,($9E)
    sms ($8A),r8
    sms ($8E),r9
    lms r10,($A0)
    dec r10
    dec r10
    to r13
    ldw (r10)
    dec r10
    dec r10
    to r12
    ldw (r10)
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r14
    ldw (r10)
    ibt r0,#$01
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:C77A FxClipXPlane
; WHAT IT DOES: As FxClipYPlane against the plane z = x * R6 * 4 (FMULT,
; ADC, one ADD), halving on x.
; HOW: As FxClipYPlane.
; Entry: as FxClipYPlane. Exit: as FxClipYPlane.
%at($01C77A)
FxClipXPlane:
    cache
    from r11
    stw (r10)
    inc r10
    inc r10
    from r14
    stw (r10)
    inc r10
    inc r10
    from r1
    fmult
    adc r0
    add r0
    from r3
    sub r0
    bmi .lC7A1
    nop
    from r4
    fmult
    adc r0
    add r0
    from r7
    sub r0
    bmi .lC7BB
    nop
    dec r10
    dec r10
    to r14
    ldw (r10)
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC7A1:
    from r4
    fmult
    adc r0
    add r0
    from r7
    sub r0
    bmi .lC7AF
    nop
    iwt r15,#.lC880
    nop
.lC7AF:
    nop
    dec r10
    dec r10
    to r14
    ldw (r10)
    ibt r0,#$FF
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC7BB:
    sms ($9A),r1
    sms ($9C),r2
    sms ($9E),r3
    from r8
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r12
    stw (r10)
    inc r10
    inc r10
    from r13
    stw (r10)
    inc r10
    inc r10
    sms ($A0),r10
    ibt r10,#$0C
    with r1
    to r0
    with r4
    to r1
    with r0
    to r4
    with r2
    to r0
    with r5
    to r2
    with r0
    to r5
    with r3
    to r0
    with r7
    to r3
    with r0
    to r7
    lms r8,($8C)
    lms r9,($90)
    lms r12,($8A)
    lms r13,($8E)
.lC7F7:
    from r1
    add r4
    to r14
    div2
    from r3
    add r7
    to r11
    div2
    from r14
    fmult
    adc r0
    add r0
    from r11
    sub r0
    beq .lC840
    nop
    bmi .lC827
    dec r10
    beq .lC840
    nop
    with r11
    to r7
    from r2
    add r5
    to r5
    div2
    with r14
    to r4
    from r8
    add r12
    to r12
    div2
    from r9
    add r13
    to r13
    div2
    bra .lC7F7
    nop
.lC827:
    beq .lC840
    nop
    with r11
    to r3
    from r2
    add r5
    to r2
    div2
    with r14
    to r1
    from r8
    add r12
    to r8
    div2
    from r9
    add r13
    to r9
    div2
    bra .lC7F7
    nop
.lC840:
    with r11
    to r7
    from r2
    add r5
    to r5
    div2
    with r14
    to r4
    from r8
    add r12
    to r12
    div2
    from r9
    add r13
    to r13
    div2
    lms r1,($9A)
    lms r2,($9C)
    lms r3,($9E)
    sms ($8C),r12
    sms ($90),r13
    lms r10,($A0)
    dec r10
    dec r10
    to r13
    ldw (r10)
    dec r10
    dec r10
    to r12
    ldw (r10)
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r14
    ldw (r10)
    ibt r0,#$01
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC880:
    sms ($9A),r4
    sms ($9C),r5
    sms ($9E),r7
    from r8
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r12
    stw (r10)
    inc r10
    inc r10
    from r13
    stw (r10)
    inc r10
    inc r10
    sms ($A0),r10
    ibt r10,#$0C
    lms r8,($8A)
    lms r9,($8E)
    lms r12,($8C)
    lms r13,($90)
.lC8AA:
    from r1
    add r4
    to r14
    div2
    from r3
    add r7
    to r11
    div2
    from r14
    fmult
    adc r0
    add r0
    from r11
    sub r0
    beq .lC8F3
    nop
    bmi .lC8DA
    dec r10
    beq .lC8F3
    nop
    with r11
    to r7
    from r2
    add r5
    to r5
    div2
    with r14
    to r4
    from r8
    add r12
    to r12
    div2
    from r9
    add r13
    to r13
    div2
    bra .lC8AA
    nop
.lC8DA:
    beq .lC8F3
    nop
    with r11
    to r3
    from r2
    add r5
    to r2
    div2
    with r14
    to r1
    from r8
    add r12
    to r8
    div2
    from r9
    add r13
    to r9
    div2
    bra .lC8AA
    nop
.lC8F3:
    with r11
    to r3
    from r2
    add r5
    to r2
    div2
    with r14
    to r1
    from r8
    add r12
    to r8
    div2
    from r9
    add r13
    to r9
    div2
    lms r4,($9A)
    lms r5,($9C)
    lms r7,($9E)
    sms ($8A),r8
    sms ($8E),r9
    lms r10,($A0)
    dec r10
    dec r10
    to r13
    ldw (r10)
    dec r10
    dec r10
    to r12
    ldw (r10)
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r14
    ldw (r10)
    ibt r0,#$01
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:C668 FxPolygonClipTop
; WHAT IT DOES: Clips the polygon at $60 ($40 vertices of five words:
; x, y, z and two attributes) against the plane z = y * slope ($94) * 8
; into the list at $62 (FxClipYPlane for each crossing edge): R0 = the
; new vertex count ($40), or 0 when fewer than 3 are left or there were 2.
; HOW: Each edge from a vertex to the next (the last to the first); a
; vertex inside is kept, a crossing adds the clipped point. R14 and the
; return address on the R10 stack.
; Entry: R10 = stack; R11 = return address. Exit: R0 as above; R9 past
; the output; R14 restored; S, Z from R10; the last RAM address R10.
%at($01C668)
FxPolygonClipTop:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r14
    stw (r10)
    inc r10
    inc r10
    lms r6,($94)
    lms r14,($40)
    sub r0
    sms ($40),r0
    lms r8,($60)
    lms r9,($62)
    from r14
    sub #2
    bne .lC68A
    nop
    iwt r15,#.lC770
    nop
.lC68A:
    nop
.lC68B:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8A),r0
    ldw (r8)
    inc r8
    inc r8
    sms ($8E),r0
    with r14
    to r0
    dec r0
    bne .lC6AC
    nop
    lms r8,($60)
.lC6AC:
    to r4
    ldw (r8)
    inc r8
    inc r8
    to r5
    ldw (r8)
    inc r8
    inc r8
    to r7
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8C),r0
    ldw (r8)
    sms ($90),r0
    with r8
    sub #8
    from r2
    fmult
    adc r0
    add r0
    add r0
    from r3
    sub r0
    bpl .lC6E2
    nop
    from r5
    fmult
    adc r0
    add r0
    add r0
    from r7
    sub r0
    bpl .lC6DF
    nop
    iwt r15,#.lC752
    nop
.lC6DF:
    bra .lC730
    nop
.lC6E2:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    from r5
    fmult
    adc r0
    add r0
    add r0
    from r7
    sub r0
    bpl .lC752
    nop
    link #4
    iwt r15,#FxClipYPlane
    nop
    from r4
    stw (r9)
    inc r9
    inc r9
    from r5
    stw (r9)
    inc r9
    inc r9
    from r7
    stw (r9)
    inc r9
    inc r9
    lms r0,($8C)
    stw (r9)
    inc r9
    inc r9
    lms r0,($90)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    iwt r15,#.lC752
    nop
.lC730:
    link #4
    iwt r15,#FxClipYPlane
    nop
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
.lC752:
    dec r14
    beq .lC75A
    nop
    iwt r15,#.lC68B
    nop
.lC75A:
    lms r0,($40)
    to r1
    sub #3
    bpl .lC767
    nop
    iwt r15,#.lC770
    nop
.lC767:
    dec r10
    dec r10
    to r14
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC770:
    dec r10
    dec r10
    to r14
    ldw (r10)
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:C933 FxPolygonClipRight
; WHAT IT DOES: As FxPolygonClipTop against z = x * slope ($92) * 4
; (FxClipXPlane).
; HOW: As FxPolygonClipTop.
; Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
%at($01C933)
FxPolygonClipRight:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r14
    stw (r10)
    inc r10
    inc r10
    lms r6,($92)
    lms r14,($40)
    sub r0
    sms ($40),r0
    lms r8,($60)
    lms r9,($62)
    from r14
    sub #2
    bne .lC955
    nop
    iwt r15,#.lCA38
    nop
.lC955:
    nop
.lC956:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8A),r0
    ldw (r8)
    inc r8
    inc r8
    sms ($8E),r0
    with r14
    to r0
    dec r0
    bne .lC977
    nop
    lms r8,($60)
.lC977:
    to r4
    ldw (r8)
    inc r8
    inc r8
    to r5
    ldw (r8)
    inc r8
    inc r8
    to r7
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8C),r0
    ldw (r8)
    sms ($90),r0
    with r8
    sub #8
    from r1
    fmult
    adc r0
    add r0
    from r3
    sub r0
    bpl .lC9AB
    nop
    from r4
    fmult
    adc r0
    add r0
    from r7
    sub r0
    bpl .lC9A8
    nop
    iwt r15,#.lCA1A
    nop
.lC9A8:
    bra .lC9F8
    nop
.lC9AB:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    from r4
    fmult
    adc r0
    add r0
    from r7
    sub r0
    bpl .lCA1A
    nop
    link #4
    iwt r15,#FxClipXPlane
    nop
    from r4
    stw (r9)
    inc r9
    inc r9
    from r5
    stw (r9)
    inc r9
    inc r9
    from r7
    stw (r9)
    inc r9
    inc r9
    lms r0,($8C)
    stw (r9)
    inc r9
    inc r9
    lms r0,($90)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    iwt r15,#.lCA1A
    nop
.lC9F8:
    link #4
    iwt r15,#FxClipXPlane
    nop
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
.lCA1A:
    dec r14
    beq .lCA22
    nop
    iwt r15,#.lC956
    nop
.lCA22:
    lms r0,($40)
    to r1
    sub #3
    bpl .lCA2F
    nop
    iwt r15,#.lCA38
    nop
.lCA2F:
    dec r10
    dec r10
    to r14
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lCA38:
    dec r10
    dec r10
    to r14
    ldw (r10)
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:CA42 FxPolygonClipLeft
; WHAT IT DOES: As FxPolygonClipRight against z = -x * slope * 4: x is
; negated around each FxClipXPlane (the clipped point's back afterwards).
; HOW: As FxPolygonClipTop, testing z + the scaled x.
; Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
%at($01CA42)
FxPolygonClipLeft:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r14
    stw (r10)
    inc r10
    inc r10
    lms r6,($92)
    lms r14,($40)
    sub r0
    sms ($40),r0
    lms r8,($60)
    lms r9,($62)
    from r14
    sub #2
    bne .lCA64
    nop
    iwt r15,#.lCB59
    nop
.lCA64:
    nop
.lCA65:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8A),r0
    ldw (r8)
    inc r8
    inc r8
    sms ($8E),r0
    with r14
    to r0
    dec r0
    bne .lCA86
    nop
    lms r8,($60)
.lCA86:
    to r4
    ldw (r8)
    inc r8
    inc r8
    to r5
    ldw (r8)
    inc r8
    inc r8
    to r7
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8C),r0
    ldw (r8)
    sms ($90),r0
    with r8
    sub #8
    from r1
    fmult
    adc r0
    add r0
    from r3
    add r0
    bpl .lCABA
    nop
    from r4
    fmult
    adc r0
    add r0
    from r7
    add r0
    bpl .lCAB7
    nop
    iwt r15,#.lCB3B
    nop
.lCAB7:
    bra .lCB10
    nop
.lCABA:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    from r4
    fmult
    adc r0
    add r0
    from r7
    add r0
    bpl .lCB3B
    nop
    with r1
    not
    inc r1
    with r4
    not
    inc r4
    link #4
    iwt r15,#FxClipXPlane
    nop
    with r4
    not
    inc r4
    from r4
    stw (r9)
    inc r9
    inc r9
    from r5
    stw (r9)
    inc r9
    inc r9
    from r7
    stw (r9)
    inc r9
    inc r9
    lms r0,($8C)
    stw (r9)
    inc r9
    inc r9
    lms r0,($90)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    iwt r15,#.lCB3B
    nop
.lCB10:
    with r1
    not
    inc r1
    with r4
    not
    inc r4
    link #4
    iwt r15,#FxClipXPlane
    nop
    with r1
    not
    inc r1
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
.lCB3B:
    dec r14
    beq .lCB43
    nop
    iwt r15,#.lCA65
    nop
.lCB43:
    lms r0,($40)
    to r1
    sub #3
    bpl .lCB50
    nop
    iwt r15,#.lCB59
    nop
.lCB50:
    dec r10
    dec r10
    to r14
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lCB59:
    dec r10
    dec r10
    to r14
    ldw (r10)
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:CB63 FxPolygonClipBottom
; WHAT IT DOES: As FxPolygonClipTop against z = -y * slope * 8: y is
; negated around each FxClipYPlane (the clipped point's back afterwards).
; HOW: As FxPolygonClipTop, testing z + the scaled y.
; Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
%at($01CB63)
FxPolygonClipBottom:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r14
    stw (r10)
    inc r10
    inc r10
    lms r6,($94)
    lms r14,($40)
    sub r0
    sms ($40),r0
    lms r8,($60)
    lms r9,($62)
    from r14
    sub #2
    bne .lCB85
    nop
    iwt r15,#.lCC7D
    nop
.lCB85:
    nop
.lCB86:
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8A),r0
    ldw (r8)
    inc r8
    inc r8
    sms ($8E),r0
    with r14
    to r0
    dec r0
    bne .lCBA7
    nop
    lms r8,($60)
.lCBA7:
    to r4
    ldw (r8)
    inc r8
    inc r8
    to r5
    ldw (r8)
    inc r8
    inc r8
    to r7
    ldw (r8)
    inc r8
    inc r8
    ldw (r8)
    inc r8
    inc r8
    sms ($8C),r0
    ldw (r8)
    sms ($90),r0
    with r8
    sub #8
    from r2
    fmult
    adc r0
    add r0
    add r0
    from r3
    add r0
    bpl .lCBDD
    nop
    from r5
    fmult
    adc r0
    add r0
    add r0
    from r7
    add r0
    bpl .lCBDA
    nop
    iwt r15,#.lCC5F
    nop
.lCBDA:
    bra .lCC34
    nop
.lCBDD:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    from r5
    fmult
    adc r0
    add r0
    add r0
    from r7
    add r0
    bpl .lCC5F
    nop
    with r2
    not
    inc r2
    with r5
    not
    inc r5
    link #4
    iwt r15,#FxClipYPlane
    nop
    with r5
    not
    inc r5
    from r4
    stw (r9)
    inc r9
    inc r9
    from r5
    stw (r9)
    inc r9
    inc r9
    from r7
    stw (r9)
    inc r9
    inc r9
    lms r0,($8C)
    stw (r9)
    inc r9
    inc r9
    lms r0,($90)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
    iwt r15,#.lCC5F
    nop
.lCC34:
    with r2
    not
    inc r2
    with r5
    not
    inc r5
    link #4
    iwt r15,#FxClipYPlane
    nop
    with r2
    not
    inc r2
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    from r3
    stw (r9)
    inc r9
    inc r9
    lms r0,($8A)
    stw (r9)
    inc r9
    inc r9
    lms r0,($8E)
    stw (r9)
    inc r9
    inc r9
    lms r0,($40)
    inc r0
    sbk
.lCC5F:
    dec r14
    beq .lCC67
    nop
    iwt r15,#.lCB86
    nop
.lCC67:
    lms r0,($40)
    to r1
    sub #3
    bpl .lCC74
    nop
    iwt r15,#.lCC7D
    nop
.lCC74:
    dec r10
    dec r10
    to r14
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lCC7D:
    dec r10
    dec r10
    to r14
    ldw (r10)
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
