/*
 * bitmap.c: the Super FX drawing small bitmaps from ROM with PLOT: two
 * bits per pixel, four pixels a byte from bit 0 up, each pixel's colour
 * looked up in a four-byte palette in RAM.
 */
#include "gsu.h"
#include "gsu_ops.h"

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_color_code[9][1] = {
    {{0, 0}},
    {{0x01F3D4, 12}},
    {{0x01D5C2, 12}},
    {{0x01DCC6, 12}},
    {{0x01E34A, 12}},
    {{0x01E9CE, 12}},
    {{0x01F052, 12}},
    {{0x01F3D4, 2}},
    {{0x01F4C2, 12}},
};

/* COLOR: the plot colour from a byte, as the plot options shape it (high
   nibble from the source's high nibble, or the high nibble kept). */
static void fx_color(uint8_t v, int copy)
{
    gsu_codes(fx_color_code[copy], 1);
    if (gsu.por & 0x04)
        gsu.colr = (uint8_t)((gsu.colr & 0xF0) | v >> 4);
    else if (gsu.por & 0x08)
        gsu.colr = (uint8_t)((gsu.colr & 0xF0) | (v & 0x0F));
    else
        gsu.colr = v;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t fx_color_from_fx_blit[11] = {0, 1, 2, 0, 3, 4, 5, 6, 7, 8, 4};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_blit_code[11][10] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01D5D8, 5}, {0, 0}, {0x01D644, 10}, {0x01D64E, 1}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01F068, 5}, {0, 0}, {0, 0}, {0x01F3E0, 1}, {0, 0}, {0, 0}},
    {{0x01D4E5, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01D4EA, 5}, {0, 0}, {0, 0}, {0x01D5CE, 1}, {0x0180E3, 1}, {0x01D5CE, 4}},
    {{0x01D5D3, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01D5D8, 5}, {0, 0}, {0x01D642, 12}, {0x01D64E, 1}, {0x0180E3, 1}, {0x01D64E, 4}},
    {{0x01D653, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01D658, 5}, {0, 0}, {0, 0}, {0x01DCD2, 1}, {0x0180E3, 1}, {0x01DCD2, 4}},
    {{0x01DCD7, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01DCDC, 5}, {0, 0}, {0, 0}, {0x01E356, 1}, {0x0180E3, 1}, {0x01E356, 4}},
    {{0x01E35B, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01E360, 5}, {0, 0}, {0, 0}, {0x01E9DA, 1}, {0x0180E3, 1}, {0x01E9DA, 4}},
    {{0x01E9DF, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01E9E4, 5}, {0, 0}, {0, 0}, {0x01F05E, 1}, {0x0180E3, 1}, {0x01F05E, 4}},
    {{0x01F063, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01F068, 5}, {0x01F3D6, 10}, {0, 0}, {0x01F3E0, 1}, {0x0180E3, 1}, {0x01F3E0, 4}},
    {{0x01D467, 4}, {0x01F3E4, 1}, {0xF3E5, 0}, {0x01F3E5, 5}, {0x01F3EA, 5}, {0, 0}, {0, 0}, {0x01F4CE, 1}, {0x01D46B, 4}, {0x01F4CE, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01DCDC, 5}, {0, 0}, {0, 0}, {0x01E356, 1}, {0, 0}, {0, 0}},
};

/* The blit every bitmap routine here is: CACHE; a LOOP of ROWS rows (R13
   = LOOP, the routine's start + 6). Each row starts R1 at R4 and takes
   BYTES bytes with GETB; for each of a byte's four pixels, AND #3 and ADD
   R9 pick the palette byte (LDB), COLOR and PLOT draw it (PLOT steps R1),
   and two RORs through the carry bring the next pixel's bits down. R14
   then skips the rest of the 32-byte row (ADD of SKIP) and INC R2, in
   LOOP's delay slot, moves down a line. Returns with MOVE R15,R11. */
static void fx_blit(uint16_t rows, int bytes, uint16_t skip, uint16_t loop, int copy)
{
    gsu_codes(fx_blit_code[copy], 4);
    int more;

    gsu.r[12] = rows;
    gsu.r[13] = loop;
    do {
        gsu_codes(fx_blit_code[copy] + 4, 1);
        int b, p;

        gsu.r[1] = gsu.r[4];
        for (b = 0; b < bytes; b++) {
            gsu.r[3] = gsu_getb();
            gsu_r14(gsu_inc(gsu.r[14]));
            for (p = 0; p < 4; p++) {
                uint16_t at = gsu_add(gsu_and(gsu.r[3], 3), gsu.r[9], 0);

                fx_color(gsu_ldb(at), GSU_COPY(fx_color_from_fx_blit[copy]));
                gsu_plot((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
                gsu.r[1]++;
                gsu.r[3] = gsu_ror(gsu_ror(gsu.r[3]));
                gsu_codes(fx_blit_code[copy] + 5, 1);
            }
        }
        gsu_codes(fx_blit_code[copy] + 6, 1);
        gsu.r[0] = skip;
        gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
        more = gsu_count_down(&gsu.r[12]);
        gsu.r[2] = gsu_inc(gsu.r[2]);       /* LOOP's delay slot */
    } while (gsu_codes_loop(fx_blit_code[copy] + 7, 1, more));
    gsu_codes(fx_blit_code[copy] + 8, 2);
}

/*
 * @routine 01:F3E4 FxDrawBitmap16x24
 * WHAT IT DOES: Draws a 16 x 24 pixel bitmap from ROM at R14 (rows 32
 * bytes apart) with its top left at (R4, R2), through the palette at R9.
 * HOW: The blit (fx_blit) of 24 rows of four bytes, R14 skipping $1C at
 * each row's end.
 * Entry: R4, R2 = top left; R9 = the palette; R14 = the bitmap, in the ROM
 * bank; R11 = return address. Exit: R2 = R2 + 24; R1 = R4 + 16; R14 past
 * the last row; R3 = the last byte as rotated; R0 = $1C; R12 = 0; R13 =
 * $F3EA (the row loop); the colour the last pixel's; the last RAM address
 * its palette byte; OV and CY from the last ADD to R14; S and Z from R2.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxDrawBitmap16x24(void)
{
    fx_blit(24, 4, 0x1C, 0xF3EA, GSU_COPY(0));
}

void abi_FxDrawBitmap16x24(void)
{
    FxDrawBitmap16x24();
}

/*
 * @routine 01:D4E4 FxDrawBitmap16x32
 * WHAT IT DOES: As FxDrawBitmap16x24 for a 16 x 32 pixel bitmap.
 * HOW: The blit (fx_blit) of 32 rows of 4 bytes, R14 skipping $1C at
 * each row's end. Not reached in the traces so far.
 * Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
 * 32, R1 = R4 + 16, R0 = $1C, R13 = $D4EA.
 */
void abi_FxDrawBitmap16x32(void)
{
    fx_blit(32, 4, 0x1C, 0xD4EA, GSU_COPY(2));
}

/*
 * @routine 01:D5D2 FxDrawBitmap8x16
 * WHAT IT DOES: As FxDrawBitmap16x24 for a 8 x 16 pixel bitmap.
 * HOW: The blit (fx_blit) of 16 rows of 2 bytes, R14 skipping $1E at
 * each row's end. Not reached in the traces so far.
 * Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
 * 16, R1 = R4 + 8, R0 = $1E, R13 = $D5D8.
 */
void abi_FxDrawBitmap8x16(void)
{
    fx_blit(16, 2, 0x1E, 0xD5D8, GSU_COPY(0));
}

/*
 * @routine 01:D652 FxDrawBitmap120x48
 * WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 48 pixel bitmap.
 * HOW: The blit (fx_blit) of 48 rows of 30 bytes, R14 skipping $02 at
 * each row's end. Nothing calls it.
 * Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
 * 48, R1 = R4 + 120, R0 = $02, R13 = $D658.
 */
void abi_FxDrawBitmap120x48(void)
{
    fx_blit(48, 30, 0x02, 0xD658, GSU_COPY(4));
}

/*
 * @routine 01:DCD6 FxDrawBitmap120x24
 * WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 24 pixel bitmap.
 * HOW: The blit (fx_blit) of 24 rows of 30 bytes, R14 skipping $02 at
 * each row's end. Not reached in the traces so far.
 * Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
 * 24, R1 = R4 + 120, R0 = $02, R13 = $DCDC.
 */
void abi_FxDrawBitmap120x24(void)
{
    fx_blit(24, 30, 0x02, 0xDCDC, GSU_COPY(5));
}

/*
 * @routine 01:E35A FxDrawBitmap120x32
 * WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 32 pixel bitmap.
 * HOW: The blit (fx_blit) of 32 rows of 30 bytes, R14 skipping $02 at
 * each row's end. Not reached in the traces so far.
 * Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
 * 32, R1 = R4 + 120, R0 = $02, R13 = $E360.
 */
void abi_FxDrawBitmap120x32(void)
{
    fx_blit(32, 30, 0x02, 0xE360, GSU_COPY(6));
}

/*
 * @routine 01:E9DE FxDrawBitmap120x16
 * WHAT IT DOES: As FxDrawBitmap16x24 for a 120 x 16 pixel bitmap.
 * HOW: The blit (fx_blit) of 16 rows of 30 bytes, R14 skipping $02 at
 * each row's end. Not reached in the traces so far.
 * Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
 * 16, R1 = R4 + 120, R0 = $02, R13 = $E9E4.
 */
void abi_FxDrawBitmap120x16(void)
{
    fx_blit(16, 30, 0x02, 0xE9E4, GSU_COPY(7));
}

/*
 * @routine 01:F062 FxDrawBitmap64x16
 * WHAT IT DOES: As FxDrawBitmap16x24 for a 64 x 16 pixel bitmap.
 * HOW: The blit (fx_blit) of 16 rows of 16 bytes, R14 skipping $10 at
 * each row's end. Not reached in the traces so far.
 * Entry: as FxDrawBitmap16x24. Exit: as FxDrawBitmap16x24 with R2 = R2 +
 * 16, R1 = R4 + 64, R0 = $10, R13 = $F068.
 */
void abi_FxDrawBitmap64x16(void)
{
    fx_blit(16, 16, 0x10, 0xF068, GSU_COPY(1));
}
