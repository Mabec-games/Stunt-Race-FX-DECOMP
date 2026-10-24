/*
 * scroll.c: BG scroll registers set by the raster interrupt.
 */
#include "machine.h"
#include "native.h"
#include "hw.h"
#include "video_calls.h"
#include "carried.h"
#include "wram_names.h"


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

/* The same with 8-bit index registers (X's low byte, no page-crossing
   cycle from a 16-bit index). */
uint8_t Bg2ScrollFromTableX8(uint8_t band)
{
    return bg2_scroll(band, 0);
}

void Bg2ScrollFromTable_cr(Carried *cr);
void ScrollHdmaSet_cr(Carried *cr);

void Bg2ScrollFromTable_cr(Carried *cr)
{
    int wide = !(cr->p & P_X);
    uint8_t v = wide ? Bg2ScrollFromTable(cr->x) : bg2_scroll((uint8_t)cr->x, 0);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
    cyc(42);
}

void abi_Bg2ScrollFromTable(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    Bg2ScrollFromTable_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    uint16_t v = rd16(W(scroll_value));

    cyc(32);
    wr16(W(wram_10AE), v);
    cyc(32);
    wr16(W(wram_10B7), v);
    cyc(56);
    wr16(W(wram_10B1), 0x0130);
    cyc(32);
    wr16(W(wram_10B4), 0x0130);
}

void ScrollHdmaSet_cr(Carried *cr)
{
    ScrollHdmaSet();
    cr->a = 0x0130;
    cyc(72);                            /* its return */
}

void abi_ScrollHdmaSet(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ScrollHdmaSet_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

