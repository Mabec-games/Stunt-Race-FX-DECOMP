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
 * @routine 03:FED9 ScrollHdmaSet
 * WHAT IT DOES: Fills the scroll HDMA table at $10AA: the scroll value from
 * $0E37 into its first and fourth entries' data and $0130 into the second
 * and third.
 * HOW: Word stores to $10AE, $10B7, $10B1, $10B4, in that order.
 * Entry: any widths. Exit: A = $0130 (16-bit); P unchanged.
 */
void ScrollHdmaSet(void)
{
    uint16_t v = rd16(0x000E37);

    wr16(0x0010AE, v);
    wr16(0x0010B7, v);
    wr16(0x0010B1, 0x0130);
    wr16(0x0010B4, 0x0130);
}

void abi_ScrollHdmaSet(void)
{
    ScrollHdmaSet();
    cpu.a = 0x0130;
}
