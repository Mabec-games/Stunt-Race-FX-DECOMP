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
static void lists_swap(void)
{
    gsu.r[1] = gsu_ldw(FX_LIST_IN);
    gsu.r[2] = gsu_ldw(FX_LIST_OUT);
    gsu_stw(FX_LIST_IN, gsu.r[2]);
    gsu_stw(FX_LIST_OUT, gsu.r[1]);
}

/* A word from R7 (LDW, INC, INC). */
static uint16_t next_in(void)
{
    uint16_t v = gsu_ldw(gsu.r[7]);

    gsu.r[7] = gsu_inc(gsu_inc(gsu.r[7]));
    return v;
}

/* A word to R2 (STW, INC, INC). */
static void next_out(uint16_t v)
{
    gsu_stw(gsu.r[2], v);
    gsu.r[2] = gsu_inc(gsu_inc(gsu.r[2]));
}

/* A word to R8 (STW, INC, INC). */
static void next_r8(uint16_t v)
{
    gsu_stw(gsu.r[8], v);
    gsu.r[8] = gsu_inc(gsu_inc(gsu.r[8]));
}

/* R4 the largest z so far, R5 the smallest (SUB, BPL / BMI on S alone). */
static void z_range(void)
{
    gsu.r[0] = gsu_sub(gsu.r[4], gsu.r[6], 0);
    if (gsu.s) {
        gsu.r[4] = gsu.r[6];
        return;
    }
    gsu.r[0] = gsu_sub(gsu.r[5], gsu.r[6], 0);
    if (!gsu.s)
        gsu.r[5] = gsu.r[6];
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

    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[3] = gsu_ldw(0x0026);
    gsu.r[13] = gsu_ldw(0x0028);
    gsu.r[9] = gsu_ldw(0x002A);
    gsu.r[0] = 0x0014;
    gsu.rombr = 0x14;
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(FX_CODES, gsu.r[0]);
    gsu_stw(0x001A, gsu.r[14]);
    gsu.r[12] = gsu_ldw(FX_COUNT);
    gsu.r[7] = gsu_ldw(FX_LIST_IN);
    gsu.r[8] = gsu_ldw(FX_OUT);
    do {
        gsu.r[1] = next_in();
        gsu.r[2] = next_in();
        gsu.r[6] = next_in();
        gsu.r[11] = 0xC445;
        abi_FxProjectVertex();
        gsu_push(&gsu.r[10], gsu.r[0]);
        gsu.r[0] = gsu_and(gsu.r[0], 15);
        c = !gsu.z;
        gsu.r[0] = gsu_lsr(gsu.r[0]);       /* BEQ's delay slot */
        if (c) {
            c = gsu.cy;
            gsu.r[0] = gsu_lsr(gsu.r[0]);
            if (c)
                gsu.r[2] = gsu_ldw(FX_EDGES + 4);
            c = gsu.cy;
            gsu.r[0] = gsu_lsr(gsu.r[0]);
            if (c)
                gsu.r[2] = gsu_ldw(FX_EDGES + 6);
            c = gsu.cy;
            gsu.r[0] = gsu_lsr(gsu.r[0]);
            if (c) {
                gsu.r[1] = gsu_ldw(FX_EDGES);   /* LMS, then IBT R1, #0 */
                gsu.r[1] = 0;
            }
            if (gsu.cy)
                gsu.r[1] = gsu_ldw(FX_EDGES + 2);
        }
        gsu.r[0] = gsu_pull(&gsu.r[10]);
        next_r8(gsu.r[1]);
        next_r8(gsu.r[2]);
        gsu.r[6] = next_in();
        gsu.r[6] = gsu_add(gsu.r[6], gsu.r[6], 0);
        next_r8(gsu.r[6]);
        gsu.r[6] = next_in();
        gsu.r[6] = gsu_add(gsu.r[6], gsu.r[6], 0);
        next_r8(gsu.r[6]);
        gsu.r[6] = gsu_ldw(FX_CODES);
        gsu.r[0] = gsu_or(gsu.r[0], gsu.r[6]);
        gsu.r[12] = gsu_dec(gsu.r[12]);
        c = !gsu.z;
        gsu_sbk(gsu.r[0]);                  /* BNE's delay slot */
    } while (c);
    gsu.r[0] = gsu_ldw(0x001C);
    gsu.rombr = (uint8_t)gsu.r[0];
    gsu_r14(gsu_ldw(0x001A));
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(FX_CODES, gsu.r[0]);
    gsu.r[0] = gsu_ldw(FX_COUNT);
    gsu_stw(FX_POINTS_N, gsu.r[0]);
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
void abi_FxPolygonNearClip(void)
{
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[5] = 0x3E80;
    gsu.r[4] = 0xC180;
    gsu.r[12] = gsu_ldw(FX_COUNT);
    gsu.r[0] = FX_LIST_B;
    gsu.r[7] = gsu_ldw(FX_LIST_IN);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[7], 0);
    if (!gsu.z) {
        gsu.r[7] = gsu_add(gsu.r[7], 4, 0);
        gsu.r[13] = 0xC23A;
        do {
            gsu.r[6] = gsu_ldw(gsu.r[7]);
            z_range();
            gsu.r[7] = gsu_add(gsu.r[7], 10, 0);
        } while (gsu_count_down(&gsu.r[12]));
    } else {
        gsu.r[2] = gsu_ldw(FX_LIST_OUT);
        gsu.r[13] = 0xC25A;
        do {
            gsu.r[0] = next_in();
            next_out(gsu.r[0]);
            gsu.r[0] = next_in();
            next_out(gsu.r[0]);
            gsu.r[6] = next_in();
            next_out(gsu.r[6]);
            z_range();
            gsu.r[0] = next_in();
            next_out(gsu.r[0]);
            gsu.r[0] = next_in();
            next_out(gsu.r[0]);
        } while (gsu_count_down(&gsu.r[12]));
        lists_swap();
    }
    gsu.r[0] = gsu_ldw(FX_COUNT);
    gsu_stw(FX_COUNT_SAVE, gsu.r[0]);
    gsu.r[0] = gsu_add(gsu.r[5], gsu.r[5], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
    gsu.r[6] = gsu_sub(gsu.r[0], gsu.r[4], 0);
    if (!gsu.s) {
        gsu.r[0] = gsu_ldw(FX_COUNT);
        gsu_pull(&gsu.r[10]);
        return;
    }
    gsu.r[0] = gsu_add(gsu.r[4], gsu.r[0], 0);
    gsu.r[0] = gsu_lsr(gsu.r[0]);
    gsu.r[0] = gsu_lsr(gsu.r[0]);
    gsu_push(&gsu.r[10], gsu.r[0]);
    gsu.r[11] = 0xC2B8;
    abi_FxPolygonClipNear();
    lists_swap();
    gsu.r[1] = gsu_pull(&gsu.r[10]);
    gsu_moves(gsu.r[0]);
    if (gsu.z) {
        gsu_pull(&gsu.r[10]);
        return;
    }
    gsu_push(&gsu.r[10], gsu.r[1]);
    gsu.r[2] = gsu_ldw(FX_OUT);
    gsu.r[2] = gsu_sub(gsu.r[2], 8, 0);
    gsu.r[0] = gsu_not(gsu.r[0]);
    gsu_stb(gsu.r[2], (uint8_t)gsu.r[0]);
    gsu.r[0] = gsu_not(gsu.r[0]);
    gsu_push(&gsu.r[10], gsu.r[2]);
    gsu.r[11] = 0xC2E4;
    abi_FxPolygonProject();
    gsu.r[1] = gsu_pull(&gsu.r[10]);
    {
        int i;

        for (i = 0; i < 4; i++) {
            gsu.r[0] = gsu_ldw(gsu.r[1]);
            gsu.r[1] = gsu_inc(gsu_inc(gsu.r[1]));
            next_r8(gsu.r[0]);
        }
    }
    gsu_stw(FX_OUT, gsu.r[8]);
    gsu.r[0] = FX_LIST_B;
    gsu_stw(FX_LIST_OUT, gsu.r[0]);
    gsu.r[0] = FX_LIST_A;
    gsu_stw(FX_LIST_IN, gsu.r[0]);
    gsu.r[0] = gsu_ldw(FX_COUNT_SAVE);
    gsu_stw(FX_COUNT, gsu.r[0]);
    gsu.r[0] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = gsu_inc(gsu_not(gsu.r[0]));
    gsu.r[11] = 0xC320;
    abi_FxPolygonClipNear();
    gsu_moves(gsu.r[0]);
    if (!gsu.z) {
        lists_swap();
        gsu.r[0] = gsu_ldw(FX_COUNT);
    }
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

/* One edge clip of FxPolygonDraw: the clip, the lists swapped; 0 when
   nothing is left (R0 = 0), else 1 with R5 = $014A again (when reload). */
static int clip_edge(void (*clip)(void), uint16_t ret, int reload)
{
    gsu.r[11] = ret;
    clip();
    lists_swap();
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        return 0;
    if (reload)
        gsu.r[5] = gsu_ldw(FX_CODES);
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
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_stw(FX_CODES, gsu.r[5]);
    gsu.r[9] = gsu_ldw(FX_POINTS_N);
    gsu_stw(FX_COUNT, gsu.r[9]);
    gsu_r14(gsu_sub(gsu.r[14], gsu.r[9], 0));
    gsu.r[11] = 0xC34D;
    abi_FxPointsGather();
    gsu.r[0] = FX_LIST_B;
    gsu_stw(FX_LIST_IN, gsu.r[0]);
    gsu.r[0] = FX_LIST_A;
    gsu_stw(FX_LIST_OUT, gsu.r[0]);
    gsu.r[5] = gsu_ldw(FX_CODES);
    gsu.r[1] = 0x0010;
    gsu.r[1] = gsu_and(gsu.r[1], gsu.r[5]);
    if (!gsu.z) {
        gsu.r[5] = 0x000F;
        gsu_stw(FX_CODES, gsu.r[5]);
    }
    gsu.r[1] = gsu_and(gsu.r[5], 2);
    if (!gsu.z && !clip_edge(abi_FxPolygonClipTop, 0xC374, 1))
        goto none;
    gsu.r[1] = gsu_and(gsu.r[5], 1);
    if (!gsu.z && !clip_edge(abi_FxPolygonClipBottom, 0xC398, 1))
        goto none;
    gsu.r[1] = gsu_and(gsu.r[5], 8);
    if (!gsu.z && !clip_edge(abi_FxPolygonClipRight, 0xC3BC, 1))
        goto none;
    gsu.r[1] = gsu_and(gsu.r[5], 4);
    if (!gsu.z && !clip_edge(abi_FxPolygonClipLeft, 0xC3E0, 0))
        goto none;
    gsu.r[11] = 0xC3FA;
    abi_FxPolygonNearClip();
    gsu_moves(gsu.r[0]);
    if (gsu.z)
        goto none;
    gsu.r[11] = 0xC404;
    abi_FxPolygonProject();
    gsu_pull(&gsu.r[10]);
    return;
none:
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(FX_COUNT, gsu.r[0]);
    gsu_pull(&gsu.r[10]);
}
