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

/*
 * @routine 01:8325 FxMatrixFromAnglesProgram
 * WHAT IT DOES: The Super FX program that builds the rotation matrix for
 * the angles at $0020-$0024 into $00E4 (FxMatrixFromAngles01).
 * HOW: R9 = $00E4, the call, then STOP.
 * Entry: started by the S-CPU. Exit (at the STOP): as FxMatrixFromAngles01,
 * R9 = $00F4, R11 = $832D.
 */
void abi_FxMatrixFromAnglesProgram(void)
{
    gsu.r[9] = 0x00E4;
    gsu.r[11] = 0x832D;
    abi_FxMatrixFromAngles01();
}

/* R0 = Ra + Rb (ADD), then Rd = R0 DIV2. */
static void fx_half_sum(int d, int a, int b)
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
            return;
        }
        gsu_stw(0x009A, gsu.r[1]);
        gsu_stw(0x009C, gsu.r[2]);
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
                fx_half_sum(4, 1, 4);
                fx_half_sum(5, 2, 5);
                gsu.r[1] = gsu_ldw(0x009A);
                gsu.r[2] = gsu_ldw(0x009C);
                gsu.r[3] = gsu_ldw(0x009E);
                gsu.r[0] = 0x0001;
                return;
            }
            if (gsu.s) {
                gsu.r[3] = gsu.r[0];
                fx_half_sum(1, 1, 4);
                fx_half_sum(2, 2, 5);
            } else {
                gsu.r[6] = gsu.r[0];
                fx_half_sum(4, 1, 4);
                fx_half_sum(5, 2, 5);
            }
        }
    }
    gsu_moves(gsu.r[6]);
    if (gsu.s) {
        gsu.r[0] = 0xFFFF;
        return;
    }
    gsu_stw(0x009A, gsu.r[4]);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_stw(0x009E, gsu.r[6]);
    for (;;) {
        gsu.r[0] = gsu_div2(gsu_add(gsu.r[3], gsu.r[6], 0));
        if (gsu.z) {
            gsu.r[3] = gsu.r[0];
            fx_half_sum(1, 1, 4);
            fx_half_sum(2, 2, 5);
            gsu.r[4] = gsu_ldw(0x009A);
            gsu.r[5] = gsu_ldw(0x009C);
            gsu.r[6] = gsu_ldw(0x009E);
            gsu.r[0] = 0x0001;
            return;
        }
        if (gsu.s) {
            gsu.r[3] = gsu.r[0];
            fx_half_sum(1, 1, 4);
            fx_half_sum(2, 2, 5);
        } else {
            gsu.r[6] = gsu.r[0];
            fx_half_sum(4, 1, 4);
            fx_half_sum(5, 2, 5);
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

/* The halving search of the plane clips: the midpoint (R14 on the axis,
   R11 = z) tested against the plane; the end on the midpoint's side moves
   to it, with its attributes (R8/R12, R9/R13). At most 12 steps (R10,
   counted in the BMI's delay slot). Then the far end (the second when
   keep_first) ends at the midpoint. ax = 1 (x) or 2 (y). */
static void fx_clip_search(int ax, int more, int keep_first)
{
    int o = 3 - ax, neg;

    for (;;) {
        gsu.r[0] = gsu_add(gsu.r[ax], gsu.r[ax + 3], 0);
        gsu.r[14] = gsu_div2(gsu.r[0]);
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[7], 0);
        gsu.r[11] = gsu_div2(gsu.r[0]);
        gsu.r[0] = fx_scaled(gsu.r[14], more);
        gsu.r[0] = gsu_sub(gsu.r[11], gsu.r[0], 0);
        if (gsu.z)
            break;
        neg = gsu.s;
        gsu.r[10] = gsu_dec(gsu.r[10]);
        if (gsu.z)
            break;
        if (!neg) {
            gsu.r[7] = gsu.r[11];
            fx_half_sum(o + 3, o, o + 3);
            gsu.r[ax + 3] = gsu.r[14];
            fx_half_sum(12, 8, 12);
            fx_half_sum(13, 9, 13);
        } else {
            gsu.r[3] = gsu.r[11];
            fx_half_sum(o, o, o + 3);
            gsu.r[ax] = gsu.r[14];
            fx_half_sum(8, 8, 12);
            fx_half_sum(9, 9, 13);
        }
    }
    if (keep_first) {
        gsu.r[7] = gsu.r[11];
        fx_half_sum(o + 3, o, o + 3);
        gsu.r[ax + 3] = gsu.r[14];
        fx_half_sum(12, 8, 12);
        fx_half_sum(13, 9, 13);
    } else {
        gsu.r[3] = gsu.r[11];
        fx_half_sum(o, o, o + 3);
        gsu.r[ax] = gsu.r[14];
        fx_half_sum(8, 8, 12);
        fx_half_sum(9, 9, 13);
    }
}

/* Saves R8, R9, R12, R13 on the R10 stack and R10 at $A0; R10 = 12. */
static void fx_clip_enter(void)
{
    gsu_push(&gsu.r[10], gsu.r[8]);
    gsu_push(&gsu.r[10], gsu.r[9]);
    gsu_push(&gsu.r[10], gsu.r[12]);
    gsu_push(&gsu.r[10], gsu.r[13]);
    gsu_stw(0x00A0, gsu.r[10]);
    gsu.r[10] = 0x000C;
}

/* R10 from $A0, then R13, R12, R9, R8, R14 pulled; R0 = 1; return. */
static void fx_clip_leave(void)
{
    gsu.r[10] = gsu_ldw(0x00A0);
    gsu.r[13] = gsu_pull(&gsu.r[10]);
    gsu.r[12] = gsu_pull(&gsu.r[10]);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu.r[14] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = 0x0001;
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
static void fx_clip_plane(int ax, int more)
{
    int i;

    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_push(&gsu.r[10], gsu.r[14]);
    gsu.r[0] = fx_scaled(gsu.r[ax], more);
    gsu.r[0] = gsu_sub(gsu.r[3], gsu.r[0], 0);
    if (!gsu.s) {
        gsu.r[0] = fx_scaled(gsu.r[ax + 3], more);
        gsu.r[0] = gsu_sub(gsu.r[7], gsu.r[0], 0);
        if (!gsu.s) {
            gsu.r[14] = gsu_pull(&gsu.r[10]);
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_pull(&gsu.r[10]);
            return;
        }
        gsu_stw(0x009A, gsu.r[1]);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_stw(0x009E, gsu.r[3]);
        fx_clip_enter();
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
        fx_clip_search(ax, more, 1);
        gsu.r[1] = gsu_ldw(0x009A);
        gsu.r[2] = gsu_ldw(0x009C);
        gsu.r[3] = gsu_ldw(0x009E);
        gsu_stw(0x008C, gsu.r[12]);
        gsu_stw(0x0090, gsu.r[13]);
        fx_clip_leave();
        return;
    }
    gsu.r[0] = fx_scaled(gsu.r[ax + 3], more);
    gsu.r[0] = gsu_sub(gsu.r[7], gsu.r[0], 0);
    if (gsu.s) {
        gsu.r[14] = gsu_pull(&gsu.r[10]);
        gsu.r[0] = 0xFFFF;
        gsu_pull(&gsu.r[10]);
        return;
    }
    gsu_stw(0x009A, gsu.r[4]);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_stw(0x009E, gsu.r[7]);
    fx_clip_enter();
    gsu.r[8] = gsu_ldw(0x008A);
    gsu.r[9] = gsu_ldw(0x008E);
    gsu.r[12] = gsu_ldw(0x008C);
    gsu.r[13] = gsu_ldw(0x0090);
    fx_clip_search(ax, more, 0);
    gsu.r[4] = gsu_ldw(0x009A);
    gsu.r[5] = gsu_ldw(0x009C);
    gsu.r[7] = gsu_ldw(0x009E);
    gsu_stw(0x008A, gsu.r[8]);
    gsu_stw(0x008E, gsu.r[9]);
    fx_clip_leave();
}

void abi_FxClipYPlane(void)
{
    fx_clip_plane(2, 2);
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
    fx_clip_plane(1, 1);
}

static void fx_near_test(uint16_t z);

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
static void fx_plane_test(const FxPlaneDriver *d, int c, int z)
{
    if (!d->slope) {
        fx_near_test(gsu.r[z]);
        return;
    }
    gsu.r[0] = fx_scaled(gsu.r[c], d->more);
    gsu.r[0] = d->neg ? gsu_add(gsu.r[z], gsu.r[0], 0) : gsu_sub(gsu.r[z], gsu.r[0], 0);
}

/* A vertex (three registers and the two attribute words) appended at R9,
   the output count $40 counted up. */
static void fx_emit(int r1, int r2, int r3, uint16_t at1, uint16_t at2)
{
    const int r[3] = {r1, r2, r3};
    int i;

    for (i = 0; i < 3; i++) {
        gsu_stw(gsu.r[fx_ro], gsu.r[r[i]]);
        gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    }
    gsu.r[0] = gsu_ldw(at1);
    gsu_stw(gsu.r[fx_ro], gsu.r[0]);
    gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    gsu.r[0] = gsu_ldw(at2);
    gsu_stw(gsu.r[fx_ro], gsu.r[0]);
    gsu.r[fx_ro] = gsu_inc(gsu_inc(gsu.r[fx_ro]));
    gsu.r[0] = gsu_inc(gsu_ldw(0x0040));
    gsu_sbk(gsu.r[0]);
}

static void fx_negate(int r)
{
    gsu.r[r] = gsu_inc(gsu_not(gsu.r[r]));
}

/* The clip call, the axis coordinates negated around it for the negative
   side (only the replaced end's afterwards). */
static void fx_plane_clip(const FxPlaneDriver *d, uint16_t ret, int fixed)
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

/* Clips the polygon at $60 (count $40, five words a vertex) against the
   plane into the list at $62: Sutherland-Hodgman, each edge from vertex
   to next (the last to the first). R0 = the new count, or 0 when fewer
   than 3 are left or there were only 2. */
static void fx_plane_driver(const FxPlaneDriver *d)
{
    int in1, i, rc, ri, ro;
    static const int reg1[3] = {1, 2, 3}, reg2[3] = {4, 5, 7};

    if (d->slope) {
        rc = 14; ri = 8; ro = 9;
    } else {
        rc = 9; ri = 6; ro = 8;
    }
    fx_ro = ro;
    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_push(&gsu.r[10], gsu.r[14]);
    if (d->slope) {
        gsu.r[6] = gsu_ldw(d->slope);
        gsu.r[14] = gsu_ldw(0x0040);
    } else {
        gsu.r[14] = gsu.r[0];
        gsu.r[9] = gsu_ldw(0x0040);
    }
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
    gsu_stw(0x0040, gsu.r[0]);
    gsu.r[ri] = gsu_ldw(0x0060);
    gsu.r[ro] = gsu_ldw(0x0062);
    gsu.r[0] = gsu_sub(gsu.r[rc], 0x0002, 0);
    if (gsu.z)
        goto fail;
    do {
        for (i = 0; i < 3; i++) {
            gsu.r[reg1[i]] = gsu_ldw(gsu.r[ri]);
            gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        }
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_stw(0x008A, gsu.r[0]);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_stw(0x008E, gsu.r[0]);
        gsu.r[0] = gsu_dec(gsu.r[rc]);
        if (gsu.z)
            gsu.r[ri] = gsu_ldw(0x0060);
        for (i = 0; i < 3; i++) {
            gsu.r[reg2[i]] = gsu_ldw(gsu.r[ri]);
            gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        }
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu.r[ri] = gsu_inc(gsu_inc(gsu.r[ri]));
        gsu_stw(0x008C, gsu.r[0]);
        gsu.r[0] = gsu_ldw(gsu.r[ri]);
        gsu_stw(0x0090, gsu.r[0]);
        gsu.r[ri] = gsu_sub(gsu.r[ri], 0x0008, 0);
        fx_plane_test(d, d->ax, 3);
        in1 = !gsu.s;
        if (in1) {
            fx_emit(1, 2, 3, 0x008A, 0x008E);
            fx_plane_test(d, d->ax + 3, 7);
            if (gsu.s) {
                fx_plane_clip(d, d->ret_out, d->ax + 3);
                fx_emit(4, 5, 7, 0x008C, 0x0090);
            }
        } else {
            fx_plane_test(d, d->ax + 3, 7);
            if (!gsu.s) {
                fx_plane_clip(d, d->ret_in, d->ax);
                fx_emit(1, 2, 3, 0x008A, 0x008E);
            }
        }
        gsu.r[rc] = gsu_dec(gsu.r[rc]);
    } while (!gsu.z);
    gsu.r[0] = gsu_ldw(0x0040);
    gsu.r[1] = gsu_sub(gsu.r[0], 0x0003, 0);
    if (!gsu.s) {
        gsu.r[14] = gsu_pull(&gsu.r[10]);
        gsu_pull(&gsu.r[10]);
        return;
    }
fail:
    gsu.r[14] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
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

    fx_plane_driver(&d);
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

    fx_plane_driver(&d);
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

    fx_plane_driver(&d);
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

    fx_plane_driver(&d);
}

/* MOVES R0, R14, then (in the BPL's delay slot) SUB z, and for a negative
   R14 ADD instead: R0 = R14 - z, or R14 + z. */
static void fx_near_test(uint16_t z)
{
    int pl;

    gsu.r[0] = gsu.r[14];
    gsu_moves(gsu.r[0]);
    pl = !gsu.s;
    gsu.r[0] = gsu_sub(gsu.r[0], z, 0);
    if (!pl)
        gsu.r[0] = gsu_add(gsu.r[14], z, 0);
}

/* The near clip's halving search, x and y both halved; then the far end
   (the second when keep_first) ends at the midpoint. */
static void fx_near_search(int keep_first)
{
    int neg;

    for (;;) {
        gsu.r[0] = gsu_add(gsu.r[3], gsu.r[7], 0);
        gsu.r[11] = gsu_div2(gsu.r[0]);
        fx_near_test(gsu.r[11]);
        if (gsu.z)
            break;
        neg = gsu.s;
        gsu.r[10] = gsu_dec(gsu.r[10]);
        if (gsu.z)
            break;
        if (!neg) {
            gsu.r[7] = gsu.r[11];
            fx_half_sum(4, 1, 4);
            fx_half_sum(5, 2, 5);
            fx_half_sum(12, 8, 12);
            fx_half_sum(13, 9, 13);
        } else {
            gsu.r[3] = gsu.r[11];
            fx_half_sum(1, 1, 4);
            fx_half_sum(2, 2, 5);
            fx_half_sum(8, 8, 12);
            fx_half_sum(9, 9, 13);
        }
    }
    if (keep_first) {
        gsu.r[7] = gsu.r[11];
        fx_half_sum(4, 1, 4);
        fx_half_sum(5, 2, 5);
        fx_half_sum(12, 8, 12);
        fx_half_sum(13, 9, 13);
    } else {
        gsu.r[3] = gsu.r[11];
        fx_half_sum(1, 1, 4);
        fx_half_sum(2, 2, 5);
        fx_half_sum(8, 8, 12);
        fx_half_sum(9, 9, 13);
    }
}

/* R10 from $A0, then R13, R12, R9, R8 pulled; R0 = 1; return. */
static void fx_near_leave(void)
{
    gsu.r[10] = gsu_ldw(0x00A0);
    gsu.r[13] = gsu_pull(&gsu.r[10]);
    gsu.r[12] = gsu_pull(&gsu.r[10]);
    gsu.r[9] = gsu_pull(&gsu.r[10]);
    gsu.r[8] = gsu_pull(&gsu.r[10]);
    gsu.r[0] = 0x0001;
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

    gsu_push(&gsu.r[10], gsu.r[11]);
    fx_near_test(gsu.r[3]);
    if (!gsu.s) {
        fx_near_test(gsu.r[7]);
        if (!gsu.s) {
            gsu.r[0] = gsu_sub(gsu.r[0], gsu.r[0], 0);
            gsu_pull(&gsu.r[10]);
            return;
        }
        gsu_stw(0x009A, gsu.r[1]);
        gsu_stw(0x009C, gsu.r[2]);
        gsu_stw(0x009E, gsu.r[3]);
        fx_clip_enter();
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
        fx_near_search(1);
        gsu.r[1] = gsu_ldw(0x009A);
        gsu.r[2] = gsu_ldw(0x009C);
        gsu.r[3] = gsu_ldw(0x009E);
        gsu_stw(0x008C, gsu.r[12]);
        gsu_stw(0x0090, gsu.r[13]);
        fx_near_leave();
        return;
    }
    fx_near_test(gsu.r[7]);
    if (gsu.s) {
        gsu.r[0] = 0xFFFF;
        gsu_pull(&gsu.r[10]);
        return;
    }
    gsu_stw(0x009A, gsu.r[4]);
    gsu_stw(0x009C, gsu.r[5]);
    gsu_stw(0x009E, gsu.r[7]);
    fx_clip_enter();
    gsu.r[8] = gsu_ldw(0x008A);
    gsu.r[9] = gsu_ldw(0x008E);
    gsu.r[12] = gsu_ldw(0x008C);
    gsu.r[13] = gsu_ldw(0x0090);
    fx_near_search(0);
    gsu.r[4] = gsu_ldw(0x009A);
    gsu.r[5] = gsu_ldw(0x009C);
    gsu.r[7] = gsu_ldw(0x009E);
    gsu_stw(0x008A, gsu.r[8]);
    gsu_stw(0x008E, gsu.r[9]);
    fx_near_leave();
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

    fx_plane_driver(&d);
}
