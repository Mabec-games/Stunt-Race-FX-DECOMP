/*
 * fx_rotate.c: the Super FX's rotations: a vector from an angle, a turn
 * about the vertical, and a 3x3 matrix transposed.
 *
 * Angles are 0-255 for a full turn; cosine and sine are signed bytes
 * (64 = 1/2) in the tables at $00:973E and $00:96FE, the cosine table being
 * the sine table 64 entries on. The Super FX reads them through the ROM
 * buffer with ROMBR = 0. Each byte is SWAPped into the high byte, so a
 * product's high word is the value times the byte / 256.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

#define ROT_COS 0x973Eu
#define ROT_SIN 0x96FEu

/* The table byte at `table` + i, SWAPped into the high byte. */
/* gsuwalk: the code of each copy of trig, read from the ROM */
static const GsuCode trig_fetch[4][2] = {
    {{0x04c15f, 7}, {0x04c166, 1}},
    {{0x04c167, 7}, {0x04c16e, 1}},
    {{0x04cda8, 2}, {0x04cdaa, 1}},
    {{0x04cdbb, 2}, {0x04cdbd, 1}},
};

static uint16_t trig(uint16_t table, uint16_t i, uint16_t *r14, int copy)
{
    gsu_codes(trig_fetch[copy], 1);
    gsu_r14(gsu_add(i, table, 0));
    *r14 = gsu.r[14];
    gsu_codes(trig_fetch[copy] + 1, 1);
    return gsu_swap((gsu_getb()));
}

/* As trig, with R14 first loaded with the table address (IWT R14, then
   ADD R14): two writes of R14. */
/* gsuwalk: the code of each copy of trig_r14, read from the ROM */
static const GsuCode trig_r14_fetch[2][1] = {
    {{0x04cda0, 8}},
    {{0x04cdb8, 3}},
};

/* gsuwalk: the copy of trig each copy of trig_r14 calls */
static const uint8_t trig_in_trig_r14[2] = {2, 3};

static uint16_t trig_r14(uint16_t table, uint16_t i, uint16_t *r14, int copy)
{
    gsu_codes(trig_r14_fetch[copy], 1);
    gsu_r14(table);
    return trig(table, i, r14, GSU_COPY(trig_in_trig_r14[copy]));
}

typedef struct {
    uint16_t x, z;      /* -sin, cos times the length / 16 */
    uint16_t angle;     /* the high byte of R0 */
    uint16_t r14;
} FxAngleOut;

/*
 * @routine 04:CD98 FxAngleVector
 * WHAT IT DOES: The vector of length R4 at the angle in R0's high byte:
 * R1 = -sin * length, R3 = cos * length, both scaled by 16 / 128 (a byte
 * table value of 64 gives length * 8), and R2 = 0.
 * HOW: HIB for the angle, ROMB 0, then for each table the byte SWAPped up,
 * FMULT by R6 = length, (NOT and INC for the sine), and four ADDs to
 * itself.
 * Entry: R0 high byte = angle; R4 = length; R11 = return address. Exit: R1,
 * R3 as above, R0 = R3; R2 = 0; R6 = length; R14 = $973E + angle; ROMBR =
 * 0; OV, S, CY, Z from the last ADD.
 */
FxAngleOut FxAngleVector(uint16_t r0, uint16_t length, int copy)
{
    FxAngleOut o;
    uint16_t v;
    int i;

    o.angle = gsu_hib(r0);
    gsu_code(0x04cd99, 7);
    gsu_romb(0);
    v = gsu_fmult(gsu_code_then(0x04cdab, 2, trig_r14(ROT_SIN, gsu_lob(o.angle), &o.r14, GSU_COPY(0))), length);
    v = gsu_inc(gsu_not(v));
    gsu_code(0x04cdad, 11);
    for (i = 0; i < 4; i++)
        v = gsu_add(v, v, 0);
    o.x = v;
    v = gsu_fmult(gsu_code_then(0x04cdbe, 2, trig_r14(ROT_COS, gsu_lob(o.angle), &o.r14, GSU_COPY(1))), length);
    gsu_code(0x04cdc0, 8);
    for (i = 0; i < 4; i++)
        v = gsu_add(v, v, 0);
    o.z = v;
    gsu_code(0x04cdc8, 2);
    return o;
}

void abi_FxAngleVector(void)
{
    FxAngleOut o = FxAngleVector(gsu.r[0], gsu.r[4], GSU_COPY(0));

    gsu.r[0] = o.z;
    gsu.r[1] = o.x;
    gsu.r[2] = 0;
    gsu.r[3] = o.z;
    gsu.r[6] = gsu.r[4];
    gsu.r[14] = o.r14;
}

typedef struct {
    uint16_t x, z;      /* the turned vector */
    uint16_t cos, sin;  /* the table bytes in the high byte */
    uint16_t lo;        /* low word of the last product */
    uint16_t lo_sum;    /* the second sum's low word */
    uint16_t hi_term;   /* the high word of sin * z */
    uint16_t hi_sum;    /* the second sum's high word */
    uint16_t r14;
} FxTurnOut;

/*
 * @routine 04:C15C FxTurn
 * WHAT IT DOES: Turns the vector (R1, R3) by the angle R2 (0-255): R2 = R1 =
 * (sin * x + cos * z) / 2^17 and R3 = (cos * x - sin * z) / 2^17 with the
 * table bytes as signed values times 256.
 * HOW: The return address is pushed on the R10 stack and pulled into R15 at
 * the end. Each component is two LMULTs summed as 32-bit (ADD then ADC of
 * the low and high words) and the high word halved with ASR. The second
 * component's low words are subtracted the other way round (sin * z minus
 * cos * x) from its high words (cos * x minus sin * z); only the borrow of
 * that low subtraction reaches the result.
 * Entry: R1 = x, R2 = angle, R3 = z; R10 = stack; R11 = return address.
 * Exit: R1 = R2 = the new x; R3 = the new z; R0 = the second high word; R4 =
 * low word of cos * x; R5 = cos * 256; R6 = the old x; R7 = the second low
 * word; R8 = high word of sin * z; R9 = sin * 256; R14 = $96FE + angle; the
 * last RAM address R10; S, Z from R10; CY from the last ASR; OV from SBC.
 */
FxTurnOut FxTurn(uint16_t x, uint16_t angle, uint16_t z, uint16_t stack, uint16_t ret, int copy)
{
    FxTurnOut o;
    uint32_t p;
    uint16_t lo, hi;

    gsu_code(0x04c15d, 2);
    gsu_stw(stack, ret);
    stack = gsu_inc(gsu_inc(stack));
    o.cos = trig(ROT_COS, angle, &o.r14, GSU_COPY(0));
    o.sin = trig(ROT_SIN, angle, &o.r14, GSU_COPY(1));

    gsu_code(0x04c16f, 8);
    p = gsu_lmult(o.sin, x);
    hi = (uint16_t)(p >> 16);
    lo = (uint16_t)p;
    gsu_code(0x04c177, 7);
    p = gsu_lmult(o.cos, z);
    lo = gsu_add(lo, (uint16_t)p, 0);
    o.x = gsu_asr(gsu_add((uint16_t)(p >> 16), hi, gsu.cy));

    gsu_code(0x04c17e, 12);
    p = gsu_lmult(o.sin, z);
    o.hi_term = (uint16_t)(p >> 16);
    lo = (uint16_t)p;
    gsu_code(0x04c18a, 7);
    p = gsu_lmult(o.cos, x);
    o.lo = (uint16_t)p;
    o.lo_sum = gsu_sub(lo, o.lo, 0);
    o.hi_sum = gsu_sub((uint16_t)(p >> 16), o.hi_term, !gsu.cy);
    o.z = gsu_asr(o.hi_sum);

    stack = gsu_dec(gsu_dec(stack));
    gsu_code(0x04c191, 12);
    gsu_ldw(stack);
    return o;
}

/* FxTurn on the whole register file, for other routines' C. */
/* gsuwalk: the code of each copy of FxTurnRegs, read from the ROM */
static const GsuCode FxTurnRegs_fetch[2][2] = {
    {{0x049505, 4}, {0x04c15c, 1}},
    {{0x04cffe, 4}, {0x04c15c, 1}},
};

FxRegs FxTurnRegs(FxRegs g, int copy)
{
    uint16_t x = g.r[1];
    gsu_codes(FxTurnRegs_fetch[copy], 2);
    FxTurnOut o = FxTurn(x, g.r[2], g.r[3], g.r[10], g.r[11], GSU_COPY(0));

    g.r[0] = o.hi_sum;
    g.r[1] = o.x;
    g.r[2] = o.x;
    g.r[3] = o.z;
    g.r[4] = o.lo;
    g.r[5] = o.cos;
    g.r[6] = x;
    g.r[7] = o.lo_sum;
    g.r[8] = o.hi_term;
    g.r[9] = o.sin;
    g.r[14] = o.r14;
    return g;
}

void abi_FxTurn(void)
{
    uint16_t x = gsu.r[1];
    FxTurnOut o = FxTurn(x, gsu.r[2], gsu.r[3], gsu.r[10], gsu.r[11], GSU_COPY(0));

    gsu.r[0] = o.hi_sum;
    gsu.r[1] = o.x;
    gsu.r[2] = o.x;
    gsu.r[3] = o.z;
    gsu.r[4] = o.lo;
    gsu.r[5] = o.cos;
    gsu.r[6] = x;
    gsu.r[7] = o.lo_sum;
    gsu.r[8] = o.hi_term;
    gsu.r[9] = o.sin;
    gsu.r[14] = o.r14;
}

/*
 * @routine 04:C2C8 FxMatrixTranspose04
 * WHAT IT DOES: Transposes the 3x3 matrix of words at R9 in place.
 * HOW: Three swaps of the off-diagonal pairs (words 1 and 3, 2 and 6, 5 and
 * 7), walking R9 with INC and ADD/SUB of small constants: loads and stores
 * in the order 1, 2, 3, store 3, store 1, load 5, 6, store 6, load 7, store
 * 7, store 5, store 2.
 * Entry: R9 = the matrix; R11 = return address. Exit: R9 = matrix + 4; R1 =
 * the old word 5, R2 = the old word 7, R3 = the old word 6; the last RAM
 * address matrix + 4; OV, S, CY, Z from the SUB that leaves R9.
 */
void FxMatrixTranspose04(uint16_t m, uint16_t *left, int copy)
{
    uint16_t w1, w2, w3, w5, w6, w7;

    gsu_code(0x04c2c9, 4);
    w1 = gsu_ldw((uint16_t)(m + 2));
    gsu_code(0x04c2cd, 4);
    w2 = gsu_ldw((uint16_t)(m + 4));
    gsu_code(0x04c2d1, 4);
    w3 = gsu_ldw((uint16_t)(m + 6));
    gsu_code(0x04c2d5, 2);
    gsu_stw((uint16_t)(m + 6), w1);
    gsu_code(0x04c2d7, 5);
    gsu_stw((uint16_t)(m + 2), w3);
    gsu_code(0x04c2dc, 5);
    w5 = gsu_ldw((uint16_t)(m + 10));
    gsu_code(0x04c2e1, 4);
    w6 = gsu_ldw((uint16_t)(m + 12));
    gsu_code(0x04c2e5, 2);
    gsu_stw((uint16_t)(m + 12), w2);
    gsu_code(0x04c2e7, 4);
    w7 = gsu_ldw((uint16_t)(m + 14));
    gsu_code(0x04c2eb, 2);
    gsu_stw((uint16_t)(m + 14), w5);
    gsu_code(0x04c2ed, 5);
    gsu_stw((uint16_t)(m + 10), w7);
    gsu_sub((uint16_t)(m + 10), 6, 0);
    gsu_code(0x04c2f2, 5);
    gsu_stw((uint16_t)(m + 4), w6);
    left[0] = w5;
    left[1] = w7;
    left[2] = w6;
    gsu_code(0x04c2f7, 1);
}

void abi_FxMatrixTranspose04(void)
{
    uint16_t m = gsu.r[9], left[3];

    FxMatrixTranspose04(m, left, GSU_COPY(0));
    gsu.r[1] = left[0];
    gsu.r[2] = left[1];
    gsu.r[3] = left[2];
    gsu.r[9] = (uint16_t)(m + 4);
    gsu_code(0x040000 | gsu.r[11], 1);
}

#define FX_SINE16 0x983Eu      /* sine words by angle / 256, 257 entries */
#define FX_ANGLES 0x0020u      /* three angle words */

/* The sine of a 16-bit angle: the word table at $00:983E entry angle / 256,
   plus the step to the next entry times the low byte / 256, the fraction
   taken as low byte * 128 with FMULT and the top bit rolled back in. */
/* gsuwalk: the code of each copy of sine16, read from the ROM */
static const GsuCode sine16_fetch[6][9] = {
    {{0x04c1b0, 8}, {0x04c1b8, 2}, {0x04c1ba, 1}, {0x04c1bb, 3}, {0x04c1be, 1}, {0x04c1bf, 1}, {0x04c1c0, 1}, {0x04c1c1, 2}, {0x04c1c3, 7}},
    {{0x04c1d0, 10}, {0x04c1da, 2}, {0x04c1dc, 1}, {0x04c1dd, 3}, {0x04c1e0, 1}, {0x04c1e1, 1}, {0x04c1e2, 1}, {0x04c1e3, 2}, {0x04c1e5, 7}},
    {{0x04c1f2, 8}, {0x04c1fa, 2}, {0x04c1fc, 1}, {0x04c1fd, 3}, {0x04c200, 1}, {0x04c201, 1}, {0x04c202, 1}, {0x04c203, 2}, {0x04c205, 7}},
    {{0x04c212, 10}, {0x04c21c, 2}, {0x04c21e, 1}, {0x04c21f, 3}, {0x04c222, 1}, {0x04c223, 1}, {0x04c224, 1}, {0x04c225, 2}, {0x04c227, 7}},
    {{0x04c234, 8}, {0x04c23c, 2}, {0x04c23e, 1}, {0x04c23f, 3}, {0x04c242, 1}, {0x04c243, 1}, {0x04c244, 1}, {0x04c245, 2}, {0x04c247, 7}},
    {{0x04c254, 10}, {0x04c25e, 2}, {0x04c260, 1}, {0x04c261, 3}, {0x04c264, 1}, {0x04c265, 1}, {0x04c266, 1}, {0x04c267, 2}, {0x04c269, 7}},
};

static uint16_t sine16(uint16_t angle, int copy)
{
    uint16_t i = gsu_hib(angle), r14, s0, s1, step, frac;

    i = gsu_add(i, i, 0);
    gsu_codes(sine16_fetch[copy], 1);
    gsu_r14(gsu_add(i, FX_SINE16, 0));
    r14 = gsu.r[14];
    gsu_codes(sine16_fetch[copy] + 1, 1);
    s0 = (gsu_getb());
    gsu_codes(sine16_fetch[copy] + 2, 1);
    gsu_r14(gsu_inc(r14));
    r14 = gsu.r[14];
    gsu_codes(sine16_fetch[copy] + 3, 1);
    s0 = gsu_getbh(s0);
    gsu_codes(sine16_fetch[copy] + 4, 1);
    gsu_r14(gsu_inc(r14));
    r14 = gsu.r[14];
    gsu_codes(sine16_fetch[copy] + 5, 1);
    s1 = (gsu_getb());
    gsu_codes(sine16_fetch[copy] + 6, 1);
    gsu_r14(gsu_inc(r14));
    gsu_codes(sine16_fetch[copy] + 7, 1);
    s1 = gsu_getbh(s1);
    step = gsu_sub(s1, s0, 0);
    frac = gsu_lsr(gsu_swap(gsu_lob(angle)));
    gsu_codes(sine16_fetch[copy] + 8, 1);
    return gsu_add(gsu_rol(gsu_fmult(frac, step)), s0, 0);
}

/* The sine, then the cosine (the sine a quarter turn on). */
/* gsuwalk: the code of each copy of sine_cosine, read from the ROM */
static const GsuCode sine_cosine_fetch[3][1] = {
    {{0x04c1ca, 6}},
    {{0x04c20c, 6}},
    {{0x04c24e, 6}},
};

/* gsuwalk: the copy of sine16 each copy of sine_cosine calls */
static const uint8_t sine16_in_sine_cosine[3] = {0, 2, 4};

/* gsuwalk: the copy of sine16 each copy of sine_cosine calls */
static const uint8_t sine16_in_sine_cosine_2[3] = {1, 3, 5};

static void sine_cosine(uint16_t angle, uint16_t *s, uint16_t *c, int copy)
{
    *s = sine16(angle, GSU_COPY(sine16_in_sine_cosine[copy]));
    gsu_codes(sine_cosine_fetch[copy], 1);
    gsu_r14(0x4000);                       /* IWT R14 */
    *c = sine16(gsu_add(angle, gsu.r[14], 0), GSU_COPY(sine16_in_sine_cosine_2[copy]));
}

/* FMULT then ROL: a 1.15 fixed point product. */
/* gsuwalk: the code of each copy of mul15, read from the ROM */
static const GsuCode mul15_fetch[12][1] = {
    {{0x04c270, 7}},
    {{0x04c277, 4}},
    {{0x04c27b, 6}},
    {{0x04c281, 4}},
    {{0x04c287, 4}},
    {{0x04c28e, 4}},
    {{0x04c295, 6}},
    {{0x04c29d, 4}},
    {{0x04c2a3, 4}},
    {{0x04c2af, 6}},
    {{0x04c2b8, 4}},
    {{0x04c2bf, 6}},
};

static uint16_t mul15(uint16_t a, uint16_t b, int copy)
{
    (void)copy;
    gsu_codes(mul15_fetch[copy], 1);
    return gsu_rol(gsu_fmult(a, b));     /* FMULT, ROL: their time and flags */
}

typedef struct {
    uint16_t s0, c0, s1, c1, s2, c2;
    uint16_t s1c2, c1c2, s1s2, c1s2;
    uint16_t last;
} FxAnglesOut;

/*
 * @routine 04:C1A7 FxMatrixFromAngles
 * WHAT IT DOES: Builds the 3x3 rotation matrix of the three angles at $0020
 * ($0022, $0024) into the nine words at R9: with s, c the sines and
 * cosines of angles 0, 1, 2, the words are s0s1s2 + c1c2, s0s1c2 - c1s2,
 * c0s1, c0s2, c0c2, -s0, s0c1s2 - s1c2, s0c1c2 + s1s2, c0c1.
 * HOW: RAMB and ROMB 0. Each sine is read from the word table at $00:983E
 * with linear interpolation (sine16); cosines are the sine a quarter turn
 * on. Products are FMULT then ROL (1.15 fixed point), the shared pairs
 * worked out first; the words are stored with STW in order.
 * Entry: R9 = where the matrix goes; R11 = return address. Exit: R9 =
 * matrix + 16; R0 = the last word; R1 = s1c2, R2 = c1c2, R3 = c1, R4 = s1,
 * R5 = c2, R6 = R8 = c0, R7 = s2, R12 = s0, R13 = s1s2, R14 = c1s2; ROMBR =
 * RAMBR = 0; S, CY, Z from the last ROL, OV from the add of the eighth
 * word; the last RAM address matrix + 16.
 */
FxAnglesOut FxMatrixFromAngles(uint16_t m, int copy)
{
    FxAnglesOut o;
    uint16_t w[9];

    gsu_sub(0, 0, 0);                      /* SUB R0 for the banks */
    gsu_code(0x04c1a8, 3);
    gsu_ramb(0);
    gsu_code(0x04c1ab, 2);
    gsu_romb(0);
    gsu_code(0x04c1ad, 3);
    sine_cosine(gsu_ldw(FX_ANGLES), &o.s0, &o.c0, GSU_COPY(0));
    gsu_code(0x04c1ec, 6);
    sine_cosine(gsu_ldw(FX_ANGLES + 2), &o.s1, &o.c1, GSU_COPY(1));
    gsu_code(0x04c22e, 6);
    sine_cosine(gsu_ldw(FX_ANGLES + 4), &o.s2, &o.c2, GSU_COPY(2));
    o.s1c2 = mul15(o.s1, o.c2, GSU_COPY(0));
    o.c1c2 = mul15(o.c1, o.c2, GSU_COPY(1));
    o.s1s2 = mul15(o.s1, o.s2, GSU_COPY(2));
    ((gsu_r14(gsu_code_then(0x04c285, 2, mul15(o.c1, o.s2, GSU_COPY(3))))));
    o.c1s2 = gsu.r[14];
    /* each element stored as it is made (STW (R9), INC R9 twice) */
    w[0] = gsu_add(mul15(o.s1s2, o.s0, GSU_COPY(4)), o.c1c2, 0);
    gsu_code(0x04c28b, 3);
    gsu_stw((uint16_t)(m + 0), w[0]);
    gsu_inc(gsu_inc((uint16_t)(m + 0)));
    w[1] = gsu_sub(mul15(o.s1c2, o.s0, GSU_COPY(5)), o.c1s2, 0);
    gsu_code(0x04c292, 3);
    gsu_stw((uint16_t)(m + 2), w[1]);
    gsu_inc(gsu_inc((uint16_t)(m + 2)));
    w[2] = mul15(o.s1, o.c0, GSU_COPY(6));
    gsu_code(0x04c29b, 2);
    gsu_stw((uint16_t)(m + 4), w[2]);
    gsu_inc(gsu_inc((uint16_t)(m + 4)));
    w[3] = mul15(o.s2, o.c0, GSU_COPY(7));
    gsu_code(0x04c2a1, 2);
    gsu_stw((uint16_t)(m + 6), w[3]);
    gsu_inc(gsu_inc((uint16_t)(m + 6)));
    w[4] = mul15(o.c2, o.c0, GSU_COPY(8));
    gsu_code(0x04c2a7, 2);
    gsu_stw((uint16_t)(m + 8), w[4]);
    gsu_inc(gsu_inc((uint16_t)(m + 8)));
    w[5] = gsu_inc(gsu_not(o.s0));
    gsu_code(0x04c2a9, 6);
    gsu_stw((uint16_t)(m + 10), w[5]);
    gsu_inc(gsu_inc((uint16_t)(m + 10)));
    w[6] = gsu_sub(mul15(o.c1s2, o.s0, GSU_COPY(9)), o.s1c2, 0);
    gsu_code(0x04c2b5, 3);
    gsu_stw((uint16_t)(m + 12), w[6]);
    gsu_inc(gsu_inc((uint16_t)(m + 12)));
    w[7] = gsu_add(mul15(o.c1c2, o.s0, GSU_COPY(10)), o.s1s2, 0);
    gsu_code(0x04c2bc, 3);
    gsu_stw((uint16_t)(m + 14), w[7]);
    gsu_inc(gsu_inc((uint16_t)(m + 14)));
    w[8] = mul15(o.c1, o.c0, GSU_COPY(11));
    gsu_code(0x04c2c5, 2);
    gsu_stw((uint16_t)(m + 16), w[8]);
    o.last = w[8];
    gsu_code(0x04c2c7, 1);
    return o;
}

void abi_FxMatrixFromAngles(void)
{
    FxAnglesOut o = FxMatrixFromAngles(gsu.r[9], GSU_COPY(0));

    gsu.r[0] = o.last;
    gsu.r[1] = o.s1c2;
    gsu.r[2] = o.c1c2;
    gsu.r[3] = o.c1;
    gsu.r[4] = o.s1;
    gsu.r[5] = o.c2;
    gsu.r[6] = o.c0;
    gsu.r[7] = o.s2;
    gsu.r[8] = o.c0;
    gsu.r[9] = (uint16_t)(gsu.r[9] + 16);
    gsu.r[12] = o.s0;
    gsu.r[13] = o.s1s2;
    gsu.r[14] = o.c1s2;
    gsu_code(0x040000 | gsu.r[11], 1);
}

#define FX_ATAN 0x9E40u        /* angle words by tangent / 64, 257 entries */

/*
 * @routine 04:C2F8 FxAngleOf
 * WHAT IT DOES: The angle of the vector (R1, R2), 0-$FFFF for a full turn,
 * to R0: $4000 for R2 = 0 (before the signs below), $2000 when |R1| =
 * |R2|, otherwise from the arctangent table at $00:9E40 indexed by the
 * smaller over the larger (times $4000, / 64); the result is negated when
 * R1 and R2 differ in sign, and $8000 is added when R2 is negative.
 * HOW: |R1| to R6 and |R2| to R0 with NOT and INC; the smaller (BLT) is
 * shifted into R0:R4 as times $4000 (LSR, ROR twice) and FxDivide gives the
 * ratio; the table word (GETB, GETBH, ROMBR 0) is the angle when |R2| is
 * the larger, else $4000 minus it. R11 and R14 are kept on the R10 stack.
 * Entry: R1, R2 = the vector; R10 = stack; R11 = return address. Exit: R0 =
 * the angle; R4 = R1 xor R2, or $8000 when R2 is negative; after the
 * divide: R5 = -1 when |R2| is the larger, else 0; R6 = the larger; R11 =
 * $C33E; R12 = 0; R13 = $C14B; R14 kept; S, Z from R10; the last RAM
 * address R10.
 */
FxRegs FxAngleOf(FxRegs g, int copy)
{
    FxQuotient q;

    gsu_code(0x04c2f9, 2);
    gsu_stw(g.r[10], g.r[11]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    gsu_code(0x04c2fb, 4);
    gsu_stw(g.r[10], g.r[14]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    g.r[6] = g.r[1];
    gsu_moves(g.r[6]);
    if (gsu.s)
        { g.r[6] = gsu_inc(gsu_not(g.r[6])); gsu_code(0x04c2ff, 10); } else { gsu_code(0x04c2ff, 6); gsu_code(0x04c308, 1); }
    g.r[0] = g.r[2];
    gsu_moves(g.r[0]);
    if (gsu.s)
        { g.r[0] = gsu_inc(gsu_not(g.r[0])); gsu_code(0x04c309, 7); } else { gsu_code(0x04c309, 4); gsu_code(0x04c30f, 1); }
    if (gsu.z) {
        g.r[0] = 0x4000;
        gsu_code(0x04c310, 9); gsu_code(0x04c35d, 1);
        goto signs;
    } else { gsu_code(0x04c310, 2); gsu_code(0x04c319, 1); }
    g.r[5] = 0;
    gsu_sub(g.r[0], g.r[6], 0);
    if (gsu.z) {
        g.r[0] = 0x2000;
        gsu_code(0x04c31a, 13); gsu_code(0x04c35d, 1);
        goto signs;
    } else { gsu_code(0x04c31a, 6); gsu_code(0x04c327, 1); }
    if (gsu.s == gsu.ov) {                     /* |R2| > |R1|: the other way up */
        g.r[4] = g.r[6];
        g.r[6] = g.r[0];
        g.r[0] = g.r[4];
        g.r[5] = gsu_dec(g.r[5]);
        gsu_code(0x04c328, 10);
    } else { gsu_code(0x04c328, 2); gsu_code(0x04c331, 1); }
    g.r[4] = 0;
    g.r[0] = gsu_lsr(g.r[0]);
    g.r[4] = gsu_ror(g.r[4]);
    g.r[0] = gsu_lsr(g.r[0]);
    g.r[4] = gsu_ror(g.r[4]);
    gsu_code(0x04c332, 12); gsu_code(0x04c144, 1);
    q = FxDivide(g.r[0], g.r[4], g.r[6], GSU_COPY(0));
    g.r[4] = q.quot;
    g.r[11] = 0xC33E;
    g.r[12] = 0;
    g.r[13] = 0xC14B;
    g.r[0] = gsu_sub(q.rem, q.rem, 0);
    gsu_code(0x04c33e, 4);
    gsu_romb(0);
    g.r[0] = gsu_lsr(gsu_lsr(gsu_lsr(gsu_lsr(gsu_lsr(g.r[4])))));
    g.r[0] = gsu_bic(g.r[0], 1);
    g.r[4] = FX_ATAN;
    gsu_code(0x04c342, 13);
    gsu_r14(gsu_add(g.r[0], g.r[4], 0));
    g.r[14] = gsu.r[14];
    gsu_code(0x04c34f, 1);
    g.r[0] = gsu_getb();
    gsu_code(0x04c350, 1);
    gsu_r14(gsu_inc(g.r[14]));
    g.r[14] = gsu.r[14];
    gsu_code(0x04c351, 2);
    g.r[0] = gsu_getbh(g.r[0]);
    gsu_moves(g.r[5]);
    if (!gsu.s) {
        g.r[0] = gsu_inc(gsu_not(g.r[0]));
        g.r[4] = 0x4000;
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        gsu_code(0x04c353, 11);
    } else { gsu_code(0x04c353, 4); gsu_code(0x04c35d, 1); }
signs:
    g.r[4] = gsu_xor(g.r[1], g.r[2]);
    if (gsu.s)
        { g.r[0] = gsu_inc(gsu_not(g.r[0])); gsu_code(0x04c35e, 9); } else { gsu_code(0x04c35e, 6); gsu_code(0x04c366, 1); }
    gsu_moves(g.r[2]);
    if (gsu.s) {
        g.r[4] = 0x8000;
        g.r[0] = gsu_add(g.r[0], g.r[4], 0);
        gsu_code(0x04c367, 9);
    } else { gsu_code(0x04c367, 4); gsu_code(0x04c36f, 1); }
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    gsu_code(0x04c370, 4);
    gsu_r14(gsu_ldw(g.r[10]));
    g.r[14] = gsu.r[14];
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    gsu_code(0x04c374, 4);
    gsu_ldw(g.r[10]);
    return g;
}

void abi_FxAngleOf(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxAngleOf(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
