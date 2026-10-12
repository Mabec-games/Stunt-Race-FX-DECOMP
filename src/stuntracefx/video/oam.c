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

/*
 * @routine 04:E317 OamSetPalette
 * WHAT IT DOES: Sets the palette bits (1-3) of the attribute byte of X
 * sprites from offset Y to the bits in A.
 * HOW: The bits are kept in direct page $F1; each attribute byte ($0378 + Y,
 * through the data bank) keeps its other bits.
 * Entry: X 16-bit, D = $0000; A low byte = palette bits, X = count (0 means
 * 65536), Y = byte offset. Exit: A low byte = the last attribute; X = 0; Y
 * past the last sprite; M 8-bit; Z set, N clear.
 */
uint8_t OamSetPalette(uint8_t bank, uint8_t bits, uint16_t count, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v = 0;

    wr8(0x0000F1, bits);
    do {
        v = (uint8_t)((rd8(b | (uint16_t)(0x0378 + *y)) & 0xF1) | rd8(0x0000F1));
        wr8(b | (uint16_t)(0x0378 + *y), v);
        *y = (uint16_t)(*y + 4);
    } while (--count);
    return v;
}

void abi_OamSetPalette(void)
{
    uint8_t v = OamSetPalette(cpu.db, (uint8_t)cpu.a, cpu.x, &cpu.y);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_M);
}

/* Sprites 52-56 (X offset $10 into the block at $0435): two X positions
   and three attribute bytes. */
static uint8_t oam_block_52(uint8_t bank, uint8_t x0, uint8_t dx, uint8_t attr_a, uint8_t attr_b, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    unsigned s = (unsigned)x0 + dx;

    wr8(b | 0x0445, x0);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0) | ((~(x0 ^ dx) & (x0 ^ (uint8_t)s) & 0x80) ? P_V : 0));
    wr8(b | 0x0449, (uint8_t)s);
    wr8(b | 0x0450, attr_a);
    wr8(b | 0x0454, attr_a);
    wr8(b | 0x0458, attr_b);
    return attr_b;
}

/*
 * @routine 07:C33A OamBlock52Left
 * WHAT IT DOES: Places sprites 52 and 53 at X = $50 and $72 and gives
 * sprites 54-56 the attribute bytes $0C, $0C, $06.
 * HOW: Byte stores through the data bank (oam_block_52).
 * Entry: M 8-bit. Exit: A low byte = $06; X = $10 (16-bit); N, Z clear; C,
 * V from the add.
 */
void OamBlock52Left(uint8_t bank, uint8_t *p)
{
    oam_block_52(bank, 0x50, 0x22, 0x0C, 0x06, p);
}

void abi_OamBlock52Left(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_block_52(cpu.db, 0x50, 0x22, 0x0C, 0x06, &p));
    cpu.x = 0x0010;
    cpu.p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/*
 * @routine 07:C36A OamBlock52Right
 * WHAT IT DOES: Places sprites 52 and 53 at X = $87 and $A1 and gives
 * sprites 54-56 the attribute bytes $06, $06, $0C.
 * HOW: As OamBlock52Left.
 * Entry: as OamBlock52Left. Exit: as OamBlock52Left, A low byte = $0C.
 */
void OamBlock52Right(uint8_t bank, uint8_t *p)
{
    oam_block_52(bank, 0x87, 0x1A, 0x06, 0x0C, p);
}

void abi_OamBlock52Right(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_block_52(cpu.db, 0x87, 0x1A, 0x06, 0x0C, &p));
    cpu.x = 0x0010;
    cpu.p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/*
 * @routine 07:C572 OamColumn8
 * WHAT IT DOES: Fills eight sprites from offset X: position words from
 * direct page $F1 (stepping $10), tile words from $F3 (stepping 2).
 * HOW: 16-bit stores; $F1 and $F3 are left past the last values.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = byte offset. Exit: A = the last
 * $F3; X past the last sprite; Y = 0; Z set, N clear; C, V from the last
 * add.
 */
uint16_t OamColumn8(uint8_t bank, uint16_t *x, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v = 0, s;
    unsigned i;

    for (i = 0; i < 8; i++) {
        v = rd16(0x0000F1);
        wr16(b | (uint16_t)(0x0375 + *x), v);
        wr16(0x0000F1, (uint16_t)(v + 0x10));
        v = rd16(0x0000F3);
        wr16(b | (uint16_t)(0x0377 + *x), v);
        s = (uint16_t)(v + 2);
        *p = (uint8_t)((*p & ~(P_C | P_V)) | ((unsigned)v + 2 > 0xFFFF ? P_C : 0) | ((~v & s & 0x8000) ? P_V : 0));
        wr16(0x0000F3, s);
        v = s;
        *x = (uint16_t)(*x + 4);
    }
    return v;
}

void abi_OamColumn8(void)
{
    uint8_t p = cpu.p;

    cpu.a = OamColumn8(cpu.db, &cpu.x, &p);
    cpu.y = 0;
    cpu.p = (uint8_t)((p & ~P_N) | P_Z);
}

/*
 * @routine 07:C50E OamHighBits58
 * WHAT IT DOES: Sets $0E15 to $F8 and turns on size or X-high bits in high
 * table bytes $16-$19 (sprites 88-103): $40, $55, $55, $15.
 * HOW: Byte ORs through the data bank.
 * Entry: any widths. Exit: A low byte = the last byte; X = $10 (high byte
 * clear); Y high byte clear; P unchanged.
 */
uint8_t OamHighBits58(uint8_t bank)
{
    static const uint8_t bits[4] = {0x40, 0x55, 0x55, 0x15};
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v = 0;
    unsigned i;

    wr8(b | 0x0E15, 0xF8);
    for (i = 0; i < 4; i++) {
        v = (uint8_t)(rd8(b | (uint16_t)(0x058B + i)) | bits[i]);
        wr8(b | (uint16_t)(0x058B + i), v);
    }
    return v;
}

void abi_OamHighBits58(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | OamHighBits58(cpu.db));
    cpu.x = 0x0010;
    cpu.y &= 0x00FF;
}

void abi_OamBufferHideLarge(void);

/*
 * @routine 03:FEF5 OamBufferHideLargeLong
 * WHAT IT DOES: OamBufferHideLarge for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as OamBufferHideLarge. Exit: as OamBufferHideLarge.
 */
void OamBufferHideLargeLong(void)
{
    OamBufferHideLarge();
}

void abi_OamBufferHideLargeLong(void)
{
    abi_OamBufferHideLarge();
}

/*
 * @routine 04:E2EC OamPaletteFromTableE30F
 * WHAT IT DOES: Sets the palette bits of 22 sprites from entry $0D79 to the
 * byte of the table at $04:E30F for $0D48's low byte.
 * HOW: OamSetPalette with X = $16, Y = $0D79 * 4.
 * Entry: any widths, D = $0000. Exit: A = the last attribute byte (high
 * byte clear); X = 0; Y past the last sprite; P unchanged.
 */
uint8_t OamPaletteFromTableE30F(uint8_t bank, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;

    *y = (uint16_t)(rd16(b | 0x0D79) << 2);
    return OamSetPalette(bank, rd8(0x04E30F + (rd16(b | 0x0D48) & 0x00FF)), 0x0016, y);
}

void abi_OamPaletteFromTableE30F(void)
{
    cpu.a = OamPaletteFromTableE30F(cpu.db, &cpu.y);
    cpu.x = 0;
}

/*
 * @routine 04:E23F OamPaletteFromTableE26A
 * WHAT IT DOES: Sets the palette bits of 22 sprites from entry $0D7B + 13 to
 * $0A when $10D8 is 0, otherwise to the byte of the table at $04:E26A for
 * ($10D8 - 1)'s low byte.
 * HOW: OamSetPalette with X = $16.
 * Entry: any widths, D = $0000. Exit: as OamPaletteFromTableE30F.
 */
uint8_t OamPaletteFromTableE26A(uint8_t bank, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n = rd16(b | 0x10D8);
    uint8_t bits = n ? rd8(0x04E26A + ((uint16_t)(n - 1) & 0x00FF)) : 0x0A;

    *y = (uint16_t)((uint16_t)(rd16(b | 0x0D7B) + 0x0D) << 2);
    return OamSetPalette(bank, bits, 0x0016, y);
}

void abi_OamPaletteFromTableE26A(void)
{
    cpu.a = OamPaletteFromTableE26A(cpu.db, &cpu.y);
    cpu.x = 0;
}

/*
 * @routine 04:E28D OamPalettesE28D
 * WHAT IT DOES: Sets sprite palette bits in three runs from entry $0D75: 18
 * sprites from the table at $04:E30B for $0D48's low byte, then 6 sprites
 * from entry + $16 and 6 from entry + $1C from the table at $04:E313 for
 * the bytes at $0D89 and $0D8A.
 * HOW: OamSetPalette three times.
 * Entry: any widths, D = $0000. Exit: A = the last attribute byte (high
 * byte clear); X = 0; Y past the last sprite; P unchanged.
 */
uint8_t OamPalettesE28D(uint8_t bank, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;

    *y = (uint16_t)(rd16(b | 0x0D75) << 2);
    OamSetPalette(bank, rd8(0x04E30B + (rd16(b | 0x0D48) & 0x00FF)), 0x0012, y);
    *y = (uint16_t)((uint16_t)(rd16(b | 0x0D75) + 0x16) << 2);
    OamSetPalette(bank, rd8(0x04E313 + (rd16(b | 0x0D89) & 0x00FF)), 0x0006, y);
    *y = (uint16_t)((uint16_t)(rd16(b | 0x0D75) + 0x1C) << 2);
    return OamSetPalette(bank, rd8(0x04E313 + (rd16(b | 0x0D8A) & 0x00FF)), 0x0006, y);
}

void abi_OamPalettesE28D(void)
{
    cpu.a = OamPalettesE28D(cpu.db, &cpu.y);
    cpu.x = 0;
}

/*
 * @routine 07:C502 OamHighBits18
 * WHAT IT DOES: Sets $0E14 to $F8 and turns on size or X-high bits in high
 * table bytes 6-9 (sprites 24-39): $40, $55, $55, $15.
 * HOW: Enters OamHighBits58's OR sequence with X = 0 instead of $10.
 * Entry: any widths. Exit: A low byte = the last byte; X = 0; Y high byte
 * clear; P unchanged.
 */
uint8_t OamHighBits18(uint8_t bank)
{
    static const uint8_t bits[4] = {0x40, 0x55, 0x55, 0x15};
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v = 0;
    unsigned i;

    wr8(b | 0x0E14, 0xF8);
    for (i = 0; i < 4; i++) {
        v = (uint8_t)(rd8(b | (uint16_t)(0x057B + i)) | bits[i]);
        wr8(b | (uint16_t)(0x057B + i), v);
    }
    return v;
}

void abi_OamHighBits18(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | OamHighBits18(cpu.db));
    cpu.x = 0x0000;
    cpu.y &= 0x00FF;
}

/*
 * @routine 07:C4F1 OamHighBits58Park112
 * WHAT IT DOES: OamHighBits58, then sets the X-high bit of sprites 112-127
 * (high table bytes $1C-$1F = $55), which puts them off the left edge.
 * HOW: Two word stores of $5555 to $0591 and $0593 through the data bank.
 * Entry: any widths. Exit: A = $5555; X = $10 (high byte clear); Y high
 * byte clear; P unchanged.
 */
void OamHighBits58Park112(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;

    OamHighBits58(bank);
    wr16(b | 0x0591, 0x5555);
    wr16(b | 0x0593, 0x5555);
}

void abi_OamHighBits58Park112(void)
{
    OamHighBits58Park112(cpu.db);
    cpu.a = 0x5555;
    cpu.x = 0x0010;
    cpu.y &= 0x00FF;
}

/*
 * @routine 07:C45D OamHide36And96
 * WHAT IT DOES: Hides sprites 36-56 and 96-111, and clears the size and
 * X-high bits of sprites 96-111 (high table bytes $18-$1B).
 * HOW: OamHideRange twice, then four byte stores of 0 through the data bank.
 * Entry: any widths. Exit: A low byte = 0; X = $01C0 ($C0 when X is 8-bit);
 * Y = 0; P unchanged.
 */
void OamHide36And96(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    unsigned i;

    OamHideRange(0x0090, 0x0015);
    OamHideRange(0x0180, 0x0010);
    for (i = 0; i < 4; i++)
        wr8(b | (uint16_t)(0x058D + i), 0x00);
}

void abi_OamHide36And96(void)
{
    OamHide36And96(cpu.db);
    cpu.a &= 0xFF00;
    cpu.x = cpu.p & P_X ? 0x00C0 : 0x01C0;  /* the final pull restores X's width */
    cpu.y = 0x0000;
}

/*
 * @routine 07:C4A4 OamHide75Up
 * WHAT IT DOES: Sets $0E0C and $0E14 to $F8 and hides sprites 75-127; when
 * the HUD mode ($0D2B) is nonzero sprites 87-95 are left alone.
 * HOW: OamHideRange from sprite 75: 53 sprites, or 12 then 32 from sprite
 * 96 when the mode is set.
 * Entry: any widths. Exit: A low byte = $F0; X = $0200 (0 when X is
 * 8-bit); Y = 0; P unchanged.
 */
void OamHide75Up(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;

    wr8(b | 0x0E0C, 0xF8);
    wr8(b | 0x0E14, 0xF8);
    if (rd8(b | 0x0D2B)) {
        OamHideRange(0x012C, 0x000C);
        OamHideRange(0x0180, 0x0020);
    } else {
        OamHideRange(0x012C, 0x0035);
    }
}

void abi_OamHide75Up(void)
{
    OamHide75Up(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xF0);
    cpu.x = cpu.p & P_X ? 0x0000 : 0x0200;  /* the final pull restores X's width */
    cpu.y = 0x0000;
}

/*
 * @routine 07:C388 OamLargeColumns
 * WHAT IT DOES: Lays out three columns of eight large sprites: sprites 40-47
 * at $4840 with tiles from $0660, 96-103 at $5840 from $0680, 104-111 at
 * $6840 from $06A0; positions step down $10 and tile words by 2. Sets the
 * size bit of all 24 (high table bytes 10, 11 and 24-27 = $AA).
 * HOW: OamColumn8 three times with its inputs in direct page $F1 and $F3,
 * then byte stores through the data bank.
 * Entry: any widths, D = $0000. Exit: A = $06AA; X = $01C0 ($C0 when X is
 * 8-bit); Y = 0; P unchanged.
 */
void OamLargeColumns(uint8_t bank)
{
    static const uint16_t pos[3] = {0x4840, 0x5840, 0x6840};
    static const uint16_t tile[3] = {0x0660, 0x0680, 0x06A0};
    static const uint16_t first[3] = {0x00A0, 0x0180, 0x01A0};
    static const uint16_t high[6] = {0x057F, 0x0580, 0x058D, 0x058E, 0x058F, 0x0590};
    uint32_t b = (uint32_t)bank << 16;
    uint8_t p = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        uint16_t x = first[i];

        wr16(0x0000F1, pos[i]);
        wr16(0x0000F3, tile[i]);
        OamColumn8(bank, &x, &p);
    }
    for (i = 0; i < 6; i++)
        wr8(b | high[i], 0xAA);
}

void abi_OamLargeColumns(void)
{
    OamLargeColumns(cpu.db);
    cpu.a = 0x06AA;
    cpu.x = cpu.p & P_X ? 0x00C0 : 0x01C0;  /* the final pull restores X's width */
    cpu.y = 0x0000;
}

/*
 * @routine 07:C3DD OamPlaceRows75And112
 * WHAT IT DOES: Sets sprite Y positions: sprites 75-86 to $93, 112-113 to
 * $70, 114-115 to $80, and, unless $10CD is set, 116-127 to $8C or $94
 * (119, 121, 123, 125 and 127 at $94; the rest at $8C). Clears the size and
 * X-high bits of sprites 40-47 and 96-111 (high table bytes 10, 11, 24-27).
 * HOW: Byte stores through the data bank.
 * Entry: any widths. Exit: A low byte = 0; X, Y unchanged; P unchanged.
 */
void OamPlaceRows75And112(uint8_t bank)
{
    static const uint8_t at8c[7] = {116, 117, 118, 120, 122, 124, 126};
    static const uint8_t at94[5] = {119, 121, 123, 125, 127};
    static const uint16_t high[6] = {0x057F, 0x0580, 0x058D, 0x058E, 0x058F, 0x0590};
    uint32_t b = (uint32_t)bank << 16;
    unsigned i;

    if (!rd8(b | 0x10CD)) {
        for (i = 0; i < 7; i++)
            wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * at8c[i]), 0x8C);
        for (i = 0; i < 5; i++)
            wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * at94[i]), 0x94);
    }
    for (i = 75; i <= 86; i++)
        wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * i), 0x93);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 112), 0x70);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 113), 0x70);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 114), 0x80);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 115), 0x80);
    for (i = 0; i < 6; i++)
        wr8(b | high[i], 0x00);
}

void abi_OamPlaceRows75And112(void)
{
    OamPlaceRows75And112(cpu.db);
    cpu.a &= 0xFF00;
}

/* A strip with a marker sprite: the strip at the position in direct page
   $02 (X at entry), then the marker at that position plus the value, less
   the strip's span once the value reaches it, in the slot before the
   strip. The OAM writes for the marker go through the data bank. */
static uint16_t bar_marker(uint8_t bank, uint16_t pos, uint16_t v, uint16_t y, uint16_t span,
                           uint16_t tile_far, uint16_t tile_near)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t tile;

    if (v >= span) {
        wr16(b | (uint16_t)(0x0375 + y), (uint16_t)(v + pos - span));
        tile = tile_far;
    } else {
        wr16(b | (uint16_t)(0x0375 + y), (uint16_t)(v + pos));
        tile = tile_near;
    }
    wr16(b | (uint16_t)(0x0377 + y), tile);
    return tile;
}

/*
 * @routine 7F:094E SpriteBar4WithMarker
 * WHAT IT DOES: Shows a value of 0-63 (larger values are shown as 63) as a
 * four-sprite strip at position X with the tiles SpriteStrip4 picks for it,
 * plus a marker sprite at X + value (X + value - 16, with a different tile,
 * from 16 up) in the sprite slot Y.
 * HOW: Value to direct page $04, position to $02; SpriteStrip4 fills slots
 * Y + 4 on; then the marker: tile word $3622 below 16, $3C20 from 16.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = value, X = position (X, Y
 * pair), Y = marker slot offset in the sprite table. Exit: A = the marker's
 * tile word; X as SpriteStrip4 leaves it; Y unchanged; P unchanged.
 */
uint16_t SpriteBar4WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y)
{
    uint16_t v = value < 0x3F ? value : 0x3F;

    wr8(0x000004, (uint8_t)v);
    wr16(0x000002, pos);
    SpriteStrip4(v, (uint16_t)(y + 4));
    return bar_marker(bank, pos, v, y, 0x0010, 0x3C20, 0x3622);
}

void abi_SpriteBar4WithMarker(void)
{
    uint16_t v = (uint8_t)cpu.a < 0x3F ? (uint8_t)cpu.a : 0x3F;

    cpu.a = SpriteBar4WithMarker(cpu.db, (uint8_t)cpu.a, cpu.x, cpu.y);
    cpu.x = (uint16_t)(((v >> 1) & 0x00F8) + 8);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 7F:09ED SpriteBar6WithMarker
 * WHAT IT DOES: Shows a value as a six-sprite strip at position X with the
 * tiles SpriteStrip6 picks for it, plus a marker sprite at X + value
 * (X + value - 8, with a different tile, from 8 up) in the sprite slot Y.
 * HOW: As SpriteBar4WithMarker without the limit: SpriteStrip6 fills slots
 * Y + 4 on; the marker's tile word is $3C1E below 8, $3C1F from 8.
 * Entry: as SpriteBar4WithMarker. Exit: A = the marker's tile word; X as
 * SpriteStrip6 leaves it; Y unchanged; P unchanged.
 */
uint16_t SpriteBar6WithMarker(uint8_t bank, uint8_t value, uint16_t pos, uint16_t y)
{
    wr8(0x000004, value);
    wr16(0x000002, pos);
    SpriteStrip6(value, (uint16_t)(y + 4));
    return bar_marker(bank, pos, value, y, 0x0008, 0x3C1F, 0x3C1E);
}

void abi_SpriteBar6WithMarker(void)
{
    uint8_t v = (uint8_t)cpu.a;

    cpu.a = SpriteBar6WithMarker(cpu.db, v, cpu.x, cpu.y);
    cpu.x = (uint16_t)(((v & 0x0038) << 1) + 12);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

typedef struct {
    uint16_t a, x, y;
    uint8_t p;
} MetaspriteEnd;

/* The body of MetaspriteDraw, with everything its exit leaves. */
static MetaspriteEnd metasprite_draw(uint8_t bank, uint16_t group, uint16_t slot, uint8_t p)
{
    uint32_t b = (uint32_t)bank << 16;
    MetaspriteEnd e;
    uint16_t x = (uint16_t)(group << 2), y = (uint16_t)(slot << 2), mask, hi;
    uint8_t n, lo = 0, v;
    unsigned s;

    n = (uint8_t)rd16(0x04EDCD + x);
    wr16(0x0000F1, rd16(0x04EDCD + x));
    x = rd16(0x04EDCB + x);
    hi = (uint16_t)(x & 0xFF00);
    do {
        s = (unsigned)rd8(0x04EE4B + x) + rd8(b | 0x0D3B);
        wr8(b | (uint16_t)(0x0375 + y), (uint8_t)s);
        v = rd8(0x04EE4C + x);
        s = (unsigned)v + rd8(b | 0x0D3C);
        p = (uint8_t)(p & ~(P_C | P_V));
        if (s > 0xFF)
            p |= P_C;
        if (~(v ^ rd8(b | 0x0D3C)) & (v ^ s) & 0x80)
            p |= P_V;
        wr8(b | (uint16_t)(0x0376 + y), (uint8_t)s);
        wr8(b | (uint16_t)(0x0377 + y), rd8(0x04EE4D + x));
        wr8(b | (uint16_t)(0x0378 + y), rd8(0x04EE4E + x));
        lo = rd8(0x04EE4F + x);
        if (!(lo & 0x01)) {
            mask = rd16(b | (uint16_t)(0xFA8D + ((y >> 2) & 0x0003)));
            wr16(0x0000F3, mask);
            hi = (uint16_t)(mask & 0xFF00);
            lo = (uint8_t)(rd8(0x000575 + (y >> 4)) & (uint8_t)mask);
            wr8(0x000575 + (y >> 4), lo);
            p = (uint8_t)((p & ~P_C) | ((y >> 3) & 1 ? P_C : 0));
        }
        x = (uint16_t)(x + 5);
        y = (uint16_t)(y + 4);
        n = (uint8_t)(n - 1);
        wr8(0x0000F1, n);
    } while (n);
    e.a = (uint16_t)(hi | lo);
    e.x = x;
    e.y = y;
    e.p = (uint8_t)(((p & ~(P_N | P_M)) | P_Z));
    return e;
}

/*
 * @routine 02:FA19 MetaspriteDraw
 * WHAT IT DOES: Draws metasprite X into the sprite table from slot Y on:
 * each piece's X and Y offsets are added to the origin at $0D3B/$0D3C and
 * its tile and attribute bytes copied; a piece whose fifth byte has bit 0
 * clear is made small (its size bit in the high table cleared).
 * HOW: The group table at $04:EDCB (4 bytes per metasprite: first piece,
 * piece count) and 5-byte pieces at $04:EE4B; the count runs down in direct
 * page $F1 (8-bit, after a 16-bit store), the size masks $FD, $F7, $DF, $7F
 * come from $02:FA8D through direct page $F3. Origin and sprite table go
 * through the data bank.
 * Entry: M 16-bit, X 16-bit, D = $0000; X = metasprite, Y = first slot.
 * Exit: A = the last piece's fifth byte (or the high-table byte after its
 * mask), high byte from the start of the group (or the last mask word);
 * X, Y past the pieces; M 16-bit; N clear, Z set; C, V from the last Y
 * add, C from bit 3 of the slot offset when the last piece was made small.
 */
uint16_t MetaspriteDraw(uint8_t bank, uint16_t group, uint16_t slot)
{
    return metasprite_draw(bank, group, slot, 0).a;
}

void abi_MetaspriteDraw(void)
{
    MetaspriteEnd e = metasprite_draw(cpu.db, cpu.x, cpu.y, cpu.p);

    cpu.a = e.a;
    cpu.x = e.x;
    cpu.y = e.y;
    cpu.p = e.p;
}

/* The cycled palette (attribute bits 1-3) for phase 1 and for phase 2 or
   more: by which of bits 3 and 1 the piece's own attributes have set (bit 3,
   else bit 1, else neither). */
static uint8_t cycled_attr(uint8_t attr, uint8_t phase)
{
    static const uint8_t pal[2][3] = {{0x04, 0x08, 0x06}, {0x06, 0x04, 0x08}};
    unsigned k = (attr & 0x08) ? 0 : (attr & 0x02) ? 1 : 2;

    return (uint8_t)((attr & 0xF1) | pal[phase == 1][k]);
}

/* The shared body: set selector to $0D35, then from $02:F958 on. */
static uint16_t flashing_metasprite_draw(uint8_t bank, uint16_t set, uint16_t *x, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t i, ph;
    uint8_t n, a = 0, phase;

    wr16(b | 0x0D35, set);
    for (i = 0; i < 0x20; i++)
        wr16(b | (uint16_t)(0x0575 + i), 0xAAAA);
    if (!rd16(b | 0x0D37)) {
        *x = 0x0020;
        return 0;
    }
    if (!(rd16(b | 0x0E3B) & 0x0003)) {
        ph = (uint16_t)(rd16(b | 0x0D39) + 1);
        wr16(b | 0x0D39, ph < 3 ? ph : 0);
    }
    i = (uint16_t)(rd16(b | 0x0D35) << 2);
    wr16(0x0000F1, rd16(0x1CE5AE + i));
    n = rd8(0x0000F1);
    *x = rd16(0x1CE5AC + i);
    *y = 0;
    i = (uint16_t)(*x & 0xFF00);
    do {
        wr8(b | (uint16_t)(0x0375 + *y), (uint8_t)(rd8(0x1CE62C + *x) + rd8(b | 0x0D37)));
        wr8(b | (uint16_t)(0x0376 + *y), (uint8_t)(rd8(0x1CE62D + *x) + rd8(b | 0x0D38)));
        wr8(b | (uint16_t)(0x0377 + *y), rd8(0x1CE62E + *x));
        phase = rd8(b | 0x0D39);
        a = rd8(0x1CE62F + *x);
        wr8(b | (uint16_t)(0x0378 + *y), phase ? cycled_attr(a, phase) : a);
        a = rd8(0x1CE630 + *x);
        *x = (uint16_t)(*x + 5);
        *y = (uint16_t)(*y + 4);
        n = (uint8_t)(n - 1);
        wr8(0x0000F1, n);
    } while (n);
    return (uint16_t)(i | a);
}

/*
 * @routine 02:F94F FlashingMetaspriteDraw
 * WHAT IT DOES: Resets the high sprite table to large sprites ($AA in
 * $0575-$0595) and, when its position at $0D37/$0D38 is non-zero, draws
 * metasprite 8 of the bank $1C set from sprite 0 with its palettes cycled:
 * the phase in $0D39 steps 0, 1, 2 every fourth frame; phase 0 keeps each
 * piece's palette, phases 1 and 2 swap the three palettes around.
 * HOW: $0D35 = 8; the set's group table at $1C:E5AC (first piece, count)
 * and 5-byte pieces at $1C:E62C as MetaspriteDraw reads them, the count
 * running down in direct page $F1. Work RAM through the data bank.
 * Entry: any widths, D = $0000. Exit: A = the last piece's fifth byte, high
 * byte from its group's first piece (A = 0 and X = $20 when not drawn); X,
 * Y past the pieces (low bytes with the 8-bit index registers P restores);
 * P unchanged.
 */
uint16_t FlashingMetaspriteDraw(uint8_t bank, uint16_t *x, uint16_t *y)
{
    return flashing_metasprite_draw(bank, 0x0008, x, y);
}

void abi_FlashingMetaspriteDraw(void)
{
    cpu.a = FlashingMetaspriteDraw(cpu.db, &cpu.x, &cpu.y);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 02:F926 FlashingMetaspriteDraw0
 * WHAT IT DOES: FlashingMetaspriteDraw with metasprite 0 of the bank $1C
 * set.
 * HOW: $0D35 = 0, then a jump into FlashingMetaspriteDraw after its own
 * store.
 * Entry: as FlashingMetaspriteDraw. Exit: as FlashingMetaspriteDraw.
 */
uint16_t FlashingMetaspriteDraw0(uint8_t bank, uint16_t *x, uint16_t *y)
{
    return flashing_metasprite_draw(bank, 0x0000, x, y);
}

void abi_FlashingMetaspriteDraw0(void)
{
    cpu.a = FlashingMetaspriteDraw0(cpu.db, &cpu.x, &cpu.y);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 02:FB9E OamAttr36x6
 * WHAT IT DOES: Gives 6 sprites from entry A attribute byte $36.
 * HOW: As OamAttr36x4.
 * Entry: as OamAttr36x4. Exit: as OamAttr36x4.
 */
void OamAttr36x6(uint8_t bank, uint16_t first)
{
    oam_attr_fill(bank, first, 6, 0x36);
}

void abi_OamAttr36x6(void)
{
    abi_oam_attr_fill(6, 0x36);
}

/*
 * @routine 02:FBB6 OamAttr38x4
 * WHAT IT DOES: Gives 4 sprites from entry A attribute byte $38.
 * HOW: As OamAttr36x4.
 * Entry: as OamAttr36x4. Exit: as OamAttr36x4, low byte $38.
 */
void OamAttr38x4(uint8_t bank, uint16_t first)
{
    oam_attr_fill(bank, first, 4, 0x38);
}

void abi_OamAttr38x4(void)
{
    abi_oam_attr_fill(4, 0x38);
}
