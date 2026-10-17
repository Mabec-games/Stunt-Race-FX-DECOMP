; polygon.asm: the Super FX taking a polygon from its points to the screen:
; gathered into vertex lists at $0688 and $082A, clipped against the screen
; edges and the near plane, projected and written out at the word at $005A.
; C: src/stuntracefx/render/polygon.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:C21E FxPolygonNearClip
; WHAT IT DOES: Splits a polygon that spans too much depth: when three
; times its nearest z is less than its farthest, it is clipped against the
; near plane at (farthest + 3 * nearest) / 4 (FxPolygonClipNear), the
; first byte of the 8-byte entry below the output ($005A - 8, the
; polygon's header) set to the clipped count's complement, the near part
; projected (FxPolygonProject) and the header copied again after it; the
; polygon is then clipped against the same plane facing the other way
; for the part beyond. R0 = the count left ($0040), or 0 when nothing is.
; HOW: Pushes R11 on the R10 stack. R4 = the largest z (from $C180), R5
; the smallest (from $3E80), over the $0040 vertices (LOOP, R13 = its
; start); a polygon still in list $082A is first copied to the list at
; $0062 as it is measured and the lists swapped. The count saved at
; $0098. The plane, the clipped polygon's output start ($005A - 8) and
; the plane again go on the R10 stack around the calls; the four words
; at that start are copied to the end of the output, $005A moved past
; them, the lists reset to $0688 in, $082A out, the count back from
; $0098, and the plane negated for the second clip. Returns by pulling
; R15.
; Entry: R10 = stack; R11 = return address. Exit: R0 as above; R4, R5 =
; the largest and smallest z; R13 the LOOP start; R12 = 0; R1, R2 = the
; lists as swapped last; R8 past the output; S and Z from R10; the last
; RAM address R10.
%at($01C21E)
FxPolygonNearClip:
    from r11
    stw (r10)
    inc r10
    inc r10
    iwt r5,#$3E80
    iwt r4,#$C180
    lms r12,(!fx_vertex_count-!gsu_ram)
    iwt r0,#$082A
    lms r7,(!fx_list_in-!gsu_ram)
    sub r7
    beq .lC255
    nop
    with r7
    add #4
    move r13,r15
    to r6
    ldw (r7)
    from r4
    sub r6
    bpl .lC246
    nop
    move r4,r6
    bra .lC24D
    nop
.lC246:
    from r5
    sub r6
    bmi .lC24D
    nop
    move r5,r6
.lC24D:
    with r7
    add #10
    loop
    nop
    bra .lC299
    nop
.lC255:
    lms r2,(!fx_list_out-!gsu_ram)
    move r13,r15
    ldw (r7)
    inc r7
    inc r7
    stw (r2)
    inc r2
    inc r2
    ldw (r7)
    inc r7
    inc r7
    stw (r2)
    inc r2
    inc r2
    to r6
    ldw (r7)
    inc r7
    inc r7
    from r6
    stw (r2)
    inc r2
    inc r2
    from r4
    sub r6
    bpl .lC278
    nop
    move r4,r6
    bra .lC27F
    nop
.lC278:
    from r5
    sub r6
    bmi .lC27F
    nop
    move r5,r6
.lC27F:
    ldw (r7)
    inc r7
    inc r7
    stw (r2)
    inc r2
    inc r2
    ldw (r7)
    inc r7
    inc r7
    stw (r2)
    inc r2
    inc r2
    loop
    nop
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
.lC299:
    lms r0,(!fx_vertex_count-!gsu_ram)
    sms (!fx_vertex_count_save-!gsu_ram),r0
    from r5
    add r5
    add r5
    to r6
    sub r4
    bmi .lC2AB
    nop
    iwt r15,#.lC331
    nop
.lC2AB:
    nop
    from r4
    add r0
    lsr
    lsr
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxPolygonClipNear
    nop
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    dec r10
    dec r10
    to r1
    ldw (r10)
    moves r0,r0
    beq .lC334
    nop
    from r1
    stw (r10)
    inc r10
    inc r10
    lms r2,(!fx_poly_out-!gsu_ram)
    with r2
    sub #8
    not
    stb (r2)
    not
    from r2
    stw (r10)
    inc r10
    inc r10
    link #4
    iwt r15,#FxPolygonProject
    nop
    dec r10
    dec r10
    to r1
    ldw (r10)
    nop
    ldw (r1)
    inc r1
    inc r1
    stw (r8)
    inc r8
    inc r8
    ldw (r1)
    inc r1
    inc r1
    stw (r8)
    inc r8
    inc r8
    ldw (r1)
    inc r1
    inc r1
    stw (r8)
    inc r8
    inc r8
    ldw (r1)
    inc r1
    inc r1
    stw (r8)
    inc r8
    inc r8
    sms (!fx_poly_out-!gsu_ram),r8
    iwt r0,#$082A
    sms (!fx_list_out-!gsu_ram),r0
    iwt r0,#$0688
    sms (!fx_list_in-!gsu_ram),r0
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sms (!fx_vertex_count-!gsu_ram),r0
    dec r10
    dec r10
    ldw (r10)
    not
    inc r0
    link #4
    iwt r15,#FxPolygonClipNear
    nop
    moves r0,r0
    beq .lC334
    nop
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
.lC331:
    lms r0,(!fx_vertex_count-!gsu_ram)
.lC334:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:C339 FxPolygonDraw
; WHAT IT DOES: Takes a polygon of R9 points (index bytes at R14 - R9)
; through to the screen: FxPointsGather fills its vertex list at $082A,
; then it is clipped against each screen edge its outcodes in R5 cross
; (bit 1 top, bit 0 bottom, bit 3 right, bit 2 left; bit 4, behind the
; view, means all four), split in depth and projected
; (FxPolygonNearClip, FxPolygonProject). When a clip leaves nothing the
; count $0040 is cleared.
; HOW: Pushes R11 on the R10 stack. R5 saved at $014A, R9 from $0146
; into $0040, R14 moved back by R9. The lists set to $082A in, $0688
; out; each clip leaves its result in the other and the two are
; swapped. Returns by pulling R15.
; Entry: R5 = the outcodes; R14 past the index bytes; R10 = stack; R11 =
; return address. Exit: R0 = the count, or 0; $0040 = 0 when nothing is
; left; S and Z from R10; the last RAM address R10; the rest as the last
; routine called leaves them. R14 and the ROM buffer move as on the Super
; FX.
%at($01C339)
FxPolygonDraw:
    from r11
    stw (r10)
    inc r10
    inc r10
    sms (!fx_outcodes-!gsu_ram),r5
    lms r9,(!fx_point_count-!gsu_ram)
    sms (!fx_vertex_count-!gsu_ram),r9
    with r14
    sub r9
    link #4
    iwt r15,#FxPointsGather
    nop
    iwt r0,#$082A
    sms (!fx_list_in-!gsu_ram),r0
    iwt r0,#$0688
    sms (!fx_list_out-!gsu_ram),r0
    lms r5,(!fx_outcodes-!gsu_ram)
    ibt r1,#$10
    with r1
    and r5
    beq .lC368
    nop
    ibt r5,#$0F
    sms (!fx_outcodes-!gsu_ram),r5
.lC368:
    from r5
    to r1
    and #2
    beq .lC38C
    nop
    link #4
    iwt r15,#FxPolygonClipTop
    nop
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    moves r0,r0
    bne .lC389
    nop
    iwt r15,#.lC409
    nop
.lC389:
    lms r5,(!fx_outcodes-!gsu_ram)
.lC38C:
    from r5
    to r1
    and #1
    beq .lC3B0
    nop
    link #4
    iwt r15,#FxPolygonClipBottom
    nop
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    moves r0,r0
    bne .lC3AD
    nop
    iwt r15,#.lC409
    nop
.lC3AD:
    lms r5,(!fx_outcodes-!gsu_ram)
.lC3B0:
    from r5
    to r1
    and #8
    beq .lC3D4
    nop
    link #4
    iwt r15,#FxPolygonClipRight
    nop
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    moves r0,r0
    bne .lC3D1
    nop
    iwt r15,#.lC409
    nop
.lC3D1:
    lms r5,(!fx_outcodes-!gsu_ram)
.lC3D4:
    from r5
    to r1
    and #4
    beq .lC3F5
    nop
    link #4
    iwt r15,#FxPolygonClipLeft
    nop
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    moves r0,r0
    bne .lC3F5
    nop
    iwt r15,#.lC409
    nop
.lC3F5:
    link #4
    iwt r15,#FxPolygonNearClip
    nop
    moves r0,r0
    beq .lC409
    nop
    link #4
    iwt r15,#FxPolygonProject
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lC409:
    sub r0
    sms (!fx_vertex_count-!gsu_ram),r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:C412 FxPolygonProject
; WHAT IT DOES: Projects the $0040 vertices of the list at $0060 to the
; screen (FxProjectVertex), each one's screen position pulled in to the
; edge it is past, and writes them out at the word at $005A: screen x,
; screen y, then the vertex's two attributes doubled. The outcode words
; are ORed into $014A, which is cleared again at the end; $0146 = the
; count.
; HOW: Pushes R11 on the R10 stack. CACHE. R3, R13, R9 from $0026-$002A
; (as FxProjectVertex's near points leave them); ROM bank $14, R14 saved
; at $001A. For each vertex: X, Y, Z to R1, R2, R6 and FxProjectVertex;
; its R0 pushed, then AND #15 and four LSRs in the branches' delay slots
; test bits 0-3: Y to $003C or $003E, X to 0 (after a load of $0038 that
; is overwritten) or $003A. The outcode back from the stack, ORed with
; $014A and stored there by SBK in the loop's delay slot. Then the ROM
; bank from $001C and R14 from $001A; returns by pulling R15.
; Entry: R10 = stack; R11 = return address. Exit: R0 = the count; R7
; past the last vertex read; R8 past the last written; R12 = 0; R6 =
; $014A as read last; R14 as saved; S and Z from R10; the last RAM
; address R10. R14 and the ROM buffer move as on the Super FX.
%at($01C412)
FxPolygonProject:
    from r11
    stw (r10)
    inc r10
    inc r10
    cache
    lms r3,(!gsu_out_26-!gsu_ram)
    lms r13,(!gsu_out_28-!gsu_ram)
    lms r9,(!gsu_out_2a-!gsu_ram)
    ibt r0,#$14
    romb
    sub r0
    sms (!fx_outcodes-!gsu_ram),r0
    sms (!fx_rom_r14-!gsu_ram),r14
    lms r12,(!fx_vertex_count-!gsu_ram)
    lms r7,(!fx_list_in-!gsu_ram)
    lms r8,(!fx_poly_out-!gsu_ram)
.lC434:
    to r1
    ldw (r7)
    inc r7
    inc r7
    to r2
    ldw (r7)
    inc r7
    inc r7
    to r6
    ldw (r7)
    inc r7
    inc r7
    link #4
    iwt r15,#FxProjectVertex
    nop
    stw (r10)
    inc r10
    inc r10
    and #15
    beq .lC467
    lsr
    bcc .lC453
    lsr
    lms r2,(!gsu_in_3c-!gsu_ram)
.lC453:
    bcc .lC459
    lsr
    lms r2,(!gsu_in_3e-!gsu_ram)
.lC459:
    bcc .lC461
    lsr
    lms r1,(!gsu_in_38-!gsu_ram)
    ibt r1,#$00
.lC461:
    bcc .lC467
    nop
    lms r1,(!gsu_in_3a-!gsu_ram)
.lC467:
    dec r10
    dec r10
    ldw (r10)
    from r1
    stw (r8)
    inc r8
    inc r8
    from r2
    stw (r8)
    inc r8
    inc r8
    to r6
    ldw (r7)
    inc r7
    inc r7
    with r6
    add r6
    from r6
    stw (r8)
    inc r8
    inc r8
    to r6
    ldw (r7)
    inc r7
    inc r7
    with r6
    add r6
    from r6
    stw (r8)
    inc r8
    inc r8
    lms r6,(!fx_outcodes-!gsu_ram)
    or r6
    dec r12
    bne .lC434
    sbk
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    lms r14,(!fx_rom_r14-!gsu_ram)
    sub r0
    sms (!fx_outcodes-!gsu_ram),r0
    lms r0,(!fx_vertex_count-!gsu_ram)
    sms (!fx_point_count-!gsu_ram),r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
