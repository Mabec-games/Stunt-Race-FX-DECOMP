/*
 * raster.c: waiting on the PPU's beam inside the raster interrupt.
 */
#include "machine.h"
#include "hw.h"

/*
 * @routine 7E:A2FD WaitHBlank
 * WHAT IT DOES: Waits until the PPU is in horizontal blank, so the raster
 * interrupt can change PPU registers between two lines.
 * HOW: Reads HVBJOY until its H-blank bit (bit 6) is set.
 * Entry: M 8-bit, X 8-bit; runs from work RAM. Exit: A = the last HVBJOY
 * read; N from it; Z clear.
 */
uint8_t WaitHBlank(void)
{
    uint8_t v;

    do
        v = rd8(HVBJOY);
    while (!(v & 0x40));
    return v;
}

void abi_WaitHBlank(void)
{
    uint8_t v = WaitHBlank();

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)(nz8(cpu.p, v) & ~P_Z);
}

/*
 * @routine 02:D7AB WaitScanline
 * WHAT IT DOES: Waits until the low byte of the PPU's vertical counter
 * equals the byte at $06E3; clears $06E4 first.
 * HOW: Latches the counters (SLHV) and reads OPVCT twice (low, high) each
 * time round; the compare is 8-bit.
 * Entry: X 16-bit. Exit: A, X, P unchanged.
 */
void WaitScanline(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t lo;

    wr8(b | 0x06E4, 0x00);
    do {
        rd8(SLHV);
        lo = rd8(OPVCT);
        rd8(OPVCT);
    } while (lo != rd8(b | 0x06E3));
}

void abi_WaitScanline(void)
{
    WaitScanline(cpu.db);
}
