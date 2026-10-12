/*
 * fx_sums.c: the Super FX's two vector sums for the object it is moving.
 *
 * $70:2340/2342/2344 is a sum in world axes (x, y, z) and $70:2346/2348/234A
 * a sum in the object's own axes. Pushes are added into them during a step;
 * the own-axes sum is turned into world axes through the 3x3 matrix at
 * $70:00E4 (columns of three words at $00E4, $00EA and $00F0, 1.15 fixed
 * point) and added into the world sum; the world sum is then added to the
 * object's Super FX record (R9).
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define FX_WORLD_SUM 0x2340u   /* x, y, z */
#define FX_LOCAL_SUM 0x2346u   /* x, y, z in the object's axes */
#define FX_MATRIX 0x00E4u      /* column j, row i at +6*j+2*i */

/* Record fields the world sum is added to. */
#define REC_1E 0x1Eu           /* +$1E/+$20/+$22: half the sum */
#define REC_2E 0x2Eu           /* +$2E/+$30/+$32: twice, eight times, twice */

/* FMULT then ROL: bits 15-30 of the signed product, a 1.15 fixed point
   multiply. */
static uint16_t fx_mul15(uint16_t a, uint16_t b)
{
    return gsu_rol(gsu_fmult(a, b));
}

/*
 * @routine 04:96A7 FxSumsClear
 * WHAT IT DOES: Clears both vector sums.
 * HOW: SUB R0 from itself, then six SMs of the zero, world x first.
 * Entry: R11 = return address. Exit: R0 = 0; Z, CY set, S, OV clear; the
 * last RAM address $234A.
 */
void FxSumsClear(void)
{
    uint16_t zero = gsu_sub(gsu.r[0], gsu.r[0], 0);
    int i;

    for (i = 0; i < 6; i++)
        gsu_stw((uint16_t)(FX_WORLD_SUM + 2 * i), zero);
}

void abi_FxSumsClear(void)
{
    FxSumsClear();
    gsu.r[0] = 0;
}

/*
 * @routine 04:97BB FxSumsRotateLocal
 * WHAT IT DOES: Adds the own-axes sum, turned into world axes by the matrix
 * at $00E4, into the world sum.
 * HOW: For world x, z, then y (in that order), the three products of the
 * own-axes sum with the matrix row, each an FMULT whose top bit is rolled
 * in, summed and added to the world sum's word with SBK.
 * Entry: R11 = return address. Exit: R0 = the new world y; R2 = the own-axes
 * sum's world y; R1, R3 = own-axes x, z; R4 = the old world y; R5 = the y
 * row's first two products summed; R6 = the matrix word at $00F2; the last
 * RAM address $2342; OV, S, CY, Z from the final add.
 */
typedef struct {
    uint16_t lx, lz;    /* own-axes x and z */
    uint16_t part;      /* the last axis's first two products summed */
    uint16_t old;       /* the last axis's world sum before the add */
    uint16_t m;         /* the last matrix word read */
    uint16_t rot;       /* the own-axes sum's world y */
    uint16_t y;         /* the new world y */
} FxRotateLeft;

FxRotateLeft FxSumsRotateLocal(void)
{
    static const uint8_t axis[3] = {0, 4, 2};   /* world x, z, y */
    FxRotateLeft o;
    uint16_t ly, sum = 0;
    int k;

    o.lx = gsu_ldw(FX_LOCAL_SUM);
    ly = gsu_ldw(FX_LOCAL_SUM + 2);
    o.lz = gsu_ldw(FX_LOCAL_SUM + 4);
    for (k = 0; k < 3; k++) {
        uint16_t row = (uint16_t)(FX_MATRIX + axis[k]);

        o.m = gsu_ldw(row);
        o.part = fx_mul15(o.lx, o.m);
        o.m = gsu_ldw((uint16_t)(row + 6));
        o.part = gsu_add(o.part, fx_mul15(ly, o.m), 0);
        o.m = gsu_ldw((uint16_t)(row + 12));
        sum = gsu_add(fx_mul15(o.lz, o.m), o.part, 0);
        o.rot = sum;
        o.old = gsu_ldw((uint16_t)(FX_WORLD_SUM + axis[k]));
        sum = gsu_add(sum, o.old, 0);
        gsu_sbk(sum);
    }
    o.y = sum;
    return o;
}

void abi_FxSumsRotateLocal(void)
{
    FxRotateLeft o = FxSumsRotateLocal();

    gsu.r[0] = o.y;
    gsu.r[1] = o.lx;
    gsu.r[2] = o.rot;
    gsu.r[3] = o.lz;
    gsu.r[4] = o.old;
    gsu.r[5] = o.part;
    gsu.r[6] = o.m;
}

/*
 * @routine 04:9BD2 FxSumsApply
 * WHAT IT DOES: Adds the world sum to the Super FX record R9: half of it to
 * +$1E/+$20/+$22, and twice x, eight times y and twice z to +$2E/+$30/+$32.
 * Nothing is changed when the sum is zero.
 * HOW: The three words are ORed to test for zero; each field is read with
 * LDW, the scaled sum added (DIV2 for the halves) and stored back with SBK,
 * in record order.
 * Entry: R9 = the record; R11 = return address. Exit: R4, R5, R6 = the world
 * sum. A zero sum: R0 = 0; S clear, Z set; the last RAM address $2344.
 * Otherwise: R0 = the new +$32; R1 = the old +$32; the last RAM address
 * R9 + $32; OV, S, RAM address $2342; OV, S, CY, Z from the final add.
 */
typedef struct {
    uint16_t v[3];      /* the world sum */
    uint16_t last;      /* the new +$32 (0 for a zero sum) */
    uint16_t field;     /* the old +$32 */
    int moved;          /* the sum was not zero */
} FxApplyLeft;

FxApplyLeft FxSumsApply(uint16_t rec)
{
    static const uint8_t doublings[3] = {1, 3, 1};   /* x2, x8, x2 */
    FxApplyLeft o = {{0, 0, 0}, 0, 0, 0};
    uint16_t r;
    int i, k;

    for (i = 0; i < 3; i++)
        o.v[i] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i));
    gsu_sz((uint16_t)(o.v[0] | o.v[1] | o.v[2]));
    if (gsu.z)
        return o;
    o.moved = 1;
    for (i = 0; i < 3; i++) {
        o.field = gsu_ldw(gsu_add((uint16_t)(REC_1E + 2 * i), rec, 0));
        gsu_sbk(gsu_add(gsu_div2(o.v[i]), o.field, 0));
    }
    for (i = 0; i < 3; i++) {
        o.field = gsu_ldw(gsu_add((uint16_t)(REC_2E + 2 * i), rec, 0));
        r = o.v[i];
        for (k = 0; k < doublings[i]; k++)
            r = gsu_add(r, r, 0);
        o.last = gsu_add(r, o.field, 0);
        gsu_sbk(o.last);
    }
    return o;
}

void abi_FxSumsApply(void)
{
    FxApplyLeft o = FxSumsApply(gsu.r[9]);

    gsu.r[4] = o.v[0];
    gsu.r[5] = o.v[1];
    gsu.r[6] = o.v[2];
    gsu.r[0] = o.last;
    if (o.moved)
        gsu.r[1] = o.field;
}

#define PUSH_MAX 0x1388u        /* 5000 */
#define PUSH_MIN 0xEC78u        /* -5000 */

/* A component held to -5000..5000 (R1, R2 the limits): MOVES for the sign,
   then CMP with the limit on that side; the reached limit replaces it
   (the replacing is decoded from the ROM bytes and not reached in the
   traces so far). */
static uint16_t clamp(FxRegs *g, uint16_t v)
{
    gsu_moves(v);
    if (!gsu.s) {
        gsu_sub(v, g->r[2], 0);
        if (!gsu.s)
            v = g->r[2];
    } else {
        gsu_sub(v, g->r[1], 0);
        if (gsu.s)
            v = g->r[1];
    }
    return v;
}

/* Both push routines: (R4, R5, R6) clamped, then added to the sum at
   `sum`; R1 and R2 kept on the R10 stack. */
static FxRegs push_into(FxRegs g, uint16_t sum)
{
    int i;

    gsu_push(&g.r[10], g.r[1]);
    gsu_push(&g.r[10], g.r[2]);
    g.r[1] = PUSH_MIN;
    g.r[2] = PUSH_MAX;
    for (i = 4; i <= 6; i++)
        g.r[i] = clamp(&g, g.r[i]);
    g.r[2] = gsu_pull(&g.r[10]);
    g.r[1] = gsu_pull(&g.r[10]);
    for (i = 0; i < 3; i++) {
        g.r[0] = gsu_add(gsu_ldw((uint16_t)(sum + 2 * i)), g.r[4 + i], 0);
        gsu_stw((uint16_t)(sum + 2 * i), g.r[0]);
    }
    return g;
}

/*
 * @routine 04:96C3 FxSumsPushWorld
 * WHAT IT DOES: Adds the push (R4, R5, R6), each held to -5000..5000, to
 * the world sum.
 * HOW: The clamp (clamp) per component with R1 = -5000 and R2 = 5000,
 * then LM, ADD, SM per word. R1 and R2 are kept on the R10 stack.
 * Entry: R4, R5, R6 = the push; R10 = stack; R11 = return address. Exit:
 * R4-R6 clamped; R0 = the new world z; R1, R2 as on entry; OV, S, CY, Z
 * from the last ADD; the last RAM address $2344.
 */
FxRegs FxSumsPushWorld(FxRegs g)
{
    return push_into(g, FX_WORLD_SUM);
}

void abi_FxSumsPushWorld(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSumsPushWorld(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:973F FxSumsPushLocal
 * WHAT IT DOES: As FxSumsPushWorld, into the own-axes sum.
 * HOW: As FxSumsPushWorld.
 * Entry: as FxSumsPushWorld. Exit: as FxSumsPushWorld; R0 = the new
 * own-axes z; the last RAM address $234A.
 */
FxRegs FxSumsPushLocal(FxRegs g)
{
    return push_into(g, FX_LOCAL_SUM);
}

void abi_FxSumsPushLocal(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSumsPushLocal(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

#define FX_PUSH_FLAGS 0x2480u    /* the pushes' flag bytes so far */
#define FX_PUSH_AMOUNT 0x2378u   /* the last push's amount, doubled */
#define FX_PUSH_TURN 0x247Eu     /* the turn applied to pushes: 0-255 */
#define FX_PUSH_DIR 0x236Cu      /* the last push's direction, turned */
#define PUSH_SKIP 0x7D00u

/*
 * @routine 04:944C FxPushApply
 * WHAT IT DOES: Applies the push whose five words are at R3 (flags, amount,
 * then a direction x, y, z) and steps R3 past them: a push with no flags,
 * flags above the low byte, a negative amount or an amount of $7D00 is
 * skipped (the last two decoded from the ROM bytes and not reached in the
 * traces so far). Otherwise its flag byte is ORed into $2480, twice the
 * amount goes to $2378, the direction is turned about the vertical by the
 * angle at $247E (a quarter, half or three quarters turn by swapping and
 * negating; any other angle by FxTurn, decoded from the ROM bytes and not
 * reached in the traces so far) and kept at $236C/$236E/$2370, and twice
 * the amount times it (FMULT) is added to the world sum (FxSumsPushWorld).
 * HOW: R0 = 1 for an applied push. The registers the general turn needs
 * and the return address are kept on the R10 stack.
 * Entry: R3 = the push; R10 = stack; R11 = return address. Exit: R3 = the
 * push + 10; R0 = 1 (applied), 8 or 6 (skipped); applied: R4-R6 as
 * FxSumsPushWorld leaves them, R11 = $955E; S, Z from R10; the last RAM
 * address R10 (applied) or the push's last word read.
 */
FxRegs FxPushApply(FxRegs g)
{
    uint16_t *sp = &g.r[10];
    int i, skip;

    g.r[0] = gsu_ldw(g.r[3]);
    g.r[3] = gsu_inc(gsu_inc(g.r[3]));
    gsu_moves(g.r[0]);
    if (gsu.z) {
        g.r[0] = 0x0008;
        g.r[3] = gsu_add(g.r[3], g.r[0], 0);
        return g;
    }
    g.r[7] = gsu_hib(g.r[0]);
    if (!gsu.z) {
        g.r[0] = 0x0008;
        g.r[3] = gsu_add(g.r[3], g.r[0], 0);
        return g;
    }
    gsu_push(sp, g.r[11]);
    g.r[1] = g.r[0];
    g.r[0] = gsu_ldw(g.r[3]);
    g.r[3] = gsu_inc(gsu_inc(g.r[3]));
    g.r[7] = g.r[0];
    gsu_moves(g.r[7]);
    skip = gsu.s;
    if (!skip) {
        g.r[0] = PUSH_SKIP;
        gsu_sub(g.r[0], g.r[7], 0);
        skip = gsu.z;
    }
    if (skip) {
        g.r[0] = 0x0006;
        g.r[3] = gsu_add(g.r[3], g.r[0], 0);
        gsu_pull(sp);
        return g;
    }
    g.r[0] = gsu_or(gsu_ldw(FX_PUSH_FLAGS), g.r[1]);
    gsu_sbk(g.r[0]);
    g.r[7] = gsu_add(g.r[7], g.r[7], 0);
    gsu_stw(FX_PUSH_AMOUNT, g.r[7]);
    for (i = 4; i <= 6; i++) {
        g.r[i] = gsu_ldw(g.r[3]);
        g.r[3] = gsu_inc(gsu_inc(g.r[3]));
    }
    g.r[0] = gsu_ldw(FX_PUSH_TURN);
    gsu_moves(g.r[0]);
    if (!gsu.z) {
        g.r[1] = 0x0040;
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.z) {                                /* a quarter turn */
            g.r[0] = gsu_inc(gsu_not(g.r[6]));
            g.r[6] = g.r[4];
            g.r[4] = g.r[0];
        } else if (g.r[1] = 0x00C0, gsu_sub(g.r[0], g.r[1], 0), gsu.z) {
            g.r[0] = gsu_inc(gsu_not(g.r[4]));
            g.r[4] = g.r[6];
            g.r[6] = g.r[0];
        } else if (g.r[1] = 0x0080, gsu_sub(g.r[0], g.r[1], 0), gsu.z) {
            g.r[4] = gsu_inc(gsu_not(g.r[4]));
            g.r[6] = gsu_inc(gsu_not(g.r[6]));
        } else {                                    /* any other angle */
            static const uint8_t keep[6] = {2, 3, 5, 7, 8, 9};

            for (i = 0; i < 6; i++)
                gsu_push(sp, g.r[keep[i]]);
            gsu_push(sp, g.r[11]);
            g.r[1] = g.r[4];
            g.r[3] = g.r[6];
            g.r[2] = g.r[0];
            g.r[0] = gsu_or(g.r[1], g.r[3]);
            if (!gsu.z) {
                g.r[11] = 0x9509;
                g = FxTurnRegs(g);
                for (i = 0; i < 2; i++) {
                    g.r[1] = gsu_add(g.r[1], g.r[1], 0);
                    g.r[1] = gsu_add(g.r[1], g.r[1], 0);
                }
                g.r[3] = gsu_add(g.r[3], g.r[3], 0);
                g.r[3] = gsu_add(g.r[3], g.r[3], 0);
            }
            g.r[11] = gsu_pull(sp);
            g.r[4] = g.r[3];
            g.r[6] = g.r[1];
            for (i = 5; i >= 0; i--)
                g.r[keep[i]] = gsu_pull(sp);
        }
    }
    for (i = 0; i < 3; i++)
        gsu_stw((uint16_t)(FX_PUSH_DIR + 2 * i), g.r[4 + i]);
    g.r[1] = g.r[7];
    g.r[6] = gsu_ldw(FX_PUSH_DIR);
    g.r[4] = g.r[0] = gsu_fmult(g.r[1], g.r[6]);
    g.r[5] = g.r[7];
    g.r[6] = gsu_ldw(FX_PUSH_DIR + 2);
    g.r[5] = g.r[0] = gsu_fmult(g.r[5], g.r[6]);
    g.r[6] = gsu_ldw(FX_PUSH_DIR + 4);
    g.r[6] = g.r[0] = gsu_fmult(g.r[7], g.r[6]);
    g.r[11] = 0x955E;
    g = FxSumsPushWorld(g);
    g.r[0] = 1;
    gsu_pull(sp);
    return g;
}

void abi_FxPushApply(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxPushApply(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
