/*
 * rotate.c: rotating a 2D vector by an 8-bit angle with the S-CPU's
 * hardware multiplier.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "math_calls.h"
#include "carried.h"
#include "wram_names.h"


/*
 * Signed 8 x 8 multiply through the unsigned multiplier: the magnitudes are
 * multiplied (the first doubled) and the high byte of the product taken,
 * then negated when the signs differ. The second factor is read from work
 * RAM at `at` after WRMPYA is written, as the assembly loads it. The time
 * is the assembly's, path by path (master clocks: a ROM byte 8, an
 * internal cycle 6): BMI 16 not taken, 22 taken; EOR #, LDA # 16; INC A,
 * ASL A, NOP, SEC 14; STA, LDA, SBC absolute 24 before the access; JMP 24.
 * Between WRMPYB and RDMPYH: four NOPs and LDA (80) when the signs agree;
 * when they differ, NOP NOP (or NOP and a JMP when the first is negative)
 * and LDA #0, SEC, SBC (82, or 92). A sign that agrees jumps on (24) after
 * the read.
 */
static uint8_t smul(uint8_t a, uint32_t at)
{
    uint8_t b, h;
    int na = (a & 0x80) != 0, nb;

    if (na) {
        cyc(90);                                /* BMI, EOR, INC, ASL, STA */
        a = (uint8_t)-a;
    } else {
        cyc(54);                                /* BMI, ASL, STA */
    }
    io_wr8(WRMPYA, (uint8_t)(a << 1));
    cyc(24);
    b = rd8(at);
    nb = (b & 0x80) != 0;
    if (nb) {
        cyc(76);                                /* BMI, EOR, INC, STA */
        b = (uint8_t)-b;
    } else {
        cyc(40);                                /* BMI, STA */
    }
    io_wr8(WRMPYB, b);
    if (na == nb) {
        cyc(80);
        h = io_rd8(RDMPYH, (uint8_t)(cpu_mul((uint8_t)(a << 1), b) >> 8));
        cyc(24);                                /* JMP */
        return h;
    }
    cyc(na ? 92 : 82);
    h = io_rd8(RDMPYH, (uint8_t)(cpu_mul((uint8_t)(a << 1), b) >> 8));
    return (uint8_t)-h;
}

/*
 * The shared body of RotateXZ, RotateXZFrom08 and RotateXY, with cos and
 * sin already kept at $0625 and $0624: x (the byte at xa) and the second
 * coordinate v (at va) rotated: x cos + v sin to the word at out, v cos -
 * x sin to the word at out2, each sign-extended. Every factor is read
 * from work RAM right before its multiply, as the assembly does. Returns
 * A: the high byte of the second result, or its low byte when that is 0.
 * The time is the assembly's (see smul): TAX, TXA, CLC 14; BIT # 16; BEQ
 * 16 or 22; BRA 22; direct page 16 before the access, absolute 24 (out2
 * is absolute, $05F2, in RotateXZ and RotateXZFrom08: 8 more each time).
 */
static uint8_t rotate8(uint32_t xa, uint32_t va, uint32_t out, uint32_t out2)
{
    uint8_t t, r, hi;
    uint32_t w2 = out2 > 0xFF ? 8 : 0;

    cyc(16);
    t = rd8(xa);
    r = smul(t, W(rot_cos));
    cyc(30);                                    /* TAX, STX */
    wr8(out, r);
    cyc(16);
    t = rd8(va);
    r = smul(t, W(rot_sin));
    cyc(58);                                    /* TAX, TXA, CLC, ADC */
    t = rd8(out);
    r = (uint8_t)(r + t);
    cyc(16);
    wr8(out, r);
    if (r & 0x80) {
        cyc(64);                                /* BIT, BEQ, LDA #$FF, STA */
        wr8(out + 1, 0xFF);
        cyc(38);                                /* BRA, LDA */
    } else {
        cyc(54);                                /* BIT, BEQ, STZ */
        wr8(out + 1, 0x00);
        cyc(16);                                /* LDA */
    }
    t = rd8(xa);
    r = smul(t, W(rot_sin));
    cyc(74 + w2);                               /* TAX, TXA, EOR, INC, STA */
    wr8(out2, (uint8_t)-r);
    cyc(16);
    t = rd8(va);
    r = smul(t, W(rot_cos));
    cyc(58 + w2);                               /* TAX, TXA, CLC, ADC */
    t = rd8(out2);
    r = (uint8_t)(r + t);
    cyc(16 + w2);
    wr8(out2, r);
    hi = (r & 0x80) ? 0xFF : 0x00;
    cyc(hi ? 64 + w2 : 54 + w2);                /* BIT, BEQ, then LDA #$FF and STA, or STZ */
    wr8(out2 + 1, hi);
    cyc(172);
    return hi ? (cyc(22), hi) : r;
}

/*
 * The angle's cos and sin from the byte tables at $00:973E and $00:96FE
 * to $0625 and $0624: LDA abs,X (24, the ROM byte 8, and 6 more when the
 * 8-bit index crosses a page), STA abs (24), each. The caller's time runs
 * through the cos STA; only the cos load's page crossing is stated here.
 */
static void sin_cos(uint8_t i)
{
    cyc((0x3E + i) > 0xFF ? 6 : 0);
    wr8(W(rot_cos), rd8(0x00973E + i));
    cyc(32 + ((0xFE + i) > 0xFF ? 6 : 0));
    wr8(W(rot_sin), cyc_then(24, rd8(0x0096FE + i)));
}

/*
 * @routine 03:C27F RotateXZ
 * WHAT IT DOES: Rotates the vector (x, z) in direct page $02 and $94 by the
 * angle A: x' = x cos + z sin goes to direct page $04, z' = z cos - x sin to
 * $05F2, each sign-extended to 16 bits.
 * HOW: cos and sin of -A come from the byte tables at $00:973E and $00:96FE
 * (kept at $0625 and $0624); four signed multiplies via smul, added in 8
 * bits. The data bank is 0 inside.
 * Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = the high byte of
 * z'; A high byte, X, Y unchanged; P unchanged except N, Z, which the final
 * pull of Y sets from Y.
 */
uint8_t RotateXZ(uint8_t angle)
{
    uint8_t i = (uint8_t)-angle;

    cyc(292);
    sin_cos(i);
    return rotate8(W(dp_02), W(dp_94), W(dp_04), W(rot_z));
}

void MulWordByte_cr(Carried *cr);
void MulWordByteLong_cr(Carried *cr);
void RotateXY_cr(Carried *cr);
void RotateXZ_cr(Carried *cr);
void RotateXZFrom08_cr(Carried *cr);
void RotateXZWide_cr(Carried *cr);
void RotateXZWideFrom08_cr(Carried *cr);
static void mulw_regs(Carried *cr, uint16_t w, uint8_t y);
static void wide_regs(Carried *cr, uint8_t p, uint16_t a, uint16_t x, uint16_t y);

void RotateXZ_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | RotateXZ((uint8_t)cr->a));
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->y) : nz16(cr->p, cr->y));
}

void abi_RotateXZ(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RotateXZ_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C3FF RotateXZFrom08
 * WHAT IT DOES: As RotateXZ, for the vector (x, z) in direct page $08 and
 * $94 and the angle A itself (not negated): x' = x cos + z sin goes to
 * direct page $0A, z' = z cos - x sin to $05F2, each sign-extended to 16
 * bits.
 * HOW: cos and sin of A from the byte tables at $00:973E and $00:96FE
 * (kept at $0625 and $0624); four signed multiplies via smul, added in 8
 * bits. The data bank is 0 inside.
 * Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = the high byte of
 * z'; A high byte, X, Y unchanged; P unchanged except N, Z, which the final
 * pull of X sets from X.
 */
uint8_t RotateXZFrom08(uint8_t angle)
{
    cyc(262);
    sin_cos(angle);
    return rotate8(W(dp_08), W(dp_94), W(dp_0a), W(rot_z));
}

void RotateXZFrom08_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | RotateXZFrom08((uint8_t)cr->a));
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->x) : nz16(cr->p, cr->x));
}

void abi_RotateXZFrom08(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RotateXZFrom08_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C57C RotateXY
 * WHAT IT DOES: Rotates the vector (x, y) in direct page $02 and $08 by the
 * angle A: x' = x cos + y sin goes to direct page $04, y' = y cos - x sin to
 * direct page $0A, each sign-extended to 16 bits.
 * HOW: As RotateXZFrom08 (cos and sin of A, four signed multiplies via
 * smul, added in 8 bits, data bank 0 inside).
 * Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = the high byte of
 * y'; A high byte, X, Y unchanged; P unchanged except N, Z, which the final
 * pull of Y sets from Y.
 */
uint8_t RotateXY(uint8_t angle)
{
    cyc(262);
    sin_cos(angle);
    return rotate8(W(dp_02), W(dp_08), W(dp_04), W(dp_0a));
}

void RotateXY_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | RotateXY((uint8_t)cr->a));
    set_p(cr, (cr->p & P_X) ? nz8(cr->p, (uint8_t)cr->y) : nz16(cr->p, cr->y));
}

void abi_RotateXY(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RotateXY_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* CLC / ADC and SEC / SBC as the binary instructions make them: the
   result and the flags N, Z, C, V in *fl. The time is the instruction's:
   the 16-bit subtract states 6 clocks of it, the 8-bit one 16. */
static uint16_t add16(uint16_t a, uint16_t m, uint8_t *fl)
{
    uint32_t r = (uint32_t)a + m;

    *fl = (uint8_t)(((r & 0x8000) ? P_N : 0) | ((r & 0xFFFF) == 0 ? P_Z : 0) |
                    (r > 0xFFFF ? P_C : 0) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    return (uint16_t)r;
}

static uint16_t sub16(uint16_t a, uint16_t m, uint8_t *fl)
{
    uint16_t nm = (uint16_t)~m;
    uint32_t r = (uint32_t)a + nm + 1;

    *fl = (uint8_t)(((r & 0x8000) ? P_N : 0) | ((r & 0xFFFF) == 0 ? P_Z : 0) |
                    (r > 0xFFFF ? P_C : 0) | ((~(a ^ nm) & (a ^ r) & 0x8000) ? P_V : 0));
    cyc(6);
    return (uint16_t)r;
}

/* SBC with the carry given (1: SEC before it); *carry is the carry after. */
static uint8_t sub8(uint8_t a, uint8_t m, unsigned *carry)
{
    unsigned r = (unsigned)a + (uint8_t)~m + *carry;

    cyc(16);
    *carry = r > 0xFF;
    return (uint8_t)r;
}

/* The flags MulWordByte leaves (N, Z, C, V of its last add or subtract),
   for its adapter. */
static uint8_t mulw_flags;

/*
 * @routine 02:DF99 MulWordByte
 * WHAT IT DOES: Multiplies the signed word in direct page $67 by the signed
 * byte at $69 times 2, keeping the product's top 16 of 24 bits: $6A = v *
 * 2s / 256. Nothing in the game calls it.
 * HOW: The hardware multiplier, on magnitudes: 2|s| times the low byte,
 * then (after reading the high byte of that product) times the high byte;
 * the two products are added, or subtracted from 0 when the signs differ.
 * The multiplier's registers through the data bank (`bank`); $6C-$6D are
 * cleared and used as the zero to add to.
 * Entry: M 8-bit, X 8-bit, D = $0000; the data bank in `bank`. Exit: M
 * 8-bit; A = the product (both bytes, the return value); Y = the first
 * product's high byte (*yout); N, Z, C, V from the last add or subtract
 * (the adapter has them).
 */
uint16_t MulWordByte(uint8_t bank, uint8_t *yout)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a, m, y, mpya = 0;
    uint16_t prod = 0;
    uint16_t w, n;
    uint32_t pre, st;
    unsigned cy;

    cyc(16);
    wr8(W(mul_zero), 0x00);
    cyc(16);
    wr8(W(dp_6d), 0x00);
    cyc(16);
    wr8(W(dp_6b), 0x00);
    cyc(16);
    a = rd8(W(mul_byte));
    if (a & 0x80)
        goto neg_s;
    a = (uint8_t)(a << 1);
    cyc(54);
    io_wr8(b | 0x4202, mpya = a);
    cyc(16);
    a = rd8(W(fx_normal_x_w));
    if (a & 0x80)
        goto pos_s_neg_v;
    cyc(32);
    a = rd8(W(mul_word));
    cyc(24);
    io_wr8(b | 0x4203, a);
    prod = cpu_mul(mpya, a);
    cyc(44);
    a = rd8(W(fx_normal_x_w));
    pre = 48;                           /* JMP, LDY */
    st = 16;
    goto add;
pos_s_neg_v:
    cyc(68);
    m = rd8(W(mul_word));
    cy = 1;
    a = sub8(0x00, m, &cy);
    cyc(8);
    io_wr8(b | 0x4203, a);
    prod = cpu_mul(mpya, a);
    cyc(32);
    m = rd8(W(fx_normal_x_w));
    a = sub8(0x00, m, &cy);         /* the low byte's borrow */
    pre = 32;                           /* what sub8 does not state: JMP, LDY */
    st = 34;
    goto sub;
neg_s:
    a = (uint8_t)(~a + 1);
    a = (uint8_t)(a << 1);
    cyc(90);
    io_wr8(b | 0x4202, mpya = a);
    cyc(16);
    a = rd8(W(fx_normal_x_w));
    if (a & 0x80)
        goto neg_s_neg_v;
    cyc(32);
    a = rd8(W(mul_word));
    cyc(24);
    io_wr8(b | 0x4203, a);
    prod = cpu_mul(mpya, a);
    cyc(58);
    a = rd8(W(fx_normal_x_w));
    pre = 24;                           /* LDY */
    st = 34;
    goto sub;
neg_s_neg_v:
    cyc(68);
    m = rd8(W(mul_word));
    cy = 1;
    a = sub8(0x00, m, &cy);
    cyc(8);
    io_wr8(b | 0x4203, a);
    prod = cpu_mul(mpya, a);
    cyc(60);
    m = rd8(W(fx_normal_x_w));
    a = sub8(0x00, m, &cy);         /* the low byte's borrow */
    pre = 8;                            /* LDY, less what sub8 states */
    st = 16;
    goto add;
sub:
    cyc(pre);
    y = io_rd8(b | 0x4217, (uint8_t)(prod >> 8));
    cyc(24);
    io_wr8(b | 0x4203, a);
    prod = cpu_mul(mpya, a);
    cyc(16);
    wr8(W(mul_product), y);
    cyc(38);
    w = rd16(W(mul_zero));
    cyc(38);
    n = io_rd16(b | 0x4216, prod);
    w = sub16(w, n, &mulw_flags);
    cyc(24);
    n = rd16(W(mul_product));
    w = sub16(w, n, &mulw_flags);
    goto store;
add:
    cyc(pre);
    y = io_rd8(b | 0x4217, (uint8_t)(prod >> 8));
    cyc(24);
    io_wr8(b | 0x4203, a);
    prod = cpu_mul(mpya, a);
    cyc(16);
    wr8(W(mul_product), y);
    cyc(38);
    w = rd16(W(mul_zero));
    cyc(24);
    n = io_rd16(b | 0x4216, prod);
    w = add16(w, n, &mulw_flags);
    cyc(30);
    n = rd16(W(mul_product));
    w = add16(w, n, &mulw_flags);
store:
    cyc(st);
    wr16(W(mul_product), w);
    cyc(64);
    *yout = y;
    return w;
}

/* MulWordByte's exit in the registers: A the product, Y the high byte, M
   set, N, Z, C, V from its last add or subtract. */
static void mulw_regs(Carried *cr, uint16_t w, uint8_t y)
{
    cr->a = w;
    cr->y = y;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | mulw_flags | P_M));
}

void MulWordByte_cr(Carried *cr)
{
    uint8_t y;
    uint16_t w = MulWordByte(cr->db, &y);

    mulw_regs(cr, w, y);
}

void abi_MulWordByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MulWordByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:DF95 MulWordByteLong
 * WHAT IT DOES: MulWordByte for callers in other banks.
 * HOW: JSR, RTL.
 * Entry: M 8-bit, X 8-bit, D = $0000. Exit: as MulWordByte.
 */
uint16_t MulWordByteLong(uint8_t bank, uint8_t *yout)
{
    uint16_t w;

    cyc(46);
    w = MulWordByte(bank, yout);
    cyc(44);                            /* RTL */
    return w;
}

void MulWordByteLong_cr(Carried *cr)
{
    uint8_t y;
    uint16_t w = MulWordByteLong(cr->db, &y);

    mulw_regs(cr, w, y);
}

void abi_MulWordByteLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MulWordByteLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One MulWordByte: $67-$68 = the word at v, $69 = the byte at f. Data
   bank 0. Returns A. */
static uint16_t mul_at(uint16_t v, uint16_t f)
{
    uint8_t t, y;

    cyc(16);
    t = rd8(v);
    cyc(16);
    wr8(W(mul_word), t);
    cyc(16);
    t = rd8((uint16_t)(v + 1));
    cyc(16);
    wr8(W(fx_normal_x_w), t);
    cyc(24);
    t = rd8(f);
    cyc(16);
    wr8(W(mul_byte), t);
    cyc(62);
    return MulWordByteLong(0x00, &y);
}

/* The shared body of RotateXZWide and RotateXZWideFrom08: x (the word at
   xa) and z ($94) rotated by sin and cos of the angle; z' = x sin + z cos
   to $05F2, x' = x cos - z sin to the word at out. Data bank 0 inside.
   Returns A: the last product's high byte over x''s high byte. */
static uint16_t rotate_wide(uint8_t i, uint16_t xa, uint16_t out)
{
    uint8_t t, v;
    uint16_t w;
    unsigned c;

    cyc(186);
    t = rd8(0x0096FE + i);
    wr8(W(rot_wide_sin), t);
    t = rd8(0x00973E + i);
    cyc(0x3E + i > 0xFF ? 62 : 56);     /* LDA abs,X (6 more over a page), STA */
    wr8(W(rot_wide_cos), t);
    mul_at(xa, 0x05F6);
    cyc(16);
    t = rd8(W(mul_product));
    cyc(24);
    wr8(W(rot_z), t);
    cyc(16);
    t = rd8(W(dp_6b));
    cyc(24);
    wr8(W(rot_z_high), t);
    mul_at(0x0094, 0x05F7);
    cyc(16);
    t = rd8(W(mul_product));
    cyc(38);
    v = rd8(W(rot_z));
    c = (unsigned)t + v;                /* CLC, ADC */
    t = (uint8_t)c;
    cyc(24);
    wr8(W(rot_z), t);
    cyc(16);
    t = rd8(W(dp_6b));
    cyc(24);
    v = rd8(W(rot_z_high));
    t = (uint8_t)(t + v + (c > 0xFF));
    cyc(24);
    wr8(W(rot_z_high), t);
    mul_at(0x0094, 0x05F6);
    cyc(16);
    t = rd8(W(mul_product));
    cyc(16);
    wr8(out, t);
    cyc(16);
    t = rd8(W(dp_6b));
    cyc(16);
    wr8((uint16_t)(out + 1), t);
    w = mul_at(xa, 0x05F7);
    cyc(16);
    t = rd8(W(mul_product));
    cyc(30);
    v = rd8(out);
    c = 1;                              /* SEC, SBC */
    t = sub8(t, v, &c);
    wr8(out, t);
    cyc(16);
    t = rd8(W(dp_6b));
    cyc(16);
    v = rd8((uint16_t)(out + 1));
    t = sub8(t, v, &c);
    wr8((uint16_t)(out + 1), t);
    return (uint16_t)((w & 0xFF00) | t);
}

/* The wide rotations' entry and exit as C: X and Y (8 bits wide when
   `x8`) are kept in $3E and $40 across the work and come back from there;
   returns A (the last product's high byte, x''s high byte below). */
static uint16_t rotate_wide_keep(uint8_t i, uint16_t xa, uint16_t out, int x8,
                                 uint16_t *x, uint16_t *y, int extra)
{
    uint16_t a;

    if (x8) {
        cyc(16);
        wr8(W(dp_3e), (uint8_t)*x);
        cyc(16);
        wr8(W(dp_40), (uint8_t)*y);
    } else {
        cyc(16);
        wr16(W(dp_3e), *x);
        cyc(16);
        wr16(W(dp_40), *y);
    }
    if (extra)
        cyc(30);
    cyc(22);                            /* SEP #$10 */
    a = rotate_wide(i, xa, out);
    cyc(42);                            /* PLP */
    *y = x8 ? rd8(cyc_then(30, W(dp_40))) : rd16(cyc_then(30, W(dp_40)));
    *x = x8 ? rd8(cyc_then(16, W(dp_3e))) : rd16(cyc_then(16, W(dp_3e)));
    cyc(44);
    return a;
}

/* The wide rotations' registers: A as returned, X and Y the reloaded
   values, DB as it came, P as it came but N, Z from X. */
static void wide_regs(Carried *cr, uint8_t p, uint16_t a, uint16_t x, uint16_t y)
{
    cr->a = a;
    cr->x = x;
    cr->y = y;
    set_p(cr, (p & P_X) ? nz8(p, (uint8_t)x) : nz16(p, x));
}

/*
 * @routine 03:C6F4 RotateXZWide
 * WHAT IT DOES: Rotates the 16-bit vector (x, z) in direct page $02 and
 * $94 by the angle A: z' = x sin + z cos goes to $05F2, x' = x cos - z
 * sin to direct page $04, at 16-bit precision (RotateXZ works in 8 bits).
 * Nothing in the game calls it.
 * HOW: sin and cos of A from the byte tables at $00:96FE and $00:973E
 * (kept at $05F6 and $05F7); four MulWordByteLong products, added and
 * subtracted a byte at a time. X and Y are kept in direct page $3E and
 * $40; P and DB are saved and restored; the data bank is 0 inside.
 * Entry: M 8-bit, D = $0000; A = angle; *x and *y the index registers,
 * 8 bits wide when `x8`. Exit: the return value (A: x''s high byte below,
 * the last product's high byte above); *x, *y the reloaded X and Y; DB, P
 * unchanged but N, Z from X (the adapter has them).
 */
uint16_t RotateXZWide(uint8_t angle, int x8, uint16_t *x, uint16_t *y)
{
    return rotate_wide_keep(angle, 0x0002, 0x0004, x8, x, y, 0);
}

void RotateXZWide_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;
    uint8_t p = cr->p;

    cr->a = RotateXZWide((uint8_t)cr->a, (p & P_X) != 0, &x, &y);
    wide_regs(cr, p, cr->a, x, y);
}

void abi_RotateXZWide(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RotateXZWide_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:C788 RotateXZWideFrom08
 * WHAT IT DOES: As RotateXZWide for the vector (x, z) in direct page $08
 * and $94 and the angle -A: z' to $05F2, x' to direct page $0A. Nothing
 * in the game calls it.
 * HOW: As RotateXZWide, the angle negated first (EOR #$FF, INC).
 * Entry and exit: as RotateXZWide.
 */
uint16_t RotateXZWideFrom08(uint8_t angle, int x8, uint16_t *x, uint16_t *y)
{
    return rotate_wide_keep((uint8_t)(~angle + 1), 0x0008, 0x000A, x8, x, y, 1);
}

void RotateXZWideFrom08_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;
    uint8_t p = cr->p;

    cr->a = RotateXZWideFrom08((uint8_t)cr->a, (p & P_X) != 0, &x, &y);
    wide_regs(cr, p, cr->a, x, y);
}

void abi_RotateXZWideFrom08(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RotateXZWideFrom08_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

