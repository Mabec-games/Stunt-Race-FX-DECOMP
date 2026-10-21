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

/* The list at $08AA up to the word at $005C drawn into c, as FxDrawList
   (raster.c) draws it; the shapes it does not draw yet (textured
   polygons) counted into *skipped. */
void fx_draw_list(const FxDrawSource *src, FxCanvas *c, int *skipped);

/* FxDrawList starting and done: the port's check of this file's fill
   against the Super FX's (PORT_FXCHECK=1; nothing otherwise) */
void fx_list_begin(void);
void fx_list_end(void);

#endif
