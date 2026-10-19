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
/* gsuwalk: the code of each copy of FxSumsClear, read from the ROM */
static const GsuCode FxSumsClear_fetch[1][6] = {
    {{0x0496a8, 5}, {0x0496ad, 4}, {0x0496b1, 4}, {0x0496b5, 4}, {0x0496b9, 4}, {0x0496bd, 4}},
};

void FxSumsClear(int copy)
{
    uint16_t zero = gsu_sub(gsu.r[0], gsu.r[0], 0);
    int i;

    for (i = 0; i < 6; i++)
        { gsu_codes(FxSumsClear_fetch[0] + i, 1); gsu_stw((uint16_t)(FX_WORLD_SUM + 2 * i), zero); }
    gsu_code(0x0496c1, 2);
}

void abi_FxSumsClear(void)
{
    FxSumsClear(GSU_COPY(0));
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

/* One world axis of FxSumsRotateLocal (the original has each in its own
   code): the local sum's terms by the matrix's column (LMS R6; FROM Rn;
   FMULT; ROL; added up as FxMatrixApply's), then the world sum's (LM R4;
   ADD R4) stored back (SBK). */
/* gsuwalk: the code of each copy of rotate_axis, read from the ROM */
static const GsuCode rotate_axis_fetch[3][5] = {
    {{0x0497c8, 3}, {0x0497cb, 7}, {0x0497d2, 8}, {0x0497da, 8}, {0x0497e2, 2}},
    {{0x0497e4, 3}, {0x0497e7, 7}, {0x0497ee, 8}, {0x0497f6, 8}, {0x0497fe, 2}},
    {{0x049800, 3}, {0x049803, 7}, {0x04980a, 8}, {0x049812, 10}, {0x04981c, 2}},
};

static uint16_t rotate_axis(FxRotateLeft *o, uint16_t ly, int k, int copy)
{
    static const uint8_t axis[3] = {0, 4, 2};   /* world x, z, y */
    uint16_t row = (uint16_t)(FX_MATRIX + axis[k]), sum;

    gsu_codes(rotate_axis_fetch[copy], 1);
    o->m = gsu_ldw(row);
    o->part = fx_mul15(o->lx, o->m);
    gsu_codes(rotate_axis_fetch[copy] + 1, 1);
    o->m = gsu_ldw((uint16_t)(row + 6));
    o->part = gsu_add(o->part, fx_mul15(ly, o->m), 0);
    gsu_codes(rotate_axis_fetch[copy] + 2, 1);
    o->m = gsu_ldw((uint16_t)(row + 12));
    sum = gsu_add(fx_mul15(o->lz, o->m), o->part, 0);
    o->rot = sum;
    gsu_codes(rotate_axis_fetch[copy] + 3, 1);
    o->old = gsu_ldw((uint16_t)(FX_WORLD_SUM + axis[k]));
    sum = gsu_add(sum, o->old, 0);
    gsu_codes(rotate_axis_fetch[copy] + 4, 1);
    gsu_sbk(sum);
    return sum;
}

FxRotateLeft FxSumsRotateLocal(int copy)
{
    FxRotateLeft o;
    uint16_t ly;

    gsu_code(0x0497bc, 4);
    o.lx = gsu_ldw(FX_LOCAL_SUM);
    gsu_code(0x0497c0, 4);
    ly = gsu_ldw(FX_LOCAL_SUM + 2);
    gsu_code(0x0497c4, 4);
    o.lz = gsu_ldw(FX_LOCAL_SUM + 4);
    rotate_axis(&o, ly, 0, GSU_COPY(0));
    rotate_axis(&o, ly, 1, GSU_COPY(1));
    o.y = rotate_axis(&o, ly, 2, GSU_COPY(2));
    gsu_code(0x04981e, 2);
    return o;
}

void abi_FxSumsRotateLocal(void)
{
    FxRotateLeft o = FxSumsRotateLocal(GSU_COPY(0));

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

/* gsuwalk: the code of each copy of FxSumsApply, read from the ROM */
static const GsuCode FxSumsApply_fetch[1][18] = {
    {{0x049bd3, 4}, {0x049bd7, 4}, {0x049bdb, 4}, {0x049be5, 7}, {0x049bf1, 6}, {0x049bfc, 6}, {0x049bec, 5}, {0x049bf7, 5}, {0x049c02, 5}, {0x049c07, 6}, {0x049c12, 6}, {0x049c1f, 6}, {0x049c0d, 4}, {0x049c18, 6}, {0x049c25, 4}, {0x049c11, 1}, {0x049c1e, 1}, {0x049c29, 1}},
};

FxApplyLeft FxSumsApply(uint16_t rec, int copy)
{
    static const uint8_t doublings[3] = {1, 3, 1};   /* x2, x8, x2 */
    FxApplyLeft o = {{0, 0, 0}, 0, 0, 0};
    uint16_t r;
    int i, k;

    for (i = 0; i < 3; i++)
        { gsu_codes(FxSumsApply_fetch[0] + i, 1); o.v[i] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i)); }
    gsu_sz((uint16_t)(o.v[0] | o.v[1] | o.v[2]));
    if (gsu.z)
        { gsu_code(0x049bdf, 6); gsu_code(0x049c29, 3); return o; } else { gsu_code(0x049bdf, 6); }
    o.moved = 1;
    for (i = 0; i < 3; i++) {
        gsu_codes(FxSumsApply_fetch[0] + 3 + i, 1);
        o.field = gsu_ldw(gsu_add((uint16_t)(REC_1E + 2 * i), rec, 0));
        gsu_codes(FxSumsApply_fetch[0] + 6 + i, 1);
        gsu_sbk(gsu_add(gsu_div2(o.v[i]), o.field, 0));
    }
    for (i = 0; i < 3; i++) {
        gsu_codes(FxSumsApply_fetch[0] + 9 + i, 1);
        o.field = gsu_ldw(gsu_add((uint16_t)(REC_2E + 2 * i), rec, 0));
        r = o.v[i];
        gsu_codes(FxSumsApply_fetch[0] + 12 + i, 1);
        for (k = 0; k < doublings[i]; k++)
            r = gsu_add(r, r, 0);
        o.last = gsu_add(r, o.field, 0);
        gsu_codes(FxSumsApply_fetch[0] + 15 + i, 1);
        gsu_sbk(o.last);
    }
    gsu_code(0x049c2a, 2);
    return o;
}

void abi_FxSumsApply(void)
{
    FxApplyLeft o = FxSumsApply(gsu.r[9], GSU_COPY(0));

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
/* gsuwalk: the code of each copy of clamp, read from the ROM */
static const GsuCode clamp_fetch[6][9] = {
    {{0x0496ca, 18}, {0x0496e9, 1}, {0x0496ca, 23}, {0x0496e9, 1}, {0x0496ca, 12}, {0x0496e1, 6}, {0x0496e9, 1}, {0x0496ca, 12}, {0x0496e1, 9}},
    {{0x0496ea, 10}, {0x049701, 1}, {0x0496ea, 15}, {0x049701, 1}, {0x0496ea, 4}, {0x0496f9, 6}, {0x049701, 1}, {0x0496ea, 4}, {0x0496f9, 9}},
    {{0x049702, 10}, {0x049719, 1}, {0x049702, 15}, {0x049719, 1}, {0x049702, 4}, {0x049711, 6}, {0x049719, 1}, {0x049702, 4}, {0x049711, 9}},
    {{0x049746, 18}, {0x049765, 1}, {0x049746, 23}, {0x049765, 1}, {0x049746, 12}, {0x04975d, 6}, {0x049765, 1}, {0x049746, 12}, {0x04975d, 9}},
    {{0x049766, 10}, {0x04977d, 1}, {0x049766, 15}, {0x04977d, 1}, {0x049766, 4}, {0x049775, 6}, {0x04977d, 1}, {0x049766, 4}, {0x049775, 9}},
    {{0x04977e, 10}, {0x049795, 1}, {0x04977e, 15}, {0x049795, 1}, {0x04977e, 4}, {0x04978d, 6}, {0x049795, 1}, {0x04977e, 4}, {0x04978d, 9}},
};

static uint16_t clamp(FxRegs *g, uint16_t v, int copy)
{
    gsu_moves(v);
    if (!gsu.s) {
        gsu_sub(v, g->r[2], 0);
        if (!gsu.s)
            { v = g->r[2]; gsu_codes(clamp_fetch[copy] + 2, 2); } else { gsu_codes(clamp_fetch[copy], 2); }
    } else {
        gsu_sub(v, g->r[1], 0);
        if (gsu.s)
            { v = g->r[1]; gsu_codes(clamp_fetch[copy] + 7, 2); } else { gsu_codes(clamp_fetch[copy] + 4, 3); }
    }
    return v;
}

/* Both push routines: (R4, R5, R6) clamped, then added to the sum at
   `sum`; R1 and R2 kept on the R10 stack. */
/* gsuwalk: the code of each copy of push_into, read from the ROM */
static const GsuCode push_into_fetch[2][10] = {
    {{0x0496c4, 2}, {0x0496c6, 4}, {0x04971a, 4}, {0x04971e, 4}, {0x049722, 4}, {0x04972b, 4}, {0x049734, 4}, {0x049726, 5}, {0x04972f, 5}, {0x049738, 5}},
    {{0x049740, 2}, {0x049742, 4}, {0x049796, 4}, {0x04979a, 4}, {0x04979e, 4}, {0x0497a7, 4}, {0x0497b0, 4}, {0x0497a2, 5}, {0x0497ab, 5}, {0x0497b4, 5}},
};

/* gsuwalk: the copy of clamp each copy of push_into calls, by pass */
static const uint8_t clamp_in_push_into[2][3] = {{0, 1, 2}, {3, 4, 5}};

static FxRegs push_into(FxRegs g, uint16_t sum, int copy)
{
    int i;

    gsu_codes(push_into_fetch[copy], 1);
    gsu_push(&g.r[10], g.r[1]);
    gsu_codes(push_into_fetch[copy] + 1, 1);
    gsu_push(&g.r[10], g.r[2]);
    g.r[1] = PUSH_MIN;
    g.r[2] = PUSH_MAX;
    for (i = 4; i <= 6; i++)
        g.r[i] = clamp(&g, g.r[i], GSU_COPY(clamp_in_push_into[copy][(i - 4)]));
    gsu_codes(push_into_fetch[copy] + 2, 1);
    g.r[2] = gsu_pull(&g.r[10]);
    gsu_codes(push_into_fetch[copy] + 3, 1);
    g.r[1] = gsu_pull(&g.r[10]);
    for (i = 0; i < 3; i++) {
        gsu_codes(push_into_fetch[copy] + 4 + i, 1);
        g.r[0] = gsu_add(gsu_ldw((uint16_t)(sum + 2 * i)), g.r[4 + i], 0);
        gsu_codes(push_into_fetch[copy] + 7 + i, 1);
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
FxRegs FxSumsPushWorld(FxRegs g, int copy)
{
    { FxRegs ret_ = push_into(g, FX_WORLD_SUM, GSU_COPY(0)); gsu_code(0x04973d, 2); return ret_; }
}

void abi_FxSumsPushWorld(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSumsPushWorld(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:973F FxSumsPushLocal
 * WHAT IT DOES: As FxSumsPushWorld, into the own-axes sum.
 * HOW: As FxSumsPushWorld.
 * Entry: as FxSumsPushWorld. Exit: as FxSumsPushWorld; R0 = the new
 * own-axes z; the last RAM address $234A.
 */
FxRegs FxSumsPushLocal(FxRegs g, int copy)
{
    { FxRegs ret_ = push_into(g, FX_LOCAL_SUM, GSU_COPY(1)); gsu_code(0x0497b9, 2); return ret_; }
}

void abi_FxSumsPushLocal(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxSumsPushLocal(g, GSU_COPY(0));
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
/* gsuwalk: the code of each copy of FxPushApply, read from the ROM */
static const GsuCode FxPushApply_fetch[1][42] = {
    {{0x04948c, 2}, {0x04948e, 4}, {0x049492, 4}, {0x04949c, 4}, {0x0494a4, 8}, {0x0494b8, 8}, {0x0494cb, 8}, {0x0494dc, 3}, {0x0494df, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x0494e3, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x0494e7, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x0494eb, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x0494ef, 4}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x049516, 8}, {0x04951e, 4}, {0x049522, 4}, {0x049526, 4}, {0x04952a, 4}, {0x04952e, 4}, {0x049532, 4}, {0x049536, 4}, {0x04953a, 4}},
};

FxRegs FxPushApply(FxRegs g, int copy)
{
    uint16_t *sp = &g.r[10];
    int i;

    gsu_code(0x04944d, 1);
    g.r[0] = gsu_ldw(g.r[3]);
    g.r[3] = gsu_inc(gsu_inc(g.r[3]));
    gsu_moves(g.r[0]);
    if (gsu.z) {
        g.r[0] = 0x0008;
        g.r[3] = gsu_add(g.r[3], g.r[0], 0);
        gsu_code(0x04944e, 10); gsu_code(0x049575, 7);
        return g;
    } else { gsu_code(0x04944e, 6); gsu_code(0x049458, 1); }
    g.r[7] = gsu_hib(g.r[0]);
    if (!gsu.z) {
        g.r[0] = 0x0008;
        g.r[3] = gsu_add(g.r[3], g.r[0], 0);
        gsu_code(0x049459, 8); gsu_code(0x049565, 7);
        return g;
    } else { gsu_code(0x049459, 4); gsu_code(0x049461, 1); }
    gsu_code(0x049462, 2);
    gsu_push(sp, g.r[11]);
    g.r[1] = g.r[0];
    gsu_code(0x049464, 5);
    g.r[0] = gsu_ldw(g.r[3]);
    g.r[3] = gsu_inc(gsu_inc(g.r[3]));
    g.r[7] = g.r[0];
    gsu_moves(g.r[7]);
    if (gsu.s)
        { gsu_code(0x049469, 10); gsu_code(0x04956c, 1); goto skipped; } else { gsu_code(0x049469, 6); gsu_code(0x049473, 1); }                          /* a negative amount */
    g.r[0] = PUSH_SKIP;
    gsu_sub(g.r[0], g.r[7], 0);
    if (gsu.z)
        { gsu_code(0x049474, 11); gsu_code(0x04956c, 1); goto skipped; } else { gsu_code(0x049474, 7); gsu_code(0x04947f, 1); }                          /* an amount of $7D00 */
    gsu_code(0x049480, 4);
    g.r[0] = gsu_or(gsu_ldw(FX_PUSH_FLAGS), g.r[1]);
    gsu_code(0x049484, 2);
    gsu_sbk(g.r[0]);
    g.r[7] = gsu_add(g.r[7], g.r[7], 0);
    gsu_code(0x049486, 6);
    gsu_stw(FX_PUSH_AMOUNT, g.r[7]);
    for (i = 4; i <= 6; i++) {
        gsu_codes(FxPushApply_fetch[0] + (i - 4), 1);
        g.r[i] = gsu_ldw(g.r[3]);
        g.r[3] = gsu_inc(gsu_inc(g.r[3]));
    }
    gsu_code(0x049496, 6);
    g.r[0] = gsu_ldw(FX_PUSH_TURN);
    gsu_moves(g.r[0]);
    if (!gsu.z) {
        g.r[1] = 0x0040;
        gsu_sub(g.r[0], g.r[1], 0);
        if (gsu.z) {                                /* a quarter turn */
            g.r[0] = gsu_inc(gsu_not(g.r[6]));
            g.r[6] = g.r[4];
            g.r[4] = g.r[0];
            gsu_code(0x04949c, 4); gsu_code(0x0494a4, 20); gsu_code(0x049531, 1);
        } else if (g.r[1] = 0x00C0, gsu_sub(g.r[0], g.r[1], 0), gsu.z) {
            g.r[0] = gsu_inc(gsu_not(g.r[4]));
            g.r[4] = g.r[6];
            g.r[6] = g.r[0];
            gsu_code(0x04949c, 4); gsu_code(0x0494a4, 8); gsu_code(0x0494b8, 19); gsu_code(0x049531, 1);
        } else if (g.r[1] = 0x0080, gsu_sub(g.r[0], g.r[1], 0), gsu.z) {
            g.r[4] = gsu_inc(gsu_not(g.r[4]));
            g.r[6] = gsu_inc(gsu_not(g.r[6]));
            gsu_code(0x04949c, 4); gsu_code(0x0494a4, 8); gsu_code(0x0494b8, 8); gsu_code(0x0494cb, 17); gsu_code(0x049531, 1);
        } else {                                    /* any other angle */
            static const uint8_t keep[6] = {2, 3, 5, 7, 8, 9};

            for (i = 0; i < 6; i++)
                { gsu_codes(FxPushApply_fetch[0] + 3 + 5 * i, 5); gsu_push(sp, g.r[keep[i]]); }
            gsu_code(0x0494f3, 4);
            gsu_push(sp, g.r[11]);
            g.r[1] = g.r[4];
            g.r[3] = g.r[6];
            g.r[2] = g.r[0];
            g.r[0] = gsu_or(g.r[1], g.r[3]);
            if (!gsu.z) {
                g.r[11] = 0x9509;
                gsu_code(0x0494f7, 14);
                g = FxTurnRegs(g, GSU_COPY(0));
                g.r[1] = gsu_add(g.r[1], g.r[1], 0);
                g.r[1] = gsu_add(g.r[1], g.r[1], 0);
                g.r[3] = gsu_add(g.r[3], g.r[3], 0);
                g.r[3] = gsu_add(g.r[3], g.r[3], 0);
                gsu_code(0x049509, 9);
            } else { gsu_code(0x0494f7, 13); gsu_code(0x049511, 1); }
            gsu_code(0x049512, 4);
            g.r[11] = gsu_pull(sp);
            g.r[4] = g.r[3];
            g.r[6] = g.r[1];
            for (i = 5; i >= 0; i--)
                { gsu_codes(FxPushApply_fetch[0] + 33 + (5 - i), 1); g.r[keep[i]] = gsu_pull(sp); }
        }
    } else { gsu_code(0x04949c, 8); gsu_code(0x049531, 1); }
    for (i = 0; i < 3; i++)
        { gsu_codes(FxPushApply_fetch[0] + 39 + i, 1); gsu_stw((uint16_t)(FX_PUSH_DIR + 2 * i), g.r[4 + i]); }
    g.r[1] = g.r[7];
    gsu_code(0x04953e, 6);
    g.r[6] = gsu_ldw(FX_PUSH_DIR);
    g.r[4] = g.r[0] = gsu_fmult(g.r[1], g.r[6]);
    g.r[5] = g.r[7];
    gsu_code(0x049544, 10);
    g.r[6] = gsu_ldw(FX_PUSH_DIR + 2);
    g.r[5] = g.r[0] = gsu_fmult(g.r[5], g.r[6]);
    gsu_code(0x04954e, 8);
    g.r[6] = gsu_ldw(FX_PUSH_DIR + 4);
    g.r[6] = g.r[0] = gsu_fmult(g.r[7], g.r[6]);
    g.r[11] = 0x955E;
    gsu_code(0x049556, 8); gsu_code(0x0496c3, 1);
    g = FxSumsPushWorld(g, GSU_COPY(0));
    g.r[0] = 1;
    gsu_code(0x04955e, 7);
    gsu_pull(sp);
    return g;
skipped:
    g.r[0] = 0x0006;
    g.r[3] = gsu_add(g.r[3], g.r[0], 0);
    gsu_code(0x04956d, 8);
    gsu_pull(sp);
    return g;
}

void abi_FxPushApply(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxPushApply(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}

/*
 * @routine 04:941C FxPushesApply
 * WHAT IT DOES: Applies the 19 pushes at $23AE (FxPushApply each) with the
 * push flags at $2480 cleared first, and returns the flags they set.
 * HOW: The return address is kept on the R10 stack; the ROM bank is set to
 * 0; R13 counts the pushes from 1, stopping at $14.
 * Entry: R10 = stack; R11 = return address. Exit: R0 = the flags at
 * $2480; R3 past the last push; R13 = $14; R11 = $943F; CY, OV from the
 * last count compare; S, Z from R10; the last RAM address R10.
 */
void abi_FxPushesApply(void)
{
    gsu_code(0x04941d, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = 0x0000;
    gsu_code(0x04941f, 6);
    gsu_romb(0);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_code(0x049425, 5);
    gsu_stw(FX_PUSH_FLAGS, gsu.r[0]);
    gsu.r[3] = 0x23AE;
    gsu.r[13] = 0x0000;
    gsu_code(0x04942a, 5);
    for (;;) {
        gsu.r[13] = gsu_inc(gsu.r[13]);
        gsu.r[0] = 0x0014;
        gsu_sub(gsu.r[0], gsu.r[13], 0);
        if (gsu.z)
            { gsu_code(0x04942f, 7); break; } else { gsu_code(0x04942f, 7); gsu_code(0x04943a, 1); }
        gsu.r[11] = 0x943F;
        gsu_code(0x04943b, 4); gsu_code(0x04944c, 1);
        abi_FxPushApply();
        gsu_code(0x04943f, 4); gsu_code(0x04942e, 1);
    }
    gsu_code(0x049436, 4); gsu_code(0x049443, 5);
    gsu.r[0] = gsu_ldw(FX_PUSH_FLAGS);
    gsu_code(0x049448, 4);
    gsu_pull(&gsu.r[10]);
}

/* ---- pushes at a point: 04:987D-9BC5 ---- */

#define FX_HIT 0x2334u          /* the hit point: x, y, z */
#define FX_PUSH_POINT 0x26F6u   /* the point pushed at, sixteen times: x, y, z */
#define FX_PUSH_ROT 0x0132u     /* the push in the object's axes: x at $132, y at $13A, z at $142 */
#define FX_PUSH_ROT_GAP 0x0134u /* cleared */
#define FX_TURN_CROSS 0x0068u   /* the point crossed with the push: x at $68, y at $2E, z at $2C */
#define FX_SPIN 0x233Au         /* the spin sums: z at $233A, x at $233C, y at $233E */

/* The three pushes of the registers the variants keep: R11, R9, R8 on the
   R10 stack, and back in the other order. */
/* gsuwalk: the code of each copy of push3, read from the ROM */
static const GsuCode push3_fetch[8][3] = {
    {{0x04987e, 2}, {0x049880, 4}, {0x049884, 4}},
    {{0x0498ac, 2}, {0x0498ae, 4}, {0x0498b2, 4}},
    {{0x049a9c, 2}, {0x049a9e, 4}, {0x049aa2, 4}},
    {{0x049986, 2}, {0x049988, 4}, {0x04998c, 4}},
    {{0x0499a2, 2}, {0x0499a4, 4}, {0x0499a8, 4}},
    {{0x0499ca, 2}, {0x0499cc, 4}, {0x0499d0, 4}},
    {{0x0499ef, 2}, {0x0499f1, 4}, {0x0499f5, 4}},
    {{0x049a1a, 2}, {0x049a1c, 4}, {0x049a20, 4}},
};

static void push3(int copy)
{
    gsu_codes(push3_fetch[copy], 1);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_codes(push3_fetch[copy] + 1, 1);
    gsu_push(&gsu.r[10], gsu.r[9]);
    gsu_codes(push3_fetch[copy] + 2, 1);
    gsu_push(&gsu.r[10], gsu.r[8]);
}

/* gsuwalk: the code of each copy of pull3, read from the ROM */
static const GsuCode pull3_fetch[3][4] = {
    {{0x04991b, 4}, {0x04991f, 4}, {0, 0}, {0x049923, 4}},
    {{0x049bba, 4}, {0x049bbe, 4}, {0, 0}, {0x049bc2, 4}},
    {{0x049990, 6}, {0x049996, 4}, {0x04999a, 4}, {0, 0}},
};

static void pull3(int r11, int copy)
{
    gsu_codes(pull3_fetch[copy], 1);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu_codes(pull3_fetch[copy] + 1, 1);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    if (r11)
        { gsu_codes(pull3_fetch[copy] + 2, 1); gsu.r[11] = gsu_pull(&gsu.r[10]); }
    else
        { gsu_codes(pull3_fetch[copy] + 3, 1); gsu_pull(&gsu.r[10]); }   /* the return address, into R15 */
}

/* One component through the matrix: FMULT and ROL of each of R1, R2, R3
   with the words at m, m + step, m + 2 * step, summed (R5 the running
   sum, R0 the result). */
/* gsuwalk: the code of each copy of fx_row, read from the ROM */
static const GsuCode fx_row_fetch[6][3] = {
    {{0x049928, 3}, {0x04992b, 7}, {0x049932, 8}},
    {{0x049942, 3}, {0x049945, 7}, {0x04994c, 8}},
    {{0x04995c, 3}, {0x04995f, 7}, {0x049966, 8}},
    {{0x049af4, 3}, {0x049af7, 7}, {0x049afe, 8}},
    {{0x049b0d, 3}, {0x049b10, 7}, {0x049b17, 8}},
    {{0x049b26, 3}, {0x049b29, 7}, {0x049b30, 8}},
};

static uint16_t fx_row(uint16_t m, uint16_t step, int copy)
{
    gsu_codes(fx_row_fetch[copy], 1);
    gsu.r[6] = gsu_ldw(m);
    gsu.r[5] = gsu_rol(gsu_fmult(gsu.r[1], gsu.r[6]));
    gsu_codes(fx_row_fetch[copy] + 1, 1);
    gsu.r[6] = gsu_ldw((uint16_t)(m + step));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[2], gsu.r[6]));
    gsu.r[5] = gsu_add(gsu.r[5], gsu.r[0], 0);
    gsu_codes(fx_row_fetch[copy] + 2, 1);
    gsu.r[6] = gsu_ldw((uint16_t)(m + 2 * step));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[3], gsu.r[6]));
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
    return gsu.r[0];
}

/* One component of the cross product: a * R6(at p) - b * R6(at q) as 32
   bits (LMULT, SUB, SBC), doubled, its high word (ADD, ROL). */
/* gsuwalk: the code of each copy of fx_cross, read from the ROM */
static const GsuCode fx_cross_fetch[3][2] = {
    {{0x049b4b, 3}, {0x049b4e, 8}},
    {{0x049b64, 3}, {0x049b67, 8}},
    {{0x049b7d, 3}, {0x049b80, 8}},
};

static uint16_t fx_cross(uint16_t a, uint16_t p, uint16_t b, uint16_t q, int copy)
{
    uint32_t m;

    gsu_codes(fx_cross_fetch[copy], 1);
    gsu.r[6] = gsu_ldw(p);
    m = gsu_lmult(a, gsu.r[6]);
    gsu.r[0] = (uint16_t)(m >> 16);
    gsu.r[4] = (uint16_t)m;
    gsu.r[7] = gsu.r[4];
    gsu_codes(fx_cross_fetch[copy] + 1, 1);
    gsu.r[6] = gsu_ldw(q);
    m = gsu_lmult(b, gsu.r[6]);
    gsu.r[5] = (uint16_t)(m >> 16);
    gsu.r[4] = (uint16_t)m;
    gsu.r[7] = gsu_sub(gsu.r[7], gsu.r[4], 0);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[5], !gsu.cy);
    gsu.r[7] = gsu_add(gsu.r[7], gsu.r[7], 0);
    gsu.r[0] = gsu_rol(gsu.r[0]);
    return gsu.r[0];
}

/* LM, three ADDs or two, ADD of the sum's word, SBK. */
/* gsuwalk: the code of each copy of spin_add, read from the ROM */
static const GsuCode spin_add_fetch[4][3] = {
    {{0x049b96, 4}, {0x049b9a, 4}, {0x049b9e, 4}},
    {{0x049ba2, 4}, {0x049ba6, 4}, {0x049baa, 4}},
    {{0x049bae, 4}, {0x049bb2, 4}, {0x049bb6, 4}},
    {{0x049a54, 4}, {0x049a58, 4}, {0x049a5c, 5}},
};

static void spin_add(uint16_t from, uint16_t sum, int reg, int times4, int copy)
{
    gsu_codes(spin_add_fetch[copy], 1);
    gsu.r[0] = gsu_ldw(from);
    gsu_codes(spin_add_fetch[copy] + 1, 1);
    gsu.r[reg] = gsu_ldw(sum);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    if (!times4)
        { gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0); }
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[reg], 0);
    gsu_codes(spin_add_fetch[copy] + 2, 1);
    gsu_sbk(gsu.r[0]);
}

/* 04:9AC2, the variants' shared part: the point (R1, R2, R3) times 16 to
   $26F6; the push at $0132/$013A/$0142 turned into the object's axes by
   the matrix at $00E4; the point crossed with it into $68/$2C/$2E and
   four times each added to the spin sums; R8, R9 pulled and return. */
/* gsuwalk: the code of each copy of push_at_tail, read from the ROM */
static const GsuCode push_at_tail_fetch[5][18] = {
    {{0x049ac3, 25}, {0, 0}, {0x049adc, 3}, {0x049adf, 4}, {0x049ae3, 4}, {0x049ae7, 4}, {0x049aeb, 3}, {0x049aee, 3}, {0x049af1, 3}, {0x049b06, 7}, {0x049b1f, 7}, {0x049b38, 7}, {0x049b3f, 4}, {0x049b43, 4}, {0x049b47, 4}, {0x049b56, 14}, {0x049b6f, 14}, {0x049b88, 14}},
    {{0x0499c6, 3}, {0x049ac2, 26}, {0x049adc, 3}, {0x049adf, 4}, {0x049ae3, 4}, {0x049ae7, 4}, {0x049aeb, 3}, {0x049aee, 3}, {0x049af1, 3}, {0x049b06, 7}, {0x049b1f, 7}, {0x049b38, 7}, {0x049b3f, 4}, {0x049b43, 4}, {0x049b47, 4}, {0x049b56, 14}, {0x049b6f, 14}, {0x049b88, 14}},
    {{0x0499eb, 3}, {0x049ac2, 26}, {0x049adc, 3}, {0x049adf, 4}, {0x049ae3, 4}, {0x049ae7, 4}, {0x049aeb, 3}, {0x049aee, 3}, {0x049af1, 3}, {0x049b06, 7}, {0x049b1f, 7}, {0x049b38, 7}, {0x049b3f, 4}, {0x049b43, 4}, {0x049b47, 4}, {0x049b56, 14}, {0x049b6f, 14}, {0x049b88, 14}},
    {{0x049a16, 3}, {0x049ac2, 26}, {0x049adc, 3}, {0x049adf, 4}, {0x049ae3, 4}, {0x049ae7, 4}, {0x049aeb, 3}, {0x049aee, 3}, {0x049af1, 3}, {0x049b06, 7}, {0x049b1f, 7}, {0x049b38, 7}, {0x049b3f, 4}, {0x049b43, 4}, {0x049b47, 4}, {0x049b56, 14}, {0x049b6f, 14}, {0x049b88, 14}},
    {{0x049a47, 3}, {0x049ac2, 26}, {0x049adc, 3}, {0x049adf, 4}, {0x049ae3, 4}, {0x049ae7, 4}, {0x049aeb, 3}, {0x049aee, 3}, {0x049af1, 3}, {0x049b06, 7}, {0x049b1f, 7}, {0x049b38, 7}, {0x049b3f, 4}, {0x049b43, 4}, {0x049b47, 4}, {0x049b56, 14}, {0x049b6f, 14}, {0x049b88, 14}},
};

static void push_at_tail(int copy)
{
    int i;

    gsu_codes(push_at_tail_fetch[copy], 2);
    for (i = 0; i < 4; i++) {
        gsu.r[1] = gsu_add(gsu.r[1], gsu.r[1], 0);
        gsu.r[2] = gsu_add(gsu.r[2], gsu.r[2], 0);
        gsu.r[3] = gsu_add(gsu.r[3], gsu.r[3], 0);
    }
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_codes(push_at_tail_fetch[copy] + 2, 1);
    gsu_stw(FX_PUSH_ROT_GAP, gsu.r[0]);
    gsu_codes(push_at_tail_fetch[copy] + 3, 1);
    gsu_stw(FX_PUSH_POINT, gsu.r[1]);
    gsu_codes(push_at_tail_fetch[copy] + 4, 1);
    gsu_stw(FX_PUSH_POINT + 2, gsu.r[2]);
    gsu_codes(push_at_tail_fetch[copy] + 5, 1);
    gsu_stw(FX_PUSH_POINT + 4, gsu.r[3]);
    gsu_codes(push_at_tail_fetch[copy] + 6, 1);
    gsu.r[1] = gsu_ldw(FX_PUSH_ROT);
    gsu_codes(push_at_tail_fetch[copy] + 7, 1);
    gsu.r[2] = gsu_ldw(FX_PUSH_ROT + 8);
    gsu_codes(push_at_tail_fetch[copy] + 8, 1);
    gsu.r[3] = gsu_ldw(FX_PUSH_ROT + 16);
    ((gsu_stw(FX_PUSH_ROT, gsu_codes_then(push_at_tail_fetch[copy] + 9, 1, fx_row(FX_MATRIX, 2, GSU_COPY(3))))));
    ((gsu_stw(FX_PUSH_ROT + 8, gsu_codes_then(push_at_tail_fetch[copy] + 10, 1, fx_row(FX_MATRIX + 6, 2, GSU_COPY(4))))));
    ((gsu_stw(FX_PUSH_ROT + 16, gsu_codes_then(push_at_tail_fetch[copy] + 11, 1, fx_row(FX_MATRIX + 12, 2, GSU_COPY(5))))));
    gsu_codes(push_at_tail_fetch[copy] + 12, 1);
    gsu.r[1] = gsu_ldw(FX_PUSH_POINT);
    gsu_codes(push_at_tail_fetch[copy] + 13, 1);
    gsu.r[2] = gsu_ldw(FX_PUSH_POINT + 2);
    gsu_codes(push_at_tail_fetch[copy] + 14, 1);
    gsu.r[3] = gsu_ldw(FX_PUSH_POINT + 4);
    ((gsu_stw(FX_TURN_CROSS, gsu_codes_then(push_at_tail_fetch[copy] + 15, 1, fx_cross(gsu.r[3], FX_PUSH_ROT + 8, gsu.r[2], FX_PUSH_ROT + 16, GSU_COPY(0))))));
    ((gsu_stw(0x002C, gsu_codes_then(push_at_tail_fetch[copy] + 16, 1, fx_cross(gsu.r[1], FX_PUSH_ROT + 16, gsu.r[3], FX_PUSH_ROT, GSU_COPY(1))))));
    ((gsu_stw(0x002E, gsu_codes_then(push_at_tail_fetch[copy] + 17, 1, fx_cross(gsu.r[2], FX_PUSH_ROT, gsu.r[1], FX_PUSH_ROT + 8, GSU_COPY(2))))));
    spin_add(FX_TURN_CROSS, FX_SPIN + 2, 4, 1, GSU_COPY(0));
    spin_add(0x002E, FX_SPIN + 4, 5, 1, GSU_COPY(1));
    spin_add(0x002C, FX_SPIN, 6, 1, GSU_COPY(2));
    pull3(0, GSU_COPY(1));
}

/* The push from the world sum, each word scaled by `shift`: 2 (two ADDs),
   1 (one ADD), 0, -1 (DIV2) or -2 (two DIV2s), to $0132/$013A/$0142. */
/* gsuwalk: the code of each copy of push_from_sum, read from the ROM */
static const GsuCode push_from_sum_fetch[5][9] = {
    {{0x049aa6, 6}, {0x049ab1, 4}, {0x049aba, 4}, {0x049aac, 2}, {0x049ab5, 2}, {0x049abe, 2}, {0x049aae, 3}, {0x049ab7, 3}, {0x049ac0, 3}},
    {{0x0499ac, 6}, {0x0499b6, 4}, {0x0499be, 4}, {0x0499b2, 1}, {0x0499ba, 1}, {0x0499c2, 1}, {0x0499b3, 3}, {0x0499bb, 3}, {0x0499c3, 3}},
    {{0x0499d4, 6}, {0x0499dd, 4}, {0x0499e4, 4}, {0, 0}, {0, 0}, {0, 0}, {0x0499da, 3}, {0x0499e1, 3}, {0x0499e8, 3}},
    {{0x0499f9, 6}, {0x049a04, 4}, {0x049a0d, 4}, {0x0499ff, 2}, {0x049a08, 2}, {0x049a11, 2}, {0x049a01, 3}, {0x049a0a, 3}, {0x049a13, 3}},
    {{0x049a24, 6}, {0x049a31, 4}, {0x049a3c, 4}, {0x049a2a, 4}, {0x049a35, 4}, {0x049a40, 4}, {0x049a2e, 3}, {0x049a39, 3}, {0x049a44, 3}},
};

static void push_from_sum(int shift, int copy)
{
    int i, k;

    for (i = 0; i < 3; i++) {
        gsu_codes(push_from_sum_fetch[copy] + i, 1);
        gsu.r[0] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i));
        gsu_codes(push_from_sum_fetch[copy] + 3 + i, 1);
        for (k = 0; k < shift; k++)
            gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        for (k = 0; k > shift; k--)
            gsu.r[0] = gsu_div2(gsu.r[0]);
        gsu_codes(push_from_sum_fetch[copy] + 6 + i, 1);
        gsu_stw((uint16_t)(FX_PUSH_ROT + 8 * i), gsu.r[0]);
    }
}

/*
 * @routine 04:9A9B FxPushAt4
 * WHAT IT DOES: Pushes the object at the point (R1, R2, R3) in its own
 * axes with four times the world sum: the sum, turned into the object's
 * axes, goes to $0132/$013A/$0142, and the point (times 16) crossed with
 * it is added, four times, to the spin sums at $233C, $233E and $233A.
 * HOW: R11, R9, R8 pushed on the R10 stack; two ADDs per word of the
 * world sum; then the part all the variants share (04:9AC2): sixteen
 * times the point to $26F6/$26F8/$26FA, $0134 cleared, the matrix at
 * $00E4 (FMULT, ROL), the cross product (LMULT, SUB, SBC, ADD, ROL) to
 * $0068/$002C/$002E and the sums by SBK. Returns by pulling R15.
 * Entry: R1, R2, R3 = the point; R10 = stack; R11 = return address.
 * Exit: R1-R3 = the point times 16; R0 the last spin sum; R4, R5, R6, R7
 * as the cross product and sums leave them; R8, R9 restored; S, Z from
 * R10; the last RAM address R10. Its variants are picked by $2654
 * through the table at $04:9BC6.
 */
void abi_FxPushAt4(void)
{
    push3(GSU_COPY(2));
    push_from_sum(2, GSU_COPY(0));
    push_at_tail(GSU_COPY(0));
}

/*
 * @routine 04:99A1 FxPushAt2
 * WHAT IT DOES: As FxPushAt4 with twice the world sum.
 * HOW: As FxPushAt4, one ADD per word. Not reached in the traces so far.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAt2(void)
{
    push3(GSU_COPY(4));
    push_from_sum(1, GSU_COPY(1));
    push_at_tail(GSU_COPY(1));
}

/*
 * @routine 04:99C9 FxPushAt1
 * WHAT IT DOES: As FxPushAt4 with the world sum as it is.
 * HOW: As FxPushAt4, the words as loaded. Not reached in the traces so
 * far.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAt1(void)
{
    push3(GSU_COPY(5));
    push_from_sum(0, GSU_COPY(2));
    push_at_tail(GSU_COPY(2));
}

/*
 * @routine 04:99EE FxPushAtHalf
 * WHAT IT DOES: As FxPushAt4 with half the world sum.
 * HOW: As FxPushAt4, one DIV2 per word.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAtHalf(void)
{
    push3(GSU_COPY(6));
    push_from_sum(-1, GSU_COPY(3));
    push_at_tail(GSU_COPY(3));
}

/*
 * @routine 04:9A19 FxPushAtQuarter
 * WHAT IT DOES: As FxPushAt4 with a quarter of the world sum.
 * HOW: As FxPushAt4, two DIV2s per word. Not reached in the traces so far.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAtQuarter(void)
{
    push3(GSU_COPY(7));
    push_from_sum(-2, GSU_COPY(4));
    push_at_tail(GSU_COPY(4));
}

/*
 * @routine 04:9A4A FxPushAt4Twist
 * WHAT IT DOES: FxPushAt4, then the cross product's x ($0068) eight times
 * more into the spin sum at $233C.
 * HOW: R11 pushed on the R10 stack around the call; three ADDs and SBK.
 * Not reached in the traces so far.
 * Entry: as FxPushAt4. Exit: as FxPushAt4, R4 = the old spin sum, R11 =
 * $9A53; S, Z from R10.
 */
void abi_FxPushAt4Twist(void)
{
    gsu_code(0x049a4b, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[11] = 0x9A53;
    gsu_code(0x049a4d, 6); gsu_code(0x049a9b, 1);
    abi_FxPushAt4();
    gsu_code(0x049a53, 1);
    spin_add(FX_TURN_CROSS, FX_SPIN + 2, 4, 0, GSU_COPY(3));
    gsu_code(0x049a61, 4);
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}

/*
 * @routine 04:9985 FxPushAt4Kept
 * WHAT IT DOES: As FxPushAt4.
 * HOW: R11, R9 and R8 are pushed and pulled straight back (R11 into R11)
 * before the jump to FxPushAt4. Nothing calls it.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAt4Kept(void)
{
    push3(GSU_COPY(3));
    pull3(1, GSU_COPY(2));
    gsu_code(0x04999e, 3); gsu_code(0x049a9b, 1);
    abi_FxPushAt4();
}

/*
 * @routine 04:9927 FxPushAtTurned
 * WHAT IT DOES: As FxPushAt4 at the point (R1, R2, R3) turned by the
 * matrix at $00E4 first (left at $26F6/$26F8/$26FA).
 * HOW: The matrix rows (FMULT, ROL) as in the shared part, SM to
 * $26F6-$26FA, LM back into R1-R3, then a jump to FxPushAt4. Nothing
 * calls it.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAtTurned(void)
{
    (((gsu_stw(FX_PUSH_POINT, gsu_code_then(0x04993a, 8, fx_row(FX_MATRIX, 6, GSU_COPY(0)))))));
    (((gsu_stw(FX_PUSH_POINT + 2, gsu_code_then(0x049954, 8, fx_row(FX_MATRIX + 2, 6, GSU_COPY(1)))))));
    (((gsu_stw(FX_PUSH_POINT + 4, gsu_code_then(0x04996e, 8, fx_row(FX_MATRIX + 4, 6, GSU_COPY(2)))))));
    gsu_code(0x049976, 4);
    gsu.r[1] = gsu_ldw(FX_PUSH_POINT);
    gsu_code(0x04997a, 4);
    gsu.r[2] = gsu_ldw(FX_PUSH_POINT + 2);
    gsu_code(0x04997e, 4);
    gsu.r[3] = gsu_ldw(FX_PUSH_POINT + 4);
    gsu_code(0x049982, 3); gsu_code(0x049a9b, 1);
    abi_FxPushAt4();
}

/*
 * @routine 04:9A65 FxSumScale
 * WHAT IT DOES: Scales the world sum by R6 as an 8.8 fraction: each word
 * becomes bits 8-23 of its product with R6.
 * HOW: LMULT per word: the high word's low byte (LOB, SWAP) ORed with the
 * low word's high byte (HIB), then three SMs. Returns with MOVE R15,R11.
 * Not reached in the traces so far.
 * Entry: R6 = the scale; R11 = return address. Exit: R1-R3 the new sum;
 * R0 = the high byte part of z, R4 its low byte part; flags from the
 * last OR; the last RAM address $2344.
 */
/* gsuwalk: the code of each copy of abi_FxSumScale, read from the ROM */
static const GsuCode abi_FxSumScale_fetch[1][6] = {
    {{0x049a66, 4}, {0x049a6a, 4}, {0x049a6e, 4}, {0x049a87, 10}, {0x049a91, 4}, {0x049a95, 4}},
};

void abi_FxSumScale(void)
{
    int i;
    uint32_t m;

    for (i = 0; i < 3; i++)
        { gsu_codes(abi_FxSumScale_fetch[0] + i, 1); gsu.r[1 + i] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i)); }
    for (i = 0; i < 3; i++) {
        m = gsu_lmult(gsu.r[1 + i], gsu.r[6]);
        gsu.r[4] = (uint16_t)m;
        gsu.r[0] = gsu_swap(gsu_lob((uint16_t)(m >> 16)));
        gsu.r[4] = gsu_hib(gsu.r[4]);
        gsu.r[1 + i] = gsu_or(gsu.r[0], gsu.r[4]);
    }
    gsu_code(0x049a72, 21);
    for (i = 0; i < 3; i++)
        { gsu_codes(abi_FxSumScale_fetch[0] + 3 + i, 1); gsu_stw((uint16_t)(FX_WORLD_SUM + 2 * i), gsu.r[1 + i]); }
    gsu_code(0x049a99, 2);
}

/* 04:98CC-9926: |R1|, |R2|, |R3| (unless skip_abs); then each times half
   the world sum's word (DIV2, FMULT) added to the hit point; R8, R9 and
   the return pulled. */
/* gsuwalk: the code of each copy of hit_add, read from the ROM */
static const GsuCode hit_add_fetch[4][27] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x049890, 9}, {0x0498e4, 5}, {0x0498f7, 4}, {0, 0}, {0x049909, 4}, {0, 0}, {0x0498e9, 9}, {0x0498fb, 9}, {0x04990d, 9}, {0x0498f2, 5}, {0x049904, 5}, {0x049916, 5}},
    {{0x0498a7, 4}, {0x0498cc, 5}, {0x0498d4, 1}, {0x0498d5, 4}, {0x0498dc, 1}, {0, 0}, {0x0498dd, 4}, {0x0498e4, 1}, {0, 0}, {0x0498a7, 4}, {0x0498cc, 9}, {0x0498d5, 8}, {0, 0}, {0x0498dd, 8}, {0, 0}, {0x0498e5, 4}, {0, 0}, {0x0498f7, 4}, {0, 0}, {0x049909, 4}, {0, 0}, {0x0498e9, 9}, {0x0498fb, 9}, {0x04990d, 9}, {0x0498f2, 5}, {0x049904, 5}, {0x049916, 5}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x0498be, 9}, {0x0498e4, 5}, {0x0498f7, 4}, {0, 0}, {0x049909, 4}, {0, 0}, {0x0498e9, 9}, {0x0498fb, 9}, {0x04990d, 9}, {0x0498f2, 5}, {0x049904, 5}, {0x049916, 5}},
    {{0x0498cd, 4}, {0x0498d4, 1}, {0, 0}, {0x0498d5, 4}, {0x0498dc, 1}, {0, 0}, {0x0498dd, 4}, {0x0498e4, 1}, {0, 0}, {0x0498cd, 8}, {0, 0}, {0x0498d5, 8}, {0, 0}, {0x0498dd, 8}, {0, 0}, {0x0498e5, 4}, {0, 0}, {0x0498f7, 4}, {0, 0}, {0x049909, 4}, {0, 0}, {0x0498e9, 9}, {0x0498fb, 9}, {0x04990d, 9}, {0x0498f2, 5}, {0x049904, 5}, {0x049916, 5}},
};

static void hit_add(int skip_abs, int copy)
{
    int i;

    if (!skip_abs)
        for (i = 1; i <= 3; i++) {
            gsu_moves(gsu.r[i]);
            if (gsu.s)
                { gsu.r[i] = gsu_inc(gsu_not(gsu.r[i])); gsu_codes(hit_add_fetch[copy] + 9 + 2 * (i - 1), 2); } else { gsu_codes(hit_add_fetch[copy] + 3 * (i - 1), 3); }
        }
    for (i = 0; i < 3; i++) {
        gsu_codes(hit_add_fetch[copy] + 15 + 2 * i, 2);
        gsu.r[6] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i));
        gsu.r[6] = gsu_div2(gsu.r[6]);
        gsu.r[1 + i] = gsu_fmult(gsu.r[1 + i], gsu.r[6]);
        gsu_codes(hit_add_fetch[copy] + 21 + i, 1);
        gsu.r[0] = gsu_ldw((uint16_t)(FX_HIT + 2 * i));
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1 + i], 0);
        gsu_codes(hit_add_fetch[copy] + 24 + i, 1);
        gsu_stw((uint16_t)(FX_HIT + 2 * i), gsu.r[0]);
    }
    pull3(0, GSU_COPY(0));
}

/*
 * @routine 04:987D FxHitAddScaled
 * WHAT IT DOES: Adds to the hit point at $2334 the vector (R1, R2, R3)
 * scaled by R6 (the low words of LMULT), made positive, each times half
 * the world sum's word (FMULT); a zero vector counts as $7FFF on each
 * axis.
 * HOW: R11, R9, R8 pushed on the R10 stack; OR of the three tests for
 * zero; MOVES, NOT, INC for the magnitudes; DIV2, FMULT, ADD, SM per axis.
 * Returns by pulling R15.
 * Entry: R1-R3 = the vector, R6 = the scale; R10 = stack; R11 = return
 * address. Exit: R1-R3 the amounts added; R0 the new hit z; R6 half the
 * world z; R4 the last low product; R8, R9 restored; S, Z from R10; the
 * last RAM address R10.
 */
void abi_FxHitAddScaled(void)
{
    uint32_t m;
    int i;

    push3(GSU_COPY(0));
    gsu.r[0] = gsu_or(gsu_or(gsu.r[1], gsu.r[2]), gsu.r[3]);
    if (gsu.z) {
        gsu.r[1] = 0x7FFF;
        gsu.r[2] = gsu.r[1];
        gsu.r[3] = gsu.r[1];
        gsu_code(0x049888, 8);
        hit_add(1, GSU_COPY(0));
        return;
    } else { gsu_code(0x049888, 7); gsu_code(0x049899, 1); }
    for (i = 1; i <= 3; i++) {
        m = gsu_lmult(gsu.r[i], gsu.r[6]);
        gsu.r[0] = (uint16_t)(m >> 16);
        gsu.r[4] = (uint16_t)m;
        gsu.r[i] = gsu.r[4];
    }
    gsu_code(0x04989a, 13);
    hit_add(0, GSU_COPY(1));
}

/*
 * @routine 04:98AB FxHitAddUnit
 * WHAT IT DOES: As FxHitAddScaled with the vector made a unit vector
 * (FxNormalize) instead of scaled.
 * HOW: As FxHitAddScaled. Not reached in the traces so far.
 * Entry: R1-R3 = the vector; R10 = stack; R11 = return address. Exit: as
 * FxHitAddScaled, the rest as FxNormalize leaves them.
 */
void abi_FxHitAddUnit(void)
{
    FxRegs g;

    push3(GSU_COPY(1));
    gsu.r[0] = gsu_or(gsu_or(gsu.r[1], gsu.r[2]), gsu.r[3]);
    if (gsu.z) {
        gsu.r[1] = 0x7FFF;
        gsu.r[2] = gsu.r[1];
        gsu.r[3] = gsu.r[1];
        gsu_code(0x0498b6, 8);
        hit_add(1, GSU_COPY(2));
        return;
    } else { gsu_code(0x0498b6, 7); gsu_code(0x0498c7, 1); }
    gsu.r[11] = 0x98CC;
    memcpy(g.r, gsu.r, sizeof g.r);
    gsu_code(0x0498c8, 4); gsu_code(0x04c087, 1);
    g = FxNormalize(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
    gsu_code(0x0498cc, 1);
    hit_add(0, GSU_COPY(3));
}
