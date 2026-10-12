/*
 * fx_collide.c: the Super FX's push between two touching objects.
 *
 * R9 is the record being pushed and R7 the other's record; R1 points at
 * the other's contact words: its velocity x, z at +$04, +$06 and position
 * x, z at +$08, +$0A. A record's +$2A is its mass. The result goes to the
 * push at $70:23A4 (object/fx_record.c) as an amount along the unit vector
 * from the other's position to this one's.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define REC_X 0x1Eu
#define REC_Z 0x22u
#define REC_MASS 0x2Au
#define REC_VEL_X 0x2Eu
#define REC_VEL_Z 0x32u
#define HIT_VEL_X 0x04u
#define HIT_VEL_Z 0x06u
#define HIT_X 0x08u
#define HIT_Z 0x0Au

#define FX_ANGLES 0x0020u      /* the difference, also FxMatrixFromAngles' angles */
#define FX_NORMAL_X 0x0068u
#define FX_NORMAL_Y 0x002Cu
#define FX_NORMAL_Z 0x002Eu
#define FX_CLOSING 0x26F6u
#define FX_PUSH 0x23A4u

/*
 * @routine 04:9301 FxCollidePush
 * WHAT IT DOES: Sets the push on record R9 from the object at R5 (record
 * R7): along the unit vector from the other's position to R9's (on the
 * ground plane), with amount = the closing speed along it (the velocity
 * difference, doubled, dotted with the unit vector; raised to $04B0 when it
 * is from 0 to $04B0) times the other's share of the two masses, halved.
 * HOW: The difference goes to $0020/$0022/$0024 and FxNormalize makes the
 * unit vector, kept at $0068/$002C/$002E; the closing speed is two FMULTs
 * and is kept at $26F6; FxDivideHalvedIfLarge gives the mass share as
 * other * $10000 / (both), halved with LSR; the amount is FMULT of the
 * doubled speed by it. The push is on when the amount is not negative.
 * R11, R1, R8 and the registers FxNormalize and the divide would lose are
 * kept on the R10 stack.
 * Entry: R1 = the contact words; R7 = the other's record; R9 = the
 * record; R10 = stack; R11 = return address. Exit: R0 = 1 (on) or 0; R1,
 * R7, R8, R9 as on entry; R5 = R1; R2 = 0; R3 = the unit z; R4 = the share
 * before halving; R6 = the share; R11 = $93B3; R12 = 0; R13 = $C14B; S, Z
 * from R10, OV, CY from the SUB setting R0; the last RAM address R10.
 */
FxRegs FxCollidePush(FxRegs g)
{
    uint16_t *sp = &g.r[10], speed;
    int negative;
    FxQuotient q;

    gsu_push(sp, g.r[11]);
    gsu_push(sp, g.r[1]);
    gsu_push(sp, g.r[8]);
    g.r[5] = g.r[1];
    g.r[1] = gsu_ldw(gsu_add(REC_X, g.r[9], 0));
    g.r[0] = gsu_ldw(gsu_add(HIT_X, g.r[5], 0));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    gsu_stw(FX_ANGLES, g.r[1]);
    g.r[2] = 0;
    gsu_stw(FX_ANGLES + 2, g.r[2]);
    g.r[3] = gsu_ldw(gsu_add(REC_Z, g.r[9], 0));
    g.r[0] = gsu_ldw(gsu_add(HIT_Z, g.r[5], 0));
    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
    gsu_stw(FX_ANGLES + 4, g.r[3]);
    gsu_push(sp, g.r[7]);
    gsu_push(sp, g.r[9]);
    gsu_push(sp, g.r[5]);
    g.r[11] = 0x9346;
    g = FxNormalize(g);
    gsu_stw(FX_NORMAL_X, g.r[1]);
    gsu_stw(FX_NORMAL_Y, g.r[2]);
    gsu_stw(FX_NORMAL_Z, g.r[3]);
    g.r[5] = gsu_pull(sp);
    g.r[9] = gsu_pull(sp);
    g.r[7] = gsu_pull(sp);

    g.r[0] = gsu_ldw(gsu_add(HIT_VEL_X, g.r[5], 0));
    g.r[1] = gsu_ldw(gsu_add(REC_VEL_X, g.r[9], 0));
    g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    g.r[6] = gsu_ldw(FX_NORMAL_X);
    g.r[8] = gsu_fmult(g.r[0], g.r[6]);
    g.r[0] = gsu_ldw(gsu_add(HIT_VEL_Z, g.r[5], 0));
    g.r[1] = gsu_ldw(gsu_add(REC_VEL_Z, g.r[9], 0));
    g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    g.r[6] = gsu_ldw(FX_NORMAL_Z);
    g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    g.r[8] = gsu_add(g.r[8], g.r[0], 0);
    gsu_moves(g.r[8]);
    if (!gsu.s) {
        g.r[0] = 0x04B0;
        gsu_sub(g.r[0], g.r[8], 0);
        if (!gsu.s)
            g.r[8] = g.r[0];
    }
    gsu_stw(FX_CLOSING, g.r[8]);

    gsu_push(sp, g.r[7]);
    gsu_push(sp, g.r[9]);
    gsu_push(sp, g.r[8]);
    g.r[0] = gsu_ldw(gsu_add(REC_MASS, g.r[7], 0));
    g.r[6] = gsu_ldw(gsu_add(REC_MASS, g.r[9], 0));
    g.r[6] = gsu_add(g.r[6], g.r[0], 0);
    g.r[4] = 0;
    g.r[11] = 0x93B3;
    q = FxDivideHalvedIfLarge(g.r[0], g.r[4], &g.r[6]);
    g.r[0] = q.rem;
    g.r[4] = q.quot;
    g.r[12] = 0;
    g.r[13] = 0xC14B;
    g.r[6] = gsu_lsr(g.r[4]);
    g.r[8] = gsu_pull(sp);
    g.r[9] = gsu_pull(sp);
    g.r[7] = gsu_pull(sp);
    speed = g.r[8];
    g.r[0] = gsu_add(speed, speed, 0);
    g.r[8] = g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    g.r[1] = gsu_ldw(FX_NORMAL_X);
    g.r[2] = 0;
    g.r[3] = gsu_ldw(FX_NORMAL_Z);
    gsu_moves(g.r[8]);
    negative = gsu.s;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);   /* in the branch's delay slot */
    if (!negative)
        g.r[0] = 1;
    gsu_stw(FX_PUSH, g.r[0]);
    gsu_stw(FX_PUSH + 2, g.r[8]);
    gsu_stw(FX_PUSH + 4, g.r[1]);
    gsu_stw(FX_PUSH + 6, g.r[2]);
    gsu_stw(FX_PUSH + 8, g.r[3]);
    g.r[8] = gsu_pull(sp);
    g.r[1] = gsu_pull(sp);
    gsu_pull(sp);
    return g;
}

void abi_FxCollidePush(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCollidePush(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
