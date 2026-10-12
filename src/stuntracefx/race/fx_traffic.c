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
