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
 * then negated when the signs differ. Each path waits the multiplier's eight
 * cycles before reading RDMPYH.
 */
static uint8_t smul(uint8_t a, uint8_t b)
{
    uint8_t neg = 0, h;

    if (a & 0x80) {
        a = (uint8_t)-a;
        neg ^= 1;
    }
    wr8(WRMPYA, (uint8_t)(a << 1));
    if (b & 0x80) {
        b = (uint8_t)-b;
        neg ^= 1;
    }
    wr8(WRMPYB, b);
    h = rd8(RDMPYH);
    return neg ? (uint8_t)-h : h;
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
    uint8_t i = (uint8_t)-angle, c = rd8(0x00973E + i), s = rd8(0x0096FE + i), x = rd8(0x000002), z = rd8(0x000094);
    uint8_t r, hi;

    wr8(0x000625, c);
    wr8(0x000624, s);
    wr8(0x000004, smul(x, c));
    r = (uint8_t)(smul(z, s) + rd8(0x000004));
    wr8(0x000004, r);
    wr8(0x000005, (r & 0x80) ? 0xFF : 0x00);
    wr8(0x0005F2, (uint8_t)-smul(x, s));
    r = (uint8_t)(smul(z, c) + rd8(0x0005F2));
    wr8(0x0005F2, r);
    hi = (r & 0x80) ? 0xFF : 0x00;
    wr8(0x0005F3, hi);
    return hi ? hi : r;
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
    uint8_t c = rd8(0x00973E + angle), s = rd8(0x0096FE + angle), x = rd8(0x000008), z = rd8(0x000094);
    uint8_t r, hi;

    wr8(0x000625, c);
    wr8(0x000624, s);
    wr8(0x00000A, smul(x, c));
    r = (uint8_t)(smul(z, s) + rd8(0x00000A));
    wr8(0x00000A, r);
    wr8(0x00000B, (r & 0x80) ? 0xFF : 0x00);
    wr8(0x0005F2, (uint8_t)-smul(x, s));
    r = (uint8_t)(smul(z, c) + rd8(0x0005F2));
    wr8(0x0005F2, r);
    hi = (r & 0x80) ? 0xFF : 0x00;
    wr8(0x0005F3, hi);
    return hi ? hi : r;
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
    uint8_t c = rd8(0x00973E + angle), s = rd8(0x0096FE + angle), x = rd8(0x000002), y = rd8(0x000008);
    uint8_t r, hi;

    wr8(0x000625, c);
    wr8(0x000624, s);
    wr8(0x000004, smul(x, c));
    r = (uint8_t)(smul(y, s) + rd8(0x000004));
    wr8(0x000004, r);
    wr8(0x000005, (r & 0x80) ? 0xFF : 0x00);
    wr8(0x00000A, (uint8_t)-smul(x, s));
    r = (uint8_t)(smul(y, c) + rd8(0x00000A));
    wr8(0x00000A, r);
    hi = (r & 0x80) ? 0xFF : 0x00;
    wr8(0x00000B, hi);
    return hi ? hi : r;
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

    wr8(0x00006C, 0x00);
    wr8(0x00006D, 0x00);
    wr8(0x00006B, 0x00);
    a = rd8(0x000069);
    cpu.p = nz8(cpu.p, a);
    if (a & 0x80)
        goto neg_s;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
    a = (uint8_t)(a << 1);
    wr8(b | 0x4202, a);
    a = rd8(0x000068);
    cpu.p = nz8(cpu.p, a);
    if (a & 0x80)
        goto pos_s_neg_v;
    a = rd8(0x000067);
    wr8(b | 0x4203, a);
    a = rd8(0x000068);
    goto add;
pos_s_neg_v:
    cpu.p |= P_C;
    m = rd8(0x000067);
    a = op_sbc8(0x00, m, &cpu.p);
    wr8(b | 0x4203, a);
    m = rd8(0x000068);
    a = op_sbc8(0x00, m, &cpu.p);
    goto sub;
neg_s:
    a = (uint8_t)(~a + 1);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
    a = (uint8_t)(a << 1);
    wr8(b | 0x4202, a);
    a = rd8(0x000068);
    cpu.p = nz8(cpu.p, a);
    if (a & 0x80)
        goto neg_s_neg_v;
    a = rd8(0x000067);
    wr8(b | 0x4203, a);
    a = rd8(0x000068);
    goto sub;
neg_s_neg_v:
    cpu.p |= P_C;
    m = rd8(0x000067);
    a = op_sbc8(0x00, m, &cpu.p);
    wr8(b | 0x4203, a);
    m = rd8(0x000068);
    a = op_sbc8(0x00, m, &cpu.p);
    goto add;
sub:
    y = rd8(b | 0x4217);
    wr8(b | 0x4203, a);
    wr8(0x00006A, y);
    w = rd16(0x00006C);
    n = rd16(b | 0x4216);
    cpu.p |= P_C;
    w = op_sbc16(w, n, &cpu.p);
    n = rd16(0x00006A);
    cpu.p |= P_C;
    w = op_sbc16(w, n, &cpu.p);
    goto store;
add:
    y = rd8(b | 0x4217);
    wr8(b | 0x4203, a);
    wr8(0x00006A, y);
    w = rd16(0x00006C);
    n = rd16(b | 0x4216);
    cpu.p &= (uint8_t)~P_C;
    w = op_adc16(w, n, &cpu.p);
    n = rd16(0x00006A);
    cpu.p &= (uint8_t)~P_C;
    w = op_adc16(w, n, &cpu.p);
store:
    wr16(0x00006A, w);
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
    abi_MulWordByte();
}

/* One MulWordByte: $67-$68 = the word at v, $69 = the byte at f. */
static uint16_t mul_at(uint16_t v, uint16_t f)
{
    uint8_t t;

    t = rd8(v);
    wr8(0x000067, t);
    t = rd8((uint16_t)(v + 1));
    wr8(0x000068, t);
    t = rd8(f);
    wr8(0x000069, t);
    abi_MulWordByteLong();
    return cpu.a;
}

/* The shared body of RotateXZWide and RotateXZWideFrom08: x (the word at
   xa) and z ($94) rotated by sin and cos of the angle; z' = x sin + z cos
   to $05F2, x' = x cos - z sin to the word at out. Data bank 0 inside. */
static void rotate_wide(uint8_t i, uint16_t xa, uint16_t out)
{
    uint8_t t, v;

    t = rd8(0x0096FE + i);
    wr8(0x0005F6, t);
    t = rd8(0x00973E + i);
    wr8(0x0005F7, t);
    mul_at(xa, 0x05F6);
    t = rd8(0x00006A);
    wr8(0x0005F2, t);
    t = rd8(0x00006B);
    wr8(0x0005F3, t);
    mul_at(0x0094, 0x05F7);
    cpu.p &= (uint8_t)~P_C;
    t = rd8(0x00006A);
    v = rd8(0x0005F2);
    t = op_adc8(t, v, &cpu.p);
    wr8(0x0005F2, t);
    t = rd8(0x00006B);
    v = rd8(0x0005F3);
    t = op_adc8(t, v, &cpu.p);
    wr8(0x0005F3, t);
    mul_at(0x0094, 0x05F6);
    t = rd8(0x00006A);
    wr8(out, t);
    t = rd8(0x00006B);
    wr8((uint16_t)(out + 1), t);
    mul_at(xa, 0x05F7);
    cpu.p |= P_C;
    t = rd8(0x00006A);
    v = rd8(out);
    t = op_sbc8(t, v, &cpu.p);
    wr8(out, t);
    t = rd8(0x00006B);
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
        wr8(0x00003E, (uint8_t)cpu.x);
        wr8(0x000040, (uint8_t)cpu.y);
    } else {
        wr16(0x00003E, cpu.x);
        wr16(0x000040, cpu.y);
    }
    op_sep(P_X);
    cpu.db = 0x00;
    rotate_wide((uint8_t)cpu.a, 0x0002, 0x0004);
    cpu.db = db;
    op_plp(p);
    cpu.y = (p & P_X) ? rd8(0x000040) : rd16(0x000040);
    cpu.x = (p & P_X) ? rd8(0x00003E) : rd16(0x00003E);
    cpu.p = (p & P_X) ? nz8(p, (uint8_t)cpu.x) : nz16(p, cpu.x);
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
        wr8(0x00003E, (uint8_t)cpu.x);
        wr8(0x000040, (uint8_t)cpu.y);
    } else {
        wr16(0x00003E, cpu.x);
        wr16(0x000040, cpu.y);
    }
    op_sep(P_X);
    cpu.db = 0x00;
    rotate_wide((uint8_t)(~(uint8_t)cpu.a + 1), 0x0008, 0x000A);
    cpu.db = db;
    op_plp(p);
    cpu.y = (p & P_X) ? rd8(0x000040) : rd16(0x000040);
    cpu.x = (p & P_X) ? rd8(0x00003E) : rd16(0x00003E);
    cpu.p = (p & P_X) ? nz8(p, (uint8_t)cpu.x) : nz16(p, cpu.x);
}
