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
#define REC_Y 0x20u
#define REC_SIZE_Y 0x0Eu
#define REC_SIZE_Z 0x10u
#define REC_MASK 0x1Au
#define REC_ALL_NEXT 0x04u
#define FX_SIZES 0x236Cu       /* R1's sizes: x, y, z */
#define FX_RECORDS 0x2332u

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
FxRegs FxCollidePush(FxRegs g, int copy)
{
    uint16_t *sp = &g.r[10], speed;
    int negative;
    FxQuotient q;

    gsu_code(0x049302, 2);
    gsu_push(sp, g.r[11]);
    gsu_code(0x049304, 4);
    gsu_push(sp, g.r[1]);
    gsu_code(0x049308, 4);
    gsu_push(sp, g.r[8]);
    g.r[5] = g.r[1];
    gsu_code(0x04930c, 10);
    g.r[1] = gsu_ldw(gsu_add(REC_X, g.r[9], 0));
    gsu_code(0x049316, 4);
    g.r[0] = gsu_ldw(gsu_add(HIT_X, g.r[5], 0));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    gsu_code(0x04931a, 6);
    gsu_stw(FX_ANGLES, g.r[1]);
    g.r[2] = 0;
    gsu_code(0x049320, 6);
    gsu_stw(FX_ANGLES + 2, g.r[2]);
    gsu_code(0x049326, 6);
    g.r[3] = gsu_ldw(gsu_add(REC_Z, g.r[9], 0));
    gsu_code(0x04932c, 4);
    g.r[0] = gsu_ldw(gsu_add(HIT_Z, g.r[5], 0));
    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
    gsu_code(0x049330, 6);
    gsu_stw(FX_ANGLES + 4, g.r[3]);
    gsu_code(0x049336, 2);
    gsu_push(sp, g.r[7]);
    gsu_code(0x049338, 4);
    gsu_push(sp, g.r[9]);
    gsu_code(0x04933c, 4);
    gsu_push(sp, g.r[5]);
    g.r[11] = 0x9346;
    
    gsu_code(0x049340, 6); gsu_code(0x04c087, 1);
    g = FxNormalize(g, GSU_COPY(0));
    gsu_code(0x049346, 4);
    gsu_stw(FX_NORMAL_X, g.r[1]);
    gsu_code(0x04934a, 3);
    gsu_stw(FX_NORMAL_Y, g.r[2]);
    gsu_code(0x04934d, 3);
    gsu_stw(FX_NORMAL_Z, g.r[3]);
    gsu_code(0x049350, 4);
    g.r[5] = gsu_pull(sp);
    gsu_code(0x049354, 4);
    g.r[9] = gsu_pull(sp);
    gsu_code(0x049358, 4);
    g.r[7] = gsu_pull(sp);

    gsu_code(0x04935c, 4);
    g.r[0] = gsu_ldw(gsu_add(HIT_VEL_X, g.r[5], 0));
    gsu_code(0x049360, 6);
    g.r[1] = gsu_ldw(gsu_add(REC_VEL_X, g.r[9], 0));
    g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    gsu_code(0x049366, 6);
    g.r[6] = gsu_ldw(FX_NORMAL_X);
    g.r[8] = gsu_fmult(g.r[0], g.r[6]);
    gsu_code(0x04936c, 7);
    g.r[0] = gsu_ldw(gsu_add(HIT_VEL_Z, g.r[5], 0));
    gsu_code(0x049373, 6);
    g.r[1] = gsu_ldw(gsu_add(REC_VEL_Z, g.r[9], 0));
    g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    gsu_code(0x049379, 6);
    g.r[6] = gsu_ldw(FX_NORMAL_Z);
    g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    g.r[8] = gsu_add(g.r[8], g.r[0], 0);
    gsu_moves(g.r[8]);
    if (!gsu.s) {
        g.r[0] = 0x04B0;
        gsu_sub(g.r[0], g.r[8], 0);
        if (!gsu.s)
            { g.r[8] = g.r[0]; gsu_code(0x04937f, 18); } else { gsu_code(0x04937f, 15); gsu_code(0x049390, 1); }
    } else { gsu_code(0x04937f, 7); gsu_code(0x049390, 1); }
    gsu_code(0x049391, 4);
    gsu_stw(FX_CLOSING, g.r[8]);

    gsu_code(0x049395, 2);
    gsu_push(sp, g.r[7]);
    gsu_code(0x049397, 4);
    gsu_push(sp, g.r[9]);
    gsu_code(0x04939b, 4);
    gsu_push(sp, g.r[8]);
    gsu_code(0x04939f, 6);
    g.r[0] = gsu_ldw(gsu_add(REC_MASS, g.r[7], 0));
    gsu_code(0x0493a5, 6);
    g.r[6] = gsu_ldw(gsu_add(REC_MASS, g.r[9], 0));
    g.r[6] = gsu_add(g.r[6], g.r[0], 0);
    g.r[4] = 0;
    g.r[11] = 0x93B3;
    
    gsu_code(0x0493ab, 8); gsu_code(0x04c135, 1);
    q = FxDivideHalvedIfLarge(g.r[0], g.r[4], &g.r[6], GSU_COPY(0));
    g.r[0] = q.rem;
    g.r[4] = q.quot;
    g.r[12] = 0;
    g.r[13] = 0xC14B;
    g.r[6] = gsu_lsr(g.r[4]);
    gsu_code(0x0493b3, 8);
    g.r[8] = gsu_pull(sp);
    gsu_code(0x0493bb, 4);
    g.r[9] = gsu_pull(sp);
    gsu_code(0x0493bf, 4);
    g.r[7] = gsu_pull(sp);
    speed = g.r[8];
    g.r[0] = gsu_add(speed, speed, 0);
    g.r[8] = g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    gsu_code(0x0493c3, 10);
    g.r[1] = gsu_ldw(FX_NORMAL_X);
    g.r[2] = 0;
    gsu_code(0x0493cd, 6);
    g.r[3] = gsu_ldw(FX_NORMAL_Z);
    gsu_moves(g.r[8]);
    negative = gsu.s;
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);   /* in the branch's delay slot */
    if (!negative)
        { g.r[0] = 1; gsu_code(0x0493d3, 7); } else { gsu_code(0x0493d3, 4); gsu_code(0x0493d9, 1); }
    gsu_code(0x0493da, 4);
    gsu_stw(FX_PUSH, g.r[0]);
    gsu_code(0x0493de, 4);
    gsu_stw(FX_PUSH + 2, g.r[8]);
    gsu_code(0x0493e2, 4);
    gsu_stw(FX_PUSH + 4, g.r[1]);
    gsu_code(0x0493e6, 4);
    gsu_stw(FX_PUSH + 6, g.r[2]);
    gsu_code(0x0493ea, 4);
    gsu_stw(FX_PUSH + 8, g.r[3]);
    gsu_code(0x0493ee, 4);
    g.r[8] = gsu_pull(sp);
    gsu_code(0x0493f2, 4);
    g.r[1] = gsu_pull(sp);
    gsu_code(0x0493f6, 4);
    gsu_pull(sp);
    return g;
}

void abi_FxCollidePush(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxCollidePush(g, GSU_COPY(0));
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
/* gsuwalk: the code of each copy of FxRecordsApart, read from the ROM */
static const GsuCode FxRecordsApart_fetch[1][20] = {
    {{0x04ca9c, 4}, {0x04caa0, 4}, {0x04caa4, 4}, {0x04caa8, 4}, {0x04caac, 4}, {0x04cb45, 4}, {0x04cb63, 7}, {0x04cb6a, 4}, {0, 0}, {0x04cb6e, 4}, {0, 0}, {0x04cb72, 4}, {0, 0}, {0x04cb76, 4}, {0, 0}, {0x04cb49, 6}, {0x04cb4f, 4}, {0x04cb53, 4}, {0x04cb57, 4}, {0x04cb5b, 4}},
};

FxRegs FxRecordsApart(FxRegs g, int copy)
{
    uint16_t *sp = &g.r[10];
    int i;

    gsu_code(0x04ca9a, 2);
    gsu_push(sp, g.r[11]);
    for (i = 1; i <= 5; i++)
        { 
        gsu_codes(FxRecordsApart_fetch[0] + (i - 1), 1);
        gsu_push(sp, g.r[i]); }
    gsu_code(0x04cab0, 8);
    g.r[1] = gsu_ldw(gsu_add(REC_X, g.r[7], 0));
    gsu_code(0x04cab8, 4);
    g.r[0] = gsu_ldw(gsu_add(REC_X, g.r[8], 0));
    g.r[1] = gsu_sub(g.r[1], g.r[0], 0);
    gsu_code(0x04cabc, 6);
    gsu_stw(FX_NORMAL_X, g.r[1]);
    gsu_code(0x04cac2, 6);
    g.r[3] = gsu_ldw(gsu_add(REC_Z, g.r[7], 0));
    gsu_code(0x04cac8, 4);
    g.r[0] = gsu_ldw(gsu_add(REC_Z, g.r[8], 0));
    g.r[3] = gsu_sub(g.r[3], g.r[0], 0);
    gsu_code(0x04cacc, 6);
    gsu_stw(FX_NORMAL_Z, g.r[3]);
    g.r[2] = 0;
    gsu_code(0x04cad2, 4);
    gsu_push(sp, g.r[8]);
    gsu_code(0x04cad6, 4);
    gsu_push(sp, g.r[7]);
    g.r[11] = 0xCAE0;
    
    gsu_code(0x04cada, 6); gsu_code(0x04c087, 1);
    g = FxNormalize(g, GSU_COPY(0));
    gsu_code(0x04cae0, 5);
    g.r[7] = gsu_pull(sp);
    gsu_code(0x04cae5, 4);
    g.r[8] = gsu_pull(sp);

    gsu_code(0x04cae9, 4);
    g.r[0] = gsu_ldw(FX_NORMAL_X);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    g.r[6] = g.r[1];
    g.r[5] = g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    gsu_code(0x04caed, 10);
    g.r[0] = gsu_ldw(FX_NORMAL_Z);
    g.r[0] = gsu_add(g.r[0], g.r[0], 0);
    g.r[6] = g.r[3];
    g.r[0] = gsu_add(gsu_fmult(g.r[0], g.r[6]), g.r[5], 0);
    gsu_code(0x04caf7, 9);
    gsu_stw(FX_NORMAL_X, g.r[0]);                   /* the distance */

    gsu_code(0x04cb00, 4);
    g.r[0] = gsu_ldw(gsu_add(REC_VEL_X, g.r[7], 0));
    gsu_code(0x04cb04, 6);
    g.r[4] = gsu_ldw(gsu_add(REC_VEL_X, g.r[8], 0));
    g.r[0] = gsu_div2(gsu_sub(g.r[0], g.r[4], 0));
    g.r[6] = g.r[1];
    g.r[5] = g.r[0] = gsu_fmult(g.r[0], g.r[6]);
    gsu_code(0x04cb0a, 12);
    g.r[0] = gsu_ldw(gsu_add(REC_VEL_Z, g.r[7], 0));
    gsu_code(0x04cb16, 6);
    g.r[4] = gsu_ldw(gsu_add(REC_VEL_Z, g.r[8], 0));
    g.r[0] = gsu_div2(gsu_sub(g.r[0], g.r[4], 0));
    g.r[6] = g.r[3];
    g.r[0] = gsu_add(gsu_fmult(g.r[0], g.r[6]), g.r[5], 0);
    g.r[0] = gsu_inc(gsu_not(g.r[0]));
    gsu_code(0x04cb1c, 13);
    gsu_stw(FX_REACH, g.r[0]);                      /* the closing speed */

    gsu_code(0x04cb29, 4);
    g.r[0] = gsu_ldw(gsu_add(REC_SIZE, g.r[7], 0));
    gsu_code(0x04cb2d, 6);
    g.r[1] = gsu_ldw(gsu_add(REC_SIZE, g.r[8], 0));
    g.r[0] = gsu_add(g.r[0], g.r[1], 0);
    g.r[1] = FX_TOUCH_SLACK;
    g.r[0] = gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.s)
        { g.r[0] = gsu_sub(g.r[0], g.r[0], 0); gsu_code(0x04cb33, 9); } else { gsu_code(0x04cb33, 7); gsu_code(0x04cb3b, 1); }
    gsu_code(0x04cb3c, 4);
    g.r[1] = gsu_ldw(FX_REACH);
    g.r[0] = gsu_add(g.r[0], g.r[1], 0);
    gsu_code(0x04cb40, 5);
    g.r[1] = gsu_ldw(FX_NORMAL_X);
    gsu_sub(g.r[0], g.r[1], 0);
    if (gsu.s) {                                    /* each way its own pulls and return */
        g.r[0] = 1;
        for (i = 5; i >= 1; i--)
            { gsu_codes(FxRecordsApart_fetch[0] + 5 + 2 * (5 - i), 2);
            g.r[i] = gsu_pull(sp); }
        gsu_code(0x04cb7a, 4);
        gsu_pull(sp);
        return g;
    } else { gsu_code(0x04cb45, 4); }
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    for (i = 5; i >= 1; i--)
        { gsu_codes(FxRecordsApart_fetch[0] + 15 + (5 - i), 1);
        g.r[i] = gsu_pull(sp); }
    gsu_code(0x04cb5f, 4);
    gsu_pull(sp);
    return g;
}

void abi_FxRecordsApart(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordsApart(g, GSU_COPY(0));
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
/* gsuwalk: the code of each copy of FxContactNote, read from the ROM */
static const GsuCode FxContactNote_fetch[1][8] = {
    {{0x04a400, 13}, {0x04a412, 6}, {0x04a41d, 6}, {0x04a428, 6}, {0x04a40d, 5}, {0x04a418, 5}, {0x04a423, 5}, {0x04a42e, 5}},
};

FxRegs FxContactNote(FxRegs g, int copy)
{
    uint16_t *sp = &g.r[10];

    gsu_code(0x04a346, 4);
    g.r[0] = gsu_ldw(gsu_add(REC_FLAGS16, g.r[6], 0));
    g.r[9] = 0x0080;
    g.r[0] = gsu_and(g.r[0], g.r[9]);
    if (!gsu.z)
        { 
        gsu_code(0x04a34a, 10); gsu_code(0x04a43b, 3);
        return g; } else { gsu_code(0x04a34a, 6); gsu_code(0x04a354, 1); }
    gsu_code(0x04a355, 6);
    g.r[7] = gsu_ldw(gsu_add(REC_CONTACTS, g.r[6], 0));
    gsu_moves(g.r[7]);
    if (!gsu.z) {                                     /* BEQ first, then BNE at the bottom */
        gsu_code(0x04a35b, 5);
        do {
            gsu_code(0x04a360, 4);
            g.r[0] = gsu_ldw(gsu_add(CON_OTHER, g.r[7], 0));
            gsu_sub(g.r[0], g.r[8], 0);
            if (gsu.z)
                { gsu_code(0x04a364, 8); gsu_code(0x04a3f9, 1);
                goto found; } else { gsu_code(0x04a364, 4); gsu_code(0x04a36c, 1); }
            gsu_code(0x04a36d, 4);
            g.r[0] = gsu_ldw(gsu_add(CON_NEXT, g.r[7], 0));
            g.r[7] = g.r[0];
            gsu_moves(g.r[7]);
        } while (gsu_code_loop(0x04a35f, 1, gsu_code_loop(0x04a371, 4, !gsu.z)));
        gsu_code(0x04a371, 4);
        gsu_code(0x04a375, 1);
    } else { gsu_code(0x04a35b, 4); gsu_code(0x04a375, 1); }
    gsu_code(0x04a376, 2);
    gsu_push(sp, g.r[8]);
    g.r[9] = FX_POOL_FIRST;
    g.r[7] = CONTACT_BYTES;
    
    gsu_code(0x04a378, 5); gsu_cache(0xa37d); gsu_code(0x04a37d, 5);
    gsu_push(sp, g.r[9]);
    gsu_code(0x04a382, 2);
    for (;;) {                                        /* an exact fit */
        gsu_code(0x04a384, 2);
        g.r[8] = gsu_ldw(g.r[9]);
        g.r[9] = gsu_inc(gsu_inc(g.r[9]));
        gsu_code(0x04a386, 3);
        g.r[0] = gsu_sub(gsu_ldw(g.r[9]), g.r[7], 0);
        if (gsu.z) {
            
            gsu_code(0x04a389, 3); gsu_code(0x04a3a8, 4);
            g.r[0] = gsu_pull(sp);
            g.r[7] = gsu_inc(gsu_not(g.r[7]));
            g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
            
            gsu_code(0x04a3ac, 6); gsu_code(0x04a3c3, 1);
            goto mark;
        } else { gsu_code(0x04a389, 3); }
        g.r[9] = g.r[8];
        gsu_moves(g.r[9]);
        if (gsu.z)
            { 
            gsu_code(0x04a38c, 5);
            break; } else { gsu_code(0x04a38c, 5); gsu_code(0x04a383, 1); }
    }
    gsu_code(0x04a391, 5);
    g.r[9] = gsu_pull(sp);
    for (;;) {                                        /* a bigger block */
        gsu_code(0x04a396, 2);
        g.r[8] = gsu_ldw(g.r[9]);
        g.r[9] = gsu_inc(gsu_inc(g.r[9]));
        gsu_code(0x04a398, 3);
        g.r[0] = gsu_sub(gsu_ldw(g.r[9]), g.r[7], 0);
        if (!gsu.s) {
            g.r[0] = gsu_sub(g.r[0], 4, 0);
            if (gsu.s) {                              /* too little to split */
                g.r[0] = gsu_add(g.r[0], 4, 0);
                g.r[7] = gsu_add(g.r[7], g.r[0], 0);
                g.r[7] = gsu_inc(gsu_not(g.r[7]));
                g.r[0] = 0xFFFC;
                
                gsu_code(0x04a39b, 3); gsu_code(0x04a3b2, 15);
            } else { gsu_code(0x04a39b, 3); gsu_code(0x04a3b2, 5); gsu_code(0x04a3c0, 1); }
            g.r[0] = gsu_add(g.r[0], 4, 0);
            gsu_code(0x04a3c1, 3);
            gsu_stw(g.r[9], g.r[0]);
            goto mark;
        } else { gsu_code(0x04a39b, 3); }
        g.r[9] = g.r[8];
        gsu_moves(g.r[9]);
        if (gsu.z) {
            g.r[7] = 0;                               /* no room */
            
            gsu_code(0x04a39e, 10); gsu_code(0x04a3cc, 1);
            goto taken;
        } else { gsu_code(0x04a39e, 5); gsu_code(0x04a395, 1); }
    }
mark:
    ;
    g.r[8] = gsu_add(g.r[9], g.r[0], 0);
    gsu_code(0x04a3c4, 5);
    gsu_stw(g.r[8], g.r[7]);
    g.r[8] = gsu_dec(gsu_dec(g.r[8]));
    g.r[7] = g.r[8];
gsu_code(0x04a3c9, 4);
taken:
    ;
    gsu_code(0x04a3cd, 4);
    g.r[8] = gsu_pull(sp);
    gsu_moves(g.r[7]);
    if (gsu.z)
        { 
        gsu_code(0x04a3d1, 8); gsu_code(0x04a43e, 3);
        return g; } else { gsu_code(0x04a3d1, 4); gsu_code(0x04a3d9, 1); }
    g.r[9] = CONTACT_BYTES;
    g.r[0] = gsu_add(CON_OTHER, g.r[7], 0);
    gsu_code(0x04a3da, 9);
    gsu_stw(g.r[0], g.r[8]);
    gsu_code(0x04a3e3, 6);
    g.r[9] = gsu_ldw(gsu_add(REC_CONTACTS, g.r[6], 0));
    g.r[0] = gsu_add(CON_NEXT, g.r[7], 0);
    gsu_code(0x04a3e9, 5);
    gsu_stw(g.r[0], g.r[9]);
    g.r[0] = gsu_add(REC_CONTACTS, g.r[6], 0);
    gsu_code(0x04a3ee, 5);
    gsu_stw(g.r[0], g.r[7]);
    g.r[9] = 0;
    g.r[0] = gsu_add(CON_FLAGS, g.r[7], 0);
    gsu_code(0x04a3f3, 7);
    gsu_stw(g.r[0], g.r[9]);
found:
    ;
    gsu_code(0x04a3fa, 6);
    g.r[9] = gsu_ldw(gsu_add(CON_FLAGS, g.r[7], 0));
    g.r[0] = gsu_and(0x0004, g.r[9]);
    if (gsu.z) {
        static const uint8_t from[4] = {REC_VEL_X, REC_VEL_Z, REC_X, REC_Z};
        int i;

        for (i = 0; i < 4; i++) {
            gsu_codes(FxContactNote_fetch[0] + i, 1);
            g.r[9] = gsu_ldw(gsu_add(from[i], g.r[8], 0));
            g.r[0] = gsu_add((uint16_t)(HIT_VEL_X + 2 * i), g.r[7], 0);
            gsu_codes(FxContactNote_fetch[0] + 4 + i, 1);
            gsu_stw(g.r[0], g.r[9]);
        }
    } else { gsu_code(0x04a400, 6); gsu_code(0x04a432, 1); }
    gsu_code(0x04a433, 4);
    g.r[0] = gsu_ldw(gsu_add(CON_FLAGS, g.r[7], 0));
    g.r[9] = 0x0001;
    g.r[0] = gsu_or(g.r[0], g.r[9]);
    gsu_code(0x04a437, 5);
    gsu_sbk(g.r[0]);
    gsu_code(0x04a43c, 2);
    return g;
}

void abi_FxContactNote(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxContactNote(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/* Whether |a - b| - (R2's size + R1's size) is negative for one axis: R6 =
   the summed size, R0 the difference. */
/* gsuwalk: the code of each copy of axis_overlaps, read from the ROM */
static const GsuCode axis_overlaps_fetch[3][6] = {
    {{0x04a249, 6}, {0x04a24f, 4}, {0x04a253, 6}, {0x04a259, 3}, {0x04a25e, 1}, {0x04a259, 6}},
    {{0x04a263, 6}, {0x04a269, 4}, {0x04a26d, 6}, {0x04a273, 3}, {0x04a278, 1}, {0x04a273, 6}},
    {{0x04a27d, 6}, {0x04a283, 4}, {0x04a287, 6}, {0x04a28d, 3}, {0x04a292, 1}, {0x04a28d, 6}},
};

static int axis_overlaps(FxRegs *g, uint16_t size, uint16_t mine, uint16_t pos, uint16_t b, int copy)
{
    gsu_codes(axis_overlaps_fetch[copy], 1);
    g->r[6] = gsu_ldw(gsu_add(size, g->r[2], 0));
    gsu_codes(axis_overlaps_fetch[copy] + 1, 1);
    g->r[0] = gsu_ldw(mine);
    g->r[6] = gsu_add(g->r[6], g->r[0], 0);
    gsu_codes(axis_overlaps_fetch[copy] + 2, 1);
    g->r[0] = gsu_sub(gsu_ldw(gsu_add(pos, g->r[2], 0)), b, 0);
    if (gsu.s)
        { g->r[0] = gsu_inc(gsu_not(g->r[0])); gsu_codes(axis_overlaps_fetch[copy] + 5, 1); } else { gsu_codes(axis_overlaps_fetch[copy] + 3, 2); }
    g->r[0] = gsu_sub(g->r[0], g->r[6], 0);
    return gsu.s;
}

/* Whether field `a` of record `ra` and the flags (+$16) of record `rb`
   share no bit: R6 = those flags, R0 the AND. */
/* gsuwalk: the code of each copy of masks_clear, read from the ROM */
static const GsuCode masks_clear_fetch[2][2] = {
    {{0x04a2bb, 4}, {0x04a2bf, 6}},
    {{0x04a2d2, 4}, {0x04a2d6, 6}},
};

static int masks_clear(FxRegs *g, uint16_t ra, uint16_t rb, int copy)
{
    gsu_codes(masks_clear_fetch[copy], 1);
    g->r[0] = gsu_ldw(gsu_add(REC_MASK, ra, 0));
    gsu_codes(masks_clear_fetch[copy] + 1, 1);
    g->r[6] = gsu_ldw(gsu_add(REC_FLAGS16, rb, 0));
    g->r[0] = gsu_and(g->r[0], g->r[6]);
    return gsu.z;
}

/*
 * @routine 04:A20B FxRecordTouches
 * WHAT IT DOES: Tests record R1 against every record after it on the list
 * of all records ($2332, linked through +$04): their boxes overlap when on
 * each axis (z, then x, then y) the distance between the positions is
 * below the two sizes (+$10, +$0C, +$0E) added. For an overlapping pair:
 * when both have bit 1 of +$16 set and FxRecordsApart says they stay
 * apart, nothing; otherwise each notes the other as a contact
 * (FxContactNote) unless its +$1A shares a bit with the other's +$16.
 * Then, when R1 has contacts, FxListSweep drops those not seen this step.
 * HOW: R1's position goes to R3-R5 and its sizes to $236C/$236E/$2370;
 * R2 walks the list. The return address is kept on the R10 stack.
 * Entry: R1 = the record; R10 = stack; R11 = return address. Exit: R2 = 0;
 * R1, R3-R5 as above; the others from the last test or call; S, Z from
 * R10; the last RAM address R10.
 */
/* gsuwalk: the code of each copy of FxRecordTouches, read from the ROM */
static const GsuCode FxRecordTouches_fetch[1][9] = {
    {{0x04a20e, 8}, {0x04a216, 6}, {0x04a21c, 6}, {0x04a222, 4}, {0x04a22a, 4}, {0x04a232, 4}, {0x04a226, 4}, {0x04a22e, 4}, {0x04a236, 4}},
};

FxRegs FxRecordTouches(FxRegs g)
{
    int i;

    gsu_code(0x04a20c, 2);
    gsu_push(&g.r[10], g.r[11]);
    for (i = 0; i < 3; i++)
        { 
        gsu_codes(FxRecordTouches_fetch[0] + i, 1);
        g.r[3 + i] = gsu_ldw(gsu_add((uint16_t)(REC_X + 2 * i), g.r[1], 0)); }
    for (i = 0; i < 3; i++) {
        gsu_codes(FxRecordTouches_fetch[0] + 3 + i, 1);
        g.r[0] = gsu_ldw(gsu_add((uint16_t)(REC_SIZE + 2 * i), g.r[1], 0));
        gsu_codes(FxRecordTouches_fetch[0] + 6 + i, 1);
        gsu_stw((uint16_t)(FX_SIZES + 2 * i), g.r[0]);
    }
    g.r[2] = g.r[1];
    gsu_code(0x04a23a, 2);
    for (;;) {
        gsu_code(0x04a23c, 4);
        g.r[0] = gsu_ldw(gsu_add(REC_ALL_NEXT, g.r[2], 0));
        g.r[2] = g.r[0];
        gsu_moves(g.r[2]);
        if (gsu.z)
            { 
            gsu_code(0x04a240, 4);
            break; } else { gsu_code(0x04a240, 4); gsu_code(0x04a248, 1); }
        if (!axis_overlaps(&g, REC_SIZE_Z, FX_SIZES + 4, REC_Z, g.r[5], GSU_COPY(0)))
            { gsu_code(0x04a25f, 3); gsu_code(0x04a23b, 1);
            continue; } else { gsu_code(0x04a25f, 3); }
        gsu_code(0x04a262, 1);
        if (!axis_overlaps(&g, REC_SIZE, FX_SIZES, REC_X, g.r[3], GSU_COPY(1)))
            { gsu_code(0x04a279, 3); gsu_code(0x04a23b, 1);
            continue; } else { gsu_code(0x04a279, 3); }
        gsu_code(0x04a27c, 1);
        if (!axis_overlaps(&g, REC_SIZE_Y, FX_SIZES + 2, REC_Y, g.r[4], GSU_COPY(2)))
            { 
            gsu_code(0x04a293, 7); gsu_code(0x04a23b, 1);
            continue; } else { gsu_code(0x04a293, 3); gsu_code(0x04a29a, 1); }
        gsu_code(0x04a29b, 4);
        g.r[0] = gsu_ldw(gsu_add(REC_FLAGS16, g.r[1], 0));
        gsu_code(0x04a29f, 6);
        g.r[6] = gsu_ldw(gsu_add(REC_FLAGS16, g.r[2], 0));
        g.r[0] = gsu_and(g.r[0], g.r[6]);
        g.r[6] = 0x0002;
        g.r[0] = gsu_and(g.r[0], g.r[6]);
        if (!gsu.z) {
            
            g.r[7] = g.r[1];
            g.r[8] = g.r[2];
            g.r[11] = 0xA2B5;
            
            gsu_code(0x04a2a5, 16); gsu_code(0x04ca99, 1);
            g = FxRecordsApart(g, GSU_COPY(0));
            gsu_moves(g.r[0]);
            if (!gsu.z)
                { 
                gsu_code(0x04a2b5, 5); gsu_code(0x04a2e8, 4); gsu_code(0x04a23b, 1);
                continue; } else { gsu_code(0x04a2b5, 5); }
            gsu_code(0x04a2ba, 1);
        } else { gsu_code(0x04a2a5, 7); gsu_code(0x04a2ba, 1); }
        if (masks_clear(&g, g.r[1], g.r[2], GSU_COPY(0))) {
            
            g.r[6] = g.r[1];
            g.r[8] = g.r[2];
            g.r[11] = 0xA2D1;
            
            gsu_code(0x04a2c5, 12); gsu_code(0x04a345, 1);
            g = FxContactNote(g, GSU_COPY(0));
            gsu_code(0x04a2d1, 1);
        } else { gsu_code(0x04a2c5, 3); gsu_code(0x04a2d1, 1); }
        if (masks_clear(&g, g.r[2], g.r[1], GSU_COPY(1))) {
            
            g.r[6] = g.r[2];
            g.r[8] = g.r[1];
            g.r[11] = 0xA2E8;
            
            gsu_code(0x04a2dc, 12); gsu_code(0x04a345, 1);
            g = FxContactNote(g, GSU_COPY(0));
            gsu_code(0x04a2e8, 1);
        } else { gsu_code(0x04a2dc, 3); gsu_code(0x04a2e8, 1); }
        
        gsu_code(0x04a2e9, 3); gsu_code(0x04a23b, 1);
    }
    
    gsu_code(0x04a244, 4); gsu_code(0x04a2ec, 5);
    g.r[0] = gsu_ldw(gsu_add(REC_CONTACTS, g.r[1], 0));
    gsu_moves(g.r[0]);
    if (!gsu.z) {
        
        g.r[6] = g.r[1];
        g.r[11] = 0xA2FC;
        gsu_code(0x04a2f1, 5);
        g = FxListSweepRegs(g);
    } else { gsu_code(0x04a2f1, 4); gsu_code(0x04a2fc, 1); }
    gsu_code(0x04a2fd, 4);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxRecordTouches(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxRecordTouches(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:A1EC FxAllTouches
 * WHAT IT DOES: FxRecordTouches for every record on the list of all
 * records ($2332), so each pair is tested once.
 * HOW: R1 walks the list through +$04; the return address is kept on the
 * R10 stack. The routine runs under CACHE (the code cache only; nothing
 * the C has to do).
 * Entry: R10 = stack; R11 = return address. Exit: R0 = R1 = 0; the others
 * from the last FxRecordTouches; S, Z from R10; the last RAM address R10.
 */
FxRegs FxAllTouches(FxRegs g)
{
    
    gsu_code(0x04a1ed, 2);
    gsu_push(&g.r[10], g.r[11]);
    
    
    gsu_code(0x04a1ef, 6); gsu_cache(0xa1f5);
    g.r[0] = gsu_ldw(FX_RECORDS);
    gsu_code(0x04a1f5, 1);
    for (;;) {
        g.r[1] = g.r[0];
        gsu_moves(g.r[1]);
        if (gsu.z)
            { 
            gsu_code(0x04a1f6, 4); gsu_code(0x04a206, 1);
            break; } else { gsu_code(0x04a1f6, 4); }
        g.r[11] = 0xA1FF;
        
        gsu_code(0x04a1fa, 5); gsu_code(0x04a20b, 1);
        g = FxRecordTouches(g);
        gsu_code(0x04a1ff, 5);
        g.r[0] = gsu_ldw(gsu_add(REC_ALL_NEXT, g.r[1], 0));
        
        gsu_code(0x04a204, 2); gsu_code(0x04a1f5, 1);
    }
    gsu_code(0x04a207, 4);
    gsu_pull(&g.r[10]);
    return g;
}

void abi_FxAllTouches(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxAllTouches(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
