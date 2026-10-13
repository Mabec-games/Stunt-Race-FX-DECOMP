; fx_record.asm: small Super FX routines on object records in Super FX RAM.
;
; R8 or R9 is the record; records in a list are linked through +$0E (0 ends
; the list). The push at $70:23A4 is five words: on (1) or off (0), an
; amount, and a vector.
; C: src/stuntracefx/object/fx_record.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 04:9683 FxPushSet
; WHAT IT DOES: Sets the push to amount R2, on when R2 is not negative, with
; the vector (0, $8100, 0).
; HOW: SUB R0 and MOVES R2 for the sign, then five SMs in field order with
; R2 reused for the zeros and $8100.
; Entry: R2 = amount; R11 = return address. Exit: R0 = 1 or 0; R2 = $8100;
; OV, S, Z from R2's sign test, CY from the SUB (set); the last RAM
; address $23AA.
%at($049683)
FxPushSet:
    sub r0
    moves r2,r2
    bmi .off
    nop
    ibt r0,#1
.off:
    sm (!fx_push),r0
    sm (!fx_push+2),r2
    ibt r2,#0
    sm (!fx_push+4),r2
    sm (!fx_push+8),r2
    iwt r2,#$8100
    sm (!fx_push+6),r2
    move r15,r11
    nop

; @routine 04:A441 FxListClearBit0
; WHAT IT DOES: Clears bit 0 of +$10 in every record of the list whose head
; is at +$18 of record R6.
; HOW: LDW, BIC R12 (= 1) and SBK per record, then the link at +$0E with
; MOVES until it is 0.
; Entry: R6 = the record holding the list; R11 = return address. Exit: R8 =
; 0; with records: R12 = 1, R0 = 0; OV, S clear, Z set; the last RAM
; address the last record's +$0E (else R6 + $18).
%at($04A441)
FxListClearBit0:
    ibt r8,#!fxrec_list
    with r8
    add r6
    to r8
    ldw (r8)
    moves r8,r8
    beq .done
    nop
    iwt r12,#$0001
.next:
    ibt r0,#!fxrec_flags10
    add r8
    ldw (r0)
    bic r12
    sbk
    ibt r0,#!fxrec_next
    add r8
    ldw (r0)
    moves r8,r0
    bne .next
    nop
.done:
    move r15,r11
    nop

; @routine 04:BC7B FxRecordHoldY
; WHAT IT DOES: Keeps record R8's +$60 as its height +$20 while bit 8 of
; +$40 is clear; while it is set, +$60 holds, and becomes 0 once it is
; below +$20.
; HOW: AND #$0100 of +$40; the copy is an LDW and STW; "below" is the sign
; of +$60 - +$20 (CMP); SUB R0 and SBK clear it.
; Entry: R8 = the record; R11 = return address. Exit: R1 = +$20; R0 = +$60
; (0 when cleared); OV, S, CY, Z from the CMP or the SUB; the last RAM
; address R8 + $60.
%at($04BC7B)
FxRecordHoldY:
    ibt r0,#!fxrec_flags40
    add r8
    ldw (r0)
    iwt r1,#$0100
    and r1
    bne .compare
    nop
    ibt r1,#!fxrec_y
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r0,#!fxrec_y_held
    add r8
    from r1
    stw (r0)
.compare:
    ibt r1,#!fxrec_y
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r0,#!fxrec_y_held
    add r8
    ldw (r0)
    cmp r1
    bpl .done
    nop
    sub r0
    sbk
.done:
    move r15,r11
    nop

; @routine 04:CBE8 FxBearingTo
; WHAT IT DOES: The angle from record R8's position (+$1E, +$22) to the
; point (R1, R3), negated: FxAngleOf of the difference, then NOT and INC.
; HOW: The differences replace R1 and R3 (SUB under WITH), R2 = the z
; difference for FxAngleOf; the return address is kept on the R10 stack.
; Entry: R1 = x, R3 = z; R8 = the record; R10 = stack; R11 = return
; address. Exit: R0 = the angle; R1 = the x difference; R2 = R3 = the z
; difference; R4-R6, R11-R14 as FxAngleOf leaves them (R11 = $CBFF when it
; did not divide); S, Z from R10; the last RAM address R10.
%at($04CBE8)
FxBearingTo:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r0,#!fxrec_half_x
    add r8
    ldw (r0)
    with r1
    sub r0
    ibt r0,#!fxrec_half_x+4
    add r8
    ldw (r0)
    with r3
    sub r0
    move r2,r3
    link #4
    iwt r15,#FxAngleOf
    nop
    not
    inc r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:BCBA FxRecordAlignment
; WHAT IT DOES: Sets record R8's +$6C to how closely its velocity points
; along the third axis of the matrix at $00E4 (the column at $00F0): the
; dot product of the unit velocity with it, $7FFF for straight along; or
; $7FFF when its speed +$4A is below 10 either way.
; HOW: FxNormalize of the velocity (+$2E, +$30, +$32), three LMULTs summed
; as 32 bits and the result doubled (ADD, ROL) to its high word; R8 and the
; return address are kept on the R10 stack.
; Entry: R8 = the record; R10 = stack; R11 = return address. Exit: R1 =
; the value stored; R0 = R8 + $6C; R2 = 10; R3, R9, R12, R13 as
; FxNormalize leaves them; R4, R5 = the last product's low and high
; words; R6 = the word at $00F4; R7 = the low word of the sum, doubled; R11
; = $C0E3 ($BCD9 for a zero velocity); S, Z from R10, OV, CY from the add
; for +$6C; the last RAM address R10.
%at($04BCBA)
FxRecordAlignment:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r8
    stw (r10)
    inc r10
    inc r10
    ibt r1,#!fxrec_velocity
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r2,#!fxrec_velocity+2
    with r2
    add r8
    to r2
    ldw (r2)
    ibt r3,#!fxrec_velocity+4
    with r3
    add r8
    to r3
    ldw (r3)
    link #4
    iwt r15,#FxNormalize
    nop
    lms r6,(!fx_matrix_w+12)
    from r1
    lmult
    move r7,r4
    lms r6,(!fx_matrix_w+14)
    from r2
    to r5
    lmult
    with r7
    add r4
    adc r5
    lms r6,(!fx_matrix_w+16)
    from r3
    to r5
    lmult
    with r7
    add r4
    adc r5
    with r7
    add r7
    rol
    move r1,r0
    dec r10
    dec r10
    to r8
    ldw (r10)
    ibt r0,#!fxrec_speed
    add r8
    ldw (r0)
    moves r0,r0
    bpl .positive
    nop
    not
    inc r0
.positive:
    iwt r2,#$000A
    cmp r2
    bpl .store
    nop
    iwt r1,#$7FFF
.store:
    ibt r0,#!fxrec_align
    add r8
    from r1
    stw (r0)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:BF51 FxRecordRaise6A
; WHAT IT DOES: A program: raises +$6A of the record whose address the
; S-CPU left at $238E by the amount at $2390 times $300 / 65536, to at most
; $0180.
; HOW: ROMB, RAMB 0; FMULT of $0300 (R2 doubled) by the amount, ADD, and
; the cap by CMP and the sign; SBK stores to the field's address, the last
; loaded. The S-CPU side is GsuRecordRaise6A (object/gsu_record.asm).
; Entry: the S-CPU starts it. Exit (at its STOP): R0 = the new +$6A; R1 =
; the old; R2 = $0180; R6 = the amount; R8 = the record; ROMBR = RAMBR = 0;
; OV, S, CY, Z from the CMP; the last RAM address R8 + $6A.
%at($04BF51)
FxRecordRaise6A:
    sub r0
    romb
    ramb
    lm r8,(!gsu_record_arg)
    lm r6,(!gsu_in_amount)
    ibt r1,#!fxrec_6a
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r2,#$0180
    from r2
    add r2
    fmult
    add r1
    cmp r2
    bmi .store
    nop
    move r0,r2
.store:
    sbk
    stop
    nop

; @routine 04:BF75 FxRecordLower82
; WHAT IT DOES: A program: lowers +$82 of the record whose address the
; S-CPU left at $238E by the amount at $2390 times $400 / 65536, to no less
; than 0.
; HOW: ROMB, RAMB 0; FMULT of $0400 (R2 doubled) by the amount, SUB from
; the field, and SUB R0 for 0 when negative; SBK stores to the field's
; address, the last loaded. The S-CPU side is GsuRecordLower82
; (object/gsu_record.asm).
; Entry: the S-CPU starts it. Exit (at its STOP): R0 = the new +$82; R1 =
; the old; R2 = $0200; R6 = the amount; R8 = the record; ROMBR = RAMBR = 0;
; OV, S, CY, Z from the last SUB; the last RAM address R8 + $82.
%at($04BF75)
FxRecordLower82:
    sub r0
    romb
    ramb
    lm r8,(!gsu_record_arg)
    lm r6,(!gsu_in_amount)
    iwt r1,#!fxrec_82
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r2,#$0200
    from r2
    add r2
    fmult
    from r1
    sub r0
    bpl .store
    nop
    sub r0
.store:
    sbk
    stop
    nop

; @routine 01:BBC7 FxTimersProgram
; WHAT IT DOES: The Super FX program that steps the 20 timed records at
; $273E (14 bytes each): a record whose timer byte (+0) is set counts it
; down; while its flags (+1) have bit 4 and the timer is below $20, +2
; takes the byte of the table at $14:D0CC (ROM); then a record with bit 0
; of +1 has it cleared, any other ends: its timer is cleared and its word
; +$0C set to 0 (also for a record whose timer was already 0).
; HOW: ROM bank $14, RAM bank 0; CACHE and LOOP over R12 = 20 records, R9
; stepping by R8 = 14; the BNE's delay slot clears R0 either way.
; Entry: started by the S-CPU. Exit (at the STOP): R9 past the records;
; R12 = 0; R10 = $0240; the other registers as the last record left them.
%at($01BBC7)
FxTimersProgram:
    iwt r10,#$0240
    ibt r0,#$14
    romb
    sub r0
    ramb
    iwt r8,#$000E
    iwt r9,#$273E
    iwt r12,#$0014
    ibt r5,#$20
    cache
    with r15
    to r13
.record:
    ldb (r9)
    lob
    beq .end
    nop
    dec r0
    stb (r9)
    with r0
    to r7
    ibt r0,#$01
    add r9
    ldb (r0)
    ibt r6,#$10
    and r6
    beq .flags
    nop
    with r7
    to r0
    cmp r5
    bpl .flags
    nop
    iwt r14,#$D0BC
    with r14
    add r0
    getb
    ibt r7,#$02
    with r7
    add r9
    from r0
    stb (r7)
.flags:
    ibt r0,#$01
    add r9
    ldb (r0)
    with r0
    to r6
    and #1
    bne .bit0
    sub r0
    stb (r9)
.end:
    ibt r7,#$0C
    with r7
    add r9
    from r0
    stw (r7)
    with r9
    add r8
    loop
    nop
    bra .done
    nop
.bit0:
    with r6
    to r0
    bic #1
    ibt r7,#$01
    with r7
    add r9
    from r0
    stb (r7)
    with r9
    add r8
    loop
    nop
.done:
    stop
    nop

arch 65816
