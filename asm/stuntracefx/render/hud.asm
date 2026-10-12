; hud.asm: the Super FX programs in bank 07 that draw the 2D pieces of the
; screen (glyphs and digits, the rising picture, scaled sprites, the noise
; fill) into the Super FX's screen buffer. The S-CPU sets their inputs in
; Super FX RAM and starts each one (gsu/programs.asm); each ends at a STOP.
;
; Bitmaps are 2 bits a pixel in ROM bank $15, 32 bytes a row; the low two
; bits of a byte are its leftmost pixel and pick one of the four colour
; bytes at $70:0200. The instruction after a branch or LOOP (its delay
; slot) always runs.
; C: src/stuntracefx/render/hud.c.

incsrc "../include/macros.inc"
incsrc "../include/hw.inc"
incsrc "../include/wram.inc"

arch superfx

; @routine 07:EE50 FxTiles2bppTo4bppHigh
; WHAT IT DOES: Converts 96 2-bit tiles at $70:C000 into 4-bit tiles at
; $70:D000: each tile's 16 bytes (two bit planes, 8 rows) become 32, the
; upper two planes 0.
; HOW: Bytes are copied with a 0 after each (8 a tile, LDB and STB), then 16
; zero bytes; R4 counts the tiles down.
; Entry: started by the S-CPU. Exit: R0 = 0; R1 = 0; R2 = $C300; R3 = $DC00;
; R4 = 0; R12 = 0; R13 = $EE7E; RAM bank 0; the last RAM address $DBFF; CY
; set, OV clear, S, Z from the tile count (Z set).
%at($07EE50)
FxTiles2bppTo4bppHigh:
    iwt r2,#$C000
    iwt r3,#$D000
    bra FxTiles2bppTo4bpp
    nop

; @routine 07:EE59 FxTiles2bppTo4bppLow
; WHAT IT DOES: As FxTiles2bppTo4bppHigh, from $70:3000 into $70:4000.
; HOW: As FxTiles2bppTo4bppHigh, with source $3000 and destination $4000.
; Entry: started by the S-CPU. Exit: as FxTiles2bppTo4bppHigh, with R2 =
; $3300, R3 = $4C00 and the last RAM address $4BFF.
%at($07EE59)
FxTiles2bppTo4bppLow:
    iwt r2,#$3000
    iwt r3,#$4000
FxTiles2bppTo4bpp:
    cache
    sub r0
    ramb
    iwt r1,#$0000
    iwt r4,#$0060
.lEE69:
    iwt r12,#$0008
    move r13,r15
    ldb (r2)
    stb (r3)
    inc r2
    inc r3
    sub r0
    stb (r3)
    loop
    inc r3
    iwt r12,#$0010
    move r13,r15
    stb (r3)
    loop
    inc r3
    dec r4
    bne .lEE69
    nop
    stop
    nop

; @routine 07:EE88 FxNoiseFill
; WHAT IT DOES: Fills the 16 x 128 pixels at the screen's top left with
; random grey: each pixel's colour is one of the eight words at $70:3C00
; (GsuEE88Ramp writes $80, $90, ... $F0 there), picked by a random number
; seeded from and saved back to $70:021C.
; HOW: The generator (seed in R8): t = the seed with its low byte XORed into
; its high byte, its bytes swapped; the new seed is (twice the old low byte
; XOR t) >> 1, XOR $FF00; when the bit shifted out is set it is XORed with
; $8180, and when not, a value of $AA55 becomes 0. Its low three bits pick
; the colour. RPIX at the end writes the pixel caches out.
; Entry: started by the S-CPU. Exit: R0 = the pixel at (0, 128); R1 = 0; R2
; = 128; R3 = $FF00; R4 = $AA55; R5 = $8180; R6 = $3C00; R8 = the new seed;
; R9 = 16; R10 = 0; R12 = 0; R13 = $EEBB; plot options 0; the colour of the
; last pixel; the last RAM address $021C; CY set, OV clear, S, Z from the
; RPIX.
%at($07EE88)
FxNoiseFill:
    sub r0
    cmode
    iwt r9,#$0010
    iwt r10,#$0080
    bra FxNoiseFillBody
    nop

; @routine 07:EE94 FxNoiseFillScreen
; WHAT IT DOES: FxNoiseFill over the whole screen: $70:003A pixels across
; and $70:003E + 1 lines down.
; HOW: The width and height into R9 and R10, then FxNoiseFill's body. No
; script has started it yet.
; Entry: started by the S-CPU. Exit: as FxNoiseFill, with R9 = the width and
; R2 = the lines drawn.
%at($07EE94)
FxNoiseFillScreen:
    sub r0
    cmode
    lm r9,(!gsu_in_3a)
    lm r10,(!gsu_in_3e)
    inc r10
FxNoiseFillBody:
    lm r8,(!fx_noise_seed)
    iwt r3,#$FF00
    iwt r4,#$AA55
    iwt r5,#$8180
    iwt r6,#$3C00
    iwt r1,#$0000
    iwt r2,#$0000
    cache
.lEEB7:
    move r12,r9
    move r13,r15
    from r8
    lob
    swap
    xor r8
    swap
    move r8,r0
    swap
    lob
    add r0
    xor r8
    lsr
    xor r3
    bcc .lEED5
    nop
    xor r5
.lEED0:
    move r8,r0
    bra .lEEDF
    nop
.lEED5:
    cmp r4
    bne .lEED0
    nop
    iwt r0,#$0000
    move r8,r0
.lEEDF:
    and #7
    add r0
    add r6
    ldw (r0)
    color
    loop                            ; delay slot: the PLOT
    plot
    with r1
    sub r1
    dec r10
    bne .lEEB7
    inc r2
    rpix
    sm (!fx_noise_seed),r8
    stop
    nop

; @routine 07:EEF5 FxPictureRiseSplit40
; WHAT IT DOES: Draws the picture at $15:3D00 rising out of line $40, the
; top $70:020C rows of it, in two pieces: picture columns 56-111 at x = $1C
; and columns 0-39 at x = $54.
; HOW: Each piece: y = $40 - height; rows of 14 (then 10) bitmap bytes, 32
; bytes apart, each byte four pixels through the colour table at $70:0200;
; RPIX after each piece.
; Entry: started by the S-CPU; the colour bytes and the height set. Exit: R0
; = the pixel at ($54, $40); R1 = R8 = $54; R2 = $40; R3 = 0; R6 = the last
; byte rotated right 8 times through the carry; R11 = $0200; R12 = 0; R13 =
; $EF81; R14 past the last row; ROM bank $15, RAM bank 0, plot options 0;
; the last RAM address that of the last colour byte read.
%at($07EEF5)
FxPictureRiseSplit40:
    iwt r8,#$001C
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r3,(!fx_draw_y)
    iwt r0,#$0040
    to r2
    sub r3
    move r1,r8
    iwt r14,#$3D0E
    move r12,r3
    move r13,r15
    iwt r3,#$000E
    cache
.lEF1A:
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    dec r3
    bne .lEF1A
    nop
    iwt r0,#$0012
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    iwt r8,#$0054
    lm r3,(!fx_draw_y)
    move r1,r8
    iwt r14,#$3D00
    iwt r0,#$0040
    to r2
    sub r3
    move r12,r3
    move r13,r15
    iwt r3,#$000A
    cache
.lEF85:
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    dec r3
    bne .lEF85
    nop
    iwt r0,#$0016
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    stop
    nop

; @routine 07:EFCD FxPictureRiseSplit60
; WHAT IT DOES: As FxPictureRiseSplit40, rising out of line $60, the pieces
; at x = $34 and $6C.
; HOW: As FxPictureRiseSplit40.
; Entry: as FxPictureRiseSplit40. Exit: as FxPictureRiseSplit40, with R0 =
; the pixel at ($6C, $60), R1 = R8 = $6C, R2 = $60 and R13 = $F059.
%at($07EFCD)
FxPictureRiseSplit60:
    iwt r8,#$0034
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r3,(!fx_draw_y)
    iwt r0,#$0060
    to r2
    sub r3
    move r1,r8
    iwt r14,#$3D0E
    move r12,r3
    move r13,r15
    iwt r3,#$000E
    cache
.lEFF2:
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    dec r3
    bne .lEFF2
    nop
    iwt r0,#$0012
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    iwt r8,#$006C
    lm r3,(!fx_draw_y)
    move r1,r8
    iwt r14,#$3D00
    iwt r0,#$0060
    to r2
    sub r3
    move r12,r3
    move r13,r15
    iwt r3,#$000A
    cache
.lF05D:
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    dec r3
    bne .lF05D
    nop
    iwt r0,#$0016
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    stop
    nop

; @routine 07:F0A5 FxPictureRise40
; WHAT IT DOES: Draws the picture at $15:3D00 rising out of line $40, the
; top $70:020C rows of its first 56 columns, at x = $28.
; HOW: y = $40 - height; rows of 14 bitmap bytes, 32 bytes apart, each byte
; four pixels through the colour table at $70:0200; RPIX at the end.
; Entry: started by the S-CPU; the colour bytes and the height set. Exit: R0
; = the pixel at ($28, $40); R1 = R8 = $28; R2 = $40; R3 = 0; R6 = the last
; byte rotated right 8 times through the carry; R11 = $0200; R12 = 0; R13 =
; $F0D5; R14 past the last row; ROM bank $15, RAM bank 0, plot options 0;
; the last RAM address that of the last colour byte read.
%at($07F0A5)
FxPictureRise40:
    iwt r8,#$0028
    lm r3,(!fx_draw_y)
    iwt r0,#$0040
    to r2
    sub r3
    bra FxPictureRiseRows
    nop

; @routine 07:F0B4 FxPictureRise60
; WHAT IT DOES: As FxPictureRise40, rising out of line $60, at x = $4C.
; HOW: As FxPictureRise40.
; Entry: as FxPictureRise40. Exit: as FxPictureRise40, with R0 = the pixel
; at ($4C, $60), R1 = R8 = $4C and R2 = $60.
%at($07F0B4)
FxPictureRise60:
    iwt r8,#$004C
    lm r3,(!fx_draw_y)
    iwt r0,#$0060
    to r2
    sub r3
FxPictureRiseRows:
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    move r1,r8
    iwt r14,#$3D00
    move r12,r3
    move r13,r15
    iwt r3,#$000E
    cache
.lF0D9:
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    dec r3
    bne .lF0D9
    nop
    iwt r0,#$0012
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    stop
    nop

; @routine 07:F121 FxGlyph16RowsFromBottom
; WHAT IT DOES: Draws the bottom $70:0210 rows of the 16 x 32 glyph at
; $70:0216 (in bank $15) at ($70:020A, $70:020C), and moves x on by
; $70:020E; nothing is drawn when the glyph address is 0. With
; FxGlyph16RowsFromTop it rolls a digit into its cell.
; HOW: The glyph's start moves down 32 - rows rows; each row is four bitmap
; bytes through the colour table at $70:0200, with an RPIX after each byte.
; Entry: started by the S-CPU. Exit: when drawn, R0 = $1C; R1 = R8 = x; R2 =
; y + rows; R3 = rows; R6 = the last byte rotated right 8 times through the
; carry; R11 = $0200; R12 = 0; R13 = $F191; R14 past the last row; ROM bank
; $15, RAM bank 0, plot options 0; the last RAM address that of the last
; colour byte read. When not drawn: R0 = x + the advance; R1 = R8 = x; R14 =
; 0; the last RAM address $0216; OV, S clear, Z set.
%at($07F121)
FxGlyph16RowsFromBottom:
    cache
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    lm r0,(!fx_draw_advance)
    lm r8,(!fx_draw_x)
    move r1,r8
    add r8
    sbk
    lm r14,(!gsu_in_216)
    moves r14,r14
    bne .lF142
    nop
    stop
    nop
.lF142:
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r3,(!fx_draw_rows)
    iwt r0,#$0020
    sub r3
    add r0
    add r0
    add r0
    add r0
    add r0
    to r14
    add r14
    bra FxGlyph16Rows
    nop

; @routine 07:F15B FxGlyph16RowsFromTop
; WHAT IT DOES: Draws the top $70:0210 rows of the 16 x 32 glyph at $70:0216
; (in bank $15) at the bottom of the 32-line cell at ($70:020A, $70:020C),
; and moves x on by $70:020E; nothing is drawn when the glyph address is 0.
; With $0210 = 32 it draws the whole glyph (digits).
; HOW: y = cell y + 32 - rows; each row is four bitmap bytes through the
; colour table at $70:0200, with an RPIX after each byte.
; Entry: started by the S-CPU. Exit: when drawn, as FxGlyph16RowsFromBottom,
; with R2 = cell y + 32. When not drawn: R0 = x + the advance; R1 = R8 = x;
; R2 = cell y + 32 - rows; R3 = rows; R11 = $0200; R14 = 0; the last RAM
; address $0216; OV, S clear, Z set.
%at($07F15B)
FxGlyph16RowsFromTop:
    cache
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r3,(!fx_draw_rows)
    lm r2,(!fx_draw_y)
    iwt r0,#$0020
    add r2
    to r2
    sub r3
    lm r0,(!fx_draw_advance)
    lm r8,(!fx_draw_x)
    move r1,r8
    add r8
    sbk
    lm r14,(!gsu_in_216)
    moves r14,r14
    bne FxGlyph16Rows
    nop
    stop
    nop
FxGlyph16Rows:
    move r12,r3
    move r13,r15
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    iwt r0,#$001C
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    stop
    nop

; @routine 07:F280 FxGlyph8x8
; WHAT IT DOES: As FxGlyph8x16, 8 rows.
; HOW: As FxGlyph8x16. No script has started it yet.
; Entry: started by the S-CPU. Exit: as FxGlyph8x16, with R2 = y + 8 and R13
; = $F2AC.
%at($07F280)
FxGlyph8x8:
    cache
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r8,(!fx_draw_x)
    move r1,r8
    iwt r0,#$0008
    add r8
    sbk
    lm r14,(!gsu_in_216)
    moves r14,r14
    bne .lF2A7
    nop
    stop
    nop
.lF2A7:
    iwt r12,#$0008
    move r13,r15
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    iwt r0,#$001E
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    stop
    nop

; @routine 07:F329 FxGlyph8x16
; WHAT IT DOES: Draws the 8 x 16 glyph at $70:0216 (in bank $15) at
; ($70:020A, $70:020C) and moves x on by 8; nothing is drawn when the glyph
; address is 0.
; HOW: 16 rows of two bitmap bytes through the colour table at $70:0200,
; with an RPIX after each byte. The address-0 exit has not been reached by
; any script yet.
; Entry: started by the S-CPU. Exit: when drawn, R0 = $1E; R1 = R8 = x; R2 =
; y + 16; R6 = the last byte rotated right 8 times through the carry; R11 =
; $0200; R12 = 0; R13 = $F355; R14 past the last row; ROM bank $15, RAM bank
; 0, plot options 0; the last RAM address that of the last colour byte read.
%at($07F329)
FxGlyph8x16:
    cache
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r8,(!fx_draw_x)
    move r1,r8
    iwt r0,#$0008
    add r8
    sbk
    lm r14,(!gsu_in_216)
    moves r14,r14
    bne .lF350
    nop
    stop                            ; not reached in the traces so far
    nop                             ; not reached in the traces so far
.lF350:
    iwt r12,#$0010
    move r13,r15
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    iwt r0,#$001E
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    stop
    nop

; @routine 07:F3D2 FxGlyph24x32Bank04
; WHAT IT DOES: Draws the 24 x 32 glyph at $70:0216 in ROM bank $04 at
; ($70:020A, $70:020C) and moves x on by 16.
; HOW: 32 rows of six bitmap bytes (R3 counts them) through the colour table
; at $70:0200; one RPIX at the end. The glyph address is not checked. No
; script has started it yet.
; Entry: started by the S-CPU. Exit: R0 = the pixel at (x, y + 32); R1 = R8
; = x; R2 = y + 32; R3 = 0; R11 = $0200; R12 = 0; R13 = $F3F7; R14 past the
; last row; ROM bank $04, RAM bank 0, plot options 0.
%at($07F3D2)
FxGlyph24x32Bank04:
    ibt r0,#$04
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r8,(!fx_draw_x)
    move r1,r8
    iwt r0,#$0010
    add r8
    sbk
    lm r14,(!gsu_in_216)
    cache
    iwt r12,#$0020
    move r13,r15
    ibt r3,#$06
.lF3F9:
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    dec r3
    bne .lF3F9
    nop
    iwt r0,#$001A
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    stop
    nop

; @routine 07:F441 FxGlyph24x16
; WHAT IT DOES: Draws the 24 x 16 glyph at $70:0216 (in bank $15) at
; ($70:020A, $70:020C) and moves x on by 16 (the player marks).
; HOW: 16 rows of six bitmap bytes through the colour table at $70:0200,
; with an RPIX after each byte and one more at the end. The glyph address is
; not checked.
; Entry: started by the S-CPU. Exit: R0 = the pixel at (x, y + 16); R1 = R8
; = x; R2 = y + 16; R6 = the last byte rotated right 8 times through the
; carry; R11 = $0200; R12 = 0; R13 = $F466; R14 past the last row; ROM bank
; $15, RAM bank 0, plot options 0; the last RAM address that of the last
; colour byte read.
%at($07F441)
FxGlyph24x16:
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r8,(!fx_draw_x)
    move r1,r8
    iwt r0,#$0010
    add r8
    sbk
    lm r14,(!gsu_in_216)
    cache
    iwt r12,#$0010
    move r13,r15
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    iwt r0,#$001A
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    stop
    nop

; @routine 07:F5C9 FxGlyph16x16
; WHAT IT DOES: Draws the 16 x 16 glyph at $70:0216 (in bank $15) at
; ($70:020A, $70:020C) and moves x on by 16.
; HOW: 16 rows of four bitmap bytes through the colour table at $70:0200,
; with an RPIX after each byte and one more at the end. The glyph address is
; not checked. No script has started it yet.
; Entry: started by the S-CPU. Exit: as FxGlyph24x16, with R13 = $F5EE.
%at($07F5C9)
FxGlyph16x16:
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r8,(!fx_draw_x)
    move r1,r8
    iwt r0,#$0010
    add r8
    sbk
    lm r14,(!gsu_in_216)
    cache
    iwt r12,#$0010
    move r13,r15
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    rpix
    iwt r0,#$001C
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    stop
    nop

; @routine 07:F6DF FxGlyph16x32
; WHAT IT DOES: Draws the 16 x 32 glyph at $70:0216 (in bank $15) at
; ($70:020A, $70:020C) and moves x on by 16; nothing is drawn when the glyph
; address is 0.
; HOW: 32 rows of four bitmap bytes through the colour table at $70:0200;
; one RPIX at the end. The address-0 exit has not been reached by any script
; yet.
; Entry: started by the S-CPU. Exit: when drawn, R0 = the pixel at (x, y +
; 32); R1 = R8 = x; R2 = y + 32; R6 = the last byte rotated right 8 times
; through the carry; R11 = $0200; R12 = 0; R13 = $F70B; R14 past the last
; row; ROM bank $15, RAM bank 0, plot options 0; the last RAM address that
; of the last colour byte read.
%at($07F6DF)
FxGlyph16x32:
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r8,(!fx_draw_x)
    move r1,r8
    iwt r0,#$0010
    add r8
    sbk
    lm r14,(!gsu_in_216)
    moves r14,r14
    bne .lF705
    nop
    stop                            ; not reached in the traces so far
    nop                             ; not reached in the traces so far
.lF705:
    cache
    iwt r12,#$0020
    move r13,r15
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    iwt r0,#$001C
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    rpix
    stop
    nop

; @routine 07:F7F4 FxGlyph16x32Double
; WHAT IT DOES: Draws the 16 x 32 glyph at $70:0216 (in bank $15) at double
; size (32 x 64) at ($70:020A, $70:020C) and moves x on by 32; nothing is
; drawn when the glyph address is 0.
; HOW: Each of the 32 rows twice (R14 back 4 bytes between), each byte's
; pixels plotted twice, with an RPIX after each byte. No script has started
; it yet.
; Entry: started by the S-CPU. Exit: when drawn, R0 = $1C; R1 = R8 = x; R2 =
; y + 64; R11 = $0200; R12 = 0; R13 = $F820; R14 past the last row; ROM bank
; $15, RAM bank 0, plot options 0.
%at($07F7F4)
FxGlyph16x32Double:
    cache
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r2,(!fx_draw_y)
    lm r8,(!fx_draw_x)
    move r1,r8
    iwt r0,#$0020
    add r8
    sbk
    lm r14,(!gsu_in_216)
    moves r14,r14
    bne .lF81B
    nop
    stop
    nop
.lF81B:
    iwt r12,#$0020
    move r13,r15
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    iwt r0,#$FFFC
    to r14
    add r14
    move r1,r8
    inc r2
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    to r6
    getb
    move r0,r6
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    with r6
    ror
    with r6
    ror
    inc r14
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    move r0,r6
    with r6
    ror
    with r6
    ror
    and #3
    add r11
    ldb (r0)
    color
    plot
    plot
    rpix
    iwt r0,#$001C
    to r14
    add r14
    move r1,r8
    loop
    inc r2
    stop
    nop

; @routine 07:FA1B FxScaledSprite13
; WHAT IT DOES: Draws a scaled sprite at ($70:020A, $70:020C), (size across
; / 2) * 13 / 16 pixels wide and (size down) / 16 high, the sizes at
; $70:020E and $70:0210.
; HOW: Width by DIV2 and MULT #13 and four doublings, the high byte; height
; by four doublings, the high byte; then the shared scaled-sprite body (see
; scaled_sprite: clipped to the screen, the source stepped by $70:0212 and
; $70:0214, RPIX after each row and at the end).
; Entry: started by the S-CPU. Exit: R1 = R10 = the left edge; R2 = the line
; below the sprite; R3, R4 = the steps; R5 = the width; R7 = 0; R8, R9 = x,
; y; R11 = $0200; R12 = 0; R13 = $FB7D; R0 = the pixel at the left edge of
; that line; ROM bank $15, RAM bank 0, plot options 0. A sprite clipped away
; or with bitmap 0 stops early with the registers as they are then.
%at($07FA1B)
FxScaledSprite13:
    lm r3,(!fx_draw_advance)
    lm r4,(!fx_draw_rows)
    lm r8,(!fx_draw_x)
    from r3
    div2
    mult #13
    add r0
    add r0
    add r0
    add r0
    to r5
    hib
    bra FxScaledSpriteHeight16
    nop

; @routine 07:FA35 FxScaledSprite12
; WHAT IT DOES: As FxScaledSprite13, (size across / 2) * 12 / 16 pixels
; wide.
; HOW: As FxScaledSprite13, with MULT #12. No script has started it yet.
; Entry: started by the S-CPU. Exit: as FxScaledSprite13.
%at($07FA35)
FxScaledSprite12:
    lm r3,(!fx_draw_advance)
    lm r4,(!fx_draw_rows)
    lm r8,(!fx_draw_x)
    from r3
    div2
    mult #12
    add r0
    add r0
    add r0
    add r0
    to r5
    hib
FxScaledSpriteHeight16:
    lm r9,(!fx_draw_y)
    from r4
    add r4
    add r0
    add r0
    add r0
    to r7
    hib
    bra FxScaledSpriteSetup
    nop

; @routine 07:FA5A FxScaledSpriteEighth
; WHAT IT DOES: Draws a scaled sprite at ($70:020A, $70:020C), (size across)
; / 32 pixels wide and (size down) / 32 high, the sizes at $70:020E and
; $70:0210.
; HOW: Each size by three doublings, the high byte; then the shared
; scaled-sprite body (see FxScaledSprite13).
; Entry: started by the S-CPU. Exit: as FxScaledSprite13.
%at($07FA5A)
FxScaledSpriteEighth:
    lm r3,(!fx_draw_advance)
    lm r4,(!fx_draw_rows)
    lm r8,(!fx_draw_x)
    from r3
    add r3
    add r0
    add r0
    to r5
    hib
    from r5
    add r8
    lm r9,(!fx_draw_y)
    from r4
    add r4
    add r0
    add r0
    to r7
    hib
FxScaledSpriteSetup:
    cache
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    bra FxScaledSpriteJoin
    nop

; @routine 07:FA88 FxSprite24x40
; WHAT IT DOES: Draws a 24 x 40 sprite at ($70:020A, $70:020C), stepping
; through the source by $70:0212 and $70:0214, and moves x on by 24.
; HOW: Width $18 and height $28, then the shared scaled-sprite body (see
; FxScaledSprite13).
; Entry: started by the S-CPU. Exit: as FxScaledSprite13.
%at($07FA88)
FxSprite24x40:
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    iwt r3,#$0100
    iwt r4,#$0100
    lm r8,(!fx_draw_x)
    iwt r0,#$0018
    add r8
    sbk
    iwt r5,#$0018
    lm r9,(!fx_draw_y)
    iwt r7,#$0028
FxScaledSpriteJoin:
    bra FxScaledSprite
    nop

; @routine 07:FAB0 FxSprite16x32
; WHAT IT DOES: Draws the 16 x 32 sprite at $70:0216 at ($70:020A, $70:020C)
; unscaled (both steps at $70:0212 and $70:0214 set to $100) and moves x on
; by 16, when x is between -15 and $D0.
; HOW: The steps stored, then two unsigned compares of x; then the shared
; scaled-sprite body. When x is $D0 or more the branch away has an IWT in
; its delay slot, which takes its operand from the two bytes at the target
; (STOP, NOP: R0 = $0100) and runs on into FxScaledSpriteAdvance (x above
; $D0); the C does the same. No script has started it yet.
; Entry: started by the S-CPU. Exit: as FxScaledSprite13.
%at($07FAB0)
FxSprite16x32:
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r0,#$0100
    sm (!fx_draw_step_x),r0
    sm (!fx_draw_step_y),r0
    iwt r11,#$0200
    iwt r3,#$0100
    iwt r4,#$0100
    lm r8,(!fx_draw_x)
    iwt r0,#$FFF0
    sub r8
    bcc .lFADE                      ; x above $FFF0: draw
    nop
    iwt r0,#$00D0
    sub r8
    bcc .lFAF0                      ; taken: the delay slot IWT takes $0100 from the target bytes
.lFADE:
    iwt r0,#$0010
    add r8
    sbk
    iwt r5,#$0010
    lm r9,(!fx_draw_y)
    iwt r7,#$0020
    bra FxScaledSprite
    nop
.lFAF0:
    stop
    nop

; @routine 07:FAF2 FxScaledSpriteAdvance
; WHAT IT DOES: Draws a scaled sprite at ($70:020A, $70:020C), (size across)
; / 16 pixels wide and (size down) / 8 high, the sizes at $70:020E and
; $70:0210, and moves x on by its width.
; HOW: Width by four doublings and height by five, each the high byte; the
; width added to $020A; then the shared scaled-sprite body (see
; FxScaledSprite13).
; Entry: started by the S-CPU. Exit: as FxScaledSprite13.
%at($07FAF2)
FxScaledSpriteAdvance:
    ibt r0,#$15
    romb
    sub r0
    ramb
    cmode
    iwt r11,#$0200
    lm r3,(!fx_draw_advance)
    lm r4,(!fx_draw_rows)
    lm r8,(!fx_draw_x)
    from r3
    add r3
    add r0
    add r0
    add r0
    to r5
    hib
    from r5
    add r8
    sbk
    lm r9,(!fx_draw_y)
    from r4
    add r4
    add r0
    add r0
    add r0
    add r0
    to r7
    hib
    bra FxScaledSprite
    nop
FxScaledSpriteStop:
    stop                            ; not reached in the traces so far
    nop                             ; not reached in the traces so far
FxScaledSprite:
    lm r14,(!gsu_in_216)
    moves r14,r14
    beq FxScaledSpriteStop          ; taken: the delay slot is only the ALT1 of the LM, then STOP
    lm r3,(!gsu_in_3a)
    moves r8,r8
    bmi .lFB48
    nop
    from r8
    sub r3
    bcs FxScaledSpriteStop          ; the FROM R8 in the delay slot is dropped by the STOP
    from r8
    add r5
    sub r3
    bcc .lFB43
    nop
    from r3                         ; not reached in the traces so far
    to r5                           ; not reached in the traces so far
    sub r8                          ; not reached in the traces so far
.lFB43:
    move r1,r8
    bra .lFB50
    nop
.lFB48:
    from r8                         ; not reached in the traces so far
    to r5                           ; not reached in the traces so far
    add r5                          ; not reached in the traces so far
    bmi FxScaledSpriteStop          ; not reached in the traces so far
    nop                             ; not reached in the traces so far
    with r1                         ; not reached in the traces so far
    sub r1                          ; not reached in the traces so far
.lFB50:
    lm r4,(!gsu_in_3e)
    moves r9,r9
    bmi .lFB68                      ; delay slot FROM R9: the ADD at the target adds R9
    from r9
    sub r4
    bcs FxScaledSpriteStop
    from r9
    add r7
    sub r4
    bcc .lFB64                      ; delay slot FROM R4: the SUB below takes R4
    from r4
    to r7                           ; not reached in the traces so far
    sub r9                          ; not reached in the traces so far
.lFB64:
    move r2,r9
    bra .lFB6E                      ; delay slot TO R7, overridden by the MOVE at the target
.lFB68:
    to r7
    add r7
    bmi .lFBBA                      ; taken: delay slot WITH R2, so the RPIX at the target writes R2
    with r2
    sub r2
.lFB6E:
    move r10,r1
    lm r3,(!fx_draw_step_x)
    lm r4,(!fx_draw_step_y)
.lFB78:
    cache
    move r12,r5
    move r13,r15
    lm r14,(!gsu_in_216)
    from r2
    sub r9
    beq .lFB91
    nop
    swap
    move r6,r4
    fmult
    add r0
    add r0
    add r0
    add r0
    add r0
    to r14
    add r14
.lFB91:
    from r1
    sub r8
    swap
    move r6,r3
    fmult
    move r6,r0
    asr
    asr
    to r14
    add r14
    getb
    with r6
    and #3
.lFBA1:
    moves r6,r6
    beq .lFBAA                      ; delay slot DEC R6 runs either way (R6 ends at -1)
    dec r6
    asr
    bra .lFBA1                      ; delay slot: the second ASR
    asr
.lFBAA:
    and #3
    add r11
    ldb (r0)
    color
    plot
    loop                            ; delay slot: the ALT1 of the RPIX; taken, it prefixes the LM
    rpix
    with r1
    sub r5
    dec r7
    bne .lFB78
    inc r2
.lFBBA:
    rpix
    stop
    nop

arch 65816
