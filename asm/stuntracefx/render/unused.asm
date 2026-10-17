; unused.asm: Super FX code at 01:AD68-B0D8, 01:8450-8560 and
; 01:B42F-B6D4 that nothing in the game calls or starts: three gauges and
; the rectangle and run plotters they use, four screen-clearing programs,
; two sprite programs, a byte rotation matrix, a pixel test, two
; model-drawing programs, a 3D text drawer and a row-copy program.
; C: src/stuntracefx/render/unused.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"
incsrc "../include/rom.inc"

arch superfx

;
; @routine 01:AD68 FxGaugeRight
; WHAT IT DOES: Draws a gauge at the screen's lower right: a frame 40
; by 8 at ($B0, $B0) in colour $0D (FxRectFrame), then a bar inside it
; at ($B2, $B2), 4 rows of the byte $01CE's pixels (at most $24) in
; colour 7 (FxRectFill). Nothing in the game calls it.
; HOW: Pushes R11 on the R10 stack around the two calls; the length
; capped with CMP / BGE; returns by pulling R15.
; Entry: R10 = stack; R11 = return address; $01CE = the bar's length.
; Exit: as FxRectFill leaves them; CMODE 9.
%at($01AD68)
FxGaugeRight:
    from r11
    stw (r10)
    inc r10
    inc r10
    iwt r1,#$00B0
    iwt r2,#$00B0
    ibt r3,#$28
    ibt r4,#$08
    ibt r0,#$0D
    link #4
    iwt r15,#FxRectFrame
    nop
    lms r3,(!fx_gauge_right-!gsu_ram)
    with r3
    lob
    ibt r0,#$24
    cmp r3
    bge .lAD8B
    nop
    move r3,r0
.lAD8B:
    iwt r1,#$00B2
    iwt r2,#$00B2
    ibt r4,#$04
    ibt r0,#$07
    link #4
    iwt r15,#FxRectFill
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:AD9F FxGaugeLeft
; WHAT IT DOES: FxGaugeRight's twin at the lower left: the frame at
; (8, $B0), the bar at ($0A, $B2) from the byte $01CA in colour 4.
; Nothing in the game calls it.
; HOW: As FxGaugeRight.
; Entry: R10 = stack; R11 = return address; $01CA = the bar's length.
; Exit: as FxRectFill leaves them; CMODE 9.
%at($01AD9F)
FxGaugeLeft:
    from r11
    stw (r10)
    inc r10
    inc r10
    ibt r1,#$08
    iwt r2,#$00B0
    ibt r3,#$28
    ibt r4,#$08
    ibt r0,#$0D
    link #4
    iwt r15,#FxRectFrame
    nop
    lms r3,(!fx_gauge_left-!gsu_ram)
    with r3
    lob
    ibt r0,#$24
    cmp r3
    bge .lADC1
    nop
    move r3,r0
.lADC1:
    ibt r1,#$0A
    iwt r2,#$00B2
    ibt r4,#$04
    ibt r0,#$04
    link #4
    iwt r15,#FxRectFill
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:ADD4 FxGaugeTop
; WHAT IT DOES: A gauge at the top right whose width is its value:
; with the byte $01AC not 0, a frame 6 rows high and 4 + $01AC pixels
; wide (4 + half when bit 7 is set) ending at x = $CE, y = 2, in
; colour $0E, and inside it a bar 2 rows high of the byte $01AE's
; pixels (halved likewise) in colour 2. $01AE is cleared first when it
; is at least $01AC + 10 (low bytes), and always at the end. Nothing
; in the game calls it.
; HOW: Pushes R11 on the R10 stack; R5/R6 keep the frame's corner over
; FxRectFrame; returns by pulling R15.
; Entry: R10 = stack; R11 = return address; $01AC, $01AE.
; Exit: $01AE = 0; R0 = 0; the rest as the last call leaves them.
%at($01ADD4)
FxGaugeTop:
    from r11
    stw (r10)
    inc r10
    inc r10
    lms r0,(!fx_gauge_top-!gsu_ram)
    add #10
    to r1
    lob
    lms r0,(!fx_gauge_top_inner-!gsu_ram)
    lob
    cmp r1
    bcc .lADED
    nop
    ibt r0,#$00
    sms (!fx_gauge_top_inner-!gsu_ram),r0
.lADED:
    lms r0,(!fx_gauge_top-!gsu_ram)
    to r3
    lob
    beq .lAE31
    nop
    bpl .lADFA
    nop
    with r3
    lsr
.lADFA:
    inc r3
    inc r3
    inc r3
    inc r3
    ibt r4,#$06
    iwt r0,#$00CE
    to r1
    sub r3
    ibt r2,#$02
    move r5,r1
    move r6,r2
    ibt r0,#$0E
    link #4
    iwt r15,#FxRectFrame
    nop
    move r1,r5
    move r2,r6
    inc r1
    inc r1
    inc r2
    inc r2
    lms r0,(!fx_gauge_top_inner-!gsu_ram)
    to r3
    lob
    lms r0,(!fx_gauge_top-!gsu_ram)
    lob
    bpl .lAE28
    nop
    with r3
    lsr
.lAE28:
    ibt r4,#$02
    ibt r0,#$02
    link #4
    iwt r15,#FxRectFill
    nop
.lAE31:
    sub r0
    sms (!fx_gauge_top_inner-!gsu_ram),r0
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
;
; @routine 01:AE3A FxRectFill
; WHAT IT DOES: Fills a rectangle: R4 rows of R3 pixels in colour R0
; from (R1, R2) down; nothing when R3 or R4 is 0.
; HOW: CMODE 0 for the COLOR, then CMODE 9; per row a LOOP of PLOT
; (R12 = R3, R13 = $AE55), R1 back by R3 and R2 one down; returns by
; JMP R11.
; Entry: R0 = colour, R1, R2 = top left, R3 = width, R4 = height; R11
; = return address.
; Exit: R1 back at the left, R2 below the last row, R4 = 0 (unless R3
; was 0), R12 = 0; CMODE 9.
%at($01AE3A)
FxRectFill:
    move r12,r0
    sub r0
    cmode
    move r0,r12
    color
    ibt r0,#$09
    cmode
    moves r3,r3
    beq .lAE5D
    nop
    moves r4,r4
    beq .lAE5D
    nop
    cache
.lAE51:
    moves r12,r3
    move r13,r15
    loop
    plot
    with r1
    sub r3
    dec r4
    bne .lAE51
    inc r2
.lAE5D:
    jmp r11
    nop
;
; @routine 01:AE5F FxRectFrame
; WHAT IT DOES: Draws a rectangle's outline: R3 pixels across from
; (R1, R2), the sides R4 - 2 rows down, the bottom row, in colour R0.
; HOW: CMODE 0 for the COLOR, then CMODE 9; three LOOPs of PLOT (top,
; both sides a row at a time, bottom); returns by JMP R11.
; Entry: R0 = colour, R1, R2 = top left, R3 = width, R4 = height (at
; least 3); R11 = return address.
; Exit: R1 one past the bottom row, R2 on it, R12 = 0; CMODE 9.
%at($01AE5F)
FxRectFrame:
    cache
    move r12,r0
    sub r0
    cmode
    move r0,r12
    color
    ibt r0,#$09
    cmode
    move r12,r3
    move r13,r15
    loop
    plot
    move r12,r4
    dec r12
    dec r12
    move r13,r15
    inc r2
    with r1
    sub r3
    plot
    with r1
    add r3
    dec r1
    dec r1
    loop
    plot
    with r1
    sub r3
    inc r2
    move r12,r3
    move r13,r15
    loop
    plot
    jmp r11
    nop
;
; @routine 01:AE8D FxPixelRun
; WHAT IT DOES: Plots R3 pixels in colour R0 from (R1, R2) across;
; nothing when R3 is 0.
; HOW: COLOR in the branch's delay slot; a LOOP of PLOT; returns by
; JMP R11.
; Entry: R0 = colour, R1, R2 = start, R3 = count; R11 = return
; address.
; Exit: R1 past the run, R12 = 0.
%at($01AE8D)
FxPixelRun:
    cache
    moves r12,r3
    beq .lAE97
    color
    move r13,r15
    loop
    plot
.lAE97:
    jmp r11
    nop
;
; @routine 01:AE99 FxClearScreenLong
; WHAT IT DOES: A program: clears $2000 words of Super FX RAM from
; $2C00 (to $6C00), then STOP.
; HOW: A LOOP of STW, INC R1 twice (R12 = $2000).
; Entry: run as a program (nothing in the game starts it).
; Exit: (at the STOP) R0 = 0, R1 = $6C00, R12 = 0.
%at($01AE99)
FxClearScreenLong:
    sub r0
    iwt r1,#$2C00
    iwt r12,#$2000
    cache
    move r13,r15
    stw (r1)
    inc r1
    loop
    inc r1
    stop
    nop
;
; @routine 01:AEA9 FxClearScreenShort
; WHAT IT DOES: A program: clears $1800 words from $2C00 (to $5C00),
; then STOP.
; HOW: As FxClearScreenLong, R12 = $1800.
; Entry: run as a program (nothing in the game starts it).
; Exit: (at the STOP) R0 = 0, R1 = $5C00, R12 = 0.
%at($01AEA9)
FxClearScreenShort:
    sub r0
    iwt r1,#$2C00
    iwt r12,#$1800
    cache
    move r13,r15
    stw (r1)
    inc r1
    loop
    inc r1
    stop
    nop
;
; @routine 01:AECA FxClearScreenIfSet
; WHAT IT DOES: A program: when the byte $01C4 is not 0, clears $1A00
; words from $2C00 (to $6000); then STOP.
; HOW: LOB of $01C4; a LOOP of STW (R12 = $1A00).
; Entry: run as a program (nothing in the game starts it); $01C4.
; Exit: (at the STOP) R0 = 0; R1 = $6000 and R12 = 0 when it cleared.
%at($01AECA)
FxClearScreenIfSet:
    lms r0,(!gsu_word_1c4-!gsu_ram)
    lob
    beq .lAEDF
    nop
    sub r0
    iwt r1,#$2C00
    iwt r12,#$1A00
    cache
    move r13,r15
    stw (r1)
    inc r1
    loop
    inc r1
.lAEDF:
    stop
    nop
;
; @routine 01:AEE1 FxClear6000IfSet
; WHAT IT DOES: A program: when the byte $01C4 is not 0, clears $1A00
; words from $6000 (to $9400); then STOP.
; HOW: As FxClearScreenIfSet from $6000.
; Entry: run as a program (nothing in the game starts it); $01C4.
; Exit: (at the STOP) R0 = 0; R1 = $9400 and R12 = 0 when it cleared.
%at($01AEE1)
FxClear6000IfSet:
    lms r0,(!gsu_word_1c4-!gsu_ram)
    lob
    beq .lAEF6
    nop
    sub r0
    iwt r1,#$6000
    iwt r12,#$1A00
    cache
    move r13,r15
    stw (r1)
    inc r1
    loop
    inc r1
.lAEF6:
    stop
    nop
;
; @routine 01:B027 FxSprite32Program
; WHAT IT DOES: A program: draws a 32 by 32 sprite centred on ($01D2,
; $01D4): texture $00AA's address from the table at $03:9D59 (three
; bytes each), its texels plotted with GETC row by row, colour $00AC
; times 16 (CMODE 8, or $0C when bit 5 of $00AA is set); then STOP.
; HOW: ROMB 3 for the table, then the texture's bank; rows of 32 by
; LOOP (R13 = $B079), R14 past the row's $E0 others; R11 counts the
; rows; RPIX writes the pixel caches out before the STOP.
; Entry: run as a program (nothing in the game starts it); $00AA, $00AC,
; $01D2, $01D4.
; Exit: (at the STOP) as the drawing leaves them.
%at($01B027)
FxSprite32Program:
    ibt r0,#$03
    romb
    sub r0
    cmode
    ibt r1,#$10
    lms r0,(!fx_sprite_colour-!gsu_ram)
    mult r1
    color
    iwt r2,#$0020
    lms r1,(!fx_sprite_size-!gsu_ram)
    from r1
    lob
    and r2
    beq .lB043
    nop
    ibt r0,#$04
.lB043:
    or #8
    cmode
    from r1
    umult #3
    iwt r1,#$9D59
    to r14
    add r1
    getb
    inc r14
    getbh
    inc r14
    to r1
    getb
    from r1
    romb
    move r14,r0
    ibt r0,#$F0
    lms r1,(!fx_sprite_x-!gsu_ram)
    to r1
    add r1
    lms r2,(!fx_sprite_y-!gsu_ram)
    to r2
    add r2
    move r9,r1
    iwt r11,#$0020
    iwt r10,#$00E0
    iwt r7,#$B079
    ibt r8,#$20
    cache
.lB075:
    move r12,r8
    move r13,r15
    getc
    inc r14
    loop
    plot
    nop
    with r14
    add r10
    move r1,r9
    dec r11
    bne .lB075
    inc r2
    rpix
    stop
    nop
;
; @routine 01:B08A FxObjectSpriteProgram
; WHAT IT DOES: A program: draws texture $00AA as a scaled object
; sprite at ($01D2, $01D4) (into $00BA, $00BC) with size $00CE: colour
; and CMODE as FxSprite32Program, the texture's address and bank into
; $00A8 and $00A6, then FxObjectSpriteAt; then STOP.
; HOW: R10 = $0240; ROMB 3; LINK to 01:B17D; RPIX before the STOP.
; Entry: run as a program (nothing in the game starts it); $00AA, $00AC,
; $00CE, $01D2, $01D4 and
; the object's set-up FxObjectSprite reads.
; Exit: (at the STOP) as FxObjectSpriteAt leaves them.
%at($01B08A)
FxObjectSpriteProgram:
    iwt r10,#$0240
    ibt r0,#$03
    romb
    sub r0
    cmode
    lms r0,(!fx_sprite_x-!gsu_ram)
    sms (!fx_model_ba-!gsu_ram),r0
    lms r0,(!fx_sprite_y-!gsu_ram)
    sms (!fx_model_bc-!gsu_ram),r0
    ibt r1,#$10
    lms r0,(!fx_sprite_colour-!gsu_ram)
    mult r1
    color
    iwt r2,#$0020
    lms r1,(!fx_sprite_size-!gsu_ram)
    from r1
    lob
    and r2
    beq .lB0B5
    nop
    ibt r0,#$04
.lB0B5:
    or #8
    cmode
    from r1
    umult #3
    iwt r1,#$9D59
    to r14
    add r1
    getb
    inc r14
    getbh
    inc r14
    sms (!fx_tex_base-!gsu_ram),r0
    getb
    sms (!fx_tex_bank-!gsu_ram),r0
    lms r1,(!fx_model_ce-!gsu_ram)
    link #4
    iwt r15,#FxObjectSpriteAt
    nop
    rpix
    stop
    nop

; @routine 01:8450 FxMatrixFromAnglesBytes
; WHAT IT DOES: Meant to build a rotation matrix of signed bytes at R9
; from the angle bytes in R1, R2 and R3 (the sines at $01:973E and the
; cosines at $01:96FE, a byte an angle): nine bytes, the rows as
; FxMatrixFromAngles01's. The first angle's two lookups do not use R1:
; IWT R1 overwrites it with the table address before it is added, so they
; read the bytes at twice the table addresses. Nothing in the game calls
; it.
; HOW: GETB from each table at its address + the angle; products with
; MULT (signed 8 by 8), doubled (ADD R0), the high byte (HIB) kept; each
; element stored with STB, R9 stepping by one.
; Entry: R2, R3 = the second and third angles; R9 = where to store; R11 =
; return address. Exit: R9 = R9 + 8; R0 = the last element; R1-R8, R12,
; R13 as the code leaves them; R14 and the ROM buffer move as on the Super
; FX.
%at($018450)
FxMatrixFromAnglesBytes:
    iwt r1,#$973E
    from r1
    to r14
    add r1
    to r4
    getb
    from r1
    to r14
    add r2
    to r6
    getb
    from r1
    to r14
    add r3
    to r8
    getb
    iwt r1,#$96FE
    from r1
    to r14
    add r1
    to r5
    getb
    from r1
    to r14
    add r2
    to r7
    getb
    from r1
    to r14
    add r3
    to r3
    getb
    from r8
    mult r7
    to r1
    add r0
    from r8
    mult r6
    to r2
    add r0
    from r3
    mult r7
    to r12
    add r0
    from r3
    mult r6
    to r13
    add r0
    from r12
    hib
    mult r5
    add r0
    add r2
    hib
    stb (r9)
    inc r9
    from r1
    hib
    mult r5
    add r0
    sub r13
    hib
    stb (r9)
    inc r9
    from r7
    mult r4
    add r0
    hib
    stb (r9)
    inc r9
    from r4
    mult r3
    add r0
    hib
    stb (r9)
    inc r9
    from r4
    mult r8
    add r0
    hib
    stb (r9)
    inc r9
    from r5
    not
    inc r0
    stb (r9)
    inc r9
    from r13
    hib
    mult r5
    add r0
    sub r1
    hib
    stb (r9)
    inc r9
    from r2
    hib
    mult r5
    add r0
    add r12
    hib
    stb (r9)
    inc r9
    from r6
    mult r4
    add r0
    hib
    stb (r9)
    jmp r11
    nop
; @routine 01:84CB FxPlotTestProgram
; WHAT IT DOES: A program: plots 32 pixels in colour $FF along the row at
; Y $40 from X 0, then STOP.
; HOW: COLOR, then a LOOP of PLOT (R12 = $20, R13 the loop).
; Entry: run as a program (nothing in the game starts it). Exit: (at the
; STOP) R1 = $20, R2 = $40, R12 = 0.
%at($0184CB)
FxPlotTestProgram:
    ibt r0,#$FF
    color
    ibt r1,#$00
    ibt r2,#$40
    ibt r12,#$20
    move r13,r15
    loop
    plot
    stop
    nop
; @routine 01:84DA FxModelMirrorProgram
; WHAT IT DOES: A program: draws the object as FxModelDrawFlagged does,
; then once more with its three angles ($0020-$0024) negated in place
; (each word's bytes swapped, then the two's complement) and the model
; flags $8000 (FxModelDrawFlags8000), with $0028 kept on the stack across
; the first draw and put back; then STOP. On the way it stores $0028 +
; $80 and clears it again into the last RAM address (SBK of 0).
; HOW: The RAM and ROM banks 0, the stack R10 at $0240; two LINK calls.
; Entry: run as a program (nothing in the game starts it); the object's
; set-up in RAM. Exit: (at the STOP) as the second draw leaves the
; registers.
%at($0184DA)
FxModelMirrorProgram:
    sub r0
    ramb
    romb
    iwt r10,#$0240
    lms r0,(!gsu_out_28-!gsu_ram)
    stw (r10)
    inc r10
    inc r10
    iwt r1,#$0080
    add r1
    sub r0
    sbk
    link #4
    iwt r15,#FxModelDrawFlagged
    nop
    lms r0,(!gsu_in_20-!gsu_ram)
    swap
    not
    inc r0
    sbk
    lms r0,(!gsu_in_22-!gsu_ram)
    swap
    not
    inc r0
    sbk
    lms r0,(!gsu_in_24-!gsu_ram)
    swap
    not
    inc r0
    sbk
    dec r10
    dec r10
    ldw (r10)
    sms (!gsu_out_28-!gsu_ram),r0
    link #4
    iwt r15,#FxModelDrawFlags8000
    nop
    stop
    nop
; @routine 01:8515 FxModelRecordProgram
; WHAT IT DOES: A program: takes an object's set-up from its record (the
; address at $001A, in ROM bank 0): +$12 (word) to $004A, +$07 (byte) to
; $0032, +$00 (word) to $0016, +$02 (byte) to $001C and +$03 (word) to
; $0018; then draws it with model flags $8000 (FxModelDrawWithFlags);
; then STOP.
; HOW: The RAM and ROM banks 0, the stack R10 at $0240; GETB, GETBH from
; R14 = the record + the field.
; Entry: run as a program (nothing in the game starts it); $001A = the
; record. Exit: (at the STOP) as the draw leaves the registers.
%at($018515)
FxModelRecordProgram:
    sub r0
    ramb
    romb
    iwt r10,#$0240
    lm r4,(!gsu_rec_mode-!gsu_ram)
    ibt r14,#$12
    from r4
    to r14
    add r14
    getb
    inc r14
    getbh
    sms (!gsu_speeds-!gsu_ram),r0
    ibt r14,#$07
    from r4
    to r14
    add r14
    getb
    sms (!gsu_rec_vel_z-!gsu_ram),r0
    ibt r14,#$00
    from r4
    to r14
    add r14
    getb
    inc r14
    getbh
    sms (!gsu_rec_flags-!gsu_ram),r0
    ibt r14,#$02
    from r4
    to r14
    add r14
    getb
    sms (!gsu_rec_1c-!gsu_ram),r0
    ibt r14,#$03
    from r4
    to r14
    add r14
    getb
    inc r14
    getbh
    sms (!gsu_rec_18-!gsu_ram),r0
    iwt r0,#$8000
    link #4
    iwt r15,#FxModelDrawWithFlags
    nop
    stop
    nop


; @routine 01:B42F FxText3D
; WHAT IT DOES: Draws the zero-terminated string at $001A (ROM bank
; $001C) at the 3D point ($0026, $0028, $002A) in colour R0: the point
; is projected (FxProjectPoint) and kept at $0026/$0028, the string's
; width (16 a character) and the scale from $00C8 projected again, and
; each character drawn from the 16 by 16 font at $14:D2BA, scaled
; (FxDivide32By16Halving) and clipped to the screen ($00B2/$00B4, rows
; up to $BF, columns up to $DF). Nothing is drawn when the point is at
; depth $80 or nearer. Nothing in the game calls it.
; HOW: R11 kept on the R10 stack; RAM bank 0, CMODE 0; two drawing loops
; (a character narrower or wider than its scaled size) of GETB/GETBH rows,
; ROL out of each bit, LOOP and PLOT; returns by pulling R15.
; Entry: R0 = colour; R10 = stack; R11 = return address; $001A/$001C =
; the string; $0026-$002A = the point; $00C8 = the size. Exit: as the
; code leaves the registers, flags and RAM; R14 and the ROM buffer move
; as on the Super FX.
%at($01B42F)
FxText3D:
    from r11
    stw (r10)
    inc r10
    inc r10
    color
    sub r0
    ramb
    cmode
    iwt r0,#$0080
    lms r1,(!gsu_out_26-!gsu_ram)
    lms r2,(!gsu_out_28-!gsu_ram)
    lms r3,(!gsu_out_2a-!gsu_ram)
    cmp r3
    bmi .lB44F
    nop
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB44F:
    link #4
    iwt r15,#FxProjectPoint
    nop
    sms (!gsu_out_26-!gsu_ram),r1
    sms (!gsu_out_28-!gsu_ram),r2
    lms r0,(!gsu_rec_1c-!gsu_ram)
    romb
    cache
    lms r14,(!gsu_rec_mode-!gsu_ram)
    ibt r12,#$FF
.lB465:
    getb
    inc r14
    lob
    bne .lB465
    inc r12
    ibt r3,#$10
    from r12
    mult r3
    stw (r10)
    inc r10
    inc r10
    ibt r2,#$7F
    lms r1,(!fx_model_c8-!gsu_ram)
    with r1
    add r2
    from r1
    to r2
    umult r12
    lms r3,(!gsu_out_2a-!gsu_ram)
    link #4
    iwt r15,#FxProjectPoint
    nop
    iwt r0,#$0065
    with r1
    sub r0
    iwt r0,#$0033
    with r2
    sub r0
    with r1
    lob
    sms (!fx_model_c8-!gsu_ram),r1
    ibt r3,#$FF
    add #0
    from r2
    ror
    sbc r3
    lms r3,(!gsu_out_26-!gsu_ram)
    from r3
    sub r0
    sbk
    ibt r3,#$FF
    add #0
    from r1
    ror
    sbc r3
    lms r3,(!gsu_out_28-!gsu_ram)
    from r3
    sub r0
    sbk
    from r1
    to r3
    not
    inc r3
    sms (!fx_sprite_clip_x-!gsu_ram),r3
    iwt r3,#$00E0
    with r3
    sub r1
    sms (!fx_sprite_clip_w-!gsu_ram),r3
    dec r10
    dec r10
    to r6
    ldw (r10)
    from r2
    sub r6
    bcc .lB4CC
    nop
    iwt r15,#.lB57D
    nop
.lB4CC:
    move r0,r2
    ibt r4,#$00
    link #4
    iwt r15,#FxDivide32By16Halving
    nop
    move r9,r4
    move r8,r4
    ibt r3,#$00
    sms (!fx_sprite_clip_x-!gsu_ram),r3
    cache
    lms r2,(!gsu_out_28-!gsu_ram)
    ibt r6,#$00
    ibt r4,#$00
.lB4E6:
    moves r2,r2
    bpl .lB4EF
    nop
    iwt r15,#.lB55A
    nop
.lB4EF:
    iwt r0,#$00BF
    cmp r2
    bpl .lB4FB
    nop
    iwt r15,#.lB55A
    nop
.lB4FB:
    lms r1,(!gsu_out_26-!gsu_ram)
    ibt r7,#$00
    lms r5,(!gsu_rec_mode-!gsu_ram)
.lB503:
    lms r0,(!gsu_rec_1c-!gsu_ram)
    romb
    move r14,r5
    getb
    inc r5
    add r0
    beq .lB55A
    nop
    ibt r3,#$14
    from r3
    romb
    ibt r3,#$10
    umult r3
    iwt r3,#$D2BA
    add r4
    add r4
    to r14
    add r3
    lms r0,(!fx_sprite_clip_x-!gsu_ram)
    cmp r1
    bmi .lB530
    nop
    lms r0,(!fx_model_c8-!gsu_ram)
    to r1
    add r1
    bra .lB503
    nop
.lB530:
    lms r3,(!fx_sprite_clip_w-!gsu_ram)
    dec r3
    dec r3
    with r3
    cmp r1
    bmi .lB55A
    nop
    getb
    inc r14
    getbh
    ibt r12,#$10
    move r13,r15
    with r7
    sub r9
    bcc .lB54D
    rol
    loop
    nop
    bra .lB503
    nop
.lB54D:
    bcs .lB555
    nop
    loop
    inc r1
    bra .lB503
    nop
.lB555:
    loop
    plot
    bra .lB503
    nop
.lB55A:
    with r6
    sub r8
    bcc .lB56A
    nop
    inc r4
    ibt r3,#$10
    from r4
    sub r3
    bne .lB55A
    nop
    bra .lB578
    nop
.lB56A:
    inc r2
    inc r4
    ibt r3,#$10
    from r4
    sub r3
    beq .lB577
    nop
    iwt r15,#.lB4E6
    nop
.lB577:
    nop
.lB578:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
.lB57D:
    move r0,r6
    move r6,r2
    cmp r6
    bne .lB58C
    nop
    iwt r4,#$FFFF
    bra .lB593
    nop
.lB58C:
    ibt r4,#$00
    link #4
    iwt r15,#FxDivide32By16Halving
    nop
.lB593:
    move r9,r4
    move r8,r4
    cache
    lms r2,(!gsu_out_28-!gsu_ram)
    ibt r6,#$00
    ibt r4,#$00
.lB59F:
    moves r2,r2
    bpl .lB5A8
    nop
    iwt r15,#.lB67E
    nop
.lB5A8:
    iwt r0,#$00BF
    cmp r2
    bpl .lB5B4
    nop
    iwt r15,#.lB67E
    nop
.lB5B4:
    lms r1,(!gsu_out_26-!gsu_ram)
    ibt r7,#$00
    lms r5,(!gsu_rec_mode-!gsu_ram)
.lB5BC:
    lms r0,(!gsu_rec_1c-!gsu_ram)
    romb
    move r14,r5
    getb
    inc r5
    add r0
    bne .lB5CD
    nop
    iwt r15,#.lB67E
    nop
.lB5CD:
    ibt r3,#$14
    from r3
    romb
    ibt r3,#$10
    umult r3
    iwt r3,#$D2BA
    add r4
    add r4
    to r14
    add r3
    lms r0,(!fx_sprite_clip_x-!gsu_ram)
    cmp r1
    bmi .lB5E9
    nop
    iwt r15,#.lB66E
    nop
.lB5E9:
    moves r1,r1
    bpl .lB618
    getb
    inc r14
    getbh
    ibt r12,#$10
    move r13,r15
    rol
    bcs .lB603
    nop
.lB5F9:
    with r7
    sub r9
    bcs .lB5F9
    inc r1
    loop
    nop
    bra .lB5BC
    nop
.lB603:
    with r7
    sub r9
    bcs .lB603
    inc r1
    bmi .lB613
    nop
    iwt r13,#$B629
    loop
    nop
    bra .lB5BC
    nop
.lB613:
    loop
    nop
    bra .lB5BC
    nop
.lB618:
    inc r14
    lms r3,(!fx_sprite_clip_w-!gsu_ram)
    dec r3
    with r3
    cmp r1
    bmi .lB642
    nop
    getbh
    ibt r12,#$10
    move r13,r15
    rol
    bcs .lB637
    nop
.lB62D:
    with r7
    sub r9
    bcs .lB62D
    inc r1
    loop
    nop
    bra .lB5BC
    nop
.lB637:
    with r7
    sub r9
    bcs .lB637
    plot
    loop
    nop
    iwt r15,#.lB5BC
    nop
.lB642:
    getbh
    iwt r3,#$00DF
    ibt r12,#$10
    move r13,r15
    rol
    bcs .lB65F
    nop
.lB64F:
    with r7
    sub r9
    bcs .lB64F
    inc r1
    with r1
    cmp r3
    bpl .lB67E
    nop
    loop
    nop
    bra .lB67E
    nop
.lB65F:
    with r1
    cmp r3
    bpl .lB67E
    with r7
    sub r9
    bcs .lB65F
    plot
    loop
    nop
    bra .lB67E
    nop
.lB66E:
    lms r3,(!fx_model_c8-!gsu_ram)
    with r1
    add r3
    with r7
    sub r9
    bcc .lB679
    nop
    inc r1
.lB679:
    iwt r15,#.lB5BC
    nop
    nop
.lB67E:
    nop
    inc r2
    with r6
    sub r8
    bcc .lB689
    nop
    iwt r15,#.lB59F
    nop
.lB689:
    inc r4
    ibt r3,#$10
    from r4
    sub r3
    beq .lB695
    nop
    iwt r15,#.lB59F
    nop
.lB695:
    dec r10
    dec r10
    to r15
    ldw (r10)
    nop
; @routine 01:B69A FxRowsCopyProgram
; WHAT IT DOES: A program: copies four rows of $50 words from the ROM at
; $0018 (bank 0) into the screen in Super FX RAM at $2C00 + $0026 * $400
; + $0028 * 4, a row every $400 bytes; then STOP. Nothing in the game
; starts it.
; HOW: RAM and ROM banks 0; GETB, GETBH and STW in a LOOP (R12 = $50),
; the destination stepping two bytes a word.
; Entry: run as a program; $0018 = the source, $0026/$0028 = the place.
; Exit: (at the STOP) R3 = 0, R12 = 0; R14 past the last word read.
%at($01B69A)
FxRowsCopyProgram:
    sub r0
    ramb
    romb
    cache
    lms r4,(!gsu_out_26-!gsu_ram)
    iwt r0,#$0100
    umult r4
    add r0
    to r4
    add r0
    lms r0,(!gsu_out_28-!gsu_ram)
    umult #4
    add r4
    iwt r4,#$2C00
    to r4
    add r4
    iwt r5,#$0400
    lms r14,(!gsu_rec_18-!gsu_ram)
    ibt r3,#$04
.lB6BE:
    move r1,r4
    ibt r12,#$50
    move r13,r15
    getb
    inc r14
    getbh
    inc r14
    stw (r1)
    inc r1
    loop
    inc r1
    with r4
    add r5
    dec r3
    bne .lB6BE
    nop
    stop
    nop

