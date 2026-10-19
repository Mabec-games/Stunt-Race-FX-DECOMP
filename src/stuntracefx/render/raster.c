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
/* gsuwalk: the code of each copy of reciprocal_at, read from the ROM */
static const GsuCode reciprocal_at_fetch[5][1] = {
    {{0x01a9e4, 7}},
    {{0x01aa23, 7}},
    {{0x01a82b, 6}},
    {{0x01a8c0, 6}},
    {{0x01a743, 6}},
};

static void reciprocal_at(uint16_t n, int copy)
{
    R[0] = RECIPROCALS;
    R[0] = gsu_add(R[0], n, 0);
    gsu_codes(reciprocal_at_fetch[copy], 1);
    gsu_r14(gsu_add(R[0], n, 0));
}

/* gsuwalk: the code of each copy of reciprocal_read, read from the ROM */
static const GsuCode reciprocal_read_fetch[5][2] = {
    {{0x01a9ee, 4}, {0x01a9f2, 1}},
    {{0x01aa2a, 4}, {0x01aa2e, 1}},
    {{0x01a831, 2}, {0x01a833, 1}},
    {{0x01a8c6, 2}, {0x01a8c8, 1}},
    {{0x01a749, 2}, {0x01a74b, 1}},
};

static void reciprocal_read(int copy)
{
    gsu_codes(reciprocal_read_fetch[copy], 1);
    R[6] = gsu_getb();
    gsu_codes(reciprocal_read_fetch[copy] + 1, 1);
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

/* WITH x; ADD step; BPL; DEC rows (the delay slot); IWT R0, #$0800;
   ADD x; BMI; NOP; IBT x, #0: an edge's x stepped, set to 0 when it has
   gone just below it (to -8.0). */
/* gsuwalk: the code of each copy of edge_step, read from the ROM */
static const GsuCode edge_step_fetch[4][7] = {
    {{0x01aa4c, 4}, {0x01aa4c, 4}, {0x01aa59, 1}, {0x01aa50, 7}, {0x01aa50, 7}, {0x01aa59, 1}, {0x01aa57, 3}},
    {{0x01aa5a, 4}, {0x01aa5a, 4}, {0x01aa67, 1}, {0x01aa5e, 7}, {0x01aa5e, 7}, {0x01aa67, 1}, {0x01aa65, 3}},
    {{0x01a7a9, 4}, {0x01a7a9, 4}, {0x01a7b6, 1}, {0x01a7ad, 7}, {0x01a7ad, 7}, {0x01a7b6, 1}, {0x01a7b4, 3}},
    {{0x01a7b7, 4}, {0x01a7b7, 4}, {0x01a7c4, 1}, {0x01a7bb, 7}, {0x01a7bb, 7}, {0x01a7c4, 1}, {0x01a7c2, 3}},
};

static void edge_step(int x, uint16_t step, int rows, int copy)
{
    int plus;

    R[x] = gsu_add(R[x], step, 0);
    plus = !gsu.s;
    if (rows >= 0)
        R[rows] = gsu_dec(R[rows]);
    if (plus)
        { gsu_codes(edge_step_fetch[copy] + 1, 2); return; } else { gsu_codes(edge_step_fetch[copy], 1); }
    R[0] = gsu_add(0x0800, R[x], 0);
    if (gsu.s)
        { gsu_codes(edge_step_fetch[copy] + 4, 2); return; } else { gsu_codes(edge_step_fetch[copy] + 3, 1); }
    R[x] = 0;
    gsu_codes(edge_step_fetch[copy] + 6, 1);
}

/* A difference of u or v in R0 to a step: LOB; SWAP; FMULT; ROL. */
static uint16_t uv_step(void)
{
    R[0] = gsu_lob(R[0]);
    R[0] = gsu_swap(R[0]);
    R[0] = gsu_fmult(R[0], R[6]);
    return gsu_rol(R[0]);
}

/* The vertices of an entry copied once more below it and the top and
   bottom rows found: R5 walks the n vertices from R1 (STRIDE bytes each),
   R4 writes x and y STRIDE * n bytes lower; R2 = the smallest y (from
   $03FF) and R6 where its copy went, R8 the largest (from 0). A LOOP of
   R12 = n (R13 = its start). */
static void flat_copy(int copy)
{
    int plus;

    R[13] = 0xA981;
    gsu_code(0x01a976, 12);
    do {
        gsu_code(0x01a982, 2);
        R[0] = gsu_ldb(R[5]);
        gsu_code(0x01a984, 2);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_code(0x01a986, 6);
        R[0] = gsu_ldb(R[5]);
        gsu_code(0x01a98c, 2);
        gsu_stb(R[4], (uint8_t)R[0]);
        gsu_sub(R[0], R[2], 0);                 /* CMP R2 */
        plus = !gsu.s;
        R[5] = gsu_inc(R[5]);                   /* BPL's delay slot */
        if (!plus) {
            R[2] = R[0];
            R[6] = R[4];
            gsu_code(0x01a98e, 9);
        } else { gsu_code(0x01a98e, 4); gsu_code(0x01a996, 1); }
        R[0] = gsu_sub(R[0], R[8], 0);
        plus = !gsu.s;
        R[5] = gsu_inc(R[5]);                   /* BMI's delay slot */
        if (plus)
            { R[8] = gsu_add(R[0], R[8], 0); gsu_code(0x01a997, 6); } else { gsu_code(0x01a997, 3); gsu_code(0x01a99c, 1); }
        R[4] = gsu_inc(R[4]);
        plus = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);                   /* LOOP's delay slot */
    } while (gsu_code_loop(0x01a981, 1, gsu_code_loop(0x01a99d, 2, plus)));
    gsu_code(0x01a99d, 2);
}

/* AA74: a polygon less than two rows high, drawn as one row at the top
   y (R2) from the smallest x to the largest (not including it). R4 is
   at the vertices again after the copy. Back to the list after. */
static void flat_thin(int copy)
{
    int go;

    R[0] = gsu_lsr(R[3]);
    R[12] = gsu_lsr(R[0]);
    R[1] = 0x03FF;
    R[8] = 0;
    R[13] = 0xAA7F;
    gsu_code(0x01a9a7, 3); gsu_code(0x01aa74, 12);
    do {
        gsu_code(0x01aa80, 2);
        R[0] = gsu_ldb(R[4]);
        gsu_sub(R[0], R[1], 0);                 /* CMP R1 */
        go = !gsu.s;
        R[4] = gsu_inc(R[4]);
        if (!go)
            { R[1] = R[0]; gsu_code(0x01aa82, 7); } else { gsu_code(0x01aa82, 4); gsu_code(0x01aa88, 1); }
        R[0] = gsu_sub(R[0], R[8], 0);
        go = !gsu.s;
        R[4] = gsu_inc(R[4]);
        if (go)
            { R[8] = gsu_add(R[0], R[8], 0); gsu_code(0x01aa89, 6); } else { gsu_code(0x01aa89, 3); gsu_code(0x01aa8e, 1); }
        R[4] = gsu_inc(R[4]);
        go = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);
    } while (gsu_code_loop(0x01aa7f, 1, gsu_code_loop(0x01aa8f, 2, go)));
    gsu_code(0x01aa8f, 2);
    R[12] = gsu_sub(R[8], R[1], 0);
    if (gsu.s)                                  /* BMI, then BEQ: each its own way back */
        { gsu_code(0x01aa91, 6); gsu_code(0x01aa9e, 1); return; } else { gsu_code(0x01aa91, 6); }
    if (gsu.z)
        { gsu_code(0x01aa97, 3); gsu_code(0x01aa9e, 1); return; } else { gsu_code(0x01aa97, 3); }
    R[13] = 0xAA9C;
    gsu_code(0x01aa9a, 3);
    do {
        go = gsu_count_down(&R[12]);
        /* the branch's delay slot */
        /* the branch's delay slot */
        gsu_code(0x01aa9d, 1); gsu_code((go) ? 0x01aa9c : 0x01aa9e, 1);   /* the branch's delay slot */
        gsu_plot_r1();
    } while (go);
}

/* A9D0 (from A9CE): the next left edge. R8 = the current x, R7 = the
   next vertex's x, its y less the row R2 in R11; edges with no height are
   passed. Returns 0 when the polygon has ended (the next vertex is above
   the row), else 1 with R7 = x in 8.8 and R8 its slope. */
/* gsuwalk: the code of each copy of flat_left_edge, read from the ROM */
static const GsuCode flat_left_edge_fetch[2][14] = {
    {{0x01a9cf, 2}, {0x01a9b5, 12}, {0x01a9d0, 1}, {0, 0}, {0, 0}, {0x01a9d1, 3}, {0x01a9d4, 4}, {0x01a9d8, 5}, {0x01a9e1, 1}, {0x01a9e2, 2}, {0x01a9ce, 1}, {0x01a9e2, 2}, {0x01a9eb, 3}, {0x01a9f3, 6}},
    {{0, 0}, {0, 0}, {0, 0}, {0x01a9cf, 2}, {0x01a9cf, 2}, {0x01a9d1, 3}, {0x01a9d4, 4}, {0x01a9d8, 5}, {0x01a9e1, 1}, {0x01a9e2, 2}, {0x01a9ce, 1}, {0x01a9e2, 2}, {0x01a9eb, 3}, {0x01a9f3, 6}},
};

static int flat_left_edge(int from_a9ce, int copy)
{
    gsu_codes(flat_left_edge_fetch[copy] + 1, 2);
    for (;;) {
        if (from_a9ce)
            { R[8] = R[7]; gsu_codes(flat_left_edge_fetch[copy] + 4, 1); } else { gsu_codes(flat_left_edge_fetch[copy] + 3, 1); }
        from_a9ce = 1;
        gsu_codes(flat_left_edge_fetch[copy] + 5, 1);
        R[7] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        gsu_codes(flat_left_edge_fetch[copy] + 6, 1);
        R[0] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[11] = gsu_sub(R[0], R[2], 0);
        if (gsu.s)
            return 0; else { gsu_codes(flat_left_edge_fetch[copy] + 7, 2); }
        {
            int zero = gsu.z;

            R[6] = gsu_inc(R[6]);               /* BEQ's delay slot */
            if (!zero)
                { gsu_codes(flat_left_edge_fetch[copy] + 11, 1); break; } else { gsu_codes(flat_left_edge_fetch[copy] + 9, 2); }
        }
        gsu_codes(flat_left_edge_fetch[copy], 1);
    }
    reciprocal_at(R[11], GSU_COPY(0));
    gsu_codes(flat_left_edge_fetch[copy] + 12, 1);
    gsu_stw(FX_LEFT_AT, R[6]);
    R[0] = gsu_sub(R[7], R[8], 0);
    reciprocal_read(GSU_COPY(0));
    R[7] = gsu_swap(R[8]);
    gsu_codes(flat_left_edge_fetch[copy] + 13, 1);
    R[6] = gsu_getbh(R[6]);
    R[8] = slope();
    return 1;
}

/* AA0D: the next right edge, walking back from R5: R10 = the current x,
   R9 = the next x, its y less R2 in R3. 0 when the polygon has ended. */
static int flat_right_edge(int copy)
{
    gsu_code(0x01aa09, 5);
    for (;;) {
        R[10] = R[9];
        gsu_code(0x01aa0e, 4);
        R[0] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        R[5] = gsu_dec(R[5]);
        gsu_code(0x01aa12, 5);
        R[9] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        R[3] = gsu_sub(R[0], R[2], 0);
        if (gsu.s)
            { return 0; } else { gsu_code(0x01aa17, 5); gsu_code(0x01aa20, 1); }
        {
            int zero = gsu.z;

            R[5] = gsu_dec(R[5]);               /* BEQ's delay slot */
            if (!zero)
                { gsu_code(0x01aa21, 2); break; } else { gsu_code(0x01aa21, 2); gsu_code(0x01aa0d, 1); }
        }
    }
    reciprocal_at(R[3], GSU_COPY(1));
    R[0] = gsu_sub(R[9], R[10], 0);
    reciprocal_read(GSU_COPY(1));
    R[9] = gsu_swap(R[10]);
    gsu_code(0x01aa2f, 6);
    R[6] = gsu_getbh(R[6]);
    R[10] = (slope());
    return 1;
}

/* A95C: a flat polygon (R0 = its type less 2, R1 at its colour byte):
   the vertices copied, then rows from the top down, each from the left
   edge's x to the right's, both rounded (+ $7F) when an edge starts. */
static void flat_polygon(int copy)
{
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
    gsu_code(0x01a95c, 1); gsu_cache(0xa95d); gsu_code(0x01a95d, 18);
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_add(R[1], R[3], 0);
    gsu_code(0x01a96f, 7);
    gsu_stw(FX_LIST_END, R[0]);
    R[5] = R[1];
    R[4] = gsu_sub(R[1], R[3], 0);
    R[2] = 0x03FF;
    R[8] = 0;
    flat_copy(GSU_COPY(0));
    R[0] = gsu_sub(R[8], R[2], 0);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_dec(R[0]);
    if (gsu.s) {
        gsu_code(0x01a99f, 8);
        flat_thin(GSU_COPY(0));
        gsu_code(0x01aa9f, 3); gsu_code(0x01a950, 1);
        return;
    } else { gsu_code(0x01a99f, 7); gsu_code(0x01a9aa, 1); }
    R[6] = gsu_dec(R[6]);
    R[6] = gsu_dec(R[6]);
    R[5] = gsu_add(R[6], R[3], 0);
    R[5] = gsu_dec(R[5]);
    R[5] = gsu_dec(R[5]);
    gsu_code(0x01a9ab, 10);
    R[8] = gsu_ldb(R[6]);
    R[6] = gsu_add(R[6], 4, 0);
    R[9] = gsu_swap(R[8]);
    R[13] = 0xAA49;
    R[3] = gsu_sub(R[3], R[3], 0);              /* BRA's delay slot */
    if (!flat_left_edge(0, GSU_COPY(0)))
        { gsu_code(0x01a9d8, 9); gsu_code(0x01aa70, 4); gsu_code(0x01a950, 1); return; } else { gsu_code(0x01a9f9, 2); }
    gsu_code(0x01a9fb, 9);
    for (;;) {
        /* AA03 */
        gsu_moves(R[3]);
        if (gsu.z) {
            R[0] = gsu_add(0x007F, R[9], 0);
            R[9] = gsu_hib(R[0]);
            gsu_code(0x01aa04, 5);
            if (!flat_right_edge(GSU_COPY(0)))
                { gsu_code(0x01aa17, 9); gsu_code(0x01aa70, 4); gsu_code(0x01a950, 1); return; } else { gsu_code(0x01aa35, 2); }
            gsu_code(0x01aa37, 9);
        } else { gsu_code(0x01aa04, 4); gsu_code(0x01aa3f, 1); }
        /* AA3F: the rows while both edges last */
        do {
            R[1] = gsu_hib(R[7]);
            R[0] = gsu_hib(R[9]);
            R[12] = gsu_sub(R[0], R[1], 0);
            go = !gsu.s;
            R[12] = gsu_inc(R[12]);             /* BMI's delay slot */
            if (go) {
                gsu_code(0x01aa40, 10);
                do {
                    go = gsu_count_down(&R[12]);
                    /* the PLOT in LOOP's delay slot runs after the fetch
                       from where LOOP goes */
                    /* the branch's delay slot */
                    /* the branch's delay slot */
                    gsu_code(0x01aa4a, 1); gsu_code((go) ? 0x01aa49 : 0x01aa4b, 1);   /* the branch's delay slot */
                    gsu_plot_r1();
                } while (go);
            } else { gsu_code(0x01aa40, 9); gsu_code(0x01aa4b, 1); }
            edge_step(7, R[8], 11, GSU_COPY(0));
            edge_step(9, R[10], 3, GSU_COPY(1));
            R[0] = gsu_and(R[11], R[3]);
            go = !gsu.z;
            R[2] = gsu_inc(R[2]);               /* BNE's delay slot */
        } while (gsu_code_loop(0x01aa3f, 1, gsu_code_loop(0x01aa68, 4, go)));
        gsu_code(0x01aa68, 4);
        /* A9C1 */
        gsu_moves(R[11]);
        if (!gsu.z)
            { gsu_code(0x01aa6c, 4); gsu_code(0x01a9c1, 5); gsu_code(0x01aa03, 1); continue; } else { gsu_code(0x01aa6c, 4); gsu_code(0x01a9c1, 5); }
        R[0] = gsu_add(0x007F, R[7], 0);
        R[7] = gsu_hib(R[0]);
        gsu_code(0x01a9c6, 9);
        R[6] = gsu_ldw(FX_LEFT_AT);
        if (!flat_left_edge(1, GSU_COPY(1)))
            { gsu_code(0x01a9d8, 9); gsu_code(0x01aa70, 4); gsu_code(0x01a950, 1); return; } else { gsu_code(0x01a9f9, 2); }
        gsu_code(0x01a9fb, 9);
    }
}

/* AAA2: a line (R1 at its colour byte), stepped along its longer axis;
   R0 is the error term, the carry from its SUB picks the step across. */
static void line(int copy)
{
    int carry, go;

    R[0] = 0x0070;
    gsu_color((uint8_t)R[0]);
    R[0] = 0;
    gsu_cmode(R[0]);
    R[7] = R[1];
    gsu_code(0x01a912, 3); gsu_code(0x01aaa2, 12);
    R[0] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_color((uint8_t)R[0]);
    gsu_code(0x01aaae, 5);
    R[1] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x01aab3, 5);
    R[2] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x01aab8, 5);
    R[3] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x01aabd, 5);
    R[4] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_code(0x01aac2, 5);
    gsu_stw(FX_LIST_END, R[7]);
    R[5] = 0x0001;
    R[3] = gsu_sub(R[3], R[1], 0);
    if (gsu.s) {
        R[5] = 0xFFFF;
        R[3] = gsu_not(R[3]);
        R[3] = gsu_inc(R[3]);
        gsu_code(0x01aac7, 12);
    } else { gsu_code(0x01aac7, 6); gsu_code(0x01aad2, 1); }
    R[6] = 0x0001;
    R[4] = gsu_sub(R[4], R[2], 0);
    if (gsu.s) {
        R[6] = 0xFFFF;
        R[4] = gsu_not(R[4]);
        R[4] = gsu_inc(R[4]);
        gsu_code(0x01aad3, 12);
    } else { gsu_code(0x01aad3, 6); gsu_code(0x01aade, 1); }
    R[0] = gsu_sub(R[3], R[4], 0);
    if (!gsu.s) {
        R[12] = R[3];
        R[12] = gsu_inc(R[12]);
        R[0] = gsu_lsr(R[0]);
        R[0] = gsu_sub(R[0], R[4], 0);
        R[13] = 0xAAEA;
        gsu_code(0x01aadf, 12);
        do {
            carry = gsu.cy;
            /* the branch's delay slot */
            /* the branch's delay slot */
            gsu_code(0x01aaeb, 2); gsu_code((carry) ? 0x01aaf0 : 0x01aaed, 1);   /* the branch's delay slot */
            gsu_plot_r1();                      /* BCS's delay slot */
            if (!carry) {
                R[0] = gsu_add(R[0], R[3], 0);
                R[2] = gsu_add(R[2], R[6], 0);
                gsu_code(0x01aaee, 3);
            }
            R[1] = gsu_dec(R[1]);
            R[1] = gsu_add(R[1], R[5], 0);
            go = gsu_count_down(&R[12]);
            R[0] = gsu_sub(R[0], R[4], 0);      /* LOOP's delay slot */
        } while (gsu_code_loop(0x01aaea, 1, gsu_code_loop(0x01aaf1, 4, go)));
        gsu_code(0x01aaf1, 4);
        gsu_code(0x01aaf5, 4); gsu_code(0x01a950, 1);
        return;
    } else { gsu_code(0x01aadf, 4); gsu_code(0x01aaf9, 1); }
    R[12] = R[4];
    R[12] = gsu_inc(R[12]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_lsr(R[0]);
    R[0] = gsu_sub(R[0], R[3], 0);
    R[13] = 0xAB02;
    gsu_code(0x01aafa, 9);
    do {
        carry = gsu.cy;
        /* the branch's delay slot */
        /* the branch's delay slot */
        gsu_code(0x01ab03, 2); gsu_code((carry) ? 0x01ab08 : 0x01ab05, 1);   /* the branch's delay slot */
        gsu_plot_r1();
        if (!carry) {
            R[0] = gsu_add(R[0], R[4], 0);
            R[1] = gsu_add(R[1], R[5], 0);
            gsu_code(0x01ab06, 3);
        }
        R[1] = gsu_dec(R[1]);
        R[2] = gsu_add(R[2], R[6], 0);
        go = gsu_count_down(&R[12]);
        R[0] = gsu_sub(R[0], R[3], 0);
    } while (gsu_code_loop(0x01ab02, 1, gsu_code_loop(0x01ab09, 4, go)));
    gsu_code(0x01ab09, 4);
    gsu_code(0x01ab0d, 4); gsu_code(0x01a950, 1);
}

/* A7D9 / A7E2: the next left edge of a textured polygon from the walk at
   $0060, with u and v ($0070, $007C) kept as bytes before it and their
   steps per row. 0 when the polygon has ended. */
static int tex_left_edge(int copy)
{
    R[0] = gsu_add(0x007F, R[9], 0);
    R[9] = gsu_hib(R[0]);
    gsu_code(0x01a7e3, 8);
    R[0] = gsu_ldw(FX_LEFT_U);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x01a7eb, 2);
    gsu_sbk(R[0]);
    gsu_code(0x01a7ed, 3);
    R[0] = gsu_ldw(FX_LEFT_V);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x01a7f0, 2);
    gsu_sbk(R[0]);
    gsu_code(0x01a7f2, 3);
    R[6] = gsu_ldw(FX_LEFT_AT);
    for (;;) {
        R[12] = R[9];
        gsu_code(0x01a7f5, 5);
        R[0] = gsu_ldw(FX_LEFT_U);
        gsu_code(0x01a7fa, 3);
        gsu_stw(FX_LEFT_U_PREV, R[0]);
        gsu_code(0x01a7fd, 3);
        R[0] = gsu_ldw(FX_LEFT_V);
        gsu_code(0x01a800, 3);
        gsu_stw(FX_LEFT_V_PREV, R[0]);
        gsu_code(0x01a803, 3);
        R[9] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        gsu_code(0x01a806, 5);
        R[4] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        gsu_code(0x01a80b, 4);
        R[0] = gsu_ldb(R[6]);
        gsu_code(0x01a80f, 3);
        gsu_stw(FX_LEFT_U, R[0]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        gsu_code(0x01a812, 4);
        R[0] = gsu_ldb(R[6]);
        gsu_code(0x01a816, 3);
        gsu_stw(FX_LEFT_V, R[0]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[11] = gsu_sub(R[4], R[2], 0);
        if (!gsu.z)
            { gsu_code(0x01a819, 7); break; } else { gsu_code(0x01a819, 7); gsu_code(0x01a7f4, 1); }
    }
    if (gsu.s)
        { return 0; } else { gsu_code(0x01a820, 3); gsu_code(0x01a827, 1); }
    gsu_code(0x01a828, 3);
    gsu_stw(FX_LEFT_AT, R[6]);
    reciprocal_at(R[11], GSU_COPY(2));
    reciprocal_read(GSU_COPY(2));
    gsu_code(0x01a834, 3);
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_sub(R[9], R[12], 0);
    R[9] = gsu_swap(R[12]);
    R[7] = slope();
    gsu_code(0x01a837, 19);
    R[0] = gsu_ldw(FX_LEFT_U);
    gsu_code(0x01a84a, 3);
    R[1] = gsu_ldw(FX_LEFT_U_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    ((gsu_stw(FX_LEFT_DU, gsu_code_then(0x01a84d, 8, (uv_step())))));
    gsu_code(0x01a855, 3);
    R[0] = gsu_ldw(FX_LEFT_U_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01a858, 4);
    gsu_stw(FX_LEFT_U, R[0]);
    gsu_code(0x01a85c, 3);
    R[0] = gsu_ldw(FX_LEFT_V);
    gsu_code(0x01a85f, 3);
    R[1] = gsu_ldw(FX_LEFT_V_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    ((gsu_stw(FX_LEFT_DV, gsu_code_then(0x01a862, 8, (uv_step())))));
    gsu_code(0x01a86a, 3);
    R[0] = gsu_ldw(FX_LEFT_V_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01a86d, 4);
    gsu_stw(FX_LEFT_V, R[0]);
    return 1;
}

/* A879: the next right edge, walking back from $0062. */
static int tex_right_edge(int copy)
{
    R[0] = gsu_add(0x007F, R[10], 0);
    R[10] = gsu_hib(R[0]);
    gsu_code(0x01a87a, 8);
    R[0] = gsu_ldw(FX_RIGHT_U);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x01a882, 2);
    gsu_sbk(R[0]);
    gsu_code(0x01a884, 3);
    R[0] = gsu_ldw(FX_RIGHT_V);
    R[0] = gsu_hib(R[0]);
    gsu_code(0x01a887, 2);
    gsu_sbk(R[0]);
    gsu_code(0x01a889, 3);
    R[6] = gsu_ldw(FX_RIGHT_AT);
    for (;;) {
        R[12] = R[10];
        gsu_code(0x01a88c, 5);
        R[0] = gsu_ldw(FX_RIGHT_U);
        gsu_code(0x01a891, 3);
        gsu_stw(FX_RIGHT_U_PREV, R[0]);
        gsu_code(0x01a894, 3);
        R[0] = gsu_ldw(FX_RIGHT_V);
        gsu_code(0x01a897, 3);
        gsu_stw(FX_RIGHT_V_PREV, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        gsu_code(0x01a89a, 4);
        R[0] = gsu_ldb(R[6]);
        gsu_code(0x01a89e, 3);
        gsu_stw(FX_RIGHT_V, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        gsu_code(0x01a8a1, 4);
        R[0] = gsu_ldb(R[6]);
        gsu_code(0x01a8a5, 3);
        gsu_stw(FX_RIGHT_U, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        gsu_code(0x01a8a8, 4);
        R[0] = gsu_ldb(R[6]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        gsu_code(0x01a8ac, 5);
        R[10] = gsu_ldb(R[6]);
        R[3] = gsu_sub(R[0], R[2], 0);
        if (!gsu.z)
            { gsu_code(0x01a8b1, 4); break; } else { gsu_code(0x01a8b1, 4); gsu_code(0x01a88b, 1); }
    }
    if (gsu.s)
        { return 0; } else { gsu_code(0x01a8b5, 3); gsu_code(0x01a8bc, 1); }
    gsu_code(0x01a8bd, 3);
    gsu_stw(FX_RIGHT_AT, R[6]);
    reciprocal_at(R[3], GSU_COPY(3));
    reciprocal_read(GSU_COPY(3));
    gsu_code(0x01a8c9, 3);
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_sub(R[10], R[12], 0);
    R[10] = gsu_swap(R[12]);
    R[8] = slope();
    gsu_code(0x01a8cc, 19);
    R[0] = gsu_ldw(FX_RIGHT_U);
    gsu_code(0x01a8df, 3);
    R[1] = gsu_ldw(FX_RIGHT_U_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    ((gsu_stw(FX_RIGHT_DU, gsu_code_then(0x01a8e2, 8, (uv_step())))));
    gsu_code(0x01a8ea, 3);
    R[0] = gsu_ldw(FX_RIGHT_U_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01a8ed, 4);
    gsu_stw(FX_RIGHT_U, R[0]);
    gsu_code(0x01a8f1, 3);
    R[0] = gsu_ldw(FX_RIGHT_V);
    gsu_code(0x01a8f4, 3);
    R[1] = gsu_ldw(FX_RIGHT_V_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    ((gsu_stw(FX_RIGHT_DV, gsu_code_then(0x01a8f7, 8, (uv_step())))));
    gsu_code(0x01a8ff, 3);
    R[0] = gsu_ldw(FX_RIGHT_V_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01a902, 4);
    gsu_stw(FX_RIGHT_V, R[0]);
    return 1;
}

/* One edge's u or v for the row: RIGHT += its step (LMS R0, step; LMS
   R4, right; ADD R4; SBK), the step across the row from right less left
   (FMULT by R6 = 1/width, ROL) to dst, then LEFT += its step. */
/* gsuwalk: the code of each copy of tex_across, read from the ROM */
static const GsuCode tex_across_fetch[2][6] = {
    {{0x01a74f, 3}, {0x01a752, 3}, {0x01a755, 2}, {0x01a757, 3}, {0x01a75a, 8}, {0x01a762, 4}},
    {{0x01a766, 3}, {0x01a769, 3}, {0x01a76c, 2}, {0x01a76e, 3}, {0x01a771, 8}, {0x01a779, 4}},
};

static uint16_t tex_across(uint16_t step_r, uint16_t right, uint16_t left, int lr, uint16_t step_l, int copy)
{
    uint16_t d;

    gsu_codes(tex_across_fetch[copy], 1);
    R[0] = gsu_ldw(step_r);
    gsu_codes(tex_across_fetch[copy] + 1, 1);
    R[4] = gsu_ldw(right);
    R[0] = gsu_add(R[0], R[4], 0);
    gsu_codes(tex_across_fetch[copy] + 2, 1);
    gsu_sbk(R[0]);
    gsu_codes(tex_across_fetch[copy] + 3, 1);
    R[lr] = gsu_ldw(left);
    R[0] = gsu_sub(R[4], R[lr], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    d = gsu_rol(R[0]);
    gsu_codes(tex_across_fetch[copy] + 4, 1);
    R[0] = gsu_ldw(step_l);
    R[0] = gsu_add(R[0], R[lr], 0);
    gsu_codes(tex_across_fetch[copy] + 5, 1);
    gsu_stw(left, R[0]);
    return d;
}

/* A710-A7D2: the rows of a textured polygon while both edges last (the
   shorter's count in R2's high byte), each row's texels read with MERGE
   of v (R7) and u (R8), the mask and the texture's address, GETC and
   PLOT; the edges' x stepped (as on a flat polygon) and the rows left on
   each kept at $0064/$0066. */
static void tex_rows(int copy)
{
    int go;

    gsu_code(0x01a711, 3);
    gsu_stw(FX_LEFT_SLOPE, R[7]);
    gsu_code(0x01a714, 3);
    gsu_stw(FX_RIGHT_SLOPE, R[8]);
    R[0] = gsu_sub(R[11], R[3], 0);
    if (!gsu.s) {
        gsu_code(0x01a717, 8);
        gsu_stw(FX_LEFT_ROWS, R[0]);
        R[0] = gsu_swap(R[3]);
        R[2] = gsu_or(R[0], R[2]);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_code(0x01a71f, 8);
        gsu_stw(FX_RIGHT_ROWS, R[0]);
        gsu_code(0x01a727, 2); gsu_code(0x01a736, 1);
    } else {
        R[0] = gsu_sub(R[3], R[11], 0);
        gsu_code(0x01a717, 4); gsu_code(0x01a729, 6);
        gsu_stw(FX_RIGHT_ROWS, R[0]);
        R[0] = gsu_swap(R[11]);
        R[2] = gsu_or(R[0], R[2]);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_code(0x01a72f, 8);
        gsu_stw(FX_LEFT_ROWS, R[0]);
    }
    do {
        /* A736 */
        R[1] = gsu_hib(R[9]);
        R[0] = gsu_hib(R[10]);
        R[12] = gsu_sub(R[0], R[1], 0);
        go = gsu.cy;
        R[12] = gsu_inc(R[12]);                 /* BCS's delay slot */
        if (!go)
            { R[12] = 0x0001; gsu_code(0x01a737, 12); } else { gsu_code(0x01a737, 9); gsu_code(0x01a742, 1); }
        reciprocal_at(R[12], GSU_COPY(4));
        reciprocal_read(GSU_COPY(4));
        gsu_code(0x01a74c, 3);
        R[6] = gsu_getbh(R[6]);
        R[5] = tex_across(FX_RIGHT_DU, FX_RIGHT_U, FX_LEFT_U, 8, FX_LEFT_DU, GSU_COPY(0));
        R[6] = tex_across(FX_RIGHT_DV, FX_RIGHT_V, FX_LEFT_V, 7, FX_LEFT_DV, GSU_COPY(1));
        gsu_code(0x01a77d, 3);
        R[4] = gsu_ldw(FX_TEX_U_OFF);
        R[8] = gsu_add(R[8], R[4], 0);
        gsu_code(0x01a780, 5);
        R[4] = gsu_ldw(FX_TEX_V_OFF);
        R[7] = gsu_add(R[7], R[4], 0);
        gsu_code(0x01a785, 5);
        R[0] = gsu_ldw(FX_TEX_BANK);
        R[0] = gsu_lob(R[0]);
        gsu_code(0x01a78a, 3);
        gsu_romb(R[0]);
        gsu_code(0x01a78d, 3);
        R[4] = gsu_ldw(FX_TEX_BASE);
        gsu_code(0x01a790, 3);
        R[3] = gsu_ldw(FX_TEX_MASK);
        R[13] = 0xA794;
        gsu_code(0x01a793, 2);
        do {
            R[0] = gsu_merge();
            R[0] = gsu_and(R[0], R[3]);
            gsu_code(0x01a795, 4);
            gsu_r14(gsu_add(R[0], R[4], 0));
            R[7] = gsu_add(R[7], R[6], 0);
            R[8] = gsu_add(R[8], R[5], 0);
            gsu_code(0x01a799, 5);
            gsu_color((gsu_getb()));              /* GETC */
            go = gsu_count_down(&R[12]);
            /* the branch's delay slot */
            /* the branch's delay slot */
            gsu_code(0x01a79e, 1); gsu_code((go) ? 0x01a794 : 0x01a79f, 1);   /* the branch's delay slot */
            (gsu_plot_r1());
        } while (go);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_code(0x01a7a0, 3);
        gsu_romb(R[0]);
        gsu_code(0x01a7a3, 3);
        R[7] = gsu_ldw(FX_LEFT_SLOPE);
        gsu_code(0x01a7a6, 3);
        R[8] = gsu_ldw(FX_RIGHT_SLOPE);
        edge_step(9, R[7], -1, GSU_COPY(2));
        edge_step(10, R[8], -1, GSU_COPY(3));
        R[2] = gsu_inc(R[2]);
        R[0] = gsu_swap(R[2]);
        R[0] = gsu_dec(R[0]);
        R[2] = gsu_swap(R[0]);
        R[0] = gsu_hib(R[2]);
    } while (gsu_code_loop(0x01a736, 1, gsu_code_loop(0x01a7c5, 14, !gsu.z)));
    gsu_code(0x01a7c5, 10); gsu_code(0x01a7d3, 1);
    gsu_code(0x01a7d4, 3);
    R[11] = gsu_ldw(FX_LEFT_ROWS);
    gsu_code(0x01a7d7, 3);
    R[3] = gsu_ldw(FX_RIGHT_ROWS);
}

/* A662: a textured polygon (R0 = NOT its type, R1 at its bank byte). The
   vertices are copied below it as for a flat polygon, u and v of each
   moved down a byte where they are; a polygon with no height is passed.
   Then the edges from the top vertex down. */
static void tex_polygon(int copy)
{
    int go;

    R[0] = gsu_lob(R[0]);
    R[12] = R[0];
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[3] = R[0];
    gsu_code(0x01a91a, 4); gsu_code(0x01a662, 11);
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01a66d, 4);
    gsu_stw(FX_TEX_BANK, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    gsu_code(0x01a671, 4);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_swap(R[0]);
    gsu_cmode(R[0]);
    gsu_code(0x01a675, 7);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01a67c, 5);
    gsu_stw(FX_TEX_BASE, R[0]);
    gsu_code(0x01a681, 1);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_code(0x01a682, 5);
    gsu_stw(FX_TEX_MASK, R[0]);
    R[0] = gsu_add(R[1], R[3], 0);
    gsu_code(0x01a687, 5);
    gsu_stw(FX_LIST_END, R[0]);
    R[5] = R[1];
    R[4] = gsu_sub(R[1], R[3], 0);
    R[2] = 0x03FF;
    R[8] = 0;
    R[13] = 0xA697;
    gsu_code(0x01a68c, 12);
    do {
        gsu_code(0x01a698, 2);
        R[0] = gsu_ldb(R[5]);
        gsu_code(0x01a69a, 2);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_code(0x01a69c, 6);
        R[0] = gsu_ldb(R[5]);
        gsu_code(0x01a6a2, 2);
        gsu_stb(R[4], (uint8_t)R[0]);
        gsu_sub(R[0], R[2], 0);                 /* CMP R2 */
        if (gsu.s) {
            R[2] = R[0];
            R[6] = R[4];
            gsu_code(0x01a6a4, 9);
        } else { gsu_code(0x01a6a4, 4); gsu_code(0x01a6ac, 1); }
        gsu_sub(R[0], R[8], 0);                 /* CMP R8 */
        if (!gsu.s)
            { R[8] = R[0]; gsu_code(0x01a6ad, 7); } else { gsu_code(0x01a6ad, 4); gsu_code(0x01a6b3, 1); }
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_sub(R[0], R[0], 0);
        R[5] = gsu_inc(R[5]);
        gsu_code(0x01a6b4, 8);
        R[0] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        gsu_code(0x01a6bc, 3);
        gsu_stb(R[5], (uint8_t)R[0]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_code(0x01a6bf, 4);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        gsu_code(0x01a6c3, 5);
        R[0] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        gsu_code(0x01a6c8, 3);
        gsu_stb(R[5], (uint8_t)R[0]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_code(0x01a6cb, 4);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        go = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);
    } while (gsu_code_loop(0x01a697, 1, gsu_code_loop(0x01a6cf, 2, go)));
    gsu_code(0x01a6cf, 2);
    R[0] = gsu_sub(R[2], R[8], 0);
    if (gsu.z)
        { gsu_code(0x01a6d1, 9); gsu_code(0x01a909, 1); return; } else { gsu_code(0x01a6d1, 5); gsu_code(0x01a6da, 1); }
    R[4] = R[6];
    R[4] = gsu_dec(R[4]);
    R[4] = gsu_dec(R[4]);
    R[5] = gsu_add(R[4], R[3], 0);
    gsu_code(0x01a6db, 9);
    R[0] = gsu_ldb(R[4]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    R[9] = gsu_swap(R[0]);
    R[10] = R[9];
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    gsu_code(0x01a6e4, 10);
    R[0] = gsu_ldb(R[4]);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01a6ee, 4);
    gsu_stw(FX_LEFT_U, R[0]);
    gsu_code(0x01a6f2, 3);
    gsu_stw(FX_RIGHT_U, R[0]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    gsu_code(0x01a6f5, 4);
    R[0] = gsu_ldb(R[4]);
    R[0] = gsu_swap(R[0]);
    gsu_code(0x01a6f9, 4);
    gsu_stw(FX_LEFT_V, R[0]);
    gsu_code(0x01a6fd, 3);
    gsu_stw(FX_RIGHT_V, R[0]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    gsu_code(0x01a700, 5);
    gsu_stw(FX_LEFT_AT, R[4]);
    gsu_code(0x01a705, 3);
    gsu_stw(FX_RIGHT_AT, R[5]);
    R[11] = 0;
    R[3] = 0;
    gsu_code(0x01a708, 4); gsu_cache(0xa70c); gsu_code(0x01a70c, 4); gsu_code(0x01a7d9, 1);
    for (;;) {
        /* A7D9 */
        gsu_moves(R[11]);
        R[0] = R[11];
        if (gsu.z) {                            /* BEQ past it, then its own test */
            gsu_code(0x01a7da, 4); gsu_code(0x01a7e2, 1);
            if (!tex_left_edge(GSU_COPY(0)))
                { gsu_code(0x01a820, 7); gsu_code(0x01a909, 1); return; }
        } else { gsu_code(0x01a7da, 8); gsu_code(0x01a870, 1); }
        /* A870 */
        gsu_moves(R[3]);
        R[0] = R[3];
        if (gsu.z) {                            /* BEQ past it, then its own test */
            gsu_code(0x01a871, 4); gsu_code(0x01a879, 1);
            if (!tex_right_edge(GSU_COPY(0)))
                { gsu_code(0x01a8b5, 7); gsu_code(0x01a909, 1); return; }
            gsu_code(0x01a906, 3); gsu_code(0x01a710, 1);
        } else { gsu_code(0x01a871, 8); gsu_code(0x01a710, 1); }
        tex_rows(GSU_COPY(0));
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

/* MULT #n: the signed product of the low byte and n. */
static uint16_t mult_imm(uint16_t v, int n, int copy)
{
    return gsu_mult(v, (uint16_t)n);       /* MULT #n */
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

/* The vertex in R1/R2 written at R9 (FROM R1; STW (R9); INC R9 twice;
   the same for R2). */
/* gsuwalk: the code of each copy of clip_put, read from the ROM */
static const GsuCode clip_put_fetch[16][2] = {
    {{0x019fac, 2}, {0x019fae, 4}},
    {{0x019f83, 2}, {0x019f85, 4}},
    {{0x01a07f, 2}, {0x01a081, 4}},
    {{0x01a056, 2}, {0x01a058, 4}},
    {{0x01a157, 2}, {0x01a159, 4}},
    {{0x01a128, 2}, {0x01a12a, 4}},
    {{0x01a237, 2}, {0x01a239, 4}},
    {{0x01a207, 2}, {0x01a209, 4}},
    {{0x01a335, 2}, {0x01a337, 4}},
    {{0x01a30c, 2}, {0x01a30e, 4}},
    {{0x01a420, 2}, {0x01a422, 4}},
    {{0x01a3f7, 2}, {0x01a3f9, 4}},
    {{0x01a511, 2}, {0x01a513, 4}},
    {{0x01a4e2, 2}, {0x01a4e4, 4}},
    {{0x01a608, 2}, {0x01a60a, 4}},
    {{0x01a5d9, 2}, {0x01a5db, 4}},
};

static void clip_put(int copy)
{
    gsu_codes(clip_put_fetch[copy], 1);
    gsu_stw(R[9], R[1]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
    gsu_codes(clip_put_fetch[copy] + 1, 1);
    gsu_stw(R[9], R[2]);
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
}

/* Where the edge from (R1, R2) to (R3, R6) crosses: R3, R6 = the
   difference, the other axis's offset = (edge - start) * its difference /
   this one's (LMULT, FxDivideSigned), written at R9 with the edge's
   value. */
/* gsuwalk: the code of each copy of clip_cross, read from the ROM */
static const GsuCode clip_cross_fetch[16][11] = {
    {{0x019f41, 19}, {0x019f54, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x019f5e, 1}, {0x019f5f, 5}, {0, 0}, {0, 0}, {0x019f64, 2}, {0x019f66, 3}},
    {{0x019f89, 9}, {0x019f92, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x019f9c, 1}, {0x019f9d, 5}, {0, 0}, {0, 0}, {0x019fa2, 2}, {0x019fa4, 3}},
    {{0x01a011, 19}, {0x01a024, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x01a02e, 1}, {0x01a02f, 5}, {0, 0}, {0, 0}, {0x01a034, 2}, {0x01a036, 3}},
    {{0x01a05c, 9}, {0x01a065, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x01a06f, 1}, {0x01a070, 5}, {0, 0}, {0, 0}, {0x01a075, 2}, {0x01a077, 3}},
    {{0x01a0e0, 25}, {0x01a0f9, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a103, 1}, {0, 0}, {0x01a104, 5}, {0x01a109, 1}, {0x01a10a, 4}, {0, 0}, {0, 0}},
    {{0x01a12e, 15}, {0x01a13d, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a147, 1}, {0, 0}, {0x01a148, 5}, {0x01a14d, 1}, {0x01a14e, 4}, {0, 0}, {0, 0}},
    {{0x01a1bc, 25}, {0x01a1d5, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a1df, 1}, {0, 0}, {0x01a1e0, 5}, {0x01a1e5, 2}, {0x01a1e7, 4}, {0, 0}, {0, 0}},
    {{0x01a20d, 15}, {0x01a21c, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a226, 1}, {0, 0}, {0x01a227, 5}, {0x01a22c, 2}, {0x01a22e, 4}, {0, 0}, {0, 0}},
    {{0x01a2ca, 19}, {0x01a2dd, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x01a2e7, 1}, {0x01a2e8, 5}, {0, 0}, {0, 0}, {0x01a2ed, 2}, {0x01a2ef, 3}},
    {{0x01a312, 9}, {0x01a31b, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x01a325, 1}, {0x01a326, 5}, {0, 0}, {0, 0}, {0x01a32b, 2}, {0x01a32d, 3}},
    {{0x01a3b3, 19}, {0x01a3c6, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x01a3d0, 1}, {0x01a3d1, 5}, {0, 0}, {0, 0}, {0x01a3d6, 2}, {0x01a3d8, 3}},
    {{0x01a3fd, 9}, {0x01a406, 10}, {0x018186, 1}, {0x8187, 0}, {0, 0}, {0x01a410, 1}, {0x01a411, 5}, {0, 0}, {0, 0}, {0x01a416, 2}, {0x01a418, 3}},
    {{0x01a49a, 25}, {0x01a4b3, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a4bd, 1}, {0, 0}, {0x01a4be, 5}, {0x01a4c3, 1}, {0x01a4c4, 4}, {0, 0}, {0, 0}},
    {{0x01a4e8, 15}, {0x01a4f7, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a501, 1}, {0, 0}, {0x01a502, 5}, {0x01a507, 1}, {0x01a508, 4}, {0, 0}, {0, 0}},
    {{0x01a58f, 25}, {0x01a5a8, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a5b2, 1}, {0, 0}, {0x01a5b3, 5}, {0x01a5b8, 1}, {0x01a5b9, 4}, {0, 0}, {0, 0}},
    {{0x01a5df, 15}, {0x01a5ee, 10}, {0x018186, 1}, {0x8187, 0}, {0x01a5f8, 1}, {0, 0}, {0x01a5f9, 5}, {0x01a5fe, 1}, {0x01a5ff, 4}, {0, 0}, {0, 0}},
};

static void clip_cross(const ClipEdge *e, uint16_t ret, int copy)
{
    R[3] = gsu_sub(R[3], R[1], 0);
    R[6] = gsu_sub(R[6], R[2], 0);
    if (e->y) {
        R[0] = R[3];
        R[3] = R[6];
        R[6] = R[0];
    }
    gsu_codes(clip_cross_fetch[copy], 1);
    R[0] = gsu_ldw(e->edge);
    R[0] = gsu_sub(R[0], R[e->y ? (2) : (1)], 0);
    {
        uint32_t p = gsu_lmult(R[0], R[6]);

        R[5] = (uint16_t)(p >> 16);
        R[4] = (uint16_t)p;
    }
    R[6] = R[3];
    R[11] = ret;
    gsu_codes(clip_cross_fetch[copy] + 1, 3);
    abi_FxDivideSigned();
    R[0] = gsu_add(R[4], R[e->y ? (gsu_codes(clip_cross_fetch[copy] + 4, 1), ((1))) : (gsu_codes(clip_cross_fetch[copy] + 5, 1), ((2)))], 0);
    gsu_codes(clip_cross_fetch[copy] + 6, 1);
    R[6] = gsu_ldw(e->edge);
    if (e->dec_edge)
        R[6] = gsu_dec(R[6]);
    if (e->y) {
        gsu_codes(clip_cross_fetch[copy] + 7, 1);
        gsu_stw(R[9], R[0]);
        R[9] = gsu_inc(R[9]);
        R[9] = gsu_inc(R[9]);
        gsu_codes(clip_cross_fetch[copy] + 8, 1);
        gsu_stw(R[9], R[6]);
    } else {
        gsu_codes(clip_cross_fetch[copy] + 9, 1);
        gsu_stw(R[9], R[6]);
        R[9] = gsu_inc(R[9]);
        R[9] = gsu_inc(R[9]);
        gsu_codes(clip_cross_fetch[copy] + 10, 1);
        gsu_stw(R[9], R[0]);
    }
    R[9] = gsu_inc(R[9]);
    R[9] = gsu_inc(R[9]);
}

/* Is the vertex in R(at) inside the edge? LMS R0, edge; FROM R(at); SUB
   R0 (and SUB #1 when strict); BGE, or BLT when strict. */
/* gsuwalk: the code of each copy of clip_inside, read from the ROM */
static const GsuCode clip_inside_fetch[24][1] = {
    {{0x019f2e, 6}},
    {{0x019f39, 3}},
    {{0x019f76, 3}},
    {{0x019ffa, 6}},
    {{0x01a007, 3}},
    {{0x01a047, 3}},
    {{0x01a0cd, 6}},
    {{0x01a0d8, 3}},
    {{0x01a11b, 3}},
    {{0x01a1a5, 6}},
    {{0x01a1b2, 3}},
    {{0x01a1f8, 3}},
    {{0x01a2b7, 6}},
    {{0x01a2c2, 3}},
    {{0x01a2ff, 3}},
    {{0x01a39c, 6}},
    {{0x01a3a9, 3}},
    {{0x01a3e8, 3}},
    {{0x01a487, 6}},
    {{0x01a492, 3}},
    {{0x01a4d5, 3}},
    {{0x01a578, 6}},
    {{0x01a585, 3}},
    {{0x01a5ca, 3}},
};

static int clip_inside(const ClipEdge *e, int at, int copy)
{
    gsu_codes(clip_inside_fetch[copy], 1);
    R[0] = gsu_ldw(e->edge);
    R[0] = gsu_sub(R[at], R[0], 0);
    if (e->strict) {
        R[0] = gsu_sub(R[0], 1, 0);
        return !ge();
    }
    return ge();
}

/* $0098 changed by one (LMS R0; INC or DEC R0; SBK). */
/* gsuwalk: the code of each copy of clip_count, read from the ROM */
static const GsuCode clip_count_fetch[16][2] = {
    {{0x019f6e, 3}, {0x019f71, 2}},
    {{0x019f7e, 3}, {0x019f81, 2}},
    {{0x01a03e, 3}, {0x01a041, 2}},
    {{0x01a051, 3}, {0x01a054, 2}},
    {{0x01a113, 3}, {0x01a116, 2}},
    {{0x01a123, 3}, {0x01a126, 2}},
    {{0x01a1f0, 3}, {0x01a1f3, 2}},
    {{0x01a202, 3}, {0x01a205, 2}},
    {{0x01a2f7, 3}, {0x01a2fa, 2}},
    {{0x01a307, 3}, {0x01a30a, 2}},
    {{0x01a3e0, 3}, {0x01a3e3, 2}},
    {{0x01a3f2, 3}, {0x01a3f5, 2}},
    {{0x01a4cd, 3}, {0x01a4d0, 2}},
    {{0x01a4dd, 3}, {0x01a4e0, 2}},
    {{0x01a5c2, 3}, {0x01a5c5, 2}},
    {{0x01a5d4, 3}, {0x01a5d7, 2}},
};

static void clip_count(int up, int copy)
{
    gsu_codes(clip_count_fetch[copy], 1);
    R[0] = gsu_ldw(FX_COUNT_OUT);
    R[0] = up ? (gsu_inc(R[0])) : (gsu_dec(R[0]));
    gsu_codes(clip_count_fetch[copy] + 1, 1);
    gsu_sbk(R[0]);
}

/* The polygon at R8 ($0040 vertices) clipped against one edge into R9,
   $0098 kept as the count written: the first vertex is copied after the
   last, then each edge of the polygon (R1, R2 to R3, R6) gives its start
   when inside and its crossing when it crosses. */
/* gsuwalk: the code of each copy of clip_against, read from the ROM */
static const GsuCode clip_against_fetch[8][35] = {
    {{0x019f08, 5}, {0x019f0d, 4}, {0x019f11, 6}, {0x019f17, 5}, {0x019f1c, 4}, {0x019fb9, 6}, {0x019f1f, 1}, {0x019fb9, 2}, {0x019fbf, 1}, {0x019f20, 2}, {0x019f22, 4}, {0x019f26, 4}, {0x019f2a, 4}, {0x019f69, 4}, {0x019fb3, 1}, {0x019f73, 2}, {0x019fb3, 1}, {0x019f38, 1}, {0, 0}, {0x019f34, 4}, {0x019f3c, 4}, {0x019f6d, 1}, {0x019f3c, 5}, {0x019fa7, 4}, {0x019fb3, 1}, {0x019fb2, 2}, {0, 0}, {0, 0}, {0x019f34, 4}, {0x019f75, 1}, {0x019f79, 4}, {0x019fab, 1}, {0x019f79, 5}, {0x019fb4, 3}, {0x019fb7, 2}},
    {{0x019fd7, 2}, {0x019fd9, 4}, {0x019fdd, 6}, {0x019fe3, 5}, {0x019fe8, 4}, {0x01a08c, 6}, {0x019feb, 1}, {0x01a08c, 2}, {0x01a092, 1}, {0x019fec, 2}, {0x019fee, 4}, {0x019ff2, 4}, {0x019ff6, 4}, {0x01a039, 4}, {0x01a086, 1}, {0x01a043, 3}, {0x01a086, 1}, {0x01a006, 1}, {0, 0}, {0x01a000, 6}, {0x01a00a, 6}, {0x01a03d, 1}, {0x01a00a, 7}, {0x01a07a, 4}, {0x01a086, 1}, {0x01a085, 2}, {0, 0}, {0, 0}, {0x01a000, 6}, {0x01a046, 1}, {0x01a04a, 6}, {0x01a07e, 1}, {0x01a04a, 7}, {0x01a087, 3}, {0x01a08a, 2}},
    {{0x01a0a7, 5}, {0x01a0ac, 4}, {0x01a0b0, 6}, {0x01a0b6, 5}, {0x01a0bb, 4}, {0x01a164, 6}, {0x01a0be, 1}, {0x01a164, 2}, {0x01a16a, 1}, {0x01a0bf, 2}, {0x01a0c1, 4}, {0x01a0c5, 4}, {0x01a0c9, 4}, {0x01a10e, 4}, {0x01a15e, 1}, {0x01a118, 2}, {0x01a15e, 1}, {0x01a0d7, 1}, {0x01a0d3, 4}, {0, 0}, {0x01a0db, 4}, {0x01a112, 1}, {0x01a0db, 5}, {0x01a152, 4}, {0x01a15e, 1}, {0x01a15d, 2}, {0x01a0d3, 4}, {0x01a11a, 1}, {0, 0}, {0, 0}, {0x01a11e, 4}, {0x01a156, 1}, {0x01a11e, 5}, {0x01a15f, 3}, {0x01a162, 2}},
    {{0x01a182, 2}, {0x01a184, 4}, {0x01a188, 6}, {0x01a18e, 5}, {0x01a193, 4}, {0x01a244, 6}, {0x01a196, 1}, {0x01a244, 2}, {0x01a24a, 1}, {0x01a197, 2}, {0x01a199, 4}, {0x01a19d, 4}, {0x01a1a1, 4}, {0x01a1eb, 4}, {0x01a23e, 1}, {0x01a1f5, 2}, {0x01a23e, 1}, {0x01a1b1, 1}, {0x01a1ab, 6}, {0, 0}, {0x01a1b5, 6}, {0x01a1ef, 1}, {0x01a1b5, 7}, {0x01a232, 4}, {0x01a23e, 1}, {0x01a23d, 2}, {0x01a1ab, 6}, {0x01a1f7, 1}, {0, 0}, {0, 0}, {0x01a1fb, 6}, {0x01a236, 1}, {0x01a1fb, 7}, {0x01a23f, 3}, {0x01a242, 2}},
    {{0x01a294, 2}, {0x01a296, 4}, {0x01a29a, 6}, {0x01a2a0, 5}, {0x01a2a5, 4}, {0x01a342, 6}, {0x01a2a8, 1}, {0x01a342, 2}, {0x01a348, 1}, {0x01a2a9, 2}, {0x01a2ab, 4}, {0x01a2af, 4}, {0x01a2b3, 4}, {0x01a2f2, 4}, {0x01a33c, 1}, {0x01a2fc, 2}, {0x01a33c, 1}, {0x01a2c1, 1}, {0, 0}, {0x01a2bd, 4}, {0x01a2c5, 4}, {0x01a2f6, 1}, {0x01a2c5, 5}, {0x01a330, 4}, {0x01a33c, 1}, {0x01a33b, 2}, {0, 0}, {0, 0}, {0x01a2bd, 4}, {0x01a2fe, 1}, {0x01a302, 4}, {0x01a334, 1}, {0x01a302, 5}, {0x01a33d, 3}, {0x01a340, 2}},
    {{0x01a379, 2}, {0x01a37b, 4}, {0x01a37f, 6}, {0x01a385, 5}, {0x01a38a, 4}, {0x01a42d, 6}, {0x01a38d, 1}, {0x01a42d, 2}, {0x01a433, 1}, {0x01a38e, 2}, {0x01a390, 4}, {0x01a394, 4}, {0x01a398, 4}, {0x01a3db, 4}, {0x01a427, 1}, {0x01a3e5, 2}, {0x01a427, 1}, {0x01a3a8, 1}, {0, 0}, {0x01a3a2, 6}, {0x01a3ac, 6}, {0x01a3df, 1}, {0x01a3ac, 7}, {0x01a41b, 4}, {0x01a427, 1}, {0x01a426, 2}, {0, 0}, {0, 0}, {0x01a3a2, 6}, {0x01a3e7, 1}, {0x01a3eb, 6}, {0x01a41f, 1}, {0x01a3eb, 7}, {0x01a428, 3}, {0x01a42b, 2}},
    {{0x01a464, 2}, {0x01a466, 4}, {0x01a46a, 6}, {0x01a470, 5}, {0x01a475, 4}, {0x01a51e, 6}, {0x01a478, 1}, {0x01a51e, 2}, {0x01a524, 1}, {0x01a479, 2}, {0x01a47b, 4}, {0x01a47f, 4}, {0x01a483, 4}, {0x01a4c8, 4}, {0x01a518, 1}, {0x01a4d2, 2}, {0x01a518, 1}, {0x01a491, 1}, {0x01a48d, 4}, {0, 0}, {0x01a495, 4}, {0x01a4cc, 1}, {0x01a495, 5}, {0x01a50c, 4}, {0x01a518, 1}, {0x01a517, 2}, {0x01a48d, 4}, {0x01a4d4, 1}, {0, 0}, {0, 0}, {0x01a4d8, 4}, {0x01a510, 1}, {0x01a4d8, 5}, {0x01a519, 3}, {0x01a51c, 2}},
    {{0x01a555, 2}, {0x01a557, 4}, {0x01a55b, 6}, {0x01a561, 5}, {0x01a566, 4}, {0x01a615, 6}, {0x01a569, 1}, {0x01a615, 2}, {0x01a61b, 1}, {0x01a56a, 2}, {0x01a56c, 4}, {0x01a570, 4}, {0x01a574, 4}, {0x01a5bd, 4}, {0x01a60f, 1}, {0x01a5c7, 2}, {0x01a60f, 1}, {0x01a584, 1}, {0x01a57e, 6}, {0, 0}, {0x01a588, 6}, {0x01a5c1, 1}, {0x01a588, 7}, {0x01a603, 4}, {0x01a60f, 1}, {0x01a60e, 2}, {0x01a57e, 6}, {0x01a5c9, 1}, {0, 0}, {0, 0}, {0x01a5cd, 6}, {0x01a607, 1}, {0x01a5cd, 7}, {0x01a610, 3}, {0x01a613, 2}},
};

/* gsuwalk: the copy of mult_imm each copy of clip_against calls */
static const uint8_t mult_imm_in_clip_against[8] = {0, 1, 2, 3, 4, 5, 6, 7};

/* gsuwalk: the copy of clip_inside each copy of clip_against calls */
static const uint8_t clip_inside_in_clip_against[8] = {0, 3, 6, 9, 12, 15, 18, 21};

/* gsuwalk: the copy of clip_inside each copy of clip_against calls */
static const uint8_t clip_inside_in_clip_against_2[8] = {1, 4, 7, 10, 13, 16, 19, 22};

/* gsuwalk: the copy of clip_count each copy of clip_against calls */
static const uint8_t clip_count_in_clip_against[8] = {0, 2, 4, 6, 8, 10, 12, 14};

/* gsuwalk: the copy of clip_cross each copy of clip_against calls */
static const uint8_t clip_cross_in_clip_against[8] = {0, 2, 4, 6, 8, 10, 12, 14};

/* gsuwalk: the copy of clip_inside each copy of clip_against calls */
static const uint8_t clip_inside_in_clip_against_3[8] = {2, 5, 8, 11, 14, 17, 20, 23};

/* gsuwalk: the copy of clip_put each copy of clip_against calls */
static const uint8_t clip_put_in_clip_against[8] = {0, 2, 4, 6, 8, 10, 12, 14};

/* gsuwalk: the copy of clip_count each copy of clip_against calls */
static const uint8_t clip_count_in_clip_against_2[8] = {1, 3, 5, 7, 9, 11, 13, 15};

/* gsuwalk: the copy of clip_put each copy of clip_against calls */
static const uint8_t clip_put_in_clip_against_2[8] = {1, 3, 5, 7, 9, 11, 13, 15};

/* gsuwalk: the copy of clip_cross each copy of clip_against calls */
static const uint8_t clip_cross_in_clip_against_2[8] = {1, 3, 5, 7, 9, 11, 13, 15};

static void clip_against(const ClipEdge *e, int copy)
{
    gsu_codes(clip_against_fetch[copy], 1);
    R[1] = gsu_ldw(R[8]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_codes(clip_against_fetch[copy] + 1, 1);
    R[2] = gsu_ldw(R[8]);
    R[8] = gsu_sub(R[8], 2, 0);
    gsu_codes(clip_against_fetch[copy] + 2, 1);
    R[0] = gsu_ldw(FX_COUNT);
    R[0] = mult_imm(R[0], 4, GSU_COPY(mult_imm_in_clip_against[copy]));
    R[0] = gsu_add(R[0], R[8], 0);
    gsu_codes(clip_against_fetch[copy] + 3, 1);
    gsu_stw(R[0], R[1]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_inc(R[0]);
    gsu_codes(clip_against_fetch[copy] + 4, 1);
    gsu_stw(R[0], R[2]);
    do {
        gsu_codes(clip_against_fetch[copy] + 9, 1);
        R[1] = gsu_ldw(R[8]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        gsu_codes(clip_against_fetch[copy] + 10, 1);
        R[2] = gsu_ldw(R[8]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        gsu_codes(clip_against_fetch[copy] + 11, 1);
        R[3] = gsu_ldw(R[8]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        gsu_codes(clip_against_fetch[copy] + 12, 1);
        R[6] = gsu_ldw(R[8]);
        R[8] = gsu_sub(R[8], 2, 0);
        /* the first end, then the second in each way, as the original tests them */
        if (!clip_inside(e, e->y ? (2) : (1), GSU_COPY(clip_inside_in_clip_against[copy]))) {
            gsu_codes(clip_against_fetch[copy] + 17, 1);
            if (!clip_inside(e, e->y ? (gsu_codes(clip_against_fetch[copy] + 18, 1), (((6)))) : (gsu_codes(clip_against_fetch[copy] + 19, 1), (((3)))), GSU_COPY(clip_inside_in_clip_against_2[copy]))) {
                gsu_codes(clip_against_fetch[copy] + 20, 2);
                clip_count(0, GSU_COPY(clip_count_in_clip_against[copy]));
                gsu_codes(clip_against_fetch[copy] + 15, 2);
            } else {
                /* coming in: the ends swapped, the crossing written */
                R[0] = R[1];
                R[1] = R[3];
                R[3] = R[0];
                R[0] = R[2];
                R[2] = R[6];
                R[6] = R[0];
                gsu_codes(clip_against_fetch[copy] + 22, 1);
                clip_cross(e, e->ret_out_in, GSU_COPY(clip_cross_in_clip_against[copy]));
                gsu_codes(clip_against_fetch[copy] + 13, 2);
            }
        } else if (clip_inside(e, e->y ? (gsu_codes(clip_against_fetch[copy] + 26, 2), (((6)))) : (gsu_codes(clip_against_fetch[copy] + 28, 2), (((3)))), GSU_COPY(clip_inside_in_clip_against_3[copy]))) {
            gsu_codes(clip_against_fetch[copy] + 30, 2);
            clip_put(GSU_COPY(clip_put_in_clip_against[copy]));
            gsu_codes(clip_against_fetch[copy] + 25, 1);
        } else {
            gsu_codes(clip_against_fetch[copy] + 32, 1);
            clip_count(1, GSU_COPY(clip_count_in_clip_against_2[copy]));
            clip_put(GSU_COPY(clip_put_in_clip_against_2[copy]));
            clip_cross(e, e->ret_in_out, GSU_COPY(clip_cross_in_clip_against_2[copy]));
            gsu_codes(clip_against_fetch[copy] + 23, 2);
        }
        gsu_codes(clip_against_fetch[copy] + 33, 1);
        R[0] = gsu_ldw(FX_COUNT);
        R[0] = gsu_dec(R[0]);
        gsu_codes(clip_against_fetch[copy] + 34, 1);
        gsu_sbk(R[0]);
    } while (gsu_codes_loop(clip_against_fetch[copy] + 5, 2, !gsu.z));
    gsu_codes(clip_against_fetch[copy] + 7, 2);
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
/* gsuwalk: the code of each copy of clip_none, read from the ROM */
static const GsuCode clip_none_fetch[8][2] = {
    {{0x019fc8, 3}, {0x01a65c, 6}},
    {{0x01a09b, 3}, {0x01a65c, 6}},
    {{0x01a173, 3}, {0x01a65c, 6}},
    {{0x01a253, 3}, {0x01a65c, 6}},
    {{0x01a35d, 3}, {0x01a65c, 6}},
    {{0x01a448, 3}, {0x01a65c, 6}},
    {{0x01a539, 3}, {0x01a65c, 6}},
    {{0x01a630, 3}, {0x01a65c, 6}},
};

static void clip_none(int copy)
{
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_codes(clip_none_fetch[copy], 2);
    gsu_pull(&R[10]);
}

/* 9F01: a line (R0 = its count, below 3) clipped against all four edges,
   from $005A to $0688 and back twice; fewer than 2 points left is
   nothing. Ends with R0 = 2, R15 pulled. */
/* One edge of clip_line (the original has each edge in its own code):
   0 when nothing is left (R15 pulled). */
/* gsuwalk: the code of each copy of clip_line_edge, read from the ROM */
static const GsuCode clip_line_edge_fetch[4][8] = {
    {{0x01a27a, 3}, {0x019f01, 4}, {0, 0}, {0x019f05, 3}, {0x019fc0, 3}, {0x019fc3, 4}, {0x019fcb, 1}, {0x019fc3, 5}},
    {{0x019fcc, 5}, {0, 0}, {0x019fd1, 6}, {0, 0}, {0x01a093, 3}, {0x01a096, 4}, {0x01a09e, 1}, {0x01a096, 5}},
    {{0x01a09f, 5}, {0, 0}, {0, 0}, {0x01a0a4, 3}, {0x01a16b, 3}, {0x01a16e, 4}, {0x01a176, 1}, {0x01a16e, 5}},
    {{0x01a177, 5}, {0, 0}, {0x01a17c, 6}, {0, 0}, {0x01a24b, 3}, {0x01a24e, 4}, {0x01a256, 1}, {0x01a24e, 5}},
};

/* gsuwalk: the copy of clip_against each copy of clip_line_edge calls */
static const uint8_t clip_against_in_clip_line_edge[4] = {0, 1, 2, 3};

/* gsuwalk: the copy of clip_none each copy of clip_line_edge calls */
static const uint8_t clip_none_in_clip_line_edge[4] = {0, 1, 2, 3};

static int clip_line_edge(int k, int copy)
{
    gsu_codes(clip_line_edge_fetch[copy], 2);
    gsu_stw(FX_COUNT, R[0]);
    if (k & 1) {
        R[8] = FX_CLIP_LIST;
        gsu_codes(clip_line_edge_fetch[copy] + 2, 1);
        R[9] = gsu_ldw(FX_POLY_OUT);
    } else {
        gsu_codes(clip_line_edge_fetch[copy] + 3, 1);
        R[8] = gsu_ldw(FX_POLY_OUT);
        R[9] = FX_CLIP_LIST;
    }
    clip_against(&line_edges[k], GSU_COPY(clip_against_in_clip_line_edge[copy]));
    gsu_codes(clip_line_edge_fetch[copy] + 4, 1);
    R[0] = gsu_ldw(FX_COUNT_OUT);
    R[0] = gsu_sub(R[0], 2, 0);
    if (gsu.s) {
        gsu_codes(clip_line_edge_fetch[copy] + 7, 1);
        clip_none(GSU_COPY(clip_none_in_clip_line_edge[copy]));
        return 0;
    } else { gsu_codes(clip_line_edge_fetch[copy] + 5, 2); }
    R[0] = gsu_add(R[0], 2, 0);
    return 1;
}

static void clip_line(int copy)
{
    if (!clip_line_edge(0, GSU_COPY(0)))
        return;
    if (!clip_line_edge(1, GSU_COPY(1)))
        return;
    if (!clip_line_edge(2, GSU_COPY(2)))
        return;
    if (!clip_line_edge(3, GSU_COPY(3)))
        return;
    R[0] = 0x0002;
    gsu_code(0x01a257, 8);
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
    return gsu_umult(v, (uint16_t)n);      /* UMULT #n */
}

/* Three words from R7 to registers (TO Rn; LDW (R7); INC R7 twice), the
   last without its INCs when LAST. */
/* gsuwalk: the code of each copy of near_load, read from the ROM */
static const GsuCode near_load_fetch[5][3] = {
    {{0x01991d, 2}, {0x01991f, 4}, {0x019923, 4}},
    {{0x019927, 4}, {0x01992b, 4}, {0x01992f, 4}},
    {{0x019959, 2}, {0x01995b, 4}, {0x01995f, 4}},
    {{0x01996e, 2}, {0x019970, 4}, {0x019974, 4}},
    {{0x019a08, 2}, {0x019a0a, 4}, {0x019a0e, 4}},
};

static void near_load(int a, int b, int c, int last, int copy)
{
    gsu_codes(near_load_fetch[copy], 1);
    R[a] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_codes(near_load_fetch[copy] + 1, 1);
    R[b] = gsu_ldw(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_codes(near_load_fetch[copy] + 2, 1);
    R[c] = gsu_ldw(R[7]);
    if (!last) {
        R[7] = gsu_inc(R[7]);
        R[7] = gsu_inc(R[7]);
    }
}

/* Three registers written at R8 (FROM Rn; STW (R8); INC R8 twice). */
/* gsuwalk: the code of each copy of near_put, read from the ROM */
static const GsuCode near_put_fetch[5][3] = {
    {{0x019938, 2}, {0x01993a, 4}, {0x01993e, 4}},
    {{0x019942, 4}, {0x019946, 4}, {0x01994a, 4}},
    {{0x01998d, 2}, {0x01998f, 4}, {0x019993, 4}},
    {{0x0199ac, 2}, {0x0199ae, 4}, {0x0199b2, 4}},
    {{0x0199c6, 2}, {0x0199c8, 4}, {0x0199cc, 4}},
};

static void near_put(int a, int b, int c, int copy)
{
    gsu_codes(near_put_fetch[copy], 1);
    gsu_stw(R[8], R[a]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_codes(near_put_fetch[copy] + 1, 1);
    gsu_stw(R[8], R[b]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
    gsu_codes(near_put_fetch[copy] + 2, 1);
    gsu_stw(R[8], R[c]);
    R[8] = gsu_inc(R[8]);
    R[8] = gsu_inc(R[8]);
}

/* $0040 counted up (LMS R0; INC R0; SBK). */
/* gsuwalk: the code of each copy of near_count, read from the ROM */
static const GsuCode near_count_fetch[3][2] = {
    {{0x019997, 5}, {0x01999c, 2}},
    {{0x0199b6, 5}, {0x0199bb, 2}},
    {{0x0199d0, 5}, {0x0199d5, 2}},
};

static void near_count(int copy)
{
    gsu_codes(near_count_fetch[copy], 1);
    R[0] = gsu_ldw(FX_COUNT);
    R[0] = gsu_inc(R[0]);
    gsu_codes(near_count_fetch[copy] + 1, 1);
    gsu_sbk(R[0]);
}

/* 9A4C, and 9A43: R0 = 0, or the count, and R15 pulled. */
/* gsuwalk: the code of each copy of gather_return, read from the ROM */
static const GsuCode gather_return_fetch[2][5] = {
    {{0, 0}, {0, 0}, {0, 0}, {0x0199e7, 3}, {0x019a4c, 6}},
    {{0x019a37, 8}, {0x019a43, 5}, {0x019a48, 4}, {0x019a37, 12}, {0x019a4c, 6}},
};

static void gather_return(int count, int copy)
{
    /* each way has its own return (the pull into R15), as the original */
    if (count) {
        gsu_codes(gather_return_fetch[copy], 2);
        R[0] = gsu_ldw(FX_COUNT);
        gsu_codes(gather_return_fetch[copy] + 2, 1);
        gsu_pull(&R[10]);
    } else {
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_codes(gather_return_fetch[copy] + 3, 2);
        gsu_pull(&R[10]);
    }
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
    int go;

    gsu_code(0x0198d8, 2);
    gsu_push(&R[10], R[11]);
    R[9] = R[0];
    gsu_code(0x0198da, 7);
    R[5] = gsu_ldw(FX_OFFSET_X);
    gsu_code(0x0198e1, 3);
    R[6] = gsu_ldw(FX_OFFSET_Y);
    gsu_code(0x0198e4, 3);
    R[7] = gsu_ldw(FX_OFFSET_Z);
    gsu_code(0x0198e7, 3);
    R[2] = gsu_ldw(FX_POLY_OUT);
    R[12] = R[9];
    R[13] = 0x98ED;
    gsu_code(0x0198ea, 4);
    do {
        gsu_code(0x0198ee, 1);
        R[0] = gsu_getb();
        gsu_code(0x0198ef, 1);
        gsu_r14(gsu_inc(R[14]));
        R[0] = umult_imm(R[0], 6);
        R[1] = gsu_add(FX_POINTS, R[0], 0);
        gsu_code(0x0198f0, 8);
        R[0] = gsu_ldw(R[1]);
        R[1] = gsu_inc(R[1]);
        R[1] = gsu_inc(R[1]);
        R[0] = gsu_add(R[0], R[5], 0);
        gsu_code(0x0198f8, 4);
        gsu_stw(R[2], R[0]);
        R[2] = gsu_inc(R[2]);
        R[2] = gsu_inc(R[2]);
        gsu_code(0x0198fc, 3);
        R[0] = gsu_ldw(R[1]);
        R[1] = gsu_inc(R[1]);
        R[1] = gsu_inc(R[1]);
        R[0] = gsu_add(R[0], R[6], 0);
        gsu_code(0x0198ff, 4);
        gsu_stw(R[2], R[0]);
        R[2] = gsu_inc(R[2]);
        R[2] = gsu_inc(R[2]);
        gsu_code(0x019903, 3);
        R[0] = gsu_ldw(R[1]);
        R[0] = gsu_add(R[0], R[7], 0);
        gsu_code(0x019906, 2);
        gsu_stw(R[2], R[0]);
        R[2] = gsu_inc(R[2]);
        go = gsu_count_down(&R[12]);
        R[2] = gsu_inc(R[2]);
    } while (gsu_code_loop(0x0198ed, 1, gsu_code_loop(0x019908, 2, go)));
    gsu_code(0x019908, 2);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01990a, 7);
    gsu_stw(FX_COUNT, R[0]);
    gsu_code(0x019911, 3);
    R[7] = gsu_ldw(FX_POLY_OUT);
    R[8] = FX_CLIP_LIST;
    R[0] = gsu_sub(R[9], 2, 0);
    if (gsu.z) {
        gsu_code(0x019914, 9);
        near_load(1, 2, 3, 0, GSU_COPY(0));
        near_load(4, 5, 6, 1, GSU_COPY(1));
        R[11] = 0x9937;
        gsu_code(0x019933, 4); gsu_code(0x01981a, 1); gsu_cache(0x981b);
        (abi_FxSegmentCrossZ());
        gsu_code(0x019937, 1);
        near_put(1, 2, 3, GSU_COPY(0));
        near_put(4, 5, 6, GSU_COPY(1));
        R[0] = 0x0002;
        gsu_code(0x01994e, 7);
        gsu_stw(FX_COUNT, R[0]);
        gsu_code(0x019955, 3); gsu_code(0x0199ea, 1);
    } else {
        gsu_code(0x019914, 8); gsu_code(0x019958, 1);
        do {
            /* 9958 */
            near_load(1, 2, 3, 0, GSU_COPY(2));
            R[0] = gsu_dec(R[9]);
            if (gsu.z)
                { gsu_code(0x019963, 11); R[7] = gsu_ldw(FX_POLY_OUT); } else { gsu_code(0x019963, 7); gsu_code(0x01996d, 1); }
            near_load(4, 5, 6, 1, GSU_COPY(3));
            R[7] = gsu_sub(R[7], 4, 0);
            R[0] = gsu_and(R[6], R[3]);
            if (!gsu.s) {
                gsu_moves(R[3]);
                if (!gsu.s) {
                    gsu_code(0x019978, 7); gsu_code(0x019983, 5); gsu_code(0x01998c, 1);
                    near_put(1, 2, 3, GSU_COPY(2));
                    near_count(GSU_COPY(0));
                    gsu_moves(R[6]);
                    if (gsu.s) {
                        R[11] = 0x99AB;
                        gsu_code(0x01999e, 4); gsu_code(0x0199a6, 5); gsu_code(0x01981a, 1); gsu_cache(0x981b);
                        abi_FxSegmentCrossZ();
                        gsu_code(0x0199ab, 1);
                        near_put(4, 5, 6, GSU_COPY(3));
                        near_count(GSU_COPY(1));
                        gsu_code(0x0199bd, 3); gsu_code(0x0199d6, 1);
                    } else { gsu_code(0x01999e, 8); gsu_code(0x0199d6, 1); }
                } else {
                    R[11] = 0x99C5;
                    gsu_code(0x019978, 7); gsu_code(0x019983, 9); gsu_code(0x0199c0, 5); gsu_code(0x01981a, 1); gsu_cache(0x981b);
                    abi_FxSegmentCrossZ();
                    gsu_code(0x0199c5, 1);
                    near_put(1, 2, 3, GSU_COPY(4));
                    near_count(GSU_COPY(2));
                }
            } else { gsu_code(0x019978, 11); gsu_code(0x0199d6, 1); }
            /* 99D6 */
            R[9] = gsu_dec(R[9]);
        } while (gsu_code_loop(0x019958, 1, gsu_code_loop(0x0199d7, 7, !gsu.z)));
        gsu_code(0x0199d7, 3); gsu_code(0x0199de, 1);
        gsu_code(0x0199df, 3);
        R[0] = gsu_ldw(FX_COUNT);
        R[0] = gsu_sub(R[0], 3, 0);
        if (gsu.s) {
            gsu_code(0x0199e2, 5);
            gather_return(0, GSU_COPY(0));
            return;
        } else { gsu_code(0x0199e2, 4); gsu_code(0x0199ea, 1); }
    }
    /* 99EA */
    gsu_code(0x0199eb, 3);
    R[3] = gsu_ldw(FX_OFFSET_X);
    gsu_code(0x0199ee, 3);
    R[13] = gsu_ldw(FX_OFFSET_Y);
    gsu_code(0x0199f1, 3);
    R[9] = gsu_ldw(FX_OFFSET_Z);
    R[0] = 0x0014;
    gsu_code(0x0199f4, 4);
    gsu_romb(R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x0199f8, 4);
    gsu_stw(FX_CODES, R[0]);
    gsu_code(0x0199fc, 3);
    gsu_stw(FX_ROM_R14, R[14]);
    gsu_code(0x0199ff, 3);
    R[12] = gsu_ldw(FX_COUNT);
    R[7] = FX_CLIP_LIST;
    gsu_code(0x019a02, 6);
    R[8] = gsu_ldw(FX_POLY_OUT);
    do {
        near_load(1, 2, 6, 0, GSU_COPY(4));
        R[11] = 0x9A18;
        gsu_code(0x019a12, 6); gsu_code(0x018bff, 1);
        (abi_FxProjectVertex());
        gsu_code(0x019a18, 3);
        gsu_stw(R[8], R[1]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        gsu_code(0x019a1b, 4);
        gsu_stw(R[8], R[2]);
        R[8] = gsu_inc(R[8]);
        R[8] = gsu_inc(R[8]);
        gsu_code(0x019a1f, 5);
        R[6] = gsu_ldw(FX_CODES);
        R[0] = gsu_or(R[0], R[6]);
        R[12] = gsu_dec(R[12]);
        go = !gsu.z;
        /* the branch's delay slot */
        /* the branch's delay slot */
        gsu_code(0x019a24, 4); gsu_code((go) ? 0x019a07 : 0x019a28, 1);   /* the branch's delay slot */
        gsu_sbk(R[0]);                          /* BNE's delay slot */
    } while (go);
    gsu_code(0x019a29, 3);
    R[0] = gsu_ldw(FX_ROM_BANK);
    gsu_code(0x019a2c, 2);
    gsu_romb(R[0]);
    gsu_code(0x019a2e, 3);
    gsu_r14(gsu_ldw(FX_ROM_R14));
    gsu_code(0x019a31, 3);
    R[3] = gsu_ldw(FX_POLY_OUT);
    gsu_code(0x019a34, 3);
    R[0] = gsu_ldw(FX_CODES);
    R[1] = 0x0F00;
    R[0] = gsu_and(R[0], R[1]);
    gsu_sub(R[0], R[1], 0);                     /* CMP R1 */
    gather_return(gsu.z, GSU_COPY(1));
}

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
/* gsuwalk: the code of each copy of clip_polygon_edge, read from the ROM */
static const GsuCode clip_polygon_edge_fetch[4][17] = {
    {{0x01a27e, 3}, {0x01a281, 5}, {0x01a28a, 1}, {0x01a281, 9}, {0x01a362, 1}, {0x01a28b, 3}, {0x01a28e, 3}, {0x01a291, 3}, {0x01a349, 3}, {0x01a34c, 3}, {0x01a34f, 3}, {0x01a352, 3}, {0x01a355, 3}, {0x01a358, 4}, {0x01a360, 1}, {0x01a358, 5}, {0x01a361, 2}},
    {{0x01a363, 3}, {0x01a366, 5}, {0x01a36f, 1}, {0x01a366, 9}, {0x01a44d, 1}, {0x01a370, 3}, {0x01a373, 3}, {0x01a376, 3}, {0x01a434, 3}, {0x01a437, 3}, {0x01a43a, 3}, {0x01a43d, 3}, {0x01a440, 3}, {0x01a443, 4}, {0x01a44b, 1}, {0x01a443, 5}, {0x01a44c, 2}},
    {{0x01a44e, 3}, {0x01a451, 5}, {0x01a45a, 1}, {0x01a451, 9}, {0x01a53e, 1}, {0x01a45b, 3}, {0x01a45e, 3}, {0x01a461, 3}, {0x01a525, 3}, {0x01a528, 3}, {0x01a52b, 3}, {0x01a52e, 3}, {0x01a531, 3}, {0x01a534, 4}, {0x01a53c, 1}, {0x01a534, 5}, {0x01a53d, 2}},
    {{0x01a53f, 3}, {0x01a542, 5}, {0x01a54b, 1}, {0x01a542, 9}, {0x01a635, 1}, {0x01a54c, 3}, {0x01a54f, 3}, {0x01a552, 3}, {0x01a61c, 3}, {0x01a61f, 3}, {0x01a622, 3}, {0x01a625, 3}, {0x01a628, 3}, {0x01a62b, 4}, {0x01a633, 1}, {0x01a62b, 5}, {0x01a634, 2}},
};

/* gsuwalk: the copy of clip_against each copy of clip_polygon_edge calls */
static const uint8_t clip_against_in_clip_polygon_edge[4] = {4, 5, 6, 7};

/* gsuwalk: the copy of clip_none each copy of clip_polygon_edge calls */
static const uint8_t clip_none_in_clip_polygon_edge[4] = {4, 5, 6, 7};

static int clip_polygon_edge(int k, int copy)
{
    static const uint16_t bits[4] = {4, 8, 1, 2};

    gsu_codes(clip_polygon_edge_fetch[copy], 1);
    R[1] = gsu_ldw(FX_CODES);
    R[1] = gsu_and(R[1], bits[k]);
    if (gsu.z)
        { gsu_codes(clip_polygon_edge_fetch[copy] + 3, 2); return 1; } else { gsu_codes(clip_polygon_edge_fetch[copy] + 1, 2); }
    gsu_codes(clip_polygon_edge_fetch[copy] + 5, 1);
    R[8] = gsu_ldw(FX_CLIP_IN);
    gsu_codes(clip_polygon_edge_fetch[copy] + 6, 1);
    R[9] = gsu_ldw(FX_CLIP_OUT);
    gsu_codes(clip_polygon_edge_fetch[copy] + 7, 1);
    gsu_stw(FX_COUNT, R[0]);
    clip_against(&polygon_edges[k], GSU_COPY(clip_against_in_clip_polygon_edge[copy]));
    gsu_codes(clip_polygon_edge_fetch[copy] + 8, 1);
    R[1] = gsu_ldw(FX_CLIP_IN);
    gsu_codes(clip_polygon_edge_fetch[copy] + 9, 1);
    R[2] = gsu_ldw(FX_CLIP_OUT);
    gsu_codes(clip_polygon_edge_fetch[copy] + 10, 1);
    gsu_stw(FX_CLIP_IN, R[2]);
    gsu_codes(clip_polygon_edge_fetch[copy] + 11, 1);
    gsu_stw(FX_CLIP_OUT, R[1]);
    gsu_codes(clip_polygon_edge_fetch[copy] + 12, 1);
    R[0] = gsu_ldw(FX_COUNT_OUT);
    R[0] = gsu_sub(R[0], 3, 0);
    if (gsu.s) {
        gsu_codes(clip_polygon_edge_fetch[copy] + 15, 1);
        clip_none(GSU_COPY(clip_none_in_clip_polygon_edge[copy]));
        return 0;
    } else { gsu_codes(clip_polygon_edge_fetch[copy] + 13, 2); }
    R[0] = gsu_add(R[0], 3, 0);
    gsu_codes(clip_polygon_edge_fetch[copy] + 16, 1);
    return 1;
}

void abi_FxPolygonClipScreen(void)
{
    gsu_code(0x01a260, 3);
    R[8] = gsu_ldw(FX_POLY_OUT);
    R[9] = FX_CLIP_LIST;
    gsu_code(0x01a263, 6);
    gsu_stw(FX_CLIP_IN, R[8]);
    gsu_code(0x01a269, 3);
    gsu_stw(FX_CLIP_OUT, R[9]);
    gsu_code(0x01a26c, 3);
    gsu_stw(FX_COUNT_OUT, R[0]);
    gsu_code(0x01a26f, 2);
    gsu_push(&R[10], R[11]);
    R[1] = 0x0003;
    gsu_sub(R[0], R[1], 0);                     /* CMP R1 */
    if (gsu.s) {
        gsu_code(0x01a271, 9);
        clip_line(GSU_COPY(0));
        return;
    } else { gsu_code(0x01a271, 8); gsu_code(0x01a27d, 1); }
    if (!clip_polygon_edge(0, GSU_COPY(0)) || (!clip_polygon_edge(1, GSU_COPY(1))) || (!clip_polygon_edge(2, GSU_COPY(2))) || (!clip_polygon_edge(3, GSU_COPY(3))))
        return;
    /* A635 */
    gsu_code(0x01a636, 3);
    R[1] = gsu_ldw(FX_POLY_OUT);
    gsu_code(0x01a639, 3);
    R[8] = gsu_ldw(FX_CLIP_IN);
    R[1] = gsu_sub(R[1], R[8], 0);
    if (!gsu.z) {
        int go;

        gsu_code(0x01a63c, 8);
        R[9] = gsu_ldw(FX_CLIP_OUT);
        R[12] = R[0];
        R[13] = 0xA647;
        gsu_code(0x01a644, 4);
        do {
            gsu_code(0x01a648, 1);
            R[0] = gsu_ldw(R[8]);
            gsu_code(0x01a649, 1);
            gsu_stw(R[9], R[0]);
            R[9] = gsu_inc(R[9]);
            R[9] = gsu_inc(R[9]);
            R[8] = gsu_inc(R[8]);
            R[8] = gsu_inc(R[8]);
            gsu_code(0x01a64a, 5);
            R[0] = gsu_ldw(R[8]);
            gsu_code(0x01a64f, 1);
            gsu_stw(R[9], R[0]);
            R[9] = gsu_inc(R[9]);
            R[9] = gsu_inc(R[9]);
            R[8] = gsu_inc(R[8]);
            go = gsu_count_down(&R[12]);
            R[8] = gsu_inc(R[8]);
        } while (gsu_code_loop(0x01a647, 1, gsu_code_loop(0x01a650, 4, go)));
        gsu_code(0x01a650, 4);
        gsu_code(0x01a654, 4);
        R[0] = gsu_ldw(FX_COUNT_OUT);
    } else { gsu_code(0x01a63c, 4); gsu_code(0x01a657, 1); }
    gsu_code(0x01a658, 4);
    gsu_pull(&R[10]);
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
    int minus;

    gsu_code(0x01a932, 3);
    R[1] = gsu_ldw(FX_LIST_END);
    R[0] = DRAW_LIST;
    R[0] = gsu_sub(R[0], R[1], 0);
    if (gsu.z)
        { gsu_code(0x01a935, 8); return; } else { gsu_code(0x01a935, 6); gsu_code(0x01a93d, 1); }
    gsu_code(0x01a93e, 2);
    gsu_push(&R[10], R[11]);
    gsu_code(0x01a940, 5);
    gsu_stw(FX_LIST_STACK, R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_code(0x01a945, 2);
    gsu_stw(R[1], R[0]);
    R[1] = DRAW_LIST;
    gsu_code(0x01a947, 6);
    gsu_stw(FX_LIST_END, R[1]);
    R[0] = 0;
    gsu_code(0x01a94d, 4);
    gsu_romb(R[0]);
    for (;;) {
        /* A950 */
        gsu_code(0x01a951, 3);
        R[1] = gsu_ldw(FX_LIST_END);
        gsu_code(0x01a954, 2);
        R[0] = gsu_ldb(R[1]);
        R[0] = gsu_sex(R[0]);
        R[1] = gsu_inc(R[1]);
        R[0] = gsu_sub(R[0], 3, 0);
        minus = gsu.s;
        R[0] = gsu_inc(R[0]);                   /* BMI's delay slot */
        if (!minus) {
            gsu_code(0x01a956, 6);
            flat_polygon(GSU_COPY(0));
            continue;
        } else { gsu_code(0x01a956, 6); gsu_code(0x01a90e, 1); }
        /* A90E */
        if (gsu.z) {
            gsu_code(0x01a90f, 3);
            line(GSU_COPY(0));
            continue;
        } else { gsu_code(0x01a90f, 2); gsu_code(0x01a915, 1); }
        R[0] = gsu_inc(R[0]);
        R[0] = gsu_inc(R[0]);
        {
            int end = gsu.z;

            R[0] = gsu_not(R[0]);               /* BEQ's delay slot */
            if (!end) {
                gsu_code(0x01a916, 4);
                tex_polygon(GSU_COPY(0));
                gsu_code(0x01a90a, 3); gsu_code(0x01a950, 1);
                continue;
            } else { gsu_code(0x01a916, 4); gsu_code(0x01a91e, 1); }
        }
        /* A91E */
        R[1] = DRAW_LIST;
        gsu_code(0x01a91f, 6);
        gsu_stw(FX_LIST_END, R[1]);
        gsu_code(0x01a925, 3);
        R[0] = gsu_ldw(FX_ROM_BANK);
        gsu_code(0x01a928, 2);
        gsu_romb(R[0]);
        gsu_code(0x01a92a, 3);
        R[10] = gsu_ldw(FX_LIST_STACK);
        gsu_code(0x01a92d, 4);
        gsu_pull(&R[10]);
        return;
    }
}
