/*
 * text.c: the Super FX drawing a string of text and pictures into the
 * screen it renders. A string is bytes in ROM (the bank in $2734, from
 * the address in $2732): a byte from '0' up is a glyph (16 x 32, or 8 x 16
 * in the small font), $0D starts a new line, 0 ends the string, and the
 * bytes $F5-$FF are commands (position, palette, font, space, half line,
 * and pictures from ROM bank $15).
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_TEXT_AT 0x2732u      /* the string's address */
#define FX_TEXT_BANK 0x2734u    /* its ROM bank */
#define FX_TEXT_X 0x2738u       /* where it starts: X, then Y */
#define FX_TEXT_Y 0x273Au
#define FX_TEXT_PALETTES 0x2702u /* four bytes a palette */
#define FX_GLYPHS_BIG 0xF4D2u   /* $01:F4D2: per glyph, its offset in bank $15 */
#define FX_GLYPHS_SMALL 0xF52Eu

void abi_FxDrawBitmap16x32(void);
void abi_FxDrawBitmap8x16(void);
void abi_FxDrawBitmap120x24(void);
void abi_FxDrawBitmap120x32(void);
void abi_FxDrawBitmap120x16(void);
void abi_FxDrawBitmap64x16(void);
void abi_FxDrawBitmap16x24(void);

/* CMP of R0 with a byte IBT loads (sign-extended). */
static void cmp_code(uint8_t code)
{
    gsu.r[3] = (uint16_t)(int16_t)(int8_t)code;
    gsu_sub(gsu.r[0], gsu.r[3], 0);
}

/* A picture: ROM bank $15, R14 = $8000 + at, the blit called with R11 at
   its return address. */
static void picture(uint16_t at, void (*blit)(void), uint16_t ret)
{
    gsu.r[1] = at;
    gsu.r[0] = 0x0015;
    gsu.rombr = 0x15;
    gsu_r14(0x8000);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[1], 0));
    gsu.r[11] = ret;
    blit();
}

/* The glyph R0 - '0' (doubled) from ROM bank 1's table, drawn and Y put
   back (SUB of the line height). */
static void glyph(uint16_t table, void (*blit)(void), uint16_t ret)
{
    gsu_r14(table);
    gsu_r14(gsu_add(gsu.r[0], gsu.r[14], 0));
    gsu.r[1] = gsu_getb();
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu.r[1] = gsu_getbh(gsu.r[1]);
    picture(gsu.r[1], blit, ret);
    gsu.r[2] = gsu_sub(gsu.r[2], gsu.r[6], 0);
}

/*
 * @routine 01:D311 FxTextDraw
 * WHAT IT DOES: Draws the string at the address in $2732 (ROM bank $2734)
 * from the position at $2738/$273A: glyphs advance X by 20 (10 in the small
 * font), $0D goes back to the start X a line down; $FF sets the position
 * (two bytes), $FE the palette (one byte: $2702 + 4 * it), $FD the font
 * (one byte: 0 big, 32 lines, else small, 16 lines), $FC a space (8, 4 in
 * the small font), $F5 half a line down; $FB, $FA, $F9, $F8, $F7 and $F6
 * draw pictures from bank $15 ($3700 120 x 32, $3B00 120 x 16, $1810 64 x
 * 16, $3700 and $3A00 120 x 24, $3D1C 16 x 24). Other bytes below '0' only
 * advance X. Nothing is drawn when $2732 is 0.
 * HOW: Pushes R11 on the R10 stack. RAMB 0, CMODE 0, R9 = $2702. R8 walks
 * the string: each byte is read by setting R14 to it in the string's bank
 * (ROMB from $2734 each time), GETB, SEX. Commands are CMPs against bytes
 * IBT sign-extends; their bytes are read through R14. A glyph's bitmap
 * offset comes from ROM bank 1 ($F4D2 big, $F52E small) and is drawn by
 * FxDrawBitmap16x32 or FxDrawBitmap8x16. At the end RPIX writes out the
 * pixel caches. Returns by pulling R15.
 * Entry: R10 = stack; R11 = return address. Exit: R0 the pixel RPIX read
 * at (R1, R2); R4, R2 the position reached; R7 the line start X; R5, R6
 * the font; R8 past the string; R9 the palette; the rest as the last
 * drawing left them; S and Z from R10; the last RAM address R10. R14 and
 * the ROM buffer move as on the Super FX.
 */
void abi_FxTextDraw(void)
{
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[7] = gsu_ldw(FX_TEXT_X);
    gsu.r[2] = gsu_ldw(FX_TEXT_Y);
    gsu.r[5] = 0;
    gsu.r[6] = 0x0020;
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu.rambr = (uint8_t)gsu.r[0];
    gsu.r[0] = 0;
    gsu.por = (uint8_t)gsu.r[0];
    gsu.r[9] = FX_TEXT_PALETTES;
    gsu.r[0] = gsu_ldw(FX_TEXT_AT);
    gsu.r[8] = gsu.r[0];
    gsu_moves(gsu.r[8]);
    if (gsu.z)
        goto end;
    gsu.r[4] = gsu.r[7];
    for (;;) {
        gsu.r[0] = gsu_lob(gsu_ldw(FX_TEXT_BANK));
        gsu.rombr = (uint8_t)gsu.r[0];
        gsu_r14(gsu.r[8]);
        gsu.r[0] = gsu_getb();
        gsu.r[8] = gsu_inc(gsu.r[8]);
        gsu.r[0] = (uint16_t)(int16_t)(int8_t)gsu.r[0];      /* SEX */
        gsu_sz(gsu.r[0]);
        if (gsu.z)
            break;
        if (gsu.s) {
            gsu_r14(gsu_inc(gsu.r[14]));
            cmp_code(0xFF);
            if (gsu.z) {
                gsu.r[7] = gsu_getb();
                gsu_r14(gsu_inc(gsu.r[14]));
                gsu.r[2] = gsu_getb();
                gsu_r14(gsu_inc(gsu.r[14]));
                gsu.r[4] = gsu.r[7];
                gsu.r[8] = gsu.r[14];
                continue;
            }
            cmp_code(0xFE);
            if (gsu.z) {
                gsu.r[0] = gsu_getb();
                gsu.r[8] = gsu_inc(gsu.r[8]);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
                gsu.r[9] = FX_TEXT_PALETTES;
                gsu.r[9] = gsu_add(gsu.r[0], gsu.r[9], 0);  /* BRA's delay slot */
                continue;
            }
            cmp_code(0xFD);
            if (gsu.z) {
                gsu.r[0] = gsu_getb();
                gsu.r[8] = gsu_inc(gsu.r[8]);
                gsu_moves(gsu.r[0]);
                if (gsu.z) {
                    gsu.r[6] = 0x0020;
                    gsu.r[5] = 0;
                } else {
                    gsu.r[6] = 0x0010;
                    gsu.r[5] = 1;
                }
                continue;
            }
            cmp_code(0xFC);
            if (gsu.z) {
                gsu.r[0] = 0x0004;
                gsu_moves(gsu.r[5]);
                if (gsu.z)
                    gsu.r[0] = 0x0008;
                gsu.r[4] = gsu_add(gsu.r[4], gsu.r[0], 0);
                continue;
            }
            cmp_code(0xF5);
            if (gsu.z) {
                gsu.r[4] = gsu.r[7];
                gsu.r[0] = gsu_div2(gsu.r[6]);
                gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
                continue;
            }
            cmp_code(0xFB);
            if (gsu.z) {
                picture(0x3700, abi_FxDrawBitmap120x32, 0xD3DF);
                continue;
            }
            cmp_code(0xFA);
            if (gsu.z) {
                picture(0x3B00, abi_FxDrawBitmap120x16, 0xD3FB);
                continue;
            }
            cmp_code(0xF9);
            if (gsu.z) {
                picture(0x1810, abi_FxDrawBitmap64x16, 0xD417);
                continue;
            }
            cmp_code(0xF8);
            if (gsu.z) {
                picture(0x3700, abi_FxDrawBitmap120x24, 0xD433);
                continue;
            }
            cmp_code(0xF7);
            if (gsu.z) {
                picture(0x3A00, abi_FxDrawBitmap120x24, 0xD44F);
                continue;
            }
            cmp_code(0xF6);
            if (gsu.z) {
                picture(0x3D1C, abi_FxDrawBitmap16x24, 0xD46B);
                continue;
            }
        }
        cmp_code(0x0D);
        if (gsu.z) {
            gsu.r[4] = gsu.r[7];
            gsu.r[2] = gsu_add(gsu.r[2], gsu.r[6], 0);
            continue;
        }
        gsu.r[3] = 0x0030;
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[3], 0);
        if (!gsu.s) {
            gsu.r[3] = 0x0001;
            gsu.rombr = (uint8_t)gsu.r[3];
            gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
            gsu_moves(gsu.r[5]);
            if (gsu.z)
                glyph(FX_GLYPHS_BIG, abi_FxDrawBitmap16x32, 0xD4AC);
            else
                glyph(FX_GLYPHS_SMALL, abi_FxDrawBitmap8x16, 0xD4CA);
        }
        gsu.r[0] = 0x000A;
        gsu_moves(gsu.r[5]);
        if (gsu.z)
            gsu.r[0] = 0x0014;
        gsu.r[4] = gsu_add(gsu.r[4], gsu.r[0], 0);
    }
end:
    gsu.r[0] = gsu_rpix((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
    gsu_sz(gsu.r[0]);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

/*
 * @routine 01:D307 FxTextProgram
 * WHAT IT DOES: The program the S-CPU starts to draw the string at $2732:
 * FxTextDraw with the R10 stack at $0240.
 * HOW: IWT R10, LINK, the jump, then STOP.
 * Entry: none. Exit: as FxTextDraw, R10 = $0240.
 */
void abi_FxTextProgram(void)
{
    gsu.r[10] = 0x0240;
    gsu.r[11] = 0xD30F;
    abi_FxTextDraw();
}

/*
 * @routine 01:D12F FxTextNotice
 * WHAT IT DOES: When bit 0 of $273C is set and there is a nearest car
 * ($2648) that is neither $2476 (the player) nor $2478, and its $264C is
 * not negative, draws its label (the string from the table at $01:D1CB,
 * by the word at $96 + the car, in ROM bank 1) at Y $68 and X = half of
 * $264A plus the screen centre ($34), held to 0..$3A - $11; $2732 is
 * cleared after.
 * HOW: Pushes R11 on the R10 stack; CMPs and MOVES for the tests; DIV2,
 * then BPL and CMP for the bounds; ROMB 1, GETB and GETBH for the string's
 * address; FxTextDraw. Returns by pulling R15.
 * Entry: R10 = stack; R11 = return address. Exit: as FxTextDraw, R0 = 0;
 * or, when nothing is drawn, R0 and R1 as the last test left them; S and
 * Z from R10; the last RAM address R10.
 */
void abi_FxTextNotice(void)
{
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = gsu_ldw(0x273C);
    gsu.r[1] = 0x0001;
    gsu.r[0] = gsu_and(gsu.r[0], gsu.r[1]);
    if (gsu.z)
        goto done;
    gsu.r[0] = gsu_ldw(0x2648);
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        goto done;
    gsu.r[1] = gsu_ldw(0x2476);
    gsu_sub(gsu.r[0], gsu.r[1], 0);     /* CMP */
    if (gsu.z)
        goto done;
    gsu.r[1] = gsu_ldw(0x2478);
    gsu_sub(gsu.r[0], gsu.r[1], 0);
    if (gsu.z)
        goto done;
    gsu.r[2] = gsu.r[0];
    gsu.r[0] = gsu_ldw(0x264C);
    gsu_moves(gsu.r[0]);
    if (gsu.s)
        goto done;
    gsu.r[0] = gsu_ldw(0x264A);
    gsu.r[0] = gsu_div2(gsu.r[0]);
    gsu.r[1] = gsu_ldw(0x0034);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1], 0);
    if (gsu.s)
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu.r[4] = 0x0011;
    gsu.r[1] = gsu_ldw(0x003A);
    gsu.r[1] = gsu_sub(gsu.r[1], gsu.r[4], 0);
    gsu_sub(gsu.r[0], gsu.r[1], 0);     /* CMP */
    if (!gsu.s)
        gsu.r[0] = gsu.r[1];
    gsu_stw(FX_TEXT_X, gsu.r[0]);
    gsu.r[0] = 0x0068;
    gsu_stw(FX_TEXT_Y, gsu.r[0]);
    gsu.r[0] = 0x0001;
    gsu_stw(FX_TEXT_BANK, gsu.r[0]);
    gsu.rombr = (uint8_t)gsu.r[0];
    gsu.r[0] = 0x0096;
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[2], 0);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_r14(0xD1CB);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu.r[0] = gsu_getb();
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu.r[0] = gsu_getbh(gsu.r[0]);
    gsu_stw(FX_TEXT_AT, gsu.r[0]);
    gsu.r[11] = 0xD1C1;
    abi_FxTextDraw();
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(FX_TEXT_AT, gsu.r[0]);
done:
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}
