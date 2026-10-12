; angle.asm: the Super FX's angles: a full turn is $10000, so $4000 is a
; quarter turn. The arctangent comes from a table of 256 words in ROM
; bank $00 at $9E40, indexed by the ratio of the smaller side to the
; larger.
; C: src/stuntracefx/render/angle.c.

incsrc "../include/macros.inc"
incsrc "../include/rom.inc"

arch superfx

; @routine 01:81F7 FxAtan2
; WHAT IT DOES: The angle of the vector (R2, R1): 0 along +R2, $4000
; along +R1, as a word where a full turn is $10000.
; HOW: Pushes R11 and R14 on the R10 stack. With R6 = |R1| and R0 = |R2|:
; R2 = 0 gives $4000 and equal sides give $2000; otherwise the smaller
; side << 14 is divided by the larger (FxDivide32By16), and the quotient
; >> 5, made even, indexes the arctangent table (GETB/GETBH from ROM bank
; 0). When |R1| is the larger side the angle is $4000 minus the table's.
; Then it is negated when R1 and R2 differ in sign, and $8000 is added
; when R2 is negative. Pulls R14 and returns by pulling R11's value into
; R15.
; Entry: R1, R2 = the vector; R10 = the stack; R11 = return address.
; Exit: R0 = the angle; R4 = $8000 when R2 is negative, else R1 XOR R2;
; R6 = the larger side (|R1| when R2 = 0 or the sides are equal); R5 =
; $FFFF when |R2| is the larger side, 0 when |R1| is or they are equal,
; kept when R2 = 0. When the table was used: R11 = $823D, R12 = 0, R13 =
; $81D2, the ROM bank 0. R14 and R10 as at entry; the last RAM address
; R10; S and Z from R10; OV and CY from the ADD of $8000 when R2 is
; negative, else OV = bit 7 of R2 and CY from the last ADD before (kept
; when R2 = 0, set when the sides are equal).
%at($0181F7)
FxAtan2:
    from r11
    stw (r10)                   ; push the return address
    inc r10
    inc r10
    from r14
    stw (r10)                   ; push R14
    inc r10
    inc r10
    moves r6,r1
    bpl .y_positive
    nop
    with r6
    not
    inc r6                      ; R6 = |R1|
.y_positive:
    moves r0,r2
    bpl .x_positive
    nop
    not
    inc r0                      ; R0 = |R2|
.x_positive:
    bne .x_nonzero
    nop
    iwt r0,#$4000
    iwt r15,#.sign
    nop
.x_nonzero:
    ibt r5,#$00
    cmp r6
    bne .unequal
    nop
    iwt r0,#$2000
    iwt r15,#.sign
    nop
.unequal:
    blt .ratio
    nop
    move r4,r6
    move r6,r0
    move r0,r4
    dec r5                      ; |R2| is the larger side
.ratio:
    ibt r4,#$00
    lsr
    with r4
    ror
    lsr
    with r4
    ror                         ; R0:R4 = the smaller side << 14
    link #4
    iwt r15,#FxDivide32By16
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
    bmi .sign
    nop
    not
    inc r0
    iwt r4,#$4000
    add r4                      ; $4000 - the table's angle
.sign:
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
    ldw (r10)                   ; pull R14
    dec r10
    dec r10
    to r15
    ldw (r10)                   ; return
    nop

arch 65816
