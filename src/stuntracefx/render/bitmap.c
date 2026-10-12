/*
 * bitmap.c: the Super FX drawing small bitmaps from ROM with PLOT: two
 * bits per pixel, four pixels a byte from bit 0 up, each pixel's colour
 * looked up in a four-byte palette in RAM.
 */
#include "gsu.h"
#include "gsu_ops.h"

/* COLOR: the plot colour from a byte, as the plot options shape it (high
   nibble from the source's high nibble, or the high nibble kept). */
static void fx_color(uint8_t v)
{
    if (gsu.por & 0x04)
        gsu.colr = (uint8_t)((gsu.colr & 0xF0) | v >> 4);
    else if (gsu.por & 0x08)
        gsu.colr = (uint8_t)((gsu.colr & 0xF0) | (v & 0x0F));
    else
        gsu.colr = v;
}

/*
 * @routine 01:F3E4 FxDrawBitmap16x24
 * WHAT IT DOES: Draws a 16 x 24 pixel bitmap from ROM at R14 (rows 32
 * bytes apart) with its top left at (R4, R2), through the palette at R9.
 * HOW: CACHE; a LOOP of 24 rows. Each row starts R1 at R4 and takes four
 * bytes with GETB; for each of a byte's four pixels, AND #3 and ADD R9
 * pick the palette byte (LDB), COLOR and PLOT draw it (PLOT steps R1), and
 * two RORs through the carry bring the next pixel's bits down. R14 then
 * skips the rest of the 32-byte row (ADD of $1C) and INC R2, in LOOP's
 * delay slot, moves down a line. Returns with MOVE R15,R11.
 * Entry: R4, R2 = top left; R9 = the palette; R14 = the bitmap, in the ROM
 * bank; R11 = return address. Exit: R2 = R2 + 24; R1 = R4 + 16; R14 past
 * the last row; R3 = the last byte as rotated; R0 = $1C; R12 = 0; R13 =
 * $F3EA (the row loop); the colour the last pixel's; the last RAM address
 * its palette byte; OV and CY from the last ADD to R14; S and Z from R2.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxDrawBitmap16x24(void)
{
    int more;

    gsu.r[12] = 24;
    gsu.r[13] = 0xF3EA;
    do {
        int b, p;

        gsu.r[1] = gsu.r[4];
        for (b = 0; b < 4; b++) {
            gsu.r[3] = gsu_getb();
            gsu_r14(gsu_inc(gsu.r[14]));
            for (p = 0; p < 4; p++) {
                uint16_t at = gsu_add(gsu_and(gsu.r[3], 3), gsu.r[9], 0);

                fx_color(gsu_ldb(at));
                gsu_plot((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
                gsu.r[1]++;
                gsu.r[3] = gsu_ror(gsu_ror(gsu.r[3]));
            }
        }
        gsu.r[0] = 0x1C;
        gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
        more = gsu_count_down(&gsu.r[12]);
        gsu.r[2] = gsu_inc(gsu.r[2]);       /* LOOP's delay slot */
    } while (more);
}

void abi_FxDrawBitmap16x24(void)
{
    FxDrawBitmap16x24();
}
