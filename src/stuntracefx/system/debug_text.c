/*
 * debug_text.c: text printed into Super FX RAM, two characters to a byte
 * column. Direct page $54 points at a zero-terminated string; $46 is the
 * place to draw the next character, $48 the base it is relative to; bit 2
 * of $0307 says whether the character goes in the high or the low nibbles.
 * The font is 6 bytes a character at $00:9022 (two 4-pixel rows packed in
 * each byte's nibbles). No script reaches this code.
 */
#include "machine.h"

#define TEXT_HALF 0x000307u
#define TEXT_HIGH 0x000308u
#define TEXT_BITS 0x001F2Bu
#define TEXT_MASK 0x001F2Cu
#define TEXT_POS 0x000046u
#define TEXT_BASE 0x000048u
#define TEXT_ROWS 0x000040u
#define TEXT_FONT 0x009022u

/* The six byte offsets of a cell row in Super FX RAM, in the order written,
   and whether the nibble from $0308 is added (the first and the last three
   are not). */
static const struct { uint8_t off, high, cleared; } cell[8] = {
    {0x00, 0, 0xFF}, {0x01, 1, 0xFF}, {0x10, 1, 0xFF}, {0x11, 1, 0xFF},
    {0x20, 1, 0xFF}, {0x21, 2, 0x00}, {0x30, 2, 0x00}, {0x31, 2, 0x00}};

/*
 * @routine 03:D651 DebugDrawChar
 * WHAT IT DOES: Draws character A of the debug font at $46 + $48 in Super
 * FX RAM, then moves $46 on: every second character to the next cell
 * (+$400, wrapping from $6800 back by $67C0).
 * HOW: Six font bytes, each split into two nibbles placed in the high or
 * the low half by bit 2 of $0307 ($1F2B the bits, $1F2C the keep mask,
 * $0308 the second nibble); writing the low half first fills the cell's
 * rows with $FF/$00. Direct page $40 counts the rows.
 * Entry: A low byte = character; D = $0000. Exit: P unchanged; A = the new
 * $46; X, Y past the cell.
 */
void DebugDrawChar(uint8_t bank, uint8_t ch, uint16_t *a, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t dest = (uint16_t)(rd16(TEXT_POS) + rd16(TEXT_BASE)), pos;
    uint8_t f, bits, high, v;
    unsigned i;

    *x = dest;
    wr16(TEXT_ROWS, (uint16_t)(ch << 1));
    *y = (uint16_t)((ch << 2) + rd16(TEXT_ROWS));
    wr16(TEXT_ROWS, 0x0006);
    do {
        f = rd8(b | (uint16_t)(TEXT_FONT + *y));
        if (rd8(b | TEXT_HALF) & 0x04) {
            high = (uint8_t)(f >> 4);
            bits = (uint8_t)(f & 0x0F);
        } else {
            high = (uint8_t)(f & 0xF0);
            bits = (uint8_t)(f << 4);
        }
        wr8(b | TEXT_HIGH, high);
        wr8(b | TEXT_BITS, bits);
        wr8(b | TEXT_MASK, (uint8_t)~(bits | high));
        if (!(rd8(b | TEXT_HALF) & 0x04))
            for (i = 0; i < 8; i++)
                wr8(0x700000 + cell[i].off + *x, cell[i].cleared);
        for (i = 0; i < 8; i++) {
            v = (uint8_t)(rd8(0x700000 + cell[i].off + *x) & rd8(b | TEXT_MASK));
            if (cell[i].high < 2)
                v |= rd8(b | TEXT_BITS);
            if (cell[i].high == 1)
                v |= rd8(b | TEXT_HIGH);
            wr8(0x700000 + cell[i].off + *x, v);
        }
        *x = (uint16_t)(*x + 2);
        *y = (uint16_t)(*y + 1);
        wr16(TEXT_ROWS, (uint16_t)(rd16(TEXT_ROWS) - 1));
    } while (rd16(TEXT_ROWS));
    pos = (uint16_t)(dest - rd16(TEXT_BASE));
    wr16(b | TEXT_HALF, (uint16_t)(rd16(b | TEXT_HALF) ^ 0x0004));
    if (!rd16(b | TEXT_HALF)) {
        pos = (uint16_t)(pos + 0x0400);
        *x = pos;
        if (pos >= 0x6800)
            pos = (uint16_t)(pos - 0x67C0);
    }
    wr16(TEXT_POS, pos);
    *a = pos;
}

void abi_DebugDrawChar(void)
{
    DebugDrawChar(cpu.db, (uint8_t)cpu.a, &cpu.a, &cpu.x, &cpu.y);
}

/*
 * @routine 03:D465 DebugPrint
 * WHAT IT DOES: Prints the zero-terminated string at the long pointer in
 * direct page $54 with DebugDrawChar, starting in the low nibbles; lower
 * case letters print as upper case.
 * HOW: Each character from $61 up loses $20 more, then all lose $20 (the
 * font starts at the space). Data bank 0 inside; every register and P are
 * saved and restored.
 * Entry: any widths; D = $0000. Exit: every register and P unchanged.
 */
void DebugPrint(uint8_t bank)
{
    uint32_t s = rd24(0x000054);
    uint16_t y = 0, a = 0, x = 0, yy = 0;
    uint8_t c;

    wr16((uint32_t)bank << 16 | TEXT_HALF, 0x0000);
    while ((c = rd8((s + y) & 0xFFFFFF)) != 0) {
        if (c >= 0x61)
            c = (uint8_t)(c - 0x20);
        c = (uint8_t)(c - 0x20);
        DebugDrawChar(0x00, c, &a, &x, &yy);
        y = (uint16_t)(y + 1);
    }
}

void abi_DebugPrint(void)
{
    DebugPrint(cpu.db);
}

/* The three messages, in bank 08, and where each is printed. */
static const struct { uint32_t flag; uint16_t pos, text; } errors[3] = {
    {0x7026F0, 0x2240, 0xFF34},  /* "!!! Old mario chip !!!" */
    {0x701090, 0x2280, 0xFF4B},  /* "! Dynamics checksum fail !" */
    {0x702650, 0x1AC0, 0xFF66}}; /* "Tried to make car with bad ..." */

/*
 * @routine 08:FEB4 DebugShowErrors
 * WHAT IT DOES: Prints an error message into Super FX RAM for each of three
 * flags that is set: $70:26F0 (an old Super FX chip), $70:1090 (a failed
 * checksum of the car dynamics), $70:2650 (a car made from bad data).
 * HOW: For each: $46 = the place, $0307 = 0, the long pointer at direct
 * page $54 = the message in bank 08, then DebugPrint. No script sets a
 * flag.
 * Entry: M 8-bit, D = $0000. Exit: M 16-bit; X, Y unchanged; A = the
 * last message's address when it printed, else the last flag; N, Z from
 * that.
 */
uint16_t DebugShowErrors(uint8_t bank)
{
    uint16_t a = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        a = rd16(errors[i].flag);
        if (!a)
            continue;
        wr16(TEXT_POS, errors[i].pos);
        wr16((uint32_t)bank << 16 | TEXT_HALF, 0x0000);
        wr8(0x000056, 0x08);
        wr16(0x000054, errors[i].text);
        DebugPrint(bank);
        a = errors[i].text;
    }
    return a;
}

void abi_DebugShowErrors(void)
{
    cpu.a = DebugShowErrors(cpu.db);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
}
