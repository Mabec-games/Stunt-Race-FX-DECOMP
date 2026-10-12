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

; @routine 01:832F FxMatrixFromAngles
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
FxMatrixFromAngles:
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

arch 65816
