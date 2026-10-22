/*
 * video_calls.h: the S-CPU's video routines (game/video) as C functions,
 * for the C that calls them.
 */
#ifndef STUNTRACEFX_VIDEO_CALLS_H
#define STUNTRACEFX_VIDEO_CALLS_H

#include <stddef.h>
#include <stdint.h>

/* backdrop.c: the backdrop scroll of the current half; bank is DB, *x
   comes back as the half * 2, m16 says A is 16-bit at entry (a cycle). */
uint16_t BackdropScrollFromView(uint8_t bank, uint16_t *x, int m16);

/* colour_math.c: the fixed colour's intensity from the step. */
uint8_t FixedColourFromStep(void);

/* scroll.c: BG2's scroll from the pair for a band (X 16-bit); the
   vertical high byte. */
uint8_t Bg2ScrollFromTable(uint16_t band);

/* fade.c: one frame of the screen fade; the new level. */
uint16_t FadeStepFixed(void);
uint16_t FadeStep(void);

/* gradient.c: the colour gradient. The expansions return the end run
   index with *y past the last colour and *carry from the course compare;
   the fills return X past the last entry. */
uint16_t GradientExpandFirst(uint8_t bank, uint16_t *y, int *carry);
uint16_t GradientExpandSecond(uint8_t bank, uint16_t *y, int *carry);
uint16_t GradientBuildColours(uint16_t x, int *carry);
uint16_t GradientFillCgadd(uint16_t value, uint16_t x, uint16_t count);
uint16_t GradientFillCgaddB(uint16_t value, uint16_t x, uint16_t count);
uint16_t GradientPointersToBank7F(void);
uint16_t GradientSetup(uint8_t bank);

/* window_anim.c: one step of a window value in the bank; *x counts the
   stops, *p the flags as the routine leaves them; the stored byte. */
uint8_t WindowStep3437(uint8_t bank, uint8_t *x, uint8_t *p);
uint8_t WindowStep3435(uint8_t bank, uint8_t *x, uint8_t *p);
uint8_t WindowStep3439(uint8_t bank, uint8_t *x, uint8_t *p);
uint8_t WindowStep3433By2(uint8_t bank, uint8_t *x, uint8_t *p);
uint8_t WindowStep3433By3(uint8_t bank, uint8_t *x, uint8_t *p);
uint8_t WindowStep3437By3(uint8_t bank, uint8_t *x, uint8_t *p);
/* db0 = DB; *p = P with M and X set; *x = the stops; A's low byte. */
uint8_t WindowEdgesStep(uint8_t db0, uint8_t *p, uint8_t *x);

/* sprite_slots.c: the eight 8-byte sprite records; bank = DB. */
uint16_t SpriteSlotFirst(uint8_t bank, uint16_t *x, uint8_t *p);
uint16_t SpriteSlotFirstLong(uint8_t bank, uint16_t *x, int x8);
uint8_t SpriteSlot1Countdown(uint8_t bank, uint16_t *x);
uint8_t SpriteSlot1Start(uint8_t bank, uint16_t *x, int x8);
uint8_t SpriteSlot2Brighten(uint8_t bank);
uint8_t SpriteSlot2BrightenLong(uint8_t bank);
uint8_t SpriteSlot2Darken(uint8_t bank);
uint8_t SpriteSlot2DarkenLong(uint8_t bank);
uint8_t SpriteSlot2FadeOut(uint8_t bank);
uint8_t SpriteSlot4Brighten(uint8_t bank);
uint8_t SpriteSlot4Darken(uint8_t bank);
uint8_t SpriteSlot4Full(uint8_t bank);
uint8_t SpriteSlot5DimRed(uint8_t bank);
uint8_t SpriteSlot5Off(uint8_t bank);
uint8_t SpriteSlot5Yellow(uint8_t bank);
uint8_t SpriteSlot6DimYellow(uint8_t bank);
uint8_t SpriteSlot6Red(uint8_t bank);
uint8_t SpriteSlot6Yellow(uint8_t bank);

#endif
