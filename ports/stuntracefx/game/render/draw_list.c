/*
 * draw_list.c: the frame's queued shapes drawn natively: FxDrawList's
 * fill (raster.c, 01:A931) as plain C, the same arithmetic step for step
 * (16-bit values, the slopes from the ROM's reciprocal table, the edges'
 * rounding and clamping), into a canvas of pixels instead of the Super
 * FX's screen. The list is the one the Super FX's 3D code queued (the
 * format is raster.c's): flat polygons and lines here; textured polygons
 * are counted, not drawn yet.
 *
 * The fill copies each polygon's vertices below its entry (so the edge
 * walks can run past the end), so it works on its own copy of the RAM.
 */
#include <stdlib.h>
#include <string.h>

#include "draw_list.h"

#define LIST_END    0x005Cu
#define DRAW_LIST   0x08AAu
#define RECIPROCALS 0x94A0u      /* bank $00: 1/n words */

typedef struct {
    uint8_t *ram;
    const FxDrawSource *src;
    FxCanvas *c;
    uint8_t colour, options;     /* COLOR and CMODE as the shape sets them */
} Fill;

static uint8_t rom_byte(const FxDrawSource *s, uint8_t bank, uint16_t addr)
{
    uint32_t off = bank < 0x40 ? (uint32_t)bank * 0x8000u + (addr & 0x7FFFu) : (uint32_t)(bank - 0x40) * 0x10000u + addr;
    return s->rom[off % s->rom_size];
}

static uint16_t reciprocal(const Fill *f, uint16_t n)
{
    uint16_t at = (uint16_t)(RECIPROCALS + n + n);
    return (uint16_t)(rom_byte(f->src, 0, at) | rom_byte(f->src, 0, (uint16_t)(at + 1)) << 8);
}

/* PLOT's rules: colour 0 (its low nibble outside 8 bits, or with the
   high nibble kept) is not written unless the options say so; dither
   picks the high nibble on odd pixels */
static void plot(Fill *f, uint8_t x, uint8_t y)
{
    uint8_t c = f->colour;
    int md = f->src->mode;
    if (!(f->options & 0x01)) {
        if (md == 3) {
            if (f->options & 0x08) {
                if (!(c & 0x0F))
                    return;
            } else if (!c) {
                return;
            }
        } else if (!(c & 0x0F)) {
            return;
        }
    }
    if ((f->options & 0x02) && md != 3) {
        if ((x ^ y) & 1)
            c >>= 4;
        c &= 0x0F;
    }
    f->c->px[y << 8 | x] = c;
    f->c->set[y << 8 | x] = 1;
}

/* an edge's slope in 8.8: dx times the reciprocal of its height, the
   product's bits 7-22 */
static uint16_t slope(uint16_t dx, uint16_t recip)
{
    uint32_t p = (uint32_t)((int32_t)(int16_t)dx * (int16_t)recip);
    return (uint16_t)(p >> 7);
}

/* an edge's x stepped: set to 0 when it has gone just below it (to -8.0) */
static uint16_t edge_step(uint16_t x, uint16_t step)
{
    x = (uint16_t)(x + step);
    if ((x & 0x8000) && !((uint16_t)(x + 0x0800) & 0x8000))
        x = 0;
    return x;
}

/* A95C: a flat polygon of n vertices, its colour byte at p */
static uint16_t flat_polygon(Fill *f, uint16_t p, int n)
{
    uint8_t *m = f->ram;
    uint16_t v = (uint16_t)(p + 1), size = (uint16_t)(4 * n), dst = (uint16_t)(v - size), top_at = 0, src = v;
    uint16_t ymin = 0x03FF, ymax = 0, left_at, right_at;
    uint16_t lx, rx, ls, rs, lrows, rrows, row, x;
    int i;
    f->options = 0;
    f->colour = m[p];
    /* the vertices copied below, the top and bottom rows found */
    for (i = 0; i < n; i++) {
        m[dst] = m[src];
        dst = (uint16_t)(dst + 2);
        src = (uint16_t)(src + 2);
        m[dst] = m[src];
        if (m[src] < ymin) {
            ymin = m[src];
            top_at = dst;
        }
        if (m[src] >= ymax)
            ymax = m[src];
        dst = (uint16_t)(dst + 2);
        src = (uint16_t)(src + 2);
    }
    if ((int16_t)(ymax - ymin - 2) < 0) {
        /* AA74: under two rows: one row at the top, the smallest x to the
           largest (not including it) */
        uint16_t xmin = 0x03FF, xmax = 0;
        dst = v;
        for (i = 0; i < n; i++) {
            if (m[dst] < xmin)
                xmin = m[dst];
            if (m[dst] >= xmax)
                xmax = m[dst];
            dst = (uint16_t)(dst + 4);
        }
        if ((int16_t)(xmax - xmin) > 0)
            for (x = xmin; x != xmax; x++)
                plot(f, (uint8_t)x, (uint8_t)ymin);
        return (uint16_t)(v + size);
    }
    row = ymin;
    left_at = (uint16_t)(top_at - 2);                 /* the top vertex's x, in the copy */
    right_at = (uint16_t)(left_at + size - 2);       /* the vertex before it, in the original */
    lx = m[left_at];
    rx = (uint16_t)(lx << 8);
    left_at = (uint16_t)(left_at + 4);
    rrows = 0;
    /* the first left edge */
    {
        uint16_t from = lx, to, y;
        for (;;) {
            to = m[left_at];
            y = m[(uint16_t)(left_at + 2)];
            left_at = (uint16_t)(left_at + 4);
            lrows = (uint16_t)(y - row);
            if (lrows & 0x8000)
                return (uint16_t)(v + size);
            if (lrows)
                break;
            from = to;
        }
        ls = slope((uint16_t)(to - from), reciprocal(f, lrows));
        lx = (uint16_t)(from << 8);
    }
    for (;;) {
        if (!rrows) {
            /* the next right edge, walking back */
            uint16_t from = (uint16_t)((uint16_t)(rx + 0x7F) >> 8), to, y;
            for (;;) {
                y = m[right_at];
                right_at = (uint16_t)(right_at - 2);
                to = m[right_at];
                right_at = (uint16_t)(right_at - 2);
                rrows = (uint16_t)(y - row);
                if (rrows & 0x8000)
                    return (uint16_t)(v + size);
                if (rrows)
                    break;
                from = to;
            }
            rs = slope((uint16_t)(to - from), reciprocal(f, rrows));
            rx = (uint16_t)(from << 8);
        }
        /* the rows while both edges last (the two counts ANDed, as the
           original tests them) */
        do {
            uint8_t l = (uint8_t)(lx >> 8), r = (uint8_t)(rx >> 8);
            int16_t count = (int16_t)(uint16_t)(r - l);
            if (count >= 0) {
                uint16_t px = l;
                int k;
                for (k = 0; k <= count; k++, px++)
                    plot(f, (uint8_t)px, (uint8_t)row);
            }
            lx = edge_step(lx, ls);
            lrows--;
            rx = edge_step(rx, rs);
            rrows--;
            row++;
        } while (lrows & rrows);
        if (lrows)
            continue;
        /* the next left edge */
        {
            uint16_t from = (uint16_t)((uint16_t)(lx + 0x7F) >> 8), to, y;
            for (;;) {
                to = m[left_at];
                y = m[(uint16_t)(left_at + 2)];
                left_at = (uint16_t)(left_at + 4);
                lrows = (uint16_t)(y - row);
                if (lrows & 0x8000)
                    return (uint16_t)(v + size);
                if (lrows)
                    break;
                from = to;
            }
            ls = slope((uint16_t)(to - from), reciprocal(f, lrows));
            lx = (uint16_t)(from << 8);
        }
    }
}

/* AAA2: a line, stepped along its longer axis */
static uint16_t line(Fill *f, uint16_t p)
{
    uint8_t *m = f->ram;
    uint16_t x = m[(uint16_t)(p + 1)], y = m[(uint16_t)(p + 3)];
    uint16_t dx = (uint16_t)(m[(uint16_t)(p + 5)] - x), dy = (uint16_t)(m[(uint16_t)(p + 7)] - y);
    uint16_t sx = 1, sy = 1, e, n;
    int carry;
    f->options = 0;
    f->colour = m[p];
    if (dx & 0x8000) { sx = 0xFFFF; dx = (uint16_t)-dx; }
    if (dy & 0x8000) { sy = 0xFFFF; dy = (uint16_t)-dy; }
    if (!((uint16_t)(dx - dy) & 0x8000)) {
        n = (uint16_t)(dx + 1);
        e = (uint16_t)((uint16_t)(dx - dy) >> 1);
        carry = e >= dy;
        e = (uint16_t)(e - dy);
        do {
            plot(f, (uint8_t)x, (uint8_t)y);
            if (!carry) {
                e = (uint16_t)(e + dx);
                y = (uint16_t)(y + sy);
            }
            x = (uint16_t)(x + sx);
            n--;
            carry = e >= dy;
            e = (uint16_t)(e - dy);
        } while (n);
    } else {
        n = (uint16_t)(dy + 1);
        e = (uint16_t)((uint16_t)(dy - dx) >> 1);
        carry = e >= dx;
        e = (uint16_t)(e - dx);
        do {
            plot(f, (uint8_t)x, (uint8_t)y);
            if (!carry) {
                e = (uint16_t)(e + dy);
                x = (uint16_t)(x + sx);
            }
            y = (uint16_t)(y + sy);
            n--;
            carry = e >= dx;
            e = (uint16_t)(e - dx);
        } while (n);
    }
    return (uint16_t)(p + 9);
}

void fx_draw_list(const FxDrawSource *src, FxCanvas *c, int *skipped)
{
    Fill f;
    uint16_t at, end;
    uint8_t *ram = (uint8_t *)malloc(0x10000);
    if (!ram)
        return;
    memcpy(ram, src->ram, 0x10000);
    f.ram = ram;
    f.src = src;
    f.c = c;
    *skipped = 0;
    end = (uint16_t)(ram[LIST_END] | ram[LIST_END + 1] << 8);
    if (end != DRAW_LIST) {
        ram[end] = ram[(uint16_t)(end + 1)] = 0;   /* the list's end, as FxDrawList writes it */
        at = DRAW_LIST;
        for (;;) {
            int8_t type = (int8_t)ram[at];
            at++;
            if (type >= 3)
                at = flat_polygon(&f, at, type);
            else if (type == 2)
                at = line(&f, at);
            else if (type == 0)
                break;
            else {
                int n = (uint8_t)~type;
                (*skipped)++;
                at = (uint16_t)(at + 7 + 8 * n);
            }
        }
    }
    free(ram);
}
