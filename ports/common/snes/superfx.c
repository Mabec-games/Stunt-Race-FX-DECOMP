/*
 * superfx.c: the Super FX (GSU-1) beside the S-CPU: its registers as the
 * S-CPU sees them ($3000-$32FF), its RAM, and the bus and plotting its C
 * uses (gsu.h).
 *
 * A program starts when the S-CPU writes R15's high byte: its C (found by
 * the runtime at PBR:R15, snes_gsu_run_hook) runs on the Super FX's own
 * coroutine and returns where the program STOPs; then GO clears and, unless
 * CFGR masks it, the Super FX's IRQ rises. Time: the Super FX keeps its own
 * clock, which moves on as the GSU-1's does (gsu_clock.c): by the original's
 * code fetches the C states (gsu_code: the code cache, its fills, ROM), its
 * loads and stores (the RAM buffer), R14 writes and ROM-buffer reads,
 * MULT and FMULT, the pixel caches' flushes, and the waits for the bus while
 * the S-CPU has it (SCMR's RON and RAN clear). The S-CPU brings it up to its
 * own time (gsu_sync) before it touches the Super FX's registers or RAM and
 * once a line, and the Super FX hands back when it gets there. So what the
 * S-CPU sees of a running program (its status, its RAM, its interrupt)
 * appears at the time the program reaches it, as on the cartridge.
 *
 * PLOT and RPIX go through the two pixel caches as on the hardware: a row
 * of 8 pixels reaches the RAM's bitplanes when it is flushed.
 */
#include <string.h>
#include <stdlib.h>

#include "snes_internal.h"
#include "coro.h"
#include "gsu_clock.h"

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

static uint8_t ram_store[0x20000];
static uint8_t cache[0x200];
static uint8_t pbr, cfgr, clsr, bramr, cbr_hi, cbr_lo;
static uint8_t reg_latch;
static uint8_t sfr_irq, sfr_alt, sfr_b;
static int go, pending;
static uint64_t gsu_now, gsu_target;   /* master clocks */
static GsuClock clk;
/* The cartridge clocks the Super FX from a crystal of its own, 21.44 MHz,
   not the console's 21.477 MHz: the model's clocks (clk.now) are the Super
   FX's, turned into the console's at the rate between them. */
#define GSU_HZ 21440000u
#define MASTER_HZ 21477273u
static uint64_t run_master, run_clk;   /* a program's start: the console's clock, the model's */
static Coro *gsu_co, *main_co;
/* PORT_GSU_SPEED=F: the Super FX F times as fast as the cartridge's (an
   overclock, as emulators offer: the game's frames come sooner; not the
   original's pacing) */
static double gsu_speed = 1.0;
void gsu_set_speed(double f) { gsu_speed = f >= 0.25 ? f : 1.0; }
double gsu_get_speed(void) { return gsu_speed; }


static void gsu_main(void *arg);
static inline uint8_t bus_byte(uint32_t addr);
static void bus_wait(GsuClock *c, int ram);
static uint8_t pix_read(GsuClock *c, uint32_t addr);
static void pix_write(GsuClock *c, uint32_t addr, uint8_t v);
uint8_t gsu_rd8(uint32_t addr);
void gsu_wr8(uint32_t addr, uint8_t value);
void gsu_plot(uint8_t x, uint8_t y);
uint8_t gsu_rpix(uint8_t x, uint8_t y);

void gsu_reset(void)
{
    {
        const char *e = getenv("PORT_GSU_SPEED");
        gsu_speed = e && atof(e) >= 0.25 ? atof(e) : 1.0;
    }
    memset(&gsu, 0, sizeof gsu);
    /* a cartridge with no save: its RAM reads $FF throughout, as the
       verification harness powers the original on */
    memset(ram_store, 0xFF, sizeof ram_store);
    memset(cache, 0, sizeof cache);
    gsu_ram_mem = ram_store;
    pbr = cfgr = clsr = bramr = cbr_hi = cbr_lo = 0;
    reg_latch = 0;
    sfr_irq = sfr_alt = sfr_b = 0;
    go = pending = 0;
    gsu_now = gsu_target = 0;
    memset(&clk, 0, sizeof clk);
    clk.bus = bus_wait;
    clk.pix_read = pix_read;
    clk.pix_write = pix_write;
    run_master = run_clk = 0;
    if (!gsu_co) {
        main_co = coro_self();
        gsu_co = coro_new(gsu_main, NULL, 8u << 20);
    }
}

uint8_t *snes_sfx_ram(void) { return ram_store; }
int gsu_running(void) { return go; }

/* ---- a sandbox (the port's in-between frames, interp.c) ----
   Everything of the Super FX's: its RAM, registers, cache and clock, saved
   and put back whole, so programs can be run on other inputs and leave no
   trace. Only while no program is running. */
typedef struct {
    uint8_t ram[0x20000], cache[0x200];
    RegsGSU gsu;
    uint8_t pbr, cfgr, clsr, bramr, cbr_hi, cbr_lo, reg_latch, sfr_irq, sfr_alt, sfr_b;
    int go, pending;
    uint64_t gsu_now, gsu_target, run_master, run_clk;
    GsuClock clk;
} GsuSandbox;

unsigned long gsu_sandbox_size(void) { return sizeof(GsuSandbox); }
int gsu_idle(void) { return !go && !pending; }

void gsu_sandbox_save(void *p)
{
    GsuSandbox *s = (GsuSandbox *)p;
    memcpy(s->ram, ram_store, sizeof s->ram);
    memcpy(s->cache, cache, sizeof s->cache);
    s->gsu = gsu;
    s->pbr = pbr; s->cfgr = cfgr; s->clsr = clsr; s->bramr = bramr; s->cbr_hi = cbr_hi; s->cbr_lo = cbr_lo;
    s->reg_latch = reg_latch; s->sfr_irq = sfr_irq; s->sfr_alt = sfr_alt; s->sfr_b = sfr_b;
    s->go = go; s->pending = pending;
    s->gsu_now = gsu_now; s->gsu_target = gsu_target; s->run_master = run_master; s->run_clk = run_clk;
    s->clk = clk;
}

/* idle: the state loaded as stopped (no program running), for running
   programs directly (gsu_sandbox_run) */
void gsu_sandbox_load(const void *p, int idle)
{
    const GsuSandbox *s = (const GsuSandbox *)p;
    memcpy(ram_store, s->ram, sizeof s->ram);
    memcpy(cache, s->cache, sizeof cache);
    gsu = s->gsu;
    pbr = s->pbr; cfgr = s->cfgr; clsr = s->clsr; bramr = s->bramr; cbr_hi = s->cbr_hi; cbr_lo = s->cbr_lo;
    reg_latch = s->reg_latch; sfr_irq = s->sfr_irq; sfr_alt = s->sfr_alt; sfr_b = s->sfr_b;
    go = idle ? 0 : s->go; pending = idle ? 0 : s->pending;
    gsu_now = s->gsu_now; gsu_target = s->gsu_target; run_master = s->run_master; run_clk = s->run_clk;
    clk = s->clk;
}

/* the program at bank:addr run to its STOP here and now, with GO clear:
   its C never hands back to the S-CPU (clk_sync, bus_wait); the clock's
   bookkeeping skipped (the sandbox is put back after: its time is
   nobody's), the pixels still plotted through the caches */
static int untimed;
/* the port's own extra Super FX work inside a running program's STOP (the
   time scale's re-posed cars): untimed, so it never hands back */
void gsu_set_untimed(int on) { untimed = on; }

void gsu_sandbox_run(uint8_t bank, uint16_t addr)
{
    pbr = bank;
    gsu.r[15] = addr;
    gsu.pre_b = gsu.pre_alt1 = gsu.pre_alt2 = gsu.pre_sreg = gsu.pre_dreg = 0;
    untimed = 1;
    snes_gsu_run_hook(bank, addr);
    untimed = 0;
}

/* ---- time ---- */

/* The Super FX's clock as the model has moved it; past the S-CPU's time,
   back to the S-CPU until it catches up. */
static void clk_sync(void)
{
    if (!go)
        return;
    gsu_now = run_master + (uint64_t)((double)(clk.now - run_clk) * MASTER_HZ / GSU_HZ / gsu_speed);
    if (gsu_now >= gsu_target)
        coro_switch(gsu_co, main_co);
}

void gsu_sync(void)
{
    if (!go) {
        gsu_now = snes_clock;
        return;
    }
    gsu_target = snes_clock;
    if (gsu_now < gsu_target)
        coro_switch(main_co, gsu_co);
}

/* A bus access while the S-CPU has that bus (SCMR's RAN bit 3 for the RAM,
   RON bit 4 for the ROM clear): 6 clocks at a time until it gives it back. */
static void bus_wait(GsuClock *c, int ram)
{
    while (go && !(gsu.scmr & (ram ? 0x08 : 0x10))) {
        gsuclk_step(c, GSUCLK_BUS, 6);
        clk_sync();
    }
}

static void program_starts(void)
{
    go = 1;
    pending = 1;
    gsu_now = snes_clock;
    /* the NOP the last STOP left in the pipeline runs first: it fetches
       the program's first byte */
    run_master = gsu_now;
    run_clk = clk.now;
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
        gsuclk_code(&clk, (uint32_t)pbr << 16 | gsu.r[15], 1);
        if (bus_byte((uint32_t)pbr << 16 | gsu.r[15]) == 0x02)
            gsuclk_cache(&clk, (uint16_t)(gsu.r[15] + 1));   /* a CACHE first: it runs after the next fetch */
        clk_sync();
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
        return cache[(addr - 0x3100 + clk.cbr) & 511];
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
        /* the code cache, CBR on; a line's last byte makes it valid */
        unsigned a = (addr - 0x3100 + clk.cbr) & 511;
        cache[a] = v;
        if ((a & 15) == 15)
            clk.valid |= 1u << (a >> 4);
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
        if (n == 15)
            program_starts();   /* R15 written: the program starts */
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
        if ((v & 0x20) && !go)
            program_starts();
        return;
    case 0x3031:
        sfr_alt = v & 3;
        sfr_b = (v >> 4) & 1;
        sfr_irq = (v >> 7) & 1;
        return;
    case 0x3033: bramr = v; return;
    case 0x3034:
        pbr = v;
        gsuclk_flush_code(&clk, clk.cbr);   /* the code cache emptied */
        return;
    case 0x3037: cfgr = v; clk.ms0 = (uint8_t)(v >> 5 & 1); return;
    case 0x3038: gsu.scbr = v; return;
    case 0x3039: clsr = v; clk.clsr = (uint8_t)(v & 1); return;
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

static int is_ram(uint32_t addr) { uint8_t bank = (uint8_t)(addr >> 16); return bank == 0x70 || bank == 0x71; }
static uint8_t *ram_byte(uint32_t addr)
{
    return &ram_store[((uint32_t)((addr >> 16) & 1) << 16 | (uint16_t)addr) & (snes_cart->sfx_ram_size - 1)];
}

/* A load or store through the RAM buffer (a read of ROM is the ROM
   buffer's, timed by gsu_buffer). */
uint8_t gsu_rd8(uint32_t addr)
{
    addr &= 0xFFFFFF;
    if (is_ram(addr) && go && !untimed) {
        gsuclk_ram_read(&clk);
        clk_sync();
    }
    return bus_byte(addr);
}

void gsu_wr8(uint32_t addr, uint8_t value)
{
    addr &= 0xFFFFFF;
    if (!is_ram(addr))
        return;
    if (go && !untimed) {
        gsuclk_ram_write(&clk);
        clk_sync();
    }
    *ram_byte(addr) = value;
}

/* the pixel caches' own accesses: straight to the RAM */
static uint8_t pix_read(GsuClock *c, uint32_t addr) { (void)c; return *ram_byte(addr); }
static void pix_write(GsuClock *c, uint32_t addr, uint8_t v) { (void)c; *ram_byte(addr) = v; }

/* ---- the time the C states (gsu.h) ---- */
void gsu_code(uint32_t addr, uint32_t n) { if (untimed) return; gsuclk_code(&clk, addr, n); clk_sync(); }
void gsu_cache(uint16_t r15) { if (untimed) return; gsuclk_cache(&clk, r15); }
void gsu_cyc(uint32_t clocks) { if (untimed) return; gsuclk_step(&clk, GSUCLK_INSN, clocks); clk_sync(); }

void gsu_time(int what)
{
    if (untimed)
        return;
    if (what == 0)
        gsuclk_mult(&clk);         /* GSU_T_MULT */
    else
        gsuclk_fmult(&clk);        /* GSU_T_FMULT */
    clk_sync();
}

void gsu_buffer(int what)
{
    if (untimed)
        return;
    if (what == 0)
        gsuclk_r14(&clk);          /* GSU_B_R14 */
    else if (what == 2)
        gsuclk_rambuf(&clk);       /* GSU_B_RAMBUF */
    else
        gsuclk_rombuf(&clk);       /* GSU_B_ROMBUF, GSU_B_ROMB */
    clk_sync();
}

/* the screen as PLOT and RPIX lay it out: SCMR's colour depth (bits 0-1)
   and height (bits 2 and 5; POR's OBJ, bit 4, the sprite layout), its base
   SCBR x 1 KB */
static GsuScreen screen(void)
{
    GsuScreen s;
    s.ht = (uint8_t)((gsu.por & 0x10) ? 3 : ((gsu.scmr >> 2) & 1) | ((gsu.scmr >> 4) & 2));
    s.md = (uint8_t)(gsu.scmr & 3);
    s.scbr = gsu.scbr;
    return s;
}

void (*snes_gsu_plot_hook)(uint8_t x, uint8_t y, uint8_t c);
/* the port's wider redraws (a sandbox drawing past the game's buffer):
   pixels at or past these left out (0: none) */
int gsu_plot_clip_w, gsu_plot_clip_h;

void gsu_plot(uint8_t x, uint8_t y)
{
    GsuScreen s = screen();
    uint8_t c = gsu.colr;
    if (gsu_plot_clip_w && (x >= gsu_plot_clip_w || y >= gsu_plot_clip_h))
        return;
    if (!(gsu.por & 0x01)) {
        if (s.md == 3) {
            if (gsu.por & 0x08) {
                if (!(c & 0x0F))
                    return;
            } else if (!c) {
                return;
            }
        } else if (!(c & 0x0F)) {
            return;
        }
    }
    if ((gsu.por & 0x02) && s.md != 3) {
        if ((x ^ y) & 1)
            c >>= 4;
        c &= 0x0F;
    }
    if (snes_gsu_plot_hook)
        snes_gsu_plot_hook(x, y, c);
    gsuclk_plot(&clk, &s, x, y, c);
    clk_sync();
}

uint8_t gsu_rpix(uint8_t x, uint8_t y)
{
    GsuScreen s = screen();
    uint8_t v = gsuclk_rpix(&clk, &s, x, y);
    clk_sync();
    return v;
}
