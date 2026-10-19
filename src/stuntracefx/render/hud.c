/*
 * hud.c: the Super FX programs in bank 07 that draw the 2D pieces of the
 * screen (glyphs and digits, the rising picture, scaled sprites, the noise
 * fill) into the Super FX's screen buffer. The S-CPU sets their inputs in
 * Super FX RAM and starts each one (gsu/programs.c); each ends at a STOP.
 *
 * Bitmaps are 2 bits a pixel in ROM bank $15, 32 bytes a row (128 pixels);
 * the low two bits of a byte are its leftmost pixel. A pixel's two bits
 * pick one of four colour bytes at $70:0200 (with the plot options at 0,
 * a colour of 0 leaves the screen as it was). The inputs are words in
 * Super FX RAM:
 *
 *   $020A   x (the glyph programs add their advance to it)
 *   $020C   y, or the height for the rising picture
 *   $020E   x advance, or the horizontal size for the scaled sprites
 *   $0210   rows, or the vertical size
 *   $0212   source step across, 8.8 (scaled sprites)
 *   $0214   source step down, 8.8 (scaled sprites)
 *   $0216   the bitmap's address in bank $15; 0 draws nothing
 *   $021C   the noise fill's random seed
 *   $003A, $003E  the screen's width and height (scaled sprites clip to them)
 *
 * The C follows the programs register for register: a program's inputs
 * and results are the Super FX's registers, flags and RAM, so each step
 * leaves them as the instruction it stands for does. Where a branch's
 * delay slot (the instruction after it, which always runs) matters, the C
 * runs it after the branch decision.
 */
#include "gsu_ops.h"

#define HUD_COLOURS     0x0200u  /* four colour bytes */
#define HUD_X           0x020Au
#define HUD_Y           0x020Cu
#define HUD_ADVANCE     0x020Eu  /* also the horizontal size */
#define HUD_ROWS        0x0210u  /* also the vertical size */
#define HUD_STEP_X      0x0212u
#define HUD_STEP_Y      0x0214u
#define HUD_BITMAP      0x0216u
#define HUD_NOISE_SEED  0x021Cu
#define SCREEN_WIDTH    0x003Au
#define SCREEN_HEIGHT   0x003Eu

#define BITMAP_BANK     0x15u
#define PICTURE         0x3D00u  /* the rising picture, in bank $15 */
#define NOISE_RAMP      0x3C00u  /* eight colour words for the noise */

#define R gsu.r

/* ---- instructions not in gsu_ops.h ---- */

/* MULT #n: the signed product of the low byte and n. */
static uint16_t mult_imm(uint16_t v, int8_t n)
{
    gsu_code(0x07FA4C, 5);
    uint16_t r = (uint16_t)((int8_t)v * n);

    gsu_sz(r);
    return r;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode times_pow2_code[7][6] = {
    {{0x07FA67, 12}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07FA73, 6}, {0xFA79, 0}, {0x07FA79, 15}, {0x07FAAD, 3}, {0x07FB25, 5}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07FA42, 10}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07FA51, 9}, {0x07FA78, 1}, {0xFA79, 0}, {0x07FA79, 15}, {0x07FAAD, 3}, {0x07FB25, 5}},
    {{0x07FB0B, 10}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07FB19, 10}, {0x07FB25, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

/* v * 2^n by n ADDs of the value to itself. */
static uint16_t times_pow2(uint16_t v, int n, int copy)
{
    gsu_codes(times_pow2_code[copy], 6);
    while (n--)
        v = gsu_add(v, v, 0);
    return v;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode loop_code[2][1] = {
    {{0, 0}},
    {{0x07F43A, 3}},
};

/* LOOP: counts R12 down; true while it has not reached 0. */
static int loop(int copy)
{
    gsu_codes(loop_code[copy], 1);
    return gsu_count_down(&R[12]);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode color_code[7][1] = {
    {{0x07F0E3, 2}},
    {{0x07F10F, 2}},
    {{0, 0}},
    {{0x07F403, 2}},
    {{0x07F42F, 2}},
    {{0x07FBB0, 2}},
    {{0x07F7E6, 7}},
};

/* COLOR, with the plot options' nibble modes. */
static void color(uint16_t v, int copy)
{
    gsu_codes(color_code[copy], 1);
    uint8_t c = (uint8_t)v;

    if (gsu.por & 0x04)
        c = (uint8_t)((gsu.colr & 0xF0) | c >> 4);
    else if (gsu.por & 0x08)
        c = (uint8_t)((gsu.colr & 0xF0) | (c & 0x0F));
    gsu.colr = c;
}

static void plot(void)
{
    gsu_plot((uint8_t)R[1], (uint8_t)R[2]);
    R[1]++;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode rpix_code[10][2] = {
    {{0, 0}, {0, 0}},
    {{0x07F43D, 3}, {0, 0}},
    {{0, 0}, {0x07FBB5, 5}},
    {{0, 0}, {0x07F6D3, 5}},
    {{0x07F5C2, 6}, {0, 0}},
    {{0x07F6DB, 3}, {0, 0}},
    {{0x07FBB3, 2}, {0x07FBB5, 5}},
    {{0x07FBBA, 3}, {0, 0}},
    {{0, 0}, {0x07F915, 5}},
    {{0, 0}, {0x07FA11, 5}},
};

/* RPIX: reads the pixel at (R1, R2), first writing out both pixel caches. */
static uint16_t rpix(int copy)
{
    gsu_codes(rpix_code[copy], 1);
    uint16_t r = gsu_rpix((uint8_t)R[1], (uint8_t)R[2]);

    gsu_sz(r);
    gsu_codes(rpix_code[copy] + 1, 1);
    return r;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode bitmap_setup_code[8][1] = {
    {{0x07F0C0, 18}},
    {{0, 0}},
    {{0x07F32A, 17}},
    {{0x07F15C, 17}},
    {{0x07F5CA, 16}},
    {{0x07FAB1, 16}},
    {{0x07F281, 17}},
    {{0x07F7F5, 17}},
};

/* ---- drawing from a bitmap ---- */

/* The setup every bitmap program makes: ROM bank $15 for GETB, RAM bank 0,
   plot options 0 (colour 0 transparent, no dither, no nibble modes). */
static void bitmap_setup(int copy)
{
    gsu_codes(bitmap_setup_code[copy], 1);
    R[0] = BITMAP_BANK;
    gsu.rombr = (uint8_t)(R[0] & 0x7F);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    gsu.por = (uint8_t)R[0];
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode pixel_code[4][1] = {
    {{0, 0}},
    {{0x07FBB0, 2}},
    {{0x07F7E6, 2}},
    {{0, 0}},
};

/* One pixel: colour byte (R0 AND 3) of the table at R11, plotted. */
static void pixel(int copy)
{
    R[0] = gsu_and(R[0], 3);
    R[0] = gsu_add(R[0], R[11], 0);
    R[0] = gsu_ldb(R[0]);
    gsu_codes(pixel_code[copy], 1);
    color(R[0], GSU_COPY(2));
    plot();
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode bitmap_byte_code[5][6] = {
    {{0, 0}, {0x07F0DC, 7}, {0x07F0E5, 5}, {0x07F0EA, 11}, {0, 0}, {0x07F111, 3}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x07F736, 11}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x07F7E6, 2}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07F3F9, 3}, {0x07F3FC, 7}, {0x07F405, 5}, {0x07F40A, 11}, {0, 0}, {0x07F431, 3}},
};

/* One bitmap byte at R14 as four pixels across, low bits first. R6 takes
   the byte and is rotated right two bits a pixel (through the carry, so
   its high byte collects the bits rotated out); R14 steps to the next
   byte after the first pixel. */
static void bitmap_byte(int copy)
{
    gsu_codes(bitmap_byte_code[copy], 1);
    R[6] = gsu_getb();
    R[0] = R[6];
    gsu_codes(bitmap_byte_code[copy] + 1, 1);
    pixel(GSU_COPY(0));
    R[6] = gsu_ror(R[6]);
    R[6] = gsu_ror(R[6]);
    gsu_codes(bitmap_byte_code[copy] + 2, 1);
    gsu_r14(gsu_inc(R[14]));
    gsu_codes(bitmap_byte_code[copy] + 3, 1);
    for (int k = 0; gsu_codes_loop(bitmap_byte_code[copy] + 4, 1, k < 3); k++) {
        R[0] = R[6];
        R[6] = gsu_ror(R[6]);
        R[6] = gsu_ror(R[6]);
        pixel(GSU_COPY(0));
        gsu_codes(bitmap_byte_code[copy] + 5, 1);
    }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode bitmap_row_end_code[7][2] = {
    {{0x07F7E8, 5}, {0, 0}},
    {{0x07F3C8, 5}, {0, 0}},
    {{0x07F276, 5}, {0x07F27B, 3}},
    {{0x07F6D3, 5}, {0x07F6D8, 3}},
    {{0x07F31F, 5}, {0x07F324, 3}},
    {{0, 0}, {0x07F6D8, 3}},
    {{0, 0}, {0x07FA16, 3}},
};

/* The end of a bitmap row: R14 on by `skip` to the next row's first byte,
   R1 back to the left edge R8, and the row count R12 down; R2 moves to the
   next line whether or not rows remain. True while rows remain. */
static int bitmap_row_end(uint16_t skip, int copy)
{
    gsu_codes(bitmap_row_end_code[copy], 1);
    int more;

    R[0] = skip;
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[1] = R[8];
    gsu_codes(bitmap_row_end_code[copy] + 1, 1);
    more = loop(GSU_COPY(0));
    R[2] = gsu_inc(R[2]);
    return more;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t bitmap_byte_from_glyph_rows[5] = {2, 3, 3, 0, 0};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode glyph_rows_code[5][6] = {
    {{0, 0}, {0, 0}, {0, 0}, {0x07F70B, 3}, {0x07F7ED, 3}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0x07F355, 3}, {0x07F3CD, 3}, {0, 0}},
    {{0x07F187, 4}, {0x07F18D, 4}, {0x07F191, 3}, {0, 0}, {0, 0}, {0x07F27E, 1}},
    {{0, 0}, {0, 0}, {0, 0}, {0x07F5EE, 3}, {0, 0}, {0, 0}},
    {{0x07F2A7, 5}, {0, 0}, {0x07F2AC, 3}, {0, 0}, {0, 0}, {0x07F327, 1}},
};

/* Glyph rows: R12 rows of `bytes` bytes (4 pixels each), R13 the row's
   start; with `rpix_each`, an RPIX after each byte writes the pixel caches
   out. Rows are 32 bytes apart. */
static void glyph_rows(uint16_t row_start, unsigned bytes, int rpix_each, int copy)
{
    gsu_codes(glyph_rows_code[copy], 2);
    R[13] = row_start;
    do {
        gsu_codes(glyph_rows_code[copy] + 2, 1);
        for (unsigned b = 0; b < bytes; b++) {
            bitmap_byte(GSU_COPY(bitmap_byte_from_glyph_rows[copy]));
            if (rpix_each)
                R[0] = rpix(GSU_COPY(0));
        }
    } while (gsu_codes_loop(glyph_rows_code[copy] + 3, 2, bitmap_row_end((uint16_t)(32 - bytes), GSU_COPY(5))));
    gsu_codes(glyph_rows_code[copy] + 5, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode glyph_x_code[6][4] = {
    {{0, 0}, {0x07F6F4, 7}, {0, 0}, {0, 0}},
    {{0, 0}, {0x07F33F, 7}, {0, 0}, {0, 0}},
    {{0x07F3E3, 4}, {0x07F3E7, 7}, {0x07F3EE, 4}, {0xF3F2, 0}},
    {{0x07F5DA, 4}, {0x07F5DE, 7}, {0x07F5E5, 4}, {0xF5E9, 0}},
    {{0x07F292, 4}, {0x07F296, 7}, {0x07F29D, 4}, {0, 0}},
    {{0x07F806, 4}, {0x07F80A, 7}, {0x07F811, 4}, {0, 0}},
};

/* The glyph programs' x: from $020A, with the advance added back to $020A
   for the next glyph. R8 and R1 = x. */
static void glyph_x(uint16_t advance, int copy)
{
    gsu_codes(glyph_x_code[copy], 1);
    R[8] = gsu_ldw(HUD_X);
    R[1] = R[8];
    R[0] = advance;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_codes(glyph_x_code[copy] + 1, 1);
    gsu_sbk(R[0]);
    gsu_codes(glyph_x_code[copy] + 2, 2);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode glyph_bitmap_code[3][1] = {
    {{0, 0}},
    {{0x07F2A1, 4}},
    {{0x07F815, 4}},
};

/* The bitmap's address into R14; false when it is 0 (nothing to draw). */
static int glyph_bitmap(int copy)
{
    gsu_r14(gsu_ldw(HUD_BITMAP));
    gsu_moves(R[14]);
    gsu_codes(glyph_bitmap_code[copy], 1);
    return !gsu.z;
}

/* The 2-bit to 4-bit tile conversion from R2 to R3: bytes are copied with
   a 0 after each (8 a tile, LDB and STB), then 16 zero bytes; R4 counts
   the 96 tiles down. */
static void tiles_2bpp_to_4bpp(void)
{
    gsu_code(0x07EE5F, 1); gsu_cache(0xEE60); gsu_code(0x07EE60, 9);
    int more;

    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    R[1] = 0;
    R[4] = 0x60;
    do {
        gsu_code(0x07EE69, 5);
        R[12] = 8;
        R[13] = 0xEE6E;
        do {
            gsu_code(0x07EE6E, 3);
            R[0] = gsu_ldb(R[2]);
            gsu_code(0x07EE71, 2);
            gsu_stb(R[3], (uint8_t)R[0]);
            R[2] = gsu_inc(R[2]);
            R[3] = gsu_inc(R[3]);
            R[0] = gsu_sub(R[0], R[0], 0);
            gsu_code(0x07EE73, 5);
            gsu_stb(R[3], (uint8_t)R[0]);
            gsu_code(0x07EE78, 1);
            more = loop(GSU_COPY(0));
            R[3] = gsu_inc(R[3]);
        } while (more);
        R[12] = 0x10;
        R[13] = 0xEE7E;
        gsu_code(0x07EE79, 5);
        do {
            gsu_code(0x07EE7E, 3);
            gsu_stb(R[3], (uint8_t)R[0]);
            gsu_code(0x07EE81, 1);
            more = loop(GSU_COPY(0));
            R[3] = gsu_inc(R[3]);
        } while (more);
        R[4] = gsu_dec(R[4]);
    } while (gsu_code_loop(0x07EE82, 4, !gsu.z));
    gsu_code(0x07EE82, 5);
}

/*
 * @routine 07:EE50 FxTiles2bppTo4bppHigh
 * WHAT IT DOES: Converts 96 2-bit tiles at $70:C000 into 4-bit tiles at
 * $70:D000: each tile's 16 bytes (two bit planes, 8 rows) become 32, the
 * upper two planes 0.
 * HOW: Bytes are copied with a 0 after each (8 a tile, LDB and STB), then 16
 * zero bytes; R4 counts the tiles down.
 * Entry: started by the S-CPU. Exit: R0 = 0; R1 = 0; R2 = $C300; R3 = $DC00;
 * R4 = 0; R12 = 0; R13 = $EE7E; RAM bank 0; the last RAM address $DBFF; CY
 * set, OV clear, S, Z from the tile count (Z set).
 */
void FxTiles2bppTo4bppHigh(void)
{
    gsu_code(0x07EE51, 8);
    R[2] = 0xC000;
    R[3] = 0xD000;
    tiles_2bpp_to_4bpp();
}

void abi_FxTiles2bppTo4bppHigh(void)
{
    FxTiles2bppTo4bppHigh();
}

/*
 * @routine 07:EE59 FxTiles2bppTo4bppLow
 * WHAT IT DOES: As FxTiles2bppTo4bppHigh, from $70:3000 into $70:4000.
 * HOW: As FxTiles2bppTo4bppHigh, with source $3000 and destination $4000.
 * Entry: started by the S-CPU. Exit: as FxTiles2bppTo4bppHigh, with R2 =
 * $3300, R3 = $4C00 and the last RAM address $4BFF.
 */
void FxTiles2bppTo4bppLow(void)
{
    gsu_code(0x07EE5A, 5);
    R[2] = 0x3000;
    R[3] = 0x4000;
    tiles_2bpp_to_4bpp();
}

void abi_FxTiles2bppTo4bppLow(void)
{
    FxTiles2bppTo4bppLow();
}

/* The noise fill's body (07:EEA0): R9 pixels across, R10 lines down, from
   the top left, the seed at $021C. */
static void noise_body(void)
{
    gsu_code(0x07EEA0, 5);
    int more;

    R[8] = gsu_ldw(HUD_NOISE_SEED);
    R[3] = 0xFF00;
    R[4] = 0xAA55;
    R[5] = 0x8180;
    R[6] = NOISE_RAMP;
    R[1] = 0;
    R[2] = 0;
    gsu_code(0x07EEA5, 18); gsu_cache(0xEEB7);
    do {
        gsu_code(0x07EEB7, 5);
        R[12] = R[9];
        R[13] = 0xEEBB;
        do {
            gsu_code(0x07EEBC, 18); gsu_code(0x07EED0, 5); gsu_code(0x07EEDF, 6);
            R[0] = gsu_lob(R[8]);
            R[0] = gsu_swap(R[0]);
            R[0] = gsu_xor(R[0], R[8]);
            R[0] = gsu_swap(R[0]);
            R[8] = R[0];
            R[0] = gsu_swap(R[0]);
            R[0] = gsu_lob(R[0]);
            R[0] = gsu_add(R[0], R[0], 0);
            R[0] = gsu_xor(R[0], R[8]);
            R[0] = gsu_lsr(R[0]);
            R[0] = gsu_xor(R[0], R[3]);
            if (gsu.cy) {
                gsu_code(0x07EECE, 2);
                R[0] = gsu_xor(R[0], R[5]);
                R[8] = R[0];
            } else {
                gsu_code(0x07EED5, 5);
                gsu_sub(R[0], R[4], 0);
                if (gsu.z)
                    R[0] = 0;
                R[8] = R[0];
            }
            R[0] = gsu_and(R[0], 7);
            R[0] = gsu_add(R[0], R[0], 0);
            R[0] = gsu_add(R[0], R[6], 0);
            R[0] = gsu_ldw(R[0]);
            gsu_code(0x07EEE5, 2);
            color(R[0], GSU_COPY(2));
            more = loop(GSU_COPY(0));
            plot();
        } while (more);
        R[1] = gsu_sub(R[1], R[1], 0);
        R[10] = gsu_dec(R[10]);
        more = !gsu.z;
        R[2] = gsu_inc(R[2]);
    } while (gsu_code_loop(0x07EEE7, 6, more));
    R[0] = rpix(gsu_code_then(0x07EEE7, 9, GSU_COPY(0)));
    gsu_code(0x07EEF0, 4);
    gsu_stw(HUD_NOISE_SEED, R[8]);
}

/*
 * @routine 07:EE88 FxNoiseFill
 * WHAT IT DOES: Fills the 16 x 128 pixels at the screen's top left with
 * random grey: each pixel's colour is one of the eight words at $70:3C00
 * (GsuEE88Ramp writes $80, $90, ... $F0 there), picked by a random number
 * seeded from and saved back to $70:021C.
 * HOW: The generator (seed in R8): t = the seed with its low byte XORed into
 * its high byte, its bytes swapped; the new seed is (twice the old low byte
 * XOR t) >> 1, XOR $FF00; when the bit shifted out is set it is XORed with
 * $8180, and when not, a value of $AA55 becomes 0. Its low three bits pick
 * the colour. RPIX at the end writes the pixel caches out.
 * Entry: started by the S-CPU. Exit: R0 = the pixel at (0, 128); R1 = 0; R2
 * = 128; R3 = $FF00; R4 = $AA55; R5 = $8180; R6 = $3C00; R8 = the new seed;
 * R9 = 16; R10 = 0; R12 = 0; R13 = $EEBB; plot options 0; the colour of the
 * last pixel; the last RAM address $021C; CY set, OV clear, S, Z from the
 * RPIX.
 */
void FxNoiseFill(void)
{
    gsu_code(0x07EE89, 11);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.por = (uint8_t)R[0];
    R[9] = 0x10;
    R[10] = 0x80;
    noise_body();
}

void abi_FxNoiseFill(void)
{
    FxNoiseFill();
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode picture_rows_code[2][8] = {
    {{0x07F0BC, 4}, {0x07F0D2, 3}, {0x07F0D5, 4}, {0xF0D9, 0}, {0x07F0D9, 3}, {0x07F114, 6}, {0x07F11A, 3}, {0x07F11D, 3}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

/* The rising picture: the top `height` ($020C) rows of the picture at
   $15:3D00 + src, `bytes` bytes (4 pixels each) of each row, drawn so
   that the last row lands just above line `base`, at x = R8. R3 counts
   each row's bytes; R13 is the row's start. RPIX at the end writes the
   pixel caches out. */
static void picture_rows(uint16_t src, uint16_t row_start, uint16_t bytes, int copy)
{
    gsu_codes(picture_rows_code[copy], 1);
    int more;

    R[1] = R[8];
    gsu_r14((uint16_t)(PICTURE + src));
    R[12] = R[3];
    R[13] = row_start;
    gsu_codes(picture_rows_code[copy] + 1, 1);
    do {
        gsu_codes(picture_rows_code[copy] + 2, 2);
        R[3] = bytes;
        do {
            gsu_codes(picture_rows_code[copy] + 4, 1);
            bitmap_byte(GSU_COPY(0));
            R[3] = gsu_dec(R[3]);
        } while (gsu_code_loop(0x07F111, 3, !gsu.z));
        R[0] = (uint16_t)(32 - bytes);
        gsu_codes(picture_rows_code[copy] + 5, 1);
        gsu_r14(gsu_add(R[0], R[14], 0));
        R[1] = R[8];
        gsu_codes(picture_rows_code[copy] + 6, 1);
        more = loop(GSU_COPY(0));
        R[2] = gsu_inc(R[2]);
    } while (more);
    R[0] = rpix(gsu_codes_then(picture_rows_code[copy] + 7, 1, GSU_COPY(0)));
}

/* A rising picture in two pieces: picture bytes 14-27 at x = left, then
   bytes 0-9 at x = left + $38, both above line `base`. */
static void picture_two_pieces(uint16_t left, uint16_t base, uint16_t start1,
                               uint16_t start2)
{
    R[8] = left;
    bitmap_setup(GSU_COPY(1));
    R[11] = HUD_COLOURS;
    R[3] = gsu_ldw(HUD_Y);
    R[0] = base;
    R[2] = gsu_sub(R[0], R[3], 0);
    picture_rows(0x0E, start1, 0x0E, GSU_COPY(0));

    bitmap_setup(GSU_COPY(0));
    R[11] = HUD_COLOURS;
    R[8] = (uint16_t)(left + 0x38);
    R[3] = gsu_ldw(HUD_Y);
    R[1] = R[8];
    R[0] = base;
    R[2] = gsu_sub(R[0], R[3], 0);
    picture_rows(0x00, start2, 0x0A, GSU_COPY(1));
}

/*
 * @routine 07:EEF5 FxPictureRiseSplit40
 * WHAT IT DOES: Draws the picture at $15:3D00 rising out of line $40, the
 * top $70:020C rows of it, in two pieces: picture columns 56-111 at x = $1C
 * and columns 0-39 at x = $54.
 * HOW: Each piece: y = $40 - height; rows of 14 (then 10) bitmap bytes, 32
 * bytes apart, each byte four pixels through the colour table at $70:0200;
 * RPIX after each piece.
 * Entry: started by the S-CPU; the colour bytes and the height set. Exit: R0
 * = the pixel at ($54, $40); R1 = R8 = $54; R2 = $40; R3 = 0; R6 = the last
 * byte rotated right 8 times through the carry; R11 = $0200; R12 = 0; R13 =
 * $EF81; R14 past the last row; ROM bank $15, RAM bank 0, plot options 0;
 * the last RAM address that of the last colour byte read.
 */
void FxPictureRiseSplit40(void)
{
    gsu_code(0x07EEF6, 19);
    picture_two_pieces(0x1C, 0x40, 0xEF16, 0xEF81);
}

void abi_FxPictureRiseSplit40(void)
{
    FxPictureRiseSplit40();
}

/*
 * @routine 07:EFCD FxPictureRiseSplit60
 * WHAT IT DOES: As FxPictureRiseSplit40, rising out of line $60, the pieces
 * at x = $34 and $6C.
 * HOW: As FxPictureRiseSplit40.
 * Entry: as FxPictureRiseSplit40. Exit: as FxPictureRiseSplit40, with R0 =
 * the pixel at ($6C, $60), R1 = R8 = $6C, R2 = $60 and R13 = $F059.
 */
void FxPictureRiseSplit60(void)
{
    gsu_code(0x07EFCE, 19);
    picture_two_pieces(0x34, 0x60, 0xEFEE, 0xF059);
}

void abi_FxPictureRiseSplit60(void)
{
    FxPictureRiseSplit60();
}

/* A rising picture in one piece: picture bytes 0-13 at x = left, above
   line `base`. */
static void picture_one_piece(uint16_t left, uint16_t base)
{
    R[8] = left;
    R[3] = gsu_ldw(HUD_Y);
    R[0] = base;
    R[2] = gsu_sub(R[0], R[3], 0);
    bitmap_setup(GSU_COPY(0));
    R[11] = HUD_COLOURS;
    picture_rows(0x00, 0xF0D5, 0x0E, GSU_COPY(0));
}

/*
 * @routine 07:F0A5 FxPictureRise40
 * WHAT IT DOES: Draws the picture at $15:3D00 rising out of line $40, the
 * top $70:020C rows of its first 56 columns, at x = $28.
 * HOW: y = $40 - height; rows of 14 bitmap bytes, 32 bytes apart, each byte
 * four pixels through the colour table at $70:0200; RPIX at the end.
 * Entry: started by the S-CPU; the colour bytes and the height set. Exit: R0
 * = the pixel at ($28, $40); R1 = R8 = $28; R2 = $40; R3 = 0; R6 = the last
 * byte rotated right 8 times through the carry; R11 = $0200; R12 = 0; R13 =
 * $F0D5; R14 past the last row; ROM bank $15, RAM bank 0, plot options 0;
 * the last RAM address that of the last colour byte read.
 */
void FxPictureRise40(void)
{
    gsu_code(0x07F0A6, 7);
    picture_one_piece(0x28, 0x40);
}

void abi_FxPictureRise40(void)
{
    FxPictureRise40();
}

/*
 * @routine 07:F0B4 FxPictureRise60
 * WHAT IT DOES: As FxPictureRise40, rising out of line $60, at x = $4C.
 * HOW: As FxPictureRise40.
 * Entry: as FxPictureRise40. Exit: as FxPictureRise40, with R0 = the pixel
 * at ($4C, $60), R1 = R8 = $4C and R2 = $60.
 */
void FxPictureRise60(void)
{
    gsu_code(0x07F0B5, 7);
    picture_one_piece(0x4C, 0x60);
}

void abi_FxPictureRise60(void)
{
    FxPictureRise60();
}

/*
 * @routine 07:F121 FxGlyph16RowsFromBottom
 * WHAT IT DOES: Draws the bottom $70:0210 rows of the 16 x 32 glyph at
 * $70:0216 (in bank $15) at ($70:020A, $70:020C), and moves x on by
 * $70:020E; nothing is drawn when the glyph address is 0. With
 * FxGlyph16RowsFromTop it rolls a digit into its cell.
 * HOW: The glyph's start moves down 32 - rows rows; each row is four bitmap
 * bytes through the colour table at $70:0200, with an RPIX after each byte.
 * Entry: started by the S-CPU. Exit: when drawn, R0 = $1C; R1 = R8 = x; R2 =
 * y + rows; R3 = rows; R6 = the last byte rotated right 8 times through the
 * carry; R11 = $0200; R12 = 0; R13 = $F191; R14 past the last row; ROM bank
 * $15, RAM bank 0, plot options 0; the last RAM address that of the last
 * colour byte read. When not drawn: R0 = x + the advance; R1 = R8 = x; R14 =
 * 0; the last RAM address $0216; OV, S clear, Z set.
 */
void FxGlyph16RowsFromBottom(void)
{
    gsu_code(0x07F122, 14);
    bitmap_setup(GSU_COPY(0));
    R[0] = gsu_ldw(HUD_ADVANCE);
    R[8] = gsu_ldw(gsu_code_then(0x07F130, 4, HUD_X));
    R[1] = R[8];
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07F134, 4);
    gsu_sbk(R[0]);
    gsu_code(0x07F138, 4);
    if (!glyph_bitmap(GSU_COPY(0)))
        { gsu_code(0x07F13C, 5); return; } else { gsu_code(0x07F13C, 4); gsu_code(0x07F142, 8); }
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    R[3] = gsu_ldw(gsu_code_then(0x07F14A, 4, HUD_ROWS));
    R[0] = 0x20;
    R[0] = gsu_sub(R[0], R[3], 0);
    R[0] = times_pow2(R[0], 5, gsu_code_then(0x07F14E, 11, GSU_COPY(0)));
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[12] = R[3];
    gsu_code(0x07F159, 2); gsu_code(0x07F18D, 4);
    glyph_rows(0xF191, 4, 1, GSU_COPY(0));
    gsu_code(0x07F27E, 1);
}

void abi_FxGlyph16RowsFromBottom(void)
{
    FxGlyph16RowsFromBottom();
}

/*
 * @routine 07:F15B FxGlyph16RowsFromTop
 * WHAT IT DOES: Draws the top $70:0210 rows of the 16 x 32 glyph at $70:0216
 * (in bank $15) at the bottom of the 32-line cell at ($70:020A, $70:020C),
 * and moves x on by $70:020E; nothing is drawn when the glyph address is 0.
 * With $0210 = 32 it draws the whole glyph (digits).
 * HOW: y = cell y + 32 - rows; each row is four bitmap bytes through the
 * colour table at $70:0200, with an RPIX after each byte.
 * Entry: started by the S-CPU. Exit: when drawn, as FxGlyph16RowsFromBottom,
 * with R2 = cell y + 32. When not drawn: R0 = x + the advance; R1 = R8 = x;
 * R2 = cell y + 32 - rows; R3 = rows; R11 = $0200; R14 = 0; the last RAM
 * address $0216; OV, S clear, Z set.
 */
void FxGlyph16RowsFromTop(void)
{
    bitmap_setup(GSU_COPY(3));
    R[11] = HUD_COLOURS;
    R[3] = gsu_ldw(HUD_ROWS);
    R[2] = gsu_ldw(gsu_code_then(0x07F16D, 4, HUD_Y));
    R[0] = 0x20;
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_sub(R[0], R[3], 0);
    R[0] = gsu_ldw(gsu_code_then(0x07F171, 10, HUD_ADVANCE));
    R[8] = gsu_ldw(gsu_code_then(0x07F17B, 4, HUD_X));
    R[1] = R[8];
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07F17F, 4);
    gsu_sbk(R[0]);
    gsu_code(0x07F183, 4);
    if (!glyph_bitmap(GSU_COPY(0)))
        { gsu_code(0x07F187, 5); return; }
    R[12] = R[3];
    glyph_rows(0xF191, 4, 1, GSU_COPY(2));
}

void abi_FxGlyph16RowsFromTop(void)
{
    FxGlyph16RowsFromTop();
}

/*
 * @routine 07:F329 FxGlyph8x16
 * WHAT IT DOES: Draws the 8 x 16 glyph at $70:0216 (in bank $15) at
 * ($70:020A, $70:020C) and moves x on by 8; nothing is drawn when the glyph
 * address is 0.
 * HOW: 16 rows of two bitmap bytes through the colour table at $70:0200,
 * with an RPIX after each byte. The address-0 exit has not been reached by
 * any script yet.
 * Entry: started by the S-CPU. Exit: when drawn, R0 = $1E; R1 = R8 = x; R2 =
 * y + 16; R6 = the last byte rotated right 8 times through the carry; R11 =
 * $0200; R12 = 0; R13 = $F355; R14 past the last row; ROM bank $15, RAM bank
 * 0, plot options 0; the last RAM address that of the last colour byte read.
 */
void FxGlyph8x16(void)
{
    bitmap_setup(GSU_COPY(2));
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    gsu_code(0x07F33B, 4);
    glyph_x(8, GSU_COPY(1));
    gsu_code(0x07F346, 4);
    if (!glyph_bitmap(GSU_COPY(0)))
        { gsu_code(0x07F34A, 5); return; } else { gsu_code(0x07F34A, 4); gsu_code(0x07F350, 8); }                     /* not reached in the traces so far */
    R[12] = 0x10;
    glyph_rows(0xF355, 2, 1, GSU_COPY(1));
    gsu_code(0x07F3CD, 4);
}

void abi_FxGlyph8x16(void)
{
    FxGlyph8x16();
}

/*
 * @routine 07:F441 FxGlyph24x16
 * WHAT IT DOES: Draws the 24 x 16 glyph at $70:0216 (in bank $15) at
 * ($70:020A, $70:020C) and moves x on by 16 (the player marks).
 * HOW: 16 rows of six bitmap bytes through the colour table at $70:0200,
 * with an RPIX after each byte and one more at the end. The glyph address is
 * not checked.
 * Entry: started by the S-CPU. Exit: R0 = the pixel at (x, y + 16); R1 = R8
 * = x; R2 = y + 16; R6 = the last byte rotated right 8 times through the
 * carry; R11 = $0200; R12 = 0; R13 = $F466; R14 past the last row; ROM bank
 * $15, RAM bank 0, plot options 0; the last RAM address that of the last
 * colour byte read.
 */
void FxGlyph24x16(void)
{
    gsu_code(0x07F442, 16);
    bitmap_setup(GSU_COPY(0));
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    gsu_code(0x07F452, 4);
    glyph_x(0x10, GSU_COPY(0));
    gsu_code(0x07F45D, 4); gsu_cache(0xF461);
    gsu_r14(gsu_ldw(HUD_BITMAP));
    R[12] = 0x10;
    gsu_code(0x07F461, 8);
    glyph_rows(0xF466, 6, 1, GSU_COPY(0));
    R[0] = rpix(GSU_COPY(4));
}

void abi_FxGlyph24x16(void)
{
    FxGlyph24x16();
}

/*
 * @routine 07:F6DF FxGlyph16x32
 * WHAT IT DOES: Draws the 16 x 32 glyph at $70:0216 (in bank $15) at
 * ($70:020A, $70:020C) and moves x on by 16; nothing is drawn when the glyph
 * address is 0.
 * HOW: 32 rows of four bitmap bytes through the colour table at $70:0200;
 * one RPIX at the end. The address-0 exit has not been reached by any script
 * yet.
 * Entry: started by the S-CPU. Exit: when drawn, R0 = the pixel at (x, y +
 * 32); R1 = R8 = x; R2 = y + 32; R6 = the last byte rotated right 8 times
 * through the carry; R11 = $0200; R12 = 0; R13 = $F70B; R14 past the last
 * row; ROM bank $15, RAM bank 0, plot options 0; the last RAM address that
 * of the last colour byte read.
 */
void FxGlyph16x32(void)
{
    gsu_code(0x07F6E0, 16);
    bitmap_setup(GSU_COPY(1));
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    gsu_code(0x07F6F0, 4);
    glyph_x(0x10, GSU_COPY(0));
    gsu_code(0x07F6FB, 4);
    if (!glyph_bitmap(GSU_COPY(0)))
        { gsu_code(0x07F6FF, 5); return; } else { gsu_code(0x07F6FF, 4); gsu_code(0x07F705, 1); gsu_cache(0xF706); gsu_code(0x07F706, 8); }                     /* not reached in the traces so far */
    R[12] = 0x20;
    glyph_rows(0xF70B, 4, 0, GSU_COPY(0));
    R[0] = rpix(gsu_code_then(0x07F7ED, 6, GSU_COPY(0)));
}

void abi_FxGlyph16x32(void)
{
    FxGlyph16x32();
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t pixel_from_scaled_sprite[5] = {0, 0, 1, 3, 0};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode scaled_sprite_code[5][36] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x07FB2A, 8}, {0, 0}, {0x07FB32, 14}, {0x07FB43, 5}, {0x07FB50, 5}, {0, 0}, {0, 0}, {0, 0}, {0x07FB55, 13}, {0x07FB64, 5}, {0x07FB6E, 7}, {0, 0}, {0, 0}, {0, 0}, {0x07FB75, 4}, {0xFB79, 0}, {0x07FB79, 4}, {0x07FB7D, 5}, {0x07FB82, 16}, {0x07FB82, 4}, {0x07FB91, 1}, {0x07FB92, 12}, {0x07FB9E, 1}, {0x07FB9F, 2}, {0x07FBAA, 6}, {0x07FBA1, 5}, {0x07FBA6, 4}, {0x07FBB2, 1}, {0x07FBB3, 2}, {0x07FB78, 1}, {0xFB79, 0}, {0x07FBBA, 3}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x07FB2A, 8}, {0x07FB48, 6}, {0x07FB32, 14}, {0x07FB43, 5}, {0x07FB50, 5}, {0, 0}, {0, 0}, {0x07FB69, 5}, {0x07FB59, 9}, {0x07FB64, 4}, {0, 0}, {0x07FB55, 4}, {0x07FB68, 1}, {0x07FB6E, 7}, {0x07FB75, 4}, {0xFB79, 0}, {0x07FB79, 4}, {0x07FB7D, 5}, {0x07FB82, 16}, {0x07FB82, 4}, {0x07FB91, 1}, {0x07FB92, 12}, {0x07FB9E, 1}, {0x07FB9F, 2}, {0x07FBAA, 6}, {0x07FBA1, 5}, {0x07FBA6, 4}, {0x07FBB2, 1}, {0x07FBB3, 2}, {0x07FB78, 1}, {0xFB79, 0}, {0x07FBBA, 3}},
    {{0x07FAEB, 5}, {0x07FB25, 5}, {0, 0}, {0, 0}, {0x07FB2A, 8}, {0x07FB48, 8}, {0x07FB36, 10}, {0x07FB43, 5}, {0, 0}, {0x07FB32, 4}, {0x07FB50, 5}, {0, 0}, {0x07FB55, 13}, {0x07FB64, 5}, {0x07FB6E, 7}, {0, 0}, {0, 0}, {0, 0}, {0x07FB75, 4}, {0xFB79, 0}, {0x07FB79, 4}, {0x07FB7D, 5}, {0x07FB82, 16}, {0x07FB82, 4}, {0x07FB91, 1}, {0x07FB92, 12}, {0x07FB9E, 1}, {0x07FB9F, 2}, {0x07FBAA, 6}, {0x07FBA1, 5}, {0x07FBA6, 4}, {0x07FBB2, 1}, {0x07FBB3, 2}, {0x07FB78, 1}, {0xFB79, 0}, {0x07FBBA, 3}},
    {{0, 0}, {0, 0}, {0x07FB23, 1}, {0x07FB2A, 4}, {0x07FB2A, 8}, {0x07FB48, 8}, {0x07FB36, 10}, {0x07FB43, 5}, {0, 0}, {0x07FB32, 4}, {0x07FB50, 5}, {0, 0}, {0x07FB55, 13}, {0x07FB64, 5}, {0x07FB6E, 7}, {0, 0}, {0, 0}, {0, 0}, {0x07FB75, 4}, {0xFB79, 0}, {0x07FB79, 4}, {0x07FB7D, 5}, {0x07FB82, 16}, {0x07FB82, 4}, {0x07FB91, 1}, {0x07FB92, 12}, {0x07FB9E, 1}, {0x07FB9F, 7}, {0x07FBAA, 6}, {0, 0}, {0, 0}, {0x07FBB0, 3}, {0x07FBB3, 2}, {0x07FB78, 1}, {0xFB79, 0}, {0x07FBBA, 3}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x07FB2A, 8}, {0x07FB48, 6}, {0x07FB36, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

/* ---- scaled sprites ---- */

/* The shared body of the scaled-sprite programs (07:FB25). Draws a sprite
   R5 pixels wide and R7 high at (R8, R9) from the bitmap at $70:0216,
   stepping through the source by $0212 across and $0214 down (8.8 fixed
   point), clipped to the screen ($003A wide, $003E high). Each pixel's
   source is row ((y - top) * step down) and column ((x - left) * step
   across) of the bitmap, 4 pixels a byte; RPIX after each row and at the
   end. A sprite off the screen, or with bitmap 0, draws nothing. */
static void scaled_sprite(int copy)
{
    gsu_codes(scaled_sprite_code[copy], 2);
    int more, zero;

    gsu_r14(gsu_ldw(HUD_BITMAP));
    gsu_moves(R[14]);
    if (gsu.z)
        { gsu_codes(scaled_sprite_code[copy] + 2, 2); return; } else { gsu_codes(scaled_sprite_code[copy] + 4, 1); }
    R[3] = gsu_ldw(SCREEN_WIDTH);
    gsu_moves(R[8]);
    if (gsu.s) {
        gsu_codes(scaled_sprite_code[copy] + 5, 1);
        /* Off the left edge: the part right of x = 0. */
        R[5] = gsu_add(R[8], R[5], 0);
        if (gsu.s)
            return;
        R[1] = gsu_sub(R[1], R[1], 0);
    } else {
        gsu_codes(scaled_sprite_code[copy] + 6, 3);
        R[0] = gsu_sub(R[8], R[3], 0);
        if (gsu.cy)
            return;
        R[0] = gsu_add(R[8], R[5], 0);
        R[0] = gsu_sub(R[0], R[3], 0);
        if (gsu.cy)
            R[5] = gsu_sub(R[3], R[8], 0);  /* off the right edge */
        R[1] = R[8];
    }
    R[4] = gsu_ldw(gsu_codes_then(scaled_sprite_code[copy] + 9, 2, SCREEN_HEIGHT));
    gsu_moves(R[9]);
    if (gsu.s) {
        gsu_codes(scaled_sprite_code[copy] + 11, 1);
        /* Off the top: the part below y = 0. */
        R[7] = gsu_add(R[9], R[7], 0);
        if (gsu.s) {
            R[2] = rpix(GSU_COPY(0));          /* a WITH R2 in the delay slot */
            return;
        }
        R[2] = gsu_sub(R[2], R[2], 0);
    } else {
        gsu_codes(scaled_sprite_code[copy] + 12, 3);
        R[0] = gsu_sub(R[9], R[4], 0);
        if (gsu.cy)
            return;
        R[0] = gsu_add(R[9], R[7], 0);
        R[0] = gsu_sub(R[0], R[4], 0);
        if (gsu.cy)
            R[7] = gsu_sub(R[4], R[9], 0);  /* off the bottom */
        R[2] = R[9];
    }
    R[10] = R[1];
    R[3] = gsu_ldw(gsu_codes_then(scaled_sprite_code[copy] + 15, 3, HUD_STEP_X));
    R[4] = gsu_ldw(gsu_codes_then(scaled_sprite_code[copy] + 18, 2, HUD_STEP_Y));
    do {
        gsu_codes(scaled_sprite_code[copy] + 20, 1);
        R[12] = R[5];
        R[13] = 0xFB7D;
        do {
            gsu_codes(scaled_sprite_code[copy] + 21, 1);
            gsu_r14(gsu_ldw(HUD_BITMAP));
            R[0] = gsu_sub(R[2], R[9], 0);
            if (!gsu.z) {
                gsu_codes(scaled_sprite_code[copy] + 22, 1);
                R[0] = gsu_swap(R[0]);
                R[6] = R[4];
                R[0] = gsu_fmult(R[0], R[6]);
                R[0] = times_pow2(R[0], 5, GSU_COPY(2));
                gsu_r14(gsu_add(R[0], R[14], 0));
            } else { gsu_codes(scaled_sprite_code[copy] + 23, 2); }
            R[0] = gsu_sub(R[1], R[8], 0);
            R[0] = gsu_swap(R[0]);
            R[6] = R[3];
            R[0] = gsu_fmult(R[0], R[6]);
            R[6] = R[0];
            R[0] = gsu_asr(R[0]);
            R[0] = gsu_asr(R[0]);
            gsu_codes(scaled_sprite_code[copy] + 25, 1);
            gsu_r14(gsu_add(R[0], R[14], 0));
            R[0] = (gsu_codes(scaled_sprite_code[copy] + 26, 1), gsu_getb());
            R[6] = gsu_and(R[6], 3);
            gsu_codes(scaled_sprite_code[copy] + 27, 2);
            for (;;) {
                gsu_codes(scaled_sprite_code[copy] + 29, 1);
                gsu_moves(R[6]);
                zero = gsu.z;
                R[6] = gsu_dec(R[6]);
                if (zero)
                    break; else { gsu_codes(scaled_sprite_code[copy] + 30, 1); }
                R[0] = gsu_asr(R[0]);
                R[0] = gsu_asr(R[0]);
            }
            pixel(GSU_COPY(pixel_from_scaled_sprite[copy]));
            gsu_codes(scaled_sprite_code[copy] + 31, 1);
            more = loop(GSU_COPY(0));
        } while (more);
        R[0] = rpix(gsu_codes_then(scaled_sprite_code[copy] + 32, 1, GSU_COPY(2)));
        R[1] = gsu_sub(R[1], R[5], 0);
        R[7] = gsu_dec(R[7]);
        more = !gsu.z;
        R[2] = gsu_inc(R[2]);
    } while (gsu_codes_loop(scaled_sprite_code[copy] + 33, 2, more));
    R[0] = rpix(gsu_codes_then(scaled_sprite_code[copy] + 35, 1, GSU_COPY(0)));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode scaled_setup_code[2][1] = {
    {{0, 0}},
    {{0x07FAF3, 16}},
};

/* The scaled-sprite programs' setup after their size is worked out. */
static void scaled_setup(int copy)
{
    gsu_codes(scaled_setup_code[copy], 1);
    bitmap_setup(GSU_COPY(1));
    R[11] = HUD_COLOURS;
}

/* FxScaledSprite13 and FxScaledSprite12 from their height (07:FA4C): y,
   and (size down) / 16 lines, then the shared body. */
static void scaled_height_16(void)
{
    R[9] = gsu_ldw(HUD_Y);
    R[0] = gsu_add(R[4], R[4], 0);
    R[0] = times_pow2(R[0], 3, GSU_COPY(4));
    R[7] = gsu_hib(R[0]);
    scaled_setup(GSU_COPY(0));
    scaled_sprite(GSU_COPY(0));
}

/*
 * @routine 07:FA1B FxScaledSprite13
 * WHAT IT DOES: Draws a scaled sprite at ($70:020A, $70:020C), (size across
 * / 2) * 13 / 16 pixels wide and (size down) / 16 high, the sizes at
 * $70:020E and $70:0210.
 * HOW: Width by DIV2 and MULT #13 and four doublings, the high byte; height
 * by four doublings, the high byte; then the shared scaled-sprite body (see
 * scaled_sprite: clipped to the screen, the source stepped by $70:0212 and
 * $70:0214, RPIX after each row and at the end).
 * Entry: started by the S-CPU. Exit: R1 = R10 = the left edge; R2 = the line
 * below the sprite; R3, R4 = the steps; R5 = the width; R7 = 0; R8, R9 = x,
 * y; R11 = $0200; R12 = 0; R13 = $FB7D; R0 = the pixel at the left edge of
 * that line; ROM bank $15, RAM bank 0, plot options 0. A sprite clipped away
 * or with bitmap 0 stops early with the registers as they are then.
 */
void FxScaledSprite13(void)
{
    gsu_code(0x07FA1C, 4);
    R[3] = gsu_ldw(HUD_ADVANCE);
    R[4] = gsu_ldw(gsu_code_then(0x07FA20, 4, HUD_ROWS));
    R[8] = gsu_ldw(gsu_code_then(0x07FA24, 4, HUD_X));
    R[0] = gsu_div2(R[3]);
    R[0] = mult_imm(R[0], gsu_code_then(0x07FA28, 13, 13));
    R[0] = times_pow2(R[0], 4, GSU_COPY(0));
    R[5] = gsu_hib(R[0]);
    scaled_height_16();
}

void abi_FxScaledSprite13(void)
{
    FxScaledSprite13();
}

/*
 * @routine 07:FA5A FxScaledSpriteEighth
 * WHAT IT DOES: Draws a scaled sprite at ($70:020A, $70:020C), (size across)
 * / 32 pixels wide and (size down) / 32 high, the sizes at $70:020E and
 * $70:0210.
 * HOW: Each size by three doublings, the high byte; then the shared
 * scaled-sprite body (see FxScaledSprite13).
 * Entry: started by the S-CPU. Exit: as FxScaledSprite13.
 */
void FxScaledSpriteEighth(void)
{
    gsu_code(0x07FA5B, 4);
    R[3] = gsu_ldw(HUD_ADVANCE);
    R[4] = gsu_ldw(gsu_code_then(0x07FA5F, 4, HUD_ROWS));
    R[8] = gsu_ldw(gsu_code_then(0x07FA63, 4, HUD_X));
    R[0] = gsu_add(R[3], R[3], 0);
    R[0] = times_pow2(R[0], 2, GSU_COPY(0));
    R[5] = gsu_hib(R[0]);
    R[0] = gsu_add(R[5], R[8], 0);
    R[9] = gsu_ldw(HUD_Y);
    R[0] = gsu_add(R[4], R[4], 0);
    R[0] = times_pow2(R[0], 2, GSU_COPY(1));
    R[7] = gsu_hib(R[0]);
    scaled_setup(GSU_COPY(0));
    scaled_sprite(GSU_COPY(0));
}

void abi_FxScaledSpriteEighth(void)
{
    FxScaledSpriteEighth();
}

/*
 * @routine 07:FA88 FxSprite24x40
 * WHAT IT DOES: Draws a 24 x 40 sprite at ($70:020A, $70:020C), stepping
 * through the source by $70:0212 and $70:0214, and moves x on by 24.
 * HOW: Width $18 and height $28, then the shared scaled-sprite body (see
 * FxScaledSprite13).
 * Entry: started by the S-CPU. Exit: as FxScaledSprite13.
 */
void FxSprite24x40(void)
{
    gsu_code(0x07FA89, 22);
    scaled_setup(GSU_COPY(0));
    R[3] = 0x0100;
    R[4] = 0x0100;
    R[8] = gsu_ldw(HUD_X);
    R[0] = 0x18;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07FA9F, 5);
    gsu_sbk(R[0]);
    R[5] = 0x18;
    R[9] = gsu_ldw(gsu_code_then(0x07FAA4, 7, HUD_Y));
    R[7] = 0x28;
    gsu_code(0x07FAAB, 5); gsu_code(0x07FB25, 5);
    scaled_sprite(GSU_COPY(0));
}

void abi_FxSprite24x40(void)
{
    FxSprite24x40();
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxScaledSpriteAdvance_code[2][6] = {
    {{0x07FB03, 4}, {0x07FB07, 4}, {0x07FB0B, 10}, {0x07FB15, 4}, {0x07FB19, 10}, {0x07FB25, 5}},
    {{0x07FB03, 4}, {0x07FB07, 4}, {0x07FB0B, 10}, {0x07FB15, 4}, {0x07FB19, 10}, {0x07FB25, 5}},
};

/*
 * @routine 07:FAF2 FxScaledSpriteAdvance
 * WHAT IT DOES: Draws a scaled sprite at ($70:020A, $70:020C), (size across)
 * / 16 pixels wide and (size down) / 8 high, the sizes at $70:020E and
 * $70:0210, and moves x on by its width.
 * HOW: Width by four doublings and height by five, each the high byte; the
 * width added to $020A; then the shared scaled-sprite body (see
 * FxScaledSprite13).
 * Entry: started by the S-CPU. Exit: as FxScaledSprite13.
 */
void FxScaledSpriteAdvance(int copy)
{
    gsu_code(0x07FAF3, 16);
    scaled_setup(GSU_COPY(0));
    R[3] = gsu_ldw(HUD_ADVANCE);
    R[4] = gsu_ldw(gsu_codes_then(FxScaledSpriteAdvance_code[copy], 1, HUD_ROWS));
    R[8] = gsu_ldw(gsu_codes_then(FxScaledSpriteAdvance_code[copy] + 1, 1, HUD_X));
    R[0] = gsu_add(R[3], R[3], 0);
    R[0] = times_pow2(R[0], 3, gsu_codes_then(FxScaledSpriteAdvance_code[copy] + 2, 1, GSU_COPY(2)));
    R[5] = gsu_hib(R[0]);
    R[0] = gsu_add(R[5], R[8], 0);
    gsu_sbk(R[0]);
    R[9] = gsu_ldw(gsu_codes_then(FxScaledSpriteAdvance_code[copy] + 3, 1, HUD_Y));
    R[0] = gsu_add(R[4], R[4], 0);
    R[0] = times_pow2(R[0], 4, gsu_codes_then(FxScaledSpriteAdvance_code[copy] + 4, 2, GSU_COPY(2)));
    R[7] = gsu_hib(R[0]);
    scaled_sprite(GSU_COPY(1));
}

void abi_FxScaledSpriteAdvance(void)
{
    FxScaledSpriteAdvance(GSU_COPY(0));
}

/* ---- programs no script has started yet ---- */

/*
 * @routine 07:EE94 FxNoiseFillScreen
 * WHAT IT DOES: FxNoiseFill over the whole screen: $70:003A pixels across
 * and $70:003E + 1 lines down.
 * HOW: The width and height into R9 and R10, then FxNoiseFill's body. No
 * script has started it yet.
 * Entry: started by the S-CPU. Exit: as FxNoiseFill, with R9 = the width and
 * R2 = the lines drawn.
 */
void FxNoiseFillScreen(void)
{
    gsu_code(0x07EE95, 7);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.por = (uint8_t)R[0];
    R[9] = gsu_ldw(SCREEN_WIDTH);
    R[10] = gsu_ldw(gsu_code_then(0x07EE9C, 4, SCREEN_HEIGHT));
    R[10] = gsu_inc(R[10]);
    noise_body();
}

void abi_FxNoiseFillScreen(void)
{
    FxNoiseFillScreen();
}

/*
 * @routine 07:F280 FxGlyph8x8
 * WHAT IT DOES: As FxGlyph8x16, 8 rows.
 * HOW: As FxGlyph8x16. No script has started it yet.
 * Entry: started by the S-CPU. Exit: as FxGlyph8x16, with R2 = y + 8 and R13
 * = $F2AC.
 */
void FxGlyph8x8(void)
{
    bitmap_setup(GSU_COPY(6));
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(8, GSU_COPY(4));
    if (!glyph_bitmap(GSU_COPY(1)))
        { gsu_code(0x07F2A5, 1); return; }
    R[12] = 8;
    glyph_rows(0xF2AC, 2, 1, GSU_COPY(4));
}

void abi_FxGlyph8x8(void)
{
    FxGlyph8x8();
}

/*
 * @routine 07:F3D2 FxGlyph24x32Bank04
 * WHAT IT DOES: Draws the 24 x 32 glyph at $70:0216 in ROM bank $04 at
 * ($70:020A, $70:020C) and moves x on by 16.
 * HOW: 32 rows of six bitmap bytes (R3 counts them) through the colour table
 * at $70:0200; one RPIX at the end. The glyph address is not checked. No
 * script has started it yet.
 * Entry: started by the S-CPU. Exit: R0 = the pixel at (x, y + 32); R1 = R8
 * = x; R2 = y + 32; R3 = 0; R11 = $0200; R12 = 0; R13 = $F3F7; R14 past the
 * last row; ROM bank $04, RAM bank 0, plot options 0.
 */
void FxGlyph24x32Bank04(void)
{
    gsu_code(0x07F3D3, 16);
    int more;

    R[0] = 0x04;
    gsu.rombr = (uint8_t)(R[0] & 0x7F);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.rambr = (uint8_t)(R[0] & 1);
    gsu.por = (uint8_t)R[0];
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x10, GSU_COPY(2));
    gsu_r14(gsu_ldw(HUD_BITMAP));
    R[12] = 0x20;
    R[13] = 0xF3F7;
    gsu_code(0x07F3F2, 5);
    do {
        gsu_code(0x07F3F7, 2);
        R[3] = 6;
        do {
            bitmap_byte(GSU_COPY(4));
            R[3] = gsu_dec(R[3]);
        } while (!gsu.z);
        R[0] = 0x1A;
        gsu_code(0x07F434, 6);
        gsu_r14(gsu_add(R[0], R[14], 0));
        R[1] = R[8];
        more = loop(GSU_COPY(1));
        R[2] = gsu_inc(R[2]);
    } while (more);
    R[0] = rpix(GSU_COPY(1));
}

void abi_FxGlyph24x32Bank04(void)
{
    FxGlyph24x32Bank04();
}

/*
 * @routine 07:F5C9 FxGlyph16x16
 * WHAT IT DOES: Draws the 16 x 16 glyph at $70:0216 (in bank $15) at
 * ($70:020A, $70:020C) and moves x on by 16.
 * HOW: 16 rows of four bitmap bytes through the colour table at $70:0200,
 * with an RPIX after each byte and one more at the end. The glyph address is
 * not checked. No script has started it yet.
 * Entry: started by the S-CPU. Exit: as FxGlyph24x16, with R13 = $F5EE.
 */
void FxGlyph16x16(void)
{
    bitmap_setup(GSU_COPY(4));
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x10, GSU_COPY(3));
    gsu_r14(gsu_ldw(HUD_BITMAP));
    R[12] = 0x10;
    gsu_code(0x07F5E9, 8);
    glyph_rows(0xF5EE, 4, 1, GSU_COPY(3));
    R[0] = rpix(GSU_COPY(5));
}

void abi_FxGlyph16x16(void)
{
    FxGlyph16x16();
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode bitmap_byte_double_code[2][1] = {
    {{0x07F820, 3}},
    {{0x07F91A, 5}},
};

/* One bitmap byte at double width: as bitmap_byte, each pixel plotted
   twice. */
static void bitmap_byte_double(int copy)
{
    gsu_codes(bitmap_byte_double_code[copy], 1);
    R[6] = gsu_getb();
    R[0] = R[6];
    R[0] = gsu_and(R[0], 3);
    R[0] = gsu_add(R[0], R[11], 0);
    R[0] = gsu_ldb(R[0]);
    color(R[0], GSU_COPY(0));
    plot();
    plot();
    R[6] = gsu_ror(R[6]);
    R[6] = gsu_ror(R[6]);
    gsu_r14(gsu_inc(R[14]));
    for (int k = 0; k < 3; k++) {
        R[0] = R[6];
        R[6] = gsu_ror(R[6]);
        R[6] = gsu_ror(R[6]);
        R[0] = gsu_and(R[0], 3);
        R[0] = gsu_add(R[0], R[11], 0);
        R[0] = gsu_ldb(R[0]);
        color(R[0], GSU_COPY(0));
        plot();
        plot();
    }
}

/*
 * @routine 07:F7F4 FxGlyph16x32Double
 * WHAT IT DOES: Draws the 16 x 32 glyph at $70:0216 (in bank $15) at double
 * size (32 x 64) at ($70:020A, $70:020C) and moves x on by 32; nothing is
 * drawn when the glyph address is 0.
 * HOW: Each of the 32 rows twice (R14 back 4 bytes between), each byte's
 * pixels plotted twice, with an RPIX after each byte. No script has started
 * it yet.
 * Entry: started by the S-CPU. Exit: when drawn, R0 = $1C; R1 = R8 = x; R2 =
 * y + 64; R11 = $0200; R12 = 0; R13 = $F820; R14 past the last row; ROM bank
 * $15, RAM bank 0, plot options 0.
 */
void FxGlyph16x32Double(void)
{
    bitmap_setup(GSU_COPY(7));
    R[11] = HUD_COLOURS;
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x20, GSU_COPY(5));
    if (!glyph_bitmap(GSU_COPY(2)))
        { gsu_code(0x07F819, 1); return; } else { gsu_code(0x07F81B, 5); }
    R[12] = 0x20;
    R[13] = 0xF820;
    do {
        for (int b = 0; b < 4; b++) {
            bitmap_byte_double(GSU_COPY(0));
            R[0] = rpix(GSU_COPY(8));
        }
        R[0] = 0xFFFC;
        gsu_r14(gsu_add(R[0], R[14], 0));
        R[1] = R[8];
        R[2] = gsu_inc(R[2]);
        for (int b = 0; b < 4; b++) {
            bitmap_byte_double(GSU_COPY(1));
            R[0] = rpix(GSU_COPY(9));
        }
    } while (bitmap_row_end(0x1C, GSU_COPY(6)));
    gsu_code(0x07FA19, 1);
}

void abi_FxGlyph16x32Double(void)
{
    FxGlyph16x32Double();
}

/*
 * @routine 07:FA35 FxScaledSprite12
 * WHAT IT DOES: As FxScaledSprite13, (size across / 2) * 12 / 16 pixels
 * wide.
 * HOW: As FxScaledSprite13, with MULT #12. No script has started it yet.
 * Entry: started by the S-CPU. Exit: as FxScaledSprite13.
 */
void FxScaledSprite12(void)
{
    gsu_code(0x07FA36, 4);
    R[3] = gsu_ldw(HUD_ADVANCE);
    R[4] = gsu_ldw(gsu_code_then(0x07FA3A, 4, HUD_ROWS));
    R[8] = gsu_ldw(gsu_code_then(0x07FA3E, 4, HUD_X));
    R[0] = gsu_div2(R[3]);
    R[0] = mult_imm(R[0], 12);
    R[0] = times_pow2(R[0], 4, GSU_COPY(3));
    R[5] = gsu_hib(R[0]);
    scaled_height_16();
}

void abi_FxScaledSprite12(void)
{
    FxScaledSprite12();
}

/*
 * @routine 07:FAB0 FxSprite16x32
 * WHAT IT DOES: Draws the 16 x 32 sprite at $70:0216 at ($70:020A, $70:020C)
 * unscaled (both steps at $70:0212 and $70:0214 set to $100) and moves x on
 * by 16, when x is between -15 and $D0.
 * HOW: The steps stored, then two unsigned compares of x; then the shared
 * scaled-sprite body. When x is $D0 or more the branch away has an IWT in
 * its delay slot, which takes its operand from the two bytes at the target
 * (STOP, NOP: R0 = $0100) and runs on into FxScaledSpriteAdvance (x above
 * $D0); the C does the same. No script has started it yet.
 * Entry: started by the S-CPU. Exit: as FxScaledSprite13.
 */
void FxSprite16x32(void)
{
    bitmap_setup(GSU_COPY(5));
    R[0] = 0x0100;
    gsu_stw(HUD_STEP_X, R[0]);
    gsu_code(0x07FAC1, 4);
    gsu_stw(HUD_STEP_Y, R[0]);
    R[11] = HUD_COLOURS;
    R[3] = 0x0100;
    R[4] = 0x0100;
    R[8] = gsu_ldw(gsu_code_then(0x07FAC5, 13, HUD_X));
    R[0] = 0xFFF0;
    R[0] = gsu_sub(R[0], R[8], 0);
    if (gsu.cy) {
        gsu_code(0x07FAD8, 6);
        R[0] = 0x00D0;
        R[0] = gsu_sub(R[0], R[8], 0);
        if (!gsu.cy) {
            gsu_code(0x07FAD2, 6); gsu_code(0x07FADE, 1); gsu_code(0x07FAF0, 3);
            R[0] = 0x0100;
            FxScaledSpriteAdvance(GSU_COPY(1));
            gsu_code(0x07FB23, 1); gsu_code(0x07FB32, 4);
            return;
        }
    }
    R[0] = 0x10;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07FAD2, 6); gsu_code(0x07FADE, 6);
    gsu_sbk(R[0]);
    R[5] = 0x10;
    R[9] = gsu_ldw(gsu_code_then(0x07FAE4, 7, HUD_Y));
    R[7] = 0x20;
    scaled_sprite(GSU_COPY(2));
}

void abi_FxSprite16x32(void)
{
    FxSprite16x32();
}
