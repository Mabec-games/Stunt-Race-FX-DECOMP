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

; @routine 04:941C FxPushesApply
; WHAT IT DOES: Applies the 19 pushes at $23AE (FxPushApply each) with the
; push flags at $2480 cleared first, and returns the flags they set.
; HOW: The return address is kept on the R10 stack; the ROM bank is set to 0;
; R13 counts the pushes from 1, stopping at $14.
; Entry: R10 = stack; R11 = return address. Exit: R0 = the flags at $2480;
; R3 past the last push; R13 = $14; R11 = $943F; CY, OV from the last count
; compare; S, Z from R10; the last RAM address R10.
%at($04941C)
FxPushesApply:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r0,#$00
    romb
    sub r0
    sm (!fx_push_flags),r0
    iwt r3,#!fx_pushes-!gsu_ram
    ibt r13,#$00
.next:
    inc r13
    ibt r0,#$14
    cmp r13
    bne .apply
    nop
    iwt r15,#.done
    nop
.apply:
    link #4
    iwt r15,#FxPushApply
    nop
    iwt r15,#.next
    nop
.done:
    lm r0,(!fx_push_flags)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

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

; ---- pushes at a point: 04:987D-9BC5 ----

; @routine 04:987D FxHitAddScaled
; WHAT IT DOES: Adds to the hit point at $2334 the vector (R1, R2, R3)
; scaled by R6 (the low words of LMULT), made positive, each times half
; the world sum's word (FMULT); a zero vector counts as $7FFF on each
; axis.
; HOW: R11, R9, R8 pushed on the R10 stack; OR of the three tests for
; zero; MOVES, NOT, INC for the magnitudes; DIV2, FMULT, ADD, SM per axis.
; Returns by pulling R15.
; Entry: R1-R3 = the vector, R6 = the scale; R10 = stack; R11 = return
; address. Exit: R1-R3 the amounts added; R0 the new hit z; R6 half the
; world z; R4 the last low product; R8, R9 restored; S, Z from R10; the
; last RAM address R10.
%at($04987D)
FxHitAddScaled:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    from r1
    or r2
    or r3
    bne .l9899
    nop
    iwt r1,#$7FFF
    move r2,r1
    move r3,r1
    bra FxHitAddSum
    nop
.l9899:
    from r1
    lmult
    move r1,r4
    from r2
    lmult
    move r2,r4
    from r3
    lmult
    move r3,r4
    bra FxHitAddAbs
    nop

; @routine 04:98AB FxHitAddUnit
; WHAT IT DOES: As FxHitAddScaled with the vector made a unit vector
; (FxNormalize) instead of scaled.
; HOW: As FxHitAddScaled. Not reached in the traces so far.
; Entry: R1-R3 = the vector; R10 = stack; R11 = return address. Exit: as
; FxHitAddScaled, the rest as FxNormalize leaves them.
%at($0498AB)
FxHitAddUnit:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    from r1
    or r2
    or r3
    bne .l98C7
    nop
    iwt r1,#$7FFF
    move r2,r1
    move r3,r1
    bra FxHitAddSum
    nop
.l98C7:
    link #4
    iwt r15,#FxNormalize
    nop
FxHitAddAbs:
    moves r1,r1
    bpl .l98D4
    nop
    with r1
    not
    inc r1
.l98D4:
    moves r2,r2
    bpl .l98DC
    nop
    with r2
    not
    inc r2
.l98DC:
    moves r3,r3
    bpl FxHitAddSum
    nop
    with r3
    not
    inc r3
FxHitAddSum:
    lm r6,(!fx_world_sum)
    with r6
    div2
    with r1
    fmult
    lm r0,(!fx_hit)
    add r1
    sm (!fx_hit),r0
    lm r6,(!fx_world_sum+2)
    with r6
    div2
    with r2
    fmult
    lm r0,(!fx_hit+2)
    add r2
    sm (!fx_hit+2),r0
    lm r6,(!fx_world_sum+4)
    with r6
    div2
    with r3
    fmult
    lm r0,(!fx_hit+4)
    add r3
    sm (!fx_hit+4),r0
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:9927 FxPushAtTurned
; WHAT IT DOES: As FxPushAt4 at the point (R1, R2, R3) turned by the
; matrix at $00E4 first (left at $26F6/$26F8/$26FA).
; HOW: The matrix rows (FMULT, ROL) as in the shared part, SM to
; $26F6-$26FA, LM back into R1-R3, then a jump to FxPushAt4. Nothing
; calls it.
; Entry: as FxPushAt4. Exit: as FxPushAt4.
%at($049927)
FxPushAtTurned:
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
    sm (!fx_push_point),r0
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
    sm (!fx_push_point+2),r0
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
    sm (!fx_push_point+4),r0
    lm r1,(!fx_push_point)
    lm r2,(!fx_push_point+2)
    lm r3,(!fx_push_point+4)
    iwt r15,#FxPushAt4
    nop

; @routine 04:9985 FxPushAt4Kept
; WHAT IT DOES: As FxPushAt4.
; HOW: R11, R9 and R8 are pushed and pulled straight back (R11 into R11)
; before the jump to FxPushAt4. Nothing calls it.
; Entry: as FxPushAt4. Exit: as FxPushAt4.
%at($049985)
FxPushAt4Kept:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r11
    ldw (r10)
    iwt r15,#FxPushAt4
    nop

; @routine 04:99A1 FxPushAt2
; WHAT IT DOES: As FxPushAt4 with twice the world sum.
; HOW: As FxPushAt4, one ADD per word. Not reached in the traces so far.
; Entry: as FxPushAt4. Exit: as FxPushAt4.
%at($0499A1)
FxPushAt2:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    lm r0,(!fx_world_sum)
    add r0
    sms (!fx_push_rot_w),r0
    lm r0,(!fx_world_sum+2)
    add r0
    sms (!fx_push_rot_w+8),r0
    lm r0,(!fx_world_sum+4)
    add r0
    sms (!fx_push_rot_w+16),r0
    iwt r15,#FxPushAtShared
    nop

; @routine 04:99C9 FxPushAt1
; WHAT IT DOES: As FxPushAt4 with the world sum as it is.
; HOW: As FxPushAt4, the words as loaded. Not reached in the traces so
; far.
; Entry: as FxPushAt4. Exit: as FxPushAt4.
%at($0499C9)
FxPushAt1:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    lm r0,(!fx_world_sum)
    sms (!fx_push_rot_w),r0
    lm r0,(!fx_world_sum+2)
    sms (!fx_push_rot_w+8),r0
    lm r0,(!fx_world_sum+4)
    sms (!fx_push_rot_w+16),r0
    iwt r15,#FxPushAtShared
    nop

; @routine 04:99EE FxPushAtHalf
; WHAT IT DOES: As FxPushAt4 with half the world sum.
; HOW: As FxPushAt4, one DIV2 per word.
; Entry: as FxPushAt4. Exit: as FxPushAt4.
%at($0499EE)
FxPushAtHalf:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    lm r0,(!fx_world_sum)
    div2
    sms (!fx_push_rot_w),r0
    lm r0,(!fx_world_sum+2)
    div2
    sms (!fx_push_rot_w+8),r0
    lm r0,(!fx_world_sum+4)
    div2
    sms (!fx_push_rot_w+16),r0
    iwt r15,#FxPushAtShared
    nop

; @routine 04:9A19 FxPushAtQuarter
; WHAT IT DOES: As FxPushAt4 with a quarter of the world sum.
; HOW: As FxPushAt4, two DIV2s per word. Not reached in the traces so far.
; Entry: as FxPushAt4. Exit: as FxPushAt4.
%at($049A19)
FxPushAtQuarter:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    lm r0,(!fx_world_sum)
    div2
    div2
    sms (!fx_push_rot_w),r0
    lm r0,(!fx_world_sum+2)
    div2
    div2
    sms (!fx_push_rot_w+8),r0
    lm r0,(!fx_world_sum+4)
    div2
    div2
    sms (!fx_push_rot_w+16),r0
    iwt r15,#FxPushAtShared
    nop

; @routine 04:9A4A FxPushAt4Twist
; WHAT IT DOES: FxPushAt4, then the cross product's x ($0068) eight times
; more into the spin sum at $233C.
; HOW: R11 pushed on the R10 stack around the call; three ADDs and SBK.
; Not reached in the traces so far.
; Entry: as FxPushAt4. Exit: as FxPushAt4, R4 = the old spin sum, R11 =
; $9A53; S, Z from R10.
%at($049A4A)
FxPushAt4Twist:
    from r11
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxPushAt4
    nop
    lm r0,(!fx_normal_x_w)
    lm r4,(!fx_spin+2)
    add r0
    add r0
    add r0
    add r4
    sbk
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:9A65 FxSumScale
; WHAT IT DOES: Scales the world sum by R6 as an 8.8 fraction: each word
; becomes bits 8-23 of its product with R6.
; HOW: LMULT per word: the high word's low byte (LOB, SWAP) ORed with the
; low word's high byte (HIB), then three SMs. Returns with MOVE R15,R11.
; Not reached in the traces so far.
; Entry: R6 = the scale; R11 = return address. Exit: R1-R3 the new sum;
; R0 = the high byte part of z, R4 its low byte part; flags from the
; last OR; the last RAM address $2344.
%at($049A65)
FxSumScale:
    lm r1,(!fx_world_sum)
    lm r2,(!fx_world_sum+2)
    lm r3,(!fx_world_sum+4)
    from r1
    lmult
    lob
    swap
    with r4
    hib
    to r1
    or r4
    from r2
    lmult
    lob
    swap
    with r4
    hib
    to r2
    or r4
    from r3
    lmult
    lob
    swap
    with r4
    hib
    to r3
    or r4
    sm (!fx_world_sum),r1
    sm (!fx_world_sum+2),r2
    sm (!fx_world_sum+4),r3
    move r15,r11
    nop

; @routine 04:9A9B FxPushAt4
; WHAT IT DOES: Pushes the object at the point (R1, R2, R3) in its own
; axes with four times the world sum: the sum, turned into the object's
; axes, goes to $0132/$013A/$0142, and the point (times 16) crossed with
; it is added, four times, to the spin sums at $233C, $233E and $233A.
; HOW: R11, R9, R8 pushed on the R10 stack; two ADDs per word of the
; world sum; then the part all the variants share (04:9AC2): sixteen
; times the point to $26F6/$26F8/$26FA, $0134 cleared, the matrix at
; $00E4 (FMULT, ROL), the cross product (LMULT, SUB, SBC, ADD, ROL) to
; $0068/$002C/$002E and the sums by SBK. Returns by pulling R15.
; Entry: R1, R2, R3 = the point; R10 = stack; R11 = return address.
; Exit: R1-R3 = the point times 16; R0 the last spin sum; R4, R5, R6, R7
; as the cross product and sums leave them; R8, R9 restored; S, Z from
; R10; the last RAM address R10. Its variants are picked by $2654
; through the table at $04:9BC6.
%at($049A9B)
FxPushAt4:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    lm r0,(!fx_world_sum)
    add r0
    add r0
    sms (!fx_push_rot_w),r0
    lm r0,(!fx_world_sum+2)
    add r0
    add r0
    sms (!fx_push_rot_w+8),r0
    lm r0,(!fx_world_sum+4)
    add r0
    add r0
    sms (!fx_push_rot_w+16),r0
FxPushAtShared:
    with r1
    add r1
    with r2
    add r2
    with r3
    add r3
    with r1
    add r1
    with r2
    add r2
    with r3
    add r3
    with r1
    add r1
    with r2
    add r2
    with r3
    add r3
    with r1
    add r1
    with r2
    add r2
    with r3
    add r3
    sub r0
    sms (!fx_push_rot_w+2),r0
    sm (!fx_push_point),r1
    sm (!fx_push_point+2),r2
    sm (!fx_push_point+4),r3
    lms r1,(!fx_push_rot_w)
    lms r2,(!fx_push_rot_w+8)
    lms r3,(!fx_push_rot_w+16)
    lms r6,(!fx_matrix_w)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix_w+2)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix_w+4)
    from r3
    fmult
    rol
    add r5
    sms (!fx_push_rot_w),r0
    lms r6,(!fx_matrix_w+6)
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
    lms r6,(!fx_matrix_w+10)
    from r3
    fmult
    rol
    add r5
    sms (!fx_push_rot_w+8),r0
    lms r6,(!fx_matrix_w+12)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix_w+14)
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
    sms (!fx_push_rot_w+16),r0
    lm r1,(!fx_push_point)
    lm r2,(!fx_push_point+2)
    lm r3,(!fx_push_point+4)
    lms r6,(!fx_push_rot_w+8)
    from r3
    lmult
    move r7,r4
    lms r6,(!fx_push_rot_w+16)
    from r2
    to r5
    lmult
    with r7
    sub r4
    sbc r5
    with r7
    add r7
    rol
    sms (!fx_normal_x_w),r0
    lms r6,(!fx_push_rot_w+16)
    from r1
    lmult
    move r7,r4
    lms r6,(!fx_push_rot_w)
    from r3
    to r5
    lmult
    with r7
    sub r4
    sbc r5
    with r7
    add r7
    rol
    sms (!fx_normal_y_w),r0
    lms r6,(!fx_push_rot_w)
    from r2
    lmult
    move r7,r4
    lms r6,(!fx_push_rot_w+8)
    from r1
    to r5
    lmult
    with r7
    sub r4
    sbc r5
    with r7
    add r7
    rol
    sms (!fx_normal_z_w),r0
    lm r0,(!fx_normal_x_w)
    lm r4,(!fx_spin+2)
    add r0
    add r0
    add r4
    sbk
    lm r0,(!fx_normal_z_w)
    lm r5,(!fx_spin+4)
    add r0
    add r0
    add r5
    sbk
    lm r0,(!fx_normal_y_w)
    lm r6,(!fx_spin)
    add r0
    add r0
    add r6
    sbk
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
