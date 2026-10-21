/*
 * video.c: the picture, native: the game's screen state (its modes, layers,
 * scroll, windows, colour maths, brightness), its tiles and maps, palette
 * and sprite table, drawn a line at a time into a 256 x 224 picture.
 *
 * The game sets the state with whole values (vid_reg: a register number of
 * the console's picture unit, $2100 + reg, and its value) and fills the
 * tiles, palette and sprites through the data calls. vid_render_line draws
 * one line from the state at that moment (the port's clock calls it as the
 * line's horizontal blank comes, after the line's changes and before its
 * per-line tables):
 *
 *   backgrounds  modes 0-7: 2, 4 and 8 bits a pixel, 8x8 and 16x16 tiles,
 *                maps of 32 or 64 tiles each way, mosaic, offset-per-tile
 *                (modes 2, 4, 6), direct colour, Mode 7 with its wrap modes
 *                and EXTBG; modes 5 and 6 drawn at half their width
 *   sprites      the 32-sprite and 34-tile limits of a line (and their
 *                flags), every size table, the name gap, priority rotation
 *   windows      two per layer with their logic, on main and sub screens
 *   colour maths add or subtract the sub screen or the fixed colour, halved,
 *                the colour window's clip and prevent regions
 *   brightness   and forced blank
 */
#include <stdlib.h>
#include <string.h>

#include "video.h"

static uint16_t vram[0x8000];
static uint16_t cgram[256];
static uint8_t oam[544];
static uint32_t picture[VID_WIDTH * VID_HEIGHT];
static uint32_t shown[VID_WIDTH * VID_HEIGHT];


static VidScreen cur = { .inidisp = 0x80 };   /* the state as the game sets it */
static const VidScreen *s = &cur;             /* the state being drawn */
/* the frames' records: one being made, the last finished */
static VidFrame frames[2];
static VidFrame *making = &frames[0], *finished = &frames[1];
static int dirty = 1;                           /* the tiles or sprites changed since the last copy */
unsigned long vid_version_overflow;             /* frames that needed more copies than VID_VERSIONS */
/* what the drawing reads: the live memory, or a record's */
static const uint16_t *d_vram = vram, *d_cgram = cgram;
static const uint8_t *d_oam = oam;

/* the data ports: where the game's writes of tiles, palette and sprites go */
static uint8_t bgofs_latch, bghofs_latch;
static uint8_t m7_latch;
static uint8_t m7b_last;
static uint8_t vmain;
static uint16_t vmaddr, vram_prefetch;
static uint8_t cgaddr, cg_flip, cg_latch;
static uint16_t oam_at;
static uint8_t oam_latch;
static uint8_t range_over, time_over;
static unsigned mosaic_y;

const uint32_t *vid_picture(void) { return shown; }
int vid_forced_blank(void) { return (cur.inidisp & 0x80) != 0; }

void vid_reset(void)
{
    memset(vram, 0, sizeof vram);
    memset(cgram, 0, sizeof cgram);
    memset(oam, 0, sizeof oam);
    memset(picture, 0, sizeof picture);
    memset(shown, 0, sizeof shown);
    cur.inidisp = 0x80;
    cur.obsel = cur.bgmode = cur.mosaic = cur.m7sel = 0;
    memset(cur.bgsc, 0, sizeof cur.bgsc);
    memset(cur.bgnba, 0, sizeof cur.bgnba);
    cur.w12sel = cur.w34sel = cur.wobjsel = cur.wbglog = cur.wobjlog = cur.tm = cur.ts = cur.tmw = cur.tsw = cur.cgwsel = cur.cgadsub = cur.setini = 0;
    memset(cur.wh, 0, sizeof cur.wh);
    memset(cur.bghofs, 0, sizeof cur.bghofs);
    memset(cur.bgvofs, 0, sizeof cur.bgvofs);
    bgofs_latch = bghofs_latch = m7_latch = 0;
    cur.m7hofs = cur.m7vofs = 0;
    cur.m7a = cur.m7b = cur.m7c = cur.m7d = cur.m7x = cur.m7y = 0;
    vmain = 0;
    vmaddr = vram_prefetch = 0;
    cgaddr = cg_flip = cg_latch = 0;
    cur.oamaddr = oam_at = 0;
    cur.oam_prio = oam_latch = 0;
    cur.fixed_r = cur.fixed_g = cur.fixed_b = 0;
    range_over = time_over = 0;
}

/* ---- registers --------------------------------------------------------------------- */
static uint16_t vram_remap(uint16_t a)
{
    switch ((vmain >> 2) & 3) {
    case 1: return (uint16_t)((a & 0xFF00) | (a & 0x00E0) >> 5 | (a & 0x001F) << 3);
    case 2: return (uint16_t)((a & 0xFE00) | (a & 0x01C0) >> 6 | (a & 0x003F) << 3);
    case 3: return (uint16_t)((a & 0xFC00) | (a & 0x0380) >> 7 | (a & 0x007F) << 3);
    }
    return a;
}

static void vram_step(void)
{
    static const uint16_t inc[4] = { 1, 32, 128, 128 };
    vmaddr = (uint16_t)(vmaddr + inc[vmain & 3]);
}

void vid_reg(uint8_t reg, uint8_t v)
{
    switch (reg) {
    case 0x00:
        /* forced blank lifted in the vertical blank: OAM's address reloads */
        cur.inidisp = v;
        return;
    case 0x01: cur.obsel = v; return;
    case 0x02:
        cur.oamaddr = (uint16_t)((cur.oamaddr & 0x100) | v);
        oam_at = (uint16_t)(cur.oamaddr << 1);
        return;
    case 0x03:
        cur.oamaddr = (uint16_t)((cur.oamaddr & 0xFF) | (v & 1) << 8);
        cur.oam_prio = v & 0x80;
        oam_at = (uint16_t)(cur.oamaddr << 1);
        return;
    case 0x04: {
        dirty = 1;
        uint16_t a = oam_at & 0x3FF;
        if (a < 0x200) {
            if (!(a & 1))
                oam_latch = v;
            else {
                oam[a - 1] = oam_latch;
                oam[a] = v;
            }
        } else {
            oam[0x200 | (a & 0x1F)] = v;
        }
        oam_at = (uint16_t)((oam_at + 1) & 0x3FF);
        return;
    }
    case 0x05: cur.bgmode = v; return;
    case 0x06: cur.mosaic = v; return;
    case 0x07: case 0x08: case 0x09: case 0x0A: cur.bgsc[reg - 7] = v; return;
    case 0x0B: cur.bgnba[0] = v; return;
    case 0x0C: cur.bgnba[1] = v; return;
    case 0x0D: case 0x0F: case 0x11: case 0x13: {
        int n = (reg - 0x0D) >> 1;
        cur.bghofs[n] = (uint16_t)(((v << 8) | (bgofs_latch & ~7) | (bghofs_latch & 7)) & 0x3FF);
        bgofs_latch = v;
        bghofs_latch = v;
        if (reg == 0x0D) {
            cur.m7hofs = (uint16_t)(((v << 8) | m7_latch) & 0x1FFF);
            m7_latch = v;
        }
        return;
    }
    case 0x0E: case 0x10: case 0x12: case 0x14: {
        int n = (reg - 0x0E) >> 1;
        cur.bgvofs[n] = (uint16_t)(((v << 8) | bgofs_latch) & 0x3FF);
        bgofs_latch = v;
        if (reg == 0x0E) {
            cur.m7vofs = (uint16_t)(((v << 8) | m7_latch) & 0x1FFF);
            m7_latch = v;
        }
        return;
    }
    case 0x15: vmain = v; return;
    case 0x16:
        vmaddr = (uint16_t)((vmaddr & 0xFF00) | v);
        vram_prefetch = vram[vram_remap(vmaddr) & 0x7FFF];
        return;
    case 0x17:
        vmaddr = (uint16_t)((vmaddr & 0x00FF) | v << 8);
        vram_prefetch = vram[vram_remap(vmaddr) & 0x7FFF];
        return;
    case 0x18: {
        uint16_t a = vram_remap(vmaddr) & 0x7FFF;
        dirty = 1;
        vram[a] = (uint16_t)((vram[a] & 0xFF00) | v);
        if (!(vmain & 0x80))
            vram_step();
        return;
    }
    case 0x19: {
        uint16_t a = vram_remap(vmaddr) & 0x7FFF;
        dirty = 1;
        vram[a] = (uint16_t)((vram[a] & 0x00FF) | v << 8);
        if (vmain & 0x80)
            vram_step();
        return;
    }
    case 0x1A: cur.m7sel = v; return;
    case 0x1B: cur.m7a = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x1C: cur.m7b = (int16_t)(v << 8 | m7_latch); m7_latch = v; m7b_last = v; return;
    case 0x1D: cur.m7c = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x1E: cur.m7d = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x1F: cur.m7x = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x20: cur.m7y = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x21: cgaddr = v; cg_flip = 0; return;
    case 0x22:
        if (!cg_flip) {
            cg_latch = v;
            cg_flip = 1;
        } else {
            cgram[cgaddr] = (uint16_t)((v & 0x7F) << 8 | cg_latch);
            cgaddr++;
            cg_flip = 0;
        }
        return;
    case 0x23: cur.w12sel = v; return;
    case 0x24: cur.w34sel = v; return;
    case 0x25: cur.wobjsel = v; return;
    case 0x26: case 0x27: case 0x28: case 0x29: cur.wh[reg - 0x26] = v; return;
    case 0x2A: cur.wbglog = v; return;
    case 0x2B: cur.wobjlog = v; return;
    case 0x2C: cur.tm = v; return;
    case 0x2D: cur.ts = v; return;
    case 0x2E: cur.tmw = v; return;
    case 0x2F: cur.tsw = v; return;
    case 0x30: cur.cgwsel = v; return;
    case 0x31: cur.cgadsub = v; return;
    case 0x32:
        if (v & 0x20) cur.fixed_r = v & 0x1F;
        if (v & 0x40) cur.fixed_g = v & 0x1F;
        if (v & 0x80) cur.fixed_b = v & 0x1F;
        return;
    case 0x33: cur.setini = v; return;
    }
}

void vid_frame_start(void)
{
    mosaic_y = 1;
    range_over = time_over = 0;
}

/* PORT_VIDCHECK=1: each finished frame drawn again from its record and
   compared with the picture drawn a line at a time */
unsigned long vid_check_frames, vid_check_bad;
static uint32_t check_pic[VID_WIDTH * VID_HEIGHT];

void vid_vblank_start(void)
{
    static int check = -1;
    VidFrame *t = finished;
    finished = making;
    making = t;
    if (check < 0)
        check = getenv("PORT_VIDCHECK") != NULL;
    if (check) {
        uint8_t ro = range_over, to = time_over;
        vid_draw_frame(finished, check_pic);
        range_over = ro;
        time_over = to;
        vid_check_frames++;
        if (memcmp(check_pic, picture, sizeof picture))
            vid_check_bad++;
    }
    if (!(cur.inidisp & 0x80))
        oam_at = (uint16_t)(cur.oamaddr << 1);
    memcpy(shown, picture, sizeof shown);
}

/* ---- drawing ---------------------------------------------------------------------- */
/* What one screen of one line holds at each pixel: a colour (15-bit BGR),
   its depth (higher in front; 0 the backdrop) and the layer it came from
   (0-3 BG1-4, 4 sprites with palettes 4-7, 5 the backdrop, 6 sprites with
   palettes 0-3, which colour maths never takes). */
typedef struct {
    uint16_t color;
    uint8_t z, layer;
} Pixel;

static Pixel main_px[256], sub_px[256];

/* a layer's pixels for one line: colour, palette-0 (transparent) and
   priority */
typedef struct {
    uint16_t color;
    uint8_t opaque, prio;
} LayerPx;

static LayerPx lp[256];

static uint16_t direct_color(uint8_t c, uint8_t pal)
{
    unsigned r = (c & 7u) << 2 | (pal & 1u) << 1;
    unsigned g = (c & 0x38u) >> 1 | (pal & 2u);
    unsigned b = (c & 0xC0u) >> 3 | (pal & 4u);
    return (uint16_t)(r | g << 5 | b << 10);
}

/* bits per pixel of each background in each mode (0: none) */
static const uint8_t mode_bpp[8][4] = {
    { 2, 2, 2, 2 }, { 4, 4, 2, 0 }, { 4, 4, 0, 0 }, { 8, 4, 0, 0 },
    { 8, 2, 0, 0 }, { 4, 2, 0, 0 }, { 4, 0, 0, 0 }, { 8, 0, 0, 0 },
};

static uint16_t map_entry(int n, unsigned tx, unsigned ty)
{
    uint8_t sc = s->bgsc[n];
    uint16_t base = (uint16_t)((sc & 0xFC) << 8);
    uint16_t off = (uint16_t)((ty & 31) << 5 | (tx & 31));
    if ((tx & 32) && (sc & 1))
        off += 0x400;
    if ((ty & 32) && (sc & 2))
        off += (sc & 1) ? 0x800 : 0x400;
    return d_vram[(base + off) & 0x7FFF];
}

/* one pixel's colour index (0 transparent) from a tile row: char address
   `ca` (words) of the 8x8 tile, row y, column x (already flipped) */
static uint8_t tile_pixel(uint16_t ca, unsigned y, unsigned x, unsigned bpp)
{
    unsigned bit = 7 - x, c = 0, p;
    for (p = 0; p < bpp; p += 2) {
        uint16_t w = d_vram[(ca + p * 4 + y) & 0x7FFF];
        c |= ((w >> bit) & 1u) << p;
        c |= ((w >> (bit + 8)) & 1u) << (p + 1);
    }
    return (uint8_t)c;
}

static void draw_bg(int n, unsigned line, unsigned mode)
{
    unsigned bpp = mode_bpp[mode][n], big = (s->bgmode >> (4 + n)) & 1;
    unsigned tshift = big ? 4 : 3, x;
    unsigned msize = (s->mosaic & (1u << n)) ? ((s->mosaic >> 4) + 1u) : 1u;
    unsigned y = msize > 1 ? mosaic_y : line;
    uint16_t nba = (uint16_t)(((n < 2 ? s->bgnba[0] : s->bgnba[1]) >> ((n & 1) * 4) & 0x0F) << 12);
    int opt = mode == 2 || mode == 4 || mode == 6;
    int hires = mode == 5 || mode == 6;
    for (x = 0; x < 256; x++) {
        unsigned sx = msize > 1 ? x - x % msize : x;
        unsigned hofs = s->bghofs[n], vofs = s->bgvofs[n];
        unsigned xx, yy, tx, ty, px, py, c;
        uint16_t e, ch, ca;
        uint8_t pal;
        if (hires)
            sx <<= 1;
        if (opt && n < 2) {
            unsigned col = (sx + (hofs & 7)) >> 3;
            if (col > 0) {
                unsigned bx = ((col - 1) * 8 + (s->bghofs[2] & ~7u)) >> 3;
                unsigned by = s->bgvofs[2] >> 3;
                uint16_t hv = map_entry(2, bx, by), vv;
                if (mode == 4) {
                    if (hv & 0x8000) {
                        if (hv & (0x2000u << n))
                            vofs = hv & 0x3FF;
                    } else if (hv & (0x2000u << n)) {
                        hofs = (hofs & 7) | (hv & 0x3F8);
                    }
                } else {
                    vv = map_entry(2, bx, by + 1);
                    if (hv & (0x2000u << n))
                        hofs = (hofs & 7) | (hv & 0x3F8);
                    if (vv & (0x2000u << n))
                        vofs = vv & 0x3FF;
                }
            }
        }
        xx = (sx + hofs) & 0x3FF;
        yy = (y + vofs) & 0x3FF;
        tx = xx >> tshift;
        ty = yy >> tshift;
        e = map_entry(n, tx, ty);
        px = xx & ((1u << tshift) - 1);
        py = yy & ((1u << tshift) - 1);
        if (e & 0x4000)
            px = (1u << tshift) - 1 - px;
        if (e & 0x8000)
            py = (1u << tshift) - 1 - py;
        ch = e & 0x3FF;
        if (big) {
            if (px >= 8) ch += 1;
            if (py >= 8) ch += 16;
        }
        ca = (uint16_t)(nba + (ch & 0x3FF) * (bpp * 4));
        c = tile_pixel(ca, py & 7, px & 7, bpp);
        pal = (uint8_t)((e >> 10) & 7);
        lp[x].opaque = c != 0;
        lp[x].prio = (uint8_t)((e >> 13) & 1);
        if (!c)
            continue;
        if (bpp == 8)
            lp[x].color = (s->cgwsel & 1) ? direct_color((uint8_t)c, pal) : d_cgram[c];
        else if (bpp == 4)
            lp[x].color = d_cgram[(pal * 16 + c) & 0xFF];
        else
            lp[x].color = d_cgram[((mode == 0 ? n * 32 : 0) + pal * 4 + c) & 0xFF];
    }
}

static int sext13(int v) { return (v & 0x1000) ? v | ~0x1FFF : v & 0x1FFF; }
static int clip10(int v) { return (v & 0x2000) ? (v | ~0x3FF) : (v & 0x3FF); }

/* Mode 7: BG1 (or BG2, EXTBG, with bit 7 as priority) through the matrix */
static void draw_m7(int n, unsigned line)
{
    int a = s->m7a, b = s->m7b, c = s->m7c, d = s->m7d;
    int cx = sext13(s->m7x), cy = sext13(s->m7y);
    int hofs = sext13(s->m7hofs), vofs = sext13(s->m7vofs);
    unsigned msize = (s->mosaic & (1u << n)) ? ((s->mosaic >> 4) + 1u) : 1u;
    int y = (int)(msize > 1 ? mosaic_y : line);
    int x0, y0, ox, oy, px, py;
    unsigned x;
    if (s->m7sel & 2)
        y = 255 - y;
    x0 = clip10(hofs - cx);
    y0 = clip10(vofs - cy);
    ox = ((a * x0) & ~63) + ((b * y) & ~63) + ((b * y0) & ~63) + (cx << 8);
    oy = ((c * x0) & ~63) + ((d * y) & ~63) + ((d * y0) & ~63) + (cy << 8);
    for (x = 0; x < 256; x++) {
        int sx = (int)(msize > 1 ? x - x % msize : x);
        int tx, ty;
        uint8_t tile, pix;
        if (s->m7sel & 1)
            sx = 255 - sx;
        px = (ox + a * sx) >> 8;
        py = (oy + c * sx) >> 8;
        if ((px | py) & ~0x3FF) {
            if ((s->m7sel & 0xC0) == 0x80) {
                lp[x].opaque = 0;
                continue;
            }
            if ((s->m7sel & 0xC0) == 0xC0)
                tile = 0;
            else
                tile = (uint8_t)d_vram[((py >> 3 & 127) << 7 | (px >> 3 & 127)) & 0x7FFF];
        } else {
            tile = (uint8_t)d_vram[((py >> 3) << 7 | (px >> 3)) & 0x7FFF];
        }
        tx = px & 7;
        ty = py & 7;
        pix = (uint8_t)(d_vram[(tile * 64 + ty * 8 + tx) & 0x7FFF] >> 8);
        if (n == 1) {
            lp[x].prio = pix >> 7;
            pix &= 0x7F;
        } else {
            lp[x].prio = 0;
        }
        lp[x].opaque = pix != 0;
        if (pix)
            lp[x].color = (s->cgwsel & 1) && n == 0 ? direct_color(pix, 0) : d_cgram[pix];
    }
}

/* ---- sprites ---- */
static const uint8_t obj_w[8][2] = { { 8, 16 }, { 8, 32 }, { 8, 64 }, { 16, 32 }, { 16, 64 }, { 32, 64 }, { 16, 32 }, { 16, 32 } };
static const uint8_t obj_h[8][2] = { { 8, 16 }, { 8, 32 }, { 8, 64 }, { 16, 32 }, { 16, 64 }, { 32, 64 }, { 32, 64 }, { 32, 32 } };

typedef struct {
    uint16_t color;
    uint8_t opaque, prio, math;
} ObjPx;

static ObjPx op[256];

static void draw_obj(unsigned line)
{
    unsigned first = s->oam_prio ? (s->oamaddr >> 1) & 0x7F : 0;
    unsigned i, n = 0, tiles = 0, k;
    unsigned list[32];
    unsigned row = (line - 1) & 0xFF;
    uint16_t base = (uint16_t)((s->obsel & 7) << 13);
    uint16_t gap = (uint16_t)((((s->obsel >> 3) & 3) + 1) << 12);
    unsigned sz = s->obsel >> 5;
    memset(op, 0, sizeof op);
    for (i = 0; i < 128; i++) {
        unsigned s = (first + i) & 0x7F;
        uint8_t hi = (uint8_t)(d_oam[0x200 + (s >> 2)] >> ((s & 3) * 2));
        unsigned big = (hi >> 1) & 1, h = obj_h[sz][big], w = obj_w[sz][big];
        int x = d_oam[s * 4] | ((hi & 1) << 8);
        unsigned y = d_oam[s * 4 + 1];
        if (x >= 256)
            x -= 512;
        if (((row - y) & 0xFF) >= h)
            continue;
        if (x <= -(int)w)
            continue;
        if (n == 32) {
            range_over = 1;
            break;
        }
        list[n++] = s;
    }
    /* the line's tiles, the sprite last in the list first (the hardware's
       order), up to 34; then drawn so the first in the list ends on top */
    {
        int keep[32];
        for (k = 0; k < n; k++)
            keep[k] = 0;
        for (k = n; k-- > 0;) {
            unsigned s = list[k];
            uint8_t hi = (uint8_t)(d_oam[0x200 + (s >> 2)] >> ((s & 3) * 2));
            unsigned big = (hi >> 1) & 1, w = obj_w[sz][big];
            int x = d_oam[s * 4] | ((hi & 1) << 8);
            unsigned c, cols = w >> 3, on = 0;
            if (x >= 256)
                x -= 512;
            for (c = 0; c < cols; c++) {
                int tx = x + (int)c * 8;
                if (tx > -8 && tx < 256)
                    on++;
            }
            if (tiles + on > 34) {
                time_over = 1;
                keep[k] = (int)(34 - tiles);
                tiles = 34;
                for (; k-- > 0;)
                    keep[k] = -1;
                break;
            }
            tiles += on;
            keep[k] = (int)on;
        }
        for (k = n; k-- > 0;) {
            unsigned s = list[k];
            uint8_t hi = (uint8_t)(d_oam[0x200 + (s >> 2)] >> ((s & 3) * 2));
            unsigned big = (hi >> 1) & 1, h = obj_h[sz][big], w = obj_w[sz][big];
            int x = d_oam[s * 4] | ((hi & 1) << 8);
            unsigned y = d_oam[s * 4 + 1];
            uint16_t name = (uint16_t)(d_oam[s * 4 + 2] | (d_oam[s * 4 + 3] & 1) << 8);
            uint8_t attr = d_oam[s * 4 + 3];
            unsigned pal = (attr >> 1) & 7, prio = (attr >> 4) & 3;
            unsigned r = (row - y) & 0xFF, c, cols = w >> 3, drawn = 0;
            if (keep[k] < 0)
                continue;
            if (x >= 256)
                x -= 512;
            if (attr & 0x80)
                r = h - 1 - r;
            for (c = 0; c < cols; c++) {
                unsigned col = (attr & 0x40) ? cols - 1 - c : c;
                int tx = x + (int)c * 8;
                unsigned t, px;
                uint16_t ca;
                if (!(tx > -8 && tx < 256))
                    continue;
                if ((int)drawn >= keep[k])
                    break;
                drawn++;
                t = (unsigned)(((name & 0xF0) + ((r >> 3) << 4)) & 0xF0) | ((name + col) & 0x0F);
                ca = (uint16_t)(base + t * 16 + ((name & 0x100) ? gap : 0));
                for (px = 0; px < 8; px++) {
                    int sx = tx + (int)px;
                    unsigned ix = (attr & 0x40) ? 7 - px : px;
                    uint8_t ci;
                    if (sx < 0 || sx >= 256)
                        continue;
                    ci = tile_pixel(ca, r & 7, ix, 4);
                    if (!ci)
                        continue;
                    op[sx].opaque = 1;
                    op[sx].prio = (uint8_t)prio;
                    op[sx].color = d_cgram[128 + pal * 16 + ci];
                    op[sx].math = pal >= 4;
                }
            }
        }
    }
}

/* ---- windows ---- */
static uint8_t win_mask(uint8_t sel, uint8_t logic, unsigned x)
{
    int w1 = x >= s->wh[0] && x <= s->wh[1], w2 = x >= s->wh[2] && x <= s->wh[3];
    int e1 = sel & 2, e2 = sel & 8;
    if (!e1 && !e2)
        return 0;
    if (sel & 1) w1 = !w1;
    if (sel & 4) w2 = !w2;
    if (e1 && !e2) return (uint8_t)w1;
    if (e2 && !e1) return (uint8_t)w2;
    switch (logic & 3) {
    case 0: return (uint8_t)(w1 | w2);
    case 1: return (uint8_t)(w1 & w2);
    case 2: return (uint8_t)(w1 ^ w2);
    }
    return (uint8_t)!(w1 ^ w2);
}

static uint8_t layer_sel(int layer)
{
    switch (layer) {
    case 0: return s->w12sel & 0x0F;
    case 1: return s->w12sel >> 4;
    case 2: return s->w34sel & 0x0F;
    case 3: return s->w34sel >> 4;
    case 4: return s->wobjsel & 0x0F;
    }
    return s->wobjsel >> 4;
}

static uint8_t layer_logic(int layer)
{
    if (layer < 4)
        return (uint8_t)(s->wbglog >> (layer * 2));
    if (layer == 4)
        return s->wobjlog;
    return (uint8_t)(s->wobjlog >> 2);
}

/* put a layer's line onto the main and sub screens at its depths */
static void put_layer(int layer, const uint8_t z[2])
{
    unsigned x;
    int on_main = (s->tm >> layer) & 1, on_sub = (s->ts >> layer) & 1;
    uint8_t sel = layer_sel(layer), logic = layer_logic(layer);
    for (x = 0; x < 256; x++) {
        uint8_t zz;
        int inside;
        if (!lp[x].opaque)
            continue;
        zz = z[lp[x].prio];
        if (!zz)
            continue;
        inside = ((s->tmw | s->tsw) >> layer & 1) ? win_mask(sel, logic, x) : 0;
        if (on_main && zz > main_px[x].z && !(inside && (s->tmw >> layer & 1))) {
            main_px[x].color = lp[x].color;
            main_px[x].z = zz;
            main_px[x].layer = (uint8_t)layer;
        }
        if (on_sub && zz > sub_px[x].z && !(inside && (s->tsw >> layer & 1))) {
            sub_px[x].color = lp[x].color;
            sub_px[x].z = zz;
            sub_px[x].layer = (uint8_t)layer;
        }
    }
}

static void put_obj(const uint8_t z[4])
{
    unsigned x;
    int on_main = (s->tm >> 4) & 1, on_sub = (s->ts >> 4) & 1;
    uint8_t sel = layer_sel(4), logic = layer_logic(4);
    for (x = 0; x < 256; x++) {
        uint8_t zz;
        int inside;
        if (!op[x].opaque)
            continue;
        zz = z[op[x].prio];
        inside = ((s->tmw | s->tsw) >> 4 & 1) ? win_mask(sel, logic, x) : 0;
        if (on_main && zz > main_px[x].z && !(inside && (s->tmw >> 4 & 1))) {
            main_px[x].color = op[x].color;
            main_px[x].z = zz;
            main_px[x].layer = op[x].math ? 4 : 6;
        }
        if (on_sub && zz > sub_px[x].z && !(inside && (s->tsw >> 4 & 1))) {
            sub_px[x].color = op[x].color;
            sub_px[x].z = zz;
            sub_px[x].layer = op[x].math ? 4 : 6;
        }
    }
}

static int region(unsigned mode, int inside)
{
    switch (mode & 3) {
    case 0: return 0;
    case 1: return !inside;
    case 2: return inside;
    }
    return 1;
}

/* this line into the frame's record: its screen state and palette, and
   which copy of the tiles and sprites it shows (a new copy when they have
   changed since the last line shown; the game changes them only while
   the screen is blanked) */
static void record_line(unsigned line)
{
    if (line == 1) {
        making->versions = 0;
        dirty = 1;
    }
    if (dirty && (making->versions == 0 || !(cur.inidisp & 0x80))) {
        if (making->versions < VID_VERSIONS) {
            memcpy(making->vram[making->versions], vram, sizeof vram);
            memcpy(making->oam[making->versions], oam, sizeof oam);
            making->versions++;
        } else {
            vid_version_overflow++;
        }
        dirty = 0;
    }
    making->line[line] = cur;
    memcpy(making->palette[line], cgram, sizeof cgram);
    making->version[line] = (uint8_t)(making->versions - 1);
}

static void draw_line(unsigned line, uint32_t *out);

void vid_render_line(unsigned line)
{
    record_line(line);
    s = &making->line[line];
    d_vram = vram;
    d_cgram = cgram;
    d_oam = oam;
    draw_line(line, &picture[(line - 1) * VID_WIDTH]);
}

void vid_draw_frame(const VidFrame *f, uint32_t *out)
{
    unsigned line, keep = mosaic_y;
    mosaic_y = 1;
    for (line = 1; line <= VID_HEIGHT; line++) {
        s = &f->line[line];
        d_vram = f->vram[f->version[line]];
        d_cgram = f->palette[line];
        d_oam = f->oam[f->version[line]];
        draw_line(line, out + (line - 1) * VID_WIDTH);
    }
    mosaic_y = keep;
}

const VidFrame *vid_frame(void) { return finished; }

static void draw_line(unsigned line, uint32_t *out)
{
    unsigned mode = s->bgmode & 7, x, n;
    uint8_t bright = s->inidisp & 0x0F;
    uint16_t fixed = (uint16_t)(s->fixed_r | s->fixed_g << 5 | s->fixed_b << 10);
    /* the depths of each layer's two priorities and of the sprites' four */
    static const uint8_t z0[4][2] = { { 8, 11 }, { 7, 10 }, { 2, 5 }, { 1, 4 } }, o0[4] = { 3, 6, 9, 12 };
    static const uint8_t z1[3][2] = { { 6, 9 }, { 5, 8 }, { 1, 3 } }, o1[4] = { 2, 4, 7, 10 };
    static const uint8_t z1p[3][2] = { { 6, 9 }, { 5, 8 }, { 1, 11 } };
    static const uint8_t z2[2][2] = { { 3, 7 }, { 1, 5 } }, o2[4] = { 2, 4, 6, 8 };
    static const uint8_t z7[2][2] = { { 3, 3 }, { 1, 5 } }, o7[4] = { 2, 4, 6, 7 };
    if (mosaic_y == 0)
        mosaic_y = 1;
    if (s->inidisp & 0x80) {
        memset(out, 0, VID_WIDTH * 4);
        goto mosaic_step;
    }
    for (x = 0; x < 256; x++) {
        main_px[x].color = d_cgram[0];
        main_px[x].z = 0;
        main_px[x].layer = 5;
        sub_px[x].color = fixed;
        sub_px[x].z = 0;
        sub_px[x].layer = 5;
    }
    if (mode == 7) {
        if ((s->tm | s->ts) & 1) {
            draw_m7(0, line);
            put_layer(0, z7[0]);
        }
        if ((s->setini & 0x40) && ((s->tm | s->ts) & 2)) {
            draw_m7(1, line);
            put_layer(1, z7[1]);
        }
    } else {
        for (n = 0; n < 4; n++) {
            const uint8_t *z;
            if (!mode_bpp[mode][n] || !((s->tm | s->ts) >> n & 1))
                continue;
            if (mode == 0)
                z = z0[n];
            else if (mode == 1)
                z = (s->bgmode & 0x08) ? z1p[n] : z1[n];
            else
                z = z2[n];
            draw_bg((int)n, line, mode);
            put_layer((int)n, z);
        }
    }
    if ((s->tm | s->ts) & 0x10) {
        draw_obj(line);
        put_obj(mode == 0 ? o0 : mode == 1 ? o1 : mode == 7 ? o7 : o2);
    }
    for (x = 0; x < 256; x++) {
        Pixel *m = &main_px[x], *sp = &sub_px[x];
        int cw = win_mask(layer_sel(5), layer_logic(5), x);
        int clip = region(s->cgwsel >> 6, cw);
        int prevent = region(s->cgwsel >> 4, cw);
        unsigned bit = m->layer == 5 ? 5 : m->layer == 6 ? 8 : m->layer;
        uint16_t c = clip ? 0 : m->color;
        unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        if (bit < 8 && (s->cgadsub >> bit & 1) && !prevent) {
            uint16_t o;
            int halve = (s->cgadsub & 0x40) != 0 && !clip;
            if (s->cgwsel & 2) {
                if (sp->layer == 5) {
                    o = fixed;
                    halve = 0;
                } else {
                    o = sp->color;
                }
            } else {
                o = fixed;
            }
            if (s->cgadsub & 0x80) {
                int rr = (int)r - (o & 31), gg = (int)g - ((o >> 5) & 31), bb = (int)b - ((o >> 10) & 31);
                r = rr < 0 ? 0 : (unsigned)rr;
                g = gg < 0 ? 0 : (unsigned)gg;
                b = bb < 0 ? 0 : (unsigned)bb;
                if (halve) {
                    r >>= 1;
                    g >>= 1;
                    b >>= 1;
                }
            } else {
                r += o & 31;
                g += (o >> 5) & 31;
                b += (o >> 10) & 31;
                if (halve) {
                    r >>= 1;
                    g >>= 1;
                    b >>= 1;
                }
                if (r > 31) r = 31;
                if (g > 31) g = 31;
                if (b > 31) b = 31;
            }
        }
        if (bright != 15) {
            r = r * (bright + 1u) / 16u;
            g = g * (bright + 1u) / 16u;
            b = b * (bright + 1u) / 16u;
        }
        out[x] = 0xFF000000u | (r << 3 | r >> 2) << 16 | (g << 3 | g >> 2) << 8 | (b << 3 | b >> 2);
    }
mosaic_step:
    /* the vertical s->mosaic: the line the next lines repeat moves on every
       `size` lines */
    {
        unsigned msize = (s->mosaic >> 4) + 1u;
        if (msize > 1 && (line % msize) == 0)
            mosaic_y = line + 1;
        else if (msize == 1)
            mosaic_y = line + 1;
    }
}
