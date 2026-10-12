/*
 * matrix.c: 3x3 matrices of signed words in Super FX RAM, nine elements
 * row by row, in 1.15 fixed point (FMULT keeps the high word of the
 * product and ROL doubles it, so each product is (a * b) >> 15). The
 * current one is at $00E4: rows at $E4, $EA, $F0.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_MATRIX 0x00E4u
#define FX_VECTORS 0x0116u  /* three vectors of three words */

/*
 * @routine 01:972B FxMatrixTranspose
 * WHAT IT DOES: Transposes the 3x3 matrix at R9 in place.
 * HOW: Swaps the three pairs across the diagonal: elements 1 and 3, then
 * 2 and 6 together with 5 and 7, loading each with LDW and storing them
 * back crossed with STW, R9 stepping between them.
 * Entry: R9 = the matrix; R11 = return address. Exit: R9 = the matrix + 4
 * (element 2); R1 = the old element 5, R2 = the old element 7, R3 = the
 * old element 6; the last RAM address R9; OV, S, CY, Z from the last SUB #6.
 * *old5, *old7, *old6 = those three, as the registers keep them.
 */
void FxMatrixTranspose(uint16_t m, uint16_t *old5, uint16_t *old7, uint16_t *old6)
{
    uint16_t e1, e2, e3, e5, e6, e7;

    e1 = gsu_ldw((uint16_t)(m + 2));
    e2 = gsu_ldw((uint16_t)(m + 4));
    e3 = gsu_ldw((uint16_t)(m + 6));
    gsu_stw((uint16_t)(m + 6), e1);
    gsu_stw((uint16_t)(m + 2), e3);
    e5 = gsu_ldw((uint16_t)(m + 10));
    e6 = gsu_ldw((uint16_t)(m + 12));
    gsu_stw((uint16_t)(m + 12), e2);
    e7 = gsu_ldw((uint16_t)(m + 14));
    gsu_stw((uint16_t)(m + 14), e5);
    gsu_stw((uint16_t)(m + 10), e7);
    gsu_stw((uint16_t)(m + 4), e6);
    *old5 = e5;
    *old7 = e7;
    *old6 = e6;
}

void abi_FxMatrixTranspose(void)
{
    uint16_t m = gsu.r[9];

    FxMatrixTranspose(m, &gsu.r[1], &gsu.r[2], &gsu.r[3]);
    gsu.r[9] = gsu_sub((uint16_t)(m + 10), 6, 0);
}

/*
 * @routine 01:978B FxMatrixApply
 * WHAT IT DOES: Multiplies the row vector (R1, R2, R3) by the current
 * matrix and stores the three results as words at R9.
 * HOW: CACHE; for each column, three FMULTs by the column's elements (LMS
 * into R6), each doubled by ROL, summed in R5 then R0 and stored with STW;
 * R9 steps 2 after each.
 * Entry: R1, R2, R3 = the vector, R9 = where to store; R11 = return
 * address. Exit: R9 = R9 + 6; R0 = the last result, R5 = its first two
 * terms, R6 = the matrix's last element; the last RAM address the last
 * result's; OV and CY from the last ADD R5, S and Z from R9.
 * Returns the last result; *partial and *m22 = R5 and R6 as left.
 */
uint16_t FxMatrixApply(uint16_t x, uint16_t y, uint16_t z, uint16_t out, uint16_t *partial, uint16_t *m22)
{
    uint16_t v[3] = {x, y, z};
    uint16_t sum = 0, part = 0, e = 0;
    int col, row;

    for (col = 0; col < 3; col++) {
        sum = 0;
        for (row = 0; row < 3; row++) {
            e = gsu_ldw((uint16_t)(FX_MATRIX + row * 6 + col * 2));
            if (row == 2)
                part = sum;
            sum = (uint16_t)(sum + (uint16_t)((int32_t)(int16_t)v[row] * (int16_t)e >> 15));
        }
        gsu_stw((uint16_t)(out + col * 2), sum);
    }
    *partial = part;
    *m22 = e;
    return sum;
}

/* The registers and flags FxMatrixApply leaves, from its last result,
   that result's first two terms and the last element. */
static void apply_exit(uint16_t out, uint16_t sum, uint16_t part, uint16_t e)
{
    gsu_add((uint16_t)(sum - part), part, 0);
    gsu.r[0] = sum;
    gsu.r[5] = part;
    gsu.r[6] = e;
    gsu.r[9] = (uint16_t)(out + 6);
    gsu_sz(gsu.r[9]);
}

void abi_FxMatrixApply(void)
{
    uint16_t out = gsu.r[9], part, e;
    uint16_t sum = FxMatrixApply(gsu.r[1], gsu.r[2], gsu.r[3], out, &part, &e);

    apply_exit(out, sum, part, e);
}

/*
 * @routine 01:975B FxMatrixApplyVectors
 * WHAT IT DOES: Multiplies the three vectors at $0116, $011C and $0122 by
 * the current matrix, storing the nine results as words from R9 on.
 * HOW: Keeps R11 in R8, then for each vector loads it into R1, R2, R3 with
 * LMS and calls FxMatrixApply, which steps R9; restores R11 and returns.
 * Entry: R9 = where to store; R11 = return address. Exit: R9 = R9 + 18;
 * R1, R2, R3 = the third vector; R8 = R11; R0, R5, R6, the last RAM address
 * and the flags as the last FxMatrixApply leaves them.
 */
void FxMatrixApplyVectors(uint16_t out, uint16_t *x, uint16_t *y, uint16_t *z,
                          uint16_t *sum, uint16_t *partial, uint16_t *m22)
{
    int i;

    for (i = 0; i < 3; i++) {
        uint16_t v = (uint16_t)(FX_VECTORS + i * 6);

        *x = gsu_ldw(v);
        *y = gsu_ldw((uint16_t)(v + 2));
        *z = gsu_ldw((uint16_t)(v + 4));
        *sum = FxMatrixApply(*x, *y, *z, (uint16_t)(out + i * 6), partial, m22);
    }
}

void abi_FxMatrixApplyVectors(void)
{
    uint16_t out = gsu.r[9], sum, part, e;

    FxMatrixApplyVectors(out, &gsu.r[1], &gsu.r[2], &gsu.r[3], &sum, &part, &e);
    apply_exit((uint16_t)(out + 12), sum, part, e);
    gsu.r[8] = gsu.r[11];
}

#define FX_ROTATE_IN_X 0x0068u
#define FX_ROTATE_IN_Y 0x002Cu
#define FX_ROTATE_IN_Z 0x002Eu
#define FX_ROTATE_OUT 0x0026u   /* three words */

/*
 * @routine 01:82CE FxMatrixRotateProgram
 * WHAT IT DOES: A program the S-CPU starts: multiplies the current matrix
 * by the column vector ($0068, $002C, $002E) and stores the result at
 * $0026, $0028, $002A.
 * HOW: CACHE; loads the vector into R1, R2, R3 with LMS; for each row, three
 * FMULTs by the row's elements, each doubled by ROL, summed in R5 then R0
 * and stored with SMS. Then STOP.
 * Entry: started by the S-CPU. Exit (at the STOP): R1, R2, R3 = the
 * vector; R0 = the last result, R5 = its first two terms, R6 = the
 * matrix's last element; the last RAM address $002A; OV, S, CY, Z from the
 * last ADD R5.
 * Returns the last result; *partial and *m22 = R5 and R6 as left.
 */
uint16_t FxMatrixRotateProgram(uint16_t v[3], uint16_t *partial, uint16_t *m22)
{
    uint16_t sum = 0;
    int row, col;

    v[0] = gsu_ldw(FX_ROTATE_IN_X);
    v[1] = gsu_ldw(FX_ROTATE_IN_Y);
    v[2] = gsu_ldw(FX_ROTATE_IN_Z);
    for (row = 0; row < 3; row++) {
        sum = 0;
        for (col = 0; col < 3; col++) {
            *m22 = gsu_ldw((uint16_t)(FX_MATRIX + row * 6 + col * 2));
            if (col == 2)
                *partial = sum;
            sum = (uint16_t)(sum + (uint16_t)((int32_t)(int16_t)v[col] * (int16_t)*m22 >> 15));
        }
        gsu_stw((uint16_t)(FX_ROTATE_OUT + row * 2), sum);
    }
    return sum;
}

void abi_FxMatrixRotateProgram(void)
{
    uint16_t v[3], part, e;
    uint16_t sum = FxMatrixRotateProgram(v, &part, &e);

    gsu.r[1] = v[0];
    gsu.r[2] = v[1];
    gsu.r[3] = v[2];
    gsu_add((uint16_t)(sum - part), part, 0);
    gsu.r[0] = sum;
    gsu.r[5] = part;
    gsu.r[6] = e;
}

#define FX_ANGLES 0x0020u            /* three angles: $0020, $0022, $0024 */
#define FX_SINE_TABLE 0x983Eu        /* ROM bank $00: 257 words, 1.15 */

/* (a * b) >> 15, signed: FMULT then ROL. */
static uint16_t mul15(uint16_t a, uint16_t b)
{
    return (uint16_t)((int32_t)(int16_t)a * (int16_t)b >> 15);
}

/* The sine of an angle (a full turn $10000) from the table, between its
   entries by the angle's low byte: GETB/GETBH through the ROM buffer. */
static uint16_t fx_sine(uint16_t angle)
{
    uint16_t at = (uint16_t)(FX_SINE_TABLE + (angle >> 8) * 2);
    uint16_t a, b;

    gsu_r14(at);
    a = gsu_getb();
    gsu_r14(++at);
    a = gsu_getbh(a);
    gsu_r14(++at);
    b = gsu_getb();
    gsu_r14(++at);
    b = gsu_getbh(b);
    return (uint16_t)(a + mul15((uint16_t)((angle & 0xFF) << 7), (uint16_t)(b - a)));
}

/* What FxMatrixFromAngles01 leaves in registers. */
typedef struct { uint16_t sz, cz, sx, cx, sy, cy, sx_cy, cx_cy, sx_sy, cx_sy; } FxAnglesLeft;

/*
 * @routine 01:832F FxMatrixFromAngles01
 * WHAT IT DOES: Builds the rotation matrix for the three angles at $0020
 * (Z), $0022 (X) and $0024 (Y) and stores its nine words at R9.
 * HOW: Sets the RAM and ROM banks to 0. Each sine comes from the table at
 * $00:983E: the angle's high byte picks two neighbouring words (GETB,
 * GETBH), and the low byte << 7 times their difference (FMULT, ROL) is
 * added to the first; each cosine is the sine of the angle + $4000. With
 * products (a * b) >> 15 (FMULT, ROL) the rows are: sx sy sz + cx cy,
 * sx cy sz - cx sy, sx cz; sy cz, cy cz, -sz; cx sy sz - sx cy,
 * cx cy sz + sx sy, cx cz.
 * Entry: R9 = where to store; R11 = return address. Exit: R9 = R9 + 16;
 * R12, R8 = sin and cos of Z; R4, R3 = of X; R7, R5 = of Y; R1 = sx cy,
 * R2 = cx cy, R13 = sx sy, R14 = cx sy; R6 = cos Z; R0 = the last element;
 * the RAM and ROM banks 0; the last RAM address the last element's; CY,
 * S, Z from its ROL; OV from the ADD R13 of the element before.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxMatrixFromAngles01(uint16_t out, FxAnglesLeft *k)
{
    uint16_t m[9];
    uint16_t a;
    int i;

    gsu.rambr = 0;
    gsu.rombr = 0;
    a = gsu_ldw(FX_ANGLES);
    k->sz = fx_sine(a);
    gsu_r14(0x4000);
    k->cz = fx_sine((uint16_t)(a + 0x4000));
    a = gsu_ldw((uint16_t)(FX_ANGLES + 2));
    k->sx = fx_sine(a);
    gsu_r14(0x4000);
    k->cx = fx_sine((uint16_t)(a + 0x4000));
    a = gsu_ldw((uint16_t)(FX_ANGLES + 4));
    k->sy = fx_sine(a);
    gsu_r14(0x4000);
    k->cy = fx_sine((uint16_t)(a + 0x4000));
    k->sx_cy = mul15(k->sx, k->cy);
    k->cx_cy = mul15(k->cx, k->cy);
    k->sx_sy = mul15(k->sx, k->sy);
    k->cx_sy = mul15(k->cx, k->sy);
    gsu_r14(k->cx_sy);
    m[0] = (uint16_t)(mul15(k->sx_sy, k->sz) + k->cx_cy);
    m[1] = (uint16_t)(mul15(k->sx_cy, k->sz) - k->cx_sy);
    m[2] = mul15(k->sx, k->cz);
    m[3] = mul15(k->sy, k->cz);
    m[4] = mul15(k->cy, k->cz);
    m[5] = (uint16_t)-k->sz;
    m[6] = (uint16_t)(mul15(k->cx_sy, k->sz) - k->sx_cy);
    m[7] = (uint16_t)(mul15(k->cx_cy, k->sz) + k->sx_sy);
    m[8] = mul15(k->cx, k->cz);
    for (i = 0; i < 9; i++)
        gsu_stw((uint16_t)(out + i * 2), m[i]);
}

void abi_FxMatrixFromAngles01(void)
{
    uint16_t out = gsu.r[9];
    FxAnglesLeft k;
    uint32_t p;

    FxMatrixFromAngles01(out, &k);
    gsu_add(mul15(k.cx_cy, k.sz), k.sx_sy, 0);      /* OV of the ADD R13 */
    p = (uint32_t)((int32_t)(int16_t)k.cx * (int16_t)k.cz);
    gsu.cy = (uint8_t)(p >> 31);                    /* ROL takes FMULT's bit 15 */
    gsu.r[0] = (uint16_t)(p >> 15);
    gsu_sz(gsu.r[0]);
    gsu.r[1] = k.sx_cy;
    gsu.r[2] = k.cx_cy;
    gsu.r[3] = k.cx;
    gsu.r[4] = k.sx;
    gsu.r[5] = k.cy;
    gsu.r[6] = k.cz;
    gsu.r[7] = k.sy;
    gsu.r[8] = k.cz;
    gsu.r[9] = (uint16_t)(out + 16);
    gsu.r[12] = k.sz;
    gsu.r[13] = k.sx_sy;
}
