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
    gsu_code(0x01844F, 1); gsu_code(0x018749, 8); gsu_code(0x01972B, 5);
    uint16_t e1, e2, e3, e5, e6, e7;

    e1 = gsu_ldw((uint16_t)(m + 2));
    gsu_code(0x019730, 4);
    e2 = gsu_ldw((uint16_t)(m + 4));
    gsu_code(0x019734, 4);
    e3 = gsu_ldw((uint16_t)(m + 6));
    gsu_code(0x019738, 2);
    gsu_stw((uint16_t)(m + 6), e1);
    gsu_code(0x01973A, 5);
    gsu_stw((uint16_t)(m + 2), e3);
    gsu_code(0x01973F, 5);
    e5 = gsu_ldw((uint16_t)(m + 10));
    gsu_code(0x019744, 4);
    e6 = gsu_ldw((uint16_t)(m + 12));
    gsu_code(0x019748, 2);
    gsu_stw((uint16_t)(m + 12), e2);
    gsu_code(0x01974A, 4);
    e7 = gsu_ldw((uint16_t)(m + 14));
    gsu_code(0x01974E, 2);
    gsu_stw((uint16_t)(m + 14), e5);
    gsu_code(0x019750, 5);
    gsu_stw((uint16_t)(m + 10), e7);
    gsu_code(0x019755, 5);
    gsu_stw((uint16_t)(m + 4), e6);
    *old5 = e5;
    *old7 = e7;
    *old6 = e6;
    gsu_code(0x018751, 4); gsu_code(0x01975A, 1);
}

void abi_FxMatrixTranspose(void)
{
    uint16_t m = gsu.r[9];

    FxMatrixTranspose(m, &gsu.r[1], &gsu.r[2], &gsu.r[3]);
    gsu.r[9] = gsu_sub((uint16_t)(m + 10), 6, 0);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode apply_column_code[3][3] = {
    {{0x019790, 7}, {0x019797, 8}, {0x01979F, 5}},
    {{0x0197A9, 7}, {0x0197B0, 8}, {0x0197B8, 5}},
    {{0x0197C2, 7}, {0x0197C9, 8}, {0x0197D1, 5}},
};

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
/* One column of FxMatrixApply (the original has each in its own code):
   LMS R6, FROM R1, FMULT, TO R5, ROL; the same for R2, then WITH R5,
   ADD R0; for R3, then ADD R5; STW (R9); INC R9 twice. Returns R0; *part
   = R5, *e = R6. */
static uint16_t apply_column(const uint16_t v[3], uint16_t col, uint16_t *out, uint16_t *part, uint16_t *e, int copy)
{
    uint16_t t;

    *e = gsu_ldw((uint16_t)(FX_MATRIX + col * 2));
    *part = gsu_rol(gsu_fmult(v[0], *e));
    gsu_codes(apply_column_code[copy], 1);
    *e = gsu_ldw((uint16_t)(FX_MATRIX + 6 + col * 2));
    t = gsu_rol(gsu_fmult(v[1], *e));
    *part = gsu_add(*part, t, 0);
    gsu_codes(apply_column_code[copy] + 1, 1);
    *e = gsu_ldw((uint16_t)(FX_MATRIX + 12 + col * 2));
    t = gsu_rol(gsu_fmult(v[2], *e));
    t = gsu_add(t, *part, 0);
    gsu_codes(apply_column_code[copy] + 2, 1);
    gsu_stw(*out, t);
    *out = gsu_inc(gsu_inc(*out));
    return t;
}

uint16_t FxMatrixApply(uint16_t x, uint16_t y, uint16_t z, uint16_t out, uint16_t *partial, uint16_t *m22, int copy)
{
    const uint16_t v[3] = {x, y, z};

    apply_column(v, 0, &out, partial, m22, GSU_COPY(0));
    gsu_code(0x0197A4, 5);
    apply_column(v, 1, &out, partial, m22, GSU_COPY(1));
    gsu_code(0x0197BD, 5);
    return apply_column(v, 2, &out, partial, m22, GSU_COPY(2));
}

/* The registers and flags FxMatrixApply leaves, from its last result,
   that result's first two terms and the last element. */
static void apply_exit(uint16_t out, uint16_t sum, uint16_t part, uint16_t e, int copy)
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
    uint16_t sum = FxMatrixApply(gsu.r[1], gsu.r[2], gsu.r[3], out, &part, &e, GSU_COPY(0));

    apply_exit(out, sum, part, e, GSU_COPY(0));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode apply_vector_code[3][6] = {
    {{0x019761, 3}, {0x019764, 3}, {0x019767, 4}, {0x01978B, 1}, {0x978C, 0}, {0x01978C, 4}},
    {{0x01976F, 3}, {0x019772, 3}, {0x019775, 4}, {0x01978B, 1}, {0x978C, 0}, {0x01978C, 4}},
    {{0x01977D, 3}, {0x019780, 3}, {0x019783, 4}, {0x01978B, 1}, {0x978C, 0}, {0x01978C, 4}},
};

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
/* One vector of FxMatrixApplyVectors (each its own code in the
   original): LMS R1, R2, R3; FxMatrixApply. */
static void apply_vector(uint16_t i, uint16_t out, uint16_t *x, uint16_t *y, uint16_t *z,
                         uint16_t *sum, uint16_t *partial, uint16_t *m22, int copy)
{
    uint16_t v = (uint16_t)(FX_VECTORS + i * 6);

    *x = gsu_ldw(v);
    gsu_codes(apply_vector_code[copy], 1);
    *y = gsu_ldw((uint16_t)(v + 2));
    gsu_codes(apply_vector_code[copy] + 1, 1);
    *z = gsu_ldw((uint16_t)(v + 4));
    gsu_codes(apply_vector_code[copy] + 2, 4);
    *sum = FxMatrixApply(*x, *y, *z, (uint16_t)(out + i * 6), partial, m22, GSU_COPY(0));
}

void FxMatrixApplyVectors(uint16_t out, uint16_t *x, uint16_t *y, uint16_t *z,
                          uint16_t *sum, uint16_t *partial, uint16_t *m22, int copy)
{
    apply_vector(0, out, x, y, z, sum, partial, m22, GSU_COPY(0));
    gsu_code(0x01976B, 4); gsu_code(0x0197D6, 3);
    apply_vector(1, out, x, y, z, sum, partial, m22, GSU_COPY(1));
    gsu_code(0x019779, 4); gsu_code(0x0197D6, 3);
    apply_vector(2, out, x, y, z, sum, partial, m22, GSU_COPY(2));
}

void abi_FxMatrixApplyVectors(void)
{
    uint16_t out = gsu.r[9], sum, part, e;

    FxMatrixApplyVectors(out, &gsu.r[1], &gsu.r[2], &gsu.r[3], &sum, &part, &e, GSU_COPY(0));
    apply_exit((uint16_t)(out + 12), sum, part, e, GSU_COPY(0));
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
 * Entry: started by the S-CPU. Exit: (at the STOP) R1, R2, R3 = the
 * vector; R0 = the last result, R5 = its first two terms, R6 = the
 * matrix's last element; the last RAM address $002A; OV, S, CY, Z from the
 * last ADD R5.
 * Returns the last result; *partial and *m22 = R5 and R6 as left.
 */
uint16_t FxMatrixRotateProgram(uint16_t v[3], uint16_t *partial, uint16_t *m22)
{
    gsu_code(0x0182CF, 4);
    uint16_t sum = 0;
    int row, col;

    v[0] = gsu_ldw(FX_ROTATE_IN_X);
    gsu_code(0x0182D3, 3);
    v[1] = gsu_ldw(FX_ROTATE_IN_Y);
    gsu_code(0x0182D6, 3);
    v[2] = gsu_ldw(FX_ROTATE_IN_Z);
    gsu_code(0x0182D9, 3);
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

/*
 * @routine 01:8277 FxMatrixRotateBackProgram
 * WHAT IT DOES: A program the S-CPU starts: as FxMatrixRotateProgram with
 * the matrix transposed (the vector times its columns), back from the
 * object's axes: ($0068, $002C, $002E) to $0026, $0028, $002A.
 * HOW: As FxMatrixRotateProgram, output i summing the elements at $00E4 +
 * 2 * i, + 6 and + 12. Then STOP.
 * Entry: started by the S-CPU. Exit: as FxMatrixRotateProgram, R6 = the
 * element at $00F4.
 */
void abi_FxMatrixRotateBackProgram(void)
{
    gsu_code(0x018278, 4);
    int out;

    gsu.r[1] = gsu_ldw(FX_ROTATE_IN_X);
    gsu_code(0x01827C, 3);
    gsu.r[2] = gsu_ldw(FX_ROTATE_IN_Y);
    gsu_code(0x01827F, 3);
    gsu.r[3] = gsu_ldw(FX_ROTATE_IN_Z);
    gsu_code(0x018282, 3);
    for (out = 0; out < 3; out++) {
        uint16_t m = (uint16_t)(FX_MATRIX + out * 2);

        gsu.r[6] = gsu_ldw(m);
        gsu.r[5] = gsu_rol(gsu_fmult(gsu.r[1], gsu.r[6]));
        gsu.r[6] = gsu_ldw((uint16_t)(m + 6));
        gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[2], gsu.r[6]));
        gsu.r[5] = gsu_add(gsu.r[5], gsu.r[0], 0);
        gsu.r[6] = gsu_ldw((uint16_t)(m + 12));
        gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[3], gsu.r[6]));
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
        gsu_stw((uint16_t)(FX_ROTATE_OUT + out * 2), gsu.r[0]);
    }
}

#define FX_ANGLES 0x0020u            /* three angles: $0020, $0022, $0024 */
#define FX_SINE_TABLE 0x983Eu        /* ROM bank $00: 257 words, 1.15 */

/* (a * b) >> 15, signed: FMULT then ROL. */
static uint16_t mul15(uint16_t a, uint16_t b, int copy)
{
    return (uint16_t)((int32_t)(int16_t)a * (int16_t)b >> 15);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_sine_code[6][7] = {
    {{0x018340, 2}, {0x018342, 1}, {0x018343, 3}, {0x018346, 1}, {0x018347, 1}, {0x018348, 1}, {0x018349, 2}},
    {{0x018362, 2}, {0x018364, 1}, {0x018365, 3}, {0x018368, 1}, {0x018369, 1}, {0x01836A, 1}, {0x01836B, 2}},
    {{0x018382, 2}, {0x018384, 1}, {0x018385, 3}, {0x018388, 1}, {0x018389, 1}, {0x01838A, 1}, {0x01838B, 2}},
    {{0x0183A4, 2}, {0x0183A6, 1}, {0x0183A7, 3}, {0x0183AA, 1}, {0x0183AB, 1}, {0x0183AC, 1}, {0x0183AD, 2}},
    {{0x0183C4, 2}, {0x0183C6, 1}, {0x0183C7, 3}, {0x0183CA, 1}, {0x0183CB, 1}, {0x0183CC, 1}, {0x0183CD, 2}},
    {{0x0183E6, 2}, {0x0183E8, 1}, {0x0183E9, 3}, {0x0183EC, 1}, {0x0183ED, 1}, {0x0183EE, 1}, {0x0183EF, 2}},
};

/* The sine of an angle (a full turn $10000) from the table, between its
   entries by the angle's low byte: GETB/GETBH through the ROM buffer. */
static uint16_t fx_sine(uint16_t angle, int copy)
{
    uint16_t at = (uint16_t)(FX_SINE_TABLE + (angle >> 8) * 2);
    uint16_t a, b;

    gsu_r14(at);
    a = (gsu_codes(fx_sine_code[copy], 1), gsu_getb());
    gsu_codes(fx_sine_code[copy] + 1, 1);
    gsu_r14(++at);
    gsu_codes(fx_sine_code[copy] + 2, 1);
    a = gsu_getbh(a);
    gsu_codes(fx_sine_code[copy] + 3, 1);
    gsu_r14(++at);
    b = (gsu_codes(fx_sine_code[copy] + 4, 1), gsu_getb());
    gsu_codes(fx_sine_code[copy] + 5, 1);
    gsu_r14(++at);
    gsu_codes(fx_sine_code[copy] + 6, 1);
    b = gsu_getbh(b);
    return (uint16_t)(a + mul15((uint16_t)((angle & 0xFF) << 7), (uint16_t)(b - a), GSU_COPY(0)));
}

/* What FxMatrixFromAngles01 leaves in registers. */
typedef struct { uint16_t sz, cz, sx, cx, sy, cy, sx_cy, cx_cy, sx_sy, cx_sy; } FxAnglesLeft;

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxMatrixFromAngles01_code[2][17] = {
    {{0x018326, 7}, {0x01832F, 9}, {0x018338, 8}, {0x01834B, 13}, {0x018358, 10}, {0x01836D, 13}, {0x01837A, 8}, {0x01838D, 13}, {0x01839A, 10}, {0x0183AF, 13}, {0x0183BC, 8}, {0x0183CF, 13}, {0x0183DC, 10}, {0x0183F1, 30}, {0x01840F, 7}, {0x01832D, 1}, {0x01844F, 1}},
    {{0, 0}, {0, 0}, {0x018338, 8}, {0x01834B, 13}, {0x018358, 10}, {0x01836D, 13}, {0x01837A, 8}, {0x01838D, 13}, {0x01839A, 10}, {0x0183AF, 13}, {0x0183BC, 8}, {0x0183CF, 13}, {0x0183DC, 10}, {0x0183F1, 30}, {0x01840F, 7}, {0, 0}, {0, 0}},
};

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
void FxMatrixFromAngles01(uint16_t out, FxAnglesLeft *k, int copy)
{
    gsu_codes(FxMatrixFromAngles01_code[copy], 2);
    uint16_t m[9];
    uint16_t a;
    int i;

    gsu_ramb(0);
    gsu_romb(0);
    a = gsu_ldw(FX_ANGLES);
    gsu_codes(FxMatrixFromAngles01_code[copy] + 2, 1);
    k->sz = fx_sine(a, GSU_COPY(0));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 3, 1);
    gsu_r14(0x4000);
    gsu_codes(FxMatrixFromAngles01_code[copy] + 4, 1);
    k->cz = fx_sine((uint16_t)(a + 0x4000), GSU_COPY(1));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 5, 1);
    a = gsu_ldw((uint16_t)(FX_ANGLES + 2));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 6, 1);
    k->sx = fx_sine(a, GSU_COPY(2));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 7, 1);
    gsu_r14(0x4000);
    gsu_codes(FxMatrixFromAngles01_code[copy] + 8, 1);
    k->cx = fx_sine((uint16_t)(a + 0x4000), GSU_COPY(3));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 9, 1);
    a = gsu_ldw((uint16_t)(FX_ANGLES + 4));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 10, 1);
    k->sy = fx_sine(a, GSU_COPY(4));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 11, 1);
    gsu_r14(0x4000);
    gsu_codes(FxMatrixFromAngles01_code[copy] + 12, 1);
    k->cy = fx_sine((uint16_t)(a + 0x4000), GSU_COPY(5));
    gsu_codes(FxMatrixFromAngles01_code[copy] + 13, 1);
    k->sx_cy = mul15(k->sx, k->cy, GSU_COPY(0));
    k->cx_cy = mul15(k->cx, k->cy, GSU_COPY(0));
    k->sx_sy = mul15(k->sx, k->sy, GSU_COPY(0));
    k->cx_sy = mul15(k->cx, k->sy, GSU_COPY(0));
    gsu_r14(k->cx_sy);
    gsu_codes(FxMatrixFromAngles01_code[copy] + 14, 1);
    m[0] = (uint16_t)(mul15(k->sx_sy, k->sz, GSU_COPY(1)) + k->cx_cy);
    m[1] = (uint16_t)(mul15(k->sx_cy, k->sz, GSU_COPY(0)) - k->cx_sy);
    m[2] = mul15(k->sx, k->cz, GSU_COPY(0));
    m[3] = mul15(k->sy, k->cz, GSU_COPY(0));
    m[4] = mul15(k->cy, k->cz, GSU_COPY(0));
    m[5] = (uint16_t)-k->sz;
    m[6] = (uint16_t)(mul15(k->cx_sy, k->sz, GSU_COPY(0)) - k->sx_cy);
    m[7] = (uint16_t)(mul15(k->cx_cy, k->sz, GSU_COPY(0)) + k->sx_sy);
    m[8] = mul15(k->cx, k->cz, GSU_COPY(0));
    for (i = 0; i < 9; i++)
        { gsu_stw((uint16_t)(out + i * 2), m[i]); gsu_codes(FxMatrixFromAngles01_code[copy] + 15, 2); }
}

void abi_FxMatrixFromAngles01(void)
{
    uint16_t out = gsu.r[9];
    FxAnglesLeft k;
    uint32_t p;

    FxMatrixFromAngles01(out, &k, GSU_COPY(0));
    gsu_add(mul15(k.cx_cy, k.sz, GSU_COPY(0)), k.sx_sy, 0);      /* OV of the ADD R13 */
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

/*
 * @routine 01:8325 FxMatrixFromAnglesProgram
 * WHAT IT DOES: The Super FX program that builds the rotation matrix for
 * the angles at $0020-$0024 into $00E4 (FxMatrixFromAngles01).
 * HOW: R9 = $00E4, the call, then STOP.
 * Entry: started by the S-CPU. Exit: (at the STOP) as FxMatrixFromAngles01,
 * R9 = $00F4, R11 = $832D.
 */
void abi_FxMatrixFromAnglesProgram(void)
{
    gsu.r[9] = 0x00E4;
    gsu.r[11] = 0x832D;
    abi_FxMatrixFromAngles01();
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_half_sum_code[13][3] = {
    {{0x019896, 25}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0, 0}},
    {{0x019896, 10}, {0x0198AF, 15}, {0, 0}},
    {{0x019897, 6}, {0x0198BE, 16}, {0, 0}},
    {{0x019853, 18}, {0, 0}, {0, 0}},
    {{0x019853, 3}, {0x019865, 15}, {0, 0}},
    {{0x01983B, 17}, {0x019874, 16}, {0, 0}},
    {{0x019896, 1}, {0x01989D, 18}, {0, 0}},
    {{0x019896, 1}, {0x01989D, 3}, {0x0198AF, 15}},
    {{0x0198BE, 16}, {0, 0}, {0, 0}},
    {{0x01984C, 25}, {0, 0}, {0, 0}},
    {{0x01984C, 10}, {0x019865, 15}, {0, 0}},
    {{0x01983B, 24}, {0x019874, 16}, {0, 0}},
};

/* R0 = Ra + Rb (ADD), then Rd = R0 DIV2. */
static void fx_half_sum(int d, int a, int b, int copy)
{
    gsu_codes(fx_half_sum_code[copy], 3);
    gsu.r[0] = gsu_add(gsu.r[a], gsu.r[b], 0);
    gsu.r[d] = gsu_div2(gsu.r[0]);
}

/*
 * @routine 01:981A FxSegmentCrossZ
 * WHAT IT DOES: Finds where the segment from (R1, R2, R3) to (R4, R5, R6)
 * crosses z = 0: R0 = 0 when both ends have z >= 0, $FFFF when both are
 * negative; otherwise R0 = 1 with the end whose z was negative replaced by
 * the crossing point (the other end kept in its registers).
 * HOW: The z >= 0 end is saved at $9A/$9C/$9E (SMS) and put back at the
 * end (LMS); the ends are brought to the midpoint (ADD, DIV2) toward the
 * crossing until the midpoint's z is 0. When the first end is the one
 * with z >= 0 the ends are swapped while searching.
 * Entry: R1-R6 = the ends; R11 = return address. Exit: as above; R1-R6 as
 * the search left them; the last RAM address $9E; S, Z from the last
 * DIV2 (or the end tests).
 */
void abi_FxSegmentCrossZ(void)
{
    int i;

    gsu_moves(gsu.r[3]);
    if (!gsu.s) {
        gsu_code(0x019820, 5); gsu_code(0x019831, 4);
        gsu_moves(gsu.r[6]);
        if (!gsu.s) {
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            return;
        }
        gsu_stw(0x009A, gsu.r[1]);
        gsu_code(0x019835, 3);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_code(0x019838, 3);
        gsu_stw(0x009E, gsu.r[3]);
        for (i = 1; i <= 3; i++) {
            gsu.r[0] = gsu.r[i];
            gsu.r[i] = gsu.r[i + 3];
            gsu.r[i + 3] = gsu.r[0];
        }
        for (;;) {
            gsu.r[0] = gsu_div2(gsu_add(gsu.r[3], gsu.r[6], 0));
            if (gsu.z) {
                gsu.r[6] = gsu.r[0];
                fx_half_sum(4, 1, 4, GSU_COPY(6));
                fx_half_sum(5, 2, 5, GSU_COPY(1));
                gsu.r[1] = gsu_ldw(0x009A);
                gsu_code(0x019884, 3);
                gsu.r[2] = gsu_ldw(0x009C);
                gsu_code(0x019887, 3);
                gsu.r[3] = gsu_ldw(0x009E);
                gsu.r[0] = 0x0001;
                gsu_code(0x01988A, 3);
                return;
            }
            if (gsu.s) {
                gsu.r[3] = gsu.r[0];
                fx_half_sum(1, 1, 4, GSU_COPY(5));
                fx_half_sum(2, 2, 5, GSU_COPY(1));
            } else {
                gsu.r[6] = gsu.r[0];
                fx_half_sum(4, 1, 4, GSU_COPY(4));
                fx_half_sum(5, 2, 5, GSU_COPY(1));
            }
        }
    } else { gsu_code(0x019828, 5); gsu_code(0x01988D, 4); }
    gsu_moves(gsu.r[6]);
    if (gsu.s) {
        gsu.r[0] = 0xFFFF;
        return;
    }
    gsu_stw(0x009A, gsu.r[4]);
    gsu_code(0x019891, 3);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_code(0x019894, 3);
    gsu_stw(0x009E, gsu.r[6]);
    for (;;) {
        gsu_code(0x019897, 6);
        gsu.r[0] = gsu_div2(gsu_add(gsu.r[3], gsu.r[6], 0));
        if (gsu.z) {
            gsu.r[3] = gsu.r[0];
            fx_half_sum(1, 1, 4, GSU_COPY(3));
            fx_half_sum(2, 2, 5, GSU_COPY(1));
            gsu.r[4] = gsu_ldw(0x009A);
            gsu_code(0x0198CE, 3);
            gsu.r[5] = gsu_ldw(0x009C);
            gsu_code(0x0198D1, 3);
            gsu.r[6] = gsu_ldw(0x009E);
            gsu.r[0] = 0x0001;
            gsu_code(0x0198D4, 3);
            return;
        }
        if (gsu.s) {
            gsu.r[3] = gsu.r[0];
            fx_half_sum(1, 1, 4, GSU_COPY(2));
            fx_half_sum(2, 2, 5, GSU_COPY(1));
        } else {
            gsu.r[6] = gsu.r[0];
            fx_half_sum(4, 1, 4, GSU_COPY(0));
            fx_half_sum(5, 2, 5, GSU_COPY(1));
        }
    }
}

/* R0 = a * R6 (FMULT) doubled with the carry (ADC R0), then doubled
   again `more` times (ADD R0). */
static uint16_t fx_scaled(uint16_t a, int more)
{
    uint16_t v = gsu_fmult(a, gsu.r[6]);

    v = gsu_add(v, v, gsu.cy);
    while (more--)
        v = gsu_add(v, v, 0);
    return v;
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_clip_search_code[6][6] = {
    {{0x01C7F8, 19}, {0x01C80B, 3}, {0x01C80E, 3}, {0x01C7F7, 1}, {0x01C80E, 25}, {0x01C827, 25}},
    {{0x01C8AB, 19}, {0x01C8BE, 3}, {0x01C8C1, 3}, {0x01C8AA, 1}, {0x01C8C1, 25}, {0x01C8DA, 25}},
    {{0x01C5DF, 20}, {0x01C5F3, 3}, {0, 0}, {0x01C5DE, 1}, {0x01C5F6, 25}, {0x01C60F, 25}},
    {{0x01C52B, 20}, {0x01C53F, 3}, {0, 0}, {0x01C52A, 1}, {0x01C542, 25}, {0x01C55B, 25}},
    {{0x01C52B, 20}, {0x01C53F, 3}, {0x01C542, 3}, {0x01C52A, 1}, {0x01C542, 25}, {0x01C55B, 25}},
    {{0x01C5DF, 20}, {0x01C5F3, 3}, {0x01C5F6, 3}, {0x01C5DE, 1}, {0x01C5F6, 25}, {0x01C60F, 25}},
};

/* The halving search of the plane clips: the midpoint (R14 on the axis,
   R11 = z) tested against the plane; the end on the midpoint's side moves
   to it, with its attributes (R8/R12, R9/R13). At most 12 steps (R10,
   counted in the BMI's delay slot). Then the far end (the second when
   keep_first) ends at the midpoint. ax = 1 (x) or 2 (y). */
static void fx_clip_search(int ax, int more, int keep_first, int copy)
{
    int o = 3 - ax, neg;

    for (;;) {
        gsu_codes(fx_clip_search_code[copy], 1);
        gsu.r[0] = gsu_add(gsu.r[ax], gsu.r[ax + 3], 0);
        gsu.r[14] = gsu_div2(gsu.r[0]);
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[7], 0);
        gsu.r[11] = gsu_div2(gsu.r[0]);
        gsu.r[0] = fx_scaled(gsu.r[14], more);
        gsu.r[0] = gsu_sub(gsu.r[11], gsu.r[0], 0);
        if (gsu.z)
            break; else { gsu_codes(fx_clip_search_code[copy] + 1, 1); }
        neg = gsu.s;
        gsu.r[10] = gsu_dec(gsu.r[10]);
        if (gsu.z)
            { gsu_codes(fx_clip_search_code[copy] + 2, 1); break; } else { gsu_codes(fx_clip_search_code[copy] + 3, 1); }
        if (!neg) {
            gsu_codes(fx_clip_search_code[copy] + 4, 1);
            gsu.r[7] = gsu.r[11];
            fx_half_sum(o + 3, o, o + 3, GSU_COPY(1));
            gsu.r[ax + 3] = gsu.r[14];
            fx_half_sum(12, 8, 12, GSU_COPY(1));
            fx_half_sum(13, 9, 13, GSU_COPY(1));
        } else {
            gsu_codes(fx_clip_search_code[copy] + 5, 1);
            gsu.r[3] = gsu.r[11];
            fx_half_sum(o, o, o + 3, GSU_COPY(1));
            gsu.r[ax] = gsu.r[14];
            fx_half_sum(8, 8, 12, GSU_COPY(1));
            fx_half_sum(9, 9, 13, GSU_COPY(1));
        }
    }
    if (keep_first) {
        gsu.r[7] = gsu.r[11];
        fx_half_sum(o + 3, o, o + 3, GSU_COPY(1));
        gsu.r[ax + 3] = gsu.r[14];
        fx_half_sum(12, 8, 12, GSU_COPY(1));
        fx_half_sum(13, 9, 13, GSU_COPY(1));
    } else {
        gsu.r[3] = gsu.r[11];
        fx_half_sum(o, o, o + 3, GSU_COPY(1));
        gsu.r[ax] = gsu.r[14];
        fx_half_sum(8, 8, 12, GSU_COPY(1));
        fx_half_sum(9, 9, 13, GSU_COPY(1));
    }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_clip_enter_code[6][6] = {
    {{0, 0}, {0x01C7C7, 4}, {0x01C7CB, 4}, {0x01C7CF, 4}, {0x01C7D3, 5}, {0, 0}},
    {{0, 0}, {0x01C88C, 4}, {0x01C890, 4}, {0x01C894, 4}, {0x01C898, 5}, {0, 0}},
    {{0, 0}, {0x01C5C0, 4}, {0x01C5C4, 4}, {0x01C5C8, 4}, {0x01C5CC, 5}, {0, 0}},
    {{0, 0}, {0x01C4FA, 4}, {0x01C4FE, 4}, {0x01C502, 4}, {0x01C506, 5}, {0, 0}},
    {{0x01CCC6, 2}, {0x01CCC8, 4}, {0x01CCCC, 4}, {0x01CCD0, 4}, {0x01CCD4, 5}, {0x01CCD9, 23}},
    {{0x01CD8B, 2}, {0x01CD8D, 4}, {0x01CD91, 4}, {0x01CD95, 4}, {0x01CD99, 5}, {0x01CD9E, 5}},
};

/* Saves R8, R9, R12, R13 on the R10 stack and R10 at $A0; R10 = 12. */
static void fx_clip_enter(int copy)
{
    gsu_codes(fx_clip_enter_code[copy], 1);
    gsu_push(&gsu.r[10], gsu.r[8]);
    gsu_codes(fx_clip_enter_code[copy] + 1, 1);
    gsu_push(&gsu.r[10], gsu.r[9]);
    gsu_codes(fx_clip_enter_code[copy] + 2, 1);
    gsu_push(&gsu.r[10], gsu.r[12]);
    gsu_codes(fx_clip_enter_code[copy] + 3, 1);
    gsu_push(&gsu.r[10], gsu.r[13]);
    gsu_codes(fx_clip_enter_code[copy] + 4, 1);
    gsu_stw(0x00A0, gsu.r[10]);
    gsu.r[10] = 0x000C;
    gsu_codes(fx_clip_enter_code[copy] + 5, 1);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_clip_leave_code[4][6] = {
    {{0x01C866, 4}, {0x01C86A, 4}, {0x01C86E, 4}, {0x01C872, 4}, {0x01C876, 4}, {0x01C87A, 6}},
    {{0x01C919, 4}, {0x01C91D, 4}, {0x01C921, 4}, {0x01C925, 4}, {0x01C929, 4}, {0x01C92D, 6}},
    {{0x01C64E, 4}, {0x01C652, 4}, {0x01C656, 4}, {0x01C65A, 4}, {0x01C65E, 4}, {0x01C662, 6}},
    {{0x01C59A, 4}, {0x01C59E, 4}, {0x01C5A2, 4}, {0x01C5A6, 4}, {0x01C5AA, 4}, {0x01C5AE, 6}},
};

/* R10 from $A0, then R13, R12, R9, R8, R14 pulled; R0 = 1; return. */
static void fx_clip_leave(int copy)
{
    gsu.r[10] = gsu_ldw(0x00A0);
    gsu_codes(fx_clip_leave_code[copy], 1);
    gsu.r[13] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_code[copy] + 1, 1);
    gsu.r[12] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_code[copy] + 2, 1);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_code[copy] + 3, 1);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_code[copy] + 4, 1);
    gsu.r[14] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = 0x0001;
    gsu_codes(fx_clip_leave_code[copy] + 5, 1);
    gsu_pull(&gsu.r[10]);
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t fx_clip_search_from_fx_clip_plane[10] = {0, 1, 2, 3, 1, 0, 4, 5, 0, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t fx_clip_leave_from_fx_clip_plane[10] = {0, 1, 2, 3, 1, 0, 3, 2, 0, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t fx_clip_enter_from_fx_clip_plane[10] = {0, 1, 2, 3, 1, 0, 3, 2, 0, 0};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_clip_plane_code[10][39] = {
    {{0, 0}, {0x01C77E, 4}, {0x01C782, 21}, {0, 0}, {0, 0}, {0, 0}, {0x01C797, 5}, {0x01C79C, 5}, {0x01C7BB, 4}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0, 0}, {0, 0}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
    {{0, 0}, {0x01C77E, 4}, {0x01C782, 11}, {0, 0}, {0, 0}, {0x01C78D, 15}, {0, 0}, {0x01C79C, 5}, {0, 0}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0x01C7A1, 14}, {0x01C880, 4}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
    {{0, 0}, {0x01C4AE, 4}, {0x01C4B2, 12}, {0, 0}, {0, 0}, {0x01C4BE, 16}, {0, 0}, {0x01C4CE, 5}, {0, 0}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0x01C4D3, 15}, {0x01C5B4, 4}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C5B8, 3}, {0x01C5BB, 3}, {0x01C5BE, 2}, {0x01C5D1, 5}, {0x01C5D6, 3}, {0x01C5D9, 3}, {0x01C5DC, 3}, {0x01C628, 23}, {0x01C63F, 3}, {0x01C642, 3}, {0x01C645, 3}, {0x01C648, 3}, {0x01C64B, 3}},
    {{0, 0}, {0x01C4AE, 4}, {0x01C4B2, 23}, {0, 0}, {0, 0}, {0, 0}, {0x01C4C9, 5}, {0x01C4CE, 5}, {0x01C4EE, 4}, {0x01C4F2, 3}, {0x01C4F5, 3}, {0x01C4F8, 2}, {0x01C50B, 23}, {0x01C522, 3}, {0x01C525, 3}, {0x01C528, 3}, {0x01C574, 23}, {0x01C58B, 3}, {0x01C58E, 3}, {0x01C591, 3}, {0x01C594, 3}, {0x01C597, 3}, {0, 0}, {0, 0}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
    {{0, 0}, {0x01C77E, 4}, {0x01C782, 11}, {0x01C7A1, 14}, {0x01C880, 4}, {0, 0}, {0, 0}, {0x01C79C, 5}, {0, 0}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0, 0}, {0, 0}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
    {{0, 0}, {0x01C77E, 4}, {0x01C782, 21}, {0x01C7BB, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01C79C, 5}, {0, 0}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0, 0}, {0, 0}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
    {{0, 0}, {0x01C4AE, 4}, {0x01C4B2, 23}, {0x01C4EE, 4}, {0, 0}, {0, 0}, {0, 0}, {0x01C79C, 5}, {0, 0}, {0x01C4F2, 3}, {0x01C4F5, 3}, {0x01C4F8, 2}, {0x01C50B, 23}, {0x01C522, 3}, {0x01C525, 3}, {0x01C528, 3}, {0x01C574, 23}, {0x01C58B, 3}, {0x01C58E, 3}, {0x01C591, 3}, {0x01C594, 3}, {0x01C597, 3}, {0, 0}, {0, 0}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
    {{0, 0}, {0x01C4AE, 4}, {0x01C4B2, 12}, {0x01C4D3, 15}, {0x01C5B4, 4}, {0, 0}, {0, 0}, {0x01C79C, 5}, {0, 0}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0, 0}, {0, 0}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C5B8, 3}, {0x01C5BB, 3}, {0x01C5BE, 2}, {0x01C5D1, 5}, {0x01C5D6, 3}, {0x01C5D9, 3}, {0x01C5DC, 3}, {0x01C628, 23}, {0x01C63F, 3}, {0x01C642, 3}, {0x01C645, 3}, {0x01C648, 3}, {0x01C64B, 3}},
    {{0x01C4AB, 3}, {0x01C4AE, 4}, {0x01C4B2, 12}, {0x01C4D3, 11}, {0x01C4E2, 6}, {0, 0}, {0, 0}, {0x01C79C, 5}, {0, 0}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0, 0}, {0, 0}, {0x01C4E8, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
    {{0x01C77B, 3}, {0x01C77E, 4}, {0x01C782, 11}, {0x01C7A1, 10}, {0x01C7AF, 6}, {0, 0}, {0, 0}, {0x01C79C, 5}, {0, 0}, {0x01C7BF, 3}, {0x01C7C2, 3}, {0x01C7C5, 2}, {0x01C7D8, 23}, {0x01C7EF, 3}, {0x01C7F2, 3}, {0x01C7F5, 3}, {0x01C840, 23}, {0x01C857, 3}, {0x01C85A, 3}, {0x01C85D, 3}, {0x01C860, 3}, {0x01C863, 3}, {0, 0}, {0, 0}, {0x01C7B5, 6}, {0x0180E3, 1}, {0x01C884, 3}, {0x01C887, 3}, {0x01C88A, 2}, {0x01C89D, 5}, {0x01C8A2, 3}, {0x01C8A5, 3}, {0x01C8A8, 3}, {0x01C8F3, 23}, {0x01C90A, 3}, {0x01C90D, 3}, {0x01C910, 3}, {0x01C913, 3}, {0x01C916, 3}},
};

/*
 * @routine 01:C4AA FxClipYPlane
 * WHAT IT DOES: Clips the segment from (R1, R2, R3) to (R4, R5, R7)
 * against the plane z = y * R6 * 8 (FMULT, ADC, two ADDs): R0 = 0 when
 * both ends have z at or above it, $FFFF when both are below; otherwise
 * R0 = 1 with the end below replaced by the crossing, found by halving
 * (at most 12 steps), its attributes halved along with it: the first
 * end's at $8A/$8E, the second's at $8C/$90 (the moved end's written
 * back there).
 * HOW: The end kept is saved at $9A-$9E and put back; R8, R9, R12, R13
 * and the return address and R14 are kept on the R10 stack, R10 itself at
 * $A0.
 * Entry: R1-R7 as above, R6 = the slope; R10 = stack; R11 = return
 * address. Exit: as above; R8, R9, R12-R14 restored; S, Z from R10; the
 * last RAM address R10.
 */
static void fx_clip_plane(int ax, int more, int copy)
{
    gsu_codes(fx_clip_plane_code[copy], 1);
    int i;

    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_codes(fx_clip_plane_code[copy] + 1, 1);
    gsu_push(&gsu.r[10], gsu.r[14]);
    gsu_codes(fx_clip_plane_code[copy] + 2, 3);
    gsu.r[0] = fx_scaled(gsu.r[ax], more);
    gsu.r[0] = gsu_sub(gsu.r[3], gsu.r[0], 0);
    if (!gsu.s) {
        gsu_codes(fx_clip_plane_code[copy] + 5, 1);
        gsu.r[0] = fx_scaled(gsu.r[ax + 3], more);
        gsu.r[0] = gsu_sub(gsu.r[7], gsu.r[0], 0);
        if (!gsu.s) {
            gsu_codes(fx_clip_plane_code[copy] + 6, 1);
            gsu.r[14] = gsu_pull(&gsu.r[10]);
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_codes(fx_clip_plane_code[copy] + 7, 1);
            gsu_pull(&gsu.r[10]);
            return;
        } else { gsu_codes(fx_clip_plane_code[copy] + 8, 1); }
        gsu_stw(0x009A, gsu.r[1]);
        gsu_codes(fx_clip_plane_code[copy] + 9, 1);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_codes(fx_clip_plane_code[copy] + 10, 1);
        gsu_stw(0x009E, gsu.r[3]);
        gsu_codes(fx_clip_plane_code[copy] + 11, 1);
        fx_clip_enter(GSU_COPY(fx_clip_enter_from_fx_clip_plane[copy]));
        {
            gsu_codes(fx_clip_plane_code[copy] + 12, 1);
            static const int a[3] = {1, 2, 3}, b[3] = {4, 5, 7};

            for (i = 0; i < 3; i++) {
                gsu.r[0] = gsu.r[a[i]];
                gsu.r[a[i]] = gsu.r[b[i]];
                gsu.r[b[i]] = gsu.r[0];
            }
        }
        gsu.r[8] = gsu_ldw(0x008C);
        gsu_codes(fx_clip_plane_code[copy] + 13, 1);
        gsu.r[9] = gsu_ldw(0x0090);
        gsu_codes(fx_clip_plane_code[copy] + 14, 1);
        gsu.r[12] = gsu_ldw(0x008A);
        gsu_codes(fx_clip_plane_code[copy] + 15, 1);
        gsu.r[13] = gsu_ldw(0x008E);
        gsu_codes(fx_clip_plane_code[copy] + 16, 1);
        fx_clip_search(ax, more, 1, GSU_COPY(fx_clip_search_from_fx_clip_plane[copy]));
        gsu.r[1] = gsu_ldw(0x009A);
        gsu_codes(fx_clip_plane_code[copy] + 17, 1);
        gsu.r[2] = gsu_ldw(0x009C);
        gsu_codes(fx_clip_plane_code[copy] + 18, 1);
        gsu.r[3] = gsu_ldw(0x009E);
        gsu_codes(fx_clip_plane_code[copy] + 19, 1);
        gsu_stw(0x008C, gsu.r[12]);
        gsu_codes(fx_clip_plane_code[copy] + 20, 1);
        gsu_stw(0x0090, gsu.r[13]);
        gsu_codes(fx_clip_plane_code[copy] + 21, 1);
        fx_clip_leave(GSU_COPY(fx_clip_leave_from_fx_clip_plane[copy]));
        return;
    } else { gsu_codes(fx_clip_plane_code[copy] + 22, 2); }
    gsu.r[0] = fx_scaled(gsu.r[ax + 3], more);
    gsu.r[0] = gsu_sub(gsu.r[7], gsu.r[0], 0);
    if (gsu.s) {
        gsu.r[14] = gsu_pull(&gsu.r[10]);
        gsu.r[0] = 0xFFFF;
        gsu_codes(fx_clip_plane_code[copy] + 24, 1);
        gsu_pull(&gsu.r[10]);
        gsu_codes(fx_clip_plane_code[copy] + 25, 1);
        return;
    }
    gsu_stw(0x009A, gsu.r[4]);
    gsu_codes(fx_clip_plane_code[copy] + 26, 1);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_codes(fx_clip_plane_code[copy] + 27, 1);
    gsu_stw(0x009E, gsu.r[7]);
    gsu_codes(fx_clip_plane_code[copy] + 28, 1);
    fx_clip_enter(GSU_COPY(fx_clip_enter_from_fx_clip_plane[copy]));
    gsu_codes(fx_clip_plane_code[copy] + 29, 1);
    gsu.r[8] = gsu_ldw(0x008A);
    gsu_codes(fx_clip_plane_code[copy] + 30, 1);
    gsu.r[9] = gsu_ldw(0x008E);
    gsu_codes(fx_clip_plane_code[copy] + 31, 1);
    gsu.r[12] = gsu_ldw(0x008C);
    gsu_codes(fx_clip_plane_code[copy] + 32, 1);
    gsu.r[13] = gsu_ldw(0x0090);
    gsu_codes(fx_clip_plane_code[copy] + 33, 1);
    fx_clip_search(ax, more, 0, GSU_COPY(fx_clip_search_from_fx_clip_plane[copy]));
    gsu.r[4] = gsu_ldw(0x009A);
    gsu_codes(fx_clip_plane_code[copy] + 34, 1);
    gsu.r[5] = gsu_ldw(0x009C);
    gsu_codes(fx_clip_plane_code[copy] + 35, 1);
    gsu.r[7] = gsu_ldw(0x009E);
    gsu_codes(fx_clip_plane_code[copy] + 36, 1);
    gsu_stw(0x008A, gsu.r[8]);
    gsu_codes(fx_clip_plane_code[copy] + 37, 1);
    gsu_stw(0x008E, gsu.r[9]);
    gsu_codes(fx_clip_plane_code[copy] + 38, 1);
    fx_clip_leave(GSU_COPY(fx_clip_leave_from_fx_clip_plane[copy]));
}

void abi_FxClipYPlane(void)
{
    fx_clip_plane(2, 2, GSU_COPY(2));
}

/*
 * @routine 01:C77A FxClipXPlane
 * WHAT IT DOES: As FxClipYPlane against the plane z = x * R6 * 4 (FMULT,
 * ADC, one ADD), halving on x.
 * HOW: As FxClipYPlane.
 * Entry: as FxClipYPlane. Exit: as FxClipYPlane.
 */
void abi_FxClipXPlane(void)
{
    fx_clip_plane(1, 1, GSU_COPY(0));
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_near_test_code[8][5] = {
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01CC8B, 6}, {0x01CC93, 8}, {0x01CC9D, 3}, {0x01CCBC, 4}, {0, 0}},
    {{0x01CC8B, 6}, {0x01CC93, 3}, {0x01CCA6, 5}, {0x01CCAD, 7}, {0x01CD81, 4}},
    {{0x01CC8B, 11}, {0x01CCA6, 14}, {0x01CD81, 4}, {0, 0}, {0, 0}},
    {{0x01CC8B, 21}, {0x01CCBC, 4}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01CC8B, 11}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01CC96, 16}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x01CCA6, 10}, {0x01CCB4, 8}, {0, 0}, {0, 0}, {0, 0}},
};

static void fx_near_test(uint16_t z, int copy);

/* One plane clip driver's settings. */
typedef struct {
    uint16_t slope;       /* the slope's RAM address (LMS R6); 0 for the near plane */
    int ax, more, neg;    /* axis (1 x, 2 y), extra doublings, the plane's side */
    void (*clip)(void);   /* the segment clip */
    uint16_t ret_out, ret_in;   /* R11 at the two calls */
} FxPlaneDriver;

/* The output pointer a driver writes through (R9; the near plane's R8). */
static int fx_ro;

/* R0 = z - (or +) the scaled coordinate: SUB or ADD after the scaling. */
static void fx_plane_test(const FxPlaneDriver *d, int c, int z, int copy)
{
    if (!d->slope) {
        fx_near_test(gsu.r[z], GSU_COPY(0));
        return;
    }
    gsu.r[0] = fx_scaled(gsu.r[c], d->more);
    gsu.r[0] = d->neg ? gsu_add(gsu.r[z], gsu.r[0], 0) : gsu_sub(gsu.r[z], gsu.r[0], 0);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_emit_code[15][7] = {
    {{0x01CAC5, 5}, {0x01CACA, 1}, {0x01CACB, 5}, {0x01CAD0, 1}, {0x01CAD1, 5}, {0x01CAD6, 2}, {0, 0}},
    {{0x01CAFA, 5}, {0x01CAFF, 1}, {0x01CB00, 5}, {0x01CB05, 1}, {0x01CB06, 5}, {0x01CB0B, 2}, {0x01CB0D, 3}},
    {{0x01CB29, 5}, {0x01CB2E, 1}, {0x01CB2F, 5}, {0x01CB34, 1}, {0x01CB35, 5}, {0x01CB3A, 2}, {0, 0}},
    {{0x01CC4D, 5}, {0x01CC52, 1}, {0x01CC53, 5}, {0x01CC58, 1}, {0x01CC59, 5}, {0x01CC5E, 2}, {0, 0}},
    {{0x01CBE8, 5}, {0x01CBED, 1}, {0x01CBEE, 5}, {0x01CBF3, 1}, {0x01CBF4, 5}, {0x01CBF9, 2}, {0, 0}},
    {{0x01CC1E, 5}, {0x01CC23, 1}, {0x01CC24, 5}, {0x01CC29, 1}, {0x01CC2A, 5}, {0x01CC2F, 2}, {0x01CC31, 3}},
    {{0x01CA08, 5}, {0x01CA0D, 1}, {0x01CA0E, 5}, {0x01CA13, 1}, {0x01CA14, 5}, {0x01CA19, 2}, {0, 0}},
    {{0x01C9B6, 5}, {0x01C9BB, 1}, {0x01C9BC, 5}, {0x01C9C1, 1}, {0x01C9C2, 5}, {0x01C9C7, 2}, {0, 0}},
    {{0x01C9E2, 5}, {0x01C9E7, 1}, {0x01C9E8, 5}, {0x01C9ED, 1}, {0x01C9EE, 5}, {0x01C9F3, 2}, {0x01C9F5, 3}},
    {{0x01CEB6, 5}, {0x01CEBB, 1}, {0x01CEBC, 5}, {0x01CEC1, 1}, {0x01CEC2, 5}, {0x01CEC7, 2}, {0, 0}},
    {{0x01C6ED, 5}, {0x01C6F2, 1}, {0x01C6F3, 5}, {0x01C6F8, 1}, {0x01C6F9, 5}, {0x01C6FE, 2}, {0, 0}},
    {{0x01C71A, 5}, {0x01C71F, 1}, {0x01C720, 5}, {0x01C725, 1}, {0x01C726, 5}, {0x01C72B, 2}, {0x01C72D, 3}},
    {{0x01C740, 5}, {0x01C745, 1}, {0x01C746, 5}, {0x01C74B, 1}, {0x01C74C, 5}, {0x01C751, 2}, {0, 0}},
    {{0x01CEE2, 5}, {0x01CEE7, 1}, {0x01CEE8, 5}, {0x01CEED, 1}, {0x01CEEE, 5}, {0x01CEF3, 2}, {0x01CEF5, 3}},
    {{0x01CF08, 5}, {0x01CF0D, 1}, {0x01CF0E, 5}, {0x01CF13, 1}, {0x01CF14, 5}, {0x01CF19, 2}, {0, 0}},
};

/* A vertex (three registers and the two attribute words) appended at R9,
   the output count $40 counted up. */
static void fx_emit(int r1, int r2, int r3, uint16_t at1, uint16_t at2, int copy)
{
    const int r[3] = {r1, r2, r3};
    int i;

    for (i = 0; i < 3; i++) {
        gsu_stw(gsu.r[fx_ro], gsu.r[r[i]]);
        gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
        gsu_codes(fx_emit_code[copy], 1);
    }
    gsu.r[0] = gsu_ldw(at1);
    gsu_codes(fx_emit_code[copy] + 1, 1);
    gsu_stw(gsu.r[fx_ro], gsu.r[0]);
    gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    gsu_codes(fx_emit_code[copy] + 2, 1);
    gsu.r[0] = gsu_ldw(at2);
    gsu_codes(fx_emit_code[copy] + 3, 1);
    gsu_stw(gsu.r[fx_ro], gsu.r[0]);
    gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    gsu_codes(fx_emit_code[copy] + 4, 1);
    gsu.r[0] = gsu_inc(gsu_ldw(0x0040));
    gsu_codes(fx_emit_code[copy] + 5, 1);
    gsu_sbk(gsu.r[0]);
    gsu_codes(fx_emit_code[copy] + 6, 1);
}

static void fx_negate(int r)
{
    gsu.r[r] = gsu_inc(gsu_not(gsu.r[r]));
}

/* The clip call, the axis coordinates negated around it for the negative
   side (only the replaced end's afterwards). */
static void fx_plane_clip(const FxPlaneDriver *d, uint16_t ret, int fixed, int copy)
{
    if (d->neg) {
        fx_negate(d->ax);
        fx_negate(d->ax + 3);
    }
    gsu.r[11] = ret;
    d->clip();
    if (d->neg)
        fx_negate(fixed);
}

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t fx_plane_test_from_fx_plane_driver[11] = {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t fx_plane_clip_from_fx_plane_driver[11] = {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0};

/* cyccopy: the copy each of the caller's copies calls */
static const uint8_t fx_emit_from_fx_plane_driver[11] = {2, 3, 6, 14, 14, 12, 0, 0, 0, 0, 0};

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_plane_driver_code[11][59] = {
    {{0, 0}, {0, 0}, {0x01CA45, 4}, {0x01CA49, 5}, {0x01CA4E, 3}, {0x01CE3B, 7}, {0x01CA51, 4}, {0x01CA55, 3}, {0x01CA58, 3}, {0x01C681, 9}, {0x01C770, 5}, {0x01CA5B, 5}, {0x01CA64, 1}, {0x01CA65, 3}, {0x01CA70, 3}, {0x01CA73, 5}, {0x01CA78, 1}, {0x01CA79, 5}, {0x01CA7E, 9}, {0x01CA7E, 5}, {0x01CA86, 1}, {0x01CA87, 2}, {0x01CA91, 3}, {0x01CA94, 5}, {0x01CA99, 1}, {0x01CA9A, 3}, {0x01CA9D, 12}, {0, 0}, {0x01CABA, 3}, {0x01CAD8, 9}, {0, 0}, {0x01C77A, 1}, {0xC77B, 0}, {0x01C77B, 3}, {0x01CAE1, 11}, {0x01CAEC, 6}, {0x01CB3B, 1}, {0x01CAA9, 10}, {0, 0}, {0x01C77A, 1}, {0xC77B, 0}, {0x01C77B, 3}, {0x01CAB7, 3}, {0x01CB10, 11}, {0x01CB1B, 6}, {0x01CAB3, 4}, {0x01CB3B, 1}, {0x01CB3C, 3}, {0x01CB3F, 4}, {0x01CB43, 4}, {0x01CB47, 5}, {0x01CB50, 5}, {0x01CB55, 4}, {0, 0}, {0x01CB47, 9}, {0x01CB59, 5}, {0x01CB5E, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x01CB66, 4}, {0x01CB6A, 5}, {0x01CB6F, 3}, {0x01CE3B, 7}, {0x01CB72, 4}, {0x01CB76, 3}, {0x01CB79, 3}, {0x01C681, 9}, {0x01C770, 5}, {0x01CB7C, 5}, {0x01CB85, 1}, {0x01CB86, 3}, {0x01CB91, 3}, {0x01CB94, 5}, {0x01CB99, 1}, {0x01CB9A, 5}, {0x01CB9F, 9}, {0x01CB9F, 5}, {0x01CBA7, 1}, {0x01CBA8, 2}, {0x01CBB2, 3}, {0x01CBB5, 5}, {0x01CBBA, 1}, {0x01CBBB, 3}, {0x01CBBE, 13}, {0, 0}, {0x01CBDD, 3}, {0x01CBFB, 10}, {0, 0}, {0x01C4AA, 1}, {0xC4AB, 0}, {0x01C4AB, 3}, {0x01CC05, 11}, {0x01CC10, 6}, {0x01CC5F, 1}, {0x01CBCB, 11}, {0, 0}, {0x01C4AA, 1}, {0xC4AB, 0}, {0x01C4AB, 3}, {0x01CBDA, 3}, {0x01CC34, 11}, {0x01CC3F, 6}, {0x01CBD6, 4}, {0x01CC5F, 1}, {0x01CC60, 3}, {0x01CC63, 4}, {0x01CC67, 4}, {0x01CC6B, 5}, {0x01CC74, 5}, {0x01CC79, 4}, {0, 0}, {0x01CC6B, 9}, {0x01CC7D, 5}, {0x01CC82, 5}, {0, 0}, {0, 0}},
    {{0, 0}, {0, 0}, {0x01C936, 4}, {0x01C93A, 5}, {0x01C93F, 3}, {0x01CE3B, 7}, {0x01C942, 4}, {0x01C946, 3}, {0x01C949, 3}, {0x01C681, 9}, {0x01C770, 5}, {0x01C94C, 5}, {0x01C955, 1}, {0x01C956, 3}, {0x01C961, 3}, {0x01C964, 5}, {0x01C969, 1}, {0x01C96A, 5}, {0x01C96F, 9}, {0x01C96F, 5}, {0x01C977, 1}, {0x01C978, 2}, {0x01C982, 3}, {0x01C985, 5}, {0x01C98A, 1}, {0x01C98B, 3}, {0x01C98E, 12}, {0, 0}, {0x01C9AB, 3}, {0x01C9C9, 9}, {0, 0}, {0x01C77A, 1}, {0xC77B, 0}, {0x01C77B, 3}, {0x01C9D2, 5}, {0x01C9D7, 3}, {0x01CA1A, 1}, {0x01C99A, 10}, {0, 0}, {0x01C77A, 1}, {0xC77B, 0}, {0x01C77B, 3}, {0x01C9A8, 3}, {0x01C9F8, 5}, {0x01C9FD, 3}, {0x01C9A4, 4}, {0x01CA1A, 1}, {0x01CA1B, 3}, {0x01CA1E, 4}, {0x01CA22, 4}, {0x01CA26, 5}, {0x01CA2F, 5}, {0x01CA34, 4}, {0, 0}, {0x01CA26, 9}, {0x01CA38, 5}, {0x01CA3D, 5}, {0, 0}, {0, 0}},
    {{0x01C2B2, 6}, {0x01CE34, 3}, {0x01CE37, 4}, {0x01CA49, 5}, {0x01CA4E, 3}, {0x01CE3B, 7}, {0x01CE42, 4}, {0x01CE46, 3}, {0x01CE49, 3}, {0x01C681, 9}, {0x01C770, 5}, {0x01CE4C, 5}, {0x01CE55, 1}, {0x01CE56, 3}, {0x01CE61, 3}, {0x01CE64, 5}, {0x01CE69, 1}, {0x01CE6A, 5}, {0x01CE6F, 9}, {0x01CE6F, 5}, {0x01CE77, 1}, {0x01CE78, 2}, {0x01CE82, 3}, {0x01CE85, 5}, {0x01CE8A, 1}, {0x01CE8B, 3}, {0x01CE8E, 7}, {0x01CE97, 3}, {0x01CEAB, 3}, {0x01CEC9, 4}, {0x01CECF, 3}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CED2, 5}, {0x01CED7, 3}, {0x01CF1A, 1}, {0x01CE9A, 5}, {0x01CEA1, 3}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CEA8, 3}, {0x01CEF8, 5}, {0x01CEFD, 3}, {0x01CEA4, 4}, {0x01CF1A, 1}, {0x01CF1B, 3}, {0x01CF1E, 4}, {0x01CF22, 4}, {0x01CF26, 5}, {0x01CF2F, 5}, {0x01CF34, 4}, {0x01C2B8, 4}, {0x01CB47, 9}, {0x01CB59, 5}, {0x01CB5E, 5}, {0x0180E3, 1}, {0, 0}},
    {{0x01C31A, 6}, {0x01CE34, 3}, {0x01CE37, 4}, {0x01CA49, 5}, {0x01CA4E, 3}, {0x01CE3B, 7}, {0x01CE42, 4}, {0x01CE46, 3}, {0x01CE49, 3}, {0x01C681, 9}, {0x01C770, 5}, {0x01CE4C, 5}, {0x01CE55, 1}, {0x01CE56, 3}, {0x01CE61, 3}, {0x01CE64, 5}, {0x01CE69, 1}, {0x01CE6A, 5}, {0x01CE6F, 9}, {0x01CE6F, 5}, {0x01CE77, 1}, {0x01CE78, 2}, {0x01CE82, 3}, {0x01CE85, 5}, {0x01CE8A, 1}, {0x01CE8B, 3}, {0x01CE8E, 12}, {0, 0}, {0x01CEAB, 3}, {0x01CEC9, 9}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CED2, 5}, {0x01CED7, 3}, {0x01CF1A, 1}, {0x01CE9A, 10}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CEA8, 3}, {0x01CEF8, 5}, {0x01CEFD, 3}, {0x01CEA4, 4}, {0x01CF1A, 1}, {0x01CF1B, 3}, {0x01CF1E, 4}, {0x01CF22, 4}, {0x01CF26, 5}, {0x01CF2F, 5}, {0x01CF34, 4}, {0x01C320, 9}, {0x01CF26, 9}, {0x01CF38, 5}, {0x01CF3D, 5}, {0x01C320, 5}, {0x01C334, 1}},
    {{0, 0}, {0, 0}, {0x01C66B, 4}, {0x01C66F, 5}, {0x01C674, 3}, {0x01CE3B, 7}, {0x01C677, 4}, {0x01C67B, 3}, {0x01C67E, 3}, {0x01C681, 9}, {0x01C770, 5}, {0x01C681, 5}, {0x01C68A, 1}, {0x01C68B, 3}, {0x01C696, 3}, {0x01C699, 5}, {0x01C69E, 1}, {0x01C69F, 5}, {0x01C6A4, 9}, {0x01C6A4, 5}, {0x01C6AC, 1}, {0x01C6AD, 2}, {0x01C6B7, 3}, {0x01C6BA, 5}, {0x01C6BF, 1}, {0x01C6C0, 3}, {0x01C6C3, 13}, {0, 0}, {0x01C6E2, 3}, {0x01C700, 10}, {0, 0}, {0x01C4AA, 1}, {0xC4AB, 0}, {0x01C4AB, 3}, {0x01C70A, 5}, {0x01C70F, 3}, {0x01C752, 1}, {0x01C6D0, 11}, {0, 0}, {0x01C4AA, 1}, {0xC4AB, 0}, {0x01C4AB, 3}, {0x01C6DF, 3}, {0x01C730, 5}, {0x01C735, 3}, {0x01C6DB, 4}, {0x01C752, 1}, {0x01C753, 3}, {0x01C756, 4}, {0x01C75A, 4}, {0x01C75E, 5}, {0x01C767, 5}, {0x01C76C, 4}, {0, 0}, {0x01C75E, 9}, {0x01C770, 5}, {0x01C775, 5}, {0, 0}, {0, 0}},
    {{0x01C669, 2}, {0, 0}, {0x01C66B, 4}, {0x01C66F, 5}, {0x01C674, 3}, {0x01CE3B, 7}, {0x01C677, 4}, {0x01C67B, 3}, {0x01C67E, 3}, {0x01C681, 9}, {0x01C770, 5}, {0x01CE4C, 5}, {0x01CE55, 1}, {0x01CE56, 3}, {0x01CE61, 3}, {0x01CE64, 5}, {0x01CE69, 1}, {0x01CE6A, 5}, {0x01CE6F, 9}, {0x01CE6F, 5}, {0x01CE77, 1}, {0x01CE78, 2}, {0x01CE82, 3}, {0x01CE85, 5}, {0x01CE8A, 1}, {0x01CE8B, 3}, {0x01CA9D, 12}, {0, 0}, {0x01CEAB, 3}, {0x01CAD8, 9}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CED2, 5}, {0x01CED7, 3}, {0x01CF1A, 1}, {0x01CAA9, 10}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CEA8, 3}, {0x01CEF8, 5}, {0x01CEFD, 3}, {0x01CEA4, 4}, {0x01CF1A, 1}, {0x01CF1B, 3}, {0x01CF1E, 4}, {0x01CF22, 4}, {0x01CF26, 5}, {0x01CF2F, 5}, {0x01CF34, 4}, {0, 0}, {0x01CB47, 9}, {0x01CB59, 5}, {0x01C775, 5}, {0x0180E3, 1}, {0, 0}},
    {{0x01C934, 2}, {0, 0}, {0x01C936, 4}, {0x01C93A, 5}, {0x01C93F, 3}, {0x01CE3B, 7}, {0x01C942, 4}, {0x01C946, 3}, {0x01C949, 3}, {0x01C94C, 9}, {0x01CA38, 5}, {0x01CE4C, 5}, {0x01CE55, 1}, {0x01CE56, 3}, {0x01CE61, 3}, {0x01CE64, 5}, {0x01CE69, 1}, {0x01CE6A, 5}, {0x01CE6F, 9}, {0x01CE6F, 5}, {0x01CE77, 1}, {0x01CE78, 2}, {0x01CE82, 3}, {0x01CE85, 5}, {0x01CE8A, 1}, {0x01CE8B, 3}, {0x01CA9D, 12}, {0, 0}, {0x01CEAB, 3}, {0x01CAD8, 9}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CED2, 5}, {0x01CED7, 3}, {0x01CF1A, 1}, {0x01CAA9, 10}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CEA8, 3}, {0x01CEF8, 5}, {0x01CEFD, 3}, {0x01CEA4, 4}, {0x01CF1A, 1}, {0x01CF1B, 3}, {0x01CF1E, 4}, {0x01CF22, 4}, {0x01CF26, 5}, {0x01CF2F, 5}, {0x01CF34, 4}, {0, 0}, {0x01CB47, 9}, {0x01CB59, 5}, {0x01CA3D, 5}, {0x0180E3, 1}, {0, 0}},
    {{0x01CA43, 2}, {0, 0}, {0x01CA45, 4}, {0x01CA49, 5}, {0x01CA4E, 3}, {0x01CE3B, 7}, {0x01CA51, 4}, {0x01CA55, 3}, {0x01CA58, 3}, {0x01CA5B, 9}, {0x01CB59, 5}, {0x01CE4C, 5}, {0x01CE55, 1}, {0x01CE56, 3}, {0x01CE61, 3}, {0x01CE64, 5}, {0x01CE69, 1}, {0x01CE6A, 5}, {0x01CE6F, 9}, {0x01CE6F, 5}, {0x01CE77, 1}, {0x01CE78, 2}, {0x01CE82, 3}, {0x01CE85, 5}, {0x01CE8A, 1}, {0x01CE8B, 3}, {0x01CA9D, 12}, {0, 0}, {0x01CEAB, 3}, {0x01CAD8, 9}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CED2, 5}, {0x01CED7, 3}, {0x01CF1A, 1}, {0x01CAA9, 10}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CEA8, 3}, {0x01CEF8, 5}, {0x01CEFD, 3}, {0x01CEA4, 4}, {0x01CF1A, 1}, {0x01CF1B, 3}, {0x01CF1E, 4}, {0x01CF22, 4}, {0x01CF26, 5}, {0x01CF2F, 5}, {0x01CF34, 4}, {0, 0}, {0x01CB47, 9}, {0x01CB59, 5}, {0x01CB5E, 5}, {0x0180E3, 1}, {0, 0}},
    {{0x01CB64, 2}, {0, 0}, {0x01CB66, 4}, {0x01CB6A, 5}, {0x01CB6F, 3}, {0x01CE3B, 7}, {0x01CB72, 4}, {0x01CB76, 3}, {0x01CB79, 3}, {0x01CB7C, 9}, {0x01CC7D, 5}, {0x01CE4C, 5}, {0x01CE55, 1}, {0x01CE56, 3}, {0x01CE61, 3}, {0x01CE64, 5}, {0x01CE69, 1}, {0x01CE6A, 5}, {0x01CE6F, 9}, {0x01CE6F, 5}, {0x01CE77, 1}, {0x01CE78, 2}, {0x01CE82, 3}, {0x01CE85, 5}, {0x01CE8A, 1}, {0x01CE8B, 3}, {0x01CA9D, 12}, {0, 0}, {0x01CEAB, 3}, {0x01CAD8, 9}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CED2, 5}, {0x01CED7, 3}, {0x01CF1A, 1}, {0x01CAA9, 10}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CEA8, 3}, {0x01CEF8, 5}, {0x01CEFD, 3}, {0x01CEA4, 4}, {0x01CF1A, 1}, {0x01CF1B, 3}, {0x01CF1E, 4}, {0x01CF22, 4}, {0x01CF26, 5}, {0x01CF2F, 5}, {0x01CF34, 4}, {0, 0}, {0x01CB47, 9}, {0x01CB59, 5}, {0x01CC82, 5}, {0x0180E3, 1}, {0, 0}},
    {{0x01CE35, 2}, {0, 0}, {0x01CE37, 4}, {0x01CA49, 5}, {0x01CA4E, 3}, {0x01CE3B, 7}, {0x01CE42, 4}, {0x01CE46, 3}, {0x01CE49, 3}, {0x01CE4C, 9}, {0x01CF38, 5}, {0x01CE4C, 5}, {0x01CE55, 1}, {0x01CE56, 3}, {0x01CE61, 3}, {0x01CE64, 5}, {0x01CE69, 1}, {0x01CE6A, 5}, {0x01CE6F, 9}, {0x01CE6F, 5}, {0x01CE77, 1}, {0x01CE78, 2}, {0x01CE82, 3}, {0x01CE85, 5}, {0x01CE8A, 1}, {0x01CE8B, 3}, {0x01CA9D, 12}, {0, 0}, {0x01CEAB, 3}, {0x01CAD8, 9}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CED2, 5}, {0x01CED7, 3}, {0x01CF1A, 1}, {0x01CAA9, 10}, {0, 0}, {0x01CC87, 1}, {0xCC88, 0}, {0x01CC88, 3}, {0x01CEA8, 3}, {0x01CEF8, 5}, {0x01CEFD, 3}, {0x01CEA4, 4}, {0x01CF1A, 1}, {0x01CF1B, 3}, {0x01CF1E, 4}, {0x01CF22, 4}, {0x01CF26, 5}, {0x01CF2F, 5}, {0x01CF34, 4}, {0, 0}, {0x01CB47, 9}, {0x01CB59, 5}, {0x01CF3D, 5}, {0x0180E3, 1}, {0, 0}},
};

/* Clips the polygon at $60 (count $40, five words a vertex) against the
   plane into the list at $62: Sutherland-Hodgman, each edge from vertex
   to next (the last to the first). R0 = the new count, or 0 when fewer
   than 3 are left or there were only 2. */
static void fx_plane_driver(const FxPlaneDriver *d, int copy)
{
    gsu_codes(fx_plane_driver_code[copy], 2);
    int in1, i, rc, ri, ro;
    static const int reg1[3] = {1, 2, 3}, reg2[3] = {4, 5, 7};

    if (d->slope) {
        rc = 14; ri = 8; ro = 9;
    } else {
        rc = 9; ri = 6; ro = 8;
    }
    fx_ro = ro;
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_codes(fx_plane_driver_code[copy] + 2, 1);
    gsu_push(&gsu.r[10], gsu.r[14]);
    if (d->slope) {
        gsu_codes(fx_plane_driver_code[copy] + 3, 1);
        gsu.r[6] = gsu_ldw(d->slope);
        gsu_codes(fx_plane_driver_code[copy] + 4, 1);
        gsu.r[14] = gsu_ldw(0x0040);
    } else {
        gsu_codes(fx_plane_driver_code[copy] + 5, 1);
        gsu.r[14] = gsu.r[0];
        gsu.r[9] = gsu_ldw(0x0040);
    }
    gsu_codes(fx_plane_driver_code[copy] + 6, 1);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(0x0040, gsu.r[0]);
    gsu_codes(fx_plane_driver_code[copy] + 7, 1);
    gsu.r[ri] = gsu_ldw(0x0060);
    gsu_codes(fx_plane_driver_code[copy] + 8, 1);
    gsu.r[ro] = gsu_ldw(0x0062);
    gsu.r[0] = gsu_sub(gsu.r[rc], 0x0002, 0);
    if (gsu.z)
        { gsu_codes(fx_plane_driver_code[copy] + 9, 2); goto fail; } else { gsu_codes(fx_plane_driver_code[copy] + 11, 2); }
    do {
        gsu_codes(fx_plane_driver_code[copy] + 13, 1);
        for (i = 0; i < 3; i++) {
            gsu.r[reg1[i]] = gsu_ldw(gsu.r[ri]);
            gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
            gsu_codes(fx_plane_driver_code[copy] + 14, 1);
        }
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_codes(fx_plane_driver_code[copy] + 15, 1);
        gsu_stw(0x008A, gsu.r[0]);
        gsu_codes(fx_plane_driver_code[copy] + 16, 1);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_codes(fx_plane_driver_code[copy] + 17, 1);
        gsu_stw(0x008E, gsu.r[0]);
        gsu.r[0] = gsu_dec(gsu.r[rc]);
        if (gsu.z)
            { gsu_codes(fx_plane_driver_code[copy] + 18, 1); gsu.r[ri] = gsu_ldw(0x0060); } else { gsu_codes(fx_plane_driver_code[copy] + 19, 2); }
        gsu_codes(fx_plane_driver_code[copy] + 21, 1);
        for (i = 0; i < 3; i++) {
            gsu.r[reg2[i]] = gsu_ldw(gsu.r[ri]);
            gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
            gsu_codes(fx_plane_driver_code[copy] + 22, 1);
        }
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_codes(fx_plane_driver_code[copy] + 23, 1);
        gsu_stw(0x008C, gsu.r[0]);
        gsu_codes(fx_plane_driver_code[copy] + 24, 1);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu_codes(fx_plane_driver_code[copy] + 25, 1);
        gsu_stw(0x0090, gsu.r[0]);
        gsu.r[ri] = gsu_sub(gsu.r[ri], 0x0008, 0);
        gsu_codes(fx_plane_driver_code[copy] + 26, 2);
        fx_plane_test(d, d->ax, 3, GSU_COPY(fx_plane_test_from_fx_plane_driver[copy]));
        in1 = !gsu.s;
        if (in1) {
            gsu_codes(fx_plane_driver_code[copy] + 28, 1);
            fx_emit(1, 2, 3, 0x008A, 0x008E, GSU_COPY(fx_emit_from_fx_plane_driver[copy]));
            gsu_codes(fx_plane_driver_code[copy] + 29, 2);
            fx_plane_test(d, d->ax + 3, 7, GSU_COPY(fx_plane_test_from_fx_plane_driver[copy]));
            if (gsu.s) {
                gsu_codes(fx_plane_driver_code[copy] + 31, 4);
                fx_plane_clip(d, d->ret_out, d->ax + 3, GSU_COPY(fx_plane_clip_from_fx_plane_driver[copy]));
                gsu_codes(fx_plane_driver_code[copy] + 35, 1);
                fx_emit(4, 5, 7, 0x008C, 0x0090, GSU_COPY(fx_emit_from_fx_plane_driver[copy]));
            }
            gsu_codes(fx_plane_driver_code[copy] + 36, 1);
        } else {
            gsu_codes(fx_plane_driver_code[copy] + 37, 2);
            fx_plane_test(d, d->ax + 3, 7, GSU_COPY(fx_plane_test_from_fx_plane_driver[copy]));
            if (!gsu.s) {
                gsu_codes(fx_plane_driver_code[copy] + 39, 5);
                fx_plane_clip(d, d->ret_in, d->ax, GSU_COPY(fx_plane_clip_from_fx_plane_driver[copy]));
                gsu_codes(fx_plane_driver_code[copy] + 44, 1);
                fx_emit(1, 2, 3, 0x008A, 0x008E, GSU_COPY(fx_emit_from_fx_plane_driver[copy]));
            } else { gsu_codes(fx_plane_driver_code[copy] + 45, 2); }
        }
        gsu_codes(fx_plane_driver_code[copy] + 47, 1);
        gsu.r[rc] = gsu_dec(gsu.r[rc]);
    } while (gsu_codes_loop(fx_plane_driver_code[copy] + 48, 1, !gsu.z));
    gsu_codes(fx_plane_driver_code[copy] + 49, 1);
    gsu.r[0] = gsu_ldw(0x0040);
    gsu.r[1] = gsu_sub(gsu.r[0], 0x0003, 0);
    if (!gsu.s) {
        gsu_codes(fx_plane_driver_code[copy] + 50, 2);
        gsu.r[14] = gsu_pull(&gsu.r[10]);
        gsu_codes(fx_plane_driver_code[copy] + 52, 1);
        gsu_pull(&gsu.r[10]);
        gsu_codes(fx_plane_driver_code[copy] + 53, 1);
        return;
    } else { gsu_codes(fx_plane_driver_code[copy] + 54, 2); }
fail:
    gsu.r[14] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_plane_driver_code[copy] + 56, 1);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_pull(&gsu.r[10]);
    gsu_codes(fx_plane_driver_code[copy] + 57, 2);
}

/*
 * @routine 01:C668 FxPolygonClipTop
 * WHAT IT DOES: Clips the polygon at $60 ($40 vertices of five words:
 * x, y, z and two attributes) against the plane z = y * slope ($94) * 8
 * into the list at $62 (FxClipYPlane for each crossing edge): R0 = the
 * new vertex count ($40), or 0 when fewer than 3 are left or there were 2.
 * HOW: Each edge from a vertex to the next (the last to the first); a
 * vertex inside is kept, a crossing adds the clipped point. R14 and the
 * return address on the R10 stack.
 * Entry: R10 = stack; R11 = return address. Exit: R0 as above; R9 past
 * the output; R14 restored; S, Z from R10; the last RAM address R10.
 */
void abi_FxPolygonClipTop(void)
{
    static const FxPlaneDriver d = {0x0094, 2, 2, 0, abi_FxClipYPlane, 0xC70F, 0xC735};

    fx_plane_driver(&d, GSU_COPY(5));
}

/*
 * @routine 01:C933 FxPolygonClipRight
 * WHAT IT DOES: As FxPolygonClipTop against z = x * slope ($92) * 4
 * (FxClipXPlane).
 * HOW: As FxPolygonClipTop.
 * Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
 */
void abi_FxPolygonClipRight(void)
{
    static const FxPlaneDriver d = {0x0092, 1, 1, 0, abi_FxClipXPlane, 0xC9D7, 0xC9FD};

    fx_plane_driver(&d, GSU_COPY(2));
}

/*
 * @routine 01:CA42 FxPolygonClipLeft
 * WHAT IT DOES: As FxPolygonClipRight against z = -x * slope * 4: x is
 * negated around each FxClipXPlane (the clipped point's back afterwards).
 * HOW: As FxPolygonClipTop, testing z + the scaled x.
 * Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
 */
void abi_FxPolygonClipLeft(void)
{
    static const FxPlaneDriver d = {0x0092, 1, 1, 1, abi_FxClipXPlane, 0xCAEC, 0xCB1B};

    fx_plane_driver(&d, GSU_COPY(0));
}

/*
 * @routine 01:CB63 FxPolygonClipBottom
 * WHAT IT DOES: As FxPolygonClipTop against z = -y * slope * 8: y is
 * negated around each FxClipYPlane (the clipped point's back afterwards).
 * HOW: As FxPolygonClipTop, testing z + the scaled y.
 * Entry: as FxPolygonClipTop. Exit: as FxPolygonClipTop.
 */
void abi_FxPolygonClipBottom(void)
{
    static const FxPlaneDriver d = {0x0094, 2, 2, 1, abi_FxClipYPlane, 0xCC10, 0xCC3F};

    fx_plane_driver(&d, GSU_COPY(1));
}

/* MOVES R0, R14, then (in the BPL's delay slot) SUB z, and for a negative
   R14 ADD instead: R0 = R14 - z, or R14 + z. */
static void fx_near_test(uint16_t z, int copy)
{
    gsu_codes(fx_near_test_code[copy], 5);
    int pl;

    gsu.r[0] = gsu.r[14];
    gsu_moves(gsu.r[0]);
    pl = !gsu.s;
    gsu.r[0] = gsu_sub(gsu.r[0], z, 0);
    if (!pl)
        gsu.r[0] = gsu_add(gsu.r[14], z, 0);
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_near_search_code[4][7] = {
    {{0x01CD42, 26}, {0x01CCF9, 9}, {0x01CD04, 3}, {0x01CCF8, 1}, {0x01CD07, 3}, {0x01CD0A, 28}, {0x01CD26, 28}},
    {{0x01CDF5, 26}, {0x01CDAC, 9}, {0x01CDB7, 3}, {0x01CDAB, 1}, {0x01CDBA, 3}, {0x01CDBD, 28}, {0x01CDD9, 28}},
    {{0x01CDF5, 26}, {0x01CDAC, 14}, {0, 0}, {0x01CDAB, 1}, {0x01CDBA, 3}, {0x01CDBD, 28}, {0x01CDD9, 28}},
    {{0x01CD42, 26}, {0x01CCF9, 14}, {0, 0}, {0x01CCF8, 1}, {0x01CD07, 3}, {0x01CD0A, 28}, {0x01CD26, 28}},
};

/* The near clip's halving search, x and y both halved; then the far end
   (the second when keep_first) ends at the midpoint. */
static void fx_near_search(int keep_first, int copy)
{
    gsu_codes(fx_near_search_code[copy], 1);
    int neg;

    for (;;) {
        gsu_codes(fx_near_search_code[copy] + 1, 2);
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[7], 0);
        gsu.r[11] = gsu_div2(gsu.r[0]);
        fx_near_test(gsu.r[11], GSU_COPY(0));
        if (gsu.z)
            break; else { gsu_codes(fx_near_search_code[copy] + 3, 2); }
        neg = gsu.s;
        gsu.r[10] = gsu_dec(gsu.r[10]);
        if (gsu.z)
            break;
        if (!neg) {
            gsu_codes(fx_near_search_code[copy] + 5, 1);
            gsu.r[7] = gsu.r[11];
            fx_half_sum(4, 1, 4, GSU_COPY(1));
            fx_half_sum(5, 2, 5, GSU_COPY(1));
            fx_half_sum(12, 8, 12, GSU_COPY(1));
            fx_half_sum(13, 9, 13, GSU_COPY(1));
        } else {
            gsu_codes(fx_near_search_code[copy] + 6, 1);
            gsu.r[3] = gsu.r[11];
            fx_half_sum(1, 1, 4, GSU_COPY(1));
            fx_half_sum(2, 2, 5, GSU_COPY(1));
            fx_half_sum(8, 8, 12, GSU_COPY(1));
            fx_half_sum(9, 9, 13, GSU_COPY(1));
        }
    }
    if (keep_first) {
        gsu.r[7] = gsu.r[11];
        fx_half_sum(4, 1, 4, GSU_COPY(1));
        fx_half_sum(5, 2, 5, GSU_COPY(1));
        fx_half_sum(12, 8, 12, GSU_COPY(1));
        fx_half_sum(13, 9, 13, GSU_COPY(1));
    } else {
        gsu.r[3] = gsu.r[11];
        fx_half_sum(1, 1, 4, GSU_COPY(1));
        fx_half_sum(2, 2, 5, GSU_COPY(1));
        fx_half_sum(8, 8, 12, GSU_COPY(1));
        fx_half_sum(9, 9, 13, GSU_COPY(1));
    }
}

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode fx_near_leave_code[2][6] = {
    {{0x01CD68, 3}, {0x01CD6B, 4}, {0x01CD6F, 4}, {0x01CD73, 4}, {0x01CD77, 4}, {0x01CD7B, 6}},
    {{0x01CE1B, 3}, {0x01CE1E, 4}, {0x01CE22, 4}, {0x01CE26, 4}, {0x01CE2A, 4}, {0x01CE2E, 6}},
};

/* R10 from $A0, then R13, R12, R9, R8 pulled; R0 = 1; return. */
static void fx_near_leave(int copy)
{
    gsu_codes(fx_near_leave_code[copy], 1);
    gsu.r[10] = gsu_ldw(0x00A0);
    gsu_codes(fx_near_leave_code[copy] + 1, 1);
    gsu.r[13] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_near_leave_code[copy] + 2, 1);
    gsu.r[12] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_near_leave_code[copy] + 3, 1);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_near_leave_code[copy] + 4, 1);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = 0x0001;
    gsu_codes(fx_near_leave_code[copy] + 5, 1);
    gsu_pull(&gsu.r[10]);
}

/*
 * @routine 01:CC87 FxClipNear
 * WHAT IT DOES: Clips the segment from (R1, R2, R3) to (R4, R5, R7)
 * against the near plane at z = R14: an end is in front when R14 - z (or
 * R14 + z for a negative R14) is not negative. R0 = 0 when both are, $FFFF
 * when neither is; otherwise R0 = 1 with the end behind replaced by the
 * crossing (halving x, y and z, at most 12 steps) and its attributes
 * written back ($8A/$8E for the first end, $8C/$90 for the second).
 * HOW: As FxClipYPlane, with R14 the plane (not saved).
 * Entry: R1-R5, R7 the ends; R14 = the near z; R10 = stack; R11 = return
 * address. Exit: as above; R8, R9, R12, R13 restored; S, Z from R10; the
 * last RAM address R10.
 */
void abi_FxClipNear(void)
{
    gsu_code(0x01CC88, 3);
    int i;

    gsu_push(&gsu.r[10], gsu.r[11]);
    fx_near_test(gsu.r[3], GSU_COPY(1));
    if (!gsu.s) {
        fx_near_test(gsu.r[7], GSU_COPY(0));
        if (!gsu.s) {
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_pull(&gsu.r[10]);
            gsu_code(0x0180E3, 1);
            return;
        }
        gsu_stw(0x009A, gsu.r[1]);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_stw(0x009E, gsu.r[3]);
        fx_clip_enter(GSU_COPY(4));
        {
            static const int a[3] = {1, 2, 3}, b[3] = {4, 5, 7};

            for (i = 0; i < 3; i++) {
                gsu.r[0] = gsu.r[a[i]];
                gsu.r[a[i]] = gsu.r[b[i]];
                gsu.r[b[i]] = gsu.r[0];
            }
        }
        gsu.r[8] = gsu_ldw(0x008C);
        gsu.r[9] = gsu_ldw(0x0090);
        gsu.r[12] = gsu_ldw(0x008A);
        gsu.r[13] = gsu_ldw(0x008E);
        fx_near_search(1, GSU_COPY(0));
        gsu.r[1] = gsu_ldw(0x009A);
        gsu.r[2] = gsu_ldw(0x009C);
        gsu.r[3] = gsu_ldw(0x009E);
        gsu_stw(0x008C, gsu.r[12]);
        gsu_stw(0x0090, gsu.r[13]);
        fx_near_leave(GSU_COPY(0));
        return;
    }
    fx_near_test(gsu.r[7], GSU_COPY(0));
    if (gsu.s) {
        gsu.r[0] = 0xFFFF;
        gsu_pull(&gsu.r[10]);
        gsu_code(0x0180E3, 1);
        return;
    }
    gsu_stw(0x009A, gsu.r[4]);
    gsu_code(0x01CD85, 3);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_code(0x01CD88, 3);
    gsu_stw(0x009E, gsu.r[7]);
    fx_clip_enter(GSU_COPY(5));
    gsu.r[8] = gsu_ldw(0x008A);
    gsu_code(0x01CDA3, 3);
    gsu.r[9] = gsu_ldw(0x008E);
    gsu_code(0x01CDA6, 3);
    gsu.r[12] = gsu_ldw(0x008C);
    gsu_code(0x01CDA9, 3);
    gsu.r[13] = gsu_ldw(0x0090);
    fx_near_search(0, GSU_COPY(1));
    gsu.r[4] = gsu_ldw(0x009A);
    gsu_code(0x01CE0F, 3);
    gsu.r[5] = gsu_ldw(0x009C);
    gsu_code(0x01CE12, 3);
    gsu.r[7] = gsu_ldw(0x009E);
    gsu_code(0x01CE15, 3);
    gsu_stw(0x008A, gsu.r[8]);
    gsu_code(0x01CE18, 3);
    gsu_stw(0x008E, gsu.r[9]);
    fx_near_leave(GSU_COPY(1));
}

/*
 * @routine 01:CE34 FxPolygonClipNear
 * WHAT IT DOES: As FxPolygonClipTop against the near plane at z = R0
 * (FxClipNear): R0 = the new vertex count, or 0.
 * HOW: As FxPolygonClipTop, with R14 = the near z and R9 counting, R6
 * reading and R8 writing the vertices.
 * Entry: R0 = the near z; R10 = stack; R11 = return address. Exit: as
 * FxPolygonClipTop, R8 past the output.
 */
void abi_FxPolygonClipNear(void)
{
    static const FxPlaneDriver d = {0x0000, 1, 0, 0, abi_FxClipNear, 0xCED7, 0xCEFD};

    fx_plane_driver(&d, GSU_COPY(3));
}
