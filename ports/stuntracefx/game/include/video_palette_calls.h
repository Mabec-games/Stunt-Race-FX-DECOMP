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

#endif
