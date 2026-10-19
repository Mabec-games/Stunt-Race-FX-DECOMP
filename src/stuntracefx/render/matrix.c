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

    gsu_code(0x01972c, 4);
    e1 = gsu_ldw((uint16_t)(m + 2));
    gsu_code(0x019730, 4);
    e2 = gsu_ldw((uint16_t)(m + 4));
    gsu_code(0x019734, 4);
    e3 = gsu_ldw((uint16_t)(m + 6));
    gsu_code(0x019738, 2);
    gsu_stw((uint16_t)(m + 6), e1);
    gsu_code(0x01973a, 5);
    gsu_stw((uint16_t)(m + 2), e3);
    gsu_code(0x01973f, 5);
    e5 = gsu_ldw((uint16_t)(m + 10));
    gsu_code(0x019744, 4);
    e6 = gsu_ldw((uint16_t)(m + 12));
    gsu_code(0x019748, 2);
    gsu_stw((uint16_t)(m + 12), e2);
    gsu_code(0x01974a, 4);
    e7 = gsu_ldw((uint16_t)(m + 14));
    gsu_code(0x01974e, 2);
    gsu_stw((uint16_t)(m + 14), e5);
    gsu_code(0x019750, 5);
    gsu_stw((uint16_t)(m + 10), e7);
    gsu_code(0x019755, 5);
    gsu_stw((uint16_t)(m + 4), e6);
    *old5 = e5;
    *old7 = e7;
    *old6 = e6;
    
    gsu_code(0x01975a, 1);
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
/* One column of FxMatrixApply (the original has each in its own code):
   LMS R6, FROM R1, FMULT, TO R5, ROL; the same for R2, then WITH R5,
   ADD R0; for R3, then ADD R5; STW (R9); INC R9 twice. Returns R0; *part
   = R5, *e = R6. */
/* gsuwalk: the code of each copy of apply_column, read from the ROM */
static const GsuCode apply_column_fetch[3][4] = {
    {{0x01978c, 4}, {0x019790, 7}, {0x019797, 8}, {0x01979f, 5}},
    {{0x0197a4, 5}, {0x0197a9, 7}, {0x0197b0, 8}, {0x0197b8, 5}},
    {{0x0197bd, 5}, {0x0197c2, 7}, {0x0197c9, 8}, {0x0197d1, 5}},
};

static uint16_t apply_column(const uint16_t v[3], uint16_t col, uint16_t *out, uint16_t *part, uint16_t *e, int copy)
{
    uint16_t t;

    gsu_codes(apply_column_fetch[copy], 1);
    *e = gsu_ldw((uint16_t)(FX_MATRIX + col * 2));
    *part = gsu_rol(gsu_fmult(v[0], *e));
    gsu_codes(apply_column_fetch[copy] + 1, 1);
    *e = gsu_ldw((uint16_t)(FX_MATRIX + 6 + col * 2));
    t = gsu_rol(gsu_fmult(v[1], *e));
    *part = gsu_add(*part, t, 0);
    gsu_codes(apply_column_fetch[copy] + 2, 1);
    *e = gsu_ldw((uint16_t)(FX_MATRIX + 12 + col * 2));
    t = gsu_rol(gsu_fmult(v[2], *e));
    t = gsu_add(t, *part, 0);
    gsu_codes(apply_column_fetch[copy] + 3, 1);
    gsu_stw(*out, t);
    *out = gsu_inc(gsu_inc(*out));
    return t;
}

uint16_t FxMatrixApply(uint16_t x, uint16_t y, uint16_t z, uint16_t out, uint16_t *partial, uint16_t *m22, int copy)
{
    const uint16_t v[3] = {x, y, z};

    apply_column(v, 0, &out, partial, m22, GSU_COPY(0));
    apply_column(v, 1, &out, partial, m22, GSU_COPY(1));
    { uint16_t ret_ = apply_column(v, 2, &out, partial, m22, GSU_COPY(2)); 
    gsu_code(0x0197d6, 3);
    return ret_; }
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

    apply_exit(out, sum, part, e, GSU_COPY(1));
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
/* One vector of FxMatrixApplyVectors (each its own code in the
   original): LMS R1, R2, R3; FxMatrixApply. */
/* gsuwalk: the code of each copy of apply_vector, read from the ROM */
static const GsuCode apply_vector_fetch[3][6] = {
    {{0x01975c, 5}, {0x019761, 3}, {0x019764, 3}, {0x019767, 4}, {0x01978b, 1}, {0x978c, 0}},
    {{0x01976c, 3}, {0x01976f, 3}, {0x019772, 3}, {0x019775, 4}, {0x01978b, 1}, {0x978c, 0}},
    {{0x01977a, 3}, {0x01977d, 3}, {0x019780, 3}, {0x019783, 4}, {0x01978b, 1}, {0x978c, 0}},
};

static void apply_vector(uint16_t i, uint16_t out, uint16_t *x, uint16_t *y, uint16_t *z,
                         uint16_t *sum, uint16_t *partial, uint16_t *m22, int copy)
{
    uint16_t v = (uint16_t)(FX_VECTORS + i * 6);

    gsu_codes(apply_vector_fetch[copy], 1);
    *x = gsu_ldw(v);
    gsu_codes(apply_vector_fetch[copy] + 1, 1);
    *y = gsu_ldw((uint16_t)(v + 2));
    gsu_codes(apply_vector_fetch[copy] + 2, 1);
    *z = gsu_ldw((uint16_t)(v + 4));
    gsu_codes(apply_vector_fetch[copy] + 3, 3);
    *sum = FxMatrixApply(*x, *y, *z, (uint16_t)(out + i * 6), partial, m22, GSU_COPY(0));
}

void FxMatrixApplyVectors(uint16_t out, uint16_t *x, uint16_t *y, uint16_t *z,
                          uint16_t *sum, uint16_t *partial, uint16_t *m22, int copy)
{
    apply_vector(0, out, x, y, z, sum, partial, m22, GSU_COPY(0));
    
    gsu_code(0x01976b, 1);
    apply_vector(1, out, x, y, z, sum, partial, m22, GSU_COPY(1));
    
    gsu_code(0x019779, 1);
    apply_vector(2, out, x, y, z, sum, partial, m22, GSU_COPY(2));
    gsu_code(0x019787, 4);
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
/* gsuwalk: the code of each copy of FxMatrixRotateProgram, read from the ROM */
static const GsuCode FxMatrixRotateProgram_fetch[1][39] = {
    {{0x0182d9, 3}, {0x0182de, 5}, {0x0182e5, 6}, {0x0182f2, 3}, {0x0182f7, 5}, {0x0182fe, 6}, {0x01830b, 3}, {0x018310, 5}, {0x018317, 6}, {0x0182dc, 2}, {0, 0}, {0, 0}, {0x0182f5, 2}, {0, 0}, {0, 0}, {0x01830e, 2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0x0182eb, 2}, {0, 0}, {0, 0}, {0x018304, 2}, {0, 0}, {0, 0}, {0x01831d, 2}, {0, 0}, {0x0182e3, 2}, {0, 0}, {0, 0}, {0x0182fc, 2}, {0, 0}, {0, 0}, {0x018315, 2}, {0, 0}, {0x0182ed, 5}, {0x018306, 5}, {0x01831f, 5}},
};

uint16_t FxMatrixRotateProgram(uint16_t v[3], uint16_t *partial, uint16_t *m22)
{
    uint16_t sum = 0;
    int row, col;

    gsu_code(0x0182cf, 4);
    v[0] = gsu_ldw(FX_ROTATE_IN_X);
    gsu_code(0x0182d3, 3);
    v[1] = gsu_ldw(FX_ROTATE_IN_Y);
    gsu_code(0x0182d6, 3);
    v[2] = gsu_ldw(FX_ROTATE_IN_Z);
    for (row = 0; row < 3; row++) {
        for (col = 0; col < 3; col++) {
            uint16_t p;

            gsu_codes(FxMatrixRotateProgram_fetch[0] + (row) * 3 + col, 1);
            *m22 = gsu_ldw((uint16_t)(FX_MATRIX + row * 6 + col * 2));
            p = gsu_rol(gsu_fmult(v[col], *m22));     /* FROM Rn; FMULT; ROL */
            if (col == 0)
                { sum = p; gsu_codes(FxMatrixRotateProgram_fetch[0] + 9 + (row) * 3 + col, 1); }                              /* TO R5 */
            else if (col == 1)
                { sum = gsu_add(sum, p, 0); gsu_codes(FxMatrixRotateProgram_fetch[0] + 27 + (row) * 3 + col, 1); }             /* WITH R5; ADD R0 */
            else {
                *partial = sum;
                sum = gsu_add(p, sum, 0);             /* ADD R5 */
                gsu_codes(FxMatrixRotateProgram_fetch[0] + 18 + (row) * 3 + col, 1);
            }
        }
        gsu_codes(FxMatrixRotateProgram_fetch[0] + 36 + row, 1);
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
/* gsuwalk: the code of each copy of abi_FxMatrixRotateBackProgram, read from the ROM */
static const GsuCode abi_FxMatrixRotateBackProgram_fetch[1][12] = {
    {{0x018282, 3}, {0x01829b, 3}, {0x0182b4, 3}, {0x018285, 7}, {0x01829e, 7}, {0x0182b7, 7}, {0x01828c, 8}, {0x0182a5, 8}, {0x0182be, 8}, {0x018294, 7}, {0x0182ad, 7}, {0x0182c6, 7}},
};

void abi_FxMatrixRotateBackProgram(void)
{
    int out;

    gsu_code(0x018278, 4);
    gsu.r[1] = gsu_ldw(FX_ROTATE_IN_X);
    gsu_code(0x01827c, 3);
    gsu.r[2] = gsu_ldw(FX_ROTATE_IN_Y);
    gsu_code(0x01827f, 3);
    gsu.r[3] = gsu_ldw(FX_ROTATE_IN_Z);
    for (out = 0; out < 3; out++) {
        uint16_t m = (uint16_t)(FX_MATRIX + out * 2);

        gsu_codes(abi_FxMatrixRotateBackProgram_fetch[0] + out, 1);
        gsu.r[6] = gsu_ldw(m);
        gsu.r[5] = gsu_rol(gsu_fmult(gsu.r[1], gsu.r[6]));
        gsu_codes(abi_FxMatrixRotateBackProgram_fetch[0] + 3 + out, 1);
        gsu.r[6] = gsu_ldw((uint16_t)(m + 6));
        gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[2], gsu.r[6]));
        gsu.r[5] = gsu_add(gsu.r[5], gsu.r[0], 0);
        gsu_codes(abi_FxMatrixRotateBackProgram_fetch[0] + 6 + out, 1);
        gsu.r[6] = gsu_ldw((uint16_t)(m + 12));
        gsu.r[0] = gsu_rol(gsu_fmult(gsu.r[3], gsu.r[6]));
        gsu.r[0] = gsu_add(gsu.r[0], gsu.r[5], 0);
        gsu_codes(abi_FxMatrixRotateBackProgram_fetch[0] + 9 + out, 1);
        gsu_stw((uint16_t)(FX_ROTATE_OUT + out * 2), gsu.r[0]);
    }
}

#define FX_ANGLES 0x0020u            /* three angles: $0020, $0022, $0024 */
#define FX_SINE_TABLE 0x983Eu        /* ROM bank $00: 257 words, 1.15 */

/* (a * b) >> 15, signed: FMULT then ROL. */
static uint16_t mul15(uint16_t a, uint16_t b, int copy)
{
    (void)copy;
    return gsu_rol(gsu_fmult(a, b));     /* FMULT, ROL: their time and flags */
}

/* The sine of an angle (a full turn $10000) from the table, between its
   entries by the angle's low byte: GETB/GETBH through the ROM buffer. */
/* gsuwalk: the code of each copy of fx_sine, read from the ROM */
static const GsuCode fx_sine_fetch[6][8] = {
    {{0x018338, 8}, {0x018340, 2}, {0x018342, 1}, {0x018343, 3}, {0x018346, 1}, {0x018347, 1}, {0x018348, 1}, {0x018349, 2}},
    {{0x018358, 10}, {0x018362, 2}, {0x018364, 1}, {0x018365, 3}, {0x018368, 1}, {0x018369, 1}, {0x01836a, 1}, {0x01836b, 2}},
    {{0x01837a, 8}, {0x018382, 2}, {0x018384, 1}, {0x018385, 3}, {0x018388, 1}, {0x018389, 1}, {0x01838a, 1}, {0x01838b, 2}},
    {{0x01839a, 10}, {0x0183a4, 2}, {0x0183a6, 1}, {0x0183a7, 3}, {0x0183aa, 1}, {0x0183ab, 1}, {0x0183ac, 1}, {0x0183ad, 2}},
    {{0x0183bc, 8}, {0x0183c4, 2}, {0x0183c6, 1}, {0x0183c7, 3}, {0x0183ca, 1}, {0x0183cb, 1}, {0x0183cc, 1}, {0x0183cd, 2}},
    {{0x0183dc, 10}, {0x0183e6, 2}, {0x0183e8, 1}, {0x0183e9, 3}, {0x0183ec, 1}, {0x0183ed, 1}, {0x0183ee, 1}, {0x0183ef, 2}},
};

/* gsuwalk: the copy of mul15 each copy of fx_sine calls */
static const uint8_t mul15_in_fx_sine[6] = {0, 1, 2, 3, 4, 5};

static uint16_t fx_sine(uint16_t angle, int copy)
{
    uint16_t at = (uint16_t)(FX_SINE_TABLE + (angle >> 8) * 2);
    uint16_t a, b;

    gsu_codes(fx_sine_fetch[copy], 1);
    gsu_r14(at);
    gsu_codes(fx_sine_fetch[copy] + 1, 1);
    a = (gsu_getb());
    gsu_codes(fx_sine_fetch[copy] + 2, 1);
    gsu_r14(++at);
    gsu_codes(fx_sine_fetch[copy] + 3, 1);
    a = gsu_getbh(a);
    gsu_codes(fx_sine_fetch[copy] + 4, 1);
    gsu_r14(++at);
    gsu_codes(fx_sine_fetch[copy] + 5, 1);
    b = (gsu_getb());
    gsu_codes(fx_sine_fetch[copy] + 6, 1);
    gsu_r14(++at);
    gsu_codes(fx_sine_fetch[copy] + 7, 1);
    b = gsu_getbh(b);
    return (uint16_t)(a + mul15((uint16_t)((angle & 0xFF) << 7), (uint16_t)(b - a), GSU_COPY(mul15_in_fx_sine[copy])));
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
void FxMatrixFromAngles01(uint16_t out, FxAnglesLeft *k, int copy)
{
    uint16_t m[9];
    uint16_t a;

    gsu_code(0x018330, 3);
    gsu_ramb(0);
    gsu_code(0x018333, 2);
    gsu_romb(0);
    gsu_code(0x018335, 3);
    a = gsu_ldw(FX_ANGLES);
    k->sz = fx_sine(a, GSU_COPY(0));
    gsu_code(0x01834b, 13);
    gsu_r14(0x4000);
    k->cz = fx_sine((uint16_t)(a + 0x4000), GSU_COPY(1));
    gsu_code(0x01836d, 13);
    a = gsu_ldw((uint16_t)(FX_ANGLES + 2));
    k->sx = fx_sine(a, GSU_COPY(2));
    gsu_code(0x01838d, 13);
    gsu_r14(0x4000);
    k->cx = fx_sine((uint16_t)(a + 0x4000), GSU_COPY(3));
    gsu_code(0x0183af, 13);
    a = gsu_ldw((uint16_t)(FX_ANGLES + 4));
    k->sy = fx_sine(a, GSU_COPY(4));
    gsu_code(0x0183cf, 13);
    gsu_r14(0x4000);
    k->cy = fx_sine((uint16_t)(a + 0x4000), GSU_COPY(5));
    gsu_code(0x0183f1, 7);
    k->sx_cy = mul15(k->sx, k->cy, GSU_COPY(6));
    gsu_code(0x0183f8, 7);
    k->cx_cy = mul15(k->cx, k->cy, GSU_COPY(7));
    gsu_code(0x0183ff, 4);
    k->sx_sy = mul15(k->sx, k->sy, GSU_COPY(8));
    gsu_code(0x018403, 6);
    k->cx_sy = mul15(k->cx, k->sy, GSU_COPY(9));
    gsu_code(0x018409, 6);
    gsu_r14(k->cx_sy);
    /* each element stored as it is made (STW (R9), INC R9 twice) */
    m[0] = (uint16_t)(mul15(k->sx_sy, k->sz, GSU_COPY(10)) + k->cx_cy);
    gsu_code(0x01840f, 7);
    gsu_stw(out, m[0]);
    m[1] = (uint16_t)(mul15(k->sx_cy, k->sz, GSU_COPY(11)) - k->cx_sy);
    gsu_code(0x018416, 7);
    gsu_stw((uint16_t)(out + 2), m[1]);
    m[2] = mul15(k->sx, k->cz, GSU_COPY(12));
    gsu_code(0x01841d, 8);
    gsu_stw((uint16_t)(out + 4), m[2]);
    m[3] = mul15(k->sy, k->cz, GSU_COPY(13));
    gsu_code(0x018425, 6);
    gsu_stw((uint16_t)(out + 6), m[3]);
    m[4] = mul15(k->cy, k->cz, GSU_COPY(14));
    gsu_code(0x01842b, 6);
    gsu_stw((uint16_t)(out + 8), m[4]);
    m[5] = (uint16_t)-k->sz;
    gsu_code(0x018431, 6);
    gsu_stw((uint16_t)(out + 10), m[5]);
    m[6] = (uint16_t)(mul15(k->cx_sy, k->sz, GSU_COPY(15)) - k->sx_cy);
    gsu_code(0x018437, 9);
    gsu_stw((uint16_t)(out + 12), m[6]);
    m[7] = (uint16_t)(mul15(k->cx_cy, k->sz, GSU_COPY(16)) + k->sx_sy);
    gsu_code(0x018440, 7);
    gsu_stw((uint16_t)(out + 14), m[7]);
    m[8] = mul15(k->cx, k->cz, GSU_COPY(17));
    gsu_code(0x018447, 8);
    gsu_stw((uint16_t)(out + 16), m[8]);
    gsu_code(0x01844f, 1);
}

void abi_FxMatrixFromAngles01(void)
{
    uint16_t out = gsu.r[9];
    FxAnglesLeft k;
    uint32_t p;

    FxMatrixFromAngles01(out, &k, GSU_COPY(0));
    gsu_add((uint16_t)((int32_t)(int16_t)k.cx_cy * (int16_t)k.sz >> 15), k.sx_sy, 0);   /* OV of the ADD R13 (its FMULT timed above) */
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
    
    
    gsu_code(0x018326, 7); gsu_code(0x01832f, 1);
    abi_FxMatrixFromAngles01();
    gsu_code(0x01832d, 1);
}

/* R0 = Ra + Rb (ADD), then Rd = R0 DIV2. */
static void fx_half_sum(int d, int a, int b, int copy)
{
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
        
        gsu_moves(gsu.r[6]);
        if (!gsu.s) {
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_code(0x01981b, 13);
            return;
        } else { gsu_code(0x01981b, 10); gsu_code(0x019831, 1); }
        gsu_code(0x019832, 3);
        gsu_stw(0x009A, gsu.r[1]);
        gsu_code(0x019835, 3);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_code(0x019838, 3);
        gsu_stw(0x009E, gsu.r[3]);
        gsu_code(0x01983b, 22);
        for (i = 1; i <= 3; i++) {
            gsu.r[0] = gsu.r[i];
            gsu.r[i] = gsu.r[i + 3];
            gsu.r[i + 3] = gsu.r[0];
        }
        for (;;) {
            gsu.r[0] = gsu_div2(gsu_add(gsu.r[3], gsu.r[6], 0));
            if (gsu.z) {
                gsu.r[6] = gsu.r[0];
                
                
                gsu_code(0x019851, 2); gsu_code(0x019874, 1);
                fx_half_sum(4, 1, 4, GSU_COPY(0));
                fx_half_sum(5, 2, 5, GSU_COPY(0));
                gsu_code(0x019875, 15);
                gsu.r[1] = gsu_ldw(0x009A);
                gsu_code(0x019884, 3);
                gsu.r[2] = gsu_ldw(0x009C);
                gsu_code(0x019887, 3);
                gsu.r[3] = gsu_ldw(0x009E);
                gsu.r[0] = 0x0001;
                gsu_code(0x01988a, 3);
                return;
            } else { gsu_code(0x019851, 2); }
            if (gsu.s) {
                gsu.r[3] = gsu.r[0];
                
                
                gsu_code(0x019853, 3); gsu_code(0x019865, 1);
                fx_half_sum(1, 1, 4, GSU_COPY(1));
                fx_half_sum(2, 2, 5, GSU_COPY(1));
                
                
                gsu_code(0x019866, 14); gsu_code(0x01984c, 1);
            } else {
                gsu.r[6] = gsu.r[0];
                gsu_code(0x019853, 4);
                fx_half_sum(4, 1, 4, GSU_COPY(2));
                fx_half_sum(5, 2, 5, GSU_COPY(2));
                
                
                gsu_code(0x019857, 14); gsu_code(0x01984c, 1);
            }
            gsu_code(0x01984d, 4);
        }
    } else { gsu_code(0x01981b, 5); gsu_code(0x019828, 1); }
    gsu_moves(gsu.r[6]);
    if (gsu.s) {
        gsu.r[0] = 0xFFFF;
        gsu_code(0x019829, 8);
        return;
    } else { gsu_code(0x019829, 4); gsu_code(0x01988d, 1); }
    gsu_code(0x01988e, 3);
    gsu_stw(0x009A, gsu.r[4]);
    gsu_code(0x019891, 3);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_code(0x019894, 3);
    gsu_stw(0x009E, gsu.r[6]);
    for (;;) {
        gsu.r[0] = gsu_div2(gsu_add(gsu.r[3], gsu.r[6], 0));
        if (gsu.z) {
            gsu.r[3] = gsu.r[0];
            
            
            gsu_code(0x019897, 6); gsu_code(0x0198be, 1);
            fx_half_sum(1, 1, 4, GSU_COPY(3));
            fx_half_sum(2, 2, 5, GSU_COPY(3));
            gsu_code(0x0198bf, 15);
            gsu.r[4] = gsu_ldw(0x009A);
            gsu_code(0x0198ce, 3);
            gsu.r[5] = gsu_ldw(0x009C);
            gsu_code(0x0198d1, 3);
            gsu.r[6] = gsu_ldw(0x009E);
            gsu.r[0] = 0x0001;
            gsu_code(0x0198d4, 3);
            return;
        } else { gsu_code(0x019897, 6); }
        if (gsu.s) {
            gsu.r[3] = gsu.r[0];
            
            
            gsu_code(0x01989d, 3); gsu_code(0x0198af, 1);
            fx_half_sum(1, 1, 4, GSU_COPY(4));
            fx_half_sum(2, 2, 5, GSU_COPY(4));
            
            
            gsu_code(0x0198b0, 14); gsu_code(0x019896, 1);
        } else {
            gsu.r[6] = gsu.r[0];
            gsu_code(0x01989d, 4);
            fx_half_sum(4, 1, 4, GSU_COPY(5));
            fx_half_sum(5, 2, 5, GSU_COPY(5));
            
            
            gsu_code(0x0198a1, 14); gsu_code(0x019896, 1);
        }
    }
}

/* R0 = a * R6 (FMULT) doubled with the carry (ADC R0), then doubled
   again `more` times (ADD R0). */
/* gsuwalk: the code of each copy of fx_scaled, read from the ROM */
static const GsuCode fx_scaled_fetch[22][1] = {
    {{0x01c6c3, 11}},
    {{0x01c700, 8}},
    {{0x01c4b2, 10}},
    {{0x01c4bf, 8}},
    {{0x01c530, 13}},
    {{0x01c4d4, 8}},
    {{0x01c5e4, 13}},
    {{0x01c6d1, 8}},
    {{0x01cbbe, 11}},
    {{0x01cbfb, 8}},
    {{0x01cbcc, 8}},
    {{0x01c98e, 10}},
    {{0x01c9c9, 7}},
    {{0x01c782, 9}},
    {{0x01c78e, 7}},
    {{0x01c7fd, 12}},
    {{0x01c7a2, 7}},
    {{0x01c8b0, 12}},
    {{0x01c99b, 7}},
    {{0x01ca9d, 10}},
    {{0x01cad8, 7}},
    {{0x01caaa, 7}},
};

static uint16_t fx_scaled(uint16_t a, int more, int copy)
{
    uint16_t v = gsu_fmult(a, gsu.r[6]);

    v = gsu_add(v, v, gsu.cy);
    gsu_codes(fx_scaled_fetch[copy], 1);
    while (more--)
        v = gsu_add(v, v, 0);
    return v;
}

/* The halving search of the plane clips: the midpoint (R14 on the axis,
   R11 = z) tested against the plane; the end on the midpoint's side moves
   to it, with its attributes (R8/R12, R9/R13). At most 12 steps (R10,
   counted in the BMI's delay slot). Then the far end (the second when
   keep_first) ends at the midpoint. ax = 1 (x) or 2 (y). */
/* gsuwalk: the code of each copy of fx_clip_search, read from the ROM */
static const GsuCode fx_clip_search_fetch[4][18] = {
    {{0x01c52b, 5}, {0x01c53d, 2}, {0x01c53d, 2}, {0x01c574, 1}, {0x01c55f, 21}, {0x01c52a, 1}, {0x01c546, 21}, {0x01c52a, 1}, {0x01c53f, 6}, {0x01c53f, 6}, {0x01c574, 1}, {0x01c545, 1}, {0x01c53f, 3}, {0x01c55b, 3}, {0x01c53f, 3}, {0x01c55b, 3}, {0x01c574, 1}, {0x01c55e, 1}},
    {{0x01c5df, 5}, {0x01c5f1, 2}, {0x01c5f1, 2}, {0x01c628, 1}, {0x01c613, 21}, {0x01c5de, 1}, {0x01c5fa, 21}, {0x01c5de, 1}, {0x01c5f3, 6}, {0x01c5f3, 6}, {0x01c628, 1}, {0x01c5f9, 1}, {0x01c5f3, 3}, {0x01c60f, 3}, {0x01c5f3, 3}, {0x01c60f, 3}, {0x01c628, 1}, {0x01c612, 1}},
    {{0x01c7f8, 5}, {0x01c809, 2}, {0x01c809, 2}, {0x01c840, 1}, {0x01c82b, 21}, {0x01c7f7, 1}, {0x01c812, 21}, {0x01c7f7, 1}, {0x01c80b, 6}, {0x01c80b, 6}, {0x01c840, 1}, {0x01c811, 1}, {0x01c80b, 3}, {0x01c827, 3}, {0x01c80b, 3}, {0x01c827, 3}, {0x01c840, 1}, {0x01c82a, 1}},
    {{0x01c8ab, 5}, {0x01c8bc, 2}, {0x01c8bc, 2}, {0x01c8f3, 1}, {0x01c8de, 21}, {0x01c8aa, 1}, {0x01c8c5, 21}, {0x01c8aa, 1}, {0x01c8be, 6}, {0x01c8be, 6}, {0x01c8f3, 1}, {0x01c8c4, 1}, {0x01c8be, 3}, {0x01c8da, 3}, {0x01c8be, 3}, {0x01c8da, 3}, {0x01c8f3, 1}, {0x01c8dd, 1}},
};

/* gsuwalk: the copy of fx_scaled each copy of fx_clip_search calls */
static const uint8_t fx_scaled_in_fx_clip_search[4] = {4, 6, 15, 17};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search[4] = {12, 15, 18, 21};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_2[4] = {12, 15, 18, 21};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_3[4] = {12, 15, 18, 21};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_4[4] = {13, 16, 19, 22};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_5[4] = {13, 16, 19, 22};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_6[4] = {13, 16, 19, 22};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_7[4] = {14, 0, 20, 0};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_8[4] = {14, 0, 20, 0};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_9[4] = {14, 0, 20, 0};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_10[4] = {0, 17, 0, 23};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_11[4] = {0, 17, 0, 23};

/* gsuwalk: the copy of fx_half_sum each copy of fx_clip_search calls */
static const uint8_t fx_half_sum_in_fx_clip_search_12[4] = {0, 17, 0, 23};

static void fx_clip_search(int ax, int more, int keep_first, int copy)
{
    int o = 3 - ax, neg;

    for (;;) {
        gsu.r[0] = gsu_add(gsu.r[ax], gsu.r[ax + 3], 0);
        gsu_codes(fx_clip_search_fetch[copy], 1);
        gsu_r14(gsu_div2(gsu.r[0]));
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[7], 0);
        gsu.r[11] = gsu_div2(gsu.r[0]);
        gsu.r[0] = fx_scaled(gsu.r[14], more, GSU_COPY(fx_scaled_in_fx_clip_search[copy]));
        gsu.r[0] = gsu_sub(gsu.r[11], gsu.r[0], 0);
        if (gsu.z)
            { 
            gsu_codes(fx_clip_search_fetch[copy] + 2, 2);
            break; } else { gsu_codes(fx_clip_search_fetch[copy] + 1, 1); }
        neg = gsu.s;
        gsu.r[10] = gsu_dec(gsu.r[10]);         /* BMI's delay slot */
        if (!neg) {
            if (gsu.z)
                { 
                gsu_codes(fx_clip_search_fetch[copy] + 9, 2);
                break; } else { gsu_codes(fx_clip_search_fetch[copy] + 8, 1); }
            gsu.r[7] = gsu.r[11];
            gsu_codes(fx_clip_search_fetch[copy] + 11, 1);
            fx_half_sum(o + 3, o, o + 3, GSU_COPY(fx_half_sum_in_fx_clip_search[copy]));
            gsu.r[ax + 3] = gsu.r[14];
            fx_half_sum(12, 8, 12, GSU_COPY(fx_half_sum_in_fx_clip_search_2[copy]));
            fx_half_sum(13, 9, 13, GSU_COPY(fx_half_sum_in_fx_clip_search_3[copy]));
            gsu_codes(fx_clip_search_fetch[copy] + 6, 2);
        } else {
            if (gsu.z)
                { 
                gsu_codes(fx_clip_search_fetch[copy] + 14, 3);
                break; } else { gsu_codes(fx_clip_search_fetch[copy] + 12, 2); }
            gsu.r[3] = gsu.r[11];
            gsu_codes(fx_clip_search_fetch[copy] + 17, 1);
            fx_half_sum(o, o, o + 3, GSU_COPY(fx_half_sum_in_fx_clip_search_4[copy]));
            gsu.r[ax] = gsu.r[14];
            fx_half_sum(8, 8, 12, GSU_COPY(fx_half_sum_in_fx_clip_search_5[copy]));
            fx_half_sum(9, 9, 13, GSU_COPY(fx_half_sum_in_fx_clip_search_6[copy]));
            gsu_codes(fx_clip_search_fetch[copy] + 4, 2);
        }
    }
    if (keep_first) {
        gsu.r[7] = gsu.r[11];
        fx_half_sum(o + 3, o, o + 3, GSU_COPY(fx_half_sum_in_fx_clip_search_7[copy]));
        gsu.r[ax + 3] = gsu.r[14];
        fx_half_sum(12, 8, 12, GSU_COPY(fx_half_sum_in_fx_clip_search_8[copy]));
        fx_half_sum(13, 9, 13, GSU_COPY(fx_half_sum_in_fx_clip_search_9[copy]));
    } else {
        gsu.r[3] = gsu.r[11];
        fx_half_sum(o, o, o + 3, GSU_COPY(fx_half_sum_in_fx_clip_search_10[copy]));
        gsu.r[ax] = gsu.r[14];
        fx_half_sum(8, 8, 12, GSU_COPY(fx_half_sum_in_fx_clip_search_11[copy]));
        fx_half_sum(9, 9, 13, GSU_COPY(fx_half_sum_in_fx_clip_search_12[copy]));
    }
}

/* Saves R8, R9, R12, R13 on the R10 stack and R10 at $A0; R10 = 12. */
/* gsuwalk: the code of each copy of fx_clip_enter, read from the ROM */
static const GsuCode fx_clip_enter_fetch[6][5] = {
    {{0x01ccc6, 2}, {0x01ccc8, 4}, {0x01cccc, 4}, {0x01ccd0, 4}, {0x01ccd4, 5}},
    {{0x01cd8b, 2}, {0x01cd8d, 4}, {0x01cd91, 4}, {0x01cd95, 4}, {0x01cd99, 5}},
    {{0x01c4f8, 2}, {0x01c4fa, 4}, {0x01c4fe, 4}, {0x01c502, 4}, {0x01c506, 5}},
    {{0x01c5be, 2}, {0x01c5c0, 4}, {0x01c5c4, 4}, {0x01c5c8, 4}, {0x01c5cc, 5}},
    {{0x01c7c5, 2}, {0x01c7c7, 4}, {0x01c7cb, 4}, {0x01c7cf, 4}, {0x01c7d3, 5}},
    {{0x01c88a, 2}, {0x01c88c, 4}, {0x01c890, 4}, {0x01c894, 4}, {0x01c898, 5}},
};

static void fx_clip_enter(int copy)
{
    gsu_codes(fx_clip_enter_fetch[copy], 1);
    gsu_push(&gsu.r[10], gsu.r[8]);
    gsu_codes(fx_clip_enter_fetch[copy] + 1, 1);
    gsu_push(&gsu.r[10], gsu.r[9]);
    gsu_codes(fx_clip_enter_fetch[copy] + 2, 1);
    gsu_push(&gsu.r[10], gsu.r[12]);
    gsu_codes(fx_clip_enter_fetch[copy] + 3, 1);
    gsu_push(&gsu.r[10], gsu.r[13]);
    gsu_codes(fx_clip_enter_fetch[copy] + 4, 1);
    gsu_stw(0x00A0, gsu.r[10]);
    gsu.r[10] = 0x000C;
}

/* R10 from $A0, then R13, R12, R9, R8, R14 pulled; R0 = 1; return. */
/* gsuwalk: the code of each copy of fx_clip_leave, read from the ROM */
static const GsuCode fx_clip_leave_fetch[4][7] = {
    {{0x01c597, 3}, {0x01c59a, 4}, {0x01c59e, 4}, {0x01c5a2, 4}, {0x01c5a6, 4}, {0x01c5aa, 4}, {0x01c5ae, 6}},
    {{0x01c64b, 3}, {0x01c64e, 4}, {0x01c652, 4}, {0x01c656, 4}, {0x01c65a, 4}, {0x01c65e, 4}, {0x01c662, 6}},
    {{0x01c863, 3}, {0x01c866, 4}, {0x01c86a, 4}, {0x01c86e, 4}, {0x01c872, 4}, {0x01c876, 4}, {0x01c87a, 6}},
    {{0x01c916, 3}, {0x01c919, 4}, {0x01c91d, 4}, {0x01c921, 4}, {0x01c925, 4}, {0x01c929, 4}, {0x01c92d, 6}},
};

static void fx_clip_leave(int copy)
{
    gsu_codes(fx_clip_leave_fetch[copy], 1);
    gsu.r[10] = gsu_ldw(0x00A0);
    gsu_codes(fx_clip_leave_fetch[copy] + 1, 1);
    gsu.r[13] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_fetch[copy] + 2, 1);
    gsu.r[12] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_fetch[copy] + 3, 1);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_fetch[copy] + 4, 1);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_clip_leave_fetch[copy] + 5, 1);
    gsu_r14(gsu_pull(&gsu.r[10]));      /* TO R14; LDW: the ROM buffer refills */
    gsu.r[0] = 0x0001;
    gsu_codes(fx_clip_leave_fetch[copy] + 6, 1);
    gsu_pull(&gsu.r[10]);
}

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
/* gsuwalk: the code of each copy of fx_clip_plane, read from the ROM */
static const GsuCode fx_clip_plane_fetch[2][39] = {
    {{0x01c4ab, 3}, {0x01c4ae, 4}, {0x01c4bc, 2}, {0x01c4d3, 1}, {0x01c4bc, 3}, {0x01c4c7, 2}, {0x01c4ee, 1}, {0x01c4c7, 7}, {0x01c4ce, 5}, {0x01c4ef, 3}, {0x01c4f2, 3}, {0x01c4f5, 3}, {0x01c50b, 20}, {0x01c51f, 3}, {0x01c522, 3}, {0x01c525, 3}, {0x01c528, 3}, {0x01c575, 22}, {0x01c58b, 3}, {0x01c58e, 3}, {0x01c591, 3}, {0x01c594, 3}, {0x01c4dc, 2}, {0x01c4dc, 2}, {0x01c4e2, 6}, {0x01c4e8, 6}, {0x01c4de, 4}, {0x01c5b4, 4}, {0x01c5b8, 3}, {0x01c5bb, 3}, {0x01c5d1, 5}, {0x01c5d6, 3}, {0x01c5d9, 3}, {0x01c5dc, 3}, {0x01c629, 22}, {0x01c63f, 3}, {0x01c642, 3}, {0x01c645, 3}, {0x01c648, 3}},
    {{0x01c77b, 3}, {0x01c77e, 4}, {0x01c78b, 2}, {0x01c7a1, 1}, {0x01c78b, 3}, {0x01c795, 2}, {0x01c7bb, 1}, {0x01c795, 7}, {0x01c79c, 5}, {0x01c7bc, 3}, {0x01c7bf, 3}, {0x01c7c2, 3}, {0x01c7d8, 20}, {0x01c7ec, 3}, {0x01c7ef, 3}, {0x01c7f2, 3}, {0x01c7f5, 3}, {0x01c841, 22}, {0x01c857, 3}, {0x01c85a, 3}, {0x01c85d, 3}, {0x01c860, 3}, {0x01c7a9, 2}, {0x01c7a9, 2}, {0x01c7af, 6}, {0x01c7b5, 6}, {0x01c7ab, 4}, {0x01c880, 4}, {0x01c884, 3}, {0x01c887, 3}, {0x01c89d, 5}, {0x01c8a2, 3}, {0x01c8a5, 3}, {0x01c8a8, 3}, {0x01c8f4, 22}, {0x01c90a, 3}, {0x01c90d, 3}, {0x01c910, 3}, {0x01c913, 3}},
};

/* gsuwalk: the copy of fx_scaled each copy of fx_clip_plane calls */
static const uint8_t fx_scaled_in_fx_clip_plane[2] = {2, 13};

/* gsuwalk: the copy of fx_scaled each copy of fx_clip_plane calls */
static const uint8_t fx_scaled_in_fx_clip_plane_2[2] = {3, 14};

/* gsuwalk: the copy of fx_clip_enter each copy of fx_clip_plane calls */
static const uint8_t fx_clip_enter_in_fx_clip_plane[2] = {2, 4};

/* gsuwalk: the copy of fx_clip_search each copy of fx_clip_plane calls */
static const uint8_t fx_clip_search_in_fx_clip_plane[2] = {0, 2};

/* gsuwalk: the copy of fx_clip_leave each copy of fx_clip_plane calls */
static const uint8_t fx_clip_leave_in_fx_clip_plane[2] = {0, 2};

/* gsuwalk: the copy of fx_scaled each copy of fx_clip_plane calls */
static const uint8_t fx_scaled_in_fx_clip_plane_3[2] = {5, 16};

/* gsuwalk: the copy of fx_clip_enter each copy of fx_clip_plane calls */
static const uint8_t fx_clip_enter_in_fx_clip_plane_2[2] = {3, 5};

/* gsuwalk: the copy of fx_clip_search each copy of fx_clip_plane calls */
static const uint8_t fx_clip_search_in_fx_clip_plane_2[2] = {1, 3};

/* gsuwalk: the copy of fx_clip_leave each copy of fx_clip_plane calls */
static const uint8_t fx_clip_leave_in_fx_clip_plane_2[2] = {1, 3};

static void fx_clip_plane(int ax, int more, int copy)
{
    int i;

    gsu_codes(fx_clip_plane_fetch[copy], 1);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_codes(fx_clip_plane_fetch[copy] + 1, 1);
    gsu_push(&gsu.r[10], gsu.r[14]);
    gsu.r[0] = fx_scaled(gsu.r[ax], more, GSU_COPY(fx_scaled_in_fx_clip_plane[copy]));
    gsu.r[0] = gsu_sub(gsu.r[3], gsu.r[0], 0);
    if (!gsu.s) {
        gsu_codes(fx_clip_plane_fetch[copy] + 4, 1);
        gsu.r[0] = fx_scaled(gsu.r[ax + 3], more, GSU_COPY(fx_scaled_in_fx_clip_plane_2[copy]));
        gsu.r[0] = gsu_sub(gsu.r[7], gsu.r[0], 0);
        if (!gsu.s) {
            gsu_codes(fx_clip_plane_fetch[copy] + 7, 1);
            gsu_r14(gsu_pull(&gsu.r[10]));      /* TO R14; LDW: the ROM buffer refills */
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_codes(fx_clip_plane_fetch[copy] + 8, 1);
            gsu_pull(&gsu.r[10]);
            return;
        } else { gsu_codes(fx_clip_plane_fetch[copy] + 5, 2); }
        gsu_codes(fx_clip_plane_fetch[copy] + 9, 1);
        gsu_stw(0x009A, gsu.r[1]);
        gsu_codes(fx_clip_plane_fetch[copy] + 10, 1);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_codes(fx_clip_plane_fetch[copy] + 11, 1);
        gsu_stw(0x009E, gsu.r[3]);
        fx_clip_enter(GSU_COPY(fx_clip_enter_in_fx_clip_plane[copy]));
        {
            static const int a[3] = {1, 2, 3}, b[3] = {4, 5, 7};

            gsu_codes(fx_clip_plane_fetch[copy] + 12, 1);
            for (i = 0; i < 3; i++) {
                gsu.r[0] = gsu.r[a[i]];
                gsu.r[a[i]] = gsu.r[b[i]];
                gsu.r[b[i]] = gsu.r[0];
            }
        }
        gsu_codes(fx_clip_plane_fetch[copy] + 13, 1);
        gsu.r[8] = gsu_ldw(0x008C);
        gsu_codes(fx_clip_plane_fetch[copy] + 14, 1);
        gsu.r[9] = gsu_ldw(0x0090);
        gsu_codes(fx_clip_plane_fetch[copy] + 15, 1);
        gsu.r[12] = gsu_ldw(0x008A);
        gsu_codes(fx_clip_plane_fetch[copy] + 16, 1);
        gsu.r[13] = gsu_ldw(0x008E);
        fx_clip_search(ax, more, 1, GSU_COPY(fx_clip_search_in_fx_clip_plane[copy]));
        gsu_codes(fx_clip_plane_fetch[copy] + 17, 1);
        gsu.r[1] = gsu_ldw(0x009A);
        gsu_codes(fx_clip_plane_fetch[copy] + 18, 1);
        gsu.r[2] = gsu_ldw(0x009C);
        gsu_codes(fx_clip_plane_fetch[copy] + 19, 1);
        gsu.r[3] = gsu_ldw(0x009E);
        gsu_codes(fx_clip_plane_fetch[copy] + 20, 1);
        gsu_stw(0x008C, gsu.r[12]);
        gsu_codes(fx_clip_plane_fetch[copy] + 21, 1);
        gsu_stw(0x0090, gsu.r[13]);
        fx_clip_leave(GSU_COPY(fx_clip_leave_in_fx_clip_plane[copy]));
        return;
    } else { gsu_codes(fx_clip_plane_fetch[copy] + 2, 2); }
    gsu.r[0] = fx_scaled(gsu.r[ax + 3], more, GSU_COPY(fx_scaled_in_fx_clip_plane_3[copy]));
    gsu.r[0] = gsu_sub(gsu.r[7], gsu.r[0], 0);
    if (gsu.s) {
        gsu_codes(fx_clip_plane_fetch[copy] + 23, 2);
        gsu_r14(gsu_pull(&gsu.r[10]));      /* TO R14; LDW: the ROM buffer refills */
        gsu.r[0] = 0xFFFF;
        gsu_codes(fx_clip_plane_fetch[copy] + 25, 1);
        gsu_pull(&gsu.r[10]);
        return;
    } else { gsu_codes(fx_clip_plane_fetch[copy] + 22, 1); }
    gsu_codes(fx_clip_plane_fetch[copy] + 26, 2);
    gsu_stw(0x009A, gsu.r[4]);
    gsu_codes(fx_clip_plane_fetch[copy] + 28, 1);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_codes(fx_clip_plane_fetch[copy] + 29, 1);
    gsu_stw(0x009E, gsu.r[7]);
    fx_clip_enter(GSU_COPY(fx_clip_enter_in_fx_clip_plane_2[copy]));
    gsu_codes(fx_clip_plane_fetch[copy] + 30, 1);
    gsu.r[8] = gsu_ldw(0x008A);
    gsu_codes(fx_clip_plane_fetch[copy] + 31, 1);
    gsu.r[9] = gsu_ldw(0x008E);
    gsu_codes(fx_clip_plane_fetch[copy] + 32, 1);
    gsu.r[12] = gsu_ldw(0x008C);
    gsu_codes(fx_clip_plane_fetch[copy] + 33, 1);
    gsu.r[13] = gsu_ldw(0x0090);
    fx_clip_search(ax, more, 0, GSU_COPY(fx_clip_search_in_fx_clip_plane_2[copy]));
    gsu_codes(fx_clip_plane_fetch[copy] + 34, 1);
    gsu.r[4] = gsu_ldw(0x009A);
    gsu_codes(fx_clip_plane_fetch[copy] + 35, 1);
    gsu.r[5] = gsu_ldw(0x009C);
    gsu_codes(fx_clip_plane_fetch[copy] + 36, 1);
    gsu.r[7] = gsu_ldw(0x009E);
    gsu_codes(fx_clip_plane_fetch[copy] + 37, 1);
    gsu_stw(0x008A, gsu.r[8]);
    gsu_codes(fx_clip_plane_fetch[copy] + 38, 1);
    gsu_stw(0x008E, gsu.r[9]);
    fx_clip_leave(GSU_COPY(fx_clip_leave_in_fx_clip_plane_2[copy]));
}

void abi_FxClipYPlane(void)
{
    fx_clip_plane(2, 2, GSU_COPY(0));
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
    fx_clip_plane(1, 1, GSU_COPY(1));
}

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
/* gsuwalk: the copy of fx_near_test each copy of fx_plane_test calls */
static const uint8_t fx_near_test_in_fx_plane_test[15] = {0, 1, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

/* gsuwalk: the copy of fx_scaled each copy of fx_plane_test calls */
static const uint8_t fx_scaled_in_fx_plane_test[15] = {0, 0, 0, 0, 1, 7, 8, 9, 10, 11, 12, 18, 19, 20, 21};

static void fx_plane_test(const FxPlaneDriver *d, int c, int z, int copy)
{
    if (!d->slope) {
        fx_near_test(gsu.r[z], GSU_COPY(fx_near_test_in_fx_plane_test[copy]));
        return;
    }
    gsu.r[0] = fx_scaled(gsu.r[c], d->more, GSU_COPY(fx_scaled_in_fx_plane_test[copy]));
    gsu.r[0] = d->neg ? gsu_add(gsu.r[z], gsu.r[0], 0) : gsu_sub(gsu.r[z], gsu.r[0], 0);
}

/* A vertex (three registers and the two attribute words) appended at R9,
   the output count $40 counted up. */
/* gsuwalk: the code of each copy of fx_emit, read from the ROM */
static const GsuCode fx_emit_fetch[15][9] = {
    {{0x01ceac, 2}, {0x01ceae, 4}, {0x01ceb2, 4}, {0x01ceb6, 5}, {0x01cebb, 1}, {0x01cebc, 5}, {0x01cec1, 1}, {0x01cec2, 5}, {0x01cec7, 2}},
    {{0x01ced8, 2}, {0x01ceda, 4}, {0x01cede, 4}, {0x01cee2, 5}, {0x01cee7, 1}, {0x01cee8, 5}, {0x01ceed, 1}, {0x01ceee, 5}, {0x01cef3, 2}},
    {{0x01cefe, 2}, {0x01cf00, 4}, {0x01cf04, 4}, {0x01cf08, 5}, {0x01cf0d, 1}, {0x01cf0e, 5}, {0x01cf13, 1}, {0x01cf14, 5}, {0x01cf19, 2}},
    {{0x01c6e3, 2}, {0x01c6e5, 4}, {0x01c6e9, 4}, {0x01c6ed, 5}, {0x01c6f2, 1}, {0x01c6f3, 5}, {0x01c6f8, 1}, {0x01c6f9, 5}, {0x01c6fe, 2}},
    {{0x01c710, 2}, {0x01c712, 4}, {0x01c716, 4}, {0x01c71a, 5}, {0x01c71f, 1}, {0x01c720, 5}, {0x01c725, 1}, {0x01c726, 5}, {0x01c72b, 2}},
    {{0x01c736, 2}, {0x01c738, 4}, {0x01c73c, 4}, {0x01c740, 5}, {0x01c745, 1}, {0x01c746, 5}, {0x01c74b, 1}, {0x01c74c, 5}, {0x01c751, 2}},
    {{0x01cbde, 2}, {0x01cbe0, 4}, {0x01cbe4, 4}, {0x01cbe8, 5}, {0x01cbed, 1}, {0x01cbee, 5}, {0x01cbf3, 1}, {0x01cbf4, 5}, {0x01cbf9, 2}},
    {{0x01cc11, 5}, {0x01cc16, 4}, {0x01cc1a, 4}, {0x01cc1e, 5}, {0x01cc23, 1}, {0x01cc24, 5}, {0x01cc29, 1}, {0x01cc2a, 5}, {0x01cc2f, 2}},
    {{0x01cc40, 5}, {0x01cc45, 4}, {0x01cc49, 4}, {0x01cc4d, 5}, {0x01cc52, 1}, {0x01cc53, 5}, {0x01cc58, 1}, {0x01cc59, 5}, {0x01cc5e, 2}},
    {{0x01c9ac, 2}, {0x01c9ae, 4}, {0x01c9b2, 4}, {0x01c9b6, 5}, {0x01c9bb, 1}, {0x01c9bc, 5}, {0x01c9c1, 1}, {0x01c9c2, 5}, {0x01c9c7, 2}},
    {{0x01c9d8, 2}, {0x01c9da, 4}, {0x01c9de, 4}, {0x01c9e2, 5}, {0x01c9e7, 1}, {0x01c9e8, 5}, {0x01c9ed, 1}, {0x01c9ee, 5}, {0x01c9f3, 2}},
    {{0x01c9fe, 2}, {0x01ca00, 4}, {0x01ca04, 4}, {0x01ca08, 5}, {0x01ca0d, 1}, {0x01ca0e, 5}, {0x01ca13, 1}, {0x01ca14, 5}, {0x01ca19, 2}},
    {{0x01cabb, 2}, {0x01cabd, 4}, {0x01cac1, 4}, {0x01cac5, 5}, {0x01caca, 1}, {0x01cacb, 5}, {0x01cad0, 1}, {0x01cad1, 5}, {0x01cad6, 2}},
    {{0x01caed, 5}, {0x01caf2, 4}, {0x01caf6, 4}, {0x01cafa, 5}, {0x01caff, 1}, {0x01cb00, 5}, {0x01cb05, 1}, {0x01cb06, 5}, {0x01cb0b, 2}},
    {{0x01cb1c, 5}, {0x01cb21, 4}, {0x01cb25, 4}, {0x01cb29, 5}, {0x01cb2e, 1}, {0x01cb2f, 5}, {0x01cb34, 1}, {0x01cb35, 5}, {0x01cb3a, 2}},
};

static void fx_emit(int r1, int r2, int r3, uint16_t at1, uint16_t at2, int copy)
{
    const int r[3] = {r1, r2, r3};
    int i;

    for (i = 0; i < 3; i++) {
        gsu_codes(fx_emit_fetch[copy] + i, 1);
        gsu_stw(gsu.r[fx_ro], gsu.r[r[i]]);
        gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    }
    gsu_codes(fx_emit_fetch[copy] + 3, 1);
    gsu.r[0] = gsu_ldw(at1);
    gsu_codes(fx_emit_fetch[copy] + 4, 1);
    gsu_stw(gsu.r[fx_ro], gsu.r[0]);
    gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    gsu_codes(fx_emit_fetch[copy] + 5, 1);
    gsu.r[0] = gsu_ldw(at2);
    gsu_codes(fx_emit_fetch[copy] + 6, 1);
    gsu_stw(gsu.r[fx_ro], gsu.r[0]);
    gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    gsu_codes(fx_emit_fetch[copy] + 7, 1);
    gsu.r[0] = gsu_inc(gsu_ldw(0x0040));
    gsu_codes(fx_emit_fetch[copy] + 8, 1);
    gsu_sbk(gsu.r[0]);
}

/* Register r set, an R14 write through gsu_r14 (the ROM buffer refills). */
/* gsuwalk: the code of each copy of set_reg, read from the ROM */
static const GsuCode set_reg_fetch[5][1] = {
    {{0, 0}},
    {{0x01c753, 1}},
    {{0x01cc60, 1}},
    {{0x01ca1b, 1}},
    {{0x01cb3c, 1}},
};

static void set_reg(int r, uint16_t v, int copy)
{
    if (r == 14)
        { 
        gsu_codes(set_reg_fetch[copy], 1);
        gsu_r14(v); }
    else
        gsu.r[r] = v;
}

static void fx_negate(int r)
{
    gsu.r[r] = gsu_inc(gsu_not(gsu.r[r]));
}

/* The clip call, the axis coordinates negated around it for the negative
   side (only the replaced end's afterwards). */
/* gsuwalk: the code of each copy of fx_plane_clip, read from the ROM */
static const GsuCode fx_plane_clip_fetch[10][6] = {
    {{0x01ced3, 4}, {0x01cc87, 1}, {0xcc88, 0}, {0, 0}, {0x01ced7, 1}, {0, 0}},
    {{0x01cea9, 2}, {0x01cef8, 5}, {0x01cc87, 1}, {0xcc88, 0}, {0x01cefd, 1}, {0, 0}},
    {{0x01c70b, 4}, {0x01c4aa, 1}, {0xc4ab, 0}, {0, 0}, {0x01c70f, 1}, {0, 0}},
    {{0x01c6e0, 2}, {0x01c730, 5}, {0x01c4aa, 1}, {0xc4ab, 0}, {0x01c735, 1}, {0, 0}},
    {{0x01cc06, 10}, {0x01c4aa, 1}, {0xc4ab, 0}, {0, 0}, {0, 0}, {0x01cc10, 1}},
    {{0x01cbdb, 2}, {0x01cc34, 11}, {0x01c4aa, 1}, {0xc4ab, 0}, {0, 0}, {0x01cc3f, 1}},
    {{0x01c9d3, 4}, {0x01c77a, 1}, {0xc77b, 0}, {0, 0}, {0x01c9d7, 1}, {0, 0}},
    {{0x01c9a9, 2}, {0x01c9f8, 5}, {0x01c77a, 1}, {0xc77b, 0}, {0x01c9fd, 1}, {0, 0}},
    {{0x01cae2, 10}, {0x01c77a, 1}, {0xc77b, 0}, {0, 0}, {0, 0}, {0x01caec, 1}},
    {{0x01cab8, 2}, {0x01cb10, 11}, {0x01c77a, 1}, {0xc77b, 0}, {0, 0}, {0x01cb1b, 1}},
};

static void fx_plane_clip(const FxPlaneDriver *d, uint16_t ret, int fixed, int copy)
{
    if (d->neg) {
        fx_negate(d->ax);
        fx_negate(d->ax + 3);
    }
    gsu.r[11] = ret;
    gsu_codes(fx_plane_clip_fetch[copy], 4);
    d->clip();
    if (d->neg)
        { 
        gsu_codes(fx_plane_clip_fetch[copy] + 5, 1);
        fx_negate(fixed); } else { gsu_codes(fx_plane_clip_fetch[copy] + 4, 1); }
}

/* Clips the polygon at $60 (count $40, five words a vertex) against the
   plane into the list at $62: Sutherland-Hodgman, each edge from vertex
   to next (the last to the first). R0 = the new count, or 0 when fewer
   than 3 are left or there were only 2. */
/* gsuwalk: the code of each copy of fx_plane_driver, read from the ROM */
static const GsuCode fx_plane_driver_fetch[5][56] = {
    {{0x01ce35, 2}, {0x01ce37, 4}, {0, 0}, {0, 0}, {0x01ce3b, 4}, {0x01ce3f, 3}, {0x01ce42, 4}, {0x01ce46, 3}, {0x01ce49, 3}, {0x01ce4c, 5}, {0x01ce55, 1}, {0x01ce4c, 9}, {0x01cf38, 1}, {0x01cf1b, 7}, {0x01ce56, 1}, {0x01cf1b, 3}, {0x01cf22, 1}, {0x01ce56, 1}, {0x01ce57, 2}, {0x01ce59, 4}, {0x01ce5d, 4}, {0x01ce61, 3}, {0x01ce64, 5}, {0x01ce69, 1}, {0x01ce6a, 5}, {0x01ce6f, 5}, {0x01ce77, 1}, {0x01ce6f, 9}, {0x01ce78, 2}, {0x01ce7a, 4}, {0x01ce7e, 4}, {0x01ce82, 3}, {0x01ce85, 5}, {0x01ce8a, 1}, {0x01ce8b, 3}, {0x01ce98, 2}, {0x01ceab, 1}, {0x01ced0, 2}, {0x01cf1a, 1}, {0x01cef5, 3}, {0x01cf1a, 1}, {0x01ced0, 3}, {0x01ce98, 3}, {0x01cea2, 6}, {0x01cf1a, 1}, {0x01cea2, 2}, {0x01cea8, 1}, {0x01cf23, 3}, {0x01cf26, 5}, {0x01cf26, 5}, {0x01cf2f, 5}, {0x01cf34, 4}, {0x01cf2b, 4}, {0x01cf38, 1}, {0x01cf39, 4}, {0x01cf3d, 5}},
    {{0x01c669, 2}, {0x01c66b, 4}, {0x01c66f, 5}, {0x01c674, 3}, {0, 0}, {0, 0}, {0x01c677, 4}, {0x01c67b, 3}, {0x01c67e, 3}, {0x01c681, 5}, {0x01c68a, 1}, {0x01c681, 9}, {0x01c770, 1}, {0x01c754, 6}, {0x01c68b, 1}, {0x01c754, 2}, {0x01c75a, 1}, {0x01c68b, 1}, {0x01c68c, 2}, {0x01c68e, 4}, {0x01c692, 4}, {0x01c696, 3}, {0x01c699, 5}, {0x01c69e, 1}, {0x01c69f, 5}, {0x01c6a4, 5}, {0x01c6ac, 1}, {0x01c6a4, 9}, {0x01c6ad, 2}, {0x01c6af, 4}, {0x01c6b3, 4}, {0x01c6b7, 3}, {0x01c6ba, 5}, {0x01c6bf, 1}, {0x01c6c0, 3}, {0x01c6ce, 2}, {0x01c6e2, 1}, {0x01c708, 2}, {0x01c752, 1}, {0x01c72d, 3}, {0x01c752, 1}, {0x01c708, 3}, {0x01c6ce, 3}, {0x01c6d9, 6}, {0x01c752, 1}, {0x01c6d9, 2}, {0x01c6df, 1}, {0x01c75b, 3}, {0x01c75e, 5}, {0x01c75e, 5}, {0x01c767, 5}, {0x01c76c, 4}, {0x01c763, 4}, {0x01c770, 1}, {0x01c771, 4}, {0x01c775, 5}},
    {{0x01cb64, 2}, {0x01cb66, 4}, {0x01cb6a, 5}, {0x01cb6f, 3}, {0, 0}, {0, 0}, {0x01cb72, 4}, {0x01cb76, 3}, {0x01cb79, 3}, {0x01cb7c, 5}, {0x01cb85, 1}, {0x01cb7c, 9}, {0x01cc7d, 1}, {0x01cc61, 6}, {0x01cb86, 1}, {0x01cc61, 2}, {0x01cc67, 1}, {0x01cb86, 1}, {0x01cb87, 2}, {0x01cb89, 4}, {0x01cb8d, 4}, {0x01cb91, 3}, {0x01cb94, 5}, {0x01cb99, 1}, {0x01cb9a, 5}, {0x01cb9f, 5}, {0x01cba7, 1}, {0x01cb9f, 9}, {0x01cba8, 2}, {0x01cbaa, 4}, {0x01cbae, 4}, {0x01cbb2, 3}, {0x01cbb5, 5}, {0x01cbba, 1}, {0x01cbbb, 3}, {0x01cbc9, 2}, {0x01cbdd, 1}, {0x01cc03, 2}, {0x01cc5f, 1}, {0x01cc31, 3}, {0x01cc5f, 1}, {0x01cc03, 3}, {0x01cbc9, 3}, {0x01cbd4, 6}, {0x01cc5f, 1}, {0x01cbd4, 2}, {0x01cbda, 1}, {0x01cc68, 3}, {0x01cc6b, 5}, {0x01cc6b, 5}, {0x01cc74, 5}, {0x01cc79, 4}, {0x01cc70, 4}, {0x01cc7d, 1}, {0x01cc7e, 4}, {0x01cc82, 5}},
    {{0x01c934, 2}, {0x01c936, 4}, {0x01c93a, 5}, {0x01c93f, 3}, {0, 0}, {0, 0}, {0x01c942, 4}, {0x01c946, 3}, {0x01c949, 3}, {0x01c94c, 5}, {0x01c955, 1}, {0x01c94c, 9}, {0x01ca38, 1}, {0x01ca1c, 6}, {0x01c956, 1}, {0x01ca1c, 2}, {0x01ca22, 1}, {0x01c956, 1}, {0x01c957, 2}, {0x01c959, 4}, {0x01c95d, 4}, {0x01c961, 3}, {0x01c964, 5}, {0x01c969, 1}, {0x01c96a, 5}, {0x01c96f, 5}, {0x01c977, 1}, {0x01c96f, 9}, {0x01c978, 2}, {0x01c97a, 4}, {0x01c97e, 4}, {0x01c982, 3}, {0x01c985, 5}, {0x01c98a, 1}, {0x01c98b, 3}, {0x01c998, 2}, {0x01c9ab, 1}, {0x01c9d0, 2}, {0x01ca1a, 1}, {0x01c9f5, 3}, {0x01ca1a, 1}, {0x01c9d0, 3}, {0x01c998, 3}, {0x01c9a2, 6}, {0x01ca1a, 1}, {0x01c9a2, 2}, {0x01c9a8, 1}, {0x01ca23, 3}, {0x01ca26, 5}, {0x01ca26, 5}, {0x01ca2f, 5}, {0x01ca34, 4}, {0x01ca2b, 4}, {0x01ca38, 1}, {0x01ca39, 4}, {0x01ca3d, 5}},
    {{0x01ca43, 2}, {0x01ca45, 4}, {0x01ca49, 5}, {0x01ca4e, 3}, {0, 0}, {0, 0}, {0x01ca51, 4}, {0x01ca55, 3}, {0x01ca58, 3}, {0x01ca5b, 5}, {0x01ca64, 1}, {0x01ca5b, 9}, {0x01cb59, 1}, {0x01cb3d, 6}, {0x01ca65, 1}, {0x01cb3d, 2}, {0x01cb43, 1}, {0x01ca65, 1}, {0x01ca66, 2}, {0x01ca68, 4}, {0x01ca6c, 4}, {0x01ca70, 3}, {0x01ca73, 5}, {0x01ca78, 1}, {0x01ca79, 5}, {0x01ca7e, 5}, {0x01ca86, 1}, {0x01ca7e, 9}, {0x01ca87, 2}, {0x01ca89, 4}, {0x01ca8d, 4}, {0x01ca91, 3}, {0x01ca94, 5}, {0x01ca99, 1}, {0x01ca9a, 3}, {0x01caa7, 2}, {0x01caba, 1}, {0x01cadf, 2}, {0x01cb3b, 1}, {0x01cb0d, 3}, {0x01cb3b, 1}, {0x01cadf, 3}, {0x01caa7, 3}, {0x01cab1, 6}, {0x01cb3b, 1}, {0x01cab1, 2}, {0x01cab7, 1}, {0x01cb44, 3}, {0x01cb47, 5}, {0x01cb47, 5}, {0x01cb50, 5}, {0x01cb55, 4}, {0x01cb4c, 4}, {0x01cb59, 1}, {0x01cb5a, 4}, {0x01cb5e, 5}},
};

/* gsuwalk: the copy of fx_plane_test each copy of fx_plane_driver calls */
static const uint8_t fx_plane_test_in_fx_plane_driver[5] = {0, 3, 6, 9, 12};

/* gsuwalk: the copy of fx_emit each copy of fx_plane_driver calls */
static const uint8_t fx_emit_in_fx_plane_driver[5] = {0, 3, 6, 9, 12};

/* gsuwalk: the copy of fx_plane_test each copy of fx_plane_driver calls */
static const uint8_t fx_plane_test_in_fx_plane_driver_2[5] = {1, 4, 7, 10, 13};

/* gsuwalk: the copy of fx_plane_clip each copy of fx_plane_driver calls */
static const uint8_t fx_plane_clip_in_fx_plane_driver[5] = {0, 2, 4, 6, 8};

/* gsuwalk: the copy of fx_emit each copy of fx_plane_driver calls */
static const uint8_t fx_emit_in_fx_plane_driver_2[5] = {1, 4, 7, 10, 13};

/* gsuwalk: the copy of fx_plane_test each copy of fx_plane_driver calls */
static const uint8_t fx_plane_test_in_fx_plane_driver_3[5] = {2, 5, 8, 11, 14};

/* gsuwalk: the copy of fx_plane_clip each copy of fx_plane_driver calls */
static const uint8_t fx_plane_clip_in_fx_plane_driver_2[5] = {1, 3, 5, 7, 9};

/* gsuwalk: the copy of fx_emit each copy of fx_plane_driver calls */
static const uint8_t fx_emit_in_fx_plane_driver_3[5] = {2, 5, 8, 11, 14};

/* gsuwalk: the copy of set_reg each copy of fx_plane_driver calls */
static const uint8_t set_reg_in_fx_plane_driver[5] = {0, 1, 2, 3, 4};

static void fx_plane_driver(const FxPlaneDriver *d, int copy)
{
    int in1, i, rc, ri, ro;
    static const int reg1[3] = {1, 2, 3}, reg2[3] = {4, 5, 7};

    if (d->slope) {
        rc = 14; ri = 8; ro = 9;
    } else {
        rc = 9; ri = 6; ro = 8;
    }
    fx_ro = ro;
    gsu_codes(fx_plane_driver_fetch[copy], 1);
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_codes(fx_plane_driver_fetch[copy] + 1, 1);
    gsu_push(&gsu.r[10], gsu.r[14]);
    if (d->slope) {
        gsu_codes(fx_plane_driver_fetch[copy] + 2, 1);
        gsu.r[6] = gsu_ldw(d->slope);
        gsu_codes(fx_plane_driver_fetch[copy] + 3, 1);
        gsu_r14(gsu_ldw(0x0040));
    } else {
        gsu_codes(fx_plane_driver_fetch[copy] + 4, 1);
        gsu_r14(gsu.r[0]);
        gsu_codes(fx_plane_driver_fetch[copy] + 5, 1);
        gsu.r[9] = gsu_ldw(0x0040);
    }
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_codes(fx_plane_driver_fetch[copy] + 6, 1);
    gsu_stw(0x0040, gsu.r[0]);
    gsu_codes(fx_plane_driver_fetch[copy] + 7, 1);
    gsu.r[ri] = gsu_ldw(0x0060);
    gsu_codes(fx_plane_driver_fetch[copy] + 8, 1);
    gsu.r[ro] = gsu_ldw(0x0062);
    gsu.r[0] = gsu_sub(gsu.r[rc], 0x0002, 0);
    if (gsu.z)
        { 
        gsu_codes(fx_plane_driver_fetch[copy] + 11, 2);
        goto fail; } else { gsu_codes(fx_plane_driver_fetch[copy] + 9, 2); }
    gsu_codes(fx_plane_driver_fetch[copy] + 17, 1);
    do {
        for (i = 0; i < 3; i++) {
            gsu_codes(fx_plane_driver_fetch[copy] + 18 + i, 1);
            gsu.r[reg1[i]] = gsu_ldw(gsu.r[ri]);
            gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        }
        gsu_codes(fx_plane_driver_fetch[copy] + 21, 1);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_codes(fx_plane_driver_fetch[copy] + 22, 1);
        gsu_stw(0x008A, gsu.r[0]);
        gsu_codes(fx_plane_driver_fetch[copy] + 23, 1);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_codes(fx_plane_driver_fetch[copy] + 24, 1);
        gsu_stw(0x008E, gsu.r[0]);
        gsu.r[0] = gsu_dec(gsu.r[rc]);
        if (gsu.z)
            { 
            gsu_codes(fx_plane_driver_fetch[copy] + 27, 1);
            gsu.r[ri] = gsu_ldw(0x0060); } else { gsu_codes(fx_plane_driver_fetch[copy] + 25, 2); }
        for (i = 0; i < 3; i++) {
            gsu_codes(fx_plane_driver_fetch[copy] + 28 + i, 1);
            gsu.r[reg2[i]] = gsu_ldw(gsu.r[ri]);
            gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        }
        gsu_codes(fx_plane_driver_fetch[copy] + 31, 1);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_codes(fx_plane_driver_fetch[copy] + 32, 1);
        gsu_stw(0x008C, gsu.r[0]);
        gsu_codes(fx_plane_driver_fetch[copy] + 33, 1);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu_codes(fx_plane_driver_fetch[copy] + 34, 1);
        gsu_stw(0x0090, gsu.r[0]);
        gsu.r[ri] = gsu_sub(gsu.r[ri], 0x0008, 0);
        fx_plane_test(d, d->ax, 3, GSU_COPY(fx_plane_test_in_fx_plane_driver[copy]));
        in1 = !gsu.s;
        if (in1) {
            gsu_codes(fx_plane_driver_fetch[copy] + 35, 2);
            fx_emit(1, 2, 3, 0x008A, 0x008E, GSU_COPY(fx_emit_in_fx_plane_driver[copy]));
            fx_plane_test(d, d->ax + 3, 7, GSU_COPY(fx_plane_test_in_fx_plane_driver_2[copy]));
            if (gsu.s) {
                gsu_codes(fx_plane_driver_fetch[copy] + 41, 1);
                fx_plane_clip(d, d->ret_out, d->ax + 3, GSU_COPY(fx_plane_clip_in_fx_plane_driver[copy]));
                fx_emit(4, 5, 7, 0x008C, 0x0090, GSU_COPY(fx_emit_in_fx_plane_driver_2[copy]));
                gsu_codes(fx_plane_driver_fetch[copy] + 39, 2);
            } else { gsu_codes(fx_plane_driver_fetch[copy] + 37, 2); }
        } else {
            gsu_codes(fx_plane_driver_fetch[copy] + 42, 1);
            fx_plane_test(d, d->ax + 3, 7, GSU_COPY(fx_plane_test_in_fx_plane_driver_3[copy]));
            if (!gsu.s) {
                gsu_codes(fx_plane_driver_fetch[copy] + 45, 2);
                fx_plane_clip(d, d->ret_in, d->ax, GSU_COPY(fx_plane_clip_in_fx_plane_driver_2[copy]));
                fx_emit(1, 2, 3, 0x008A, 0x008E, GSU_COPY(fx_emit_in_fx_plane_driver_3[copy]));
            } else { gsu_codes(fx_plane_driver_fetch[copy] + 43, 2); }
        }
        set_reg(rc, gsu_dec(gsu.r[rc]), GSU_COPY(set_reg_in_fx_plane_driver[copy]));
    } while (gsu_codes_loop(fx_plane_driver_fetch[copy] + 13, 2, !gsu.z));
    gsu_codes(fx_plane_driver_fetch[copy] + 15, 2);
    gsu_codes(fx_plane_driver_fetch[copy] + 47, 1);
    gsu.r[0] = gsu_ldw(0x0040);
    gsu.r[1] = gsu_sub(gsu.r[0], 0x0003, 0);
    if (!gsu.s) {
        gsu_codes(fx_plane_driver_fetch[copy] + 49, 2);
        gsu_r14(gsu_pull(&gsu.r[10]));      /* TO R14; LDW: the ROM buffer refills */
        gsu_codes(fx_plane_driver_fetch[copy] + 51, 1);
        gsu_pull(&gsu.r[10]);
        return;
    } else { gsu_codes(fx_plane_driver_fetch[copy] + 48, 1); }
gsu_codes(fx_plane_driver_fetch[copy] + 52, 2);
fail:
    ;
    gsu_codes(fx_plane_driver_fetch[copy] + 54, 1);
    gsu_r14(gsu_pull(&gsu.r[10]));      /* TO R14; LDW: the ROM buffer refills */
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_codes(fx_plane_driver_fetch[copy] + 55, 1);
    gsu_pull(&gsu.r[10]);
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

    fx_plane_driver(&d, GSU_COPY(1));
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

    fx_plane_driver(&d, GSU_COPY(3));
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

    fx_plane_driver(&d, GSU_COPY(4));
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

    fx_plane_driver(&d, GSU_COPY(2));
}

/* MOVES R0, R14, then (in the BPL's delay slot) SUB z, and for a negative
   R14 ADD instead: R0 = R14 - z, or R14 + z. */
/* gsuwalk: the code of each copy of fx_near_test, read from the ROM */
static const GsuCode fx_near_test_fetch[8][3] = {
    {{0x01ce8e, 7}, {0x01ce97, 1}, {0x01ce8e, 10}},
    {{0x01cec9, 4}, {0x01cecf, 1}, {0x01cec9, 7}},
    {{0x01cc8b, 6}, {0x01cc93, 1}, {0x01cc8b, 9}},
    {{0x01cc97, 4}, {0x01cc9d, 1}, {0x01cc97, 7}},
    {{0x01ccf9, 9}, {0x01cd04, 1}, {0x01ccf9, 12}},
    {{0x01cca7, 4}, {0x01ccad, 1}, {0x01cca7, 7}},
    {{0x01cdac, 9}, {0x01cdb7, 1}, {0x01cdac, 12}},
    {{0x01ce9b, 4}, {0x01cea1, 1}, {0x01ce9b, 7}},
};

static void fx_near_test(uint16_t z, int copy)
{
    int pl;

    gsu.r[0] = gsu.r[14];
    gsu_moves(gsu.r[0]);
    pl = !gsu.s;
    gsu.r[0] = gsu_sub(gsu.r[0], z, 0);
    if (!pl)
        { gsu.r[0] = gsu_add(gsu.r[14], z, 0); gsu_codes(fx_near_test_fetch[copy] + 2, 1); } else { gsu_codes(fx_near_test_fetch[copy], 2); }
}

/* The near clip's halving search, x and y both halved; then the far end
   (the second when keep_first) ends at the midpoint. */
/* gsuwalk: the code of each copy of fx_near_search, read from the ROM */
static const GsuCode fx_near_search_fetch[2][17] = {
    {{0x01cd05, 2}, {0x01cd05, 2}, {0x01cd42, 1}, {0x01cd2a, 24}, {0x01ccf8, 1}, {0x01cd0e, 24}, {0x01ccf8, 1}, {0x01cd07, 6}, {0x01cd07, 6}, {0x01cd42, 1}, {0x01cd0d, 1}, {0x01cd07, 3}, {0x01cd26, 3}, {0x01cd07, 3}, {0x01cd26, 3}, {0x01cd42, 1}, {0x01cd29, 1}},
    {{0x01cdb8, 2}, {0x01cdb8, 2}, {0x01cdf5, 1}, {0x01cddd, 24}, {0x01cdab, 1}, {0x01cdc1, 24}, {0x01cdab, 1}, {0x01cdba, 6}, {0x01cdba, 6}, {0x01cdf5, 1}, {0x01cdc0, 1}, {0x01cdba, 3}, {0x01cdd9, 3}, {0x01cdba, 3}, {0x01cdd9, 3}, {0x01cdf5, 1}, {0x01cddc, 1}},
};

/* gsuwalk: the copy of fx_near_test each copy of fx_near_search calls */
static const uint8_t fx_near_test_in_fx_near_search[2] = {4, 6};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search[2] = {6, 9};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search_2[2] = {6, 9};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search_3[2] = {6, 9};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search_4[2] = {6, 9};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search_5[2] = {7, 10};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search_6[2] = {7, 10};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search_7[2] = {7, 10};

/* gsuwalk: the copy of fx_half_sum each copy of fx_near_search calls */
static const uint8_t fx_half_sum_in_fx_near_search_8[2] = {7, 10};

static void fx_near_search(int keep_first, int copy)
{
    int neg;

    for (;;) {
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[7], 0);
        gsu.r[11] = gsu_div2(gsu.r[0]);
        fx_near_test(gsu.r[11], GSU_COPY(fx_near_test_in_fx_near_search[copy]));
        if (gsu.z)
            { 
            gsu_codes(fx_near_search_fetch[copy] + 1, 2);
            break; } else { gsu_codes(fx_near_search_fetch[copy], 1); }
        neg = gsu.s;
        gsu.r[10] = gsu_dec(gsu.r[10]);         /* BMI's delay slot */
        if (!neg) {
            if (gsu.z)
                { 
                gsu_codes(fx_near_search_fetch[copy] + 8, 2);
                break; } else { gsu_codes(fx_near_search_fetch[copy] + 7, 1); }
            gsu.r[7] = gsu.r[11];
            gsu_codes(fx_near_search_fetch[copy] + 10, 1);
            fx_half_sum(4, 1, 4, GSU_COPY(fx_half_sum_in_fx_near_search[copy]));
            fx_half_sum(5, 2, 5, GSU_COPY(fx_half_sum_in_fx_near_search_2[copy]));
            fx_half_sum(12, 8, 12, GSU_COPY(fx_half_sum_in_fx_near_search_3[copy]));
            fx_half_sum(13, 9, 13, GSU_COPY(fx_half_sum_in_fx_near_search_4[copy]));
            gsu_codes(fx_near_search_fetch[copy] + 5, 2);
        } else {
            if (gsu.z)
                { 
                gsu_codes(fx_near_search_fetch[copy] + 13, 3);
                break; } else { gsu_codes(fx_near_search_fetch[copy] + 11, 2); }
            gsu.r[3] = gsu.r[11];
            gsu_codes(fx_near_search_fetch[copy] + 16, 1);
            fx_half_sum(1, 1, 4, GSU_COPY(fx_half_sum_in_fx_near_search_5[copy]));
            fx_half_sum(2, 2, 5, GSU_COPY(fx_half_sum_in_fx_near_search_6[copy]));
            fx_half_sum(8, 8, 12, GSU_COPY(fx_half_sum_in_fx_near_search_7[copy]));
            fx_half_sum(9, 9, 13, GSU_COPY(fx_half_sum_in_fx_near_search_8[copy]));
            gsu_codes(fx_near_search_fetch[copy] + 3, 2);
        }
    }
    if (keep_first) {
        gsu.r[7] = gsu.r[11];
        fx_half_sum(4, 1, 4, GSU_COPY(8));
        fx_half_sum(5, 2, 5, GSU_COPY(8));
        fx_half_sum(12, 8, 12, GSU_COPY(8));
        fx_half_sum(13, 9, 13, GSU_COPY(8));
    } else {
        gsu.r[3] = gsu.r[11];
        fx_half_sum(1, 1, 4, GSU_COPY(11));
        fx_half_sum(2, 2, 5, GSU_COPY(11));
        fx_half_sum(8, 8, 12, GSU_COPY(11));
        fx_half_sum(9, 9, 13, GSU_COPY(11));
    }
}

/* R10 from $A0, then R13, R12, R9, R8 pulled; R0 = 1; return. */
/* gsuwalk: the code of each copy of fx_near_leave, read from the ROM */
static const GsuCode fx_near_leave_fetch[2][6] = {
    {{0x01cd68, 3}, {0x01cd6b, 4}, {0x01cd6f, 4}, {0x01cd73, 4}, {0x01cd77, 4}, {0x01cd7b, 6}},
    {{0x01ce1b, 3}, {0x01ce1e, 4}, {0x01ce22, 4}, {0x01ce26, 4}, {0x01ce2a, 4}, {0x01ce2e, 6}},
};

static void fx_near_leave(int copy)
{
    gsu_codes(fx_near_leave_fetch[copy], 1);
    gsu.r[10] = gsu_ldw(0x00A0);
    gsu_codes(fx_near_leave_fetch[copy] + 1, 1);
    gsu.r[13] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_near_leave_fetch[copy] + 2, 1);
    gsu.r[12] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_near_leave_fetch[copy] + 3, 1);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    gsu_codes(fx_near_leave_fetch[copy] + 4, 1);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = 0x0001;
    gsu_codes(fx_near_leave_fetch[copy] + 5, 1);
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
    int i;

    gsu_code(0x01cc88, 3);
    gsu_push(&gsu.r[10], gsu.r[11]);
    fx_near_test(gsu.r[3], GSU_COPY(2));
    if (!gsu.s) {
        gsu_code(0x01cc94, 3);
        fx_near_test(gsu.r[7], GSU_COPY(3));
        if (!gsu.s) {
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_code(0x01cc9e, 8);
            gsu_pull(&gsu.r[10]);
            return;
        } else { gsu_code(0x01cc9e, 2); gsu_code(0x01ccbc, 1); }
        gsu_code(0x01ccbd, 3);
        gsu_stw(0x009A, gsu.r[1]);
        gsu_code(0x01ccc0, 3);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_code(0x01ccc3, 3);
        gsu_stw(0x009E, gsu.r[3]);
        fx_clip_enter(GSU_COPY(0));
        {
            static const int a[3] = {1, 2, 3}, b[3] = {4, 5, 7};

            gsu_code(0x01ccd9, 20);
            for (i = 0; i < 3; i++) {
                gsu.r[0] = gsu.r[a[i]];
                gsu.r[a[i]] = gsu.r[b[i]];
                gsu.r[b[i]] = gsu.r[0];
            }
        }
        gsu_code(0x01cced, 3);
        gsu.r[8] = gsu_ldw(0x008C);
        gsu_code(0x01ccf0, 3);
        gsu.r[9] = gsu_ldw(0x0090);
        gsu_code(0x01ccf3, 3);
        gsu.r[12] = gsu_ldw(0x008A);
        gsu_code(0x01ccf6, 3);
        gsu.r[13] = gsu_ldw(0x008E);
        fx_near_search(1, GSU_COPY(0));
        gsu_code(0x01cd43, 25);
        gsu.r[1] = gsu_ldw(0x009A);
        gsu_code(0x01cd5c, 3);
        gsu.r[2] = gsu_ldw(0x009C);
        gsu_code(0x01cd5f, 3);
        gsu.r[3] = gsu_ldw(0x009E);
        gsu_code(0x01cd62, 3);
        gsu_stw(0x008C, gsu.r[12]);
        gsu_code(0x01cd65, 3);
        gsu_stw(0x0090, gsu.r[13]);
        fx_near_leave(GSU_COPY(0));
        return;
    } else { gsu_code(0x01cc94, 2); gsu_code(0x01cca6, 1); }
    fx_near_test(gsu.r[7], GSU_COPY(5));
    if (gsu.s) {
        gsu.r[0] = 0xFFFF;
        
        
        gsu_code(0x01ccae, 2); gsu_code(0x01ccb4, 8);
        gsu_pull(&gsu.r[10]);
        return;
    } else { gsu_code(0x01ccae, 2); }
    
    
    gsu_code(0x01ccb0, 4); gsu_code(0x01cd81, 4);
    gsu_stw(0x009A, gsu.r[4]);
    gsu_code(0x01cd85, 3);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_code(0x01cd88, 3);
    gsu_stw(0x009E, gsu.r[7]);
    fx_clip_enter(GSU_COPY(1));
    gsu_code(0x01cd9e, 5);
    gsu.r[8] = gsu_ldw(0x008A);
    gsu_code(0x01cda3, 3);
    gsu.r[9] = gsu_ldw(0x008E);
    gsu_code(0x01cda6, 3);
    gsu.r[12] = gsu_ldw(0x008C);
    gsu_code(0x01cda9, 3);
    gsu.r[13] = gsu_ldw(0x0090);
    fx_near_search(0, GSU_COPY(1));
    gsu_code(0x01cdf6, 25);
    gsu.r[4] = gsu_ldw(0x009A);
    gsu_code(0x01ce0f, 3);
    gsu.r[5] = gsu_ldw(0x009C);
    gsu_code(0x01ce12, 3);
    gsu.r[7] = gsu_ldw(0x009E);
    gsu_code(0x01ce15, 3);
    gsu_stw(0x008A, gsu.r[8]);
    gsu_code(0x01ce18, 3);
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

    fx_plane_driver(&d, GSU_COPY(0));
}
