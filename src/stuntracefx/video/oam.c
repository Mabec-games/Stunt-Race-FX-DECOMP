/*
 * oam.c: the work-RAM copy of the sprite table (OAM), copied to the PPU
 * each frame. Each sprite is four bytes: X, Y, tile, attributes.
 */
#include "machine.h"
#include "hw.h"
#include "wram.h"

/*
 * @routine 07:C4D2 OamHideRange
 * WHAT IT DOES: Hides a run of sprites by moving them below the screen.
 * HOW: Stores Y = $F0 into each sprite's Y byte, starting at the sprite whose
 * table offset is X, for the given count.
 * Entry: M 8-bit, X 16-bit; X = first sprite * 4, Y = count (1 or more).
 * Exit: A low byte = $F0; X past the last sprite; Y = 0; Z set, N clear.
 */
void OamHideRange(uint16_t offset, uint16_t count)
{
    do {
        wr8(OAM_BUFFER + 1 + offset, 0xF0);
        offset += 4;
    } while (--count);
}

void abi_OamHideRange(void)
{
    uint16_t count = cpu.y;

    OamHideRange(cpu.x, count);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xF0);
    cpu.x = (uint16_t)(cpu.x + 4 * (count ? count : 0x10000u));
    cpu.y = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z);
}

/*
 * @routine 7E:ADFD OamWriteSprites36To46
 * WHAT IT DOES: Copies sprites 36 to 46 from the work-RAM table straight to
 * OAM.
 * HOW: Sets the OAM word address to $48 (sprite 36) and writes the 44 bytes
 * from table offset $90 to OAMDATA one by one.
 * Entry: M 8-bit, DB a bank where the table is work RAM. Exit: X = $BC
 * (16-bit); Y = 0; A low byte = the last byte written; Z set, N clear.
 */
uint8_t OamWriteSprites36To46(void)
{
    uint8_t v = 0;
    unsigned i;

    wr8(OAMADDL, 0x48);
    wr8(OAMADDH, 0x00);
    for (i = 0; i < 0x2C; i++) {
        v = rd8(OAM_BUFFER + 0x90 + i);
        wr8(OAMDATA, v);
    }
    return v;
}

void abi_OamWriteSprites36To46(void)
{
    uint8_t v = OamWriteSprites36To46();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = 0x00BC;
    cpu.y = 0;
    cpu.p = (uint8_t)(((cpu.p & ~(P_N | P_X)) | P_Z));
}

/*
 * @routine 7E:B005 SpriteYOnScreen
 * WHAT IT DOES: Returns a sprite Y coordinate, or $F0 (hidden) when it is
 * $B0 or more, below the part of the screen the sprites may cover.
 * HOW: Compares with $B0.
 * Entry: M 8-bit; A = Y. Exit: A low byte = the Y to use; C set when it was
 * replaced; N, Z from the compare, or from $F0.
 */
uint8_t SpriteYOnScreen(uint8_t y)
{
    return y >= 0xB0 ? 0xF0 : y;
}

void abi_SpriteYOnScreen(void)
{
    uint8_t y = (uint8_t)cpu.a;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SpriteYOnScreen(y));
    if (y >= 0xB0)
        cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N | P_C);
    else
        cpu.p = (uint8_t)(nz8(cpu.p, (uint8_t)(y - 0xB0)) & ~P_C);
}

/* Build a strip of n+1 sprites: positions from direct page $02 stepping by
   `step` (in the X byte), tiles and attributes from a table in bank $7F.
   Direct page $02 is restored; $08 ends at $FFFF. */
static void sprite_strip(uint16_t index, uint16_t offset, uint32_t table, uint16_t step, uint16_t n)
{
    uint16_t pos0 = rd16(0x000002), pos = pos0;

    wr16(0x000008, n);
    for (;;) {
        wr16(0x000002, pos);
        wr16(OAM_BUFFER + offset, pos);
        pos = (uint16_t)(pos + step);
        wr16(0x000002, pos);
        wr16(OAM_BUFFER + 2 + offset, rd16(table + index));
        index = (uint16_t)(index + 2);
        offset = (uint16_t)(offset + 4);
        n = (uint16_t)(n - 1);
        wr16(0x000008, n);
        if (n & 0x8000)
            break;
    }
    wr16(0x000002, pos0);
}

/*
 * @routine 7F:099C SpriteStrip4
 * WHAT IT DOES: Places a horizontal strip of four sprites, 16 pixels apart,
 * from the position in direct page $02, with the tiles of the set A selects.
 * HOW: The set is (A / 2) rounded down to a multiple of 8 bytes in the table
 * at $7F:09CD; each sprite takes the next X and tile word.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = set selector, Y = first sprite
 * offset. Exit: A = the position; X = the table offset + 8; Y + 16; P
 * unchanged.
 */
void SpriteStrip4(uint16_t selector, uint16_t offset)
{
    sprite_strip((uint16_t)((selector >> 1) & 0x00F8), offset, 0x7F09CD, 0x0010, 3);
}

void abi_SpriteStrip4(void)
{
    uint16_t index = (uint16_t)((cpu.a >> 1) & 0x00F8);

    SpriteStrip4(cpu.a, cpu.y);
    cpu.a = rd16(0x000002);
    cpu.x = (uint16_t)(index + 8);
    cpu.y = (uint16_t)(cpu.y + 16);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 7F:0A35 SpriteStrip6
 * WHAT IT DOES: Places a horizontal strip of six sprites, 8 pixels apart,
 * from the position in direct page $02, with the tiles of the set A selects.
 * HOW: The set is (A AND $38) * 2 bytes into the table at $7F:0A66.
 * Entry: as SpriteStrip4. Exit: A = the position; X = the table offset +
 * 12; Y + 24; P unchanged.
 */
void SpriteStrip6(uint16_t selector, uint16_t offset)
{
    sprite_strip((uint16_t)((selector & 0x0038) << 1), offset, 0x7F0A66, 0x0008, 5);
}

void abi_SpriteStrip6(void)
{
    uint16_t index = (uint16_t)((cpu.a & 0x0038) << 1);

    SpriteStrip6(cpu.a, cpu.y);
    cpu.a = rd16(0x000002);
    cpu.x = (uint16_t)(index + 12);
    cpu.y = (uint16_t)(cpu.y + 24);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 03:ECD1 OamBufferClear
 * WHAT IT DOES: Moves every sprite in the sprite table copy off screen at
 * X = Y = $F0 and clears the high table (small size, X bit 8 clear).
 * HOW: Word stores of $F0F0 to the X/Y pair of entries 127 down to 0, then
 * word stores of zero over the 32-byte high table from the end.
 * Entry: any widths. Exit: A = 0; X = $FFFE ($FE when X is 8-bit); P
 * unchanged.
 */
void OamBufferClear(void)
{
    int x;

    for (x = 0x01FC; x >= 0; x -= 4)
        wr16(0x000375 + (unsigned)x, 0xF0F0);
    for (x = 0x1E; x >= 0; x -= 2)
        wr16(0x000575 + (unsigned)x, 0x0000);
}

void abi_OamBufferClear(void)
{
    OamBufferClear();
    cpu.a = 0x0000;
    cpu.x = (cpu.p & P_X) ? 0x00FE : 0xFFFE;
}

/*
 * @routine 03:FEF9 OamBufferHideLarge
 * WHAT IT DOES: Moves every sprite below the screen (Y = $F0) and sets the
 * high table to $AA: large size, X bit 8 clear, for all 128.
 * HOW: Byte stores of $F0 to each entry's Y, then $AA to each high table
 * byte, both counting up.
 * Entry: any widths. Exit: A low byte = $AA; X = $20; P
 * unchanged.
 */
void OamBufferHideLarge(void)
{
    unsigned x;

    for (x = 0; x < 0x200; x += 4)
        wr8(0x000376 + x, 0xF0);
    for (x = 0; x < 0x20; x++)
        wr8(0x000575 + x, 0xAA);
}

void abi_OamBufferHideLarge(void)
{
    OamBufferHideLarge();
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xAA);
    cpu.x = 0x0020;
}

/*
 * @routine 03:E9FE OamFromList
 * WHAT IT DOES: Writes sprite entries from the list at $F883 + Y in the data
 * bank into the sprite table copy at $0375 + X, until an $FF byte.
 * HOW: Each list item is either $FE, one byte, which hides the entry by
 * storing $FE as its Y, or four bytes (X, Y, tile, attributes) copied as
 * they are. X steps four per entry.
 * Entry: M 8-bit, X 16-bit; X = sprite table offset, Y = list offset. Exit:
 * X, Y past the last entry and item; A low byte = $FF; Z, C set, N clear.
 */
void OamFromList(uint8_t bank, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;

    while ((v = rd8(b | (uint16_t)(0xF883 + *y))) != 0xFF) {
        if (v == 0xFE) {
            wr8(b | (uint16_t)(0x0376 + *x), v);
        } else {
            wr8(b | (uint16_t)(0x0375 + *x), v);
            wr8(b | (uint16_t)(0x0376 + *x), rd8(b | (uint16_t)(0xF884 + *y)));
            wr8(b | (uint16_t)(0x0377 + *x), rd8(b | (uint16_t)(0xF885 + *y)));
            wr8(b | (uint16_t)(0x0378 + *x), rd8(b | (uint16_t)(0xF886 + *y)));
            *y = (uint16_t)(*y + 3);
        }
        *y = (uint16_t)(*y + 1);
        *x = (uint16_t)(*x + 4);
    }
}

void abi_OamFromList(void)
{
    OamFromList(cpu.db, &cpu.x, &cpu.y);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_C);
}

/*
 * @routine 02:FB8A OamSprite0AddY
 * WHAT IT DOES: Moves sprite 0 of the sprite table copy down by A lines.
 * HOW: An 8-bit add to its Y byte ($0376) through the data bank.
 * Entry: M 8-bit; A = lines. Exit: A low byte = the new Y; N, Z, C, V from
 * the add.
 */
uint8_t OamSprite0AddY(uint8_t bank, uint8_t lines, uint8_t *p)
{
    uint32_t a = (uint32_t)bank << 16 | 0x0376;
    uint8_t y = rd8(a);
    unsigned s = (unsigned)lines + y;

    *p = (uint8_t)((nz8(*p, (uint8_t)s) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(lines ^ y) & (lines ^ (uint8_t)s) & 0x80) ? P_V : 0));
    wr8(a, (uint8_t)s);
    return (uint8_t)s;
}

void abi_OamSprite0AddY(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | OamSprite0AddY(cpu.db, (uint8_t)cpu.a, &p));
    cpu.p = p;
}

/* Set the attribute byte of `count` sprites from entry `first` on. */
static uint16_t oam_attr_fill(uint8_t bank, uint16_t first, uint16_t count, uint8_t attr)
{
    uint16_t x = (uint16_t)(first << 2);

    do {
        wr8((uint32_t)bank << 16 | (uint16_t)(0x0378 + x), attr);
        x = (uint16_t)(x + 4);
    } while (--count);
    return x;
}

/* The common exit of the three attribute fills. */
static void abi_oam_attr_fill(uint16_t count, uint8_t attr)
{
    uint16_t a4 = (uint16_t)(cpu.a << 2);

    cpu.p = (uint8_t)((cpu.p & ~(P_C | P_N | P_M)) | ((cpu.a << 1) & 0x8000 ? P_C : 0) | P_Z);
    cpu.x = oam_attr_fill(cpu.db, cpu.a, count, attr);
    cpu.y = 0;
    cpu.a = (uint16_t)((a4 & 0xFF00) | attr);
}

/*
 * @routine 02:FB92 OamAttr36x4
 * WHAT IT DOES: Gives 4 sprites from entry A attribute byte $36.
 * HOW: Byte stores to each entry's attributes ($0378 + 4n) through the data
 * bank; the loop is OamAttr38x7's.
 * Entry: M 16-bit, X 16-bit; A = first entry. Exit: A = (A * 4) with $36 as
 * its low byte; X past the last entry; Y = 0; M 16-bit; Z set, N clear; C
 * from the doubling.
 */
void OamAttr36x4(uint8_t bank, uint16_t first)
{
    oam_attr_fill(bank, first, 4, 0x36);
}

void abi_OamAttr36x4(void)
{
    abi_oam_attr_fill(4, 0x36);
}

/*
 * @routine 02:FBC2 OamAttr38x6
 * WHAT IT DOES: Gives 6 sprites from entry A attribute byte $38.
 * HOW: As OamAttr36x4.
 * Entry: as OamAttr36x4. Exit: as OamAttr36x4, low byte $38.
 */
void OamAttr38x6(uint8_t bank, uint16_t first)
{
    oam_attr_fill(bank, first, 6, 0x38);
}

void abi_OamAttr38x6(void)
{
    abi_oam_attr_fill(6, 0x38);
}

/*
 * @routine 02:FBCE OamAttr38x7
 * WHAT IT DOES: Gives 7 sprites from entry A attribute byte $38.
 * HOW: As OamAttr36x4; the loop at $02:FBD8 is shared with the two above.
 * Entry: as OamAttr36x4. Exit: as OamAttr36x4, low byte $38.
 */
void OamAttr38x7(uint8_t bank, uint16_t first)
{
    oam_attr_fill(bank, first, 7, 0x38);
}

void abi_OamAttr38x7(void)
{
    abi_oam_attr_fill(7, 0x38);
}

/*
 * @routine 02:FC83 OamHideEntries3And9
 * WHAT IT DOES: Hides the sprites at entries 3 and 9 after offset Y.
 * HOW: Stores $F000 (X = 0, Y = $F0) to their position words ($0381 + Y,
 * $0399 + Y) through the data bank.
 * Entry: M 16-bit; Y = byte offset. Exit: A = $F000; N set, Z clear.
 */
void OamHideEntries3And9(uint8_t bank, uint16_t y)
{
    uint32_t b = (uint32_t)bank << 16;

    wr16(b | (uint16_t)(0x0381 + y), 0xF000);
    wr16(b | (uint16_t)(0x0399 + y), 0xF000);
}

void abi_OamHideEntries3And9(void)
{
    OamHideEntries3And9(cpu.db, cpu.y);
    cpu.a = 0xF000;
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N);
}

/*
 * @routine 02:FC8D OamHideEntries0To2
 * WHAT IT DOES: Hides the three sprites from offset Y.
 * HOW: As OamHideEntries3And9, at $0375, $0379 and $037D + Y.
 * Entry: as OamHideEntries3And9. Exit: as OamHideEntries3And9.
 */
void OamHideEntries0To2(uint8_t bank, uint16_t y)
{
    uint32_t b = (uint32_t)bank << 16;

    wr16(b | (uint16_t)(0x0375 + y), 0xF000);
    wr16(b | (uint16_t)(0x0379 + y), 0xF000);
    wr16(b | (uint16_t)(0x037D + y), 0xF000);
}

void abi_OamHideEntries0To2(void)
{
    OamHideEntries0To2(cpu.db, cpu.y);
    cpu.a = 0xF000;
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | P_N);
}
