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

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode cmp_code_code[14][2] = {
    {{0x01D357, 6}, {0, 0}},
    {{0x01D36A, 7}, {0, 0}},
    {{0x01D37C, 7}, {0, 0}},
    {{0x01D39C, 7}, {0, 0}},
    {{0x01D46F, 7}, {0, 0}},
    {{0x01D3B4, 7}, {0, 0}},
    {{0x01D3C7, 7}, {0, 0}},
    {{0x01D3E3, 7}, {0, 0}},
    {{0x01D3FF, 7}, {0, 0}},
    {{0x01D41B, 7}, {0, 0}},
    {{0x01D437, 7}, {0, 0}},
    {{0x01D453, 7}, {0, 0}},
    {{0x01D357, 6}, {0x01D36A, 7}},
    {{0, 0}, {0, 0}},
};

/* CMP of R0 with a byte IBT loads (sign-extended). */
static void cmp_code(uint8_t code, int copy)
{
    gsu_codes(cmp_code_code[copy], 2);
    gsu.r[3] = (uint16_t)(int16_t)(int8_t)code;
    gsu_sub(gsu.r[0], gsu.r[3], 0);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode picture_code[5][16] = {
    {{0x01D4BD, 7}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01D4C4, 2}, {0x01D4C6, 4}, {0x01D5D2, 1}, {0xD5D3, 0}, {0x01D5D3, 5}, {0x01D4CA, 2}, {0x01D64E, 4}},
    {{0x01D406, 11}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01D411, 2}, {0x01D413, 4}, {0x01F062, 1}, {0xF063, 0}, {0x01F063, 5}, {0x01D417, 4}, {0x01F3E0, 4}},
    {{0x01D37C, 7}, {0x01D39C, 7}, {0x01D3B4, 7}, {0x01D3C7, 7}, {0x01D3E3, 7}, {0x01D3FF, 7}, {0x01D41B, 7}, {0x01D437, 7}, {0x01D453, 18}, {0x01D465, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01D422, 11}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01D42D, 2}, {0x01D42F, 4}, {0x01DCD6, 1}, {0xDCD7, 0}, {0x01DCD7, 5}, {0x01D433, 4}, {0x01E356, 4}},
    {{0x01D43E, 11}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01D449, 2}, {0x01D44B, 4}, {0x01DCD6, 1}, {0xDCD7, 0}, {0x01DCD7, 5}, {0x01D44F, 4}, {0x01E356, 4}},
};

/* A picture: ROM bank $15, R14 = $8000 + at, the blit called with R11 at
   its return address. */
static void picture(uint16_t at, void (*blit)(void), uint16_t ret, int copy)
{
    gsu_codes(picture_code[copy], 9);
    gsu.r[1] = at;
    gsu.r[0] = 0x0015;
    gsu.rombr = 0x15;
    gsu_r14(0x8000);
    gsu_codes(picture_code[copy] + 9, 1);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[1], 0));
    gsu.r[11] = ret;
    (gsu_codes(picture_code[copy] + 10, 4), blit());
    gsu_codes(picture_code[copy] + 14, 2);
}

/* The glyph R0 - '0' (doubled) from ROM bank 1's table, drawn and Y put
   back (SUB of the line height). */
static void glyph(uint16_t table, void (*blit)(void), uint16_t ret)
{
    gsu_r14(table);
    gsu_code(0x01D4B5, 2);
    gsu_r14(gsu_add(gsu.r[0], gsu.r[14], 0));
    gsu.r[1] = (gsu_code(0x01D4B7, 2), gsu_getb());
    gsu_code(0x01D4B9, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01D4BA, 3);
    gsu.r[1] = gsu_getbh(gsu.r[1]);
    picture(gsu.r[1], blit, ret, GSU_COPY(0));
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
    gsu_code(0x01D1BD, 4); gsu_code(0x01D311, 3);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x01D314, 6);
    gsu.r[7] = gsu_ldw(FX_TEXT_X);
    gsu_code(0x01D31A, 4);
    gsu.r[2] = gsu_ldw(FX_TEXT_Y);
    gsu.r[5] = 0;
    gsu.r[6] = 0x0020;
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu.rambr = (uint8_t)gsu.r[0];
    gsu.r[0] = 0;
    gsu.por = (uint8_t)gsu.r[0];
    gsu.r[9] = FX_TEXT_PALETTES;
    gsu_code(0x01D31E, 19);
    gsu.r[0] = gsu_ldw(FX_TEXT_AT);
    gsu.r[8] = gsu.r[0];
    gsu_moves(gsu.r[8]);
    if (gsu.z)
        { gsu_code(0x01D331, 8); goto end; } else { gsu_code(0x01D331, 4); gsu_code(0x01D339, 2); }
    gsu.r[4] = gsu.r[7];
    for (;;) {
        gsu_code(0x01D33B, 5);
        gsu.r[0] = gsu_lob(gsu_ldw(FX_TEXT_BANK));
        gsu.rombr = (uint8_t)gsu.r[0];
        gsu_code(0x01D340, 5);
        gsu_r14(gsu.r[8]);
        gsu.r[0] = (gsu_code(0x01D345, 1), gsu_getb());
        gsu.r[8] = gsu_inc(gsu.r[8]);
        gsu.r[0] = (uint16_t)(int16_t)(int8_t)gsu.r[0];      /* SEX */
        gsu_sz(gsu.r[0]);
        if (gsu.z)
            { gsu_code(0x01D346, 8); gsu_code(0x01D4DD, 3); break; } else { gsu_code(0x01D346, 4); gsu_code(0x01D34E, 3); gsu_code(0x01D355, 2); }
        if (gsu.s) {
            gsu_code(0x01D355, 2);
            gsu_r14(gsu_inc(gsu.r[14]));
            gsu_code(0x01D357, 6); gsu_code(0x01D36A, 7);
            cmp_code(0xFF, GSU_COPY(0));
            if (gsu.z) {
                gsu_code(0x01D35D, 3);
                gsu.r[7] = gsu_getb();
                gsu_code(0x01D360, 1);
                gsu_r14(gsu_inc(gsu.r[14]));
                gsu.r[2] = (gsu_code(0x01D361, 2), gsu_getb());
                gsu_code(0x01D363, 1);
                gsu_r14(gsu_inc(gsu.r[14]));
                gsu.r[4] = gsu.r[7];
                gsu.r[8] = gsu.r[14];
                gsu_code(0x01D364, 6);
                continue;
            }
            cmp_code(0xFE, GSU_COPY(1));
            if (gsu.z) {
                gsu_code(0x01D371, 2);
                gsu.r[0] = gsu_getb();
                gsu.r[8] = gsu_inc(gsu.r[8]);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
                gsu.r[9] = FX_TEXT_PALETTES;
                gsu.r[9] = gsu_add(gsu.r[0], gsu.r[9], 0);  /* BRA's delay slot */
                gsu_code(0x01D373, 9);
                continue;
            }
            cmp_code(0xFD, GSU_COPY(2));
            if (gsu.z) {
                gsu_code(0x01D383, 2);
                gsu.r[0] = gsu_getb();
                gsu.r[8] = gsu_inc(gsu.r[8]);
                gsu_moves(gsu.r[0]);
                if (gsu.z) {
                    gsu_code(0x01D393, 9);
                    gsu.r[6] = 0x0020;
                    gsu.r[5] = 0;
                } else {
                    gsu_code(0x01D38A, 9);
                    gsu.r[6] = 0x0010;
                    gsu.r[5] = 1;
                }
                gsu_code(0x01D385, 5);
                continue;
            }
            cmp_code(0xFC, GSU_COPY(3));
            if (gsu.z) {
                gsu_code(0x01D3A3, 8); gsu_code(0x01D3AE, 6);
                gsu.r[0] = 0x0004;
                gsu_moves(gsu.r[5]);
                if (gsu.z)
                    { gsu_code(0x01D3AB, 3); gsu.r[0] = 0x0008; }
                gsu.r[4] = gsu_add(gsu.r[4], gsu.r[0], 0);
                continue;
            }
            cmp_code(0xF5, GSU_COPY(5));
            if (gsu.z) {
                gsu_code(0x01D3BB, 12);
                gsu.r[4] = gsu.r[7];
                gsu.r[0] = gsu_div2(gsu.r[6]);
                gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
                continue;
            }
            cmp_code(0xFB, GSU_COPY(6));
            if (gsu.z) {
                gsu_code(0x01D3CE, 11);
                picture(0x3700, abi_FxDrawBitmap120x32, 0xD3DF, GSU_COPY(0));
                gsu_code(0x01D3DF, 4); gsu_code(0x01E9DB, 3);
                continue;
            }
            cmp_code(0xFA, GSU_COPY(7));
            if (gsu.z) {
                gsu_code(0x01D3EA, 11);
                picture(0x3B00, abi_FxDrawBitmap120x16, 0xD3FB, GSU_COPY(0));
                gsu_code(0x01D3FB, 4); gsu_code(0x01F05F, 3);
                continue;
            }
            cmp_code(0xF9, GSU_COPY(8));
            if (gsu.z) {
                picture(0x1810, abi_FxDrawBitmap64x16, 0xD417, GSU_COPY(1));
                continue;
            }
            cmp_code(0xF8, GSU_COPY(9));
            if (gsu.z) {
                picture(0x3700, abi_FxDrawBitmap120x24, 0xD433, GSU_COPY(3));
                continue;
            }
            cmp_code(0xF7, GSU_COPY(10));
            if (gsu.z) {
                picture(0x3A00, abi_FxDrawBitmap120x24, 0xD44F, GSU_COPY(4));
                continue;
            }
            cmp_code(0xF6, GSU_COPY(11));
            if (gsu.z) {
                picture(0x3D1C, abi_FxDrawBitmap16x24, 0xD46B, GSU_COPY(0));
                continue;
            }
        } else { gsu_code(0x01D351, 4); }
        cmp_code(0x0D, GSU_COPY(4));
        if (gsu.z) {
            gsu_code(0x01D476, 8);
            gsu.r[4] = gsu.r[7];
            gsu.r[2] = gsu_add(gsu.r[2], gsu.r[6], 0);
            continue;
        } else { gsu_code(0x01D47E, 6); }
        gsu.r[3] = 0x0030;
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[3], 0);
        if (!gsu.s) {
            gsu_code(0x01D488, 11);
            gsu.r[3] = 0x0001;
            gsu.rombr = (uint8_t)gsu.r[3];
            gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
            gsu_moves(gsu.r[5]);
            if (gsu.z)
                { gsu_code(0x01D493, 4); glyph(FX_GLYPHS_BIG, abi_FxDrawBitmap16x32, 0xD4AC); }
            else
                { gsu_code(0x01D4B1, 4); glyph(FX_GLYPHS_SMALL, abi_FxDrawBitmap8x16, 0xD4CA); }
        } else { gsu_code(0x01D484, 4); }
        gsu_code(0x01D4CC, 8); gsu_code(0x01D4D7, 6);
        gsu.r[0] = 0x000A;
        gsu_moves(gsu.r[5]);
        if (gsu.z)
            { gsu_code(0x01D4D4, 3); gsu.r[0] = 0x0014; }
        gsu.r[4] = gsu_add(gsu.r[4], gsu.r[0], 0);
    }
end:
    gsu_code(0x01D4DD, 3);
    gsu.r[0] = gsu_rpix((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
    gsu_code(0x01D4E0, 4);
    gsu_sz(gsu.r[0]);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
    gsu_code(0x01D1C1, 6);
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
    gsu_code(0x01D308, 7);
    gsu.r[10] = 0x0240;
    gsu.r[11] = 0xD30F;
    abi_FxTextDraw();
    gsu_code(0x01D30F, 1);
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
    gsu_code(0x01D130, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x01D132, 6);
    gsu.r[0] = gsu_ldw(0x273C);
    gsu.r[1] = 0x0001;
    gsu.r[0] = gsu_and(gsu.r[0], gsu.r[1]);
    if (gsu.z)
        { gsu_code(0x01D138, 10); gsu_code(0x01D1C6, 1); goto done; } else { gsu_code(0x01D138, 6); gsu_code(0x01D142, 5); }
    gsu.r[0] = gsu_ldw(0x2648);
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        { gsu_code(0x01D147, 8); gsu_code(0x01D1C6, 1); goto done; } else { gsu_code(0x01D147, 4); gsu_code(0x01D14F, 5); }
    gsu.r[1] = gsu_ldw(0x2476);
    gsu_sub(gsu.r[0], gsu.r[1], 0);     /* CMP */
    if (gsu.z)
        { gsu_code(0x01D154, 8); gsu_code(0x01D1C6, 1); goto done; } else { gsu_code(0x01D154, 4); gsu_code(0x01D15C, 5); }
    gsu.r[1] = gsu_ldw(0x2478);
    gsu_sub(gsu.r[0], gsu.r[1], 0);
    if (gsu.z)
        { gsu_code(0x01D161, 8); gsu_code(0x01D1C6, 1); goto done; } else { gsu_code(0x01D161, 4); gsu_code(0x01D169, 7); }
    gsu.r[2] = gsu.r[0];
    gsu.r[0] = gsu_ldw(0x264C);
    gsu_moves(gsu.r[0]);
    if (gsu.s)
        { gsu_code(0x01D170, 8); gsu_code(0x01D1C6, 1); goto done; } else { gsu_code(0x01D170, 4); gsu_code(0x01D178, 5); }
    gsu.r[0] = gsu_ldw(0x264A);
    gsu.r[0] = gsu_div2(gsu.r[0]);
    gsu_code(0x01D17D, 6);
    gsu.r[1] = gsu_ldw(0x0034);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1], 0);
    if (gsu.s)
        { gsu_code(0x01D186, 1); gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0); }
    gsu_code(0x01D183, 3); gsu_code(0x01D187, 7);
    gsu.r[4] = 0x0011;
    gsu.r[1] = gsu_ldw(0x003A);
    gsu.r[1] = gsu_sub(gsu.r[1], gsu.r[4], 0);
    gsu_sub(gsu.r[0], gsu.r[1], 0);     /* CMP */
    if (!gsu.s)
        { gsu_code(0x01D194, 2); gsu.r[0] = gsu.r[1]; }
    gsu_code(0x01D18E, 6); gsu_code(0x01D196, 5);
    gsu_stw(FX_TEXT_X, gsu.r[0]);
    gsu.r[0] = 0x0068;
    gsu_code(0x01D19B, 7);
    gsu_stw(FX_TEXT_Y, gsu.r[0]);
    gsu.r[0] = 0x0001;
    gsu_code(0x01D1A2, 6);
    gsu_stw(FX_TEXT_BANK, gsu.r[0]);
    gsu.rombr = (uint8_t)gsu.r[0];
    gsu.r[0] = 0x0096;
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[2], 0);
    gsu_code(0x01D1A8, 7);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01D1AF, 4);
    gsu_r14(0xD1CB);
    gsu_code(0x01D1B3, 2);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu.r[0] = (gsu_code(0x01D1B5, 1), gsu_getb());
    gsu_code(0x01D1B6, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01D1B7, 2);
    gsu.r[0] = gsu_getbh(gsu.r[0]);
    gsu_code(0x01D1B9, 4);
    gsu_stw(FX_TEXT_AT, gsu.r[0]);
    gsu.r[11] = 0xD1C1;
    (gsu_code(0x01D1BD, 4), abi_FxTextDraw());
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01D1C1, 6);
    gsu_stw(FX_TEXT_AT, gsu.r[0]);
done:
    gsu_code(0x01D1C7, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
    gsu_code(0x01AB7D, 1);
}
