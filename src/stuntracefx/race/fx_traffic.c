/*
 * fx_traffic.c: the Super FX looks for cars about to be caught up with.
 *
 * For car R8, every other car on the cars' list ($70:262E, linked through
 * +$74) that is not flagged out (bit 10 of +$16) and not faster (+$4A) is
 * tested: it counts when the closing speed along the line between them,
 * plus 20, is at least their distance (|dx| + |dz|) / 32, and it is less
 * than $200 to the side (along the first column of the matrix at $00E4).
 * Bits 0 and 1 of R8's +$42 then say on which side it is.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_CARS 0x262Eu
#define FX_ME_X 0x26F6u
#define FX_ME_Z 0x26FAu
#define FX_ME_VX 0x26FCu
#define FX_ME_VZ 0x2700u
#define FX_DIST 0x263Au
#define FX_CLOSE 0x2638u
#define FX_D_X 0x0068u
#define FX_D_Y 0x002Cu
#define FX_D_Z 0x002Eu
#define FX_MATRIX 0x00E4u

#define REC_X 0x1Eu
#define REC_Z 0x22u
#define REC_VX 0x2Eu
#define REC_VZ 0x32u
#define REC_FLAGS16 0x16u
#define REC_SIDES 0x42u
#define REC_SPEED 0x4Au
#define REC_NEXT_CAR 0x74u
#define REC_Y 0x20u
#define REC_YAW_RATE 0x34u
#define REC_SPREAD 0x7Cu
#define NEAR 0x07D0u           /* 2000 */
#define FX_PLAYER 0x2476u
#define FX_NEAR_LEVEL 0x2642u   /* smoothed closing level, -1 for none */
#define FX_NEAR_SIDE 0x2644u
#define FX_NEAR_BAND 0x2646u
#define FX_NEAR_CAR 0x2648u
#define FX_NEAR_Z 0x264Au
#define FX_NEAR_X 0x264Cu
#define FX_VIEW_X 0x00D8u
#define FX_VIEW_Z 0x00DCu
#define FX_VIEW_YAW 0x00E0u
#define REC_64 0x64u
#define REC_NEAR 0x8Cu

/* The field `field` of record `rec` (R0 = field + rec, then LDW). */
static uint16_t field(uint16_t f, uint16_t rec)
{
    return gsu_ldw(gsu_add(f, rec, 0));
}

/* |v| into a register: MOVES, then NOT and INC when negative. */
static uint16_t magnitude(uint16_t v)
{
    gsu_moves(v);
    return gsu.s ? gsu_inc(gsu_not(v)) : v;
}

/* The dot product of (a, b, c) with the words at $0068, $002C, $002E, each
   FMULT then ROL; R5 = the first two summed, R6 = the last word read. */
static uint16_t dot(FxRegs *g, uint16_t a, uint16_t b, uint16_t c)
{
    g->r[6] = gsu_ldw(FX_D_X);
    g->r[5] = gsu_rol(gsu_fmult(a, g->r[6]));
    g->r[6] = gsu_ldw(FX_D_Y);
    g->r[5] = gsu_add(g->r[5], gsu_rol(gsu_fmult(b, g->r[6])), 0);
    g->r[6] = gsu_ldw(FX_D_Z);
    return gsu_add(gsu_rol(gsu_fmult(c, g->r[6])), g->r[5], 0);
}

/*
 * @routine 04:C933 FxTrafficSides
 * WHAT IT DOES: As described above: for each car that counts, bit 1 of R8's
 * +$42 is set when it is to the left (its offset negative) and bit 0
 * cleared if it was set when the routine started; to the right, bit 0 set
 * and bit 1 cleared likewise. $2638 = -1 for a car $200 or more to the
 * side.
 * HOW: R8's position and velocity are copied to $26F6, $26FA, $26FC,
 * $2700; per car, the difference goes to $0068/$002C/$002E, the distance
 * / 32 (DIV2 five times) to $263A, the closing speed (FMULT, ROL) plus 20
 * to $2638, then the side offset to $263A. R9 holds +$42 as it was at the
 * start; the bits are changed in R6, read again per car. The return
 * address is kept on the R10 stack.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R7 = 0; R9 = the old +$42; the other registers from the last car tested;
 * S, Z from R10; the last RAM address R10.
 */
FxRegs FxTrafficSides(FxRegs g)
{
    static const uint16_t me[4] = {FX_ME_X, FX_ME_Z, FX_ME_VX, FX_ME_VZ};
    static const uint8_t from[4] = {REC_X, REC_Z, REC_VX, REC_VZ};
    int i;

    gsu_push(&g.r[10], g.r[11]);
    g.r[9] = field(REC_SIDES, g.r[8]);
    for (i = 0; i < 4; i++) {
        g.r[0] = field(from[i], g.r[8]);
        gsu_stw(me[i], g.r[0]);
    }
    g.r[0] = gsu_ldw(FX_CARS);
    for (;; g.r[0] = field(REC_NEXT_CAR, g.r[7])) {
        g.r[7] = g.r[0];
        gsu_moves(g.r[7]);
        if (gsu.z)
            break;
        gsu_sub(g.r[0], g.r[8], 0);
        if (gsu.z)
            continue;                                /* itself */
        g.r[1] = field(REC_FLAGS16, g.r[7]);
        g.r[0] = gsu_and(0x0400, g.r[1]);
        if (!gsu.z)
            continue;
        g.r[1] = field(REC_SPEED, g.r[7]);
        g.r[0] = gsu_sub(field(REC_SPEED, g.r[8]), g.r[1], 0);
        if (gsu.s)
            continue;                                /* faster */
        g.r[0] = field(REC_X, g.r[7]);
        g.r[1] = gsu_ldw(FX_ME_X);
        g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
        gsu_stw(FX_D_X, g.r[0]);
        g.r[1] = magnitude(g.r[0]);
        g.r[0] = field(REC_Z, g.r[7]);
        g.r[3] = gsu_ldw(FX_ME_Z);
        g.r[0] = gsu_sub(g.r[0], g.r[3], 0);
        gsu_stw(FX_D_Z, g.r[0]);
        g.r[3] = magnitude(g.r[0]);
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
        gsu_stw(FX_D_Y, g.r[0]);
        g.r[0] = gsu_add(g.r[1], g.r[3], 0);
        for (i = 0; i < 5; i++)
            g.r[0] = gsu_div2(g.r[0]);
        gsu_stw(FX_DIST, g.r[0]);
        g.r[0] = field(REC_VX, g.r[7]);
        g.r[1] = gsu_sub(gsu_ldw(FX_ME_VX), g.r[0], 0);
        g.r[0] = field(REC_VZ, g.r[7]);
        g.r[3] = gsu_sub(gsu_ldw(FX_ME_VZ), g.r[0], 0);
        g.r[2] = 0;
        g.r[0] = dot(&g, g.r[1], g.r[2], g.r[3]);
        g.r[1] = 0x0014;
        g.r[0] = gsu_add(g.r[0], g.r[1], 0);
        gsu_stw(FX_CLOSE, g.r[0]);
        g.r[1] = gsu_ldw(FX_DIST);
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.s)
            continue;                                /* not closing in time */
        g.r[1] = gsu_ldw(FX_MATRIX);
        g.r[2] = gsu_ldw(FX_MATRIX + 2);
        g.r[3] = gsu_ldw(FX_MATRIX + 4);
        g.r[0] = dot(&g, g.r[1], g.r[2], g.r[3]);
        gsu_stw(FX_DIST, g.r[0]);
        g.r[1] = g.r[0];
        g.r[0] = magnitude(g.r[0]);
        g.r[2] = 0x0200;
        gsu_sub(g.r[0], g.r[2], 0);
        if (!gsu.s) {
            g.r[2] = 0xFFFF;
            gsu_stw(FX_CLOSE, g.r[2]);
        }
        g.r[6] = field(REC_SIDES, g.r[8]);
        g.r[0] = gsu_ldw(FX_CLOSE);
        gsu_moves(g.r[0]);
        if (!gsu.s) {
            gsu_moves(g.r[1]);
            if (gsu.s) {                             /* to the left */
                g.r[0] = gsu_and(0x0001, g.r[9]);
                if (!gsu.z) {
                    g.r[0] = 0xFFFE;
                    g.r[6] = gsu_and(g.r[0], g.r[6]);
                }
                g.r[0] = 0x0002;
                g.r[6] = gsu_or(g.r[0], g.r[6]);
            } else {
                g.r[0] = gsu_and(0x0002, g.r[9]);
                if (!gsu.z) {
                    g.r[0] = 0xFFFD;
                    g.r[6] = gsu_and(g.r[0], g.r[6]);
                }
                g.r[0] = 0x0001;
                g.r[6] = gsu_or(g.r[0], g.r[6]);
            }
        }
        g.r[0] = gsu_add(REC_SIDES, g.r[8], 0);
        gsu_stw(g.r[0], g.r[6]);
    }
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxTrafficSides(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxTrafficSides(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* |v| / 2 by DIV2 (S from it), then NOT and INC when negative. */
static uint16_t half_magnitude(uint16_t v)
{
    v = gsu_div2(v);
    return gsu.s ? gsu_inc(gsu_not(v)) : v;
}

/*
 * @routine 04:BF98 FxSpreadCars
 * WHAT IT DOES: Pushes the other cars near car R8 away from it: for each
 * car on the cars' list (not R8) whose distance (|dx| + |dy| + |dz|) / 2
 * is below 2000, its velocity x and z gain twice (2000 - that) along the
 * unit vector from R8 to it, its +$34 gains or loses a share of it (by the
 * sign of +$34: times $C180 or $3E80, scaled), and its +$7C = 30. A
 * negative half z distance is made positive by code decoded from the ROM
 * bytes and not reached in the traces so far.
 * HOW: R8's position in R6, R5, R7 and R2 = 2000; per car, the differences
 * (x into R1, z into R3) halved with DIV2; for a near car R8, R6, R5, R7,
 * R2, the push and the car are kept on the R10 stack around FxNormalize of
 * (dx, 0, dz), and each push is FMULT then ADD and SBK. R8 and the return
 * address are kept on the R10 stack.
 * Entry: R8 = the car's record; R10 = stack; R11 = return address. Exit:
 * R9 = 0; R8, R6, R5, R7, R2 as set up; the others from the last car; S,
 * Z from R10; the last RAM address R10.
 */
FxRegs FxSpreadCars(FxRegs g)
{
    uint16_t *sp = &g.r[10];

    gsu_push(sp, g.r[11]);
    gsu_push(sp, g.r[8]);
    g.r[6] = gsu_ldw(gsu_add(REC_X, g.r[8], 0));
    g.r[5] = gsu_ldw(gsu_add(REC_Y, g.r[8], 0));
    g.r[7] = gsu_ldw(gsu_add(REC_Z, g.r[8], 0));
    g.r[2] = NEAR;
    g.r[0] = gsu_ldw(FX_CARS);
    for (;; g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[9], 0))) {
        g.r[9] = g.r[0];
        gsu_moves(g.r[9]);
        if (gsu.z)
            break;
        gsu_sub(g.r[0], g.r[8], 0);
        if (gsu.z)
            continue;
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_X, g.r[9], 0)), g.r[6], 0);
        g.r[1] = g.r[0];
        g.r[4] = half_magnitude(g.r[0]);
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_Y, g.r[9], 0)), g.r[5], 0);
        g.r[4] = gsu_add(g.r[4], half_magnitude(g.r[0]), 0);
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_Z, g.r[9], 0)), g.r[7], 0);
        g.r[3] = g.r[0];
        g.r[0] = half_magnitude(g.r[0]);
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        g.r[0] = gsu_sub(g.r[0], g.r[2], 0);
        if (!gsu.s)
            continue;                                /* not near */
        gsu_push(sp, g.r[8]);
        gsu_push(sp, g.r[6]);
        gsu_push(sp, g.r[5]);
        gsu_push(sp, g.r[7]);
        gsu_push(sp, g.r[2]);
        g.r[0] = magnitude(g.r[0]);
        g.r[0] = gsu_add(g.r[0], g.r[0], 0);
        gsu_push(sp, g.r[0]);
        gsu_push(sp, g.r[9]);
        g.r[2] = 0;
        g.r[11] = 0xC026;
        g = FxNormalize(g);
        g.r[9] = gsu_pull(sp);
        g.r[6] = gsu_pull(sp);
        g.r[0] = gsu_fmult(g.r[1], g.r[6]);
        g.r[4] = gsu_ldw(gsu_add(REC_VX, g.r[9], 0));
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        gsu_sbk(g.r[0]);
        g.r[0] = gsu_fmult(g.r[3], g.r[6]);
        g.r[4] = gsu_ldw(gsu_add(REC_VZ, g.r[9], 0));
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        gsu_sbk(g.r[0]);
        g.r[0] = gsu_ldw(gsu_add(REC_YAW_RATE, g.r[9], 0));
        g.r[1] = g.r[0];
        g.r[0] = 0xC180;
        gsu_moves(g.r[1]);
        if (!gsu.s)
            g.r[0] = 0x3E80;
        g.r[6] = gsu_add(g.r[6], g.r[6], 0);
        g.r[6] = gsu_add(g.r[6], g.r[6], 0);
        g.r[0] = gsu_add(gsu_fmult(g.r[0], g.r[6]), g.r[1], 0);
        gsu_sbk(g.r[0]);
        g.r[4] = 0x001E;
        g.r[0] = gsu_add(REC_SPREAD, g.r[9], 0);
        gsu_stw(g.r[0], g.r[4]);
        g.r[2] = gsu_pull(sp);
        g.r[7] = gsu_pull(sp);
        g.r[5] = gsu_pull(sp);
        g.r[6] = gsu_pull(sp);
        g.r[8] = gsu_pull(sp);
    }
    g.r[8] = gsu_pull(sp);
    gsu_pull(sp);
    return g;
}

void abi_FxSpreadCars(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSpreadCars(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* The early exit: no car near; $2642 = -1, $2648 = 0. */
static FxRegs near_none(FxRegs g)
{
    g.r[0] = 0xFFFF;
    gsu_stw(FX_NEAR_LEVEL, g.r[0]);
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    gsu_stw(FX_NEAR_CAR, g.r[0]);
    gsu_pull(&g.r[10]);
    return g;
}

/*
 * @routine 04:CF08 FxNearestCar
 * WHAT IT DOES: Finds the car nearest the player's car ($2476; from the
 * view's position $00D8/$00DC instead when bit 3 of its +$64 is set) by
 * |dx| + |dz|, keeping each car's distance in its +$8C, and for the
 * nearest: $2648 = the car; $2642 = a smoothed level from how fast it
 * closed ((old distance - new) / 2 + $80, 0 at least, 255 at most, a
 * quarter of the way from the old level each step); $2646 = its distance
 * / 512, 7 at most; $264A/$264C = its offset turned into the view's axes
 * (FxTurn by the high byte of $00E0), x negated; $2644 = 7 + that z / 32,
 * 0 to 15. With no player car, no other car or no old distance: $2642 =
 * -1, $2648 = 0. The case of an empty cars' list is decoded from the ROM
 * bytes and not reached in the traces so far.
 * HOW: R5 = the best distance (from $1000), R7 its car, R6 its old +$8C;
 * the sums are checked for overflow (BVS). The return address is kept on
 * the R10 stack. The walk runs under CACHE (the code cache only; nothing
 * the C has to do).
 * Entry: R10 = stack; R11 = return address. Exit: R0 = the side value or
 * 0; R7 = the nearest car; R11 = $D002 when it got that far; S, Z from
 * R10; the last RAM address R10.
 */
FxRegs FxNearestCar(FxRegs g)
{
    int i;

    gsu_push(&g.r[10], g.r[11]);
    g.r[3] = gsu_ldw(FX_PLAYER);
    gsu_moves(g.r[3]);
    if (gsu.z)
        return near_none(g);
    g.r[1] = gsu_ldw(gsu_add(REC_X, g.r[3], 0));
    g.r[2] = gsu_ldw(gsu_add(REC_Z, g.r[3], 0));
    g.r[8] = gsu_ldw(gsu_add(REC_64, g.r[3], 0));
    g.r[0] = gsu_and(0x0008, g.r[8]);
    if (!gsu.z) {
        g.r[3] = 0;
        g.r[1] = gsu_ldw(FX_VIEW_X);
        g.r[2] = gsu_ldw(FX_VIEW_Z);
    }
    g.r[5] = 0x1000;
    g.r[6] = 0;
    g.r[0] = gsu_ldw(FX_CARS);
    gsu_moves(g.r[0]);
    if (gsu.z)
        return near_none(g);
    for (;; g.r[0] = gsu_ldw(gsu_add(REC_NEXT_CAR, g.r[8], 0))) {
        g.r[8] = g.r[0];
        gsu_moves(g.r[8]);
        if (gsu.z)
            break;
        gsu_sub(g.r[0], g.r[3], 0);
        if (gsu.z)
            continue;
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_X, g.r[8], 0)), g.r[1], 0);
        if (gsu.s)
            g.r[0] = gsu_inc(gsu_not(g.r[0]));
        g.r[4] = g.r[0];
        g.r[0] = gsu_sub(gsu_ldw(gsu_add(REC_Z, g.r[8], 0)), g.r[2], 0);
        if (gsu.s)
            g.r[0] = gsu_inc(gsu_not(g.r[0]));
        g.r[4] = gsu_add(g.r[4], g.r[0], 0);
        if (gsu.ov)
            continue;
        gsu_sub(g.r[4], g.r[5], 0);
        if (gsu.s) {
            g.r[6] = gsu_ldw(gsu_add(REC_NEAR, g.r[8], 0));
            g.r[5] = g.r[4];
            g.r[7] = g.r[8];
        }
        g.r[0] = gsu_add(REC_NEAR, g.r[8], 0);
        gsu_stw(g.r[0], g.r[4]);
    }
    g.r[0] = g.r[6];
    gsu_moves(g.r[0]);
    if (gsu.z)
        return near_none(g);
    g.r[0] = gsu_sub(g.r[6], g.r[5], 0);
    g.r[6] = gsu_div2(g.r[0]);
    g.r[0] = 0x0080;
    g.r[6] = gsu_add(g.r[6], g.r[0], 0);
    if (gsu.s)
        g.r[6] = 0;
    g.r[0] = gsu_ldw(FX_NEAR_LEVEL);
    gsu_moves(g.r[0]);
    if (gsu.s) {
        g.r[0] = g.r[6];
        gsu_stw(FX_NEAR_LEVEL, g.r[0]);
    }
    g.r[0] = gsu_sub(g.r[0], g.r[6], 0);
    g.r[0] = gsu_div2(gsu_div2(g.r[0]));
    g.r[8] = gsu_ldw(FX_NEAR_LEVEL);
    g.r[0] = gsu_sub(g.r[8], g.r[0], 0);
    gsu_sbk(g.r[0]);
    g.r[6] = 0x0080;
    g.r[0] = gsu_fmult(g.r[5], g.r[6]);
    g.r[8] = 0x0007;
    gsu_sub(g.r[0], g.r[8], 0);
    if (!gsu.s)
        g.r[0] = g.r[8];
    gsu_stw(FX_NEAR_BAND, g.r[0]);
    g.r[8] = 0x00FF;
    g.r[0] = gsu_ldw(FX_NEAR_LEVEL);
    gsu_sub(g.r[0], g.r[8], 0);
    if (!gsu.s)
        gsu_sbk(g.r[8]);
    gsu_stw(FX_NEAR_CAR, g.r[7]);
    g.r[0] = gsu_ldw(gsu_add(REC_X, g.r[7], 0));
    g.r[1] = gsu_sub(g.r[0], g.r[1], 0);
    g.r[0] = gsu_ldw(gsu_add(REC_Z, g.r[7], 0));
    g.r[3] = gsu_sub(g.r[0], g.r[2], 0);
    g.r[0] = gsu_ldw(FX_VIEW_YAW);
    g.r[2] = gsu_hib(g.r[0]);
    g.r[0] = 0;
    gsu.rombr = 0;
    g.r[11] = 0xD002;
    g = FxTurnRegs(g);
    gsu_stw(FX_NEAR_Z, g.r[3]);
    g.r[1] = gsu_inc(gsu_not(g.r[1]));
    gsu_stw(FX_NEAR_X, g.r[1]);
    g.r[0] = g.r[3];
    for (i = 0; i < 5; i++)
        g.r[0] = gsu_div2(g.r[0]);
    g.r[0] = gsu_add(g.r[0], 7, 0);
    if (gsu.s)
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[3] = 0x000F;
    gsu_sub(g.r[0], g.r[3], 0);
    if (!gsu.s)
        g.r[0] = g.r[3];
    gsu_stw(FX_NEAR_SIDE, g.r[0]);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxNearestCar(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxNearestCar(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
