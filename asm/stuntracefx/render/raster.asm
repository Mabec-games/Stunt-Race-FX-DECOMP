; raster.asm: the Super FX drawing the frame's queued shapes (flat
; polygons, lines, textured polygons) from the draw list at $70:08AA into
; its screen buffer. C: src/stuntracefx/render/raster.c, which describes
; the list's entries.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; FxDrawTextured (A662, reached from FxDrawListDispatch with R0 = NOT the
; type): a textured polygon. Its vertices are copied below the entry (u
; and v moved down a byte), then the left and right edges are walked from
; the top vertex (A7D9 / A870, set up at A7E2 / A879) and the rows between
; them filled (A710-A7D2): each texel from MERGE of v and u, the mask at
; $00D4 and the texture's address, GETC, PLOT. Ends at A909, back to the
; list. (Inside the catalog's 01:A25F; the CACHE at A90D is never run.)

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

