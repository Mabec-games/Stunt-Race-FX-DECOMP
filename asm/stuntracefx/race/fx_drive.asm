; fx_drive.asm: the Super FX drives a computer car.
;
; The decision goes to the car's +$42 as bits: 0 steer one way, 1 the
; other, 2 and 3 their harder forms, 4 accelerate, 5 a reversing turn, 6
; brake, 10 a boost. +$7C counts down a period of not accelerating (set
; when cars are pushed apart, race/fx_traffic.asm); +$7E counts how long the
; car has been stuck.
; C: src/stuntracefx/race/fx_drive.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

macro fx_bits_or(v)
    iwt r0,#<v>
    to r9
    or r9
endmacro

macro fx_bits_and(v)
    iwt r0,#<v>
    to r9
    and r9
endmacro

; @routine 04:C7D5 FxDriveDecide
; WHAT IT DOES: Decides computer car R8's controls toward the point (R1,
; R3) at speed R4 into +$42 (bits above). The heading error is the bearing
; (FxBearingTo) less +$24, less +$3A: from -$38E to $38E nothing, beyond
; it steer (bit 1 to the right, bit 0 to the left), and beyond $FA4 also
; no accelerating; with bit 14 of +$40 the harder form too (decoded from
; the ROM bytes and not reached in the traces so far). Bit 6 when at or
; above R4, else bit 10 when R4 is $227 or more, else bit 4; bit 4 also
; below speed 200. A running +$7C counts down and clears bits 4 and 10.
; +$7E counts up from -40 to -20 for a reversing turn (bits 0, 1 swapped,
; bit 5) once it has reached 20 while slower than 10, and down otherwise.
; Bits 4 and 10 are cleared while +$44 is not 0. Bit 8 of +$40 takes a
; quarter off +$26; bit 5 of +$70 makes R4 three quarters (decoded from
; the ROM bytes and not reached in the traces so far).
; HOW: R9 collects the bits; limits are CMP and the sign. R4 and the
; return address are kept on the R10 stack.
; Entry: R1, R3 = the point; R4 = the speed wanted; R8 = the car's record;
; R10 = stack; R11 = return address. Exit: R9 = the bits; R4 as used; R1
; the heading error or the last limit; the others from the tests and
; FxBearingTo; S, Z from R10; the last RAM address R10.
%at($04C7D5)
FxDriveDecide:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r0,#!fxrec_direction
    add r8
    ldw (r0)
    iwt r5,#$0020
    and r5
    beq .speed_set
    nop
    with r4
    div2
    from r4
    div2
    with r4
    add r0
.speed_set:
    from r4
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxBearingTo
    nop
    move r1,r0
    ibt r0,#!fxcar_pitch
    add r8
    ldw (r0)
    with r1
    sub r0
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r2,#$0100
    and r2
    beq .no_yaw_cut
    nop
    ibt r0,#!fxcar_yaw
    add r8
    ldw (r0)
    move r2,r0
    div2
    div2
    with r2
    sub r0
    from r2
    sbk
.no_yaw_cut:
    dec r10
    dec r10
    to r4
    ldw (r10)
    ibt r9,#0
    ibt r0,#!fxrec_speed
    add r8
    ldw (r0)
    move r3,r4
    cmp r3
    bpl .too_fast
    nop
    iwt r2,#$0227
    from r4
    cmp r2
    bmi .accelerate
    nop
    %fx_bits_or($0400)
    bra .steer
    nop
.accelerate:
    %fx_bits_or($0010)
    bra .steer
    nop
.too_fast:
    %fx_bits_or($0040)
.steer:
    ibt r2,#!fxcar_steer_bias
    with r2
    add r8
    to r2
    ldw (r2)
    move r0,r1
    sub r2
    bpl .right
    nop
    iwt r1,#$FC72
    cmp r1
    bpl .slow
    nop
    iwt r1,#$F05C
    cmp r1
    bpl .left_soft
    nop
    %fx_bits_and($FFEF)
.left_soft:
    %fx_bits_or($0002)
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r1,#$4000
    and r1
    beq .slow
    nop
    %fx_bits_or($0008)
    bra .slow
    nop
.right:
    iwt r1,#$038E
    cmp r1
    bmi .slow
    nop
    iwt r1,#$0FA4
    cmp r1
    bmi .right_soft
    nop
    %fx_bits_and($FFEF)
.right_soft:
    %fx_bits_or($0001)
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r1,#$4000
    and r1
    beq .slow
    nop
    %fx_bits_or($0004)
    bra .slow
    nop
.slow:
    ibt r0,#!fxrec_speed
    add r8
    ldw (r0)
    iwt r3,#$00C8
    cmp r3
    bpl .spread
    nop
    %fx_bits_or($0010)
.spread:
    ibt r0,#!fxrec_spread
    add r8
    ldw (r0)
    moves r1,r0
    beq .stuck
    dec r0
    sbk
    %fx_bits_and($FBEF)
.stuck:
    ibt r0,#!fxcar_stuck
    add r8
    ldw (r0)
    moves r0,r0
    bmi .count_up
    nop
    iwt r1,#$0014
    cmp r1
    bmi .moving
    nop
    iwt r0,#$FFD8
.count_up:
    inc r0
    sbk
    iwt r1,#$FFEC
    cmp r1
    bpl .store
    nop
    %fx_bits_and($FBAF)
    %fx_bits_or($0020)
    iwt r0,#$0003
    to r9
    xor r9
    bra .store
    nop
.moving:
    ibt r0,#!fxrec_speed
    add r8
    ldw (r0)
    iwt r1,#$000A
    cmp r1
    bpl .unstick
    nop
    ibt r0,#!fxcar_stuck
    add r8
    ldw (r0)
    inc r0
    sbk
    bra .store
    nop
.unstick:
    ibt r0,#!fxcar_stuck
    add r8
    ldw (r0)
    moves r0,r0
    beq .store
    dec r0
    sbk
.store:
    ibt r0,#!fxcar_44
    add r8
    ldw (r0)
    moves r0,r0
    beq .bits
    nop
    %fx_bits_and($FBEF)
.bits:
    ibt r0,#!fxrec_sides
    add r8
    from r9
    stw (r0)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
