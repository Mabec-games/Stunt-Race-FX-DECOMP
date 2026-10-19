/*
 * superfx.c: the Super FX (GSU-1) beside the S-CPU: its registers as the
 * S-CPU sees them ($3000-$32FF), its RAM, and the bus and plotting its C
 * uses (gsu.h).
 *
 * A program starts when the S-CPU writes R15's high byte: its C (found by
 * the runtime at PBR:R15, snes_gsu_run_hook) runs on the Super FX's own
 * coroutine and returns where the program STOPs; then GO clears and, unless
 * CFGR masks it, the Super FX's IRQ rises. Time: the Super FX keeps its own
 * clock, which moves on by each access the C makes on its bus and by a
 * share of the instructions between them (gsu_access_cycles and friends);
 * the S-CPU brings it up to its own time (gsu_sync) before it touches the
 * Super FX's registers or RAM and once a line, and the Super FX hands back
 * when it gets there. So what the S-CPU sees of a running program (its
 * status, its RAM, its interrupt) appears at the time the program reaches
 * it, as on the cartridge.
 *
 * PLOT writes the pixel straight into the RAM's bitplanes (the hardware's
 * two pixel caches only delay those writes: a program's picture is the
 * same once it is done) and RPIX reads it back.
 */
#include <string.h>

#include "snes_internal.h"
#include "coro.h"

/* the decomp's register file (gsu.h's RegsGSU), laid out as it declares */
typedef struct {
    uint16_t r[16];
    uint8_t z, cy, s, ov;
    uint8_t rombr, rambr;
    uint8_t colr, por;
    uint8_t scmr, scbr;
    uint16_t ramaddr;
    uint8_t romdr;
    uint8_t pre_b, pre_alt1, pre_alt2, pre_sreg, pre_dreg;
} RegsGSU;

RegsGSU gsu;
uint8_t *gsu_ram_mem;

void (*snes_gsu_run_hook)(uint8_t bank, uint16_t addr);

/* Super FX cycles each kind of access stands for: the access with its
   wait states and the instructions around it that make none (one cycle
   each from the cache). Measured against the original: with 19 for a
   byte of RAM or ROM (and the S-CPU's overhead at 26) the attract race
   runs at the original's pace, 360 race frames in 1,750 video frames. */
unsigned gsu_access_cycles = 19;    /* a RAM or ROM byte */
unsigned gsu_plot_cycles = 3;       /* PLOT */
unsigned gsu_rpix_cycles = 10;      /* RPIX */

static uint8_t ram_store[0x20000];
static uint8_t cache[0x200];
static uint8_t pbr, cfgr, clsr, bramr, cbr_hi, cbr_lo;
static uint8_t reg_latch;
static uint8_t sfr_irq, sfr_alt, sfr_b;
static int go, pending;
static uint64_t gsu_time, gsu_target;
static Coro *gsu_co, *main_co;

static void gsu_main(void *arg);
uint8_t gsu_rd8(uint32_t addr);
void gsu_wr8(uint32_t addr, uint8_t value);
void gsu_plot(uint8_t x, uint8_t y);
uint8_t gsu_rpix(uint8_t x, uint8_t y);

void gsu_reset(void)
{
    memset(&gsu, 0, sizeof gsu);
    memset(ram_store, 0, sizeof ram_store);
    memset(cache, 0, sizeof cache);
    gsu_ram_mem = ram_store;
    pbr = cfgr = clsr = bramr = cbr_hi = cbr_lo = 0;
    reg_latch = 0;
    sfr_irq = sfr_alt = sfr_b = 0;
    go = pending = 0;
    gsu_time = gsu_target = 0;
    if (!gsu_co) {
        main_co = coro_self();
        gsu_co = coro_new(gsu_main, NULL, 8u << 20);
    }
}

uint8_t *snes_sfx_ram(void) { return ram_store; }
int gsu_running(void) { return go; }

/* ---- time ---- */
static void gsu_advance(unsigned cycles)
{
    gsu_time += (uint64_t)cycles * ((clsr & 1) ? 1u : 2u);
    if (gsu_time >= gsu_target)
        coro_switch(gsu_co, main_co);
}

void gsu_sync(void)
{
    if (!go) {
        gsu_time = snes_clock;
        return;
    }
    gsu_target = snes_clock;
    if (gsu_time < gsu_target)
        coro_switch(main_co, gsu_co);
}

static void gsu_main(void *arg)
{
    (void)arg;
    for (;;) {
        while (!pending)
            coro_switch(gsu_co, main_co);
        pending = 0;
        gsu.pre_b = gsu.pre_alt1 = gsu.pre_alt2 = gsu.pre_sreg = gsu.pre_dreg = 0;
        if (!snes_gsu_run_hook)
            snes_fatal("Super FX started with no runtime");
        snes_gsu_run_hook(pbr, gsu.r[15]);
        /* the STOP: GO clear, the interrupt unless masked */
        go = 0;
        sfr_alt = sfr_b = 0;
        if (!(cfgr & 0x80)) {
            sfr_irq = 1;
            snes_cart_irq(1);
        }
        coro_switch(gsu_co, main_co);
    }
}

/* ---- the S-CPU's side ---- */
static uint16_t sfr(void)
{
    return (uint16_t)(gsu.z << 1 | gsu.cy << 2 | gsu.s << 3 | gsu.ov << 4 | go << 5 |
                      (sfr_alt & 3) << 8 | sfr_b << 12 | sfr_irq << 15);
}

uint8_t gsu_reg_read(uint16_t addr)
{
    if (addr >= 0x3100 && addr < 0x3300)
        return cache[addr - 0x3100];
    if (addr < 0x3020) {
        uint16_t r = gsu.r[(addr >> 1) & 15];
        return (uint8_t)((addr & 1) ? r >> 8 : r);
    }
    switch (addr) {
    case 0x3030: return (uint8_t)sfr();
    case 0x3031: {
        uint8_t v = (uint8_t)(sfr() >> 8);
        sfr_irq = 0;
        snes_cart_irq(0);
        return v;
    }
    case 0x3034: return pbr;
    case 0x3036: return gsu.rombr;
    case 0x303B: return 0x01;            /* VCR: the GSU-1 */
    case 0x303C: return gsu.rambr;
    case 0x303E: return cbr_lo;
    case 0x303F: return cbr_hi;
    }
    return snes_mdr;
}

void gsu_reg_write(uint16_t addr, uint8_t v)
{
    if (addr >= 0x3100 && addr < 0x3300) {
        cache[addr - 0x3100] = v;
        return;
    }
    if (addr < 0x3020) {
        int n = (addr >> 1) & 15;
        if (!(addr & 1)) {
            reg_latch = v;
            return;
        }
        gsu.r[n] = (uint16_t)(v << 8 | reg_latch);
        if (n == 14)
            gsu.romdr = gsu_rd8((uint32_t)gsu.rombr << 16 | gsu.r[14]);
        if (n == 15) {
            /* R15 written: the program starts */
            go = 1;
            pending = 1;
            gsu_time = snes_clock;
        }
        return;
    }
    switch (addr) {
    case 0x3030:
        gsu.z = (v >> 1) & 1;
        gsu.cy = (v >> 2) & 1;
        gsu.s = (v >> 3) & 1;
        gsu.ov = (v >> 4) & 1;
        if (!(v & 0x20) && go)
            snes_fatal("the S-CPU stopped a running Super FX program (not supported)");
        if ((v & 0x20) && !go) {
            go = 1;
            pending = 1;
            gsu_time = snes_clock;
        }
        return;
    case 0x3031:
        sfr_alt = v & 3;
        sfr_b = (v >> 4) & 1;
        sfr_irq = (v >> 7) & 1;
        return;
    case 0x3033: bramr = v; return;
    case 0x3034: pbr = v; return;
    case 0x3037: cfgr = v; return;
    case 0x3038: gsu.scbr = v; return;
    case 0x3039: clsr = v; return;
    case 0x303A: gsu.scmr = v; return;
    }
}

/* ---- the Super FX's own bus (gsu.h) ---- */

static inline uint8_t bus_byte(uint32_t addr)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    uint16_t off = (uint16_t)addr;
    if (bank == 0x70 || bank == 0x71)
        return ram_store[((uint32_t)(bank & 1) << 16 | off) & (snes_cart->sfx_ram_size - 1)];
    if (bank < 0x40)
        return snes_cart->rom[((uint32_t)(bank & 0x3F) << 15 | (off & 0x7FFF)) & (snes_cart->rom_size - 1)];
    if (bank < 0x60)
        return snes_cart->rom[((uint32_t)(bank & 0x1F) << 16 | off) & (snes_cart->rom_size - 1)];
    return 0;
}

uint8_t gsu_rd8(uint32_t addr)
{
    uint8_t v = bus_byte(addr & 0xFFFFFF);
    if (go)
        gsu_advance(gsu_access_cycles);
    return v;
}

void gsu_wr8(uint32_t addr, uint8_t value)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    if (bank == 0x70 || bank == 0x71)
        ram_store[((uint32_t)(bank & 1) << 16 | (uint16_t)addr) & (snes_cart->sfx_ram_size - 1)] = value;
    if (go)
        gsu_advance(gsu_access_cycles);
}

/* the screen: SCMR's colour depth (bits 0-1: 2, 4, 4 or 8 bits) and height
   (bits 2 and 5: 128, 160, 192 lines or the sprite layout, which POR bit
   4 also picks), its base at SCBR x 1 KB; a pixel's byte address and bit */
static uint32_t pixel_at(uint8_t x, uint8_t y, unsigned *bpp, uint8_t *mask)
{
    unsigned md = gsu.scmr & 3, ht = ((gsu.scmr >> 2) & 1) | ((gsu.scmr >> 4) & 2);
    unsigned cn;
    *bpp = md == 0 ? 2 : md == 3 ? 8 : 4;
    if (gsu.por & 0x10)
        ht = 3;
    switch (ht) {
    case 0: cn = ((x & 0xF8u) << 1) + ((y & 0xF8u) >> 3); break;
    case 1: cn = ((x & 0xF8u) << 1) + ((x & 0xF8u) >> 1) + ((y & 0xF8u) >> 3); break;
    case 2: cn = ((x & 0xF8u) << 1) + (x & 0xF8u) + ((y & 0xF8u) >> 3); break;
    default: cn = ((y & 0x80u) << 2) + ((x & 0x80u) << 1) + ((y & 0x78u) << 1) + ((x & 0x78u) >> 3); break;
    }
    *mask = (uint8_t)(0x80 >> (x & 7));
    return (uint32_t)gsu.scbr * 1024u + cn * (*bpp * 8u) + (y & 7u) * 2u;
}

void gsu_plot(uint8_t x, uint8_t y)
{
    unsigned bpp, p;
    uint8_t mask, c = gsu.colr;
    uint32_t a = pixel_at(x, y, &bpp, &mask);
    if (go)
        gsu_advance(gsu_plot_cycles);
    if ((gsu.por & 0x02) && bpp != 8) {
        if ((x ^ y) & 1)
            c >>= 4;
        c &= 0x0F;
    }
    if (!(gsu.por & 0x01)) {
        if (bpp == 8 && !(gsu.por & 0x08)) {
            if (!c)
                return;
        } else if (!(c & 0x0F)) {
            return;
        }
    }
    for (p = 0; p < bpp; p++) {
        uint32_t b = (a + (p >> 1) * 16u + (p & 1)) & (snes_cart->sfx_ram_size - 1);
        if ((c >> p) & 1)
            ram_store[b] |= mask;
        else
            ram_store[b] &= (uint8_t)~mask;
    }
}

uint8_t gsu_rpix(uint8_t x, uint8_t y)
{
    unsigned bpp, p;
    uint8_t mask, c = 0;
    uint32_t a = pixel_at(x, y, &bpp, &mask);
    if (go)
        gsu_advance(gsu_rpix_cycles);
    for (p = 0; p < bpp; p++) {
        uint32_t b = (a + (p >> 1) * 16u + (p & 1)) & (snes_cart->sfx_ram_size - 1);
        if (ram_store[b] & mask)
            c |= (uint8_t)(1u << p);
    }
    return c;
}
