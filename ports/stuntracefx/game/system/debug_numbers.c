/*
 * debug_numbers.c: numbers printed with DebugDrawChar (debug_text.c) in
 * hexadecimal, decimal and binary. The long entries save every register
 * and P and run with the data bank 0; the byte to print is passed through
 * $06E7. No play runs them; unit_debug_numbers calls them.
 *
 * The functions take the byte or word to print and the data bank; where
 * the routine leaves A, X and Y as the last DebugDrawChar made them, they
 * come back in a DebugRegs. The flags are the adapters'.
 */
#include "machine.h"
#include "system_calls.h"
#include "carried.h"
#include "wram_names.h"


#define HEX_DIGITS 0x03D7B1u
#define POWERS_OF_TEN 0x03D57Au
#define DECIMAL_PAIRS 0x03D5EDu

/* DebugDrawChar (a JSR from here) with the character ch; A, X, Y as it
   leaves them. */
static void draw(uint8_t bank, uint8_t ch, DebugRegs *r)
{
    cyc(76);
    DebugDrawChar(bank, ch, &r->a, &r->x, &r->y);
}

/*
 * @routine 03:D782 DebugHexByte
 * WHAT IT DOES: Prints the byte in A as two hexadecimal digits.
 * HOW: Each nibble indexes the ASCII digits at $03:D7B1 (a word read, its
 * low byte kept), less $20 for the font. 16-bit inside; P saved.
 * Entry: A low byte = the byte. Exit: P unchanged; A, X, Y as the second
 * DebugDrawChar leaves them (*r), 8 bits wide in X and Y when P says so
 * (the adapter).
 */
void DebugHexByte(uint8_t bank, uint8_t byte, DebugRegs *r)
{
    uint16_t d;

    cyc(248);
    d = rd16(HEX_DIGITS + ((byte & 0x00F0) >> 4));
    draw(bank, (uint8_t)((d & 0x00FF) - 0x0020), r);
    cyc(154);
    d = rd16(HEX_DIGITS + (byte & 0x000F));
    draw(bank, (uint8_t)((d & 0x00FF) - 0x0020), r);
    cyc(28);
    cyc(42);                                    /* PLP */
}

void DebugDecimalByte_cr(Carried *cr);
void DebugHexByte_cr(Carried *cr);
void DebugPrintBinaryByte_cr(Carried *cr);
void DebugPrintDecimalByte_cr(Carried *cr);
void DebugPrintDecimalWord_cr(Carried *cr);
void DebugPrintHexByte_cr(Carried *cr);
void DebugPrintHexWord_cr(Carried *cr);
void DebugPrintSignedByte_cr(Carried *cr);
void DebugSignedByte_cr(Carried *cr);
static void dec_regs(Carried *cr);
static void long_regs(Carried *cr, uint16_t a);

void DebugHexByte_cr(Carried *cr)
{
    DebugRegs r = {cr->a, cr->x, cr->y};

    DebugHexByte(cr->db, (uint8_t)cr->a, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_DebugHexByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugHexByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* What DebugDecimalByte leaves in P besides what it leaves unchanged: C
   from the last test of the tens, V from the subtract of 200 or 100 (-1:
   none made), N and Z from the units digit. For the adapters. */
static int dec_c, dec_v;
static uint8_t dec_units;

/* SEC / SBC (8-bit, binary): the difference, V in dec_v. */
static uint8_t dec_sbc(uint8_t a, uint8_t m)
{
    uint8_t nm = (uint8_t)~m;
    unsigned r = (unsigned)a + nm + 1;

    cyc(16);
    dec_v = ((~(a ^ nm) & (a ^ r) & 0x80) != 0);
    return (uint8_t)r;
}

static void dec_regs(Carried *cr)
{
    uint8_t p = (uint8_t)((cr->p & ~P_C) | (dec_c ? P_C : 0));

    if (dec_v >= 0)
        p = (uint8_t)((p & ~P_V) | (dec_v ? P_V : 0));
    set_p(cr, nz8(p, dec_units));
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
 * DebugDrawChar leaves them (*r); N, Z from the units digit; C from the
 * last test of the tens (bit 3 of the pair, or no hundreds digit); V from
 * the subtract of 200 or 100 when there was one (the adapter has the flags).
 */
void DebugDecimalByte(uint8_t bank, uint8_t byte, DebugRegs *r)
{
    uint8_t v = byte, h, t;

    dec_v = -1;
    if (v >= 200) {
        v = dec_sbc(v, 200);
        h = 0x02;
    } else if (v >= 100) {
        cyc(38);
        v = dec_sbc(v, 100);
        h = 0x01;
    } else {
        h = 0xFF;
    }
    cyc(104);
    if (h != 0xFF)
        { cyc(26); draw(bank, h, r); }
    cyc(h != 0xFF ? 296 : 366);
    t = rd8(DECIMAL_PAIRS + v);
    dec_c = (t >> 4) ? (t >> 3 & 1) : (cyc(34), (h == 0xFF));
    if ((t >> 4) || h != 0xFF)
        { cyc(34); draw(bank, (uint8_t)(t >> 4), r); cyc(14); }
    else { cyc(84); }
    dec_units = t & 0x0F;
    draw(bank, dec_units, r);
    cyc(42); /* RTS */
}

void DebugDecimalByte_cr(Carried *cr)
{
    DebugRegs r = {cr->a, cr->x, cr->y};

    DebugDecimalByte(cr->db, (uint8_t)cr->a, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    dec_regs(cr);
}

void abi_DebugDecimalByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugDecimalByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D584 DebugSignedByte
 * WHAT IT DOES: Prints the signed byte in A: character $24 and the
 * magnitude when it is negative, else character $29 and the value
 * (DebugDecimalByte).
 * HOW: The byte kept on the stack across the sign's draw.
 * Entry: as DebugDecimalByte. Exit: as DebugDecimalByte.
 */
void DebugSignedByte(uint8_t bank, uint8_t byte, DebugRegs *r)
{
    uint8_t v = byte;

    if (v & 0x80) {
        cyc(40);
        draw(bank, 0x24, r);
        cyc(30); /* EOR, INC */
        v = (uint8_t)(~v + 1);
    } else {
        cyc(46);
        draw(bank, 0x29, r);
    }
    cyc(52);
    DebugDecimalByte(bank, v, r);
}

void DebugSignedByte_cr(Carried *cr)
{
    DebugRegs r = {cr->a, cr->x, cr->y};

    DebugSignedByte(cr->db, (uint8_t)cr->a, &r);
    cr->a = r.a;
    cr->x = r.x;
    cr->y = r.y;
    dec_regs(cr);
}

void abi_DebugSignedByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugSignedByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The long wrappers: every register and P saved (`p` is the P it comes
   with); the byte stored at $06E7 through the caller's data bank and read
   back with the data bank 0; then the printer. A is pushed as wide as it
   came: with M 8-bit only its low byte comes back, the high byte left as
   the printer leaves it (the high byte of the text position
   DebugDrawChar returns, the value returned here as A). */
static uint16_t long_wrapper(void (*print)(uint8_t, uint8_t, DebugRegs *),
                             uint8_t bank, uint8_t byte, uint8_t p)
{
    DebugRegs r = {0, 0, 0};

    (void)bank;

    /* PHA pushes a word with M 16-bit; PHX and PHY a byte each with X 8-bit */
    cyc((p & P_M ? 194 : 202) - (p & P_X ? 16 : 0));
    wr8(0x7E0000 | W(debug_byte), byte);
    cyc(90);
    byte = rd8(W(debug_byte));
    (cyc(46), print(0x00, byte, &r));
    if (!(p & P_M))
        cyc(8);
    cyc(200 - (p & P_X ? 16 : 0)); /* PLP, PLY, PLX, PLA, PLB, RTL */
    return r.a;
}

/* The long wrappers' registers: X, Y, DB, P kept but N, Z, which the final
   PLB sets from the data bank; A kept with M 16-bit, its low byte only
   with M 8-bit (its high byte the printer's). */
static void long_regs(Carried *cr, uint16_t a)
{
    if (cr->p & P_M)
        cr->a = (uint16_t)((a & 0xFF00) | (cr->a & 0x00FF));
    set_p(cr, nz8(cr->p, cr->db));
}

/*
 * @routine 03:D4DE DebugPrintHexByte
 * WHAT IT DOES: DebugHexByte for callers in other banks, with every
 * register kept.
 * HOW: long_wrapper.
 * Entry: A low byte = the byte; D = $0000; `p` the P it comes with. Exit:
 * X, Y, P unchanged but N, Z, which the final PLB sets from the data bank;
 * A unchanged with M 16-bit, its low byte only with M 8-bit (the return
 * value is the A the printer left, for that high byte).
 */
uint16_t DebugPrintHexByte(uint8_t bank, uint8_t byte, uint8_t p)
{
    return long_wrapper(DebugHexByte, bank, byte, p);
}

void DebugPrintHexByte_cr(Carried *cr)
{
    long_regs(cr, DebugPrintHexByte(cr->db, (uint8_t)cr->a, cr->p));
}

void abi_DebugPrintHexByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPrintHexByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D4A6 DebugPrintDecimalByte
 * WHAT IT DOES: DebugDecimalByte for callers in other banks, with every
 * register kept.
 * HOW: long_wrapper.
 * Entry: as DebugPrintHexByte. Exit: as DebugPrintHexByte.
 */
uint16_t DebugPrintDecimalByte(uint8_t bank, uint8_t byte, uint8_t p)
{
    return long_wrapper(DebugDecimalByte, bank, byte, p);
}

void DebugPrintDecimalByte_cr(Carried *cr)
{
    long_regs(cr, DebugPrintDecimalByte(cr->db, (uint8_t)cr->a, cr->p));
}

void abi_DebugPrintDecimalByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPrintDecimalByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D4C2 DebugPrintSignedByte
 * WHAT IT DOES: DebugSignedByte for callers in other banks, with every
 * register kept.
 * HOW: long_wrapper.
 * Entry: as DebugPrintHexByte. Exit: as DebugPrintHexByte.
 */
uint16_t DebugPrintSignedByte(uint8_t bank, uint8_t byte, uint8_t p)
{
    return long_wrapper(DebugSignedByte, bank, byte, p);
}

void DebugPrintSignedByte_cr(Carried *cr)
{
    long_regs(cr, DebugPrintSignedByte(cr->db, (uint8_t)cr->a, cr->p));
}

void abi_DebugPrintSignedByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPrintSignedByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * Entry: M 8-bit or 16-bit (`p`), D = $0000; A = the word (`a`). Exit: the
 * return value is A: with M 8-bit its low byte unchanged, its high byte as
 * the second print leaves it, N, Z from the low byte; with M 16-bit A
 * unchanged, N, Z from it (the adapter has the flags).
 */
uint16_t DebugPrintHexWord(uint8_t bank, uint16_t a, uint8_t p)
{
    uint16_t pos;
    uint8_t byte2;

    cyc((p & P_M ? 22 : 30) + 82);              /* PHA, XBA, JSL */
    pos = DebugPrintHexByte(bank, (uint8_t)(a >> 8), p);
    byte2 = (p & P_M) ? (uint8_t)(pos >> 8) : (uint8_t)a;
    cyc(82);                                    /* XBA, JSL */
    pos = DebugPrintHexByte(bank, byte2, p);
    if (!(p & P_M))
        cyc(8);
    cyc(72);
    return (p & P_M) ? (uint16_t)((pos & 0xFF00) | (a & 0x00FF)) : a;
}

void DebugPrintHexWord_cr(Carried *cr)
{
    uint16_t a = cr->a;

    cr->a = DebugPrintHexWord(cr->db, a, cr->p);
    set_p(cr, (cr->p & P_M) ? nz8(cr->p, (uint8_t)a) : nz16(cr->p, a));
}

void abi_DebugPrintHexWord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPrintHexWord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void DebugPrintDecimalWord(uint16_t value)
{
    DebugRegs r = {0, 0, 0};
    uint16_t v, pw, n, x, drawn = 0, neg = 0, skip;

    cyc(172);
    wr16(W(dp_42), value);
    cyc(126);
    v = rd16(W(dp_42));
    if (v & 0x8000) {
        cyc(70);
        wr16(W(dp_42), (uint16_t)(~v + 1));
        cyc(30);
        draw(0x00, 0x0D, &r);
        neg = 1;
    }
    x = 0;
    cyc(neg ? 62 : 84);
    wr16(W(dp_3e), 0x0000);
    do {
        cyc(2);
        for (;;) {
            cyc(38);
            v = rd16(W(dp_42));
            pw = rd16(POWERS_OF_TEN + x);
            if ((uint16_t)(v - pw) & 0x8000)
                break;
            cyc(100);
            wr16(W(dp_42), (uint16_t)(v - pw));
            cyc(16);
            n = rd16(W(dp_3e));
            cyc(14);
            wr16(W(dp_3e), (uint16_t)(n + 1));
        }
        cyc(156);
        pw = rd16(POWERS_OF_TEN + x);
        wr16(W(dp_42), v);
        cyc(16);
        n = rd16(W(dp_3e));
        skip = !(n || drawn);
        if (!skip) {
            drawn = 1;
            cyc(n ? 168 : 202);
            n = rd8(W(dp_3e));
            draw(0x00, (uint8_t)(n + 0x10), &r);
        }
        cyc(skip ? 124 : 110);
        wr16(W(dp_3e), 0x0000);
        x = (uint16_t)(x + 2);
    } while (cyc_loop(50, x != 0x000A));
    cyc(22);
    if (!drawn) {
        cyc(94);
        draw(0x00, 0x10, &r);
    }
    cyc(drawn ? 300 : 230);
}

void DebugPrintDecimalWord_cr(Carried *cr)
{
    DebugPrintDecimalWord(cr->a);
}

void abi_DebugPrintDecimalWord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPrintDecimalWord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * DebugDrawChar leaves them (*r; X, Y 8 bits wide, in the adapter).
 */
void DebugPrintBinaryByte(uint8_t bank, uint8_t byte, DebugRegs *r)
{
    uint8_t pair[4], v, lo;
    int i;

    cyc(60);
    wr8(W(dp_40), byte);
    for (i = 0; i < 4; i++) {
        cyc(i ? 152 : 32); /* later pairs: ROL, ASL x3, PLP, ROL, PHA, LDA #0 first */
        v = rd8(W(dp_40));
        lo = v & 1;
        cyc(6);
        wr8(W(dp_40), (uint8_t)(v >> 1));
        cyc(38);
        v = rd8(W(dp_40));
        cyc(6);
        wr8(W(dp_40), (uint8_t)(v >> 1));
        pair[i] = (uint8_t)((v & 1) << 4 | lo);
    }
    cyc(22);
    for (i = 3; i >= 0; i--) {
        cyc(i == 3 ? 172 : 74); /* the first: ROL, ASL x3, PLP, ROL, PHA, PLA, JSR */
        DebugHexByte(bank, pair[i], r);
    }
    cyc(30);
    cyc(42);                                    /* PLP */
}

void DebugPrintBinaryByte_cr(Carried *cr)
{
    DebugRegs r = {cr->a, cr->x, cr->y};

    DebugPrintBinaryByte(cr->db, (uint8_t)cr->a, &r);
    cr->a = r.a;
    cr->x = r.x & 0x00FF;               /* DebugHexByte ran with 8-bit X and Y */
    cr->y = r.y & 0x00FF;
}

void abi_DebugPrintBinaryByte(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    DebugPrintBinaryByte_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

