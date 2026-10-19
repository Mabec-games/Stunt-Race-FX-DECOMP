/*
 * fx_math.c: the Super FX's 32-bit divide and square root.
 *
 * Both run sixteen rounds under LOOP (R12 the count, R13 the round's start)
 * and keep a 32-bit value in two registers, high word first.
 */
#include <string.h>
#include "gsu_ops.h"
#include "fx_calls.h"

/*
 * @routine 04:C144 FxDivide
 * WHAT IT DOES: Divides the 32-bit R0:R4 by R6: the quotient to R4, the
 * remainder to R0. The quotient must fit in 16 bits (R0 < R6).
 * HOW: Restoring division, one quotient bit a round: the pair is shifted
 * left (ADD R4 to itself, ROL R0), R6 is subtracted from R0 and, on a
 * borrow, added back. The shift into R0 runs one bit ahead (the ROL in
 * LOOP's delay slot) and the closing LSR takes it back, so bit 15 of a
 * remainder above $7FFF is lost.
 * Entry: R0 = high word, R4 = low word, R6 = divisor; R11 = return address.
 * Exit: R4 = quotient; R0 = remainder; R12 = 0; R13 = $C14B; S clear, Z
 * from R0, CY = the bit shifted out; OV from the last round's SUB or ADD.
 */
FxQuotient FxDivide(uint16_t hi, uint16_t lo, uint16_t divisor, int copy)
{
    FxQuotient o;
    uint16_t count = 16;
    int again;

    lo = gsu_add(lo, lo, 0);
    hi = gsu_rol(hi);
    /* each way through a round has its own LOOP (and its own return after
       the last round), as the original has */
    gsu_code(0x04c145, 7);
    for (;;) {
        hi = gsu_sub(hi, divisor, 0);
        if (gsu.cy) {
            lo = gsu_rol(lo);                /* quotient bit 1 */
            again = gsu_count_down(&count);
            hi = gsu_rol(hi);
            if (!again)
                { gsu_code(0x04c14c, 9); break; } else { gsu_code(0x04c14c, 7); gsu_code(0x04c14b, 1); }
        } else {
            hi = gsu_add(hi, divisor, 0);    /* restore; quotient bit 0 */
            lo = gsu_add(lo, lo, 0);
            again = gsu_count_down(&count);
            hi = gsu_rol(hi);
            if (!again)
                { gsu_code(0x04c14c, 3); gsu_code(0x04c155, 7); break; } else { gsu_code(0x04c14c, 3); gsu_code(0x04c155, 5); gsu_code(0x04c14b, 1); }
        }
    }
    o.quot = lo;
    o.rem = gsu_lsr(hi);
    return o;
}

static void abi_divide_exit(FxQuotient o)
{
    gsu.r[0] = o.rem;
    gsu.r[4] = o.quot;
    gsu.r[12] = 0;
    gsu.r[13] = 0xC14B;
}

void abi_FxDivide(void)
{
    abi_divide_exit(FxDivide(gsu.r[0], gsu.r[4], gsu.r[6], GSU_COPY(0)));
}

/*
 * @routine 04:C135 FxDivideHalvedIfLarge
 * WHAT IT DOES: FxDivide, except that when R0 or R6 has bit 15 set the
 * divisor and the 32-bit dividend are first halved, so the quotient is
 * kept in range.
 * HOW: MOVES R0 and R6 for their signs; the halving is LSR R6, LSR R0 and
 * ROR R4, then the code runs on into FxDivide. The halving path is decoded
 * from the ROM bytes and not reached in the traces so far.
 * Entry: as FxDivide. Exit: as FxDivide; R6 = the divisor used.
 */
FxQuotient FxDivideHalvedIfLarge(uint16_t hi, uint16_t lo, uint16_t *divisor, int copy)
{
    gsu_moves(hi);
    if (!gsu.s) {
        gsu_moves(*divisor);
        if (!gsu.s)
            { gsu_code(0x04c136, 9); gsu_code(0x04c144, 1); return FxDivide(hi, lo, *divisor, GSU_COPY(0)); } else { gsu_code(0x04c136, 9); }
        gsu_code(0x04c13f, 1);
    } else { gsu_code(0x04c136, 4); gsu_code(0x04c13f, 1); }
    *divisor = gsu_lsr(*divisor);
    hi = gsu_lsr(hi);
    lo = gsu_ror(lo);
    gsu_code(0x04c140, 5);
    return FxDivide(hi, lo, *divisor, GSU_COPY(0));
}

void abi_FxDivideHalvedIfLarge(void)
{
    uint16_t divisor = gsu.r[6];

    abi_divide_exit(FxDivideHalvedIfLarge(gsu.r[0], gsu.r[4], &divisor, GSU_COPY(0)));
    gsu.r[6] = divisor;
}

/*
 * @routine 04:C10F FxSquareRoot
 * WHAT IT DOES: The integer square root of the 32-bit R4:R5 to R6, the
 * remainder (value - root squared) to R8.
 * HOW: Digit by digit: each round shifts two bits of R4:R5 into R8 (ADD R5
 * to itself, ROL R4, ROL R8, twice), doubles the root, and when R8 exceeds
 * twice the root, subtracts twice the root plus one (SBC with the borrow
 * the compare left) and sets the root's new low bit. LOOP's delay slot
 * holds the WITH R5 for the next round's first ADD.
 * Entry: R4 = high word, R5 = low word; R11 = return address. Exit: R6 =
 * root; R8 = remainder; R4 = R5 = 0; R7 = the last round's twice the root;
 * R0 = R7 - R8 from the last round's compare; R12 = 0; R13 = $C119; Z set,
 * S clear; CY, OV from the last compare or subtract.
 */
FxRoot FxSquareRoot(uint16_t hi, uint16_t lo, int copy)
{
    FxRoot o = {0, 0, 0, 0};
    uint16_t count = 16;
    int i;

    gsu_code(0x04c110, 11);
    do {
        gsu_code(0x04c11b, 17);
        for (i = 0; i < 2; i++) {
            lo = gsu_add(lo, lo, 0);
            hi = gsu_rol(hi);
            o.rem = gsu_rol(o.rem);
        }
        o.root = gsu_add(o.root, o.root, 0);
        o.trial = gsu_add(o.root, o.root, 0);
        o.diff = gsu_sub(o.trial, o.rem, 0);
        if (!gsu.cy) {
            o.rem = gsu_sub(o.rem, o.trial, 1);   /* SBC: minus trial + 1 */
            o.root = gsu_inc(o.root);
            gsu_code(0x04c12c, 6);
        } else { gsu_code(0x04c12c, 2); gsu_code(0x04c131, 1); }
    } while (gsu_code_loop(0x04c119, 2, gsu_code_loop(0x04c132, 1, gsu_count_down(&count))));
    gsu_code(0x04c132, 1);
    gsu_code(0x04c133, 2);
    return o;
}

void abi_FxSquareRoot(void)
{
    FxRoot o = FxSquareRoot(gsu.r[4], gsu.r[5], GSU_COPY(0));

    gsu.r[0] = o.diff;
    gsu.r[4] = 0;
    gsu.r[5] = 0;
    gsu.r[6] = o.root;
    gsu.r[7] = o.trial;
    gsu.r[8] = o.rem;
    gsu.r[12] = 0;
    gsu.r[13] = 0xC119;
}

/*
 * @routine 04:C087 FxNormalize
 * WHAT IT DOES: Scales the vector (R1, R2, R3) to length $7FFF (1.0 in 1.15
 * fixed point); a zero vector stays zero.
 * HOW: The squared length as 32 bits (three LMULTs summed with ADD and
 * ADC), FxSquareRoot of it, then k = the root's top bit number (LSR until
 * zero); FxDivideHalvedIfLarge divides $7FFF << k by the root, each
 * component is LMULTed by the quotient and the 32-bit products shifted
 * right by k (ASR, ROR under LOOP). The return address is kept on the R10
 * stack around the two calls.
 * Entry: R1, R2, R3 = the vector; R10 = stack; R11 = return address.
 * Exit: R1, R2, R3 = the scaled vector (low words of the products); R7, R8,
 * R0 = their high words; R6 = the quotient; R9 = k; R4 = R3; R5 = 0; R11 =
 * $C0E3; R12 = 0; R13 = $C0FD ($C14B when k = 0); the last RAM address R10;
 * the flags from the last LOOP and ROR. A zero vector: R0-R6 = 0; R11
 * unchanged; Z set, S, OV clear.
 */
FxRegs FxNormalize(FxRegs g, int copy)
{
    uint32_t p;
    uint16_t hi, count, divisor;
    FxRoot root;
    FxQuotient q;
    int again;

    gsu_code(0x04c088, 2);
    gsu_stw(g.r[10], g.r[11]);
    g.r[10] = gsu_inc(gsu_inc(g.r[10]));
    gsu_code(0x04c08a, 7);
    p = gsu_lmult(g.r[1], g.r[1]);
    hi = (uint16_t)(p >> 16);
    g.r[5] = (uint16_t)p;
    gsu_code(0x04c091, 8);
    p = gsu_lmult(g.r[2], g.r[2]);
    g.r[5] = gsu_add(g.r[5], (uint16_t)p, 0);
    hi = gsu_add(hi, (uint16_t)(p >> 16), gsu.cy);
    gsu_code(0x04c099, 10);
    p = gsu_lmult(g.r[3], g.r[3]);
    g.r[4] = (uint16_t)p;
    g.r[6] = (uint16_t)(p >> 16);
    g.r[5] = gsu_add(g.r[5], g.r[4], 0);
    g.r[0] = hi = gsu_add(hi, g.r[6], gsu.cy);
    gsu_moves(hi);
    if (gsu.z) {
        gsu_moves(g.r[5]);
        if (gsu.z) {
            g.r[1] = g.r[2] = g.r[3] = 0;
            g.r[10] = gsu_dec(gsu_dec(g.r[10]));
            gsu_code(0x04c0a3, 24);
            gsu_ldw(g.r[10]);
            return g;
        } else { gsu_code(0x04c0a3, 13); gsu_code(0x04c0bb, 1); }
    } else { gsu_code(0x04c0a3, 8); gsu_code(0x04c0bb, 1); }
    gsu_code(0x04c0bc, 6); gsu_code(0x04c10f, 1);
    root = FxSquareRoot(hi, g.r[5], GSU_COPY(0));
    g.r[4] = g.r[5] = 0;
    g.r[7] = root.trial;
    g.r[8] = root.rem;
    g.r[0] = root.root;
    g.r[9] = 0;
    gsu_code(0x04c0c2, 5);
    for (;;) {
        g.r[0] = gsu_lsr(g.r[0]);
        if (gsu.z)
            { gsu_code(0x04c0c7, 3); gsu_code(0x04c0cd, 1); break; } else { gsu_code(0x04c0c7, 3); }
        g.r[9] = gsu_inc(g.r[9]);
        gsu_code(0x04c0ca, 3); gsu_code(0x04c0c6, 1);
    }
    g.r[0] = gsu_sub(g.r[0], g.r[0], 0);
    g.r[4] = 0x7FFF;
    count = g.r[9];
    gsu_moves(count);
    if (!gsu.z) {
        gsu_code(0x04c0ce, 11);
        do {
            g.r[4] = gsu_add(g.r[4], g.r[4], 0);
            again = gsu_count_down(&count);
            g.r[0] = gsu_rol(g.r[0]);
        } while (gsu_code_loop(0x04c0d8, 1, gsu_code_loop(0x04c0d9, 3, again)));
        gsu_code(0x04c0d9, 3);
        gsu_code(0x04c0dc, 1);
    } else { gsu_code(0x04c0ce, 8); gsu_code(0x04c0dc, 1); }
    divisor = root.root;
    gsu_code(0x04c0dd, 6); gsu_code(0x04c135, 1);
    q = FxDivideHalvedIfLarge(g.r[0], g.r[4], &divisor, GSU_COPY(0));
    g.r[13] = 0xC14B;
    g.r[6] = q.quot;
    gsu_code(0x04c0e3, 7);
    p = gsu_lmult(g.r[1], q.quot);
    g.r[7] = (uint16_t)(p >> 16);
    g.r[1] = (uint16_t)p;
    gsu_code(0x04c0ea, 6);
    p = gsu_lmult(g.r[2], q.quot);
    g.r[8] = (uint16_t)(p >> 16);
    g.r[2] = (uint16_t)p;
    gsu_code(0x04c0f0, 5);
    p = gsu_lmult(g.r[3], q.quot);
    g.r[0] = (uint16_t)(p >> 16);
    g.r[3] = g.r[4] = (uint16_t)p;
    count = g.r[9];
    gsu_moves(count);
    if (!gsu.z) {
        g.r[13] = 0xC0FD;
        gsu_code(0x04c0f5, 9);
        do {
            g.r[7] = gsu_asr(g.r[7]);
            g.r[1] = gsu_ror(g.r[1]);
            g.r[8] = gsu_asr(g.r[8]);
            g.r[2] = gsu_ror(g.r[2]);
            g.r[0] = gsu_asr(g.r[0]);
            g.r[3] = gsu_ror(g.r[3]);
        } while (gsu_code_loop(0x04c0fd, 1, gsu_code_loop(0x04c0fe, 12, gsu_count_down(&count))));
        gsu_code(0x04c0fe, 12);
        gsu_code(0x04c10a, 1);
    } else { gsu_code(0x04c0f5, 6); gsu_code(0x04c10a, 1); }
    g.r[11] = 0xC0E3;
    g.r[12] = 0;
    g.r[10] = gsu_dec(gsu_dec(g.r[10]));
    gsu_code(0x04c10b, 4);
    gsu_ldw(g.r[10]);
    return g;
}

void abi_FxNormalize(void)
{
    FxRegs g;

    memcpy(g.r, gsu.r, sizeof g.r);
    g = FxNormalize(g, GSU_COPY(0));
    memcpy(gsu.r, g.r, 15 * sizeof g.r[0]);
}
