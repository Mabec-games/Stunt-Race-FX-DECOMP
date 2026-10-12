; arith.asm: the Super FX's long division and square root, done a bit at a
; time in LOOPs (the GSU-1 has no divide).
;
; The three dividers share one loop, each with its own copy of it: the
; dividend is shifted left one bit first (its low word into the high
; word), then 16 passes of restoring division: subtract the divisor from
; the high word, and if that borrows add it back and shift a 0 into the
; low word, else shift a 1 in; the low word's top bit shifts into the high
; word. The low word ends as the quotient and the high word as the
; remainder shifted left one, with the bit shifted out of it dropped each
; pass, so the remainder is exact only while it fits in 15 bits.
; C: src/stuntracefx/render/arith.c.

incsrc "../include/macros.inc"

arch superfx

; @routine 01:8096 FxSqrt32
; WHAT IT DOES: The integer square root of the 32-bit value R4:R5 (R4 the
; high word): R6 = the root, R8 = the remainder.
; HOW: 16 passes, each shifting the next two bits of R4:R5 into R8 and
; doubling R6; when R8 exceeds twice R6 (R7), R8 loses twice R6 plus 1 and
; R6 gains 1. The pass loop starts at $80A0, the ADD R5 after the first
; WITH R5: LOOP's delay slot is the WITH R5 for the next pass, and the
; JMP R11 after the last pass runs with it pending.
; Entry: R4:R5 = value; R11 = return address. Exit: R6 = root, R8 =
; remainder; R4 = R5 = 0; R7 = twice the root before its last bit; R0 =
; R7 - R8 as the last pass compared them; R12 = 0; R13 = $80A0; Z set, S
; clear; CY and OV from the last pass's SUB R8, or from its SBC R7 when the
; root gained a bit.
%at($018096)
FxSqrt32:
    ibt r6,#$00
    ibt r8,#$00
    ibt r12,#$10
    iwt r13,#.pass&$FFFF
    with r5
.pass:
    add r5                      ; R5 <<= 1 (a WITH R5 is pending here)
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

; @routine 01:8186 FxDivideSigned
; WHAT IT DOES: Divides the 32-bit value R5:R4 (R5 the high word) by R6,
; signed: R4 = the quotient, R0 = the remainder of the magnitudes, shifted
; left one.
; HOW: CACHE; R7 = R5 XOR R6 keeps the quotient's sign; R6 and R5:R4 are
; made positive (SUB from R0 = 0, the 32-bit one as SUB then SBC); the
; shared loop; then R4 is negated (NOT, INC) when R7 is negative. There is
; no closing LSR: the remainder is left shifted.
; Entry: R5:R4 = dividend, R6 = divisor; R11 = return address. Exit: R4 =
; quotient, R0 = remainder * 2 plus the last bit shifted in; R5 = the
; dividend's high word made positive, R6 = the divisor made positive, R7 =
; the sign word; R12 = 0; R13 = $81A3 (the loop); OV, S, Z from MOVES R7
; (S and Z from the quotient when it was negated); CY from the loop.
%at($018186)
FxDivideSigned:
    cache
    from r5
    to r7
    xor r6                      ; R7 = the quotient's sign
    sub r0
    moves r6,r6
    bpl .dividend
    to r6
    sub r6                      ; R6 = -R6
.dividend:
    moves r5,r5
    bpl .shift
    to r4
    sub r4
    to r5
    sbc r5                      ; R5:R4 = -R5:R4
.shift:
    with r4
    add r4
    from r5
    rol
    ibt r12,#$10
    move r13,r15
    sub r6
    bcc .restore
    nop
    with r4
    rol
    loop
    rol
    bra .sign
    nop
.restore:
    add r6
    with r4
    add r4
    loop
    rol
.sign:
    moves r7,r7
    bpl .done
    with r4
    not
    inc r4
.done:
    jmp r11
    nop

; @routine 01:81CB FxDivide32By16
; WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
; unsigned: R4 = the quotient, R0 = the remainder.
; HOW: The shared loop (no CACHE); LSR in the return's delay slot undoes
; the remainder's extra shift. 01:81BC branches here for dividends and
; divisors below $8000.
; Entry: R0:R4 = dividend, R6 = divisor; R11 = return address. Exit: R4 =
; quotient, R0 = remainder; R12 = 0; R13 = $81D2 (the loop); S clear, Z
; and CY from the LSR; OV from the last pass.
%at($0181CB)
FxDivide32By16:
    ibt r12,#$10
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

; @routine 01:96C0 FxDivideHalving
; WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
; unsigned, halving both first when either has its top bit set: R4 = the
; quotient, R0 = the remainder (of the halved values).
; HOW: MOVES tests R0 and R6; when either is negative, LSR R6 and the
; 32-bit LSR/ROR of R0:R4 halve them. Then its own copy of the shared
; loop, with the closing LSR.
; Entry: R0:R4 = dividend, R6 = divisor; R11 = return address. Exit: R4 =
; quotient, R0 = remainder; R6 = the divisor as used; R12 = 0; R13 =
; $96D6 (the loop); S clear, Z and CY from the LSR; OV from the last pass.
%at($0196C0)
FxDivideHalving:
    moves r0,r0
    bmi .halve
    nop
    moves r6,r6
    bpl .shift
    nop
.halve:
    with r6
    lsr
    lsr
    with r4
    ror
.shift:
    ibt r12,#$10
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
