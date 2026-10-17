; text.asm: the Super FX drawing a string of text and pictures into the
; screen it renders: glyphs from '0' up, $0D a new line, 0 the end, and the
; commands $F5-$FF (position, palette, font, space, half line, pictures).
; C: src/stuntracefx/render/text.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 01:D12F FxTextNotice
; WHAT IT DOES: When bit 0 of $273C is set and there is a nearest car
; ($2648) that is neither $2476 (the player) nor $2478, and its $264C is
; not negative, draws its label (the string from the table at $01:D1CB,
; by the word at $96 + the car, in ROM bank 1) at Y $68 and X = half of
; $264A plus the screen centre ($34), held to 0..$3A - $11; $2732 is
; cleared after.
; HOW: Pushes R11 on the R10 stack; CMPs and MOVES for the tests; DIV2,
; then BPL and CMP for the bounds; ROMB 1, GETB and GETBH for the string's
; address; FxTextDraw. Returns by pulling R15.
; Entry: R10 = stack; R11 = return address. Exit: as FxTextDraw, R0 = 0;
; or, when nothing is drawn, R0 and R1 as the last test left them; S and
; Z from R10; the last RAM address R10.
%at($01D12F)
FxTextNotice:
    from r11
    stw (r10)
    inc r10
    inc r10
    lm r0,(!fx_text_flags)
    iwt r1,#$0001
    and r1
    bne .lD142
    nop
    iwt r15,#.lD1C6
    nop
.lD142:
    lm r0,(!fx_near_car)
    moves r0,r0
    bne .lD14F
    nop
    iwt r15,#.lD1C6
    nop
.lD14F:
    lm r1,(!fx_player)
    cmp r1
    bne .lD15C
    nop
    iwt r15,#.lD1C6
    nop
.lD15C:
    lm r1,(!gsu_match_2478)
    cmp r1
    bne .lD169
    nop
    iwt r15,#.lD1C6
    nop
.lD169:
    move r2,r0
    lm r0,(!fx_near_x)
    moves r0,r0
    bpl .lD178
    nop
    iwt r15,#.lD1C6
    nop
.lD178:
    lm r0,(!fx_near_z)
    div2
    lm r1,(!gsu_in_34)
    add r1
    bpl .lD187
    nop
    sub r0
.lD187:
    ibt r4,#$11
    lm r1,(!gsu_in_3a)
    with r1
    sub r4
    cmp r1
    bmi .lD196
    nop
    move r0,r1
.lD196:
    sm (!fx_text_x),r0
    iwt r0,#$0068
    sm (!fx_text_y),r0
    ibt r0,#$01
    sm (!fx_text_bank),r0
    romb
    iwt r0,#$0096
    add r2
    ldw (r0)
    add r0
    iwt r14,#$D1CB
    with r14
    add r0
    getb
    inc r14
    getbh
    sm (!fx_text_at),r0
    link #4
    iwt r15,#FxTextDraw
    nop
    sub r0
    sm (!fx_text_at),r0
.lD1C6:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

; @routine 01:D307 FxTextProgram
; WHAT IT DOES: The program the S-CPU starts to draw the string at $2732:
; FxTextDraw with the R10 stack at $0240.
; HOW: IWT R10, LINK, the jump, then STOP.
; Entry: none. Exit: as FxTextDraw, R10 = $0240.
%at($01D307)
FxTextProgram:
    iwt r10,#$0240
    link #4
    iwt r15,#FxTextDraw
    nop
    stop
    nop

; @routine 01:D311 FxTextDraw
; WHAT IT DOES: Draws the string at the address in $2732 (ROM bank $2734)
; from the position at $2738/$273A: glyphs advance X by 20 (10 in the small
; font), $0D goes back to the start X a line down; $FF sets the position
; (two bytes), $FE the palette (one byte: $2702 + 4 * it), $FD the font
; (one byte: 0 big, 32 lines, else small, 16 lines), $FC a space (8, 4 in
; the small font), $F5 half a line down; $FB, $FA, $F9, $F8, $F7 and $F6
; draw pictures from bank $15 ($3700 120 x 32, $3B00 120 x 16, $1810 64 x
; 16, $3700 and $3A00 120 x 24, $3D1C 16 x 24). Other bytes below '0' only
; advance X. Nothing is drawn when $2732 is 0.
; HOW: Pushes R11 on the R10 stack. RAMB 0, CMODE 0, R9 = $2702. R8 walks
; the string: each byte is read by setting R14 to it in the string's bank
; (ROMB from $2734 each time), GETB, SEX. Commands are CMPs against bytes
; IBT sign-extends; their bytes are read through R14. A glyph's bitmap
; offset comes from ROM bank 1 ($F4D2 big, $F52E small) and is drawn by
; FxDrawBitmap16x32 or FxDrawBitmap8x16. At the end RPIX writes out the
; pixel caches. Returns by pulling R15.
; Entry: R10 = stack; R11 = return address. Exit: R0 the pixel RPIX read
; at (R1, R2); R4, R2 the position reached; R7 the line start X; R5, R6
; the font; R8 past the string; R9 the palette; the rest as the last
; drawing left them; S and Z from R10; the last RAM address R10. R14 and
; the ROM buffer move as on the Super FX.
%at($01D311)
FxTextDraw:
    from r11
    stw (r10)
    inc r10
    inc r10
    lm r7,(!fx_text_x)
    lm r2,(!fx_text_y)
    ibt r5,#$00
    iwt r6,#$0020
    sub r0
    ramb
    ibt r0,#$00
    cmode
    iwt r9,#$2702
    lm r0,(!fx_text_at)
    moves r8,r0
    bne .lD339
    nop
    iwt r15,#.lD4DD
    nop
.lD339:
    move r4,r7
.lD33B:
    lm r0,(!fx_text_bank)
    lob
    romb
    move r14,r8
    getb
    inc r8
    sex
    bne .lD34E
    nop
    iwt r15,#.lD4DD
    nop
.lD34E:
    bmi .lD355
    nop
    iwt r15,#.lD46F
    nop
.lD355:
    inc r14
    ibt r3,#$FF
    cmp r3
    bne .lD36A
    nop
    to r7
    getb
    inc r14
    to r2
    getb
    inc r14
    move r4,r7
    move r8,r14
    bra .lD33B
    nop
.lD36A:
    ibt r3,#$FE
    cmp r3
    bne .lD37C
    nop
    getb
    inc r8
    add r0
    add r0
    iwt r9,#$2702
    to r9
    bra .lD33B
    add r9
.lD37C:
    ibt r3,#$FD
    cmp r3
    bne .lD39C
    nop
    getb
    inc r8
    moves r0,r0
    beq .lD393
    nop
    iwt r6,#$0010
    ibt r5,#$01
    iwt r15,#.lD33B
    nop
.lD393:
    iwt r6,#$0020
    ibt r5,#$00
    iwt r15,#.lD33B
    nop
.lD39C:
    ibt r3,#$FC
    cmp r3
    bne .lD3B4
    nop
    iwt r0,#$0004
    moves r5,r5
    bne .lD3AE
    nop
    iwt r0,#$0008
.lD3AE:
    with r4
    add r0
    iwt r15,#.lD33B
    nop
.lD3B4:
    ibt r3,#$F5
    cmp r3
    bne .lD3C7
    nop
    move r4,r7
    move r0,r6
    div2
    with r2
    add r0
    iwt r15,#.lD33B
    nop
.lD3C7:
    ibt r3,#$FB
    cmp r3
    bne .lD3E3
    nop
    iwt r1,#$3700
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap120x32
    nop
    iwt r15,#.lD33B
    nop
.lD3E3:
    ibt r3,#$FA
    cmp r3
    bne .lD3FF
    nop
    iwt r1,#$3B00
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap120x16
    nop
    iwt r15,#.lD33B
    nop
.lD3FF:
    ibt r3,#$F9
    cmp r3
    bne .lD41B
    nop
    iwt r1,#$1810
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap64x16
    nop
    iwt r15,#.lD33B
    nop
.lD41B:
    ibt r3,#$F8
    cmp r3
    bne .lD437
    nop
    iwt r1,#$3700
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap120x24
    nop
    iwt r15,#.lD33B
    nop
.lD437:
    ibt r3,#$F7
    cmp r3
    bne .lD453
    nop
    iwt r1,#$3A00
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap120x24
    nop
    iwt r15,#.lD33B
    nop
.lD453:
    ibt r3,#$F6
    cmp r3
    bne .lD46F
    nop
    iwt r1,#$3D1C
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap16x24
    nop
    iwt r15,#.lD33B
    nop
.lD46F:
    ibt r3,#$0D
    cmp r3
    bne .lD47E
    nop
    move r4,r7
    with r2
    add r6
    iwt r15,#.lD33B
    nop
.lD47E:
    ibt r3,#$30
    sub r3
    bpl .lD488
    nop
    iwt r15,#.lD4CC
    nop
.lD488:
    ibt r3,#$01
    from r3
    romb
    add r0
    moves r5,r5
    bne .lD4B1
    nop
    iwt r14,#$F4D2
    to r14
    add r14
    to r1
    getb
    inc r14
    with r1
    getbh
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap16x32
    nop
    with r2
    sub r6
    bra .lD4CC
    nop
.lD4B1:
    iwt r14,#$F52E
    to r14
    add r14
    to r1
    getb
    inc r14
    with r1
    getbh
    ibt r0,#$15
    romb
    iwt r14,#$8000
    with r14
    add r1
    link #4
    iwt r15,#FxDrawBitmap8x16
    nop
    with r2
    sub r6
.lD4CC:
    iwt r0,#$000A
    moves r5,r5
    bne .lD4D7
    nop
    iwt r0,#$0014
.lD4D7:
    with r4
    add r0
    iwt r15,#.lD33B
    nop
.lD4DD:
    rpix
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop

arch 65816
