; project.asm: the Super FX projecting a point onto the screen: X and Y over
; the depth (FxRatioClamp, clamped to $3FFF), moved to the screen centre,
; with an outcode of which screen edges the point is beyond.
; C: src/stuntracefx/render/project.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:947A FxProjectPoint
; WHAT IT DOES: Projects the point (R1, R2) at depth R3 to the screen,
; moved to the centre at $0034, and returns its outcode: bits 0-1 past
; the X edges, bits 2-3 past the Y edges, bit 4 behind the view.
; HOW: Pushes R11 on the R10 stack. A negative depth is negated and sets
; R5 = 4 (else R5 = 0; a zero depth counts as 1). FxRatioClamp divides the
; larger of |R1|, |R2| first (the pair swapped around the call when |R1|
; is the smaller), then the signs go back, each one reversed when the
; depth was negative: sixteen copies of the code, one per case. Then
; CACHE, the centre is added, and against the edges at $0038-$003E
; (SUB, BGE) R5 counts 1 for past the low edge and 2 for past the high
; one, its two bits swapped when behind, X then (after MULT #4) Y. R0 =
; the outcode's complement in its low 5 bits << 8, OR the outcode.
; Returns by pulling R11's value into R15.
; Entry: R1, R2 = the point, R3 = its depth; R10 = the stack; R11 =
; return address. Exit: R1, R2 = the screen position; R5 = the outcode;
; R0 = ((R5 XOR $1F) << 8) | R5; R3 = |depth| (1 for 0); R4, R6, R11,
; R12, R13 as FxRatioClamp leaves them; R10 as at entry; the last RAM
; address R10; S and Z from R10; OV and CY from the last SUB R2.
%at($01947A)
FxProjectPoint:
    from r11
    stw (r10)
    inc r10
    inc r10
    moves r3,r3
    bpl .ahead
    nop
    iwt r15,#.behind
    nop
.ahead:
    nop
    bne .front
    nop
    inc r3
.front:
    ibt r5,#$00
    moves r1,r1
    bmi .f_nx
    nop
    moves r2,r2
    bmi .f_pn
    nop
    from r1
    sub r2
    bcc .f_pp_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    iwt r15,#.tail
    nop
.f_pp_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
    iwt r15,#.tail
    nop
.f_pn:
    with r2
    not
    inc r2
    from r1
    sub r2
    bcc .f_pn_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    with r2
    not
    inc r2
    iwt r15,#.tail
    nop
.f_pn_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
    with r2
    not
    inc r2
    iwt r15,#.tail
    nop
.f_nx:
    with r1
    not
    inc r1
    moves r2,r2
    bmi .f_nn
    nop
    from r1
    sub r2
    bcc .f_np_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    with r1
    not
    inc r1
    iwt r15,#.tail
    nop
.f_np_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
    with r1
    not
    inc r1
    iwt r15,#.tail
    nop
.f_nn:
    with r2
    not
    inc r2
    from r1
    sub r2
    bcc .f_nn_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    with r2
    not
    inc r2
    with r1
    not
    inc r1
    iwt r15,#.tail
    nop
.f_nn_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
    with r2
    not
    inc r2
    with r1
    not
    inc r1
    iwt r15,#.tail
    nop
.behind:
    with r3
    not
    inc r3
    ibt r5,#$04
    moves r1,r1
    bmi .b_nx
    nop
    moves r2,r2
    bmi .b_pn
    nop
    from r1
    sub r2
    bcc .b_pp_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    with r1
    not
    inc r1
    with r2
    not
    inc r2
    iwt r15,#.tail
    nop
.b_pp_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
    with r1
    not
    inc r1
    with r2
    not
    inc r2
    iwt r15,#.tail
    nop
.b_pn:
    with r2
    not
    inc r2
    from r1
    sub r2
    bcc .b_pn_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    with r1
    not
    inc r1
    iwt r15,#.tail
    nop
.b_pn_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
    with r1
    not
    inc r1
    iwt r15,#.tail
    nop
.b_nx:
    with r1
    not
    inc r1
    moves r2,r2
    bmi .b_nn
    nop
    from r1
    sub r2
    bcc .b_np_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    with r2
    not
    inc r2
    iwt r15,#.tail
    nop
.b_np_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
    with r2
    not
    inc r2
    iwt r15,#.tail
    nop
.b_nn:
    with r2
    not
    inc r2
    from r1
    sub r2
    bcc .b_nn_swap
    nop
    link #4
    iwt r15,#FxRatioClamp
    nop
    iwt r15,#.tail
    nop
.b_nn_swap:
    move r0,r1
    move r1,r2
    move r2,r0
    link #4
    iwt r15,#FxRatioClamp
    nop
    move r0,r1
    move r1,r2
    move r2,r0
.tail:
    cache
    lms r0,(!gsu_in_34-!gsu_ram)
    with r1
    add r0
    lms r0,(!gsu_in_36-!gsu_ram)
    with r2
    add r0
    lms r0,(!gsu_in_38-!gsu_ram)
    sub r1
    bge .x_one
    nop
    lms r0,(!gsu_in_3a-!gsu_ram)
    sub r1
    bge .y_axis
    nop
    inc r5
.x_one:
    inc r5
    ibt r0,#$04
    and r5
    beq .y_axis
    nop
    with r5
    xor #3
.y_axis:
    with r5
    mult #4
    lms r0,(!gsu_in_3c-!gsu_ram)
    sub r2
    bge .y_one
    nop
    lms r0,(!gsu_in_3e-!gsu_ram)
    sub r2
    bge .code
    nop
    inc r5
.y_one:
    inc r5
    ibt r0,#$10
    and r5
    beq .code
    nop
    with r5
    xor #3
.code:
    ibt r0,#$1F
    xor r5
    swap
    or r5
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
