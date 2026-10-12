; fx_rotate.asm: the Super FX's rotations: a vector from an angle, a turn
; about the vertical, and a 3x3 matrix transposed.
;
; Angles are 0-255 for a full turn; cosine and sine are signed bytes
; (64 = 1/2) in the tables at $00:973E and $00:96FE, the cosine table being
; the sine table 64 entries on. The Super FX reads them through the ROM
; buffer with ROMBR = 0. Each byte is SWAPped into the high byte, so a
; product's high word is the value times the byte / 256.
; C: src/stuntracefx/math/fx_rotate.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch superfx

; @routine 04:C15C FxTurn
; WHAT IT DOES: Turns the vector (R1, R3) by the angle R2 (0-255): R2 = R1 =
; (sin * x + cos * z) / 2^17 and R3 = (cos * x - sin * z) / 2^17 with the
; table bytes as signed values times 256.
; HOW: The return address is pushed on the R10 stack and pulled into R15 at
; the end. Each component is two LMULTs summed as 32-bit (ADD then ADC of
; the low and high words) and the high word halved with ASR. The second
; component's low words are subtracted the other way round (sin * z minus
; cos * x) from its high words (cos * x minus sin * z); only the borrow of
; that low subtraction reaches the result.
; Entry: R1 = x, R2 = angle, R3 = z; R10 = stack; R11 = return address.
; Exit: R1 = R2 = the new x; R3 = the new z; R0 = the second high word; R4 =
; low word of cos * x; R5 = cos * 256; R6 = the old x; R7 = the second low
; word; R8 = high word of sin * z; R9 = sin * 256; R14 = $96FE + angle; the
; last RAM address R10; S, Z from R10; CY from the last ASR; OV from SBC.
%at($04C15C)
FxTurn:
    from r11
    stw (r10)
    inc r10
    inc r10
    iwt r0,#!rot_cos_table
    to r14
    add r2
    getb
    to r5
    swap
    iwt r0,#!rot_sin_table
    to r14
    add r2
    getb
    to r9
    swap
    move r6,r1
    from r9
    to r8
    lmult
    move r7,r4
    move r6,r3
    from r5
    lmult
    with r7
    add r4
    adc r8
    to r2
    asr
    move r6,r3
    from r9
    to r8
    lmult
    move r7,r4
    move r6,r1
    from r5
    lmult
    with r7
    sub r4
    sbc r8
    to r3
    asr
    move r1,r2
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; The sine of the angle in R1 into <dest>: the word table at $00:983E entry
; angle / 256, plus the step to the next entry times the low byte / 256.
macro fx_sine16(dest)
    iwt r6,#!fx_sine16_table
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
    to <dest>
    add r2
endmacro

; The sine into <s>, then the cosine (the angle a quarter turn on) into <c>.
macro fx_sine_cosine(s, c)
    %fx_sine16(<s>)
    iwt r14,#$4000
    with r1
    add r14
    %fx_sine16(<c>)
endmacro

; @routine 04:C1A7 FxMatrixFromAngles
; WHAT IT DOES: Builds the 3x3 rotation matrix of the three angles at $0020
; ($0022, $0024) into the nine words at R9: with s, c the sines and
; cosines of angles 0, 1, 2, the words are s0s1s2 + c1c2, s0s1c2 - c1s2,
; c0s1, c0s2, c0c2, -s0, s0c1s2 - s1c2, s0c1c2 + s1s2, c0c1.
; HOW: RAMB and ROMB 0. Each sine is read from the word table at $00:983E
; with linear interpolation (sine16); cosines are the sine a quarter turn
; on. Products are FMULT then ROL (1.15 fixed point), the shared pairs
; worked out first; the words are stored with STW in order.
; Entry: R9 = where the matrix goes; R11 = return address. Exit: R9 =
; matrix + 16; R0 = the last word; R1 = s1c2, R2 = c1c2, R3 = c1, R4 = s1,
; R5 = c2, R6 = R8 = c0, R7 = s2, R12 = s0, R13 = s1s2, R14 = c1s2; ROMBR =
; RAMBR = 0; S, CY, Z from the last ROL, OV from the add of the eighth
; word; the last RAM address matrix + 16.
%at($04C1A7)
FxMatrixFromAngles:
    sub r0
    ramb
    romb
    lms r1,(!fx_angles_w)
    %fx_sine_cosine(r12, r8)
    lms r1,(!fx_angles_w+2)
    %fx_sine_cosine(r4, r3)
    lms r1,(!fx_angles_w+4)
    %fx_sine_cosine(r7, r5)
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

; @routine 04:C2C8 FxMatrixTranspose
; WHAT IT DOES: Transposes the 3x3 matrix of words at R9 in place.
; HOW: Three swaps of the off-diagonal pairs (words 1 and 3, 2 and 6, 5 and
; 7), walking R9 with INC and ADD/SUB of small constants: loads and stores
; in the order 1, 2, 3, store 3, store 1, load 5, 6, store 6, load 7, store
; 7, store 5, store 2.
; Entry: R9 = the matrix; R11 = return address. Exit: R9 = matrix + 4; R1 =
; the old word 5, R2 = the old word 7, R3 = the old word 6; the last RAM
; address matrix + 4; OV, S, CY, Z from the SUB that leaves R9.
%at($04C2C8)
FxMatrixTranspose:
    inc r9
    inc r9
    to r1
    ldw (r9)
    inc r9
    inc r9
    to r2
    ldw (r9)
    inc r9
    inc r9
    to r3
    ldw (r9)
    from r1
    stw (r9)
    with r9
    sub #4
    from r3
    stw (r9)
    with r9
    add #8
    to r1
    ldw (r9)
    inc r9
    inc r9
    to r3
    ldw (r9)
    from r2
    stw (r9)
    inc r9
    inc r9
    to r2
    ldw (r9)
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

; @routine 04:C2F8 FxAngleOf
; WHAT IT DOES: The angle of the vector (R1, R2), 0-$FFFF for a full turn,
; to R0: $4000 for R2 = 0 (before the signs below), $2000 when |R1| =
; |R2|, otherwise from the arctangent table at $00:9E40 indexed by the
; smaller over the larger (times $4000, / 64); the result is negated when
; R1 and R2 differ in sign, and $8000 is added when R2 is negative.
; HOW: |R1| to R6 and |R2| to R0 with NOT and INC; the smaller (BLT) is
; shifted into R0:R4 as times $4000 (LSR, ROR twice) and FxDivide gives the
; ratio; the table word (GETB, GETBH, ROMBR 0) is the angle when |R2| is
; the larger, else $4000 minus it. R11 and R14 are kept on the R10 stack.
; Entry: R1, R2 = the vector; R10 = stack; R11 = return address. Exit: R0 =
; the angle; R4 = R1 xor R2, or $8000 when R2 is negative; after the
; divide: R5 = -1 when |R2| is the larger, else 0; R6 = the larger; R11 =
; $C33E; R12 = 0; R13 = $C14B; R14 kept; S, Z from R10; the last RAM
; address R10.
%at($04C2F8)
FxAngleOf:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r14
    stw (r10)
    inc r10
    inc r10
    moves r6,r1
    bpl .x_positive
    nop
    with r6
    not
    inc r6
.x_positive:
    moves r0,r2
    bpl .y_positive
    nop
    not
    inc r0
.y_positive:
    bne .y_nonzero
    nop
    iwt r0,#$4000
    iwt r15,#.signs
    nop
.y_nonzero:
    ibt r5,#0
    cmp r6
    bne .unequal
    nop
    iwt r0,#$2000
    iwt r15,#.signs
    nop
.unequal:
    blt .smaller
    nop
    move r4,r6
    move r6,r0
    move r0,r4
    dec r5
.smaller:
    ibt r4,#0
    lsr
    with r4
    ror
    lsr
    with r4
    ror
    link #4
    iwt r15,#FxDivide
    nop
    sub r0
    romb
    from r4
    lsr
    lsr
    lsr
    lsr
    lsr
    bic #1
    iwt r4,#!fx_atan_table
    to r14
    add r4
    getb
    inc r14
    getbh
    moves r5,r5
    bmi .signs
    nop
    not
    inc r0
    iwt r4,#$4000
    add r4
.signs:
    from r1
    to r4
    xor r2
    bpl .same_sign
    nop
    not
    inc r0
.same_sign:
    moves r2,r2
    bpl .done
    nop
    iwt r4,#$8000
    add r4
.done:
    dec r10
    dec r10
    to r14
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:CD98 FxAngleVector
; WHAT IT DOES: The vector of length R4 at the angle in R0's high byte:
; R1 = -sin * length, R3 = cos * length, both scaled by 16 / 128 (a byte
; table value of 64 gives length * 8), and R2 = 0.
; HOW: HIB for the angle, ROMB 0, then for each table the byte SWAPped up,
; FMULT by R6 = length, (NOT and INC for the sine), and four ADDs to
; itself.
; Entry: R0 high byte = angle; R4 = length; R11 = return address. Exit: R1,
; R3 as above, R0 = R3; R2 = 0; R6 = length; R14 = $973E + angle; ROMBR =
; 0; OV, S, CY, Z from the last ADD.
%at($04CD98)
FxAngleVector:
    hib
    move r3,r0
    ibt r0,#0
    romb
    move r6,r4
    move r0,r3
    lob
    iwt r14,#!rot_sin_table
    to r14
    add r14
    getb
    swap
    fmult
    not
    inc r0
    add r0
    add r0
    add r0
    add r0
    move r1,r0
    move r0,r3
    lob
    iwt r14,#!rot_cos_table
    to r14
    add r14
    getb
    swap
    fmult
    add r0
    add r0
    add r0
    add r0
    move r3,r0
    ibt r2,#0
    move r15,r11
    nop

arch 65816
