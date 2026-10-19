/*
 * polygon.c: the Super FX taking a polygon from its points to the screen.
 * The points are gathered into vertex records of five words (x, y, z and
 * two attributes) in one of two lists, $0688 and $082A: the word at $0060
 * holds the list being read, $0062 the one being written, $0040 the
 * vertex count. Each clip against a screen edge or the near plane writes
 * the clipped polygon into the other list, and the two are swapped. The
 * projected vertices go out four words each (screen x, y and the two
 * attributes doubled) at the address in $005A, with an outcode word per
 * vertex ORed into $014A.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_COUNT 0x0040u    /* the vertex count */
#define FX_LIST_IN 0x0060u  /* the list being read */
#define FX_LIST_OUT 0x0062u /* the list being written */
#define FX_OUT 0x005Au      /* where the projected vertices go */
#define FX_CODES 0x014Au    /* the outcodes ORed together; the edges to clip */
#define FX_POINTS_N 0x0146u /* the point count: FxPolygonProject leaves $0040 there */
#define FX_COUNT_SAVE 0x0098u
#define FX_LIST_A 0x0688u
#define FX_LIST_B 0x082Au
#define FX_EDGES 0x0038u    /* X at $38 and $3A, Y at $3C and $3E */

void abi_FxProjectVertex(void);
void abi_FxPointsGather(void);
void abi_FxPolygonClipTop(void);
void abi_FxPolygonClipBottom(void);
void abi_FxPolygonClipRight(void);
void abi_FxPolygonClipLeft(void);
void abi_FxPolygonClipNear(void);

/* LMS R1, ($60); LMS R2, ($62); SMS ($60), R2; SMS ($62), R1. */
/* gsuwalk: the code of each copy of lists_swap, read from the ROM */
static const GsuCode lists_swap_fetch[7][4] = {
    {{0x01c28e, 3}, {0x01c291, 3}, {0x01c294, 3}, {0x01c297, 3}},
    {{0x01c2b9, 3}, {0x01c2bc, 3}, {0x01c2bf, 3}, {0x01c2c2, 3}},
    {{0x01c326, 3}, {0x01c329, 3}, {0x01c32c, 3}, {0x01c32f, 3}},
    {{0x01c375, 3}, {0x01c378, 3}, {0x01c37b, 3}, {0x01c37e, 3}},
    {{0x01c399, 3}, {0x01c39c, 3}, {0x01c39f, 3}, {0x01c3a2, 3}},
    {{0x01c3bd, 3}, {0x01c3c0, 3}, {0x01c3c3, 3}, {0x01c3c6, 3}},
    {{0x01c3e1, 3}, {0x01c3e4, 3}, {0x01c3e7, 3}, {0x01c3ea, 3}},
};

static void lists_swap(int copy)
{
    gsu_codes(lists_swap_fetch[copy], 1);
    gsu.r[1] = gsu_ldw(FX_LIST_IN);
    gsu_codes(lists_swap_fetch[copy] + 1, 1);
    gsu.r[2] = gsu_ldw(FX_LIST_OUT);
    gsu_codes(lists_swap_fetch[copy] + 2, 1);
    gsu_stw(FX_LIST_IN, gsu.r[2]);
    gsu_codes(lists_swap_fetch[copy] + 3, 1);
    gsu_stw(FX_LIST_OUT, gsu.r[1]);
}

/* A word from R7 (LDW, INC, INC). */
/* gsuwalk: the code of each copy of next_in, read from the ROM */
static const GsuCode next_in_fetch[10][1] = {
    {{0x01c25b, 1}},
    {{0x01c25f, 3}},
    {{0x01c265, 4}},
    {{0x01c280, 1}},
    {{0x01c284, 3}},
    {{0x01c435, 2}},
    {{0x01c437, 4}},
    {{0x01c43b, 4}},
    {{0x01c471, 4}},
    {{0x01c47b, 4}},
};

static uint16_t next_in(int copy)
{
    gsu_codes(next_in_fetch[copy], 1);
    uint16_t v = gsu_ldw(gsu.r[7]);

    gsu.r[7] = gsu_inc(gsu_inc(gsu.r[7]));
    return v;
}

/* A word to R2 (STW, INC, INC). */
/* gsuwalk: the code of each copy of next_out, read from the ROM */
static const GsuCode next_out_fetch[5][1] = {
    {{0x01c25c, 3}},
    {{0x01c262, 3}},
    {{0x01c269, 4}},
    {{0x01c281, 3}},
    {{0x01c287, 3}},
};

static void next_out(uint16_t v, int copy)
{
    gsu_codes(next_out_fetch[copy], 1);
    gsu_stw(gsu.r[2], v);
    gsu.r[2] = gsu_inc(gsu_inc(gsu.r[2]));
}

/* A word to R8 (STW, INC, INC). */
/* gsuwalk: the code of each copy of next_r8, read from the ROM */
static const GsuCode next_r8_fetch[8][1] = {
    {{0x01c46b, 2}},
    {{0x01c46d, 4}},
    {{0x01c475, 6}},
    {{0x01c47f, 6}},
    {{0x01c2eb, 3}},
    {{0x01c2f1, 3}},
    {{0x01c2f7, 3}},
    {{0x01c2fd, 3}},
};

static void next_r8(uint16_t v, int copy)
{
    gsu_codes(next_r8_fetch[copy], 1);
    gsu_stw(gsu.r[8], v);
    gsu.r[8] = gsu_inc(gsu_inc(gsu.r[8]));
}

/* R4 the largest z so far, R5 the smallest (SUB, BPL / BMI on S alone). */
/* gsuwalk: the code of each copy of z_range, read from the ROM */
static const GsuCode z_range_fetch[2][7] = {
    {{0x01c23d, 4}, {0x01c246, 1}, {0x01c23d, 9}, {0x01c24d, 1}, {0x01c247, 4}, {0x01c24d, 1}, {0x01c247, 7}},
    {{0x01c26d, 6}, {0x01c278, 1}, {0x01c26d, 11}, {0x01c27f, 1}, {0x01c279, 4}, {0x01c27f, 1}, {0x01c279, 7}},
};

static void z_range(int copy)
{
    gsu.r[0] = gsu_sub(gsu.r[4], gsu.r[6], 0);
    if (gsu.s) {
        gsu.r[4] = gsu.r[6];
        gsu_codes(z_range_fetch[copy] + 2, 2);
        return;
    } else { gsu_codes(z_range_fetch[copy], 2); }
    gsu.r[0] = gsu_sub(gsu.r[5], gsu.r[6], 0);
    if (!gsu.s)
        { gsu.r[5] = gsu.r[6]; gsu_codes(z_range_fetch[copy] + 6, 1); } else { gsu_codes(z_range_fetch[copy] + 4, 2); }
}

/*
 * @routine 01:C412 FxPolygonProject
 * WHAT IT DOES: Projects the $0040 vertices of the list at $0060 to the
 * screen (FxProjectVertex), each one's screen position pulled in to the
 * edge it is past, and writes them out at the word at $005A: screen x,
 * screen y, then the vertex's two attributes doubled. The outcode words
 * are ORed into $014A, which is cleared again at the end; $0146 = the
 * count.
 * HOW: Pushes R11 on the R10 stack. CACHE. R3, R13, R9 from $0026-$002A
 * (as FxProjectVertex's near points leave them); ROM bank $14, R14 saved
 * at $001A. For each vertex: X, Y, Z to R1, R2, R6 and FxProjectVertex;
 * its R0 pushed, then AND #15 and four LSRs in the branches' delay slots
 * test bits 0-3: Y to $003C or $003E, X to 0 (after a load of $0038 that
 * is overwritten) or $003A. The outcode back from the stack, ORed with
 * $014A and stored there by SBK in the loop's delay slot. Then the ROM
 * bank from $001C and R14 from $001A; returns by pulling R15.
 * Entry: R10 = stack; R11 = return address. Exit: R0 = the count; R7
 * past the last vertex read; R8 past the last written; R12 = 0; R6 =
 * $014A as read last; R14 as saved; S and Z from R10; the last RAM
 * address R10. R14 and the ROM buffer move as on the Super FX.
 */
void abi_FxPolygonProject(void)
{
    int c;

    gsu_code(0x01c413, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    
    gsu_code(0x01c415, 2); gsu_cache(0xc417); gsu_code(0x01c417, 4);
    gsu.r[3] = gsu_ldw(0x0026);
    gsu_code(0x01c41b, 3);
    gsu.r[13] = gsu_ldw(0x0028);
    gsu_code(0x01c41e, 3);
    gsu.r[9] = gsu_ldw(0x002A);
    gsu.r[0] = 0x0014;
    gsu_code(0x01c421, 4);
    gsu_romb(0x14);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01c425, 4);
    gsu_stw(FX_CODES, gsu.r[0]);
    gsu_code(0x01c429, 3);
    gsu_stw(0x001A, gsu.r[14]);
    gsu_code(0x01c42c, 3);
    gsu.r[12] = gsu_ldw(FX_COUNT);
    gsu_code(0x01c42f, 3);
    gsu.r[7] = gsu_ldw(FX_LIST_IN);
    gsu_code(0x01c432, 3);
    gsu.r[8] = gsu_ldw(FX_OUT);
    do {
        gsu.r[1] = next_in(GSU_COPY(5));
        gsu.r[2] = next_in(GSU_COPY(6));
        gsu.r[6] = next_in(GSU_COPY(7));
        gsu.r[11] = 0xC445;
        gsu_code(0x01c43f, 6); gsu_code(0x018bff, 1);
        abi_FxProjectVertex();
        gsu_code(0x01c445, 2);
        gsu_push(&gsu.r[10], gsu.r[0]);
        gsu.r[0] = gsu_and(gsu.r[0], 15);
        c = !gsu.z;
        gsu.r[0] = gsu_lsr(gsu.r[0]);       /* BEQ's delay slot */
        if (c) {
            
            c = gsu.cy;
            gsu.r[0] = gsu_lsr(gsu.r[0]);
            if (c)
                { gsu_code(0x01c447, 13);
                gsu.r[2] = gsu_ldw(FX_EDGES + 4); } else { gsu_code(0x01c447, 9); gsu_code(0x01c453, 1); }
            
            c = gsu.cy;
            gsu.r[0] = gsu_lsr(gsu.r[0]);
            if (c)
                { gsu_code(0x01c454, 6);
                gsu.r[2] = gsu_ldw(FX_EDGES + 6); } else { gsu_code(0x01c454, 2); gsu_code(0x01c459, 1); }
            c = gsu.cy;
            gsu.r[0] = gsu_lsr(gsu.r[0]);
            if (c) {
                gsu_code(0x01c45a, 6);
                gsu.r[1] = gsu_ldw(FX_EDGES);   /* LMS, then IBT R1, #0 */
                gsu.r[1] = 0;
                gsu_code(0x01c460, 2);
            } else { gsu_code(0x01c45a, 2); gsu_code(0x01c461, 1); }
            
            if (gsu.cy)
                { gsu_code(0x01c462, 6);
                gsu.r[1] = gsu_ldw(FX_EDGES + 2); } else { gsu_code(0x01c462, 2); gsu_code(0x01c467, 1); }
        } else { gsu_code(0x01c447, 6); gsu_code(0x01c467, 1); }
        gsu_code(0x01c468, 3);
        gsu.r[0] = gsu_pull(&gsu.r[10]);
        next_r8(gsu.r[1], GSU_COPY(0));
        next_r8(gsu.r[2], GSU_COPY(1));
        gsu.r[6] = next_in(GSU_COPY(8));
        gsu.r[6] = gsu_add(gsu.r[6], gsu.r[6], 0);
        next_r8(gsu.r[6], GSU_COPY(2));
        gsu.r[6] = next_in(GSU_COPY(9));
        gsu.r[6] = gsu_add(gsu.r[6], gsu.r[6], 0);
        next_r8(gsu.r[6], GSU_COPY(3));
        gsu_code(0x01c485, 5);
        gsu.r[6] = gsu_ldw(FX_CODES);
        gsu.r[0] = gsu_or(gsu.r[0], gsu.r[6]);
        gsu.r[12] = gsu_dec(gsu.r[12]);
        c = !gsu.z;
        gsu_code(0x01c48a, 4); gsu_code((c) ? 0x01c434 : 0x01c48e, 1);   /* the branch's delay slot */
        gsu_sbk(gsu.r[0]);                  /* BNE's delay slot */
    } while (c);
    gsu_code(0x01c48f, 3);
    gsu.r[0] = gsu_ldw(0x001C);
    gsu_code(0x01c492, 2);
    gsu_romb(gsu.r[0]);
    gsu_code(0x01c494, 3);
    gsu_r14(gsu_ldw(0x001A));
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01c497, 4);
    gsu_stw(FX_CODES, gsu.r[0]);
    gsu_code(0x01c49b, 3);
    gsu.r[0] = gsu_ldw(FX_COUNT);
    gsu_code(0x01c49e, 3);
    gsu_stw(FX_POINTS_N, gsu.r[0]);
    gsu_code(0x01c4a1, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

/*
 * @routine 01:C21E FxPolygonNearClip
 * WHAT IT DOES: Splits a polygon that spans too much depth: when three
 * times its nearest z is less than its farthest, it is clipped against the
 * near plane at (farthest + 3 * nearest) / 4 (FxPolygonClipNear), the
 * first byte of the 8-byte entry below the output ($005A - 8, the
 * polygon's header) set to the clipped count's complement, the near part
 * projected (FxPolygonProject) and the header copied again after it; the
 * polygon is then clipped against the same plane facing the other way
 * for the part beyond. R0 = the count left ($0040), or 0 when nothing is.
 * HOW: Pushes R11 on the R10 stack. R4 = the largest z (from $C180), R5
 * the smallest (from $3E80), over the $0040 vertices (LOOP, R13 = its
 * start); a polygon still in list $082A is first copied to the list at
 * $0062 as it is measured and the lists swapped. The count saved at
 * $0098. The plane, the clipped polygon's output start ($005A - 8) and
 * the plane again go on the R10 stack around the calls; the four words
 * at that start are copied to the end of the output, $005A moved past
 * them, the lists reset to $0688 in, $082A out, the count back from
 * $0098, and the plane negated for the second clip. Returns by pulling
 * R15.
 * Entry: R10 = stack; R11 = return address. Exit: R0 as above; R4, R5 =
 * the largest and smallest z; R13 the LOOP start; R12 = 0; R1, R2 = the
 * lists as swapped last; R8 past the output; S and Z from R10; the last
 * RAM address R10.
 */
/* gsuwalk: the code of each copy of abi_FxPolygonNearClip, read from the ROM */
static const GsuCode abi_FxPolygonNearClip_fetch[1][4] = {
    {{0x01c2e9, 2}, {0x01c2ee, 3}, {0x01c2f4, 3}, {0x01c2fa, 3}},
};

/* gsuwalk: the copy of next_r8 each copy of abi_FxPolygonNearClip calls, by pass */
static const uint8_t next_r8_in_abi_FxPolygonNearClip[1][4] = {{4, 5, 6, 7}};

void abi_FxPolygonNearClip(void)
{
    
    gsu_code(0x01c21f, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[5] = 0x3E80;
    gsu.r[4] = 0xC180;
    gsu_code(0x01c221, 11);
    gsu.r[12] = gsu_ldw(FX_COUNT);
    gsu.r[0] = FX_LIST_B;
    gsu_code(0x01c22c, 6);
    gsu.r[7] = gsu_ldw(FX_LIST_IN);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[7], 0);
    if (!gsu.z) {
        gsu.r[7] = gsu_add(gsu.r[7], 4, 0);
        gsu.r[13] = 0xC23A;
        gsu_code(0x01c232, 9);
        do {
            gsu_code(0x01c23b, 2);
            gsu.r[6] = gsu_ldw(gsu.r[7]);
            z_range(GSU_COPY(0));
            gsu.r[7] = gsu_add(gsu.r[7], 10, 0);
        } while (gsu_code_loop(0x01c23a, 1, gsu_code_loop(0x01c24e, 4, gsu_count_down(&gsu.r[12]))));
        gsu_code(0x01c24e, 4);
        
        gsu_code(0x01c252, 3); gsu_code(0x01c299, 1);
    } else {
        
        gsu_code(0x01c232, 3); gsu_code(0x01c255, 4);
        gsu.r[2] = gsu_ldw(FX_LIST_OUT);
        gsu.r[13] = 0xC25A;
        gsu_code(0x01c259, 2);
        do {
            gsu.r[0] = next_in(GSU_COPY(0));
            next_out(gsu.r[0], GSU_COPY(0));
            gsu.r[0] = next_in(GSU_COPY(1));
            next_out(gsu.r[0], GSU_COPY(1));
            gsu.r[6] = next_in(GSU_COPY(2));
            next_out(gsu.r[6], GSU_COPY(2));
            z_range(GSU_COPY(1));
            gsu.r[0] = next_in(GSU_COPY(3));
            next_out(gsu.r[0], GSU_COPY(3));
            gsu.r[0] = next_in(GSU_COPY(4));
            next_out(gsu.r[0], GSU_COPY(4));
        } while (gsu_code_loop(0x01c25a, 1, gsu_code_loop(0x01c28a, 3, gsu_count_down(&gsu.r[12]))));
        gsu_code(0x01c28a, 3);
        gsu_code(0x01c28d, 1);
        lists_swap(GSU_COPY(0));
    }
    gsu_code(0x01c29a, 3);
    gsu.r[0] = gsu_ldw(FX_COUNT);
    gsu_code(0x01c29d, 3);
    gsu_stw(FX_COUNT_SAVE, gsu.r[0]);
    gsu.r[0] = gsu_add(gsu.r[5], gsu.r[5], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
    gsu.r[6] = gsu_sub(gsu.r[0], gsu.r[4], 0);
    if (!gsu.s) {
        
        gsu_code(0x01c2a0, 11); gsu_code(0x01c331, 4);
        gsu.r[0] = gsu_ldw(FX_COUNT);
        gsu_code(0x01c335, 4);
        gsu_pull(&gsu.r[10]);
        return;
    } else { gsu_code(0x01c2a0, 7); gsu_code(0x01c2ab, 1); }
    gsu.r[0] = gsu_add(gsu.r[4], gsu.r[0], 0);
    gsu.r[0] = gsu_lsr(gsu.r[0]);
    gsu.r[0] = gsu_lsr(gsu.r[0]);
    gsu_code(0x01c2ac, 6);
    gsu_push(&gsu.r[10], gsu.r[0]);
    gsu.r[11] = 0xC2B8;
    gsu_code(0x01c2b2, 6); gsu_code(0x01ce34, 1);
    abi_FxPolygonClipNear();
    gsu_code(0x01c2b8, 1);
    lists_swap(GSU_COPY(1));
    gsu_code(0x01c2c5, 4);
    gsu.r[1] = gsu_pull(&gsu.r[10]);
    gsu_moves(gsu.r[0]);
    if (gsu.z) {
        gsu_code(0x01c2c9, 4); gsu_code(0x01c334, 5);
        gsu_pull(&gsu.r[10]);
        return;
    } else { gsu_code(0x01c2c9, 4); }
    gsu_code(0x01c2cd, 3);
    gsu_push(&gsu.r[10], gsu.r[1]);
    gsu_code(0x01c2d0, 5);
    gsu.r[2] = gsu_ldw(FX_OUT);
    gsu.r[2] = gsu_sub(gsu.r[2], 8, 0);
    gsu.r[0] = gsu_not(gsu.r[0]);
    gsu_code(0x01c2d5, 6);
    gsu_stb(gsu.r[2], (uint8_t)gsu.r[0]);
    gsu.r[0] = gsu_not(gsu.r[0]);
    gsu_code(0x01c2db, 3);
    gsu_push(&gsu.r[10], gsu.r[2]);
    gsu.r[11] = 0xC2E4;
    gsu_code(0x01c2de, 6); gsu_code(0x01c412, 1);
    (abi_FxPolygonProject());
    gsu_code(0x01c2e4, 5);
    gsu.r[1] = gsu_pull(&gsu.r[10]);
    {
        int i;

        for (i = 0; i < 4; i++) {
            gsu_codes(abi_FxPolygonNearClip_fetch[0] + i, 1);
            gsu.r[0] = gsu_ldw(gsu.r[1]);
            gsu.r[1] = gsu_inc(gsu_inc(gsu.r[1]));
            next_r8(gsu.r[0], GSU_COPY(next_r8_in_abi_FxPolygonNearClip[0][i]));
        }
    }
    gsu_code(0x01c300, 5);
    gsu_stw(FX_OUT, gsu.r[8]);
    gsu.r[0] = FX_LIST_B;
    gsu_code(0x01c305, 6);
    gsu_stw(FX_LIST_OUT, gsu.r[0]);
    gsu.r[0] = FX_LIST_A;
    gsu_code(0x01c30b, 6);
    gsu_stw(FX_LIST_IN, gsu.r[0]);
    gsu_code(0x01c311, 3);
    gsu.r[0] = gsu_ldw(FX_COUNT_SAVE);
    gsu_code(0x01c314, 3);
    gsu_stw(FX_COUNT, gsu.r[0]);
    gsu_code(0x01c317, 3);
    gsu.r[0] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = gsu_inc(gsu_not(gsu.r[0]));
    gsu.r[11] = 0xC320;
    gsu_code(0x01c31a, 6); gsu_code(0x01ce34, 1);
    abi_FxPolygonClipNear();
    gsu_moves(gsu.r[0]);
    if (!gsu.z) {
        gsu_code(0x01c320, 6);
        lists_swap(GSU_COPY(2));
        gsu_code(0x01c332, 3);
        gsu.r[0] = gsu_ldw(FX_COUNT);
    } else { gsu_code(0x01c320, 5); gsu_code(0x01c334, 1); }
    gsu_code(0x01c335, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

/* One edge clip of FxPolygonDraw: the clip, the lists swapped; 0 when
   nothing is left (R0 = 0), else 1 with R5 = $014A again (when reload). */
/* gsuwalk: the code of each copy of clip_edge, read from the ROM */
static const GsuCode clip_edge_fetch[4][7] = {
    {{0x01c370, 4}, {0x01c668, 1}, {0x01c374, 1}, {0x01c381, 4}, {0x01c389, 1}, {0x01c381, 4}, {0x01c38a, 3}},
    {{0x01c394, 4}, {0x01cb63, 1}, {0x01c398, 1}, {0x01c3a5, 4}, {0x01c3ad, 1}, {0x01c3a5, 4}, {0x01c3ae, 3}},
    {{0x01c3b8, 4}, {0x01c933, 1}, {0x01c3bc, 1}, {0x01c3c9, 4}, {0x01c3d1, 1}, {0x01c3c9, 4}, {0x01c3d2, 3}},
    {{0x01c3dc, 4}, {0x01ca42, 1}, {0x01c3e0, 1}, {0x01c3ed, 4}, {0x01c3f5, 1}, {0x01c3ed, 4}, {0, 0}},
};

/* gsuwalk: the copy of lists_swap each copy of clip_edge calls */
static const uint8_t lists_swap_in_clip_edge[4] = {3, 4, 5, 6};

static int clip_edge(void (*clip)(void), uint16_t ret, int reload, int copy)
{
    gsu.r[11] = ret;
    gsu_codes(clip_edge_fetch[copy], 2);
    clip();
    gsu_codes(clip_edge_fetch[copy] + 2, 1);
    lists_swap(GSU_COPY(lists_swap_in_clip_edge[copy]));
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        { gsu_codes(clip_edge_fetch[copy] + 5, 1);
        return 0; } else { gsu_codes(clip_edge_fetch[copy] + 3, 2); }
    if (reload)
        { gsu_codes(clip_edge_fetch[copy] + 6, 1); gsu.r[5] = gsu_ldw(FX_CODES); }
    return 1;
}

/*
 * @routine 01:C339 FxPolygonDraw
 * WHAT IT DOES: Takes a polygon of R9 points (index bytes at R14 - R9)
 * through to the screen: FxPointsGather fills its vertex list at $082A,
 * then it is clipped against each screen edge its outcodes in R5 cross
 * (bit 1 top, bit 0 bottom, bit 3 right, bit 2 left; bit 4, behind the
 * view, means all four), split in depth and projected
 * (FxPolygonNearClip, FxPolygonProject). When a clip leaves nothing the
 * count $0040 is cleared.
 * HOW: Pushes R11 on the R10 stack. R5 saved at $014A, R9 from $0146
 * into $0040, R14 moved back by R9. The lists set to $082A in, $0688
 * out; each clip leaves its result in the other and the two are
 * swapped. Returns by pulling R15.
 * Entry: R5 = the outcodes; R14 past the index bytes; R10 = stack; R11 =
 * return address. Exit: R0 = the count, or 0; $0040 = 0 when nothing is
 * left; S and Z from R10; the last RAM address R10; the rest as the last
 * routine called leaves them. R14 and the ROM buffer move as on the Super
 * FX.
 */
void abi_FxPolygonDraw(void)
{
    gsu_code(0x01c33a, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_code(0x01c33c, 5);
    gsu_stw(FX_CODES, gsu.r[5]);
    gsu_code(0x01c341, 3);
    gsu.r[9] = gsu_ldw(FX_POINTS_N);
    gsu_code(0x01c344, 3);
    gsu_stw(FX_COUNT, gsu.r[9]);
    gsu_code(0x01c347, 2);
    gsu_r14(gsu_sub(gsu.r[14], gsu.r[9], 0));
    gsu.r[11] = 0xC34D;
    gsu_code(0x01c349, 4); gsu_code(0x01c1be, 1); gsu_cache(0xc1bf);
    abi_FxPointsGather();
    gsu.r[0] = FX_LIST_B;
    gsu_code(0x01c34d, 7);
    gsu_stw(FX_LIST_IN, gsu.r[0]);
    gsu.r[0] = FX_LIST_A;
    gsu_code(0x01c354, 6);
    gsu_stw(FX_LIST_OUT, gsu.r[0]);
    gsu_code(0x01c35a, 3);
    gsu.r[5] = gsu_ldw(FX_CODES);
    gsu.r[1] = 0x0010;
    gsu.r[1] = gsu_and(gsu.r[1], gsu.r[5]);
    if (!gsu.z) {
        gsu.r[5] = 0x000F;
        gsu_code(0x01c35d, 12);
        gsu_stw(FX_CODES, gsu.r[5]);
    } else { gsu_code(0x01c35d, 6); gsu_code(0x01c368, 1); }
    gsu.r[1] = gsu_and(gsu.r[5], 2);
    if (!gsu.z) {                             /* BEQ past it, then its own test */
        gsu_code(0x01c369, 7);
        if (!clip_edge(abi_FxPolygonClipTop, 0xC374, 1, GSU_COPY(0)))
            { gsu_code(0x01c385, 4); gsu_code(0x01c409, 1); goto none; }
    } else { gsu_code(0x01c369, 6); gsu_code(0x01c38c, 1); }
    gsu.r[1] = gsu_and(gsu.r[5], 1);
    if (!gsu.z) {                             /* BEQ past it, then its own test */
        gsu_code(0x01c38d, 7);
        if (!clip_edge(abi_FxPolygonClipBottom, 0xC398, 1, GSU_COPY(1)))
            { gsu_code(0x01c3a9, 4); gsu_code(0x01c409, 1); goto none; }
    } else { gsu_code(0x01c38d, 6); gsu_code(0x01c3b0, 1); }
    gsu.r[1] = gsu_and(gsu.r[5], 8);
    if (!gsu.z) {                             /* BEQ past it, then its own test */
        gsu_code(0x01c3b1, 7);
        if (!clip_edge(abi_FxPolygonClipRight, 0xC3BC, 1, GSU_COPY(2)))
            { gsu_code(0x01c3cd, 4); gsu_code(0x01c409, 1); goto none; }
    } else { gsu_code(0x01c3b1, 6); gsu_code(0x01c3d4, 1); }
    gsu.r[1] = gsu_and(gsu.r[5], 4);
    if (!gsu.z) {                             /* BEQ past it, then its own test */
        gsu_code(0x01c3d5, 7);
        if (!clip_edge(abi_FxPolygonClipLeft, 0xC3E0, 0, GSU_COPY(3)))
            { gsu_code(0x01c3f1, 4); gsu_code(0x01c409, 1); goto none; }
    } else { gsu_code(0x01c3d5, 6); gsu_code(0x01c3f5, 1); }
    gsu.r[11] = 0xC3FA;
    gsu_code(0x01c3f6, 4); gsu_code(0x01c21e, 1);
    abi_FxPolygonNearClip();
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        { gsu_code(0x01c3fa, 5); gsu_code(0x01c409, 1);
        goto none; } else { gsu_code(0x01c3fa, 5); }
    gsu.r[11] = 0xC404;
    gsu_code(0x01c3ff, 5); gsu_code(0x01c412, 1);
    abi_FxPolygonProject();
    gsu_code(0x01c404, 5);
    gsu_pull(&gsu.r[10]);
    return;
none:
    ;
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x01c40a, 4);
    gsu_stw(FX_COUNT, gsu.r[0]);
    gsu_code(0x01c40e, 4);
    gsu_pull(&gsu.r[10]);
}
