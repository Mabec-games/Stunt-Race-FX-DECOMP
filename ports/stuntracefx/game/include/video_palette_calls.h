/*
 * video_palette_calls.h: the S-CPU's palette and colour routines
 * (game/video/palette.c) as C functions, for the C that calls them.
 */
#ifndef STUNTRACEFX_VIDEO_PALETTE_CALLS_H
#define STUNTRACEFX_VIDEO_PALETTE_CALLS_H

#include <stdint.h>

/* The raster interrupt's colour writes to CGRAM: the last byte written.
   RasterPaletteUpload sets *animating when the animated colours were in
   use. */
uint8_t RasterColours(void);
uint8_t RasterPaletteUpload(int *animating);
uint8_t RasterColoursCycling(uint16_t x);
uint8_t RasterColoursE4(uint16_t x);
uint8_t RasterColoursUnless0D22(uint8_t bank, int *ran);

/* PaletteCycle returns the last phase stored and sets *x to the last table
   offset. */
uint16_t PaletteCycle(uint16_t *x);

/* The palette copies: the last colour (word) copied. */
uint16_t PaletteLoad16(uint8_t bank, uint16_t src);
uint16_t PaletteCopy16To7F39D7(uint8_t bank, uint16_t src);
uint16_t PaletteDarkenFour(uint16_t v);
uint16_t PaletteCourseLoad(uint8_t bank, uint16_t *xo, uint16_t *yo, uint8_t *p);

/* Colour blending (data bank `bank` for the work at $344E-$3470): the
   blended colour. */
uint16_t ColourBlend(uint8_t bank);
uint16_t ColourBlendBy(uint8_t bank);
uint16_t ColourBlendToTarget(uint8_t bank, uint16_t colour, uint16_t x);

/* PaletteKeptColoursStep takes A on entry (its high byte may survive),
   returns A, sets *xo to the cycle's offset and *stepped when the first two
   colours stepped. */
uint16_t PaletteKeptColoursStep(uint8_t bank, uint16_t a, uint16_t x, uint16_t y, uint16_t *xo, int *stepped);

/* The frame's palette effects. PlayerColoursFlash returns the third colour
   and sets *xo to the player's palette offset; StatusColourPick returns A
   and sets *stored when the colour was stored; PalettesStep sets *ran when
   $0E5F was set (else its result is only the low byte 0) and *xo. */
uint16_t PlayerColoursFlash(uint8_t bank, uint8_t level, uint16_t y, uint16_t *xo);
void PlayerColoursFlashRts(void);
uint16_t StatusColourPick(uint8_t bank, uint16_t y, int *stored);
uint16_t PalettesStep(uint8_t bank, int *ran, uint16_t *xo);

/* The fade and the course cycle: FadeValueStep and CoursePaletteCycle take
   A on entry and return it; CoursePaletteCycle sets *xo to the step's
   offset when the course has one; PaletteFadeStep returns whether it
   stepped, then *xo and *yo are X and Y at the end. */
uint16_t FadeValueStep(uint16_t a);
int PaletteFadeStep(uint8_t bank, uint16_t *xo, uint16_t *yo);
uint16_t CoursePaletteCycle(uint16_t a, int index8, uint16_t *xo);

/* HudColourStep and its two inner entries: A, X and Y at the end; for an
   inner entry also the data bank and P pulled from the stack (popped). */
typedef struct {
    uint16_t a, x, y;
    uint8_t db, p;
    int popped;
} HudColourEnd;

HudColourEnd HudColourStep(uint16_t x, uint16_t y);
HudColourEnd HudColourRowsRest(uint8_t bank, uint16_t a, uint16_t x, uint16_t y);
HudColourEnd HudColourStepFrom14(uint8_t bank, uint16_t y);

/* PaletteCycleFourth returns the colour's high byte; *xo is the new phase,
   *db the pulled data bank, *wrapped whether the phase wrapped. */
uint8_t PaletteCycleFourth(uint8_t bank, uint16_t y, uint16_t *xo, uint8_t *db, int *wrapped);

#endif
