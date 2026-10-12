/*
 * sprite_text.c: lines of text drawn as sprites. A line is a count byte
 * then one byte per character; a byte with bit 7 set is a space (it moves
 * the pen without using a sprite). Lines are found through the word table
 * at $07:8000, indexed by $0E3D.
 */
#include "machine.h"
#include "wram.h"

/*
 * @routine 07:AA6E OamTextLine
 * WHAT IT DOES: Draws line $0E3D as sprites from sprite $F2 on, centred on
 * X position $F3: each character's sprite gets X = the pen (stepping 8),
 * Y = $F4, tile = the character + $F1 and attributes = $F5.
 * HOW: The pen starts at $F3 - 4 * count; $F2 then counts the characters
 * down and $F3 is the pen. A space steps the pen and the count without a
 * sprite, and is not followed by the end-of-line test, so a line never ends
 * in a space. Line and sprites are read and written through the data bank.
 * Entry: M 8-bit, D = $0000. Exit: A = the line pointer's high byte and
 * $F5; X past the line; Y past the last sprite (4 bytes each), both cut to
 * 8 bits when X is 8-bit; P unchanged.
 */
void OamTextLine(uint8_t bank, uint16_t *xp, uint16_t *yp, uint16_t *ap)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t y = (uint16_t)((rd8(0x0000F2) & 0xFF) << 2);
    uint16_t x = rd16(0x078000 + ((rd16(b | 0x0E3D) << 1) & 0x00FF));
    uint8_t c;

    *ap = x;
    c = rd8(b | x);
    wr8(0x0000F2, c);
    x++;
    wr8(0x0000F3, (uint8_t)(rd8(0x0000F3) - (uint8_t)(c << 2)));
    for (;;) {
        c = rd8(b | x);
        if (c & 0x80) {
            x++;
            wr8(0x0000F3, (uint8_t)(rd8(0x0000F3) + 8));
            wr8(0x0000F2, (uint8_t)(rd8(0x0000F2) - 1));
            continue;
        }
        wr8(b | (uint16_t)(OAM_BUFFER + 2 + y), (uint8_t)(c + rd8(0x0000F1)));
        x++;
        wr8(b | (uint16_t)(OAM_BUFFER + y), rd8(0x0000F3));
        wr8(0x0000F3, (uint8_t)(rd8(0x0000F3) + 8));
        wr8(b | (uint16_t)(OAM_BUFFER + 1 + y), rd8(0x0000F4));
        wr8(b | (uint16_t)(OAM_BUFFER + 3 + y), rd8(0x0000F5));
        y += 4;
        c = (uint8_t)(rd8(0x0000F2) - 1);
        wr8(0x0000F2, c);
        if (!c)
            break;
    }
    *xp = x;
    *yp = y;
    *ap = (uint16_t)((*ap & 0xFF00) | rd8(0x0000F5));
}

void abi_OamTextLine(void)
{
    uint16_t x, y, a;

    OamTextLine(cpu.db, &x, &y, &a);
    cpu.a = a;
    cpu.x = cpu.p & P_X ? (uint16_t)(x & 0xFF) : x;
    cpu.y = cpu.p & P_X ? (uint16_t)(y & 0xFF) : y;
}

/*
 * @routine 07:AA60 OamTextLineLong
 * WHAT IT DOES: OamTextLine for callers in other banks.
 * HOW: Sets the data bank to $07 around the call.
 * Entry: M 8-bit, D = $0000. Exit: as OamTextLine; DB unchanged.
 */
void OamTextLineLong(uint16_t *xp, uint16_t *yp, uint16_t *ap)
{
    OamTextLine(0x07, xp, yp, ap);
}

void abi_OamTextLineLong(void)
{
    uint16_t x, y, a;

    OamTextLineLong(&x, &y, &a);
    cpu.a = a;
    cpu.x = cpu.p & P_X ? (uint16_t)(x & 0xFF) : x;
    cpu.y = cpu.p & P_X ? (uint16_t)(y & 0xFF) : y;
}
