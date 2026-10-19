/*
 * raster.c: the Super FX drawing the frame's queued shapes into its screen
 * buffer. The 3D object code (01:8F7D) queues each shape's screen points
 * in a list at $08AA, the word at $005C pointing past the last entry; an
 * entry starts with a type byte:
 *
 *   3 or more  a flat polygon of that many vertices: a colour byte, then
 *              four bytes a vertex (x, a byte unused, y, a byte unused)
 *   2          a line: a colour byte, then x1, y1, x2, y2 two bytes each
 *   negative   a textured polygon of (NOT type) & $FF vertices: a ROM bank
 *              byte, a word (low byte the colour, high byte the plot
 *              options), the texture's address and mask words, then
 *              eight bytes a vertex (x, y, u, v, each a byte and a byte
 *              unused; u and v are moved down a byte as they are read)
 *   0          the end, which FxDrawList writes itself
 *
 * Polygons are filled a row at a time from the top vertex down, one walk
 * down the left edges and one down the right, each edge's x stepped by a
 * slope in 8.8 from a table of reciprocals in ROM ($00:94A0, a word for
 * each height). The vertices are first copied once more just below the
 * entry, so that both walks can run past the end of the vertex list.
 *
 * The C follows the code register for register: every register, flag and
 * RAM access as the instructions leave them, and a branch's delay slot
 * (the instruction after it, which always runs) after the branch decision.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define R gsu.r

#define FX_LIST_END     0x005Cu  /* past the last entry of the list at $08AA */
#define FX_LIST_STACK   0x005Eu  /* R10 as FxDrawList left it */
#define FX_LEFT_AT      0x0060u  /* the left edges' walk through the vertices */
#define FX_RIGHT_AT     0x0062u  /* the right edges' walk */
#define FX_LEFT_ROWS    0x0064u  /* rows left on the left edge after a run */
#define FX_RIGHT_ROWS   0x0066u
#define FX_LEFT_SLOPE   0x006Cu
#define FX_RIGHT_SLOPE  0x006Eu
#define FX_LEFT_U       0x0070u  /* u and v along each edge, 8.8 */
#define FX_RIGHT_U      0x0072u
#define FX_LEFT_U_PREV  0x0074u
#define FX_RIGHT_U_PREV 0x0076u
#define FX_LEFT_DU      0x0078u
#define FX_RIGHT_DU     0x007Au
#define FX_LEFT_V       0x007Cu
#define FX_RIGHT_V      0x007Eu
#define FX_LEFT_V_PREV  0x0080u
#define FX_RIGHT_V_PREV 0x0082u
#define FX_LEFT_DV      0x0084u
#define FX_RIGHT_DV     0x0086u
#define FX_TEX_BANK     0x00A6u
#define FX_TEX_BASE     0x00A8u
#define FX_TEX_U_OFF    0x00C0u
#define FX_TEX_V_OFF    0x00C2u
#define FX_TEX_MASK     0x00D4u
#define FX_ROM_BANK     0x001Cu

#define DRAW_LIST       0x08AAu
#define RECIPROCALS     0x94A0u  /* 1/n words in ROM bank 0 */

/* R14 at the reciprocal of n (IWT R0, #$94A0; ADD Rn; TO R14; ADD Rn),
   then the word read into R6 (TO R6; GETB; INC R14; WITH R6; GETBH). The
   difference of x (or R0 as set between) is not part of it. */
static void reciprocal_at(uint16_t n, int copy)
{
    R[0] = RECIPROCALS;
    R[0] = gsu_add(R[0], n, 0);
    gsu_r14(gsu_add(R[0], n, 0));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode reciprocal_read_code[3][1] = {
    {{0, 0}},
    {{0x01AA2E, 1}},
    {{0x01A74B, 1}},
};

static void reciprocal_read(int copy)
{
    R[6] = gsu_getb();
    gsu_codes(reciprocal_read_code[copy], 1);
    gsu_r14(gsu_inc(R[14]));
}

/* An edge's x slope from the difference in R0 and the reciprocal in R6:
   LMULT; WITH R4; ADD R4; ROL; LOB; SWAP; WITH R4; HIB; TO dst; OR R4
   (the product's middle word, rounded by the low word's top bit). */
static uint16_t slope(void)
{
    uint32_t p = gsu_lmult(R[0], R[6]);

    R[0] = (uint16_t)(p >> 16);
    R[4] = (uint16_t)p;
    R[4] = gsu_add(R[4], R[4], 0);
    R[0] = gsu_rol(R[0]);
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_swap(R[0]);
    R[4] = gsu_hib(R[4]);
    return gsu_or(R[0], R[4]);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode edge_step_code[9][3] = {
    {{0x01AA50, 7}, {0, 0}, {0x01AA57, 2}},
    {{0x01AA5E, 7}, {0, 0}, {0x01AA65, 2}},
    {{0x01A7AD, 7}, {0, 0}, {0x01A7B4, 2}},
    {{0x01A7BB, 7}, {0, 0}, {0x01A7C2, 2}},
    {{0, 0}, {0, 0}, {0, 0}},
    {{0x01AA50, 7}, {0, 0}, {0, 0}},
    {{0x01AA5E, 7}, {0, 0}, {0, 0}},
    {{0x01A7AD, 7}, {0, 0}, {0, 0}},
    {{0x01A7AD, 7}, {0x01A7BB, 7}, {0, 0}},
};

/* WITH x; ADD step; BPL; DEC rows (the delay slot); IWT R0, #$0800;
   ADD x; BMI; NOP; IBT x, #0: an edge's x stepped, set to 0 when it has
   gone just below it (to -8.0). */
static void edge_step(int x, uint16_t step, int rows, int copy)
{
    int plus;

    R[x] = gsu_add(R[x], step, 0);
    plus = !gsu.s;
    if (rows >= 0)
        R[rows] = gsu_dec(R[rows]);
    if (plus)
        return; else { gsu_codes(edge_step_code[copy], 2); }
    R[0] = gsu_add(0x0800, R[x], 0);
    if (gsu.s)
        return; else { gsu_codes(edge_step_code[copy] + 2, 1); }
    R[x] = 0;
}

/* A difference of u or v in R0 to a step: LOB; SWAP; FMULT; ROL. */
static uint16_t uv_step(void)
{
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_fmult(R[0], R[6]);
    return gsu_rol(R[0]);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode flat_copy_code[5][17] = {
    {{0x01A976, 11}, {0x01A981, 3}, {0x01A984, 2}, {0x01A986, 6}, {0x01A98C, 2}, {0x01A992, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A98E, 4}, {0x01A996, 4}, {0x01A99C, 3}, {0x01A98E, 4}, {0x01A996, 4}, {0x01A99C, 10}, {0, 0}},
    {{0x01A976, 11}, {0x01A981, 3}, {0x01A984, 2}, {0x01A986, 6}, {0x01A98C, 2}, {0x01A98E, 12}, {0x01A99C, 3}, {0x01A98E, 4}, {0x01A996, 4}, {0x01A99C, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A99F, 7}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01A976, 11}, {0x01A981, 3}, {0x01A984, 2}, {0x01A986, 6}, {0x01A98C, 2}, {0x01A98E, 12}, {0x01A99C, 3}, {0x01A98E, 4}, {0x01A996, 4}, {0x01A99C, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A99F, 7}, {0x01A9AA, 11}, {0, 0}, {0, 0}},
    {{0x01A976, 11}, {0x01A981, 3}, {0x01A984, 2}, {0x01A986, 6}, {0x01A98C, 2}, {0x01A98E, 12}, {0x01A99C, 3}, {0x01A98E, 4}, {0x01A996, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A99F, 7}, {0x01A9AA, 11}, {0, 0}, {0, 0}},
    {{0x01A976, 11}, {0x01A981, 3}, {0x01A984, 2}, {0x01A986, 6}, {0x01A98C, 2}, {0x01A992, 4}, {0, 0}, {0x01A99A, 2}, {0, 0}, {0, 0}, {0x01A98E, 4}, {0x01A996, 4}, {0x01A99C, 3}, {0x01A98E, 4}, {0x01A996, 4}, {0x01A99C, 10}, {0x01A9AA, 11}},
};

/* The vertices of an entry copied once more below it and the top and
   bottom rows found: R5 walks the n vertices from R1 (STRIDE bytes each),
   R4 writes x and y STRIDE * n bytes lower; R2 = the smallest y (from
   $03FF) and R6 where its copy went, R8 the largest (from 0). A LOOP of
   R12 = n (R13 = its start). */
static void flat_copy(int copy)
{
    gsu_codes(flat_copy_code[copy], 1);
    int plus;

    R[13] = 0xA981;
    do {
        gsu_codes(flat_copy_code[copy] + 1, 1);
        R[0] = gsu_ldb(R[5]);
        gsu_codes(flat_copy_code[copy] + 2, 1);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_ldb(gsu_codes_then(flat_copy_code[copy] + 3, 1, R[5]));
        gsu_codes(flat_copy_code[copy] + 4, 1);
        gsu_stb(R[4], (uint8_t)R[0]);
        gsu_sub(R[0], R[2], 0);                 /* CMP R2 */
        plus = !gsu.s;
        R[5] = gsu_inc(R[5]);                   /* BPL's delay slot */
        if (!plus) {
            gsu_codes(flat_copy_code[copy] + 5, 2);
            R[2] = R[0];
            R[6] = R[4];
        } else { gsu_codes(flat_copy_code[copy] + 7, 3); }
        R[0] = gsu_sub(R[0], R[8], 0);
        plus = !gsu.s;
        R[5] = gsu_inc(R[5]);                   /* BMI's delay slot */
        if (plus)
            R[8] = gsu_add(R[0], R[8], 0);
        R[4] = gsu_inc(R[4]);
        plus = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);                   /* LOOP's delay slot */
    } while (gsu_codes_loop(flat_copy_code[copy] + 10, 3, plus));
    gsu_codes(flat_copy_code[copy] + 13, 4);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode flat_thin_code[11][28] = {
    {{0x01A99A, 2}, {0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 18}, {0x01AA9E, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 12}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA8C, 2}, {0x01AA9E, 1}, {0x01AA9A, 3}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0, 0}, {0x01AA9D, 1}, {0, 0}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 12}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA8C, 2}, {0x01AA9E, 1}, {0x01AA9A, 3}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9D, 1}, {0, 0}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 15}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9D, 1}, {0x01AA9F, 3}},
    {{0x01A99A, 2}, {0x01A9A6, 4}, {0x01AA74, 11}, {0x01AA7F, 3}, {0x01AA82, 10}, {0x01AA8E, 3}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 15}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9C, 2}, {0x01AA9E, 4}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 15}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9D, 1}, {0x01AA9E, 4}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 12}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA8C, 2}, {0x01AA9E, 1}, {0x01AA9A, 3}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9D, 1}, {0, 0}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 12}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA8C, 2}, {0x01AA9E, 1}, {0x01AA9A, 3}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9D, 1}, {0, 0}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 12}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA8C, 2}, {0x01AA9E, 1}, {0x01AA9A, 3}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9D, 1}, {0, 0}},
    {{0x01A9A6, 4}, {0x01AA74, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 12}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA8C, 2}, {0x01AA9E, 1}, {0x01AA9A, 3}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0x01AA9C, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9D, 1}, {0, 0}},
    {{0x01A9A6, 4}, {0x01AA74, 11}, {0, 0}, {0x01AA7F, 3}, {0x01AA82, 10}, {0x01AA8E, 3}, {0x01AA82, 4}, {0x01AA88, 4}, {0x01AA8E, 14}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AA9C, 2}, {0x01AA9E, 4}},
};

/* AA74: a polygon less than two rows high, drawn as one row at the top
   y (R2) from the smallest x to the largest (not including it). R4 is
   at the vertices again after the copy. Back to the list after. */
static void flat_thin(int copy)
{
    gsu_codes(flat_thin_code[copy], 3);
    int go;

    R[0] = gsu_lsr(R[3]);
    R[12] = gsu_lsr(R[0]);
    R[1] = 0x03FF;
    R[8] = 0;
    R[13] = 0xAA7F;
    do {
        gsu_codes(flat_thin_code[copy] + 3, 1);
        R[0] = gsu_ldb(R[4]);
        gsu_sub(R[0], R[1], 0);                 /* CMP R1 */
        go = !gsu.s;
        R[4] = gsu_inc(R[4]);
        if (!go)
            R[1] = R[0];
        R[0] = gsu_sub(R[0], R[8], 0);
        go = !gsu.s;
        R[4] = gsu_inc(R[4]);
        if (go)
            R[8] = gsu_add(R[0], R[8], 0);
        R[4] = gsu_inc(R[4]);
        go = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);
    } while (gsu_codes_loop(flat_thin_code[copy] + 4, 2, go));
    R[12] = gsu_sub(R[8], R[1], 0);
    if (gsu.s || (gsu_codes(flat_thin_code[copy] + 6, 8), gsu.z))
        { gsu_codes(flat_thin_code[copy] + 14, 2); return; } else { gsu_codes(flat_thin_code[copy] + 16, 10); }
    R[13] = 0xAA9C;
    do {
        gsu_codes(flat_thin_code[copy] + 26, 1);
        go = gsu_count_down(&R[12]);
        gsu_plot_r1();
    } while (go);
    gsu_codes(flat_thin_code[copy] + 27, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode flat_left_edge_code[5][8] = {
    {{0x01A9D0, 4}, {0x01A9D4, 4}, {0x01A9D8, 5}, {0x01A9E1, 3}, {0x01A9E4, 7}, {0x01A9CE, 2}, {0x01A9EB, 3}, {0x01A9EE, 4}},
    {{0x01A9CF, 5}, {0x01A9D4, 4}, {0x01A9D8, 5}, {0x01A9E1, 3}, {0x01A9E4, 7}, {0x01A9CE, 1}, {0x01A9EB, 3}, {0x01A9EE, 4}},
    {{0, 0}, {0x01A9D4, 4}, {0x01A9D8, 5}, {0x01A9E1, 10}, {0, 0}, {0, 0}, {0x01A9EB, 3}, {0x01A9EE, 4}},
    {{0, 0}, {0x01A9D4, 4}, {0x01A9D8, 5}, {0x01A9E1, 10}, {0, 0}, {0, 0}, {0x01A9EB, 3}, {0, 0}},
    {{0, 0}, {0x01A9D4, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
};

/* A9D0 (from A9CE): the next left edge. R8 = the current x, R7 = the
   next vertex's x, its y less the row R2 in R11; edges with no height are
   passed. Returns 0 when the polygon has ended (the next vertex is above
   the row), else 1 with R7 = x in 8.8 and R8 its slope. */
static int flat_left_edge(int from_a9ce, int copy)
{
    for (;;) {
        gsu_codes(flat_left_edge_code[copy], 1);
        if (from_a9ce)
            R[8] = R[7];
        from_a9ce = 1;
        R[7] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[0] = gsu_ldb(gsu_codes_then(flat_left_edge_code[copy] + 1, 1, R[6]));
        R[6] = gsu_inc(R[6]);
        R[11] = gsu_sub(R[0], R[2], 0);
        if (gsu.s)
            return 0; else { gsu_codes(flat_left_edge_code[copy] + 2, 2); }
        {
            int zero = gsu.z;

            R[6] = gsu_inc(R[6]);               /* BEQ's delay slot */
            if (!zero)
                { gsu_codes(flat_left_edge_code[copy] + 4, 1); break; } else { gsu_codes(flat_left_edge_code[copy] + 5, 1); }
        }
    }
    reciprocal_at(R[11], GSU_COPY(0));
    gsu_codes(flat_left_edge_code[copy] + 6, 1);
    gsu_stw(FX_LEFT_AT, R[6]);
    R[0] = gsu_sub(R[7], R[8], 0);
    gsu_codes(flat_left_edge_code[copy] + 7, 1);
    reciprocal_read(GSU_COPY(0));
    R[7] = gsu_swap(R[8]);
    R[6] = gsu_getbh(R[6]);
    R[8] = slope();
    return 1;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode flat_right_edge_code[5][12] = {
    {{0x01AA08, 5}, {0, 0}, {0x01AA0D, 5}, {0x01AA12, 5}, {0, 0}, {0, 0}, {0x01AA17, 5}, {0x01AA20, 3}, {0x01AA23, 7}, {0x01AA2A, 4}, {0x01AA2F, 6}, {0x01AA35, 10}},
    {{0x01AA08, 5}, {0, 0}, {0x01AA0D, 5}, {0x01AA12, 5}, {0, 0}, {0, 0}, {0x01AA17, 5}, {0x01AA20, 3}, {0x01AA23, 7}, {0x01AA2A, 4}, {0x01AA2F, 6}, {0x01AA35, 10}},
    {{0x01AA08, 10}, {0, 0}, {0, 0}, {0x01AA12, 5}, {0, 0}, {0, 0}, {0x01AA17, 5}, {0x01AA20, 10}, {0, 0}, {0x01AA2A, 4}, {0x01AA2F, 6}, {0x01AA35, 10}},
    {{0x01A9EE, 21}, {0x01AA08, 10}, {0, 0}, {0x01AA12, 5}, {0, 0}, {0, 0}, {0x01AA17, 5}, {0x01AA20, 10}, {0, 0}, {0x01AA2A, 4}, {0x01AA2F, 6}, {0x01AA35, 10}},
    {{0x01AA08, 5}, {0, 0}, {0x01AA0D, 5}, {0x01AA12, 5}, {0x01AA17, 9}, {0x01AA70, 4}, {0x01AA17, 5}, {0x01AA20, 3}, {0x01AA23, 7}, {0x01AA2A, 4}, {0x01AA2F, 6}, {0x01AA35, 10}},
};

/* AA0D: the next right edge, walking back from R5: R10 = the current x,
   R9 = the next x, its y less R2 in R3. 0 when the polygon has ended. */
static int flat_right_edge(int copy)
{
    gsu_codes(flat_right_edge_code[copy], 2);
    for (;;) {
        gsu_codes(flat_right_edge_code[copy] + 2, 1);
        R[10] = R[9];
        R[0] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        R[5] = gsu_dec(R[5]);
        R[9] = gsu_ldb(gsu_codes_then(flat_right_edge_code[copy] + 3, 1, R[5]));
        R[5] = gsu_dec(R[5]);
        R[3] = gsu_sub(R[0], R[2], 0);
        if (gsu.s)
            { gsu_codes(flat_right_edge_code[copy] + 4, 2); return 0; } else { gsu_codes(flat_right_edge_code[copy] + 6, 2); }
        {
            int zero = gsu.z;

            R[5] = gsu_dec(R[5]);               /* BEQ's delay slot */
            if (!zero)
                { gsu_codes(flat_right_edge_code[copy] + 8, 1); break; }
        }
    }
    reciprocal_at(R[3], GSU_COPY(1));
    R[0] = gsu_sub(R[9], R[10], 0);
    gsu_codes(flat_right_edge_code[copy] + 9, 1);
    reciprocal_read(GSU_COPY(1));
    R[9] = gsu_swap(R[10]);
    R[6] = gsu_getbh(gsu_codes_then(flat_right_edge_code[copy] + 10, 1, R[6]));
    R[10] = (gsu_codes(flat_right_edge_code[copy] + 11, 1), slope());
    return 1;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t flat_thin_from_flat_polygon[15] = {0, 1, 2, 3, 0, 0, 0, 0, 4, 5, 6, 7, 8, 9, 10};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t flat_right_edge_from_flat_polygon[15] = {0, 1, 1, 1, 1, 2, 3, 3, 1, 1, 4, 1, 1, 1, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t flat_left_edge_from_flat_polygon[15] = {0, 1, 1, 1, 1, 2, 4, 4, 1, 1, 1, 1, 1, 1, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t flat_copy_from_flat_polygon[15] = {0, 0, 1, 0, 2, 2, 3, 4, 1, 0, 0, 1, 0, 0, 1};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t edge_step_from_flat_polygon[15] = {0, 1, 1, 1, 4, 4, 6, 6, 6, 1, 1, 1, 1, 1, 0};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode flat_polygon_code[15][26] = {
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0x01A9CF, 5}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0x01AA9F, 3}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0x01AA9F, 3}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0, 0}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0, 0}, {0x01A9B5, 12}, {0x01A9D0, 4}, {0x01A9F2, 17}, {0x01AA03, 5}, {0x01A9F2, 17}, {0x01AA3F, 10}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0x01A9CF, 5}, {0x01A9D8, 9}, {0x01AA70, 4}, {0, 0}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0, 0}, {0x01A9B5, 12}, {0x01A9D0, 4}, {0, 0}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0x01A9CF, 5}, {0x01A9D8, 9}, {0x01AA70, 4}, {0, 0}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0, 0}, {0x01A9B5, 12}, {0x01A9D0, 4}, {0, 0}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0x01A9CF, 5}, {0x01A9D8, 9}, {0x01AA70, 4}, {0, 0}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0x01AA9F, 3}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0x01AA9F, 3}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0x01AA9F, 3}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0x01AA9F, 3}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
    {{0x01A956, 7}, {0xA95D, 0}, {0x01A95D, 18}, {0x01A96F, 7}, {0, 0}, {0x01A9AA, 11}, {0x01A9B5, 12}, {0, 0}, {0x01A9F2, 17}, {0x01AA03, 5}, {0, 0}, {0x01AA3F, 10}, {0, 0}, {0, 0}, {0, 0}, {0x01AA4B, 1}, {0x01AA4C, 4}, {0x01AA59, 5}, {0x01AA67, 5}, {0x01A9C1, 5}, {0x01AA6C, 4}, {0x01A9C6, 9}, {0, 0}, {0x01A9D8, 9}, {0x01AA70, 4}, {0x01A9F2, 17}},
};

/* A95C: a flat polygon (R0 = its type less 2, R1 at its colour byte):
   the vertices copied, then rows from the top down, each from the left
   edge's x to the right's, both rounded (+ $7F) when an edge starts. */
static void flat_polygon(int copy)
{
    gsu_codes(flat_polygon_code[copy], 3);
    int go;

    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    R[12] = R[0];
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[3] = R[0];
    R[0] = 0x0070;
    gsu_color((uint8_t)R[0]);
    R[0] = 0;
    gsu_cmode(R[0]);
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_add(R[1], R[3], 0);
    gsu_codes(flat_polygon_code[copy] + 3, 1);
    gsu_stw(FX_LIST_END, R[0]);
    R[5] = R[1];
    R[4] = gsu_sub(R[1], R[3], 0);
    R[2] = 0x03FF;
    R[8] = 0;
    flat_copy(GSU_COPY(flat_copy_from_flat_polygon[copy]));
    R[0] = gsu_sub(R[8], R[2], 0);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_dec(R[0]);
    if (gsu.s) {
        flat_thin(GSU_COPY(flat_thin_from_flat_polygon[copy]));
        gsu_codes(flat_polygon_code[copy] + 4, 1);
        return;
    } else { gsu_codes(flat_polygon_code[copy] + 5, 1); }
    R[6] = gsu_dec(R[6]);
    R[6] = gsu_dec(R[6]);
    R[5] = gsu_add(R[6], R[3], 0);
    R[5] = gsu_dec(R[5]);
    R[5] = gsu_dec(R[5]);
    R[8] = gsu_ldb(R[6]);
    R[6] = gsu_add(R[6], 4, 0);
    R[9] = gsu_swap(R[8]);
    R[13] = 0xAA49;
    R[3] = gsu_sub(R[3], R[3], 0);              /* BRA's delay slot */
    gsu_codes(flat_polygon_code[copy] + 6, 2);
    if (!flat_left_edge(0, GSU_COPY(flat_left_edge_from_flat_polygon[copy])))
        return; else { gsu_codes(flat_polygon_code[copy] + 8, 1); }
    for (;;) {
        gsu_codes(flat_polygon_code[copy] + 9, 1);
        /* AA03 */
        gsu_moves(R[3]);
        if (gsu.z) {
            R[0] = gsu_add(0x007F, R[9], 0);
            R[9] = gsu_hib(R[0]);
            if (!flat_right_edge(GSU_COPY(flat_right_edge_from_flat_polygon[copy])))
                return;
        } else { gsu_codes(flat_polygon_code[copy] + 10, 1); }
        /* AA3F: the rows while both edges last */
        gsu_code(0x01A9C1, 5); gsu_code(0x01AA6C, 4);
        do {
            gsu_codes(flat_polygon_code[copy] + 11, 1);
            R[1] = gsu_hib(R[7]);
            R[0] = gsu_hib(R[9]);
            R[12] = gsu_sub(R[0], R[1], 0);
            go = !gsu.s;
            R[12] = gsu_inc(R[12]);             /* BMI's delay slot */
            if (go) {
                gsu_code(0x01aa49, 1);
                do {
                    gsu_code(0x01aa4a, 1);
                    go = gsu_count_down(&R[12]);
                    /* the PLOT in LOOP's delay slot runs after the fetch
                       from where LOOP goes */
                    gsu_code(go ? 0x01aa49 : 0x01aa4b, 1);
                    gsu_plot_r1();
                } while (go);
                gsu_codes(flat_polygon_code[copy] + 12, 3);
            } else { gsu_codes(flat_polygon_code[copy] + 15, 1); }
            gsu_codes(flat_polygon_code[copy] + 16, 3);
            edge_step(7, R[8], 11, GSU_COPY(edge_step_from_flat_polygon[copy]));
            edge_step(9, R[10], 3, GSU_COPY(edge_step_from_flat_polygon[copy]));
            R[0] = gsu_and(R[11], R[3]);
            go = !gsu.z;
            R[2] = gsu_inc(R[2]);               /* BNE's delay slot */
        } while (go);
        /* A9C1 */
        gsu_codes(flat_polygon_code[copy] + 19, 2);
        gsu_moves(R[11]);
        if (!gsu.z)
            continue; else { gsu_codes(flat_polygon_code[copy] + 21, 1); }
        R[0] = gsu_add(0x007F, R[7], 0);
        R[7] = gsu_hib(R[0]);
        R[6] = gsu_ldw(FX_LEFT_AT);
        gsu_codes(flat_polygon_code[copy] + 22, 1);
        if (!flat_left_edge(1, GSU_COPY(flat_left_edge_from_flat_polygon[copy])))
            { gsu_codes(flat_polygon_code[copy] + 23, 2); return; } else { gsu_codes(flat_polygon_code[copy] + 25, 1); }
    }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode line_code[11][46] = {
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAD9, 5}, {0x01AAF9, 9}, {0x01AB08, 1}, {0x01AAE3, 7}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAEA, 3}, {0, 0}, {0x01AAEE, 3}, {0, 0}, {0, 0}, {0x01AAED, 1}, {0x01AAF1, 4}, {0x01AAF1, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 3}, {0, 0}, {0, 0}, {0x01AB05, 1}, {0x01AB09, 4}, {0x01AB09, 8}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AACD, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 17}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAE3, 7}, {0, 0}, {0, 0}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0x01AAEE, 2}, {0, 0}, {0, 0}, {0x01AAF1, 4}, {0, 0}, {0x01AAF1, 8}, {0x01AAF9, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 2}, {0, 0}, {0, 0}, {0x01AB08, 5}, {0, 0}, {0x01AB09, 8}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AACD, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 12}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0x01AAEE, 2}, {0, 0}, {0, 0}, {0x01AAF1, 4}, {0, 0}, {0x01AAF1, 8}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0x01AB08, 1}, {0x01AB06, 2}, {0x01AB09, 4}, {0x01AB09, 4}, {0, 0}, {0, 0}, {0x01AB0D, 4}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AAC7, 18}, {0x01AADE, 5}, {0x01AAF9, 9}, {0x01AB05, 1}, {0, 0}, {0, 0}, {0, 0}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 7}, {0, 0}, {0x01AB09, 4}, {0x01AB08, 1}, {0, 0}, {0x01AB0D, 4}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AAC7, 18}, {0x01AADE, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAE3, 7}, {0, 0}, {0, 0}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0, 0}, {0, 0}, {0x01AAF1, 4}, {0, 0}, {0, 0}, {0x01AAF5, 4}, {0x01AAF9, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0x01AB08, 1}, {0x01AB06, 7}, {0, 0}, {0x01AB09, 4}, {0, 0}, {0, 0}, {0x01AB0D, 4}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AACD, 5}, {0, 0}, {0, 0}, {0, 0}, {0x01AB05, 1}, {0, 0}, {0, 0}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 12}, {0x01AAEA, 3}, {0, 0}, {0x01AAEE, 2}, {0, 0}, {0, 0}, {0x01AAF0, 5}, {0, 0}, {0x01AAF1, 8}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAF9, 9}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 3}, {0, 0}, {0, 0}, {0x01AB05, 1}, {0x01AB08, 5}, {0x01AB09, 8}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AAC7, 18}, {0x01AADE, 5}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAE3, 7}, {0, 0}, {0, 0}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0x01AAEE, 2}, {0, 0}, {0, 0}, {0x01AAF1, 4}, {0, 0}, {0x01AAF1, 8}, {0x01AAF9, 9}, {0x01AB08, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 7}, {0x01AB08, 1}, {0x01AB09, 4}, {0, 0}, {0, 0}, {0x01AB0D, 4}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AACD, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 12}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0x01AAEE, 2}, {0, 0}, {0, 0}, {0x01AAF1, 4}, {0, 0}, {0x01AAF1, 8}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAF9, 9}, {0x01AB05, 1}, {0x01AB05, 1}, {0x01AB05, 1}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 3}, {0, 0}, {0, 0}, {0x01AB09, 4}, {0, 0}, {0x01AB09, 8}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AACD, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 12}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0x01AAEE, 2}, {0, 0}, {0, 0}, {0x01AAF1, 4}, {0, 0}, {0x01AAF1, 8}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAF9, 9}, {0x01AB05, 1}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 3}, {0, 0}, {0, 0}, {0x01AB08, 5}, {0, 0}, {0x01AB09, 8}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AAC7, 18}, {0x01AADE, 5}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAE3, 7}, {0, 0}, {0, 0}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0x01AAEE, 2}, {0, 0}, {0, 0}, {0x01AAF1, 4}, {0, 0}, {0x01AAF1, 8}, {0x01AAF9, 9}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0x01AB08, 1}, {0x01AB06, 3}, {0, 0}, {0, 0}, {0x01AB09, 4}, {0, 0}, {0x01AB09, 8}},
    {{0x01A911, 4}, {0x01AAA2, 12}, {0x01AAAE, 5}, {0x01AAB3, 5}, {0x01AAB8, 5}, {0x01AABD, 5}, {0x01AAC2, 5}, {0x01AAC7, 18}, {0x01AADE, 5}, {0, 0}, {0, 0}, {0x01AAC7, 6}, {0x01AAD2, 7}, {0x01AADE, 5}, {0x01AAD9, 5}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AAE3, 7}, {0, 0}, {0, 0}, {0x01AAEA, 3}, {0x01AAF0, 1}, {0x01AAEE, 2}, {0x01AAF1, 4}, {0x01AAF1, 4}, {0, 0}, {0, 0}, {0x01AAF5, 4}, {0x01AAF9, 9}, {0x01AB05, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01AB02, 3}, {0, 0}, {0x01AB06, 7}, {0, 0}, {0x01AB09, 4}, {0x01AB08, 1}, {0, 0}, {0x01AB0D, 4}},
};

/* AAA2: a line (R1 at its colour byte), stepped along its longer axis;
   R0 is the error term, the carry from its SUB picks the step across. */
static void line(int copy)
{
    gsu_codes(line_code[copy], 2);
    int carry, go;

    R[0] = 0x0070;
    gsu_color((uint8_t)R[0]);
    R[0] = 0;
    gsu_cmode(R[0]);
    R[7] = R[1];
    R[0] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_color((uint8_t)R[0]);
    R[1] = gsu_ldb(gsu_codes_then(line_code[copy] + 2, 1, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[2] = gsu_ldb(gsu_codes_then(line_code[copy] + 3, 1, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[3] = gsu_ldb(gsu_codes_then(line_code[copy] + 4, 1, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[4] = gsu_ldb(gsu_codes_then(line_code[copy] + 5, 1, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_codes(line_code[copy] + 6, 1);
    gsu_stw(FX_LIST_END, R[7]);
    R[5] = 0x0001;
    R[3] = gsu_sub(R[3], R[1], 0);
    if (gsu.s) {
        gsu_codes(line_code[copy] + 7, 4);
        R[5] = 0xFFFF;
        R[3] = gsu_not(R[3]);
        R[3] = gsu_inc(R[3]);
    } else { gsu_codes(line_code[copy] + 11, 3); }
    R[6] = 0x0001;
    R[4] = gsu_sub(R[4], R[2], 0);
    if (gsu.s) {
        gsu_codes(line_code[copy] + 14, 3);
        R[6] = 0xFFFF;
        R[4] = gsu_not(R[4]);
        R[4] = gsu_inc(R[4]);
    } else { gsu_codes(line_code[copy] + 17, 3); }
    R[0] = gsu_sub(R[3], R[4], 0);
    if (!gsu.s) {
        gsu_codes(line_code[copy] + 20, 3);
        R[12] = R[3];
        R[12] = gsu_inc(R[12]);
        R[0] = gsu_lsr(R[0]);
        R[0] = gsu_sub(R[0], R[4], 0);
        R[13] = 0xAAEA;
        do {
            gsu_codes(line_code[copy] + 23, 2);
            carry = gsu.cy;
            gsu_plot_r1();                      /* BCS's delay slot */
            if (!carry) {
                gsu_codes(line_code[copy] + 25, 2);
                R[0] = gsu_add(R[0], R[3], 0);
                R[2] = gsu_add(R[2], R[6], 0);
            } else { gsu_codes(line_code[copy] + 27, 1); }
            R[1] = gsu_dec(R[1]);
            R[1] = gsu_add(R[1], R[5], 0);
            go = gsu_count_down(&R[12]);
            R[0] = gsu_sub(R[0], R[4], 0);      /* LOOP's delay slot */
        } while (gsu_codes_loop(line_code[copy] + 28, 2, go));
        gsu_codes(line_code[copy] + 30, 1);
        return;
    } else { gsu_codes(line_code[copy] + 31, 7); }
    R[12] = R[4];
    R[12] = gsu_inc(R[12]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_lsr(R[0]);
    R[0] = gsu_sub(R[0], R[3], 0);
    R[13] = 0xAB02;
    do {
        gsu_codes(line_code[copy] + 38, 2);
        carry = gsu.cy;
        gsu_plot_r1();
        if (!carry) {
            gsu_codes(line_code[copy] + 40, 2);
            R[0] = gsu_add(R[0], R[4], 0);
            R[1] = gsu_add(R[1], R[5], 0);
        } else { gsu_codes(line_code[copy] + 42, 1); }
        R[1] = gsu_dec(R[1]);
        R[2] = gsu_add(R[2], R[6], 0);
        go = gsu_count_down(&R[12]);
        R[0] = gsu_sub(R[0], R[3], 0);
    } while (gsu_codes_loop(line_code[copy] + 43, 2, go));
    gsu_codes(line_code[copy] + 45, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode tex_left_edge_code[3][34] = {
    {{0x01A7E2, 9}, {0, 0}, {0x01A7EB, 2}, {0x01A7ED, 3}, {0x01A7F0, 2}, {0x01A7F2, 3}, {0x01A7F5, 5}, {0x01A7FA, 3}, {0x01A7FD, 3}, {0x01A800, 3}, {0x01A803, 3}, {0x01A806, 5}, {0x01A80B, 4}, {0x01A80F, 3}, {0x01A812, 4}, {0x01A816, 3}, {0x01A819, 10}, {0x01A7F4, 1}, {0x01A819, 7}, {0x01A823, 4}, {0x01A909, 4}, {0x01A827, 4}, {0x01A82B, 31}, {0x01A84A, 3}, {0x01A84D, 8}, {0x01A855, 3}, {0x01A858, 4}, {0x01A85C, 3}, {0x01A85F, 3}, {0x01A862, 8}, {0x01A86A, 3}, {0x01A86D, 4}, {0, 0}, {0, 0}},
    {{0x01A7DA, 4}, {0x01A7E2, 9}, {0x01A7EB, 2}, {0x01A7ED, 3}, {0x01A7F0, 2}, {0x01A7F2, 3}, {0x01A7F5, 5}, {0x01A7FA, 3}, {0x01A7FD, 3}, {0x01A800, 3}, {0x01A803, 3}, {0x01A806, 5}, {0x01A80B, 4}, {0x01A80F, 3}, {0x01A812, 4}, {0x01A816, 3}, {0x01A819, 10}, {0x01A7F4, 1}, {0x01A819, 7}, {0x01A823, 4}, {0x01A909, 4}, {0x01A827, 4}, {0x01A82B, 31}, {0x01A84A, 3}, {0x01A84D, 8}, {0x01A855, 3}, {0x01A858, 4}, {0x01A85C, 3}, {0x01A85F, 3}, {0x01A862, 8}, {0x01A86A, 3}, {0x01A86D, 4}, {0x01A871, 4}, {0x01A879, 9}},
    {{0x01A7E2, 9}, {0, 0}, {0x01A7EB, 2}, {0x01A7ED, 3}, {0x01A7F0, 2}, {0x01A7F2, 3}, {0x01A7F5, 5}, {0x01A7FA, 3}, {0x01A7FD, 3}, {0x01A800, 3}, {0x01A803, 3}, {0x01A806, 5}, {0x01A80B, 4}, {0x01A80F, 3}, {0x01A812, 4}, {0x01A816, 3}, {0x01A819, 10}, {0x01A7F4, 1}, {0x01A819, 7}, {0x01A823, 4}, {0x01A909, 4}, {0x01A827, 4}, {0x01A82B, 31}, {0x01A84A, 3}, {0x01A84D, 8}, {0x01A855, 3}, {0x01A858, 4}, {0x01A85C, 3}, {0x01A85F, 3}, {0x01A862, 8}, {0x01A86A, 3}, {0x01A86D, 4}, {0, 0}, {0, 0}},
};

/* A7D9 / A7E2: the next left edge of a textured polygon from the walk at
   $0060, with u and v ($0070, $007C) kept as bytes before it and their
   steps per row. 0 when the polygon has ended. */
static int tex_left_edge(int copy)
{
    gsu_codes(tex_left_edge_code[copy], 2);
    R[0] = gsu_add(0x007F, R[9], 0);
    R[9] = gsu_hib(R[0]);
    R[0] = gsu_ldw(FX_LEFT_U);
    R[0] = gsu_hib(R[0]);
    gsu_codes(tex_left_edge_code[copy] + 2, 1);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 3, 1, FX_LEFT_V));
    R[0] = gsu_hib(R[0]);
    gsu_codes(tex_left_edge_code[copy] + 4, 1);
    gsu_sbk(R[0]);
    R[6] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 5, 1, FX_LEFT_AT));
    gsu_code(0x01A7F5, 5);
    for (;;) {
        gsu_codes(tex_left_edge_code[copy] + 6, 1);
        R[12] = R[9];
        R[0] = gsu_ldw(FX_LEFT_U);
        gsu_codes(tex_left_edge_code[copy] + 7, 1);
        gsu_stw(FX_LEFT_U_PREV, R[0]);
        R[0] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 8, 1, FX_LEFT_V));
        gsu_codes(tex_left_edge_code[copy] + 9, 1);
        gsu_stw(FX_LEFT_V_PREV, R[0]);
        R[9] = gsu_ldb(gsu_codes_then(tex_left_edge_code[copy] + 10, 1, R[6]));
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[4] = gsu_ldb(gsu_codes_then(tex_left_edge_code[copy] + 11, 1, R[6]));
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[0] = gsu_ldb(gsu_codes_then(tex_left_edge_code[copy] + 12, 1, R[6]));
        gsu_codes(tex_left_edge_code[copy] + 13, 1);
        gsu_stw(FX_LEFT_U, R[0]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[0] = gsu_ldb(gsu_codes_then(tex_left_edge_code[copy] + 14, 1, R[6]));
        gsu_codes(tex_left_edge_code[copy] + 15, 1);
        gsu_stw(FX_LEFT_V, R[0]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[11] = gsu_sub(R[4], R[2], 0);
        if (!gsu.z)
            { gsu_codes(tex_left_edge_code[copy] + 16, 1); break; } else { gsu_codes(tex_left_edge_code[copy] + 17, 2); }
    }
    if (gsu.s)
        { gsu_codes(tex_left_edge_code[copy] + 19, 2); return 0; } else { gsu_codes(tex_left_edge_code[copy] + 21, 1); }
    gsu_stw(FX_LEFT_AT, R[6]);
    gsu_codes(tex_left_edge_code[copy] + 22, 1);
    reciprocal_at(R[11], GSU_COPY(2));
    reciprocal_read(GSU_COPY(0));
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_sub(R[9], R[12], 0);
    R[9] = gsu_swap(R[12]);
    R[7] = slope();
    R[0] = gsu_ldw(FX_LEFT_U);
    R[1] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 23, 1, FX_LEFT_U_PREV));
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_LEFT_DU, (gsu_codes(tex_left_edge_code[copy] + 24, 1), uv_step()));
    R[0] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 25, 1, FX_LEFT_U_PREV));
    R[0] = gsu_swap(R[0]);
    gsu_codes(tex_left_edge_code[copy] + 26, 1);
    gsu_stw(FX_LEFT_U, R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 27, 1, FX_LEFT_V));
    R[1] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 28, 1, FX_LEFT_V_PREV));
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_LEFT_DV, (gsu_codes(tex_left_edge_code[copy] + 29, 1), uv_step()));
    R[0] = gsu_ldw(gsu_codes_then(tex_left_edge_code[copy] + 30, 1, FX_LEFT_V_PREV));
    R[0] = gsu_swap(R[0]);
    gsu_codes(tex_left_edge_code[copy] + 31, 1);
    gsu_stw(FX_LEFT_V, R[0]);
    gsu_codes(tex_left_edge_code[copy] + 32, 2);
    return 1;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode tex_right_edge_code[6][35] = {
    {{0x01A879, 9}, {0x01A882, 2}, {0x01A884, 3}, {0x01A887, 2}, {0x01A889, 3}, {0, 0}, {0x01A88C, 5}, {0x01A891, 3}, {0x01A894, 3}, {0x01A897, 3}, {0x01A89A, 4}, {0x01A89E, 3}, {0x01A8A1, 4}, {0x01A8A5, 3}, {0x01A8A8, 4}, {0x01A8AC, 5}, {0x01A8B1, 7}, {0, 0}, {0x01A88B, 1}, {0x01A8B1, 4}, {0x01A8B8, 4}, {0x01A909, 4}, {0x01A8BC, 4}, {0x01A8C0, 31}, {0x01A8DF, 3}, {0x01A8E2, 8}, {0x01A8EA, 3}, {0x01A8ED, 4}, {0x01A8F1, 3}, {0x01A8F4, 3}, {0x01A8F7, 8}, {0x01A8FF, 3}, {0x01A902, 4}, {0x01A906, 3}, {0, 0}},
    {{0x01A879, 9}, {0x01A882, 2}, {0x01A884, 3}, {0x01A887, 2}, {0x01A889, 3}, {0, 0}, {0x01A88C, 5}, {0x01A891, 3}, {0x01A894, 3}, {0x01A897, 3}, {0x01A89A, 4}, {0x01A89E, 3}, {0x01A8A1, 4}, {0x01A8A5, 3}, {0x01A8A8, 4}, {0x01A8AC, 5}, {0x01A8B1, 7}, {0x01A8BC, 4}, {0x01A88B, 1}, {0x01A8B1, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01A8C0, 31}, {0x01A8DF, 3}, {0x01A8E2, 8}, {0x01A8EA, 3}, {0x01A8ED, 4}, {0x01A8F1, 3}, {0x01A8F4, 3}, {0x01A8F7, 8}, {0x01A8FF, 3}, {0x01A902, 4}, {0x01A906, 3}, {0, 0}},
    {{0, 0}, {0x01A882, 2}, {0x01A884, 3}, {0x01A887, 2}, {0x01A889, 3}, {0x01A88C, 5}, {0, 0}, {0x01A891, 3}, {0x01A894, 3}, {0x01A897, 3}, {0x01A89A, 4}, {0x01A89E, 3}, {0x01A8A1, 4}, {0x01A8A5, 3}, {0x01A8A8, 4}, {0x01A8AC, 5}, {0x01A8B1, 7}, {0x01A8BC, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A8C0, 31}, {0x01A8DF, 3}, {0x01A8E2, 8}, {0x01A8EA, 3}, {0x01A8ED, 4}, {0x01A8F1, 3}, {0x01A8F4, 3}, {0x01A8F7, 8}, {0x01A8FF, 3}, {0x01A902, 4}, {0x01A710, 4}, {0x01A906, 3}},
    {{0x01A879, 9}, {0x01A882, 2}, {0x01A884, 3}, {0x01A887, 2}, {0x01A889, 3}, {0x01A88C, 5}, {0, 0}, {0x01A891, 3}, {0x01A894, 3}, {0x01A897, 3}, {0x01A89A, 4}, {0x01A89E, 3}, {0x01A8A1, 4}, {0x01A8A5, 3}, {0x01A8A8, 4}, {0x01A8AC, 5}, {0x01A8B1, 7}, {0x01A8BC, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A8C0, 31}, {0x01A8DF, 3}, {0x01A8E2, 8}, {0x01A8EA, 3}, {0x01A8ED, 4}, {0x01A8F1, 3}, {0x01A8F4, 3}, {0x01A8F7, 8}, {0x01A8FF, 3}, {0x01A902, 4}, {0x01A906, 3}, {0, 0}},
    {{0x01A879, 9}, {0x01A882, 2}, {0x01A884, 3}, {0x01A887, 2}, {0x01A889, 3}, {0x01A88C, 5}, {0, 0}, {0x01A891, 3}, {0x01A894, 3}, {0x01A897, 3}, {0x01A89A, 4}, {0x01A89E, 3}, {0x01A8A1, 4}, {0x01A8A5, 3}, {0x01A8A8, 4}, {0x01A8AC, 5}, {0x01A8B1, 7}, {0x01A8BC, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A8C0, 31}, {0x01A8DF, 3}, {0x01A8E2, 8}, {0x01A8EA, 3}, {0x01A8ED, 4}, {0x01A8F1, 3}, {0x01A8F4, 3}, {0x01A8F7, 8}, {0x01A8FF, 3}, {0x01A902, 4}, {0x01A906, 3}, {0, 0}},
    {{0x01A879, 9}, {0x01A882, 2}, {0x01A884, 3}, {0x01A887, 2}, {0x01A889, 3}, {0, 0}, {0x01A88C, 5}, {0x01A891, 3}, {0x01A894, 3}, {0x01A897, 3}, {0x01A89A, 4}, {0x01A89E, 3}, {0x01A8A1, 4}, {0x01A8A5, 3}, {0x01A8A8, 4}, {0x01A8AC, 5}, {0x01A8B1, 7}, {0x01A8BC, 4}, {0x01A88B, 1}, {0x01A8B1, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01A8C0, 31}, {0x01A8DF, 3}, {0x01A8E2, 8}, {0x01A8EA, 3}, {0x01A8ED, 4}, {0x01A8F1, 3}, {0x01A8F4, 3}, {0x01A8F7, 8}, {0x01A8FF, 3}, {0x01A902, 4}, {0x01A906, 3}, {0, 0}},
};

/* A879: the next right edge, walking back from $0062. */
static int tex_right_edge(int copy)
{
    gsu_codes(tex_right_edge_code[copy], 1);
    R[0] = gsu_add(0x007F, R[10], 0);
    R[10] = gsu_hib(R[0]);
    R[0] = gsu_ldw(FX_RIGHT_U);
    R[0] = gsu_hib(R[0]);
    gsu_codes(tex_right_edge_code[copy] + 1, 1);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 2, 1, FX_RIGHT_V));
    R[0] = gsu_hib(R[0]);
    gsu_codes(tex_right_edge_code[copy] + 3, 1);
    gsu_sbk(R[0]);
    R[6] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 4, 1, FX_RIGHT_AT));
    gsu_codes(tex_right_edge_code[copy] + 5, 1);
    for (;;) {
        gsu_codes(tex_right_edge_code[copy] + 6, 1);
        R[12] = R[10];
        R[0] = gsu_ldw(FX_RIGHT_U);
        gsu_codes(tex_right_edge_code[copy] + 7, 1);
        gsu_stw(FX_RIGHT_U_PREV, R[0]);
        R[0] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 8, 1, FX_RIGHT_V));
        gsu_codes(tex_right_edge_code[copy] + 9, 1);
        gsu_stw(FX_RIGHT_V_PREV, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[0] = gsu_ldb(gsu_codes_then(tex_right_edge_code[copy] + 10, 1, R[6]));
        gsu_codes(tex_right_edge_code[copy] + 11, 1);
        gsu_stw(FX_RIGHT_V, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[0] = gsu_ldb(gsu_codes_then(tex_right_edge_code[copy] + 12, 1, R[6]));
        gsu_codes(tex_right_edge_code[copy] + 13, 1);
        gsu_stw(FX_RIGHT_U, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[0] = gsu_ldb(gsu_codes_then(tex_right_edge_code[copy] + 14, 1, R[6]));
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[10] = gsu_ldb(gsu_codes_then(tex_right_edge_code[copy] + 15, 1, R[6]));
        R[3] = gsu_sub(R[0], R[2], 0);
        if (!gsu.z)
            { gsu_codes(tex_right_edge_code[copy] + 16, 2); break; } else { gsu_codes(tex_right_edge_code[copy] + 18, 2); }
    }
    if (gsu.s)
        { gsu_codes(tex_right_edge_code[copy] + 20, 2); return 0; } else { gsu_codes(tex_right_edge_code[copy] + 22, 1); }
    gsu_stw(FX_RIGHT_AT, R[6]);
    gsu_codes(tex_right_edge_code[copy] + 23, 1);
    reciprocal_at(R[3], GSU_COPY(2));
    reciprocal_read(GSU_COPY(0));
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_sub(R[10], R[12], 0);
    R[10] = gsu_swap(R[12]);
    R[8] = slope();
    R[0] = gsu_ldw(FX_RIGHT_U);
    R[1] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 24, 1, FX_RIGHT_U_PREV));
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_RIGHT_DU, (gsu_codes(tex_right_edge_code[copy] + 25, 1), uv_step()));
    R[0] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 26, 1, FX_RIGHT_U_PREV));
    R[0] = gsu_swap(R[0]);
    gsu_codes(tex_right_edge_code[copy] + 27, 1);
    gsu_stw(FX_RIGHT_U, R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 28, 1, FX_RIGHT_V));
    R[1] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 29, 1, FX_RIGHT_V_PREV));
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_RIGHT_DV, (gsu_codes(tex_right_edge_code[copy] + 30, 1), uv_step()));
    R[0] = gsu_ldw(gsu_codes_then(tex_right_edge_code[copy] + 31, 1, FX_RIGHT_V_PREV));
    R[0] = gsu_swap(R[0]);
    gsu_codes(tex_right_edge_code[copy] + 32, 1);
    gsu_stw(FX_RIGHT_V, R[0]);
    gsu_codes(tex_right_edge_code[copy] + 33, 2);
    return 1;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode tex_across_code[2][5] = {
    {{0x01A752, 3}, {0x01A755, 2}, {0x01A757, 3}, {0x01A75A, 8}, {0x01A762, 4}},
    {{0x01A769, 3}, {0x01A76C, 2}, {0x01A76E, 3}, {0x01A771, 8}, {0x01A779, 4}},
};

/* One edge's u or v for the row: RIGHT += its step (LMS R0, step; LMS
   R4, right; ADD R4; SBK), the step across the row from right less left
   (FMULT by R6 = 1/width, ROL) to dst, then LEFT += its step. */
static uint16_t tex_across(uint16_t step_r, uint16_t right, uint16_t left, int lr, uint16_t step_l, int copy)
{
    uint16_t d;

    R[0] = gsu_ldw(step_r);
    R[4] = gsu_ldw(gsu_codes_then(tex_across_code[copy], 1, right));
    R[0] = gsu_add(R[0], R[4], 0);
    gsu_codes(tex_across_code[copy] + 1, 1);
    gsu_sbk(R[0]);
    R[lr] = gsu_ldw(gsu_codes_then(tex_across_code[copy] + 2, 1, left));
    R[0] = gsu_sub(R[4], R[lr], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    d = gsu_rol(R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_across_code[copy] + 3, 1, step_l));
    R[0] = gsu_add(R[0], R[lr], 0);
    gsu_codes(tex_across_code[copy] + 4, 1);
    gsu_stw(left, R[0]);
    return d;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t edge_step_from_tex_rows[6] = {3, 4, 4, 0, 4, 3};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode tex_rows_code[6][32] = {
    {{0x01A710, 4}, {0x01A714, 3}, {0x01A717, 8}, {0x01A71F, 8}, {0x01A717, 4}, {0x01A729, 6}, {0x01A72F, 8}, {0, 0}, {0x01A736, 10}, {0x01A742, 7}, {0x01A7CF, 4}, {0x01A749, 2}, {0x01A74C, 3}, {0x01A74F, 3}, {0x01A766, 3}, {0x01A77D, 3}, {0x01A780, 5}, {0x01A785, 5}, {0x01A78A, 6}, {0x01A790, 3}, {0x01A793, 2}, {0x01A795, 4}, {0x01A799, 5}, {0x01A79E, 1}, {0x01A7A0, 6}, {0x01A7A6, 3}, {0x01A7A9, 4}, {0x01A7B6, 5}, {0x01A7C4, 11}, {0, 0}, {0x01A7D3, 4}, {0x01A7D7, 3}},
    {{0, 0}, {0x01A714, 3}, {0x01A717, 8}, {0x01A71F, 8}, {0, 0}, {0, 0}, {0, 0}, {0x01A727, 2}, {0x01A736, 10}, {0x01A742, 7}, {0, 0}, {0x01A749, 2}, {0x01A74C, 3}, {0x01A74F, 3}, {0x01A766, 3}, {0x01A77D, 3}, {0x01A780, 5}, {0x01A785, 5}, {0x01A78A, 6}, {0x01A790, 3}, {0x01A793, 2}, {0x01A795, 4}, {0x01A799, 5}, {0x01A79E, 1}, {0x01A7A0, 6}, {0x01A7A6, 3}, {0x01A7A9, 4}, {0x01A7B6, 5}, {0x01A7C4, 11}, {0x01A7CF, 4}, {0x01A7D3, 4}, {0x01A7D7, 3}},
    {{0, 0}, {0x01A714, 3}, {0x01A717, 8}, {0x01A71F, 8}, {0, 0}, {0, 0}, {0, 0}, {0x01A727, 2}, {0x01A736, 10}, {0x01A742, 7}, {0, 0}, {0x01A749, 2}, {0x01A74C, 3}, {0x01A74F, 3}, {0x01A766, 3}, {0x01A77D, 3}, {0x01A780, 5}, {0x01A785, 5}, {0x01A78A, 6}, {0x01A790, 3}, {0x01A793, 2}, {0x01A795, 4}, {0x01A799, 5}, {0x01A79E, 1}, {0x01A79F, 7}, {0x01A7A6, 3}, {0x01A7A9, 4}, {0x01A7B6, 5}, {0x01A7C4, 11}, {0x01A7CF, 4}, {0x01A7D3, 4}, {0x01A7D7, 3}},
    {{0x01A710, 4}, {0x01A714, 3}, {0x01A717, 8}, {0x01A71F, 8}, {0x01A717, 4}, {0x01A729, 6}, {0x01A72F, 8}, {0, 0}, {0x01A736, 10}, {0x01A742, 7}, {0x01A7CF, 4}, {0x01A749, 2}, {0x01A74C, 3}, {0x01A74F, 3}, {0x01A766, 3}, {0x01A77D, 3}, {0x01A780, 5}, {0x01A785, 5}, {0x01A78A, 6}, {0x01A790, 3}, {0x01A793, 2}, {0x01A795, 4}, {0x01A799, 5}, {0x01A79E, 1}, {0x01A7A0, 6}, {0x01A7A6, 3}, {0x01A7A9, 4}, {0x01A7B6, 5}, {0x01A7C4, 11}, {0, 0}, {0x01A7D3, 4}, {0x01A7D7, 3}},
    {{0x01A710, 4}, {0x01A714, 3}, {0x01A717, 8}, {0x01A71F, 8}, {0x01A717, 4}, {0x01A729, 6}, {0x01A72F, 8}, {0, 0}, {0x01A736, 10}, {0x01A742, 7}, {0x01A7CF, 4}, {0x01A749, 2}, {0x01A74C, 3}, {0x01A74F, 3}, {0x01A766, 3}, {0x01A77D, 3}, {0x01A780, 5}, {0x01A785, 5}, {0x01A78A, 6}, {0x01A790, 3}, {0x01A793, 2}, {0x01A795, 4}, {0x01A799, 5}, {0x01A79E, 1}, {0x01A7A0, 6}, {0x01A7A6, 3}, {0x01A7A9, 4}, {0x01A7B6, 5}, {0x01A7C4, 11}, {0, 0}, {0x01A7D3, 4}, {0x01A7D7, 3}},
    {{0x01A710, 4}, {0x01A714, 3}, {0x01A717, 8}, {0x01A71F, 8}, {0x01A717, 4}, {0x01A729, 6}, {0x01A72F, 8}, {0, 0}, {0x01A736, 10}, {0x01A742, 7}, {0x01A7CF, 4}, {0x01A749, 2}, {0x01A74C, 3}, {0x01A74F, 3}, {0x01A766, 3}, {0x01A77D, 3}, {0x01A780, 5}, {0x01A785, 5}, {0x01A78A, 6}, {0x01A790, 3}, {0x01A793, 2}, {0x01A795, 4}, {0x01A799, 5}, {0x01A79E, 1}, {0x01A7A0, 6}, {0x01A7A6, 3}, {0x01A7A9, 4}, {0x01A7B6, 5}, {0x01A7C4, 11}, {0, 0}, {0x01A7D3, 4}, {0x01A7D7, 3}},
};

/* A710-A7D2: the rows of a textured polygon while both edges last (the
   shorter's count in R2's high byte), each row's texels read with MERGE
   of v (R7) and u (R8), the mask and the texture's address, GETC and
   PLOT; the edges' x stepped (as on a flat polygon) and the rows left on
   each kept at $0064/$0066. */
static void tex_rows(int copy)
{
    gsu_codes(tex_rows_code[copy], 1);
    int go;

    gsu_stw(FX_LEFT_SLOPE, R[7]);
    gsu_codes(tex_rows_code[copy] + 1, 1);
    gsu_stw(FX_RIGHT_SLOPE, R[8]);
    R[0] = gsu_sub(R[11], R[3], 0);
    if (!gsu.s) {
        gsu_codes(tex_rows_code[copy] + 2, 1);
        gsu_stw(FX_LEFT_ROWS, R[0]);
        R[0] = gsu_swap(R[3]);
        R[2] = gsu_or(R[0], R[2]);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_codes(tex_rows_code[copy] + 3, 1);
        gsu_stw(FX_RIGHT_ROWS, R[0]);
    } else {
        gsu_codes(tex_rows_code[copy] + 4, 2);
        R[0] = gsu_sub(R[3], R[11], 0);
        gsu_stw(FX_RIGHT_ROWS, R[0]);
        R[0] = gsu_swap(R[11]);
        R[2] = gsu_or(R[0], R[2]);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_codes(tex_rows_code[copy] + 6, 1);
        gsu_stw(FX_LEFT_ROWS, R[0]);
    }
    gsu_codes(tex_rows_code[copy] + 7, 1);
    do {
        gsu_codes(tex_rows_code[copy] + 8, 3);
        /* A736 */
        R[1] = gsu_hib(R[9]);
        R[0] = gsu_hib(R[10]);
        R[12] = gsu_sub(R[0], R[1], 0);
        go = gsu.cy;
        R[12] = gsu_inc(R[12]);                 /* BCS's delay slot */
        if (!go)
            R[12] = 0x0001;
        reciprocal_at(R[12], GSU_COPY(3));
        gsu_codes(tex_rows_code[copy] + 11, 1);
        reciprocal_read(GSU_COPY(2));
        R[6] = gsu_getbh(gsu_codes_then(tex_rows_code[copy] + 12, 1, R[6]));
        R[5] = tex_across(FX_RIGHT_DU, FX_RIGHT_U, FX_LEFT_U, 8, FX_LEFT_DU, gsu_codes_then(tex_rows_code[copy] + 13, 1, GSU_COPY(0)));
        R[6] = tex_across(FX_RIGHT_DV, FX_RIGHT_V, FX_LEFT_V, 7, FX_LEFT_DV, gsu_codes_then(tex_rows_code[copy] + 14, 1, GSU_COPY(1)));
        R[4] = gsu_ldw(gsu_codes_then(tex_rows_code[copy] + 15, 1, FX_TEX_U_OFF));
        R[8] = gsu_add(R[8], R[4], 0);
        R[4] = gsu_ldw(gsu_codes_then(tex_rows_code[copy] + 16, 1, FX_TEX_V_OFF));
        R[7] = gsu_add(R[7], R[4], 0);
        R[0] = gsu_ldw(gsu_codes_then(tex_rows_code[copy] + 17, 1, FX_TEX_BANK));
        R[0] = gsu_lob(R[0]);
        gsu_codes(tex_rows_code[copy] + 18, 1);
        gsu_romb(R[0]);
        R[4] = gsu_ldw(FX_TEX_BASE);
        R[3] = gsu_ldw(gsu_codes_then(tex_rows_code[copy] + 19, 1, FX_TEX_MASK));
        R[13] = 0xA794;
        gsu_codes(tex_rows_code[copy] + 20, 1);
        do {
            gsu_codes(tex_rows_code[copy] + 21, 1);
            R[0] = gsu_merge();
            R[0] = gsu_and(R[0], R[3]);
            gsu_r14(gsu_add(R[0], R[4], 0));
            R[7] = gsu_add(R[7], R[6], 0);
            R[8] = gsu_add(R[8], R[5], 0);
            gsu_color((gsu_codes(tex_rows_code[copy] + 22, 1), gsu_getb()));              /* GETC */
            go = gsu_count_down(&R[12]);
            (gsu_codes(tex_rows_code[copy] + 23, 1), gsu_plot_r1());
        } while (go);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_codes(tex_rows_code[copy] + 24, 1);
        gsu_romb(R[0]);
        R[7] = gsu_ldw(FX_LEFT_SLOPE);
        R[8] = gsu_ldw(gsu_codes_then(tex_rows_code[copy] + 25, 1, FX_RIGHT_SLOPE));
        gsu_codes(tex_rows_code[copy] + 26, 3);
        edge_step(9, R[7], -1, GSU_COPY(edge_step_from_tex_rows[copy]));
        edge_step(10, R[8], -1, GSU_COPY(edge_step_from_tex_rows[copy]));
        R[2] = gsu_inc(R[2]);
        R[0] = gsu_swap(R[2]);
        R[0] = gsu_dec(R[0]);
        R[2] = gsu_swap(R[0]);
        R[0] = gsu_hib(R[2]);
    } while (gsu_codes_loop(tex_rows_code[copy] + 29, 1, !gsu.z));
    R[11] = gsu_ldw(gsu_codes_then(tex_rows_code[copy] + 30, 1, FX_LEFT_ROWS));
    R[3] = gsu_ldw(gsu_codes_then(tex_rows_code[copy] + 31, 1, FX_RIGHT_ROWS));
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t tex_rows_from_tex_polygon[9] = {0, 0, 1, 2, 3, 4, 3, 5, 3};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t tex_right_edge_from_tex_polygon[9] = {0, 1, 2, 2, 3, 4, 5, 1, 3};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t tex_left_edge_from_tex_polygon[9] = {0, 0, 1, 1, 2, 0, 2, 0, 2};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode tex_polygon_code[9][41] = {
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0, 0}, {0, 0}, {0, 0}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0x01A6CF, 11}, {0x01A909, 4}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0x01A7DA, 4}, {0x01A871, 4}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0, 0}, {0, 0}, {0, 0}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0x01A6CF, 11}, {0x01A909, 4}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0x01A7DA, 4}, {0x01A871, 4}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0x01A6A8, 4}, {0x01A6A4, 4}, {0x01A6AC, 16}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0, 0}, {0, 0}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0, 0}, {0, 0}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0x01A6A8, 4}, {0x01A6A4, 4}, {0x01A6AC, 16}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0, 0}, {0, 0}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0, 0}, {0, 0}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0, 0}, {0, 0}, {0, 0}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0, 0}, {0, 0}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0x01A7DA, 4}, {0x01A871, 4}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0, 0}, {0, 0}, {0, 0}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0, 0}, {0, 0}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0x01A7DA, 4}, {0x01A871, 4}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0, 0}, {0, 0}, {0, 0}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0, 0}, {0, 0}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0x01A7DA, 4}, {0x01A871, 4}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0, 0}, {0, 0}, {0, 0}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0x01A6CF, 11}, {0x01A909, 4}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0x01A7DA, 4}, {0x01A871, 4}},
    {{0x01A662, 11}, {0x01A91A, 4}, {0x01A66D, 4}, {0x01A671, 4}, {0x01A675, 7}, {0x01A67C, 5}, {0x01A681, 1}, {0x01A682, 5}, {0x01A687, 5}, {0x01A68C, 11}, {0x01A697, 3}, {0x01A69A, 2}, {0x01A69C, 6}, {0x01A6A2, 2}, {0x01A6A4, 24}, {0, 0}, {0, 0}, {0x01A6BC, 3}, {0x01A6BF, 4}, {0x01A6C3, 5}, {0x01A6C8, 3}, {0x01A6CB, 4}, {0x01A6CF, 2}, {0, 0}, {0, 0}, {0x01A6CF, 7}, {0x01A6DA, 10}, {0x01A6E4, 10}, {0x01A6EE, 4}, {0x01A6F2, 3}, {0x01A6F5, 4}, {0x01A6F9, 4}, {0x01A6FD, 3}, {0x01A700, 5}, {0x01A705, 3}, {0x01A708, 4}, {0xA70C, 0}, {0x01A70C, 4}, {0x01A7D9, 1}, {0x01A7DA, 4}, {0x01A871, 4}},
};

/* A662: a textured polygon (R0 = NOT its type, R1 at its bank byte). The
   vertices are copied below it as for a flat polygon, u and v of each
   moved down a byte where they are; a polygon with no height is passed.
   Then the edges from the top vertex down. */
static void tex_polygon(int copy)
{
    gsu_codes(tex_polygon_code[copy], 2);
    int go;

    R[0] = gsu_lob(R[0]);
    R[12] = R[0];
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[3] = R[0];
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_codes(tex_polygon_code[copy] + 2, 1);
    gsu_stw(FX_TEX_BANK, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_polygon_code[copy] + 3, 1, R[1]));
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_swap(R[0]);
    gsu_cmode(R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_polygon_code[copy] + 4, 1, R[1]));
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_codes(tex_polygon_code[copy] + 5, 1);
    gsu_stw(FX_TEX_BASE, R[0]);
    R[0] = gsu_ldw(gsu_codes_then(tex_polygon_code[copy] + 6, 1, R[1]));
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_codes(tex_polygon_code[copy] + 7, 1);
    gsu_stw(FX_TEX_MASK, R[0]);
    R[0] = gsu_add(R[1], R[3], 0);
    gsu_codes(tex_polygon_code[copy] + 8, 1);
    gsu_stw(FX_LIST_END, R[0]);
    R[5] = R[1];
    R[4] = gsu_sub(R[1], R[3], 0);
    R[2] = 0x03FF;
    R[8] = 0;
    R[13] = 0xA697;
    gsu_codes(tex_polygon_code[copy] + 9, 1);
    do {
        gsu_codes(tex_polygon_code[copy] + 10, 1);
        R[0] = gsu_ldb(R[5]);
        gsu_codes(tex_polygon_code[copy] + 11, 1);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_ldb(gsu_codes_then(tex_polygon_code[copy] + 12, 1, R[5]));
        gsu_codes(tex_polygon_code[copy] + 13, 1);
        gsu_stb(R[4], (uint8_t)R[0]);
        gsu_sub(R[0], R[2], 0);                 /* CMP R2 */
        if (gsu.s) {
            gsu_codes(tex_polygon_code[copy] + 14, 1);
            R[2] = R[0];
            R[6] = R[4];
        }
        gsu_codes(tex_polygon_code[copy] + 15, 2);
        gsu_sub(R[0], R[8], 0);                 /* CMP R8 */
        if (!gsu.s)
            R[8] = R[0];
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_sub(R[0], R[0], 0);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        gsu_codes(tex_polygon_code[copy] + 17, 1);
        gsu_stb(R[5], (uint8_t)R[0]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_codes(tex_polygon_code[copy] + 18, 1);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_ldb(gsu_codes_then(tex_polygon_code[copy] + 19, 1, R[5]));
        R[5] = gsu_dec(R[5]);
        gsu_codes(tex_polygon_code[copy] + 20, 1);
        gsu_stb(R[5], (uint8_t)R[0]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_codes(tex_polygon_code[copy] + 21, 1);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        go = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);
    } while (gsu_codes_loop(tex_polygon_code[copy] + 22, 1, go));
    R[0] = gsu_sub(R[2], R[8], 0);
    if (gsu.z)
        { gsu_codes(tex_polygon_code[copy] + 23, 2); return; } else { gsu_codes(tex_polygon_code[copy] + 25, 2); }
    R[4] = R[6];
    R[4] = gsu_dec(R[4]);
    R[4] = gsu_dec(R[4]);
    R[5] = gsu_add(R[4], R[3], 0);
    R[0] = gsu_ldb(R[4]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    R[9] = gsu_swap(R[0]);
    R[10] = R[9];
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    R[0] = gsu_ldb(gsu_codes_then(tex_polygon_code[copy] + 27, 1, R[4]));
    R[0] = gsu_swap(R[0]);
    gsu_codes(tex_polygon_code[copy] + 28, 1);
    gsu_stw(FX_LEFT_U, R[0]);
    gsu_codes(tex_polygon_code[copy] + 29, 1);
    gsu_stw(FX_RIGHT_U, R[0]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    R[0] = gsu_ldb(gsu_codes_then(tex_polygon_code[copy] + 30, 1, R[4]));
    R[0] = gsu_swap(R[0]);
    gsu_codes(tex_polygon_code[copy] + 31, 1);
    gsu_stw(FX_LEFT_V, R[0]);
    gsu_codes(tex_polygon_code[copy] + 32, 1);
    gsu_stw(FX_RIGHT_V, R[0]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    gsu_codes(tex_polygon_code[copy] + 33, 1);
    gsu_stw(FX_LEFT_AT, R[4]);
    gsu_codes(tex_polygon_code[copy] + 34, 1);
    gsu_stw(FX_RIGHT_AT, R[5]);
    R[11] = 0;
    R[3] = 0;
    gsu_codes(tex_polygon_code[copy] + 35, 4);
    for (;;) {
        gsu_codes(tex_polygon_code[copy] + 39, 1);
        /* A7D9 */
        gsu_moves(R[11]);
        R[0] = R[11];
        if (gsu.z && !tex_left_edge(GSU_COPY(tex_left_edge_from_tex_polygon[copy])))
            return; else { gsu_codes(tex_polygon_code[copy] + 40, 1); }
        /* A870 */
        gsu_moves(R[3]);
        R[0] = R[3];
        if (gsu.z && !tex_right_edge(GSU_COPY(tex_right_edge_from_tex_polygon[copy])))
            return;
        tex_rows(GSU_COPY(tex_rows_from_tex_polygon[copy]));
    }
}

/* ---- the screen-edge clip that feeds the list (01:9F01-A661) ---- */

#define FX_POLY_OUT     0x005Au  /* the projected polygon: x and y words a vertex */
#define FX_CLIP_IN      0x0060u  /* the list a clip reads (in FxPolygonClipScreen) */
#define FX_CLIP_OUT     0x0062u  /* and writes */
#define FX_CLIP_LEFT    0x0038u  /* the screen's edges: x from $38 to $3A, y from $3C to $3E */
#define FX_CLIP_RIGHT   0x003Au
#define FX_CLIP_TOP     0x003Cu
#define FX_CLIP_BOTTOM  0x003Eu
#define FX_CLIP_LIST    0x0688u  /* the other list */
#define FX_COUNT        0x0040u  /* vertices left to read */
#define FX_COUNT_OUT    0x0098u  /* vertices written */
#define FX_CODES        0x014Au  /* the outcodes: the edges to clip against */

void abi_FxDivideSigned(void);

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode mult_imm_code[8][1] = {
    {{0x01A2A0, 5}},
    {{0x01A385, 5}},
    {{0x01A470, 5}},
    {{0x01A561, 5}},
    {{0x019F17, 5}},
    {{0x019FE3, 5}},
    {{0x01A0B6, 5}},
    {{0x01A18E, 5}},
};

/* MULT #n: the signed product of the low byte and n. */
static uint16_t mult_imm(uint16_t v, int n, int copy)
{
    gsu_codes(mult_imm_code[copy], 1);
    uint16_t r = (uint16_t)((int8_t)v * n);

    gsu_sz(r);
    return r;
}

/* BGE: S equal to OV. */
static int ge(void)
{
    return gsu.s == gsu.ov;
}

/* One screen edge. Y: the edge is a row (top, bottom), compared with a
   vertex's y; STRICT: the inside is below the edge's value (right,
   bottom), SUB #1 after the compare; DEC_EDGE: a crossing is put one
   inside it; RET_OUT_IN and RET_IN_OUT: R11 for FxDivideSigned at the
   two crossings (LINK #4). */
typedef struct {
    uint16_t edge;
    int y, strict, dec_edge;
    uint16_t ret_out_in, ret_in_out;
} ClipEdge;

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode clip_put_code[19][2] = {
    {{0x01A337, 4}, {0x01A33B, 1}},
    {{0x01A30E, 4}, {0, 0}},
    {{0x01A60A, 4}, {0x01A60E, 1}},
    {{0x01A422, 4}, {0x01A426, 1}},
    {{0x01A3F9, 4}, {0, 0}},
    {{0x019F85, 4}, {0, 0}},
    {{0x01A081, 4}, {0x01A085, 1}},
    {{0x01A159, 4}, {0x01A15D, 1}},
    {{0x01A239, 4}, {0x01A23D, 1}},
    {{0x019FAE, 4}, {0x019FB2, 1}},
    {{0x01A058, 4}, {0, 0}},
    {{0x01A5DB, 4}, {0, 0}},
    {{0x01A513, 4}, {0x01A517, 1}},
    {{0x01A4E4, 4}, {0, 0}},
    {{0x01A12A, 4}, {0, 0}},
    {{0x01A209, 4}, {0, 0}},
    {{0x01A159, 4}, {0, 0}},
    {{0x01A239, 4}, {0, 0}},
    {{0x01A60A, 4}, {0, 0}},
};

/* The vertex in R1/R2 written at R9 (FROM R1; STW (R9); INC R9 twice;
   the same for R2). */
static void clip_put(int copy)
{
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_codes(clip_put_code[copy], 1);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_codes(clip_put_code[copy] + 1, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode clip_cross_code[30][65] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A2ED, 2}, {0, 0}, {0x01A2EF, 3}, {0x01A2F2, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A32B, 2}, {0, 0}, {0x01A32D, 3}, {0x01A330, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A416, 2}, {0, 0}, {0x01A418, 3}, {0x01A41B, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A3D6, 2}, {0, 0}, {0x01A3D8, 3}, {0x01A3DB, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x019F64, 2}, {0, 0}, {0x019F66, 3}, {0x019F69, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x019FA2, 2}, {0, 0}, {0x019FA4, 3}, {0x019FA7, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A075, 2}, {0, 0}, {0x01A077, 3}, {0x01A07A, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A034, 2}, {0, 0}, {0x01A036, 3}, {0x01A039, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A5FE, 1}, {0x01A5FF, 4}, {0, 0}, {0x01A603, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A5B8, 1}, {0x01A5B9, 4}, {0, 0}, {0x01A5BD, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A507, 1}, {0x01A508, 4}, {0, 0}, {0x01A50C, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A4C3, 1}, {0x01A4C4, 4}, {0, 0}, {0x01A4C8, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A14D, 1}, {0x01A14E, 4}, {0, 0}, {0x01A152, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A109, 1}, {0x01A10A, 4}, {0, 0}, {0x01A10E, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A1E5, 2}, {0x01A1E7, 4}, {0, 0}, {0x01A1EB, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A22C, 2}, {0x01A22E, 4}, {0, 0}, {0x01A232, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 32}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A31B, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A32B, 2}, {0, 0}, {0x01A32D, 3}, {0x01A330, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 16}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A2DD, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A2ED, 2}, {0, 0}, {0x01A2EF, 3}, {0x01A2F2, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A406, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A416, 2}, {0, 0}, {0x01A418, 3}, {0x01A41B, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A3C6, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A3D6, 2}, {0, 0}, {0x01A3D8, 3}, {0x01A3DB, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181AE, 10}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181BA, 2}, {0x01A024, 16}, {0x01A034, 2}, {0, 0}, {0x01A036, 3}, {0x01A039, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181AE, 10}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181BA, 2}, {0x01A065, 16}, {0x01A075, 2}, {0, 0}, {0x01A077, 3}, {0x01A07A, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 32}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181AB, 3}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A2DD, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A2ED, 2}, {0, 0}, {0x01A2EF, 3}, {0x01A2F2, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 32}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A31B, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A32B, 2}, {0, 0}, {0x01A32D, 3}, {0x01A330, 4}},
    {{0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A5A8, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A5B8, 1}, {0x01A5B9, 4}, {0, 0}, {0x01A5BD, 4}},
    {{0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A5EE, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A5FE, 1}, {0x01A5FF, 4}, {0, 0}, {0x01A603, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 36}, {0x0181A3, 8}, {0x0181A3, 8}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181AE, 10}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181BA, 2}, {0x019F54, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x019F64, 2}, {0, 0}, {0x019F66, 3}, {0x019F69, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 36}, {0x0181A3, 8}, {0x0181A3, 8}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181AE, 10}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181AE, 5}, {0x0181BA, 2}, {0x019F92, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x019FA2, 2}, {0, 0}, {0x019FA4, 3}, {0x019FA7, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 16}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A3C6, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A3D6, 2}, {0, 0}, {0x01A3D8, 3}, {0x01A3DB, 4}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x018186, 1}, {0x8187, 0}, {0x018187, 10}, {0x018192, 5}, {0x01819B, 12}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181A3, 4}, {0x0181B3, 5}, {0x0181BA, 2}, {0x01A406, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A416, 2}, {0, 0}, {0x01A418, 3}, {0x01A41B, 4}},
};

/* Where the edge from (R1, R2) to (R3, R6) crosses: R3, R6 = the
   difference, the other axis's offset = (edge - start) * its difference /
   this one's (LMULT, FxDivideSigned), written at R9 with the edge's
   value. */
static void clip_cross(const ClipEdge *e, uint16_t ret, int copy)
{
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    if (e->y) {
        R[0] = R[3];
        R[3] = R[6];
        R[6] = R[0];
    }
    R[0] = gsu_ldw(e->edge);
    R[0] = gsu_sub(R[0], R[e->y ? (gsu_codes(clip_cross_code[copy], 23), 2) : (gsu_codes(clip_cross_code[copy] + 23, 38), 1)], 0);
    {
        uint32_t p = gsu_lmult(R[0], R[6]);

        R[5] = (uint16_t)(p >> 16);
        R[4] = (uint16_t)p;
    }
    R[6] = R[3];
    R[11] = ret;
    abi_FxDivideSigned();
    R[0] = gsu_add(R[4], R[e->y ? 1 : 2], 0);
    R[6] = gsu_ldw(e->edge);
    if (e->dec_edge)
        R[6] = gsu_dec(R[6]);
    gsu_codes(clip_cross_code[copy] + 61, 1);
    if (e->y) {
        gsu_stw(R[9], R[0]);
        R[9] = gsu_inc(R[9]);
        R[9] = gsu_inc(R[9]);
        gsu_codes(clip_cross_code[copy] + 62, 1);
        gsu_stw(R[9], R[6]);
    } else {
        gsu_stw(R[9], R[6]);
        R[9] = gsu_inc(R[9]);
        R[9] = gsu_inc(R[9]);
        gsu_codes(clip_cross_code[copy] + 63, 1);
        gsu_stw(R[9], R[0]);
    }
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_codes(clip_cross_code[copy] + 64, 1);
}

/* Is the vertex in R(at) inside the edge? LMS R0, edge; FROM R(at); SUB
   R0 (and SUB #1 when strict); BGE, or BLT when strict. */
static int clip_inside(const ClipEdge *e, int at, int copy)
{
    R[0] = gsu_ldw(e->edge);
    R[0] = gsu_sub(R[at], R[0], 0);
    if (e->strict) {
        R[0] = gsu_sub(R[0], 1, 0);
        return !ge();
    }
    return ge();
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode clip_count_code[14][3] = {
    {{0x01A30A, 2}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A2FA, 2}, {0x01A2FC, 2}},
    {{0x01A3F5, 2}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A3E3, 2}, {0x01A3E5, 2}},
    {{0x019F81, 2}, {0, 0}, {0, 0}},
    {{0x01A054, 2}, {0, 0}, {0, 0}},
    {{0x01A5D7, 2}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A5C5, 2}, {0x01A5C7, 2}},
    {{0x01A4E0, 2}, {0, 0}, {0, 0}},
    {{0x01A126, 2}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A116, 2}, {0x01A118, 2}},
    {{0, 0}, {0x01A4D0, 2}, {0x01A4D2, 2}},
    {{0x01A205, 2}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A1F3, 2}, {0x01A1F5, 2}},
};

/* $0098 changed by one (LMS R0; INC or DEC R0; SBK). */
static void clip_count(int up, int copy)
{
    R[0] = gsu_ldw(FX_COUNT_OUT);
    R[0] = up ? (gsu_codes(clip_count_code[copy], 1), gsu_inc(R[0])) : (gsu_codes(clip_count_code[copy] + 1, 1), gsu_dec(R[0]));
    gsu_sbk(R[0]);
    gsu_codes(clip_count_code[copy] + 2, 1);
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t mult_imm_from_clip_against[25] = {0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 4, 5, 6, 7, 3, 0, 1, 3, 2, 6, 7, 1, 3, 4, 5};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t clip_put_from_clip_against[25] = {1, 4, 13, 11, 5, 10, 14, 15, 1, 4, 5, 10, 16, 17, 11, 1, 4, 2, 13, 7, 8, 4, 18, 9, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t clip_inside_from_clip_against[25] = {1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 2, 1, 2, 2, 1, 1, 2, 0, 0, 1, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t clip_cross_from_clip_against[25] = {1, 2, 10, 8, 5, 6, 12, 15, 16, 18, 27, 21, 0, 0, 25, 23, 29, 0, 10, 0, 0, 2, 0, 0, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t clip_count_from_clip_against[25] = {1, 3, 11, 7, 4, 5, 10, 13, 0, 3, 4, 5, 0, 0, 7, 1, 3, 0, 11, 0, 0, 3, 0, 0, 0};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode clip_against_code[25][26] = {
    {{0, 0}, {0x01A296, 4}, {0x01A29A, 6}, {0x01A2A5, 4}, {0x01A2A9, 2}, {0x01A2AB, 4}, {0x01A2AF, 4}, {0x01A2B3, 4}, {0, 0}, {0x01A2B7, 6}, {0, 0}, {0, 0}, {0x01A2C5, 4}, {0x01A2F6, 4}, {0x01A2C9, 20}, {0x01A302, 4}, {0x01A334, 3}, {0x01A302, 8}, {0x01A30C, 2}, {0x01A312, 9}, {0x01A33C, 4}, {0x01A340, 2}, {0x01A2A8, 1}, {0x01A342, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A37B, 4}, {0x01A37F, 6}, {0x01A38A, 4}, {0x01A38E, 2}, {0x01A390, 4}, {0x01A394, 4}, {0x01A398, 4}, {0, 0}, {0x01A39C, 6}, {0, 0}, {0, 0}, {0x01A3AC, 6}, {0x01A3DF, 4}, {0x01A3B2, 20}, {0x01A3EB, 6}, {0x01A41F, 3}, {0x01A3EB, 10}, {0x01A3F7, 2}, {0x01A3FD, 9}, {0x01A427, 4}, {0x01A42B, 2}, {0x01A38D, 1}, {0x01A42D, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A466, 4}, {0x01A46A, 6}, {0x01A475, 4}, {0x01A479, 2}, {0x01A47B, 4}, {0x01A47F, 4}, {0x01A483, 4}, {0x01A487, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A495, 4}, {0x01A4CC, 4}, {0x01A499, 26}, {0x01A4D8, 4}, {0x01A510, 3}, {0x01A4D8, 8}, {0x01A4E2, 2}, {0x01A4E8, 15}, {0x01A518, 4}, {0x01A51C, 2}, {0x01A478, 1}, {0x01A51E, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A557, 4}, {0x01A55B, 6}, {0x01A566, 4}, {0x01A56A, 2}, {0x01A56C, 4}, {0x01A570, 4}, {0x01A574, 4}, {0x01A578, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A588, 6}, {0x01A5C1, 4}, {0x01A58E, 26}, {0x01A5CD, 6}, {0x01A607, 3}, {0x01A5CD, 10}, {0x01A5D9, 2}, {0x01A5DF, 15}, {0x01A60F, 4}, {0x01A613, 2}, {0x01A569, 1}, {0x01A615, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x019F0D, 4}, {0x019F11, 6}, {0x019F1C, 4}, {0x019F20, 2}, {0x019F22, 4}, {0x019F26, 4}, {0x019F2A, 4}, {0, 0}, {0x019F2E, 6}, {0, 0}, {0, 0}, {0x019F3C, 24}, {0, 0}, {0, 0}, {0x019F79, 4}, {0x019FAB, 3}, {0x019F79, 8}, {0x019F83, 2}, {0x019F89, 9}, {0x019FB3, 4}, {0x019FB7, 2}, {0x019F1F, 1}, {0x019FB9, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x019FD9, 4}, {0x019FDD, 6}, {0x019FE8, 4}, {0x019FEC, 2}, {0x019FEE, 4}, {0x019FF2, 4}, {0x019FF6, 4}, {0, 0}, {0x019FFA, 6}, {0, 0}, {0, 0}, {0x01A00A, 26}, {0, 0}, {0, 0}, {0x01A04A, 6}, {0x01A07E, 3}, {0x01A04A, 10}, {0x01A056, 2}, {0x01A05C, 9}, {0x01A086, 4}, {0x01A08A, 2}, {0x019FEB, 1}, {0x01A08C, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A0AC, 4}, {0x01A0B0, 6}, {0x01A0BB, 4}, {0x01A0BF, 2}, {0x01A0C1, 4}, {0x01A0C5, 4}, {0x01A0C9, 4}, {0x01A0CD, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A0DB, 4}, {0x01A112, 4}, {0x01A0DF, 26}, {0x01A11E, 4}, {0x01A156, 3}, {0x01A11E, 8}, {0x01A128, 2}, {0x01A12E, 15}, {0x01A15E, 4}, {0x01A162, 2}, {0x01A0BE, 1}, {0x01A164, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A184, 4}, {0x01A188, 6}, {0x01A193, 4}, {0x01A197, 2}, {0x01A199, 4}, {0x01A19D, 4}, {0x01A1A1, 4}, {0x01A1A5, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A1B5, 6}, {0x01A1EF, 4}, {0x01A1BB, 26}, {0x01A1FB, 6}, {0x01A236, 3}, {0x01A1FB, 10}, {0x01A207, 2}, {0x01A20D, 15}, {0x01A23E, 4}, {0x01A242, 2}, {0x01A196, 1}, {0x01A244, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A296, 4}, {0x01A29A, 6}, {0x01A2A5, 4}, {0x01A2A9, 2}, {0x01A2AB, 4}, {0x01A2AF, 4}, {0x01A2B3, 4}, {0, 0}, {0x01A2B7, 6}, {0, 0}, {0, 0}, {0x01A2C5, 4}, {0x01A2F6, 4}, {0x01A2C9, 20}, {0x01A302, 4}, {0x01A334, 3}, {0x01A302, 8}, {0x01A30C, 2}, {0x01A312, 9}, {0x01A33C, 4}, {0x01A340, 2}, {0x01A2A8, 1}, {0x01A342, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A37B, 4}, {0x01A37F, 6}, {0x01A38A, 4}, {0x01A38E, 2}, {0x01A390, 4}, {0x01A394, 4}, {0x01A398, 4}, {0, 0}, {0x01A39C, 6}, {0, 0}, {0, 0}, {0x01A3AC, 6}, {0x01A3DF, 4}, {0x01A3B2, 20}, {0x01A3EB, 6}, {0x01A41F, 3}, {0x01A3EB, 10}, {0x01A3F7, 2}, {0x01A3FD, 9}, {0x01A427, 4}, {0x01A42B, 2}, {0x01A38D, 1}, {0x01A42D, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x019F0D, 4}, {0x019F11, 6}, {0x019F1C, 4}, {0x019F20, 2}, {0x019F22, 4}, {0x019F26, 4}, {0x019F2A, 4}, {0, 0}, {0x019F2E, 6}, {0, 0}, {0, 0}, {0x019F3C, 24}, {0, 0}, {0, 0}, {0x019F79, 4}, {0x019FAB, 3}, {0x019F79, 8}, {0x019F83, 2}, {0x019F89, 9}, {0x019FB3, 4}, {0x019FB7, 2}, {0x019F1F, 1}, {0x019FB9, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x019FD9, 4}, {0x019FDD, 6}, {0x019FE8, 4}, {0x019FEC, 2}, {0x019FEE, 4}, {0x019FF2, 4}, {0x019FF6, 4}, {0, 0}, {0x019FFA, 6}, {0, 0}, {0, 0}, {0x01A00A, 26}, {0, 0}, {0, 0}, {0x01A04A, 6}, {0x01A07E, 3}, {0x01A04A, 10}, {0x01A056, 2}, {0x01A05C, 9}, {0x01A086, 4}, {0x01A08A, 2}, {0x019FEB, 1}, {0x01A08C, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A0AC, 4}, {0x01A0B0, 6}, {0x01A0BB, 4}, {0x01A0BF, 2}, {0x01A0C1, 4}, {0x01A0C5, 4}, {0x01A0C9, 4}, {0x01A0CD, 6}, {0, 0}, {0x01A0D3, 4}, {0x01A11A, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01A11E, 4}, {0x01A156, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A15D, 5}, {0x01A162, 2}, {0x01A0BE, 1}, {0x01A164, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A184, 4}, {0x01A188, 6}, {0x01A193, 4}, {0x01A197, 2}, {0x01A199, 4}, {0x01A19D, 4}, {0x01A1A1, 4}, {0x01A1A5, 6}, {0, 0}, {0x01A1AB, 6}, {0x01A1F7, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01A1FB, 6}, {0x01A236, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A23D, 5}, {0x01A242, 2}, {0x01A196, 1}, {0x01A244, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A557, 4}, {0x01A55B, 6}, {0x01A566, 4}, {0x01A56A, 2}, {0x01A56C, 4}, {0x01A570, 4}, {0x01A574, 4}, {0x01A578, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A588, 6}, {0x01A5C1, 4}, {0x01A58E, 26}, {0x01A5CD, 6}, {0x01A607, 3}, {0x01A5CD, 10}, {0x01A5D9, 2}, {0x01A5DF, 15}, {0x01A60F, 4}, {0x01A613, 2}, {0x01A569, 1}, {0x01A615, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A296, 4}, {0x01A29A, 6}, {0x01A2A5, 4}, {0x01A2A9, 2}, {0x01A2AB, 4}, {0x01A2AF, 4}, {0x01A2B3, 4}, {0, 0}, {0x01A2B7, 6}, {0, 0}, {0, 0}, {0x01A2C5, 4}, {0x01A2F6, 4}, {0x01A2C9, 20}, {0x01A302, 4}, {0x01A334, 3}, {0x01A302, 8}, {0x01A30C, 2}, {0x01A312, 9}, {0x01A33C, 4}, {0x01A340, 2}, {0x01A2A8, 1}, {0x01A342, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A37B, 4}, {0x01A37F, 6}, {0x01A38A, 4}, {0x01A38E, 2}, {0x01A390, 4}, {0x01A394, 4}, {0x01A398, 4}, {0, 0}, {0x01A39C, 6}, {0, 0}, {0, 0}, {0x01A3AC, 6}, {0x01A3DF, 4}, {0x01A3B2, 20}, {0x01A3EB, 6}, {0x01A41F, 3}, {0x01A3EB, 10}, {0x01A3F7, 2}, {0x01A3FD, 9}, {0x01A427, 4}, {0x01A42B, 2}, {0x01A38D, 1}, {0x01A42D, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A557, 4}, {0x01A55B, 6}, {0x01A566, 4}, {0x01A56A, 2}, {0x01A56C, 4}, {0x01A570, 4}, {0x01A574, 4}, {0x01A578, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A588, 6}, {0x01A5C1, 4}, {0x01A58E, 26}, {0x01A5CD, 6}, {0x01A607, 3}, {0x01A5CD, 10}, {0x01A5D9, 2}, {0x01A5DF, 15}, {0x01A60F, 4}, {0x01A613, 2}, {0x01A569, 1}, {0x01A615, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A466, 4}, {0x01A46A, 6}, {0x01A475, 4}, {0x01A479, 2}, {0x01A47B, 4}, {0x01A47F, 4}, {0x01A483, 4}, {0x01A487, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A495, 4}, {0x01A4CC, 4}, {0x01A499, 26}, {0x01A4D8, 4}, {0x01A510, 3}, {0x01A4D8, 8}, {0x01A4E2, 2}, {0x01A4E8, 15}, {0x01A518, 4}, {0x01A51C, 2}, {0x01A478, 1}, {0x01A51E, 6}, {0x01A51E, 2}, {0, 0}},
    {{0, 0}, {0x01A0AC, 4}, {0x01A0B0, 6}, {0x01A0BB, 4}, {0x01A0BF, 2}, {0x01A0C1, 4}, {0x01A0C5, 4}, {0x01A0C9, 4}, {0x01A0CD, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A0DB, 4}, {0x01A112, 4}, {0x01A0DF, 26}, {0x01A11E, 4}, {0x01A156, 3}, {0x01A11E, 8}, {0x01A128, 2}, {0x01A12E, 15}, {0x01A15E, 4}, {0x01A162, 2}, {0x01A0BE, 1}, {0x01A164, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A184, 4}, {0x01A188, 6}, {0x01A193, 4}, {0x01A197, 2}, {0x01A199, 4}, {0x01A19D, 4}, {0x01A1A1, 4}, {0x01A1A5, 6}, {0, 0}, {0, 0}, {0, 0}, {0x01A1B5, 6}, {0x01A1EF, 4}, {0x01A1BB, 26}, {0x01A1FB, 6}, {0x01A236, 3}, {0x01A1FB, 10}, {0x01A207, 2}, {0x01A20D, 15}, {0x01A23E, 4}, {0x01A242, 2}, {0x01A196, 1}, {0x01A244, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A37B, 4}, {0x01A37F, 6}, {0x01A38A, 4}, {0x01A38E, 2}, {0x01A390, 4}, {0x01A394, 4}, {0x01A398, 4}, {0, 0}, {0x01A39C, 6}, {0, 0}, {0, 0}, {0x01A3AC, 6}, {0x01A3DF, 4}, {0x01A3B2, 20}, {0, 0}, {0, 0}, {0x01A3EB, 10}, {0x01A3F7, 2}, {0x01A3FD, 9}, {0x01A427, 4}, {0x01A42B, 2}, {0x01A38D, 1}, {0x01A42D, 6}, {0, 0}, {0, 0}},
    {{0x01A555, 2}, {0x01A557, 4}, {0x01A55B, 6}, {0x01A566, 4}, {0x01A56A, 2}, {0x01A56C, 4}, {0x01A570, 4}, {0x01A574, 4}, {0x01A578, 6}, {0, 0}, {0x01A57E, 6}, {0x01A5C9, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01A5CD, 6}, {0x01A607, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A60E, 5}, {0x01A613, 2}, {0x01A569, 1}, {0x01A615, 6}, {0x01A615, 2}, {0x01A61B, 4}},
    {{0, 0}, {0x019F0D, 4}, {0x019F11, 6}, {0x019F1C, 4}, {0x019F20, 2}, {0x019F22, 4}, {0x019F26, 4}, {0x019F2A, 4}, {0, 0}, {0x019F2E, 6}, {0, 0}, {0, 0}, {0x019F3C, 24}, {0, 0}, {0, 0}, {0x019F79, 4}, {0x019FAB, 3}, {0x019F79, 8}, {0x019F83, 2}, {0x019F89, 9}, {0x019FB3, 4}, {0x019FB7, 2}, {0x019F1F, 1}, {0x019FB9, 6}, {0, 0}, {0, 0}},
    {{0, 0}, {0x019FD9, 4}, {0x019FDD, 6}, {0x019FE8, 4}, {0x019FEC, 2}, {0x019FEE, 4}, {0x019FF2, 4}, {0x019FF6, 4}, {0, 0}, {0x019FFA, 6}, {0, 0}, {0, 0}, {0x01A00A, 26}, {0, 0}, {0, 0}, {0x01A04A, 6}, {0x01A07E, 3}, {0x01A04A, 10}, {0x01A056, 2}, {0x01A05C, 9}, {0x01A086, 4}, {0x01A08A, 2}, {0x019FEB, 1}, {0x01A08C, 6}, {0, 0}, {0, 0}},
};

/* The polygon at R8 ($0040 vertices) clipped against one edge into R9,
   $0098 kept as the count written: the first vertex is copied after the
   last, then each edge of the polygon (R1, R2 to R3, R6) gives its start
   when inside and its crossing when it crosses. */
static void clip_against(const ClipEdge *e, int copy)
{
    gsu_codes(clip_against_code[copy], 1);
    int a, b;

    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    R[2] = gsu_ldw(gsu_codes_then(clip_against_code[copy] + 1, 1, R[8]));
    R[8] = gsu_sub(R[8], 2, 0);
    R[0] = gsu_ldw(gsu_codes_then(clip_against_code[copy] + 2, 1, FX_COUNT));
    R[0] = mult_imm(R[0], 4, GSU_COPY(mult_imm_from_clip_against[copy]));
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_codes(clip_against_code[copy] + 3, 1);
    gsu_stw(R[0], R[2]);
    do {
        gsu_codes(clip_against_code[copy] + 4, 1);
        R[1] = gsu_ldw(R[8]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        R[2] = gsu_ldw(gsu_codes_then(clip_against_code[copy] + 5, 1, R[8]));
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        R[3] = gsu_ldw(gsu_codes_then(clip_against_code[copy] + 6, 1, R[8]));
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        R[6] = gsu_ldw(gsu_codes_then(clip_against_code[copy] + 7, 1, R[8]));
        R[8] = gsu_sub(R[8], 2, 0);
        a = clip_inside(e, e->y ? (gsu_codes(clip_against_code[copy] + 8, 1), 2) : (gsu_codes(clip_against_code[copy] + 9, 1), 1), GSU_COPY(clip_inside_from_clip_against[copy]));
        b = clip_inside(e, e->y ? (gsu_codes(clip_against_code[copy] + 10, 2), 6) : 3, GSU_COPY(clip_inside_from_clip_against[copy]));
        if (!a && (gsu_codes(clip_against_code[copy] + 12, 1), !b)) {
            gsu_codes(clip_against_code[copy] + 13, 1);
            clip_count(0, GSU_COPY(clip_count_from_clip_against[copy]));
        } else if (!a) {
            gsu_codes(clip_against_code[copy] + 14, 1);
            /* coming in: the ends swapped, the crossing written */
            R[0] = R[1];
            R[1] = R[3];
            R[3] = R[0];
            R[0] = R[2];
            R[2] = R[6];
            R[6] = R[0];
            clip_cross(e, e->ret_out_in, GSU_COPY(clip_cross_from_clip_against[copy]));
        } else if (b) {
            gsu_codes(clip_against_code[copy] + 15, 2);
            clip_put(GSU_COPY(clip_put_from_clip_against[copy]));
        } else {
            gsu_codes(clip_against_code[copy] + 17, 1);
            clip_count(1, GSU_COPY(clip_count_from_clip_against[copy]));
            gsu_codes(clip_against_code[copy] + 18, 1);
            clip_put(GSU_COPY(clip_put_from_clip_against[copy]));
            gsu_codes(clip_against_code[copy] + 19, 1);
            clip_cross(e, e->ret_in_out, GSU_COPY(clip_cross_from_clip_against[copy]));
        }
        R[0] = gsu_ldw(gsu_codes_then(clip_against_code[copy] + 20, 1, FX_COUNT));
        R[0] = gsu_dec(R[0]);
        gsu_codes(clip_against_code[copy] + 21, 1);
        gsu_sbk(R[0]);
    } while (gsu_codes_loop(clip_against_code[copy] + 22, 2, !gsu.z));
    gsu_codes(clip_against_code[copy] + 24, 2);
}

static const ClipEdge line_edges[4] = {
    {FX_CLIP_LEFT, 0, 0, 0, 0x9F5E, 0x9F9C},
    {FX_CLIP_RIGHT, 0, 1, 0, 0xA02E, 0xA06F},
    {FX_CLIP_TOP, 1, 0, 0, 0xA103, 0xA147},
    {FX_CLIP_BOTTOM, 1, 1, 1, 0xA1DF, 0xA226},
};

static const ClipEdge polygon_edges[4] = {
    {FX_CLIP_LEFT, 0, 0, 0, 0xA2E7, 0xA325},
    {FX_CLIP_RIGHT, 0, 1, 0, 0xA3D0, 0xA410},
    {FX_CLIP_TOP, 1, 0, 0, 0xA4BD, 0xA501},
    {FX_CLIP_BOTTOM, 1, 1, 0, 0xA5B2, 0xA5F8},
};

/* A65C: nothing is left: R0 = 0, R15 pulled. */
static void clip_none(void)
{
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_pull(&R[10]);
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t clip_against_from_clip_line_edge[12] = {4, 5, 6, 7, 10, 11, 12, 13, 19, 20, 23, 24};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode clip_line_edge_code[12][7] = {
    {{0, 0}, {0x019F05, 3}, {0x019F08, 5}, {0x019FB9, 2}, {0x019FBF, 4}, {0, 0}, {0, 0}},
    {{0x019FD1, 6}, {0, 0}, {0x019FD7, 2}, {0x01A08C, 2}, {0x01A092, 4}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A0A4, 3}, {0x01A0A7, 5}, {0x01A164, 2}, {0x01A16A, 4}, {0x01A16E, 8}, {0x01A65C, 6}},
    {{0x01A17C, 6}, {0, 0}, {0x01A182, 2}, {0x01A244, 2}, {0x01A24A, 4}, {0x01A24E, 8}, {0x01A65C, 6}},
    {{0, 0}, {0x019F05, 3}, {0x019F08, 5}, {0x019FB9, 2}, {0x019FBF, 4}, {0, 0}, {0, 0}},
    {{0x019FD1, 6}, {0, 0}, {0x019FD7, 2}, {0x01A08C, 2}, {0x01A092, 4}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A0A4, 3}, {0x01A0A7, 5}, {0x01A164, 2}, {0x01A16A, 4}, {0, 0}, {0, 0}},
    {{0x01A17C, 6}, {0, 0}, {0x01A182, 2}, {0x01A244, 2}, {0x01A24A, 4}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01A0A4, 3}, {0x01A0A7, 5}, {0x01A164, 2}, {0x01A16A, 4}, {0x01A16E, 8}, {0x01A65C, 6}},
    {{0x01A17C, 6}, {0, 0}, {0x01A182, 2}, {0x01A244, 2}, {0x01A24A, 4}, {0, 0}, {0, 0}},
    {{0, 0}, {0x019F05, 3}, {0x019F08, 5}, {0x019FB9, 2}, {0x019FBF, 4}, {0, 0}, {0, 0}},
    {{0x019FD1, 6}, {0, 0}, {0x019FD7, 2}, {0x01A08C, 2}, {0x01A092, 4}, {0, 0}, {0, 0}},
};

/* 9F01: a line (R0 = its count, below 3) clipped against all four edges,
   from $005A to $0688 and back twice; fewer than 2 points left is
   nothing. Ends with R0 = 2, R15 pulled. */
/* One edge of clip_line (the original has each edge in its own code):
   0 when nothing is left (R15 pulled). */
static int clip_line_edge(int k, int copy)
{
    gsu_stw(FX_COUNT, R[0]);
    if (k & 1) {
        gsu_codes(clip_line_edge_code[copy], 1);
        R[8] = FX_CLIP_LIST;
        R[9] = gsu_ldw(FX_POLY_OUT);
    } else {
        gsu_codes(clip_line_edge_code[copy] + 1, 1);
        R[8] = gsu_ldw(FX_POLY_OUT);
        R[9] = FX_CLIP_LIST;
    }
    gsu_codes(clip_line_edge_code[copy] + 2, 1);
    clip_against(&line_edges[k], GSU_COPY(clip_against_from_clip_line_edge[copy]));
    R[0] = gsu_ldw(gsu_codes_then(clip_line_edge_code[copy] + 3, 2, FX_COUNT_OUT));
    R[0] = gsu_sub(R[0], 2, 0);
    if (gsu.s) {
        gsu_codes(clip_line_edge_code[copy] + 5, 2);
        clip_none();
        return 0;
    }
    R[0] = gsu_add(R[0], 2, 0);
    return 1;
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t clip_line_edge_from_clip_line[5] = {3, 7, 0, 11, 9};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode clip_line_code[5][10] = {
    {{0x019F01, 4}, {0x01A271, 12}, {0x019FC3, 4}, {0x019FCB, 6}, {0x01A096, 4}, {0x01A09E, 6}, {0x01A16E, 4}, {0x01A176, 6}, {0x01A24E, 4}, {0x01A256, 9}},
    {{0x019F01, 4}, {0x01A271, 12}, {0x019FC3, 4}, {0x019FCB, 6}, {0x01A096, 4}, {0x01A09E, 6}, {0x01A16E, 4}, {0x01A176, 6}, {0x01A24E, 4}, {0x01A256, 9}},
    {{0x019F01, 4}, {0x01A271, 12}, {0x019FC3, 4}, {0x019FCB, 6}, {0x01A096, 4}, {0x01A09E, 6}, {0x01A16E, 4}, {0x01A176, 6}, {0x01A24E, 4}, {0x01A256, 9}},
    {{0x019F01, 4}, {0x01A271, 12}, {0x019FC3, 4}, {0x019FCB, 6}, {0x01A096, 4}, {0x01A09E, 6}, {0x01A176, 6}, {0, 0}, {0x01A256, 9}, {0, 0}},
    {{0x019F01, 4}, {0x01A271, 12}, {0x019FC3, 4}, {0x019FCB, 6}, {0x01A096, 4}, {0x01A09E, 6}, {0x01A16E, 4}, {0x01A176, 6}, {0x01A24E, 4}, {0x01A256, 9}},
};

static void clip_line(int copy)
{
    gsu_codes(clip_line_code[copy], 2);
    if (!clip_line_edge(0, GSU_COPY(clip_line_edge_from_clip_line[copy])))
        return; else { gsu_codes(clip_line_code[copy] + 2, 2); }
    if (!clip_line_edge(1, GSU_COPY(clip_line_edge_from_clip_line[copy])))
        return; else { gsu_codes(clip_line_code[copy] + 4, 2); }
    if (!clip_line_edge(2, GSU_COPY(clip_line_edge_from_clip_line[copy])))
        return; else { gsu_codes(clip_line_code[copy] + 6, 2); }
    if (!clip_line_edge(3, GSU_COPY(clip_line_edge_from_clip_line[copy])))
        return; else { gsu_codes(clip_line_code[copy] + 8, 2); }
    R[0] = 0x0002;
    gsu_pull(&R[10]);
}

/* ---- a shape's points gathered, clipped to the near plane, projected
   (01:98D7-9A51) ---- */

#define FX_POINTS       0x0340u  /* the object's points: x, y, z words */
#define FX_OFFSET_X     0x0026u  /* the object's position, added to each point */
#define FX_OFFSET_Y     0x0028u
#define FX_OFFSET_Z     0x002Au
#define FX_ROM_R14      0x001Au

void abi_FxSegmentCrossZ(void);
void abi_FxProjectVertex(void);

/* UMULT #n: the unsigned product of the low byte and n. */
static uint16_t umult_imm(uint16_t v, int n)
{
    gsu_code(0x0198F0, 8);
    uint16_t r = (uint16_t)((v & 0xFF) * n);

    gsu_sz(r);
    return r;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode near_load_code[5][10] = {
    {{0x019958, 3}, {0x01995B, 4}, {0x01995F, 4}, {0x019963, 7}, {0x01996D, 1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01996E, 2}, {0x019970, 4}, {0x019974, 4}, {0, 0}, {0, 0}, {0, 0}, {0x019978, 7}, {0, 0}, {0, 0}, {0, 0}},
    {{0x019A08, 2}, {0x019A0A, 4}, {0x019A0E, 4}, {0x018BFF, 7}, {0x018C0E, 7}, {0x019A12, 6}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x019914, 11}, {0x01991F, 4}, {0x019923, 4}, {0x019927, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0x01992B, 4}, {0x01992F, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01981A, 1}, {0x981B, 0}, {0x01981B, 5}, {0x019933, 4}},
};

/* Three words from R7 to registers (TO Rn; LDW (R7); INC R7 twice), the
   last without its INCs when LAST. */
static void near_load(int a, int b, int c, int last, int copy)
{
    gsu_codes(near_load_code[copy], 1);
    R[a] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[b] = gsu_ldw(gsu_codes_then(near_load_code[copy] + 1, 1, R[7]));
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[c] = gsu_ldw(gsu_codes_then(near_load_code[copy] + 2, 1, R[7]));
    if (!last) {
        gsu_codes(near_load_code[copy] + 3, 3);
        R[7] = gsu_inc(R[7]);
        R[7] = gsu_inc(R[7]);
    } else { gsu_codes(near_load_code[copy] + 6, 4); }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode near_put_code[5][5] = {
    {{0x0198D4, 3}, {0x0199C5, 3}, {0x0199C8, 4}, {0x0199CC, 4}, {0x0199D0, 5}},
    {{0x01998C, 3}, {0, 0}, {0x01998F, 4}, {0x019993, 4}, {0x019997, 5}},
    {{0x01988A, 3}, {0x0199AB, 3}, {0x0199AE, 4}, {0x0199B2, 4}, {0x0199B6, 5}},
    {{0x019937, 3}, {0, 0}, {0x01993A, 4}, {0x01993E, 4}, {0x019942, 4}},
    {{0, 0}, {0, 0}, {0x019946, 4}, {0x01994A, 4}, {0x01994E, 7}},
};

/* Three registers written at R8 (FROM Rn; STW (R8); INC R8 twice). */
static void near_put(int a, int b, int c, int copy)
{
    gsu_codes(near_put_code[copy], 2);
    gsu_stw(R[8], R[a]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_codes(near_put_code[copy] + 2, 1);
    gsu_stw(R[8], R[b]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_codes(near_put_code[copy] + 3, 1);
    gsu_stw(R[8], R[c]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_codes(near_put_code[copy] + 4, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode near_count_code[3][2] = {
    {{0x0199D5, 2}, {0, 0}},
    {{0x01999C, 2}, {0x01999E, 4}},
    {{0x0199BB, 2}, {0x0199BD, 3}},
};

/* $0040 counted up (LMS R0; INC R0; SBK). */
static void near_count(int copy)
{
    R[0] = gsu_ldw(FX_COUNT);
    R[0] = gsu_inc(R[0]);
    gsu_codes(near_count_code[copy], 1);
    gsu_sbk(R[0]);
    gsu_codes(near_count_code[copy] + 1, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode gather_return_code[2][3] = {
    {{0x019A37, 8}, {0x019A43, 5}, {0, 0}},
    {{0x019A37, 8}, {0x019A43, 5}, {0x0191DA, 1}},
};

/* 9A4C, and 9A43: R0 = 0, or the count, and R15 pulled. */
static void gather_return(int count, int copy)
{
    gsu_codes(gather_return_code[copy], 1);
    if (count)
        R[0] = gsu_ldw(gsu_codes_then(gather_return_code[copy] + 1, 1, FX_COUNT));
    else
        R[0] = gsu_sub(R[0], R[0], 0);
    gsu_pull(&R[10]);
    gsu_codes(gather_return_code[copy] + 2, 1);
}

/*
 * @routine 01:98D7 FxPolygonGather
 * WHAT IT DOES: Takes a shape of R0 points (index bytes in ROM at R14)
 * to the screen: each point's x, y, z from the object's points at $0340
 * (6 bytes each) plus the object's position ($0026-$002A) into the list
 * at the word at $005A; the shape clipped against the near plane (z < 0
 * is behind) into $0688, $0040 = the count left; then each vertex
 * projected (FxProjectVertex), screen x and y back at $005A and the
 * outcodes ORed into $014A. R0 = the count, or 0 when fewer than 3 are
 * left or not every one of the outcode bits $0F00 is set.
 * HOW: Pushes R11 on the R10 stack. A LOOP of R12 = the count (R13 =
 * $98ED) of GETB, UMULT #6, three LDW and ADD. A line (2 points) is
 * clipped as one segment (FxSegmentCrossZ) and both ends kept. Else each
 * edge, the last back to the first: both ends behind (z AND z negative)
 * gives nothing, the start behind its crossing, the start in front
 * itself and the crossing when the end is behind; each written counts
 * $0040 up. The projection (ROMB $14, R3, R13, R9 from $0026-$002A, R14
 * saved at $001A, $014A cleared) is a count of DEC R12 with the SBK of
 * the outcode in the branch's delay slot; then ROMB from $001C, R14 from
 * $001A, R3 = $005A. Returns by pulling R15. The jump at 99E6 (fewer
 * than 3 left) is decoded from the ROM bytes.
 * Entry: R0 = the count; R14 at the index bytes; R10 = stack; R11 =
 * return address. Exit: R0 as above; R1 = $0F00 and R0's AND (or the last
 * clip's when it ended early); R3 = $005A; R8 past the screen points;
 * R12 = 0; R14 as saved; S and Z from R10; the last RAM address R10. The
 * rest as the last call leaves them. The ROM buffer moves as on the
 * Super FX.
 */
void abi_FxPolygonGather(void)
{
    gsu_code(0x0198D8, 2);
    int go;

    gsu_push(&R[10], R[11]);
    R[9] = R[0];
    R[5] = gsu_ldw(gsu_code_then(0x0198DA, 7, FX_OFFSET_X));
    R[6] = gsu_ldw(gsu_code_then(0x0198E1, 3, FX_OFFSET_Y));
    R[7] = gsu_ldw(gsu_code_then(0x0198E4, 3, FX_OFFSET_Z));
    R[2] = gsu_ldw(gsu_code_then(0x0198E7, 3, FX_POLY_OUT));
    R[12] = R[9];
    R[13] = 0x98ED;
    gsu_code(0x0198EA, 3);
    do {
        gsu_code(0x0198ED, 2);
        R[0] = gsu_getb();
        gsu_code(0x0198EF, 1);
        gsu_r14(gsu_inc(R[14]));
        R[0] = umult_imm(R[0], 6);
        R[1] = gsu_add(FX_POINTS, R[0], 0);
        R[0] = gsu_ldw(R[1]);
        R[1] = gsu_inc(R[1]);
        R[1] = gsu_inc(R[1]);
        R[0] = gsu_add(R[0], R[5], 0);
        gsu_code(0x0198F8, 4);
        gsu_stw(R[2], R[0]);
        R[2] = gsu_inc(R[2]);
        R[2] = gsu_inc(R[2]);
        R[0] = gsu_ldw(gsu_code_then(0x0198FC, 3, R[1]));
        R[1] = gsu_inc(R[1]);
        R[1] = gsu_inc(R[1]);
        R[0] = gsu_add(R[0], R[6], 0);
        gsu_code(0x0198FF, 4);
        gsu_stw(R[2], R[0]);
        R[2] = gsu_inc(R[2]);
        R[2] = gsu_inc(R[2]);
        R[0] = gsu_ldw(gsu_code_then(0x019903, 3, R[1]));
        R[0] = gsu_add(R[0], R[7], 0);
        gsu_code(0x019906, 2);
        gsu_stw(R[2], R[0]);
        R[2] = gsu_inc(R[2]);
        go = gsu_count_down(&R[12]);
        R[2] = gsu_inc(R[2]);
    } while (gsu_code_loop(0x019908, 2, go));
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x019908, 9);
    gsu_stw(FX_COUNT, R[0]);
    R[7] = gsu_ldw(gsu_code_then(0x019911, 3, FX_POLY_OUT));
    R[8] = FX_CLIP_LIST;
    R[0] = gsu_sub(R[9], 2, 0);
    if (gsu.z) {
        gsu_code(0x019914, 11);
        near_load(1, 2, 3, 0, GSU_COPY(3));
        gsu_code(0x019927, 4);
        near_load(4, 5, 6, 1, GSU_COPY(4));
        R[11] = 0x9937;
        (gsu_code(0x01981A, 1), gsu_cache(0x981B), gsu_code(0x01981B, 5), gsu_code(0x019933, 4), abi_FxSegmentCrossZ());
        gsu_code(0x019937, 3);
        near_put(1, 2, 3, GSU_COPY(3));
        gsu_code(0x019942, 4);
        near_put(4, 5, 6, GSU_COPY(4));
        R[0] = 0x0002;
        gsu_code(0x01994E, 7);
        gsu_stw(FX_COUNT, R[0]);
    } else {
        gsu_code(0x019914, 8);
        do {
            gsu_code(0x019958, 3);
            /* 9958 */
            near_load(1, 2, 3, 0, GSU_COPY(0));
            R[0] = gsu_dec(R[9]);
            if (gsu.z)
                R[7] = gsu_ldw(gsu_code_then(0x01996A, 3, FX_POLY_OUT));
            gsu_code(0x01996E, 2);
            near_load(4, 5, 6, 1, GSU_COPY(1));
            R[7] = gsu_sub(R[7], 4, 0);
            R[0] = gsu_and(R[6], R[3]);
            if (!gsu.s) {
                gsu_code(0x019978, 7); gsu_code(0x019983, 5);
                gsu_moves(R[3]);
                if (!gsu.s) {
                    gsu_code(0x01998C, 3);
                    near_put(1, 2, 3, GSU_COPY(1));
                    gsu_code(0x019997, 5);
                    near_count(GSU_COPY(1));
                    gsu_moves(R[6]);
                    if (gsu.s) {
                        gsu_code(0x01981A, 1); gsu_cache(0x981B); gsu_code(0x01981B, 10); gsu_code(0x019831, 4); gsu_code(0x01999E, 4); gsu_code(0x0199A6, 5);
                        R[11] = 0x99AB;
                        abi_FxSegmentCrossZ();
                        gsu_code(0x01988A, 3); gsu_code(0x0199AB, 3);
                        near_put(4, 5, 6, GSU_COPY(2));
                        gsu_code(0x0199B6, 5);
                        near_count(GSU_COPY(2));
                    } else { gsu_code(0x01999E, 8); }
                    gsu_code(0x0199D6, 1);
                } else {
                    gsu_code(0x01981A, 1); gsu_cache(0x981B); gsu_code(0x01981B, 5); gsu_code(0x019828, 5); gsu_code(0x01988D, 4); gsu_code(0x019988, 4); gsu_code(0x0199C0, 5);
                    R[11] = 0x99C5;
                    abi_FxSegmentCrossZ();
                    gsu_code(0x0198D4, 3); gsu_code(0x0199C5, 3);
                    near_put(1, 2, 3, GSU_COPY(0));
                    gsu_code(0x0199D0, 5);
                    near_count(GSU_COPY(0));
                }
                gsu_code(0x0199D7, 3);
            } else { gsu_code(0x019978, 11); gsu_code(0x0199D6, 4); }
            /* 99D6 */
            R[9] = gsu_dec(R[9]);
        } while (gsu_code_loop(0x0199DA, 4, !gsu.z));
        R[0] = gsu_ldw(gsu_code_then(0x0199DE, 4, FX_COUNT));
        R[0] = gsu_sub(R[0], 3, 0);
        if (gsu.s) {
            gsu_code(0x0199E2, 8); gsu_code(0x019A4C, 6);
            gather_return(0, GSU_COPY(0));
            gsu_code(0x0180E3, 1);
            return;
        } else { gsu_code(0x0199E2, 4); }
    }
    /* 99EA */
    R[3] = gsu_ldw(gsu_code_then(0x0199EA, 4, FX_OFFSET_X));
    R[13] = gsu_ldw(gsu_code_then(0x0199EE, 3, FX_OFFSET_Y));
    R[9] = gsu_ldw(gsu_code_then(0x0199F1, 3, FX_OFFSET_Z));
    R[0] = 0x0014;
    gsu_code(0x0199F4, 8);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(FX_CODES, R[0]);
    gsu_code(0x0199FC, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    R[12] = gsu_ldw(gsu_code_then(0x0199FF, 3, FX_COUNT));
    R[7] = FX_CLIP_LIST;
    R[8] = gsu_ldw(gsu_code_then(0x019A02, 6, FX_POLY_OUT));
    do {
        gsu_code(0x019A08, 2);
        near_load(1, 2, 6, 0, GSU_COPY(2));
        R[11] = 0x9A18;
        (gsu_code(0x018BFF, 7), gsu_code(0x018C0E, 7), gsu_code(0x019A12, 6), abi_FxProjectVertex());
        gsu_code(0x019A18, 3);
        gsu_stw(R[8], R[1]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        gsu_code(0x019A1B, 4);
        gsu_stw(R[8], R[2]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        R[6] = gsu_ldw(gsu_code_then(0x019A1F, 5, FX_CODES));
        R[0] = gsu_or(R[0], R[6]);
        R[12] = gsu_dec(R[12]);
        go = !gsu.z;
        gsu_sbk(R[0]);                          /* BNE's delay slot */
    } while (go);
    R[0] = gsu_ldw(gsu_code_then(0x019A29, 3, FX_ROM_BANK));
    gsu_code(0x019A2C, 5);
    gsu_romb(R[0]);
    gsu_r14(gsu_ldw(FX_ROM_R14));
    R[3] = gsu_ldw(gsu_code_then(0x019A31, 3, FX_POLY_OUT));
    R[0] = gsu_ldw(gsu_code_then(0x019A34, 3, FX_CODES));
    R[1] = 0x0F00;
    R[0] = gsu_and(R[0], R[1]);
    gsu_sub(R[0], R[1], 0);                     /* CMP R1 */
    gsu_code(0x019A37, 8);
    gather_return(gsu.z, GSU_COPY(0));
    gsu_code(0x0191DA, 1);
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t clip_against_from_clip_polygon_edge[20] = {0, 1, 2, 3, 8, 16, 0, 17, 15, 9, 14, 0, 0, 0, 2, 3, 0, 0, 0, 0};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode clip_polygon_edge_code[20][20] = {
    {{0x01A271, 8}, {0x01A27D, 4}, {0x01A281, 9}, {0, 0}, {0x01A281, 5}, {0x01A28A, 4}, {0x01A28E, 3}, {0x01A291, 3}, {0x01A294, 2}, {0x01A342, 2}, {0x01A348, 4}, {0x01A34C, 3}, {0x01A34F, 3}, {0x01A352, 3}, {0x01A355, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A358, 4}, {0x01A360, 2}},
    {{0x01A362, 4}, {0, 0}, {0x01A366, 9}, {0, 0}, {0x01A366, 5}, {0x01A36F, 4}, {0x01A373, 3}, {0x01A376, 3}, {0x01A379, 2}, {0x01A42D, 2}, {0x01A433, 4}, {0x01A437, 3}, {0x01A43A, 3}, {0x01A43D, 3}, {0x01A440, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A443, 4}, {0x01A44B, 2}},
    {{0x01A44D, 4}, {0, 0}, {0x01A451, 9}, {0, 0}, {0x01A451, 5}, {0x01A45A, 4}, {0x01A45E, 3}, {0x01A461, 3}, {0x01A464, 2}, {0x01A51E, 2}, {0x01A524, 4}, {0x01A528, 3}, {0x01A52B, 3}, {0x01A52E, 3}, {0x01A531, 3}, {0x01A534, 8}, {0x01A65C, 6}, {0, 0}, {0x01A534, 4}, {0x01A53C, 2}},
    {{0x01A53E, 4}, {0, 0}, {0x01A542, 9}, {0x01A635, 4}, {0x01A542, 5}, {0x01A54B, 4}, {0x01A54F, 3}, {0x01A552, 3}, {0x01A555, 2}, {0x01A615, 2}, {0x01A61B, 4}, {0x01A61F, 3}, {0x01A622, 3}, {0x01A625, 3}, {0x01A628, 3}, {0x01A62B, 8}, {0x01A65C, 6}, {0, 0}, {0x01A62B, 4}, {0x01A633, 6}},
    {{0x01A271, 8}, {0x01A27D, 4}, {0x01A281, 9}, {0, 0}, {0x01A281, 5}, {0x01A28A, 4}, {0x01A28E, 3}, {0x01A291, 3}, {0x01A294, 2}, {0x01A342, 2}, {0x01A348, 4}, {0x01A34C, 3}, {0x01A34F, 3}, {0x01A352, 3}, {0x01A355, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A358, 4}, {0x01A360, 2}},
    {{0x01A362, 4}, {0, 0}, {0x01A366, 9}, {0, 0}, {0x01A366, 5}, {0x01A36F, 4}, {0x01A373, 3}, {0x01A376, 3}, {0x01A379, 2}, {0x01A42D, 2}, {0x01A433, 4}, {0x01A437, 3}, {0x01A43A, 3}, {0x01A43D, 3}, {0x01A440, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A443, 4}, {0x01A44B, 2}},
    {{0x01A44D, 4}, {0, 0}, {0x01A451, 9}, {0x01A53E, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x01A542, 9}, {0x01A635, 4}, {0x01A542, 5}, {0x01A54B, 4}, {0x01A54F, 3}, {0x01A552, 3}, {0x01A555, 2}, {0x01A615, 2}, {0x01A61B, 4}, {0x01A61F, 3}, {0x01A622, 3}, {0x01A625, 3}, {0x01A628, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A62B, 4}, {0x01A633, 6}},
    {{0x01A271, 8}, {0x01A27D, 4}, {0x01A281, 9}, {0, 0}, {0x01A281, 5}, {0x01A28A, 4}, {0x01A28E, 3}, {0x01A291, 3}, {0x01A294, 2}, {0x01A342, 2}, {0x01A348, 4}, {0x01A34C, 3}, {0x01A34F, 3}, {0x01A352, 3}, {0x01A355, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A358, 4}, {0x01A360, 2}},
    {{0x01A362, 4}, {0, 0}, {0x01A366, 9}, {0, 0}, {0x01A366, 5}, {0x01A36F, 4}, {0x01A373, 3}, {0x01A376, 3}, {0x01A379, 2}, {0x01A42D, 2}, {0x01A433, 4}, {0x01A437, 3}, {0x01A43A, 3}, {0x01A43D, 3}, {0x01A440, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A443, 4}, {0x01A44B, 2}},
    {{0, 0}, {0, 0}, {0x01A542, 9}, {0x01A635, 4}, {0x01A542, 5}, {0x01A54B, 4}, {0x01A54F, 3}, {0x01A552, 3}, {0x01A555, 2}, {0x01A615, 2}, {0x01A61B, 4}, {0x01A61F, 3}, {0x01A622, 3}, {0x01A625, 3}, {0x01A628, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A62B, 4}, {0x01A633, 6}},
    {{0x01A271, 8}, {0x01A27D, 4}, {0x01A281, 9}, {0, 0}, {0x01A281, 5}, {0x01A28A, 4}, {0x01A28E, 3}, {0x01A291, 3}, {0x01A294, 2}, {0x01A342, 2}, {0x01A348, 4}, {0x01A34C, 3}, {0x01A34F, 3}, {0x01A352, 3}, {0x01A355, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A358, 4}, {0x01A360, 2}},
    {{0x01A44D, 4}, {0, 0}, {0x01A451, 9}, {0, 0}, {0x01A451, 5}, {0x01A45A, 4}, {0x01A45E, 3}, {0x01A461, 3}, {0x01A464, 2}, {0x01A524, 4}, {0, 0}, {0x01A528, 3}, {0x01A52B, 3}, {0x01A52E, 3}, {0x01A531, 3}, {0x01A534, 8}, {0x01A65C, 6}, {0, 0}, {0x01A534, 4}, {0x01A53C, 2}},
    {{0x01A53E, 4}, {0, 0}, {0x01A542, 9}, {0x01A635, 4}, {0x01A542, 5}, {0x01A54B, 4}, {0x01A54F, 3}, {0x01A552, 3}, {0x01A555, 2}, {0x01A615, 2}, {0x01A61B, 4}, {0x01A61F, 3}, {0x01A622, 3}, {0x01A625, 3}, {0x01A628, 3}, {0x01A62B, 8}, {0x01A65C, 6}, {0, 0}, {0x01A62B, 4}, {0x01A633, 6}},
    {{0x01A44D, 4}, {0, 0}, {0x01A451, 9}, {0, 0}, {0x01A451, 5}, {0x01A45A, 4}, {0x01A45E, 3}, {0x01A461, 3}, {0x01A464, 2}, {0x01A51E, 2}, {0x01A524, 4}, {0x01A528, 3}, {0x01A52B, 3}, {0x01A52E, 3}, {0x01A531, 3}, {0x01A534, 8}, {0x01A65C, 6}, {0, 0}, {0x01A534, 4}, {0x01A53C, 2}},
    {{0x01A53E, 4}, {0, 0}, {0x01A542, 9}, {0x01A635, 4}, {0x01A542, 5}, {0x01A54B, 4}, {0x01A54F, 3}, {0x01A552, 3}, {0x01A555, 2}, {0x01A615, 2}, {0x01A61B, 4}, {0x01A61F, 3}, {0x01A622, 3}, {0x01A625, 3}, {0x01A628, 3}, {0x01A62B, 8}, {0x01A65C, 6}, {0, 0}, {0x01A62B, 4}, {0x01A633, 6}},
    {{0x01A271, 8}, {0x01A27D, 4}, {0x01A281, 9}, {0x01A362, 4}, {0x01A281, 5}, {0x01A28A, 4}, {0x01A28E, 3}, {0x01A291, 3}, {0x01A294, 2}, {0x01A342, 2}, {0x01A348, 4}, {0x01A34C, 3}, {0x01A34F, 3}, {0x01A352, 3}, {0x01A355, 3}, {0x01A358, 8}, {0x01A65C, 6}, {0x0180E3, 1}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A366, 5}, {0x01A36F, 4}, {0x01A373, 3}, {0x01A376, 3}, {0x01A379, 2}, {0x01A42D, 2}, {0x01A433, 4}, {0x01A437, 3}, {0x01A43A, 3}, {0x01A43D, 3}, {0x01A440, 3}, {0x01A443, 8}, {0x01A65C, 6}, {0x0180E3, 1}, {0x01A443, 4}, {0x01A44B, 6}},
    {{0, 0}, {0, 0}, {0x01A451, 9}, {0x01A53E, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x01A542, 5}, {0x01A54B, 4}, {0x01A54F, 3}, {0x01A552, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A61F, 3}, {0x01A622, 3}, {0x01A625, 3}, {0x01A628, 3}, {0, 0}, {0, 0}, {0, 0}, {0x01A62B, 4}, {0x01A633, 6}},
};

/*
 * @routine 01:A25F FxPolygonClipScreen
 * WHAT IT DOES: Clips the projected polygon at the word at $005A (R0
 * vertices, x and y words each) against the edges of the screen ($38 and
 * $3A in x, $3C and $3E in y) that its outcodes at $014A cross (bit 2
 * left, 3 right, 0 top, 1 bottom), leaving the result at $005A again; a
 * line (fewer than 3 points) is clipped against all four (9F01). R0 =
 * the count left, 0 when fewer than 3 (2 for a line) are.
 * HOW: $0060 = $005A, $0062 = $0688, the count at $0098; pushes R11 on
 * the R10 stack. For each edge clipped: the count to $0040, the first
 * vertex copied after the last, then for each edge of the polygon the
 * start written when inside, the crossing (LMULT, FxDivideSigned) when
 * it crosses, $0098 counted up or down; the lists swapped; fewer than 3
 * left goes to A65C (R0 = 0). Then, unless the list read last is $005A's,
 * the vertices are copied back there (LOOP) and R0 = $0098. Returns by
 * pulling R15. A line goes from $005A to $0688 and back twice instead,
 * with the bottom edge's crossings one row inside it, and ends with R0 =
 * 2. The incoming and outgoing crossings are the same code twice, with
 * their own LINK.
 * Entry: R0 = the vertex count; R10 = stack; R11 = return address. Exit:
 * R0 as above; R11 = the last LINK's return; R4-R6 from the last division
 * (when any); R8, R9 past the last vertices read and written; R12 = 0 and
 * R13 the copy's LOOP when it ran; S and Z from R10, the last RAM address
 * R10. The rest as the last step leaves them.
 */
/* One edge of FxPolygonClipScreen (the original has each edge in its own
   code), skipped when no vertex is outside it: 0 when nothing is left (R15
   pulled). */
static int clip_polygon_edge(int k, int copy)
{
    gsu_codes(clip_polygon_edge_code[copy], 2);
    static const uint16_t bits[4] = {4, 8, 1, 2};

    R[1] = gsu_ldw(FX_CODES);
    R[1] = gsu_and(R[1], bits[k]);
    if (gsu.z)
        { gsu_codes(clip_polygon_edge_code[copy] + 2, 2); return 1; } else { gsu_codes(clip_polygon_edge_code[copy] + 4, 2); }
    R[8] = gsu_ldw(FX_CLIP_IN);
    R[9] = gsu_ldw(gsu_codes_then(clip_polygon_edge_code[copy] + 6, 1, FX_CLIP_OUT));
    gsu_codes(clip_polygon_edge_code[copy] + 7, 1);
    gsu_stw(FX_COUNT, R[0]);
    gsu_codes(clip_polygon_edge_code[copy] + 8, 1);
    clip_against(&polygon_edges[k], GSU_COPY(clip_against_from_clip_polygon_edge[copy]));
    R[1] = gsu_ldw(gsu_codes_then(clip_polygon_edge_code[copy] + 9, 2, FX_CLIP_IN));
    R[2] = gsu_ldw(gsu_codes_then(clip_polygon_edge_code[copy] + 11, 1, FX_CLIP_OUT));
    gsu_codes(clip_polygon_edge_code[copy] + 12, 1);
    gsu_stw(FX_CLIP_IN, R[2]);
    gsu_codes(clip_polygon_edge_code[copy] + 13, 1);
    gsu_stw(FX_CLIP_OUT, R[1]);
    R[0] = gsu_ldw(gsu_codes_then(clip_polygon_edge_code[copy] + 14, 1, FX_COUNT_OUT));
    R[0] = gsu_sub(R[0], 3, 0);
    if (gsu.s) {
        gsu_codes(clip_polygon_edge_code[copy] + 15, 2);
        clip_none();
        gsu_codes(clip_polygon_edge_code[copy] + 17, 1);
        return 0;
    } else { gsu_codes(clip_polygon_edge_code[copy] + 18, 2); }
    R[0] = gsu_add(R[0], 3, 0);
    return 1;
}

void abi_FxPolygonClipScreen(void)
{
    gsu_code(0x01A260, 3);
    R[8] = gsu_ldw(FX_POLY_OUT);
    R[9] = FX_CLIP_LIST;
    gsu_code(0x01A263, 6);
    gsu_stw(FX_CLIP_IN, R[8]);
    gsu_code(0x01A269, 3);
    gsu_stw(FX_CLIP_OUT, R[9]);
    gsu_code(0x01A26C, 3);
    gsu_stw(FX_COUNT_OUT, R[0]);
    gsu_code(0x01A26F, 2);
    gsu_push(&R[10], R[11]);
    R[1] = 0x0003;
    gsu_sub(R[0], R[1], 0);                     /* CMP R1 */
    if (gsu.s) {
        clip_line(GSU_COPY(0));
        gsu_code(0x0180E3, 1);
        return;
    } else { gsu_code(0x01A271, 8); gsu_code(0x01A27D, 4); }
    if (!clip_polygon_edge(0, GSU_COPY(0)) || (gsu_code(0x01A362, 4), !clip_polygon_edge(1, GSU_COPY(1))) || (gsu_code(0x01A44D, 4), !clip_polygon_edge(2, GSU_COPY(2))) || (gsu_code(0x01A451, 9), gsu_code(0x01A53E, 4), !clip_polygon_edge(3, GSU_COPY(3))))
        return; else { gsu_code(0x01A635, 4); }
    /* A635 */
    R[1] = gsu_ldw(FX_POLY_OUT);
    R[8] = gsu_ldw(gsu_code_then(0x01A639, 3, FX_CLIP_IN));
    R[1] = gsu_sub(R[1], R[8], 0);
    if (!gsu.z) {
        gsu_code(0x01A63C, 8);
        int go;

        R[9] = gsu_ldw(FX_CLIP_OUT);
        R[12] = R[0];
        R[13] = 0xA647;
        gsu_code(0x01A644, 3);
        do {
            gsu_code(0x01A647, 2);
            R[0] = gsu_ldw(R[8]);
            gsu_code(0x01A649, 1);
            gsu_stw(R[9], R[0]);
            R[9] = gsu_inc(R[9]);
            R[9] = gsu_inc(R[9]);
            R[8] = gsu_inc(R[8]);
            R[8] = gsu_inc(R[8]);
            R[0] = gsu_ldw(gsu_code_then(0x01A64A, 5, R[8]));
            gsu_code(0x01A64F, 1);
            gsu_stw(R[9], R[0]);
            R[9] = gsu_inc(R[9]);
            R[9] = gsu_inc(R[9]);
            R[8] = gsu_inc(R[8]);
            go = gsu_count_down(&R[12]);
            R[8] = gsu_inc(R[8]);
        } while (gsu_code_loop(0x01A650, 4, go));
        R[0] = gsu_ldw(gsu_code_then(0x01A650, 8, FX_COUNT_OUT));
    } else { gsu_code(0x01A63C, 4); gsu_code(0x01A657, 1); }
    gsu_code(0x01A658, 4);
    gsu_pull(&R[10]);
    gsu_code(0x0191FA, 1);
}

/*
 * @routine 01:A931 FxDrawList
 * WHAT IT DOES: Draws the frame's queued shapes, the list at $08AA up to
 * the word at $005C: flat polygons, lines and textured polygons (see the
 * file's head), each into the Super FX's screen with PLOT; then empties
 * the list ($005C = $08AA). An empty list returns at once.
 * HOW: R0 = $08AA - $005C; equal, JMP R11. Else pushes R11 on the R10
 * stack and keeps R10 at $005E, writes a 0 word (the end) at the list's
 * end, $005C = $08AA, ROMB 0. For each entry (A950): the type byte (LDB,
 * SEX), less 3; BMI, with INC R0 in its delay slot, to the others
 * (A90E): Z there means a line (AAA2); INC twice gives the type, Z ends
 * the list (A91E: $005C = $08AA, ROMB from $001C, R10 from $005E, pulls
 * R15); else NOT (the delay slot) and a textured polygon (A662). A flat
 * polygon (A95C) CACHEs, COLOR $70 then CMODE 0 and COLOR its colour,
 * copies its vertices (finding the top and bottom rows) and, less than
 * two rows high, draws one row (AA74); else walks its edges (A9C1-AA6F).
 * Each shape leaves $005C at the next entry, and each jumps back to A950.
 * Entry: R10 = stack; R11 = return address. Exit: an empty list: R0 = 0,
 * R1 = $08AA, flags from the SUB. Else R1 = $08AA, R0 = the ROM bank
 * from $001C, R10 = $005E - 2, the last RAM address R10, S and Z from
 * DEC R10; the rest as the last shape drawn leaves them. R14, the ROM
 * buffer, the colour and plot options move as on the Super FX.
 */
void abi_FxDrawList(void)
{
    gsu_code(0x01A932, 3);
    int minus;

    R[1] = gsu_ldw(FX_LIST_END);
    R[0] = DRAW_LIST;
    R[0] = gsu_sub(R[0], R[1], 0);
    if (gsu.z)
        { gsu_code(0x019248, 1); gsu_code(0x01A935, 8); return; } else { gsu_code(0x01A935, 6); gsu_code(0x01A93D, 3); }
    gsu_push(&R[10], R[11]);
    gsu_code(0x01A940, 5);
    gsu_stw(FX_LIST_STACK, R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01A945, 2);
    gsu_stw(R[1], R[0]);
    R[1] = DRAW_LIST;
    gsu_code(0x01A947, 6);
    gsu_stw(FX_LIST_END, R[1]);
    R[0] = 0;
    gsu_code(0x01A94D, 3);
    gsu_romb(R[0]);
    for (;;) {
        gsu_code(0x01A950, 4);
        /* A950 */
        R[1] = gsu_ldw(FX_LIST_END);
        R[0] = gsu_ldb(gsu_code_then(0x01A954, 2, R[1]));
        R[0] = gsu_sex(R[0]);
        R[1] = gsu_inc(R[1]);
        R[0] = gsu_sub(R[0], 3, 0);
        minus = gsu.s;
        R[0] = gsu_inc(R[0]);                   /* BMI's delay slot */
        if (!minus) {
            flat_polygon(GSU_COPY(0));
            continue;
        } else { gsu_code(0x01A90E, 3); gsu_code(0x01A956, 6); }
        /* A90E */
        if (gsu.z) {
            line(GSU_COPY(0));
            continue;
        } else { gsu_code(0x01A915, 5); }
        R[0] = gsu_inc(R[0]);
        R[0] = gsu_inc(R[0]);
        {
            int end = gsu.z;

            R[0] = gsu_not(R[0]);               /* BEQ's delay slot */
            if (!end) {
                tex_polygon(GSU_COPY(0));
                continue;
            } else { gsu_code(0x01A91E, 7); }
        }
        /* A91E */
        R[1] = DRAW_LIST;
        gsu_stw(FX_LIST_END, R[1]);
        R[0] = gsu_ldw(gsu_code_then(0x01A925, 3, FX_ROM_BANK));
        gsu_code(0x01A928, 5);
        gsu_romb(R[0]);
        R[10] = gsu_ldw(FX_LIST_STACK);
        gsu_code(0x01A92D, 4);
        gsu_pull(&R[10]);
        gsu_code(0x0180E3, 1);
        return;
    }
}
