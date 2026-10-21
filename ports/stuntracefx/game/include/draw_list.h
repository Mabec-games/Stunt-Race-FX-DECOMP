/*
 * draw_list.h: the frame's queued shapes drawn natively (draw_list.c).
 */
#ifndef STUNTRACEFX_DRAW_LIST_H
#define STUNTRACEFX_DRAW_LIST_H

#include <stdint.h>

/* What the shapes leave on the Super FX's screen: a colour for each pixel
   written (x, y each 0-255), and which were. */
typedef struct {
    uint8_t px[256 * 256];
    uint8_t set[256 * 256];
} FxCanvas;

/* The screen's colour depth, as the plot rules need it (SCMR's mode bits:
   0 2 bits a pixel, 1 4 bits, 3 8 bits). */
typedef struct {
    const uint8_t *ram;      /* the game pak RAM (bank $70), 64 KB */
    const uint8_t *rom;      /* the cartridge ROM */
    uint32_t rom_size;
    int mode;
} FxDrawSource;

/* One row of one shape, as the fill draws it: n pixels (1-256) from x on
   row y, the x wrapping past 255 as the Super FX's byte does. A flat
   polygon's or a line's pixels take colour (the plot options 0); a
   textured polygon's (textured = 1) each take a texel: the k-th pixel's
   byte at bank:(((tv + k dv) & $FF00 | (tu + k du) >> 8) & mask) + base
   (16-bit sums), shaped into colour by options (CMODE) as COLOR does.
   Every pixel then follows PLOT's rules (draw_list.c fx_fill_span), and
   nothing else carries from one pixel to the next. */
typedef struct {
    uint8_t x, y;
    uint16_t n;
    uint8_t colour, options, textured, bank;
    uint16_t tu, tv, du, dv, base, mask;
} FxSpan;

/* The list at $08AA up to the word at $005C, as FxDrawList (raster.c)
   walks it: each shape's rows in the order it draws them, into
   sink(ctx, span). */
typedef void (*FxSpanSink)(void *ctx, const FxSpan *s);
void fx_list_spans(const FxDrawSource *src, FxSpanSink sink, void *ctx);
/* one row's pixels into c, as the Super FX plots them */
void fx_fill_span(const FxDrawSource *src, FxCanvas *c, const FxSpan *s);
/* the whole list drawn into c: the two above */
void fx_draw_list(const FxDrawSource *src, FxCanvas *c);

/* FxDrawList starting and done: the port's check of this file's fill
   against the Super FX's (PORT_FXCHECK=1; nothing otherwise) */
void fx_list_begin(void);
void fx_list_end(void);

#endif
