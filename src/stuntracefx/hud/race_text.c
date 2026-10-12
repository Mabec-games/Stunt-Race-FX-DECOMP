/*
 * race_text.c: words the race screen spells with sprites, one letter tile
 * per sprite of the work-RAM sprite table ($0375, four bytes a sprite;
 * the tile is byte 2).
 */
#include "machine.h"

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/* "MASTER" and "EXPERT" in the sprite font, six tiles each. */
#define CLASS_NAMES 0x02EFD6u

/*
 * @routine 02:EFA3 OamClassName
 * WHAT IT DOES: Spells the race class ($0D4A) on the six sprites after
 * sprite $0D7B: "EXPERT" for class 1, "MASTER" for class 2 (not reached in
 * the traces so far), nothing for class 0.
 * HOW: The tiles come from $02:EFD6 (six bytes a name), counted down in
 * direct page $F1; 8-bit stores through the data bank. P is saved and
 * restored.
 * Entry: D = $0000. Exit: Y = ($0D7B + 1) * 4, plus $18 after a name; A
 * low byte = the class or the last tile, high byte from Y's value before
 * the name; X past the name's last tile, unchanged for class 0; P
 * unchanged.
 */
uint16_t OamClassName(uint8_t bank, uint16_t *x, uint16_t *y)
{
    uint16_t a = (uint16_t)((uint16_t)(rd16(dbw(bank, 0x0D7B)) + 1) << 2);
    uint8_t c, n;

    *y = a;
    c = rd8(dbw(bank, 0x0D4A));
    a = (uint16_t)((a & 0xFF00) | c);
    if (!c)
        return a;
    *x = c == 1 ? 0x0006 : 0x0000;
    wr8(0x0000F1, 0x06);
    do {
        c = rd8(CLASS_NAMES + *x);
        wr8(dbw(bank, (uint16_t)(0x0377 + *y)), c);
        *x = (uint16_t)(*x + 1);
        *y = (uint16_t)(*y + 4);
        n = (uint8_t)(rd8(0x0000F1) - 1);
        wr8(0x0000F1, n);
    } while (n);
    return (uint16_t)((a & 0xFF00) | c);
}

void abi_OamClassName(void)
{
    uint16_t x = cpu.x, y = cpu.y;

    cpu.a = OamClassName(cpu.db, &x, &y);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
    cpu.y = (cpu.p & P_X) ? (uint8_t)y : y;
}
