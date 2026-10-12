/*
 * arith.c: the Super FX's long division and square root, done a bit at a
 * time in LOOPs (the GSU-1 has no divide).
 *
 * The three dividers share one loop, each with its own copy of it: the
 * dividend is shifted left one bit first (its low word into the high
 * word), then 16 passes of restoring division: subtract the divisor from
 * the high word, and if that borrows add it back and shift a 0 into the
 * low word, else shift a 1 in; the low word's top bit shifts into the high
 * word. The low word ends as the quotient and the high word as the
 * remainder shifted left one, with the bit shifted out of it dropped each
 * pass, so the remainder is exact only while it fits in 15 bits.
 */
#include "gsu.h"

/* The 16 passes, from the dividend as it stands after the first shift.
   Returns the quotient; *rem_out = the high word after the last pass. */
static uint16_t divide_passes(uint16_t rem, uint16_t quot, uint16_t divisor, uint16_t *rem_out)
{
    int n;

    for (n = 0; n < 16; n++) {
        uint16_t out = quot >> 15;

        if (rem >= divisor) {
            rem = (uint16_t)(rem - divisor);
            quot = (uint16_t)(quot << 1 | 1);
        } else {
            quot = (uint16_t)(quot << 1);
        }
        rem = (uint16_t)(rem << 1 | out);
    }
    *rem_out = rem;
    return quot;
}

/* The flags the passes leave: OV from the last pass's SUB, or from the
   ADD R4 after its restoring ADD; CY from its last ROL of the high word.
   The passes are repeated for them with the instructions' flag rules. */
static void divide_flags(uint16_t rem, uint16_t quot, uint16_t divisor)
{
    int n;

    for (n = 0; n < 16; n++) {
        uint16_t out = quot >> 15;
        uint16_t diff = gsu_sub(rem, divisor, 0);

        if (gsu.cy) {
            rem = diff;
            quot = (uint16_t)(quot << 1 | 1);
        } else {
            quot = gsu_add(quot, quot, 0);
        }
        gsu.cy = (uint8_t)(rem >> 15);
        rem = (uint16_t)(rem << 1 | out);
    }
}

/*
 * @routine 01:8186 FxDivideSigned
 * WHAT IT DOES: Divides the 32-bit value R5:R4 (R5 the high word) by R6,
 * signed: R4 = the quotient, R0 = the remainder of the magnitudes, shifted
 * left one.
 * HOW: CACHE; R7 = R5 XOR R6 keeps the quotient's sign; R6 and R5:R4 are
 * made positive (SUB from R0 = 0, the 32-bit one as SUB then SBC); the
 * shared loop; then R4 is negated (NOT, INC) when R7 is negative. There is
 * no closing LSR: the remainder is left shifted.
 * Entry: R5:R4 = dividend, R6 = divisor; R11 = return address. Exit: R4 =
 * quotient, R0 = remainder * 2 plus the last bit shifted in; R5 = the
 * dividend's high word made positive, R6 = the divisor made positive, R7 =
 * the sign word; R12 = 0; R13 = $81A3 (the loop); OV, S, Z from MOVES R7
 * (S and Z from the quotient when it was negated); CY from the loop.
 */
uint16_t FxDivideSigned(uint16_t hi, uint16_t lo, uint16_t divisor, uint16_t *remainder)
{
    uint16_t sign = hi ^ divisor;
    uint16_t quot;

    if (divisor & 0x8000)
        divisor = (uint16_t)-divisor;
    if (hi & 0x8000) {
        uint32_t v = (uint32_t)-((uint32_t)hi << 16 | lo);
        hi = (uint16_t)(v >> 16);
        lo = (uint16_t)v;
    }
    quot = divide_passes((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), divisor, remainder);
    if (sign & 0x8000)
        quot = (uint16_t)-quot;
    return quot;
}

void abi_FxDivideSigned(void)
{
    uint16_t hi = gsu.r[5], lo = gsu.r[4], d = gsu.r[6];
    uint16_t sign = hi ^ d;
    uint16_t rem, quot = FxDivideSigned(hi, lo, d, &rem);

    if (d & 0x8000)
        d = (uint16_t)-d;
    if (hi & 0x8000) {
        uint32_t v = (uint32_t)-((uint32_t)hi << 16 | lo);
        hi = (uint16_t)(v >> 16);
        lo = (uint16_t)v;
    }
    divide_flags((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), d);
    gsu_moves(sign);
    if (sign & 0x8000)
        gsu_sz(quot);
    gsu.r[0] = rem;
    gsu.r[4] = quot;
    gsu.r[5] = hi;
    gsu.r[6] = d;
    gsu.r[7] = sign;
    gsu.r[12] = 0;
    gsu.r[13] = 0x81A3;
}

/*
 * @routine 01:81CB FxDivide32By16
 * WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
 * unsigned: R4 = the quotient, R0 = the remainder.
 * HOW: The shared loop (no CACHE); LSR in the return's delay slot undoes
 * the remainder's extra shift. 01:81BC branches here for dividends and
 * divisors below $8000.
 * Entry: R0:R4 = dividend, R6 = divisor; R11 = return address. Exit: R4 =
 * quotient, R0 = remainder; R12 = 0; R13 = $81D2 (the loop); S clear, Z
 * and CY from the LSR; OV from the last pass.
 */
uint16_t FxDivide32By16(uint16_t hi, uint16_t lo, uint16_t divisor, uint16_t *remainder)
{
    uint16_t rem, quot = divide_passes((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), divisor, &rem);

    *remainder = rem >> 1;
    return quot;
}

void abi_FxDivide32By16(void)
{
    uint16_t hi = gsu.r[0], lo = gsu.r[4];
    uint16_t rem, quot = FxDivide32By16(hi, lo, gsu.r[6], &rem);
    uint16_t shifted;

    divide_flags((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), gsu.r[6]);
    divide_passes((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), gsu.r[6], &shifted);
    gsu.cy = shifted & 1;
    gsu_sz(rem);
    gsu.r[0] = rem;
    gsu.r[4] = quot;
    gsu.r[12] = 0;
    gsu.r[13] = 0x81D2;
}

/*
 * @routine 01:96C0 FxDivideHalving
 * WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
 * unsigned, halving both first when either has its top bit set: R4 = the
 * quotient, R0 = the remainder (of the halved values).
 * HOW: MOVES tests R0 and R6; when either is negative, LSR R6 and the
 * 32-bit LSR/ROR of R0:R4 halve them. Then its own copy of the shared
 * loop, with the closing LSR.
 * Entry: R0:R4 = dividend, R6 = divisor; R11 = return address. Exit: R4 =
 * quotient, R0 = remainder; R6 = the divisor as used; R12 = 0; R13 =
 * $96D6 (the loop); S clear, Z and CY from the LSR; OV from the last pass.
 */
uint16_t FxDivideHalving(uint16_t hi, uint16_t lo, uint16_t *divisor, uint16_t *remainder)
{
    if ((hi | *divisor) & 0x8000) {
        *divisor >>= 1;
        lo = (uint16_t)(lo >> 1 | (hi & 1) << 15);
        hi >>= 1;
    }
    return FxDivide32By16(hi, lo, *divisor, remainder);
}

void abi_FxDivideHalving(void)
{
    uint16_t hi = gsu.r[0], lo = gsu.r[4], d = gsu.r[6];
    uint16_t rem, quot = FxDivideHalving(hi, lo, &d, &rem);
    uint16_t shifted;

    if ((hi | gsu.r[6]) & 0x8000) {
        lo = (uint16_t)(lo >> 1 | (hi & 1) << 15);
        hi >>= 1;
    }
    divide_flags((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), d);
    divide_passes((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), d, &shifted);
    gsu.cy = shifted & 1;
    gsu_sz(rem);
    gsu.r[0] = rem;
    gsu.r[4] = quot;
    gsu.r[6] = d;
    gsu.r[12] = 0;
    gsu.r[13] = 0x96D6;
}

/*
 * @routine 01:8096 FxSqrt32
 * WHAT IT DOES: The integer square root of the 32-bit value R4:R5 (R4 the
 * high word): R6 = the root, R8 = the remainder.
 * HOW: 16 passes, each shifting the next two bits of R4:R5 into R8 and
 * doubling R6; when R8 exceeds twice R6 (R7), R8 loses twice R6 plus 1 and
 * R6 gains 1. The pass loop starts at $80A0, the ADD R5 after the first
 * WITH R5: LOOP's delay slot is the WITH R5 for the next pass, and the
 * JMP R11 after the last pass runs with it pending.
 * Entry: R4:R5 = value; R11 = return address. Exit: R6 = root, R8 =
 * remainder; R4 = R5 = 0; R7 = twice the root before its last bit; R0 =
 * R7 - R8 as the last pass compared them; R12 = 0; R13 = $80A0; Z set, S
 * clear; CY and OV from the last pass's SUB R8, or from its SBC R7 when the
 * root gained a bit.
 */
uint16_t FxSqrt32(uint16_t hi, uint16_t lo, uint16_t *remainder)
{
    uint32_t v = (uint32_t)hi << 16 | lo;
    uint16_t root = 0, rem = 0;
    int n;

    for (n = 0; n < 16; n++) {
        uint16_t twice;

        rem = (uint16_t)(rem << 2 | v >> 30);
        v <<= 2;
        root = (uint16_t)(root << 1);
        twice = (uint16_t)(root << 1);
        if (twice < rem) {
            rem = (uint16_t)(rem - twice - 1);
            root++;
        }
    }
    *remainder = rem;
    return root;
}

void abi_FxSqrt32(void)
{
    uint16_t rem, root = FxSqrt32(gsu.r[4], gsu.r[5], &rem);
    uint16_t twice = (uint16_t)((root & 0xFFFE) << 1);

    if (root & 1) {
        /* SUB R8 borrowed; SBC R7 took twice + 1 off R8. */
        uint16_t before = (uint16_t)(rem + twice + 1);
        gsu.r[0] = (uint16_t)(twice - before);
        gsu_sub(before, twice, 1);
    } else {
        gsu.r[0] = gsu_sub(twice, rem, 0);
    }
    gsu.z = 1;
    gsu.s = 0;
    gsu.r[4] = 0;
    gsu.r[5] = 0;
    gsu.r[6] = root;
    gsu.r[7] = twice;
    gsu.r[8] = rem;
    gsu.r[12] = 0;
    gsu.r[13] = 0x80A0;
}
