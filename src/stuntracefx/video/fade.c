/*
 * fade.c: the screen fade, stepped once a frame by the raster interrupt.
 *
 * The level is a 16-bit accumulator whose high byte is the brightness the
 * frame code writes to INIDISP; $0F00 is full brightness and $8000 (bit 15)
 * is forced blank. While $0E3A is zero the screen fades in, otherwise out.
 */
#include "machine.h"
#include "wram.h"

/* Per-brightness fade-in steps, in the frame code's RAM copy at $7E:AC9D. */
#define FADE_IN_STEPS 0x7EAC9Du

/* Flags left by CMP #$0F00 on a sum, or by LDA #$0F00 when it was clamped. */
static uint8_t clamp_flags(uint8_t p, uint16_t sum)
{
    if (sum >= 0x0F00)
        return (uint8_t)((p & ~(P_N | P_Z)) | P_C);
    return (uint8_t)(nz16(p, (uint16_t)(sum - 0x0F00)) & ~P_C);
}

static uint16_t fade_in(uint16_t step, uint8_t *p)
{
    uint16_t level = rd16(FADE_LEVEL);
    uint16_t sum = (uint16_t)(level + step);

    /* V comes from the add; CMP leaves it alone. */
    *p = (uint8_t)((*p & ~P_V) | ((~(level ^ step) & (level ^ sum) & 0x8000) ? P_V : 0));
    *p = clamp_flags(*p, sum);
    if (sum >= 0x0F00)
        sum = 0x0F00;
    cyc(88);
    wr16(FADE_LEVEL, sum);
    return sum;
}

/* One fade-out step: $58 down; a level that would drop below 0 becomes
   forced blank; a level already in forced blank stays. */
static uint16_t fade_out(uint8_t *p)
{
    cyc(62);
    uint16_t level = rd16(FADE_LEVEL);

    if (level & 0x8000) {
        *p = nz16(*p, level);
    } else if (level >= 0x0058) {
        cyc(54);
        level = (uint16_t)(level - 0x0058);
        *p = (uint8_t)(nz16(*p, level) | P_C);
        *p = (uint8_t)(*p & ~P_V);
    } else {
        cyc(94);
        *p = (uint8_t)((*p & ~(P_C | P_V | P_Z)) | P_N);
        level = 0x8000;
    }
    cyc(46);
    wr16(FADE_LEVEL, level);
    return level;
}

/*
 * @routine 7E:AC4D FadeStepFixed
 * WHAT IT DOES: Steps the fade by one frame: in by a fixed $98, or out.
 * HOW: Fading in, adds $98 and stops at $0F00; fading out, as FadeStep.
 * Entry: M 8-bit. Exit: A = the new level; M 8-bit; flags from the last
 * compare or load.
 */
uint16_t FadeStepFixed(void)
{
    uint8_t p = 0;

    return rd8(FADE_OUT) == 0 ? fade_in(0x0098, &p) : fade_out(&p);
}

void abi_FadeStepFixed(void)
{
    uint8_t p = cpu.p;

    cyc(140);
    cpu.a = rd8(FADE_OUT) == 0 ? (cyc(62), fade_in(0x0098, &p)) : (cyc(6), fade_out(&p));
    cpu.p = (uint8_t)(p | P_M);
}

/*
 * @routine 7E:AC69 FadeStep
 * WHAT IT DOES: Steps the fade by one frame, fading in faster or slower
 * depending on the current brightness.
 * HOW: Fading in, adds the step for the current brightness (the high byte of
 * the level indexes a table of 16-bit steps) and stops at $0F00. Fading out,
 * subtracts $58, going to forced blank instead of below zero.
 * Entry: M 8-bit, X 8-bit. Exit: A = the new level; X = brightness * 2 when
 * fading in; M 8-bit; flags from the last compare, subtract or load.
 */
uint16_t FadeStep(void)
{
    uint8_t p = 0;

    if (rd8(FADE_OUT) != 0)
        return fade_out(&p);
    return fade_in(rd16(FADE_IN_STEPS + (uint8_t)(rd8(FADE_LEVEL + 1) << 1)), &p);
}

void abi_FadeStep(void)
{
    uint8_t p = cpu.p;

    cyc(24);
    if (rd8(FADE_OUT) != 0) {
        cpu.a = fade_out(&p);
    } else {
        cyc(46);
        uint8_t x = (uint8_t)(rd8(FADE_LEVEL + 1) << 1);
        cyc(136);
        cpu.a = fade_in(rd16(FADE_IN_STEPS + x), &p);
        cpu.x = x;
    }
    cyc(64);
    cpu.p = (uint8_t)(p | P_M);
}
