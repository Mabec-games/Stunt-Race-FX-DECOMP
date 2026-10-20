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
    cyc(68);
    uint8_t t = (uint8_t)((rd8(0x0011DC) + 1) & 0x01), v, target;

    cyc(54);
    wr8(0x0011DC, t);
    if (t)
        { cyc(22); return t; }
    cyc(40);
    v = rd8(0x0011DA);
    cyc(24);
    target = rd8(0x0011DB);
    if (v == target)
        { cyc(22); return v; }
    cyc(62);
    v = rd8(0x0011DA);
    v = (uint8_t)(v < target ? v + 1 : v - 1);
    cyc(38);
    wr8(0x0011DA, v);
    return v;
}

void abi_HalfRateApproach(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | HalfRateApproach());
    cyc(70);
}

/*
 * @routine 0B:E2C5 ApproachTargetFull
 * WHAT IT DOES: Sets the target at $11DB to $0F.
 * HOW: One byte store through the data bank.
 * Entry: any widths. Exit: A low byte = $0F; P unchanged.
 */
void ApproachTargetFull(uint8_t bank)
{
    cyc(84);
    wr8((uint32_t)bank << 16 | 0x11DB, 0x0F);
}

void abi_ApproachTargetFull(void)
{
    ApproachTargetFull(cpu.db);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x0F);
}

/*
 * @routine 0B:E2CF ApproachTargetZero
 * WHAT IT DOES: Sets the target at $11DB to 0.
 * HOW: One byte store through the data bank.
 * Entry: any widths. Exit: every register unchanged.
 */
void ApproachTargetZero(uint8_t bank)
{
    cyc(68);
    wr8((uint32_t)bank << 16 | 0x11DB, 0x00);
    cyc(70);
}

void abi_ApproachTargetZero(void)
{
    ApproachTargetZero(cpu.db);
}
