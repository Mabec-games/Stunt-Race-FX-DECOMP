/*
 * rotate.c: rotating a 2D vector by an 8-bit angle with the S-CPU's
 * hardware multiplier.
 */
#include "machine.h"
#include "hw.h"
#include "cpu_ops.h"

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
    wr8(WRMPYA, (uint8_t)(a << 1));
    cyc(24);
    b = rd8(at);
    nb = (b & 0x80) != 0;
    if (nb) {
        cyc(76);                                /* BMI, EOR, INC, STA */
        b = (uint8_t)-b;
    } else {
        cyc(40);                                /* BMI, STA */
    }
    wr8(WRMPYB, b);
    if (na == nb) {
        cyc(80);
        h = rd8(RDMPYH);
        cyc(24);                                /* JMP */
        return h;
    }
    cyc(na ? 92 : 82);
    h = rd8(RDMPYH);
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
    r = smul(t, 0x000625);
    cyc(30);                                    /* TAX, STX */
    wr8(out, r);
    cyc(16);
    t = rd8(va);
    r = smul(t, 0x000624);
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
    r = smul(t, 0x000624);
    cyc(74 + w2);                               /* TAX, TXA, EOR, INC, STA */
    wr8(out2, (uint8_t)-r);
    cyc(16);
    t = rd8(va);
    r = smul(t, 0x000625);
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
    wr8(0x000625, rd8(0x00973E + i));
    cyc(32 + ((0xFE + i) > 0xFF ? 6 : 0));
    wr8(0x000624, cyc_then(24, rd8(0x0096FE + i)));
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
    return rotate8(0x000002, 0x000094, 0x000004, 0x0005F2);
}

void abi_RotateXZ(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | RotateXZ((uint8_t)cpu.a));
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.y) : nz16(cpu.p, cpu.y);
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
    return rotate8(0x000008, 0x000094, 0x00000A, 0x0005F2);
}

void abi_RotateXZFrom08(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | RotateXZFrom08((uint8_t)cpu.a));
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.x) : nz16(cpu.p, cpu.x);
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
    return rotate8(0x000002, 0x000008, 0x000004, 0x00000A);
}

void abi_RotateXY(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | RotateXY((uint8_t)cpu.a));
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.y) : nz16(cpu.p, cpu.y);
}


/*
 * @routine 02:DF99 MulWordByte
 * WHAT IT DOES: Multiplies the signed word in direct page $67 by the signed
 * byte at $69 times 2, keeping the product's top 16 of 24 bits: $6A = v *
 * 2s / 256. Nothing in the game calls it.
 * HOW: The hardware multiplier, on magnitudes: 2|s| times the low byte,
 * then (after reading the high byte of that product) times the high byte;
 * the two products are added, or subtracted from 0 when the signs differ.
 * The multiplier's registers through the data bank; $6C-$6D are cleared
 * and used as the zero to add to.
 * Entry: M 8-bit, X 8-bit, D = $0000. Exit: M 8-bit; A = the product
 * (both bytes); Y = the first product's high byte; N, Z, C, V from the
 * last add or subtract.
 */
void abi_MulWordByte(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t a, m, y;
    uint16_t w, n;
    uint32_t pre, st;

    cyc(16);
    wr8(0x00006C, 0x00);
    cyc(16);
    wr8(0x00006D, 0x00);
    cyc(16);
    wr8(0x00006B, 0x00);
    cyc(16);
    a = rd8(0x000069);
    cpu.p = nz8(cpu.p, a);
    if (a & 0x80)
        goto neg_s;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
    a = (uint8_t)(a << 1);
    cyc(54);
    wr8(b | 0x4202, a);
    cyc(16);
    a = rd8(0x000068);
    cpu.p = nz8(cpu.p, a);
    if (a & 0x80)
        goto pos_s_neg_v;
    cyc(32);
    a = rd8(0x000067);
    cyc(24);
    wr8(b | 0x4203, a);
    cyc(44);
    a = rd8(0x000068);
    pre = 48;                           /* JMP, LDY */
    st = 16;
    goto add;
pos_s_neg_v:
    cpu.p |= P_C;
    cyc(68);
    m = rd8(0x000067);
    a = op_sbc8(0x00, m, &cpu.p);
    cyc(8);
    wr8(b | 0x4203, a);
    cyc(32);
    m = rd8(0x000068);
    a = op_sbc8(0x00, m, &cpu.p);
    pre = 32;                           /* what op_sbc8 does not state: JMP, LDY */
    st = 34;
    goto sub;
neg_s:
    a = (uint8_t)(~a + 1);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
    a = (uint8_t)(a << 1);
    cyc(90);
    wr8(b | 0x4202, a);
    cyc(16);
    a = rd8(0x000068);
    cpu.p = nz8(cpu.p, a);
    if (a & 0x80)
        goto neg_s_neg_v;
    cyc(32);
    a = rd8(0x000067);
    cyc(24);
    wr8(b | 0x4203, a);
    cyc(58);
    a = rd8(0x000068);
    pre = 24;                           /* LDY */
    st = 34;
    goto sub;
neg_s_neg_v:
    cpu.p |= P_C;
    cyc(68);
    m = rd8(0x000067);
    a = op_sbc8(0x00, m, &cpu.p);
    cyc(8);
    wr8(b | 0x4203, a);
    cyc(60);
    m = rd8(0x000068);
    a = op_sbc8(0x00, m, &cpu.p);
    pre = 8;                            /* LDY, less what op_sbc8 states */
    st = 16;
    goto add;
sub:
    cyc(pre);
    y = rd8(b | 0x4217);
    cyc(24);
    wr8(b | 0x4203, a);
    cyc(16);
    wr8(0x00006A, y);
    cyc(38);
    w = rd16(0x00006C);
    cyc(38);
    n = rd16(b | 0x4216);
    cpu.p |= P_C;
    w = op_sbc16(w, n, &cpu.p);
    cyc(24);
    n = rd16(0x00006A);
    cpu.p |= P_C;
    w = op_sbc16(w, n, &cpu.p);
    goto store;
add:
    cyc(pre);
    y = rd8(b | 0x4217);
    cyc(24);
    wr8(b | 0x4203, a);
    cyc(16);
    wr8(0x00006A, y);
    cyc(38);
    w = rd16(0x00006C);
    cyc(24);
    n = rd16(b | 0x4216);
    cpu.p &= (uint8_t)~P_C;
    w = op_adc16(w, n, &cpu.p);
    cyc(30);
    n = rd16(0x00006A);
    cpu.p &= (uint8_t)~P_C;
    w = op_adc16(w, n, &cpu.p);
store:
    cyc(st);
    wr16(0x00006A, w);
    cyc(64);
    cpu.a = w;
    cpu.y = y;
    cpu.p |= P_M;
}

/*
 * @routine 02:DF95 MulWordByteLong
 * WHAT IT DOES: MulWordByte for callers in other banks.
 * HOW: JSR, RTL.
 * Entry: M 8-bit, X 8-bit, D = $0000. Exit: as MulWordByte.
 */
void abi_MulWordByteLong(void)
{
    (cyc(46), abi_MulWordByte());
    cyc(44);                            /* RTL */
}

/* One MulWordByte: $67-$68 = the word at v, $69 = the byte at f. */
static uint16_t mul_at(uint16_t v, uint16_t f)
{
    uint8_t t;

    cyc(16);
    t = rd8(v);
    cyc(16);
    wr8(0x000067, t);
    cyc(16);
    t = rd8((uint16_t)(v + 1));
    cyc(16);
    wr8(0x000068, t);
    cyc(24);
    t = rd8(f);
    cyc(16);
    wr8(0x000069, t);
    (cyc(62), abi_MulWordByteLong());
    return cpu.a;
}

/* The shared body of RotateXZWide and RotateXZWideFrom08: x (the word at
   xa) and z ($94) rotated by sin and cos of the angle; z' = x sin + z cos
   to $05F2, x' = x cos - z sin to the word at out. Data bank 0 inside. */
static void rotate_wide(uint8_t i, uint16_t xa, uint16_t out)
{
    uint8_t t, v;

    cyc(186);
    t = rd8(0x0096FE + i);
    wr8(0x0005F6, t);
    t = rd8(0x00973E + i);
    cyc(0x3E + i > 0xFF ? 62 : 56);     /* LDA abs,X (6 more over a page), STA */
    wr8(0x0005F7, t);
    mul_at(xa, 0x05F6);
    cyc(16);
    t = rd8(0x00006A);
    cyc(24);
    wr8(0x0005F2, t);
    cyc(16);
    t = rd8(0x00006B);
    cyc(24);
    wr8(0x0005F3, t);
    mul_at(0x0094, 0x05F7);
    cpu.p &= (uint8_t)~P_C;
    cyc(16);
    t = rd8(0x00006A);
    cyc(38);
    v = rd8(0x0005F2);
    t = op_adc8(t, v, &cpu.p);
    cyc(24);
    wr8(0x0005F2, t);
    cyc(16);
    t = rd8(0x00006B);
    cyc(24);
    v = rd8(0x0005F3);
    t = op_adc8(t, v, &cpu.p);
    cyc(24);
    wr8(0x0005F3, t);
    mul_at(0x0094, 0x05F6);
    cyc(16);
    t = rd8(0x00006A);
    cyc(16);
    wr8(out, t);
    cyc(16);
    t = rd8(0x00006B);
    cyc(16);
    wr8((uint16_t)(out + 1), t);
    mul_at(xa, 0x05F7);
    cpu.p |= P_C;
    cyc(16);
    t = rd8(0x00006A);
    cyc(30);
    v = rd8(out);
    t = op_sbc8(t, v, &cpu.p);
    wr8(out, t);
    cyc(16);
    t = rd8(0x00006B);
    cyc(16);
    v = rd8((uint16_t)(out + 1));
    t = op_sbc8(t, v, &cpu.p);
    wr8((uint16_t)(out + 1), t);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | t);
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
 * Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = x''s high
 * byte; X, Y, DB, P unchanged (X and Y reloaded from $3E and $40).
 */
void abi_RotateXZWide(void)
{
    uint8_t p = cpu.p, db = cpu.db;

    if (p & P_X) {
        cyc(16);
        wr8(0x00003E, (uint8_t)cpu.x);
        cyc(16);
        wr8(0x000040, (uint8_t)cpu.y);
    } else {
        cyc(16);
        wr16(0x00003E, cpu.x);
        cyc(16);
        wr16(0x000040, cpu.y);
    }
    op_sep(P_X);
    cpu.db = 0x00;
    rotate_wide((uint8_t)cpu.a, 0x0002, 0x0004);
    cpu.db = db;
    op_plp(p);
    cpu.y = (p & P_X) ? rd8(cyc_then(30, 0x000040)) : rd16(cyc_then(30, 0x000040));
    cpu.x = (p & P_X) ? rd8(cyc_then(16, 0x00003E)) : rd16(cyc_then(16, 0x00003E));
    cpu.p = (p & P_X) ? (cyc(44), nz8(p, (uint8_t)cpu.x)) : (cyc(44), nz16(p, cpu.x));
}

/*
 * @routine 03:C788 RotateXZWideFrom08
 * WHAT IT DOES: As RotateXZWide for the vector (x, z) in direct page $08
 * and $94 and the angle -A: z' to $05F2, x' to direct page $0A. Nothing
 * in the game calls it.
 * HOW: As RotateXZWide, the angle negated first (EOR #$FF, INC).
 * Entry: M 8-bit, D = $0000; A = angle. Exit: A low byte = x''s high
 * byte; X, Y, DB, P unchanged (X and Y reloaded from $3E and $40).
 */
void abi_RotateXZWideFrom08(void)
{
    uint8_t p = cpu.p, db = cpu.db;

    if (p & P_X) {
        cyc(16);
        wr8(0x00003E, (uint8_t)cpu.x);
        cyc(16);
        wr8(0x000040, (uint8_t)cpu.y);
    } else {
        cyc(16);
        wr16(0x00003E, cpu.x);
        cyc(16);
        wr16(0x000040, cpu.y);
    }
    cyc(30);
    op_sep(P_X);
    cpu.db = 0x00;
    rotate_wide((uint8_t)(~(uint8_t)cpu.a + 1), 0x0008, 0x000A);
    cpu.db = db;
    op_plp(p);
    cpu.y = (p & P_X) ? rd8(cyc_then(30, 0x000040)) : rd16(cyc_then(30, 0x000040));
    cpu.x = (p & P_X) ? rd8(cyc_then(16, 0x00003E)) : rd16(cyc_then(16, 0x00003E));
    cpu.p = (p & P_X) ? (cyc(44), nz8(p, (uint8_t)cpu.x)) : (cyc(44), nz16(p, cpu.x));
}
