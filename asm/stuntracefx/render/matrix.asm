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
%at($01978B)
FxMatrixApply:
    cache
    lms r6,(!fx_matrix)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix+6)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix+12)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    lms r6,(!fx_matrix+2)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix+8)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix+14)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    lms r6,(!fx_matrix+4)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix+10)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix+16)
    from r3
    fmult
    rol
    add r5
    stw (r9)
    inc r9
    inc r9
    jmp r11
    nop

arch 65816
