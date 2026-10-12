; bitmap.asm: the Super FX drawing small bitmaps from ROM with PLOT: two
; bits per pixel, four pixels a byte from bit 0 up, each pixel's colour
; looked up in a four-byte palette in RAM.
; C: src/stuntracefx/render/bitmap.c.

incsrc "../include/macros.inc"

arch superfx

; @routine 01:F3E4 FxDrawBitmap16x24
; WHAT IT DOES: Draws a 16 x 24 pixel bitmap from ROM at R14 (rows 32
; bytes apart) with its top left at (R4, R2), through the palette at R9.
; HOW: CACHE; a LOOP of 24 rows. Each row starts R1 at R4 and takes four
; bytes with GETB; for each of a byte's four pixels, AND #3 and ADD R9
; pick the palette byte (LDB), COLOR and PLOT draw it (PLOT steps R1), and
; two RORs through the carry bring the next pixel's bits down. R14 then
; skips the rest of the 32-byte row (ADD of $1C) and INC R2, in LOOP's
; delay slot, moves down a line. Returns with MOVE R15,R11.
; Entry: R4, R2 = top left; R9 = the palette; R14 = the bitmap, in the ROM
; bank; R11 = return address. Exit: R2 = R2 + 24; R1 = R4 + 16; R14 past
; the last row; R3 = the last byte as rotated; R0 = $1C; R12 = 0; R13 =
; $F3EA (the row loop); the colour the last pixel's; the last RAM address
; its palette byte; OV and CY from the last ADD to R14; S and Z from R2.
; R14 and the ROM buffer move as on the Super FX.
%at($01F3E4)
FxDrawBitmap16x24:
    cache
    iwt r12,#$0018
    move r13,r15
    move r1,r4
    to r3
    getb
    inc r14
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    to r3
    getb
    inc r14
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    to r3
    getb
    inc r14
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    to r3
    getb
    inc r14
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    from r3
    and #3
    add r9
    ldb (r0)
    color
    plot
    from r3
    ror
    ror
    move r3,r0
    iwt r0,#$001C
    with r14
    add r0
    loop
    inc r2
    move r15,r11
    nop

arch 65816
