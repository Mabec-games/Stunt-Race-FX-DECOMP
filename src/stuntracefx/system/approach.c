/*
 * approach.c: a value at $11DA that moves one step toward its target at
 * $11DB every other call.
 */
#include "machine.h"

/*
 * @routine 7F:0FDC HalfRateApproach
 * WHAT IT DOES: Every other call, moves $11DA one step toward $11DB.
 * HOW: Toggles bit 0 of $11DC; on the calls where it becomes 0, compares and
 * steps $11DA up or down by one, or leaves it when equal.
 * Entry: any widths. Exit: A low byte = the toggle (1), or $11DA's value;
 * P unchanged.
 */
uint8_t HalfRateApproach(void)
{
    uint8_t t = (uint8_t)((rd8(0x0011DC) + 1) & 0x01), v, target;

    wr8(0x0011DC, t);
    if (t)
        return t;
    v = rd8(0x0011DA);
    target = rd8(0x0011DB);
    if (v == target)
        return v;
    v = (uint8_t)(v < target ? v + 1 : v - 1);
    wr8(0x0011DA, v);
    return v;
}

void abi_HalfRateApproach(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | HalfRateApproach());
}
