/*
 * project.c: the Super FX projecting a point onto the screen: X and Y over
 * the depth (FxRatioClamp, clamped to $3FFF), moved to the screen centre,
 * with an outcode of which screen edges the point is beyond.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_CENTRE 0x0034u   /* the screen centre: X at $34, Y at $36 */
#define FX_EDGES 0x0038u    /* the edges: X at $38 and $3A, Y at $3C and $3E */

void abi_FxRatioClamp(void);

/* The LINK return address of each of the sixteen FxRatioClamp calls, by
   depth negative, X negative, Y negative, |X| below |Y|. */
static const uint16_t clamp_ret[16] = {
    0x94A2, 0x94B1, 0x94C8, 0x94DA, 0x94F9, 0x950B, 0x9525, 0x953A,
    0x9563, 0x9578, 0x9595, 0x95A7, 0x95C6, 0x95D8, 0x95F2, 0x9601,
};

/* NOT then INC. */
static uint16_t negate(uint16_t v)
{
    return gsu_inc(gsu_not(v));
}

/* One axis of the outcode: from R5, one more for a point past the low
   edge and two more past the high edge (SUB and BGE against each), the
   two bits swapped (XOR #3) when the point is behind (the bit in mask). */
static void outcode_axis(uint16_t v, uint16_t low_edge, uint16_t high_edge, uint16_t mask)
{
    gsu.r[0] = gsu_sub(gsu_ldw(low_edge), v, 0);
    if (gsu.s == gsu.ov)
        goto one;
    gsu.r[0] = gsu_sub(gsu_ldw(high_edge), v, 0);
    if (gsu.s == gsu.ov)
        return;
    gsu.r[5] = gsu_inc(gsu.r[5]);
one:
    gsu.r[5] = gsu_inc(gsu.r[5]);
    gsu.r[0] = gsu_and((uint16_t)mask, gsu.r[5]);
    if (!gsu.z)
        gsu.r[5] = gsu_xor(gsu.r[5], 3);
}

/*
 * @routine 01:947A FxProjectPoint
 * WHAT IT DOES: Projects the point (R1, R2) at depth R3 to the screen,
 * moved to the centre at $0034, and returns its outcode: bits 0-1 past
 * the X edges, bits 2-3 past the Y edges, bit 4 behind the view.
 * HOW: Pushes R11 on the R10 stack. A negative depth is negated and sets
 * R5 = 4 (else R5 = 0; a zero depth counts as 1). FxRatioClamp divides the
 * larger of |R1|, |R2| first (the pair swapped around the call when |R1|
 * is the smaller), then the signs go back, each one reversed when the
 * depth was negative: sixteen copies of the code, one per case. Then
 * CACHE, the centre is added, and against the edges at $0038-$003E
 * (SUB, BGE) R5 counts 1 for past the low edge and 2 for past the high
 * one, its two bits swapped when behind, X then (after MULT #4) Y. R0 =
 * the outcode's complement in its low 5 bits << 8, OR the outcode.
 * Returns by pulling R11's value into R15.
 * Entry: R1, R2 = the point, R3 = its depth; R10 = the stack; R11 =
 * return address. Exit: R1, R2 = the screen position; R5 = the outcode;
 * R0 = ((R5 XOR $1F) << 8) | R5; R3 = |depth| (1 for 0); R4, R6, R11,
 * R12, R13 as FxRatioClamp leaves them; R10 as at entry; the last RAM
 * address R10; S and Z from R10; OV and CY from the last SUB R2.
 */
void FxProjectPoint(void)
{
    int behind, xneg, yneg, swap;

    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_moves(gsu.r[3]);
    behind = gsu.s;
    if (behind) {
        gsu.r[3] = negate(gsu.r[3]);
        gsu.r[5] = 4;
    } else {
        if (gsu.z)
            gsu.r[3] = gsu_inc(gsu.r[3]);
        gsu.r[5] = 0;
    }
    gsu_moves(gsu.r[1]);
    xneg = gsu.s;
    if (xneg)
        gsu.r[1] = negate(gsu.r[1]);
    gsu_moves(gsu.r[2]);
    yneg = gsu.s;
    if (yneg)
        gsu.r[2] = negate(gsu.r[2]);
    gsu.r[0] = gsu_sub(gsu.r[1], gsu.r[2], 0);
    swap = !gsu.cy;
    if (swap) {
        gsu.r[0] = gsu.r[1];
        gsu.r[1] = gsu.r[2];
        gsu.r[2] = gsu.r[0];
    }
    gsu.r[11] = clamp_ret[behind << 3 | xneg << 2 | yneg << 1 | swap];
    abi_FxRatioClamp();
    if (swap) {
        gsu.r[0] = gsu.r[1];
        gsu.r[1] = gsu.r[2];
        gsu.r[2] = gsu.r[0];
    }
    /* The signs back, each reversed behind the view. */
    if (yneg != behind)
        gsu.r[2] = negate(gsu.r[2]);
    if (xneg != behind)
        gsu.r[1] = negate(gsu.r[1]);

    gsu.r[0] = gsu_ldw(FX_CENTRE);
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[0], 0);
    gsu.r[0] = gsu_ldw((uint16_t)(FX_CENTRE + 2));
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    outcode_axis(gsu.r[1], FX_EDGES, (uint16_t)(FX_EDGES + 2), 0x04);
    gsu.r[5] = (uint16_t)((int8_t)gsu.r[5] * 4);      /* MULT #4 */
    gsu_sz(gsu.r[5]);
    outcode_axis(gsu.r[2], (uint16_t)(FX_EDGES + 4), (uint16_t)(FX_EDGES + 6), 0x10);
    gsu.r[0] = gsu_or(gsu_swap(gsu_xor(0x1F, gsu.r[5])), gsu.r[5]);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

void abi_FxProjectPoint(void)
{
    FxProjectPoint();
}

#define FX_SCALE_TABLE 0x896Cu  /* $14:896C: the scale for each even depth */

/*
 * @routine 01:B2A4 FxProjectScaled
 * WHAT IT DOES: Projects a point and a size at a depth: X (R1), Y (R2) and
 * the size (R5) times the scale for the depth (R3), X and Y moved to the
 * screen centre.
 * HOW: ROM bank $14. While the depth is above $4400 (SUB, BPL), ASR halves
 * X, Y, the size and the depth; while $100 - the depth is not negative,
 * ADD doubles them, so the depth ends from $101 to $4400. The scale is
 * the word at $14:896C + the depth AND NOT 1 (GETB, GETBH); FMULT by it
 * gives each, X plus $34 and Y plus $36.
 * Entry: R1 = X, R2 = Y, R3 = the depth (positive), R5 = the size; R11 =
 * return address. Exit: R1, R2 = the screen position; R5 = R0 = the size;
 * R3 = the depth as scaled, bit 0 clear; R6 = the scale; R14 = the
 * table's address + 1; the ROM bank $14; flags from the last FMULT.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxProjectScaled(void)
{
    gsu.r[0] = 0x0014;
    gsu.rombr = 0x14;
    for (;;) {
        gsu.r[0] = gsu_sub(0x4400, gsu.r[3], 0);
        if (!gsu.s)
            break;
        gsu.r[1] = gsu_asr(gsu.r[1]);
        gsu.r[2] = gsu_asr(gsu.r[2]);
        gsu.r[5] = gsu_asr(gsu.r[5]);
        gsu.r[3] = gsu_asr(gsu.r[3]);       /* BRA's delay slot */
    }
    for (;;) {
        gsu.r[0] = gsu_sub(0x0100, gsu.r[3], 0);
        if (gsu.s)
            break;
        gsu.r[5] = gsu_add(gsu.r[5], gsu.r[5], 0);
        gsu.r[1] = gsu_add(gsu.r[1], gsu.r[1], 0);
        gsu.r[2] = gsu_add(gsu.r[2], gsu.r[2], 0);
        gsu.r[3] = gsu_add(gsu.r[3], gsu.r[3], 0);  /* BRA's delay slot */
    }
    gsu.r[3] = gsu_bic(gsu.r[3], 0x0001);
    gsu.r[0] = FX_SCALE_TABLE;
    gsu_r14(gsu_add(gsu.r[0], gsu.r[3], 0));
    gsu.r[6] = gsu_getb();
    gsu_r14(gsu_inc(gsu.r[14]));
    gsu.r[6] = gsu_getbh(gsu.r[6]);
    gsu.r[0] = gsu_fmult(gsu.r[1], gsu.r[6]);
    gsu.r[1] = gsu_ldw(FX_CENTRE);
    gsu.r[1] = gsu_add(gsu.r[1], gsu.r[0], 0);
    gsu.r[0] = gsu_fmult(gsu.r[2], gsu.r[6]);
    gsu.r[2] = gsu_ldw(FX_CENTRE + 2);
    gsu.r[2] = gsu_add(gsu.r[2], gsu.r[0], 0);
    gsu.r[0] = gsu_fmult(gsu.r[5], gsu.r[6]);
    gsu.r[5] = gsu.r[0];
}

void abi_FxProjectScaled(void)
{
    FxProjectScaled();
}
