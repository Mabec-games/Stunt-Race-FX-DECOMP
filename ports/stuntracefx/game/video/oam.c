/*
 * oam.c: the work-RAM copy of the sprite table (OAM), copied to the PPU
 * each frame. Each sprite is four bytes: X, Y, tile, attributes.
 */
#include "machine.h"
#include "hw.h"
#include "wram.h"
#include "cpu_ops.h"

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
    cyc(0);
    do {
        cyc(48);
        wr8(OAM_BUFFER + 1 + offset, 0xF0);
        offset += 4;
    } while (cyc_loop(76, --count));
    cyc(112);
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

    cyc(170);
    wr8(OAMADDL, 0x48);
    cyc(24);
    wr8(OAMADDH, 0x00);
    for (i = 0; i < 0x2C; i++) {
        cyc(i ? 80 : 54);               /* INX, DEY, BNE taken; the first: LDY */
        v = rd8(OAM_BUFFER + 0x90 + i);
        cyc(24);
        wr8(OAMDATA, v);
    }
    cyc(114);
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
    return y >= 0xB0 ? (cyc(10), 0xF0) : y;
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
   Direct page $02 is restored and returned (A at the end); $08 ends at
   $FFFF. */
static uint16_t sprite_strip(uint16_t index, uint16_t offset, uint32_t table, uint16_t step, uint16_t n)
{
    cyc(112);
    uint16_t pos0 = rd16(0x000002), pos, v;

    cyc(70);
    wr16(0x000008, n);
    do {
        cyc(16);
        pos = rd16(0x000002);
        cyc(30);
        wr16(OAM_BUFFER + offset, pos);
        pos = (uint16_t)(pos + step);
        cyc(54);
        wr16(0x000002, pos);
        cyc(32);
        v = rd16(table + index);
        cyc(30);
        wr16(OAM_BUFFER + 2 + offset, v);
        index = (uint16_t)(index + 2);
        offset = (uint16_t)(offset + 4);
        cyc(100);
        n = rd16(0x000008);
        n = (uint16_t)(n - 1);
        cyc(14);
        wr16(0x000008, n);
    } while (cyc_loop(22, !(n & 0x8000)));
    cyc(68);
    wr16(0x000002, pos0);
    return pos0;
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
uint16_t SpriteStrip4(uint16_t selector, uint16_t offset)
{
    return sprite_strip((uint16_t)((selector >> 1) & 0x00F8), offset, 0x7F09CD, 0x0010, 3);
}

void abi_SpriteStrip4(void)
{
    uint16_t index = (uint16_t)((cpu.a >> 1) & 0x00F8);

    cpu.a = SpriteStrip4(cpu.a, cpu.y);
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
uint16_t SpriteStrip6(uint16_t selector, uint16_t offset)
{
    return sprite_strip((uint16_t)((selector & 0x0038) << 1), offset, 0x7F0A66, 0x0008, 5);
}

void abi_SpriteStrip6(void)
{
    uint16_t index = (uint16_t)((cpu.a & 0x0038) << 1);

    cpu.a = SpriteStrip6(cpu.a, cpu.y);
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

    cyc(14);
    for (x = 0x01FC; x >= 0; x -= 4)
        { cyc(110); wr16(0x000375 + (unsigned)x, 0xF0F0); }
    cyc(70);
    for (x = 0x1E; x >= 0; x -= 2)
        { cyc(82); wr16(0x000575 + (unsigned)x, 0x0000); }
    cyc(116);
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

    cyc(4);
    for (x = 0; x < 0x200; x += 4)
        { cyc(132); wr8(0x000376 + x, 0xF0); }
    cyc(76);
    for (x = 0; x < 0x20; x++)
        { cyc(90); wr8(0x000575 + x, 0xAA); }
    cyc(124);
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
    uint32_t b = (uint32_t)bank << 16, tail = 0;
    uint8_t v;

    /* each item: LDA abs,Y with its ROM byte (38), then to the store: CMP,
       BEQ, CMP, BNE (94 for $FE, 100 for an entry); after it the INYs, INXs
       and BRA back (134, or 114 after $FE with its BRA) */
    while ((v = rd8(cyc_then(tail + 38, b | (uint16_t)(0xF883 + *y)))) != 0xFF) {
        if (v == 0xFE) {
            cyc(94);
            wr8(b | (uint16_t)(0x0376 + *x), v);
            tail = 114;
        } else {
            cyc(100);
            wr8(b | (uint16_t)(0x0375 + *x), v);
            cyc(68);
            wr8(b | (uint16_t)(0x0376 + *x), rd8(b | (uint16_t)(0xF884 + *y)));
            cyc(68);
            wr8(b | (uint16_t)(0x0377 + *x), rd8(b | (uint16_t)(0xF885 + *y)));
            cyc(68);
            wr8(b | (uint16_t)(0x0378 + *x), rd8(b | (uint16_t)(0xF886 + *y)));
            *y = (uint16_t)(*y + 3);
            tail = 134;
        }
        *y = (uint16_t)(*y + 1);
        *x = (uint16_t)(*x + 4);
    }
    cyc(80);                            /* CMP #$FF, BEQ */
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
    cyc(38);
    uint8_t y = rd8(a);
    unsigned s = (unsigned)lines + y;

    *p = (uint8_t)((nz8(*p, (uint8_t)s) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(lines ^ y) & (lines ^ (uint8_t)s) & 0x80) ? P_V : 0));
    cyc(24);
    wr8(a, (uint8_t)s);
    cyc(42);
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

    cyc(12);
    do {
        cyc(122);
        wr8((uint32_t)bank << 16 | (uint16_t)(0x0378 + x), attr);
        x = (uint16_t)(x + 4);
    } while (--count);
    cyc(150);
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
    cyc(22);
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
    cyc(22);
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

    cyc(54);
    wr16(b | (uint16_t)(0x0381 + y), 0xF000);
    cyc(30);
    wr16(b | (uint16_t)(0x0399 + y), 0xF000);
    cyc(42);
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

    cyc(54);
    wr16(b | (uint16_t)(0x0375 + y), 0xF000);
    cyc(30);
    wr16(b | (uint16_t)(0x0379 + y), 0xF000);
    cyc(30);
    wr16(b | (uint16_t)(0x037D + y), 0xF000);
    cyc(42);
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

    cyc(38);
    wr8(0x0000F1, bits);
    do {
        cyc(30);
        v = (uint8_t)(rd8(b | (uint16_t)(0x0378 + *y)) & 0xF1);
        cyc(32);
        v = (uint8_t)(v | rd8(0x0000F1));
        cyc(30);
        wr8(b | (uint16_t)(0x0378 + *y), v);
        *y = (uint16_t)(*y + 4);
    } while (cyc_loop(92, --count));
    cyc(128);
    return v;
}

void abi_OamSetPalette(void)
{
    uint8_t v = OamSetPalette(cpu.db, (uint8_t)cpu.a, cpu.x, &cpu.y);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = 0;
    cpu.p = (uint8_t)((cpu.p & ~P_N) | P_Z | P_M);
}

/* Sprites 52-56 (X offset $10 into the block at $0435; $2C and $12C move
   it on by 7 and 71 sprites): two X positions and three attribute bytes. */
static uint8_t oam_block(uint8_t bank, uint16_t x, uint8_t x0, uint8_t dx, uint8_t attr_a, uint8_t attr_b, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    unsigned s = (unsigned)x0 + dx;

    cyc(92);
    wr8(b | (uint16_t)(0x0435 + x), x0);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0) | ((~(x0 ^ dx) & (x0 ^ (uint8_t)s) & 0x80) ? P_V : 0));
    cyc(60);
    wr8(b | (uint16_t)(0x0439 + x), (uint8_t)s);
    cyc(46);
    wr8(b | (uint16_t)(0x0440 + x), attr_a);
    cyc(30);
    wr8(b | (uint16_t)(0x0444 + x), attr_a);
    cyc(46);
    wr8(b | (uint16_t)(0x0448 + x), attr_b);
    cyc(42);
    return attr_b;
}

static uint8_t oam_block_52(uint8_t bank, uint8_t x0, uint8_t dx, uint8_t attr_a, uint8_t attr_b, uint8_t *p)
{
    return oam_block(bank, 0x0010, x0, dx, attr_a, attr_b, p);
}

/*
 * @routine 07:C328 OamBlock59Left
 * WHAT IT DOES: Places sprites 59 and 60 at X = $30 and $52 and gives
 * sprites 61-63 the attribute bytes $0C, $0C, $06.
 * HOW: OamBlock52Left's stores with X = $2C.
 * Entry: M 8-bit. Exit: A low byte = $06; X = $2C (16-bit); N, Z clear; C,
 * V from the add.
 */
void abi_OamBlock59Left(void)
{
    cyc(22);
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_block(cpu.db, 0x002C, 0x30, 0x22, 0x0C, 0x06, &p));
    cpu.x = 0x002C;
    cpu.p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/*
 * @routine 07:C331 OamBlock123Left
 * WHAT IT DOES: Places sprites 123 and 124 at X = $7E and $A0 and gives
 * sprites 125-127 the attribute bytes $0C, $0C, $06.
 * HOW: OamBlock52Left's stores with X = $12C. Not reached in the traces
 * so far.
 * Entry: M 8-bit. Exit: A low byte = $06; X = $12C (16-bit); N, Z clear;
 * C, V from the add.
 */
void abi_OamBlock123Left(void)
{
    cyc(22);
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_block(cpu.db, 0x012C, 0x7E, 0x22, 0x0C, 0x06, &p));
    cpu.x = 0x012C;
    cpu.p = (uint8_t)(p & ~(P_N | P_Z | P_X));
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
 * @routine 07:C358 OamBlock59Right
 * WHAT IT DOES: Places sprites 59 and 60 at X = $5F and $79 and gives
 * sprites 61-63 the attribute bytes $06, $06, $0C.
 * HOW: OamBlock52Right's stores with X = $2C.
 * Entry: M 8-bit. Exit: A low byte = $0C; X = $2C (16-bit); N, Z clear; C,
 * V from the add.
 */
void abi_OamBlock59Right(void)
{
    cyc(22);
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_block(cpu.db, 0x002C, 0x5F, 0x1A, 0x06, 0x0C, &p));
    cpu.x = 0x002C;
    cpu.p = (uint8_t)(p & ~(P_N | P_Z | P_X));
}

/*
 * @routine 07:C361 OamBlock123Right
 * WHAT IT DOES: Places sprites 123 and 124 at X = $AD and $C7 and gives
 * sprites 125-127 the attribute bytes $06, $06, $0C.
 * HOW: OamBlock52Right's stores with X = $12C.
 * Entry: M 8-bit. Exit: A low byte = $0C; X = $12C (16-bit); N, Z clear; C,
 * V from the add.
 */
void abi_OamBlock123Right(void)
{
    cyc(22);
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | oam_block(cpu.db, 0x012C, 0xAD, 0x1A, 0x06, 0x0C, &p));
    cpu.x = 0x012C;
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

    cyc(2);
    for (i = 0; i < 8; i++) {
        cyc(108);
        v = rd16(0x0000F1);
        cyc(30);
        wr16(b | (uint16_t)(0x0375 + *x), v);
        cyc(54);
        wr16(0x0000F1, (uint16_t)(v + 0x10));
        cyc(16);
        v = rd16(0x0000F3);
        cyc(30);
        wr16(b | (uint16_t)(0x0377 + *x), v);
        s = (uint16_t)(v + 2);
        *p = (uint8_t)((*p & ~(P_C | P_V)) | ((unsigned)v + 2 > 0xFFFF ? P_C : 0) | ((~v & s & 0x8000) ? P_V : 0));
        cyc(54);
        wr16(0x0000F3, s);
        v = s;
        *x = (uint16_t)(*x + 4);
    }
    cyc(128);
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

    cyc(130);
    wr8(b | 0x0E15, 0xF8);
    cyc(16);
    for (i = 0; i < 4; i++) {
        cyc(24);
        v = (uint8_t)(rd8(b | (uint16_t)(0x058B + i)) | bits[i]);
        cyc(46);
        wr8(b | (uint16_t)(0x058B + i), v);
    }
    cyc(70);
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
    cyc(46);                            /* the JSR from the long entry */
    abi_OamBufferHideLarge();
    cyc(44);                            /* the RTL */
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

    cyc(68);
    *y = (uint16_t)(rd16(b | 0x0D79) << 2);
    cyc(66);
    return cyc_then(72, OamSetPalette(bank, rd8(cyc_then(180, 0x04E30F + (rd16(b | 0x0D48) & 0x00FF))), 0x0016, y));
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
    cyc(68);
    *y = (uint16_t)((uint16_t)(rd16(b | 0x0D7B) + 0x0D) << 2);
    cyc(104);
    uint16_t n = rd16(b | 0x10D8);
    uint8_t bits = n ? rd8(cyc_then(84, 0x04E26A + ((uint16_t)(n - 1) & 0x00FF))) : 0x0A;

    cyc(132);
    return cyc_then(72, OamSetPalette(bank, bits, 0x0016, y));
}

void abi_OamPaletteFromTableE26A(void)
{
    cpu.a = OamPaletteFromTableE26A(cpu.db, &cpu.y);
    cpu.x = 0;
}

/*
 * @routine 04:E26E OamPaletteFromTableE26AByMode
 * WHAT IT DOES: Sets the palette bits of 18 sprites from entry $0D79 to the
 * byte of the table at $04:E26A for $0D48's low byte.
 * HOW: OamSetPalette with X = $12.
 * Entry: any widths, D = $0000. Exit: A = the last attribute byte (high
 * byte clear); X = 0; Y past the last sprite; P unchanged.
 */
uint8_t OamPaletteFromTableE26AByMode(uint8_t bank, uint16_t *y)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(68);
    *y = (uint16_t)(rd16(b | 0x0D79) << 2);
    cyc(66);
    return cyc_then(72, OamSetPalette(bank, rd8(cyc_then(180, 0x04E26A + (rd16(b | 0x0D48) & 0x00FF))), 0x0012, y));
}

void abi_OamPaletteFromTableE26AByMode(void)
{
    cpu.a = OamPaletteFromTableE26AByMode(cpu.db, &cpu.y);
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

    cyc(68);
    *y = (uint16_t)(rd16(b | 0x0D75) << 2);
    cyc(66);
    OamSetPalette(bank, rd8(cyc_then(180, 0x04E30B + (rd16(b | 0x0D48) & 0x00FF))), 0x0012, y);
    cyc(46);
    *y = (uint16_t)((uint16_t)(rd16(b | 0x0D75) + 0x16) << 2);
    cyc(104);
    OamSetPalette(bank, rd8(cyc_then(180, 0x04E313 + (rd16(b | 0x0D89) & 0x00FF))), 0x0006, y);
    cyc(46);
    *y = (uint16_t)((uint16_t)(rd16(b | 0x0D75) + 0x1C) << 2);
    cyc(104);
    return cyc_then(72, OamSetPalette(bank, rd8(cyc_then(180, 0x04E313 + (rd16(b | 0x0D8A) & 0x00FF))), 0x0006, y));
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

    cyc(130);
    wr8(b | 0x0E14, 0xF8);
    cyc(38);
    for (i = 0; i < 4; i++) {
        cyc(24);
        v = (uint8_t)(rd8(b | (uint16_t)(0x057B + i)) | bits[i]);
        cyc(46);
        wr8(b | (uint16_t)(0x057B + i), v);
    }
    cyc(70);
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

    cyc(68);
    OamHighBits58(bank);
    cyc(70);
    wr16(b | 0x0591, 0x5555);
    cyc(24);
    wr16(b | 0x0593, 0x5555);
    cyc(70);
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

    cyc(160);
    OamHideRange(0x0090, 0x0015);
    cyc(110);
    OamHideRange(0x0180, 0x0010);
    cyc(32);
    for (i = 0; i < 4; i++)
        { cyc(24); wr8(b | (uint16_t)(0x058D + i), 0x00); }
    cyc(70);
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

    cyc(106);
    wr8(b | 0x0E0C, 0xF8);
    cyc(24);
    wr8(b | 0x0E14, 0xF8);
    cyc(24);
    if (rd8(b | 0x0D2B)) {
        cyc(110);
        OamHideRange(0x012C, 0x000C);
        cyc(132);
        OamHideRange(0x0180, 0x0020);
    } else {
        cyc(116);
        OamHideRange(0x012C, 0x0035);
    }
    cyc(86);
}

void abi_OamHide75Up(void)
{
    OamHide75Up(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xF0);
    cpu.x = cpu.p & P_X ? (cyc(0), 0x0000) : 0x0200;  /* the final pull restores X's width */
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
    cyc(44);
    static const uint16_t pos[3] = {0x4840, 0x5840, 0x6840};
    static const uint16_t tile[3] = {0x0660, 0x0680, 0x06A0};
    static const uint16_t first[3] = {0x00A0, 0x0180, 0x01A0};
    static const uint16_t high[6] = {0x057F, 0x0580, 0x058D, 0x058E, 0x058F, 0x0590};
    uint32_t b = (uint32_t)bank << 16;
    uint8_t p = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        uint16_t x = first[i];

        cyc(40);
        wr16(0x0000F1, pos[i]);
        cyc(40);
        wr16(0x0000F3, tile[i]);
        OamColumn8(bank, &x, &p);
    }
    cyc(22);
    for (i = 0; i < 6; i++)
        { cyc(40); wr8(b | high[i], 0xAA); }
    cyc(70);
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

    cyc(108);
    if (!rd8(b | 0x10CD)) {
        cyc(32);
        for (i = 0; i < 7; i++)
            { cyc(24); wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * at8c[i]), 0x8C); }
        cyc(16);
        for (i = 0; i < 5; i++)
            { cyc(24); wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * at94[i]), 0x94); }
    } else { cyc(22); }
    cyc(16);
    for (i = 75; i <= 86; i++)
        { cyc(24); wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * i), 0x93); }
    cyc(40);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 112), 0x70);
    cyc(24);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 113), 0x70);
    cyc(40);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 114), 0x80);
    cyc(24);
    wr8(b | (uint16_t)(OAM_BUFFER + 1 + 4 * 115), 0x80);
    for (i = 0; i < 6; i++)
        { cyc(i & 1 ? 24 : 40); wr8(b | high[i], 0x00); }   /* LDA # before each pair */
    cyc(70);
}

void abi_OamPlaceRows75And112(void)
{
    OamPlaceRows75And112(cpu.db);
    cpu.a &= 0xFF00;
}

/* A strip with a marker sprite: the strip at the position in direct page
   $02 (X at entry), then the marker at that position (read back from $02)
   plus the value, less
   the strip's span once the value reaches it, in the slot before the
   strip. The OAM writes for the marker go through the data bank. */
static uint16_t bar_marker(uint8_t bank, uint16_t v, uint16_t y, uint16_t span,
                           uint16_t tile_far, uint16_t tile_near, uint32_t in)
{
    uint32_t b = (uint32_t)bank << 16;
    cyc(in);                            /* from the strip's last store: each bar's own */
    uint16_t tile, pos = rd16(0x000002);

    if (v >= span) {
        cyc(68);
        wr16(b | (uint16_t)(0x0375 + y), (uint16_t)(v + pos - span));
        tile = tile_far;
    } else {
        cyc(30);
        wr16(b | (uint16_t)(0x0375 + y), (uint16_t)(v + pos));
        tile = tile_near;
    }
    cyc(54);
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

    /* PHP, SEP, REP (66), CMP # (16), BCC taken (22) or not and LDA #$3F
       (32), STA (16) */
    cyc(value < 0x3F ? 120 : 130);
    wr8(0x000004, (uint8_t)v);
    cyc(38);
    wr16(0x000002, pos);
    cyc(134);
    v = rd8(0x000004);
    cyc(122);
    SpriteStrip4(v, (uint16_t)(y + 4));
    return bar_marker(bank, v, y, 0x0010, 0x3C20, 0x3622, v >= 0x0010 ? 212 : 240);   /* BCC taken: 28 more */
}

void abi_SpriteBar4WithMarker(void)
{
    uint16_t v = (uint8_t)cpu.a < 0x3F ? (uint8_t)cpu.a : 0x3F;

    cpu.a = SpriteBar4WithMarker(cpu.db, (uint8_t)cpu.a, cpu.x, cpu.y);
    cpu.x = (uint16_t)(((v >> 1) & 0x00F8) + 8);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    } else { cyc(72); }
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
    uint16_t v;

    cyc(82);                            /* PHP, SEP, REP, STA */
    wr8(0x000004, value);
    cyc(38);
    wr16(0x000002, pos);
    cyc(134);
    v = rd8(0x000004);
    cyc(122);
    SpriteStrip6(v, (uint16_t)(y + 4));
    return bar_marker(bank, v, y, 0x0008, 0x3C1F, 0x3C1E, v >= 0x0008 ? 212 : 240);   /* BCC taken: 28 more */
}

void abi_SpriteBar6WithMarker(void)
{
    uint8_t v = (uint8_t)cpu.a;

    cpu.a = SpriteBar6WithMarker(cpu.db, v, cpu.x, cpu.y);
    cpu.x = (uint16_t)(((v & 0x0038) << 1) + 12);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    } else { cyc(72); }
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
    uint16_t x = (uint16_t)(group << 2), y = (uint16_t)(slot << 2), mask, hi, n16;
    uint8_t n, lo = 0, v, o;
    unsigned s;

    cyc(176);
    n16 = rd16(0x04EDCD + x);
    wr16(0x0000F1, n16);
    cyc(62);
    x = rd16(0x04EDCB + x);
    hi = (uint16_t)(x & 0xFF00);
    do {
        cyc(100);
        v = rd8(0x04EE4B + x);
        o = rd8(b | 0x0D3B);
        cyc(30);
        wr8(b | (uint16_t)(0x0375 + y), (uint8_t)(v + o));
        cyc(78);
        v = rd8(0x04EE4C + x);
        o = rd8(b | 0x0D3C);
        s = (unsigned)v + o;
        p = (uint8_t)(p & ~(P_C | P_V));
        if (s > 0xFF)
            p |= P_C;
        if (~(v ^ o) & (v ^ s) & 0x80)
            p |= P_V;
        cyc(30);
        wr8(b | (uint16_t)(0x0376 + y), (uint8_t)s);
        cyc(70);
        v = rd8(0x04EE4D + x);
        wr8(b | (uint16_t)(0x0377 + y), v);
        cyc(70);
        v = rd8(0x04EE4E + x);
        wr8(b | (uint16_t)(0x0378 + y), v);
        cyc(6);
        lo = rd8(0x04EE4F + x);
        if (!(lo & 0x01)) {
            cyc(366);
            mask = rd16(b | (uint16_t)(0xFA8D + ((y >> 2) & 0x0003)));
            /* the mask's two bytes are stated when they are ROM; RAM is placed */
            cyc(bank == 0x7E || bank == 0x7F || bank == 0x70 || bank == 0x71 ? 16 : 32);
            wr16(0x0000F3, mask);
            hi = (uint16_t)(mask & 0xFF00);
            cyc(54);
            v = rd8(0x000575 + (y >> 4));
            cyc(16);
            o = rd8(0x0000F3);
            lo = (uint8_t)(v & o);
            cyc(32);
            wr8(0x000575 + (y >> 4), lo);
            p = (uint8_t)((p & ~P_C) | ((y >> 3) & 1 ? P_C : 0));
        }
        x = (uint16_t)(x + 5);
        y = (uint16_t)(y + 4);
        cyc(214);
        n = rd8(0x0000F1);
        n = (uint8_t)(n - 1);
        cyc(6);
        wr8(0x0000F1, n);
    } while (n);
    cyc(80);
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

/* The shared body: set selector to $0D35, then from $02:F958 on; first is
   the time from the store of $0D35 to the first high-table store (each
   entry's own way in: 78 falling in, 102 with a JMP). */
static uint16_t flashing_metasprite_draw(uint8_t bank, uint16_t set, uint16_t *x, uint16_t *y, uint32_t first)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t i, ph;
    uint8_t n, a = 0, phase;

    cyc(88);
    wr16(b | 0x0D35, set);
    for (i = 0; i < 0x20; i++)
        { cyc(i ? 90 : first); wr16(b | (uint16_t)(0x0575 + i), 0xAAAA); }
    cyc(78);
    if (!rd16(b | 0x0D37)) {
        cyc(86);
        *x = 0x0020;
        return 0;
    }
    cyc(46);
    if (!(rd16(b | 0x0E3B) & 0x0003)) {
        cyc(64);
        ph = (uint16_t)(rd16(b | 0x0D39) + 1);
        cyc(84);
        wr16(b | 0x0D39, ph < 3 ? ph : (cyc(18), 0));
    } else { cyc(46); }
    cyc(48);
    i = (uint16_t)(rd16(b | 0x0D35) << 2);
    cyc(106);
    wr16(0x0000F1, rd16(0x1CE5AE + i));
    *x = rd16(0x1CE5AC + i);
    *y = 0;
    i = (uint16_t)(*x & 0xFF00);
    /* Time from the assembly (the pieces' ROM bytes 8 each, not placed):
       in, LDA long and TAX (62); round again, BNE (22); then SEP, LDA
       long, CLC, ADC abs (100) */
    cyc(162);
    do {
        uint8_t v = rd8(0x1CE62C + *x);

        v = (uint8_t)(v + rd8(b | 0x0D37));
        cyc(30);                                /* STA abs,Y */
        wr8(b | (uint16_t)(0x0375 + *y), v);
        cyc(78);                                /* LDA long, CLC, ADC abs */
        v = rd8(0x1CE62D + *x);
        v = (uint8_t)(v + rd8(b | 0x0D38));
        cyc(30);
        wr8(b | (uint16_t)(0x0376 + *y), v);
        cyc(70);                                /* LDA long, STA abs,Y */
        wr8(b | (uint16_t)(0x0377 + *y), rd8(0x1CE62E + *x));
        cyc(24);                                /* LDA abs */
        phase = rd8(b | 0x0D39);
        a = rd8(0x1CE62F + *x);
        /* CMP #, BCC (taken for phase 0, then LDA long, STA abs,Y: 108);
           else BEQ, LDA long, BIT #$08 and its ladder (bit 3: 76, bit 1:
           108, neither: 102) and STA abs,Y: 140 + it for phase 1, 134 + it
           for phase 2 */
        if (!phase)
            cyc(108);
        else
            cyc((phase == 1 ? 140 : 134) + ((a & 0x08) ? 76 : (a & 0x02) ? 108 : 102));
        wr8(b | (uint16_t)(0x0378 + *y), phase ? cycled_attr(a, phase) : a);
        a = rd8(0x1CE630 + *x);
        *x = (uint16_t)(*x + 5);
        *y = (uint16_t)(*y + 4);
        cyc(198);                               /* LDA long, BIT #, INX x5, INY x4, DEC's fetch */
        n = (uint8_t)(rd8(0x0000F1) - 1);
        cyc(6);
        wr8(0x0000F1, n);
        if (n)
            cyc(122); else { cyc(86); }                           /* BNE, SEP, LDA long, CLC, ADC abs */
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
    cyc(4);
    return flashing_metasprite_draw(bank, 0x0008, x, y, 78);
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
    cyc(4);
    return flashing_metasprite_draw(bank, 0x0000, x, y, 102);
}

void abi_FlashingMetaspriteDraw0(void)
{
    cpu.a = FlashingMetaspriteDraw0(cpu.db, &cpu.x, &cpu.y);
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
    cyc(0);
}

/*
 * @routine 02:F932 FlashingMetaspriteDrawByMode
 * WHAT IT DOES: FlashingMetaspriteDraw with metasprite $14 of the bank $1C
 * set when $0D4C's low byte is set, else metasprite $0E.
 * HOW: $0D35 = $14 or $0E, then a jump into FlashingMetaspriteDraw after
 * its own store.
 * Entry: as FlashingMetaspriteDraw. Exit: as FlashingMetaspriteDraw.
 */
void abi_FlashingMetaspriteDrawByMode(void)
{
    cyc(68);
    uint16_t set = (rd16(((uint32_t)cpu.db << 16) | 0x0D4C) & 0x00FF) ? 0x0014 : (cyc(6), 0x000E);

    cpu.a = flashing_metasprite_draw(cpu.db, set, &cpu.x, &cpu.y, 102);
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
    cyc(22);
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
    cyc(22);
    abi_oam_attr_fill(4, 0x38);
}

/*
 * @routine 02:FBAA OamAttr36x7
 * WHAT IT DOES: Gives 7 sprites from entry A attribute byte $36.
 * HOW: As OamAttr36x4. Not reached in the traces so far.
 * Entry: as OamAttr36x4. Exit: as OamAttr36x4.
 */
void abi_OamAttr36x7(void)
{
    cyc(22);
    abi_oam_attr_fill(7, 0x36);
}

/*
 * @routine 07:C4E0 OamHighBits18Park48
 * WHAT IT DOES: OamHighBits18, then sets the X-high bit of sprites 48-63
 * (high table bytes 12-15 = $55), which puts them off the left edge.
 * HOW: Two word stores of $5555 to $0581 and $0583 through the data bank.
 * Entry: any widths. Exit: A = $5555; X = 0; Y high byte clear; P
 * unchanged.
 */
void OamHighBits18Park48(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(68);
    OamHighBits18(bank);
    cyc(70);
    wr16(b | 0x0581, 0x5555);
    cyc(24);
    wr16(b | 0x0583, 0x5555);
    cyc(70);
}

void abi_OamHighBits18Park48(void)
{
    OamHighBits18Park48(cpu.db);
    cpu.a = 0x5555;
    cpu.x = 0x0000;
    cpu.y &= 0x00FF;
}

/*
 * @routine 07:B985 SpritePieces3
 * WHAT IT DOES: Writes three sprites of a piece set to the sprite buffer
 * from entry offset Y: the set is the word at $1C:F09E + ((the byte at
 * $07:B981 + $10BB's low byte) + $10C3) * 4 (low byte); each piece (five
 * bytes from $1C:F11E + the set) adds its X, Y and tile to $10BD, $10BF and
 * $10C1, and gives its attribute, with the palette bits (1-3) from $10C5
 * when that is not 0.
 * HOW: Direct page $F1 counts the pieces down. $10C5 is read again for the
 * palette when it is set.
 * Entry: M 16-bit, X 16-bit, D = $0000; Y = sprite buffer offset. Exit:
 * A = the set's high byte and the last attribute; X = the set + 15; Y
 * past the third sprite; M 8-bit; N clear, Z set ($F1 counted to 0); C, V from the last
 * tile add.
 */
uint8_t SpritePieces3(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p, uint16_t *set)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t i;
    uint8_t attr = 0, n;
    unsigned t;

    cyc(24);
    i = rd8(cyc_then(148, 0x07B981 + (rd16(b | 0x10BB) & 0x00FF)));
    i = (uint16_t)(i + rd16(b | 0x10C3));
    cyc(182);
    *x = *set = rd16(0x1CF09E + ((uint16_t)(i << 2) & 0x00FF));
    wr8(0x0000F1, 0x03);
    do {
        cyc(24);
        wr8(b | (uint16_t)(0x0375 + *y), (uint8_t)(rd8(b | 0x10BD) + rd8(cyc_then(84, 0x1CF11E + *x))));
        cyc(24);
        wr8(b | (uint16_t)(0x0376 + *y), (uint8_t)(rd8(b | 0x10BF) + rd8(cyc_then(84, 0x1CF11F + *x))));
        {
            cyc(24);
            uint8_t base = rd8(b | 0x10C1), add = rd8(cyc_then(84, 0x1CF120 + *x));

            t = (unsigned)base + add;
            *p = (uint8_t)((*p & ~(P_C | P_V)) | (t > 0xFF ? P_C : 0)
                           | ((~(base ^ add) & (base ^ t) & 0x80) ? P_V : 0));
            wr8(b | (uint16_t)(0x0377 + *y), (uint8_t)t);
        }
        cyc(24);
        if (rd8(b | 0x10C5))
            { cyc(102); attr = (uint8_t)((rd8(0x1CF121 + *x) & 0xF1) | rd8(b | 0x10C5)); }
        else
            { cyc(78); attr = rd8(0x1CF121 + *x); }
        cyc(30);
        wr8(b | (uint16_t)(0x0378 + *y), attr);
        *x = (uint16_t)(*x + 5);
        *y = (uint16_t)(*y + 4);
        cyc(142);
        n = (uint8_t)(rd8(0x0000F1) - 1);
        cyc(6);
        wr8(0x0000F1, n);
    } while (cyc_loop(22, n));
    cyc(60);
    *p = (uint8_t)(nz8(*p, 0) | P_M);
    return attr;
}

void abi_SpritePieces3(void)
{
    uint8_t p = cpu.p, attr;
    uint16_t set;

    attr = SpritePieces3(cpu.db, &cpu.x, &cpu.y, &p, &set);
    cpu.a = (uint16_t)((set & 0xFF00) | attr);
    cpu.p = p;
}

/* Piece set A at screen X = x, Y = $7E, then SpritePieces3; the flags
   left by the set-up subtracts are SpritePieces3's to overwrite. */
static void pieces_at(uint16_t x)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(24);
    wr16(b | 0x10C3, cpu.a);
    cyc(86);
    wr16(b | 0x10BD, x);
    cyc(x == 0x0081 - 0x0024 ? 108 : 86);   /* the left one's BRA to SpritePiecesAtY */
    wr16(b | 0x10BF, 0x0089 - 0x000B);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z | P_V)) | P_C);
    (cyc(62), abi_SpritePieces3());
    cpu.p &= (uint8_t)~P_M;
    cyc(64);
}

/*
 * @routine 02:FD3E SpritePiecesAtLeft
 * WHAT IT DOES: Writes the three sprites of piece set A ($10C3) at screen
 * X = $5D, Y = $7E from sprite buffer offset Y.
 * HOW: Stores the set and position ($81 - $24, $89 - $0B) and calls
 * SpritePieces3.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the set, Y = sprite buffer
 * offset. Exit: as SpritePieces3, M 16-bit.
 */
void abi_SpritePiecesAtLeft(void)
{
    pieces_at(0x0081 - 0x0024);
}

/*
 * @routine 02:FD4D SpritePiecesAtRight
 * WHAT IT DOES: As SpritePiecesAtLeft at screen X = $A5.
 * HOW: As SpritePiecesAtLeft ($81 + $24).
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAtRight(void)
{
    pieces_at(0x0081 + 0x0024);
}

/* The position (x, y) of the piece set in $10C3 (Y stored first when
   `y_first`; 2 with the BRA to the shared tail), then SpritePieces3 from sprite buffer offset Y and REP #$20.
   The set-up subtracts leave C set and N, Z, V clear for SpritePieces3 to
   overwrite. */
static void pieces_xy(uint16_t x, uint16_t y, int y_first)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    if (y_first) {
        cyc(86);
        wr16(b | 0x10BF, y);
        cyc(y_first == 2 ? 108 : 86);   /* 2: the BRA to the shared tail */
        wr16(b | 0x10BD, x);
    } else {
        cyc(86);
        wr16(b | 0x10BD, x);
        cyc(86);
        wr16(b | 0x10BF, y);
    }
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z | P_V)) | P_C);
    (cyc(62), abi_SpritePieces3());
    cpu.p &= (uint8_t)~P_M;
    cyc(64);
}

/* Piece set A to $10C3, then pieces_xy. */
static void pieces_at_xy(uint16_t x, uint16_t y, int y_first)
{
    cyc(24);
    wr16(((uint32_t)cpu.db << 16) | 0x10C3, cpu.a);
    pieces_xy(x, y, y_first);
}

/*
 * @routine 02:FC9A SpritePiecesAt46Y71
 * WHAT IT DOES: Writes the three sprites of piece set A at screen X = $46,
 * Y = $71 from sprite buffer offset Y: the car of the records screen's
 * first row.
 * HOW: Stores the set, Y ($89 - $18) and X ($81 - $3B), then
 * SpritePieces3. Not reached in the traces so far.
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAt46Y71(void)
{
    pieces_at_xy(0x0081 - 0x003B, 0x0089 - 0x0018, 2);
}

/*
 * @routine 02:FCA9 SpritePiecesAt46Y82
 * WHAT IT DOES: As SpritePiecesAt46Y71 at Y = $82 (the second row).
 * HOW: As SpritePiecesAt46Y71 ($89 - $07). Not reached in the traces so
 * far.
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAt46Y82(void)
{
    pieces_at_xy(0x0081 - 0x003B, 0x0089 - 0x0007, 2);
}

/*
 * @routine 02:FCB8 SpritePiecesAt46Y93
 * WHAT IT DOES: As SpritePiecesAt46Y71 at Y = $93 (the third row).
 * HOW: As SpritePiecesAt46Y71 ($89 + $0A). Not reached in the traces so
 * far.
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAt46Y93(void)
{
    pieces_at_xy(0x0081 - 0x003B, 0x0089 + 0x000A, 1);
}

/*
 * @routine 02:FCD6 SpritePiecesAt62Y71
 * WHAT IT DOES: As SpritePiecesAt46Y71 at X = $62.
 * HOW: Stores the set, Y ($89 - $18) and X ($89 - $27), then
 * SpritePieces3. Not reached in the traces so far.
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAt62Y71(void)
{
    pieces_at_xy(0x0089 - 0x0027, 0x0089 - 0x0018, 2);
}

/*
 * @routine 02:FCE5 SpritePiecesAt62Y83
 * WHAT IT DOES: As SpritePiecesAt62Y71 at Y = $83.
 * HOW: As SpritePiecesAt62Y71 ($89 - $06). Not reached in the traces so
 * far.
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAt62Y83(void)
{
    pieces_at_xy(0x0089 - 0x0027, 0x0089 - 0x0006, 2);
}

/*
 * @routine 02:FCF4 SpritePiecesAt62Y95
 * WHAT IT DOES: As SpritePiecesAt62Y71 at Y = $95.
 * HOW: As SpritePiecesAt62Y71 ($89 + $0C). Not reached in the traces so
 * far.
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAt62Y95(void)
{
    pieces_at_xy(0x0089 - 0x0027, 0x0089 + 0x000C, 1);
}

/*
 * @routine 02:FD12 SpritePiecesCarAt51
 * WHAT IT DOES: Writes car A's three sprites (piece set A * 3, low four
 * bits) with palette 0 at screen X = $51, Y = $87 from sprite buffer
 * offset Y: the car of a records screen's best time.
 * HOW: The set worked out in $10C3 itself (store, double, add it back,
 * mask), $10C5 cleared, then X ($81 - $30), Y ($89 - $02) and
 * SpritePieces3. Not reached in the traces so far.
 * Entry: as SpritePiecesAtLeft; A = the car. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesCarAt51(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v;

    cyc(24);
    wr16(b | 0x10C3, cpu.a);
    v = (uint16_t)(cpu.a << 1);
    cyc(52);
    v = (uint16_t)((v + rd16(b | 0x10C3)) & 0x000F);
    cyc(48);
    wr16(b | 0x10C3, v);
    cyc(24);
    wr16(b | 0x10C5, 0x0000);
    pieces_xy(0x0081 - 0x0030, 0x0089 - 0x0002, 0);
}

/*
 * @routine 02:FD6B SpritePiecesAt59Y87
 * WHAT IT DOES: Writes the three sprites of piece set A at screen X = $59,
 * Y = $87 from sprite buffer offset Y.
 * HOW: As SpritePiecesCarAt51 without the set's working out. No caller
 * found: not reached in the traces so far.
 * Entry: as SpritePiecesAtLeft. Exit: as SpritePiecesAtLeft.
 */
void abi_SpritePiecesAt59Y87(void)
{
    pieces_at_xy(0x0089 - 0x0030, 0x0089 - 0x0002, 0);
}

/* SpritePieces3 into sprite offset `y`, keeping its A, X, Y and flags. */
static void car_pieces(uint16_t y)
{
    cyc(108);
    uint8_t p, attr;
    uint16_t set;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.y = y;
    p = cpu.p;
    attr = SpritePieces3(cpu.db, &cpu.x, &cpu.y, &p, &set);
    cpu.a = (uint16_t)((set & 0xFF00) | attr);
    cpu.p = p;
    cyc(46);
}

/*
 * @routine 07:C1C3 SpritePiecesTwoCarsTop
 * WHAT IT DOES: As SpritePiecesTwoCars higher on the screen: the selector
 * $10BB steps every eighth frame; Y $38; player 1's car at X $34 (sprite
 * offset $C0), player 2's at X $7C ($CC); then the bytes $20, $58 at
 * $0581-$0582.
 * HOW: As SpritePiecesTwoCars, entered with SEP #$20 only; the selector
 * steps when the frame count's low three bits are 7. Reached only by
 * unit_pause.
 * Entry: D = $0000. Exit: as SpritePiecesTwoCars.
 */
void abi_SpritePiecesTwoCarsTop(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v, n;

    cpu.p |= P_M;
    cyc(46);
    if ((rd8(b | 0x0E3B) & 0x07) == 0x07) {
        cyc(72);
        v = (uint8_t)(rd8(b | 0x10BB) + 1);
        if (v >= 0x04)
            v = 0x00;
        cyc(86);
        wr8(b | 0x10BB, v);
    } else { cyc(54); }
    cyc(40);
    wr8(b | 0x10C1, 0x30);
    cyc(40);
    wr8(b | 0x10C5, 0x30);
    cyc(40);
    wr8(b | 0x10BF, 0x38);
    cyc(40);
    wr8(b | 0x10BD, 0x34);
    cyc(24);
    n = rd8(b | 0x0D89);
    cyc(52);
    n = (uint8_t)((n << 1) + rd8(b | 0x0D89));
    cyc(24);
    wr8(b | 0x10C3, n);
    car_pieces(0x00C0);
    cpu.p |= P_M | P_X;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    n = rd8(b | 0x0D8A);
    cyc(52);
    n = (uint8_t)((n << 1) + rd8(b | 0x0D8A));
    cyc(24);
    wr8(b | 0x10C3, n);
    cyc(40);
    wr8(b | 0x10BD, 0x7C);
    car_pieces(0x00CC);
    cpu.p |= P_M;
    cyc(16);
    wr8(b | 0x0581, 0x20);
    cyc(40);
    wr8(b | 0x0582, 0x58);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x58);
    cpu.p = nz8(cpu.p, 0x58);
    cyc(110);                           /* RTS, the caller's BRA, the rows' SEP and JSR */
}

/*
 * @routine 07:C22B SpritePiecesTwoCars
 * WHAT IT DOES: Shows the two players' cars as three-piece sprites: every
 * fourth frame steps the piece selector $10BB through 0-3; with tile and
 * palette bases $30 and Y $A0, player 1's car ($0D89 * 3) at X $84 into
 * sprite offset $C0 and player 2's ($0D8A * 3) at X $CC into $CC
 * (SpritePieces3); then the bytes $20, $58 at $0581-$0582 (the second
 * sprite table's size bits).
 * HOW: 8-bit stores through the data bank; the selector steps when the
 * frame count's low two bits are 3.
 * Entry: D = $0000. Exit: A = $58 (high byte as SpritePieces3 left it);
 * X, Y as the second SpritePieces3 left them, Y truncated to 8 bits
 * between the two; M 8-bit, X 16-bit; N, Z clear.
 */
void abi_SpritePiecesTwoCars(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v, n;

    cpu.p |= P_M | P_X;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cyc(46);
    if ((rd8(b | 0x0E3B) & 0x03) == 0x03) {
        cyc(72);
        v = (uint8_t)(rd8(b | 0x10BB) + 1);
        if (v >= 0x04)
            { cyc(10); v = 0x00; }
        cyc(76);
        wr8(b | 0x10BB, v);
    } else { cyc(54); }
    cyc(40);
    wr8(b | 0x10C1, 0x30);
    cyc(40);
    wr8(b | 0x10C5, 0x30);
    cyc(40);
    wr8(b | 0x10BF, 0xA0);
    cyc(40);
    wr8(b | 0x10BD, 0x84);
    cyc(24);
    n = rd8(b | 0x0D89);
    cyc(52);
    n = (uint8_t)((n << 1) + rd8(b | 0x0D89));
    cyc(24);
    wr8(b | 0x10C3, n);
    car_pieces(0x00C0);
    cpu.p |= P_M | P_X;
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    n = rd8(b | 0x0D8A);
    cyc(52);
    n = (uint8_t)((n << 1) + rd8(b | 0x0D8A));
    cyc(24);
    wr8(b | 0x10C3, n);
    cyc(40);
    wr8(b | 0x10BD, 0xCC);
    car_pieces(0x00CC);
    cpu.p |= P_M;
    cyc(16);
    wr8(b | 0x0581, 0x20);
    cyc(40);
    wr8(b | 0x0582, 0x58);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x58);
    cpu.p = nz8(cpu.p, 0x58);
    cyc(112);
}

/* OAM address addr (word), then n bytes: from the buffer at $0375 + x, or
   $F0 each when hide. Leaves X past them, Y = 0. */
static void oam_direct(uint32_t b, uint8_t addr, uint16_t x, uint16_t n, int hide)
{
    uint8_t v = addr;

    cyc(80);
    wr8(b | 0x2102, addr);
    cyc(24);
    wr8(b | 0x2103, 0x00);
    cpu.x = x;
    for (cpu.y = n; cpu.y; cpu.y--) {
        cyc(cpu.y != n ? (hide ? 50 : 40) : (hide ? 40 : 14));  /* round again: INX, DEY, BNE */
        v = hide ? 0xF0 : rd8(cyc_then(40, b | (uint16_t)(0x0375 + cpu.x)));
        cyc(24);
        wr8(b | 0x2104, v);
        cpu.x = (uint16_t)(cpu.x + 1);
    }
    cyc(72);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((cpu.p & ~(P_N | P_Z)) | P_Z);
}

/* Step $0D54 on by 2 (two INCs) when the byte at `at` equals v. */
static void stage_next_if(uint32_t b, uint16_t at, uint8_t v)
{
    cyc(70);
    uint8_t c = rd8(b | at);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | c);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(c - v)) & ~P_C) | (c >= v ? P_C : 0));
    if (c != v)
        { cyc(38); return; }            /* CMP, BNE taken */
    cyc(56);                            /* CMP, BNE not taken, INC's fetch */
    wr8(b | 0x0D54, cyc_then(6, (uint8_t)(rd8(b | 0x0D54) + 1)));
    cyc(24);
    wr8(b | 0x0D54, cyc_then(6, (uint8_t)(rd8(b | 0x0D54) + 1)));
}

/* Restart $0D54 when the byte at `at` equals v. */
static void stage_reset_if(uint32_t b, uint16_t at, uint8_t v)
{
    cyc(70);
    uint8_t c = rd8(b | at);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | c);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(c - v)) & ~P_C) | (c >= v ? P_C : 0));
    if (c == v)
        wr8(b | 0x0D54, 0x00);
}

/*
 * @routine 7E:AD26 OamDirectStage
 * WHAT IT DOES: Writes a band of sprites straight to OAM mid-frame. Away
 * from mode 1 ($0D2B) with $0E5D set it hides sprites 116-127 ($F0 in all
 * their bytes); otherwise stage $0D54 picks: 0 sprites 116-127; 2 the same,
 * moving on when $0E16 is $AC; 4 sprites 75-86, moving on when $0E12 is
 * $1F; 6 sprites 112-115, with $0D55 = $2A and back to 0 when $0E0B is
 * $2A; 8 sprites 36-46 (OamWriteSprites36To46), back to 0 when $0D57 is
 * $10; 10 the same and sprites 116-127, back to 0 when $0D57 is $D0.
 * HOW: JMP ($AD58,X) on the stage; OAMADDL then OAMDATA bytes from the
 * buffer at $0375 through the data bank.
 * Entry: any widths; DB a bank where work RAM and the PPU are mapped.
 * Exit: X past the last byte copied (its low byte when P restores 8-bit
 * index registers); Y = 0; A low byte = the last byte written; P restored.
 */
void abi_OamDirectStage(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;
    uint16_t w;

    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cyc(90);
    v = rd8(b | 0x0D2B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v != 0x01) {
        cyc(56);
        w = rd16(b | 0x0E5D);
        cpu.x = w;
        if (w) {
            oam_direct(b, 0xE8, 0x01D0, 0x0030, 1);
            op_plp(p0);
            return;
        }
    } else { cyc(16); }
    op_sep(P_X);
    cyc(46);
    cpu.x = rd8(b | 0x0D54);
    switch (cpu.x) {
    case 0x02:
        stage_next_if(b, 0x0E16, 0xAC);
        cyc(6);                         /* on into the stage 0 code, no JMP */
        cpu.p &= (uint8_t)~P_X;
        oam_direct(b, 0xE8, 0x01D0, 0x0030, 0);
        break;
    case 0x00:
        cyc(52);
        cpu.p &= (uint8_t)~P_X;
        oam_direct(b, 0xE8, 0x01D0, 0x0030, 0);
        break;
    case 0x04:
        stage_next_if(b, 0x0E12, 0x1F);
        cyc(6);
        cpu.p &= (uint8_t)~P_X;
        oam_direct(b, 0x96, 0x012C, 0x0030, 0);
        break;
    case 0x06:
        cyc(70);
        v = rd8(b | 0x0E0B);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        if (v == 0x2A) {
            cyc(56);
            wr8(b | 0x0D55, v);
            cyc(24);
            wr8(b | 0x0D54, 0x00);
        } else { cyc(38); }
        cyc(6);
        cpu.p &= (uint8_t)~P_X;
        oam_direct(b, 0xE0, 0x01C0, 0x0010, 0);
        break;
    case 0x08:
        stage_reset_if(b, 0x0D57, 0x10);
        abi_OamWriteSprites36To46();
        break;
    default:                                        /* 0x0A */
        stage_reset_if(b, 0x0D57, 0xD0);
        abi_OamWriteSprites36To46();
        cpu.p &= (uint8_t)~P_X;
        oam_direct(b, 0xE8, 0x01D0, 0x0030, 0);
        break;
    }
    op_plp(p0);
}

/*
 * @routine 07:AC08 OamFillEvery4
 * WHAT IT DOES: Stores the byte A at $0376 + X and every fourth byte after
 * it, Y times: one field of Y consecutive sprites in the OAM buffer.
 * HOW: STA, four INX, DEY under BNE; through the data bank.
 * Entry: M 8-bit; X = the first offset; Y = the count (not 0). Exit: X past
 * the last; Y = 0; Z set, N clear.
 */
void abi_OamFillEvery4(void)
{
    uint16_t mask = (cpu.p & P_X) ? 0x00FF : (cyc(0), 0xFFFF);

    do {
        cyc(30);
        wr8((uint32_t)cpu.db << 16 | (uint16_t)(0x0376 + cpu.x), (uint8_t)cpu.a);
        cpu.x = (uint16_t)((cpu.x + 4) & mask);
        cpu.y = (uint16_t)((cpu.y - 1) & mask);
    } while (cyc_loop(92, cpu.y));
    cyc(128);
    cpu.p = nz16(cpu.p, 0);
}

/* JMP ($BB3C,X) / ($BBA2,X) with X = $0D2F after SEP #$30: the handler
   from the table (jump_routine). */
static void oam_stage_dispatch(uint32_t table)
{
    op_sep(P_M | P_X);
    cyc(24);
    cpu.x = rd8((uint32_t)cpu.db << 16 | 0x0D2F);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    cyc(46);
    jump_routine((uint32_t)0x070000 | rd16(table + cpu.x));
}

/*
 * @routine 07:BB34 OamStageDispatchBB3C
 * WHAT IT DOES: Runs the handler for stage $0D2F from the table at
 * $07:BB3C (eight entries, $07:BC32 to $07:BFC9).
 * HOW: SEP #$30, then JMP (table,X); the handler returns to the caller,
 * or (the stage at $07:BEC6) through the caller's frame.
 * Entry: any widths. Exit: as the handler leaves it.
 */
void abi_OamStageDispatchBB3C(void)
{
    oam_stage_dispatch(0x07BB3C);
}

/*
 * @routine 07:BB9A OamStageDispatchBBA2
 * WHAT IT DOES: As OamStageDispatchBB3C, with the table at $07:BBA2.
 * HOW: SEP #$30, then JMP (table,X).
 * Entry: any widths. Exit: as the handler leaves it.
 */
void abi_OamStageDispatchBBA2(void)
{
    oam_stage_dispatch(0x07BBA2);
}

void abi_OamSetPalette(void);

/*
 * @routine 04:E227 OamPaletteForD75
 * WHAT IT DOES: Gives ten sprites from sprite ($0D75 + 13) * 4 palette
 * $16 (OamSetPalette with A = 10, X = $16, Y = the offset).
 * HOW: P saved and restored; 16-bit.
 * Entry: any widths. Exit: P unchanged; A, X, Y as OamSetPalette leaves
 * them.
 */
void abi_OamPaletteForD75(void)
{
    uint8_t p0 = cpu.p;

    op_rep(P_M | P_X);
    cyc(46);
    op_lda16((uint32_t)cpu.db << 16 | 0x0D75);
    op_add16(0x000D);
    op_asl16();
    op_asl16();
    op_tay16();
    op_lda16_imm(0x000A);
    cpu.x = 0x0016;
    cpu.p = nz16(cpu.p, cpu.x);
    (cyc(136), abi_OamSetPalette());
    op_plp(p0);
    cyc(30);
}

/*
 * @routine 07:D576 OamWordRow5
 * WHAT IT DOES: Puts the five-letter word from the table at $D5AD (in the
 * data bank, $07 when called) as
 * sprites 90-94 ($04E1): x from $A5 by 8, y $4E, tile $C0 + the letter,
 * attributes $38.
 * HOW: 8-bit; x and y in direct page $F3 and $F4; P saved and restored.
 * Entry: D = $0000. Exit: P restored; A = $38; X = $14; Y = 5.
 */
void abi_OamWordRow5(void)
{
    uint32_t b;
    uint8_t p0 = cpu.p, v;

    op_sep(P_M | P_X);
    b = (uint32_t)cpu.db << 16;
    cyc(54);
    wr8(0x0000F3, 0xA5);
    cyc(32);
    wr8(0x0000F4, 0x4E);
    cpu.x = 0;
    cpu.y = 0;
    do {
        cyc(48);
        v = rd8(0x0000F3);
        cyc(30);
        wr8(b | (uint16_t)(0x04E1 + cpu.x), v);
        cyc(46);
        wr8(0x0000F3, (uint8_t)(v + 8));
        cyc(16);
        wr8(b | (uint16_t)(0x04E2 + cpu.x), cyc_then(30, rd8(0x0000F4)));
        cyc(92);
        v = rd8(b | (uint16_t)(0xD5AD + cpu.y));
        wr8(b | (uint16_t)(0x04E3 + cpu.x), (uint8_t)(v + 0xC0));
        cyc(46);
        wr8(b | (uint16_t)(0x04E4 + cpu.x), 0x38);
        cpu.x = (uint16_t)(cpu.x + 4);
        cpu.y = (uint16_t)(cpu.y + 1);
    } while (cyc_loop(76, cpu.y < 5));
    cyc(130);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x38);
    op_plp(p0);
}

void abi_SoundPort1QueuePush(void);
void abi_OamHighBits18(void);

/* The pause's frames: each waits for $0D52 (set in the vertical blank),
   clears it, runs the stage's handler and clears both pads' presses,
   until $0D2D is 0. */
static void stage_loop(uint32_t b, void (*dispatch)(void))
{
    do {
        do {
            op_sep(P_M);
            cyc(46);
            op_lda8(b | 0x0D52);
        } while (cyc_loop(22, !(uint8_t)cpu.a));
        cyc(40);
        wr8(b | 0x0D52, 0x00);
        (cyc(46), dispatch());
        op_rep(P_M);
        cyc(24);
        wr16(b | 0x0D1A, 0x0000);
        cyc(24);
        wr16(b | 0x0D1C, 0x0000);
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x0D2D);
    } while (cyc_loop(22, (uint8_t)cpu.a));
    cyc(44);
}

/* PLP, RTS through the stack as it is: P, then the return address in the
   program bank $07; the caller's frame is left (cpu.unwound). */
static void plp_rts(void)
{
    uint16_t ret;

    op_plp(rd8((uint16_t)(cpu.s + 1)));
    cpu.s = (uint16_t)(cpu.s + 1);
    ret = rd16((uint16_t)(cpu.s + 1));
    cpu.s = (uint16_t)(cpu.s + 2);
    cpu.pc = 0x070000u | (uint16_t)(ret + 1);
    cpu.unwound = 1;
}

/*
 * @routine 07:BAE6 OamStagePause
 * WHAT IT DOES: When Select was pressed ($0D1A bit 12): $0D2E = 0, sound
 * $15 on port 1, the layout request $19BD = 1, the high bits of sprites 18
 * on (OamHighBits18), $0D2D down by one, $0E5B and $0E5F cleared and stage
 * $0D2F = 0; then, each frame (waiting for $0D52 from the vertical blank),
 * the stage's handler (OamStageDispatchBB3C) with the presses cleared,
 * until $0D2D is 0.
 * HOW: P saved and restored; 8-bit through the data bank, the presses
 * 16-bit.
 * Entry: any widths. Exit: P restored; A as the last step left it.
 */
void abi_OamStagePause(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    op_rep(P_M);
    cyc(46);
    op_lda16(b | 0x0D1A);
    if (!(cpu.a & 0x1000)) {
        cyc(68);
        cpu.p |= P_Z;
        op_plp(p0);
        return;
    }
    cpu.p &= (uint8_t)~P_Z;
    op_sep(P_M);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0);
    cyc(108);
    wr8(b | 0x0D2E, 0x00);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x15);
    cpu.p = nz8(cpu.p, 0x15);
    (cyc(78), abi_SoundPort1QueuePush());
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
    cyc(40);
    wr8(b | 0x19BD, 0x01);
    abi_OamHighBits18();
    cyc(24);
    v = (uint8_t)(rd8(b | 0x0D2D) - 1);
    cyc(6);
    wr8(b | 0x0D2D, v);
    cpu.p = nz8(cpu.p, v);
    cyc(24);
    wr8(b | 0x0E5B, 0x00);
    cyc(24);
    wr8(b | 0x0E5F, 0x00);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0);
    cyc(40);
    wr8(b | 0x0D2F, 0x00);
    stage_loop(b, abi_OamStageDispatchBB3C);
    op_plp(p0);
}

/*
 * @routine 07:BB16 OamStagePauseLoop
 * WHAT IT DOES: OamStagePause's frames from inside: each frame (waiting
 * for $0D52 from the vertical blank) the stage's handler
 * (OamStageDispatchBB3C) with both pads' presses cleared, until $0D2D is
 * 0; then the PLP and RTS of OamStagePause, taking P and the return
 * address from whatever the stack holds. PauseResults jumps here (its
 * stage $A); the stages after it leave by RaceRestart or RaceEndToAttract,
 * which reset the stack, so in play this end is not reached in the traces
 * so far (from PauseResults it would pull the low byte of RaceSoundsPlayer's
 * JSR as P).
 * HOW: As OamStagePause's loop; the PLP and RTS through the stack, so the
 * routine leaves through its caller's frame (cpu.unwound).
 * Entry: data bank $07; D = $0000. Exit: P pulled; S 3 higher; pc where
 * the RTS lands.
 */
void abi_OamStagePauseLoop(void)
{
    stage_loop((uint32_t)cpu.db << 16, abi_OamStageDispatchBB3C);
    plp_rts();
}

void abi_GsuCF42To3000(void);
void abi_GsuCF42ToC000(void);
void abi_GsuRun(void);

/*
 * @routine 07:C593 PictureTurnStep
 * WHAT IT DOES: Turns the picture one step at a pace set by the buttons.
 * After $0D32 reads 0, a nonzero delay $0D31 is counted down and nothing
 * else happens. Otherwise the delay is set from the buttons $0D1E (X 3,
 * Y 2, L 6, R 8, none 4), the picture number $0D33 moves back with Select
 * held ($0309 bit $2000; 0 wraps to 7) or on, and the entry of the table
 * at $C648 (three bytes each: source word, bank byte) is drawn: by
 * GsuCF42To3000 and Super FX program $07:EE59 with $0D7F nonzero, or
 * GsuCF42ToC000 and $07:EE50. $0D32 is decremented at the end.
 * HOW: P saved and restored. A table word below $8000 is a picture number
 * to go to instead (entry 8 sends the count back to 0). The R pace runs
 * only from unit_cpu.
 * Entry: M 8-bit, X 8-bit, D = $0000, data bank $07. Exit: P restored; A
 * as the last call leaves it, X = the program address, Y = $0D7F.
 */
void abi_PictureTurnStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;
    uint16_t m, k, t, y;
    int first, buf3000;

    op_sep(P_M);
    do
        { cyc(68); op_lda8(b | 0x0D32); }
    while ((uint8_t)cpu.a);
    cyc(40);
    op_lda8(b | 0x0D31);
    if ((uint8_t)cpu.a) {
        cyc(40);
        wr8(b | 0x0D31, cyc_then(6, (uint8_t)(rd8(b | 0x0D31) - 1)));
        op_plp(p0);
        cyc(28);
        return;
    }
    op_rep(P_M);
    cyc(46);
    op_lda16(b | 0x0D1E);
    m = cpu.a;
    k = (m & 0x0040) ? 3 : (cyc(46), (m & 0x4000) ? 2 : (cyc(46), (m & 0x0020) ? 6
        : (cyc(30), (m & 0x0010) ? (cyc(16), 8) : 4)));
    cpu.a = k;
    op_sep(P_M);
    cyc(132);
    wr8(b | 0x0D31, (uint8_t)k);
    op_rep(P_M | P_X);
    cyc(24);
    op_lda16(b | 0x0309);
    if (cpu.a & 0x2000) {
        cyc(64);
        op_lda16(b | 0x0D33);
        t = (uint16_t)(cpu.a - 1);
        if (t & 0x8000)
            { cyc(40); t = 0x0007; }
    cyc(22);
        } else {
        cyc(70);
        op_lda16(b | 0x0D33);
        t = (uint16_t)(cpu.a + 1);
    }
    for (first = 1;; first = 0) {
        cyc(first ? 38 : 46);           /* going round: BRA, then the STA again */
        wr16(b | 0x0D33, t);
        cyc(52);
        y = (uint16_t)((uint16_t)(t << 1) + rd16(b | 0x0D33));
        cpu.y = y;
        cyc(170);
        cpu.x = (uint16_t)(rd16(b | (uint16_t)(0xC64A + y)) & 0x00FF);
        cpu.a = rd16(b | (uint16_t)(0xC648 + y));
        if (cpu.a >= 0x8000)
            break;
        t = cpu.a;
        cyc(38);
        wr16(b | 0x0D33, t);            /* STA, BRA back to the STA at .lC5F3 */
    }
    cpu.p = (uint8_t)(nz16(cpu.p, (uint16_t)(cpu.a - 0x8000)) | P_C);
    cyc(44);
    cpu.y = rd16(b | 0x0D7F);
    cpu.p = nz16(cpu.p, cpu.y);
    buf3000 = cpu.y != 0;
    if (cpu.y) {
        cyc(78);
        abi_GsuCF42To3000();
        op_sep(P_M);
        op_rep(P_X);
        cpu.x = 0xEE59;
        cpu.p = nz16(cpu.p, 0xEE59);
    } else {
        cyc(84);
        abi_GsuCF42ToC000();
        op_sep(P_M);
        op_rep(P_X);
        cpu.x = 0xEE50;
        cpu.p = nz16(cpu.p, 0xEE50);
    }
    cyc(240);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x07);
    cpu.p = nz8(cpu.p, 0x07);
    abi_GsuRun();
    op_rep(P_M | P_X);
    op_sep(P_M);
    cyc(buf3000 ? 90 : 68);             /* REP, REP, SEP, DEC's opcode; the $3000 arm's BRA */
    wr8(b | 0x0D32, cyc_then(6, (uint8_t)(rd8(b | 0x0D32) - 1)));
    op_plp(p0);
    cyc(28);
}

/* LDA #$E0 (8-bit), LDX, LDY, then OamFillEvery4: hides sprites. */
static void oam_hide(uint16_t x, uint16_t y)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xE0);
    cpu.p = nz8(cpu.p, 0xE0);
    cpu.x = x;
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
    abi_OamFillEvery4();
    cyc(94);
}

/*
 * @routine 07:ABBB OamHideForHudMode
 * WHAT IT DOES: Hides sprites by putting them at y = $E0: the two whose y
 * is at $0E0C and $0E14, sprites 36 to 56, and by the HUD mode $0D2B: with
 * mode 0 sprites 75 to 127 (twice), with mode 2 no more, otherwise 75 to
 * 87 and 96 to 127.
 * HOW: OamFillEvery4 on the OAM buffer's y bytes; P saved and restored.
 * The modes other than 0 run only from unit_cpu.
 * Entry: D = $0000. Exit: P restored; A low byte $E0 (or 2 with mode 2);
 * X and Y as OamFillEvery4 leaves them.
 */
void abi_OamHideForHudMode(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    op_sep(P_M);
    op_rep(P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xE0);
    cpu.p = nz8(cpu.p, 0xE0);
    cyc(84);
    wr8(b | 0x0E0C, 0xE0);
    cyc(24);
    wr8(b | 0x0E14, 0xE0);
    cpu.x = 0x0090;
    cpu.y = 0x0015;
    cpu.p = nz16(cpu.p, cpu.y);
    (cyc(94), abi_OamFillEvery4());
    cyc(24);
    op_lda8(b | 0x0D2B);
    v = (uint8_t)cpu.a;
    if (!v) {
        cyc(132);
        oam_hide(0x012C, 0x0035);
        cpu.x = 0x012C;
        cpu.y = 0x0035;
        cpu.p = nz16(cpu.p, cpu.y);
        abi_OamFillEvery4();
    } else if (v != 0x02) {
        cyc(v < 0x02 ? 164 : 174);      /* BEQ, CMP, BCC, BEQ, LDA, LDX, LDY, JSR */
        oam_hide(0x012C, 0x000D);
        cpu.x = 0x0180;
        cpu.y = 0x0020;
        cpu.p = nz16(cpu.p, cpu.y);
        abi_OamFillEvery4();
    } else { cyc(70); }
    cyc(28);
    op_plp(p0);
}

/*
 * @routine 07:BB4C OamStagePause2
 * WHAT IT DOES: As OamStagePause for pad 2: when its Select was pressed
 * ($0D1C bit 12): $0D2E = $FF, sound $15 on port 1, the layout request
 * $19BD = 1, the high bits of sprites 58 on (OamHighBits58), $0D2D down by
 * one, $0E5B and $0E5F cleared and stage $0D2F = 0; then, each frame, the
 * stage's handler from the table at $07:BBA2 (OamStageDispatchBBA2) with
 * the presses cleared, until $0D2D is 0.
 * HOW: As OamStagePause. The loop is also entered by a jump from $07:EB42;
 * the press runs only from unit_cpu.
 * Entry: any widths. Exit: P restored; A as the last step left it.
 */
void abi_OamStagePause2(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;

    op_rep(P_M);
    cyc(46);
    op_lda16(b | 0x0D1C);
    if (!(cpu.a & 0x1000)) {
        cyc(68);
        cpu.p |= P_Z;
        op_plp(p0);
        return;
    }
    cpu.p &= (uint8_t)~P_Z;
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xFF);
    cpu.p = nz8(cpu.p, 0xFF);
    cyc(108);
    wr8(b | 0x0D2E, 0xFF);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x15);
    cpu.p = nz8(cpu.p, 0x15);
    (cyc(78), abi_SoundPort1QueuePush());
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.p = nz8(cpu.p, 0x01);
    cyc(40);
    wr8(b | 0x19BD, 0x01);
    abi_OamHighBits58();
    cyc(24);
    v = (uint8_t)(rd8(b | 0x0D2D) - 1);
    cyc(6);
    wr8(b | 0x0D2D, v);
    cpu.p = nz8(cpu.p, v);
    cyc(24);
    wr8(b | 0x0E5B, 0x00);
    cyc(24);
    wr8(b | 0x0E5F, 0x00);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0);
    cyc(40);
    wr8(b | 0x0D2F, 0x00);
    stage_loop(b, abi_OamStageDispatchBBA2);
    op_plp(p0);
}

/*
 * @routine 07:BB7C OamStagePause2Loop
 * WHAT IT DOES: OamStagePause2's frames from inside, as
 * OamStagePauseLoop with the stage table at $07:BBA2
 * (OamStageDispatchBBA2): PauseResultsP2 jumps here.
 * HOW: As OamStagePauseLoop.
 * Entry: as OamStagePauseLoop. Exit: as OamStagePauseLoop.
 */
void abi_OamStagePause2Loop(void)
{
    stage_loop((uint32_t)cpu.db << 16, abi_OamStageDispatchBBA2);
    plp_rts();
}

/* OverlaySpritesStep's state 0: the overlay at (154, 106), plain, state
   1. */
static void overlay_state0(void)
{
    const uint32_t b = 0x040000;

    op_rep(P_M);
    cyc(48);
    wr16(b | 0x10A4, 0x009A);
    cyc(48);
    wr16(b | 0x10A6, 0x006A);
    cyc(24);
    wr16(b | 0x10A8, 0x0000);
    cyc(48);
    wr16(b | 0x10A2, 0x0001);
}

/* OverlaySpritesStep after its state's step: the bob and tile step by the
   frame count, the palette by the picture at $10A0, the twelve sprites
   placed (or off screen), their high bits $AA. */
static void overlay_draw(unsigned lead)
{
    const uint32_t b = 0x040000;
    uint8_t v, bob, step;
    uint16_t a, pic;
    uint32_t in, off;

    op_sep(P_M);
    cyc(lead);                          /* 22 more after a BRA to it */
    if (rd8(b | 0x0E3B) & 0x04) {
        bob = 0x00;
        step = 0x00;
    } else {
        cyc(6);
        bob = 0x01;
        step = 0x08;
    }
    cyc(64);
    wr8(0x0000F5, bob);
    cyc(bob ? 32 : 38);                 /* to STA $F1: LDA #8, or the BRA */
    wr8(0x0000F1, step);
    op_rep(P_M | P_X);
    cyc(24);
    pic = rd16(b | 0x10A0);
    cyc(140);
    a = pic == rd16(0x04E066) ? 0x0000 : pic == rd16(cyc_then(86, 0x04E068)) ? 0x0002
        : pic == rd16(cyc_then(58, 0x04E06A)) ? (cyc(6), 0x0004) : 0x0008;
    op_sep(P_M);
    v = rd8(b | 0x10A8);
    cyc(16);
    wr8(0x0000F3, (uint8_t)(a | v));
    cyc(24);
    cpu.x = rd8(b | 0x10A8) ? (cyc(16), 0x0024) : 0x0000;
    cpu.y = 0x0090;
    in = 180;
    do {
        op_rep(P_M);
        cyc(in);
        a = (uint16_t)(rd16(0x04E1B6 + (uint32_t)cpu.x) & 0x00FF);
        a = (uint16_t)(a + rd16(b | 0x10A4));
        cpu.a = a;
        op_sep(P_M);
        if ((a & 0x8000) || (cyc(34), a >= 0x00F0)) {
            cyc(90);
            wr8(b | (uint16_t)(0x0376 + cpu.y), 0xF0);
            off = 22;                   /* BRA to .lE19C */
        } else {
            off = 0;
            cyc(80);
            wr8(b | (uint16_t)(0x0375 + cpu.y), (uint8_t)a);
            cyc(78);
            v = rd8(0x04E1B7 + (uint32_t)cpu.x);
            v = (uint8_t)(v + rd8(b | 0x10A6));
            cyc(30);
            v = (uint8_t)(v - rd8(0x0000F5));
            cyc(30);
            wr8(b | (uint16_t)(0x0376 + cpu.y), v);
            cyc(70);
            v = rd8(0x04E1B8 + (uint32_t)cpu.x);
            v = (uint8_t)(v + rd8(0x0000F1));
            cyc(30);
            wr8(b | (uint16_t)(0x0377 + cpu.y), v);
            cyc(32);
            wr8(b | (uint16_t)(0x0378 + cpu.y), cyc_then(30, (uint8_t)(0x10 | rd8(0x0000F3))));
        }
        cpu.x = (uint16_t)(cpu.x + 3);
        cpu.y = (uint16_t)(cpu.y + 4);
        /* INX x3, INY x4, CPY # (122), BCC; round again 22 and REP, LDA
           long, AND #, CLC, ADC abs (132 with the table word's ROM read;
           the REP's 22 is op_rep's) */
        in = off + 122 + 22 + 132 - 22;
    } while (cpu.y < 0x00C0);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xAA);
    cyc(off + 122 + 16 + 40);           /* BCC not taken, LDA #, STA abs */
    wr8(b | 0x057E, 0xAA);
    cyc(24);
    wr8(b | 0x057F, 0xAA);
    cyc(24);
    wr8(b | 0x0580, 0xAA);
}

/*
 * @routine 04:E048 OverlaySpritesStep
 * WHAT IT DOES: Moves and draws a 12-sprite overlay (sprites 36 to 47) by
 * its state $10A2: 0 sets it up at (154, 106), plain, and goes to 1; 2
 * slides it left 9 a frame until x is below -80, then 1; 3 puts it at x =
 * -512 with $10A8 = $40 and goes to 4; 4 slides it right 2 a frame to x =
 * 32, then 7; 5 puts it at x = 256, plain, and goes to 6; 6 slides it left
 * 3 a frame to x = 154, then 7; 1 and 7 hold it. Then each sprite is
 * placed from the table at $04:E1B6 (x, y, tile; the second set of 12 at
 * $E1DA when $10A8 is set): off screen (y = $F0) when its x is negative or
 * past 239; its y bobbing up 1 and its tile 8 on while bit 2 of the frame
 * count is clear; attributes $10 with palette 0, 1 or 2 for the pictures
 * $7F:63D7, $6FD7 and $7BD7 at $10A0 (4 otherwise), ORed with $10A8. The
 * twelve are large ($AA in their high bits).
 * HOW: JMP (table,X) on the state; the bob in $F5, the tile step in $F1,
 * the attributes in $F3. P and the data bank saved and restored. States 5
 * and 6 run only from unit_cpu.
 * Entry: D = $0000. Exit: P and the data bank restored; A low byte $AA;
 * X and Y past the last sprite.
 */
void abi_OverlaySpritesStep(void)
{
    const uint32_t b = 0x040000;
    uint8_t p0 = cpu.p, db0 = cpu.db, s;
    uint16_t a;
    unsigned lead = 68;                 /* 46 when the draw is entered by falling into it */

    op_sep(P_M | P_X);
    cpu.db = 0x04;
    cyc(118);
    s = rd8(b | 0x10A2);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)(s << 1));
    cpu.x = (uint8_t)(s << 1);
    cyc(52);
    switch (rd16(0x040000 | (uint16_t)(0xE056 + cpu.x))) {
    case 0xE06E:
        cyc(22);
        overlay_state0();
        cyc(2);
        break;
    case 0xE088:
        op_rep(P_M);
        cyc(46);
        a = (uint16_t)(rd16(b | 0x10A4) - 9);
        cyc(62);
        wr16(b | 0x10A4, a);
        if (!(a & 0x8000) || (cyc(40), a >= 0xFFB0))
            break;
        cyc(64);
        wr16(b | 0x10A2, 0x0001);
        break;
    case 0xE0A3:
        op_rep(P_M);
        cyc(70);
        wr16(b | 0x10A4, 0xFE00);
        cyc(48);
        wr16(b | 0x10A8, 0x0040);
        cyc(24);
        wr16(b | 0x10A2, cyc_then(14, (uint16_t)(rd16(b | 0x10A2) + 1)));
        break;
    case 0xE0B6:
        op_rep(P_M);
        cyc(46);
        a = (uint16_t)(rd16(b | 0x10A4) + 2);
        cyc(62);
        wr16(b | 0x10A4, a);
        if ((a & 0x8000) || (cyc(40), a < 0x0020))
            break;
        cyc(64);
        wr16(b | 0x10A4, 0x0020);
        cyc(48);
        wr16(b | 0x10A2, 0x0007);
        break;
    case 0xE0D7:
        op_rep(P_M);
        cyc(70);
        wr16(b | 0x10A4, 0x0100);
        cyc(24);
        wr16(b | 0x10A8, 0x0000);
        cyc(24);
        wr16(b | 0x10A2, cyc_then(14, (uint16_t)(rd16(b | 0x10A2) + 1)));
        break;
    case 0xE0E7:
        op_rep(P_M);
        cyc(46);
        a = (uint16_t)(rd16(b | 0x10A4) - 3);
        cyc(62);
        wr16(b | 0x10A4, a);
        if (a >= 0x009A)
            { cyc(24); break; }
        cyc(88);
        wr16(b | 0x10A4, 0x009A);
        cyc(48);
        wr16(b | 0x10A2, 0x0007);
        lead = 46;
        break;
    }
    overlay_draw(lead);
    cyc(58);                            /* PLB, PLP, RTL less op_plp's */
    cpu.db = db0;
    op_plp(p0);
}

/*
 * @routine 04:E06E OverlaySpritesState0
 * WHAT IT DOES: OverlaySpritesStep's state 0 entered from its jump table:
 * the overlay set up at (154, 106), plain ($10A8 = 0), state $10A2 = 1;
 * then the sprites drawn as OverlaySpritesStep, and the data bank and P
 * that OverlaySpritesStep pushed pulled.
 * HOW: Word stores, then OverlaySpritesStep's drawing; PLB, PLP, RTL.
 * Entry: M and X 8-bit; data bank $04; D = $0000; OverlaySpritesStep's P
 * and data bank on the stack. Exit: as OverlaySpritesStep.
 */
void abi_OverlaySpritesState0(void)
{
    overlay_state0();
    (cyc(2), overlay_draw(68));
    cyc(20);
    cpu.db = rd8((uint16_t)(cpu.s + 1));
    cpu.s = (uint16_t)(cpu.s + 1);
    cpu.p = nz8(cpu.p, cpu.db);
    cyc(20);
    op_plp(rd8((uint16_t)(cpu.s + 1)));
    cpu.s = (uint16_t)(cpu.s + 1);
    cyc(2);
}

/* Waits for the vertical blank: $0D52, set by each, read until it is set,
   then cleared. `again`: the clocks the caller's copy of the loop adds each
   time round (07:BBFB's runs its SEP #$20 each pass: 22; 07:C685's none). */
static void oam_wait_tick(uint32_t b, uint32_t again)
{
    op_sep(P_M);
    do
        { cyc(24); op_lda8(b | 0x0D52); }
    while (cyc_loop(22, cyc_loop(again, !(uint8_t)cpu.a)));
    cyc(40);
    wr8(b | 0x0D52, 0x00);
}

/*
 * @routine 07:C683 OverlayFramesUntilFaded
 * WHAT IT DOES: Runs frames until the fade's high byte $0D61 is negative:
 * each waits for the vertical blank ($0D52, then cleared), runs 07:C6AA and
 * the overlay sprites (OverlaySpritesStep), and clears both players' presses
 * ($0D1A, $0D1C) and held buttons ($0309, $030D).
 * HOW: 8-bit registers, 16-bit A for the clears; through the data bank.
 * Entry: any widths. Exit: M 8-bit, X 8-bit; A low byte = $0D61 (negative);
 * N set; X, Y as the last call left them.
 */
void abi_OverlayFramesUntilFaded(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    cyc(0);
    do {
        op_sep(P_M | P_X);
        oam_wait_tick(b, 0);
        cyc(46);
        call_routine(0x07C6AA, 0);
        (cyc(62), abi_OverlaySpritesStep());
        op_rep(P_M);
        cyc(24);
        wr16(b | 0x0D1A, 0x0000);
        cyc(24);
        wr16(b | 0x0D1C, 0x0000);
        cyc(24);
        wr16(b | 0x0309, 0x0000);
        cyc(24);
        wr16(b | 0x030D, 0x0000);
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x0D61);
    } while (cyc_loop(22, !(cpu.a & 0x80)));
    cyc(58);
}

/* DEC abs (8-bit). */
static void oam_dec8(uint32_t at)
{
    cyc(24);
    uint8_t v = (uint8_t)(rd8(at) - 1);

    cyc(6);
    wr8(at, v);
    cpu.p = nz8(cpu.p, v);
}

/* LDA # (8-bit). */
static void oam_lda8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 07:BBB2 PauseOnStart
 * WHAT IT DOES: The race's pause: with START pressed ($0D1A bit 12) it clears
 * $0D2E, plays sound $15 on port 1, asks for layout 1 ($19BD), hides sprites
 * 75 up and lays out the large columns (OamHide75Up, OamLargeColumns),
 * counts $0D2D and $0D22 down, clears $0E5B, $0E5F, $0D32-$0D34, $0D31 and
 * $0D2F, sets $0D23 = $0A; then runs frames (each after the vertical blank,
 * $0D52) of 07:BC1C and PictureTurnStep, clearing both players' presses,
 * until $0D2D is 0 again.
 * HOW: 8-bit A for the flags, 16-bit for the presses; through the data bank;
 * P saved and put back.
 * Entry: any widths. Exit: P unchanged; A = the presses (no START), or A low
 * byte 0 ($0D2D).
 */
void abi_PauseOnStart(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;

    op_sep(P_M);
    op_rep(P_M);
    cyc(68);
    op_lda16(b | 0x0D1A);
    cpu.p = (uint8_t)((cpu.p & ~P_Z) | ((cpu.a & 0x1000) ? 0 : (cyc(68), P_Z)));
    if (!(cpu.a & 0x1000)) {
        op_plp(p0);
        return;
    }
    op_sep(P_M);
    oam_lda8(0x00);
    cyc(108);
    wr8(b | 0x0D2E, 0x00);
    cyc(78);
    oam_lda8(0x15);
    abi_SoundPort1QueuePush();
    oam_lda8(0x01);
    cyc(40);
    wr8(b | 0x19BD, 0x01);
    (cyc(46), abi_OamHide75Up());
    (cyc(46), abi_OamLargeColumns());
    oam_dec8(b | 0x0D2D);
    cyc(24);
    wr8(b | 0x0E5B, 0x00);
    cyc(24);
    wr8(b | 0x0E5F, 0x00);
    cyc(24);
    wr8(b | 0x0D32, 0x00);
    cyc(24);
    wr8(b | 0x0D33, 0x00);
    cyc(24);
    wr8(b | 0x0D34, 0x00);
    cyc(24);
    wr8(b | 0x0D31, 0x00);
    oam_dec8(b | 0x0D22);
    oam_lda8(0x00);
    cyc(40);
    wr8(b | 0x0D2F, 0x00);
    oam_lda8(0x0A);
    cyc(40);
    wr8(b | 0x0D23, 0x0A);
    do {
        cyc(22);
        oam_wait_tick(b, 22);
        cyc(46);
        call_routine(0x07BC1C, 0);
        (cyc(46), abi_PictureTurnStep());
        op_rep(P_M);
        cyc(24);
        wr16(b | 0x0D1A, 0x0000);
        cyc(24);
        wr16(b | 0x0D1C, 0x0000);
        op_sep(P_M);
        cyc(46);
        op_lda8(b | 0x0D2D);
    } while (cyc_loop(22, (uint8_t)cpu.a));
    cyc(44);
    op_plp(p0);
}

/* PHP, SEP #$20, REP #$10, OamHideRange from `offset` for 9 sprites,
   PLP: A low byte = $F0, X past the run (its low byte with X 8-bit after
   the pull), Y = 0, P unchanged. */
static void hide9(uint16_t offset)
{
    cyc(160);
    OamHideRange(offset, 0x0009);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0xF0);
    cpu.x = (uint16_t)(offset + 4 * 9);
    if (cpu.p & P_X)
        cpu.x &= 0x00FF;
    cyc(86);
    cpu.y = 0x0000;
}

/*
 * @routine 07:C484 OamHide55To63
 * WHAT IT DOES: Hides sprites 55-63. Nothing in the game calls it.
 * HOW: OamHideRange from offset $DC for 9 sprites; P saved and restored.
 * Entry: any widths. Exit: A low byte = $F0; X = $0100 ($00 when X is
 * 8-bit); Y = 0; P unchanged.
 */
void abi_OamHide55To63(void)
{
    hide9(0x00DC);
}

/*
 * @routine 07:C494 OamHide119To127
 * WHAT IT DOES: Hides sprites 119-127. Nothing in the game calls it.
 * HOW: OamHideRange from offset $1DC for 9 sprites; P saved and restored.
 * Entry: any widths. Exit: A low byte = $F0; X = $0200 ($00 when X is
 * 8-bit); Y = 0; P unchanged.
 */
void abi_OamHide119To127(void)
{
    hide9(0x01DC);
}
