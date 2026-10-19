/*
 * ppu.c: the picture processor, drawn a line at a time.
 *
 * The registers ($2100-$213F) behave as the hardware's do: the double-
 * written scroll and Mode 7 registers with their shared latches, VRAM's
 * address increment and remapping and its read prefetch, CGRAM's and OAM's
 * byte pairs, the counters' latch and flip-flops. ppu_render_line draws one
 * line from the state at that moment (the machine calls it as the clock
 * passes the line's horizontal blank, after the line's register writes and
 * before its HDMA), into a 256 x 224 picture:
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
#include <string.h>

#include "snes_internal.h"

static uint16_t vram[0x8000];
static uint16_t cgram[256];
static uint8_t oam[544];
static uint32_t picture[SNES_WIDTH * SNES_HEIGHT];
static uint32_t shown[SNES_WIDTH * SNES_HEIGHT];

static uint8_t inidisp = 0x80, obsel, bgmode, mosaic, bgsc[4], bgnba[2], m7sel;
static uint8_t w12sel, w34sel, wobjsel, wh[4], wbglog, wobjlog, tm, ts, tmw, tsw, cgwsel, cgadsub, setini;
static uint16_t bghofs[4], bgvofs[4];
static uint8_t bgofs_latch, bghofs_latch;
static uint16_t m7hofs, m7vofs;
static uint8_t m7_latch;
static int16_t m7a, m7b, m7c, m7d, m7x, m7y;
static uint8_t m7b_last;
static uint8_t vmain;
static uint16_t vmaddr, vram_prefetch;
static uint8_t cgaddr, cg_flip, cg_latch;
static uint16_t oamaddr, oam_at;
static uint8_t oam_prio, oam_latch;
static uint8_t fixed_r, fixed_g, fixed_b;
static uint8_t ppu1_mdr, ppu2_mdr;
static uint16_t h_latch, v_latch;
static uint8_t h_flip, v_flip, latch_flag;
static uint8_t range_over, time_over;
static unsigned mosaic_y;

const uint32_t *snes_picture(void) { return shown; }
int ppu_forced_blank(void) { return (inidisp & 0x80) != 0; }

void ppu_reset(void)
{
    memset(vram, 0, sizeof vram);
    memset(cgram, 0, sizeof cgram);
    memset(oam, 0, sizeof oam);
    memset(picture, 0, sizeof picture);
    memset(shown, 0, sizeof shown);
    inidisp = 0x80;
    obsel = bgmode = mosaic = m7sel = 0;
    memset(bgsc, 0, sizeof bgsc);
    memset(bgnba, 0, sizeof bgnba);
    w12sel = w34sel = wobjsel = wbglog = wobjlog = tm = ts = tmw = tsw = cgwsel = cgadsub = setini = 0;
    memset(wh, 0, sizeof wh);
    memset(bghofs, 0, sizeof bghofs);
    memset(bgvofs, 0, sizeof bgvofs);
    bgofs_latch = bghofs_latch = m7_latch = 0;
    m7hofs = m7vofs = 0;
    m7a = m7b = m7c = m7d = m7x = m7y = 0;
    vmain = 0;
    vmaddr = vram_prefetch = 0;
    cgaddr = cg_flip = cg_latch = 0;
    oamaddr = oam_at = 0;
    oam_prio = oam_latch = 0;
    fixed_r = fixed_g = fixed_b = 0;
    h_flip = v_flip = latch_flag = 0;
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

void ppu_latch_counters(void)
{
    h_latch = (uint16_t)snes_hcounter();
    v_latch = (uint16_t)snes_vcounter();
    latch_flag = 1;
}

void ppu_write(uint8_t reg, uint8_t v)
{
    switch (reg) {
    case 0x00:
        /* forced blank lifted in the vertical blank: OAM's address reloads */
        inidisp = v;
        return;
    case 0x01: obsel = v; return;
    case 0x02:
        oamaddr = (uint16_t)((oamaddr & 0x100) | v);
        oam_at = (uint16_t)(oamaddr << 1);
        return;
    case 0x03:
        oamaddr = (uint16_t)((oamaddr & 0xFF) | (v & 1) << 8);
        oam_prio = v & 0x80;
        oam_at = (uint16_t)(oamaddr << 1);
        return;
    case 0x04: {
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
    case 0x05: bgmode = v; return;
    case 0x06: mosaic = v; return;
    case 0x07: case 0x08: case 0x09: case 0x0A: bgsc[reg - 7] = v; return;
    case 0x0B: bgnba[0] = v; return;
    case 0x0C: bgnba[1] = v; return;
    case 0x0D: case 0x0F: case 0x11: case 0x13: {
        int n = (reg - 0x0D) >> 1;
        bghofs[n] = (uint16_t)(((v << 8) | (bgofs_latch & ~7) | (bghofs_latch & 7)) & 0x3FF);
        bgofs_latch = v;
        bghofs_latch = v;
        if (reg == 0x0D) {
            m7hofs = (uint16_t)(((v << 8) | m7_latch) & 0x1FFF);
            m7_latch = v;
        }
        return;
    }
    case 0x0E: case 0x10: case 0x12: case 0x14: {
        int n = (reg - 0x0E) >> 1;
        bgvofs[n] = (uint16_t)(((v << 8) | bgofs_latch) & 0x3FF);
        bgofs_latch = v;
        if (reg == 0x0E) {
            m7vofs = (uint16_t)(((v << 8) | m7_latch) & 0x1FFF);
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
        vram[a] = (uint16_t)((vram[a] & 0xFF00) | v);
        if (!(vmain & 0x80))
            vram_step();
        return;
    }
    case 0x19: {
        uint16_t a = vram_remap(vmaddr) & 0x7FFF;
        vram[a] = (uint16_t)((vram[a] & 0x00FF) | v << 8);
        if (vmain & 0x80)
            vram_step();
        return;
    }
    case 0x1A: m7sel = v; return;
    case 0x1B: m7a = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x1C: m7b = (int16_t)(v << 8 | m7_latch); m7_latch = v; m7b_last = v; return;
    case 0x1D: m7c = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x1E: m7d = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x1F: m7x = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
    case 0x20: m7y = (int16_t)(v << 8 | m7_latch); m7_latch = v; return;
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
    case 0x23: w12sel = v; return;
    case 0x24: w34sel = v; return;
    case 0x25: wobjsel = v; return;
    case 0x26: case 0x27: case 0x28: case 0x29: wh[reg - 0x26] = v; return;
    case 0x2A: wbglog = v; return;
    case 0x2B: wobjlog = v; return;
    case 0x2C: tm = v; return;
    case 0x2D: ts = v; return;
    case 0x2E: tmw = v; return;
    case 0x2F: tsw = v; return;
    case 0x30: cgwsel = v; return;
    case 0x31: cgadsub = v; return;
    case 0x32:
        if (v & 0x20) fixed_r = v & 0x1F;
        if (v & 0x40) fixed_g = v & 0x1F;
        if (v & 0x80) fixed_b = v & 0x1F;
        return;
    case 0x33: setini = v; return;
    }
}

uint8_t ppu_read(uint8_t reg)
{
    uint8_t v;
    switch (reg) {
    case 0x34: case 0x35: case 0x36: {
        int32_t p = (int32_t)m7a * (int8_t)m7b_last;
        return ppu1_mdr = (uint8_t)(p >> ((reg - 0x34) * 8));
    }
    case 0x37:
        ppu_latch_counters();
        return snes_mdr;
    case 0x38: {
        uint16_t a = oam_at & 0x3FF;
        v = a < 0x200 ? oam[a] : oam[0x200 | (a & 0x1F)];
        oam_at = (uint16_t)((oam_at + 1) & 0x3FF);
        return ppu1_mdr = v;
    }
    case 0x39:
        v = (uint8_t)vram_prefetch;
        if (!(vmain & 0x80)) {
            vram_prefetch = vram[vram_remap(vmaddr) & 0x7FFF];
            vram_step();
        }
        return ppu1_mdr = v;
    case 0x3A:
        v = (uint8_t)(vram_prefetch >> 8);
        if (vmain & 0x80) {
            vram_prefetch = vram[vram_remap(vmaddr) & 0x7FFF];
            vram_step();
        }
        return ppu1_mdr = v;
    case 0x3B:
        if (!cg_flip)
            v = (uint8_t)cgram[cgaddr];
        else {
            v = (uint8_t)((cgram[cgaddr] >> 8 & 0x7F) | (ppu2_mdr & 0x80));
            cgaddr++;
        }
        cg_flip ^= 1;
        return ppu2_mdr = v;
    case 0x3C:
        v = h_flip ? (uint8_t)((h_latch >> 8 & 1) | (ppu2_mdr & 0xFE)) : (uint8_t)h_latch;
        h_flip ^= 1;
        return ppu2_mdr = v;
    case 0x3D:
        v = v_flip ? (uint8_t)((v_latch >> 8 & 1) | (ppu2_mdr & 0xFE)) : (uint8_t)v_latch;
        v_flip ^= 1;
        return ppu2_mdr = v;
    case 0x3E:
        return ppu1_mdr = (uint8_t)(time_over << 7 | range_over << 6 | (ppu1_mdr & 0x10) | 0x01);
    case 0x3F:
        v = (uint8_t)(latch_flag << 6 | (ppu2_mdr & 0x20) | 0x03);
        h_flip = v_flip = 0;
        latch_flag = 0;
        return ppu2_mdr = v;
    }
    return snes_mdr;
}

void ppu_frame_start(void)
{
    mosaic_y = 1;
    range_over = time_over = 0;
}

void ppu_vblank_start(void)
{
    if (!(inidisp & 0x80))
        oam_at = (uint16_t)(oamaddr << 1);
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
    uint8_t sc = bgsc[n];
    uint16_t base = (uint16_t)((sc & 0xFC) << 8);
    uint16_t off = (uint16_t)((ty & 31) << 5 | (tx & 31));
    if ((tx & 32) && (sc & 1))
        off += 0x400;
    if ((ty & 32) && (sc & 2))
        off += (sc & 1) ? 0x800 : 0x400;
    return vram[(base + off) & 0x7FFF];
}

/* one pixel's colour index (0 transparent) from a tile row: char address
   `ca` (words) of the 8x8 tile, row y, column x (already flipped) */
static uint8_t tile_pixel(uint16_t ca, unsigned y, unsigned x, unsigned bpp)
{
    unsigned bit = 7 - x, c = 0, p;
    for (p = 0; p < bpp; p += 2) {
        uint16_t w = vram[(ca + p * 4 + y) & 0x7FFF];
        c |= ((w >> bit) & 1u) << p;
        c |= ((w >> (bit + 8)) & 1u) << (p + 1);
    }
    return (uint8_t)c;
}

static void draw_bg(int n, unsigned line, unsigned mode)
{
    unsigned bpp = mode_bpp[mode][n], big = (bgmode >> (4 + n)) & 1;
    unsigned tshift = big ? 4 : 3, x;
    unsigned msize = (mosaic & (1u << n)) ? ((mosaic >> 4) + 1u) : 1u;
    unsigned y = msize > 1 ? mosaic_y : line;
    uint16_t nba = (uint16_t)(((n < 2 ? bgnba[0] : bgnba[1]) >> ((n & 1) * 4) & 0x0F) << 12);
    int opt = mode == 2 || mode == 4 || mode == 6;
    int hires = mode == 5 || mode == 6;
    for (x = 0; x < 256; x++) {
        unsigned sx = msize > 1 ? x - x % msize : x;
        unsigned hofs = bghofs[n], vofs = bgvofs[n];
        unsigned xx, yy, tx, ty, px, py, c;
        uint16_t e, ch, ca;
        uint8_t pal;
        if (hires)
            sx <<= 1;
        if (opt && n < 2) {
            unsigned col = (sx + (hofs & 7)) >> 3;
            if (col > 0) {
                unsigned bx = ((col - 1) * 8 + (bghofs[2] & ~7u)) >> 3;
                unsigned by = bgvofs[2] >> 3;
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
            lp[x].color = (cgwsel & 1) ? direct_color((uint8_t)c, pal) : cgram[c];
        else if (bpp == 4)
            lp[x].color = cgram[(pal * 16 + c) & 0xFF];
        else
            lp[x].color = cgram[((mode == 0 ? n * 32 : 0) + pal * 4 + c) & 0xFF];
    }
}

static int sext13(int v) { return (v & 0x1000) ? v | ~0x1FFF : v & 0x1FFF; }
static int clip10(int v) { return (v & 0x2000) ? (v | ~0x3FF) : (v & 0x3FF); }

/* Mode 7: BG1 (or BG2, EXTBG, with bit 7 as priority) through the matrix */
static void draw_m7(int n, unsigned line)
{
    int a = m7a, b = m7b, c = m7c, d = m7d;
    int cx = sext13(m7x), cy = sext13(m7y);
    int hofs = sext13(m7hofs), vofs = sext13(m7vofs);
    unsigned msize = (mosaic & (1u << n)) ? ((mosaic >> 4) + 1u) : 1u;
    int y = (int)(msize > 1 ? mosaic_y : line);
    int x0, y0, ox, oy, px, py;
    unsigned x;
    if (m7sel & 2)
        y = 255 - y;
    x0 = clip10(hofs - cx);
    y0 = clip10(vofs - cy);
    ox = ((a * x0) & ~63) + ((b * y) & ~63) + ((b * y0) & ~63) + (cx << 8);
    oy = ((c * x0) & ~63) + ((d * y) & ~63) + ((d * y0) & ~63) + (cy << 8);
    for (x = 0; x < 256; x++) {
        int sx = (int)(msize > 1 ? x - x % msize : x);
        int tx, ty;
        uint8_t tile, pix;
        if (m7sel & 1)
            sx = 255 - sx;
        px = (ox + a * sx) >> 8;
        py = (oy + c * sx) >> 8;
        if ((px | py) & ~0x3FF) {
            if ((m7sel & 0xC0) == 0x80) {
                lp[x].opaque = 0;
                continue;
            }
            if ((m7sel & 0xC0) == 0xC0)
                tile = 0;
            else
                tile = (uint8_t)vram[((py >> 3 & 127) << 7 | (px >> 3 & 127)) & 0x7FFF];
        } else {
            tile = (uint8_t)vram[((py >> 3) << 7 | (px >> 3)) & 0x7FFF];
        }
        tx = px & 7;
        ty = py & 7;
        pix = (uint8_t)(vram[(tile * 64 + ty * 8 + tx) & 0x7FFF] >> 8);
        if (n == 1) {
            lp[x].prio = pix >> 7;
            pix &= 0x7F;
        } else {
            lp[x].prio = 0;
        }
        lp[x].opaque = pix != 0;
        if (pix)
            lp[x].color = (cgwsel & 1) && n == 0 ? direct_color(pix, 0) : cgram[pix];
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
    unsigned first = oam_prio ? (oamaddr >> 1) & 0x7F : 0;
    unsigned i, n = 0, tiles = 0, k;
    unsigned list[32];
    unsigned row = (line - 1) & 0xFF;
    uint16_t base = (uint16_t)((obsel & 7) << 13);
    uint16_t gap = (uint16_t)((((obsel >> 3) & 3) + 1) << 12);
    unsigned sz = obsel >> 5;
    memset(op, 0, sizeof op);
    for (i = 0; i < 128; i++) {
        unsigned s = (first + i) & 0x7F;
        uint8_t hi = (uint8_t)(oam[0x200 + (s >> 2)] >> ((s & 3) * 2));
        unsigned big = (hi >> 1) & 1, h = obj_h[sz][big], w = obj_w[sz][big];
        int x = oam[s * 4] | ((hi & 1) << 8);
        unsigned y = oam[s * 4 + 1];
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
            uint8_t hi = (uint8_t)(oam[0x200 + (s >> 2)] >> ((s & 3) * 2));
            unsigned big = (hi >> 1) & 1, w = obj_w[sz][big];
            int x = oam[s * 4] | ((hi & 1) << 8);
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
            uint8_t hi = (uint8_t)(oam[0x200 + (s >> 2)] >> ((s & 3) * 2));
            unsigned big = (hi >> 1) & 1, h = obj_h[sz][big], w = obj_w[sz][big];
            int x = oam[s * 4] | ((hi & 1) << 8);
            unsigned y = oam[s * 4 + 1];
            uint16_t name = (uint16_t)(oam[s * 4 + 2] | (oam[s * 4 + 3] & 1) << 8);
            uint8_t attr = oam[s * 4 + 3];
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
                    op[sx].color = cgram[128 + pal * 16 + ci];
                    op[sx].math = pal >= 4;
                }
            }
        }
    }
}

/* ---- windows ---- */
static uint8_t win_mask(uint8_t sel, uint8_t logic, unsigned x)
{
    int w1 = x >= wh[0] && x <= wh[1], w2 = x >= wh[2] && x <= wh[3];
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
    case 0: return w12sel & 0x0F;
    case 1: return w12sel >> 4;
    case 2: return w34sel & 0x0F;
    case 3: return w34sel >> 4;
    case 4: return wobjsel & 0x0F;
    }
    return wobjsel >> 4;
}

static uint8_t layer_logic(int layer)
{
    if (layer < 4)
        return (uint8_t)(wbglog >> (layer * 2));
    if (layer == 4)
        return wobjlog;
    return (uint8_t)(wobjlog >> 2);
}

/* put a layer's line onto the main and sub screens at its depths */
static void put_layer(int layer, const uint8_t z[2])
{
    unsigned x;
    int on_main = (tm >> layer) & 1, on_sub = (ts >> layer) & 1;
    uint8_t sel = layer_sel(layer), logic = layer_logic(layer);
    for (x = 0; x < 256; x++) {
        uint8_t zz;
        int inside;
        if (!lp[x].opaque)
            continue;
        zz = z[lp[x].prio];
        if (!zz)
            continue;
        inside = ((tmw | tsw) >> layer & 1) ? win_mask(sel, logic, x) : 0;
        if (on_main && zz > main_px[x].z && !(inside && (tmw >> layer & 1))) {
            main_px[x].color = lp[x].color;
            main_px[x].z = zz;
            main_px[x].layer = (uint8_t)layer;
        }
        if (on_sub && zz > sub_px[x].z && !(inside && (tsw >> layer & 1))) {
            sub_px[x].color = lp[x].color;
            sub_px[x].z = zz;
            sub_px[x].layer = (uint8_t)layer;
        }
    }
}

static void put_obj(const uint8_t z[4])
{
    unsigned x;
    int on_main = (tm >> 4) & 1, on_sub = (ts >> 4) & 1;
    uint8_t sel = layer_sel(4), logic = layer_logic(4);
    for (x = 0; x < 256; x++) {
        uint8_t zz;
        int inside;
        if (!op[x].opaque)
            continue;
        zz = z[op[x].prio];
        inside = ((tmw | tsw) >> 4 & 1) ? win_mask(sel, logic, x) : 0;
        if (on_main && zz > main_px[x].z && !(inside && (tmw >> 4 & 1))) {
            main_px[x].color = op[x].color;
            main_px[x].z = zz;
            main_px[x].layer = op[x].math ? 4 : 6;
        }
        if (on_sub && zz > sub_px[x].z && !(inside && (tsw >> 4 & 1))) {
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

void ppu_render_line(unsigned line)
{
    uint32_t *out = &picture[(line - 1) * SNES_WIDTH];
    unsigned mode = bgmode & 7, x, n;
    uint8_t bright = inidisp & 0x0F;
    uint16_t fixed = (uint16_t)(fixed_r | fixed_g << 5 | fixed_b << 10);
    /* the depths of each layer's two priorities and of the sprites' four */
    static const uint8_t z0[4][2] = { { 8, 11 }, { 7, 10 }, { 2, 5 }, { 1, 4 } }, o0[4] = { 3, 6, 9, 12 };
    static const uint8_t z1[3][2] = { { 6, 9 }, { 5, 8 }, { 1, 3 } }, o1[4] = { 2, 4, 7, 10 };
    static const uint8_t z1p[3][2] = { { 6, 9 }, { 5, 8 }, { 1, 11 } };
    static const uint8_t z2[2][2] = { { 3, 7 }, { 1, 5 } }, o2[4] = { 2, 4, 6, 8 };
    static const uint8_t z7[2][2] = { { 3, 3 }, { 1, 5 } }, o7[4] = { 2, 4, 6, 7 };
    if (mosaic_y == 0)
        mosaic_y = 1;
    if (inidisp & 0x80) {
        memset(out, 0, SNES_WIDTH * 4);
        goto mosaic_step;
    }
    for (x = 0; x < 256; x++) {
        main_px[x].color = cgram[0];
        main_px[x].z = 0;
        main_px[x].layer = 5;
        sub_px[x].color = fixed;
        sub_px[x].z = 0;
        sub_px[x].layer = 5;
    }
    if (mode == 7) {
        if ((tm | ts) & 1) {
            draw_m7(0, line);
            put_layer(0, z7[0]);
        }
        if ((setini & 0x40) && ((tm | ts) & 2)) {
            draw_m7(1, line);
            put_layer(1, z7[1]);
        }
    } else {
        for (n = 0; n < 4; n++) {
            const uint8_t *z;
            if (!mode_bpp[mode][n] || !((tm | ts) >> n & 1))
                continue;
            if (mode == 0)
                z = z0[n];
            else if (mode == 1)
                z = (bgmode & 0x08) ? z1p[n] : z1[n];
            else
                z = z2[n];
            draw_bg((int)n, line, mode);
            put_layer((int)n, z);
        }
    }
    if ((tm | ts) & 0x10) {
        draw_obj(line);
        put_obj(mode == 0 ? o0 : mode == 1 ? o1 : mode == 7 ? o7 : o2);
    }
    for (x = 0; x < 256; x++) {
        Pixel *m = &main_px[x], *s = &sub_px[x];
        int cw = win_mask(layer_sel(5), layer_logic(5), x);
        int clip = region(cgwsel >> 6, cw);
        int prevent = region(cgwsel >> 4, cw);
        unsigned bit = m->layer == 5 ? 5 : m->layer == 6 ? 8 : m->layer;
        uint16_t c = clip ? 0 : m->color;
        unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
        if (bit < 8 && (cgadsub >> bit & 1) && !prevent) {
            uint16_t o;
            int halve = (cgadsub & 0x40) != 0 && !clip;
            if (cgwsel & 2) {
                if (s->layer == 5) {
                    o = fixed;
                    halve = 0;
                } else {
                    o = s->color;
                }
            } else {
                o = fixed;
            }
            if (cgadsub & 0x80) {
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
    /* the vertical mosaic: the line the next lines repeat moves on every
       `size` lines */
    {
        unsigned msize = (mosaic >> 4) + 1u;
        if (msize > 1 && (line % msize) == 0)
            mosaic_y = line + 1;
        else if (msize == 1)
            mosaic_y = line + 1;
    }
}
