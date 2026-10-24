/*
 * scale.c: fixed scalings of signed 16-bit values.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "math_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"



/* An arithmetic shift right by one, as CMP #$8000 / ROR A does it: the sign
   bit stays, the low bit falls out into carry. */
static uint16_t asr1(uint16_t v)
{
    cyc(46);
    return (uint16_t)((v >> 1) | (v & 0x8000));
}

/* CLC / ADC (16-bit, binary): the sum, and N, Z, C, V in *p. */
static uint16_t adc16p(uint16_t a, uint16_t m, uint8_t *p)
{
    uint32_t r = (uint32_t)a + m;

    *p = (uint8_t)((*p & ~(P_N | P_Z | P_C | P_V)) | ((r & 0x8000) ? P_N : 0) |
                   ((r & 0xFFFF) == 0 ? P_Z : 0) | (r > 0xFFFF ? P_C : 0) |
                   ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    return (uint16_t)r;
}

/* ScaleThreeQuarters with the flags of its add. */
static uint16_t three_quarters(uint16_t v, uint8_t *p)
{
    uint16_t half = asr1(v), quarter;

    cyc(8);
    WR16(scale_scratch, half);
    cyc(2);
    quarter = asr1(half);
    *p &= (uint8_t)~P_C;
    cyc(20);
    return adc16p(quarter, RD16(scale_scratch), p);
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

void Mode7ScaleProduct_cr(Carried *cr);
void ScaleDp02Dp94_cr(Carried *cr);
void ScaleDp02Dp94Long_cr(Carried *cr);
void ScaleFifteenSixteenths_cr(Carried *cr);
void ScaleFiveEighths_cr(Carried *cr);
void ScaleFiveThirtySeconds_cr(Carried *cr);
void ScaleNineSixteenths_cr(Carried *cr);
void ScaleObjectXZ_cr(Carried *cr);
void ScaleQuarter_cr(Carried *cr);
void ScaleSevenEighths_cr(Carried *cr);
void ScaleThreeQuarters_cr(Carried *cr);
static void blend_regs(Carried *cr, uint16_t a);
void BlendStepUp_cr(Carried *cr);
void BlendStepDown_cr(Carried *cr);

void ScaleThreeQuarters_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = three_quarters(cr->a, &p);
    set_p(cr, p);
}

void abi_ScaleThreeQuarters(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleThreeQuarters_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void ScaleQuarter_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleQuarter(cr->a, &p);
    set_p(cr, p);
    cyc(4);
}

void abi_ScaleQuarter(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleQuarter_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(8);
    WR16(scale_scratch, half);
    sixteenth = asr1(asr1(asr1(half)));
    *p &= (uint8_t)~P_C;
    cyc(6);
    return adc16p(sixteenth, RD16(scale_scratch), p);
}

void ScaleNineSixteenths_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleNineSixteenths(cr->a, &p);
    set_p(cr, p);
    cyc(44);
}

void abi_ScaleNineSixteenths(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleNineSixteenths_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(8);
    WR16(scale_scratch, half);
    eighth = asr1(asr1(half));
    *p &= (uint8_t)~P_C;
    cyc(14);
    return adc16p(eighth, RD16(scale_scratch), p);
}

void ScaleFiveEighths_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleFiveEighths(cr->a, &p);
    set_p(cr, p);
    cyc(44);
}

void abi_ScaleFiveEighths(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleFiveEighths_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(8);
    WR16(scale_scratch, half);
    quarter = asr1(half);
    cyc(8);
    WR16(dp_40, quarter);
    r = asr1(quarter);
    *p &= (uint8_t)~P_C;
    cyc(22);
    r = adc16p(r, RD16(scale_scratch), p);
    *p &= (uint8_t)~P_C;
    cyc(30);
    return adc16p(r, RD16(dp_40), p);
}

void ScaleSevenEighths_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleSevenEighths(cr->a, &p);
    set_p(cr, p);
    cyc(44);
}

void abi_ScaleSevenEighths(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleSevenEighths_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    (void)bank;
    uint32_t eighth_at = WA(wram_1F2B);
    uint16_t half = asr1(v), quarter, eighth, r;

    cyc(8);
    WR16(scale_scratch, half);
    quarter = asr1(half);
    cyc(8);
    WR16(dp_40, quarter);
    eighth = asr1(quarter);
    cyc(16);
    wr16(eighth_at, eighth);
    r = asr1(eighth);
    *p &= (uint8_t)~P_C;
    cyc(30);
    r = adc16p(r, rd16(eighth_at), p);
    *p &= (uint8_t)~P_C;
    cyc(30);
    r = adc16p(r, RD16(scale_scratch), p);
    *p &= (uint8_t)~P_C;
    cyc(30);
    return adc16p(r, RD16(dp_40), p);
}

void ScaleFifteenSixteenths_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleFifteenSixteenths(cr->db, cr->a, &p);
    set_p(cr, p);
    cyc(44);
}

void abi_ScaleFifteenSixteenths(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleFifteenSixteenths_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint16_t q, n;
    uint8_t d, m;

    cyc(40);
    io_wr16(WRDIVL, n = positive ? value : (cyc(44), (uint16_t)-value));
    cyc(54);
    io_wr8(WRDIVB, d = cyc_then(24, RD8(scale_divisor)));
    cyc(144);
    q = io_rd16(RDDIVL, cpu_div(n, d));
    if (!positive)
        { cyc(16); q = (uint16_t)-q; }
    cyc(68);
    io_wr8(M7A, (uint8_t)q);
    cyc(44);
    io_wr8(M7A, (uint8_t)(q >> 8));
    ppu_mul_factor = (int16_t)q;
    cyc(32);
    io_wr8(M7B, m = cyc_then(24, RD8(fixed_colour_step)));
    cyc(46);
    return cyc_then(42, io_rd16(MPYL, (uint16_t)ppu_mul(m)));
}

void Mode7ScaleProduct_cr(Carried *cr)
{
    cr->a = Mode7ScaleProduct(cr->a, cr->p & P_C);
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) & ~P_M));
}

void abi_Mode7ScaleProduct(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Mode7ScaleProduct_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint16_t a = asr1(asr1(RD16(dp_02))), b, s;

    /* XBA, BIT #, then ORA # and BRA (negative) or AND #, then CLC,
       ADC #, AND #, STA (the shifts' time is asr1's) */
    cyc((a & 0x8000) ? 164 : 148);
    a = (uint16_t)(int16_t)(int8_t)(uint8_t)(a >> 8);
    a = (uint16_t)((uint16_t)(a + 0x20) & 0xFFFE);
    cyc(4);
    WR16(dp_02, a);
    cyc(16);
    b = (uint16_t)((int16_t)(RD16(dp_94) & 0xF800) >> 5);
    b = (uint16_t)(b + 0x0400);
    cyc(268);
    WR16(dp_94, b);
    s = (uint16_t)(b + a);
    *p = (uint8_t)((nz16(*p, s) & ~(P_C | P_V)) | ((unsigned)b + a > 0xFFFF ? P_C : 0)
                   | ((~(b ^ a) & (b ^ s) & 0x8000) ? P_V : 0));
    return s;
}

void ScaleDp02Dp94_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleDp02Dp94(&p);
    set_p(cr, p);
}

void abi_ScaleDp02Dp94(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleDp02Dp94_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    WR16(dp_02, cyc_then(16, RD16X(obj_pos_x_long, obj)));
    cyc(32);
    WR16(dp_94, cyc_then(16, RD16X(obj_pos_z_long, obj)));
    return ScaleDp02Dp94(p);
}

void ScaleObjectXZ_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleObjectXZ(cr->x, &p);
    set_p(cr, p);
}

void abi_ScaleObjectXZ(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleObjectXZ_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void ScaleDp02Dp94Long_cr(Carried *cr)
{
    ScaleDp02Dp94_cr(cr);
}

void abi_ScaleDp02Dp94Long(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleDp02Dp94Long_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    (void)bank;
    uint32_t eighth_at = WA(wram_1F2B);
    uint16_t e = asr1(asr1(asr1(v))), t;

    cyc(0);
    wr16(eighth_at, e);
    t = asr1(asr1(e));
    *p &= (uint8_t)~P_C;
    cyc(22);
    return adc16p(t, rd16(eighth_at), p);
}

void ScaleFiveThirtySeconds_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ScaleFiveThirtySeconds(cr->db, cr->a, &p);
    set_p(cr, p);
    cyc(44);
}

void abi_ScaleFiveThirtySeconds(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScaleFiveThirtySeconds_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The flags the blend routines leave, and which of N, Z, C, V they set
   (the rest are the caller's), for their adapters. */
static uint8_t blend_flags, blend_mask;

/* The four words at $7F:3432: base ($7F:3442) + (target ($7F:343A) - base)
   through Mode7ScaleProduct, i.e. scaled by $7F:3408 / $7F:340A. Returns
   the last word; the flags are its add's. */
static uint16_t blend_words(void)
{
    unsigned i;
    uint16_t a = 0, base, target;
    uint8_t p = 0;

    for (i = 0; i < 4; i++) {
        cyc(32);
        target = rd16((uint32_t)(W(scaled_words_target) + i * 2));
        cyc(46);
        base = rd16((uint32_t)(W(scaled_words_base) + i * 2));
        cyc(6);                                 /* SEC, SBC */
        a = (uint16_t)(target - base);
        cyc(40);
        a = Mode7ScaleProduct(a, target >= base);
        p = 0;
        cyc(46);
        a = adc16p(a, rd16((uint32_t)(W(scaled_words_base) + i * 2)), &p);
        cyc(32);
        wr16((uint32_t)(W(scaled_words) + i * 2), a);
    }
    cyc(44);
    blend_flags = (uint8_t)(p & (P_N | P_Z | P_C | P_V));
    blend_mask = P_N | P_Z | P_C | P_V;
    return a;
}

/* The blend routines' registers: M and X 16-bit, A, the flags they set. */
static void blend_regs(Carried *cr, uint16_t a)
{
    cr->a = a;
    set_p(cr, (uint8_t)((cr->p & ~(P_M | P_X | blend_mask)) | blend_flags));
}

/*
 * @routine 03:FD32 BlendStepUp
 * WHAT IT DOES: Unless the factor $7F:3408 has reached $7F:340A, steps it
 * up by one and sets the four words at $7F:3432 between their bases
 * ($7F:3442) and targets ($7F:343A) by it.
 * HOW: (target - base) through Mode7ScaleProduct, plus the base. The return
 * at the top is not reached in the traces so far.
 * Entry: any widths. Exit: M, X 16-bit; the return value is A = the last
 * word; N, Z, C, V from its add; or, at the top, A = the factor with Z and
 * C set (the adapter has the flags).
 */
uint16_t BlendStepUp(void)
{
    uint16_t a, limit;

    cyc(54);
    a = RD16(fixed_colour_step);
    cyc(32);
    limit = RD16(scale_divisor);
    if (a == limit) {
        blend_flags = P_Z | P_C;                /* CMP: equal */
        blend_mask = P_N | P_Z | P_C;
        cyc(60);
        return a;
    }
    a = (uint16_t)(a + 1);
    cyc(68);
    WR16(fixed_colour_step, a);
    cyc(22);
    return blend_words();
}

void abi_BlendStepUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BlendStepUp_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* BlendStepUp on carried registers, as abi_BlendStepUp leaves them. */
void BlendStepUp_cr(Carried *cr)
{
    blend_regs(cr, BlendStepUp());
}

/*
 * @routine 03:FD46 BlendStepDown
 * WHAT IT DOES: Unless the factor $7F:3408 is 0, steps it down by one and
 * sets the four words at $7F:3432 by it (as BlendStepUp).
 * HOW: As BlendStepUp. The return at the top is not reached in the traces
 * so far.
 * Entry: any widths. Exit: as BlendStepUp; or, at the top, A = 0 with Z
 * set (and N clear).
 */
uint16_t BlendStepDown(void)
{
    uint16_t a;

    cyc(54);
    a = RD16(fixed_colour_step);
    if (a == 0) {
        blend_flags = P_Z;
        blend_mask = P_N | P_Z;
        cyc(60);
        return a;
    }
    a = (uint16_t)(a - 1);
    cyc(68);
    WR16(fixed_colour_step, a);
    return blend_words();
}

void abi_BlendStepDown(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BlendStepDown_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* BlendStepDown on carried registers, as abi_BlendStepDown leaves them. */
void BlendStepDown_cr(Carried *cr)
{
    blend_regs(cr, BlendStepDown());
}

