; fx_physics.asm: the Super FX physics program (04:8FCA), the record
; handlers, collisions and contacts, a car's parts and its course.
; C: src/stuntracefx/object/fx_physics.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

;
; @routine 04:8FCA FxPhysicsProgram
; WHAT IT DOES: A program the S-CPU starts once a race frame: steps
; the race timers ($2472 down to 0, $2470 up), notes every touch
; between records (FxAllTouches, unless $2472 is still running), then
; runs each record of the list at $2332: one with a handler (+$0A) has
; it run (twice when +$1C bit 3 is set, $263C 0 then 1); one without
; is freed when its +$1C bit 2 is clear (FxObjectFree with its +$04),
; else that bit is cleared. Then the cars are ranked (FxRankCars) and
; the nearest found (FxNearestCar), and STOP. Nothing runs when $246C
; is 0.
; HOW: R10 = $0240; ROMB and RAMB 0; $246E = $FFFF while it runs; the
; list walked by +$04 (R8 = the record); a handler is reached by LINK
; #2; JMP R12, so it returns to 04:9030 or 04:903D.
; Entry: started by the S-CPU.
; Exit: (at the STOP) $246E = 0; the rest as the last record's handler
; and FxNearestCar leave them.
%at($048FCA)
FxPhysicsProgram:
    iwt r10,#$0240
    sub r0
    romb
    ramb
    lm r0,(!fx_pushes+190-!gsu_ram)
    moves r0,r0
    bne .l8FDF
    nop
    iwt r15,#.l9078
    nop
.l8FDF:
    lm r0,(!fx_init_2472-!gsu_ram)
    moves r0,r0
    beq .l8FE9
    dec r0
    sbk
.l8FE9:
    lm r0,(!fx_init_2470-!gsu_ram)
    inc r0
    sbk
    iwt r0,#$FFFF
    sm (!fx_physics_busy-!gsu_ram),r0
    lm r0,(!fx_init_2472-!gsu_ram)
    moves r0,r0
    bne .l9004
    nop
    link #4
    iwt r15,#FxAllTouches
    nop
.l9004:
    lm r0,(!fx_records-!gsu_ram)
.l9008:
    moves r8,r0
    beq .l9069
    nop
    ibt r0,#$0A
    add r8
    ldw (r0)
    moves r0,r0
    beq .l903D
    nop
    move r12,r0
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r1,#$0008
    and r1
    beq .l9034
    nop
    from r12
    stw (r10)
    inc r10
    inc r10
    ibt r0,#$00
    sm (!fx_hit_on-!gsu_ram),r0
    link #2
    jmp r12
    nop
    dec r10
    dec r10
    to r12
    ldw (r10)
.l9034:
    ibt r0,#$01
    sm (!fx_hit_on-!gsu_ram),r0
    link #2
    jmp r12
    nop
.l903D:
    ibt r0,#$1C
    add r8
    ldw (r0)
    move r2,r0
    iwt r1,#$0004
    and r1
    bne .l905E
    nop
    ibt r0,#$04
    add r8
    ldw (r0)
    stw (r10)
    inc r10
    inc r10
    move r1,r8
    link #4
    iwt r15,#FxObjectFree
    nop
    dec r10
    dec r10
    ldw (r10)
    bra .l9008
    nop
.l905E:
    from r2
    bic r1
    sbk
    ibt r0,#$04
    add r8
    ldw (r0)
    bra .l9008
    nop
.l9069:
    link #4
    iwt r15,#FxRankCars
    nop
    link #4
    iwt r15,#FxNearestCar
    nop
    sub r0
    sm (!fx_physics_busy-!gsu_ram),r0
.l9078:
    stop
    nop
;
; @routine 04:907A FxObjectBounce
; WHAT IT DOES: A record handler: an object that falls and bounces:
; its y speed +$30 gains $78, it moves (FxRecordMove, R1 = 3) and
; collides (FxRecordCollide); hit from below ($239C positive) its y
; speed loses the hit, and on any hit its x and z speeds (+$2E, +$32)
; lose an eighth. Not reached in the traces so far.
; HOW: Pushes R11 and R8 on the R10 stack; R9 = R8; returns by pulling
; R15.
; Entry: R8 = the record; R10 = stack; R11 = return address.
; Exit: R8 back; the rest as FxRecordCollide and the damping leave
; them.
%at($04907A)
FxObjectBounce:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    move r9,r8
    ibt r1,#$30
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$0078
    to r1
    add r1
    ibt r0,#$30
    add r9
    from r1
    stw (r0)
    ibt r1,#$03
    link #4
    iwt r15,#FxRecordMove
    nop
    link #4
    iwt r15,#FxRecordCollide
    nop
    lm r0,(!fx_hit_depth-!gsu_ram)
    moves r0,r0
    beq .l90DF
    nop
    bmi .l90B9
    nop
    ibt r1,#$30
    with r1
    add r9
    to r1
    ldw (r1)
    with r1
    sub r0
    ibt r0,#$30
    add r9
    from r1
    stw (r0)
.l90B9:
    ibt r0,#$2E
    add r9
    ldw (r0)
    move r1,r0
    div2
    div2
    div2
    with r1
    sub r0
    ibt r0,#$2E
    add r9
    from r1
    stw (r0)
    ibt r0,#$32
    add r9
    ldw (r0)
    move r1,r0
    div2
    div2
    div2
    with r1
    sub r0
    ibt r0,#$32
    add r9
    from r1
    stw (r0)
.l90DF:
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:90E8 FxObjectMarkTouched
; WHAT IT DOES: A record handler: marks every record in this one's
; touch list (+$18, linked by +$0E) touched (bit 1 of +$10).
; HOW: A loop of LDW, OR, SBK; returns by JMP R11 (MOVE R15, R11).
; Entry: R8 = the record; R11 = return address.
; Exit: R0 = 0 at the list's end; R1 = 0; R2 = 2.
%at($0490E8)
FxObjectMarkTouched:
    move r8,r8
    ibt r1,#$18
    with r1
    add r8
    to r1
    ldw (r1)
    moves r1,r1
    beq .l9107
    nop
    iwt r2,#$0002
.l90F8:
    ibt r0,#$10
    add r1
    ldw (r0)
    or r2
    sbk
    ibt r0,#$0E
    add r1
    ldw (r0)
    moves r1,r0
    bne .l90F8
    nop
.l9107:
    move r15,r11
    nop
;
; @routine 04:910A FxObjectFall
; WHAT IT DOES: A record handler: an object that falls while it
; touches something: with a touch list (+$18) its y speed +$30 gains
; $78; then it moves (FxRecordMove, R1 = 3) and collides
; (FxRecordCollide); on a hit its x and z speeds lose a quarter.
; HOW: Pushes R11 and R8 on the R10 stack; R9 = R8; returns by pulling
; R15.
; Entry: R8 = the record; R10 = stack; R11 = return address.
; Exit: R8 back; the rest as FxRecordCollide and the damping leave
; them.
%at($04910A)
FxObjectFall:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    move r9,r8
    ibt r0,#$18
    add r9
    ldw (r0)
    moves r0,r0
    beq .l912D
    nop
    ibt r1,#$30
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$0078
    to r1
    add r1
    ibt r0,#$30
    add r9
    from r1
    stw (r0)
.l912D:
    ibt r1,#$03
    link #4
    iwt r15,#FxRecordMove
    nop
    link #4
    iwt r15,#FxRecordCollide
    nop
    lm r0,(!fx_hit_depth-!gsu_ram)
    moves r0,r0
    beq .l9164
    nop
    ibt r0,#$2E
    add r9
    ldw (r0)
    move r1,r0
    div2
    div2
    with r1
    sub r0
    ibt r0,#$2E
    add r9
    from r1
    stw (r0)
    ibt r0,#$32
    add r9
    ldw (r0)
    move r1,r0
    div2
    div2
    with r1
    sub r0
    ibt r0,#$32
    add r9
    from r1
    stw (r0)
.l9164:
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:916D FxObjectCollideOnly
; WHAT IT DOES: A record handler: only collides the record with what
; it touches (FxRecordCollide), without moving it. Not reached in the
; traces so far.
; HOW: Pushes R11 and R8 on the R10 stack; R9 = R8; returns by pulling
; R15.
; Entry: R8 = the record; R10 = stack; R11 = return address.
; Exit: R8 back; the rest as FxRecordCollide leaves them.
%at($04916D)
FxObjectCollideOnly:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    move r9,r8
    link #4
    iwt r15,#FxRecordCollide
    nop
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:9185 FxObjectDrop
; WHAT IT DOES: A record handler: an object dropped straight down: its
; y speed gains $3C, its x and z speeds are 0, it moves (FxRecordMove,
; R1 = 3) and collides keeping x and z (+$1E, +$22) from before; on a
; hit its handler (+$0A) is cleared, so it stops. Not reached in the
; traces so far.
; HOW: Pushes R11, R8 and the x and z on the R10 stack; returns by
; pulling R15.
; Entry: R8 = the record; R10 = stack; R11 = return address.
; Exit: R8 back; the rest as FxRecordCollide leaves them.
%at($049185)
FxObjectDrop:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    move r9,r8
    ibt r1,#$30
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$003C
    to r1
    add r1
    ibt r0,#$30
    add r9
    from r1
    stw (r0)
    ibt r1,#$00
    ibt r0,#$32
    add r9
    from r1
    stw (r0)
    ibt r0,#$32
    add r9
    from r1
    stw (r0)
    ibt r1,#$03
    link #4
    iwt r15,#FxRecordMove
    nop
    ibt r1,#$1E
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r3,#$22
    with r3
    add r9
    to r3
    ldw (r3)
    from r1
    stw (r10)
    inc r10
    inc r10
    from r3
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxRecordCollide
    nop
    dec r10
    dec r10
    to r3
    ldw (r10)
    dec r10
    dec r10
    to r1
    ldw (r10)
    lm r0,(!fx_hit_depth-!gsu_ram)
    moves r0,r0
    bne .l91EF
    nop
    ibt r0,#$1E
    add r9
    from r1
    stw (r0)
    ibt r0,#$22
    add r9
    from r3
    stw (r0)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.l91EF:
    ibt r1,#$00
    ibt r0,#$0A
    add r9
    from r1
    stw (r0)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:91FF FxRecordCollide
; WHAT IT DOES: Collides record R9 with each record in its touch list
; (+$18): with the other's model (FxCollideModel) unless the other's
; flags (+$16) have bit 4, else as a box (FxCollidePush); the pushes
; found are applied (FxPushesApply, FxPushApply), and $239C keeps the
; first hit's depth. Clears bit 0 of R9's +$1C first and bit 1 of each
; other's +$10.
; HOW: Pushes R11 and each list entry on the R10 stack; $247E and
; $239C cleared and the sums cleared (FxSumsClear) first; the next
; entry by +$0E; returns by pulling R15.
; Entry: R9 = the record; R10 = stack; R11 = return address.
; Exit: $239C = the hit depth (0: none); the rest as the last
; collision leaves them.
%at($0491FF)
FxRecordCollide:
    from r11
    stw (r10)
    inc r10
    inc r10
    sub r0
    sm (!fx_push_turn-!gsu_ram),r0
    sm (!fx_hit_depth-!gsu_ram),r0
    link #4
    iwt r15,#FxSumsClear
    nop
    iwt r3,#$0001
    ibt r0,#$1C
    add r9
    ldw (r0)
    bic r3
    sbk
    ibt r0,#$18
    add r9
    ldw (r0)
    moves r1,r0
    bne .l9228
    nop
    iwt r15,#.l92C5
    nop
.l9228:
    nop
.l9229:
    from r1
    stw (r10)
    inc r10
    inc r10
    ibt r7,#$0C
    with r7
    add r1
    to r7
    ldw (r7)
    sm (!fx_collide_model-!gsu_ram),r7
    ibt r0,#$10
    add r1
    ldw (r0)
    iwt r3,#$0002
    bic r3
    sbk
    sub r0
    sm (!fx_push_amount-!gsu_ram),r0
    ibt r0,#$16
    add r7
    ldw (r0)
    iwt r2,#$0010
    and r2
    bne .l926A
    nop
    link #4
    iwt r15,#FxCollideModel
    nop
    moves r0,r0
    beq .l92B4
    nop
    link #4
    iwt r15,#FxPushesApply
    nop
    lm r2,(!fx_push_amount-!gsu_ram)
    with r2
    not
    inc r2
    bra .l927B
    nop
.l926A:
    link #4
    iwt r15,#FxCollidePush
    nop
    iwt r3,#$23A4
    link #4
    iwt r15,#FxPushApply
    nop
    lm r2,(!fx_push_amount-!gsu_ram)
.l927B:
    lm r0,(!fx_hit_depth-!gsu_ram)
    moves r0,r0
    bne .l9288
    nop
    sm (!fx_hit_depth-!gsu_ram),r2
.l9288:
    moves r2,r2
    beq .l92B4
    nop
    dec r10
    dec r10
    to r1
    ldw (r10)
    lm r0,(!fx_push_flags-!gsu_ram)
    iwt r3,#$0020
    and r3
    beq .l92A7
    nop
    ibt r3,#$2C
    with r3
    add r9
    to r3
    ldw (r3)
    iwt r0,#$0020
    or r3
    sbk
.l92A7:
    ibt r3,#$10
    with r3
    add r1
    to r3
    ldw (r3)
    iwt r0,#$0002
    or r3
    bra .l92B8
    sbk
.l92B4:
    dec r10
    dec r10
    to r1
    ldw (r10)
.l92B8:
    ibt r0,#$0E
    add r1
    ldw (r0)
    moves r1,r0
    beq .l92C5
    nop
    iwt r15,#.l9229
    nop
.l92C5:
    ibt r2,#$20
    with r2
    add r9
    to r2
    ldw (r2)
    link #4
    iwt r15,#FxPushSet
    nop
    iwt r3,#$23A4
    link #4
    iwt r15,#FxPushApply
    nop
    moves r0,r0
    beq .l92F7
    nop
    iwt r3,#$0001
    ibt r0,#$1C
    add r9
    ldw (r0)
    or r3
    sbk
    lm r0,(!fx_hit_depth-!gsu_ram)
    moves r0,r0
    bne .l92F7
    nop
    lm r0,(!fx_push_amount-!gsu_ram)
    sm (!fx_hit_depth-!gsu_ram),r0
.l92F7:
    link #4
    iwt r15,#FxSumsApply
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:957C FxContactsAnyTouch
; WHAT IT DOES: Whether any of the twenty contact slots at $23AE holds
; a touch: R0 = 1 at the first slot in use whose depth word is neither
; $7D00 nor negative, else 0. Not reached in the traces so far.
; HOW: CACHE; R13 counts the slots to $14, R3 walks them (8 past an
; empty one, 6 past a counted one); returns by JMP R11 (MOVE R15,
; R11).
; Entry: R11 = return address.
; Exit: R0 = 1 or 0; R3 at the slot reached; R4 its depth; R13 the
; slot count.
%at($04957C)
FxContactsAnyTouch:
    cache
    iwt r3,#$23AE
    ibt r13,#$00
.l9582:
    inc r13
    ibt r0,#$14
    cmp r13
    beq .l95B4
    nop
    ldw (r3)
    inc r3
    inc r3
    moves r0,r0
    beq .l95A6
    nop
    to r4
    ldw (r3)
    inc r3
    inc r3
    iwt r0,#$7D00
    cmp r4
    beq .l95AD
    nop
    moves r4,r4
    bpl .l95B9
    nop
    bra .l95AD
    nop
.l95A6:
    ibt r0,#$08
    with r3
    add r0
    bra .l9582
    nop
.l95AD:
    ibt r0,#$06
    with r3
    add r0
    bra .l9582
    nop
.l95B4:
    ibt r0,#$00
    move r15,r11
    nop
.l95B9:
    ibt r0,#$01
    move r15,r11
    nop
;
; @routine 04:9C3D FxCollideModel
; WHAT IT DOES: Collides record R9 with the model of record R7 (its
; shape's +$14 in ROM): R9's place relative to R7's is turned into the
; model's axes (by quarter turns, or FxTurn for other headings), then
; each face of the model's list is tested against it: a face the place
; lies behind, by less than the face's best so far, is kept in one of
; the twenty contact slots at $23AE (its flags, depth and normal). R0
; = 0 when the model has no collision list.
; HOW: Pushes R11, R8 and R9; the slots' depth words set to $7D00
; first; the face corners from the point table at $2488 (FxModelPoints
; when the model or its scale changed), the normals by FxNormalize and
; LMULT cross products, the planes by FMULT; returns by pulling R15.
; Entry: R7 = the model's record, R9 = the record; R10 = stack; R11 =
; return address.
; Exit: R0 = 0 (no list) or as the face walk leaves it; R8, R9 back;
; the slots filled.
%at($049C3D)
FxCollideModel:
    from r11
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
    ibt r0,#$12
    add r7
    ldw (r0)
    sms (!gsu_speed_44-!gsu_ram),r0
    iwt r0,#$0000
    iwt r1,#$7D00
    sm (!fx_pushes-!gsu_ram),r0
    sm (!fx_pushes+2-!gsu_ram),r1
    sm (!fx_pushes+10-!gsu_ram),r0
    sm (!fx_pushes+12-!gsu_ram),r1
    sm (!fx_pushes+20-!gsu_ram),r0
    sm (!fx_pushes+22-!gsu_ram),r1
    sm (!fx_pushes+30-!gsu_ram),r0
    sm (!fx_pushes+32-!gsu_ram),r1
    sm (!fx_pushes+40-!gsu_ram),r0
    sm (!fx_pushes+42-!gsu_ram),r1
    sm (!fx_pushes+50-!gsu_ram),r0
    sm (!fx_pushes+52-!gsu_ram),r1
    sm (!fx_pushes+60-!gsu_ram),r0
    sm (!fx_pushes+62-!gsu_ram),r1
    sm (!fx_pushes+70-!gsu_ram),r0
    sm (!fx_pushes+72-!gsu_ram),r1
    sm (!fx_pushes+80-!gsu_ram),r0
    sm (!fx_pushes+82-!gsu_ram),r1
    sm (!fx_pushes+90-!gsu_ram),r0
    sm (!fx_pushes+92-!gsu_ram),r1
    sm (!fx_pushes+100-!gsu_ram),r0
    sm (!fx_pushes+102-!gsu_ram),r1
    sm (!fx_pushes+110-!gsu_ram),r0
    sm (!fx_pushes+112-!gsu_ram),r1
    sm (!fx_pushes+120-!gsu_ram),r0
    sm (!fx_pushes+122-!gsu_ram),r1
    sm (!fx_pushes+130-!gsu_ram),r0
    sm (!fx_pushes+132-!gsu_ram),r1
    sm (!fx_pushes+140-!gsu_ram),r0
    sm (!fx_pushes+142-!gsu_ram),r1
    sm (!fx_pushes+150-!gsu_ram),r0
    sm (!fx_pushes+152-!gsu_ram),r1
    sm (!fx_pushes+160-!gsu_ram),r0
    sm (!fx_pushes+162-!gsu_ram),r1
    sm (!fx_pushes+170-!gsu_ram),r0
    sm (!fx_pushes+172-!gsu_ram),r1
    sm (!fx_pushes+180-!gsu_ram),r0
    sm (!fx_pushes+182-!gsu_ram),r1
    ibt r0,#$00
    romb
    ibt r1,#$14
    with r1
    add r7
    to r1
    ldw (r1)
    moves r1,r1
    bne .l9D01
    nop
    iwt r15,#.lA145
    nop
.l9D01:
    move r14,r1
    iwt r0,#$000C
    with r14
    add r0
    getb
    inc r14
    getbh
    moves r0,r0
    bne .l9D15
    nop
    iwt r15,#.lA145
    nop
.l9D15:
    move r14,r1
    ibt r4,#$1E
    with r4
    add r9
    to r4
    ldw (r4)
    ibt r0,#$1E
    add r7
    ldw (r0)
    with r4
    sub r0
    sms (!gsu_in_20-!gsu_ram),r4
    ibt r5,#$20
    with r5
    add r9
    to r5
    ldw (r5)
    ibt r0,#$20
    add r7
    ldw (r0)
    with r5
    sub r0
    sms (!gsu_in_22-!gsu_ram),r5
    ibt r8,#$22
    with r8
    add r9
    to r8
    ldw (r8)
    ibt r0,#$22
    add r7
    ldw (r0)
    with r8
    sub r0
    sms (!gsu_in_24-!gsu_ram),r8
    sub r0
    sm (!fx_push_turn-!gsu_ram),r0
    ibt r0,#$24
    add r7
    ldw (r0)
    moves r0,r0
    bne .l9D56
    nop
    iwt r15,#.l9DEF
    nop
.l9D56:
    hib
    sm (!fx_push_turn-!gsu_ram),r0
    iwt r7,#$00C0
    cmp r7
    bne .l9D6F
    nop
    move r0,r8
    not
    inc r0
    move r8,r4
    move r4,r0
    iwt r15,#.l9DEF
    nop
.l9D6F:
    iwt r7,#$0040
    cmp r7
    bne .l9D82
    nop
    move r0,r4
    not
    inc r0
    move r4,r8
    move r8,r0
    bra .l9DEF
    nop
.l9D82:
    iwt r7,#$0080
    cmp r7
    bne .l9D93
    nop
    with r4
    not
    inc r4
    with r8
    not
    inc r8
    bra .l9DEF
    nop
.l9D93:
    from r14
    stw (r10)
    inc r10
    inc r10
    from r1
    stw (r10)
    inc r10
    inc r10
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
    from r9
    stw (r10)
    inc r10
    inc r10
    move r1,r4
    move r3,r8
    not
    inc r0
    iwt r2,#$00FF
    to r2
    and r2
    ibt r0,#$00
    romb
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
    move r4,r3
    move r8,r1
    dec r10
    dec r10
    to r9
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
    dec r10
    dec r10
    to r1
    ldw (r10)
    dec r10
    dec r10
    to r14
    ldw (r10)
    ibt r0,#$00
    romb
.l9DEF:
    iwt r0,#$0003
    with r14
    add r0
    to r2
    getb
    inc r14
    with r2
    getbh
    iwt r0,#$0000
    move r14,r1
    with r14
    add r0
    to r9
    getb
    inc r14
    with r9
    getbh
    iwt r0,#$0007
    move r14,r1
    with r14
    add r0
    getbs
    move r12,r0
    ibt r0,#$01
    move r13,r15
    add r0
    loop
    nop
    sm (!fx_face_scale-!gsu_ram),r0
    iwt r0,#$0002
    move r14,r1
    with r14
    add r0
    getb
    romb
    lm r3,(!gsu_speed_44-!gsu_ram)
    lm r0,(!fx_init_2380-!gsu_ram)
    cmp r1
    bne .l9E3D
    nop
    lm r0,(!fx_init_2382-!gsu_ram)
    cmp r3
    beq .l9E4A
    nop
.l9E3D:
    sm (!fx_init_2380-!gsu_ram),r1
    sm (!fx_init_2382-!gsu_ram),r3
    link #4
    iwt r15,#FxModelPoints
    nop
.l9E4A:
    iwt r9,#$2488
    move r14,r2
    inc r14
    getb
    inc r14
    umult #3
    with r14
    add r0
    cache
    getbs
    ibt r13,#$3C
    cmp r13
    bne .l9E92
    inc r14
    ibt r1,#$28
    ibt r7,#$05
    ibt r2,#$44
    ibt r12,#$03
    ibt r3,#$40
.l9E6A:
    getbs
    cmp r3
    bne .l9E74
    nop
    bra .l9E6A
    inc r14
.l9E74:
    cmp r1
    bne .l9E7D
    nop
    with r14
    bra .l9E6A
    add r7
.l9E7D:
    cmp r2
    bne .l9E87
    with r14
    bra .l9E6A
    add r12
.l9E85:
    getbs
.l9E87:
    inc r14
    moves r0,r0
    bne .l9E91
    nop
    iwt r15,#.lA0F8
    nop
.l9E91:
    cache
.l9E92:
    to r13
    getbs
    moves r13,r13
    bmi .l9E85
    inc r14
    sub r0
    to r11
    getbs
    inc r14
    moves r11,r11
    bne .l9EA8
    nop
    iwt r15,#.lA0EF
    nop
.l9EA8:
    move r0,r11
    sm (!fx_face_flags-!gsu_ram),r0
    iwt r1,#$00E0
    with r11
    bic r1
    ibt r0,#$0A
    to r11
    mult r11
    iwt r0,#$23A4
    with r11
    add r0
    ldw (r11)
    hib
    beq .l9EC6
    nop
    iwt r15,#.lA0EF
    nop
.l9EC6:
    inc r14
    inc r14
    lms r0,(!gsu_speed_44-!gsu_ram)
    moves r0,r0
    bne .l9ED4
    nop
    iwt r15,#.lA013
    nop
.l9ED4:
    lm r7,(!fx_face_flags-!gsu_ram)
    iwt r0,#$0040
    and r7
    bne .l9EE3
    nop
    iwt r15,#.lA013
    nop
.l9EE3:
    with r14
    add #3
    from r14
    stw (r10)
    inc r10
    inc r10
    move r12,r9
    getb
    inc r14
    umult #6
    to r9
    add r12
    ldw (r9)
    inc r9
    inc r9
    sms (!gsu_in_68-!gsu_ram),r0
    ldw (r9)
    inc r9
    inc r9
    sms (!gsu_dest-!gsu_ram),r0
    ldw (r9)
    inc r9
    inc r9
    sms (!gsu_in_2e-!gsu_ram),r0
    getb
    inc r14
    umult #6
    to r9
    add r12
    ldw (r9)
    inc r9
    inc r9
    sms (!gsu_in_6a-!gsu_ram),r0
    ldw (r9)
    inc r9
    inc r9
    sms (!gsu_in_a2-!gsu_ram),r0
    ldw (r9)
    inc r9
    inc r9
    sms (!gsu_in_a4-!gsu_ram),r0
    getb
    inc r14
    umult #6
    to r9
    add r12
    to r1
    ldw (r9)
    inc r9
    inc r9
    to r2
    ldw (r9)
    inc r9
    inc r9
    to r3
    ldw (r9)
    inc r9
    inc r9
    lms r0,(!gsu_in_68-!gsu_ram)
    lms r9,(!gsu_in_6a-!gsu_ram)
    sub r9
    sms (!gsu_in_68-!gsu_ram),r0
    lms r0,(!gsu_dest-!gsu_ram)
    lms r9,(!gsu_in_a2-!gsu_ram)
    sub r9
    sms (!gsu_dest-!gsu_ram),r0
    lms r0,(!gsu_in_2e-!gsu_ram)
    lms r9,(!gsu_in_a4-!gsu_ram)
    sub r9
    sms (!gsu_in_2e-!gsu_ram),r0
    lms r0,(!gsu_in_6a-!gsu_ram)
    sub r1
    sms (!gsu_in_6a-!gsu_ram),r0
    lms r0,(!gsu_in_a2-!gsu_ram)
    sub r2
    sms (!gsu_in_a2-!gsu_ram),r0
    lms r0,(!gsu_in_a4-!gsu_ram)
    sub r3
    sms (!gsu_in_a4-!gsu_ram),r0
    from r4
    stw (r10)
    inc r10
    inc r10
    from r5
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    from r12
    stw (r10)
    inc r10
    inc r10
    from r13
    stw (r10)
    inc r10
    inc r10
    from r11
    stw (r10)
    inc r10
    inc r10
    lms r1,(!gsu_in_68-!gsu_ram)
    lms r2,(!gsu_dest-!gsu_ram)
    lms r3,(!gsu_in_2e-!gsu_ram)
    link #4
    iwt r15,#FxNormalize
    nop
    sms (!gsu_in_68-!gsu_ram),r1
    sms (!gsu_dest-!gsu_ram),r2
    sms (!gsu_in_2e-!gsu_ram),r3
    lms r1,(!gsu_in_6a-!gsu_ram)
    lms r2,(!gsu_in_a2-!gsu_ram)
    lms r3,(!gsu_in_a4-!gsu_ram)
    link #4
    iwt r15,#FxNormalize
    nop
    lms r6,(!gsu_dest-!gsu_ram)
    from r3
    lmult
    move r7,r4
    lms r6,(!gsu_in_2e-!gsu_ram)
    from r2
    to r5
    lmult
    with r7
    sub r4
    sbc r5
    with r7
    add r7
    rol
    sms (!gsu_in_6a-!gsu_ram),r0
    lms r6,(!gsu_in_2e-!gsu_ram)
    from r1
    lmult
    move r7,r4
    lms r6,(!gsu_in_68-!gsu_ram)
    from r3
    to r5
    lmult
    with r7
    sub r4
    sbc r5
    with r7
    add r7
    rol
    sms (!gsu_in_a2-!gsu_ram),r0
    lms r6,(!gsu_in_68-!gsu_ram)
    from r2
    lmult
    move r7,r4
    lms r6,(!gsu_dest-!gsu_ram)
    from r1
    to r5
    lmult
    with r7
    sub r4
    sbc r5
    with r7
    add r7
    rol
    sms (!gsu_in_a4-!gsu_ram),r0
    lms r1,(!gsu_in_6a-!gsu_ram)
    lms r2,(!gsu_in_a2-!gsu_ram)
    lms r3,(!gsu_in_a4-!gsu_ram)
    dec r10
    dec r10
    to r11
    ldw (r10)
    dec r10
    dec r10
    to r13
    ldw (r10)
    dec r10
    dec r10
    to r12
    ldw (r10)
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r5
    ldw (r10)
    dec r10
    dec r10
    to r4
    ldw (r10)
    move r9,r12
    dec r10
    dec r10
    to r14
    ldw (r10)
    bra .lA028
    nop
.lA013:
    getb
    inc r14
    to r1
    swap
    with r1
    not
    inc r1
    getb
    inc r14
    to r2
    swap
    with r2
    not
    inc r2
    getb
    inc r14
    to r3
    swap
    with r3
    not
    inc r3
.lA028:
    lm r7,(!fx_face_scale-!gsu_ram)
    getbs
    with r14
    add r13
    move r13,r9
    umult #6
    with r9
    add r0
    ldw (r9)
    inc r9
    inc r9
    mult r7
    sm (!fx_face_normal-!gsu_ram),r0
    ldw (r9)
    inc r9
    inc r9
    mult r7
    sm (!fx_face_normal+2-!gsu_ram),r0
    ldw (r9)
    mult r7
    sm (!fx_face_normal+4-!gsu_ram),r0
    move r9,r13
    ldw (r11)
    hib
    beq .lA057
    nop
    iwt r15,#.l9E92
    nop
.lA057:
    move r6,r4
    from r1
    to r7
    fmult
    move r6,r5
    from r2
    fmult
    with r7
    add r0
    move r6,r8
    from r3
    fmult
    add r7
    sm (!fx_face_plane-!gsu_ram),r0
    lm r6,(!fx_face_normal-!gsu_ram)
    from r1
    to r7
    fmult
    lm r6,(!fx_face_normal+2-!gsu_ram)
    from r2
    fmult
    with r7
    add r0
    lm r6,(!fx_face_normal+4-!gsu_ram)
    from r3
    fmult
    add r7
    lm r7,(!fx_face_plane-!gsu_ram)
    sub r7
    bpl .lA09C
    nop
    move r12,r0
    lm r0,(!fx_face_flags-!gsu_ram)
    swap
    to r7
    ldw (r11)
    or r7
    iwt r7,#$FF00
    and r7
    sbk
    iwt r15,#.l9E92
    nop
.lA09C:
    lm r7,(!fx_face_flags-!gsu_ram)
    iwt r12,#$0080
    with r12
    and r7
    beq .lA0B8
    nop
    lm r0,(!fx_face_flags-!gsu_ram)
    iwt r7,#$00FF
    to r7
    and r7
    ldw (r11)
    or r7
    sbk
    iwt r15,#.l9E92
    nop
.lA0B8:
    inc r11
    inc r11
    to r12
    ldw (r11)
    moves r7,r12
    bpl .lA0C4
    nop
    with r7
    not
    inc r7
.lA0C4:
    moves r12,r0
    bpl .lA0CC
    nop
    with r12
    not
    inc r12
.lA0CC:
    with r7
    cmp r12
    bpl .lA0D6
    nop
    iwt r15,#.l9E92
    nop
.lA0D6:
    stw (r11)
    dec r11
    dec r11
    lm r0,(!fx_face_flags-!gsu_ram)
    stw (r11)
    with r11
    add #4
    from r1
    stw (r11)
    inc r11
    inc r11
    from r2
    stw (r11)
    inc r11
    inc r11
    from r3
    stw (r11)
    iwt r15,#.l9E92
    nop
.lA0EF:
    with r14
    add r13
    with r14
    add #5
    iwt r15,#.l9E92
    nop
.lA0F8:
    iwt r11,#$23A4
    iwt r12,#$0013
    move r13,r15
    with r11
    add #10
    ldw (r11)
    moves r0,r0
    beq .lA10E
    nop
    to r7
    hib
    beq .lA115
    nop
.lA10E:
    loop
    nop
    ibt r0,#$00
    bra .lA117
    nop
.lA115:
    ibt r0,#$01
.lA117:
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
    to r15
    ldw (r10)
    nop
    bra .lA0F8
    nop
    iwt r11,#$23A4
    ibt r0,#$1E
    add r9
    ldw (r0)
    ibt r2,#$22
    with r2
    add r9
    to r2
    ldw (r2)
    add r2
    ibt r2,#$7F
    and r2
    not
    inc r0
    ibt r2,#$20
    with r2
    add r9
    to r2
    ldw (r2)
    add r2
    stw (r11)
    bra .lA0F8
    nop
.lA145:
    ibt r0,#$00
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
    to r15
    ldw (r10)
    nop
;
; @routine 04:A607 FxPartsUpdate
; WHAT IT DOES: Updates each part of car R8 (its list at +$3E, linked
; by +$34): a part with bit 11 of its flags (+$46) is pulled toward
; where it belongs (FxPartPull), any other is a wheel (FxWheelUpdate);
; each part's contact flags (+$2C) are gathered into the car's +$92.
; Clears the car's +$44, +$46 and +$56 first.
; HOW: Pushes R11; R9 = the part; returns by pulling R15.
; Entry: R8 = the car; R10 = stack; R11 = return address.
; Exit: R0 = 0 at the list's end; R9 the last part; the rest as the
; last part's update leaves them.
%at($04A607)
FxPartsUpdate:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$00
    ibt r0,#$44
    add r8
    from r1
    stw (r0)
    ibt r0,#$46
    add r8
    from r1
    stw (r0)
    ibt r0,#$56
    add r8
    from r1
    stw (r0)
    ibt r0,#$3E
    add r8
    ldw (r0)
.lA620:
    moves r9,r0
    beq .lA651
    nop
    ibt r0,#$46
    add r9
    ldw (r0)
    iwt r1,#$0800
    and r1
    beq .lA638
    nop
    link #4
    iwt r15,#FxPartPull
    nop
    bra .lA63D
    nop
.lA638:
    link #4
    iwt r15,#FxWheelUpdate
    nop
.lA63D:
    ibt r1,#$2C
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$0092
    add r8
    ldw (r0)
    or r1
    sbk
    ibt r0,#$34
    add r9
    ldw (r0)
    bra .lA620
    nop
.lA651:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:A656 FxPartPull
; WHAT IT DOES: Pulls part R9 of car R8 toward its place on the car:
; unless its flag bit 2 was clear (then it is only set), the part's
; offset from the first wheel (by +$8E's list) is taken, each axis
; under $C8 dropped, scaled by four and added to that wheel's speed,
; the part's own speed subtracted where the signs differ; the result
; is pushed on the car (FxSumsPushWorld, FxHitAddUnit) at the part's
; place through the FxPushAt variant $2654 picks. With bit 0 of the
; car's +$64 set it takes the car's speed ($2334-$2338) instead
; (04:A7B4). Not reached in the traces so far.
; HOW: Pushes R11; the push variant reached by LINK #2; JMP R12
; through $04:9BC6; returns by pulling R15.
; Entry: R8 = the car, R9 = the part; R10 = stack; R11 = return
; address.
; Exit: as the push leaves them.
%at($04A656)
FxPartPull:
    ibt r0,#$64
    add r8
    ldw (r0)
    iwt r4,#$0001
    and r4
    beq .lA665
    nop
    iwt r15,#.lA7B4
    nop
.lA665:
    nop
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$46
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$0004
    and r1
    bne .lA681
    nop
    iwt r0,#$0004
    or r1
    sbk
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lA681:
    iwt r2,#$0800
    iwt r7,#$008E
    with r7
    add r8
    to r7
    ldw (r7)
    ibt r1,#$28
    with r1
    add r7
    to r1
    ldw (r1)
    ibt r0,#$28
    add r8
    from r1
    stw (r0)
    ibt r0,#$3E
    add r7
    ldw (r0)
.lA69A:
    moves r0,r0
    beq .lA6B0
    nop
    move r7,r0
    ibt r0,#$46
    add r7
    ldw (r0)
    and r2
    bne .lA6B0
    nop
    ibt r0,#$34
    add r7
    ldw (r0)
    bra .lA69A
    nop
.lA6B0:
    ibt r1,#$1E
    with r1
    add r7
    to r1
    ldw (r1)
    ibt r2,#$20
    with r2
    add r7
    to r2
    ldw (r2)
    ibt r3,#$22
    with r3
    add r7
    to r3
    ldw (r3)
    ibt r0,#$1E
    add r9
    ldw (r0)
    with r1
    sub r0
    ibt r0,#$20
    add r9
    ldw (r0)
    with r2
    sub r0
    ibt r0,#$22
    add r9
    ldw (r0)
    with r3
    sub r0
    sm (!fx_ram_test-!gsu_ram),r3
    iwt r4,#$00C8
    moves r0,r1
    bpl .lA6E2
    nop
    not
    inc r0
.lA6E2:
    cmp r4
    bpl .lA6E9
    nop
    ibt r1,#$00
.lA6E9:
    moves r0,r2
    bpl .lA6F0
    nop
    not
    inc r0
.lA6F0:
    cmp r4
    bpl .lA6F7
    nop
    ibt r2,#$00
.lA6F7:
    moves r0,r3
    bpl .lA6FE
    nop
    not
    inc r0
.lA6FE:
    cmp r4
    bpl .lA705
    nop
    ibt r3,#$00
.lA705:
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
    ibt r4,#$2E
    with r4
    add r7
    to r4
    ldw (r4)
    ibt r5,#$30
    with r5
    add r7
    to r5
    ldw (r5)
    ibt r6,#$32
    with r6
    add r7
    to r6
    ldw (r6)
    ibt r5,#$00
    with r4
    add r4
    with r5
    add r5
    with r6
    add r6
    with r4
    add r4
    with r5
    add r5
    with r6
    add r6
    ibt r0,#$2E
    add r9
    ldw (r0)
    to r7
    xor r1
    bpl .lA73D
    nop
    with r1
    sub r0
.lA73D:
    ibt r0,#$30
    add r9
    ldw (r0)
    to r7
    xor r2
    bpl .lA749
    nop
    with r2
    sub r0
.lA749:
    ibt r0,#$32
    add r9
    ldw (r0)
    sm (!fx_part_z_speed-!gsu_ram),r0
    to r7
    xor r3
    bpl .lA759
    nop
    with r2
    sub r0
.lA759:
    with r1
    add r4
    with r2
    add r5
    with r3
    add r6
    move r4,r1
    move r5,r2
    move r6,r3
    link #4
    iwt r15,#FxSumsPushWorld
    nop
    ibt r1,#$36
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r2,#$38
    with r2
    add r9
    to r2
    ldw (r2)
    ibt r3,#$3A
    with r3
    add r9
    to r3
    ldw (r3)
    ibt r1,#$00
    ibt r2,#$00
    ibt r3,#$00
    link #4
    iwt r15,#FxHitAddUnit
    nop
    ibt r1,#$3C
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r2,#$3E
    with r2
    add r9
    to r2
    ldw (r2)
    ibt r3,#$40
    with r3
    add r9
    to r3
    ldw (r3)
    ibt r0,#$04
    romb
    lm r0,(!fx_param_push-!gsu_ram)
    iwt r14,#$9BC6
    with r14
    add r0
    getb
    inc r14
    getbh
    move r12,r0
    link #2
    jmp r12
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lA7B4:
    lm r1,(!fx_hit-!gsu_ram)
    ibt r0,#$2E
    add r9
    from r1
    stw (r0)
    lm r1,(!fx_hit+2-!gsu_ram)
    ibt r0,#$30
    add r9
    from r1
    stw (r0)
    lm r1,(!fx_hit+4-!gsu_ram)
    ibt r0,#$32
    add r9
    from r1
    stw (r0)
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$46
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$0004
    and r1
    bne .lA7EA
    nop
    iwt r0,#$0004
    or r1
    sbk
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lA7EA:
    iwt r2,#$0800
    iwt r7,#$008E
    with r7
    add r8
    to r7
    ldw (r7)
    ibt r0,#$3E
    add r7
    ldw (r0)
.lA7F8:
    moves r0,r0
    beq .lA80E
    nop
    move r7,r0
    ibt r0,#$46
    add r7
    ldw (r0)
    and r2
    bne .lA80E
    nop
    ibt r0,#$34
    add r7
    ldw (r0)
    bra .lA7F8
    nop
.lA80E:
    ibt r1,#$1E
    with r1
    add r7
    to r1
    ldw (r1)
    ibt r2,#$20
    with r2
    add r7
    to r2
    ldw (r2)
    ibt r3,#$22
    with r3
    add r7
    to r3
    ldw (r3)
    ibt r0,#$1E
    add r9
    ldw (r0)
    with r1
    sub r0
    ibt r0,#$20
    add r9
    ldw (r0)
    with r2
    sub r0
    ibt r0,#$22
    add r9
    ldw (r0)
    with r3
    sub r0
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
    move r4,r1
    move r5,r2
    move r6,r3
    link #4
    iwt r15,#FxSumsPushWorld
    nop
    ibt r1,#$36
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r2,#$38
    with r2
    add r9
    to r2
    ldw (r2)
    ibt r3,#$3A
    with r3
    add r9
    to r3
    ldw (r3)
    ibt r1,#$00
    ibt r2,#$00
    ibt r3,#$00
    link #4
    iwt r15,#FxHitAddUnit
    nop
    ibt r1,#$3C
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r2,#$3E
    with r2
    add r9
    to r2
    ldw (r2)
    ibt r3,#$40
    with r3
    add r9
    to r3
    ldw (r3)
    ibt r0,#$04
    romb
    lm r0,(!fx_param_push-!gsu_ram)
    iwt r14,#$9BC6
    with r14
    add r0
    getb
    inc r14
    getbh
    move r12,r0
    link #2
    jmp r12
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:A893 FxWheelUpdate
; WHAT IT DOES: Updates wheel R9 of car R8: copies the car's heading
; and height, finds the ground under it (FxCollideModel with the
; touched record, FxContactsResolve), takes the surface kind from the
; model's flags into its contact flags (+$2C), and from the contact
; depth and force works the suspension load, the grip along and across
; the car (by the handling words $2652-$2682) and pushes the car with
; them (FxSumsPushWorld, FxSumsPushLocal, FxSumsRotateLocal,
; FxHitAddScaled). A wheel off the ground ($237E = $7D00) is handled
; at 04:AEE5.
; HOW: Pushes R11; $2484 = the wheel's flags while it runs; the
; contact slots cleared and the sums cleared (FxSumsClear); returns by
; pulling R15.
; Entry: R8 = the car, R9 = the wheel; R10 = stack; R11 = return
; address.
; Exit: $2484 = the wheel's flags; the rest as the pushes leave them.
%at($04A893)
FxWheelUpdate:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r0,#$46
    add r9
    ldw (r0)
    sm (!fx_wheel_flags-!gsu_ram),r0
    ibt r1,#$00
    ibt r0,#$2C
    add r9
    from r1
    stw (r0)
    iwt r3,#$0400
    ibt r2,#$40
    with r2
    add r8
    to r2
    ldw (r2)
    lm r1,(!fx_wheel_flags-!gsu_ram)
    with r1
    bic r3
    iwt r0,#$0200
    and r1
    beq .lA8CC
    nop
    iwt r0,#$0000
    and r2
    bne .lA8CC
    nop
    with r1
    or r3
    from r1
    sbk
    iwt r15,#.lAED7
    nop
.lA8CC:
    from r1
    sbk
    ibt r1,#$24
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r0,#$24
    add r9
    from r1
    stw (r0)
    ibt r1,#$28
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r0,#$28
    add r9
    from r1
    stw (r0)
    lm r2,(!fx_wheel_flags-!gsu_ram)
    iwt r0,#$0002
    and r2
    beq .lA8FB
    nop
    ibt r0,#$3A
    add r8
    ldw (r0)
    ibt r1,#$24
    with r1
    add r9
    to r1
    ldw (r1)
    add r1
    sbk
.lA8FB:
    lm r0,(!fx_wheel_flags-!gsu_ram)
    iwt r1,#$0080
    and r1
    beq .lA90A
    nop
    iwt r15,#.lAEB9
    nop
.lA90A:
    link #4
    iwt r15,#FxSumsClear
    nop
    iwt r0,#$0000
    iwt r1,#$7D00
    sm (!fx_pushes-!gsu_ram),r0
    sm (!fx_pushes+2-!gsu_ram),r1
    sm (!fx_pushes+10-!gsu_ram),r0
    sm (!fx_pushes+12-!gsu_ram),r1
    sm (!fx_pushes+20-!gsu_ram),r0
    sm (!fx_pushes+22-!gsu_ram),r1
    sm (!fx_pushes+30-!gsu_ram),r0
    sm (!fx_pushes+32-!gsu_ram),r1
    sm (!fx_pushes+40-!gsu_ram),r0
    sm (!fx_pushes+42-!gsu_ram),r1
    sm (!fx_pushes+50-!gsu_ram),r0
    sm (!fx_pushes+52-!gsu_ram),r1
    sm (!fx_pushes+60-!gsu_ram),r0
    sm (!fx_pushes+62-!gsu_ram),r1
    sm (!fx_pushes+70-!gsu_ram),r0
    sm (!fx_pushes+72-!gsu_ram),r1
    sm (!fx_pushes+80-!gsu_ram),r0
    sm (!fx_pushes+82-!gsu_ram),r1
    sm (!fx_pushes+90-!gsu_ram),r0
    sm (!fx_pushes+92-!gsu_ram),r1
    sm (!fx_pushes+100-!gsu_ram),r0
    sm (!fx_pushes+102-!gsu_ram),r1
    sm (!fx_pushes+110-!gsu_ram),r0
    sm (!fx_pushes+112-!gsu_ram),r1
    sm (!fx_pushes+120-!gsu_ram),r0
    sm (!fx_pushes+122-!gsu_ram),r1
    sm (!fx_pushes+130-!gsu_ram),r0
    sm (!fx_pushes+132-!gsu_ram),r1
    sm (!fx_pushes+140-!gsu_ram),r0
    sm (!fx_pushes+142-!gsu_ram),r1
    sm (!fx_pushes+150-!gsu_ram),r0
    sm (!fx_pushes+152-!gsu_ram),r1
    sm (!fx_pushes+160-!gsu_ram),r0
    sm (!fx_pushes+162-!gsu_ram),r1
    sm (!fx_pushes+170-!gsu_ram),r0
    sm (!fx_pushes+172-!gsu_ram),r1
    sm (!fx_pushes+180-!gsu_ram),r0
    sm (!fx_pushes+182-!gsu_ram),r1
    sub r0
    sm (!fx_contact_sum-!gsu_ram),r0
    sm (!fx_contact_sum+2-!gsu_ram),r0
    sm (!fx_contact_sum+4-!gsu_ram),r0
    iwt r0,#$7D00
    sm (!fx_contact_depth-!gsu_ram),r0
    sub r0
    sm (!fx_wheel_contact-!gsu_ram),r0
    lm r0,(!fx_wheel_flags-!gsu_ram)
    iwt r1,#$0030
    bic r1
    sbk
    ibt r0,#$18
    add r9
    ldw (r0)
    moves r1,r0
    bne .lA9DD
    nop
    iwt r15,#.lAA98
    nop
.lA9DD:
    ibt r7,#$0C
    with r7
    add r1
    to r7
    ldw (r7)
    sm (!fx_collide_model-!gsu_ram),r7
    link #4
    iwt r15,#FxCollideModel
    nop
    moves r0,r0
    beq .lAA0D
    nop
    link #4
    iwt r15,#FxContactsResolve
    nop
    moves r1,r0
    beq .lAA0D
    nop
    iwt r0,#$001F
    and r1
    bne .lAA06
    nop
    iwt r15,#.lAA98
    nop
.lAA06:
    iwt r0,#$0020
    and r1
    beq .lAA5A
    nop
.lAA0D:
    lm r2,(!fx_collide_model-!gsu_ram)
    ibt r1,#$16
    with r1
    add r2
    to r1
    ldw (r1)
    iwt r0,#$0040
    and r1
    bne .lAA5A
    nop
    iwt r0,#$3800
    to r1
    and r1
    beq .lAA8E
    nop
    iwt r2,#$00A0
    iwt r0,#$0800
    cmp r1
    beq .lAA52
    nop
    iwt r2,#$2020
    iwt r0,#$2000
    cmp r1
    beq .lAA52
    nop
    iwt r2,#$1020
    iwt r0,#$1000
    cmp r1
    beq .lAA52
    nop
    iwt r2,#$0820
    iwt r0,#$1800
    cmp r1
    bne .lAA5A
    nop
.lAA52:
    ibt r0,#$2C
    add r9
    ldw (r0)
    or r2
    bra .lAA82
    sbk
.lAA5A:
    lm r2,(!fx_collide_model-!gsu_ram)
    ibt r1,#$16
    with r1
    add r2
    to r1
    ldw (r1)
    iwt r0,#$4000
    and r1
    bne .lAA82
    nop
    iwt r1,#$008A
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0040
    and r1
    beq .lAA82
    nop
    iwt r2,#$1000
    ibt r0,#$2C
    add r9
    ldw (r0)
    or r2
    sbk
.lAA82:
    ibt r0,#$2C
    add r8
    ldw (r0)
    iwt r2,#$0040
    bic r2
    bra .lAAD0
    sbk
.lAA8E:
    ibt r0,#$2C
    add r8
    ldw (r0)
    iwt r2,#$0040
    bic r2
    sbk
.lAA98:
    iwt r1,#$008A
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r2,#$0040
    iwt r0,#$0004
    and r1
    bne .lAACA
    nop
    iwt r2,#$00C0
    iwt r0,#$0020
    and r1
    bne .lAACA
    nop
    iwt r2,#$1040
    iwt r0,#$0040
    and r1
    bne .lAACA
    nop
    iwt r2,#$0840
    iwt r0,#$0002
    and r1
    bne .lAACA
    nop
    iwt r2,#$0060
.lAACA:
    ibt r0,#$2C
    add r9
    ldw (r0)
    or r2
    sbk
.lAAD0:
    ibt r2,#$20
    with r2
    add r9
    to r2
    ldw (r2)
    moves r2,r2
    bmi .lAB15
    nop
    ibt r0,#$2C
    add r9
    ldw (r0)
    iwt r1,#$0020
    and r1
    beq .lAB01
    nop
    iwt r0,#$000A
    with r2
    add r0
    ibt r0,#$1E
    add r9
    ldw (r0)
    ibt r1,#$22
    with r1
    add r9
    to r1
    ldw (r1)
    add r1
    div2
    div2
    div2
    ibt r1,#$0F
    and r1
    with r2
    add r0
.lAB01:
    link #4
    iwt r15,#FxPushSet
    nop
    moves r0,r0
    beq .lAB15
    nop
    iwt r3,#$23A4
    ibt r2,#$00
    link #4
    iwt r15,#FxContactResolve
    nop
.lAB15:
    lm r0,(!fx_contact_force_up-!gsu_ram)
    sm (!fx_contact_force-!gsu_ram),r0
    ibt r1,#$2C
    with r1
    add r9
    to r1
    ldw (r1)
    iwt r0,#$0020
    and r1
    beq .lAB4B
    nop
    lm r0,(!fx_contact_force_side-!gsu_ram)
    sm (!fx_contact_force-!gsu_ram),r0
    ibt r0,#$46
    add r8
    ldw (r0)
    inc r0
    sbk
    lm r0,(!fx_world_sum+2-!gsu_ram)
    moves r0,r0
    bne .lAB4B
    nop
    ibt r0,#$2C
    add r9
    ldw (r0)
    iwt r1,#$0020
    bic r1
    sbk
.lAB4B:
    lm r2,(!fx_wheel_flags-!gsu_ram)
    lm r0,(!fx_contact_depth-!gsu_ram)
    iwt r1,#$7D00
    cmp r1
    bne .lAB5F
    nop
    iwt r15,#.lAEE5
    nop
.lAB5F:
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r1,#$0100
    bic r1
    sbk
    iwt r0,#$0010
    to r1
    or r2
    sm (!fx_wheel_flags-!gsu_ram),r1
    lm r2,(!fx_wheel_flags-!gsu_ram)
    iwt r0,#$0008
    and r2
    beq .lABE5
    nop
    sub r0
    sm (!fx_wheel_load-!gsu_ram),r0
    ibt r0,#$50
    add r9
    ldw (r0)
    add r0
    lm r6,(!fx_contact_force-!gsu_ram)
    fmult
    ibt r6,#$3C
    with r6
    add r8
    to r6
    ldw (r6)
    ibt r1,#$4E
    with r1
    add r8
    to r1
    ldw (r1)
    with r1
    sub #2
    bpl .lABC8
    nop
    moves r6,r6
    beq .lABE5
    nop
    bmi .lABC8
    nop
    cmp r6
    bpl .lABC8
    nop
    move r6,r0
    iwt r0,#$0020
    to r1
    or r2
    sm (!fx_wheel_flags-!gsu_ram),r1
    ibt r1,#$56
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$1F40
    add r1
    sbk
    lm r0,(!fx_contact_force-!gsu_ram)
    div2
    sbk
.lABC8:
    with r6
    add r6
    sm (!fx_wheel_load-!gsu_ram),r6
    lms r0,(!fx_object_matrix_6-!gsu_ram)
    lms r1,(!fx_object_matrix_7-!gsu_ram)
    lms r2,(!fx_object_matrix_8-!gsu_ram)
    fmult
    move r4,r0
    from r1
    to r5
    fmult
    from r2
    to r6
    fmult
    link #4
    iwt r15,#FxSumsPushWorld
    nop
.lABE5:
    ibt r0,#$00
    romb
    ibt r0,#$24
    add r9
    ldw (r0)
    ibt r1,#$24
    with r1
    add r8
    to r1
    ldw (r1)
    sub r1
    hib
    move r1,r0
    iwt r2,#$F060
    iwt r3,#$0FA0
    lob
    iwt r14,#$96FE
    to r14
    add r14
    getb
    swap
    div2
    div2
    moves r0,r0
    bmi .lAC18
    nop
    cmp r3
    bmi .lAC1F
    nop
    move r0,r3
    bra .lAC1F
    nop
.lAC18:
    cmp r2
    bpl .lAC1F
    nop
    move r0,r2
.lAC1F:
    sm (!fx_view_matrix_2-!gsu_ram),r0
    move r0,r1
    lob
    iwt r14,#$973E
    to r14
    add r14
    getb
    swap
    moves r0,r0
    bpl .lAC34
    nop
    not
    inc r0
.lAC34:
    sm (!fx_view_matrix_8-!gsu_ram),r0
    ibt r1,#$2E
    with r1
    add r9
    to r1
    ldw (r1)
    with r1
    not
    inc r1
    ibt r2,#$30
    with r2
    add r9
    to r2
    ldw (r2)
    with r2
    not
    inc r2
    ibt r3,#$32
    with r3
    add r9
    to r3
    ldw (r3)
    with r3
    not
    inc r3
    lms r6,(!gsu_out_e4-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_1-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_object_matrix_2-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    sm (!fx_wheel_side-!gsu_ram),r0
    lms r6,(!fx_object_matrix_6-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_7-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_object_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    sm (!fx_wheel_ahead-!gsu_ram),r0
    move r1,r0
    ibt r0,#$26
    add r9
    ldw (r0)
    to r1
    add r1
    ibt r0,#$26
    add r9
    from r1
    stw (r0)
    lm r1,(!fx_wheel_side-!gsu_ram)
    lm r3,(!fx_wheel_ahead-!gsu_ram)
    lms r6,(!fx_view_matrix_8-!gsu_ram)
    with r1
    fmult
    lms r6,(!fx_view_matrix_2-!gsu_ram)
    from r3
    add r3
    fmult
    add r1
    add r0
    add r0
    move r4,r0
    move r1,r0
    ibt r5,#$50
    with r5
    add r9
    to r5
    ldw (r5)
    ibt r0,#$3C
    add r8
    ldw (r0)
    moves r0,r0
    bne .lACC6
    nop
    lm r6,(!fx_param_grip_free-!gsu_ram)
    from r5
    add r5
    fmult
    with r5
    add r0
.lACC6:
    ibt r1,#$3A
    with r1
    add r8
    to r1
    ldw (r1)
    moves r1,r1
    beq .lACE9
    nop
    ibt r2,#$34
    with r2
    add r8
    to r2
    ldw (r2)
    moves r2,r2
    bpl .lACE4
    nop
    moves r1,r1
    bpl .lACE9
    nop
    bra .lACFC
    nop
.lACE4:
    moves r1,r1
    bpl .lACFC
    nop
.lACE9:
    lm r2,(!fx_wheel_flags-!gsu_ram)
    iwt r0,#$0002
    and r2
    beq .lACFC
    nop
    lm r6,(!fx_param_grip_slide-!gsu_ram)
    with r5
    add r5
    with r5
    fmult
.lACFC:
    lm r2,(!fx_wheel_flags-!gsu_ram)
    ibt r1,#$42
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$000C
    and r1
    beq .lAD2F
    nop
    iwt r0,#$0002
    and r2
    beq .lAD1F
    nop
    lm r6,(!fx_param_grip_side-!gsu_ram)
    from r5
    add r5
    fmult
    with r5
    bra .lAD2F
    add r0
.lAD1F:
    iwt r0,#$0001
    and r2
    beq .lAD2F
    nop
    lm r6,(!fx_param_grip_ahead-!gsu_ram)
    from r5
    add r5
    fmult
    with r5
    add r0
.lAD2F:
    iwt r0,#$0002
    and r2
    beq .lAD3D
    nop
    iwt r0,#$4650
    sm (!fx_contact_force-!gsu_ram),r0
.lAD3D:
    lm r6,(!fx_contact_force-!gsu_ram)
    with r5
    add r5
    with r5
    fmult
    moves r0,r4
    bpl .lAD4C
    nop
    not
    inc r0
.lAD4C:
    sub r5
    bmi .lAD80
    nop
    move r2,r0
    moves r4,r4
    bpl .lAD5A
    nop
    with r5
    not
    inc r5
.lAD5A:
    move r1,r4
    move r4,r5
    from r1
    sub r5
    moves r1,r0
    bpl .lAD68
    nop
    with r1
    not
    inc r1
.lAD68:
    lm r0,(!fx_param_slide_limit-!gsu_ram)
    cmp r1
    bpl .lAD80
    nop
    ibt r0,#$56
    add r8
    ldw (r0)
    add r1
    sbk
    lm r2,(!fx_wheel_flags-!gsu_ram)
    iwt r0,#$0020
    or r2
    sbk
.lAD80:
    iwt r5,#$0000
    iwt r6,#$0000
    link #4
    iwt r15,#FxSumsPushLocal
    nop
    ibt r6,#$48
    with r6
    add r8
    to r6
    ldw (r6)
    moves r6,r6
    beq .lADC4
    nop
    lm r0,(!fx_wheel_ahead-!gsu_ram)
    add r0
    to r6
    fmult
    iwt r4,#$FF38
    iwt r5,#$00C8
    moves r6,r6
    bmi .lADB3
    nop
    with r6
    cmp r5
    bmi .lADBB
    nop
    move r6,r5
    bra .lADBB
    nop
.lADB3:
    with r6
    cmp r4
    bpl .lADBB
    nop
    move r6,r4
.lADBB:
    ibt r4,#$00
    ibt r5,#$00
    link #4
    iwt r15,#FxSumsPushLocal
    nop
.lADC4:
    link #4
    iwt r15,#FxSumsRotateLocal
    nop
    lm r0,(!fx_world_sum-!gsu_ram)
    lm r1,(!fx_world_sum+2-!gsu_ram)
    lm r2,(!fx_world_sum+4-!gsu_ram)
    or r1
    or r2
    bne .lADDE
    nop
    iwt r15,#.lAE39
    nop
.lADDE:
    nop
    ibt r1,#$36
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r2,#$38
    with r2
    add r9
    to r2
    ldw (r2)
    ibt r3,#$3A
    with r3
    add r9
    to r3
    ldw (r3)
    lm r6,(!fx_param_hit-!gsu_ram)
    link #4
    iwt r15,#FxHitAddScaled
    nop
    lm r2,(!fx_wheel_flags-!gsu_ram)
    iwt r0,#$0200
    and r2
    beq .lAE05
    nop
.lAE05:
    ibt r1,#$3C
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r2,#$3E
    with r2
    add r9
    to r2
    ldw (r2)
    ibt r3,#$40
    with r3
    add r9
    to r3
    ldw (r3)
    ibt r0,#$5A
    add r8
    ldw (r0)
    with r2
    add r0
    ibt r0,#$5C
    add r8
    ldw (r0)
    with r3
    add r0
    ibt r0,#$04
    romb
    lm r0,(!fx_param_push-!gsu_ram)
    iwt r14,#$9BC6
    with r14
    add r0
    getb
    inc r14
    getbh
    move r12,r0
    link #2
    jmp r12
    nop
.lAE39:
    lm r3,(!fx_param_bounce-!gsu_ram)
    move r2,r3
    with r2
    not
    inc r2
    lm r0,(!fx_contact_sum-!gsu_ram)
    div2
    div2
    div2
    moves r0,r0
    bmi .lAE5B
    nop
    cmp r3
    bmi .lAE62
    nop
    move r0,r3
    bra .lAE62
    nop
.lAE5B:
    cmp r2
    bpl .lAE62
    nop
    move r0,r2
.lAE62:
    move r1,r0
    ibt r0,#$4A
    add r9
    from r1
    stw (r0)
    lm r0,(!fx_contact_sum+4-!gsu_ram)
    div2
    div2
    div2
    moves r0,r0
    bmi .lAE82
    nop
    cmp r3
    bmi .lAE89
    nop
    move r0,r3
    bra .lAE89
    nop
.lAE82:
    cmp r2
    bpl .lAE89
    nop
    move r0,r2
.lAE89:
    move r1,r0
    ibt r0,#$4E
    add r9
    from r1
    stw (r0)
    lm r0,(!fx_contact_sum+2-!gsu_ram)
    div2
    div2
    div2
    move r4,r0
    moves r0,r0
    bmi .lAEAB
    nop
    cmp r3
    bmi .lAEB2
    nop
    move r0,r3
    bra .lAEB2
    nop
.lAEAB:
    cmp r2
    bpl .lAEB2
    nop
    move r0,r2
.lAEB2:
    move r1,r0
    ibt r0,#$4C
    add r9
    from r1
    stw (r0)
.lAEB9:
    ibt r2,#$46
    with r2
    add r9
    to r2
    ldw (r2)
    iwt r0,#$0002
    and r2
    beq .lAED7
    nop
    ibt r1,#$24
    with r1
    add r9
    to r1
    ldw (r1)
    ibt r0,#$3A
    add r8
    ldw (r0)
    with r1
    add r0
    ibt r0,#$24
    add r9
    from r1
    stw (r0)
.lAED7:
    lm r1,(!fx_wheel_flags-!gsu_ram)
    ibt r0,#$46
    add r9
    from r1
    stw (r0)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lAEE5:
    ibt r1,#$00
    ibt r0,#$4A
    add r9
    from r1
    stw (r0)
    ibt r0,#$4C
    add r9
    from r1
    stw (r0)
    ibt r0,#$4E
    add r9
    from r1
    stw (r0)
    ibt r0,#$44
    add r8
    ldw (r0)
    inc r0
    sbk
    bra .lAEB9
    nop
;
; @routine 04:AEFF FxContactsResolve
; WHAT IT DOES: Resolves the contacts in the twenty slots at $23AE:
; each slot in use goes through FxContactResolve and its upward force
; ($23A0) is kept in $2386. R0 = $2480, the contact flags gathered.
; HOW: Pushes R11; ROMB 0; $2480 = 0 and $239C = the wheel's +$2C
; first; CACHE; R13 counts the slots to $14; returns by pulling R15.
; Entry: R8 = the car; R10 = stack; R11 = return address.
; Exit: R0 = $2480; the wheel's +$2C = $239C.
%at($04AEFF)
FxContactsResolve:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r0,#$00
    romb
    sub r0
    sm (!fx_push_flags-!gsu_ram),r0
    ibt r0,#$2C
    add r8
    ldw (r0)
    sm (!fx_hit_depth-!gsu_ram),r0
    iwt r3,#$23AE
    ibt r13,#$00
    cache
.lAF1A:
    inc r13
    ibt r0,#$14
    cmp r13
    bne .lAF26
    nop
    iwt r15,#.lAF3C
    nop
.lAF26:
    ldw (r3)
    moves r0,r0
    beq .lAF4E
    nop
    link #4
    iwt r15,#FxContactResolve
    nop
    lm r0,(!fx_contact_force_up-!gsu_ram)
    sm (!fx_contact_force-!gsu_ram),r0
    bra .lAF1A
    nop
.lAF3C:
    lm r1,(!fx_hit_depth-!gsu_ram)
    ibt r0,#$2C
    add r8
    from r1
    stw (r0)
    lm r0,(!fx_push_flags-!gsu_ram)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lAF4E:
    ibt r0,#$0A
    with r3
    bra .lAF1A
    add r0
;
; @routine 04:AF54 FxContactResolve
; WHAT IT DOES: Resolves one contact slot (R3): marks its flags in
; $2480, notes the shallowest depth in $237E, and from the depth and
; the face's normal works the force pushing the wheel out ($239E
; across, $23A0 up). The code after it to 04:B9AB holds the car
; handler (FxCarHandler, 04:B1A1) and its helpers.
; HOW: Pushes R11 for slots it resolves; R3 steps through the slot;
; returns by pulling R15 (or JMP R11 for an empty or skipped slot,
; 04:B185-B19A).
; Entry: R3 = the slot; R8 = the car; R10 = stack; R11 = return
; address.
; Exit: R3 past the slot; the rest as the code leaves them.
%at($04AF54)
FxContactResolve:
    ldw (r3)
    inc r3
    inc r3
    moves r1,r0
    bne .lAF60
    nop
    iwt r15,#.lB19A
    nop
.lAF60:
    lm r2,(!fx_push_flags-!gsu_ram)
    or r2
    sbk
    from r1
    hib
    beq .lAF6F
    nop
    iwt r15,#.lB18A
    nop
.lAF6F:
    from r11
    stw (r10)
    inc r10
    inc r10
    ldw (r3)
    inc r3
    inc r3
    moves r7,r0
    bpl .lAF7F
    nop
    iwt r15,#.lB191
    nop
.lAF7F:
    lm r0,(!fx_contact_depth-!gsu_ram)
    moves r5,r0
    bpl .lAF8B
    nop
    with r5
    not
    inc r5
.lAF8B:
    moves r4,r7
    bpl .lAF93
    nop
    with r4
    not
    inc r4
.lAF93:
    with r5
    cmp r4
    bmi .lAF9D
    nop
    sm (!fx_contact_depth-!gsu_ram),r7
.lAF9D:
    iwt r0,#$7D00
    cmp r7
    bne .lAFA9
    nop
    iwt r15,#.lB191
    nop
.lAFA9:
    with r7
    add r7
    with r7
    add r7
    with r7
    add r7
    with r7
    add r7
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
    from r4
    or r6
    bne .lAFD2
    nop
    lm r0,(!fx_wheel_contact-!gsu_ram)
    moves r0,r0
    beq .lAFCF
    nop
    iwt r15,#.lB185
    nop
.lAFCF:
    ibt r0,#$01
    sbk
.lAFD2:
    lm r0,(!fx_push_turn-!gsu_ram)
    moves r0,r0
    bne .lAFDF
    nop
    iwt r15,#.lB06C
    nop
.lAFDF:
    iwt r1,#$0040
    cmp r1
    bne .lAFF3
    nop
    move r0,r6
    not
    inc r0
    move r6,r4
    move r4,r0
    iwt r15,#.lB06C
    nop
.lAFF3:
    iwt r1,#$00C0
    cmp r1
    bne .lB006
    nop
    move r0,r4
    not
    inc r0
    move r4,r6
    move r6,r0
    bra .lB06C
    nop
.lB006:
    iwt r1,#$0080
    cmp r1
    bne .lB017
    nop
    with r4
    not
    inc r4
    with r6
    not
    inc r6
    bra .lB06C
    nop
.lB017:
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
    beq .lB04C
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
.lB04C:
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
.lB06C:
    with r3
    sub #6
    from r4
    stw (r3)
    inc r3
    inc r3
    from r5
    stw (r3)
    inc r3
    inc r3
    from r6
    stw (r3)
    inc r3
    inc r3
    sm (!fx_sizes-!gsu_ram),r4
    sm (!fx_sizes+2-!gsu_ram),r5
    sm (!fx_sizes+4-!gsu_ram),r6
    ibt r0,#$2E
    add r9
    ldw (r0)
    add r0
    lm r6,(!fx_sizes-!gsu_ram)
    fmult
    move r1,r0
    ibt r0,#$30
    add r9
    ldw (r0)
    add r0
    lm r6,(!fx_sizes+2-!gsu_ram)
    fmult
    with r1
    add r0
    ibt r0,#$32
    add r9
    ldw (r0)
    add r0
    lm r6,(!fx_sizes+4-!gsu_ram)
    fmult
    with r1
    add r0
    with r1
    not
    inc r1
    move r2,r1
    ibt r0,#$42
    add r9
    from r1
    stw (r0)
    move r0,r1
    ibt r6,#$44
    with r6
    add r9
    to r6
    ldw (r6)
    lmult
    lob
    swap
    with r4
    hib
    or r4
    moves r0,r0
    bpl .lB0DB
    nop
    div2
    add r7
    lm r1,(!fx_wheel_flags-!gsu_ram)
    iwt r4,#$0100
    with r4
    and r1
    beq .lB0E4
    nop
    bra .lB0E4
    sub r0
.lB0DB:
    iwt r1,#$07D0
    cmp r1
    bpl .lB0E4
    nop
    add r7
.lB0E4:
    move r1,r0
    lm r6,(!fx_sizes+2-!gsu_ram)
    move r0,r1
    fmult
    move r5,r0
    moves r6,r6
    bne .lB133
    nop
    moves r0,r2
    bpl .lB0FA
    nop
    sub r0
.lB0FA:
    add r0
    add r0
    add r7
    move r1,r0
    iwt r6,#$1000
    add r0
    fmult
    with r1
    add r0
    moves r6,r0
    bpl .lB10E
    nop
    with r6
    not
    inc r6
.lB10E:
    iwt r0,#$0064
    cmp r6
    bpl .lB133
    nop
    ibt r0,#$3C
    add r9
    ldw (r0)
    moves r0,r0
    bpl .lB12A
    nop
    lm r0,(!fx_hit_depth-!gsu_ram)
    iwt r6,#$0100
    or r6
    bra .lB133
    sbk
.lB12A:
    lm r0,(!fx_hit_depth-!gsu_ram)
    iwt r6,#$0200
    or r6
    sbk
.lB133:
    lm r0,(!fx_sizes-!gsu_ram)
    lm r6,(!fx_sizes+4-!gsu_ram)
    or r6
    beq .lB148
    nop
    lm r0,(!fx_hit_depth-!gsu_ram)
    iwt r6,#$0400
    or r6
    sbk
.lB148:
    lm r6,(!fx_sizes-!gsu_ram)
    move r0,r1
    fmult
    move r4,r0
    lm r6,(!fx_sizes+4-!gsu_ram)
    move r0,r1
    fmult
    move r6,r0
    link #4
    iwt r15,#FxSumsPushWorld
    nop
    lm r6,(!fx_sizes-!gsu_ram)
    from r7
    fmult
    lm r1,(!fx_contact_sum-!gsu_ram)
    add r1
    sbk
    lm r6,(!fx_sizes+2-!gsu_ram)
    from r7
    fmult
    move r2,r0
    lm r1,(!fx_contact_sum+2-!gsu_ram)
    add r1
    sbk
    lm r6,(!fx_sizes+4-!gsu_ram)
    from r7
    fmult
    lm r1,(!fx_contact_sum+4-!gsu_ram)
    add r1
    sbk
.lB185:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB18A:
    ibt r0,#$08
    with r3
    add r0
    move r15,r11
    nop
.lB191:
    ibt r0,#$06
    with r3
    add r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB19A:
    ibt r0,#$08
    with r3
    add r0
    move r15,r11
    nop
;
; @routine 04:B1A1 FxCarHandler
; WHAT IT DOES: The record handler of a car: steering from the input
; (FxSteerFromInput), its pose (FxCarPose), its parts (FxPartsUpdate),
; the move (FxRecordMove) and its collisions (FxCarCollide, FxCarHit),
; holding it on the ground (FxRecordHoldY, FxRecordAlignment), and its
; place on the course (FxCarFollowCourse, FxLapSegment,
; FxCourseNextDistance).
; HOW: Pushes R11 and R8; returns by pulling R15.
; Entry: R8 = the car; R10 = stack; R11 = return address.
; Exit: R8 back; the rest as the last call leaves them.
%at($04B1A1)
FxCarHandler:
    from r11
    stw (r10)
    inc r10
    inc r10
    iwt r1,#$0004
    ibt r0,#$1C
    add r8
    ldw (r0)
    or r1
    sbk
    ibt r0,#$3E
    add r8
    ldw (r0)
.lB1B2:
    moves r2,r0
    beq .lB1C4
    nop
    ibt r0,#$1C
    add r2
    ldw (r0)
    or r1
    sbk
    ibt r0,#$34
    add r2
    ldw (r0)
    bra .lB1B2
    nop
.lB1C4:
    lm r0,(!fx_init_2472-!gsu_ram)
    moves r0,r0
    beq .lB1D7
    nop
    link #4
    iwt r15,#FxCarPose
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB1D7:
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r1,#$0002
    bic r1
    sbk
    ibt r1,#$00
    iwt r0,#$0092
    add r8
    from r1
    stw (r0)
    ibt r0,#$09
    romb
    ibt r0,#$6E
    add r8
    ldw (r0)
    move r14,r0
    iwt r1,#$2652
    iwt r12,#$0019
    move r13,r15
    getb
    inc r14
    getbh
    inc r14
    stw (r1)
    inc r1
    inc r1
    loop
    nop
    ibt r1,#$1C
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0020
    and r1
    beq .lB216
    nop
    iwt r15,#.lB990
    nop
.lB216:
    nop
    iwt r0,#$0010
    and r1
    beq .lB23B
    nop
    ibt r0,#$64
    add r8
    ldw (r0)
    iwt r1,#$0008
    and r1
    bne .lB22D
    nop
    iwt r15,#.lB990
    nop
.lB22D:
    link #4
    iwt r15,#FxCarFollowCourse
    nop
    moves r0,r0
    bne .lB23B
    nop
    iwt r15,#.lB990
    nop
.lB23B:
    ibt r1,#$06
    link #4
    iwt r15,#FxRecordMove
    nop
    from r8
    stw (r10)
    inc r10
    inc r10
    ibt r0,#$26
    add r8
    ldw (r0)
    not
    inc r0
    sms (!gsu_in_20-!gsu_ram),r0
    ibt r0,#$24
    add r8
    ldw (r0)
    not
    inc r0
    sms (!gsu_in_22-!gsu_ram),r0
    ibt r0,#$28
    add r8
    ldw (r0)
    not
    inc r0
    sms (!gsu_in_24-!gsu_ram),r0
    iwt r9,#$00E4
    link #4
    iwt r15,#FxMatrixFromAngles
    nop
    iwt r9,#$00E4
    link #4
    iwt r15,#FxMatrixTranspose04
    nop
    dec r10
    dec r10
    to r8
    ldw (r10)
    ibt r0,#$64
    add r8
    ldw (r0)
    iwt r1,#$0008
    and r1
    beq .lB28E
    nop
    link #4
    iwt r15,#FxCarFollowCourse
    nop
    moves r0,r0
    bne .lB29A
    nop
    iwt r15,#.lB990
    nop
.lB28E:
    link #4
    iwt r15,#FxRecordSegment
    nop
    move r7,r0
    ibt r0,#$58
    add r8
    from r7
    stw (r0)
.lB29A:
    sub r0
    sm (!fx_hit-!gsu_ram),r0
    sm (!fx_hit+2-!gsu_ram),r0
    sm (!fx_hit+4-!gsu_ram),r0
    sm (!fx_spin+2-!gsu_ram),r0
    sm (!fx_spin-!gsu_ram),r0
    sm (!fx_spin+4-!gsu_ram),r0
    ibt r0,#$4A
    add r8
    ldw (r0)
    lm r6,(!fx_param_spin_damp-!gsu_ram)
    fmult
    bpl .lB2C1
    nop
    not
    inc r0
.lB2C1:
    iwt r6,#$0078
    with r6
    add r0
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r1,#$0008
    and r1
    bne .lB2D6
    nop
    iwt r0,#$003C
    with r6
    add r0
.lB2D6:
    sm (!fx_hit+2-!gsu_ram),r6
    ibt r0,#$1E
    add r8
    ldw (r0)
    sm (!fx_car_pos-!gsu_ram),r0
    ibt r0,#$20
    add r8
    ldw (r0)
    sm (!fx_car_pos+2-!gsu_ram),r0
    ibt r0,#$22
    add r8
    ldw (r0)
    sm (!fx_car_pos+4-!gsu_ram),r0
    iwt r0,#$008A
    add r8
    ldw (r0)
    sm (!fx_car_pos+6-!gsu_ram),r0
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r1,#$0100
    iwt r2,#$086A
    bic r2
    or r1
    sbk
    ibt r2,#$42
    with r2
    add r8
    to r2
    ldw (r2)
    ibt r1,#$64
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0008
    and r1
    beq .lB337
    nop
    ibt r0,#$7C
    add r8
    ldw (r0)
    moves r0,r0
    beq .lB32E
    nop
    iwt r3,#$3E80
    iwt r7,#$3E80
    bra .lB37D
    nop
.lB32E:
    iwt r3,#$7D00
    iwt r7,#$7D00
    bra .lB37D
    nop
.lB337:
    iwt r3,#$3E80
    iwt r7,#$4650
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r1,#$0010
    and r1
    beq .lB34C
    nop
    iwt r15,#.lB37D
    nop
.lB34C:
    lm r1,(!fx_car_pos+6-!gsu_ram)
    iwt r0,#$0004
    and r1
    beq .lB360
    nop
    iwt r3,#$445C
    iwt r7,#$4524
    bra .lB37D
    nop
.lB360:
    iwt r0,#$0001
    and r1
    beq .lB370
    nop
    iwt r3,#$3E80
    iwt r7,#$4074
    bra .lB37D
    nop
.lB370:
    iwt r0,#$0042
    and r1
    beq .lB37D
    nop
    iwt r3,#$4268
    iwt r7,#$3A98
.lB37D:
    sm (!fx_contact_force_side-!gsu_ram),r3
    sm (!fx_contact_force_up-!gsu_ram),r7
    iwt r3,#$00F0
    iwt r0,#$0400
    and r2
    beq .lB392
    nop
    iwt r3,#$014A
.lB392:
    ibt r4,#$40
    with r4
    add r8
    to r4
    ldw (r4)
    iwt r0,#$2000
    and r4
    bne .lB3D5
    nop
    lm r0,(!fx_car_pos+6-!gsu_ram)
    iwt r1,#$0080
    and r1
    bne .lB3BA
    nop
    iwt r0,#$0010
    and r4
    bne .lB3D5
    nop
    ibt r0,#$46
    add r8
    ldw (r0)
    moves r0,r0
    beq .lB3D5
    nop
.lB3BA:
    ibt r0,#$4A
    add r8
    ldw (r0)
    moves r0,r0
    bpl .lB3C5
    nop
    not
    inc r0
.lB3C5:
    cmp r3
    bmi .lB3D5
    nop
    iwt r1,#$0040
    with r2
    or r1
    iwt r1,#$0430
    with r2
    bic r1
.lB3D5:
    ibt r0,#$44
    add r8
    ldw (r0)
    sub #3
    bmi .lB3ED
    nop
    iwt r1,#$0430
    with r2
    bic r1
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r1,#$0008
    or r1
    sbk
.lB3ED:
    ibt r0,#$54
    add r8
    ldw (r0)
    moves r0,r0
    bne .lB401
    nop
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r1,#$0004
    and r1
    beq .lB407
    nop
.lB401:
    iwt r1,#$043F
    with r2
    bic r1
.lB407:
    ibt r0,#$5E
    add r8
    ldw (r0)
    moves r0,r0
    beq .lB415
    nop
    dec r0
    sbk
    bra .lB436
    nop
.lB415:
    iwt r0,#$0080
    and r2
    beq .lB436
    nop
    lm r1,(!fx_param_lift-!gsu_ram)
    moves r1,r1
    beq .lB436
    nop
    lm r0,(!fx_hit+2-!gsu_ram)
    add r1
    sm (!fx_hit+2-!gsu_ram),r0
    iwt r1,#$0020
    ibt r0,#$5E
    add r8
    from r1
    stw (r0)
.lB436:
    link #4
    iwt r15,#FxSteerFromInput
    nop
    ibt r1,#$70
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0008
    and r1
    beq .lB44A
    nop
    with r3
    add r3
.lB44A:
    ibt r0,#$3A
    add r8
    from r3
    stw (r0)
    ibt r1,#$00
    iwt r0,#$0020
    and r2
    beq .lB45F
    nop
    lm r1,(!fx_param_reverse-!gsu_ram)
    with r1
    not
    inc r1
.lB45F:
    iwt r0,#$0400
    and r2
    beq .lB4A4
    nop
    lm r1,(!fx_param_thrust_boost-!gsu_ram)
    ibt r6,#$6A
    with r6
    add r8
    to r6
    ldw (r6)
    moves r6,r6
    beq .lB4AB
    nop
    ibt r4,#$40
    with r4
    add r8
    to r4
    ldw (r4)
    iwt r0,#$2000
    and r4
    bne .lB49D
    nop
    lm r0,(!fx_param_speed_cap-!gsu_ram)
    with r6
    sub r0
    bpl .lB48D
    nop
    ibt r6,#$00
.lB48D:
    lm r0,(!gsu_control_26f2-!gsu_ram)
    iwt r1,#$0200
    and r1
    bne .lB49D
    nop
    ibt r0,#$6A
    add r8
    from r6
    stw (r0)
.lB49D:
    lm r1,(!fx_param_thrust-!gsu_ram)
    bra .lB4AF
    nop
.lB4A4:
    iwt r0,#$0010
    and r2
    beq .lB4AF
    nop
.lB4AB:
    lm r1,(!fx_param_thrust_boost-!gsu_ram)
.lB4AF:
    ibt r4,#$40
    with r4
    add r8
    to r4
    ldw (r4)
    iwt r0,#$2000
    and r4
    beq .lB4C0
    nop
    with r1
    add r1
    with r1
    add r1
.lB4C0:
    ibt r0,#$78
    add r8
    from r1
    stw (r0)
    iwt r0,#$0009
    romb
    lm r14,(!fx_param_curve_a-!gsu_ram)
    ibt r4,#$01
    ibt r5,#$00
    ibt r3,#$4A
    with r3
    add r8
    to r3
    ldw (r3)
    moves r3,r3
    bpl .lB4F1
    nop
    iwt r0,#$0410
    and r2
    beq .lB4EC
    nop
    ibt r4,#$01
    ibt r5,#$0F
    iwt r15,#.lB562
    nop
.lB4EC:
    ibt r4,#$00
    bra .lB506
    nop
.lB4F1:
    getb
    inc r14
    getbh
    inc r14
    moves r0,r0
    beq .lB505
    nop
    cmp r3
    bpl .lB506
    nop
    move r5,r0
    bra .lB4F1
    inc r4
.lB505:
    dec r4
.lB506:
    lm r0,(!fx_init_2470-!gsu_ram)
    and #3
    bne .lB529
    nop
    ibt r0,#$4E
    add r8
    ldw (r0)
    cmp r4
    beq .lB529
    nop
    from r4
    sbk
    from r4
    sub #4
    bne .lB529
    nop
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r6,#$0002
    or r6
    sbk
.lB529:
    lm r14,(!fx_param_curve_b-!gsu_ram)
    ibt r0,#$4E
    add r8
    ldw (r0)
    dec r0
    iwt r6,#$0011
    mult r6
    with r14
    add r0
    sub r0
    to r6
    getbh
    move r4,r6
    inc r14
    ibt r0,#$4A
    add r8
    ldw (r0)
    moves r0,r0
    bpl .lB54A
    nop
    not
    inc r0
.lB54A:
    sub r5
    add r0
    fmult
    ibt r3,#$0F
    cmp r3
    bmi .lB556
    nop
    move r0,r3
.lB556:
    move r5,r0
    with r14
    add r0
    sub r0
    to r6
    getbh
    with r1
    add r1
    with r1
    fmult
.lB562:
    ibt r0,#$3C
    add r8
    from r1
    stw (r0)
    ibt r1,#$50
    with r1
    add r8
    to r1
    ldw (r1)
    move r0,r5
    ibt r6,#$40
    lmult
    from r4
    sub r1
    div2
    div2
    add r1
    sbk
    iwt r0,#$0800
    and r2
    beq .lB5AE
    nop
    from r2
    stw (r10)
    inc r10
    inc r10
    iwt r4,#$0000
    iwt r5,#$0000
    iwt r6,#$0258
    link #4
    iwt r15,#FxSumsPushLocal
    nop
    link #4
    iwt r15,#FxSumsRotateLocal
    nop
    iwt r1,#$0000
    iwt r2,#$0000
    iwt r3,#$0000
    iwt r6,#$7FFF
    link #4
    iwt r15,#FxHitAddScaled
    nop
    dec r10
    dec r10
    to r2
    ldw (r10)
.lB5AE:
    iwt r1,#$0000
    iwt r0,#$0410
    and r2
    bne .lB5D6
    nop
    ibt r0,#$4A
    add r8
    ldw (r0)
    add r0
    add r0
    add r0
    bpl .lB5C4
    nop
    not
    inc r0
.lB5C4:
    lm r6,(!fx_param_turn_grip-!gsu_ram)
    add r0
    fmult
    iwt r1,#$0800
    cmp r1
    bpl .lB5D4
    nop
    move r0,r1
.lB5D4:
    move r1,r0
.lB5D6:
    iwt r0,#$0040
    and r2
    beq .lB5FF
    nop
    ibt r0,#$4A
    add r8
    ldw (r0)
    moves r0,r0
    bmi .lB5FF
    nop
    move r3,r0
    iwt r0,#$00C8
    cmp r3
    bpl .lB5FB
    nop
    ibt r1,#$40
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0800
    or r1
    sbk
.lB5FB:
    lm r1,(!fx_param_wheel_48-!gsu_ram)
.lB5FF:
    ibt r0,#$48
    add r8
    from r1
    stw (r0)
    ibt r0,#$42
    add r8
    from r2
    stw (r0)
    iwt r1,#$0040
    ibt r0,#$2C
    add r8
    from r1
    stw (r0)
    link #4
    iwt r15,#FxCarMoveParts
    nop
    link #4
    iwt r15,#FxPartsUpdate
    nop
    link #4
    iwt r15,#FxRecordHoldY
    nop
    ibt r2,#$42
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0400
    and r2
    beq .lB653
    nop
    lm r2,(!fx_param_jump-!gsu_ram)
    moves r2,r2
    beq .lB653
    nop
    ibt r0,#$4E
    add r8
    ldw (r0)
    ibt r1,#$02
    cmp r1
    bpl .lB653
    nop
    ibt r0,#$26
    add r8
    ldw (r0)
    iwt r1,#$EAAB
    cmp r1
    bmi .lB653
    nop
    lm r0,(!fx_spin+2-!gsu_ram)
    sub r2
    sbk
.lB653:
    link #4
    iwt r15,#FxCarCollide
    nop
    link #4
    iwt r15,#FxRecordAlignment
    nop
    link #4
    iwt r15,#FxCarHit
    nop
    ibt r1,#$70
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0080
    and r1
    beq .lB6C1
    nop
    ibt r0,#$58
    add r8
    ldw (r0)
    moves r0,r0
    bne .lB67C
    nop
    iwt r15,#.lB6B7
    nop
.lB67C:
    iwt r1,#$008A
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0100
    and r1
    bne .lB68E
    nop
    iwt r15,#.lB6B7
    nop
.lB68E:
    link #4
    iwt r15,#FxRecordHeadingAhead
    nop
    move r2,r0
    ibt r1,#$24
    with r1
    add r8
    to r1
    ldw (r1)
    sub r1
    div2
    div2
    add r1
    sbk
    sub r0
    sm (!fx_spin-!gsu_ram),r0
    move r3,r0
    ibt r0,#$34
    add r8
    from r3
    stw (r0)
    from r1
    sub r2
    hib
    sex
    div2
    bne .lB6C1
    nop
.lB6B7:
    iwt r1,#$0080
    ibt r0,#$70
    add r8
    ldw (r0)
    bic r1
    sbk
.lB6C1:
    iwt r1,#$F448
    iwt r2,#$0BB8
    lm r0,(!fx_hit+2-!gsu_ram)
    moves r0,r0
    bmi .lB6DA
    nop
    cmp r2
    bmi .lB6E1
    nop
    move r0,r2
    bra .lB6E1
    nop
.lB6DA:
    cmp r1
    bpl .lB6E1
    nop
    move r0,r1
.lB6E1:
    sbk
    lm r0,(!fx_spin+4-!gsu_ram)
    moves r0,r0
    bmi .lB6F5
    nop
    cmp r2
    bmi .lB6FC
    nop
    move r0,r2
    bra .lB6FC
    nop
.lB6F5:
    cmp r1
    bpl .lB6FC
    nop
    move r0,r1
.lB6FC:
    sbk
    lm r0,(!fx_spin+2-!gsu_ram)
    moves r0,r0
    bmi .lB710
    nop
    cmp r2
    bmi .lB717
    nop
    move r0,r2
    bra .lB717
    nop
.lB710:
    cmp r1
    bpl .lB717
    nop
    move r0,r1
.lB717:
    sbk
    lm r0,(!fx_spin-!gsu_ram)
    moves r0,r0
    bmi .lB72B
    nop
    cmp r2
    bmi .lB732
    nop
    move r0,r2
    bra .lB732
    nop
.lB72B:
    cmp r1
    bpl .lB732
    nop
    move r0,r1
.lB732:
    sbk
    ibt r2,#$00
    ibt r0,#$28
    add r8
    ldw (r0)
    moves r0,r0
    bmi .lB750
    nop
    iwt r1,#$2000
    cmp r1
    bmi .lB75F
    nop
    from r1
    sbk
    ibt r0,#$38
    add r8
    from r2
    stw (r0)
    bra .lB75F
    nop
.lB750:
    iwt r1,#$E000
    cmp r1
    bpl .lB75F
    nop
    from r1
    sbk
    ibt r0,#$38
    add r8
    from r2
    stw (r0)
.lB75F:
    ibt r1,#$40
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0000
    and r1
    bne .lB799
    nop
    ibt r0,#$26
    add r8
    ldw (r0)
    moves r0,r0
    bmi .lB787
    nop
    iwt r1,#$2000
    cmp r1
    bmi .lB7CA
    nop
    from r1
    sbk
    ibt r0,#$36
    add r8
    from r2
    stw (r0)
    bra .lB7CA
    nop
.lB787:
    iwt r1,#$E000
    cmp r1
    bpl .lB7CA
    nop
    from r1
    sbk
    ibt r0,#$36
    add r8
    from r2
    stw (r0)
    bra .lB7CA
    nop
.lB799:
    ibt r0,#$26
    add r8
    ldw (r0)
    moves r0,r0
    bpl .lB7A4
    nop
    not
    inc r0
.lB7A4:
    iwt r1,#$4000
    cmp r1
    bmi .lB7CA
    nop
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r1,#$0000
    or r1
    sbk
    ibt r0,#$76
    add r8
    ldw (r0)
    moves r0,r0
    bne .lB7C1
    nop
    ibt r0,#$28
    sbk
.lB7C1:
    lm r0,(!fx_spin-!gsu_ram)
    not
    inc r0
    bra .lB7D4
    sbk
.lB7CA:
    ibt r0,#$40
    add r8
    ldw (r0)
    iwt r1,#$0000
    bic r1
    sbk
.lB7D4:
    ibt r0,#$4A
    add r8
    ldw (r0)
    moves r0,r0
    bpl .lB7DF
    nop
    not
    inc r0
.lB7DF:
    iwt r1,#$0258
    cmp r1
    bmi .lB80F
    sub r0
    sm (!fx_hit-!gsu_ram),r0
    sm (!fx_hit+4-!gsu_ram),r0
    ibt r1,#$2E
    with r1
    add r8
    to r1
    ldw (r1)
    from r1
    div2
    div2
    div2
    from r1
    sub r0
    sbk
    ibt r1,#$32
    with r1
    add r8
    to r1
    ldw (r1)
    from r1
    div2
    div2
    div2
    from r1
    sub r0
    sbk
.lB80F:
    cache
    iwt r4,#$2334
    iwt r2,#$002E
    with r2
    add r8
    ibt r12,#$06
    move r13,r15
    ldw (r4)
    inc r4
    inc r4
    to r3
    ldw (r2)
    add r3
    stw (r2)
    inc r2
    inc r2
    loop
    nop
    ibt r2,#$64
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0008
    and r2
    beq .lB838
    nop
    iwt r15,#.lB920
    nop
.lB838:
    ibt r2,#$42
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0430
    and r2
    beq .lB849
    nop
    iwt r15,#.lB8BD
    nop
.lB849:
    ibt r2,#$2C
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0400
    and r2
    beq .lB85A
    nop
    iwt r15,#.lB8BD
    nop
.lB85A:
    ibt r0,#$2E
    add r8
    ldw (r0)
    moves r0,r0
    bmi .lB870
    nop
    iwt r2,#$00C8
    cmp r2
    bpl .lB87A
    nop
    ibt r0,#$00
    bra .lB87A
    nop
.lB870:
    iwt r2,#$FF38
    cmp r2
    bmi .lB87A
    nop
    ibt r0,#$00
.lB87A:
    sbk
    ibt r0,#$32
    add r8
    ldw (r0)
    moves r0,r0
    bmi .lB891
    nop
    iwt r2,#$00C8
    cmp r2
    bpl .lB89B
    nop
    ibt r0,#$00
    bra .lB89B
    nop
.lB891:
    iwt r2,#$FF38
    cmp r2
    bmi .lB89B
    nop
    ibt r0,#$00
.lB89B:
    sbk
    ibt r0,#$30
    add r8
    ldw (r0)
    moves r0,r0
    bmi .lB8B2
    nop
    iwt r2,#$0032
    cmp r2
    bpl .lB8BC
    nop
    ibt r0,#$00
    bra .lB8BC
    nop
.lB8B2:
    iwt r2,#$FFCE
    cmp r2
    bmi .lB8BC
    nop
    ibt r0,#$00
.lB8BC:
    sbk
.lB8BD:
    iwt r1,#$0034
    with r1
    add r8
    ldw (r1)
    inc r1
    inc r1
    moves r0,r0
    bmi .lB8D7
    nop
    iwt r2,#$0040
    cmp r2
    bpl .lB8E1
    nop
    ibt r0,#$00
    bra .lB8E1
    nop
.lB8D7:
    iwt r2,#$FFC0
    cmp r2
    bmi .lB8E1
    nop
    ibt r0,#$00
.lB8E1:
    sbk
    ldw (r1)
    inc r1
    inc r1
    moves r0,r0
    bmi .lB8F7
    nop
    iwt r2,#$0040
    cmp r2
    bpl .lB901
    nop
    ibt r0,#$00
    bra .lB901
    nop
.lB8F7:
    iwt r2,#$FFC0
    cmp r2
    bmi .lB901
    nop
    ibt r0,#$00
.lB901:
    sbk
    ldw (r1)
    moves r0,r0
    bmi .lB915
    nop
    iwt r2,#$0040
    cmp r2
    bpl .lB91F
    nop
    ibt r0,#$00
    bra .lB91F
    nop
.lB915:
    iwt r2,#$FFC0
    cmp r2
    bmi .lB91F
    nop
    ibt r0,#$00
.lB91F:
    sbk
.lB920:
    ibt r1,#$2E
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r2,#$30
    with r2
    add r8
    to r2
    ldw (r2)
    ibt r3,#$32
    with r3
    add r8
    to r3
    ldw (r3)
    lms r6,(!fx_object_matrix_6-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_7-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_object_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    div2
    div2
    div2
    move r1,r0
    ibt r0,#$4A
    add r8
    from r1
    stw (r0)
    ibt r0,#$04
    romb
    ibt r2,#$50
    with r2
    add r8
    to r2
    ldw (r2)
    ibt r0,#$44
    add r8
    ldw (r0)
    ibt r1,#$04
    cmp r1
    bmi .lB97F
    nop
    with r2
    add r2
    ibt r1,#$28
    with r1
    add r8
    to r1
    ldw (r1)
    move r0,r1
    div2
    div2
    with r1
    sub r0
    ibt r0,#$28
    add r8
    from r1
    stw (r0)
.lB97F:
    ibt r1,#$52
    with r1
    add r8
    to r1
    ldw (r1)
    from r2
    sub r1
    div2
    to r1
    add r1
    ibt r0,#$52
    add r8
    from r1
    stw (r0)
.lB990:
    link #4
    iwt r15,#FxLapSegment
    nop
    iwt r0,#$0086
    add r8
    ldw (r0)
    link #4
    iwt r15,#FxCourseNextDistance
    nop
    move r1,r0
    iwt r0,#$0084
    add r8
    from r1
    stw (r0)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:BD20 FxCarCollide
; WHAT IT DOES: Collides car R8 with each record in its touch list
; (+$18): another car's model (FxCollideModel with the roles swapped),
; then if any slot holds a touch (FxContactsAnyTouch) the pushes are
; applied (FxPushesApply), the world sum turned back and doubled, and
; the car pushed at its place (FxHitAddUnit, FxPushAt4) and the other
; record too (FxCollidePush, FxPushApply). Records whose flags rule
; them out (+$1C bit 6, +$1A bit 1, +$16 bit 5) are skipped.
; HOW: Pushes R11 and each entry; $247E, $239C, $2378 and $26F6
; cleared first; returns by pulling R15.
; Entry: R8 = the car; R10 = stack; R11 = return address.
; Exit: as the last collision leaves them.
%at($04BD20)
FxCarCollide:
    from r11
    stw (r10)
    inc r10
    inc r10
    sub r0
    sm (!fx_push_turn-!gsu_ram),r0
    sm (!fx_hit_depth-!gsu_ram),r0
    sm (!fx_push_amount-!gsu_ram),r0
    sm (!fx_closing-!gsu_ram),r0
    lm r0,(!fx_hit_on-!gsu_ram)
    moves r0,r0
    bne .lBD46
    nop
    ibt r1,#$00
    iwt r0,#$0094
    add r8
    from r1
    stw (r0)
.lBD46:
    move r9,r8
    ibt r0,#$18
    add r8
    ldw (r0)
    moves r1,r0
    bne .lBD55
    nop
    iwt r15,#.lBF42
    nop
.lBD55:
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r2,#$0040
    and r2
    beq .lBD64
    nop
    iwt r15,#.lBF3D
    nop
.lBD64:
    ibt r7,#$0C
    with r7
    add r1
    to r7
    ldw (r7)
    sm (!fx_collide_model-!gsu_ram),r7
    ibt r2,#$1A
    with r2
    add r7
    to r2
    ldw (r2)
    iwt r0,#$0002
    and r2
    beq .lBD7F
    nop
    iwt r15,#.lBF30
    nop
.lBD7F:
    ibt r2,#$16
    with r2
    add r7
    to r2
    ldw (r2)
    iwt r0,#$0020
    and r2
    beq .lBD90
    nop
    iwt r15,#.lBF30
    nop
.lBD90:
    iwt r0,#$0002
    and r2
    beq .lBDBD
    nop
    iwt r3,#$0014
    lm r0,(!fx_drive_level-!gsu_ram)
    add r0
    with r3
    sub r0
    bpl .lBDA6
    nop
    ibt r3,#$00
.lBDA6:
    ibt r0,#$7C
    add r8
    from r3
    stw (r0)
    ibt r0,#$5E
    add r8
    ldw (r0)
    ibt r3,#$5E
    with r3
    add r7
    to r3
    ldw (r3)
    add r3
    beq .lBDBD
    nop
    iwt r15,#.lBF30
    nop
.lBDBD:
    from r1
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxSumsClear
    nop
    iwt r0,#$0002
    and r2
    beq .lBDF0
    nop
    ibt r0,#$64
    add r8
    ldw (r0)
    iwt r3,#$0010
    and r3
    beq .lBDE7
    nop
    ibt r0,#$64
    add r7
    ldw (r0)
    iwt r3,#$0003
    and r3
    beq .lBDF0
    nop
.lBDE7:
    ibt r0,#$2C
    add r8
    ldw (r0)
    iwt r3,#$0004
    or r3
    sbk
.lBDF0:
    ibt r0,#$16
    add r8
    ldw (r0)
    iwt r2,#$0010
    and r2
    beq .lBDFF
    nop
    iwt r15,#.lBED1
    nop
.lBDFF:
    from r1
    stw (r10)
    inc r10
    inc r10
    from r7
    stw (r10)
    inc r10
    inc r10
    from r9
    stw (r10)
    inc r10
    inc r10
    move r0,r9
    move r9,r7
    move r7,r0
    link #4
    iwt r15,#FxCollideModel
    nop
    dec r10
    dec r10
    to r9
    ldw (r10)
    dec r10
    dec r10
    to r7
    ldw (r10)
    dec r10
    dec r10
    to r1
    ldw (r10)
    moves r0,r0
    bne .lBE2B
    nop
    iwt r15,#.lBF28
    nop
.lBE2B:
    link #4
    iwt r15,#FxContactsAnyTouch
    nop
    moves r0,r0
    bne .lBE39
    nop
    iwt r15,#.lBF28
    nop
.lBE39:
    link #4
    iwt r15,#FxPushesApply
    nop
    lm r0,(!fx_world_sum-!gsu_ram)
    not
    inc r0
    add r0
    sbk
    lm r0,(!fx_world_sum+2-!gsu_ram)
    ibt r0,#$00
    sbk
    lm r0,(!fx_world_sum+4-!gsu_ram)
    not
    inc r0
    add r0
    sbk
    ibt r1,#$00
    ibt r2,#$00
    ibt r3,#$00
    link #4
    iwt r15,#FxHitAddUnit
    nop
    lms r1,(!gsu_in_20-!gsu_ram)
    with r1
    div2
    with r1
    div2
    ibt r2,#$00
    lms r3,(!gsu_in_24-!gsu_ram)
    with r3
    div2
    with r3
    div2
    lms r6,(!gsu_out_e4-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_1-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_object_matrix_2-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    sms (!gsu_in_20-!gsu_ram),r0
    lms r6,(!fx_object_matrix_3-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_4-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_object_matrix_5-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    sms (!gsu_in_22-!gsu_ram),r0
    lms r6,(!fx_object_matrix_6-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_7-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_object_matrix_8-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    sms (!gsu_in_24-!gsu_ram),r0
    lms r1,(!gsu_in_20-!gsu_ram)
    lms r2,(!gsu_in_22-!gsu_ram)
    lms r3,(!gsu_in_24-!gsu_ram)
    link #4
    iwt r15,#FxPushAt4
    nop
    iwt r15,#.lBF28
    nop
.lBED1:
    link #4
    iwt r15,#FxCollidePush
    nop
    sub r0
    sm (!fx_push_turn-!gsu_ram),r0
    iwt r3,#$23A4
    lm r2,(!fx_collide_model-!gsu_ram)
    link #4
    iwt r15,#FxPushApply
    nop
    moves r0,r0
    beq .lBEFC
    nop
    lm r1,(!fx_closing-!gsu_ram)
    moves r1,r1
    bmi .lBEFC
    nop
    iwt r0,#$0094
    add r8
    ldw (r0)
    add r1
    sbk
.lBEFC:
    lm r0,(!fx_world_sum-!gsu_ram)
    add r0
    sbk
    move r1,r0
    lm r0,(!fx_world_sum+4-!gsu_ram)
    add r0
    sbk
    lm r1,(!fx_world_sum-!gsu_ram)
    lm r0,(!fx_hit-!gsu_ram)
    add r1
    sbk
    lm r1,(!fx_world_sum+4-!gsu_ram)
    lm r0,(!fx_hit+4-!gsu_ram)
    add r1
    sbk
    sub r0
    sm (!fx_world_sum+2-!gsu_ram),r0
    sub r0
    sm (!fx_world_sum+2-!gsu_ram),r0
.lBF28:
    dec r10
    dec r10
    to r8
    ldw (r10)
    dec r10
    dec r10
    to r1
    ldw (r10)
.lBF30:
    ibt r0,#$0E
    add r1
    ldw (r0)
    moves r1,r0
    beq .lBF3D
    nop
    iwt r15,#.lBD64
    nop
.lBF3D:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lBF42:
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r1,#$0040
    bic r1
    sbk
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:C3D5 FxCarFollowCourse
; WHAT IT DOES: Keeps car R8 on the course: off the course (+$16 bit
; 10 clear) a car marked fallen (+$1C bit 4) is put back (FxCarPose,
; FxCarAimAlongCourse); then, unless it is driven by the course data
; (+$70 bit 2: FxRecordSegment), its nearest course entry is found
; (FxCourseNearest, FxCourseNextEntry) and it decides its driving
; (FxDriveDecide) and its sides (FxTrafficSides). R0 = 1, or 0 when it
; is caught up (FxCarCatchUp).
; HOW: Pushes R11; returns by pulling R15.
; Entry: R8 = the car; R10 = stack; R11 = return address.
; Exit: R0 = 1 or 0; the rest as the last call leaves them.
%at($04C3D5)
FxCarFollowCourse:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$16
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0400
    and r1
    bne .lC3EA
    nop
    iwt r15,#.lC43D
    nop
.lC3EA:
    nop
    ibt r0,#$18
    add r8
    ldw (r0)
    moves r0,r0
    beq .lC3FD
    nop
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r1,#$0040
    or r1
    sbk
.lC3FD:
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r1,#$0010
    to r2
    and r1
    bne .lC423
    nop
    or r1
    sbk
    ibt r1,#$FF
    ibt r0,#$72
    add r8
    from r1
    stw (r0)
    ibt r1,#$00
    ibt r0,#$2E
    add r8
    from r1
    stw (r0)
    ibt r0,#$30
    add r8
    from r1
    stw (r0)
    ibt r0,#$32
    add r8
    from r1
    stw (r0)
.lC423:
    ibt r2,#$70
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0004
    and r2
    beq .lC434
    nop
    iwt r15,#.lC4BE
    nop
.lC434:
    link #4
    iwt r15,#FxCarCatchUp
    nop
    iwt r15,#.lC4B7
    nop
.lC43D:
    ibt r0,#$1C
    add r8
    ldw (r0)
    iwt r1,#$0010
    to r2
    and r1
    beq .lC456
    nop
    bic r1
    sbk
    link #4
    iwt r15,#FxCarPose
    nop
    link #4
    iwt r15,#FxCarAimAlongCourse
    nop
.lC456:
    ibt r2,#$70
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0004
    and r2
    beq .lC467
    nop
    iwt r15,#.lC4BE
    nop
.lC467:
    ibt r2,#$2C
    with r2
    add r8
    to r2
    ldw (r2)
    iwt r0,#$0040
    and r2
    beq .lC478
    nop
    iwt r15,#.lC4AB
    nop
.lC478:
    ibt r1,#$1E
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r2,#$20
    with r2
    add r8
    to r2
    ldw (r2)
    ibt r3,#$22
    with r3
    add r8
    to r3
    ldw (r3)
    link #4
    iwt r15,#FxCourseNearest
    nop
    moves r0,r0
    bne .lC498
    nop
    iwt r15,#.lC4AB
    nop
.lC498:
    move r7,r0
    ibt r0,#$58
    add r8
    from r7
    stw (r0)
    move r0,r7
    link #4
    iwt r15,#FxCourseNextEntry
    nop
    link #4
    iwt r15,#FxDriveDecide
    nop
.lC4AB:
    link #4
    iwt r15,#FxTrafficSides
    nop
    ibt r0,#$01
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC4B7:
    ibt r0,#$00
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC4BE:
    link #4
    iwt r15,#FxRecordSegment
    nop
    move r7,r0
    ibt r0,#$58
    add r8
    from r7
    stw (r0)
    ibt r0,#$01
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 04:C72F FxCarAimAlongCourse
; WHAT IT DOES: Aims a fallen car along the course: when it has a
; course entry (+$58) its direction there (FxTableEntryWords,
; FxCourseEntryRead, FxAngleVector) times the speed of the drive level
; ($26F4, table at $04:C781) becomes its speed (+$2E, +$30, +$32). It
; returns past FxCarFollowCourse (its R11 pulled from the stack), to
; that one's caller.
; HOW: DEC R10 twice and R11 pulled; ROMB 4 for the table; returns by
; pulling R15 a level up.
; Entry: called by FxCarFollowCourse with its R11 on the R10 stack; R8
; = the car.
; Exit: R1-R3 the speed; R11 = FxCarFollowCourse's return address.
%at($04C72F)
FxCarAimAlongCourse:
    dec r10
    dec r10
    to r11
    ldw (r10)
    ibt r0,#$58
    add r8
    ldw (r0)
    moves r0,r0
    bne .lC740
    nop
    iwt r15,#.lC77C
    nop
.lC740:
    link #4
    iwt r15,#FxTableEntryWords
    nop
    link #4
    iwt r15,#FxCourseEntryRead
    nop
    ibt r0,#$04
    romb
    lm r0,(!fx_drive_level-!gsu_ram)
    lob
    add r0
    iwt r14,#$C781
    with r14
    add r0
    to r6
    getb
    inc r14
    with r6
    getbh
    from r4
    add r4
    fmult
    move r4,r0
    ibt r0,#$24
    add r8
    ldw (r0)
    link #4
    iwt r15,#FxAngleVector
    nop
    ibt r0,#$2E
    add r8
    from r1
    stw (r0)
    ibt r0,#$30
    add r8
    from r2
    stw (r0)
    ibt r0,#$32
    add r8
    from r3
    stw (r0)
.lC77C:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
