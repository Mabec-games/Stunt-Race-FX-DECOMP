; fx_math.asm: the Super FX's 32-bit divide and square root.
;
; Both run sixteen rounds under LOOP (R12 the count, R13 the round's start)
; and keep a 32-bit value in two registers, high word first.
; C: src/stuntracefx/math/fx_math.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:C087 FxNormalize
; WHAT IT DOES: Scales the vector (R1, R2, R3) to length $7FFF (1.0 in 1.15
; fixed point); a zero vector stays zero.
; HOW: The squared length as 32 bits (three LMULTs summed with ADD and
; ADC), FxSquareRoot of it, then k = the root's top bit number (LSR until
; zero); FxDivideHalvedIfLarge divides $7FFF << k by the root, each
; component is LMULTed by the quotient and the 32-bit products shifted
; right by k (ASR, ROR under LOOP). The return address is kept on the R10
; stack around the two calls.
; Entry: R1, R2, R3 = the vector; R10 = stack; R11 = return address.
; Exit: R1, R2, R3 = the scaled vector (low words of the products); R7, R8,
; R0 = their high words; R6 = the quotient; R9 = k; R4 = R3; R5 = 0; R11 =
; $C0E3; R12 = 0; R13 = $C0FD ($C14B when k = 0); the last RAM address R10;
; the flags from the last LOOP and ROR. A zero vector: R0-R6 = 0; R11
; unchanged; Z set, S, OV clear.
%at($04C087)
FxNormalize:
    from r11
    stw (r10)
    inc r10
    inc r10
    move r6,r1
    from r1
    lmult
    move r5,r4
    move r6,r2
    from r2
    to r6
    lmult
    with r5
    add r4
    adc r6
    move r6,r3
    from r3
    to r6
    lmult
    with r5
    add r4
    adc r6
    moves r0,r0
    bne .nonzero
    nop
    moves r5,r5
    bne .nonzero
    nop
    ibt r1,#0
    ibt r2,#0
    ibt r3,#0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.nonzero:
    move r4,r0
    link #4
    iwt r15,#FxSquareRoot
    nop
    move r0,r6
    ibt r9,#0
.bits:
    lsr
    beq .counted
    nop
    bra .bits
    inc r9
.counted:
    sub r0
    iwt r4,#$7FFF
    moves r12,r9
    beq .divide
    nop
    move r13,r15
    with r4
    add r4
    loop
    rol
.divide:
    move r6,r6
    link #4
    iwt r15,#FxDivideHalvedIfLarge
    nop
    move r6,r4
    from r1
    to r7
    lmult
    move r1,r4
    from r2
    to r8
    lmult
    move r2,r4
    from r3
    lmult
    move r3,r4
    moves r12,r9
    beq .done
    nop
    move r13,r15
    with r7
    asr
    with r1
    ror
    with r8
    asr
    with r2
    ror
    asr
    with r3
    ror
    loop
    nop
.done:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:C10F FxSquareRoot
; WHAT IT DOES: The integer square root of the 32-bit R4:R5 to R6, the
; remainder (value - root squared) to R8.
; HOW: Digit by digit: each round shifts two bits of R4:R5 into R8 (ADD R5
; to itself, ROL R4, ROL R8, twice), doubles the root, and when R8 exceeds
; twice the root, subtracts twice the root plus one (SBC with the borrow
; the compare left) and sets the root's new low bit. LOOP's delay slot
; holds the WITH R5 for the next round's first ADD.
; Entry: R4 = high word, R5 = low word; R11 = return address. Exit: R6 =
; root; R8 = remainder; R4 = R5 = 0; R7 = the last round's twice the root;
; R0 = R7 - R8 from the last round's compare; R12 = 0; R13 = $C119; Z set,
; S clear; CY, OV from the last compare or subtract.
%at($04C10F)
FxSquareRoot:
    ibt r6,#0
    ibt r8,#0
    ibt r12,#16
    iwt r13,#.round
    with r5
.round:
    add r5
    with r4
    rol
    with r8
    rol
    with r5
    add r5
    with r4
    rol
    with r8
    rol
    with r6
    add r6
    from r6
    to r7
    add r6
    from r7
    sub r8
    bcs .next
    with r8
    sbc r7
    inc r6
.next:
    loop
    with r5
    jmp r11
    nop

; @routine 04:C135 FxDivideHalvedIfLarge
; WHAT IT DOES: FxDivide, except that when R0 or R6 has bit 15 set the
; divisor and the 32-bit dividend are first halved, so the quotient is
; kept in range.
; HOW: MOVES R0 and R6 for their signs; the halving is LSR R6, LSR R0 and
; ROR R4, then the code runs on into FxDivide. The halving path is decoded
; from the ROM bytes and not reached in the traces so far.
; Entry: as FxDivide. Exit: as FxDivide; R6 = the divisor used.
%at($04C135)
FxDivideHalvedIfLarge:
    moves r0,r0
    bmi .halve
    nop
    moves r6,r6
    bpl FxDivide
    nop
.halve:
    with r6
    lsr
    lsr
    with r4
    ror

; @routine 04:C144 FxDivide
; WHAT IT DOES: Divides the 32-bit R0:R4 by R6: the quotient to R4, the
; remainder to R0. The quotient must fit in 16 bits (R0 < R6).
; HOW: Restoring division, one quotient bit a round: the pair is shifted
; left (ADD R4 to itself, ROL R0), R6 is subtracted from R0 and, on a
; borrow, added back. The shift into R0 runs one bit ahead (the ROL in
; LOOP's delay slot) and the closing LSR takes it back, so bit 15 of a
; remainder above $7FFF is lost.
; Entry: R0 = high word, R4 = low word, R6 = divisor; R11 = return address.
; Exit: R4 = quotient; R0 = remainder; R12 = 0; R13 = $C14B; S clear, Z
; from R0, CY = the bit shifted out; OV from the last round's SUB or ADD.
%at($04C144)
FxDivide:
    ibt r12,#16
    with r4
    add r4
    rol
    move r13,r15
    sub r6
    bcc .restore
    nop
    with r4
    rol
    loop
    rol
    jmp r11
    lsr
.restore:
    add r6
    with r4
    add r4
    loop
    rol
    jmp r11
    lsr

arch 65816
