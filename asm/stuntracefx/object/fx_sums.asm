; fx_sums.asm: the Super FX's two vector sums for the object it is moving.
;
; $70:2340/2342/2344 is a sum in world axes (x, y, z) and $70:2346/2348/234A
; a sum in the object's own axes. Pushes are added into them during a step;
; the own-axes sum is turned into world axes through the 3x3 matrix at
; $70:00E4 (columns of three words at $00E4, $00EA and $00F0, 1.15 fixed
; point) and added into the world sum; the world sum is then added to the
; object's Super FX record (R9).
; C: src/stuntracefx/object/fx_sums.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:944C FxPushApply
; WHAT IT DOES: Applies the push whose five words are at R3 (flags, amount,
; then a direction x, y, z) and steps R3 past them: a push with no flags,
; flags above the low byte, a negative amount or an amount of $7D00 is
; skipped (the last two decoded from the ROM bytes and not reached in the
; traces so far). Otherwise its flag byte is ORed into $2480, twice the
; amount goes to $2378, the direction is turned about the vertical by the
; angle at $247E (a quarter, half or three quarters turn by swapping and
; negating; any other angle by FxTurn, decoded from the ROM bytes and not
; reached in the traces so far) and kept at $236C/$236E/$2370, and twice
; the amount times it (FMULT) is added to the world sum (FxSumsPushWorld).
; HOW: R0 = 1 for an applied push. The registers the general turn needs
; and the return address are kept on the R10 stack.
; Entry: R3 = the push; R10 = stack; R11 = return address. Exit: R3 = the
; push + 10; R0 = 1 (applied), 8 or 6 (skipped); applied: R4-R6 as
; FxSumsPushWorld leaves them, R11 = $955E; S, Z from R10; the last RAM
; address R10 (applied) or the push's last word read.
%at($04944C)
FxPushApply:
    ldw (r3)
    inc r3
    inc r3
    moves r0,r0
    bne .flags
    nop
    iwt r15,#.none
    nop
.flags:
    to r7
    hib
    beq .push
    nop
    iwt r15,#.other
    nop
.push:
    from r11
    stw (r10)
    inc r10
    inc r10
    move r1,r0
    ldw (r3)
    inc r3
    inc r3
    moves r7,r0
    bpl .amount
    nop
    iwt r15,#.skip
    nop
.amount:
    iwt r0,#$7D00
    cmp r7
    bne .apply
    nop
    iwt r15,#.skip
    nop
.apply:
    lm r0,(!fx_push_flags)
    or r1
    sbk
    with r7
    add r7
    sm (!fx_push_amount),r7
    to r4
    ldw (r3)
    inc r3
    inc r3
    to r5
    ldw (r3)
    inc r3
    inc r3
    to r6
    ldw (r3)
    inc r3
    inc r3
    lm r0,(!fx_push_turn)
    moves r0,r0
    bne .turn
    nop
    iwt r15,#.direction
    nop
.turn:
    iwt r1,#$0040
    cmp r1
    bne .not_quarter
    nop
    move r0,r6
    not
    inc r0
    move r6,r4
    move r4,r0
    iwt r15,#.direction
    nop
.not_quarter:
    iwt r1,#$00C0
    cmp r1
    bne .not_three
    nop
    move r0,r4
    not
    inc r0
    move r4,r6
    move r6,r0
    bra .direction
    nop
.not_three:
    iwt r1,#$0080
    cmp r1
    bne .any
    nop
    with r4
    not
    inc r4
    with r6
    not
    inc r6
    bra .direction
    nop
.any:
    from r2
    stw (r10)
    inc r10
    inc r10
    from r3
    stw (r10)
    inc r10
    inc r10
    from r5
    stw (r10)
    inc r10
    inc r10
    from r7
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r11
    stw (r10)
    inc r10
    inc r10
    move r1,r4
    move r3,r6
    move r2,r0
    move r0,r1
    or r3
    beq .turned
    nop
    link #4
    iwt r15,#FxTurn
    nop
    with r1
    add r1
    with r1
    add r1
    with r3
    add r3
    with r3
    add r3
.turned:
    dec r10
    dec r10
    to r11
    ldw (r10)
    move r4,r3
    move r6,r1
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
    to r7
    ldw (r10)
    dec r10
    dec r10
    to r5
    ldw (r10)
    dec r10
    dec r10
    to r3
    ldw (r10)
    dec r10
    dec r10
    to r2
    ldw (r10)
.direction:
    sm (!fx_sizes),r4
    sm (!fx_sizes+2),r5
    sm (!fx_sizes+4),r6
    move r1,r7
    lm r6,(!fx_sizes)
    from r1
    fmult
    move r4,r0
    move r5,r7
    lm r6,(!fx_sizes+2)
    from r5
    fmult
    move r5,r0
    lm r6,(!fx_sizes+4)
    from r7
    fmult
    move r6,r0
    link #4
    iwt r15,#FxSumsPushWorld
    nop
    ibt r0,#1
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.other:
    ibt r0,#8
    with r3
    add r0
    move r15,r11
    nop
.skip:
    ibt r0,#6
    with r3
    add r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.none:
    ibt r0,#8
    with r3
    add r0
    move r15,r11
    nop

; @routine 04:96A7 FxSumsClear
; WHAT IT DOES: Clears both vector sums.
; HOW: SUB R0 from itself, then six SMs of the zero, world x first.
; Entry: R11 = return address. Exit: R0 = 0; Z, CY set, S, OV clear; the
; last RAM address $234A.
%at($0496A7)
FxSumsClear:
    sub r0
    sm (!fx_world_sum),r0
    sm (!fx_world_sum+2),r0
    sm (!fx_world_sum+4),r0
    sm (!fx_local_sum),r0
    sm (!fx_local_sum+2),r0
    sm (!fx_local_sum+4),r0
    move r15,r11
    nop

; One component of the push in <r> held to R1..R2 (-5000..5000).
macro fx_clamp(r)
    moves <r>,<r>
    bmi ?negative
    nop
    with <r>
    cmp r2
    bmi ?done
    nop
    move <r>,r2
    bra ?done
    nop
?negative:
    with <r>
    cmp r1
    bpl ?done
    nop
    move <r>,r1
?done:
endmacro

; The push in R4-R6, clamped, added to the sum at <sum>.
macro fx_push_into(sum)
    from r1
    stw (r10)
    inc r10
    inc r10
    from r2
    stw (r10)
    inc r10
    inc r10
    iwt r1,#-5000
    iwt r2,#5000
    %fx_clamp(r4)
    %fx_clamp(r5)
    %fx_clamp(r6)
    dec r10
    dec r10
    to r2
    ldw (r10)
    dec r10
    dec r10
    to r1
    ldw (r10)
    lm r0,(<sum>)
    add r4
    sm (<sum>),r0
    lm r0,(<sum>+2)
    add r5
    sm (<sum>+2),r0
    lm r0,(<sum>+4)
    add r6
    sm (<sum>+4),r0
    move r15,r11
    nop
endmacro

; @routine 04:96C3 FxSumsPushWorld
; WHAT IT DOES: Adds the push (R4, R5, R6), each held to -5000..5000, to
; the world sum.
; HOW: The clamp (clamp) per component with R1 = -5000 and R2 = 5000,
; then LM, ADD, SM per word. R1 and R2 are kept on the R10 stack.
; Entry: R4, R5, R6 = the push; R10 = stack; R11 = return address. Exit:
; R4-R6 clamped; R0 = the new world z; R1, R2 as on entry; OV, S, CY, Z
; from the last ADD; the last RAM address $2344.
%at($0496C3)
FxSumsPushWorld:
    %fx_push_into(!fx_world_sum)

; @routine 04:973F FxSumsPushLocal
; WHAT IT DOES: As FxSumsPushWorld, into the own-axes sum.
; HOW: As FxSumsPushWorld.
; Entry: as FxSumsPushWorld. Exit: as FxSumsPushWorld; R0 = the new
; own-axes z; the last RAM address $234A.
%at($04973F)
FxSumsPushLocal:
    %fx_push_into(!fx_local_sum)

; @routine 04:97BB FxSumsRotateLocal
; WHAT IT DOES: Adds the own-axes sum, turned into world axes by the matrix
; at $00E4, into the world sum.
; HOW: For world x, z, then y (in that order), the three products of the
; own-axes sum with the matrix row, each an FMULT whose top bit is rolled
; in, summed and added to the world sum's word with SBK.
; Entry: R11 = return address. Exit: R0 = the new world y; R2 = the own-axes
; sum's world y; R1, R3 = own-axes x, z; R4 = the old world y; R5 = the y
; row's first two products summed; R6 = the matrix word at $00F2; the last
; RAM address $2342; OV, S, CY, Z from the final add.
%at($0497BB)
FxSumsRotateLocal:
    lm r1,(!fx_local_sum)
    lm r2,(!fx_local_sum+2)
    lm r3,(!fx_local_sum+4)
    lms r6,(!fx_matrix_w)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix_w+6)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix_w+12)
    from r3
    fmult
    rol
    add r5
    lm r4,(!fx_world_sum)
    add r4
    sbk
    lms r6,(!fx_matrix_w+4)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix_w+10)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix_w+16)
    from r3
    fmult
    rol
    add r5
    lm r4,(!fx_world_sum+4)
    add r4
    sbk
    lms r6,(!fx_matrix_w+2)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix_w+8)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix_w+14)
    from r3
    fmult
    rol
    add r5
    move r2,r0
    lm r4,(!fx_world_sum+2)
    add r4
    sbk
    move r15,r11
    nop

; @routine 04:9BD2 FxSumsApply
; WHAT IT DOES: Adds the world sum to the Super FX record R9: half of it to
; +$1E/+$20/+$22, and twice x, eight times y and twice z to +$2E/+$30/+$32.
; Nothing is changed when the sum is zero.
; HOW: The three words are ORed to test for zero; each field is read with
; LDW, the scaled sum added (DIV2 for the halves) and stored back with SBK,
; in record order.
; Entry: R9 = the record; R11 = return address. Exit: R4, R5, R6 = the world
; sum. A zero sum: R0 = 0; S clear, Z set; the last RAM address $2344.
; Otherwise: R0 = the new +$32; R1 = the old +$32; the last RAM address
; R9 + $32; OV, S, CY, Z from the final add.
%at($049BD2)
FxSumsApply:
    lm r4,(!fx_world_sum)
    lm r5,(!fx_world_sum+2)
    lm r6,(!fx_world_sum+4)
    move r0,r4
    or r5
    or r6
    beq .done
    nop
    ibt r1,#!fxrec_half_x
    with r1
    add r9
    to r1
    ldw (r1)
    from r4
    div2
    add r1
    sbk
    ibt r1,#!fxrec_half_x+2
    with r1
    add r9
    to r1
    ldw (r1)
    from r5
    div2
    add r1
    sbk
    ibt r1,#!fxrec_half_x+4
    with r1
    add r9
    to r1
    ldw (r1)
    from r6
    div2
    add r1
    sbk
    ibt r1,#!fxrec_scaled_x
    with r1
    add r9
    to r1
    ldw (r1)
    move r0,r4
    add r0
    add r1
    sbk
    ibt r1,#!fxrec_scaled_x+2
    with r1
    add r9
    to r1
    ldw (r1)
    move r0,r5
    add r0
    add r0
    add r0
    add r1
    sbk
    ibt r1,#!fxrec_scaled_x+4
    with r1
    add r9
    to r1
    ldw (r1)
    move r0,r6
    add r0
    add r1
    sbk
.done:
    move r15,r11
    nop

arch 65816
