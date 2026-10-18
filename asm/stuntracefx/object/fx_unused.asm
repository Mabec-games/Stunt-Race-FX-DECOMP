; fx_unused.asm: Super FX code of bank 04 that nothing in the game calls
; or starts: a checksum program over the object pools' RAM, an object-free
; program, an object-new program, a matrix program and eight small
; routines (a turned push, a clamp, a record's aim, two scalings, a
; record step, a spare exit).
; C: src/stuntracefx/object/fx_unused.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch superfx

; @routine 04:8915 FxPoolChecksumProgram
; WHAT IT DOES: A program: the object pools' checksum (FxPoolChecksum)
; into $2686, then STOP. Nothing in the game starts it.
; HOW: R10 = $0240, LINK to FxPoolChecksum, SM.
; Entry: run as a program. Exit: (at the STOP) as FxPoolChecksum leaves
; the registers, R11 = $891D.
%at($048915)
FxPoolChecksumProgram:
    iwt r10,#$0240
    link #4
    iwt r15,#FxPoolChecksum
    nop
    sm (!fx_init_2686-!gsu_ram),r0
    stop
    nop
; @routine 04:8923 FxPoolChecksum
; WHAT IT DOES: The XOR of the $AF9 words of Super FX RAM from $1090
; (the object pools) into R0. Only FxPoolChecksumProgram calls it.
; HOW: RAM bank 0; a LOOP of LDW, INC R1 twice, XOR.
; Entry: R11 = return address. Exit: R0 = the checksum, S and Z from it;
; R1 = $2682; R2 = the last word; R12 = 0; the cache base at 04:8920.
%at($048923)
FxPoolChecksum:
    sub r0
    ramb
    iwt r1,#$1090
    iwt r12,#$0AF9
    cache
    move r13,r15
    to r2
    ldw (r1)
    inc r1
    inc r1
    xor r2
    loop
    nop
    move r15,r11
    nop


; @routine 04:89FA FxObjectFreeProgram
; WHAT IT DOES: A program: frees the Super FX object at $238E
; (FxObjectFree), then STOP. Nothing in the game starts it.
; HOW: R10 = $0240, R1 = $238E, LINK to FxObjectFree.
; Entry: run as a program; $238E = the object. Exit: (at the STOP) as
; FxObjectFree leaves the registers.
%at($0489FA)
FxObjectFreeProgram:
    iwt r10,#$0240
    lm r1,(!gsu_record_arg-!gsu_ram)
    link #4
    iwt r15,#FxObjectFree
    nop
    stop
    nop


; @routine 04:8AD0 FxObjectNewProgram
; WHAT IT DOES: A program: takes an object of $28 bytes from the pool at
; $1098 (FxPoolTake); none: $238E = 0. Otherwise it is placed at
; ($238E, $2390, $2392) (+$1E/+$20/+$22), its +$16 gets bits $A0,
; +$1A = $FFFF, $238E = it, then its course segment (FxRecordSegment)
; and FxCourseNextEntry, whose R0 goes to $2390; then STOP. Nothing in
; the game starts it.
; HOW: R10 = $0240; the object in R1, then R8 for the calls.
; Entry: run as a program; $238E-$2392 = the place. Exit: (at the STOP)
; as the last call leaves the registers.
%at($048AD0)
FxObjectNewProgram:
    iwt r10,#$0240
    iwt r4,#$0028
    iwt r3,#$1098
    link #4
    iwt r15,#FxPoolTake
    nop
    moves r1,r1
    beq .l8B2C
    nop
    lm r2,(!gsu_record_arg-!gsu_ram)
    ibt r0,#$1E
    add r1
    from r2
    stw (r0)
    lm r2,(!gsu_in_amount-!gsu_ram)
    ibt r0,#$20
    add r1
    from r2
    stw (r0)
    lm r2,(!fx_script_at-!gsu_ram)
    ibt r0,#$22
    add r1
    from r2
    stw (r0)
    ibt r2,#$16
    with r2
    add r1
    to r2
    ldw (r2)
    iwt r0,#$00A0
    to r2
    or r2
    ibt r0,#$16
    add r1
    from r2
    stw (r0)
    iwt r2,#$FFFF
    ibt r0,#$1A
    add r1
    from r2
    stw (r0)
    sm (!gsu_record_arg-!gsu_ram),r1
    move r8,r1
    link #4
    iwt r15,#FxRecordSegment
    nop
    link #4
    iwt r15,#FxCourseNextEntry
    nop
    sm (!gsu_in_amount-!gsu_ram),r0
    stop
    nop
.l8B2C:
    sub r0
    sm (!gsu_record_arg-!gsu_ram),r0
    stop
    nop


; @routine 04:9820 FxHitAddRotated
; WHAT IT DOES: Turns the push (R1, R2, R3) by the current matrix at
; $00E4 (each result the sum of the three products with one column:
; $E4/$EA/$F0, $E6/$EC/$F2, $E8/$EE/$F4) into $26F6-$26FA, then
; adds it as FxHitAddUnit does. Nothing in the game calls it.
; HOW: FMULT, ROL for each product (1.15); the sums stored with SM and
; read back into R1-R3; BRA into FxHitAddUnit.
; Entry: R1-R3 = the push; R10 = stack; R11 = return address. Exit: as
; FxHitAddUnit.
%at($049820)
FxHitAddRotated:
    lms r6,(!gsu_out_e4-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_3-!gsu_ram)
    from r2
    fmult
    rol
    with r5
    add r0
    lms r6,(!fx_object_matrix_6-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    sm (!fx_closing-!gsu_ram),r0
    lms r6,(!fx_object_matrix_1-!gsu_ram)
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
    lms r6,(!fx_object_matrix_7-!gsu_ram)
    from r3
    fmult
    rol
    add r5
    sm (!fx_haze_x8-!gsu_ram),r0
    lms r6,(!fx_object_matrix_2-!gsu_ram)
    from r1
    fmult
    to r5
    rol
    lms r6,(!fx_object_matrix_5-!gsu_ram)
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
    sm (!fx_me_z-!gsu_ram),r0
    lm r1,(!fx_closing-!gsu_ram)
    lm r2,(!fx_haze_x8-!gsu_ram)
    lm r3,(!fx_me_z-!gsu_ram)
    bra FxHitAddUnit
    nop


; @routine 04:CB7E FxObjectSlideClamp
; WHAT IT DOES: For the object at R8 and the one at R5: none (R5 = 0)
; gives R1 = 0; otherwise R5 is replaced by the one at its +$26 when that
; is set, stored at R8's +$58, and R1 = (R8 + $58) - R8's +$24, kept
; within plus or minus R1 as it came in (pushed at the start, pulled into
; R3, its negative in R2). R8's position (+$1E/+$20/+$22) is loaded into
; R1-R3 first. Nothing in the game calls it.
; HOW: R11 and R1 on the R10 stack; signed compares (CMP, BMI); returns by
; pulling R15.
; Entry: R1 = the limit; R5, R8 = the objects; R10 = stack; R11 = return
; address. Exit: R1 as above; R0 = R8 + $58, or the limit with no R5.
%at($04CB7E)
FxObjectSlideClamp:
    from r11
    stw (r10)
    inc r10
    inc r10
    from r1
    stw (r10)
    inc r10
    inc r10
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
    moves r5,r5
    beq .lCBDD
    nop
    ibt r1,#$26
    with r1
    add r5
    to r1
    ldw (r1)
    moves r1,r1
    beq .lCBAA
    nop
    move r5,r1
.lCBAA:
    ibt r0,#$58
    add r8
    from r5
    stw (r0)
    move r1,r0
    ibt r0,#$24
    add r8
    ldw (r0)
    with r1
    sub r0
    dec r10
    dec r10
    to r3
    ldw (r10)
    move r2,r3
    with r2
    not
    inc r2
    moves r1,r1
    bmi .lCBD0
    nop
    with r1
    cmp r3
    bmi .lCBD8
    nop
    move r1,r3
    bra .lCBD8
    nop
.lCBD0:
    with r1
    cmp r2
    bpl .lCBD8
    nop
    move r1,r2
.lCBD8:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lCBDD:
    dec r10
    dec r10
    ldw (r10)
    iwt r1,#$0000
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop


; @routine 04:A301 FxRecordAimFromLink
; WHAT IT DOES: Sets the aim words $0068, $002C, $002E for the record at
; R7: from the record linked at its +$18 ($68 = its +$04, $2E = its +$06,
; $2C = 0) when there is one, else from the record's own +$2E, +$30 and
; +$32. Nothing in the game calls it.
; HOW: LDW from R7 + each field; R11 on the R10 stack; returns by pulling
; R15.
; Entry: R7 = the record; R10 = stack; R11 = return address. Exit: R0 the
; last word stored; R1 = the link.
%at($04A301)
FxRecordAimFromLink:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$18
    with r1
    add r7
    to r1
    ldw (r1)
    moves r1,r1
    beq .lA328
    nop
    ibt r0,#$04
    add r1
    ldw (r0)
    sm (!gsu_in_68-!gsu_ram),r0
    ibt r0,#$06
    add r1
    ldw (r0)
    sm (!gsu_in_2e-!gsu_ram),r0
    sub r0
    sm (!gsu_dest-!gsu_ram),r0
    bra .lA340
    nop
.lA328:
    ibt r0,#$2E
    add r7
    ldw (r0)
    sm (!gsu_in_68-!gsu_ram),r0
    ibt r0,#$30
    add r7
    ldw (r0)
    sm (!gsu_dest-!gsu_ram),r0
    ibt r0,#$32
    add r7
    ldw (r0)
    sm (!gsu_in_2e-!gsu_ram),r0
.lA340:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop


; @routine 04:93FA FxScaleFromTable14
; WHAT IT DOES: R0 = 2 * FMULT($0100, the word at $14:896C + $0200):
; an entry of the bank $14 table at $896C halved, ROM bank $14 left set.
; Its last five bytes, a pull return after the JMP R11's delay slot that
; nothing reaches, are filed on their own (FxScaleFromTable14Pull,
; 04:9417). Nothing in the game calls it.
; HOW: ROMB $14, R14 = the table + ($0200 BIC 1), GETB, GETBH, FMULT, ADD
; R0.
; Entry: R11 = return address. Exit: R0 as above; R1 = $0100; R6 = the
; table word; ROM bank $14; R14 and the ROM buffer move as on the Super
; FX.
%at($0493FA)
FxScaleFromTable14:
    iwt r1,#$0100
    ibt r0,#$14
    romb
    iwt r14,#$896C
    iwt r0,#$0200
    bic #1
    with r14
    add r0
    to r6
    getb
    inc r14
    with r6
    getbh
    from r1
    fmult
    add r0
    move r15,r11
    nop
;
; @routine 04:9417 FxScaleFromTable14Pull
; WHAT IT DOES: The pull return after FxScaleFromTable14's JMP R11 and
; its delay slot, which no path reaches. A unit start runs it so the C
; is judged against the original.
; HOW: DEC R10 twice, then R15 = the word there (NOP in the delay slot).
; Entry: R10 = stack, the return address the word below it. Exit: R10
; two lower; S, Z from R10.
%at($049417)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop


; @routine 04:BCA5 FxAbove100Scaled
; WHAT IT DOES: When R0 is $0100 or more, R0 = the low byte of the middle
; bytes of (R0 - $0100) * $80 (that is ((R0 - $100) >> 1) & $FF); below
; $0100 R0 is left R0 - $0100. Nothing in the game calls it.
; HOW: SUB, BMI; LMULT by $80, SWAP of the low word, LOB.
; Entry: R0 = the value; R11 = return address. Exit: R0 as above; R1 =
; $0100; R4, R6 as the LMULT leaves them.
%at($04BCA5)
FxAbove100Scaled:
    iwt r1,#$0100
    sub r1
    bmi .lBCB7
    nop
    iwt r6,#$0080
    lmult
    from r4
    swap
    lob
    move r15,r11
    nop
.lBCB7:
    move r15,r11
    nop


; @routine 04:C3A2 FxRecordSegmentNext
; WHAT IT DOES: FxRecordSegment, then FxCourseNextEntry. Nothing in the
; game calls it.
; HOW: R11 on the R10 stack around the two LINK calls; returns by pulling
; R15.
; Entry: as FxRecordSegment; R10 = stack; R11 = return address. Exit: as
; FxCourseNextEntry.
%at($04C3A2)
FxRecordSegmentNext:
    from r11
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxRecordSegment
    nop
    link #4
    iwt r15,#FxCourseNextEntry
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop


; @routine 04:9C2C FxReturnOneRestore
; WHAT IT DOES: Returns R0 = 1 to the caller whose R11, R8 and R9 are
; pushed on the R10 stack: pulls R9, R8 and R11, then jumps to R11. A spare
; exit nothing reaches.
; HOW: IBT R0, three pulls, MOVE R15, R11.
; Entry: R10 = stack (R11, R8, R9 pushed in that order). Exit: R0 = 1;
; R8, R9, R11 as pulled; R10 six lower.
%at($049C2C)
FxReturnOneRestore:
    ibt r0,#$01
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
    to r11
    ldw (r10)
    move r15,r11
    nop


; @routine 04:C19D FxMatrixFromAnglesProgram04
; WHAT IT DOES: A program: the rotation matrix for the angles into $00E4
; (FxMatrixFromAngles, R9 = $00E4), then STOP. Nothing in the game starts
; it.
; HOW: IWT R9, LINK, STOP.
; Entry: run as a program. Exit: (at the STOP) as FxMatrixFromAngles
; leaves the registers, R11 = $C1A5.
%at($04C19D)
FxMatrixFromAnglesProgram04:
    iwt r9,#$00E4
    link #4
    iwt r15,#FxMatrixFromAngles
    nop
    stop
    nop

