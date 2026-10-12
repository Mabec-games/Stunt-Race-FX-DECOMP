/*
 * rotate.c: rotating a 2D vector by an 8-bit angle with the S-CPU's
 * hardware multiplier.
 */
#include "machine.h"
#include "hw.h"

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
