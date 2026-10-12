; fx_collide.asm: the Super FX's push between two touching objects.
;
; R9 is the record being pushed and R7 the other's record; R1 points at
; the other's contact words: its velocity x, z at +$04, +$06 and position
; x, z at +$08, +$0A. A record's +$2A is its mass. The result goes to the
; push at $70:23A4 (object/fx_record.asm) as an amount along the unit vector
; from the other's position to this one's.
; C: src/stuntracefx/object/fx_collide.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

macro fx_push(reg)
    from <reg>
    stw (r10)
    inc r10
    inc r10
endmacro

macro fx_pull(reg)
    dec r10
    dec r10
    to <reg>
    ldw (r10)
endmacro

; @routine 04:9301 FxCollidePush
; WHAT IT DOES: Sets the push on record R9 from the object at R5 (record
; R7): along the unit vector from the other's position to R9's (on the
; ground plane), with amount = the closing speed along it (the velocity
; difference, doubled, dotted with the unit vector; raised to $04B0 when it
; is from 0 to $04B0) times the other's share of the two masses, halved.
; HOW: The difference goes to $0020/$0022/$0024 and FxNormalize makes the
; unit vector, kept at $0068/$002C/$002E; the closing speed is two FMULTs
; and is kept at $26F6; FxDivideHalvedIfLarge gives the mass share as
; other * $10000 / (both), halved with LSR; the amount is FMULT of the
; doubled speed by it. The push is on when the amount is not negative.
; R11, R1, R8 and the registers FxNormalize and the divide would lose are
; kept on the R10 stack.
; Entry: R1 = the contact words; R7 = the other's record; R9 = the
; record; R10 = stack; R11 = return address. Exit: R0 = 1 (on) or 0; R1,
; R7, R8, R9 as on entry; R5 = R1; R2 = 0; R3 = the unit z; R4 = the share
; before halving; R6 = the share; R11 = $93B3; R12 = 0; R13 = $C14B; S, Z
; from R10, OV, CY from the SUB setting R0; the last RAM address R10.
%at($049301)
FxCollidePush:
    %fx_push(r11)
    %fx_push(r1)
    %fx_push(r8)
    move r5,r1
    ibt r1,#!fxrec_half_x
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r0,#!fxhit_x
    add r5
    ldw (r0)
    with r1
    sub r0
    sm (!fx_angles_w),r1
    ibt r2,#0
    sm (!fx_angles_w+2),r2
    ibt r3,#!fxrec_half_x+4
    with r3
    add r9
    to r3
    ldw (r3)
    ibt r0,#!fxhit_x+2
    add r5
    ldw (r0)
    with r3
    sub r0
    sm (!fx_angles_w+4),r3
    %fx_push(r7)
    %fx_push(r9)
    %fx_push(r5)
    link #4
    iwt r15,#FxNormalize
    nop
    sms (!fx_normal_x_w),r1
    sms (!fx_normal_y_w),r2
    sms (!fx_normal_z_w),r3
    %fx_pull(r5)
    %fx_pull(r9)
    %fx_pull(r7)
    ibt r0,#!fxhit_velocity
    add r5
    ldw (r0)
    ibt r1,#!fxrec_velocity
    with r1
    add r9
    to r1
    ldw (r1)
    sub r1
    add r0
    lm r6,(!fx_normal_x_w)
    fmult
    move r8,r0
    ibt r0,#!fxhit_velocity+2
    add r5
    ldw (r0)
    ibt r1,#!fxrec_velocity+4
    with r1
    add r9
    to r1
    ldw (r1)
    sub r1
    add r0
    lm r6,(!fx_normal_z_w)
    fmult
    with r8
    add r0
    moves r8,r8
    bmi .closing
    nop
    iwt r0,#$04B0
    cmp r8
    bmi .closing
    nop
    move r8,r0
.closing:
    sm (!fx_closing),r8
    %fx_push(r7)
    %fx_push(r9)
    %fx_push(r8)
    ibt r0,#!fxrec_mass
    add r7
    ldw (r0)
    ibt r6,#!fxrec_mass
    with r6
    add r9
    to r6
    ldw (r6)
    with r6
    add r0
    ibt r4,#0
    link #4
    iwt r15,#FxDivideHalvedIfLarge
    nop
    to r6
    from r4
    lsr
    %fx_pull(r8)
    %fx_pull(r9)
    %fx_pull(r7)
    move r0,r8
    add r0
    fmult
    move r8,r0
    lm r1,(!fx_normal_x_w)
    ibt r2,#0
    lm r3,(!fx_normal_z_w)
    moves r8,r8
    bmi .off
    sub r0
    ibt r0,#1
.off:
    sm (!fx_push),r0
    sm (!fx_push+2),r8
    sm (!fx_push+4),r1
    sm (!fx_push+6),r2
    sm (!fx_push+8),r3
    %fx_pull(r8)
    %fx_pull(r1)
    %fx_pull(r15)
    nop

; @routine 04:CA99 FxRecordsApart
; WHAT IT DOES: Whether records R8 and R7 stay apart this step: R0 = 1 when
; their distance on the ground plane is more than their sizes (+$0C each)
; less $294 (no less than 0, decoded from the ROM bytes and not reached in
; the traces so far) plus how fast they close (half the velocity
; difference along the line between them), else R0 = 0.
; HOW: The difference (R7 - R8) goes to $0068/$002E and FxNormalize makes
; the unit vector; the distance is the difference, doubled, dotted with
; it (FMULT) and kept at $0068; the closing speed is the velocity
; difference halved (DIV2) dotted with it, negated, at $006A. R1-R5 and
; the return address are kept on the R10 stack, R8 and R7 across the call.
; Entry: R7, R8 = the records; R10 = stack; R11 = return address. Exit: R0
; = 1 or 0; R1-R5, R7, R8 as on entry; R6 = the unit z; R9, R11-R13 as
; FxNormalize leaves them; S, Z from R10; OV, CY from the CMP (R0 = 1) or
; SUB R0 (R0 = 0); the last RAM address R10.
%at($04CA99)
FxRecordsApart:
    %fx_push(r11)
    %fx_push(r1)
    %fx_push(r2)
    %fx_push(r3)
    %fx_push(r4)
    %fx_push(r5)
    ibt r1,#!fxrec_half_x
    with r1
    add r7
    to r1
    ldw (r1)
    ibt r0,#!fxrec_half_x
    add r8
    ldw (r0)
    with r1
    sub r0
    sm (!fx_normal_x),r1
    ibt r3,#!fxrec_half_x+4
    with r3
    add r7
    to r3
    ldw (r3)
    ibt r0,#!fxrec_half_x+4
    add r8
    ldw (r0)
    with r3
    sub r0
    sm (!fx_normal_z),r3
    ibt r2,#0
    %fx_push(r8)
    %fx_push(r7)
    link #4
    iwt r15,#FxNormalize
    nop
    %fx_pull(r7)
    %fx_pull(r8)
    lm r0,(!fx_normal_x)
    add r0
    move r6,r1
    fmult
    move r5,r0
    lm r0,(!fx_normal_z)
    add r0
    move r6,r3
    fmult
    add r5
    sm (!fx_normal_x),r0
    ibt r0,#!fxrec_velocity
    add r7
    ldw (r0)
    ibt r4,#!fxrec_velocity
    with r4
    add r8
    to r4
    ldw (r4)
    sub r4
    div2
    move r6,r1
    fmult
    move r5,r0
    ibt r0,#!fxrec_velocity+4
    add r7
    ldw (r0)
    ibt r4,#!fxrec_velocity+4
    with r4
    add r8
    to r4
    ldw (r4)
    sub r4
    div2
    move r6,r3
    fmult
    add r5
    not
    inc r0
    sm (!fx_reach),r0
    ibt r0,#!fxrec_size
    add r7
    ldw (r0)
    ibt r1,#!fxrec_size
    with r1
    add r8
    to r1
    ldw (r1)
    add r1
    iwt r1,#$0294
    sub r1
    bpl .slack
    nop
    sub r0
.slack:
    lm r1,(!fx_reach)
    add r1
    lm r1,(!fx_normal_x)
    cmp r1
    bmi .apart
    nop
    sub r0
    %fx_pull(r5)
    %fx_pull(r4)
    %fx_pull(r3)
    %fx_pull(r2)
    %fx_pull(r1)
    %fx_pull(r15)
    nop
.apart:
    ibt r0,#1
    %fx_pull(r5)
    %fx_pull(r4)
    %fx_pull(r3)
    %fx_pull(r2)
    %fx_pull(r1)
    %fx_pull(r15)
    nop

; @routine 04:A345 FxContactNote
; WHAT IT DOES: Notes that record R6 touches record R8: finds R8's contact
; on R6's list or makes one (a $12-byte block from the first pool, on the
; front of the list, +$0C = R8, +$10 = 0), copies R8's velocity x, z and
; position x, z into it unless bit 2 of its +$10 is set, and sets bit 0 of
; +$10. Nothing is done when bit 7 of R6's +$16 is set. With no room in
; the pool nothing is noted (decoded from the ROM bytes, as is the
; whole-block case of the allocation below, and not reached in the traces
; so far).
; HOW: The allocation is FxPoolTake's done again inline, with R7 the size,
; R9 the walk and R8 (kept on the R10 stack) the link: an exact fit stays
; on the chain with its size negated, else the block is cut from the end
; of a bigger one. The routine runs under CACHE (the code cache only;
; nothing the C has to do).
; Entry: R6 = the record; R8 = the other; R10 = stack; R11 = return
; address. Exit: R7 = the contact; R0 = its new +$10; R9 = 1; S, Z from
; the OR; the last RAM address the contact + $10. Bit 7 set: R0 = R6's
; +$16 and $80; R9 = $80.
%at($04A345)
FxContactNote:
    ibt r0,#!fxrec_flags16
    add r6
    ldw (r0)
    iwt r9,#$0080
    and r9
    beq .search
    nop
    iwt r15,#.done
    nop
.search:
    ibt r7,#!fxrec_list
    with r7
    add r6
    to r7
    ldw (r7)
    moves r7,r7
    beq .new
    nop
.contact:
    ibt r0,#!fxcon_other
    add r7
    ldw (r0)
    cmp r8
    bne .next
    nop
    iwt r15,#.found
    nop
.next:
    ibt r0,#!fxrec_next
    add r7
    ldw (r0)
    moves r7,r0
    bne .contact
    nop
.new:
    %fx_push(r8)
    iwt r9,#!fx_pool_first
    cache
    ibt r7,#!fx_contact_bytes
    %fx_push(r9)
.exact:
    to r8
    ldw (r9)
    inc r9
    inc r9
    ldw (r9)
    sub r7
    beq .exact_found
    nop
    moves r9,r8
    bne .exact
    nop
    %fx_pull(r9)
.bigger:
    to r8
    ldw (r9)
    inc r9
    inc r9
    ldw (r9)
    sub r7
    bpl .bigger_found
    nop
    moves r9,r8
    bne .bigger
    nop
    ibt r7,#0
    bra .taken
    nop
.exact_found:
    dec r10
    dec r10
    ldw (r10)
    with r7
    not
    inc r7
    sub r0
    bra .mark
    nop
.bigger_found:
    sub #4
    bpl .split
    nop
    add #4
    with r7
    add r0
    with r7
    not
    inc r7
    ibt r0,#-4
.split:
    add #4
    stw (r9)
.mark:
    from r9
    to r8
    add r0
    from r7
    stw (r8)
    dec r8
    dec r8
    move r7,r8
.taken:
    %fx_pull(r8)
    moves r7,r7
    bne .link
    nop
    iwt r15,#.no_room
    nop
.link:
    nop
    iwt r9,#!fx_contact_bytes
    ibt r0,#!fxcon_other
    add r7
    from r8
    stw (r0)
    ibt r9,#!fxrec_list
    with r9
    add r6
    to r9
    ldw (r9)
    ibt r0,#!fxrec_next
    add r7
    from r9
    stw (r0)
    ibt r0,#!fxrec_list
    add r6
    from r7
    stw (r0)
    ibt r9,#0
    ibt r0,#!fxrec_flags10
    add r7
    from r9
    stw (r0)
.found:
    ibt r9,#!fxrec_flags10
    with r9
    add r7
    to r9
    ldw (r9)
    iwt r0,#$0004
    and r9
    bne .seen
    nop
    ibt r9,#!fxrec_velocity
    with r9
    add r8
    to r9
    ldw (r9)
    ibt r0,#!fxhit_velocity
    add r7
    from r9
    stw (r0)
    ibt r9,#!fxrec_velocity+4
    with r9
    add r8
    to r9
    ldw (r9)
    ibt r0,#!fxhit_velocity+2
    add r7
    from r9
    stw (r0)
    ibt r9,#!fxrec_half_x
    with r9
    add r8
    to r9
    ldw (r9)
    ibt r0,#!fxhit_x
    add r7
    from r9
    stw (r0)
    ibt r9,#!fxrec_half_x+4
    with r9
    add r8
    to r9
    ldw (r9)
    ibt r0,#!fxhit_x+2
    add r7
    from r9
    stw (r0)
.seen:
    ibt r0,#!fxrec_flags10
    add r7
    ldw (r0)
    iwt r9,#$0001
    or r9
    sbk
.done:
    move r15,r11
    nop
.no_room:
    move r15,r11
    nop

; Whether, on one axis, |R2's <pos> - <b>| < R2's <size> + R1's (at
; <mine>), from the sign of the last SUB.
macro fx_axis(size, mine, pos, b)
    ibt r6,#<size>
    with r6
    add r2
    to r6
    ldw (r6)
    lm r0,(<mine>)
    with r6
    add r0
    ibt r0,#<pos>
    add r2
    ldw (r0)
    sub <b>
    bpl ?positive
    nop
    not
    inc r0
?positive:
    sub r6
endmacro

; @routine 04:A1EC FxAllTouches
; WHAT IT DOES: FxRecordTouches for every record on the list of all
; records ($2332), so each pair is tested once.
; HOW: R1 walks the list through +$04; the return address is kept on the
; R10 stack. The routine runs under CACHE (the code cache only; nothing
; the C has to do).
; Entry: R10 = stack; R11 = return address. Exit: R0 = R1 = 0; the others
; from the last FxRecordTouches; S, Z from R10; the last RAM address R10.
%at($04A1EC)
FxAllTouches:
    %fx_push(r11)
    lm r0,(!fx_records)
    cache
.record:
    moves r1,r0
    beq .done
    nop
    link #4
    iwt r15,#FxRecordTouches
    nop
    ibt r0,#!fxrec_all_next
    add r1
    ldw (r0)
    bra .record
    nop
.done:
    %fx_pull(r15)
    nop

; @routine 04:A20B FxRecordTouches
; WHAT IT DOES: Tests record R1 against every record after it on the list
; of all records ($2332, linked through +$04): their boxes overlap when on
; each axis (z, then x, then y) the distance between the positions is
; below the two sizes (+$10, +$0C, +$0E) added. For an overlapping pair:
; when both have bit 1 of +$16 set and FxRecordsApart says they stay
; apart, nothing; otherwise each notes the other as a contact
; (FxContactNote) unless its +$1A shares a bit with the other's +$16.
; Then, when R1 has contacts, FxListSweep drops those not seen this step.
; HOW: R1's position goes to R3-R5 and its sizes to $236C/$236E/$2370;
; R2 walks the list. The return address is kept on the R10 stack.
; Entry: R1 = the record; R10 = stack; R11 = return address. Exit: R2 = 0;
; R1, R3-R5 as above; the others from the last test or call; S, Z from
; R10; the last RAM address R10.
%at($04A20B)
FxRecordTouches:
    %fx_push(r11)
    ibt r3,#!fxrec_half_x
    with r3
    add r1
    to r3
    ldw (r3)
    ibt r4,#!fxrec_half_x+2
    with r4
    add r1
    to r4
    ldw (r4)
    ibt r5,#!fxrec_half_x+4
    with r5
    add r1
    to r5
    ldw (r5)
    ibt r0,#!fxrec_size
    add r1
    ldw (r0)
    sm (!fx_sizes),r0
    ibt r0,#!fxrec_size+2
    add r1
    ldw (r0)
    sm (!fx_sizes+2),r0
    ibt r0,#!fxrec_size+4
    add r1
    ldw (r0)
    sm (!fx_sizes+4),r0
    move r2,r1
.other:
    ibt r0,#!fxrec_all_next
    add r2
    ldw (r0)
    moves r2,r0
    bne .test
    nop
    iwt r15,#.sweep
    nop
.test:
    %fx_axis(!fxrec_size+4, !fx_sizes+4, !fxrec_half_x+4, r5)
    bpl .other
    nop
    %fx_axis(!fxrec_size, !fx_sizes, !fxrec_half_x, r3)
    bpl .other
    nop
    %fx_axis(!fxrec_size+2, !fx_sizes+2, !fxrec_half_x+2, r4)
    bmi .overlap
    nop
    iwt r15,#.other
    nop
.overlap:
    ibt r0,#!fxrec_flags16
    add r1
    ldw (r0)
    ibt r6,#!fxrec_flags16
    with r6
    add r2
    to r6
    ldw (r6)
    and r6
    iwt r6,#$0002
    and r6
    beq .note
    nop
    move r7,r1
    move r8,r2
    link #4
    iwt r15,#FxRecordsApart
    nop
    moves r0,r0
    bne .next
    nop
.note:
    ibt r0,#!fxrec_mode
    add r1
    ldw (r0)
    ibt r6,#!fxrec_flags16
    with r6
    add r2
    to r6
    ldw (r6)
    and r6
    bne .note_other
    nop
    move r6,r1
    move r8,r2
    link #4
    iwt r15,#FxContactNote
    nop
.note_other:
    ibt r0,#!fxrec_mode
    add r2
    ldw (r0)
    ibt r6,#!fxrec_flags16
    with r6
    add r1
    to r6
    ldw (r6)
    and r6
    bne .next
    nop
    move r6,r2
    move r8,r1
    link #4
    iwt r15,#FxContactNote
    nop
.next:
    iwt r15,#.other
    nop
.sweep:
    ibt r0,#!fxrec_list
    add r1
    ldw (r0)
    moves r0,r0
    beq .done
    nop
    move r6,r1
    link #4
    iwt r15,#FxListSweep
    nop
.done:
    %fx_pull(r15)
    nop

arch 65816
