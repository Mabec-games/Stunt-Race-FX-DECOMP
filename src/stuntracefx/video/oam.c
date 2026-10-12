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
