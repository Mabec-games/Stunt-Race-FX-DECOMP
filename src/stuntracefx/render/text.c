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
static void cmp_code(uint8_t code, int copy)
{
    gsu.r[3] = (uint16_t)(int16_t)(int8_t)code;
    gsu_sub(gsu.r[0], gsu.r[3], 0);
}

/* A picture: ROM bank $15, R14 = $8000 + at, the blit called with R11 at
   its return address. */
/* gsuwalk: the code of each copy of picture, read from the ROM */
static const GsuCode picture_fetch[8][6] = {
    {{0x01d3cf, 7}, {0x01d3d6, 3}, {0x01d3d9, 2}, {0x01d3db, 4}, {0x01e35a, 1}, {0xe35b, 0}},
    {{0x01d3eb, 7}, {0x01d3f2, 3}, {0x01d3f5, 2}, {0x01d3f7, 4}, {0x01e9de, 1}, {0xe9df, 0}},
    {{0x01d407, 7}, {0x01d40e, 3}, {0x01d411, 2}, {0x01d413, 4}, {0x01f062, 1}, {0xf063, 0}},
    {{0x01d423, 7}, {0x01d42a, 3}, {0x01d42d, 2}, {0x01d42f, 4}, {0x01dcd6, 1}, {0xdcd7, 0}},
    {{0x01d43f, 7}, {0x01d446, 3}, {0x01d449, 2}, {0x01d44b, 4}, {0x01dcd6, 1}, {0xdcd7, 0}},
    {{0x01d45b, 7}, {0x01d462, 3}, {0x01d465, 2}, {0x01d467, 4}, {0x01f3e4, 1}, {0xf3e5, 0}},
    {{0x01d49f, 4}, {0x01d4a3, 3}, {0x01d4a6, 2}, {0x01d4a8, 4}, {0x01d4e4, 1}, {0xd4e5, 0}},
    {{0x01d4bd, 4}, {0x01d4c1, 3}, {0x01d4c4, 2}, {0x01d4c6, 4}, {0x01d5d2, 1}, {0xd5d3, 0}},
};

static void picture(uint16_t at, void (*blit)(void), uint16_t ret, int copy)
{
    gsu.r[1] = at;
    gsu.r[0] = 0x0015;
    gsu_codes(picture_fetch[copy], 1);
    gsu_romb(0x15);
    gsu_codes(picture_fetch[copy] + 1, 1);
    gsu_r14(0x8000);
    gsu_codes(picture_fetch[copy] + 2, 1);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[1], 0));
    gsu.r[11] = ret;
    gsu_codes(picture_fetch[copy] + 3, 3);
    (blit());
}

/* The glyph R0 - '0' (doubled) from ROM bank 1's table, drawn and Y put
   back (SUB of the line height). */
/* gsuwalk: the code of each copy of glyph, read from the ROM */
static const GsuCode glyph_fetch[2][5] = {
    {{0x01d494, 3}, {0x01d497, 2}, {0x01d499, 2}, {0x01d49b, 1}, {0x01d49c, 3}},
    {{0x01d4b2, 3}, {0x01d4b5, 2}, {0x01d4b7, 2}, {0x01d4b9, 1}, {0x01d4ba, 3}},
};

/* gsuwalk: the copy of picture each copy of glyph calls */
static const uint8_t picture_in_glyph[2] = {6, 7};

static void glyph(uint16_t table, void (*blit)(void), uint16_t ret, int copy)
{
    gsu_codes(glyph_fetch[copy], 1);
    gsu_r14(table);
    gsu_codes(glyph_fetch[copy] + 1, 1);
    gsu_r14(gsu_add(gsu.r[0], gsu.r[14], 0));
    gsu_codes(glyph_fetch[copy] + 2, 1);
    gsu.r[1] = (gsu_getb());
    gsu_codes(glyph_fetch[copy] + 3, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_codes(glyph_fetch[copy] + 4, 1);
    gsu.r[1] = gsu_getbh(gsu.r[1]);
    picture(gsu.r[1], blit, ret, GSU_COPY(picture_in_glyph[copy]));
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
    gsu_code(0x01d312, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x01d314, 6);
    gsu.r[7] = gsu_ldw(FX_TEXT_X);
    gsu_code(0x01d31a, 4);
    gsu.r[2] = gsu_ldw(FX_TEXT_Y);
    gsu.r[5] = 0;
    gsu.r[6] = 0x0020;
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01d31e, 8);
    gsu_ramb(gsu.r[0]);
    gsu.r[0] = 0;
    gsu.por = (uint8_t)gsu.r[0];
    gsu.r[9] = FX_TEXT_PALETTES;
    gsu_code(0x01d326, 11);
    gsu.r[0] = gsu_ldw(FX_TEXT_AT);
    gsu.r[8] = gsu.r[0];
    gsu_moves(gsu.r[8]);
    if (gsu.z)
        { gsu_code(0x01d331, 8); gsu_code(0x01d4dd, 1);
        goto end; } else { gsu_code(0x01d331, 4); gsu_code(0x01d339, 1); }
    gsu.r[4] = gsu.r[7];
    gsu_code(0x01d33a, 2);
    for (;;) {
        gsu_code(0x01d33c, 4);
        gsu.r[0] = gsu_lob(gsu_ldw(FX_TEXT_BANK));
        gsu_code(0x01d340, 3);
        gsu_romb(gsu.r[0]);
        gsu_code(0x01d343, 2);
        gsu_r14(gsu.r[8]);
        gsu_code(0x01d345, 1);
        gsu.r[0] = (gsu_getb());
        gsu.r[8] = gsu_inc(gsu.r[8]);
        gsu.r[0] = (uint16_t)(int16_t)(int8_t)gsu.r[0];      /* SEX */
        gsu_sz(gsu.r[0]);
        if (gsu.z)
            { gsu_code(0x01d346, 4);
            break; } else { gsu_code(0x01d346, 4); gsu_code(0x01d34e, 1); }
        if (gsu.s) {
            gsu_code(0x01d34f, 2); gsu_code(0x01d355, 2);
            gsu_r14(gsu_inc(gsu.r[14]));
            cmp_code(0xFF, GSU_COPY(0));
            if (gsu.z) {
                gsu_code(0x01d357, 9);
                gsu.r[7] = gsu_getb();
                gsu_code(0x01d360, 1);
                gsu_r14(gsu_inc(gsu.r[14]));
                gsu_code(0x01d361, 2);
                gsu.r[2] = (gsu_getb());
                gsu_code(0x01d363, 1);
                gsu_r14(gsu_inc(gsu.r[14]));
                gsu.r[4] = gsu.r[7];
                gsu.r[8] = gsu.r[14];
                gsu_code(0x01d364, 6); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d357, 6); gsu_code(0x01d36a, 1); }
            cmp_code(0xFE, GSU_COPY(1));
            if (gsu.z) {
                gsu_code(0x01d36b, 8);
                gsu.r[0] = gsu_getb();
                gsu.r[8] = gsu_inc(gsu.r[8]);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
                gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
                gsu.r[9] = FX_TEXT_PALETTES;
                gsu.r[9] = gsu_add(gsu.r[0], gsu.r[9], 0);  /* BRA's delay slot */
                gsu_code(0x01d373, 9); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d36b, 6); gsu_code(0x01d37c, 1); }
            cmp_code(0xFD, GSU_COPY(2));
            if (gsu.z) {
                gsu_code(0x01d37d, 8);
                gsu.r[0] = gsu_getb();
                gsu.r[8] = gsu_inc(gsu.r[8]);
                gsu_moves(gsu.r[0]);
                if (gsu.z) {
                    gsu.r[6] = 0x0020;
                    gsu.r[5] = 0;
                    gsu_code(0x01d385, 5); gsu_code(0x01d393, 9); gsu_code(0x01d33b, 1);
                } else {
                    gsu.r[6] = 0x0010;
                    gsu.r[5] = 1;
                    gsu_code(0x01d385, 14); gsu_code(0x01d33b, 1);
                }
                continue;
            } else { gsu_code(0x01d37d, 6); gsu_code(0x01d39c, 1); }
            cmp_code(0xFC, GSU_COPY(3));
            if (gsu.z) {
                
                gsu.r[0] = 0x0004;
                gsu_moves(gsu.r[5]);
                if (gsu.z)
                    { gsu.r[0] = 0x0008; gsu_code(0x01d39d, 18); } else { gsu_code(0x01d39d, 14); gsu_code(0x01d3ae, 1); }
                gsu.r[4] = gsu_add(gsu.r[4], gsu.r[0], 0);
                gsu_code(0x01d3af, 5); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d39d, 6); gsu_code(0x01d3b4, 1); }
            cmp_code(0xF5, GSU_COPY(4));
            if (gsu.z) {
                gsu.r[4] = gsu.r[7];
                gsu.r[0] = gsu_div2(gsu.r[6]);
                gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
                gsu_code(0x01d3b5, 18); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d3b5, 6); gsu_code(0x01d3c7, 1); }
            cmp_code(0xFB, GSU_COPY(5));
            if (gsu.z) {
                gsu_code(0x01d3c8, 7);
                picture(0x3700, abi_FxDrawBitmap120x32, 0xD3DF, GSU_COPY(0));
                gsu_code(0x01d3df, 4); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d3c8, 6); gsu_code(0x01d3e3, 1); }
            cmp_code(0xFA, GSU_COPY(6));
            if (gsu.z) {
                gsu_code(0x01d3e4, 7);
                picture(0x3B00, abi_FxDrawBitmap120x16, 0xD3FB, GSU_COPY(1));
                gsu_code(0x01d3fb, 4); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d3e4, 6); gsu_code(0x01d3ff, 1); }
            cmp_code(0xF9, GSU_COPY(7));
            if (gsu.z) {
                gsu_code(0x01d400, 7);
                picture(0x1810, abi_FxDrawBitmap64x16, 0xD417, GSU_COPY(2));
                gsu_code(0x01d417, 4); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d400, 6); gsu_code(0x01d41b, 1); }
            cmp_code(0xF8, GSU_COPY(8));
            if (gsu.z) {
                gsu_code(0x01d41c, 7);
                picture(0x3700, abi_FxDrawBitmap120x24, 0xD433, GSU_COPY(3));
                gsu_code(0x01d433, 4); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d41c, 6); gsu_code(0x01d437, 1); }
            cmp_code(0xF7, GSU_COPY(9));
            if (gsu.z) {
                gsu_code(0x01d438, 7);
                picture(0x3A00, abi_FxDrawBitmap120x24, 0xD44F, GSU_COPY(4));
                gsu_code(0x01d44f, 4); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d438, 6); gsu_code(0x01d453, 1); }
            cmp_code(0xF6, GSU_COPY(10));
            if (gsu.z) {
                gsu_code(0x01d454, 7);
                picture(0x3D1C, abi_FxDrawBitmap16x24, 0xD46B, GSU_COPY(5));
                gsu_code(0x01d46b, 4); gsu_code(0x01d33b, 1);
                continue;
            } else { gsu_code(0x01d454, 6); gsu_code(0x01d46f, 1); }
        } else { gsu_code(0x01d34f, 6); gsu_code(0x01d46f, 1); }
        cmp_code(0x0D, GSU_COPY(11));
        if (gsu.z) {
            gsu.r[4] = gsu.r[7];
            gsu.r[2] = gsu_add(gsu.r[2], gsu.r[6], 0);
            gsu_code(0x01d470, 14); gsu_code(0x01d33b, 1);
            continue;
        } else { gsu_code(0x01d470, 6); gsu_code(0x01d47e, 1); }
        gsu.r[3] = 0x0030;
        gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[3], 0);
        if (!gsu.s) {
            gsu.r[3] = 0x0001;
            gsu_code(0x01d47f, 5); gsu_code(0x01d488, 6);
            gsu_romb(gsu.r[3]);
            gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
            gsu_moves(gsu.r[5]);
            if (gsu.z)
                { { gsu_code(0x01d48e, 6); glyph(FX_GLYPHS_BIG, abi_FxDrawBitmap16x32, 0xD4AC, GSU_COPY(0)); gsu_code(0x01d4ac, 5); gsu_code(0x01d4cc, 1); } }
            else
                { { gsu_code(0x01d48e, 5); gsu_code(0x01d4b1, 1); glyph(FX_GLYPHS_SMALL, abi_FxDrawBitmap8x16, 0xD4CA, GSU_COPY(1)); gsu_code(0x01d4ca, 3); } }
        } else { gsu_code(0x01d47f, 9); gsu_code(0x01d4cc, 1); }
        
        gsu.r[0] = 0x000A;
        gsu_moves(gsu.r[5]);
        if (gsu.z)
            { gsu.r[0] = 0x0014; gsu_code(0x01d4cd, 11); } else { gsu_code(0x01d4cd, 7); gsu_code(0x01d4d7, 1); }
        gsu.r[4] = gsu_add(gsu.r[4], gsu.r[0], 0);
        gsu_code(0x01d4d8, 5); gsu_code(0x01d33b, 1);
    }
gsu_code(0x01d34a, 4); gsu_code(0x01d4dd, 1);
end:
    ;
    gsu_code(0x01d4de, 2);
    gsu.r[0] = gsu_rpix((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
    gsu_sz(gsu.r[0]);
    gsu_code(0x01d4e0, 4);
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
    gsu_code(0x01d308, 7); gsu_code(0x01d311, 1);
    abi_FxTextDraw();
    gsu_code(0x01d30f, 1);
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
    gsu_code(0x01d130, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x01d132, 6);
    gsu.r[0] = gsu_ldw(0x273C);
    gsu.r[1] = 0x0001;
    gsu.r[0] = gsu_and(gsu.r[0], gsu.r[1]);
    if (gsu.z)
        { gsu_code(0x01d138, 10); gsu_code(0x01d1c6, 1);
        goto done; } else { gsu_code(0x01d138, 6); gsu_code(0x01d142, 1); }
    gsu_code(0x01d143, 4);
    gsu.r[0] = gsu_ldw(0x2648);
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        { gsu_code(0x01d147, 8); gsu_code(0x01d1c6, 1);
        goto done; } else { gsu_code(0x01d147, 4); gsu_code(0x01d14f, 1); }
    gsu_code(0x01d150, 4);
    gsu.r[1] = gsu_ldw(0x2476);
    gsu_sub(gsu.r[0], gsu.r[1], 0);     /* CMP */
    if (gsu.z)
        { gsu_code(0x01d154, 8); gsu_code(0x01d1c6, 1);
        goto done; } else { gsu_code(0x01d154, 4); gsu_code(0x01d15c, 1); }
    gsu_code(0x01d15d, 4);
    gsu.r[1] = gsu_ldw(0x2478);
    gsu_sub(gsu.r[0], gsu.r[1], 0);
    if (gsu.z)
        { gsu_code(0x01d161, 8); gsu_code(0x01d1c6, 1);
        goto done; } else { gsu_code(0x01d161, 4); gsu_code(0x01d169, 1); }
    gsu.r[2] = gsu.r[0];
    gsu_code(0x01d16a, 6);
    gsu.r[0] = gsu_ldw(0x264C);
    gsu_moves(gsu.r[0]);
    if (gsu.s)
        { gsu_code(0x01d170, 8); gsu_code(0x01d1c6, 1);
        goto done; } else { gsu_code(0x01d170, 4); gsu_code(0x01d178, 1); }
    gsu_code(0x01d179, 4);
    gsu.r[0] = gsu_ldw(0x264A);
    gsu.r[0] = gsu_div2(gsu.r[0]);
    gsu_code(0x01d17d, 6);
    gsu.r[1] = gsu_ldw(0x0034);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1], 0);
    if (gsu.s)
        { gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0); gsu_code(0x01d183, 5); } else { gsu_code(0x01d183, 3); gsu_code(0x01d187, 1); }
    
    gsu.r[4] = 0x0011;
    gsu_code(0x01d188, 6);
    gsu.r[1] = gsu_ldw(0x003A);
    gsu.r[1] = gsu_sub(gsu.r[1], gsu.r[4], 0);
    gsu_sub(gsu.r[0], gsu.r[1], 0);     /* CMP */
    if (!gsu.s)
        { gsu.r[0] = gsu.r[1]; gsu_code(0x01d18e, 9); } else { gsu_code(0x01d18e, 6); gsu_code(0x01d196, 1); }
    
    gsu_code(0x01d197, 4);
    gsu_stw(FX_TEXT_X, gsu.r[0]);
    gsu.r[0] = 0x0068;
    gsu_code(0x01d19b, 7);
    gsu_stw(FX_TEXT_Y, gsu.r[0]);
    gsu.r[0] = 0x0001;
    gsu_code(0x01d1a2, 6);
    gsu_stw(FX_TEXT_BANK, gsu.r[0]);
    gsu_code(0x01d1a8, 2);
    gsu_romb(gsu.r[0]);
    gsu.r[0] = 0x0096;
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[2], 0);
    gsu_code(0x01d1aa, 5);
    gsu.r[0] = gsu_ldw(gsu.r[0]);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01d1af, 4);
    gsu_r14(0xD1CB);
    gsu_code(0x01d1b3, 2);
    gsu_r14(gsu_add(gsu.r[14], gsu.r[0], 0));
    gsu_code(0x01d1b5, 1);
    gsu.r[0] = (gsu_getb());
    gsu_code(0x01d1b6, 1);
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu_code(0x01d1b7, 2);
    gsu.r[0] = gsu_getbh(gsu.r[0]);
    gsu_code(0x01d1b9, 4);
    gsu_stw(FX_TEXT_AT, gsu.r[0]);
    gsu.r[11] = 0xD1C1;
    gsu_code(0x01d1bd, 4); gsu_code(0x01d311, 1);
    (abi_FxTextDraw());
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01d1c1, 6);
    gsu_stw(FX_TEXT_AT, gsu.r[0]);
done:
    ;
    gsu_code(0x01d1c7, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}
