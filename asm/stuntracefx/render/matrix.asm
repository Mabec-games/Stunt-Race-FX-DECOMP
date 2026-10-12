; matrix.asm: 3x3 matrices of signed words in Super FX RAM, nine elements
; row by row, in 1.15 fixed point (FMULT keeps the high word of the
; product and ROL doubles it, so each product is (a * b) >> 15). The
; current one is at $00E4: rows at $E4, $EA, $F0.
; C: src/stuntracefx/render/matrix.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

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

arch 65816
