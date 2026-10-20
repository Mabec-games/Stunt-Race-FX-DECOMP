/*
 * apu.c: the sound unit beside the S-CPU: the SPC700's 64 KB of RAM, its
 * registers ($F0-$FF: control, the DSP's address and data, the four ports,
 * the three timers), the bus its C uses (spc.h), and the boot ROM's work.
 *
 * The SPC700's C runs on a coroutine of its own with its own clock
 * (1,025,280 Hz: 32,040 DSP samples a second, a real console's rate), which
 * each access moves on by spc_access_cycles. The S-CPU brings it up to its
 * own time before it touches the ports and once a line (apu_sync); the
 * SPC700 hands back when it gets there. Every 32 cycles the DSP makes a
 * sample, into a ring the host's audio thread drains (snes_audio_take).
 *
 * At power on the SPC700 runs its boot ROM: it clears its zero page, puts
 * $AA, $BB on ports 0 and 1, and takes the blocks the S-CPU sends by the
 * ROM's protocol, then jumps where the S-CPU says. The ROM's own bytes are
 * not here: ipl() does what that code does, access for access, so the
 * S-CPU's upload sees the same replies at the same pace. Code the S-CPU
 * uploads and starts is the game's C (the runtime's snes_spc_hooks).
 */
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "snes_internal.h"
#include "coro.h"
#include "jump.h"

/* the decomp's SPC700 registers (spc.h's SpcRegs) */
typedef struct {
    uint8_t a, x, y, sp;
    uint8_t n, v, p, b, h, i, z, c;
} SpcRegs;

SpcRegs spc;

#define SPC_HZ 1025280u
unsigned spc_access_cycles = 3;

static uint8_t aram[0x10000];
static uint8_t port_in[4], port_out[4];
static uint8_t control, dsp_addr;
static uint8_t t_target[3], t_stage[3], t_count[3];
static unsigned t01_div, t2_div, dsp_div;
static uint64_t spc_cycles, spc_target;
static Coro *spc_co, *main_co;
static SnesSpcHooks hooks;
static port_jmp_buf jb;
static uint16_t jump_to;
static void (*jump_entry)(void);

/* the samples: a ring of stereo frames */
#define RING 32768
static int16_t ring[RING * 2];
static atomic_uint ring_w, ring_r;

static void spc_main(void *arg);

void snes_spc_set_hooks(const SnesSpcHooks *h) { hooks = *h; }

void apu_reset(void)
{
    memset(&spc, 0, sizeof spc);
    memset(aram, 0, sizeof aram);
    memset(port_in, 0, sizeof port_in);
    memset(port_out, 0, sizeof port_out);
    control = 0x80;
    dsp_addr = 0;
    memset(t_target, 0, sizeof t_target);
    memset(t_stage, 0, sizeof t_stage);
    memset(t_count, 0, sizeof t_count);
    t01_div = t2_div = dsp_div = 0;
    spc_cycles = spc_target = 0;
    atomic_store(&ring_w, 0);
    atomic_store(&ring_r, 0);
    dsp_reset(aram);
    if (!spc_co) {
        main_co = coro_self();
        spc_co = coro_new(spc_main, NULL, 4u << 20);
    }
}

/* ---- time ---- */
static void timer_tick(int n)
{
    if (!(control & (1u << n)))
        return;
    if (++t_stage[n] == t_target[n]) {          /* a target of 0 counts 256 */
        t_stage[n] = 0;
        t_count[n] = (uint8_t)((t_count[n] + 1) & 15);
    }
}

static void spc_advance(unsigned cycles)
{
    spc_cycles += cycles;
    t01_div += cycles;
    while (t01_div >= 128) {
        t01_div -= 128;
        timer_tick(0);
        timer_tick(1);
    }
    t2_div += cycles;
    while (t2_div >= 16) {
        t2_div -= 16;
        timer_tick(2);
    }
    dsp_div += cycles;
    while (dsp_div >= 32) {
        unsigned w = atomic_load_explicit(&ring_w, memory_order_relaxed);
        unsigned r = atomic_load_explicit(&ring_r, memory_order_acquire);
        int16_t l, rr;
        dsp_div -= 32;
        dsp_sample(&l, &rr);
        if (w - r < RING) {
            ring[(w % RING) * 2] = l;
            ring[(w % RING) * 2 + 1] = rr;
            atomic_store_explicit(&ring_w, w + 1, memory_order_release);
        }
    }
    if (spc_cycles >= spc_target)
        coro_switch(spc_co, main_co);
}

void apu_sync(void)
{
    uint64_t c = snes_clock;
    spc_target = c / 21477273u * SPC_HZ + c % 21477273u * SPC_HZ / 21477273u;
    if (spc_cycles < spc_target)
        coro_switch(main_co, spc_co);
}

int snes_audio_queued(void)
{
    return (int)(atomic_load(&ring_w) - atomic_load(&ring_r));
}

int snes_audio_take(int16_t *lr, int max)
{
    unsigned r = atomic_load_explicit(&ring_r, memory_order_relaxed);
    unsigned w = atomic_load_explicit(&ring_w, memory_order_acquire);
    int n = 0;
    while (r != w && n < max) {
        lr[n * 2] = ring[(r % RING) * 2];
        lr[n * 2 + 1] = ring[(r % RING) * 2 + 1];
        r++;
        n++;
    }
    atomic_store_explicit(&ring_r, r, memory_order_release);
    return n;
}

/* ---- the S-CPU's side ---- */
uint8_t apu_port_read(uint8_t port) { return port_out[port & 3]; }
void apu_port_write(uint8_t port, uint8_t v) { port_in[port & 3] = v; }
uint8_t apu_debug_port(int n) { return port_in[n & 3]; }
static uint16_t dbg_last;
void apu_debug(char *out, int n)
{
    snprintf(out, (size_t)n, "SPC %llu/%llu last %04X control %02X", (unsigned long long)spc_cycles,
             (unsigned long long)spc_target, dbg_last, control);
}

/* ---- the SPC700's bus (spc.h) ---- */
uint8_t spc_rd(uint16_t addr);
void spc_wr(uint16_t addr, uint8_t v);
void spc_restart(uint16_t at, void (*entry)(void));
void spc_boot_rom(void);
void spc_cyc(unsigned n);

/* Stated time: the port's SPC700 spends spc_access_cycles per access. */
void spc_cyc(unsigned n) { (void)n; }

uint8_t spc_rd(uint16_t addr)
{
    uint8_t v;
    dbg_last = addr;
    spc_advance(spc_access_cycles);
    if (addr < 0xF0 || addr > 0xFF)
        return aram[addr];
    switch (addr) {
    case 0xF2: return dsp_addr;
    case 0xF3: return dsp_read(dsp_addr & 0x7F);
    case 0xF4: case 0xF5: case 0xF6: case 0xF7: return port_in[addr - 0xF4];
    case 0xF8: case 0xF9: return aram[addr];
    case 0xFD: case 0xFE: case 0xFF:
        v = t_count[addr - 0xFD];
        t_count[addr - 0xFD] = 0;
        return v;
    }
    return 0;
}

void spc_wr(uint16_t addr, uint8_t v)
{
    spc_advance(spc_access_cycles);
    if (addr < 0xF0 || addr > 0xFF) {
        aram[addr] = v;
        return;
    }
    switch (addr) {
    case 0xF1: {
        int n;
        for (n = 0; n < 3; n++)
            if (!(control & (1u << n)) && (v & (1u << n))) {
                t_stage[n] = 0;
                t_count[n] = 0;
            }
        if (v & 0x10)
            port_in[0] = port_in[1] = 0;
        if (v & 0x20)
            port_in[2] = port_in[3] = 0;
        control = v;
        return;
    }
    case 0xF2: dsp_addr = v; return;
    case 0xF3:
        if (dsp_addr < 0x80)
            dsp_write(dsp_addr, v);
        return;
    case 0xF4: case 0xF5: case 0xF6: case 0xF7: port_out[addr - 0xF4] = v; return;
    case 0xF8: case 0xF9: aram[addr] = v; return;
    case 0xFA: case 0xFB: case 0xFC: t_target[addr - 0xFA] = v; return;
    }
}

/* spc.h's two ends: the driver's jump to code that restarts it (its reload
   loop), and its jump to the boot ROM. The C's own frames end here (a jump
   back to the coroutine's top), and the coroutine goes on at that code. */
void spc_restart(uint16_t at, void (*entry)(void))
{
    jump_to = at;
    jump_entry = entry;
    port_longjmp(jb);
}

void spc_boot_rom(void)
{
    jump_to = 0xFFC0;
    jump_entry = NULL;
    port_longjmp(jb);
}

const uint8_t *snes_aram(void) { return aram; }

/* the boot ROM's work (see the file's comment), its accesses in its order */
static void ipl(void)
{
    uint8_t x, y, a, p;
    spc.sp = 0xEF;
    for (x = 0xEF; x; x--)
        spc_wr(x, 0x00);
    spc_wr(0xF4, 0xAA);
    spc_wr(0xF5, 0xBB);
    while (spc_rd(0xF4) != 0xCC)
        ;
    for (;;) {
        uint16_t at;
        /* a block's address from ports 2-3, the command on port 1 echoed */
        a = spc_rd(0xF6);
        y = spc_rd(0xF7);
        spc_wr(0x00, a);
        spc_wr(0x01, y);
        a = spc_rd(0xF4);
        y = spc_rd(0xF5);
        spc_wr(0xF4, a);
        x = y;
        if (!x)
            break;
        /* the block's bytes: port 0 counts them from 0 */
        do
            y = spc_rd(0xF4);
        while (y);
        for (;;) {
            p = spc_rd(0xF4);
            if (p == y) {
                a = spc_rd(0xF5);
                spc_wr(0xF4, y);
                at = (uint16_t)(aram[0x00] | aram[0x01] << 8);
                spc_wr((uint16_t)(at + y), a);
                y++;
                if (!y)
                    spc_wr(0x01, (uint8_t)(spc_rd(0x01) + 1));
                continue;
            }
            if (!((uint8_t)(y - p) & 0x80))
                continue;
            p = spc_rd(0xF4);
            if (!((uint8_t)(y - p) & 0x80))
                continue;
            break;
        }
    }
    spc.a = a;
    spc.x = 0;
    spc.y = y;
    jump_to = (uint16_t)(aram[0x00] | aram[0x01] << 8);
    jump_entry = NULL;
    port_longjmp(jb);
}

static void spc_main(void *arg)
{
    (void)arg;
    jump_to = 0xFFC0;
    jump_entry = NULL;
    for (;;) {
        if (port_setjmp(jb) == 0) {
            if (jump_to == 0xFFC0 && (control & 0x80))
                ipl();
            else if (jump_to == 0xFFC0)
                snes_fatal("SPC700 jumps to $FFC0 with the boot ROM unmapped");
            if (jump_entry)
                jump_entry();
            else if (hooks.run)
                hooks.run(jump_to);
            else
                snes_fatal("SPC700 started with no runtime");
            snes_fatal("SPC700 code at $%04X returned", jump_to);
        }
    }
}
