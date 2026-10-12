/*
 * scale.c: fixed scalings of signed 16-bit values.
 */
#include "machine.h"
#include "hw.h"

/* Direct page $3E: scratch the routines below leave behind. */
#define SCALE_SCRATCH 0x00003Eu

/* An arithmetic shift right by one, as CMP #$8000 / ROR A does it: the sign
   bit stays, the low bit falls out into carry. */
static uint16_t asr1(uint16_t v)
{
    return (uint16_t)((v >> 1) | (v & 0x8000));
}

/*
 * @routine 03:CC5A ScaleThreeQuarters
 * WHAT IT DOES: Returns three quarters of a signed 16-bit value.
 * HOW: Halves the value (arithmetic shift), keeps the half in direct page
 * $3E, halves it again and adds the two: v/2 + v/4, each rounded toward
 * minus infinity.
 * Entry: M 16-bit, D = $0000; A = value.
 * Exit: A = the result; $3E = v/2; C and V from the add; N and Z from A.
 */
uint16_t ScaleThreeQuarters(uint16_t v)
{
    uint16_t half = asr1(v);

    wr16(SCALE_SCRATCH, half);
    return (uint16_t)(half + asr1(half));
}

void abi_ScaleThreeQuarters(void)
{
    uint16_t half = asr1(cpu.a), quarter = asr1(half);
    uint32_t sum = (uint32_t)half + quarter;
    uint16_t r = ScaleThreeQuarters(cpu.a);

    cpu.a = r;
    cpu.p = nz16(cpu.p, r);
    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_V)) | (sum > 0xFFFF ? P_C : 0)
                      | ((~(half ^ quarter) & (half ^ r) & 0x8000) ? P_V : 0));
}

/*
 * @routine 03:FCE0 Mode7ScaleProduct
 * WHAT IT DOES: Divides the signed value A by the byte at $7F:340A and
 * multiplies the quotient by the byte at $7F:3408 with the PPU's signed
 * multiplier, returning the low 16 bits of the product.
 * HOW: Carry clear means A is negative: its magnitude is divided and the
 * quotient negated. The quotient goes to M7A (low then high byte), the
 * multiplier to M7B; MPYL/MPYM is read back.
 * Entry: M 16-bit; A = value; C = 1 when A is to be taken as positive.
 * Exit: A = the product's low word; M 16-bit; N, Z from A; C, V unchanged.
 */
uint16_t Mode7ScaleProduct(uint16_t value, int positive)
{
    uint16_t q;

    wr16(WRDIVL, positive ? value : (uint16_t)-value);
    wr8(WRDIVB, rd8(0x7F340A));
    q = rd16(RDDIVL);
    if (!positive)
        q = (uint16_t)-q;
    wr8(M7A, (uint8_t)q);
    wr8(M7A, (uint8_t)(q >> 8));
    wr8(M7B, rd8(0x7F3408));
    return rd16(MPYL);
}

void abi_Mode7ScaleProduct(void)
{
    cpu.a = Mode7ScaleProduct(cpu.a, cpu.p & P_C);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
}

/*
 * @routine 08:83F8 ScaleDp02Dp94
 * WHAT IT DOES: Rescales the pair in direct page $02 and $94: $02 becomes
 * ($02 >> 10, signed) + $20 with bit 0 cleared, $94 becomes (its top five
 * bits >> 5, signed) + $400; returns their sum.
 * HOW: Arithmetic shifts by CMP #$8000 / ROR A; the high byte of $02 >> 2
 * is taken and sign-extended.
 * Entry: M 16-bit, D = $0000. Exit: A = the sum; N, Z, C, V from the last
 * add.
 */
uint16_t ScaleDp02Dp94(uint8_t *p)
{
    uint16_t a = asr1(asr1(rd16(0x000002))), b, s;

    a = (uint16_t)(int16_t)(int8_t)(uint8_t)(a >> 8);
    a = (uint16_t)((uint16_t)(a + 0x20) & 0xFFFE);
    wr16(0x000002, a);
    b = (uint16_t)((int16_t)(rd16(0x000094) & 0xF800) >> 5);
    b = (uint16_t)(b + 0x0400);
    wr16(0x000094, b);
    s = (uint16_t)(b + a);
    *p = (uint8_t)((nz16(*p, s) & ~(P_C | P_V)) | ((unsigned)b + a > 0xFFFF ? P_C : 0)
                   | ((~(b ^ a) & (b ^ s) & 0x8000) ? P_V : 0));
    return s;
}

void abi_ScaleDp02Dp94(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleDp02Dp94(&p);
    cpu.p = p;
}

void abi_ScaleDp02Dp94(void);

/*
 * @routine 08:83E8 ScaleDp02Dp94Long
 * WHAT IT DOES: ScaleDp02Dp94 for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as ScaleDp02Dp94. Exit: as ScaleDp02Dp94.
 */
uint16_t ScaleDp02Dp94Long(uint8_t *p)
{
    return ScaleDp02Dp94(p);
}

void abi_ScaleDp02Dp94Long(void)
{
    abi_ScaleDp02Dp94();
}

/*
 * @routine 03:CC0D ScaleFiveThirtySeconds
 * WHAT IT DOES: Returns v/8 + v/32 of a signed 16-bit value (5/32), keeping
 * v/8 at $1F2B.
 * HOW: Arithmetic shifts by CMP #$8000 / ROR A (rounding toward minus
 * infinity), then an add.
 * Entry: M 16-bit; A = value. Exit: A = the result; $1F2B = v/8; N, Z, C, V
 * from the add.
 */
uint16_t ScaleFiveThirtySeconds(uint8_t bank, uint16_t v, uint8_t *p)
{
    uint16_t e = asr1(asr1(asr1(v))), t = asr1(asr1(e)), s = (uint16_t)(t + e);

    wr16((uint32_t)bank << 16 | 0x1F2B, e);
    *p = (uint8_t)((nz16(*p, s) & ~(P_C | P_V)) | ((unsigned)t + e > 0xFFFF ? P_C : 0)
                   | ((~(t ^ e) & (t ^ s) & 0x8000) ? P_V : 0));
    return s;
}

void abi_ScaleFiveThirtySeconds(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleFiveThirtySeconds(cpu.db, cpu.a, &p);
    cpu.p = p;
}
