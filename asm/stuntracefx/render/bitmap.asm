; bitmap.asm: the Super FX drawing small bitmaps from ROM with PLOT: two
; bits per pixel, four pixels a byte from bit 0 up, each pixel's colour
; looked up in a four-byte palette in RAM.
; C: src/stuntracefx/render/bitmap.c.

incsrc "../include/macros.inc"

arch superfx

; fx_blit(rows, bytes, skip): the blit each routine here is (see fx_blit in
; the C): CACHE, a LOOP of rows, each of bytes GETBs of four pixels, the
; row's rest skipped, INC R2 in the delay slot; MOVE R15,R11 returns.
macro fx_blit(rows, bytes, skip)
    cache
    iwt r12,#<rows>
    move r13,r15
    move r1,r4
    for b = 0..<bytes>
        to r3
        getb
        inc r14
        for p = 0..4
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
        endfor
    endfor
    iwt r0,#<skip>
    with r14
    add r0
    loop
    inc r2
    move r15,r11
    nop
endmacro

; @routine 01:D4E4 FxDrawBitmap16x32
; WHAT IT DOES: As FxDrawBitmap16x24 for a 16 x 32 pixel bitmap.
; HOW: The blit (fx_blit) of 32 rows of 4 bytes, R14 skipping $1C at
; each row's end. Not reached in the traces so far.
; Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
; 32, R1 = R4 + 16, R0 = $1C, R13 = $D4EA.
%at($01D4E4)
FxDrawBitmap16x32:
    %fx_blit(32, 4, 28)

; @routine 01:D5D2 FxDrawBitmap8x16
; WHAT IT DOES: As FxDrawBitmap16x24 for a 8 x 16 pixel bitmap.
; HOW: The blit (fx_blit) of 16 rows of 2 bytes, R14 skipping $1E at
; each row's end. Not reached in the traces so far.
; Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
; 16, R1 = R4 + 8, R0 = $1E, R13 = $D5D8.
%at($01D5D2)
FxDrawBitmap8x16:
    %fx_blit(16, 2, 30)

; @routine 01:D652 FxDrawBitmap120x48
; WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 48 pixel bitmap.
; HOW: The blit (fx_blit) of 48 rows of 30 bytes, R14 skipping $02 at
; each row's end. Nothing calls it.
; Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
; 48, R1 = R4 + 120, R0 = $02, R13 = $D658.
%at($01D652)
FxDrawBitmap120x48:
    %fx_blit(48, 30, 2)

; @routine 01:DCD6 FxDrawBitmap120x24
; WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 24 pixel bitmap.
; HOW: The blit (fx_blit) of 24 rows of 30 bytes, R14 skipping $02 at
; each row's end. Not reached in the traces so far.
; Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
; 24, R1 = R4 + 120, R0 = $02, R13 = $DCDC.
%at($01DCD6)
FxDrawBitmap120x24:
    %fx_blit(24, 30, 2)

; @routine 01:E35A FxDrawBitmap120x32
; WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 32 pixel bitmap.
; HOW: The blit (fx_blit) of 32 rows of 30 bytes, R14 skipping $02 at
; each row's end. Not reached in the traces so far.
; Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
; 32, R1 = R4 + 120, R0 = $02, R13 = $E360.
%at($01E35A)
FxDrawBitmap120x32:
    %fx_blit(32, 30, 2)

; @routine 01:E9DE FxDrawBitmap120x16
; WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 16 pixel bitmap.
; HOW: The blit (fx_blit) of 16 rows of 30 bytes, R14 skipping $02 at
; each row's end. Not reached in the traces so far.
; Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
; 16, R1 = R4 + 120, R0 = $02, R13 = $E9E4.
%at($01E9DE)
FxDrawBitmap120x16:
    %fx_blit(16, 30, 2)

; @routine 01:F062 FxDrawBitmap64x16
; WHAT IT DOES: As FxDrawBitmap16x24 for a 64 x 16 pixel bitmap.
; HOW: The blit (fx_blit) of 16 rows of 16 bytes, R14 skipping $10 at
; each row's end. Not reached in the traces so far.
; Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
; 16, R1 = R4 + 64, R0 = $10, R13 = $F068.
%at($01F062)
FxDrawBitmap64x16:
    %fx_blit(16, 16, 16)

; @routine 01:F3E4 FxDrawBitmap16x24
; WHAT IT DOES: Draws a 16 x 24 pixel bitmap from ROM at R14 (rows 32
; bytes apart) with its top left at (R4, R2), through the palette at R9.
; HOW: The blit (fx_blit) of 24 rows of four bytes, R14 skipping $1C at
; each row's end.
; Entry: R4, R2 = top left; R9 = the palette; R14 = the bitmap, in the ROM
; bank; R11 = return address. Exit: R2 = R2 + 24; R1 = R4 + 16; R14 past
; the last row; R3 = the last byte as rotated; R0 = $1C; R12 = 0; R13 =
; $F3EA (the row loop); the colour the last pixel's; the last RAM address
; its palette byte; OV and CY from the last ADD to R14; S and Z from R2.
; R14 and the ROM buffer move as on the Super FX.
%at($01F3E4)
FxDrawBitmap16x24:
    %fx_blit(24, 4, 28)

arch 65816
