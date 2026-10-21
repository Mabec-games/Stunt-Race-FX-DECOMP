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
    int mode, layout;
} FxDrawSource;

/* One row of one shape, as the fill draws it: n pixels (1-256) from x on
   row y, the x wrapping past 255 as the Super FX's byte does. By kind:
     0  flat: every pixel colour (a flat polygon, a line);
     1  textured: the k-th pixel's texel the byte at
        bank:(((tv + k dv) & $FF00 | (tu + k du) >> 8) & mask) + base
        (16-bit sums; a textured polygon, a shrunk sprite's row);
     2  texel runs: the k-th pixel's texel the byte at bank:base + i, or
        base - i when dv is 1, i = (tu + k du) >> 16 in full: the texels
        in turn, each for as long as a 16-bit sum of du from tu takes to
        carry (a magnified sprite's row);
     3  a bitmap's two bits: the k-th pixel's texel byte p of palette
        (four bytes, the first lowest), p the bits 2 (k mod 4) and up of
        the byte at bank:base + k / 4 (fx_blit's rows, the HUD's glyphs);
     4  a scaled bitmap's two bits: as 3 at column c of the row at base,
        c = (swap(tu + k) du) >> 16 (signed 16-bit factors, swap the
        bytes exchanged), the byte at base + (c >> 2) (signed) and the
        bits 2 (c mod 4) and up (the HUD's scaled sprites);
     5  noise: the k-th pixel's texel byte r & 7 of the ramp palette, tv,
        dv (eight bytes, the first lowest), r the generator's value after
        k + 1 steps from tu (draw_list.c noise_step; the HUD's noise
        fill).
   A texel is shaped into colour by options (CMODE) as COLOR does. Every
   pixel then follows PLOT's rules at the screen's colour depth (mode, as
   FxDrawSource's) (draw_list.c fx_fill_span), and nothing else carries
   from one pixel to the next. PLOT's (x, y) lands where the screen's
   layout (layout, gsu.h gsu_layout) puts it: in the 128, 160 and 192-line
   layouts a line y at or past the height h is line y mod h, 8 (y / h)
   pixels to the right (past x 255, outside the canvas). */
typedef struct {
    uint8_t x, y;
    uint16_t n;
    uint8_t colour, options, kind, bank, mode, layout;
    uint16_t tu, tv, du, dv, base, mask;
    uint32_t palette;
} FxSpan;

/* The list at $08AA up to the word at $005C, as FxDrawList (raster.c)
   walks it: each shape's rows in the order it draws them, into
   sink(ctx, span). */
typedef void (*FxSpanSink)(void *ctx, const FxSpan *s);
void fx_list_spans(const FxDrawSource *src, FxSpanSink sink, void *ctx);
/* one row's pixels into c, as the Super FX plots them (the ROM from src) */
void fx_fill_span(const FxDrawSource *src, FxCanvas *c, const FxSpan *s);
/* the whole list drawn into c: the two above */
void fx_draw_list(const FxDrawSource *src, FxCanvas *c);

/* FxDrawList starting and done: the port's check of this file's fill
   against the Super FX's (PORT_FXCHECK=1; nothing otherwise) */
void fx_list_begin(void);
void fx_list_end(void);

/* The screen's operations, in the order the Super FX makes them: the
   buffer cleared to colour 0 (FxScreenClear), and the rows of the drawers
   other than the draw lists' (whose rows come from fx_list_spans). */
void fx_op_clear(void);
void fx_op_span(const FxSpan *s);
/* a row through a palette (kinds 3, 4), the palette the four bytes at
   palette in the RAM as the row is drawn; a noise row (kind 5), the ramp
   the low bytes of the eight words at ramp */
void fx_op_palette_span(FxSpan *s, uint16_t palette);
void fx_op_ramp_span(FxSpan *s, uint16_t ramp);
/* the noise fill's generator, one step (07:EEBB) */
uint16_t fx_noise_step(uint16_t s);
/* the S-CPU copying the screen to VRAM (VramDmaRowsFromGsuRam,
   VramDmaWideRowsFromGsuRam): on 1 as it starts, 0 as it ends */
void fx_op_upload(int on);

#endif
