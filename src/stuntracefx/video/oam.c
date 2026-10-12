/*
 * oam.c: the work-RAM copy of the sprite table (OAM), copied to the PPU
 * each frame. Each sprite is four bytes: X, Y, tile, attributes.
 */
#include "machine.h"
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
