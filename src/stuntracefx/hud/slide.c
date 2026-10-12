/*
 * slide.c: HUD panels that slide on and off the screen.
 */
#include "machine.h"

/* The sliding panel's offset; $10 is fully shown. */
#define HUD_SLIDE 0x000D57u

/*
 * @routine 7E:AECE HudSlideIn
 * WHAT IT DOES: Moves the sliding HUD panel 4 pixels further in, stopping at
 * $10; a negative offset just moves.
 * HOW: Adds 4; when the result is not negative and is $10 or more, uses $10.
 * Entry: M 8-bit. Exit: A low byte = the new offset; flags from the add,
 * the compare or the load.
 */
static uint8_t hud_slide_in(uint8_t *p)
{
    uint8_t v = rd8(HUD_SLIDE);
    unsigned sum = v + 4;
    uint8_t r = (uint8_t)sum;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (sum > 0xFF ? P_C : 0)
                   | ((~(v ^ 4) & (v ^ r) & 0x80) ? P_V : 0));
    *p = nz8(*p, r);
    if (!(r & 0x80)) {
        *p = (uint8_t)(nz8(*p, (uint8_t)(r - 0x10)) & ~P_C) | (r >= 0x10 ? P_C : 0);
        if (r >= 0x10) {
            r = 0x10;
            *p = nz8(*p, r);
        }
    }
    wr8(HUD_SLIDE, r);
    return r;
}

void HudSlideIn(void)
{
    uint8_t p = 0;

    hud_slide_in(&p);
}

void abi_HudSlideIn(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | hud_slide_in(&p));
    cpu.p = p;
}
