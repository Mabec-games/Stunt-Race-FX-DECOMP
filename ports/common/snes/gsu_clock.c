/*
 * gsu_clock.c: the Super FX's time (gsu_clock.h), step by step as the
 * GSU-1 takes it.
 */
#include "gsu_clock.h"

static unsigned bus_clocks(const GsuClock *c) { return c->clsr ? 5u : 6u; }

static void bus(GsuClock *c, int ram)
{
    if (c->bus)
        c->bus(c, ram);
}

/* The buffers count down with every clock; each makes its access as it
   ends (the ROM buffer's read, the RAM buffer's store). */
void gsuclk_step(GsuClock *c, int kind, unsigned clocks)
{
    if (c->romcl) {
        c->romcl = (uint8_t)(clocks < c->romcl ? c->romcl - clocks : 0);
        if (!c->romcl)
            bus(c, 0);
    }
    if (c->ramcl) {
        c->ramcl = (uint8_t)(clocks < c->ramcl ? c->ramcl - clocks : 0);
        if (!c->ramcl)
            bus(c, 1);
    }
    c->now += clocks;
    c->by[kind] += clocks;
}

static void sync_rom(GsuClock *c)
{
    if (c->romcl)
        gsuclk_step(c, GSUCLK_ROMBUF, c->romcl);
}

static void sync_ram(GsuClock *c)
{
    if (c->ramcl)
        gsuclk_step(c, GSUCLK_RAMBUF, c->ramcl);
}

void gsuclk_code(GsuClock *c, uint32_t addr, unsigned n)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    for (unsigned i = 0; i < n; i++) {
        uint16_t a = (uint16_t)(addr + i);
        uint16_t offset = (uint16_t)(a - c->cbr);
        if (offset < 512) {
            unsigned line = offset >> 4;
            if (!(c->valid >> line & 1)) {
                for (unsigned k = 0; k < 16; k++) {
                    gsuclk_step(c, GSUCLK_FILL, bus_clocks(c));
                    bus(c, 0);
                }
                c->valid |= 1u << line;
            } else {
                gsuclk_step(c, GSUCLK_INSN, c->clsr ? 1u : 2u);
            }
        } else if (bank <= 0x5F) {
            sync_rom(c);
            gsuclk_step(c, GSUCLK_INSN, bus_clocks(c));
            bus(c, 0);
        } else {
            sync_ram(c);
            gsuclk_step(c, GSUCLK_INSN, bus_clocks(c));
            bus(c, 1);
        }
        /* a CACHE fetched just before runs now, the byte after it in the
           pipeline */
        if (c->cache_pending) {
            c->cache_pending = 0;
            if (c->cbr != c->cbr_next)
                gsuclk_flush_code(c, c->cbr_next);
        }
    }
}

void gsuclk_cache(GsuClock *c, uint16_t r15)
{
    c->cbr_next = (uint16_t)(r15 & 0xFFF0);
    c->cache_pending = 1;
}

void gsuclk_flush_code(GsuClock *c, uint16_t cbr)
{
    c->cbr = cbr;
    c->valid = 0;
}

void gsuclk_mult(GsuClock *c)
{
    if (!c->ms0)
        gsuclk_step(c, GSUCLK_INSN, c->clsr ? 1u : 2u);
}

void gsuclk_fmult(GsuClock *c)
{
    gsuclk_step(c, GSUCLK_INSN, (c->ms0 ? 3u : 7u) * (c->clsr ? 1u : 2u));
}

void gsuclk_r14(GsuClock *c) { c->romcl = (uint8_t)bus_clocks(c); }
void gsuclk_rombuf(GsuClock *c) { sync_rom(c); }

void gsuclk_ram_read(GsuClock *c)
{
    sync_ram(c);
    bus(c, 1);
}

void gsuclk_ram_write(GsuClock *c)
{
    sync_ram(c);
    c->ramcl = (uint8_t)bus_clocks(c);
}

void gsuclk_rambuf(GsuClock *c) { sync_ram(c); }

static unsigned bpp_of(const GsuScreen *s) { return 2u << (s->md - (s->md >> 1)); }

/* the address of the row of pixel (x, y)'s character */
static uint32_t char_addr(const GsuScreen *s, uint8_t x, uint8_t y)
{
    unsigned cn = 0;
    switch (s->ht) {
    case 0: cn = ((x & 0xF8u) << 1) + ((y & 0xF8u) >> 3); break;
    case 1: cn = ((x & 0xF8u) << 1) + ((x & 0xF8u) >> 1) + ((y & 0xF8u) >> 3); break;
    case 2: cn = ((x & 0xF8u) << 1) + ((x & 0xF8u) << 0) + ((y & 0xF8u) >> 3); break;
    default: cn = ((y & 0x80u) << 2) + ((x & 0x80u) << 1) + ((y & 0x78u) << 1) + ((x & 0x78u) >> 3); break;
    }
    return 0x700000u + cn * (bpp_of(s) << 3) + ((uint32_t)s->scbr << 10) + (uint32_t)(y & 7) * 2;
}

/* A cache's row written out, a bitplane byte at a time: read first (5)
   when not all 8 pixels are plotted, the others kept; written (5). */
static void flush_pixels(GsuClock *c, const GsuScreen *s, struct GsuPixels *p)
{
    if (!p->bitpend)
        return;
    uint32_t addr = char_addr(s, (uint8_t)(p->offset << 3), (uint8_t)(p->offset >> 5));
    unsigned bpp = bpp_of(s);
    for (unsigned n = 0; n < bpp; n++) {
        uint32_t byte = ((n >> 1) << 4) + (n & 1);
        uint8_t data = 0;
        for (unsigned k = 0; k < 8; k++)
            data |= (uint8_t)(((p->data[k] >> n) & 1) << k);
        if (p->bitpend != 0xFF) {
            gsuclk_step(c, GSUCLK_FLUSH, bus_clocks(c));
            bus(c, 1);
            data &= p->bitpend;
            data |= (uint8_t)(c->pix_read(c, addr + byte) & ~p->bitpend);
        }
        gsuclk_step(c, GSUCLK_FLUSH, bus_clocks(c));
        bus(c, 1);
        c->pix_write(c, addr + byte, data);
    }
    p->bitpend = 0;
}

void gsuclk_plot(GsuClock *c, const GsuScreen *s, uint8_t x, uint8_t y, uint8_t color)
{
    uint16_t offset = (uint16_t)((y << 5) + (x >> 3));
    if (offset != c->pix[0].offset) {
        flush_pixels(c, s, &c->pix[1]);
        c->pix[1] = c->pix[0];
        c->pix[0].bitpend = 0;
        c->pix[0].offset = offset;
    }
    x = (uint8_t)((x & 7) ^ 7);
    c->pix[0].data[x] = color;
    c->pix[0].bitpend |= (uint8_t)(1u << x);
    if (c->pix[0].bitpend == 0xFF) {
        flush_pixels(c, s, &c->pix[1]);
        c->pix[1] = c->pix[0];
        c->pix[0].bitpend = 0;
    }
}

uint8_t gsuclk_rpix(GsuClock *c, const GsuScreen *s, uint8_t x, uint8_t y)
{
    flush_pixels(c, s, &c->pix[1]);
    flush_pixels(c, s, &c->pix[0]);
    uint32_t addr = char_addr(s, x, y);
    unsigned bpp = bpp_of(s);
    uint8_t data = 0;
    x = (uint8_t)((x & 7) ^ 7);
    for (unsigned n = 0; n < bpp; n++) {
        uint32_t byte = ((n >> 1) << 4) + (n & 1);
        gsuclk_step(c, GSUCLK_RPIX, bus_clocks(c));
        bus(c, 1);
        data |= (uint8_t)(((c->pix_read(c, addr + byte) >> x) & 1) << n);
    }
    return data;
}
