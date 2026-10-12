/*
 * scroll.c: BG scroll registers set by the raster interrupt.
 */
#include "machine.h"
#include "hw.h"

/* BG2 scroll pairs in work RAM, indexed by the screen band: horizontal at
   $0D92, vertical at $0D8E (low, high). */
#define BG2_SCROLL_X 0x000D92u
#define BG2_SCROLL_Y 0x000D8Eu

/*
 * @routine 7E:B28C Bg2ScrollFromTable
 * WHAT IT DOES: Sets BG2's scroll from the pair for a screen band.
 * HOW: Writes the horizontal pair at $0D92+X to BG2HOFS and the vertical pair
 * at $0D8E+X to BG2VOFS, low byte first.
 * Entry: M 8-bit, DB a bank where these are work RAM; X = band offset.
 * Exit: A low byte = the vertical high byte; N, Z from it.
 */
uint8_t Bg2ScrollFromTable(uint16_t band)
{
    uint8_t v;

    wr8(BG2HOFS, rd8(BG2_SCROLL_X + band));
    wr8(BG2HOFS, rd8(BG2_SCROLL_X + 1 + band));
    wr8(BG2VOFS, rd8(BG2_SCROLL_Y + band));
    v = rd8(BG2_SCROLL_Y + 1 + band);
    wr8(BG2VOFS, v);
    return v;
}

void abi_Bg2ScrollFromTable(void)
{
    uint8_t v = Bg2ScrollFromTable((cpu.p & P_X) ? (uint8_t)cpu.x : cpu.x);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/*
 * @routine 7E:B2A5 ScrollCopyToBank7F
 * WHAT IT DOES: Copies two 16-bit scroll values ($0D96, $0D98) to their
 * places in bank $7F ($7F:348D, $7F:3493).
 * HOW: Two 16-bit copies.
 * Entry: any widths, DB a bank where $0D96 is work RAM. Exit: A = the second
 * value; N, Z from it; M 16-bit.
 */
uint16_t ScrollCopyToBank7F(void)
{
    uint16_t v = rd16(0x000D96);

    wr16(0x7F348D, v);
    v = rd16(0x000D98);
    wr16(0x7F3493, v);
    return v;
}

void abi_ScrollCopyToBank7F(void)
{
    cpu.a = ScrollCopyToBank7F();
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
}
