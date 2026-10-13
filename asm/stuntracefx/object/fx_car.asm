; fx_car.asm: the Super FX sets up a car's record from a script in ROM.
;
; The script is read through the ROM buffer from R14 (in ROMBR's bank): a
; byte giving the number of parts, a word for the parts' mode, seven words
; per part, then a byte choosing one of the 5-byte entries that follow.
; Each part is a $52-byte record of its own on the car's list of parts:
; the car's +$3E is the first part, a part's +$34 the next.
; C: src/stuntracefx/object/fx_car.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; A word of the script into R3.
macro fx_script_word()
    to r3
    getb
    inc r14
    with r3
    getbh
    inc r14
endmacro

; Field <field> of record <rec> = R3, through R0.
macro fx_field_set(field, rec)
    ibt r0,#<field>
    add <rec>
    from r3
    stw (r0)
endmacro

; @routine 04:8E84 FxCarAddPart
; WHAT IT DOES: Adds a part to car R1: a new $52-byte record from the first
; pool whose +$12, +$3C (negated), +$3E, +$40, +$44, +$50 and +$46 are the
; next seven script words; its +$0C, +$0E, +$10 = $14, bit 2 of +$16 set,
; +$1A = the parts' mode ($0068), +$2A = the car's +$2A; it goes on the
; front of the car's parts list. With no room the script is not read
; (decoded from the ROM bytes and not reached in the traces so far).
; HOW: FxPoolTake; each field is STW through R0 = field + R2 with R3 the
; value. R1 and the return address are kept on the R10 stack.
; Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
; return address. Exit: R1 = the car; R2 = the part (0 without); R3 = the
; car's +$2A; R0 = the part + $2A; R14 past the seven words; R11 = $8E97;
; R4, R12, R13 as FxPoolTake leaves them; S, Z from R10, OV, CY from the
; last ADD; the last RAM address R10.
%at($048E84)
FxCarAddPart:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r1
    stw (r10)
    inc r10
    inc r10
    iwt r4,#!fx_part_bytes
    iwt r3,#!fx_pool_first
    link #4
    iwt r15,#FxPoolTake
    nop
    move r2,r1
    dec r10
    dec r10
    to r1
    ldw (r10)
    moves r2,r2
    bne .part
    nop
    iwt r15,#.done
    nop
.part:
    %fx_script_word()
    %fx_field_set(!fxpart_12, r2)
    %fx_script_word()
    with r3
    not
    inc r3
    %fx_field_set(!fxpart_3c, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_3c+2, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_3c+4, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_44, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_50, r2)
    %fx_script_word()
    %fx_field_set(!fxpart_46, r2)
    ibt r3,#!fxcar_parts
    with r3
    add r1
    to r3
    ldw (r3)
    %fx_field_set(!fxpart_next, r2)
    ibt r0,#!fxcar_parts
    add r1
    from r2
    stw (r0)
    iwt r3,#$0014
    %fx_field_set(!fxpart_0c, r2)
    %fx_field_set(!fxpart_0c+2, r2)
    %fx_field_set(!fxpart_0c+4, r2)
    ibt r3,#!fxrec_flags16
    with r3
    add r2
    to r3
    ldw (r3)
    iwt r0,#$0004
    to r3
    or r3
    %fx_field_set(!fxrec_flags16, r2)
    lm r3,(!fx_part_mode)
    %fx_field_set(!fxrec_mode, r2)
    ibt r3,#!fxrec_mass
    with r3
    add r1
    to r3
    ldw (r3)
    %fx_field_set(!fxrec_mass, r2)
.done:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:8F48 FxCarFromScript
; WHAT IT DOES: Sets up car R1 from the script at R14: its parts
; (FxCarAddPart for each), $2392 = where the entries start less $8090, +$6E
; = the entry the next byte chooses (5 bytes each), and +$6A = $0180, +$8C
; = $1000, +$8A = the word at $26F2, +$7A = 0 (laps), +$86 = 2 and +$58 = 1
; (segments).
; HOW: The part count goes to R12 for LOOP, with R13 and R12 kept on the
; R10 stack around each call; the entry is LMULT of the byte by 5 added to
; R14. The return address is kept on the R10 stack.
; Entry: R1 = the car's record; R14 = the script; R10 = stack; R11 =
; return address. Exit: R14 = R2 = the chosen entry; R0 = R1 + $58; R4 =
; the byte times 5; R6 = 5; R11 = $8E97 (with parts); R12 = 0; R13 =
; $8F61; S, Z from R10, OV, CY from the last ADD; the last RAM address R10.
%at($048F48)
FxCarFromScript:
    from r11
    stw (r10)
    inc r10
    inc r10
    sub r0
    getb
    inc r14
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    sm (!fx_part_mode),r2
    moves r12,r0
    beq .parts_done
    nop
    move r13,r15
    from r13
    stw (r10)
    inc r10
    inc r10
    from r12
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxCarAddPart
    nop
    dec r10
    dec r10
    to r12
    ldw (r10)
    dec r10
    dec r10
    to r13
    ldw (r10)
    loop
    nop
.parts_done:
    iwt r0,#$8090
    from r14
    sub r0
    sm (!fx_script_at),r0
    sub r0
    getb
    inc r14
    ibt r6,#5
    lmult
    with r14
    add r4
    move r2,r14
    ibt r0,#!fxcar_entry
    add r1
    from r2
    stw (r0)
    iwt r2,#$0180
    ibt r0,#!fxrec_6a
    add r1
    from r2
    stw (r0)
    iwt r2,#$1000
    iwt r0,#!fxcar_8c
    add r1
    from r2
    stw (r0)
    lm r2,(!fx_car_26f2)
    iwt r0,#!fxcar_8c-2
    add r1
    from r2
    stw (r0)
    iwt r2,#$0000
    ibt r0,#!fxrec_laps
    add r1
    from r2
    stw (r0)
    iwt r2,#$0002
    iwt r0,#!fxrec_counted
    add r1
    from r2
    stw (r0)
    iwt r2,#$0001
    ibt r0,#!fxrec_segment
    add r1
    from r2
    stw (r0)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; The row at offsets <a>, <b>, <c> of the matrix at $00E4 times (R1, R2, R3), FMULT then ROL each,
; stored at R11, which steps on.
macro fx_turn_row(a, b, c)
    lms r6,(!fx_matrix_w+<a>)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_matrix_w+<b>)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_matrix_w+<c>)
    from r3
    fmult
    rol
    add r5
    stw (r11)
    inc r11
    inc r11
endmacro

; The axis at word offset <o> of a part: position = the car's + the turned offset; velocity
; = the change times 8.
macro fx_part_axis(o)
    ibt r1,#!fxpart_world+<o>
    with r1
    add r9
    to r1
    ldw (r1)
    lm r0,(!fx_car_pos+<o>)
    ibt r2,#!fxrec_half_x+<o>
    with r2
    add r9
    to r2
    ldw (r2)
    with r1
    add r0
    ibt r0,#!fxrec_half_x+<o>
    add r9
    from r1
    stw (r0)
    with r1
    sub r2
    with r1
    lmult
    ibt r0,#!fxrec_velocity+<o>
    add r9
    from r4
    stw (r0)
endmacro

; @routine 04:A4ED FxCarMoveParts
; WHAT IT DOES: Moves each part of car R8 with it: the part's offset
; (+$3C/+$3E/+$40, in the car's axes) turned into world axes by the
; matrix at $00E4 goes to +$36/+$38/+$3A; its position +$1E/+$20/+$22
; becomes the car's ($2354/$2356/$2358) plus that, and its velocity
; +$2E/+$30/+$32 eight times the change. A part with bit 10 of +$46 set
; gets velocity 0 instead (decoded from the ROM bytes, with four bytes
; after the walk's jump and one after this path's that nothing reaches,
; and not reached in the traces so far).
; HOW: R12 keeps R8 while R8 and R11 walk the offset and its turned copy;
; the turn is three rows of FMULT, ROL; the change times 8 is LMULT by R6
; = 8 (R4 takes the low word). The return address is kept on the R10
; stack. The routine runs under CACHE (the code cache only; nothing the
; C has to do).
; Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
; R9 = 0; R8 = R12 = the car (R12 as on entry without parts); the others
; from the last part; S, Z from R10; the last RAM address R10.
%at($04A4ED)
FxCarMoveParts:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r0,#!fxcar_parts
    add r8
    ldw (r0)
    cache
.part:
    moves r9,r0
    bne .move
    nop
    iwt r15,#.done
    nop
.move:
    nop
    ibt r1,#!fxpart_46
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$0400
    and r1
    beq .follow
    nop
    iwt r15,#.still
    nop
.follow:
    move r12,r8
    iwt r8,#!fxpart_3c
    with r8
    add r9
    iwt r11,#!fxpart_world
    with r11
    add r9
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    inc r8
    inc r8
    to r3
    ldw (r8)
    inc r8
    inc r8
    %fx_turn_row(0, 6, 12)
    %fx_turn_row(2, 8, 14)
    %fx_turn_row(4, 10, 16)
    move r8,r12
    iwt r6,#$0008
    %fx_part_axis(0)
    %fx_part_axis(2)
    %fx_part_axis(4)
    ibt r0,#!fxpart_next
    add r9
    ldw (r0)
    iwt r15,#.part
    nop
    nop
    bra .done
    nop
.still:
    ibt r1,#0
    ibt r0,#!fxrec_velocity
    add r9
    from r1
    stw (r0)
    ibt r0,#!fxrec_velocity+2
    add r9
    from r1
    stw (r0)
    ibt r0,#!fxrec_velocity+4
    add r9
    from r1
    stw (r0)
    ibt r0,#!fxpart_next
    add r9
    ldw (r0)
    iwt r15,#.part
    nop
    nop
.done:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:B9AC FxCarPose
; WHAT IT DOES: Sets up the matrix at $00E4 for car R8 (from its angles,
; negated: +$26, +$24, +$28 as angles 0-2, by FxMatrixFromAngles, then
; FxMatrixTranspose04) and moves its parts with it (FxCarMoveParts from its
; position, copied to $2354).
; HOW: The negated angles go to $0020/$0022/$0024 (NOT, INC, SMS); R8 and
; the return address are kept on the R10 stack.
; Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
; as FxCarMoveParts leaves them (R11 = $BA00 for a car without parts); S,
; Z from R10; the last RAM address R10.
%at($04B9AC)
FxCarPose:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    ibt r0,#!fxcar_yaw
    add r8
    ldw (r0)
    not
    inc r0
    sms (!fx_angles_w),r0
    ibt r0,#!fxcar_pitch
    add r8
    ldw (r0)
    not
    inc r0
    sms (!fx_angles_w+2),r0
    ibt r0,#!fxcar_roll
    add r8
    ldw (r0)
    not
    inc r0
    sms (!fx_angles_w+4),r0
    iwt r9,#!fx_matrix_w
    link #4
    iwt r15,#FxMatrixFromAngles
    nop
    iwt r9,#!fx_matrix_w
    link #4
    iwt r15,#FxMatrixTranspose04
    nop
    dec r10
    dec r10
    to r8
    ldw (r10)
    ibt r0,#!fxrec_half_x
    add r8
    ldw (r0)
    sm (!fx_car_pos),r0
    ibt r0,#!fxrec_half_x+2
    add r8
    ldw (r0)
    sm (!fx_car_pos+2),r0
    ibt r0,#!fxrec_half_x+4
    add r8
    ldw (r0)
    sm (!fx_car_pos+4),r0
    link #4
    iwt r15,#FxCarMoveParts
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; A word of the script into R2 and field <f> of the new car R1 = it.
macro fx_car_word(f)
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    ibt r0,#<f>
    add r1
    from r2
    stw (r0)
endmacro

; Field <f> of the new car R1 = R2.
macro fx_car_r2(f)
    ibt r0,#<f>
    add r1
    from r2
    stw (r0)
endmacro

; @routine 04:8CC6 FxCarNew
; WHAT IT DOES: A program: makes a car from the script whose address (in
; bank 9) the S-CPU left at $238E: a $98-byte record from the first pool
; (its address back to $238E, 0 when there is no room: decoded from the
; ROM bytes and not reached in the traces so far), set up from $2392,
; $0068/$002C/$002E (the position), $0022, $2390 and the script's own
; words, put on the front of the cars' list ($262E, +$74) and noted at
; $2474; then FxCarFromScript and FxCarPose. A script not among the nine
; known ones is noted at $2650 and $81B9 used instead (decoded from the
; ROM bytes and not reached in the traces so far, as are the matches of
; scripts $8669 and $8886).
; HOW: ROMB 9 and R14 = the script; nine CMPs with the known addresses.
; The script gives two words, of which the second is used for $2390 when
; the low byte of $108C is not 0, a byte for $2394, a word skipped, words
; for +$5A, +$5C, +$16, +$40, +$1A, +$0C, +$0E, +$10, then (after $2396 =
; the script offset from $8090 and 10 bytes skipped) +$2A and +$14. The
; record also gets +$96 = $2392, +$24 = $0022, +$64 = the old $2390, +$66 =
; the script, +$0A = $B1A1.
; Entry: the S-CPU starts it. Exit: (at its STOP) R1 = R8 = the car; R10
; = $0240; the others as FxCarPose leaves them.
%at($048CC6)
FxCarNew:
    iwt r10,#!fx_stack
    iwt r0,#$0009
    romb
    lm r14,(!gsu_record_arg)
    iwt r0,#$81B9
    cmp r14
    bne .not_0
    nop
    iwt r15,#.known
    nop
.not_0:
    nop
    iwt r0,#$853D
    cmp r14
    bne .not_1
    nop
    iwt r15,#.known
    nop
.not_1:
    nop
    iwt r0,#$82E5
    cmp r14
    bne .not_2
    nop
    iwt r15,#.known
    nop
.not_2:
    nop
    iwt r0,#$8411
    cmp r14
    bne .not_3
    nop
    iwt r15,#.known
    nop
.not_3:
    nop
    iwt r0,#$8669
    cmp r14
    bne .not_4
    nop
    iwt r15,#.known
    nop
.not_4:
    nop
    iwt r0,#$876E
    cmp r14
    bne .not_5
    nop
    iwt r15,#.known
    nop
.not_5:
    nop
    iwt r0,#$8AA8
    cmp r14
    bne .not_6
    nop
    iwt r15,#.known
    nop
.not_6:
    nop
    iwt r0,#$8886
    cmp r14
    bne .not_7
    nop
    iwt r15,#.known
    nop
.not_7:
    nop
    iwt r0,#$8990
    cmp r14
    bne .unknown
    nop
    iwt r15,#.known
    nop
.unknown:
    nop
    sm (!fx_car_bad),r14
    iwt r14,#$81B9
.known:
    iwt r4,#$0098
    iwt r3,#!fx_pool_first
    link #4
    iwt r15,#FxPoolTake
    nop
    sm (!gsu_record_arg),r1
    moves r1,r1
    bne .made
    nop
    iwt r15,#.done
    nop
.made:
    lm r2,(!fx_car_2392)
    iwt r0,#$0096
    add r1
    from r2
    stw (r0)
    lm r2,(!fx_normal_x)
    %fx_car_r2(!fxrec_half_x)
    lm r2,(!fx_car_002c)
    %fx_car_r2(!fxrec_half_x+2)
    lm r2,(!fx_normal_z)
    %fx_car_r2(!fxrec_half_x+4)
    lm r2,(!fx_car_0022)
    %fx_car_r2(!fxcar_pitch)
    lm r2,(!gsu_in_amount)
    %fx_car_r2(!fxcar_64)
    move r2,r14
    %fx_car_r2(!fxcar_script)
    iwt r2,#$B1A1
    %fx_car_r2(!fxcar_0a)
    to r2
    getb
    inc r14
    with r2
    getbh
    inc r14
    to r3
    getb
    inc r14
    with r3
    getbh
    inc r14
    sm (!gsu_in_amount),r2
    lm r0,(!fx_car_108c)
    lob
    beq .amount
    nop
    sm (!gsu_in_amount),r3
.amount:
    getb
    inc r14
    sm (!fx_car_2394),r0
    getb
    inc r14
    getbh
    inc r14
    %fx_car_word($5A)
    %fx_car_word($5C)
    %fx_car_word(!fxrec_flags16)
    %fx_car_word(!fxrec_flags40)
    %fx_car_word(!fxrec_mode)
    %fx_car_word(!fxrec_size)
    %fx_car_word(!fxrec_size+2)
    %fx_car_word(!fxrec_size+4)
    iwt r0,#$8090
    from r14
    sub r0
    sm (!fx_car_2396),r0
    with r14
    add #10
    %fx_car_word(!fxrec_mass)
    %fx_car_word($14)
    lm r0,(!fx_cars)
    moves r2,r0
    bne .link
    nop
    sm (!fx_cars),r1
    bra .script
    nop
.link:
    ibt r0,#!fxrec_next_car
    add r1
    from r2
    stw (r0)
    sm (!fx_cars),r1
.script:
    link #4
    iwt r15,#FxCarFromScript
    nop
    sm (!fx_car_last),r1
    move r8,r1
    link #4
    iwt r15,#FxCarPose
    nop
.done:
    stop
    nop

; @routine 04:BB17 FxCarHit
; WHAT IT DOES: The damage from car R8's hit this step. Nothing for a car
; with bit 3 of +$64 set; +$62 = 0 for one with any of bits $2201 of +$40
; (or bit 9 of +$8A, decoded from the ROM bytes and not reached in the
; traces so far). Otherwise +$62 = the hit's y ($2336) / 4, and the impact
; is |that| (a quarter of it when +$44 is below 2; 0 when +$30 is negative)
; plus |hit x| / 8 and |hit z| / 8, scaled by $2678 (FMULT), halved for a
; car with contacts; it is added to $263E (cleared first when $263C is 0).
; With $263C set, half the sum, when 16 or more (96 at most, decoded from
; the ROM bytes and not reached in the traces so far), is added to the
; damage +$82 and sets bit 7 of +$40 (below 24), bit 7 of +$70 and bit 6 of
; +$40 (below 56) or bit 5 (else); bit 5 for the player's car ($2476) also
; pushes the cars near it apart (FxSpreadCars). Damage of $200 or more sets
; bit 0 of +$40.
; HOW: DIV2s for the halvings, CMP and the sign for the limits; the hit
; sum is LM, ADD, SBK. R11 is kept on the R10 stack around FxSpreadCars.
; Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
; R1 = the impact (halved); R2, R0 and the flags from the last test; the
; last RAM address the last field changed.
%at($04BB17)
FxCarHit:
    ibt r1,#!fxcar_64
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0008
    and r1
    beq .counts
    nop
    iwt r15,#.done
    nop
.counts:
    ibt r1,#!fxrec_flags40
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$2201
    and r1
    beq .not_out
    nop
    iwt r15,#.no_impact
    nop
.not_out:
    iwt r1,#!fxcar_8c-2
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0200
    and r1
    beq .impact
    nop
    iwt r15,#.no_impact
    nop
.impact:
    lm r0,(!fx_hit+2)
    div2
    div2
    move r1,r0
    ibt r0,#!fxcar_impact
    add r8
    from r1
    stw (r0)
    move r0,r1
    moves r0,r0
    bpl .y
    nop
    not
    inc r0
.y:
    ibt r1,#!fxcar_44
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r2,#$0002
    with r1
    cmp r2
    bpl .y_full
    nop
    div2
    div2
.y_full:
    move r1,r0
    ibt r0,#!fxrec_velocity+2
    add r8
    ldw (r0)
    moves r0,r0
    bpl .rising
    nop
    ibt r1,#0
.rising:
    lm r0,(!fx_hit)
    moves r0,r0
    bpl .x
    nop
    not
    inc r0
.x:
    div2
    div2
    div2
    with r1
    add r0
    lm r0,(!fx_hit+4)
    moves r0,r0
    bpl .z
    nop
    not
    inc r0
.z:
    div2
    div2
    div2
    with r1
    add r0
    lm r6,(!fx_hit_scale)
    with r1
    fmult
    ibt r0,#!fxrec_list
    add r8
    ldw (r0)
    moves r0,r0
    beq .alone
    nop
    with r1
    div2
.alone:
    lm r2,(!fx_hit_on)
    moves r2,r2
    bne .add
    sub r0
    sm (!fx_hit_sum),r0
.add:
    lm r0,(!fx_hit_sum)
    add r1
    sbk
    move r1,r0
    moves r2,r2
    bne .damage
    nop
    iwt r15,#.done
    nop
.damage:
    with r1
    div2
    iwt r2,#$0010
    with r1
    cmp r2
    bpl .hard
    nop
    iwt r15,#.done
    nop
.hard:
    iwt r2,#$0060
    with r1
    cmp r2
    bmi .capped
    nop
    move r1,r2
.capped:
    iwt r0,#!fxrec_82
    add r8
    ldw (r0)
    add r1
    sbk
    iwt r2,#$0018
    with r1
    cmp r2
    bmi .light
    nop
    ibt r0,#!fxrec_direction
    add r8
    ldw (r0)
    iwt r2,#$0080
    or r2
    sbk
    iwt r2,#$0038
    with r1
    cmp r2
    bmi .medium
    nop
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r2,#$0020
    or r2
    bra .severity
    sbk
.light:
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r2,#$0080
    or r2
    bra .severity
    sbk
.medium:
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r2,#$0040
    or r2
    sbk
.severity:
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r2,#$0020
    and r2
    beq .wrecked
    nop
    lm r0,(!fx_player)
    cmp r8
    bne .wrecked
    nop
    from r11
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxSpreadCars
    nop
    dec r10
    dec r10
    to r11
    ldw (r10)
.wrecked:
    iwt r0,#!fxrec_82
    add r8
    ldw (r0)
    iwt r2,#$0200
    cmp r2
    bmi .done
    nop
    ibt r2,#!fxrec_flags40
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0001
    or r2
    sbk
.done:
    move r15,r11
    nop
.no_impact:
    ibt r1,#0
    ibt r0,#!fxcar_impact
    add r8
    from r1
    stw (r0)
    move r15,r11
    nop

arch 65816
