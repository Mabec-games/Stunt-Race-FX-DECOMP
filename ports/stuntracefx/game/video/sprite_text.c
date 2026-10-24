/*
 * sprite_text.c: lines of text drawn as sprites. A line is a count byte
 * then one byte per character; a byte with bit 7 set is a space (it moves
 * the pen without using a sprite). Lines are found through the word table
 * at $07:8000, indexed by $0E3D.
 */
#include "machine.h"
#include "carried.h"
#include "wram_names.h"


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
    cyc(60);
    uint16_t y = (uint16_t)((rd16(W(dp_f2)) & 0xFF) << 2);
    cyc(90);
    uint16_t x = rd16(cyc_then(190, 0x078000 + ((rd16(0x7E0000 | W(course)) << 1) & 0x00FF)));
    uint8_t c, pen, attr = 0;

    *ap = x;
    c = rd8(b | x);
    wr8(W(dp_f2), c);
    x++;
    cyc(72);
    wr8(W(dp_f3), cyc_then(46, (uint8_t)(rd8(W(dp_f3)) - (uint8_t)(c << 2))));
    for (;;) {
        cyc(84);
        c = rd8(b | x);
        if (c & 0x80) {
            x++;
            wr8(W(dp_f3), cyc_then(46, (uint8_t)(rd8(W(dp_f3)) + 8)));
            cyc(16);
            wr8(W(dp_f2), cyc_then(6, (uint8_t)(rd8(W(dp_f2)) - 1)));
            cyc(22);
            continue;
        }
        cyc(6);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 2 + y), cyc_then(30, (uint8_t)(c + rd8(W(dp_f1)))));
        x++;
        cyc(30);
        pen = rd8(W(dp_f3));
        cyc(30);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + y), pen);
        cyc(46);
        wr8(W(dp_f3), (uint8_t)(pen + 8));
        cyc(16);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 1 + y), cyc_then(30, rd8(W(dp_f4))));
        cyc(16);
        attr = rd8(W(dp_f5));
        cyc(30);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 3 + y), attr);
        y += 4;
        cyc(72);
        c = (uint8_t)(rd8(W(dp_f2)) - 1);
        cyc(6);
        wr8(W(dp_f2), c);
        if (!c)
            break; else { cyc(22); }
    }
    *xp = x;
    *yp = y;
    *ap = (uint16_t)((*ap & 0xFF00) | attr);
}

void OamTextLine_cr(Carried *cr);
void OamTextLineLong_cr(Carried *cr);
void SpriteTextAt8152_cr(Carried *cr);
void SpriteTextAt8152B_cr(Carried *cr);
void SpriteTextAt8152C_cr(Carried *cr);
void SpriteTextAt8952_cr(Carried *cr);
void SpriteTextAt8952B_cr(Carried *cr);
void SpriteTextAt895E_cr(Carried *cr);
static void abi_sprite_text_line_at(Carried *cr, uint8_t x, uint8_t y);

void OamTextLine_cr(Carried *cr)
{
    uint16_t x, y, a;

    OamTextLine(cr->db, &x, &y, &a);
    cr->a = a;
    cr->x = cr->p & P_X ? (uint16_t)(x & 0xFF) : x;
    cr->y = cr->p & P_X ? (uint16_t)(y & 0xFF) : y;
    cyc(86);
}

void abi_OamTextLine(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    OamTextLine_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AA60 OamTextLineLong
 * WHAT IT DOES: OamTextLine for callers in other banks.
 * HOW: Sets the data bank to $07 around the call.
 * Entry: M 8-bit, D = $0000. Exit: as OamTextLine; DB unchanged.
 */
void OamTextLineLong(uint16_t *xp, uint16_t *yp, uint16_t *ap)
{
    cyc(224);
    OamTextLine(0x07, xp, yp, ap);
}

void OamTextLineLong_cr(Carried *cr)
{
    uint16_t x, y, a;

    OamTextLineLong(&x, &y, &a);
    cr->a = a;
    cr->x = cr->p & P_X ? (uint16_t)(x & 0xFF) : x;
    cr->y = cr->p & P_X ? (uint16_t)(y & 0xFF) : y;
}

void abi_OamTextLineLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    OamTextLineLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The body the entries below share ($02:FC1C): text line $0E3D as sprites
   from slot $70 at (x, y), attributes $36, tiles from 0; then the high
   table bytes $058E-$0595 cleared (small sprites, X bit 8 clear), through
   the data bank. Returns X and Y as OamTextLine leaves them. */
static void sprite_text_line_at(uint8_t bank, uint8_t x, uint8_t y, uint16_t *xp, uint16_t *yp)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t a;
    unsigned i;

    cyc(74);
    wr8(W(dp_f4), y);
    cyc(36);
    wr8(W(dp_f3), x);
    cyc(32);
    wr8(W(dp_f2), 0x70);
    cyc(32);
    wr8(W(dp_f5), 0x36);
    cyc(16);
    wr8(W(dp_f1), 0x00);
    OamTextLineLong(xp, yp, &a);
    cyc(254);
    for (i = 0; i < 8; i += 2)
        { cyc(24); wr16(0x7E0000 | (uint16_t)(W(oam_buffer_high) + 25 + i), 0x0000); }
    cyc(42);
}

/* For the abi_ wrappers: A = 0, M 16-bit, Z set, N clear; C and V from the
   subtract that made the Y position (C set, V set for every entry here). */
static void abi_sprite_text_line_at(Carried *cr, uint8_t x, uint8_t y)
{
    uint16_t xo, yo;

    sprite_text_line_at(cr->db, x, y, &xo, &yo);
    cr->a = 0x0000;
    cr->x = cr->p & P_X ? (uint16_t)(xo & 0xFF) : xo;
    cr->y = cr->p & P_X ? (uint16_t)(yo & 0xFF) : yo;
    set_p(cr, (uint8_t)(((cr->p & ~(P_N | P_M)) | P_Z | P_C | P_V)));
}

/*
 * @routine 02:FBE5 SpriteTextAt895E
 * WHAT IT DOES: Draws text line $0E3D as sprites from slot $70 at X $89,
 * Y $5E, attributes $36, and makes those sprites small.
 * HOW: X in A's high byte, Y as $89 - $2B, then the shared body at
 * $02:FC1F (OamTextLineLong, then $058E-$0595 cleared).
 * Entry: X 16-bit, D = $0000. Exit: A = 0; X, Y as OamTextLine leaves
 * them; M 16-bit; Z, C, V set, N clear.
 */
void SpriteTextAt895E(uint8_t bank)
{
    uint16_t x, y;

    sprite_text_line_at(bank, 0x89, 0x5E, &x, &y);
}

void SpriteTextAt895E_cr(Carried *cr)
{
    cyc(68);
    abi_sprite_text_line_at(cr, 0x89, 0x5E);
}

void abi_SpriteTextAt895E(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpriteTextAt895E_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FBF1 SpriteTextAt8152
 * WHAT IT DOES: As SpriteTextAt895E at X $81, Y $52.
 * HOW: Y as $89 - $37, then the shared body at $02:FC1C.
 * Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
 */
void SpriteTextAt8152(uint8_t bank)
{
    uint16_t x, y;

    sprite_text_line_at(bank, 0x81, 0x52, &x, &y);
}

void SpriteTextAt8152_cr(Carried *cr)
{
    cyc(68);
    abi_sprite_text_line_at(cr, 0x81, 0x52);
}

void abi_SpriteTextAt8152(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpriteTextAt8152_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FBFA SpriteTextAt8152B
 * WHAT IT DOES: The same as SpriteTextAt8152 (a second copy of its entry).
 * HOW: As SpriteTextAt8152.
 * Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
 */
void SpriteTextAt8152B(uint8_t bank)
{
    SpriteTextAt8152(bank);
}

void SpriteTextAt8152B_cr(Carried *cr)
{
    cyc(68);
    abi_sprite_text_line_at(cr, 0x81, 0x52);
}

void abi_SpriteTextAt8152B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpriteTextAt8152B_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FC03 SpriteTextAt8952
 * WHAT IT DOES: As SpriteTextAt895E at X $89, Y $52. Nothing calls this
 * entry in the traces so far.
 * HOW: As SpriteTextAt8152.
 * Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
 */
void SpriteTextAt8952(uint8_t bank)
{
    uint16_t x, y;

    sprite_text_line_at(bank, 0x89, 0x52, &x, &y);
}

void SpriteTextAt8952_cr(Carried *cr)
{
    cyc(68);
    abi_sprite_text_line_at(cr, 0x89, 0x52);
}

void abi_SpriteTextAt8952(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpriteTextAt8952_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FC0C SpriteTextAt8152C
 * WHAT IT DOES: A third copy of SpriteTextAt8152's entry. Nothing calls it
 * in the traces so far.
 * HOW: As SpriteTextAt8152.
 * Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
 */
void SpriteTextAt8152C(uint8_t bank)
{
    SpriteTextAt8152(bank);
}

void SpriteTextAt8152C_cr(Carried *cr)
{
    cyc(68);
    abi_sprite_text_line_at(cr, 0x81, 0x52);
}

void abi_SpriteTextAt8152C(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpriteTextAt8152C_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FC15 SpriteTextAt8952B
 * WHAT IT DOES: A second copy of SpriteTextAt8952's entry, falling into the
 * shared body. Nothing calls it in the traces so far.
 * HOW: As SpriteTextAt8152.
 * Entry: as SpriteTextAt895E. Exit: as SpriteTextAt895E.
 */
void SpriteTextAt8952B(uint8_t bank)
{
    SpriteTextAt8952(bank);
}

void SpriteTextAt8952B_cr(Carried *cr)
{
    cyc(46);
    abi_sprite_text_line_at(cr, 0x89, 0x52);
}

void abi_SpriteTextAt8952B(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpriteTextAt8952B_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

