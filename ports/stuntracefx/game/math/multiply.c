/*
 * multiply.c: the PPU's signed multiplier (Mode 7 registers) as a fast
 * 16 x 8 multiply for the S-CPU's 3D maths.
 *
 * M7A takes a 16-bit factor (two writes, low then high); writing M7B with an
 * 8-bit factor multiplies; MPYL/M/H hold the 24-bit product.
 */
#include "machine.h"
#include "hw.h"

/*
 * @routine 7E:E6BA PpuMultiplierSetLowByteTimes8
 * WHAT IT DOES: Loads the multiplier's 16-bit factor with the low byte of A
 * times 8.
 * HOW: Moves the low byte to the high byte and shifts right 5, then writes
 * the result to M7A low then high.
 * Entry: M 16-bit. Exit: A = the factor with its bytes swapped; N, Z from its
 * high byte; C clear; M 16-bit.
 */
uint16_t PpuMultiplierSetLowByteTimes8(uint8_t v)
{
    uint16_t factor = (uint16_t)(v << 3);

    cyc(168);
    wr8(M7A, (uint8_t)factor);
    cyc(52);
    wr8(M7A, (uint8_t)(factor >> 8));
    return factor;
}

void abi_PpuMultiplierSetLowByteTimes8(void)
{
    cyc(0);
    uint16_t factor = PpuMultiplierSetLowByteTimes8((uint8_t)cpu.a);

    cpu.a = (uint16_t)(factor << 8 | factor >> 8);
    cpu.p = (uint8_t)(nz8(cpu.p, (uint8_t)(factor >> 8)) & ~(P_C | P_M));
    cyc(64);
}

/*
 * @routine 7E:E7A1 PpuMultiply
 * WHAT IT DOES: Multiplies the multiplier's 16-bit factor by the low byte of
 * A (both signed) and returns the product's upper 16 bits.
 * HOW: Writes the low byte to M7B, then reads MPYM and MPYH.
 * Entry: M 16-bit. Exit: A = product >> 8; N, Z from it; M 16-bit.
 */
uint16_t PpuMultiply(uint8_t v)
{
    cyc(54);
    wr8(M7B, v);
    cyc(54);
    return rd16(MPYM);
}

void abi_PpuMultiply(void)
{
    cpu.a = PpuMultiply((uint8_t)cpu.a);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
    cyc(42);
}
