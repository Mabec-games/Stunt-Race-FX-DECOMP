/*
 * map.c: the course map's markers, one sprite per object, placed from the
 * objects' world positions scaled down to map pixels.
 */
#include "machine.h"

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
    cyc(30);
    uint32_t b = (uint32_t)bank << 16;
    uint16_t i, obj, sx, v;

    for (i = 0;; i = (uint16_t)(i + 2)) {
        cyc(70);
        obj = rd16(0x7EFFBD + i);
        if (!obj)
            { cyc(118); return i; }
        cyc(98);
        v = asr3(rd16(0x7E200C + obj));
        cyc(164);
        sx = (uint16_t)((uint16_t)((v >> 8 | v << 8) + rd16(0x0000F1)) & 0x00FF);
        cyc(40);
        wr16(0x000002, sx);
        cyc(32);
        v = asr3((uint16_t)(-rd16(0x7E2010 + obj)));
        cyc(182);
        v = (uint16_t)(((uint16_t)(v + rd16(0x0000F3)) & 0xFF00) | sx);
        cyc(72);
        wr16(0x000008, v);
        cyc(30);
        wr16(b | (uint16_t)(0x0375 + *y), v);
        cyc(54);
        v = rd16(0x7E2026 + obj);
        cyc(16);
        wr16(0x000008, v);
        cyc(84);
        v = rd16(0x7F0B46 + 2 * (v & 0x0007));
        cyc(16);
        wr16(0x000002, v);
        cyc(62);
        wr16(b | (uint16_t)(0x0377 + *y), v);
        cyc(108);
        *y = (uint16_t)(*y + rd16(0x0000F5));
    }
}

void abi_MapMarkerSprites(void)
{
    cpu.x = MapMarkerSprites(cpu.db, &cpu.y);
    cpu.a = 0;
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

void abi_MapMarkerSprites(void);

/* For the abi_ wrappers: the map origin from the byte at x_at ($F1) and the
   byte at x_at + 3 (high byte of $F3), the slot step in $F5, and Y = the
   first slot. */
static void abi_map_markers_setup(uint32_t x_at, uint16_t step, uint16_t slot)
{
    wr16(0x0000F1, cyc_then(40, (uint16_t)(rd16(x_at) & 0x00FF)));   /* AND #, STA */
    cyc(24);
    wr16(0x0000F3, cyc_then(40, (uint16_t)(rd16(x_at + 2) & 0xFF00)));
    cyc(40);
    wr16(0x0000F5, step);
    cpu.y = slot;
}

/*
 * @routine 7E:EEF7 MapMarkersPanelA
 * WHAT IT DOES: Draws the map markers with the map origin at ($0E0A,
 * $0E0D), from sprite table offset $158 downward.
 * HOW: Direct page $F1 = $0E0A, $F3 high byte = $0E0D, $F5 = -4, Y = $158,
 * then MapMarkerSprites. Work RAM through the data bank.
 * Entry: X 16-bit, D = $0000. Exit: as MapMarkerSprites, with M 16-bit.
 */
void MapMarkersPanelA(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t y = 0x0158;

    wr16(0x0000F1, (uint16_t)(rd16(b | 0x0E0A) & 0x00FF));
    wr16(0x0000F3, (uint16_t)(rd16(b | 0x0E0C) & 0xFF00));
    wr16(0x0000F5, 0xFFFC);
    MapMarkerSprites(bank, &y);
}

void abi_MapMarkersPanelA(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(46);
    abi_map_markers_setup(b | 0x0E0A, 0xFFFC, 0x0158);
    cpu.a = 0xFFFC;
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_M), 0x0158));
    (cyc(86), abi_MapMarkerSprites());
    cyc(42);
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
 * 8-bit, Z set, N clear; otherwise as MapMarkerSprites, with M 8-bit.
 */
void MapMarkersPanelB(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t y;

    if (!rd8(0x000D55) && !rd8(0x000D62))
        return;
    wr16(0x0000F1, (uint16_t)(rd16(b | 0x0E09) & 0x00FF));
    wr16(0x0000F3, (uint16_t)(rd16(b | 0x0E0B) & 0xFF00));
    wr16(0x0000F5, 0x0004);
    y = rd8(b | 0x0D62) ? 0x0054 : 0x0180;
    MapMarkerSprites(bank, &y);
}

void abi_MapMarkersPanelB(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    cyc(54);
    uint8_t v = rd8(0x000D55);

    if (!v)
        { cyc(48); v = rd8(0x000D62); }
    cyc(58);
    if (!v) {
        cpu.a &= 0xFF00;
        cpu.p = (uint8_t)(nz8(cpu.p, 0) | P_M);
        return;
    }
    /* the setup's stores come before the $0D62 read that picks the first
       slot */
    cyc(10);
    abi_map_markers_setup(b | 0x0E09, 0x0004, 0);
    cyc(46);                            /* SEP, LDA long */
    v = rd8(b | 0x0D62);
    cpu.y = v ? 0x0054 : 0x0180;
    cyc(v ? 108 : 124);                 /* BNE, LDY # (and BRA), JSL */
    cpu.a = (uint16_t)(0x0000 | v);
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p | P_M), cpu.y));
    abi_MapMarkerSprites();
    cyc(42);
}
