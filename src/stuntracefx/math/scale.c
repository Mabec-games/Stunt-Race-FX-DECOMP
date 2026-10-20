/*
 * scale.c: fixed scalings of signed 16-bit values.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

/* Direct page $3E and $40: scratch the routines below leave behind. */
#define SCALE_SCRATCH 0x00003Eu
#define SCALE_SCRATCH2 0x000040u

/* An arithmetic shift right by one, as CMP #$8000 / ROR A does it: the sign
   bit stays, the low bit falls out into carry. */
static uint16_t asr1(uint16_t v)
{
    cyc(46);
    return (uint16_t)((v >> 1) | (v & 0x8000));
}

/* ScaleThreeQuarters with the flags of its add. */
static uint16_t three_quarters(uint16_t v, uint8_t *p)
{
    uint16_t half = asr1(v), quarter;

    cyc(8);
    wr16(SCALE_SCRATCH, half);
    cyc(2);
    quarter = asr1(half);
    *p &= (uint8_t)~P_C;
    cyc(20);
    return op_adc16(quarter, rd16(SCALE_SCRATCH), p);
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
    uint8_t p = 0;

    return cyc_then(44, three_quarters(v, &p));
}

void abi_ScaleThreeQuarters(void)
{
    uint8_t p = cpu.p;

    cpu.a = three_quarters(cpu.a, &p);
    cpu.p = p;
}

/*
 * @routine 03:CC29 ScaleQuarter
 * WHAT IT DOES: Returns a quarter of a signed 16-bit value.
 * HOW: Two arithmetic shifts right (CMP #$8000 / ROR A), rounding toward
 * minus infinity.
 * Entry: M 16-bit; A = value. Exit: A = v/4; N, Z from it; C = bit 1 of v;
 * V unchanged.
 */
uint16_t ScaleQuarter(uint16_t v, uint8_t *p)
{
    cyc(24);
    uint16_t r = asr1(asr1(v));

    *p = (uint8_t)((nz16(*p, r) & ~P_C) | (v >> 1 & 1));
    return r;
}

void abi_ScaleQuarter(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleQuarter(cpu.a, &p);
    cpu.p = p;
}

/*
 * @routine 03:CC32 ScaleNineSixteenths
 * WHAT IT DOES: Returns v/2 + v/16 of a signed 16-bit value (9/16).
 * HOW: Halves the value (arithmetic shift), keeps the half in $3E, halves
 * it three more times and adds $3E back.
 * Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E = v/2;
 * N, Z, C, V from the add.
 */
uint16_t ScaleNineSixteenths(uint16_t v, uint8_t *p)
{
    uint16_t half = asr1(v), sixteenth;

    cyc(6);
    wr16(SCALE_SCRATCH, half);
    sixteenth = asr1(asr1(asr1(half)));
    *p &= (uint8_t)~P_C;
    return op_adc16(sixteenth, rd16(SCALE_SCRATCH), p);
}

void abi_ScaleNineSixteenths(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleNineSixteenths(cpu.a, &p);
    cpu.p = p;
}

/*
 * @routine 03:CC48 ScaleFiveEighths
 * WHAT IT DOES: Returns v/2 + v/8 of a signed 16-bit value (5/8).
 * HOW: As ScaleNineSixteenths, with two halvings after the first instead
 * of three.
 * Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E = v/2;
 * N, Z, C, V from the add.
 */
uint16_t ScaleFiveEighths(uint16_t v, uint8_t *p)
{
    uint16_t half = asr1(v), eighth;

    cyc(6);
    wr16(SCALE_SCRATCH, half);
    eighth = asr1(asr1(half));
    *p &= (uint8_t)~P_C;
    cyc(10);
    return op_adc16(eighth, rd16(SCALE_SCRATCH), p);
}

void abi_ScaleFiveEighths(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleFiveEighths(cpu.a, &p);
    cpu.p = p;
}

/*
 * @routine 03:CC68 ScaleSevenEighths
 * WHAT IT DOES: Returns v/2 + v/4 + v/8 of a signed 16-bit value (7/8).
 * HOW: Halves three times, keeping v/2 in $3E and v/4 in $40, then adds
 * them to v/8 in that order.
 * Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E = v/2,
 * $40 = v/4; N, Z, C, V from the last add.
 */
uint16_t ScaleSevenEighths(uint16_t v, uint8_t *p)
{
    uint16_t half = asr1(v), quarter, r;

    cyc(6);
    wr16(SCALE_SCRATCH, half);
    quarter = asr1(half);
    cyc(6);
    wr16(SCALE_SCRATCH2, quarter);
    r = asr1(quarter);
    *p &= (uint8_t)~P_C;
    cyc(20);
    r = op_adc16(r, rd16(SCALE_SCRATCH), p);
    *p &= (uint8_t)~P_C;
    cyc(30);
    return op_adc16(r, rd16(SCALE_SCRATCH2), p);
}

void abi_ScaleSevenEighths(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleSevenEighths(cpu.a, &p);
    cpu.p = p;
}

/*
 * @routine 03:CC7F ScaleFifteenSixteenths
 * WHAT IT DOES: Returns v/2 + v/4 + v/8 + v/16 of a signed 16-bit value
 * (15/16).
 * HOW: Halves four times, keeping v/2 in $3E, v/4 in $40 and v/8 at $1F2B
 * (through the data bank), then adds v/8, v/2 and v/4 to v/16 in that
 * order.
 * Entry: M 16-bit, D = $0000; A = value. Exit: A = the result; $3E, $40,
 * $1F2B as above; N, Z, C, V from the last add.
 */
uint16_t ScaleFifteenSixteenths(uint8_t bank, uint16_t v, uint8_t *p)
{
    uint32_t eighth_at = (uint32_t)bank << 16 | 0x1F2B;
    uint16_t half = asr1(v), quarter, eighth, r;

    cyc(6);
    wr16(SCALE_SCRATCH, half);
    quarter = asr1(half);
    cyc(6);
    wr16(SCALE_SCRATCH2, quarter);
    eighth = asr1(quarter);
    cyc(14);
    wr16(eighth_at, eighth);
    r = asr1(eighth);
    *p &= (uint8_t)~P_C;
    cyc(28);
    r = op_adc16(r, rd16(eighth_at), p);
    *p &= (uint8_t)~P_C;
    cyc(30);
    r = op_adc16(r, rd16(SCALE_SCRATCH), p);
    *p &= (uint8_t)~P_C;
    cyc(30);
    return op_adc16(r, rd16(SCALE_SCRATCH2), p);
}

void abi_ScaleFifteenSixteenths(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleFifteenSixteenths(cpu.db, cpu.a, &p);
    cpu.p = p;
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

    cyc(40);
    wr16(WRDIVL, positive ? value : (cyc(44), (uint16_t)-value));
    cyc(54);
    wr8(WRDIVB, cyc_then(24, rd8(0x7F340A)));
    cyc(144);
    q = rd16(RDDIVL);
    if (!positive)
        { cyc(16); q = (uint16_t)-q; }
    cyc(68);
    wr8(M7A, (uint8_t)q);
    cyc(44);
    wr8(M7A, (uint8_t)(q >> 8));
    cyc(32);
    wr8(M7B, cyc_then(24, rd8(0x7F3408)));
    cyc(46);
    return cyc_then(42, rd16(MPYL));
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
    cyc(16);
    uint16_t a = asr1(asr1(rd16(0x000002))), b, s;

    /* XBA, BIT #, then ORA # and BRA (negative) or AND #, then CLC,
       ADC #, AND #, STA (the shifts' time is asr1's) */
    cyc((a & 0x8000) ? 164 : 148);
    a = (uint16_t)(int16_t)(int8_t)(uint8_t)(a >> 8);
    a = (uint16_t)((uint16_t)(a + 0x20) & 0xFFFE);
    cyc(4);
    wr16(0x000002, a);
    cyc(16);
    b = (uint16_t)((int16_t)(rd16(0x000094) & 0xF800) >> 5);
    b = (uint16_t)(b + 0x0400);
    cyc(268);
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

/*
 * @routine 08:83EC ScaleObjectXZ
 * WHAT IT DOES: ScaleDp02Dp94 on object X's X and Z position: $02 = its
 * +$0C, $94 = its +$10 (bank $7E), then rescaled.
 * HOW: Two long loads, then falls into ScaleDp02Dp94.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = object. Exit: as
 * ScaleDp02Dp94.
 */
uint16_t ScaleObjectXZ(uint16_t obj, uint8_t *p)
{
    cyc(32);
    wr16(0x000002, cyc_then(16, rd16(0x7E200C + obj)));
    cyc(32);
    wr16(0x000094, cyc_then(16, rd16(0x7E2010 + obj)));
    return ScaleDp02Dp94(p);
}

void abi_ScaleObjectXZ(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleObjectXZ(cpu.x, &p);
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
    cyc(46);
    return cyc_then(44, ScaleDp02Dp94(p));
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
    uint32_t eighth_at = (uint32_t)bank << 16 | 0x1F2B;
    uint16_t e = asr1(asr1(asr1(v))), t;

    cyc(0);
    wr16(eighth_at, e);
    t = asr1(asr1(e));
    *p &= (uint8_t)~P_C;
    cyc(22);
    return op_adc16(t, rd16(eighth_at), p);
}

void abi_ScaleFiveThirtySeconds(void)
{
    uint8_t p = cpu.p;

    cpu.a = ScaleFiveThirtySeconds(cpu.db, cpu.a, &p);
    cpu.p = p;
    cyc(44);
}

/* The four words at $7F:3432: base ($7F:3442) + (target ($7F:343A) - base)
   through Mode7ScaleProduct, i.e. scaled by $7F:3408 / $7F:340A. */
static void blend_words(void)
{
    unsigned i;

    for (i = 0; i < 4; i++) {
        cyc(32);
        cpu.a = rd16((uint32_t)(0x7F343A + i * 2));
        cpu.p |= P_C;
        cyc(46);
        cpu.a = op_sbc16(cpu.a, rd16((uint32_t)(0x7F3442 + i * 2)), &cpu.p);
        (cyc(40), abi_Mode7ScaleProduct());
        cpu.p &= (uint8_t)~P_C;
        cyc(46);
        cpu.a = op_adc16(cpu.a, rd16((uint32_t)(0x7F3442 + i * 2)), &cpu.p);
        cyc(32);
        wr16((uint32_t)(0x7F3432 + i * 2), cpu.a);
    }
    cyc(44);
}

/*
 * @routine 03:FD32 BlendStepUp
 * WHAT IT DOES: Unless the factor $7F:3408 has reached $7F:340A, steps it
 * up by one and sets the four words at $7F:3432 between their bases
 * ($7F:3442) and targets ($7F:343A) by it.
 * HOW: (target - base) through Mode7ScaleProduct, plus the base. The return
 * at the top is not reached in the traces so far.
 * Entry: any widths. Exit: M, X 16-bit; A = the last word; N, Z, C, V
 * from its add; or, at the top, A = the factor with Z and C set.
 */
void abi_BlendStepUp(void)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(54);
    cpu.a = rd16(0x7F3408);
    cyc(32);
    op_cmp16(rd16(0x7F340A));
    if (cpu.p & P_Z)
        { cyc(56); return; }
    cpu.a = (uint16_t)(cpu.a + 1);
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(68);
    wr16(0x7F3408, cpu.a);
    (cyc(22), blend_words());
}

/*
 * @routine 03:FD46 BlendStepDown
 * WHAT IT DOES: Unless the factor $7F:3408 is 0, steps it down by one and
 * sets the four words at $7F:3432 by it (as BlendStepUp).
 * HOW: As BlendStepUp. The return at the top is not reached in the traces
 * so far.
 * Entry: any widths. Exit: as BlendStepUp; or, at the top, A = 0 with Z
 * set.
 */
void abi_BlendStepDown(void)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(54);
    cpu.a = rd16(0x7F3408);
    cpu.p = nz16(cpu.p, cpu.a);
    if (cpu.p & P_Z)
        { cyc(60); return; }
    cpu.a = (uint16_t)(cpu.a - 1);
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(68);
    wr16(0x7F3408, cpu.a);
    blend_words();
}
