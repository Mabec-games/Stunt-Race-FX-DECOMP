/*
 * draw_list.c: the frame's queued shapes drawn natively: FxDrawList's
 * fill (raster.c, 01:A931) as plain C, the same arithmetic step for step
 * (16-bit values, the slopes from the ROM's reciprocal table, the edges'
 * rounding and clamping), into a canvas of pixels instead of the Super
 * FX's screen. The list is the one the Super FX's 3D code queued (the
 * format is raster.c's): flat polygons, lines and textured polygons.
 *
 * The fill copies each polygon's vertices below its entry (so the edge
 * walks can run past the end), so it works on its own copy of the RAM.
 *
 * The edge walks give each shape's rows as spans (draw_list.h FxSpan);
 * fx_fill_span plots a span's pixels as the Super FX does, and a GPU fills
 * the same spans in its shader.
 */
#include <stdlib.h>
#include <string.h>

#include "draw_list.h"

#define LIST_END    0x005Cu
#define DRAW_LIST   0x08AAu
#define RECIPROCALS 0x94A0u      /* bank $00: 1/n words */
/* the textured fill's words in RAM (raster.c's names) */
#define LEFT_U      0x0070u
#define RIGHT_U     0x0072u
#define LEFT_U_PREV 0x0074u
#define RIGHT_U_PREV 0x0076u
#define LEFT_DU     0x0078u
#define RIGHT_DU    0x007Au
#define LEFT_V      0x007Cu
#define RIGHT_V     0x007Eu
#define LEFT_V_PREV 0x0080u
#define RIGHT_V_PREV 0x0082u
#define LEFT_DV     0x0084u
#define RIGHT_DV    0x0086u
#define TEX_U_OFF   0x00C0u
#define TEX_V_OFF   0x00C2u

typedef struct {
    uint8_t *ram;
    const FxDrawSource *src;
    FxSpanSink sink;
    void *ctx;
    uint8_t colour, options;     /* COLOR and CMODE as the shape sets them */
} Fill;

/* a flat row (or a line's pixel): n pixels of the shape's colour */
static void flat_span(Fill *f, uint8_t x, uint8_t y, uint16_t n)
{
    FxSpan s;
    memset(&s, 0, sizeof s);
    s.x = x;
    s.y = y;
    s.n = n;
    s.colour = f->colour;
    s.options = f->options;
    s.mode = (uint8_t)f->src->mode;
    s.layout = (uint8_t)f->src->layout;
    f->sink(f->ctx, &s);
}

static uint16_t rd16(const uint8_t *m, uint16_t a) { return (uint16_t)(m[a] | m[(uint16_t)(a + 1)] << 8); }
static void wr16(uint8_t *m, uint16_t a, uint16_t v) { m[a] = (uint8_t)v; m[(uint16_t)(a + 1)] = (uint8_t)(v >> 8); }

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

/* COLOR and GETC: the colour from a byte as the plot options shape it
   (the source's high nibble into the low one, or the high nibble kept) */
typedef struct {
    const FxDrawSource *src;
    FxCanvas *c;
    uint8_t colour, options, mode, layout;
} Plot;

static void set_colour(Plot *f, uint8_t v)
{
    if (f->options & 0x04)
        f->colour = (uint8_t)((f->colour & 0xF0) | v >> 4);
    else if (f->options & 0x08)
        f->colour = (uint8_t)((f->colour & 0xF0) | (v & 0x0F));
    else
        f->colour = v;
}

/* PLOT's rules: colour 0 (its low nibble outside 8 bits, or with the
   high nibble kept) is not written unless the options say so; dither
   picks the high nibble on odd pixels */
static void plot(Plot *f, uint8_t x, uint8_t y)
{
    uint8_t c = f->colour;
    int md = f->mode;
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
    if (f->layout < 3) {                 /* where the layout puts a line past the height */
        unsigned h = 128u + 32u * f->layout, ax = x + 8u * (y / h);
        if (ax > 255u)
            return;
        x = (uint8_t)ax;
        y = (uint8_t)(y % h);
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
    uint16_t lx, rx, ls, rs, lrows, rrows, row;
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
            flat_span(f, (uint8_t)xmin, (uint8_t)ymin, (uint16_t)(xmax - xmin));
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
            if (count >= 0)
                flat_span(f, l, (uint8_t)row, (uint16_t)(count + 1));
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
            flat_span(f, (uint8_t)x, (uint8_t)y, 1);
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
            flat_span(f, (uint8_t)x, (uint8_t)y, 1);
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

/* a difference of u or v to a step: its low byte as the high byte of a
   signed word, times the reciprocal (FMULT), then ROL with FMULT's carry */
static uint16_t uv_step(uint16_t d, uint16_t recip)
{
    uint32_t p = (uint32_t)((int32_t)(int16_t)(uint16_t)((d & 0xFF) << 8) * (int16_t)recip);
    return (uint16_t)((p >> 16) << 1 | (p >> 15 & 1));
}

/* one edge's u or v for the row (tex_across): RIGHT += its step, the step
   across the row from right less left, LEFT += its step; *left_was the
   left value before its step */
static uint16_t across(uint8_t *m, uint16_t step_r, uint16_t right, uint16_t left, uint16_t step_l, uint16_t recip,
                       uint16_t *left_was)
{
    uint16_t r = rd16(m, right), l = rd16(m, left), d;
    uint32_t p;
    wr16(m, right, (uint16_t)(rd16(m, step_r) + r));
    p = (uint32_t)((int32_t)(int16_t)(uint16_t)(r - l) * (int16_t)recip);
    d = (uint16_t)((p >> 16) << 1 | (p >> 15 & 1));
    wr16(m, left, (uint16_t)(rd16(m, step_l) + l));
    *left_was = l;
    return d;
}

typedef struct {
    uint16_t x;                 /* 8.8 */
    uint16_t slope, rows, at;
} TexEdge;

/* A7D9: the next left edge (the walk forward from at) */
static int tex_left(Fill *f, TexEdge *e, uint16_t row)
{
    uint8_t *m = f->ram;
    uint16_t from, to, y, recip;
    e->x = (uint16_t)((uint16_t)(e->x + 0x7F) >> 8);
    wr16(m, LEFT_U, (uint16_t)(rd16(m, LEFT_U) >> 8));
    wr16(m, LEFT_V, (uint16_t)(rd16(m, LEFT_V) >> 8));
    to = e->x;
    for (;;) {
        from = to;
        wr16(m, LEFT_U_PREV, rd16(m, LEFT_U));
        wr16(m, LEFT_V_PREV, rd16(m, LEFT_V));
        to = m[e->at];
        y = m[(uint16_t)(e->at + 2)];
        wr16(m, LEFT_U, m[(uint16_t)(e->at + 4)]);
        wr16(m, LEFT_V, m[(uint16_t)(e->at + 6)]);
        e->at = (uint16_t)(e->at + 8);
        e->rows = (uint16_t)(y - row);
        if (e->rows)
            break;
    }
    if (e->rows & 0x8000)
        return 0;
    recip = reciprocal(f, e->rows);
    e->x = (uint16_t)(from << 8);
    e->slope = slope((uint16_t)(to - from), recip);
    wr16(m, LEFT_DU, uv_step((uint16_t)(rd16(m, LEFT_U) - rd16(m, LEFT_U_PREV)), recip));
    wr16(m, LEFT_U, (uint16_t)(rd16(m, LEFT_U_PREV) << 8 | rd16(m, LEFT_U_PREV) >> 8));
    wr16(m, LEFT_DV, uv_step((uint16_t)(rd16(m, LEFT_V) - rd16(m, LEFT_V_PREV)), recip));
    wr16(m, LEFT_V, (uint16_t)(rd16(m, LEFT_V_PREV) << 8 | rd16(m, LEFT_V_PREV) >> 8));
    return 1;
}

/* A879: the next right edge (the walk back from at) */
static int tex_right(Fill *f, TexEdge *e, uint16_t row)
{
    uint8_t *m = f->ram;
    uint16_t from, to, y, recip;
    e->x = (uint16_t)((uint16_t)(e->x + 0x7F) >> 8);
    wr16(m, RIGHT_U, (uint16_t)(rd16(m, RIGHT_U) >> 8));
    wr16(m, RIGHT_V, (uint16_t)(rd16(m, RIGHT_V) >> 8));
    to = e->x;
    for (;;) {
        from = to;
        wr16(m, RIGHT_U_PREV, rd16(m, RIGHT_U));
        wr16(m, RIGHT_V_PREV, rd16(m, RIGHT_V));
        wr16(m, RIGHT_V, m[(uint16_t)(e->at - 2)]);
        wr16(m, RIGHT_U, m[(uint16_t)(e->at - 4)]);
        y = m[(uint16_t)(e->at - 6)];
        e->at = (uint16_t)(e->at - 8);
        to = m[e->at];
        e->rows = (uint16_t)(y - row);
        if (e->rows)
            break;
    }
    if (e->rows & 0x8000)
        return 0;
    recip = reciprocal(f, e->rows);
    e->x = (uint16_t)(from << 8);
    e->slope = slope((uint16_t)(to - from), recip);
    wr16(m, RIGHT_DU, uv_step((uint16_t)(rd16(m, RIGHT_U) - rd16(m, RIGHT_U_PREV)), recip));
    wr16(m, RIGHT_U, (uint16_t)(rd16(m, RIGHT_U_PREV) << 8 | rd16(m, RIGHT_U_PREV) >> 8));
    wr16(m, RIGHT_DV, uv_step((uint16_t)(rd16(m, RIGHT_V) - rd16(m, RIGHT_V_PREV)), recip));
    wr16(m, RIGHT_V, (uint16_t)(rd16(m, RIGHT_V_PREV) << 8 | rd16(m, RIGHT_V_PREV) >> 8));
    return 1;
}

/* A662: a textured polygon of n vertices, its bank byte at p */
static uint16_t tex_polygon(Fill *f, uint16_t p, int n)
{
    uint8_t *m = f->ram;
    uint8_t bank = m[p];
    uint16_t word = rd16(m, (uint16_t)(p + 1)), base = rd16(m, (uint16_t)(p + 3)), mask = rd16(m, (uint16_t)(p + 5));
    uint16_t v = (uint16_t)(p + 7), size = (uint16_t)(8 * n), src = v, dst = (uint16_t)(v - size), top_at = 0;
    uint16_t ymin = 0x03FF, ymax = 0, row;
    TexEdge l, r;
    FxSpan s;
    int i;
    /* COLOR with the options clear, then CMODE */
    f->colour = (uint8_t)word;
    f->options = (uint8_t)((word >> 8) & 0x1F);
    memset(&s, 0, sizeof s);
    s.options = f->options;
    s.kind = 1;
    s.bank = bank;
    s.mode = (uint8_t)f->src->mode;
    s.layout = (uint8_t)f->src->layout;
    s.base = base;
    s.mask = mask;
    /* the vertices copied below, u and v of each moved down a byte where
       they are (and so in the copy) */
    for (i = 0; i < n; i++) {
        uint8_t y;
        m[dst] = m[src];
        y = m[(uint16_t)(src + 2)];
        m[(uint16_t)(dst + 2)] = y;
        if ((int16_t)(uint16_t)(y - ymin) < 0) {
            ymin = y;
            top_at = (uint16_t)(dst + 2);
        }
        if ((int16_t)(uint16_t)(y - ymax) >= 0)
            ymax = y;
        m[(uint16_t)(src + 4)] = m[(uint16_t)(src + 5)];
        m[(uint16_t)(dst + 4)] = m[(uint16_t)(src + 5)];
        m[(uint16_t)(src + 6)] = m[(uint16_t)(src + 7)];
        m[(uint16_t)(dst + 6)] = m[(uint16_t)(src + 7)];
        src = (uint16_t)(src + 8);
        dst = (uint16_t)(dst + 8);
    }
    if (ymin == ymax)
        return (uint16_t)(v + size);
    {
        uint16_t t = (uint16_t)(top_at - 2), x = m[t];
        l.x = r.x = (uint16_t)(x << 8);
        wr16(m, LEFT_U, (uint16_t)(m[(uint16_t)(t + 4)] << 8));
        wr16(m, RIGHT_U, (uint16_t)(m[(uint16_t)(t + 4)] << 8));
        wr16(m, LEFT_V, (uint16_t)(m[(uint16_t)(t + 6)] << 8));
        wr16(m, RIGHT_V, (uint16_t)(m[(uint16_t)(t + 6)] << 8));
        l.at = (uint16_t)(t + 8);
        r.at = (uint16_t)(t + size);
    }
    l.rows = r.rows = 0;
    row = ymin;
    for (;;) {
        uint16_t count, left_rows, right_rows;   /* count: the shorter edge's rows */
        if (!l.rows && !tex_left(f, &l, row))
            break;
        if (!r.rows && !tex_right(f, &r, row))
            break;
        /* A710: the rows while both edges last, the shorter's count */
        if (!((uint16_t)(l.rows - r.rows) & 0x8000)) {
            left_rows = (uint16_t)(l.rows - r.rows);
            right_rows = 0;
            count = r.rows;
        } else {
            right_rows = (uint16_t)(r.rows - l.rows);
            left_rows = 0;
            count = l.rows;
        }
        /* the row in R2's low byte, the rows left in its high byte */
        row = (uint16_t)(count << 8 | (row & 0xFF));
        do {
            uint8_t xl = (uint8_t)(l.x >> 8), xr = (uint8_t)(r.x >> 8);
            uint16_t width = (uint16_t)(xr - xl), recip, du, dv, tu, tv;
            if (xr >= xl)
                width = (uint16_t)(width + 1);
            else
                width = 1;
            recip = reciprocal(f, width);
            du = across(m, RIGHT_DU, RIGHT_U, LEFT_U, LEFT_DU, recip, &tu);
            dv = across(m, RIGHT_DV, RIGHT_V, LEFT_V, LEFT_DV, recip, &tv);
            tu = (uint16_t)(tu + rd16(m, TEX_U_OFF));
            tv = (uint16_t)(tv + rd16(m, TEX_V_OFF));
            s.x = xl;
            s.y = (uint8_t)row;
            s.n = width;
            s.colour = f->colour;
            s.tu = tu;
            s.tv = tv;
            s.du = du;
            s.dv = dv;
            f->sink(f->ctx, &s);
            l.x = edge_step(l.x, l.slope);
            r.x = edge_step(r.x, r.slope);
            row++;
            row = (uint16_t)(row << 8 | row >> 8);
            row--;
            row = (uint16_t)(row << 8 | row >> 8);
        } while (row >> 8);
        l.rows = left_rows;
        r.rows = right_rows;
    }
    return (uint16_t)(v + size);
}

uint16_t fx_noise_step(uint16_t s)
{
    uint16_t t = (uint16_t)((s & 0xFF) << 8 ^ s), w = (uint16_t)(t << 8 | t >> 8), r;
    r = (uint16_t)(((t & 0xFF) * 2u) ^ w);
    if (r & 1)
        return (uint16_t)((r >> 1) ^ 0xFF00 ^ 0x8180);
    r = (uint16_t)((r >> 1) ^ 0xFF00);
    return r == 0xAA55 ? 0 : r;
}

void fx_fill_span(const FxDrawSource *src, FxCanvas *c, const FxSpan *s)
{
    uint16_t noise = s->tu;
    Plot f;
    uint16_t k, tu = s->tu, tv = s->tv;
    uint8_t x = s->x;
    f.src = src;
    f.c = c;
    f.colour = s->colour;
    f.options = s->options;
    f.mode = s->mode;
    f.layout = s->layout;
    for (k = 0; k < s->n; k++, x++) {
        if (s->kind == 1) {
            uint16_t at = (uint16_t)((((tv & 0xFF00) | tu >> 8) & s->mask) + s->base);
            tv = (uint16_t)(tv + s->dv);
            tu = (uint16_t)(tu + s->du);
            set_colour(&f, rom_byte(src, s->bank, at));
        } else if (s->kind == 2) {
            uint16_t i = (uint16_t)(((uint32_t)s->tu + (uint32_t)k * s->du) >> 16);
            set_colour(&f, rom_byte(src, s->bank, (uint16_t)(s->dv ? s->base - i : s->base + i)));
        } else if (s->kind == 3) {
            unsigned p = (unsigned)(rom_byte(src, s->bank, (uint16_t)(s->base + (k >> 2))) >> ((k & 3) * 2)) & 3u;
            set_colour(&f, (uint8_t)(s->palette >> (p * 8)));
        } else if (s->kind == 5) {
            unsigned i;
            noise = fx_noise_step(noise);
            i = noise & 7u;
            set_colour(&f, (uint8_t)(i < 4 ? s->palette >> (i * 8) : (i < 6 ? s->tv : s->dv) >> ((i & 1) * 8)));
        } else if (s->kind == 4) {
            uint16_t d = (uint16_t)(s->tu + k), sw = (uint16_t)(d << 8 | d >> 8);
            uint16_t c = (uint16_t)(((int32_t)(int16_t)sw * (int16_t)s->du) >> 16);
            uint8_t b = rom_byte(src, s->bank, (uint16_t)(s->base + (uint16_t)((int16_t)c >> 2)));
            set_colour(&f, (uint8_t)(s->palette >> (((b >> ((c & 3) * 2)) & 3u) * 8)));
        }
        plot(&f, x, s->y);
    }
}

void fx_list_spans(const FxDrawSource *src, FxSpanSink sink, void *ctx)
{
    Fill f;
    uint16_t at, end;
    uint8_t *ram = (uint8_t *)malloc(0x10000);
    if (!ram)
        return;
    memcpy(ram, src->ram, 0x10000);
    f.ram = ram;
    f.src = src;
    f.sink = sink;
    f.ctx = ctx;
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
            else
                at = tex_polygon(&f, at, (uint8_t)~type);
        }
    }
    free(ram);
}

typedef struct {
    const FxDrawSource *src;
    FxCanvas *c;
} CanvasSink;

static void to_canvas(void *ctx, const FxSpan *s)
{
    const CanvasSink *k = (const CanvasSink *)ctx;
    fx_fill_span(k->src, k->c, s);
}

void fx_draw_list(const FxDrawSource *src, FxCanvas *c)
{
    CanvasSink k;
    k.src = src;
    k.c = c;
    fx_list_spans(src, to_canvas, &k);
}
