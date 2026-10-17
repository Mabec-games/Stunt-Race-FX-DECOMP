; raster.asm: the Super FX drawing the frame's queued shapes (flat
; polygons, lines, textured polygons) from the draw list at $70:08AA into
; its screen buffer, and the steps that feed it: a shape's points gathered
; and projected (01:98D7) and clipped to the screen's edges (01:9F01-A661).
; C: src/stuntracefx/render/raster.c, which describes the list's entries.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

;
; @routine 01:98D7 FxPolygonGather
; WHAT IT DOES: Takes a shape of R0 points (index bytes in ROM at R14)
; to the screen: each point's x, y, z from the object's points at $0340
; (6 bytes each) plus the object's position ($0026-$002A) into the list
; at the word at $005A; the shape clipped against the near plane (z < 0
; is behind) into $0688, $0040 = the count left; then each vertex
; projected (FxProjectVertex), screen x and y back at $005A and the
; outcodes ORed into $014A. R0 = the count, or 0 when fewer than 3 are
; left or not every one of the outcode bits $0F00 is set.
; HOW: Pushes R11 on the R10 stack. A LOOP of R12 = the count (R13 =
; $98ED) of GETB, UMULT #6, three LDW and ADD. A line (2 points) is
; clipped as one segment (FxSegmentCrossZ) and both ends kept. Else each
; edge, the last back to the first: both ends behind (z AND z negative)
; gives nothing, the start behind its crossing, the start in front
; itself and the crossing when the end is behind; each written counts
; $0040 up. The projection (ROMB $14, R3, R13, R9 from $0026-$002A, R14
; saved at $001A, $014A cleared) is a count of DEC R12 with the SBK of
; the outcode in the branch's delay slot; then ROMB from $001C, R14 from
; $001A, R3 = $005A. Returns by pulling R15. The jump at 99E6 (fewer
; than 3 left) is decoded from the ROM bytes; the bytes after the routine
; (01:9A52-9F00) are not filed.
; Entry: R0 = the count; R14 at the index bytes; R10 = stack; R11 =
; return address. Exit: R0 as above; R1 = $0F00 and R0's AND (or the
; last clip's when it ended early); R3 = $005A; R8 past the screen
; points; R12 = 0; R14 as saved; S and Z from R10; the last RAM address
; R10. The rest as the last call leaves them. The ROM buffer moves as on
; the Super FX.
%at($0198D7)
FxPolygonGather:
    from r11
    stw (r10)
    inc r10
    inc r10
    move r9,r0
    lms r5,(!fx_object_x-!gsu_ram)
    lms r6,(!fx_object_y-!gsu_ram)
    lms r7,(!fx_object_z-!gsu_ram)
    lms r2,(!fx_poly_out-!gsu_ram)
    move r12,r9
    move r13,r15
    getb
    inc r14
    umult #6
    iwt r1,#$0340
    with r1
    add r0
    ldw (r1)
    inc r1
    inc r1
    add r5
    stw (r2)
    inc r2
    inc r2
    ldw (r1)
    inc r1
    inc r1
    add r6
    stw (r2)
    inc r2
    inc r2
    ldw (r1)
    add r7
    stw (r2)
    inc r2
    loop
    inc r2
    move r9,r9
    sub r0
    sms (!fx_vertex_count-!gsu_ram),r0
    lms r7,(!fx_poly_out-!gsu_ram)
    iwt r8,#$0688
    from r9
    sub #2
    bne .l9958
    nop
    to r1
    ldw (r7)
    inc r7
    inc r7
    to r2
    ldw (r7)
    inc r7
    inc r7
    to r3
    ldw (r7)
    inc r7
    inc r7
    to r4
    ldw (r7)
    inc r7
    inc r7
    to r5
    ldw (r7)
    inc r7
    inc r7
    to r6
    ldw (r7)
    link #4
    iwt r15,#FxSegmentCrossZ
    nop
    from r1
    stw (r8)
    inc r8
    inc r8
    from r2
    stw (r8)
    inc r8
    inc r8
    from r3
    stw (r8)
    inc r8
    inc r8
    from r4
    stw (r8)
    inc r8
    inc r8
    from r5
    stw (r8)
    inc r8
    inc r8
    from r6
    stw (r8)
    inc r8
    inc r8
    ibt r0,#$02
    sms (!fx_vertex_count-!gsu_ram),r0
    iwt r15,#.l99EA
    nop
.l9958:
    to r1
    ldw (r7)
    inc r7
    inc r7
    to r2
    ldw (r7)
    inc r7
    inc r7
    to r3
    ldw (r7)
    inc r7
    inc r7
    move r0,r9
    dec r0
    bne .l996D
    nop
    lms r7,(!fx_poly_out-!gsu_ram)
.l996D:
    to r4
    ldw (r7)
    inc r7
    inc r7
    to r5
    ldw (r7)
    inc r7
    inc r7
    to r6
    ldw (r7)
    with r7
    sub #4
    from r6
    and r3
    bpl .l9983
    nop
    iwt r15,#.l99D6
    nop
.l9983:
    moves r3,r3
    bpl .l998C
    nop
    iwt r15,#.l99C0
    nop
.l998C:
    from r1
    stw (r8)
    inc r8
    inc r8
    from r2
    stw (r8)
    inc r8
    inc r8
    from r3
    stw (r8)
    inc r8
    inc r8
    lms r0,(!fx_vertex_count-!gsu_ram)
    inc r0
    sbk
    moves r6,r6
    bmi .l99A6
    nop
    iwt r15,#.l99D6
    nop
.l99A6:
    link #4
    iwt r15,#FxSegmentCrossZ
    nop
    from r4
    stw (r8)
    inc r8
    inc r8
    from r5
    stw (r8)
    inc r8
    inc r8
    from r6
    stw (r8)
    inc r8
    inc r8
    lms r0,(!fx_vertex_count-!gsu_ram)
    inc r0
    sbk
    iwt r15,#.l99D6
    nop
.l99C0:
    link #4
    iwt r15,#FxSegmentCrossZ
    nop
    from r1
    stw (r8)
    inc r8
    inc r8
    from r2
    stw (r8)
    inc r8
    inc r8
    from r3
    stw (r8)
    inc r8
    inc r8
    lms r0,(!fx_vertex_count-!gsu_ram)
    inc r0
    sbk
.l99D6:
    dec r9
    beq .l99DE
    nop
    iwt r15,#.l9958
    nop
.l99DE:
    lms r0,(!fx_vertex_count-!gsu_ram)
    sub #3
    bpl .l99EA
    nop
    iwt r15,#.l9A4C
    nop
.l99EA:
    lms r3,(!fx_object_x-!gsu_ram)
    lms r13,(!fx_object_y-!gsu_ram)
    lms r9,(!fx_object_z-!gsu_ram)
    ibt r0,#$14
    romb
    sub r0
    sms (!fx_outcodes-!gsu_ram),r0
    sms (!fx_rom_r14-!gsu_ram),r14
    lms r12,(!fx_vertex_count-!gsu_ram)
    iwt r7,#$0688
    lms r8,(!fx_poly_out-!gsu_ram)
.l9A07:
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
    from r1
    stw (r8)
    inc r8
    inc r8
    from r2
    stw (r8)
    inc r8
    inc r8
    lms r6,(!fx_outcodes-!gsu_ram)
    or r6
    dec r12
    bne .l9A07
    sbk
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    lms r14,(!fx_rom_r14-!gsu_ram)
    lms r3,(!fx_poly_out-!gsu_ram)
    lms r0,(!fx_outcodes-!gsu_ram)
    iwt r1,#$0F00
    and r1
    cmp r1
    beq .l9A43
    nop
    iwt r15,#.l9A4C
    nop
.l9A43:
    nop
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.l9A4C:
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop


; FxClipLine (9F01, from FxPolygonClipScreen with fewer than 3 points):
; the line at $005A clipped against all four screen edges, from $005A to
; $0688 and back twice, each edge's pass as in FxPolygonClipScreen (the
; bottom edge's crossings one row inside it); fewer than 2 points left
; ends at A65C. Ends with R0 = 2, pulling R15. The passes' untraced
; paths (both ends outside the left or right edge, nothing left after
; them) are decoded from the ROM bytes: 9F6D, 9FC7, A03D, A09A.
%at($019F01)
FxClipLine:
    sms (!fx_vertex_count-!gsu_ram),r0
    lms r8,(!fx_poly_out-!gsu_ram)
    iwt r9,#$0688
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.l9F1F:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_left-!gsu_ram)
    from r1
    sub r0
    bge .l9F75
    nop
    lms r0,(!fx_screen_left-!gsu_ram)
    from r3
    sub r0
    blt .l9F6D
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_left-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_left-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .l9FB3
    nop
.l9F6D:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    bra .l9FB3
    nop
.l9F75:
    lms r0,(!fx_screen_left-!gsu_ram)
    from r3
    sub r0
    bge .l9FAB
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_left-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_left-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .l9FB3
    nop
.l9FAB:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.l9FB3:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .l9FBF
    nop
    iwt r15,#.l9F1F
    nop
.l9FBF:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #2
    bpl .l9FCB
    nop
    iwt r15,#FxPolygonClipScreen_lA65C
    nop
.l9FCB:
    add #2
    sms (!fx_vertex_count-!gsu_ram),r0
    iwt r8,#$0688
    lms r9,(!fx_poly_out-!gsu_ram)
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.l9FEB:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_right-!gsu_ram)
    from r1
    sub r0
    sub #1
    blt .lA046
    nop
    lms r0,(!fx_screen_right-!gsu_ram)
    from r3
    sub r0
    sub #1
    bge .lA03D
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_right-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_right-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .lA086
    nop
.lA03D:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    iwt r15,#.lA086
    nop
.lA046:
    lms r0,(!fx_screen_right-!gsu_ram)
    from r3
    sub r0
    sub #1
    blt .lA07E
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_right-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_right-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .lA086
    nop
.lA07E:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.lA086:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .lA092
    nop
    iwt r15,#.l9FEB
    nop
.lA092:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #2
    bpl .lA09E
    nop
    iwt r15,#FxPolygonClipScreen_lA65C
    nop
.lA09E:
    add #2
    sms (!fx_vertex_count-!gsu_ram),r0
    lms r8,(!fx_poly_out-!gsu_ram)
    iwt r9,#$0688
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.lA0BE:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_top-!gsu_ram)
    from r2
    sub r0
    bge .lA11A
    nop
    lms r0,(!fx_screen_top-!gsu_ram)
    from r6
    sub r0
    blt .lA112
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_top-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_top-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA15E
    nop
.lA112:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    bra .lA15E
    nop
.lA11A:
    lms r0,(!fx_screen_top-!gsu_ram)
    from r6
    sub r0
    bge .lA156
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_top-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_top-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA15E
    nop
.lA156:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.lA15E:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .lA16A
    nop
    iwt r15,#.lA0BE
    nop
.lA16A:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #2
    bpl .lA176
    nop
    iwt r15,#FxPolygonClipScreen_lA65C
    nop
.lA176:
    add #2
    sms (!fx_vertex_count-!gsu_ram),r0
    iwt r8,#$0688
    lms r9,(!fx_poly_out-!gsu_ram)
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.lA196:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r2
    sub r0
    sub #1
    blt .lA1F7
    nop
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r6
    sub r0
    sub #1
    bge .lA1EF
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_bottom-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_bottom-!gsu_ram)
    dec r6
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA23E
    nop
.lA1EF:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    bra .lA23E
    nop
.lA1F7:
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r6
    sub r0
    sub #1
    blt .lA236
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_bottom-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_bottom-!gsu_ram)
    dec r6
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA23E
    nop
.lA236:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.lA23E:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .lA24A
    nop
    iwt r15,#.lA196
    nop
.lA24A:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #2
    bpl .lA256
    nop
    iwt r15,#FxPolygonClipScreen_lA65C
    nop
.lA256:
    add #2
    ibt r0,#$02
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:A25F FxPolygonClipScreen
; WHAT IT DOES: Clips the projected polygon at the word at $005A (R0
; vertices, x and y words each) against the edges of the screen ($38
; and $3A in x, $3C and $3E in y) that its outcodes at $014A cross (bit
; 2 left, 3 right, 0 top, 1 bottom), leaving the result at $005A again;
; a line (fewer than 3 points) is clipped against all four
; (FxClipLine). R0 = the count left, 0 when fewer than 3 (2 for a line)
; are.
; HOW: $0060 = $005A, $0062 = $0688, the count at $0098; pushes R11
; on the R10 stack. For each edge clipped: the count to $0040, the first
; vertex copied after the last, then for each edge of the polygon the
; start written when inside, the crossing (LMULT, FxDivideSigned) when
; it crosses, $0098 counted up or down; the lists swapped; fewer than 3
; left goes to A65C (R0 = 0). Then, unless the list read last is
; $005A's, the vertices are copied back there (LOOP) and R0 = $0098.
; Returns by pulling R15. The incoming and outgoing crossings are the
; same code twice, with their own LINK. (The catalog's routine runs on
; through FxDrawTextured to 01:A90C.)
; Entry: R0 = the vertex count; R10 = stack; R11 = return address. Exit:
; R0 as above; R11 = the last LINK's return; R4-R6 from the last
; division (when any); R8, R9 past the last vertices read and written;
; R12 = 0 and R13 the copy's LOOP when it ran; S and Z from R10, the
; last RAM address R10. The rest as the last step leaves them.
FxPolygonClipScreen:
    lms r8,(!fx_poly_out-!gsu_ram)
    iwt r9,#$0688
    sms (!fx_list_in-!gsu_ram),r8
    sms (!fx_list_out-!gsu_ram),r9
    sms (!fx_vertex_count_save-!gsu_ram),r0
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$03
    cmp r1
    bpl .lA27D
    nop
    iwt r15,#FxClipLine
    nop
.lA27D:
    lms r1,(!fx_outcodes-!gsu_ram)
    with r1
    and #4
    bne .lA28A
    nop
    iwt r15,#.lA362
    nop
.lA28A:
    lms r8,(!fx_list_in-!gsu_ram)
    lms r9,(!fx_list_out-!gsu_ram)
    sms (!fx_vertex_count-!gsu_ram),r0
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.lA2A8:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_left-!gsu_ram)
    from r1
    sub r0
    bge .lA2FE
    nop
    lms r0,(!fx_screen_left-!gsu_ram)
    from r3
    sub r0
    blt .lA2F6
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_left-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_left-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .lA33C
    nop
.lA2F6:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    bra .lA33C
    nop
.lA2FE:
    lms r0,(!fx_screen_left-!gsu_ram)
    from r3
    sub r0
    bge .lA334
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_left-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_left-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .lA33C
    nop
.lA334:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.lA33C:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .lA348
    nop
    iwt r15,#.lA2A8
    nop
.lA348:
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .lA360
    nop
    iwt r15,#.lA65C
    nop
.lA360:
    add #3
.lA362:
    lms r1,(!fx_outcodes-!gsu_ram)
    with r1
    and #8
    bne .lA36F
    nop
    iwt r15,#.lA44D
    nop
.lA36F:
    lms r8,(!fx_list_in-!gsu_ram)
    lms r9,(!fx_list_out-!gsu_ram)
    sms (!fx_vertex_count-!gsu_ram),r0
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.lA38D:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_right-!gsu_ram)
    from r1
    sub r0
    sub #1
    blt .lA3E7
    nop
    lms r0,(!fx_screen_right-!gsu_ram)
    from r3
    sub r0
    sub #1
    bge .lA3DF
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_right-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_right-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .lA427
    nop
.lA3DF:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    bra .lA427
    nop
.lA3E7:
    lms r0,(!fx_screen_right-!gsu_ram)
    from r3
    sub r0
    sub #1
    blt .lA41F
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    lms r0,(!fx_screen_right-!gsu_ram)
    sub r1
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r2
    lms r6,(!fx_screen_right-!gsu_ram)
    from r6
    stw (r9)
    inc r9
    inc r9
    stw (r9)
    inc r9
    inc r9
    bra .lA427
    nop
.lA41F:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.lA427:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .lA433
    nop
    iwt r15,#.lA38D
    nop
.lA433:
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .lA44B
    nop
    iwt r15,#.lA65C
    nop
.lA44B:
    add #3
.lA44D:
    lms r1,(!fx_outcodes-!gsu_ram)
    with r1
    and #1
    bne .lA45A
    nop
    iwt r15,#.lA53E
    nop
.lA45A:
    lms r8,(!fx_list_in-!gsu_ram)
    lms r9,(!fx_list_out-!gsu_ram)
    sms (!fx_vertex_count-!gsu_ram),r0
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.lA478:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_top-!gsu_ram)
    from r2
    sub r0
    bge .lA4D4
    nop
    lms r0,(!fx_screen_top-!gsu_ram)
    from r6
    sub r0
    blt .lA4CC
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_top-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_top-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA518
    nop
.lA4CC:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    bra .lA518
    nop
.lA4D4:
    lms r0,(!fx_screen_top-!gsu_ram)
    from r6
    sub r0
    bge .lA510
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_top-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_top-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA518
    nop
.lA510:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.lA518:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .lA524
    nop
    iwt r15,#.lA478
    nop
.lA524:
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .lA53C
    nop
    iwt r15,#.lA65C
    nop
.lA53C:
    add #3
.lA53E:
    lms r1,(!fx_outcodes-!gsu_ram)
    with r1
    and #2
    bne .lA54B
    nop
    iwt r15,#.lA635
    nop
.lA54B:
    lms r8,(!fx_list_in-!gsu_ram)
    lms r9,(!fx_list_out-!gsu_ram)
    sms (!fx_vertex_count-!gsu_ram),r0
    to r1
    ldw (r8)
    inc r8
    inc r8
    to r2
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_vertex_count-!gsu_ram)
    mult #4
    add r8
    from r1
    stw (r0)
    inc r0
    inc r0
    from r2
    stw (r0)
.lA569:
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
    to r6
    ldw (r8)
    with r8
    sub #2
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r2
    sub r0
    sub #1
    blt .lA5C9
    nop
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r6
    sub r0
    sub #1
    bge .lA5C1
    nop
    move r0,r1
    move r1,r3
    move r3,r0
    move r0,r2
    move r2,r6
    move r6,r0
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_bottom-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_bottom-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA60F
    nop
.lA5C1:
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    dec r0
    sbk
    bra .lA60F
    nop
.lA5C9:
    lms r0,(!fx_screen_bottom-!gsu_ram)
    from r6
    sub r0
    sub #1
    blt .lA607
    nop
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    inc r0
    sbk
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
    with r3
    sub r1
    with r6
    sub r2
    move r0,r3
    move r3,r6
    move r6,r0
    lms r0,(!fx_screen_bottom-!gsu_ram)
    sub r2
    to r5
    lmult
    move r6,r3
    link #4
    iwt r15,#FxDivideSigned
    nop
    from r4
    add r1
    lms r6,(!fx_screen_bottom-!gsu_ram)
    stw (r9)
    inc r9
    inc r9
    from r6
    stw (r9)
    inc r9
    inc r9
    bra .lA60F
    nop
.lA607:
    from r1
    stw (r9)
    inc r9
    inc r9
    from r2
    stw (r9)
    inc r9
    inc r9
.lA60F:
    lms r0,(!fx_vertex_count-!gsu_ram)
    dec r0
    sbk
    beq .lA61B
    nop
    iwt r15,#.lA569
    nop
.lA61B:
    lms r1,(!fx_list_in-!gsu_ram)
    lms r2,(!fx_list_out-!gsu_ram)
    sms (!fx_list_in-!gsu_ram),r2
    sms (!fx_list_out-!gsu_ram),r1
    lms r0,(!fx_vertex_count_save-!gsu_ram)
    sub #3
    bpl .lA633
    nop
    iwt r15,#.lA65C
    nop
.lA633:
    add #3
.lA635:
    lms r1,(!fx_poly_out-!gsu_ram)
    lms r8,(!fx_list_in-!gsu_ram)
    with r1
    sub r8
    beq .lA657
    nop
    lms r9,(!fx_list_out-!gsu_ram)
    move r12,r0
    move r13,r15
    ldw (r8)
    stw (r9)
    inc r9
    inc r9
    inc r8
    inc r8
    ldw (r8)
    stw (r9)
    inc r9
    inc r9
    inc r8
    loop
    inc r8
    lms r0,(!fx_vertex_count_save-!gsu_ram)
.lA657:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lA65C:
    sub r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop


; FxDrawTextured (A662, reached from FxDrawListDispatch with R0 = NOT the
; type): a textured polygon. Its vertices are copied below the entry (u
; and v moved down a byte), then the left and right edges are walked from
; the top vertex (A7D9 / A870, set up at A7E2 / A879) and the rows between
; them filled (A710-A7D2): each texel from MERGE of v and u, the mask at
; $00D4 and the texture's address, GETC, PLOT. Ends at A909, back to the
; list. (Inside the catalog's 01:A25F, after FxPolygonClipScreen's code;
; the CACHE at A90D is never run and is left in the raw bank.)

%at($01A662)
FxDrawTextured:
    lob
    move r12,r0
    add r0
    add r0
    add r0
    move r3,r0
    ldb (r1)
    inc r1
    sms (!fx_tex_bank-!gsu_ram),r0
    sub r0
    cmode
    ldw (r1)
    inc r1
    inc r1
    color
    swap
    cmode
    ldw (r1)
    inc r1
    inc r1
    sms (!fx_tex_base-!gsu_ram),r0
    ldw (r1)
    inc r1
    inc r1
    sms (!fx_tex_mask-!gsu_ram),r0
    from r1
    add r3
    sms (!fx_list_end-!gsu_ram),r0
    move r5,r1
    from r1
    to r4
    sub r3
    iwt r2,#$03FF
    ibt r8,#$00
    move r13,r15
    ldb (r5)
    stb (r4)
    inc r4
    inc r4
    inc r5
    inc r5
    ldb (r5)
    stb (r4)
    cmp r2
    bpl .lA6AC
    nop
    move r2,r0
    move r6,r4
.lA6AC:
    cmp r8
    bmi .lA6B3
    nop
    move r8,r0
.lA6B3:
    inc r4
    inc r4
    inc r5
    inc r5
    sub r0
    inc r5
    ldb (r5)
    dec r5
    stb (r5)
    inc r5
    inc r5
    stb (r4)
    inc r4
    inc r4
    inc r5
    ldb (r5)
    dec r5
    stb (r5)
    inc r5
    inc r5
    stb (r4)
    inc r4
    loop
    inc r4
    from r2
    sub r8
    bne .lA6DA
    nop
    iwt r15,#.lA909
    nop
.lA6DA:
    move r4,r6
    dec r4
    dec r4
    from r4
    to r5
    add r3
    ldb (r4)
    inc r4
    inc r4
    to r9
    swap
    move r10,r9
    inc r4
    inc r4
    ldb (r4)
    swap
    sms (!fx_left_u-!gsu_ram),r0
    sms (!fx_right_u-!gsu_ram),r0
    inc r4
    inc r4
    ldb (r4)
    swap
    sms (!fx_left_v-!gsu_ram),r0
    sms (!fx_right_v-!gsu_ram),r0
    inc r4
    inc r4
    sms (!fx_left_at-!gsu_ram),r4
    sms (!fx_right_at-!gsu_ram),r5
    ibt r11,#$00
    ibt r3,#$00
    cache
    iwt r15,#.lA7D9
    nop
.lA710:
    sms (!fx_left_slope-!gsu_ram),r7
    sms (!fx_right_slope-!gsu_ram),r8
    from r11
    sub r3
    bmi .lA729
    nop
    sms (!fx_left_rows-!gsu_ram),r0
    from r3
    swap
    to r2
    or r2
    sub r0
    sms (!fx_right_rows-!gsu_ram),r0
    bra .lA736
    nop
.lA729:
    from r3
    sub r11
    sms (!fx_right_rows-!gsu_ram),r0
    from r11
    swap
    to r2
    or r2
    sub r0
    sms (!fx_left_rows-!gsu_ram),r0
.lA736:
    from r9
    to r1
    hib
    from r10
    hib
    to r12
    sub r1
    bcs .lA742
    inc r12
    ibt r12,#$01
.lA742:
    iwt r0,#$94A0
    add r12
    to r14
    add r12
    to r6
    getb
    inc r14
    with r6
    getbh
    lms r0,(!fx_right_du-!gsu_ram)
    lms r4,(!fx_right_u-!gsu_ram)
    add r4
    sbk
    lms r8,(!fx_left_u-!gsu_ram)
    from r4
    sub r8
    fmult
    to r5
    rol
    lms r0,(!fx_left_du-!gsu_ram)
    add r8
    sms (!fx_left_u-!gsu_ram),r0
    lms r0,(!fx_right_dv-!gsu_ram)
    lms r4,(!fx_right_v-!gsu_ram)
    add r4
    sbk
    lms r7,(!fx_left_v-!gsu_ram)
    from r4
    sub r7
    fmult
    to r6
    rol
    lms r0,(!fx_left_dv-!gsu_ram)
    add r7
    sms (!fx_left_v-!gsu_ram),r0
    lms r4,(!fx_tex_u_off-!gsu_ram)
    with r8
    add r4
    lms r4,(!fx_tex_v_off-!gsu_ram)
    with r7
    add r4
    lms r0,(!fx_tex_bank-!gsu_ram)
    lob
    romb
    lms r4,(!fx_tex_base-!gsu_ram)
    lms r3,(!fx_tex_mask-!gsu_ram)
    move r13,r15
    merge
    and r3
    to r14
    add r4
    with r7
    add r6
    with r8
    add r5
    getc
    loop
    plot
    sub r0
    romb
    lms r7,(!fx_left_slope-!gsu_ram)
    lms r8,(!fx_right_slope-!gsu_ram)
    with r9
    add r7
    bpl .lA7B6
    nop
    iwt r0,#$0800
    add r9
    bmi .lA7B6
    nop
    ibt r9,#$00
.lA7B6:
    with r10
    add r8
    bpl .lA7C4
    nop
    iwt r0,#$0800
    add r10
    bmi .lA7C4
    nop
    ibt r10,#$00
.lA7C4:
    inc r2
    from r2
    swap
    dec r0
    to r2
    swap
    from r2
    hib
    beq .lA7D3
    nop
    iwt r15,#.lA736
    nop
.lA7D3:
    lms r11,(!fx_left_rows-!gsu_ram)
    lms r3,(!fx_right_rows-!gsu_ram)
.lA7D9:
    moves r0,r11
    beq .lA7E2
    nop
    iwt r15,#.lA870
    nop
.lA7E2:
    ibt r0,#$7F
    add r9
    to r9
    hib
    lms r0,(!fx_left_u-!gsu_ram)
    hib
    sbk
    lms r0,(!fx_left_v-!gsu_ram)
    hib
    sbk
    lms r6,(!fx_left_at-!gsu_ram)
.lA7F4:
    move r12,r9
    lms r0,(!fx_left_u-!gsu_ram)
    sms (!fx_left_u_prev-!gsu_ram),r0
    lms r0,(!fx_left_v-!gsu_ram)
    sms (!fx_left_v_prev-!gsu_ram),r0
    to r9
    ldb (r6)
    inc r6
    inc r6
    to r4
    ldb (r6)
    inc r6
    inc r6
    ldb (r6)
    sms (!fx_left_u-!gsu_ram),r0
    inc r6
    inc r6
    ldb (r6)
    sms (!fx_left_v-!gsu_ram),r0
    inc r6
    inc r6
    from r4
    to r11
    sub r2
    beq .lA7F4
    nop
    bpl .lA827
    nop
    iwt r15,#.lA909
    nop
.lA827:
    sms (!fx_left_at-!gsu_ram),r6
    iwt r0,#$94A0
    add r11
    to r14
    add r11
    to r6
    getb
    inc r14
    with r6
    getbh
    from r9
    sub r12
    from r12
    to r9
    swap
    lmult
    with r4
    add r4
    rol
    lob
    swap
    with r4
    hib
    to r7
    or r4
    lms r0,(!fx_left_u-!gsu_ram)
    lms r1,(!fx_left_u_prev-!gsu_ram)
    sub r1
    lob
    swap
    fmult
    rol
    sms (!fx_left_du-!gsu_ram),r0
    lms r0,(!fx_left_u_prev-!gsu_ram)
    swap
    sms (!fx_left_u-!gsu_ram),r0
    lms r0,(!fx_left_v-!gsu_ram)
    lms r1,(!fx_left_v_prev-!gsu_ram)
    sub r1
    lob
    swap
    fmult
    rol
    sms (!fx_left_dv-!gsu_ram),r0
    lms r0,(!fx_left_v_prev-!gsu_ram)
    swap
    sms (!fx_left_v-!gsu_ram),r0
.lA870:
    moves r0,r3
    beq .lA879
    nop
    iwt r15,#.lA710
    nop
.lA879:
    ibt r0,#$7F
    add r10
    to r10
    hib
    lms r0,(!fx_right_u-!gsu_ram)
    hib
    sbk
    lms r0,(!fx_right_v-!gsu_ram)
    hib
    sbk
    lms r6,(!fx_right_at-!gsu_ram)
.lA88B:
    move r12,r10
    lms r0,(!fx_right_u-!gsu_ram)
    sms (!fx_right_u_prev-!gsu_ram),r0
    lms r0,(!fx_right_v-!gsu_ram)
    sms (!fx_right_v_prev-!gsu_ram),r0
    dec r6
    dec r6
    ldb (r6)
    sms (!fx_right_v-!gsu_ram),r0
    dec r6
    dec r6
    ldb (r6)
    sms (!fx_right_u-!gsu_ram),r0
    dec r6
    dec r6
    ldb (r6)
    dec r6
    dec r6
    to r10
    ldb (r6)
    to r3
    sub r2
    beq .lA88B
    nop
    bpl .lA8BC
    nop
    iwt r15,#.lA909
    nop
.lA8BC:
    sms (!fx_right_at-!gsu_ram),r6
    iwt r0,#$94A0
    add r3
    to r14
    add r3
    to r6
    getb
    inc r14
    with r6
    getbh
    from r10
    sub r12
    from r12
    to r10
    swap
    lmult
    with r4
    add r4
    rol
    lob
    swap
    with r4
    hib
    to r8
    or r4
    lms r0,(!fx_right_u-!gsu_ram)
    lms r1,(!fx_right_u_prev-!gsu_ram)
    sub r1
    lob
    swap
    fmult
    rol
    sms (!fx_right_du-!gsu_ram),r0
    lms r0,(!fx_right_u_prev-!gsu_ram)
    swap
    sms (!fx_right_u-!gsu_ram),r0
    lms r0,(!fx_right_v-!gsu_ram)
    lms r1,(!fx_right_v_prev-!gsu_ram)
    sub r1
    lob
    swap
    fmult
    rol
    sms (!fx_right_dv-!gsu_ram),r0
    lms r0,(!fx_right_v_prev-!gsu_ram)
    swap
    sms (!fx_right_v-!gsu_ram),r0
    iwt r15,#.lA710
    nop
.lA909:
    iwt r15,#FxDrawList_lA950
    nop

; FxDrawListDispatch (A90E, from FxDrawList's BMI with R0 = type - 2 and
; its flags): 2 is a line; 0 ends the list (A91E: $005C = $08AA, the ROM
; bank back from $001C, R15 pulled from the stack at $005E); a negative
; type is a textured polygon, its count NOT the type.
%at($01A90E)
FxDrawListDispatch:
    bne .lA915
    nop
    iwt r15,#FxDrawLine
    nop
.lA915:
    inc r0
    inc r0
    beq .lA91E
    not
    iwt r15,#FxDrawTextured
    nop
.lA91E:
    iwt r1,#$08AA
    sms (!fx_list_end-!gsu_ram),r1
    lms r0,(!fx_rom_bank-!gsu_ram)
    romb
    lms r10,(!fx_list_stack-!gsu_ram)
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:A931 FxDrawList
; WHAT IT DOES: Draws the frame's queued shapes, the list at $08AA up to
; the word at $005C: flat polygons, lines and textured polygons (raster.c
; describes the entries), each into the Super FX's screen with PLOT; then
; empties the list ($005C = $08AA). An empty list returns at once.
; HOW: R0 = $08AA - $005C; equal, JMP R11. Else pushes R11 on the R10
; stack and keeps R10 at $005E, writes a 0 word (the end) at the list's
; end, $005C = $08AA, ROMB 0. For each entry (A950): the type byte (LDB,
; SEX), less 3; BMI, with INC R0 in its delay slot, to
; FxDrawListDispatch. A flat polygon (A95C) CACHEs, COLOR $70 then CMODE
; 0 and COLOR its colour, copies its vertices below the entry (finding
; the top and bottom rows: R2, R8, R6 at the top one's copy) and, less
; than two rows high, draws one row (AA74); else R6 walks the left edges
; forward, R5 the right ones back (A9D0, AA0D), each edge's x slope from
; the reciprocal of its height at $00:94A0 (LMULT), and the rows between
; are drawn (AA3F) while R11 AND R3, the rows left on each edge, is not 0.
; Each shape leaves $005C at the next entry and jumps back to A950.
; Entry: R10 = stack; R11 = return address. Exit: an empty list: R0 = 0,
; R1 = $08AA, flags from the SUB. Else R1 = $08AA, R0 = the ROM bank
; from $001C, R10 = $005E - 2, the last RAM address R10, S and Z from
; DEC R10; the rest as the last shape drawn leaves them. R14, the ROM
; buffer, the colour and plot options move as on the Super FX.
%at($01A931)
FxDrawList:
    lms r1,(!fx_list_end-!gsu_ram)
    iwt r0,#$08AA
    sub r1
    bne .lA93D
    nop
    jmp r11
    nop
.lA93D:
    from r11
    stw (r10)
    inc r10
    inc r10
    sms (!fx_list_stack-!gsu_ram),r10
    sub r0
    stw (r1)
    iwt r1,#$08AA
    sms (!fx_list_end-!gsu_ram),r1
    ibt r0,#$00
    romb
.lA950:
    lms r1,(!fx_list_end-!gsu_ram)
    ldb (r1)
    sex
    inc r1
    sub #3
    bmi FxDrawListDispatch
    inc r0
    cache
    inc r0
    inc r0
    move r12,r0
    add r0
    add r0
    move r3,r0
    ibt r0,#$70
    color
    ibt r0,#$00
    cmode
    ldb (r1)
    inc r1
    color
    from r1
    add r3
    sms (!fx_list_end-!gsu_ram),r0
    move r5,r1
    from r1
    to r4
    sub r3
    iwt r2,#$03FF
    ibt r8,#$00
    move r13,r15
    ldb (r5)
    stb (r4)
    inc r4
    inc r4
    inc r5
    inc r5
    ldb (r5)
    stb (r4)
    cmp r2
    bpl .lA996
    inc r5
    move r2,r0
    move r6,r4
.lA996:
    sub r8
    bmi .lA99C
    inc r5
    to r8
    add r8
.lA99C:
    inc r4
    loop
    inc r4
    from r8
    sub r2
    dec r0
    dec r0
    bpl .lA9AA
    nop
    iwt r15,#.lAA74
    nop
.lA9AA:
    dec r6
    dec r6
    from r6
    to r5
    add r3
    dec r5
    dec r5
    to r8
    ldb (r6)
    with r6
    add #4
    to r9
    from r8
    swap
    iwt r13,#$AA49
    with r3
    bra .lA9D0
    sub r3
.lA9C1:
    moves r11,r11
    bne .lAA03
    nop
    ibt r0,#$7F
    add r7
    to r7
    hib
    lms r6,(!fx_left_at-!gsu_ram)
.lA9CE:
    move r8,r7
.lA9D0:
    to r7
    ldb (r6)
    inc r6
    inc r6
    ldb (r6)
    inc r6
    to r11
    sub r2
    bpl .lA9E1
    nop
    iwt r15,#.lAA70
    nop
.lA9E1:
    beq .lA9CE
    inc r6
    iwt r0,#$94A0
    add r11
    to r14
    add r11
    sms (!fx_left_at-!gsu_ram),r6
    from r7
    sub r8
    to r6
    getb
    inc r14
    from r8
    to r7
    swap
    with r6
    getbh
    lmult
    with r4
    add r4
    rol
    lob
    swap
    with r4
    hib
    to r8
    or r4
.lAA03:
    moves r3,r3
    bne .lAA3F
    nop
    ibt r0,#$7F
    add r9
    to r9
    hib
.lAA0D:
    move r10,r9
    ldb (r5)
    dec r5
    dec r5
    to r9
    ldb (r5)
    dec r5
    to r3
    sub r2
    bpl .lAA20
    nop
    iwt r15,#.lAA70
    nop
.lAA20:
    beq .lAA0D
    dec r5
    iwt r0,#$94A0
    add r3
    to r14
    add r3
    from r9
    sub r10
    to r6
    getb
    inc r14
    from r10
    to r9
    swap
    with r6
    getbh
    lmult
    with r4
    add r4
    rol
    lob
    swap
    with r4
    hib
    to r10
    or r4
.lAA3F:
    from r7
    to r1
    hib
    from r9
    hib
    to r12
    sub r1
    bmi .lAA4B
    inc r12
    loop
    plot
.lAA4B:
    with r7
    add r8
    bpl .lAA59
    dec r11
    iwt r0,#$0800
    add r7
    bmi .lAA59
    nop
    ibt r7,#$00
.lAA59:
    with r9
    add r10
    bpl .lAA67
    dec r3
    iwt r0,#$0800
    add r9
    bmi .lAA67
    nop
    ibt r9,#$00
.lAA67:
    from r11
    and r3
    bne .lAA3F
    inc r2
    iwt r15,#.lA9C1
    nop
.lAA70:
    iwt r15,#.lA950
    nop
.lAA74:
    from r3
    lsr
    to r12
    lsr
    iwt r1,#$03FF
    ibt r8,#$00
    move r13,r15
    ldb (r4)
    cmp r1
    bpl .lAA88
    inc r4
    move r1,r0
.lAA88:
    sub r8
    bmi .lAA8E
    inc r4
    to r8
    add r8
.lAA8E:
    inc r4
    loop
    inc r4
    from r8
    to r12
    sub r1
    bmi .lAA9E
    nop
    beq .lAA9E
    nop
    move r13,r15
    loop
    plot
.lAA9E:
    iwt r15,#.lA950
    nop
; FxDrawLine (AAA2, from FxDrawListDispatch with R1 at the colour byte):
; a line from (x1, y1) to (x2, y2), stepped along the longer axis with
; the error in R0; the carry of its SUB picks the step across. (The
; JMP R11; NOP after it, 01:AB11, is never run and is left in the raw
; bank.)
FxDrawLine:
    ibt r0,#$70
    color
    ibt r0,#$00
    cmode
    move r7,r1
    ldb (r7)
    inc r7
    color
    to r1
    ldb (r7)
    inc r7
    inc r7
    to r2
    ldb (r7)
    inc r7
    inc r7
    to r3
    ldb (r7)
    inc r7
    inc r7
    to r4
    ldb (r7)
    inc r7
    inc r7
    sms (!fx_list_end-!gsu_ram),r7
    ibt r5,#$01
    with r3
    sub r1
    bpl .lAAD2
    nop
    ibt r5,#$FF
    with r3
    not
    inc r3
.lAAD2:
    ibt r6,#$01
    with r4
    sub r2
    bpl .lAADE
    nop
    ibt r6,#$FF
    with r4
    not
    inc r4
.lAADE:
    from r3
    sub r4
    bmi .lAAF9
    nop
    move r12,r3
    inc r12
    lsr
    sub r4
    move r13,r15
    bcs .lAAF0
    plot
    add r3
    with r2
    add r6
.lAAF0:
    dec r1
    with r1
    add r5
    loop
    sub r4
    iwt r15,#FxDrawList_lA950
    nop
.lAAF9:
    move r12,r4
    inc r12
    not
    inc r0
    lsr
    sub r3
    move r13,r15
    bcs .lAB08
    plot
    add r4
    with r1
    add r5
.lAB08:
    dec r1
    with r2
    add r6
    loop
    sub r3
    iwt r15,#FxDrawList_lA950
    nop

