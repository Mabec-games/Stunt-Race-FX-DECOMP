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
    gsu_code(0x0497C0, 4);
    ly = gsu_ldw(FX_LOCAL_SUM + 2);
    gsu_code(0x0497C4, 4);
    o.lz = gsu_ldw(FX_LOCAL_SUM + 4);
    gsu_code(0x0497C8, 3);
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
    gsu_code(0x0496CA, 12); gsu_code(0x0496E1, 6); gsu_code(0x0496E9, 5); gsu_code(0x0496F9, 6); gsu_code(0x049701, 5); gsu_code(0x049711, 6); gsu_code(0x049719, 5);
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
    gsu_code(0x0496C3, 3);
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
    gsu_code(0x04973F, 3);
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
                g.r[1] = gsu_add(g.r[1], g.r[1], 0);
                g.r[1] = gsu_add(g.r[1], g.r[1], 0);
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
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[0] = 0x0000;
    gsu.rombr = 0;
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(FX_PUSH_FLAGS, gsu.r[0]);
    gsu.r[3] = 0x23AE;
    gsu.r[13] = 0x0000;
    for (;;) {
        gsu.r[13] = gsu_inc(gsu.r[13]);
        gsu.r[0] = 0x0014;
        gsu_sub(gsu.r[0], gsu.r[13], 0);
        if (gsu.z)
            break;
        gsu.r[11] = 0x943F;
        abi_FxPushApply();
    }
    gsu.r[0] = gsu_ldw(FX_PUSH_FLAGS);
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
static void push3(void)
{
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_push(&gsu.r[10], gsu.r[9]);
    gsu_push(&gsu.r[10], gsu.r[8]);
}

static void pull3(int r11)
{
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    if (r11)
        gsu.r[11] = gsu_pull(&gsu.r[10]);
    else
        gsu_pull(&gsu.r[10]);   /* the return address, into R15 */
}

/* One component through the matrix: FMULT and ROL of each of R1, R2, R3
   with the words at m, m + step, m + 2 * step, summed (R5 the running
   sum, R0 the result). */
static uint16_t fx_row(uint16_t m, uint16_t step)
{
    gsu.r[6] = gsu_ldw(m);
    gsu.r[5] = gsu_rol(gsu_fmult(gsu.r[1], gsu.r[6]));
    gsu.r[6] = gsu_ldw((uint16_t)(m + step));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[2], gsu.r[6]));
    gsu.r[5] = gsu_add(gsu.r[5], gsu.r[0], 0);
    gsu.r[6] = gsu_ldw((uint16_t)(m + 2 * step));
    gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[3], gsu.r[6]));
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
    return gsu.r[0];
}

/* One component of the cross product: a * R6(at p) - b * R6(at q) as 32
   bits (LMULT, SUB, SBC), doubled, its high word (ADD, ROL). */
static uint16_t fx_cross(uint16_t a, uint16_t p, uint16_t b, uint16_t q)
{
    uint32_t m;

    gsu.r[6] = gsu_ldw(p);
    m = gsu_lmult(a, gsu.r[6]);
    gsu.r[0] = (uint16_t)(m >> 16);
    gsu.r[4] = (uint16_t)m;
    gsu.r[7] = gsu.r[4];
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
static void spin_add(uint16_t from, uint16_t sum, int reg, int times4)
{
    gsu.r[0] = gsu_ldw(from);
    gsu.r[reg] = gsu_ldw(sum);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    if (!times4)
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
    gsu.r[0] = gsu_add(gsu.r[0], gsu.r[reg], 0);
    gsu_sbk(gsu.r[0]);
}

/* 04:9AC2, the variants' shared part: the point (R1, R2, R3) times 16 to
   $26F6; the push at $0132/$013A/$0142 turned into the object's axes by
   the matrix at $00E4; the point crossed with it into $68/$2C/$2E and
   four times each added to the spin sums; R8, R9 pulled and return. */
static void push_at_tail(void)
{
    gsu_code(0x049AC3, 28);
    int i;

    for (i = 0; i < 4; i++) {
        gsu.r[1] = gsu_add(gsu.r[1], gsu.r[1], 0);
        gsu.r[2] = gsu_add(gsu.r[2], gsu.r[2], 0);
        gsu.r[3] = gsu_add(gsu.r[3], gsu.r[3], 0);
    }
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(FX_PUSH_ROT_GAP, gsu.r[0]);
    gsu_code(0x049ADF, 4);
    gsu_stw(FX_PUSH_POINT, gsu.r[1]);
    gsu_code(0x049AE3, 4);
    gsu_stw(FX_PUSH_POINT + 2, gsu.r[2]);
    gsu_code(0x049AE7, 4);
    gsu_stw(FX_PUSH_POINT + 4, gsu.r[3]);
    gsu_code(0x049AEB, 3);
    gsu.r[1] = gsu_ldw(FX_PUSH_ROT);
    gsu_code(0x049AEE, 3);
    gsu.r[2] = gsu_ldw(FX_PUSH_ROT + 8);
    gsu_code(0x049AF1, 3);
    gsu.r[3] = gsu_ldw(FX_PUSH_ROT + 16);
    gsu_code(0x049AF4, 3);
    gsu_stw(FX_PUSH_ROT, gsu_code_then(0x049B06, 7, fx_row(FX_MATRIX, 2)));
    gsu_code(0x049B0D, 3);
    gsu_stw(FX_PUSH_ROT + 8, gsu_code_then(0x049B1F, 7, fx_row(FX_MATRIX + 6, 2)));
    gsu_code(0x049B26, 3);
    gsu_stw(FX_PUSH_ROT + 16, gsu_code_then(0x049B38, 7, fx_row(FX_MATRIX + 12, 2)));
    gsu_code(0x049B3F, 4);
    gsu.r[1] = gsu_ldw(FX_PUSH_POINT);
    gsu_code(0x049B43, 4);
    gsu.r[2] = gsu_ldw(FX_PUSH_POINT + 2);
    gsu_code(0x049B47, 4);
    gsu.r[3] = gsu_ldw(FX_PUSH_POINT + 4);
    gsu_code(0x049B4B, 3);
    gsu_stw(FX_TURN_CROSS, gsu_code_then(0x049B56, 14, fx_cross(gsu.r[3], FX_PUSH_ROT + 8, gsu.r[2], FX_PUSH_ROT + 16)));
    gsu_code(0x049B64, 3);
    gsu_stw(0x002C, gsu_code_then(0x049B6F, 14, fx_cross(gsu.r[1], FX_PUSH_ROT + 16, gsu.r[3], FX_PUSH_ROT)));
    gsu_code(0x049B7D, 3);
    gsu_stw(0x002E, gsu_code_then(0x049B88, 14, fx_cross(gsu.r[2], FX_PUSH_ROT, gsu.r[1], FX_PUSH_ROT + 8)));
    gsu_code(0x049B96, 4);
    spin_add(FX_TURN_CROSS, FX_SPIN + 2, 4, 1);
    gsu_code(0x049BA2, 4);
    spin_add(0x002E, FX_SPIN + 4, 5, 1);
    gsu_code(0x049BAE, 4);
    spin_add(0x002C, FX_SPIN, 6, 1);
    gsu_code(0x049BBA, 4);
    pull3(0);
}

/* The push from the world sum, each word scaled by `shift`: 2 (two ADDs),
   1 (one ADD), 0, -1 (DIV2) or -2 (two DIV2s), to $0132/$013A/$0142. */
static void push_from_sum(int shift)
{
    gsu_code(0x049AA6, 6);
    int i, k;

    for (i = 0; i < 3; i++) {
        gsu.r[0] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i));
        for (k = 0; k < shift; k++)
            gsu.r[0] = gsu_add(gsu.r[0], gsu.r[0], 0);
        for (k = 0; k > shift; k--)
            gsu.r[0] = gsu_div2(gsu.r[0]);
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
    push3();
    push_from_sum(2);
    push_at_tail();
}

/*
 * @routine 04:99A1 FxPushAt2
 * WHAT IT DOES: As FxPushAt4 with twice the world sum.
 * HOW: As FxPushAt4, one ADD per word. Not reached in the traces so far.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAt2(void)
{
    push3();
    push_from_sum(1);
    push_at_tail();
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
    push3();
    push_from_sum(0);
    push_at_tail();
}

/*
 * @routine 04:99EE FxPushAtHalf
 * WHAT IT DOES: As FxPushAt4 with half the world sum.
 * HOW: As FxPushAt4, one DIV2 per word.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAtHalf(void)
{
    push3();
    push_from_sum(-1);
    push_at_tail();
}

/*
 * @routine 04:9A19 FxPushAtQuarter
 * WHAT IT DOES: As FxPushAt4 with a quarter of the world sum.
 * HOW: As FxPushAt4, two DIV2s per word. Not reached in the traces so far.
 * Entry: as FxPushAt4. Exit: as FxPushAt4.
 */
void abi_FxPushAtQuarter(void)
{
    push3();
    push_from_sum(-2);
    push_at_tail();
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
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu.r[11] = 0x9A53;
    abi_FxPushAt4();
    spin_add(FX_TURN_CROSS, FX_SPIN + 2, 4, 0);
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
    push3();
    pull3(1);
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
    gsu_stw(FX_PUSH_POINT, fx_row(FX_MATRIX, 6));
    gsu_stw(FX_PUSH_POINT + 2, fx_row(FX_MATRIX + 2, 6));
    gsu_stw(FX_PUSH_POINT + 4, fx_row(FX_MATRIX + 4, 6));
    gsu.r[1] = gsu_ldw(FX_PUSH_POINT);
    gsu.r[2] = gsu_ldw(FX_PUSH_POINT + 2);
    gsu.r[3] = gsu_ldw(FX_PUSH_POINT + 4);
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
void abi_FxSumScale(void)
{
    int i;
    uint32_t m;

    for (i = 0; i < 3; i++)
        gsu.r[1 + i] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i));
    for (i = 0; i < 3; i++) {
        m = gsu_lmult(gsu.r[1 + i], gsu.r[6]);
        gsu.r[4] = (uint16_t)m;
        gsu.r[0] = gsu_swap(gsu_lob((uint16_t)(m >> 16)));
        gsu.r[4] = gsu_hib(gsu.r[4]);
        gsu.r[1 + i] = gsu_or(gsu.r[0], gsu.r[4]);
    }
    for (i = 0; i < 3; i++)
        gsu_stw((uint16_t)(FX_WORLD_SUM + 2 * i), gsu.r[1 + i]);
}

/* 04:98CC-9926: |R1|, |R2|, |R3| (unless skip_abs); then each times half
   the world sum's word (DIV2, FMULT) added to the hit point; R8, R9 and
   the return pulled. */
static void hit_add(int skip_abs)
{
    gsu_code(0x049888, 7); gsu_code(0x049899, 18); gsu_code(0x0498CC, 5); gsu_code(0x0498D4, 5); gsu_code(0x0498DC, 5); gsu_code(0x0498E4, 5);
    int i;

    if (!skip_abs)
        for (i = 1; i <= 3; i++) {
            gsu_moves(gsu.r[i]);
            if (gsu.s)
                gsu.r[i] = gsu_inc(gsu_not(gsu.r[i]));
        }
    for (i = 0; i < 3; i++) {
        gsu.r[6] = gsu_ldw((uint16_t)(FX_WORLD_SUM + 2 * i));
        gsu.r[6] = gsu_div2(gsu.r[6]);
        gsu.r[1 + i] = gsu_fmult(gsu.r[1 + i], gsu.r[6]);
        gsu.r[0] = gsu_ldw((uint16_t)(FX_HIT + 2 * i));
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[1 + i], 0);
        gsu_stw((uint16_t)(FX_HIT + 2 * i), gsu.r[0]);
        gsu_code(0x04991B, 4);
    }
    pull3(0);
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

    push3();
    gsu.r[0] = gsu_or(gsu_or(gsu.r[1], gsu.r[2]), gsu.r[3]);
    if (gsu.z) {
        gsu.r[1] = 0x7FFF;
        gsu.r[2] = gsu.r[1];
        gsu.r[3] = gsu.r[1];
        hit_add(1);
        return;
    }
    for (i = 1; i <= 3; i++) {
        m = gsu_lmult(gsu.r[i], gsu.r[6]);
        gsu.r[0] = (uint16_t)(m >> 16);
        gsu.r[4] = (uint16_t)m;
        gsu.r[i] = gsu.r[4];
    }
    hit_add(0);
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

    push3();
    gsu.r[0] = gsu_or(gsu_or(gsu.r[1], gsu.r[2]), gsu.r[3]);
    if (gsu.z) {
        gsu.r[1] = 0x7FFF;
        gsu.r[2] = gsu.r[1];
        gsu.r[3] = gsu.r[1];
        hit_add(1);
        return;
    }
    gsu.r[11] = 0x98CC;
    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxNormalize(g);
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
    hit_add(0);
}
