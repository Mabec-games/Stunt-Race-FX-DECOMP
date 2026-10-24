/*
 * race_text.c: words the race screen spells with sprites, one letter tile
 * per sprite of the work-RAM sprite table ($0375, four bytes a sprite;
 * the tile is byte 2).
 */
#include "machine.h"
#include "carried.h"
#include "wram_names.h"

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
    uint16_t a = (uint16_t)((uint16_t)(rd16(cyc_then(68, (0x7E0000 | W(wram_0D7B)))) + 1) << 2);
    uint8_t c, n;

    *y = a;
    c = rd8(cyc_then(102, (0x7E0000 | W(wram_0D4A))));
    a = (uint16_t)((a & 0xFF00) | c);
    if (!c)
        { cyc(6); return a; }
    *x = c == 1 ? (cyc(16), 0x0006) : 0x0000;
    cyc(110);
    wr8(W(dp_f1), 0x06);
    do {
        cyc(70);
        c = rd8(CLASS_NAMES + *x);
        wr8((0x7E0000 | (uint16_t)(W(oam_buffer) + 2 + *y)), c);
        *x = (uint16_t)(*x + 1);
        *y = (uint16_t)(*y + 4);
        cyc(86);
        n = (uint8_t)(rd8(W(dp_f1)) - 1);
        cyc(6);
        wr8(W(dp_f1), n);
    } while (cyc_loop(22, n));
    return (uint16_t)((a & 0xFF00) | c);
}

/* OamClassName on carried registers. */
void OamClassName_cr(Carried *cr)
{
    uint16_t x = cr->x, y = cr->y;

    cr->a = OamClassName(cr->db, &x, &y);
    cr->x = (cr->p & P_X) ? (uint8_t)x : (cyc(86), x);
    cr->y = (cr->p & P_X) ? (uint8_t)y : y;
}

void abi_OamClassName(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    OamClassName_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
