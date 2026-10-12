/*
 * slide.c: a 16-bit value at $19F6 that slides toward a target while bit 7
 * of $0DA5 is set; its high byte is copied to $7F:3468.
 */
#include "machine.h"

/*
 * @routine 7E:E0E8 SlideStep
 * WHAT IT DOES: Moves the sliding value one step toward its target and
 * publishes its high byte; ends the slide when the target is reached going
 * down.
 * HOW: Direction by the sign of $19FC: adds or subtracts the step $19F8.
 * Reaching the target exactly: going up, nothing is stored; going down, the
 * slide flag is cleared, $7F:3470 = $FFFF and the high byte published.
 * Otherwise the new value is stored and its high byte published.
 * Entry: any widths. Exit: A depends on the path (see the abi wrapper);
 * P unchanged.
 */
static uint16_t slide_step(uint16_t a)
{
    uint16_t v;

    if (!(rd8(0x000DA5) & 0x80))
        return (uint16_t)(a & 0xFF00);
    if (!(rd16(0x0019FC) & 0x8000))
        v = (uint16_t)(rd16(0x0019F6) + rd16(0x0019F8));
    else
        v = (uint16_t)(rd16(0x0019F6) - rd16(0x0019F8));
    if (v == rd16(0x0019FA)) {
        uint16_t dir = rd16(0x0019FC);
        if (!(dir & 0x8000))
            return dir;
        wr8(0x000DA5, rd8(0x000DA5) & 0x7F);
        wr16(0x7F3470, 0xFFFF);
        a = 0xFFFF;
    } else {
        wr16(0x0019F6, v);
        a = v;
    }
    {
        uint8_t hi = rd8(0x0019F7);
        wr8(0x7F3468, hi);
        return (uint16_t)((a & 0xFF00) | hi);
    }
}

void SlideStep(void)
{
    slide_step(0);
}

void abi_SlideStep(void)
{
    cpu.a = slide_step(cpu.a);
}
