; fx_traffic.asm: the Super FX looks for cars about to be caught up with.
;
; For car R8, every other car on the cars' list ($70:262E, linked through
; +$74) that is not flagged out (bit 10 of +$16) and not faster (+$4A) is
; tested: it counts when the closing speed along the line between them,
; plus 20, is at least their distance (|dx| + |dz|) / 32, and it is less
; than $200 to the side (along the first column of the matrix at $00E4).
; Bits 0 and 1 of R8's +$42 then say on which side it is.
; C: src/stuntracefx/race/fx_traffic.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; The dot product of (R1, R2, R3) with the words at $0068, $002C, $002E.
macro fx_dot()
    lms r6,(!fx_normal_x_w)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_normal_y_w)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_normal_z_w)
    from r3
    fmult
    rol
    add r5
endmacro

; @routine 04:C933 FxTrafficSides
; WHAT IT DOES: As described above: for each car that counts, bit 1 of R8's
; +$42 is set when it is to the left (its offset negative) and bit 0
; cleared if it was set when the routine started; to the right, bit 0 set
; and bit 1 cleared likewise. $2638 = -1 for a car $200 or more to the
; side.
; HOW: R8's position and velocity are copied to $26F6, $26FA, $26FC,
; $2700; per car, the difference goes to $0068/$002C/$002E, the distance
; / 32 (DIV2 five times) to $263A, the closing speed (FMULT, ROL) plus 20
; to $2638, then the side offset to $263A. R9 holds +$42 as it was at the
; start; the bits are changed in R6, read again per car. The return
; address is kept on the R10 stack.
; Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
; R7 = 0; R9 = the old +$42; the other registers from the last car tested;
; S, Z from R10; the last RAM address R10.
%at($04C933)
FxTrafficSides:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r9,#!fxrec_sides
    with r9
    add r8
    to r9
    ldw (r9)
    ibt r0,#!fxrec_half_x
    add r8
    ldw (r0)
    sm (!fx_me_x),r0
    ibt r0,#!fxrec_half_x+4
    add r8
    ldw (r0)
    sm (!fx_me_z),r0
    ibt r0,#!fxrec_velocity
    add r8
    ldw (r0)
    sm (!fx_me_vx),r0
    ibt r0,#!fxrec_velocity+4
    add r8
    ldw (r0)
    sm (!fx_me_vz),r0
    lm r0,(!fx_cars)
.car:
    moves r7,r0
    bne .test
    nop
    iwt r15,#.done
    nop
.test:
    cmp r8
    bne .other
    nop
    iwt r15,#.next
    nop
.other:
    ibt r1,#!fxrec_flags16
    with r1
    add r7
    to r1
    ldw (r1)
    iwt r0,#$0400
    and r1
    beq .in
    nop
    iwt r15,#.next
    nop
.in:
    nop
    ibt r1,#!fxrec_speed
    with r1
    add r7
    to r1
    ldw (r1)
    ibt r0,#!fxrec_speed
    add r8
    ldw (r0)
    sub r1
    bpl .slower
    nop
    iwt r15,#.next
    nop
.slower:
    ibt r0,#!fxrec_half_x
    add r7
    ldw (r0)
    lm r1,(!fx_me_x)
    sub r1
    sms (!fx_normal_x_w),r0
    moves r1,r0
    bpl .dx
    nop
    with r1
    not
    inc r1
.dx:
    ibt r0,#!fxrec_half_x+4
    add r7
    ldw (r0)
    lm r3,(!fx_me_z)
    sub r3
    sms (!fx_normal_z_w),r0
    moves r3,r0
    bpl .dz
    nop
    with r3
    not
    inc r3
.dz:
    sub r0
    sms (!fx_normal_y_w),r0
    from r1
    add r3
    div2
    div2
    div2
    div2
    div2
    sm (!fx_traffic_dist),r0
    ibt r0,#!fxrec_velocity
    add r7
    ldw (r0)
    lm r1,(!fx_me_vx)
    with r1
    sub r0
    ibt r0,#!fxrec_velocity+4
    add r7
    ldw (r0)
    lm r3,(!fx_me_vz)
    with r3
    sub r0
    ibt r2,#0
    %fx_dot()
    iwt r1,#$0014
    add r1
    sm (!fx_traffic_close),r0
    lm r1,(!fx_traffic_dist)
    cmp r1
    bpl .closing
    nop
    iwt r15,#.next
    nop
.closing:
    lms r1,(!fx_matrix_w)
    lms r2,(!fx_matrix_w+2)
    lms r3,(!fx_matrix_w+4)
    %fx_dot()
    sm (!fx_traffic_dist),r0
    move r1,r0
    moves r0,r0
    bpl .offset
    nop
    not
    inc r0
.offset:
    iwt r2,#$0200
    cmp r2
    bmi .near
    nop
    ibt r2,#-1
    sm (!fx_traffic_close),r2
.near:
    ibt r6,#!fxrec_sides
    with r6
    add r8
    to r6
    ldw (r6)
    lm r0,(!fx_traffic_close)
    moves r0,r0
    bmi .store
    nop
    moves r1,r1
    bpl .right
    nop
    iwt r0,#$0001
    and r9
    beq .left
    nop
    iwt r0,#$FFFE
    to r6
    and r6
.left:
    iwt r0,#$0002
    to r6
    or r6
    bra .store
    nop
.right:
    iwt r0,#$0002
    and r9
    beq .right_set
    nop
    iwt r0,#$FFFD
    to r6
    and r6
.right_set:
    iwt r0,#$0001
    to r6
    or r6
.store:
    ibt r0,#!fxrec_sides
    add r8
    from r6
    stw (r0)
.next:
    ibt r0,#!fxrec_next_car
    add r7
    ldw (r0)
    iwt r15,#.car
    nop
.done:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:BF98 FxSpreadCars
; WHAT IT DOES: Pushes the other cars near car R8 away from it: for each
; car on the cars' list (not R8) whose distance (|dx| + |dy| + |dz|) / 2
; is below 2000, its velocity x and z gain twice (2000 - that) along the
; unit vector from R8 to it, its +$34 gains or loses a share of it (by the
; sign of +$34: times $C180 or $3E80, scaled), and its +$7C = 30. A
; negative half z distance is made positive by code decoded from the ROM
; bytes and not reached in the traces so far.
; HOW: R8's position in R6, R5, R7 and R2 = 2000; per car, the differences
; (x into R1, z into R3) halved with DIV2; for a near car R8, R6, R5, R7,
; R2, the push and the car are kept on the R10 stack around FxNormalize of
; (dx, 0, dz), and each push is FMULT then ADD and SBK. R8 and the return
; address are kept on the R10 stack.
; Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
; R9 = 0; R8, R6, R5, R7, R2 as set up; the others from the last car; S,
; Z from R10; the last RAM address R10.
%at($04BF98)
FxSpreadCars:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    ibt r6,#!fxrec_half_x
    with r6
    add r8
    to r6
    ldw (r6)
    ibt r5,#!fxrec_half_x+2
    with r5
    add r8
    to r5
    ldw (r5)
    ibt r7,#!fxrec_half_x+4
    with r7
    add r8
    to r7
    ldw (r7)
    iwt r2,#2000
    lm r0,(!fx_cars)
.car:
    moves r9,r0
    bne .test
    nop
    iwt r15,#.done
    nop
.test:
    cmp r8
    bne .other
    nop
    iwt r15,#.next
    nop
.other:
    ibt r0,#!fxrec_half_x
    add r9
    ldw (r0)
    sub r6
    move r1,r0
    div2
    bpl .dx
    nop
    not
    inc r0
.dx:
    move r4,r0
    ibt r0,#!fxrec_half_x+2
    add r9
    ldw (r0)
    sub r5
    div2
    bpl .dy
    nop
    not
    inc r0
.dy:
    with r4
    add r0
    ibt r0,#!fxrec_half_x+4
    add r9
    ldw (r0)
    sub r7
    move r3,r0
    div2
    bpl .dz
    nop
    not
    inc r0
.dz:
    add r4
    sub r2
    bpl .next
    nop
    from r8
    stw (r10)
    inc r10
    inc r10
    from r6
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
    from r2
    stw (r10)
    inc r10
    inc r10
    moves r0,r0
    bpl .push
    nop
    not
    inc r0
.push:
    add r0
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    ibt r2,#0
    link #4
    iwt r15,#FxNormalize
    nop
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r6
    ldw (r10)
    from r1
    fmult
    ibt r4,#!fxrec_velocity
    with r4
    add r9
    to r4
    ldw (r4)
    add r4
    sbk
    from r3
    fmult
    ibt r4,#!fxrec_velocity+4
    with r4
    add r9
    to r4
    ldw (r4)
    add r4
    sbk
    ibt r0,#!fxrec_yaw_rate
    add r9
    ldw (r0)
    move r1,r0
    iwt r0,#$C180
    moves r1,r1
    bmi .turn
    nop
    iwt r0,#$3E80
.turn:
    with r6
    add r6
    with r6
    add r6
    fmult
    add r1
    sbk
    iwt r4,#$001E
    ibt r0,#!fxrec_spread
    add r9
    from r4
    stw (r0)
    dec r10
    dec r10
    to r2
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
    to r6
    ldw (r10)
    dec r10
    dec r10
    to r8
    ldw (r10)
.next:
    ibt r0,#!fxrec_next_car
    add r9
    ldw (r0)
    iwt r15,#.car
    nop
.done:
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
