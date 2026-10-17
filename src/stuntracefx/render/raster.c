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
static void reciprocal_at(uint16_t n)
{
    R[0] = RECIPROCALS;
    R[0] = gsu_add(R[0], n, 0);
    gsu_r14(gsu_add(R[0], n, 0));
}

static void reciprocal_read(void)
{
    R[6] = gsu_getb();
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
static void edge_step(int x, uint16_t step, int rows)
{
    int plus;

    R[x] = gsu_add(R[x], step, 0);
    plus = !gsu.s;
    if (rows >= 0)
        R[rows] = gsu_dec(R[rows]);
    if (plus)
        return;
    R[0] = gsu_add(0x0800, R[x], 0);
    if (gsu.s)
        return;
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

/* The vertices of an entry copied once more below it and the top and
   bottom rows found: R5 walks the n vertices from R1 (STRIDE bytes each),
   R4 writes x and y STRIDE * n bytes lower; R2 = the smallest y (from
   $03FF) and R6 where its copy went, R8 the largest (from 0). A LOOP of
   R12 = n (R13 = its start). */
static void flat_copy(void)
{
    int plus;

    R[13] = 0xA981;
    do {
        R[0] = gsu_ldb(R[5]);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_ldb(R[5]);
        gsu_stb(R[4], (uint8_t)R[0]);
        gsu_sub(R[0], R[2], 0);                 /* CMP R2 */
        plus = !gsu.s;
        R[5] = gsu_inc(R[5]);                   /* BPL's delay slot */
        if (!plus) {
            R[2] = R[0];
            R[6] = R[4];
        }
        R[0] = gsu_sub(R[0], R[8], 0);
        plus = !gsu.s;
        R[5] = gsu_inc(R[5]);                   /* BMI's delay slot */
        if (plus)
            R[8] = gsu_add(R[0], R[8], 0);
        R[4] = gsu_inc(R[4]);
        plus = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);                   /* LOOP's delay slot */
    } while (plus);
}

/* AA74: a polygon less than two rows high, drawn as one row at the top
   y (R2) from the smallest x to the largest (not including it). R4 is
   at the vertices again after the copy. Back to the list after. */
static void flat_thin(void)
{
    int go;

    R[0] = gsu_lsr(R[3]);
    R[12] = gsu_lsr(R[0]);
    R[1] = 0x03FF;
    R[8] = 0;
    R[13] = 0xAA7F;
    do {
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
    } while (go);
    R[12] = gsu_sub(R[8], R[1], 0);
    if (gsu.s || gsu.z)
        return;
    R[13] = 0xAA9C;
    do {
        go = gsu_count_down(&R[12]);
        gsu_plot_r1();
    } while (go);
}

/* A9D0 (from A9CE): the next left edge. R8 = the current x, R7 = the
   next vertex's x, its y less the row R2 in R11; edges with no height are
   passed. Returns 0 when the polygon has ended (the next vertex is above
   the row), else 1 with R7 = x in 8.8 and R8 its slope. */
static int flat_left_edge(int from_a9ce)
{
    for (;;) {
        if (from_a9ce)
            R[8] = R[7];
        from_a9ce = 1;
        R[7] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[0] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[11] = gsu_sub(R[0], R[2], 0);
        if (gsu.s)
            return 0;
        {
            int zero = gsu.z;

            R[6] = gsu_inc(R[6]);               /* BEQ's delay slot */
            if (!zero)
                break;
        }
    }
    reciprocal_at(R[11]);
    gsu_stw(FX_LEFT_AT, R[6]);
    R[0] = gsu_sub(R[7], R[8], 0);
    reciprocal_read();
    R[7] = gsu_swap(R[8]);
    R[6] = gsu_getbh(R[6]);
    R[8] = slope();
    return 1;
}

/* AA0D: the next right edge, walking back from R5: R10 = the current x,
   R9 = the next x, its y less R2 in R3. 0 when the polygon has ended. */
static int flat_right_edge(void)
{
    for (;;) {
        R[10] = R[9];
        R[0] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        R[5] = gsu_dec(R[5]);
        R[9] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        R[3] = gsu_sub(R[0], R[2], 0);
        if (gsu.s)
            return 0;
        {
            int zero = gsu.z;

            R[5] = gsu_dec(R[5]);               /* BEQ's delay slot */
            if (!zero)
                break;
        }
    }
    reciprocal_at(R[3]);
    R[0] = gsu_sub(R[9], R[10], 0);
    reciprocal_read();
    R[9] = gsu_swap(R[10]);
    R[6] = gsu_getbh(R[6]);
    R[10] = slope();
    return 1;
}

/* A95C: a flat polygon (R0 = its type less 2, R1 at its colour byte):
   the vertices copied, then rows from the top down, each from the left
   edge's x to the right's, both rounded (+ $7F) when an edge starts. */
static void flat_polygon(void)
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
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_add(R[1], R[3], 0);
    gsu_stw(FX_LIST_END, R[0]);
    R[5] = R[1];
    R[4] = gsu_sub(R[1], R[3], 0);
    R[2] = 0x03FF;
    R[8] = 0;
    flat_copy();
    R[0] = gsu_sub(R[8], R[2], 0);
    R[0] = gsu_dec(R[0]);
    R[0] = gsu_dec(R[0]);
    if (gsu.s) {
        flat_thin();
        return;
    }
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
    if (!flat_left_edge(0))
        return;
    for (;;) {
        /* AA03 */
        gsu_moves(R[3]);
        if (gsu.z) {
            R[0] = gsu_add(0x007F, R[9], 0);
            R[9] = gsu_hib(R[0]);
            if (!flat_right_edge())
                return;
        }
        /* AA3F: the rows while both edges last */
        do {
            R[1] = gsu_hib(R[7]);
            R[0] = gsu_hib(R[9]);
            R[12] = gsu_sub(R[0], R[1], 0);
            go = !gsu.s;
            R[12] = gsu_inc(R[12]);             /* BMI's delay slot */
            if (go) {
                do {
                    go = gsu_count_down(&R[12]);
                    gsu_plot_r1();
                } while (go);
            }
            edge_step(7, R[8], 11);
            edge_step(9, R[10], 3);
            R[0] = gsu_and(R[11], R[3]);
            go = !gsu.z;
            R[2] = gsu_inc(R[2]);               /* BNE's delay slot */
        } while (go);
        /* A9C1 */
        gsu_moves(R[11]);
        if (!gsu.z)
            continue;
        R[0] = gsu_add(0x007F, R[7], 0);
        R[7] = gsu_hib(R[0]);
        R[6] = gsu_ldw(FX_LEFT_AT);
        if (!flat_left_edge(1))
            return;
    }
}

/* AAA2: a line (R1 at its colour byte), stepped along its longer axis;
   R0 is the error term, the carry from its SUB picks the step across. */
static void line(void)
{
    int carry, go;

    R[0] = 0x0070;
    gsu_color((uint8_t)R[0]);
    R[0] = 0;
    gsu_cmode(R[0]);
    R[7] = R[1];
    R[0] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_color((uint8_t)R[0]);
    R[1] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[2] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[3] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    R[4] = gsu_ldb(R[7]);
    R[7] = gsu_inc(R[7]);
    R[7] = gsu_inc(R[7]);
    gsu_stw(FX_LIST_END, R[7]);
    R[5] = 0x0001;
    R[3] = gsu_sub(R[3], R[1], 0);
    if (gsu.s) {
        R[5] = 0xFFFF;
        R[3] = gsu_not(R[3]);
        R[3] = gsu_inc(R[3]);
    }
    R[6] = 0x0001;
    R[4] = gsu_sub(R[4], R[2], 0);
    if (gsu.s) {
        R[6] = 0xFFFF;
        R[4] = gsu_not(R[4]);
        R[4] = gsu_inc(R[4]);
    }
    R[0] = gsu_sub(R[3], R[4], 0);
    if (!gsu.s) {
        R[12] = R[3];
        R[12] = gsu_inc(R[12]);
        R[0] = gsu_lsr(R[0]);
        R[0] = gsu_sub(R[0], R[4], 0);
        R[13] = 0xAAEA;
        do {
            carry = gsu.cy;
            gsu_plot_r1();                      /* BCS's delay slot */
            if (!carry) {
                R[0] = gsu_add(R[0], R[3], 0);
                R[2] = gsu_add(R[2], R[6], 0);
            }
            R[1] = gsu_dec(R[1]);
            R[1] = gsu_add(R[1], R[5], 0);
            go = gsu_count_down(&R[12]);
            R[0] = gsu_sub(R[0], R[4], 0);      /* LOOP's delay slot */
        } while (go);
        return;
    }
    R[12] = R[4];
    R[12] = gsu_inc(R[12]);
    R[0] = gsu_not(R[0]);
    R[0] = gsu_inc(R[0]);
    R[0] = gsu_lsr(R[0]);
    R[0] = gsu_sub(R[0], R[3], 0);
    R[13] = 0xAB02;
    do {
        carry = gsu.cy;
        gsu_plot_r1();
        if (!carry) {
            R[0] = gsu_add(R[0], R[4], 0);
            R[1] = gsu_add(R[1], R[5], 0);
        }
        R[1] = gsu_dec(R[1]);
        R[2] = gsu_add(R[2], R[6], 0);
        go = gsu_count_down(&R[12]);
        R[0] = gsu_sub(R[0], R[3], 0);
    } while (go);
}

/* A7D9 / A7E2: the next left edge of a textured polygon from the walk at
   $0060, with u and v ($0070, $007C) kept as bytes before it and their
   steps per row. 0 when the polygon has ended. */
static int tex_left_edge(void)
{
    R[0] = gsu_add(0x007F, R[9], 0);
    R[9] = gsu_hib(R[0]);
    R[0] = gsu_ldw(FX_LEFT_U);
    R[0] = gsu_hib(R[0]);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(FX_LEFT_V);
    R[0] = gsu_hib(R[0]);
    gsu_sbk(R[0]);
    R[6] = gsu_ldw(FX_LEFT_AT);
    for (;;) {
        R[12] = R[9];
        R[0] = gsu_ldw(FX_LEFT_U);
        gsu_stw(FX_LEFT_U_PREV, R[0]);
        R[0] = gsu_ldw(FX_LEFT_V);
        gsu_stw(FX_LEFT_V_PREV, R[0]);
        R[9] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[4] = gsu_ldb(R[6]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[0] = gsu_ldb(R[6]);
        gsu_stw(FX_LEFT_U, R[0]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[0] = gsu_ldb(R[6]);
        gsu_stw(FX_LEFT_V, R[0]);
        R[6] = gsu_inc(R[6]);
        R[6] = gsu_inc(R[6]);
        R[11] = gsu_sub(R[4], R[2], 0);
        if (!gsu.z)
            break;
    }
    if (gsu.s)
        return 0;
    gsu_stw(FX_LEFT_AT, R[6]);
    reciprocal_at(R[11]);
    reciprocal_read();
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_sub(R[9], R[12], 0);
    R[9] = gsu_swap(R[12]);
    R[7] = slope();
    R[0] = gsu_ldw(FX_LEFT_U);
    R[1] = gsu_ldw(FX_LEFT_U_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_LEFT_DU, uv_step());
    R[0] = gsu_ldw(FX_LEFT_U_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_LEFT_U, R[0]);
    R[0] = gsu_ldw(FX_LEFT_V);
    R[1] = gsu_ldw(FX_LEFT_V_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_LEFT_DV, uv_step());
    R[0] = gsu_ldw(FX_LEFT_V_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_LEFT_V, R[0]);
    return 1;
}

/* A879: the next right edge, walking back from $0062. */
static int tex_right_edge(void)
{
    R[0] = gsu_add(0x007F, R[10], 0);
    R[10] = gsu_hib(R[0]);
    R[0] = gsu_ldw(FX_RIGHT_U);
    R[0] = gsu_hib(R[0]);
    gsu_sbk(R[0]);
    R[0] = gsu_ldw(FX_RIGHT_V);
    R[0] = gsu_hib(R[0]);
    gsu_sbk(R[0]);
    R[6] = gsu_ldw(FX_RIGHT_AT);
    for (;;) {
        R[12] = R[10];
        R[0] = gsu_ldw(FX_RIGHT_U);
        gsu_stw(FX_RIGHT_U_PREV, R[0]);
        R[0] = gsu_ldw(FX_RIGHT_V);
        gsu_stw(FX_RIGHT_V_PREV, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[0] = gsu_ldb(R[6]);
        gsu_stw(FX_RIGHT_V, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[0] = gsu_ldb(R[6]);
        gsu_stw(FX_RIGHT_U, R[0]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[0] = gsu_ldb(R[6]);
        R[6] = gsu_dec(R[6]);
        R[6] = gsu_dec(R[6]);
        R[10] = gsu_ldb(R[6]);
        R[3] = gsu_sub(R[0], R[2], 0);
        if (!gsu.z)
            break;
    }
    if (gsu.s)
        return 0;
    gsu_stw(FX_RIGHT_AT, R[6]);
    reciprocal_at(R[3]);
    reciprocal_read();
    R[6] = gsu_getbh(R[6]);
    R[0] = gsu_sub(R[10], R[12], 0);
    R[10] = gsu_swap(R[12]);
    R[8] = slope();
    R[0] = gsu_ldw(FX_RIGHT_U);
    R[1] = gsu_ldw(FX_RIGHT_U_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_RIGHT_DU, uv_step());
    R[0] = gsu_ldw(FX_RIGHT_U_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_RIGHT_U, R[0]);
    R[0] = gsu_ldw(FX_RIGHT_V);
    R[1] = gsu_ldw(FX_RIGHT_V_PREV);
    R[0] = gsu_sub(R[0], R[1], 0);
    gsu_stw(FX_RIGHT_DV, uv_step());
    R[0] = gsu_ldw(FX_RIGHT_V_PREV);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_RIGHT_V, R[0]);
    return 1;
}

/* One edge's u or v for the row: RIGHT += its step (LMS R0, step; LMS
   R4, right; ADD R4; SBK), the step across the row from right less left
   (FMULT by R6 = 1/width, ROL) to dst, then LEFT += its step. */
static uint16_t tex_across(uint16_t step_r, uint16_t right, uint16_t left, int lr, uint16_t step_l)
{
    uint16_t d;

    R[0] = gsu_ldw(step_r);
    R[4] = gsu_ldw(right);
    R[0] = gsu_add(R[0], R[4], 0);
    gsu_sbk(R[0]);
    R[lr] = gsu_ldw(left);
    R[0] = gsu_sub(R[4], R[lr], 0);
    R[0] = gsu_fmult(R[0], R[6]);
    d = gsu_rol(R[0]);
    R[0] = gsu_ldw(step_l);
    R[0] = gsu_add(R[0], R[lr], 0);
    gsu_stw(left, R[0]);
    return d;
}

/* A710-A7D2: the rows of a textured polygon while both edges last (the
   shorter's count in R2's high byte), each row's texels read with MERGE
   of v (R7) and u (R8), the mask and the texture's address, GETC and
   PLOT; the edges' x stepped (as on a flat polygon) and the rows left on
   each kept at $0064/$0066. */
static void tex_rows(void)
{
    int go;

    gsu_stw(FX_LEFT_SLOPE, R[7]);
    gsu_stw(FX_RIGHT_SLOPE, R[8]);
    R[0] = gsu_sub(R[11], R[3], 0);
    if (!gsu.s) {
        gsu_stw(FX_LEFT_ROWS, R[0]);
        R[0] = gsu_swap(R[3]);
        R[2] = gsu_or(R[0], R[2]);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_stw(FX_RIGHT_ROWS, R[0]);
    } else {
        R[0] = gsu_sub(R[3], R[11], 0);
        gsu_stw(FX_RIGHT_ROWS, R[0]);
        R[0] = gsu_swap(R[11]);
        R[2] = gsu_or(R[0], R[2]);
        R[0] = gsu_sub(R[0], R[0], 0);
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
            R[12] = 0x0001;
        reciprocal_at(R[12]);
        reciprocal_read();
        R[6] = gsu_getbh(R[6]);
        R[5] = tex_across(FX_RIGHT_DU, FX_RIGHT_U, FX_LEFT_U, 8, FX_LEFT_DU);
        R[6] = tex_across(FX_RIGHT_DV, FX_RIGHT_V, FX_LEFT_V, 7, FX_LEFT_DV);
        R[4] = gsu_ldw(FX_TEX_U_OFF);
        R[8] = gsu_add(R[8], R[4], 0);
        R[4] = gsu_ldw(FX_TEX_V_OFF);
        R[7] = gsu_add(R[7], R[4], 0);
        R[0] = gsu_ldw(FX_TEX_BANK);
        R[0] = gsu_lob(R[0]);
        gsu_romb(R[0]);
        R[4] = gsu_ldw(FX_TEX_BASE);
        R[3] = gsu_ldw(FX_TEX_MASK);
        R[13] = 0xA794;
        do {
            R[0] = gsu_merge();
            R[0] = gsu_and(R[0], R[3]);
            gsu_r14(gsu_add(R[0], R[4], 0));
            R[7] = gsu_add(R[7], R[6], 0);
            R[8] = gsu_add(R[8], R[5], 0);
            gsu_color(gsu_getb());              /* GETC */
            go = gsu_count_down(&R[12]);
            gsu_plot_r1();
        } while (go);
        R[0] = gsu_sub(R[0], R[0], 0);
        gsu_romb(R[0]);
        R[7] = gsu_ldw(FX_LEFT_SLOPE);
        R[8] = gsu_ldw(FX_RIGHT_SLOPE);
        edge_step(9, R[7], -1);
        edge_step(10, R[8], -1);
        R[2] = gsu_inc(R[2]);
        R[0] = gsu_swap(R[2]);
        R[0] = gsu_dec(R[0]);
        R[2] = gsu_swap(R[0]);
        R[0] = gsu_hib(R[2]);
    } while (!gsu.z);
    R[11] = gsu_ldw(FX_LEFT_ROWS);
    R[3] = gsu_ldw(FX_RIGHT_ROWS);
}

/* A662: a textured polygon (R0 = NOT its type, R1 at its bank byte). The
   vertices are copied below it as for a flat polygon, u and v of each
   moved down a byte where they are; a polygon with no height is passed.
   Then the edges from the top vertex down. */
static void tex_polygon(void)
{
    int go;

    R[0] = gsu_lob(R[0]);
    R[12] = R[0];
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[0] = gsu_add(R[0], R[0], 0);
    R[3] = R[0];
    R[0] = gsu_ldb(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_stw(FX_TEX_BANK, R[0]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_cmode(R[0]);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_color((uint8_t)R[0]);
    R[0] = gsu_swap(R[0]);
    gsu_cmode(R[0]);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_stw(FX_TEX_BASE, R[0]);
    R[0] = gsu_ldw(R[1]);
    R[1] = gsu_inc(R[1]);
    R[1] = gsu_inc(R[1]);
    gsu_stw(FX_TEX_MASK, R[0]);
    R[0] = gsu_add(R[1], R[3], 0);
    gsu_stw(FX_LIST_END, R[0]);
    R[5] = R[1];
    R[4] = gsu_sub(R[1], R[3], 0);
    R[2] = 0x03FF;
    R[8] = 0;
    R[13] = 0xA697;
    do {
        R[0] = gsu_ldb(R[5]);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_ldb(R[5]);
        gsu_stb(R[4], (uint8_t)R[0]);
        gsu_sub(R[0], R[2], 0);                 /* CMP R2 */
        if (gsu.s) {
            R[2] = R[0];
            R[6] = R[4];
        }
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
        gsu_stb(R[5], (uint8_t)R[0]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        R[4] = gsu_inc(R[4]);
        R[5] = gsu_inc(R[5]);
        R[0] = gsu_ldb(R[5]);
        R[5] = gsu_dec(R[5]);
        gsu_stb(R[5], (uint8_t)R[0]);
        R[5] = gsu_inc(R[5]);
        R[5] = gsu_inc(R[5]);
        gsu_stb(R[4], (uint8_t)R[0]);
        R[4] = gsu_inc(R[4]);
        go = gsu_count_down(&R[12]);
        R[4] = gsu_inc(R[4]);
    } while (go);
    R[0] = gsu_sub(R[2], R[8], 0);
    if (gsu.z)
        return;
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
    R[0] = gsu_ldb(R[4]);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_LEFT_U, R[0]);
    gsu_stw(FX_RIGHT_U, R[0]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    R[0] = gsu_ldb(R[4]);
    R[0] = gsu_swap(R[0]);
    gsu_stw(FX_LEFT_V, R[0]);
    gsu_stw(FX_RIGHT_V, R[0]);
    R[4] = gsu_inc(R[4]);
    R[4] = gsu_inc(R[4]);
    gsu_stw(FX_LEFT_AT, R[4]);
    gsu_stw(FX_RIGHT_AT, R[5]);
    R[11] = 0;
    R[3] = 0;
    for (;;) {
        /* A7D9 */
        gsu_moves(R[11]);
        R[0] = R[11];
        if (gsu.z && !tex_left_edge())
            return;
        /* A870 */
        gsu_moves(R[3]);
        R[0] = R[3];
        if (gsu.z && !tex_right_edge())
            return;
        tex_rows();
    }
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

    R[1] = gsu_ldw(FX_LIST_END);
    R[0] = DRAW_LIST;
    R[0] = gsu_sub(R[0], R[1], 0);
    if (gsu.z)
        return;
    gsu_push(&R[10], R[11]);
    gsu_stw(FX_LIST_STACK, R[10]);
    R[0] = gsu_sub(R[0], R[0], 0);
    gsu_stw(R[1], R[0]);
    R[1] = DRAW_LIST;
    gsu_stw(FX_LIST_END, R[1]);
    R[0] = 0;
    gsu_romb(R[0]);
    for (;;) {
        /* A950 */
        R[1] = gsu_ldw(FX_LIST_END);
        R[0] = gsu_ldb(R[1]);
        R[0] = gsu_sex(R[0]);
        R[1] = gsu_inc(R[1]);
        R[0] = gsu_sub(R[0], 3, 0);
        minus = gsu.s;
        R[0] = gsu_inc(R[0]);                   /* BMI's delay slot */
        if (!minus) {
            flat_polygon();
            continue;
        }
        /* A90E */
        if (gsu.z) {
            line();
            continue;
        }
        R[0] = gsu_inc(R[0]);
        R[0] = gsu_inc(R[0]);
        {
            int end = gsu.z;

            R[0] = gsu_not(R[0]);               /* BEQ's delay slot */
            if (!end) {
                tex_polygon();
                continue;
            }
        }
        /* A91E */
        R[1] = DRAW_LIST;
        gsu_stw(FX_LIST_END, R[1]);
        R[0] = gsu_ldw(FX_ROM_BANK);
        gsu_romb(R[0]);
        R[10] = gsu_ldw(FX_LIST_STACK);
        gsu_pull(&R[10]);
        return;
    }
}
