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
    iwt r13,#.pass
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

; @routine 01:81BC FxDivide32By16Halving
; WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
; unsigned, halving both first when either has its top bit set: R4 = the
; quotient, R0 = the remainder (of the halved values).
; HOW: MOVES tests R0 and R6; when either is negative, LSR R6 and the
; 32-bit LSR/ROR of R0:R4 halve them; then on into FxDivide32By16. The
; halving ($81C6-$81CA) is not reached in the traces so far: every call
; seen has both values below $8000.
; Entry: R0:R4 = dividend, R6 = divisor; R11 = return address. Exit: as
; FxDivide32By16; R6 = the divisor as used.
%at($0181BC)
FxDivide32By16Halving:
    moves r0,r0
    bmi .halve
    nop
    moves r6,r6
    bpl FxDivide32By16
    nop
.halve:
    with r6
    lsr
    lsr
    with r4
    ror

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

; @routine 01:9658 FxRatioClamp
; WHAT IT DOES: Turns the pair (R1, R2), each halved, into the ratios
; R1 * 256 / R3 and R2 * 256 / R3; when the first would pass $3FFF it is
; held at $3FFF and the second scaled to keep the pair's ratio.
; HOW: Pushes R11 on the R10 stack; ASR R1, ASR R2. When R1's high byte
; is below R3, FxDivideHalving divides R1 << 8 by R3; if that is at most
; $3FFF, R2 << 8 is divided by R3 too and the routine returns. Otherwise
; (R1's high byte at least R3, or a first quotient over $3FFF) R2 is made
; R2 * $3FFF / R1 ((R2 << 14) - R2, then FxDivideHalving; R1 and R2 here
; the halved values or the two quotients) and R1 = $3FFF. Returns by
; pulling R11's value into R15.
; Entry: R1, R2 = the pair, R3 = the divisor, R10 = the stack; R11 =
; return address. Exit: R1, R2 = the results; R10 as at entry; R11 = the
; return address of the last call; R0, R4, R6, R12, R13, OV and CY as the
; last FxDivideHalving leaves them; S and Z from R10; the last RAM address
; R10.
%at($019658)
FxRatioClamp:
    from r11
    stw (r10)                   ; push the return address
    inc r10
    inc r10
    with r1
    asr
    with r2
    asr
    from r1
    lob
    to r4
    swap
    from r1
    hib                         ; R0:R4 = R1 << 8
    cmp r3
    bcs .clamp
    nop
    move r6,r3
    link #4
    iwt r15,#FxDivideHalving
    nop
    iwt r0,#$3FFF
    sub r4
    bcc .over
    nop
    move r1,r4
    from r2
    lob
    to r4
    swap
    from r2
    hib
    move r6,r3
    link #4
    iwt r15,#FxDivideHalving
    nop
    move r2,r4
    dec r10
    dec r10
    to r15
    ldw (r10)                   ; return
    nop
.over:
    move r1,r4
    from r2
    lob
    to r4
    swap
    from r2
    hib
    move r6,r3
    link #4
    iwt r15,#FxDivideHalving
    nop
    move r2,r4
.clamp:
    ibt r4,#$00
    from r2
    lsr
    with r4
    ror
    lsr
    with r4
    ror                         ; R0:R4 = R2 << 14
    with r4
    sub r2
    ibt r6,#$00
    sbc r6                      ; R0:R4 = R2 * $3FFF
    move r6,r1
    link #4
    iwt r15,#FxDivideHalving
    nop
    move r2,r4
    iwt r1,#$3FFF
    dec r10
    dec r10
    to r15
    ldw (r10)                   ; return
    nop

; @routine 01:96C0 FxDivideHalving
; WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
; unsigned, halving both first when either has its top bit set: R4 = the
; quotient, R0 = the remainder (of the halved values).
; HOW: FxDivide32By16Halving's code again, with its own copy of the loop.
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
