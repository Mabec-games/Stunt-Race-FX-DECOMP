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
#include "gsu_ops.h"

uint16_t FxDivideHalving(uint16_t hi, uint16_t lo, uint16_t *divisor, uint16_t *remainder, int copy);

/* The 16 passes, from the dividend as it stands after the first shift,
   with the flags their instructions leave: OV from the last pass's SUB,
   or from the ADD R4 after its restoring ADD; CY from its last ROL of the
   high word. Each way through a pass ends in its own LOOP, with the ROL
   of the high word in its delay slot, as in the original. Returns the
   quotient; *rem_out = the high word after the last pass. */
/* gsuwalk: the code of each copy of divide_passes, read from the ROM */
static const GsuCode divide_passes_fetch[3][10] = {
    {{0x01819c, 8}, {0x0181a4, 7}, {0x0181a3, 1}, {0x0181a4, 10}, {0x0181b3, 1}, {0x0181a4, 3}, {0x0181ae, 5}, {0x0181a3, 1}, {0x0181a4, 3}, {0x0181ae, 6}},
    {{0x0181cc, 7}, {0x0181d3, 7}, {0x0181d2, 1}, {0x0181d3, 9}, {0, 0}, {0x0181d3, 3}, {0x0181dc, 5}, {0x0181d2, 1}, {0x0181d3, 3}, {0x0181dc, 7}},
    {{0x0196d0, 7}, {0x0196d7, 7}, {0x0196d6, 1}, {0x0196d7, 9}, {0, 0}, {0x0196d7, 3}, {0x0196e0, 5}, {0x0196d6, 1}, {0x0196d7, 3}, {0x0196e0, 7}},
};

static uint16_t divide_passes(uint16_t rem, uint16_t quot, uint16_t divisor, uint16_t *rem_out, int copy)
{
    int n = 0;

    gsu_codes(divide_passes_fetch[copy], 1);
    for (;;) {
        uint16_t out = quot >> 15;
        uint16_t diff = gsu_sub(rem, divisor, 0);
        int again;

        if (gsu.cy) {
            rem = diff;
            quot = (uint16_t)(quot << 1 | 1);           /* WITH R4; ROL */
            again = ++n < 16;                           /* LOOP */
            gsu.cy = (uint8_t)(rem >> 15);              /* ROL */
            rem = (uint16_t)(rem << 1 | out);
            if (!again)
                { gsu_codes(divide_passes_fetch[copy] + 3, 2); break; } else { gsu_codes(divide_passes_fetch[copy] + 1, 2); }
        } else {
            quot = gsu_add(quot, quot, 0);              /* ADD R6; WITH R4; ADD R4 */
            again = ++n < 16;                           /* LOOP */
            gsu.cy = (uint8_t)(rem >> 15);              /* ROL */
            rem = (uint16_t)(rem << 1 | out);
            if (!again)
                { gsu_codes(divide_passes_fetch[copy] + 8, 2); break; } else { gsu_codes(divide_passes_fetch[copy] + 5, 3); }
        }
    }
    *rem_out = rem;
    return quot;
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
uint16_t FxDivideSigned(uint16_t hi, uint16_t lo, uint16_t divisor, uint16_t *remainder, int copy)
{
    uint16_t sign = hi ^ divisor;
    uint16_t quot;

    if (divisor & 0x8000)
        { divisor = (uint16_t)-divisor; gsu_code(0x018187, 12); } else { gsu_code(0x018187, 10); gsu_code(0x018192, 1); }
    if (hi & 0x8000) {
        uint32_t v = (uint32_t)-((uint32_t)hi << 16 | lo);
        hi = (uint16_t)(v >> 16);
        lo = (uint16_t)v;
        gsu_code(0x018193, 9);
    } else { gsu_code(0x018193, 4); gsu_code(0x01819b, 1); }
    quot = divide_passes((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), divisor, remainder, GSU_COPY(0));
    if (sign & 0x8000)
        { quot = (uint16_t)-quot; gsu_code(0x0181b4, 7); } else { gsu_code(0x0181b4, 4); gsu_code(0x0181ba, 1); }
    gsu_code(0x0181bb, 1);
    return quot;
}

void abi_FxDivideSigned(void)
{
    uint16_t hi = gsu.r[5], lo = gsu.r[4], d = gsu.r[6];
    uint16_t sign = hi ^ d;
    uint16_t rem, quot = FxDivideSigned(hi, lo, d, &rem, GSU_COPY(0));

    if (d & 0x8000)
        d = (uint16_t)-d;
    if (hi & 0x8000) {
        uint32_t v = (uint32_t)-((uint32_t)hi << 16 | lo);
        hi = (uint16_t)(v >> 16);
        lo = (uint16_t)v;
    }
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
uint16_t FxDivide32By16(uint16_t hi, uint16_t lo, uint16_t divisor, uint16_t *remainder, int copy)
{
    uint16_t rem, quot = divide_passes((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), divisor, &rem, GSU_COPY(1));

    *remainder = rem >> 1;
    return quot;
}

/* The passes and the LSR in the return's delay slot, leaving the
   registers and flags as the original: R4 = the quotient, R0 = the
   remainder; R12 = 0, R13 = the loop's address; OV from the last pass;
   CY, S and Z from the LSR. */
/* gsuwalk: the copy of divide_passes each copy of divide_exit calls */
static const uint8_t divide_passes_in_divide_exit[2] = {1, 2};

static uint16_t divide_exit(uint16_t hi, uint16_t lo, uint16_t d, uint16_t loop, int copy)
{
    uint16_t rem, quot = divide_passes((uint16_t)(hi << 1 | lo >> 15), (uint16_t)(lo << 1), d, &rem, GSU_COPY(divide_passes_in_divide_exit[copy]));

    gsu.cy = rem & 1;
    rem >>= 1;
    gsu_sz(rem);
    gsu.r[0] = rem;
    gsu.r[4] = quot;
    gsu.r[12] = 0;
    gsu.r[13] = loop;
    return quot;
}

void abi_FxDivide32By16(void)
{
    divide_exit(gsu.r[0], gsu.r[4], gsu.r[6], 0x81D2, GSU_COPY(0));
}

/* A halving divide (FxDivide32By16Halving, and FxDivideHalving's copy of
   it with the loop at another address), leaving the registers and flags
   as the original: as divide_exit, with R6 = the divisor as used. */
/* gsuwalk: the code of each copy of halving_divide, read from the ROM */
static const GsuCode halving_divide_fetch[2][7] = {
    {{0x0181bd, 4}, {0x0181c6, 1}, {0x0181c6, 1}, {0x0181bd, 9}, {0x0181bd, 9}, {0x0181cb, 1}, {0x0181c7, 5}},
    {{0x0196c1, 4}, {0x0196ca, 1}, {0x0196ca, 1}, {0x0196c1, 9}, {0x0196c1, 9}, {0x0196cf, 1}, {0x0196cb, 5}},
};

/* gsuwalk: the copy of divide_exit each copy of halving_divide calls */
static const uint8_t divide_exit_in_halving_divide[2] = {0, 1};

static uint16_t halving_divide(uint16_t hi, uint16_t lo, uint16_t d, uint16_t loop, int copy)
{
    gsu_moves(hi);
    if (!gsu.s) {
        gsu_moves(d);
        if (!gsu.s)
            { gsu_codes(halving_divide_fetch[copy] + 4, 2); goto divide; } else { gsu_codes(halving_divide_fetch[copy] + 3, 1); }                     /* BPL to the divide */
        gsu_codes(halving_divide_fetch[copy] + 2, 1);
    } else { gsu_codes(halving_divide_fetch[copy], 2); }
    d = gsu_lsr(d);                          /* the halving: LSR R6, LSR R0, ROR R4 */
    hi = gsu_lsr(hi);
    lo = gsu_ror(lo);
gsu_codes(halving_divide_fetch[copy] + 6, 1);
divide:
    gsu.r[6] = d;
    return divide_exit(hi, lo, d, loop, GSU_COPY(divide_exit_in_halving_divide[copy]));
}

/*
 * @routine 01:81BC FxDivide32By16Halving
 * WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
 * unsigned, halving both first when either has its top bit set: R4 = the
 * quotient, R0 = the remainder (of the halved values).
 * HOW: MOVES tests R0 and R6; when either is negative, LSR R6 and the
 * 32-bit LSR/ROR of R0:R4 halve them; then on into FxDivide32By16. The
 * halving ($81C6-$81CA) is not reached in the traces so far: every call
 * seen has both values below $8000.
 * Entry: R0:R4 = dividend, R6 = divisor; R11 = return address. Exit: as
 * FxDivide32By16; R6 = the divisor as used.
 */
uint16_t FxDivide32By16Halving(uint16_t hi, uint16_t lo, uint16_t *divisor, uint16_t *remainder, int copy)
{
    gsu_moves(hi);
    if (!gsu.s) {
        gsu_moves(*divisor);
        if (!gsu.s)
            goto divide;                     /* BPL to the divide */
    }
    *divisor = gsu_lsr(*divisor);            /* the halving: LSR R6, LSR R0, ROR R4 */
    hi = gsu_lsr(hi);
    lo = gsu_ror(lo);
divide:
    return FxDivide32By16(hi, lo, *divisor, remainder, GSU_COPY(0));
}

void abi_FxDivide32By16Halving(void)
{
    halving_divide(gsu.r[0], gsu.r[4], gsu.r[6], 0x81D2, GSU_COPY(0));
}

/*
 * @routine 01:96C0 FxDivideHalving
 * WHAT IT DOES: Divides the 32-bit value R0:R4 (R0 the high word) by R6,
 * unsigned, halving both first when either has its top bit set: R4 = the
 * quotient, R0 = the remainder (of the halved values).
 * HOW: FxDivide32By16Halving's code again, with its own copy of the loop.
 * Entry: R0:R4 = dividend, R6 = divisor; R11 = return address. Exit: R4 =
 * quotient, R0 = remainder; R6 = the divisor as used; R12 = 0; R13 =
 * $96D6 (the loop); S clear, Z and CY from the LSR; OV from the last pass.
 */
uint16_t FxDivideHalving(uint16_t hi, uint16_t lo, uint16_t *divisor, uint16_t *remainder, int copy)
{
    return FxDivide32By16Halving(hi, lo, divisor, remainder, GSU_COPY(0));
}

void abi_FxDivideHalving(void)
{
    halving_divide(gsu.r[0], gsu.r[4], gsu.r[6], 0x96D6, GSU_COPY(1));
}

/*
 * @routine 01:9658 FxRatioClamp
 * WHAT IT DOES: Turns the pair (R1, R2), each halved, into the ratios
 * R1 * 256 / R3 and R2 * 256 / R3; when the first would pass $3FFF it is
 * held at $3FFF and the second scaled to keep the pair's ratio.
 * HOW: Pushes R11 on the R10 stack; ASR R1, ASR R2. When R1's high byte
 * is below R3, FxDivideHalving divides R1 << 8 by R3; if that is at most
 * $3FFF, R2 << 8 is divided by R3 too and the routine returns. Otherwise
 * (R1's high byte at least R3, or a first quotient over $3FFF) R2 is made
 * R2 * $3FFF / R1 ((R2 << 14) - R2, then FxDivideHalving; R1 and R2 here
 * the halved values or the two quotients) and R1 = $3FFF. Returns by
 * pulling R11's value into R15.
 * Entry: R1, R2 = the pair, R3 = the divisor, R10 = the stack; R11 =
 * return address. Exit: R1, R2 = the results; R10 as at entry; R11 = the
 * return address of the last call; R0, R4, R6, R12, R13, OV and CY as the
 * last FxDivideHalving leaves them; S and Z from R10; the last RAM address
 * R10.
 */
void FxRatioClamp(uint16_t *x, uint16_t *y, uint16_t d, uint16_t *ret, int copy)
{
    uint16_t a, b;

    gsu_code(0x019659, 2);
    gsu_push(&gsu.r[10], gsu.r[11]);            /* the return address */
    a = (uint16_t)((int16_t)*x >> 1);
    b = (uint16_t)((int16_t)*y >> 1);

    /* each division leaves R0, R4, R6, R12, R13 and the flags; R11 is
       the return address of the last call (LINK) */
    if ((a >> 8) < d) {
        *ret = 0x9672;
        gsu_code(0x01965b, 23); gsu_code(0x0196c0, 1);
        a = halving_divide(a >> 8, (uint16_t)(a << 8), d, 0x96D6, GSU_COPY(1));
        if (a <= 0x3FFF) {
            *ret = 0x9688;
            gsu_code(0x019672, 22); gsu_code(0x0196c0, 1);
            b = halving_divide(b >> 8, (uint16_t)(b << 8), d, 0x96D6, GSU_COPY(1));
            *x = a;
            *y = b;
            gsu_code(0x019688, 7);
            gsu_pull(&gsu.r[10]);               /* the return address, into R15 */
            return;
        } else { gsu_code(0x019672, 7); gsu_code(0x01968f, 1); }
        *ret = 0x969E;
        gsu_code(0x019690, 14); gsu_code(0x0196c0, 1);
        b = halving_divide(b >> 8, (uint16_t)(b << 8), d, 0x96D6, GSU_COPY(1));
        gsu_code(0x01969e, 3);
    } else { gsu_code(0x01965b, 16); gsu_code(0x0196a0, 1); }
    {
        uint32_t v = ((uint32_t)b << 14) - b;

        *ret = 0x96B6;
        gsu_code(0x0196a1, 21); gsu_code(0x0196c0, 1);
        *y = halving_divide((uint16_t)(v >> 16), (uint16_t)v, a, 0x96D6, GSU_COPY(1));
        *x = 0x3FFF;
    }
    gsu_code(0x0196b6, 10);
    gsu_pull(&gsu.r[10]);                       /* the return address, into R15 */
}

void abi_FxRatioClamp(void)
{
    uint16_t ret;

    FxRatioClamp(&gsu.r[1], &gsu.r[2], gsu.r[3], &ret, GSU_COPY(0));
    gsu.r[11] = ret;
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
uint16_t FxSqrt32(uint16_t hi, uint16_t lo, uint16_t *remainder, int copy)
{
    uint32_t v = (uint32_t)hi << 16 | lo;
    uint16_t root = 0, rem = 0;
    int n;

    /* the passes, LOOP at the bottom of each */
    gsu_code(0x018097, 11);
    for (n = 0;;) {
        uint16_t twice;

        rem = (uint16_t)(rem << 2 | v >> 30);
        v <<= 2;
        root = (uint16_t)(root << 1);
        twice = (uint16_t)(root << 1);
        if (twice < rem) {
            rem = (uint16_t)(rem - twice - 1);
            root++;
            gsu_code(0x0180a2, 23);
        } else { gsu_code(0x0180a2, 19); gsu_code(0x0180b8, 1); }
        if (++n == 16)
            { gsu_code(0x0180b9, 1); break; } else { gsu_code(0x0180b9, 1); gsu_code(0x0180a0, 1); }
        gsu_code(0x0180a1, 1);
    }
    *remainder = rem;
    gsu_code(0x0180ba, 2);
    return root;
}

/* The registers and flags FxSqrt32 leaves, from its root and remainder. */
static void sqrt32_leave(uint16_t root, uint16_t rem)
{
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

void abi_FxSqrt32(void)
{
    uint16_t rem, root = FxSqrt32(gsu.r[4], gsu.r[5], &rem, GSU_COPY(0));

    sqrt32_leave(root, rem);
}

#define FX_STACK 0x0240u    /* where each program starts the R10 stack */
#define FX_LENGTH_IN 0x0068u
#define FX_LENGTH_IN2 0x002Eu
#define FX_LENGTH_OUT 0x006Au

/*
 * @routine 01:80BC FxLengthProgram
 * WHAT IT DOES: A program the S-CPU starts: the length of the vector
 * ($0068, $002E), stored at $006A.
 * HOW: Sets the R10 stack to $0240; squares each word with LMULT (signed,
 * R4 the low word), adds the two 32-bit squares (ADD, ADC) and takes
 * FxSqrt32 of the sum; SM stores the root. Then STOP.
 * Entry: started by the S-CPU. Exit: (at the STOP) R10 = $0240; R11 =
 * $80DF; the registers and flags as FxSqrt32 leaves them; the last RAM
 * address $006A.
 * Returns the sum of the squares; *root = the length.
 */
uint32_t FxLengthProgram(uint16_t *root, uint16_t *rem)
{
    gsu_code(0x0180bd, 7);
    int16_t x = (int16_t)gsu_ldw(FX_LENGTH_IN);
    gsu_code(0x0180c4, 5);
    uint32_t xx = gsu_lmult((uint16_t)x, (uint16_t)x);      /* LMULT: R8:R4 */
    gsu_code(0x0180c9, 6);
    int16_t z = (int16_t)gsu_ldw(FX_LENGTH_IN2);
    gsu_code(0x0180cf, 4);
    uint32_t sum = xx + gsu_lmult((uint16_t)z, (uint16_t)z);  /* LMULT, ADD, ADC */

    gsu_code(0x0180d3, 12); gsu_code(0x018096, 1);
    *root = FxSqrt32((uint16_t)(sum >> 16), (uint16_t)sum, rem, GSU_COPY(0));
    gsu_code(0x0180df, 5);
    gsu_stw(FX_LENGTH_OUT, *root);
    return sum;
}

void abi_FxLengthProgram(void)
{
    uint16_t root, rem;

    FxLengthProgram(&root, &rem);
    sqrt32_leave(root, rem);                  /* the registers as FxSqrt32 left them */
    gsu.r[10] = FX_STACK;
    gsu.r[11] = 0x80DF;
}
