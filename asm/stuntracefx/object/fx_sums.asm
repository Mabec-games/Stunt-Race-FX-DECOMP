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
