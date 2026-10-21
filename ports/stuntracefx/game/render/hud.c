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
#include "draw_list.h"

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

/* v * 2^n by n ADDs of the value to itself. */
/* gsuwalk: the code of each copy of times_pow2, read from the ROM */
static const GsuCode times_pow2_fetch[9][4] = {
    {{0x07f14e, 9}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07fa2d, 8}, {0x07fa4c, 1}, {0, 0}, {0, 0}},
    {{0x07fa51, 9}, {0x07fa78, 1}, {0xfa79, 0}, {0x07fa79, 3}},
    {{0x07fb8b, 5}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07fa47, 6}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07fa67, 8}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07fa73, 6}, {0xfa79, 0}, {0x07fa79, 3}, {0, 0}},
    {{0x07fb0b, 9}, {0, 0}, {0, 0}, {0, 0}},
    {{0x07fb19, 10}, {0x07fb25, 1}, {0, 0}, {0, 0}},
};

static uint16_t times_pow2(uint16_t v, int n, int copy)
{
    gsu_codes(times_pow2_fetch[copy], 4);
    while (n--)
        v = gsu_add(v, v, 0);
    return v;
}

/* LOOP: counts R12 down; true while it has not reached 0. */
static int loop(int copy)
{
    return gsu_count_down(&R[12]);
}

/* COLOR, with the plot options' nibble modes. */
/* gsuwalk: the code of each copy of color, read from the ROM */
static const GsuCode color_fetch[146][3] = {
    {{0x07eee5, 1}, {0x07eee5, 1}, {0x07eee5, 1}},
    {{0x07ef24, 1}, {0x07ef24, 1}, {0x07ef24, 1}},
    {{0x07ef36, 1}, {0x07ef36, 1}, {0x07ef36, 1}},
    {{0x07ef43, 1}, {0x07ef43, 1}, {0x07ef43, 1}},
    {{0x07ef50, 1}, {0x07ef50, 1}, {0x07ef50, 1}},
    {{0x07ef8f, 1}, {0x07ef8f, 1}, {0x07ef8f, 1}},
    {{0x07efa1, 1}, {0x07efa1, 1}, {0x07efa1, 1}},
    {{0x07efae, 1}, {0x07efae, 1}, {0x07efae, 1}},
    {{0x07efbb, 1}, {0x07efbb, 1}, {0x07efbb, 1}},
    {{0x07effc, 1}, {0x07effc, 1}, {0x07effc, 1}},
    {{0x07f00e, 1}, {0x07f00e, 1}, {0x07f00e, 1}},
    {{0x07f01b, 1}, {0x07f01b, 1}, {0x07f01b, 1}},
    {{0x07f028, 1}, {0x07f028, 1}, {0x07f028, 1}},
    {{0x07f067, 1}, {0x07f067, 1}, {0x07f067, 1}},
    {{0x07f079, 1}, {0x07f079, 1}, {0x07f079, 1}},
    {{0x07f086, 1}, {0x07f086, 1}, {0x07f086, 1}},
    {{0x07f093, 1}, {0x07f093, 1}, {0x07f093, 1}},
    {{0x07f0e3, 1}, {0x07f0e3, 1}, {0x07f0e3, 1}},
    {{0x07f0f5, 1}, {0x07f0f5, 1}, {0x07f0f5, 1}},
    {{0x07f102, 1}, {0x07f102, 1}, {0x07f102, 1}},
    {{0x07f10f, 1}, {0x07f10f, 1}, {0x07f10f, 1}},
    {{0x07f19b, 1}, {0x07f19b, 1}, {0x07f19b, 1}},
    {{0x07f1ad, 1}, {0x07f1ad, 1}, {0x07f1ad, 1}},
    {{0x07f1ba, 1}, {0x07f1ba, 1}, {0x07f1ba, 1}},
    {{0x07f1c7, 1}, {0x07f1c7, 1}, {0x07f1c7, 1}},
    {{0x07f1d4, 1}, {0x07f1d4, 1}, {0x07f1d4, 1}},
    {{0x07f1e6, 1}, {0x07f1e6, 1}, {0x07f1e6, 1}},
    {{0x07f1f3, 1}, {0x07f1f3, 1}, {0x07f1f3, 1}},
    {{0x07f200, 1}, {0x07f200, 1}, {0x07f200, 1}},
    {{0x07f20d, 1}, {0x07f20d, 1}, {0x07f20d, 1}},
    {{0x07f21f, 1}, {0x07f21f, 1}, {0x07f21f, 1}},
    {{0x07f22c, 1}, {0x07f22c, 1}, {0x07f22c, 1}},
    {{0x07f239, 1}, {0x07f239, 1}, {0x07f239, 1}},
    {{0x07f246, 1}, {0x07f246, 1}, {0x07f246, 1}},
    {{0x07f258, 1}, {0x07f258, 1}, {0x07f258, 1}},
    {{0x07f265, 1}, {0x07f265, 1}, {0x07f265, 1}},
    {{0x07f272, 1}, {0x07f272, 1}, {0x07f272, 1}},
    {{0x07f2b6, 1}, {0x07f2b6, 1}, {0x07f2b6, 1}},
    {{0x07f2c8, 1}, {0x07f2c8, 1}, {0x07f2c8, 1}},
    {{0x07f2d5, 1}, {0x07f2d5, 1}, {0x07f2d5, 1}},
    {{0x07f2e2, 1}, {0x07f2e2, 1}, {0x07f2e2, 1}},
    {{0x07f2ef, 1}, {0x07f2ef, 1}, {0x07f2ef, 1}},
    {{0x07f301, 1}, {0x07f301, 1}, {0x07f301, 1}},
    {{0x07f30e, 1}, {0x07f30e, 1}, {0x07f30e, 1}},
    {{0x07f31b, 1}, {0x07f31b, 1}, {0x07f31b, 1}},
    {{0x07f35f, 1}, {0x07f35f, 1}, {0x07f35f, 1}},
    {{0x07f371, 1}, {0x07f371, 1}, {0x07f371, 1}},
    {{0x07f37e, 1}, {0x07f37e, 1}, {0x07f37e, 1}},
    {{0x07f38b, 1}, {0x07f38b, 1}, {0x07f38b, 1}},
    {{0x07f398, 1}, {0x07f398, 1}, {0x07f398, 1}},
    {{0x07f3aa, 1}, {0x07f3aa, 1}, {0x07f3aa, 1}},
    {{0x07f3b7, 1}, {0x07f3b7, 1}, {0x07f3b7, 1}},
    {{0x07f3c4, 1}, {0x07f3c4, 1}, {0x07f3c4, 1}},
    {{0x07f403, 1}, {0x07f403, 1}, {0x07f403, 1}},
    {{0x07f415, 1}, {0x07f415, 1}, {0x07f415, 1}},
    {{0x07f422, 1}, {0x07f422, 1}, {0x07f422, 1}},
    {{0x07f42f, 1}, {0x07f42f, 1}, {0x07f42f, 1}},
    {{0x07f470, 1}, {0x07f470, 1}, {0x07f470, 1}},
    {{0x07f482, 1}, {0x07f482, 1}, {0x07f482, 1}},
    {{0x07f48f, 1}, {0x07f48f, 1}, {0x07f48f, 1}},
    {{0x07f49c, 1}, {0x07f49c, 1}, {0x07f49c, 1}},
    {{0x07f4a9, 1}, {0x07f4a9, 1}, {0x07f4a9, 1}},
    {{0x07f4bb, 1}, {0x07f4bb, 1}, {0x07f4bb, 1}},
    {{0x07f4c8, 1}, {0x07f4c8, 1}, {0x07f4c8, 1}},
    {{0x07f4d5, 1}, {0x07f4d5, 1}, {0x07f4d5, 1}},
    {{0x07f4e2, 1}, {0x07f4e2, 1}, {0x07f4e2, 1}},
    {{0x07f4f4, 1}, {0x07f4f4, 1}, {0x07f4f4, 1}},
    {{0x07f501, 1}, {0x07f501, 1}, {0x07f501, 1}},
    {{0x07f50e, 1}, {0x07f50e, 1}, {0x07f50e, 1}},
    {{0x07f51b, 1}, {0x07f51b, 1}, {0x07f51b, 1}},
    {{0x07f52d, 1}, {0x07f52d, 1}, {0x07f52d, 1}},
    {{0x07f53a, 1}, {0x07f53a, 1}, {0x07f53a, 1}},
    {{0x07f547, 1}, {0x07f547, 1}, {0x07f547, 1}},
    {{0x07f554, 1}, {0x07f554, 1}, {0x07f554, 1}},
    {{0x07f566, 1}, {0x07f566, 1}, {0x07f566, 1}},
    {{0x07f573, 1}, {0x07f573, 1}, {0x07f573, 1}},
    {{0x07f580, 1}, {0x07f580, 1}, {0x07f580, 1}},
    {{0x07f58d, 1}, {0x07f58d, 1}, {0x07f58d, 1}},
    {{0x07f59f, 1}, {0x07f59f, 1}, {0x07f59f, 1}},
    {{0x07f5ac, 1}, {0x07f5ac, 1}, {0x07f5ac, 1}},
    {{0x07f5b9, 1}, {0x07f5b9, 1}, {0x07f5b9, 1}},
    {{0x07f5f8, 1}, {0x07f5f8, 1}, {0x07f5f8, 1}},
    {{0x07f60a, 1}, {0x07f60a, 1}, {0x07f60a, 1}},
    {{0x07f617, 1}, {0x07f617, 1}, {0x07f617, 1}},
    {{0x07f624, 1}, {0x07f624, 1}, {0x07f624, 1}},
    {{0x07f631, 1}, {0x07f631, 1}, {0x07f631, 1}},
    {{0x07f643, 1}, {0x07f643, 1}, {0x07f643, 1}},
    {{0x07f650, 1}, {0x07f650, 1}, {0x07f650, 1}},
    {{0x07f65d, 1}, {0x07f65d, 1}, {0x07f65d, 1}},
    {{0x07f66a, 1}, {0x07f66a, 1}, {0x07f66a, 1}},
    {{0x07f67c, 1}, {0x07f67c, 1}, {0x07f67c, 1}},
    {{0x07f689, 1}, {0x07f689, 1}, {0x07f689, 1}},
    {{0x07f696, 1}, {0x07f696, 1}, {0x07f696, 1}},
    {{0x07f6a3, 1}, {0x07f6a3, 1}, {0x07f6a3, 1}},
    {{0x07f6b5, 1}, {0x07f6b5, 1}, {0x07f6b5, 1}},
    {{0x07f6c2, 1}, {0x07f6c2, 1}, {0x07f6c2, 1}},
    {{0x07f6cf, 1}, {0x07f6cf, 1}, {0x07f6cf, 1}},
    {{0x07f715, 1}, {0x07f715, 1}, {0x07f715, 1}},
    {{0x07f727, 1}, {0x07f727, 1}, {0x07f727, 1}},
    {{0x07f734, 1}, {0x07f734, 1}, {0x07f734, 1}},
    {{0x07f741, 1}, {0x07f741, 1}, {0x07f741, 1}},
    {{0x07f74c, 1}, {0x07f74c, 1}, {0x07f74c, 1}},
    {{0x07f75e, 1}, {0x07f75e, 1}, {0x07f75e, 1}},
    {{0x07f76b, 1}, {0x07f76b, 1}, {0x07f76b, 1}},
    {{0x07f778, 1}, {0x07f778, 1}, {0x07f778, 1}},
    {{0x07f783, 1}, {0x07f783, 1}, {0x07f783, 1}},
    {{0x07f795, 1}, {0x07f795, 1}, {0x07f795, 1}},
    {{0x07f7a2, 1}, {0x07f7a2, 1}, {0x07f7a2, 1}},
    {{0x07f7af, 1}, {0x07f7af, 1}, {0x07f7af, 1}},
    {{0x07f7ba, 1}, {0x07f7ba, 1}, {0x07f7ba, 1}},
    {{0x07f7cc, 1}, {0x07f7cc, 1}, {0x07f7cc, 1}},
    {{0x07f7d9, 1}, {0x07f7d9, 1}, {0x07f7d9, 1}},
    {{0x07f7e6, 1}, {0x07f7e6, 1}, {0x07f7e6, 1}},
    {{0x07f82a, 1}, {0x07f82a, 1}, {0x07f82a, 1}},
    {{0x07f83d, 1}, {0x07f83d, 1}, {0x07f83d, 1}},
    {{0x07f84b, 1}, {0x07f84b, 1}, {0x07f84b, 1}},
    {{0x07f859, 1}, {0x07f859, 1}, {0x07f859, 1}},
    {{0x07f867, 1}, {0x07f867, 1}, {0x07f867, 1}},
    {{0x07f87a, 1}, {0x07f87a, 1}, {0x07f87a, 1}},
    {{0x07f888, 1}, {0x07f888, 1}, {0x07f888, 1}},
    {{0x07f896, 1}, {0x07f896, 1}, {0x07f896, 1}},
    {{0x07f8a4, 1}, {0x07f8a4, 1}, {0x07f8a4, 1}},
    {{0x07f8b7, 1}, {0x07f8b7, 1}, {0x07f8b7, 1}},
    {{0x07f8c5, 1}, {0x07f8c5, 1}, {0x07f8c5, 1}},
    {{0x07f8d3, 1}, {0x07f8d3, 1}, {0x07f8d3, 1}},
    {{0x07f8e1, 1}, {0x07f8e1, 1}, {0x07f8e1, 1}},
    {{0x07f8f4, 1}, {0x07f8f4, 1}, {0x07f8f4, 1}},
    {{0x07f902, 1}, {0x07f902, 1}, {0x07f902, 1}},
    {{0x07f910, 1}, {0x07f910, 1}, {0x07f910, 1}},
    {{0x07f926, 1}, {0x07f926, 1}, {0x07f926, 1}},
    {{0x07f939, 1}, {0x07f939, 1}, {0x07f939, 1}},
    {{0x07f947, 1}, {0x07f947, 1}, {0x07f947, 1}},
    {{0x07f955, 1}, {0x07f955, 1}, {0x07f955, 1}},
    {{0x07f963, 1}, {0x07f963, 1}, {0x07f963, 1}},
    {{0x07f976, 1}, {0x07f976, 1}, {0x07f976, 1}},
    {{0x07f984, 1}, {0x07f984, 1}, {0x07f984, 1}},
    {{0x07f992, 1}, {0x07f992, 1}, {0x07f992, 1}},
    {{0x07f9a0, 1}, {0x07f9a0, 1}, {0x07f9a0, 1}},
    {{0x07f9b3, 1}, {0x07f9b3, 1}, {0x07f9b3, 1}},
    {{0x07f9c1, 1}, {0x07f9c1, 1}, {0x07f9c1, 1}},
    {{0x07f9cf, 1}, {0x07f9cf, 1}, {0x07f9cf, 1}},
    {{0x07f9dd, 1}, {0x07f9dd, 1}, {0x07f9dd, 1}},
    {{0x07f9f0, 1}, {0x07f9f0, 1}, {0x07f9f0, 1}},
    {{0x07f9fe, 1}, {0x07f9fe, 1}, {0x07f9fe, 1}},
    {{0x07fa0c, 1}, {0x07fa0c, 1}, {0x07fa0c, 1}},
    {{0x07fbb0, 1}, {0x07fbb0, 1}, {0x07fbb0, 1}},
};

static void color(uint16_t v, int copy)
{
    uint8_t c = (uint8_t)v;

    if (gsu.por & 0x04)
        { c = (uint8_t)((gsu.colr & 0xF0) | c >> 4); gsu_codes(color_fetch[copy], 1); }
    else if (gsu.por & 0x08)
        { c = (uint8_t)((gsu.colr & 0xF0) | (c & 0x0F)); gsu_codes(color_fetch[copy] + 2, 1); } else { gsu_codes(color_fetch[copy] + 1, 1); }
    gsu.colr = c;
}

/* gsuwalk: the code of each copy of plot, read from the ROM */
static const GsuCode plot_fetch[177][1] = {
    {{0x07ef25, 1}},
    {{0x07ef37, 1}},
    {{0x07ef44, 1}},
    {{0x07ef51, 1}},
    {{0x07ef90, 1}},
    {{0x07efa2, 1}},
    {{0x07efaf, 1}},
    {{0x07efbc, 1}},
    {{0x07effd, 1}},
    {{0x07f00f, 1}},
    {{0x07f01c, 1}},
    {{0x07f029, 1}},
    {{0x07f068, 1}},
    {{0x07f07a, 1}},
    {{0x07f087, 1}},
    {{0x07f094, 1}},
    {{0x07f0e4, 1}},
    {{0x07f0f6, 1}},
    {{0x07f103, 1}},
    {{0x07f110, 1}},
    {{0x07f19c, 1}},
    {{0x07f1ae, 1}},
    {{0x07f1bb, 1}},
    {{0x07f1c8, 1}},
    {{0x07f1d5, 1}},
    {{0x07f1e7, 1}},
    {{0x07f1f4, 1}},
    {{0x07f201, 1}},
    {{0x07f20e, 1}},
    {{0x07f220, 1}},
    {{0x07f22d, 1}},
    {{0x07f23a, 1}},
    {{0x07f247, 1}},
    {{0x07f259, 1}},
    {{0x07f266, 1}},
    {{0x07f273, 1}},
    {{0x07f2b7, 1}},
    {{0x07f2c9, 1}},
    {{0x07f2d6, 1}},
    {{0x07f2e3, 1}},
    {{0x07f2f0, 1}},
    {{0x07f302, 1}},
    {{0x07f30f, 1}},
    {{0x07f31c, 1}},
    {{0x07f360, 1}},
    {{0x07f372, 1}},
    {{0x07f37f, 1}},
    {{0x07f38c, 1}},
    {{0x07f399, 1}},
    {{0x07f3ab, 1}},
    {{0x07f3b8, 1}},
    {{0x07f3c5, 1}},
    {{0x07f404, 1}},
    {{0x07f416, 1}},
    {{0x07f423, 1}},
    {{0x07f430, 1}},
    {{0x07f471, 1}},
    {{0x07f483, 1}},
    {{0x07f490, 1}},
    {{0x07f49d, 1}},
    {{0x07f4aa, 1}},
    {{0x07f4bc, 1}},
    {{0x07f4c9, 1}},
    {{0x07f4d6, 1}},
    {{0x07f4e3, 1}},
    {{0x07f4f5, 1}},
    {{0x07f502, 1}},
    {{0x07f50f, 1}},
    {{0x07f51c, 1}},
    {{0x07f52e, 1}},
    {{0x07f53b, 1}},
    {{0x07f548, 1}},
    {{0x07f555, 1}},
    {{0x07f567, 1}},
    {{0x07f574, 1}},
    {{0x07f581, 1}},
    {{0x07f58e, 1}},
    {{0x07f5a0, 1}},
    {{0x07f5ad, 1}},
    {{0x07f5ba, 1}},
    {{0x07f5f9, 1}},
    {{0x07f60b, 1}},
    {{0x07f618, 1}},
    {{0x07f625, 1}},
    {{0x07f632, 1}},
    {{0x07f644, 1}},
    {{0x07f651, 1}},
    {{0x07f65e, 1}},
    {{0x07f66b, 1}},
    {{0x07f67d, 1}},
    {{0x07f68a, 1}},
    {{0x07f697, 1}},
    {{0x07f6a4, 1}},
    {{0x07f6b6, 1}},
    {{0x07f6c3, 1}},
    {{0x07f6d0, 1}},
    {{0x07f716, 1}},
    {{0x07f728, 1}},
    {{0x07f735, 1}},
    {{0x07f742, 1}},
    {{0x07f74d, 1}},
    {{0x07f75f, 1}},
    {{0x07f76c, 1}},
    {{0x07f779, 1}},
    {{0x07f784, 1}},
    {{0x07f796, 1}},
    {{0x07f7a3, 1}},
    {{0x07f7b0, 1}},
    {{0x07f7bb, 1}},
    {{0x07f7cd, 1}},
    {{0x07f7da, 1}},
    {{0x07f7e7, 1}},
    {{0x07f82b, 1}},
    {{0x07f82c, 1}},
    {{0x07f83e, 1}},
    {{0x07f83f, 1}},
    {{0x07f84c, 1}},
    {{0x07f84d, 1}},
    {{0x07f85a, 1}},
    {{0x07f85b, 1}},
    {{0x07f868, 1}},
    {{0x07f869, 1}},
    {{0x07f87b, 1}},
    {{0x07f87c, 1}},
    {{0x07f889, 1}},
    {{0x07f88a, 1}},
    {{0x07f897, 1}},
    {{0x07f898, 1}},
    {{0x07f8a5, 1}},
    {{0x07f8a6, 1}},
    {{0x07f8b8, 1}},
    {{0x07f8b9, 1}},
    {{0x07f8c6, 1}},
    {{0x07f8c7, 1}},
    {{0x07f8d4, 1}},
    {{0x07f8d5, 1}},
    {{0x07f8e2, 1}},
    {{0x07f8e3, 1}},
    {{0x07f8f5, 1}},
    {{0x07f8f6, 1}},
    {{0x07f903, 1}},
    {{0x07f904, 1}},
    {{0x07f911, 1}},
    {{0x07f912, 1}},
    {{0x07f927, 1}},
    {{0x07f928, 1}},
    {{0x07f93a, 1}},
    {{0x07f93b, 1}},
    {{0x07f948, 1}},
    {{0x07f949, 1}},
    {{0x07f956, 1}},
    {{0x07f957, 1}},
    {{0x07f964, 1}},
    {{0x07f965, 1}},
    {{0x07f977, 1}},
    {{0x07f978, 1}},
    {{0x07f985, 1}},
    {{0x07f986, 1}},
    {{0x07f993, 1}},
    {{0x07f994, 1}},
    {{0x07f9a1, 1}},
    {{0x07f9a2, 1}},
    {{0x07f9b4, 1}},
    {{0x07f9b5, 1}},
    {{0x07f9c2, 1}},
    {{0x07f9c3, 1}},
    {{0x07f9d0, 1}},
    {{0x07f9d1, 1}},
    {{0x07f9de, 1}},
    {{0x07f9df, 1}},
    {{0x07f9f1, 1}},
    {{0x07f9f2, 1}},
    {{0x07f9ff, 1}},
    {{0x07fa00, 1}},
    {{0x07fa0d, 1}},
    {{0x07fa0e, 1}},
    {{0x07fbb1, 1}},
};

static void plot(int copy)
{
    gsu_codes(plot_fetch[copy], 1);
    gsu_plot((uint8_t)R[1], (uint8_t)R[2]);
    R[1]++;
}

/* RPIX: reads the pixel at (R1, R2), first writing out both pixel caches. */
/* gsuwalk: the code of each copy of rpix, read from the ROM */
static const GsuCode rpix_fetch[39][1] = {
    {{0x07eeed, 3}},
    {{0x07ef5e, 3}},
    {{0x07efc9, 3}},
    {{0x07f036, 3}},
    {{0x07f0a1, 3}},
    {{0x07f11d, 3}},
    {{0x07f1c9, 2}},
    {{0x07f202, 2}},
    {{0x07f23b, 2}},
    {{0x07f274, 2}},
    {{0x07f2e4, 2}},
    {{0x07f31d, 2}},
    {{0x07f38d, 2}},
    {{0x07f3c6, 2}},
    {{0x07f43d, 3}},
    {{0x07f49e, 2}},
    {{0x07f4d7, 2}},
    {{0x07f510, 2}},
    {{0x07f549, 2}},
    {{0x07f582, 2}},
    {{0x07f5bb, 2}},
    {{0x07f5c5, 3}},
    {{0x07f626, 2}},
    {{0x07f65f, 2}},
    {{0x07f698, 2}},
    {{0x07f6d1, 2}},
    {{0x07f6db, 3}},
    {{0x07f7f0, 3}},
    {{0x07f85c, 2}},
    {{0x07f899, 2}},
    {{0x07f8d6, 2}},
    {{0x07f913, 2}},
    {{0x07f958, 2}},
    {{0x07f995, 2}},
    {{0x07f9d2, 2}},
    {{0x07fa0f, 2}},
    {{0x07fbbb, 2}},
    {{0x07fbb3, 2}},
    {{0x07fbba, 3}},
};

static uint16_t rpix(int copy)
{
    gsu_codes(rpix_fetch[copy], 1);
    uint16_t r = gsu_rpix((uint8_t)R[1], (uint8_t)R[2]);

    gsu_sz(r);
    return r;
}

/* ---- drawing from a bitmap ---- */

/* The setup every bitmap program makes: ROM bank $15 for GETB, RAM bank 0,
   plot options 0 (colour 0 transparent, no dither, no nibble modes). */
/* gsuwalk: the code of each copy of bitmap_setup, read from the ROM */
static const GsuCode bitmap_setup_fetch[18][3] = {
    {{0x07eef6, 7}, {0, 0}, {0x07eefd, 3}},
    {{0x07ef61, 4}, {0, 0}, {0x07ef65, 3}},
    {{0x07efce, 7}, {0, 0}, {0x07efd5, 3}},
    {{0x07f039, 4}, {0, 0}, {0x07f03d, 3}},
    {{0x07f0ad, 7}, {0x07f0c0, 5}, {0x07f0c5, 3}},
    {{0x07f0bc, 9}, {0, 0}, {0x07f0c5, 3}},
    {{0x07f122, 5}, {0, 0}, {0x07f127, 3}},
    {{0x07f15c, 5}, {0, 0}, {0x07f161, 3}},
    {{0x07f281, 5}, {0, 0}, {0x07f286, 3}},
    {{0x07f32a, 5}, {0, 0}, {0x07f32f, 3}},
    {{0x07f442, 4}, {0, 0}, {0x07f446, 3}},
    {{0x07f5ca, 4}, {0, 0}, {0x07f5ce, 3}},
    {{0x07f6e0, 4}, {0, 0}, {0x07f6e4, 3}},
    {{0x07f7f5, 5}, {0, 0}, {0x07f7fa, 3}},
    {{0x07fa7c, 2}, {0, 0}, {0x07fa7e, 3}},
    {{0x07fa89, 4}, {0, 0}, {0x07fa8d, 3}},
    {{0x07fab1, 4}, {0, 0}, {0x07fab5, 3}},
    {{0x07faf3, 4}, {0, 0}, {0x07faf7, 3}},
};

static void bitmap_setup(int copy)
{
    R[0] = BITMAP_BANK;
    gsu_codes(bitmap_setup_fetch[copy], 2);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_codes(bitmap_setup_fetch[copy] + 2, 1);
    gsu_ramb(R[0]);
    gsu.por = (uint8_t)R[0];
}

/* One pixel: colour byte (R0 AND 3) of the table at R11, plotted. */
/* gsuwalk: the code of each copy of pixel, read from the ROM */
static const GsuCode pixel_fetch[113][1] = {
    {{0x07ef1d, 7}},
    {{0x07ef2b, 11}},
    {{0x07ef38, 11}},
    {{0x07ef45, 11}},
    {{0x07ef88, 7}},
    {{0x07ef96, 11}},
    {{0x07efa3, 11}},
    {{0x07efb0, 11}},
    {{0x07eff5, 7}},
    {{0x07f003, 11}},
    {{0x07f010, 11}},
    {{0x07f01d, 11}},
    {{0x07f060, 7}},
    {{0x07f06e, 11}},
    {{0x07f07b, 11}},
    {{0x07f088, 11}},
    {{0x07f0dc, 7}},
    {{0x07f0ea, 11}},
    {{0x07f0f7, 11}},
    {{0x07f104, 11}},
    {{0x07f194, 7}},
    {{0x07f1a2, 11}},
    {{0x07f1af, 11}},
    {{0x07f1bc, 11}},
    {{0x07f1cd, 7}},
    {{0x07f1db, 11}},
    {{0x07f1e8, 11}},
    {{0x07f1f5, 11}},
    {{0x07f206, 7}},
    {{0x07f214, 11}},
    {{0x07f221, 11}},
    {{0x07f22e, 11}},
    {{0x07f23f, 7}},
    {{0x07f24d, 11}},
    {{0x07f25a, 11}},
    {{0x07f267, 11}},
    {{0x07f2af, 7}},
    {{0x07f2bd, 11}},
    {{0x07f2ca, 11}},
    {{0x07f2d7, 11}},
    {{0x07f2e8, 7}},
    {{0x07f2f6, 11}},
    {{0x07f303, 11}},
    {{0x07f310, 11}},
    {{0x07f358, 7}},
    {{0x07f366, 11}},
    {{0x07f373, 11}},
    {{0x07f380, 11}},
    {{0x07f391, 7}},
    {{0x07f39f, 11}},
    {{0x07f3ac, 11}},
    {{0x07f3b9, 11}},
    {{0x07f3fc, 7}},
    {{0x07f40a, 11}},
    {{0x07f417, 11}},
    {{0x07f424, 11}},
    {{0x07f469, 7}},
    {{0x07f477, 11}},
    {{0x07f484, 11}},
    {{0x07f491, 11}},
    {{0x07f4a2, 7}},
    {{0x07f4b0, 11}},
    {{0x07f4bd, 11}},
    {{0x07f4ca, 11}},
    {{0x07f4db, 7}},
    {{0x07f4e9, 11}},
    {{0x07f4f6, 11}},
    {{0x07f503, 11}},
    {{0x07f514, 7}},
    {{0x07f522, 11}},
    {{0x07f52f, 11}},
    {{0x07f53c, 11}},
    {{0x07f54d, 7}},
    {{0x07f55b, 11}},
    {{0x07f568, 11}},
    {{0x07f575, 11}},
    {{0x07f586, 7}},
    {{0x07f594, 11}},
    {{0x07f5a1, 11}},
    {{0x07f5ae, 11}},
    {{0x07f5f1, 7}},
    {{0x07f5ff, 11}},
    {{0x07f60c, 11}},
    {{0x07f619, 11}},
    {{0x07f62a, 7}},
    {{0x07f638, 11}},
    {{0x07f645, 11}},
    {{0x07f652, 11}},
    {{0x07f663, 7}},
    {{0x07f671, 11}},
    {{0x07f67e, 11}},
    {{0x07f68b, 11}},
    {{0x07f69c, 7}},
    {{0x07f6aa, 11}},
    {{0x07f6b7, 11}},
    {{0x07f6c4, 11}},
    {{0x07f70e, 7}},
    {{0x07f71c, 11}},
    {{0x07f729, 11}},
    {{0x07f736, 11}},
    {{0x07f745, 7}},
    {{0x07f753, 11}},
    {{0x07f760, 11}},
    {{0x07f76d, 11}},
    {{0x07f77c, 7}},
    {{0x07f78a, 11}},
    {{0x07f797, 11}},
    {{0x07f7a4, 11}},
    {{0x07f7b3, 7}},
    {{0x07f7c1, 11}},
    {{0x07f7ce, 11}},
    {{0x07f7db, 11}},
    {{0x07fbab, 5}},
};

/* gsuwalk: the copy of color each copy of pixel calls */
static const uint8_t color_in_pixel[113] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 145};

/* gsuwalk: the copy of plot each copy of pixel calls */
static const uint8_t plot_in_pixel[113] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 176};

static void pixel(int copy)
{
    R[0] = gsu_and(R[0], 3);
    R[0] = gsu_add(R[0], R[11], 0);
    gsu_codes(pixel_fetch[copy], 1);
    R[0] = gsu_ldb(R[0]);
    color(R[0], GSU_COPY(color_in_pixel[copy]));
    plot(GSU_COPY(plot_in_pixel[copy]));
}

/* One of a bitmap byte's last three pixels (each its own code in the
   original): R0 = R6, R6 rotated right two bits. */
/* gsuwalk: the copy of pixel each copy of bitmap_pixel calls */
static const uint8_t pixel_in_bitmap_pixel[84] = {1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 17, 18, 19, 21, 22, 23, 25, 26, 27, 29, 30, 31, 33, 34, 35, 37, 38, 39, 41, 42, 43, 45, 46, 47, 49, 50, 51, 53, 54, 55, 57, 58, 59, 61, 62, 63, 65, 66, 67, 69, 70, 71, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89, 90, 91, 93, 94, 95, 97, 98, 99, 101, 102, 103, 105, 106, 107, 109, 110, 111};

static void bitmap_pixel(int copy)
{
    R[0] = R[6];
    R[6] = gsu_ror(R[6]);
    R[6] = gsu_ror(R[6]);
    pixel(GSU_COPY(pixel_in_bitmap_pixel[copy]));
}

/* One bitmap byte at R14 as four pixels across, low bits first. R6 takes
   the byte and is rotated right two bits a pixel (through the carry, so
   its high byte collects the bits rotated out); R14 steps to the next
   byte after the first pixel. */
/* gsuwalk: the code of each copy of bitmap_byte, read from the ROM */
static const GsuCode bitmap_byte_fetch[28][2] = {
    {{0x07ef1b, 2}, {0x07ef26, 5}},
    {{0x07ef86, 2}, {0x07ef91, 5}},
    {{0x07eff3, 2}, {0x07effe, 5}},
    {{0x07f05e, 2}, {0x07f069, 5}},
    {{0x07f0da, 2}, {0x07f0e5, 5}},
    {{0x07f192, 2}, {0x07f19d, 5}},
    {{0x07f1cb, 2}, {0x07f1d6, 5}},
    {{0x07f204, 2}, {0x07f20f, 5}},
    {{0x07f23d, 2}, {0x07f248, 5}},
    {{0x07f2ad, 2}, {0x07f2b8, 5}},
    {{0x07f2e6, 2}, {0x07f2f1, 5}},
    {{0x07f356, 2}, {0x07f361, 5}},
    {{0x07f38f, 2}, {0x07f39a, 5}},
    {{0x07f3fa, 2}, {0x07f405, 5}},
    {{0x07f467, 2}, {0x07f472, 5}},
    {{0x07f4a0, 2}, {0x07f4ab, 5}},
    {{0x07f4d9, 2}, {0x07f4e4, 5}},
    {{0x07f512, 2}, {0x07f51d, 5}},
    {{0x07f54b, 2}, {0x07f556, 5}},
    {{0x07f584, 2}, {0x07f58f, 5}},
    {{0x07f5ef, 2}, {0x07f5fa, 5}},
    {{0x07f628, 2}, {0x07f633, 5}},
    {{0x07f661, 2}, {0x07f66c, 5}},
    {{0x07f69a, 2}, {0x07f6a5, 5}},
    {{0x07f70c, 2}, {0x07f717, 5}},
    {{0x07f743, 2}, {0x07f74e, 5}},
    {{0x07f77a, 2}, {0x07f785, 5}},
    {{0x07f7b1, 2}, {0x07f7bc, 5}},
};

/* gsuwalk: the copy of pixel each copy of bitmap_byte calls */
static const uint8_t pixel_in_bitmap_byte[28] = {0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48, 52, 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96, 100, 104, 108};

/* gsuwalk: the copy of bitmap_pixel each copy of bitmap_byte calls */
static const uint8_t bitmap_pixel_in_bitmap_byte[28] = {0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63, 66, 69, 72, 75, 78, 81};

/* gsuwalk: the copy of bitmap_pixel each copy of bitmap_byte calls */
static const uint8_t bitmap_pixel_in_bitmap_byte_2[28] = {1, 4, 7, 10, 13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 52, 55, 58, 61, 64, 67, 70, 73, 76, 79, 82};

/* gsuwalk: the copy of bitmap_pixel each copy of bitmap_byte calls */
static const uint8_t bitmap_pixel_in_bitmap_byte_3[28] = {2, 5, 8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80, 83};

static void bitmap_byte(int copy)
{
    gsu_codes(bitmap_byte_fetch[copy], 1);
    R[6] = gsu_getb();
    R[0] = R[6];
    pixel(GSU_COPY(pixel_in_bitmap_byte[copy]));
    R[6] = gsu_ror(R[6]);
    R[6] = gsu_ror(R[6]);
    gsu_codes(bitmap_byte_fetch[copy] + 1, 1);
    gsu_r14(gsu_inc(R[14]));
    bitmap_pixel(GSU_COPY(bitmap_pixel_in_bitmap_byte[copy]));
    bitmap_pixel(GSU_COPY(bitmap_pixel_in_bitmap_byte_2[copy]));
    bitmap_pixel(GSU_COPY(bitmap_pixel_in_bitmap_byte_3[copy]));
}

/* The end of a bitmap row: R14 on by `skip` to the next row's first byte,
   R1 back to the left edge R8, and the row count R12 down; R2 moves to the
   next line whether or not rows remain. True while rows remain. */
/* gsuwalk: the code of each copy of bitmap_row_end, read from the ROM */
static const GsuCode bitmap_row_end_fetch[7][1] = {
    {{0x07f276, 5}},
    {{0x07f31f, 5}},
    {{0x07f3c8, 5}},
    {{0x07f5bd, 5}},
    {{0x07f6d3, 5}},
    {{0x07f7e8, 5}},
    {{0x07fa11, 5}},
};

/* gsuwalk: the copy of loop each copy of bitmap_row_end calls */
static const uint8_t loop_in_bitmap_row_end[7] = {8, 9, 10, 12, 13, 14, 15};

static int bitmap_row_end(uint16_t skip, int copy)
{
    int more;

    R[0] = skip;
    gsu_codes(bitmap_row_end_fetch[copy], 1);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[1] = R[8];
    more = loop(GSU_COPY(loop_in_bitmap_row_end[copy]));
    R[2] = gsu_inc(R[2]);
    return more;
}

/* Glyph rows: R12 rows of `bytes` bytes (4 pixels each), R13 the row's
   start; with `rpix_each`, an RPIX after each byte writes the pixel caches
   out. Rows are 32 bytes apart. */
/* gsuwalk: the code of each copy of glyph_rows, read from the ROM */
static const GsuCode glyph_rows_fetch[7][5] = {
    {{0x07f27b, 3}, {0x07f191, 1}, {0x07f27b, 3}, {0x07f159, 2}, {0x07f18d, 5}},
    {{0x07f27b, 3}, {0x07f191, 1}, {0x07f27b, 3}, {0x07f18e, 4}, {0, 0}},
    {{0x07f324, 3}, {0x07f2ac, 1}, {0x07f324, 3}, {0x07f2a8, 5}, {0, 0}},
    {{0x07f3cd, 3}, {0x07f355, 1}, {0x07f3cd, 3}, {0x07f351, 5}, {0, 0}},
    {{0x07f5c2, 3}, {0x07f466, 1}, {0x07f5c2, 3}, {0x07f461, 6}, {0, 0}},
    {{0x07f6d8, 3}, {0x07f5ee, 1}, {0x07f6d8, 3}, {0x07f5e9, 6}, {0, 0}},
    {{0x07f7ed, 3}, {0x07f70b, 1}, {0x07f7ed, 3}, {0x07f706, 6}, {0, 0}},
};

/* gsuwalk: the copy of bitmap_byte each copy of glyph_rows calls, by pass */
static const uint8_t bitmap_byte_in_glyph_rows[7][6] = {{5, 6, 7, 8, 0, 0}, {5, 6, 7, 8, 0, 0}, {9, 10, 0, 0, 0, 0}, {11, 12, 0, 0, 0, 0}, {14, 15, 16, 17, 18, 19}, {20, 21, 22, 23, 0, 0}, {24, 25, 26, 27, 0, 0}};

/* gsuwalk: the copy of rpix each copy of glyph_rows calls, by pass */
static const uint8_t rpix_in_glyph_rows[7][6] = {{6, 7, 8, 9, 0, 0}, {6, 7, 8, 9, 0, 0}, {10, 11, 0, 0, 0, 0}, {12, 13, 0, 0, 0, 0}, {15, 16, 17, 18, 19, 20}, {22, 23, 24, 25, 0, 0}, {0, 0, 0, 0, 0, 0}};

/* gsuwalk: the copy of bitmap_row_end each copy of glyph_rows calls */
static const uint8_t bitmap_row_end_in_glyph_rows[7] = {0, 0, 1, 2, 3, 4, 5};

/* a row about to be drawn at (x, R2) from the bitmap, through the colour
   table at R11, as a span (draw_list.h) */
static void row_span(uint16_t x, uint8_t kind, uint16_t n, uint16_t tu, uint16_t du, uint16_t base)
{
    FxSpan row;
    row.x = (uint8_t)x;
    row.y = (uint8_t)R[2];
    row.n = n;
    row.colour = gsu.colr;
    row.options = gsu.por;
    row.kind = kind;
    row.bank = gsu.rombr;
    row.mode = (uint8_t)(gsu.scmr & 3);
        row.layout = gsu_layout();
    row.tu = tu;
    row.du = du;
    row.tv = row.dv = row.mask = 0;
    row.base = base;
    fx_op_palette_span(&row, R[11]);
}

static void glyph_rows(uint16_t row_start, unsigned bytes, int rpix_each, int copy)
{
    R[13] = row_start;
    gsu_codes(glyph_rows_fetch[copy] + 3, 2);
    do {
        row_span(R[1], 3, (uint16_t)(bytes * 4), 0, 0, R[14]);
        for (unsigned b = 0; b < bytes; b++) {
            bitmap_byte(GSU_COPY(bitmap_byte_in_glyph_rows[copy][b]));
            if (rpix_each)
                R[0] = rpix(GSU_COPY(rpix_in_glyph_rows[copy][b]));
        }
    } while (gsu_codes_loop(glyph_rows_fetch[copy], 2, bitmap_row_end((uint16_t)(32 - bytes), GSU_COPY(bitmap_row_end_in_glyph_rows[copy]))));
    gsu_codes(glyph_rows_fetch[copy] + 2, 1);
}

/* The glyph programs' x: from $020A, with the advance added back to $020A
   for the next glyph. R8 and R1 = x. */
/* gsuwalk: the code of each copy of glyph_x, read from the ROM */
static const GsuCode glyph_x_fetch[7][2] = {
    {{0x07f292, 4}, {0x07f296, 7}},
    {{0x07f33b, 4}, {0x07f33f, 7}},
    {{0x07f3e3, 4}, {0x07f3e7, 7}},
    {{0x07f452, 4}, {0x07f456, 7}},
    {{0x07f5da, 4}, {0x07f5de, 7}},
    {{0x07f6f0, 4}, {0x07f6f4, 7}},
    {{0x07f806, 4}, {0x07f80a, 7}},
};

static void glyph_x(uint16_t advance, int copy)
{
    gsu_codes(glyph_x_fetch[copy], 1);
    R[8] = gsu_ldw(HUD_X);
    R[1] = R[8];
    R[0] = advance;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_codes(glyph_x_fetch[copy] + 1, 1);
    gsu_sbk(R[0]);
}

/* The bitmap's address into R14; false when it is 0 (nothing to draw). */
/* gsuwalk: the code of each copy of glyph_bitmap, read from the ROM */
static const GsuCode glyph_bitmap_fetch[6][2] = {
    {{0x07f138, 4}, {0x07f13c, 2}},
    {{0x07f183, 4}, {0x07f187, 2}},
    {{0x07f29d, 4}, {0x07f2a1, 2}},
    {{0x07f346, 4}, {0x07f34a, 2}},
    {{0x07f6fb, 4}, {0x07f6ff, 2}},
    {{0x07f811, 4}, {0x07f815, 2}},
};

static int glyph_bitmap(int copy)
{
    gsu_codes(glyph_bitmap_fetch[copy], 1);
    gsu_r14(gsu_ldw(HUD_BITMAP));
    gsu_moves(R[14]);
    gsu_codes(glyph_bitmap_fetch[copy] + 1, 1);
    gsu_r14(R[14]);                         /* MOVES R14 writes R14: the ROM buffer refills */
    return !gsu.z;
}

/* The 2-bit to 4-bit tile conversion from R2 to R3: bytes are copied with
   a 0 after each (8 a tile, LDB and STB), then 16 zero bytes; R4 counts
   the 96 tiles down. */
/* gsuwalk: the code of each copy of tiles_2bpp_to_4bpp, read from the ROM */
static const GsuCode tiles_2bpp_to_4bpp_fetch[2][20] = {
    {{0x07ee51, 8}, {0x07ee5f, 1}, {0xee60, 0}, {0x07ee60, 4}, {0x07ee82, 4}, {0x07ee69, 1}, {0x07ee82, 4}, {0x07ee64, 6}, {0x07ee78, 1}, {0x07ee6e, 1}, {0x07ee78, 1}, {0x07ee6a, 5}, {0x07ee6f, 2}, {0x07ee71, 2}, {0x07ee73, 5}, {0x07ee81, 1}, {0x07ee7e, 1}, {0x07ee81, 1}, {0x07ee79, 6}, {0x07ee7f, 2}},
    {{0x07ee5a, 6}, {0xee60, 0}, {0x07ee60, 4}, {0, 0}, {0x07ee82, 4}, {0x07ee69, 1}, {0x07ee82, 4}, {0x07ee64, 6}, {0x07ee78, 1}, {0x07ee6e, 1}, {0x07ee78, 1}, {0x07ee6a, 5}, {0x07ee6f, 2}, {0x07ee71, 2}, {0x07ee73, 5}, {0x07ee81, 1}, {0x07ee7e, 1}, {0x07ee81, 1}, {0x07ee79, 6}, {0x07ee7f, 2}},
};

static void tiles_2bpp_to_4bpp(int copy)
{
    int more;

    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_codes(tiles_2bpp_to_4bpp_fetch[copy], 4);
    gsu_ramb(R[0]);
    R[1] = 0;
    R[4] = 0x60;
    gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 7, 1);
    do {
        R[12] = 8;
        R[13] = 0xEE6E;
        gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 11, 1);
        do {
            gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 12, 1);
            R[0] = gsu_ldb(R[2]);
            gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 13, 1);
            gsu_stb(R[3], (uint8_t)R[0]);
            R[2] = gsu_inc(R[2]);
            R[3] = gsu_inc(R[3]);
            R[0] = gsu_sub(R[0], R[0], 0);
            gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 14, 1);
            gsu_stb(R[3], (uint8_t)R[0]);
            more = loop(GSU_COPY(0));
            R[3] = gsu_inc(R[3]);
        } while (gsu_codes_loop(tiles_2bpp_to_4bpp_fetch[copy] + 8, 2, more));
        gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 10, 1);
        R[12] = 0x10;
        R[13] = 0xEE7E;
        gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 18, 1);
        do {
            gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 19, 1);
            gsu_stb(R[3], (uint8_t)R[0]);
            more = loop(GSU_COPY(1));
            R[3] = gsu_inc(R[3]);
        } while (gsu_codes_loop(tiles_2bpp_to_4bpp_fetch[copy] + 15, 2, more));
        gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 17, 1);
        R[4] = gsu_dec(R[4]);
    } while (gsu_codes_loop(tiles_2bpp_to_4bpp_fetch[copy] + 4, 2, !gsu.z));
    gsu_codes(tiles_2bpp_to_4bpp_fetch[copy] + 6, 1);
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
    R[2] = 0xC000;
    R[3] = 0xD000;
    tiles_2bpp_to_4bpp(GSU_COPY(0));
    gsu_code(0x07ee86, 1);
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
    R[2] = 0x3000;
    R[3] = 0x4000;
    tiles_2bpp_to_4bpp(GSU_COPY(1));
    gsu_code(0x07ee86, 1);
}

void abi_FxTiles2bppTo4bppLow(void)
{
    FxTiles2bppTo4bppLow();
}

/* The noise fill's body (07:EEA0): R9 pixels across, R10 lines down, from
   the top left, the seed at $021C. */
/* gsuwalk: the code of each copy of noise_body, read from the ROM */
static const GsuCode noise_body_fetch[2][22] = {
    {{0x07ee89, 11}, {0x07eea0, 5}, {0x07eee8, 5}, {0x07eeb7, 1}, {0x07eee8, 5}, {0x07eea5, 18}, {0xeeb7, 0}, {0x07eeb7, 1}, {0x07eeb8, 4}, {0x07eebc, 25}, {0x07eedf, 1}, {0x07eebc, 18}, {0x07eed5, 5}, {0x07eed0, 5}, {0x07eedf, 1}, {0x07eebc, 18}, {0x07eed5, 11}, {0x07eee0, 5}, {0x07eee6, 1}, {0x07eebb, 1}, {0x07eee7, 1}, {0x07eef0, 4}},
    {{0x07eea0, 5}, {0, 0}, {0x07eee8, 5}, {0x07eeb7, 1}, {0x07eee8, 5}, {0x07eea5, 18}, {0xeeb7, 0}, {0x07eeb7, 1}, {0x07eeb8, 4}, {0x07eebc, 25}, {0x07eedf, 1}, {0x07eebc, 18}, {0x07eed5, 5}, {0x07eed0, 5}, {0x07eedf, 1}, {0x07eebc, 18}, {0x07eed5, 11}, {0x07eee0, 5}, {0x07eee6, 1}, {0x07eebb, 1}, {0x07eee7, 1}, {0x07eef0, 4}},
};

static void noise_body(int copy)
{
    int more;

    gsu_codes(noise_body_fetch[copy], 2);
    R[8] = gsu_ldw(HUD_NOISE_SEED);
    R[3] = 0xFF00;
    R[4] = 0xAA55;
    R[5] = 0x8180;
    R[6] = NOISE_RAMP;
    R[1] = 0;
    R[2] = 0;
    gsu_codes(noise_body_fetch[copy] + 5, 3);
    do {
        FxSpan row;                          /* the row as a span (draw_list.h) */

        R[12] = R[9];
        R[13] = 0xEEBB;
        row.x = (uint8_t)R[1];
        row.y = (uint8_t)R[2];
        row.n = R[9];
        row.colour = gsu.colr;
        row.options = gsu.por;
        row.kind = 5;
        row.bank = 0;
        row.mode = (uint8_t)(gsu.scmr & 3);
        row.layout = gsu_layout();
        row.tu = R[8];
        row.du = row.base = row.mask = 0;
        fx_op_ramp_span(&row, R[6]);
        gsu_codes(noise_body_fetch[copy] + 8, 1);
        do {
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
                R[0] = gsu_xor(R[0], R[5]);
                R[8] = R[0];
                gsu_codes(noise_body_fetch[copy] + 9, 2);
            } else {
                gsu_sub(R[0], R[4], 0);
                if (gsu.z)
                    { R[0] = 0; gsu_codes(noise_body_fetch[copy] + 15, 2); } else { gsu_codes(noise_body_fetch[copy] + 11, 4); }
                R[8] = R[0];
            }
            R[0] = gsu_and(R[0], 7);
            R[0] = gsu_add(R[0], R[0], 0);
            R[0] = gsu_add(R[0], R[6], 0);
            gsu_codes(noise_body_fetch[copy] + 17, 1);
            R[0] = gsu_ldw(R[0]);
            color(R[0], GSU_COPY(0));
            more = loop(GSU_COPY(2));
            /* the branch's delay slot */
            gsu_codes(noise_body_fetch[copy] + 18, 1); gsu_codes(noise_body_fetch[copy] + 19 + !(more), 1);   /* the branch's delay slot */
            gsu_plot_r1();                      /* PLOT in LOOP's delay slot */
        } while (more);
        R[1] = gsu_sub(R[1], R[1], 0);
        R[10] = gsu_dec(R[10]);
        more = !gsu.z;
        R[2] = gsu_inc(R[2]);
    } while (gsu_codes_loop(noise_body_fetch[copy] + 2, 2, more));
    gsu_codes(noise_body_fetch[copy] + 4, 1);
    R[0] = rpix(GSU_COPY(0));
    gsu_codes(noise_body_fetch[copy] + 21, 1);
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
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.por = (uint8_t)R[0];
    R[9] = 0x10;
    R[10] = 0x80;
    noise_body(GSU_COPY(0));
}

void abi_FxNoiseFill(void)
{
    FxNoiseFill();
}

/* The rising picture: the top `height` ($020C) rows of the picture at
   $15:3D00 + src, `bytes` bytes (4 pixels each) of each row, drawn so
   that the last row lands just above line `base`, at x = R8. R3 counts
   each row's bytes; R13 is the row's start. RPIX at the end writes the
   pixel caches out. */
/* gsuwalk: the code of each copy of picture_rows, read from the ROM */
static const GsuCode picture_rows_fetch[5][12] = {
    {{0x07ef09, 10}, {0x07ef5b, 3}, {0x07ef16, 1}, {0x07ef5b, 3}, {0x07ef13, 4}, {0x07ef52, 3}, {0x07ef1a, 1}, {0x07ef52, 3}, {0x07ef17, 3}, {0xef1a, 0}, {0x07ef1a, 1}, {0x07ef55, 6}},
    {{0x07ef74, 5}, {0x07efc6, 3}, {0x07ef81, 1}, {0x07efc6, 3}, {0x07ef79, 9}, {0x07efbd, 3}, {0x07ef85, 1}, {0x07efbd, 3}, {0x07ef82, 3}, {0xef85, 0}, {0x07ef85, 1}, {0x07efc0, 6}},
    {{0x07efe1, 10}, {0x07f033, 3}, {0x07efee, 1}, {0x07f033, 3}, {0x07efeb, 4}, {0x07f02a, 3}, {0x07eff2, 1}, {0x07f02a, 3}, {0x07efef, 3}, {0xeff2, 0}, {0x07eff2, 1}, {0x07f02d, 6}},
    {{0x07f04c, 5}, {0x07f09e, 3}, {0x07f059, 1}, {0x07f09e, 3}, {0x07f051, 9}, {0x07f095, 3}, {0x07f05d, 1}, {0x07f095, 3}, {0x07f05a, 3}, {0xf05d, 0}, {0x07f05d, 1}, {0x07f098, 6}},
    {{0x07f0c8, 10}, {0x07f11a, 3}, {0x07f0d5, 1}, {0x07f11a, 3}, {0x07f0d2, 4}, {0x07f111, 3}, {0x07f0d9, 1}, {0x07f111, 3}, {0x07f0d6, 3}, {0xf0d9, 0}, {0x07f0d9, 1}, {0x07f114, 6}},
};

/* gsuwalk: the copy of bitmap_byte each copy of picture_rows calls */
static const uint8_t bitmap_byte_in_picture_rows[5] = {0, 1, 2, 3, 4};

/* gsuwalk: the copy of loop each copy of picture_rows calls */
static const uint8_t loop_in_picture_rows[5] = {3, 4, 5, 6, 7};

/* gsuwalk: the copy of rpix each copy of picture_rows calls */
static const uint8_t rpix_in_picture_rows[5] = {1, 2, 3, 4, 5};

static void picture_rows(uint16_t src, uint16_t row_start, uint16_t bytes, int copy)
{
    int more;

    R[1] = R[8];
    gsu_codes(picture_rows_fetch[copy], 1);
    gsu_r14((uint16_t)(PICTURE + src));
    R[12] = R[3];
    R[13] = row_start;
    gsu_codes(picture_rows_fetch[copy] + 4, 1);
    do {
        row_span(R[1], 3, (uint16_t)(bytes * 4), 0, 0, R[14]);
        R[3] = bytes;
        gsu_codes(picture_rows_fetch[copy] + 8, 3);
        do {
            bitmap_byte(GSU_COPY(bitmap_byte_in_picture_rows[copy]));
            R[3] = gsu_dec(R[3]);
        } while (gsu_codes_loop(picture_rows_fetch[copy] + 5, 2, !gsu.z));
        gsu_codes(picture_rows_fetch[copy] + 7, 1);
        R[0] = (uint16_t)(32 - bytes);
        gsu_codes(picture_rows_fetch[copy] + 11, 1);
        gsu_r14(gsu_add(R[0], R[14], 0));
        R[1] = R[8];
        more = loop(GSU_COPY(loop_in_picture_rows[copy]));
        R[2] = gsu_inc(R[2]);
    } while (gsu_codes_loop(picture_rows_fetch[copy] + 1, 2, more));
    gsu_codes(picture_rows_fetch[copy] + 3, 1);
    R[0] = rpix(GSU_COPY(rpix_in_picture_rows[copy]));
}

/* A rising picture in two pieces: picture bytes 14-27 at x = left, then
   bytes 0-9 at x = left + $38, both above line `base`. */
/* gsuwalk: the code of each copy of picture_two_pieces, read from the ROM */
static const GsuCode picture_two_pieces_fetch[2][2] = {
    {{0x07ef00, 9}, {0x07ef68, 12}},
    {{0x07efd8, 9}, {0x07f040, 12}},
};

/* gsuwalk: the copy of bitmap_setup each copy of picture_two_pieces calls */
static const uint8_t bitmap_setup_in_picture_two_pieces[2] = {0, 2};

/* gsuwalk: the copy of picture_rows each copy of picture_two_pieces calls */
static const uint8_t picture_rows_in_picture_two_pieces[2] = {0, 2};

/* gsuwalk: the copy of bitmap_setup each copy of picture_two_pieces calls */
static const uint8_t bitmap_setup_in_picture_two_pieces_2[2] = {1, 3};

/* gsuwalk: the copy of picture_rows each copy of picture_two_pieces calls */
static const uint8_t picture_rows_in_picture_two_pieces_2[2] = {1, 3};

static void picture_two_pieces(uint16_t left, uint16_t base, uint16_t start1,
                               uint16_t start2, int copy)
{
    R[8] = left;
    bitmap_setup(GSU_COPY(bitmap_setup_in_picture_two_pieces[copy]));
    R[11] = HUD_COLOURS;
    gsu_codes(picture_two_pieces_fetch[copy], 1);
    R[3] = gsu_ldw(HUD_Y);
    R[0] = base;
    R[2] = gsu_sub(R[0], R[3], 0);
    picture_rows(0x0E, start1, 0x0E, GSU_COPY(picture_rows_in_picture_two_pieces[copy]));

    bitmap_setup(GSU_COPY(bitmap_setup_in_picture_two_pieces_2[copy]));
    R[11] = HUD_COLOURS;
    R[8] = (uint16_t)(left + 0x38);
    gsu_codes(picture_two_pieces_fetch[copy] + 1, 1);
    R[3] = gsu_ldw(HUD_Y);
    R[1] = R[8];
    R[0] = base;
    R[2] = gsu_sub(R[0], R[3], 0);
    picture_rows(0x00, start2, 0x0A, GSU_COPY(picture_rows_in_picture_two_pieces_2[copy]));
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
    picture_two_pieces(0x1C, 0x40, 0xEF16, 0xEF81, GSU_COPY(0));
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
    picture_two_pieces(0x34, 0x60, 0xEFEE, 0xF059, GSU_COPY(1));
}

void abi_FxPictureRiseSplit60(void)
{
    FxPictureRiseSplit60();
}

/* A rising picture in one piece: picture bytes 0-13 at x = left, above
   line `base`. */
/* gsuwalk: the code of each copy of picture_one_piece, read from the ROM */
static const GsuCode picture_one_piece_fetch[2][1] = {
    {{0x07f0a6, 7}},
    {{0x07f0b5, 7}},
};

/* gsuwalk: the copy of bitmap_setup each copy of picture_one_piece calls */
static const uint8_t bitmap_setup_in_picture_one_piece[2] = {4, 5};

static void picture_one_piece(uint16_t left, uint16_t base, int copy)
{
    R[8] = left;
    gsu_codes(picture_one_piece_fetch[copy], 1);
    R[3] = gsu_ldw(HUD_Y);
    R[0] = base;
    R[2] = gsu_sub(R[0], R[3], 0);
    bitmap_setup(GSU_COPY(bitmap_setup_in_picture_one_piece[copy]));
    R[11] = HUD_COLOURS;
    picture_rows(0x00, 0xF0D5, 0x0E, GSU_COPY(4));
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
    picture_one_piece(0x28, 0x40, GSU_COPY(0));
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
    picture_one_piece(0x4C, 0x60, GSU_COPY(1));
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
    bitmap_setup(GSU_COPY(6));
    gsu_code(0x07f12a, 6);
    R[0] = gsu_ldw(HUD_ADVANCE);
    gsu_code(0x07f130, 4);
    R[8] = gsu_ldw(HUD_X);
    R[1] = R[8];
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07f134, 4);
    gsu_sbk(R[0]);
    if (!glyph_bitmap(GSU_COPY(0)))
        { gsu_code(0x07f13e, 3); return; } else { gsu_code(0x07f13e, 2); gsu_code(0x07f142, 1); }
    R[11] = HUD_COLOURS;
    gsu_code(0x07f143, 7);
    R[2] = gsu_ldw(HUD_Y);
    gsu_code(0x07f14a, 4);
    R[3] = gsu_ldw(HUD_ROWS);
    R[0] = 0x20;
    R[0] = gsu_sub(R[0], R[3], 0);
    R[0] = times_pow2(R[0], 5, GSU_COPY(0));
    gsu_code(0x07f157, 2);
    gsu_r14(gsu_add(R[0], R[14], 0));
    R[12] = R[3];
    glyph_rows(0xF191, 4, 1, GSU_COPY(0));
    gsu_code(0x07f27e, 1);
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
    bitmap_setup(GSU_COPY(7));
    R[11] = HUD_COLOURS;
    gsu_code(0x07f164, 9);
    R[3] = gsu_ldw(HUD_ROWS);
    gsu_code(0x07f16d, 4);
    R[2] = gsu_ldw(HUD_Y);
    R[0] = 0x20;
    R[0] = gsu_add(R[0], R[2], 0);
    R[2] = gsu_sub(R[0], R[3], 0);
    gsu_code(0x07f171, 10);
    R[0] = gsu_ldw(HUD_ADVANCE);
    gsu_code(0x07f17b, 4);
    R[8] = gsu_ldw(HUD_X);
    R[1] = R[8];
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07f17f, 4);
    gsu_sbk(R[0]);
    if (!glyph_bitmap(GSU_COPY(1)))
        { gsu_code(0x07f189, 3); return; } else { gsu_code(0x07f189, 2); gsu_code(0x07f18d, 1); }
    R[12] = R[3];
    glyph_rows(0xF191, 4, 1, GSU_COPY(1));
    gsu_code(0x07f27e, 1);
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
    bitmap_setup(GSU_COPY(9));
    R[11] = HUD_COLOURS;
    gsu_code(0x07f332, 9);
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(8, GSU_COPY(1));
    if (!glyph_bitmap(GSU_COPY(3)))
        { gsu_code(0x07f34c, 3); return; } else { gsu_code(0x07f34c, 2); gsu_code(0x07f350, 1); }                     /* not reached in the traces so far */
    R[12] = 0x10;
    glyph_rows(0xF355, 2, 1, GSU_COPY(3));
    gsu_code(0x07f3d0, 1);
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
    bitmap_setup(GSU_COPY(10));
    R[11] = HUD_COLOURS;
    gsu_code(0x07f449, 9);
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x10, GSU_COPY(3));
    gsu_code(0x07f45d, 4); gsu_cache(0xf461);
    gsu_r14(gsu_ldw(HUD_BITMAP));
    R[12] = 0x10;
    glyph_rows(0xF466, 6, 1, GSU_COPY(4));
    R[0] = rpix(GSU_COPY(21));
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
    bitmap_setup(GSU_COPY(12));
    R[11] = HUD_COLOURS;
    gsu_code(0x07f6e7, 9);
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x10, GSU_COPY(5));
    if (!glyph_bitmap(GSU_COPY(4)))
        { gsu_code(0x07f701, 3); return; } else { gsu_code(0x07f701, 2); gsu_code(0x07f705, 1); gsu_cache(0xf706); }                     /* not reached in the traces so far */
    R[12] = 0x20;
    glyph_rows(0xF70B, 4, 0, GSU_COPY(6));
    R[0] = rpix(GSU_COPY(27));
}

void abi_FxGlyph16x32(void)
{
    FxGlyph16x32();
}

/* ---- scaled sprites ---- */

/* The shared body of the scaled-sprite programs (07:FB25). Draws a sprite
   R5 pixels wide and R7 high at (R8, R9) from the bitmap at $70:0216,
   stepping through the source by $0212 across and $0214 down (8.8 fixed
   point), clipped to the screen ($003A wide, $003E high). Each pixel's
   source is row ((y - top) * step down) and column ((x - left) * step
   across) of the bitmap, 4 pixels a byte; RPIX after each row and at the
   end. A sprite off the screen, or with bitmap 0, draws nothing. */
/* gsuwalk: the code of each copy of scaled_sprite, read from the ROM */
static const GsuCode scaled_sprite_fetch[4][62] = {
    {{0x07fa81, 7}, {0x07faad, 3}, {0x07fb25, 5}, {0x07fb2a, 2}, {0x07fb2c, 2}, {0x07fb2c, 2}, {0x07fb23, 1}, {0x07fb2e, 4}, {0x07fb44, 4}, {0x07fb50, 1}, {0x07fb4e, 3}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb23, 1}, {0x07fb32, 9}, {0x07fb32, 9}, {0x07fb23, 1}, {0x07fb3b, 5}, {0x07fb43, 1}, {0x07fb3b, 9}, {0x07fb51, 4}, {0x07fb65, 4}, {0x07fb6e, 1}, {0x07fb6d, 2}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fbba, 1}, {0x07fb55, 8}, {0x07fb55, 8}, {0x07fb23, 1}, {0x07fb5d, 5}, {0x07fb64, 1}, {0x07fb5d, 8}, {0x07fb6f, 6}, {0x07fb75, 4}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fb78, 1}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fbb2, 1}, {0x07fb7d, 1}, {0x07fbb2, 1}, {0x07fb79, 5}, {0x07fb7e, 4}, {0x07fb82, 4}, {0x07fb91, 1}, {0x07fb82, 9}, {0x07fb90, 2}, {0x07fb92, 6}, {0x07fb98, 6}, {0x07fb9e, 1}, {0x07fba6, 4}, {0x07fba1, 1}, {0x07fb9f, 3}, {0x07fba2, 4}, {0x07fba2, 4}, {0x07fbaa, 1}},
    {{0x07faab, 5}, {0x07fb25, 5}, {0, 0}, {0x07fb2a, 2}, {0x07fb2c, 2}, {0x07fb2c, 2}, {0x07fb23, 1}, {0x07fb2e, 4}, {0x07fb44, 4}, {0x07fb50, 1}, {0x07fb4e, 3}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb23, 1}, {0x07fb32, 9}, {0x07fb32, 9}, {0x07fb23, 1}, {0x07fb3b, 5}, {0x07fb43, 1}, {0x07fb3b, 9}, {0x07fb51, 4}, {0x07fb65, 4}, {0x07fb6e, 1}, {0x07fb6d, 2}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fbba, 1}, {0x07fb55, 8}, {0x07fb55, 8}, {0x07fb23, 1}, {0x07fb5d, 5}, {0x07fb64, 1}, {0x07fb5d, 8}, {0x07fb6f, 6}, {0x07fb75, 4}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fb78, 1}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fbb2, 1}, {0x07fb7d, 1}, {0x07fbb2, 1}, {0x07fb79, 5}, {0x07fb7e, 4}, {0x07fb82, 4}, {0x07fb91, 1}, {0x07fb82, 9}, {0x07fb90, 2}, {0x07fb92, 6}, {0x07fb98, 6}, {0x07fb9e, 1}, {0x07fba6, 4}, {0x07fba1, 1}, {0x07fb9f, 3}, {0x07fba2, 4}, {0x07fba2, 4}, {0x07fbaa, 1}},
    {{0x07fb26, 4}, {0, 0}, {0, 0}, {0x07fb2a, 2}, {0x07fb2c, 2}, {0x07fb2c, 2}, {0x07fb23, 1}, {0x07fb2e, 4}, {0x07fb44, 4}, {0x07fb50, 1}, {0x07fb4e, 3}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb23, 1}, {0x07fb32, 9}, {0x07fb32, 9}, {0x07fb23, 1}, {0x07fb3b, 5}, {0x07fb43, 1}, {0x07fb3b, 9}, {0x07fb51, 4}, {0x07fb65, 4}, {0x07fb6e, 1}, {0x07fb6d, 2}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fbba, 1}, {0x07fb55, 8}, {0x07fb55, 8}, {0x07fb23, 1}, {0x07fb5d, 5}, {0x07fb64, 1}, {0x07fb5d, 8}, {0x07fb6f, 6}, {0x07fb75, 4}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fb78, 1}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fbb2, 1}, {0x07fb7d, 1}, {0x07fbb2, 1}, {0x07fb79, 5}, {0x07fb7e, 4}, {0x07fb82, 4}, {0x07fb91, 1}, {0x07fb82, 9}, {0x07fb90, 2}, {0x07fb92, 6}, {0x07fb98, 6}, {0x07fb9e, 1}, {0x07fba6, 4}, {0x07fba1, 1}, {0x07fb9f, 3}, {0x07fba2, 4}, {0x07fba2, 4}, {0x07fbaa, 1}},
    {{0x07faeb, 5}, {0x07fb25, 5}, {0, 0}, {0x07fb2a, 2}, {0x07fb2c, 2}, {0x07fb2c, 2}, {0x07fb23, 1}, {0x07fb2e, 4}, {0x07fb44, 4}, {0x07fb50, 1}, {0x07fb4e, 3}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb32, 4}, {0x07fb48, 6}, {0x07fb23, 1}, {0x07fb32, 9}, {0x07fb32, 9}, {0x07fb23, 1}, {0x07fb3b, 5}, {0x07fb43, 1}, {0x07fb3b, 9}, {0x07fb51, 4}, {0x07fb65, 4}, {0x07fb6e, 1}, {0x07fb6d, 2}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fb55, 4}, {0x07fb68, 5}, {0x07fbba, 1}, {0x07fb55, 8}, {0x07fb55, 8}, {0x07fb23, 1}, {0x07fb5d, 5}, {0x07fb64, 1}, {0x07fb5d, 8}, {0x07fb6f, 6}, {0x07fb75, 4}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fb78, 1}, {0xfb79, 0}, {0x07fbb5, 5}, {0x07fbb2, 1}, {0x07fb7d, 1}, {0x07fbb2, 1}, {0x07fb79, 5}, {0x07fb7e, 4}, {0x07fb82, 4}, {0x07fb91, 1}, {0x07fb82, 9}, {0x07fb90, 2}, {0x07fb92, 6}, {0x07fb98, 6}, {0x07fb9e, 1}, {0x07fba6, 4}, {0x07fba1, 1}, {0x07fb9f, 3}, {0x07fba2, 4}, {0x07fba2, 4}, {0x07fbaa, 1}},
};

static void scaled_sprite(int copy)
{
    int more, zero;

    gsu_codes(scaled_sprite_fetch[copy], 3);
    gsu_r14(gsu_ldw(HUD_BITMAP));
    gsu_moves(R[14]);
    gsu_codes(scaled_sprite_fetch[copy] + 3, 1);
    gsu_r14(R[14]);                         /* MOVES R14 writes R14: the ROM buffer refills */
    if (gsu.z)
        { gsu_codes(scaled_sprite_fetch[copy] + 5, 2); return; } else { gsu_codes(scaled_sprite_fetch[copy] + 4, 1); }
    gsu_codes(scaled_sprite_fetch[copy] + 7, 1);
    R[3] = gsu_ldw(SCREEN_WIDTH);
    gsu_moves(R[8]);
    if (gsu.s) {
        /* Off the left edge: the part right of x = 0. */
        R[5] = gsu_add(R[8], R[5], 0);
        if (gsu.s)
            { gsu_codes(scaled_sprite_fetch[copy] + 13, 3); return; } else { gsu_codes(scaled_sprite_fetch[copy] + 11, 2); }
        R[1] = gsu_sub(R[1], R[1], 0);
        gsu_codes(scaled_sprite_fetch[copy] + 10, 1);
    } else {
        R[0] = gsu_sub(R[8], R[3], 0);
        if (gsu.cy)
            { gsu_codes(scaled_sprite_fetch[copy] + 17, 2); return; } else { gsu_codes(scaled_sprite_fetch[copy] + 16, 1); }
        R[0] = gsu_add(R[8], R[5], 0);
        R[0] = gsu_sub(R[0], R[3], 0);
        if (gsu.cy)
            { R[5] = gsu_sub(R[3], R[8], 0); gsu_codes(scaled_sprite_fetch[copy] + 21, 1); } else { gsu_codes(scaled_sprite_fetch[copy] + 19, 2); }  /* off the right edge */
        R[1] = R[8];
        gsu_codes(scaled_sprite_fetch[copy] + 8, 2);
    }
    gsu_codes(scaled_sprite_fetch[copy] + 22, 1);
    R[4] = gsu_ldw(SCREEN_HEIGHT);
    gsu_moves(R[9]);
    if (gsu.s) {
        /* Off the top: the part below y = 0. */
        R[7] = gsu_add(R[9], R[7], 0);
        if (gsu.s) {
            gsu_codes(scaled_sprite_fetch[copy] + 28, 3);
            R[2] = rpix(GSU_COPY(36));          /* a WITH R2 in the delay slot */
            return;
        } else { gsu_codes(scaled_sprite_fetch[copy] + 26, 2); }
        R[2] = gsu_sub(R[2], R[2], 0);
        gsu_codes(scaled_sprite_fetch[copy] + 25, 1);
    } else {
        R[0] = gsu_sub(R[9], R[4], 0);
        if (gsu.cy)
            { gsu_codes(scaled_sprite_fetch[copy] + 32, 2); return; } else { gsu_codes(scaled_sprite_fetch[copy] + 31, 1); }
        R[0] = gsu_add(R[9], R[7], 0);
        R[0] = gsu_sub(R[0], R[4], 0);
        if (gsu.cy)
            { R[7] = gsu_sub(R[4], R[9], 0); gsu_codes(scaled_sprite_fetch[copy] + 36, 1); } else { gsu_codes(scaled_sprite_fetch[copy] + 34, 2); }  /* off the bottom */
        R[2] = R[9];
        gsu_codes(scaled_sprite_fetch[copy] + 23, 2);
    }
    R[10] = R[1];
    gsu_codes(scaled_sprite_fetch[copy] + 37, 1);
    R[3] = gsu_ldw(HUD_STEP_X);
    gsu_codes(scaled_sprite_fetch[copy] + 38, 2);
    R[4] = gsu_ldw(HUD_STEP_Y);
    do {
        uint16_t row_x = R[1];
        int first = 1;

        R[12] = R[5];
        R[13] = 0xFB7D;
        gsu_codes(scaled_sprite_fetch[copy] + 47, 1);
        do {
            gsu_codes(scaled_sprite_fetch[copy] + 48, 1);
            gsu_r14(gsu_ldw(HUD_BITMAP));
            R[0] = gsu_sub(R[2], R[9], 0);
            if (!gsu.z) {
                R[0] = gsu_swap(R[0]);
                R[6] = R[4];
                gsu_codes(scaled_sprite_fetch[copy] + 51, 1);
                R[0] = gsu_fmult(R[0], R[6]);
                R[0] = times_pow2(R[0], 5, GSU_COPY(3));
                gsu_codes(scaled_sprite_fetch[copy] + 52, 1);
                gsu_r14(gsu_add(R[0], R[14], 0));
            } else { gsu_codes(scaled_sprite_fetch[copy] + 49, 2); }
            if (first) {
                /* the row as a span: the row's start in the bitmap, as each
                   pixel works it out */
                row_span(row_x, 4, R[5], (uint16_t)(row_x - R[8]), R[3], R[14]);
                first = 0;
            }
            R[0] = gsu_sub(R[1], R[8], 0);
            R[0] = gsu_swap(R[0]);
            R[6] = R[3];
            gsu_codes(scaled_sprite_fetch[copy] + 53, 1);
            R[0] = gsu_fmult(R[0], R[6]);
            R[6] = R[0];
            R[0] = gsu_asr(R[0]);
            R[0] = gsu_asr(R[0]);
            gsu_codes(scaled_sprite_fetch[copy] + 54, 1);
            gsu_r14(gsu_add(R[0], R[14], 0));
            gsu_codes(scaled_sprite_fetch[copy] + 55, 1);
            R[0] = (gsu_getb());
            R[6] = gsu_and(R[6], 3);
            gsu_codes(scaled_sprite_fetch[copy] + 58, 1);
            for (;;) {
                gsu_moves(R[6]);
                zero = gsu.z;
                R[6] = gsu_dec(R[6]);
                if (zero)
                    { gsu_codes(scaled_sprite_fetch[copy] + 60, 2); break; } else { gsu_codes(scaled_sprite_fetch[copy] + 59, 1); }
                R[0] = gsu_asr(R[0]);
                R[0] = gsu_asr(R[0]);
                gsu_codes(scaled_sprite_fetch[copy] + 56, 2);
            }
            pixel(GSU_COPY(112));
            more = loop(GSU_COPY(16));
        } while (gsu_codes_loop(scaled_sprite_fetch[copy] + 44, 2, more));
        gsu_codes(scaled_sprite_fetch[copy] + 46, 1);
        R[0] = rpix(GSU_COPY(37));
        R[1] = gsu_sub(R[1], R[5], 0);
        R[7] = gsu_dec(R[7]);
        more = !gsu.z;
        R[2] = gsu_inc(R[2]);
    } while (gsu_codes_loop(scaled_sprite_fetch[copy] + 40, 3, more));
    gsu_codes(scaled_sprite_fetch[copy] + 43, 1);
    R[0] = rpix(GSU_COPY(38));
}

/* The scaled-sprite programs' setup after their size is worked out. */
/* gsuwalk: the copy of bitmap_setup each copy of scaled_setup calls */
static const uint8_t bitmap_setup_in_scaled_setup[3] = {14, 15, 17};

static void scaled_setup(int copy)
{
    bitmap_setup(GSU_COPY(bitmap_setup_in_scaled_setup[copy]));
    R[11] = HUD_COLOURS;
}

/* FxScaledSprite13 and FxScaledSprite12 from their height (07:FA4C): y,
   and (size down) / 16 lines, then the shared body. */
static void scaled_height_16(void)
{
    gsu_code(0x07fa4d, 4);
    R[9] = gsu_ldw(HUD_Y);
    R[0] = gsu_add(R[4], R[4], 0);
    R[0] = times_pow2(R[0], 3, GSU_COPY(2));
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
    gsu_code(0x07fa1c, 4);
    R[3] = gsu_ldw(HUD_ADVANCE);
    gsu_code(0x07fa20, 4);
    R[4] = gsu_ldw(HUD_ROWS);
    gsu_code(0x07fa24, 4);
    R[8] = gsu_ldw(HUD_X);
    R[0] = gsu_div2(R[3]);
    gsu_code(0x07fa28, 5);
    R[0] = gsu_mult(R[0], 13);
    R[0] = times_pow2(R[0], 4, GSU_COPY(1));
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
    gsu_code(0x07fa5b, 4);
    R[3] = gsu_ldw(HUD_ADVANCE);
    gsu_code(0x07fa5f, 4);
    R[4] = gsu_ldw(HUD_ROWS);
    gsu_code(0x07fa63, 4);
    R[8] = gsu_ldw(HUD_X);
    R[0] = gsu_add(R[3], R[3], 0);
    R[0] = times_pow2(R[0], 2, GSU_COPY(5));
    R[5] = gsu_hib(R[0]);
    R[0] = gsu_add(R[5], R[8], 0);
    gsu_code(0x07fa6f, 4);
    R[9] = gsu_ldw(HUD_Y);
    R[0] = gsu_add(R[4], R[4], 0);
    R[0] = times_pow2(R[0], 2, GSU_COPY(6));
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
    scaled_setup(GSU_COPY(1));
    R[3] = 0x0100;
    R[4] = 0x0100;
    gsu_code(0x07fa90, 15);
    R[8] = gsu_ldw(HUD_X);
    R[0] = 0x18;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07fa9f, 5);
    gsu_sbk(R[0]);
    R[5] = 0x18;
    gsu_code(0x07faa4, 7);
    R[9] = gsu_ldw(HUD_Y);
    R[7] = 0x28;
    scaled_sprite(GSU_COPY(1));
}

void abi_FxSprite24x40(void)
{
    FxSprite24x40();
}

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
    scaled_setup(GSU_COPY(2));
    gsu_code(0x07fafa, 9);
    R[3] = gsu_ldw(HUD_ADVANCE);
    gsu_code(0x07fb03, 4);
    R[4] = gsu_ldw(HUD_ROWS);
    gsu_code(0x07fb07, 4);
    R[8] = gsu_ldw(HUD_X);
    R[0] = gsu_add(R[3], R[3], 0);
    R[0] = times_pow2(R[0], 3, GSU_COPY(7));
    R[5] = gsu_hib(R[0]);
    R[0] = gsu_add(R[5], R[8], 0);
    gsu_code(0x07fb14, 1);
    gsu_sbk(R[0]);
    gsu_code(0x07fb15, 4);
    R[9] = gsu_ldw(HUD_Y);
    R[0] = gsu_add(R[4], R[4], 0);
    R[0] = times_pow2(R[0], 4, GSU_COPY(8));
    R[7] = gsu_hib(R[0]);
    scaled_sprite(GSU_COPY(2));
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
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu.por = (uint8_t)R[0];
    gsu_code(0x07ee95, 7);
    R[9] = gsu_ldw(SCREEN_WIDTH);
    gsu_code(0x07ee9c, 4);
    R[10] = gsu_ldw(SCREEN_HEIGHT);
    R[10] = gsu_inc(R[10]);
    noise_body(GSU_COPY(1));
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
    bitmap_setup(GSU_COPY(8));
    R[11] = HUD_COLOURS;
    gsu_code(0x07f289, 9);
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(8, GSU_COPY(0));
    if (!glyph_bitmap(GSU_COPY(2)))
        { gsu_code(0x07f2a3, 3); return; } else { gsu_code(0x07f2a3, 2); gsu_code(0x07f2a7, 1); }
    R[12] = 8;
    glyph_rows(0xF2AC, 2, 1, GSU_COPY(2));
    gsu_code(0x07f327, 1);
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
    int more;

    R[0] = 0x04;
    gsu_code(0x07f3d3, 4);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x07f3d7, 3);
    gsu_ramb(R[0]);
    gsu.por = (uint8_t)R[0];
    R[11] = HUD_COLOURS;
    gsu_code(0x07f3da, 9);
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x10, GSU_COPY(2));
    gsu_code(0x07f3ee, 4); gsu_cache(0xf3f2);
    gsu_r14(gsu_ldw(HUD_BITMAP));
    R[12] = 0x20;
    R[13] = 0xF3F7;
    gsu_code(0x07f3f2, 6);
    do {
        R[3] = 6;
        gsu_code(0x07f3f8, 2);
        do {
            bitmap_byte(GSU_COPY(13));
            R[3] = gsu_dec(R[3]);
        } while (gsu_code_loop(0x07f3f9, 1, gsu_code_loop(0x07f431, 3, !gsu.z)));
        gsu_code(0x07f431, 3);
        R[0] = 0x1A;
        gsu_code(0x07f434, 6);
        gsu_r14(gsu_add(R[0], R[14], 0));
        R[1] = R[8];
        more = loop(GSU_COPY(11));
        R[2] = gsu_inc(R[2]);
    } while (gsu_code_loop(0x07f3f7, 1, gsu_code_loop(0x07f43a, 3, more)));
    gsu_code(0x07f43a, 3);
    R[0] = rpix(GSU_COPY(14));
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
    bitmap_setup(GSU_COPY(11));
    R[11] = HUD_COLOURS;
    gsu_code(0x07f5d1, 9);
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x10, GSU_COPY(4));
    gsu_code(0x07f5e5, 4); gsu_cache(0xf5e9);
    gsu_r14(gsu_ldw(HUD_BITMAP));
    R[12] = 0x10;
    glyph_rows(0xF5EE, 4, 1, GSU_COPY(5));
    R[0] = rpix(GSU_COPY(26));
}

void abi_FxGlyph16x16(void)
{
    FxGlyph16x16();
}

/* One bitmap byte at double width: as bitmap_byte, each pixel plotted
   twice. */
/* gsuwalk: the code of each copy of bitmap_byte_double, read from the ROM */
static const GsuCode bitmap_byte_double_fetch[8][6] = {
    {{0x07f821, 2}, {0x07f823, 7}, {0x07f82d, 5}, {0x07f832, 11}, {0x07f840, 11}, {0x07f84e, 11}},
    {{0x07f85e, 2}, {0x07f860, 7}, {0x07f86a, 5}, {0x07f86f, 11}, {0x07f87d, 11}, {0x07f88b, 11}},
    {{0x07f89b, 2}, {0x07f89d, 7}, {0x07f8a7, 5}, {0x07f8ac, 11}, {0x07f8ba, 11}, {0x07f8c8, 11}},
    {{0x07f8d8, 2}, {0x07f8da, 7}, {0x07f8e4, 5}, {0x07f8e9, 11}, {0x07f8f7, 11}, {0x07f905, 11}},
    {{0x07f91a, 5}, {0x07f91f, 7}, {0x07f929, 5}, {0x07f92e, 11}, {0x07f93c, 11}, {0x07f94a, 11}},
    {{0x07f95a, 2}, {0x07f95c, 7}, {0x07f966, 5}, {0x07f96b, 11}, {0x07f979, 11}, {0x07f987, 11}},
    {{0x07f997, 2}, {0x07f999, 7}, {0x07f9a3, 5}, {0x07f9a8, 11}, {0x07f9b6, 11}, {0x07f9c4, 11}},
    {{0x07f9d4, 2}, {0x07f9d6, 7}, {0x07f9e0, 5}, {0x07f9e5, 11}, {0x07f9f3, 11}, {0x07fa01, 11}},
};

/* gsuwalk: the copy of color each copy of bitmap_byte_double calls */
static const uint8_t color_in_bitmap_byte_double[8] = {113, 117, 121, 125, 129, 133, 137, 141};

/* gsuwalk: the copy of plot each copy of bitmap_byte_double calls */
static const uint8_t plot_in_bitmap_byte_double[8] = {112, 120, 128, 136, 144, 152, 160, 168};

/* gsuwalk: the copy of plot each copy of bitmap_byte_double calls */
static const uint8_t plot_in_bitmap_byte_double_2[8] = {113, 121, 129, 137, 145, 153, 161, 169};

/* gsuwalk: the copy of color each copy of bitmap_byte_double calls, by pass */
static const uint8_t color_in_bitmap_byte_double_2[8][3] = {{114, 115, 116}, {118, 119, 120}, {122, 123, 124}, {126, 127, 128}, {130, 131, 132}, {134, 135, 136}, {138, 139, 140}, {142, 143, 144}};

/* gsuwalk: the copy of plot each copy of bitmap_byte_double calls, by pass */
static const uint8_t plot_in_bitmap_byte_double_3[8][3] = {{114, 116, 118}, {122, 124, 126}, {130, 132, 134}, {138, 140, 142}, {146, 148, 150}, {154, 156, 158}, {162, 164, 166}, {170, 172, 174}};

/* gsuwalk: the copy of plot each copy of bitmap_byte_double calls, by pass */
static const uint8_t plot_in_bitmap_byte_double_4[8][3] = {{115, 117, 119}, {123, 125, 127}, {131, 133, 135}, {139, 141, 143}, {147, 149, 151}, {155, 157, 159}, {163, 165, 167}, {171, 173, 175}};

static void bitmap_byte_double(int copy)
{
    gsu_codes(bitmap_byte_double_fetch[copy], 1);
    R[6] = gsu_getb();
    R[0] = R[6];
    R[0] = gsu_and(R[0], 3);
    R[0] = gsu_add(R[0], R[11], 0);
    gsu_codes(bitmap_byte_double_fetch[copy] + 1, 1);
    R[0] = gsu_ldb(R[0]);
    color(R[0], GSU_COPY(color_in_bitmap_byte_double[copy]));
    plot(GSU_COPY(plot_in_bitmap_byte_double[copy]));
    plot(GSU_COPY(plot_in_bitmap_byte_double_2[copy]));
    R[6] = gsu_ror(R[6]);
    R[6] = gsu_ror(R[6]);
    gsu_codes(bitmap_byte_double_fetch[copy] + 2, 1);
    gsu_r14(gsu_inc(R[14]));
    for (int k = 0; k < 3; k++) {
        R[0] = R[6];
        R[6] = gsu_ror(R[6]);
        R[6] = gsu_ror(R[6]);
        R[0] = gsu_and(R[0], 3);
        R[0] = gsu_add(R[0], R[11], 0);
        gsu_codes(bitmap_byte_double_fetch[copy] + 3 + k, 1);
        R[0] = gsu_ldb(R[0]);
        color(R[0], GSU_COPY(color_in_bitmap_byte_double_2[copy][k]));
        plot(GSU_COPY(plot_in_bitmap_byte_double_3[copy][k]));
        plot(GSU_COPY(plot_in_bitmap_byte_double_4[copy][k]));
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
/* gsuwalk: the copy of bitmap_byte_double each copy of FxGlyph16x32Double calls, by pass */
static const uint8_t bitmap_byte_double_in_FxGlyph16x32Double[1][4] = {{0, 1, 2, 3}};

/* gsuwalk: the copy of rpix each copy of FxGlyph16x32Double calls, by pass */
static const uint8_t rpix_in_FxGlyph16x32Double[1][4] = {{28, 29, 30, 31}};

/* gsuwalk: the copy of bitmap_byte_double each copy of FxGlyph16x32Double calls, by pass */
static const uint8_t bitmap_byte_double_in_FxGlyph16x32Double_2[1][4] = {{4, 5, 6, 7}};

/* gsuwalk: the copy of rpix each copy of FxGlyph16x32Double calls, by pass */
static const uint8_t rpix_in_FxGlyph16x32Double_2[1][4] = {{32, 33, 34, 35}};

void FxGlyph16x32Double(void)
{
    bitmap_setup(GSU_COPY(13));
    R[11] = HUD_COLOURS;
    gsu_code(0x07f7fd, 9);
    R[2] = gsu_ldw(HUD_Y);
    glyph_x(0x20, GSU_COPY(6));
    if (!glyph_bitmap(GSU_COPY(5)))
        { gsu_code(0x07f817, 3); return; } else { gsu_code(0x07f817, 2); gsu_code(0x07f81b, 1); }
    R[12] = 0x20;
    R[13] = 0xF820;
    gsu_code(0x07f81c, 5);
    do {
        for (int b = 0; b < 4; b++) {
            bitmap_byte_double(GSU_COPY(bitmap_byte_double_in_FxGlyph16x32Double[0][b]));
            R[0] = rpix(GSU_COPY(rpix_in_FxGlyph16x32Double[0][b]));
        }
        R[0] = 0xFFFC;
        gsu_code(0x07f915, 5);
        gsu_r14(gsu_add(R[0], R[14], 0));
        R[1] = R[8];
        R[2] = gsu_inc(R[2]);
        for (int b = 0; b < 4; b++) {
            bitmap_byte_double(GSU_COPY(bitmap_byte_double_in_FxGlyph16x32Double_2[0][b]));
            R[0] = rpix(GSU_COPY(rpix_in_FxGlyph16x32Double_2[0][b]));
        }
    } while (gsu_code_loop(0x07f820, 1, gsu_code_loop(0x07fa16, 3, bitmap_row_end(0x1C, GSU_COPY(6)))));
    gsu_code(0x07fa16, 3);
    gsu_code(0x07fa19, 1);
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
    gsu_code(0x07fa36, 4);
    R[3] = gsu_ldw(HUD_ADVANCE);
    gsu_code(0x07fa3a, 4);
    R[4] = gsu_ldw(HUD_ROWS);
    gsu_code(0x07fa3e, 4);
    R[8] = gsu_ldw(HUD_X);
    R[0] = gsu_div2(R[3]);
    gsu_code(0x07fa42, 5);
    R[0] = gsu_mult(R[0], 12);
    R[0] = times_pow2(R[0], 4, GSU_COPY(4));
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
    bitmap_setup(GSU_COPY(16));
    R[0] = 0x0100;
    gsu_code(0x07fab8, 9);
    gsu_stw(HUD_STEP_X, R[0]);
    gsu_code(0x07fac1, 4);
    gsu_stw(HUD_STEP_Y, R[0]);
    R[11] = HUD_COLOURS;
    R[3] = 0x0100;
    R[4] = 0x0100;
    gsu_code(0x07fac5, 13);
    R[8] = gsu_ldw(HUD_X);
    R[0] = 0xFFF0;
    R[0] = gsu_sub(R[0], R[8], 0);
    if (gsu.cy) {
        R[0] = 0x00D0;
        R[0] = gsu_sub(R[0], R[8], 0);
        if (!gsu.cy) {
            R[0] = 0x0100;
            gsu_code(0x07fad2, 13); gsu_code(0x07faf0, 3);
            FxScaledSpriteAdvance(GSU_COPY(0));
            return;
        } else { gsu_code(0x07fad2, 13); }
    } else { gsu_code(0x07fad2, 6); gsu_code(0x07fade, 1); }
    R[0] = 0x10;
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_code(0x07fadf, 5);
    gsu_sbk(R[0]);
    R[5] = 0x10;
    gsu_code(0x07fae4, 7);
    R[9] = gsu_ldw(HUD_Y);
    R[7] = 0x20;
    scaled_sprite(GSU_COPY(3));
}

void abi_FxSprite16x32(void)
{
    FxSprite16x32();
}
