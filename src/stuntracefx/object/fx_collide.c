/*
 * fx_collide.c: the Super FX's push between two touching objects.
 *
 * R9 is the record being pushed and R7 the other's record; R1 points at
 * the other's contact words: its velocity x, z at +$04, +$06 and position
 * x, z at +$08, +$0A. A record's +$2A is its mass. The result goes to the
 * push at $70:23A4 (object/fx_record.c) as an amount along the unit vector
 * from the other's position to this one's.
 *
 * A record (R6) keeps a list of its contacts from +$18, linked through +$0E
 * (FxListClearBit0 and FxListSweep in object/fx_record.c and fx_pool.c
 * clear and sweep it): a contact is $12 bytes from the first pool, with
 * the other record at +$0C, flags at +$10 (bit 0: seen this step, bit 2:
 * keep the copy) and a copy of the other's velocity x, z and position x,
 * z at +$04-+$0A, the contact words FxCollidePush reads.
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
#define FX_REACH 0x006Au

#define REC_SIZE 0x0Cu
#define FX_TOUCH_SLACK 0x0294u
#define FX_POOL_FIRST 0x1098u
#define CONTACT_BYTES 0x0012u
#define REC_FLAGS16 0x16u
#define REC_CONTACTS 0x18u
#define CON_OTHER 0x0Cu
#define CON_NEXT 0x0Eu
#define CON_FLAGS 0x10u

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

/*
 * @routine 04:CA99 FxRecordsApart
 * WHAT IT DOES: Whether records R8 and R7 stay apart this step: R0 = 1 when
 * their distance on the ground plane is more than their sizes (+$0C each)
 * less $294 (no less than 0, decoded from the ROM bytes and not reached in
 * the traces so far) plus how fast they close (half the velocity
 * difference along the line between them), else R0 = 0.
 * HOW: The difference (R7 - R8) goes to $0068/$002E and FxNormalize makes
 * the unit vector; the distance is the difference, doubled, dotted with
 * it (FMULT) and kept at $0068; the closing speed is the velocity
 * difference halved (DIV2) dotted with it, negated, at $006A. R1-R5 and
 * the return address are kept on the R10 stack, R8 and R7 across the call.
 * Entry: R7, R8 = the records; R10 = stack; R11 = return address. Exit: R0
 * = 1 or 0; R1-R5, R7, R8 as on entry; R6 = the unit z; R9, R11-R13 as
 * FxNormalize leaves them; S, Z from R10; OV, CY from the CMP (R0 = 1) or
 * SUB R0 (R0 = 0); the last RAM address R10.
 */
FxRegs FxRecordsApart(FxRegs g)
{
    uint16_t *sp = &g.r[10];
    int i;

    gsu_push(sp, g.r[11]);
    for (i = 1; i <= 5; i++)
        gsu_push(sp, g.r[i]);
    g.r[1] = gsu_ldw(gsu_add(REC_X, g.r[7], 0));
    g.r[0] = gsu_ldw(gsu_add(REC_X, g.r[8], 0));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    gsu_stw(FX_NORMAL_X, g.r[1]);
    g.r[3] = gsu_ldw(gsu_add(REC_Z, g.r[7], 0));
    g.r[0] = gsu_ldw(gsu_add(REC_Z, g.r[8], 0));
    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
    gsu_stw(FX_NORMAL_Z, g.r[3]);
    g.r[2] = 0;
    gsu_push(sp, g.r[8]);
    gsu_push(sp, g.r[7]);
    g.r[11] = 0xCAE0;
    g = FxNormalize(g);
    g.r[7] = gsu_pull(sp);
    g.r[8] = gsu_pull(sp);

    g.r[0] = gsu_ldw(FX_NORMAL_X);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    g.r[6] = g.r[1];
    g.r[5] = g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    g.r[0] = gsu_ldw(FX_NORMAL_Z);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    g.r[6] = g.r[3];
    g.r[0] = gsu_add(gsu_fmult(g.r[0], g.r[6]), g.r[5], 0);
    gsu_stw(FX_NORMAL_X, g.r[0]);                   /* the distance */

    g.r[0] = gsu_ldw(gsu_add(REC_VEL_X, g.r[7], 0));
    g.r[4] = gsu_ldw(gsu_add(REC_VEL_X, g.r[8], 0));
    g.r[0] = gsu_div2(gsu_sub(g.r[0], g.r[4], 0));
    g.r[6] = g.r[1];
    g.r[5] = g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    g.r[0] = gsu_ldw(gsu_add(REC_VEL_Z, g.r[7], 0));
    g.r[4] = gsu_ldw(gsu_add(REC_VEL_Z, g.r[8], 0));
    g.r[0] = gsu_div2(gsu_sub(g.r[0], g.r[4], 0));
    g.r[6] = g.r[3];
    g.r[0] = gsu_add(gsu_fmult(g.r[0], g.r[6]), g.r[5], 0);
    g.r[0] = gsu_inc(gsu_not(g.r[0]));
    gsu_stw(FX_REACH, g.r[0]);                      /* the closing speed */

    g.r[0] = gsu_ldw(gsu_add(REC_SIZE, g.r[7], 0));
    g.r[1] = gsu_ldw(gsu_add(REC_SIZE, g.r[8], 0));
    g.r[0] = gsu_add(g.r[0], g.r[1], 0);
    g.r[1] = FX_TOUCH_SLACK;
    g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.s)
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[1] = gsu_ldw(FX_REACH);
    g.r[0] = gsu_add(g.r[0], g.r[1], 0);
    g.r[1] = gsu_ldw(FX_NORMAL_X);
    gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.s)
        g.r[0] = 1;
    else
        g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    for (i = 5; i >= 1; i--)
        g.r[i] = gsu_pull(sp);
    gsu_pull(sp);
    return g;
}

void abi_FxRecordsApart(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordsApart(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:A345 FxContactNote
 * WHAT IT DOES: Notes that record R6 touches record R8: finds R8's contact
 * on R6's list or makes one (a $12-byte block from the first pool, on the
 * front of the list, +$0C = R8, +$10 = 0), copies R8's velocity x, z and
 * position x, z into it unless bit 2 of its +$10 is set, and sets bit 0 of
 * +$10. Nothing is done when bit 7 of R6's +$16 is set. With no room in
 * the pool nothing is noted (decoded from the ROM bytes, as is the
 * whole-block case of the allocation below, and not reached in the traces
 * so far).
 * HOW: The allocation is FxPoolTake's done again inline, with R7 the size,
 * R9 the walk and R8 (kept on the R10 stack) the link: an exact fit stays
 * on the chain with its size negated, else the block is cut from the end
 * of a bigger one. The routine runs under CACHE (the code cache only;
 * nothing the C has to do).
 * Entry: R6 = the record; R8 = the other; R10 = stack; R11 = return
 * address. Exit: R7 = the contact; R0 = its new +$10; R9 = 1; S, Z from
 * the OR; the last RAM address the contact + $10. Bit 7 set: R0 = R6's
 * +$16 and $80; R9 = $80.
 */
FxRegs FxContactNote(FxRegs g)
{
    uint16_t *sp = &g.r[10];

    g.r[0] = gsu_ldw(gsu_add(REC_FLAGS16, g.r[6], 0));
    g.r[9] = 0x0080;
    g.r[0] = gsu_and(g.r[0], g.r[9]);
    if (!gsu.z)
        return g;
    g.r[7] = gsu_ldw(gsu_add(REC_CONTACTS, g.r[6], 0));
    gsu_moves(g.r[7]);
    while (!gsu.z) {
        g.r[0] = gsu_ldw(gsu_add(CON_OTHER, g.r[7], 0));
        gsu_sub(g.r[0], g.r[8], 0);
        if (gsu.z)
            goto found;
        g.r[0] = gsu_ldw(gsu_add(CON_NEXT, g.r[7], 0));
        g.r[7] = g.r[0];
        gsu_moves(g.r[7]);
    }
    gsu_push(sp, g.r[8]);
    g.r[9] = FX_POOL_FIRST;
    g.r[7] = CONTACT_BYTES;
    gsu_push(sp, g.r[9]);
    for (;;) {                                        /* an exact fit */
        g.r[8] = gsu_ldw(g.r[9]);
        g.r[9] = gsu_inc(gsu_inc(g.r[9]));
        g.r[0] = gsu_sub(gsu_ldw(g.r[9]), g.r[7], 0);
        if (gsu.z) {
            g.r[0] = gsu_pull(sp);
            g.r[7] = gsu_inc(gsu_not(g.r[7]));
            g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
            goto mark;
        }
        g.r[9] = g.r[8];
        gsu_moves(g.r[9]);
        if (gsu.z)
            break;
    }
    g.r[9] = gsu_pull(sp);
    for (;;) {                                        /* a bigger block */
        g.r[8] = gsu_ldw(g.r[9]);
        g.r[9] = gsu_inc(gsu_inc(g.r[9]));
        g.r[0] = gsu_sub(gsu_ldw(g.r[9]), g.r[7], 0);
        if (!gsu.s) {
            g.r[0] = gsu_sub(g.r[0], 4, 0);
            if (gsu.s) {                              /* too little to split */
                g.r[0] = gsu_add(g.r[0], 4, 0);
                g.r[7] = gsu_add(g.r[7], g.r[0], 0);
                g.r[7] = gsu_inc(gsu_not(g.r[7]));
                g.r[0] = 0xFFFC;
            }
            g.r[0] = gsu_add(g.r[0], 4, 0);
            gsu_stw(g.r[9], g.r[0]);
            goto mark;
        }
        g.r[9] = g.r[8];
        gsu_moves(g.r[9]);
        if (gsu.z) {
            g.r[7] = 0;                               /* no room */
            goto taken;
        }
    }
mark:
    g.r[8] = gsu_add(g.r[9], g.r[0], 0);
    gsu_stw(g.r[8], g.r[7]);
    g.r[8] = gsu_dec(gsu_dec(g.r[8]));
    g.r[7] = g.r[8];
taken:
    g.r[8] = gsu_pull(sp);
    gsu_moves(g.r[7]);
    if (gsu.z)
        return g;
    g.r[9] = CONTACT_BYTES;
    g.r[0] = gsu_add(CON_OTHER, g.r[7], 0);
    gsu_stw(g.r[0], g.r[8]);
    g.r[9] = gsu_ldw(gsu_add(REC_CONTACTS, g.r[6], 0));
    g.r[0] = gsu_add(CON_NEXT, g.r[7], 0);
    gsu_stw(g.r[0], g.r[9]);
    g.r[0] = gsu_add(REC_CONTACTS, g.r[6], 0);
    gsu_stw(g.r[0], g.r[7]);
    g.r[9] = 0;
    g.r[0] = gsu_add(CON_FLAGS, g.r[7], 0);
    gsu_stw(g.r[0], g.r[9]);
found:
    g.r[9] = gsu_ldw(gsu_add(CON_FLAGS, g.r[7], 0));
    g.r[0] = gsu_and(0x0004, g.r[9]);
    if (gsu.z) {
        static const uint8_t from[4] = {REC_VEL_X, REC_VEL_Z, REC_X, REC_Z};
        int i;

        for (i = 0; i < 4; i++) {
            g.r[9] = gsu_ldw(gsu_add(from[i], g.r[8], 0));
            g.r[0] = gsu_add((uint16_t)(HIT_VEL_X + 2 * i), g.r[7], 0);
            gsu_stw(g.r[0], g.r[9]);
        }
    }
    g.r[0] = gsu_ldw(gsu_add(CON_FLAGS, g.r[7], 0));
    g.r[9] = 0x0001;
    g.r[0] = gsu_or(g.r[0], g.r[9]);
    gsu_sbk(g.r[0]);
    return g;
}

void abi_FxContactNote(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxContactNote(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
