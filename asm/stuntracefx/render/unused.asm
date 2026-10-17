; unused.asm: Super FX code at 01:AD68-B0D8 that nothing in the game
; calls or starts: three gauges and the rectangle and run plotters they
; use, four screen-clearing programs and two sprite programs.
; C: src/stuntracefx/render/unused.c.

incsrc "../include/macros.inc"
incsrc "../include/wram.inc"

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
