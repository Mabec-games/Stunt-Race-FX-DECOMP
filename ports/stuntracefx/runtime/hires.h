/*
 * hires.h: a race draw's 3D as the Super FX made it, kept as shapes with
 * exact (unrounded) positions, for drawing again at any resolution
 * (hires.c captures, hirender.c draws).
 *
 * A list holds, in the order the Super FX draws them: polygons from its
 * draw lists (flat, textured, lines), each corner where the projection
 * put it before the cut to whole pixels (corners the screen-edge clip
 * made stay whole: they lie on an edge); and sprites (a scaled bitmap:
 * its exact centre and size, and the rows of texels the Super FX plotted
 * for it), and any other plotted rows.
 */
#ifndef PORT_HIRES_H
#define PORT_HIRES_H

#include <stdint.h>
#include "draw_list.h"

#define HR_MAXV 12
#define HR_MAXOPS 768
#define HR_MAXROWS 2048

enum { HR_FLAT, HR_LINE, HR_TEX, HR_SPRITE, HR_ROWS };

typedef struct {
    uint8_t kind, n, colour, options, bank;
    uint16_t base, mask;
    float x[HR_MAXV], y[HR_MAXV];      /* exact corners (Super FX pixels) */
    uint16_t u[HR_MAXV], v[HR_MAXV];   /* textured: 8.8 */
    /* sprites: the projection (whole and exact) and its rows */
    int16_t xi, yi, si;
    float xf, yf, sf;
    uint16_t row0, nrows;              /* rows[row0 ..] */
} HrOp;

typedef struct {
    int nops, nrows, overflow;
    int16_t wl, wt, wr, wb;            /* drawn wider than the game's window by these (left, top, right, bottom; 0 the game's) */
    HrOp ops[HR_MAXOPS];
    FxSpan rows[HR_MAXROWS];
} HrList;

/* the list the next race draw ($01:AB13) is captured into (NULL: none);
   capture starts at the program's start and ends at its STOP */
void hires_capture(HrList *l);
void hires_capture_begin(void);
void hires_capture_end(void);

/* small shapes as they are (1), or widened and grown like all (0) */
void hires_set_small(int on);
int hires_get_small(void);

/* draw list l's 3D at s x the Super FX's pixels, a x a samples a pixel,
   into argb (cw s x ch s pixels, alpha = coverage; 0 nothing drawn):
   colours from palette (256 BGR555 words) at brightness bright (0-15) */
void hires_render(const HrList *l, const uint8_t *rom, uint32_t rom_size, const uint16_t *palette, int bright,
                  int s, int a, int cw, int ch, uint32_t *argb);

#endif
