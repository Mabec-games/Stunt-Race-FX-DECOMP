; fx_course.asm: the Super FX's reads of the course's segment table and the
; lap count it keeps from them.
;
; The table is in ROM: its bank is the byte at $70:1094 and its address
; the word at $70:1092. Its first word is the number of segments; 8-byte
; entries follow (see render/table.asm). A car's Super FX record (R8) holds
; its current segment at +$58, the last segment it was counted through at
; +$86, and its laps at +$7A.
; C: src/stuntracefx/race/fx_course.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; R1-R3 = record R8's position (+$1E, +$20, +$22).
macro fx_position()
    ibt r1,#!fxrec_half_x
    with r1
    add r8
    to r1
    ldw (r1)
    ibt r2,#!fxrec_half_x+2
    with r2
    add r8
    to r2
    ldw (r2)
    ibt r3,#!fxrec_half_x+4
    with r3
    add r8
    to r3
    ldw (r3)
endmacro

; @routine 04:C378 FxRecordHeadingAhead
; WHAT IT DOES: The bearing (FxBearingTo) from record R8 to the segment
; after the one nearest it (FxCourseNearest, FxCourseNextEntry): R0 = the
; negated angle.
; HOW: The position into R1-R3, then the three calls in a row; the return
; address is kept on the R10 stack.
; Entry: R8 = the record; R10 = stack; R11 = return address. Exit: as
; FxBearingTo leaves them (R11 = $C39D when it did not divide); S, Z from
; R10; the last RAM address R10.
%at($04C378)
FxRecordHeadingAhead:
    from r11
    stw (r10)
    inc r10
    inc r10
    %fx_position()
    link #4
    iwt r15,#FxCourseNearest
    nop
    link #4
    iwt r15,#FxCourseNextEntry
    nop
    link #4
    iwt r15,#FxBearingTo
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 04:C3B5 FxRecordSegment
; WHAT IT DOES: FxCourseNearest for record R8's position.
; HOW: The position (+$1E, +$20, +$22) into R1-R3, then the call; the
; return address is kept on the R10 stack.
; Entry: R8 = the record; R10 = stack; R11 = return address. Exit: as
; FxCourseNearest; R11 = $C3D0; S, Z from R10; the last RAM address R10.
%at($04C3B5)
FxRecordSegment:
    from r11
    stw (r10)
    inc r10
    inc r10
    %fx_position()
    link #4
    iwt r15,#FxCourseNearest
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; A word of the table into <reg> through the ROM buffer.
macro fx_table_word(reg)
    to <reg>
    getb
    inc r14
    with <reg>
    getbh
    inc r14
endmacro

; @routine 04:CC06 FxCourseNearest
; WHAT IT DOES: Finds the segment table entry nearest the point (R1, R2,
; R3) by |dx| + 4|dy| + |dz|: R0 = its number (from 1), R1-R3 = its first
; three words, R5 its address. An entry whose sums overflow is passed
; over. With no table (address 0) or no entries, R0 = 0.
; HOW: A LOOP over the count with the ROM buffer (ROMBR = the table bank)
; reading each entry's x, y, z; SUB with BVS for the overflow, NOT and INC
; for the magnitudes; a sum below the best so far (CMP, BCS) takes its
; place, R6 holding the best sum, R7 its number and R5 the address just
; past its z. The routine runs under CACHE (the code cache only; nothing
; the C has to do). The not-reached-yet path for a zero table address is
; decoded from the ROM bytes and not reached in the traces so far.
; Entry: R1, R2, R3 = the point; R11 = return address. Exit: R0 = R7 = the
; number; R1-R3 = the entry's words; R4 = the count; R5 = the entry; R6 =
; its sum ($7FFF when none was nearer); R9 from the last entry; R12 = 0;
; R13 = $CC33; R14 = R5 + 6; ROMBR = the table bank; S, Z from R14, OV, CY
; from R5's SUB; the last RAM address $1092. No entries: R0 = 0; R12 = 0;
; R14 = table + 2; the flags from SUB R0.
%at($04CC06)
FxCourseNearest:
    lm r0,(!gsu_table_bank)
    romb
    lm r0,(!gsu_table_addr)
    moves r14,r0
    bne .table
    nop
    iwt r15,#.none
    nop
.table:
    getb
    inc r14
    getbh
    inc r14
    moves r12,r0
    bne .entries
    nop
    iwt r15,#.none
    nop
.entries:
    ibt r5,#0
    iwt r6,#$7FFF
    ibt r4,#0
    ibt r7,#0
    cache
    move r13,r15
    inc r4
    getb
    inc r14
    getbh
    inc r14
    sub r1
    bvs .skip_yz
    nop
    bpl .x_positive
    nop
    not
    inc r0
.x_positive:
    move r9,r0
    getb
    inc r14
    getbh
    inc r14
    sub r2
    bpl .y_positive
    nop
    not
    inc r0
.y_positive:
    add r0
    add r0
    with r9
    add r0
    bvs .skip_z
    nop
    getb
    inc r14
    getbh
    inc r14
    sub r3
    bvs .next
    nop
    bpl .z_positive
    nop
    not
    inc r0
.z_positive:
    add r9
    bvs .next
    nop
    cmp r6
    bcs .next
    nop
    move r6,r0
    move r5,r14
    move r7,r4
    bra .next
    nop
.skip_z:
    inc r14
    bra .next
    inc r14
.skip_yz:
    with r14
    add #4
.next:
    inc r14
    loop
    inc r14
    move r0,r7
    from r5
    to r5
    sub #6
    move r14,r5
    %fx_table_word(r1)
    %fx_table_word(r2)
    %fx_table_word(r3)
    move r15,r11
    nop
.none:
    sub r0
    move r15,r11
    nop

; The four words of the entry at R14 into R1-R4, R5 = the entry; R0 = R6.
macro fx_entry_words()
    move r5,r14
    %fx_table_word(r1)
    %fx_table_word(r2)
    %fx_table_word(r3)
    %fx_table_word(r4)
    move r0,r6
endmacro

; @routine 04:CCA4 FxCourseNextEntry
; WHAT IT DOES: Steps car R8 on one segment: from number R0 at entry R5 to
; the next (from the last to the first); R0 = R6 = the new number, R5 its
; entry, R1-R4 its words. A car going the wrong way (bit 1 of +$70) steps
; back instead (FxCoursePrevEntry).
; HOW: The count is read through the ROM buffer (ROMBR = the table bank)
; and compared with the number; the first entry is table + 2, otherwise R5
; + 8. The wrong-way jump is decoded from the ROM bytes and not reached in
; the traces so far.
; Entry: R0 = number; R5 = its entry; R8 = the car's record; R11 = return
; address. Exit: R0 = R6 = the number; R1-R4 = the words; R5 = the entry;
; R14 = R5 + 8; ROMBR = the table bank; S, Z from R14; OV, CY from the
; address ADD (or the CMP with the count); the last RAM address $1092.
%at($04CCA4)
FxCourseNextEntry:
    move r6,r0
    lm r0,(!gsu_table_bank)
    romb
    ibt r1,#!fxrec_direction
    with r1
    add r8
    to r1
    ldw (r1)
    iwt r0,#$0002
    and r1
    move r0,r6
    beq .forward
    nop
    iwt r15,#FxCoursePrevEntry
    nop
.forward:
    move r6,r0
    inc r6
    lm r14,(!gsu_table_addr)
    to r1
    getb
    inc r14
    with r1
    getbh
    cmp r1
    bne .on
    nop
    lm r0,(!gsu_table_addr)
    add #2
    move r14,r0
    ibt r6,#1
    bra .read
    nop
.on:
    from r5
    to r14
    add #8
.read:
    %fx_entry_words()
    move r15,r11
    nop

; @routine 04:CD05 FxCoursePrevEntry
; WHAT IT DOES: Steps back one segment: from number R0 at entry R5 to the
; one before (from the first to the last); R0 = R6 = the new number, R5 its
; entry, R1-R4 its words.
; HOW: The last entry is table + 2 + (count - 1) * 8 (LMULT by R6 = 8),
; the count read through the ROM buffer; otherwise R5 - 8. It is reached
; only by FxCourseNextEntry's jump for a car going the wrong way; decoded
; from the ROM bytes and not reached in the traces so far.
; Entry: R0 = number; R5 = its entry; R11 = return address. Exit: R0 = R6 =
; the number; R1-R4 = the words; R5 = the entry; R14 = R5 + 8; ROMBR = the
; table bank; S, Z from R14; OV, CY from the address SUB or ADD; the last
; RAM address $1094 or $1092.
%at($04CD05)
FxCoursePrevEntry:
    move r6,r0
    lm r0,(!gsu_table_bank)
    romb
    dec r6
    bne .back
    nop
    lm r14,(!gsu_table_addr)
    getb
    inc r14
    getbh
    move r1,r0
    dec r0
    iwt r6,#$0008
    lmult
    lm r0,(!gsu_table_addr)
    add #2
    from r4
    to r14
    add r0
    move r6,r1
    bra .read
    nop
.back:
    from r5
    to r14
    sub #8
.read:
    %fx_entry_words()
    move r15,r11
    nop

; @routine 04:CD56 FxCourseEntryRead
; WHAT IT DOES: Reads the four words of the segment table entry at R5 into
; R1-R4, keeping R0.
; HOW: R0 is pushed on the R10 stack, ROMB from the table bank byte, then
; GETB and GETBH through R14 = R5 and pulled back.
; Entry: R0 = any; R5 = entry address; R10 = stack; R11 = return address.
; Exit: R1-R4 = the words; R14 = R5 + 8; ROMBR = the table bank; R0 as on
; entry; the last RAM address R10; S, Z from R10.
%at($04CD56)
FxCourseEntryRead:
    stw (r10)
    inc r10
    inc r10
    lm r0,(!gsu_table_bank)
    romb
    move r14,r5
    to r1
    getb
    inc r14
    with r1
    getbh
    inc r14
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
    to r4
    getb
    inc r14
    with r4
    getbh
    inc r14
    dec r10
    dec r10
    ldw (r10)
    move r15,r11
    nop

; @routine 04:CDCA FxLapSegment
; WHAT IT DOES: Counts car R8 through its current segment: when +$58 has
; moved on from +$86 by 1 to 4 segments, or has wrapped from the last
; segment (+$86 = the table's count) to a lower one, +$86 takes it; when
; the segment counted is 1, the lap count +$7A goes up by one.
; HOW: Returns early when the two are equal or the table address is 0. The
; count is the table's first word (GETB, GETBH with ROMBR = the table
; bank); the step forward is +$58 - +$86 compared with 5 by sign.
; Entry: R8 = the car's record; R11 = return address. Exit: R3 = +$58; R1
; = the old +$86; when they differed and the table is there: R14 = table +
; 1, R4 = the count, R2 = 5 or 1; R0 and the flags from the last compare,
; or from the INC of the laps. The last RAM address is the last field or
; table word read or written.
%at($04CDCA)
FxLapSegment:
    ibt r0,#!fxrec_segment
    add r8
    ldw (r0)
    iwt r1,#!fxrec_counted
    with r1
    add r8
    to r1
    ldw (r1)
    move r3,r0
    cmp r1
    beq .done
    nop
    lm r0,(!gsu_table_bank)
    romb
    lm r0,(!gsu_table_addr)
    moves r14,r0
    beq .done
    nop
    to r4
    getb
    inc r14
    with r4
    getbh
    from r3
    sub r1
    bpl .forward
    nop
    with r1
    cmp r4
    beq .count
    nop
    bra .done
    nop
.forward:
    ibt r2,#5
    cmp r2
    bpl .done
    nop
.count:
    iwt r0,#!fxrec_counted
    add r8
    from r3
    stw (r0)
    ibt r2,#1
    with r3
    cmp r2
    bne .done
    nop
    ibt r0,#!fxrec_laps
    add r8
    ldw (r0)
    inc r0
    sbk
.done:
    move r15,r11
    nop

; @routine 04:CED6 FxCourseNextDistance
; WHAT IT DOES: Finds the segment after entry R0 for car R8 (FxTableEntryWords,
; FxCourseNextEntry) and returns how far the car is from it: R0 = |car x
; (+$1E) - R1| / 4 + |car z (+$22) - R3| / 4.
; HOW: The return address is kept on the R10 stack; each difference is made
; positive with NOT and INC and halved twice with DIV2.
; Entry: R0 = entry; R8 = the car's record; R10 = stack; R11 = return
; address. Exit: R0 = the distance; R4 = the x part; R1-R3, R5, R6 as
; FxCourseNextEntry leaves them; R11 = $CEE4; OV, CY from the final add; S,
; Z from R10; the last RAM address R10.
%at($04CED6)
FxCourseNextDistance:
    from r11
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxTableEntryWords
    nop
    link #4
    iwt r15,#FxCourseNextEntry
    nop
    ibt r0,#$1E
    add r8
    ldw (r0)
    sub r1
    bpl .x
    nop
    not
    inc r0
.x:
    div2
    div2
    move r4,r0
    ibt r0,#$22
    add r8
    ldw (r0)
    sub r3
    bpl .z
    nop
    not
    inc r0
.z:
    div2
    div2
    add r4
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816

