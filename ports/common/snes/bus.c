/*
 * bus.c: the S-CPU's bus and clock.
 *
 * rd8 and wr8 are the decomp's view of the machine (machine.h): one byte of
 * the 24-bit address space each. Here they reach the work RAM, the
 * cartridge (LoROM with a Super FX: ROM at $00-$3F:8000 and $40-$5F, its
 * RAM at $70-$71 and $00-$3F:6000, its registers at $3000), the PPU and the
 * sound ports on the B bus ($2100-$21FF), the joypads ($4016-$4017) and
 * the CPU's own registers ($4200-$421F, DMA at $4300).
 *
 * Each access moves the master clock on by the time the bus takes for it
 * (8 clocks for ROM and RAM on this SlowROM cartridge, 6 for most
 * registers, 12 for the joypads' serial ports); the opcode and operand
 * fetches and internal cycles between two accesses are the time the C
 * states (cyc: snes_cpu_clocks), the original's own. Passing a line's horizontal blank draws that
 * line and runs HDMA; passing line 225 starts the vertical blank (the NMI
 * flag, the joypads' automatic read, the frame's hook).
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "snes_internal.h"

uint8_t rd8(uint32_t addr);
void wr8(uint32_t addr, uint8_t value);

uint64_t snes_clock;
uint8_t snes_mdr;
const SnesCart *snes_cart;
static SnesCart s_cart;

void (*snes_interrupt_hook)(int nmi);
int (*snes_irq_masked)(void);
void (*snes_frame_hook)(void);

static uint8_t wram[0x20000];
static uint32_t wram_port;

/* the CPU's registers */
static uint8_t nmitimen, wrio = 0xFF, wrmpya = 0xFF, wrmpyb, wrdivb, memsel;
static uint16_t wrdiv = 0xFFFF, rddiv, rdmpy;
static uint16_t htime = 0x1FF, vtime = 0x1FF;
static uint8_t rdnmi_flag, timeup_flag;
static int nmi_pending, cart_irq;

/* the joypads */
static uint16_t joy_held[2], joy_auto[2];
static uint16_t joy_shift[2];
static int joy_shifted[2];
static uint8_t joy_strobe;
static uint64_t joy_busy_until;

/* the beam: the line now and where it began */
static unsigned vline;
static uint64_t line_start;
static uint32_t frames;
static int line_done_hblank, line_done_refresh;
static uint64_t irq_at, next_event;
#define NEVER UINT64_MAX
#define HBLANK_AT 1096                    /* dot 274 */
/* the work RAM's refresh: the S-CPU stopped for 40 clocks once a line, at
   dot 134 */
#define REFRESH_AT 536
#define REFRESH_CLOCKS 40

/* where an interrupt may not be taken: the read straight after a read of
   the byte before it (the second byte of one word) */
static uint32_t last_read = 0xFFFFFFFFu;
static unsigned dbg_irqs, dbg_nmis;          /* for PORT_DEBUG */
static int last_was_read;

void snes_fatal(const char *fmt, ...)
{
    va_list ap;
    fprintf(stderr, "port: ");
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(70);
}

uint32_t snes_frame(void) { return frames; }
unsigned snes_vcounter(void) { return vline; }
unsigned snes_hcounter(void) { return (unsigned)((snes_clock - line_start) / 4); }
const uint8_t *snes_wram(void) { return wram; }

/* ---- interrupts and the beam --------------------------------------------------- */
/* This line's IRQ time. At the line's start (written 0) every time on the
   line counts, however far the clock has run on; after a write to the
   registers mid-line, a time already passed gives no IRQ this line. */
static void irq_schedule(int written)
{
    unsigned mode = (nmitimen >> 4) & 3;
    uint64_t at = NEVER;
    if (mode == 1 && htime < 340)
        at = line_start + htime * 4u + 14;
    else if (mode == 2 && vline == vtime)
        at = line_start + 10;
    else if (mode == 3 && vline == vtime && htime < 340)
        at = line_start + htime * 4u + 14;
    irq_at = at;
    if (written && at != NEVER && at < snes_clock)
        irq_at = NEVER;
}

static void events_next(void)
{
    uint64_t e = !line_done_refresh ? line_start + REFRESH_AT
               : line_done_hblank ? line_start + SNES_LINE_CLOCKS : line_start + HBLANK_AT;
    next_event = irq_at < e ? irq_at : e;
}

static void line_begin(void)
{
    if (vline == 225) {
        rdnmi_flag = 1;
        if (nmitimen & 0x80)
            nmi_pending = 1;
        ppu_vblank_start();
        /* the frame is done: the front end shows it and sets the buttons
           the joypads' read now takes */
        if (getenv("PORT_DEBUG")) {
            char a[128];
            apu_debug(a, sizeof a);
            fprintf(stderr, "%s; irqs %u nmis %u nmitimen %02X vtime %u htime %u; ", a, dbg_irqs, dbg_nmis, nmitimen, vtime, htime);
            dbg_irqs = dbg_nmis = 0;
        }
        if (getenv("PORT_DEBUG"))
            fprintf(stderr, "frame %u: last read %06X, Super FX %s, APU in %02X %02X %02X %02X out %02X %02X %02X %02X\n",
                    frames, last_read, gsu_running() ? "running" : "idle",
                    apu_debug_port(0), apu_debug_port(1), apu_debug_port(2), apu_debug_port(3),
                    apu_port_read(0), apu_port_read(1), apu_port_read(2), apu_port_read(3));
        if (snes_frame_hook)
            snes_frame_hook();
        if (nmitimen & 1) {
            joy_auto[0] = joy_held[0];
            joy_auto[1] = joy_held[1];
            joy_shift[0] = joy_shift[1] = 0xFFFF;
            joy_shifted[0] = joy_shifted[1] = 16;
            joy_busy_until = snes_clock + 130 + 4224;
        }
    } else if (vline == 0) {
        rdnmi_flag = 0;
        frames++;
        ppu_frame_start();
        hdma_init();
    }
    irq_schedule(0);
}

static void run_events(void)
{
    for (;;) {
        if (irq_at != NEVER && snes_clock >= irq_at && irq_at <= line_start + SNES_LINE_CLOCKS) {
            timeup_flag = 1;
            irq_at = NEVER;
            continue;
        }
        if (!line_done_refresh && snes_clock >= line_start + REFRESH_AT) {
            line_done_refresh = 1;
            snes_clock += REFRESH_CLOCKS;
            continue;
        }
        if (!line_done_hblank && snes_clock >= line_start + HBLANK_AT) {
            line_done_hblank = 1;
            if (vline >= 1 && vline <= 224)
                ppu_render_line(vline);
            if (vline <= 224)
                hdma_line();
            gsu_sync();
            apu_sync();
            continue;
        }
        if (snes_clock >= line_start + SNES_LINE_CLOCKS) {
            line_start += SNES_LINE_CLOCKS;
            vline = vline + 1 == SNES_LINES ? 0 : vline + 1;
            line_done_hblank = line_done_refresh = 0;
            irq_at = NEVER;
            line_begin();
            continue;
        }
        break;
    }
    events_next();
}

void snes_tick(unsigned clocks)
{
    snes_clock += clocks;
    if (snes_clock >= next_event)
        run_events();
}

void snes_cart_irq(int level) { cart_irq = level; }

static void check_interrupts(void)
{
    if (nmi_pending) {
        nmi_pending = 0;
        dbg_nmis++;
        if (snes_interrupt_hook)
            snes_interrupt_hook(1);
        return;
    }
    if ((timeup_flag || cart_irq) && snes_interrupt_hook && !(snes_irq_masked && snes_irq_masked())) {
        dbg_irqs++;
        snes_interrupt_hook(0);
    }
}

/* ---- the CPU's registers ---------------------------------------------------------- */
static uint8_t cpu_reg_read(uint16_t off)
{
    uint8_t v;
    switch (off) {
    case 0x4210:
        v = (uint8_t)(rdnmi_flag << 7 | (snes_mdr & 0x70) | 0x02);
        rdnmi_flag = 0;
        return v;
    case 0x4211:
        v = (uint8_t)(timeup_flag << 7 | (snes_mdr & 0x7F));
        timeup_flag = 0;
        return v;
    case 0x4212: {
        uint64_t pos = snes_clock - line_start;
        v = (uint8_t)(snes_mdr & 0x3E);
        if (vline >= 225)
            v |= 0x80;
        if (pos >= HBLANK_AT || pos < 4)
            v |= 0x40;
        if (snes_clock < joy_busy_until)
            v |= 0x01;
        return v;
    }
    case 0x4213: return wrio;
    case 0x4214: return (uint8_t)rddiv;
    case 0x4215: return (uint8_t)(rddiv >> 8);
    case 0x4216: return (uint8_t)rdmpy;
    case 0x4217: return (uint8_t)(rdmpy >> 8);
    case 0x4218: return (uint8_t)joy_auto[0];
    case 0x4219: return (uint8_t)(joy_auto[0] >> 8);
    case 0x421A: return (uint8_t)joy_auto[1];
    case 0x421B: return (uint8_t)(joy_auto[1] >> 8);
    case 0x421C: case 0x421D: case 0x421E: case 0x421F: return 0;
    }
    if (off >= 0x4300 && off < 0x4380)
        return dma_read(off);
    return snes_mdr;
}

static void cpu_reg_write(uint16_t off, uint8_t v)
{
    switch (off) {
    case 0x4200: {
        uint8_t was = nmitimen;
        nmitimen = v;
        /* the NMI enabled during the vertical blank, its flag still set */
        if (!(was & 0x80) && (v & 0x80) && rdnmi_flag)
            nmi_pending = 1;
        if (!(v & 0x30))
            timeup_flag = 0;
        irq_schedule(1);
        events_next();
        return;
    }
    case 0x4201:
        if ((wrio & 0x80) && !(v & 0x80))
            ppu_latch_counters();
        wrio = v;
        return;
    case 0x4202: wrmpya = v; return;
    case 0x4203:
        wrmpyb = v;
        rdmpy = (uint16_t)(wrmpya * wrmpyb);
        rddiv = wrmpyb;
        return;
    case 0x4204: wrdiv = (uint16_t)((wrdiv & 0xFF00) | v); return;
    case 0x4205: wrdiv = (uint16_t)((wrdiv & 0x00FF) | v << 8); return;
    case 0x4206:
        wrdivb = v;
        if (v) {
            rddiv = (uint16_t)(wrdiv / v);
            rdmpy = (uint16_t)(wrdiv % v);
        } else {
            rddiv = 0xFFFF;
            rdmpy = wrdiv;
        }
        return;
    case 0x4207: htime = (uint16_t)((htime & 0x100) | v); irq_schedule(1); events_next(); return;
    case 0x4208: htime = (uint16_t)((htime & 0xFF) | (v & 1) << 8); irq_schedule(1); events_next(); return;
    case 0x4209: vtime = (uint16_t)((vtime & 0x100) | v); irq_schedule(1); events_next(); return;
    case 0x420A: vtime = (uint16_t)((vtime & 0xFF) | (v & 1) << 8); irq_schedule(1); events_next(); return;
    case 0x420B: dma_start(v); return;
    case 0x420C: dma_hdmaen = v; return;
    case 0x420D: memsel = v; return;
    }
    if (off >= 0x4300 && off < 0x4380)
        dma_write(off, v);
}

/* the joypads' serial ports: a strobe latches the buttons, then each read
   shifts out one (B first); past the 16th, a standard pad reads 1 */
static uint8_t joy_serial_read(int port)
{
    uint8_t bit;
    if (joy_strobe & 1)
        return (uint8_t)(joy_held[port] >> 15 & 1);
    bit = (uint8_t)(joy_shift[port] >> 15 & 1);
    joy_shift[port] = (uint16_t)(joy_shift[port] << 1 | 1);
    return bit;
}

static void joy_strobe_write(uint8_t v)
{
    if ((joy_strobe & 1) && !(v & 1)) {
        joy_shift[0] = joy_held[0];
        joy_shift[1] = joy_held[1];
    }
    joy_strobe = v;
}

void snes_set_joypad(int port, uint16_t buttons)
{
    if (port == 0 || port == 1)
        joy_held[port] = (uint16_t)(buttons & 0xFFF0);
}

/* ---- the B bus --------------------------------------------------------------------- */
uint8_t bus_read_b(uint8_t reg)
{
    if (reg >= 0x34 && reg < 0x40)
        return ppu_read(reg);
    if (reg >= 0x40 && reg < 0x80) {
        apu_sync();
        return apu_port_read(reg & 3);
    }
    if (reg == 0x80) {
        uint8_t v = wram[wram_port];
        wram_port = (wram_port + 1) & 0x1FFFF;
        return v;
    }
    return snes_mdr;
}

void bus_write_b(uint8_t reg, uint8_t v)
{
    if (reg < 0x34) {
        ppu_write(reg, v);
        return;
    }
    if (reg >= 0x40 && reg < 0x80) {
        apu_sync();
        apu_port_write(reg & 3, v);
        return;
    }
    switch (reg) {
    case 0x80:
        wram[wram_port] = v;
        wram_port = (wram_port + 1) & 0x1FFFF;
        return;
    case 0x81: wram_port = (wram_port & 0x1FF00) | v; return;
    case 0x82: wram_port = (wram_port & 0x100FF) | (uint32_t)v << 8; return;
    case 0x83: wram_port = (wram_port & 0x0FFFF) | (uint32_t)(v & 1) << 16; return;
    }
}

/* ---- the address space ------------------------------------------------------------- */
static inline uint32_t rom_lo(uint8_t bank, uint16_t off)
{
    return ((uint32_t)(bank & 0x3F) << 15 | (off & 0x7FFF)) & (s_cart.rom_size - 1);
}

static inline uint32_t rom_hi(uint8_t bank, uint16_t off)
{
    return ((uint32_t)(bank & 0x1F) << 16 | off) & (s_cart.rom_size - 1);
}

static inline uint8_t *sfx_ram_at(uint32_t a)
{
    gsu_sync();
    return &gsu_ram_mem[a & (s_cart.sfx_ram_size - 1)];
}

/* kind: 0 the CPU, 1 DMA's A bus (which reaches no registers) */
static uint8_t bus_read(uint32_t addr, unsigned *cost, int kind)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    uint16_t off = (uint16_t)addr;
    *cost = 8;
    if (bank == 0x7E || bank == 0x7F)
        return wram[(uint32_t)(bank & 1) << 16 | off];
    if (!(bank & 0x40)) {
        if (off < 0x2000)
            return wram[off];
        if (off >= 0x8000) {
            if ((bank & 0x80) && (memsel & 1))
                *cost = 6;
            return s_cart.rom[rom_lo(bank, off)];
        }
        if (off >= 0x6000) {
            if (s_cart.superfx)
                return *sfx_ram_at(off - 0x6000);
            return snes_mdr;
        }
        *cost = 6;
        if (kind == 1 && ((off >= 0x2100 && off < 0x2200) || (off >= 0x4000 && off < 0x4400)))
            return snes_mdr;
        if (off >= 0x2100 && off < 0x2200)
            return bus_read_b((uint8_t)off);
        if (off >= 0x3000 && off < 0x3300 && s_cart.superfx) {
            gsu_sync();
            return gsu_reg_read(off);
        }
        if (off == 0x4016 || off == 0x4017) {
            *cost = 12;
            if (off == 0x4016)
                return (uint8_t)((snes_mdr & 0xFC) | joy_serial_read(0));
            return (uint8_t)((snes_mdr & 0xE0) | 0x1C | joy_serial_read(1));
        }
        if (off >= 0x4200 && off < 0x4400)
            return cpu_reg_read(off);
        return snes_mdr;
    }
    bank &= 0x7F;
    if (bank < 0x60)
        return s_cart.rom[rom_hi(bank, off)];
    if ((bank == 0x70 || bank == 0x71) && s_cart.superfx)
        return *sfx_ram_at((uint32_t)(bank & 1) << 16 | off);
    return snes_mdr;
}

static void bus_write(uint32_t addr, uint8_t v, unsigned *cost, int kind)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    uint16_t off = (uint16_t)addr;
    *cost = 8;
    if (bank == 0x7E || bank == 0x7F) {
        wram[(uint32_t)(bank & 1) << 16 | off] = v;
        return;
    }
    if (!(bank & 0x40)) {
        if (off < 0x2000) {
            wram[off] = v;
            return;
        }
        if (off >= 0x8000)
            return;
        if (off >= 0x6000) {
            if (s_cart.superfx)
                *sfx_ram_at(off - 0x6000) = v;
            return;
        }
        *cost = 6;
        if (kind == 1 && ((off >= 0x2100 && off < 0x2200) || (off >= 0x4000 && off < 0x4400)))
            return;
        if (off >= 0x2100 && off < 0x2200) {
            bus_write_b((uint8_t)off, v);
            return;
        }
        if (off >= 0x3000 && off < 0x3300 && s_cart.superfx) {
            gsu_sync();
            gsu_reg_write(off, v);
            return;
        }
        if (off == 0x4016) {
            *cost = 12;
            joy_strobe_write(v);
            return;
        }
        if (off >= 0x4200 && off < 0x4400)
            cpu_reg_write(off, v);
        return;
    }
    bank &= 0x7F;
    if ((bank == 0x70 || bank == 0x71) && s_cart.superfx)
        *sfx_ram_at((uint32_t)(bank & 1) << 16 | off) = v;
}

uint8_t bus_read_a(uint32_t addr)
{
    unsigned c;
    uint8_t v = bus_read(addr & 0xFFFFFF, &c, 1);
    snes_mdr = v;
    return v;
}

void bus_write_a(uint32_t addr, uint8_t v)
{
    unsigned c;
    snes_mdr = v;
    bus_write(addr & 0xFFFFFF, v, &c, 1);
}

uint8_t snes_peek(uint32_t addr)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    uint16_t off = (uint16_t)addr;
    if (bank == 0x7E || bank == 0x7F)
        return wram[(uint32_t)(bank & 1) << 16 | off];
    if (!(bank & 0x40)) {
        if (off < 0x2000)
            return wram[off];
        if (off >= 0x8000)
            return s_cart.rom[rom_lo(bank, off)];
        return 0;
    }
    if ((bank & 0x7F) < 0x60)
        return s_cart.rom[rom_hi(bank & 0x7F, off)];
    return 0;
}

/* ---- the decomp's bus -------------------------------------------------------------- */
/* the clocks one access takes: 8 for memory (ROM, RAM), 6 for registers,
   12 for the joypads' serial ports */
unsigned snes_access_clocks(uint32_t addr)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    uint16_t off = (uint16_t)addr;
    if ((bank & 0x40) || off < 0x2000 || off >= 0x6000) {
        if (off >= 0x8000 && (bank & 0x80) && (memsel & 1) && bank != 0xFE && bank != 0xFF)
            return 6;
        return 8;
    }
    if (off == 0x4016 || off == 0x4017)
        return 12;
    return 6;
}

/* A read of the cartridge's ROM by the C: the C states its time with the
   instruction around it (the checks place the C's reads of RAM and the
   registers on the original's, and leave a ROM read in the time between),
   so it adds none here. */
static int rom_data(uint32_t addr)
{
    uint8_t bank = (uint8_t)(addr >> 16);
    if (!(bank & 0x40))
        return (uint16_t)addr >= 0x8000;
    bank &= 0x7F;
    return bank != 0x70 && bank != 0x71 && bank != 0x7E && bank != 0x7F;
}

/* No interrupt is taken between the two bytes of one word: a read
   straight after a read of the byte before it. */

uint8_t rd8(uint32_t addr)
{
    unsigned cost;
    uint8_t v;
    int second;
    addr &= 0xFFFFFF;
    second = last_was_read && addr == ((last_read + 1) & 0xFFFFFF);
    if (!second)
        check_interrupts();
    last_read = addr;
    last_was_read = 1;
    /* the clock first: what the read sees is the machine at its end */
    snes_tick(rom_data(addr) ? 0 : snes_access_clocks(addr));
    v = bus_read(addr, &cost, 0);
    snes_mdr = v;
    return v;
}

void wr8(uint32_t addr, uint8_t value)
{
    unsigned cost;
    addr &= 0xFFFFFF;
    last_was_read = 0;
    snes_tick(snes_access_clocks(addr));
    snes_mdr = value;
    bus_write(addr, value, &cost, 0);
}

/* A stretch with no access (a delay loop): the clock moves on in short
   steps, interrupts taken between them as between the loop's turns */
void snes_cpu_wait(uint32_t clocks)
{
    while (clocks) {
        unsigned n = clocks > 36 ? 36 : (unsigned)clocks;
        snes_tick(n);
        clocks -= n;
        check_interrupts();
    }
    last_was_read = 0;
}
void snes_cpu_clocks(uint32_t clocks) { snes_tick(clocks); }
void snes_cpu_interrupts(void) { check_interrupts(); }

/* ---- power ---------------------------------------------------------------------- */
void snes_power(const SnesCart *cart)
{
    s_cart = *cart;
    snes_cart = &s_cart;
    memset(wram, 0, sizeof wram);
    wram_port = 0;
    snes_clock = 0;
    nmitimen = 0;
    wrio = 0xFF;
    htime = vtime = 0x1FF;
    rdnmi_flag = timeup_flag = 0;
    nmi_pending = cart_irq = 0;
    joy_auto[0] = joy_auto[1] = 0;
    joy_busy_until = 0;
    vline = 0;
    line_start = 0;
    frames = 0;
    line_done_hblank = line_done_refresh = 0;
    irq_at = NEVER;
    ppu_reset();
    dma_reset();
    gsu_reset();
    apu_reset();
    events_next();
}
