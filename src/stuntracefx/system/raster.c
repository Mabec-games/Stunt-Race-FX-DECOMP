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
