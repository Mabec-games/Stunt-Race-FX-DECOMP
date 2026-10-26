/*
 * hirender.c: a race draw's shapes (hires.h) drawn again at s times the
 * Super FX's resolution, a x a samples a pixel, in the order it drew them.
 *
 * Polygons are filled a sample row at a time between their edges; a
 * textured one takes u and v down its left and right edges and across the
 * row between them (as the Super FX's fill walks it), each texel shaped by
 * the plot options as COLOR does and colour 0 left clear as PLOT does. A
 * shape thinner than one of the Super FX's pixels is drawn one pixel thick
 * (its fill always plots a row). A sprite is drawn from its exact centre
 * and size: each sample taken back into the sprite's own pixels and its
 * texel found as the Super FX's row found it there, continued between its
 * pixels (more of a shrunk bitmap's texels show) within the texels the
 * row's own pixels reached. Other plotted rows are drawn as they were.
 *
 * The samples are averaged into colour and coverage (alpha): what the 3D
 * leaves uncovered is for the caller to fill (the sky behind).
 *
 * The picture is made in bands of rows, one a thread (PORT_HIRES_THREADS,
 * default 4): each band draws every shape in order, its own rows only.
 */
#include <math.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "hires.h"

static uint32_t *samp;
static size_t samp_size;
static uint32_t lut[256];
static const uint8_t *rom_p;
static uint32_t rom_n;
static int SW, SH, N;

static uint8_t rom_byte(uint8_t bank, uint16_t addr)
{
    uint32_t off = bank < 0x40 ? (uint32_t)bank * 0x8000u + (addr & 0x7FFFu) : (uint32_t)(bank - 0x40) * 0x10000u + addr;
    return rom_p[(rom_n & (rom_n - 1)) ? off % rom_n : off & (rom_n - 1)];
}

/* COLOR as the options shape texel t on colour c; -1: PLOT writes nothing */
static int shape(uint8_t c, uint8_t options, uint8_t t)
{
    int v;
    options &= 0x1F;
    if (options & 0x04)
        v = (c & 0xF0) | t >> 4;
    else if (options & 0x08)
        v = (c & 0xF0) | (t & 0x0F);
    else
        v = t;
    if (!(options & 0x01)) {
        if (options & 0x08 ? !(v & 0x0F) : !v)
            return -1;
    }
    return v;
}

static int s16(uint16_t v) { return (int16_t)v; }

/* the samples of row j inside a convex polygon: [*i0, *i1); 0 none. With
   uv, u and v at both ends (along the edges) */
static int row_span(const float *x, const float *y, int n, int j, int *i0, int *i1,
                    const float *u, const float *v, float *ul, float *vl, float *ur, float *vr, float *xl_out, float *xr_out)
{
    float yc = (j + 0.5f) / N, xl = 1e30f, xr = -1e30f;
    int k;
    for (k = 0; k < n; k++) {
        int m = (k + 1) % n;
        float y0 = y[k], y1 = y[m], lo = y0 < y1 ? y0 : y1, hi = y0 < y1 ? y1 : y0, t, xc;
        if (y0 == y1 || yc < lo || yc >= hi)
            continue;
        t = (yc - y0) / (y1 - y0);
        xc = x[k] + t * (x[m] - x[k]);
        if (xc < xl) {
            xl = xc;
            if (u) {
                *ul = u[k] + t * (u[m] - u[k]);
                *vl = v[k] + t * (v[m] - v[k]);
            }
        }
        if (xc > xr) {
            xr = xc;
            if (u) {
                *ur = u[k] + t * (u[m] - u[k]);
                *vr = v[k] + t * (v[m] - v[k]);
            }
        }
    }
    if (xl > xr)
        return 0;
    *i0 = (int)ceilf(xl * N - 0.5f);
    *i1 = (int)ceilf(xr * N - 0.5f);
    if (*i0 < 0)
        *i0 = 0;
    if (*i1 > SW)
        *i1 = SW;
    *xl_out = xl;
    *xr_out = xr;
    return *i0 < *i1;
}

/* a polygon at least a pixel across each way (as the Super FX's fill
   plots it): too thin a one widened about its middle */
static void thicken(float *x, float *y, int n)
{
    float x0 = x[0], x1 = x[0], y0 = y[0], y1 = y[0], cx, cy;
    int k;
    for (k = 1; k < n; k++) {
        if (x[k] < x0) x0 = x[k];
        if (x[k] > x1) x1 = x[k];
        if (y[k] < y0) y0 = y[k];
        if (y[k] > y1) y1 = y[k];
    }
    cx = (x0 + x1) / 2;
    cy = (y0 + y1) / 2;
    if (y1 - y0 < 1.0f)
        for (k = 0; k < n; k++)
            y[k] = y1 - y0 > 1e-4f ? cy + (y[k] - cy) / (y1 - y0) : cy + (k < n / 2 ? -0.5f : 0.5f);
    if (x1 - x0 < 1.0f)
        for (k = 0; k < n; k++)
            x[k] = x1 - x0 > 1e-4f ? cx + (x[k] - cx) / (x1 - x0) : cx + (k == 0 || k == n - 1 ? -0.5f : 0.5f);
}

/* a convex polygon grown by e on every side (each edge moved out along
   its normal, the corners where the moved edges meet, held to 2 e out at
   sharp ones): the Super FX's fill covers a pixel more to the right and
   below than the shape, which closes the small gaps the course's pieces
   leave between them; drawn exactly they would show as seams */
static float grow_by = -1;
/* small shapes left as they are (F6, PORT_HIRES_SMALL=1): off, every shape
   at least a pixel thick and grown by the full half pixel */
int hires_small = -1;
void hires_set_small(int on) { hires_small = on; }
int hires_get_small(void)
{
    if (hires_small < 0)
        hires_small = getenv("PORT_HIRES_SMALL") && strcmp(getenv("PORT_HIRES_SMALL"), "0");
    return hires_small;
}

static int grow(float *x, float *y, int n, float e)
{
    float nx[HR_MAXV + 2], ny[HR_MAXV + 2], ox[HR_MAXV + 2], oy[HR_MAXV + 2], area = 0;
    int k, m = 0;
    /* corners repeated in a row dropped */
    for (k = 0; k < n; k++) {
        int p = (k + n - 1) % n;
        if (fabsf(x[k] - x[p]) > 1e-4f || fabsf(y[k] - y[p]) > 1e-4f) {
            ox[m] = x[k];
            oy[m] = y[k];
            m++;
        }
    }
    if (m < 3)
        return n;
    for (k = 0; k < m; k++) {
        int q = (k + 1) % m;
        area += ox[k] * oy[q] - ox[q] * oy[k];
    }
    if (fabsf(area) < 1e-6f)
        return n;
    for (k = 0; k < m; k++) {
        /* edge k: from corner k to k + 1; its outward normal */
        int q = (k + 1) % m;
        float dx = ox[q] - ox[k], dy = oy[q] - oy[k], l = sqrtf(dx * dx + dy * dy);
        nx[k] = area > 0 ? dy / l : -dy / l;
        ny[k] = area > 0 ? -dx / l : dx / l;
    }
    for (k = 0; k < m; k++) {
        /* corner k between edges k - 1 and k: along the two normals' mean,
           so far that both edges move by e */
        int p = (k + m - 1) % m;
        float bx = nx[p] + nx[k], by = ny[p] + ny[k], d = bx * nx[k] + by * ny[k];
        float f = d > 1e-3f ? e / d : 2 * e;
        float lx, ly;
        lx = bx * f;
        ly = by * f;
        {
            float len = sqrtf(lx * lx + ly * ly);
            if (len > 2 * e) {
                lx *= 2 * e / len;
                ly *= 2 * e / len;
            }
        }
        x[k] = ox[k] + lx;
        y[k] = oy[k] + ly;
    }
    return m;
}

static void poly(const HrOp *o, int b0, int b1)
{
    float x[HR_MAXV + 2], y[HR_MAXV + 2], u[HR_MAXV + 2], v[HR_MAXV + 2];
    float ymin = 1e30f, ymax = -1e30f, umin = 1e30f, umax = -1e30f, vmin = 1e30f, vmax = -1e30f;
    int n = o->n, k, j, tex = o->kind == HR_TEX;
    if (o->kind == HR_LINE) {
        /* a line: a quad one pixel wide */
        float dx = o->x[1] - o->x[0], dy = o->y[1] - o->y[0], l = sqrtf(dx * dx + dy * dy), hx, hy;
        if (l < 1e-6f) {
            dx = 1;
            dy = 0;
            l = 1;
        }
        hx = -dy / l * 0.5f;
        hy = dx / l * 0.5f;
        x[0] = o->x[0] + hx; y[0] = o->y[0] + hy;
        x[1] = o->x[1] + hx; y[1] = o->y[1] + hy;
        x[2] = o->x[1] - hx; y[2] = o->y[1] - hy;
        x[3] = o->x[0] - hx; y[3] = o->y[0] - hy;
        n = 4;
    } else {
        if (n < 3)
            return;
        for (k = 0; k < n; k++) {
            x[k] = o->x[k];
            y[k] = o->y[k];
            u[k] = o->u[k];
            v[k] = o->v[k];
            if (u[k] < umin) umin = u[k];
            if (u[k] > umax) umax = u[k];
            if (v[k] < vmin) vmin = v[k];
            if (v[k] > vmax) vmax = v[k];
        }
        /* by the shape's size (as captured): a small one is left as it is
           (its coverage averaged; widened or grown it would cover its
           neighbours, a far finish line's squares or a fence's slats, and
           which shows would change from picture to picture); a long thin
           one made a pixel thick (a far lane line); growth at most a
           quarter of the smaller side (the full half pixel for the course's
           large pieces, whose joins it closes) */
        float bw, bh, e;
        {
            float x0 = x[0], x1 = x[0], y0 = y[0], y1 = y[0];
            for (k = 1; k < n; k++) {
                if (x[k] < x0) x0 = x[k];
                if (x[k] > x1) x1 = x[k];
                if (y[k] < y0) y0 = y[k];
                if (y[k] > y1) y1 = y[k];
            }
            bw = x1 - x0;
            bh = y1 - y0;
        }
        int old = !hires_get_small();
        if (old || bw >= 1.5f || bh >= 1.5f)
            thicken(x, y, n);
        if (grow_by < 0)
            grow_by = getenv("PORT_HIRES_GROW") ? (float)atof(getenv("PORT_HIRES_GROW")) : 0.5f;
        e = grow_by;
        {
            float small = bw < bh ? bw : bh;
            if (small < 1.0f)
                small = 1.0f;       /* (a sliver made a pixel thick) */
            if (bw < 1.5f && bh < 1.5f)
                small = bw < bh ? bw : bh;
            if (e > small / 4 && !old)
                e = small / 4;
        }
        if (e > 0.01f) {
            if (tex) {
                /* u and v kept with their corners (the moved ones, the
                   corners repeated in a row dropped the same way) */
                float tu[HR_MAXV + 2], tv[HR_MAXV + 2];
                int m = 0;
                for (k = 0; k < n; k++) {
                    int pk = (k + n - 1) % n;
                    if (fabsf(x[k] - x[pk]) > 1e-4f || fabsf(y[k] - y[pk]) > 1e-4f) {
                        tu[m] = u[k];
                        tv[m] = v[k];
                        m++;
                    }
                }
                if (m >= 3 && grow(x, y, n, e) == m) {
                    memcpy(u, tu, sizeof(float) * (size_t)m);
                    memcpy(v, tv, sizeof(float) * (size_t)m);
                    n = m;
                }
            } else
                n = grow(x, y, n, e);
        }
    }
    for (k = 0; k < n; k++) {
        if (y[k] < ymin) ymin = y[k];
        if (y[k] > ymax) ymax = y[k];
    }
    {
        int j0 = (int)floorf(ymin * N), j1 = (int)ceilf(ymax * N);
        if (j0 < b0) j0 = b0;
        if (j1 > b1) j1 = b1;
        for (j = j0; j < j1; j++) {
            int i0, i1, i;
            float ul = 0, vl = 0, ur = 0, vr = 0, xl, xr;
            uint32_t *row = samp + (size_t)j * SW;
            if (!row_span(x, y, n, j, &i0, &i1, tex ? u : NULL, v, &ul, &vl, &ur, &vr, &xl, &xr))
                continue;
            if (!tex) {
                uint32_t c = lut[o->colour];
                for (i = i0; i < i1; i++)
                    row[i] = c;
                continue;
            }
            for (i = i0; i < i1; i++) {
                float t = xr > xl ? ((i + 0.5f) / N - xl) / (xr - xl) : 0, uu = ul + t * (ur - ul), vv = vl + t * (vr - vl);
                int ui, vi, c;
                uint16_t at;
                if (uu < umin) uu = umin;
                if (uu > umax) uu = umax;
                if (vv < vmin) vv = vmin;
                if (vv > vmax) vv = vmax;
                ui = (int)uu;
                vi = (int)vv;
                at = (uint16_t)((((vi & 0xFF00) | (ui >> 8 & 0xFF)) & o->mask) + o->base);
                c = shape(o->colour, o->options, rom_byte(o->bank, at));
                if (c >= 0)
                    row[i] = lut[c];
            }
        }
    }
}

/* row r's texel at (fractional) pixel k of it and fraction fy down it;
   -1 clear. kind 1: dvrow the 8.8 step to the next row, vlo/vhi the texel
   rows the sprite's rows reached */
static int span_texel(const FxSpan *r, float k, float fy, int dvrow, int vlo, int vhi)
{
    if (r->kind == 0)                    /* flat: the colour, 0 clear unless the options plot it */
        return r->colour || (r->options & 0x01) ? r->colour : -1;
    if (r->kind == 1) {
        int du = s16(r->du);
        float ua = floorf(r->tu / 256.0f), ub = floorf((r->tu + (r->n - 1) * (float)du) / 256.0f);
        float uu = floorf((r->tu + k * du) / 256.0f), vv = floorf((r->tv + fy * dvrow) / 256.0f);
        float lo = ua < ub ? ua : ub, hi = ua < ub ? ub : ua;
        int ui, vi;
        if (uu < lo) uu = lo;
        if (uu > hi) uu = hi;
        if (vv < vlo) vv = (float)vlo;
        if (vv > vhi) vv = (float)vhi;
        ui = (int)uu & 0xFF;
        vi = (int)vv & 0xFF;
        return shape(r->colour, r->options, rom_byte(r->bank, (uint16_t)((((vi << 8) | ui) & r->mask) + r->base)));
    }
    if (r->kind == 2) {
        float i = floorf((r->tu + k * (float)r->du) / 65536.0f), lo = (float)(r->tu >> 16 ? 0 : 0), hi;
        lo = floorf(r->tu / 65536.0f);
        hi = floorf((r->tu + (r->n - 1) * (float)r->du) / 65536.0f);
        if (i < lo) i = lo;
        if (i > hi) i = hi;
        return shape(r->colour, r->options, rom_byte(r->bank, (uint16_t)(r->dv ? r->base - (int)i : r->base + (int)i)));
    }
    return -1;      /* the HUD's bitmaps: not in a race's 3D */
}

static void sprite(const HrList *l, const HrOp *o, int b0, int b1)
{
    const FxSpan *rows = l->rows + o->row0;
    int nr = o->nrows, k, ymin = 255, ymax = 0, xmin = 255, xmax = 0, half, exact = o->kind == HR_SPRITE && o->sf > 0;
    const FxSpan *by_y[256];
    int vlo = 255, vhi = 0, j0, j1, i0, i1, j;
    float oix, oiy, ofx, ofy, sc;
    if (!nr)
        return;
    memset(by_y, 0, sizeof by_y);
    for (k = 0; k < nr; k++) {
        const FxSpan *r = &rows[k];
        by_y[r->y] = r;
        if (r->y < ymin) ymin = r->y;
        if (r->y > ymax) ymax = r->y;
        if (r->x < xmin) xmin = r->x;
        if (r->x + r->n > xmax) xmax = r->x + r->n;
        if ((r->tv >> 8) < vlo) vlo = r->tv >> 8;
        if ((r->tv >> 8) > vhi) vhi = r->tv >> 8;
    }
    half = (o->si + 1) / 2;
    oix = (float)(o->xi - half);
    oiy = (float)(o->yi - half);
    if (exact) {
        ofx = o->xf - o->sf / 2;
        ofy = o->yf - o->sf / 2;
        sc = o->si / o->sf;
    } else {
        ofx = oix;
        ofy = oiy;
        sc = 1;
    }
    /* the sprite's rows' extent, taken to the screen */
    j0 = (int)floorf((ofy + (ymin - oiy) / sc) * N);
    j1 = (int)ceilf((ofy + (ymax + 1 - oiy) / sc) * N);
    i0 = (int)floorf((ofx + (xmin - oix) / sc) * N);
    i1 = (int)ceilf((ofx + (xmax - oix) / sc) * N);
    if (j0 < b0) j0 = b0;
    if (i0 < 0) i0 = 0;
    if (j1 > b1) j1 = b1;
    if (i1 > SW) i1 = SW;
    for (j = j0; j < j1; j++) {
        float qy = oiy + ((j + 0.5f) / N - ofy) * sc, fy;
        int ry = (int)floorf(qy), i, dvrow = 0;
        const FxSpan *r;
        uint32_t *row = samp + (size_t)j * SW;
        if (ry < 0 || ry > 255 || !(r = by_y[ry]))
            continue;
        fy = exact ? qy - ry : 0;
        if (r->kind == 1) {
            if (ry < 255 && by_y[ry + 1])
                dvrow = s16((uint16_t)(by_y[ry + 1]->tv - r->tv));
            else if (ry > 0 && by_y[ry - 1])
                dvrow = s16((uint16_t)(r->tv - by_y[ry - 1]->tv));
        }
        for (i = i0; i < i1; i++) {
            float qx = oix + ((i + 0.5f) / N - ofx) * sc, kk = qx - r->x;
            int c;
            if (kk < 0 || kk >= r->n)
                continue;
            c = span_texel(r, exact ? kk : floorf(kk), fy, dvrow, vlo, vhi);
            if (c >= 0)
                row[i] = lut[c];
        }
    }
}

/* a band of output rows [y0, y1): its samples cleared, every shape drawn
   in them, averaged */
typedef struct { const HrList *l; int s, a, cw, y0, y1; uint32_t *argb; } Band;

static void *band(void *arg)
{
    const Band *bd = (const Band *)arg;
    const HrList *l = bd->l;
    int s = bd->s, a = bd->a, cw = bd->cw, k, x, y, b0 = bd->y0 * a, b1 = bd->y1 * a;
    memset(samp + (size_t)b0 * SW, 0, (size_t)(b1 - b0) * SW * 4);
    for (k = 0; k < l->nops; k++) {
        const HrOp *o = &l->ops[k];
        if (o->kind == HR_SPRITE || o->kind == HR_ROWS)
            sprite(l, o, b0, b1);
        else
            poly(o, b0, b1);
    }
    /* the samples averaged: colour of what was drawn, coverage in alpha */
    for (y = bd->y0; y < bd->y1; y++) {
        uint32_t *o = bd->argb + (size_t)y * cw * s;
        const uint32_t *base = samp + (size_t)y * a * SW;
        if (a == 1) {
            memcpy(o, base, (size_t)SW * 4);
            continue;
        }
        for (x = 0; x < cw * s; x++) {
            unsigned rs = 0, gs = 0, bs = 0, n = 0, yy, xx;
            const uint32_t *p = base + (size_t)x * a;
            if (a == 2 && p[0] && p[0] == p[1] && p[0] == p[SW] && p[0] == p[SW + 1]) {
                o[x] = p[0];            /* all four the same: the common case */
                continue;
            }
            for (yy = 0; yy < (unsigned)a; yy++, p += SW)
                for (xx = 0; xx < (unsigned)a; xx++)
                    if (p[xx]) {
                        rs += p[xx] >> 16 & 0xFF;
                        gs += p[xx] >> 8 & 0xFF;
                        bs += p[xx] & 0xFF;
                        n++;
                    }
            o[x] = n ? (n * 255u / (unsigned)(a * a)) << 24 | (rs / n) << 16 | (gs / n) << 8 | bs / n : 0;
        }
    }
    return NULL;
}

void hires_render(const HrList *l, const uint8_t *rom, uint32_t rom_size, const uint16_t *palette, int bright,
                  int s, int a, int cw, int ch, uint32_t *argb)
{
    enum { MAXT = 16 };
    static int threads;
    size_t need;
    int k, t, rows = ch * s;
    pthread_t th[MAXT];
    Band bd[MAXT];
    if (!threads) {
        const char *e = getenv("PORT_HIRES_THREADS");
        threads = e ? atoi(e) : 4;
        if (threads < 1) threads = 1;
        if (threads > MAXT) threads = MAXT;
    }
    N = s * a;
    SW = cw * N;
    SH = ch * N;
    need = (size_t)SW * SH;
    if (need > samp_size) {
        free(samp);
        samp = (uint32_t *)malloc(need * 4);
        samp_size = need;
    }
    rom_p = rom;
    rom_n = rom_size;
    for (k = 0; k < 256; k++) {
        unsigned c = palette[k], r = c & 31, g = c >> 5 & 31, b = c >> 10 & 31;
        if (bright != 15) {
            r = r * (bright + 1u) / 16u;
            g = g * (bright + 1u) / 16u;
            b = b * (bright + 1u) / 16u;
        }
        lut[k] = 0xFF000000u | (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
    }
    for (t = 0; t < threads; t++) {
        bd[t].l = l;
        bd[t].s = s;
        bd[t].a = a;
        bd[t].cw = cw;
        bd[t].y0 = rows * t / threads;
        bd[t].y1 = rows * (t + 1) / threads;
        bd[t].argb = argb;
    }
    for (t = 1; t < threads; t++)
        if (pthread_create(&th[t], NULL, band, &bd[t]))
            band(&bd[t]), th[t] = 0;
    band(&bd[0]);
    for (t = 1; t < threads; t++)
        if (th[t])
            pthread_join(th[t], NULL);
}
