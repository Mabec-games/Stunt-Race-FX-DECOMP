/*
 * map.c: the course map's markers, one sprite per object, placed from the
 * objects' world positions scaled down to map pixels.
 */
#include "machine.h"
#include "hud_calls.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"

/* Arithmetic shift right by 3, as three CMP #$8000 / ROR A. */
static uint16_t asr3(uint16_t v)
{
    unsigned i;

    for (i = 0; i < 3; i++)
        v = (uint16_t)((v >> 1) | (v & 0x8000));
    return v;
}

/*
 * @routine 7F:0AD6 MapMarkerSprites
 * WHAT IT DOES: Places a map marker sprite for each object in the list at
 * $7E:FFBD (object offsets, ended by 0): at X = the object's X ($200C) / 2048
 * plus the map origin X in direct page $F1, Y = -(its Z, $2010) / 2048 plus
 * the origin in $F3, with the tile word that the low 3 bits of its +$26 pick
 * from $7F:0B46. Successive markers go to sprite table offsets Y, Y + $F5,
 * ...
 * HOW: Each coordinate is shifted right 3 with its sign kept, then the high
 * byte taken by XBA (X) or by AND $FF00 (Y) and added to the origin word.
 * Direct page $02 and $08 are left with the last tile word and +$26 word;
 * the sprite table is written through the data bank.
 * Entry: X 16-bit, D = $0000; Y = first sprite table offset. Exit: A = 0;
 * X = the list position of the terminating 0; Y past the last marker (low
 * bytes only when P restores 8-bit index registers); P unchanged.
 */
uint16_t MapMarkerSprites(uint8_t bank, uint16_t *y)
{
    (void)bank;
    cyc(30);
    uint16_t i, obj, sx, v;

    for (i = 0;; i = (uint16_t)(i + 2)) {
        cyc(70);
        obj = RD16X(object_list, i);
        if (!obj)
            { cyc(118); return i; }
        cyc(98);
        v = asr3(RD16X(obj_pos_x_long, obj));
        cyc(164);
        sx = (uint16_t)((uint16_t)((v >> 8 | v << 8) + RD16(dp_f1)) & 0x00FF);
        cyc(40);
        WR16(dp_02, sx);
        cyc(32);
        v = asr3((uint16_t)(-RD16X(obj_pos_z_long, obj)));
        cyc(182);
        v = (uint16_t)(((uint16_t)(v + RD16(dp_f3)) & 0xFF00) | sx);
        cyc(72);
        WR16(dp_08, v);
        cyc(30);
        WR16I(oam_buffer, *y, v);
        cyc(54);
        v = rd16(0x7E0000 | W(obj_26) + obj);
        cyc(16);
        WR16(dp_08, v);
        cyc(84);
        v = RD16X(map_marker_tiles, 2 * (v & 0x0007));
        cyc(16);
        WR16(dp_02, v);
        cyc(62);
        WR16I(oam_buffer, 2 + *y, v);
        cyc(108);
        *y = (uint16_t)(*y + RD16(dp_f5));
    }
}

/* MapMarkerSprites on carried registers. */
void MapMarkerSprites_cr(Carried *cr)
{
    cr->x = MapMarkerSprites(cr->db, &cr->y);
    cr->a = 0;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_MapMarkerSprites(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MapMarkerSprites_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The map origin from the word at x_at ($F1, low byte) and at x_at + 2
   ($F3, high byte), and the slot step ($F5), as the panels store them. */
static void map_origin(uint32_t x_at, uint16_t step)
{
    uint16_t v;

    v = (uint16_t)(rd16(x_at) & 0x00FF);
    cyc(40);                                    /* AND #, STA */
    WR16(dp_f1, v);
    cyc(24);
    v = (uint16_t)(rd16(x_at + 2) & 0xFF00);
    cyc(40);
    WR16(dp_f3, v);
    cyc(40);
    WR16(dp_f5, step);
}

/*
 * @routine 7E:EEF7 MapMarkersPanelA
 * WHAT IT DOES: Draws the map markers with the map origin at ($0E0A,
 * $0E0D), from sprite table offset $158 downward.
 * HOW: Direct page $F1 = $0E0A, $F3 high byte = $0E0D, $F5 = -4, Y = $158,
 * then MapMarkerSprites. Work RAM through the data bank.
 * Entry: X 16-bit, D = $0000. Exit: returns the list position (X) and sets *y
 * (Y); the adapter leaves A = 0 and P with M 16-bit, N and Z clear, as
 * MapMarkerSprites otherwise.
 */
uint16_t MapMarkersPanelA(uint8_t bank, uint16_t *y)
{
    uint16_t x;

    cyc(46);
    map_origin(WA(map_a_x), 0xFFFC);
    *y = 0x0158;
    cyc(86);                                    /* JSL */
    x = MapMarkerSprites(bank, y);
    cyc(42);
    return x;
}

/* MapMarkersPanelA on carried registers. */
void MapMarkersPanelA_cr(Carried *cr)
{
    uint16_t y;

    /* an interrupt inside the markers pushes the P they run under */
    set_p(cr, (uint8_t)nz16((uint8_t)(cr->p & ~P_M), 0x0158));
    cr->x = MapMarkersPanelA(cr->db, &y);
    cr->y = y;
    cr->a = 0;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_MapMarkersPanelA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MapMarkersPanelA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:EF16 MapMarkersPanelB
 * WHAT IT DOES: When $0D55 or $0D62 is set, draws the map markers with the
 * map origin at ($0E09, $0E0C), from sprite table offset $54 upward when
 * $0D62 is set, else from $180.
 * HOW: Direct page $F1 = $0E09, $F3 high byte = $0E0C, $F5 = 4, then
 * MapMarkerSprites. $0D55 and $0D62 are tested with long loads, the rest
 * goes through the data bank.
 * Entry: X 16-bit, D = $0000. Exit: when neither is set, A low byte = 0, M
 * 8-bit, Z set, N clear; otherwise as MapMarkerSprites, with M 8-bit, N and
 * Z clear. Returns 0 when neither is set; else 1 with *x, *y as
 * MapMarkerSprites gives them.
 */
int MapMarkersPanelB(uint8_t bank, uint16_t *x, uint16_t *y)
{
    uint8_t v;

    cyc(54);
    v = RD8(panel_b_shown);
    if (!v)
        { cyc(48); v = RD8(wram_0D62); }
    cyc(58);
    if (!v)
        return 0;
    /* the setup's stores come before the $0D62 read that picks the first
       slot */
    cyc(10);
    map_origin(WA(map_b_x), 0x0004);
    cyc(46);                                    /* SEP, LDA long */
    v = RD8(wram_0D62);
    *y = v ? 0x0054 : 0x0180;
    cyc(v ? 108 : 124);                         /* BNE, LDY # (and BRA), JSL */
    *x = MapMarkerSprites(bank, y);
    cyc(42);
    return 1;
}

/* MapMarkersPanelB on carried registers. */
void MapMarkersPanelB_cr(Carried *cr)
{
    uint16_t x, y;
    uint8_t p0 = cr->p;

    set_p(cr, (uint8_t)((p0 | P_M) & ~(P_N | P_Z)));
    if (!MapMarkersPanelB(cr->db, &x, &y)) {
        set_p(cr, p0);
        cr->a &= 0xFF00;
        set_p(cr, (uint8_t)(nz8(cr->p, 0) | P_M));
        return;
    }
    /* N, Z from the first slot, which is never 0 or negative */
    set_p(cr, (uint8_t)((cr->p | P_M) & ~(P_N | P_Z)));
    cr->x = x;
    cr->y = y;
    cr->a = 0;
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_MapMarkersPanelB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    MapMarkersPanelB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
