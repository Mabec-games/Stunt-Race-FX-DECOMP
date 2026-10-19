/*
 * debug_numbers.c: numbers printed with DebugDrawChar (debug_text.c) in
 * hexadecimal, decimal and binary. The long entries save every register
 * and P and run with the data bank 0; the byte to print is passed through
 * $06E7. No play runs them; unit_debug_numbers calls them.
 */
#include "machine.h"
#include "cpu_ops.h"

void abi_DebugDrawChar(void);

#define NUM_BYTE 0x0006E7u
#define NUM_VALUE 0x000042u
#define NUM_DIGIT 0x00003Eu
#define NUM_BITS 0x000040u
#define HEX_DIGITS 0x03D7B1u
#define POWERS_OF_TEN 0x03D57Au
#define DECIMAL_PAIRS 0x03D5EDu

/* DebugDrawChar with A = ch (the high byte as the caller's A has it). */
static void draw(uint16_t ch)
{
    cpu.a = ch;
    (cyc(76), abi_DebugDrawChar());
}

/*
 * @routine 03:D782 DebugHexByte
 * WHAT IT DOES: Prints the byte in A as two hexadecimal digits.
 * HOW: Each nibble indexes the ASCII digits at $03:D7B1 (a word read, its
 * low byte kept), less $20 for the font. 16-bit inside; P saved.
 * Entry: A low byte = the byte. Exit: P unchanged; A, X, Y as the second
 * DebugDrawChar leaves them.
 */
void DebugHexByte(void)
{
    uint8_t p = cpu.p;
    uint16_t a = cpu.a, d;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(248);
    d = rd16(HEX_DIGITS + ((a & 0x00F0) >> 4));
    draw((uint16_t)((d & 0x00FF) - 0x0020));
    cyc(224);
    d = rd16(HEX_DIGITS + (a & 0x000F));
    draw((uint16_t)((d & 0x00FF) - 0x0020));
    op_plp(p);
}

void abi_DebugHexByte(void)
{
    DebugHexByte();
}

/*
 * @routine 03:D59E DebugDecimalByte
 * WHAT IT DOES: Prints the byte in A (0-255) in decimal: the hundreds
 * digit only when there is one, the tens digit only when not 0 or after a
 * hundreds digit, then the units digit. The digits are drawn as the values
 * 0-9 (not the font's digit characters at $10).
 * HOW: 200 and 100 taken off by compares; the rest looked up in the
 * two-digit decimal table at $03:D5ED. The hundreds digit and the rest are
 * kept on the stack across the draws.
 * Entry: M 8-bit, X 16-bit; A = the byte. Exit: A, X, Y as the last
 * DebugDrawChar leaves them; N, Z from the units digit; C from the last
 * test of the tens (bit 3 of the pair, or no hundreds digit); V from the
 * subtract of 200 or 100 when there was one.
 */
void DebugDecimalByte(void)
{
    uint8_t v = (uint8_t)cpu.a, h, t, p = cpu.p;

    if (v >= 200) {
        p |= P_C;
        v = op_sbc8(v, 200, &p);
        h = 0x02;
    } else if (v >= 100) {
        cyc(38);
        p |= P_C;
        v = op_sbc8(v, 100, &p);
        h = 0x01;
    } else {
        h = 0xFF;
    }
    cyc(104);
    cpu.p = p;
    if (h != 0xFF)
        { cyc(26); draw((uint16_t)((cpu.a & 0xFF00) | h)); }
    cpu.y = h;
    cpu.x = v;
    cyc(366);
    t = rd8(DECIMAL_PAIRS + v);
    cpu.p = (uint8_t)((p & ~P_C) | ((t >> 4) ? (t >> 3 & 1) : (cyc(34), (h == 0xFF))));
    if ((t >> 4) || h != 0xFF)
        { cyc(34); draw((uint16_t)((cpu.a & 0xFF00) | (t >> 4))); }
    cyc(84);
    cpu.p = nz8(cpu.p, (uint8_t)(t & 0x0F));
    draw((uint16_t)(t & 0x0F));
}

void abi_DebugDecimalByte(void)
{
    DebugDecimalByte();
}

/*
 * @routine 03:D584 DebugSignedByte
 * WHAT IT DOES: Prints the signed byte in A: character $24 and the
 * magnitude when it is negative, else character $29 and the value
 * (DebugDecimalByte).
 * HOW: The byte kept on the stack across the sign's draw.
 * Entry: as DebugDecimalByte. Exit: as DebugDecimalByte.
 */
void DebugSignedByte(void)
{
    uint8_t v = (uint8_t)cpu.a;

    if (v & 0x80) {
        cyc(40);
        draw((uint16_t)((cpu.a & 0xFF00) | 0x24));
        v = (uint8_t)(~v + 1);
    } else {
        cyc(46);
        draw((uint16_t)((cpu.a & 0xFF00) | 0x29));
    }
    cyc(122);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    DebugDecimalByte();
}

void abi_DebugSignedByte(void)
{
    DebugSignedByte();
}

/* The long wrappers: every register and P saved; the byte stored at $06E7
   through the caller's data bank and read back with the data bank 0; then
   the printer. A is pushed as wide as it came: with M 8-bit only its low
   byte comes back, the high byte left as the printer leaves it (the high
   byte of the text position DebugDrawChar returns). */
static void long_wrapper(void (*print)(void))
{
    Regs65816 saved = cpu;
    uint16_t a;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cyc(194);
    wr8((uint32_t)cpu.db << 16 | NUM_BYTE, (uint8_t)cpu.a);
    cpu.db = 0x00;
    cyc(90);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(NUM_BYTE));
    (cyc(46), print());
    a = cpu.a;
    cpu = saved;
    if (saved.p & P_M)
        cpu.a = (uint16_t)((a & 0xFF00) | (saved.a & 0x00FF)); else { cyc(8); }
    cyc(298);
    cpu.p = nz8(cpu.p, cpu.db);
}

/*
 * @routine 03:D4DE DebugPrintHexByte
 * WHAT IT DOES: DebugHexByte for callers in other banks, with every
 * register kept.
 * HOW: long_wrapper.
 * Entry: A low byte = the byte; D = $0000. Exit: X, Y, P unchanged but N,
 * Z, which the final PLB sets from the data bank; A unchanged with M
 * 16-bit, its low byte only with M 8-bit.
 */
void DebugPrintHexByte(void)
{
    long_wrapper(DebugHexByte);
}

void abi_DebugPrintHexByte(void)
{
    (cyc(82), long_wrapper(DebugHexByte));
}

/*
 * @routine 03:D4A6 DebugPrintDecimalByte
 * WHAT IT DOES: DebugDecimalByte for callers in other banks, with every
 * register kept.
 * HOW: long_wrapper.
 * Entry: as DebugPrintHexByte. Exit: as DebugPrintHexByte.
 */
void DebugPrintDecimalByte(void)
{
    long_wrapper(DebugDecimalByte);
}

void abi_DebugPrintDecimalByte(void)
{
    long_wrapper(DebugDecimalByte);
}

/*
 * @routine 03:D4C2 DebugPrintSignedByte
 * WHAT IT DOES: DebugSignedByte for callers in other banks, with every
 * register kept.
 * HOW: long_wrapper.
 * Entry: as DebugPrintHexByte. Exit: as DebugPrintHexByte.
 */
void DebugPrintSignedByte(void)
{
    long_wrapper(DebugSignedByte);
}

void abi_DebugPrintSignedByte(void)
{
    long_wrapper(DebugSignedByte);
}

/*
 * @routine 03:D499 DebugPrintHexWord
 * WHAT IT DOES: Meant to print A as four hexadecimal digits, the high byte
 * first. The second pair printed is not A's low byte: the first
 * DebugPrintHexByte leaves the high byte of the text position in A's high
 * byte, which the second XBA brings down.
 * HOW: XBA, DebugPrintHexByte, XBA, DebugPrintHexByte; A pushed and pulled
 * (one byte).
 * With M 16-bit (the debug panels of bank 08) the long wrapper keeps the
 * whole of A, both bytes print right and the word comes back whole.
 * Entry: M 8-bit or 16-bit, D = $0000; A = the word. Exit: with M 8-bit A
 * low byte unchanged, its high byte as the second print leaves it, N, Z
 * from the low byte; with M 16-bit A unchanged, N, Z from it.
 */
void DebugPrintHexWord(void)
{
    uint16_t a = cpu.a;

    cpu.a = (uint16_t)(a << 8 | a >> 8);
    (cyc(22), abi_DebugPrintHexByte());
    cpu.a = (uint16_t)(cpu.a << 8 | cpu.a >> 8);
    abi_DebugPrintHexByte();
    if (cpu.p & P_M) {
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | (a & 0x00FF));
        cpu.p = nz8(cpu.p, (uint8_t)a);
    } else {
        cyc(24);
        cpu.a = a;
        cpu.p = nz16(cpu.p, a);
    }
    cyc(72);
}

void abi_DebugPrintHexWord(void)
{
    DebugPrintHexWord();
}

/*
 * @routine 03:D4FA DebugPrintDecimalWord
 * WHAT IT DOES: Prints A as a signed decimal number: character $0D first
 * when it is negative, then the digits of its magnitude without leading
 * zeros (character $10 + digit), or one $10 for 0.
 * HOW: The magnitude kept at $42; each digit counted in $3E by taking off
 * the power of ten from the word table at $03:D57A (10000 to 1) while the
 * result stays positive; Y says a digit has been drawn. 16-bit, the data
 * bank 0; every register and P saved.
 * Entry: any widths; A = the value; D = $0000. Exit: every register and P
 * unchanged.
 */
void DebugPrintDecimalWord(void)
{
    Regs65816 saved = cpu;
    uint16_t v, pw, n, x, drawn = 0;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cyc(172);
    wr16(NUM_VALUE, cpu.a);
    cpu.db = 0x00;
    cyc(126);
    v = rd16(NUM_VALUE);
    if (v & 0x8000) {
        cyc(70);
        wr16(NUM_VALUE, (uint16_t)(~v + 1));
        cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
        cyc(30);
        draw((uint16_t)((cpu.a & 0xFF00) | 0x0D));
    }
    cpu.p &= (uint8_t)~P_M;
    x = 0;
    cyc(84);
    wr16(NUM_DIGIT, 0x0000);
    do {
        cyc(2);
        for (;;) {
            cyc(38);
            v = rd16(NUM_VALUE);
            pw = rd16(POWERS_OF_TEN + x);
            if ((uint16_t)(v - pw) & 0x8000)
                break;
            cyc(100);
            wr16(NUM_VALUE, (uint16_t)(v - pw));
            cyc(16);
            n = rd16(NUM_DIGIT);
            cyc(14);
            wr16(NUM_DIGIT, (uint16_t)(n + 1));
        }
        cyc(156);
        pw = rd16(POWERS_OF_TEN + x);
        wr16(NUM_VALUE, v);
        cyc(16);
        n = rd16(NUM_DIGIT);
        if (n || drawn) {
            drawn = 1;
            cpu.p |= P_M;
            cyc(168);
            n = rd8(NUM_DIGIT);
            draw((uint16_t)((cpu.a & 0xFF00) | (uint8_t)(n + 0x10)));
        }
        cpu.p &= (uint8_t)~P_M;
        cyc(180);
        wr16(NUM_DIGIT, 0x0000);
        x = (uint16_t)(x + 2);
    } while (cyc_loop(50, x != 0x000A));
    cyc(22);
    if (!drawn) {
        cyc(94);
        cpu.p |= P_M;
        draw((uint16_t)((cpu.a & 0xFF00) | 0x10));
    }
    cyc(300);
    cpu = saved;
}

void abi_DebugPrintDecimalWord(void)
{
    DebugPrintDecimalWord();
}

/*
 * @routine 03:D7C1 DebugPrintBinaryByte
 * WHAT IT DOES: Prints the byte in A as eight binary digits, the highest
 * bit first.
 * HOW: Each pair of bits, taken from the low end of $40 by two LSRs,
 * becomes a byte $00, $01, $10 or $11 on the stack; the four are then
 * printed with DebugHexByte, the last pushed (the highest bits) first. 8-bit
 * registers; P saved.
 * Entry: A = the byte; D = $0000. Exit: P unchanged; A, X, Y as the last
 * DebugDrawChar leaves them (X, Y 8 bits wide when P says so).
 */
void DebugPrintBinaryByte(void)
{
    uint8_t p = cpu.p, pair[4], v, lo;
    int i;

    cpu.p |= P_M | P_X;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cyc(60);
    wr8(NUM_BITS, (uint8_t)cpu.a);
    for (i = 0; i < 4; i++) {
        cyc(32);
        v = rd8(NUM_BITS);
        lo = v & 1;
        cyc(6);
        wr8(NUM_BITS, (uint8_t)(v >> 1));
        cyc(38);
        v = rd8(NUM_BITS);
        cyc(6);
        wr8(NUM_BITS, (uint8_t)(v >> 1));
        pair[i] = (uint8_t)((v & 1) << 4 | lo);
    }
    cyc(22);
    for (i = 3; i >= 0; i--) {
        cyc(172);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | pair[i]);
        DebugHexByte();
    }
    cyc(128);
    op_plp(p);
}

void abi_DebugPrintBinaryByte(void)
{
    DebugPrintBinaryByte();
}
