/*
 * scroll.c: BG scroll registers set by the raster interrupt.
 */
#include "machine.h"
#include "native.h"
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
/* LDA abs,X: 24, and 6 more with a 16-bit index or when an 8-bit one
   crosses a page. */
static uint32_t abs_x(uint16_t base, uint16_t x, int wide)
{
    return 24 + (wide || (base & 0xFF) + (x & 0xFF) > 0xFF ? 6 : 0);
}

static uint8_t bg2_scroll(uint16_t band, int wide)
{
    uint8_t v;

    cyc(abs_x(BG2_SCROLL_X, band, wide));
    ppu_wr(BG2HOFS, cyc_then(24, rd8(BG2_SCROLL_X + band)));
    cyc(abs_x(BG2_SCROLL_X + 1, band, wide));
    ppu_wr(BG2HOFS, cyc_then(24, rd8(BG2_SCROLL_X + 1 + band)));
    cyc(abs_x(BG2_SCROLL_Y, band, wide));
    ppu_wr(BG2VOFS, cyc_then(24, rd8(BG2_SCROLL_Y + band)));
    cyc(abs_x(BG2_SCROLL_Y + 1, band, wide));
    v = rd8(BG2_SCROLL_Y + 1 + band);
    cyc(24);
    ppu_wr(BG2VOFS, v);
    return v;
}

uint8_t Bg2ScrollFromTable(uint16_t band)
{
    return bg2_scroll(band, 1);
}

void abi_Bg2ScrollFromTable(void)
{
    int wide = !(cpu.p & P_X);
    uint8_t v = bg2_scroll(wide ? cpu.x : (uint8_t)cpu.x, wide);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(42);
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
    cyc(76);
    uint16_t v = rd16(0x000E37);

    cyc(32);
    wr16(0x0010AE, v);
    cyc(32);
    wr16(0x0010B7, v);
    cyc(56);
    wr16(0x0010B1, 0x0130);
    cyc(32);
    wr16(0x0010B4, 0x0130);
}

void abi_ScrollHdmaSet(void)
{
    ScrollHdmaSet();
    cpu.a = 0x0130;
    cyc(72);                            /* its return */
}
