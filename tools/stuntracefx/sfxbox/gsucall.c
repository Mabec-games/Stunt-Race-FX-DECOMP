/*
 * gsucall.c: Super FX routines in C, in the per-call oracle (A5) and the
 * lockstep run (A7). The same design as the S-CPU's (check.c), on the
 * Super FX's terms.
 *
 * Where a routine starts and ends. The Super FX has no stack and no call
 * instruction: a caller sets R11 to its return address with LINK and jumps
 * (IWT R15 or JMP Rn); the instruction after the jump, its delay slot, runs
 * before the routine's first instruction. A routine returns by jumping to
 * R11 (JMP R11, MOVE R15,R11, or a pull of a saved R11 into R15), again
 * with one delay-slot instruction. The core reports each instruction about
 * to run with the address just past the last byte the previous instruction
 * fetched. A routine with C is entered when the Super FX is about to run its
 * first instruction, arrived at by a jump (not by falling through), with R11
 * pointing just past the instruction that ran last (the delay slot of a
 * LINK call). A prefix (WITH, TO, FROM, ALT1-3) in that delay slot applies
 * to the routine's first instruction: the C gets it in gsu.pre_*, and the
 * prefix state the original leaves at the return is restored after it. The call ends
 * when the Super FX is about to run the instruction at that R11 in the same
 * program bank: R15 is then one past it and the pipeline holds its opcode.
 * A tail branch into a routine (no LINK) is not an entry; the routine that
 * branches has to carry it in its C. A program the S-CPU starts (by writing
 * R15) is entered at its first instruction, the one after the NOP that the
 * last STOP left in the pipeline, and ends when the Super FX is about to run
 * its STOP: the C does everything before the STOP, the core runs the STOP.
 *
 * A5. The original runs untouched. At a call's entry the Super FX's state
 * is taken (registers, flags, colour, plot options, banks, the last RAM
 * address, both pixel caches, a RAM store still in the RAM buffer); its RAM
 * writes are recorded with the values they replaced. When it returns, the C
 * runs against the entry state (the call's writes undone, the buffered store
 * counted as done) and must leave the same registers R0-R14, flags, colour,
 * options, banks, last RAM address, pixel caches, ROM buffer and RAM (a
 * store still buffered at the return counted as done). A call during which
 * the S-CPU or a DMA wrote Super FX RAM, and which differs, is INCONCLUSIVE.
 *
 * A7. The original side records each call of a routine with C as the
 * sequence of what the Super FX did: clock steps, RAM reads and writes,
 * code-cache line fills and cache flushes, and at its return the state of
 * the ROM and RAM buffers and of any prefix. The hybrid runs the C on the
 * live Super FX at the entry: each RAM access of the C first replays the
 * original's steps up to the original's matching access (same kind, same
 * byte), then is made for real, so the RAM changes at the same Super FX
 * clock and the S-CPU, the screen DMA and the frame comparison see the same
 * bytes at the same times. After the C it replays the rest of the steps,
 * repeats the cache fills and flushes, and leaves the Super FX as the
 * original was at the return: the C's registers and flags, R15 and the
 * pipeline at the return address, the buffers as recorded. The Super FX
 * then finishes its program at the same clock and the S-CPU, which polls
 * its status, goes on at the same moment.
 */
#include <dlfcn.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "gsu.h"
#include "gsucall.h"
#include "gsu_clock.h"

RegsGSU gsu;

/* ---- the core's Super FX interface (emulator/sfxbox.hpp) ---- */

struct PixelCache { uint16_t offset; uint8_t bitpend; uint8_t data[8]; };
struct GsuState {
    uint16_t r[16];
    uint16_t sfr;
    uint8_t pbr, rombr, rambr, scbr, scmr, colr, por, cfgr, clsr, pipeline, sreg, dreg;
    uint16_t cbr, ramaddr;
    uint8_t romcl, romdr, ramcl, ramdr;
    uint16_t ramar;
    uint32_t pipe_addr, last_byte;
    struct PixelCache pixel[2];
};
struct GsuCallbacks {
    void (*insn)(uint32_t pc, uint32_t prev_end, uint8_t opcode);
    void (*step)(uint32_t clocks);
    void (*ram)(int write, uint32_t addr, uint8_t data);
    void (*cache)(int kind, uint32_t value);
};

enum { SFR_Z = 1 << 1, SFR_CY = 1 << 2, SFR_S = 1 << 3, SFR_OV = 1 << 4, SFR_R = 1 << 6,
       SFR_PREFIX = 1 << 8 | 1 << 9 | 1 << 12 };

static void (*core_get)(struct GsuState *);
static void (*core_put)(const struct GsuState *);
static void (*core_time)(uint32_t);
static uint8_t (*core_bus)(int, uint32_t, uint8_t);
static void (*core_fill)(uint32_t);
static void (*core_flush)(uint32_t);
static uint8_t (*core_fetch)(uint16_t);
static void (*core_set_cb)(const struct GsuCallbacks *, int (*)(uint32_t, uint32_t));
static uint32_t (*core_cache_valid)(void);

static uint8_t *ram, *rom;
static uint32_t ram_n, rom_n;

typedef struct { uint32_t addr; void (*abi)(void); const char *name; } Routine;
static Routine *rt;
static int nrt;
static uint8_t *bits;

static int routine_at(uint32_t pc)
{
    if (!bits || !(bits[pc >> 3] >> (pc & 7) & 1)) return -1;
    for (int i = 0; i < nrt; i++) if (rt[i].addr == pc) return i;
    return -1;
}

int gsu_count(void) { return nrt; }

int gsu_load(void *core, void *lib, const char *only)
{
    const Routine *table = dlsym(lib, "sfx_gsu_routine_table");
    const int *count = dlsym(lib, "sfx_gsu_routine_count");
    if (!table || !count || !*count) return 0;
    rt = calloc((size_t)*count, sizeof *rt);
    for (int i = 0; i < *count; i++) {
        if (only) {
            char buf[16]; snprintf(buf, sizeof buf, "%06X", table[i].addr);
            if (!strstr(only, buf)) continue;
        }
        rt[nrt++] = table[i];
    }
    if (!nrt) return 0;
    *(void **)&core_get = dlsym(core, "sfxbox_gsu_get");
    *(void **)&core_put = dlsym(core, "sfxbox_gsu_put");
    *(void **)&core_time = dlsym(core, "sfxbox_gsu_time");
    *(void **)&core_bus = dlsym(core, "sfxbox_gsu_bus");
    *(void **)&core_fill = dlsym(core, "sfxbox_gsu_fill");
    *(void **)&core_flush = dlsym(core, "sfxbox_gsu_flush");
    *(void **)&core_fetch = dlsym(core, "sfxbox_gsu_fetch");
    *(void **)&core_set_cb = dlsym(core, "sfxbox_gsu_set_callbacks");
    *(void **)&core_cache_valid = dlsym(core, "sfxbox_gsu_cache_valid");
    uint8_t *(*mem)(const char *, size_t *) = dlsym(core, "sfxbox_memory");
    if (!core_get || !core_put || !core_time || !core_bus || !core_fill || !core_flush || !core_fetch || !core_set_cb || !mem) {
        fprintf(stderr, "the library has Super FX routines and the core has no Super FX hooks "
                "(tools/toolchains/patches/bsnes/0002-sfxbox-gsu-hooks.patch): rebuild it with ./setup.sh bsnes\n");
        nrt = 0;
        return -1;
    }
    size_t n;
    ram = mem("gsuram", &n); ram_n = (uint32_t)n;
    rom = mem("rom", &n); rom_n = (uint32_t)n;
    bits = calloc(1 << 21, 1);
    for (int i = 0; i < nrt; i++) bits[rt[i].addr >> 3] |= (uint8_t)(1 << (rt[i].addr & 7));
    return nrt;
}

/* A program the S-CPU starts: after a STOP (and at power-on) the Super FX
   is halted; the S-CPU's write of R15 starts it with the NOP the STOP left
   in the pipeline, and the instruction after that NOP is the program's
   first. Returns 1 for that instruction. Every side calls this for every
   instruction it is told of, so all agree. */
static int halted = 1, starting;
static int program_start(uint8_t opcode)
{
    int start = 0;
    if (halted) { halted = 0; starting = 1; }
    else if (starting) { starting = 0; start = 1; }
    if (opcode == 0x00) halted = 1;
    return start;
}

#define PROGRAM 0xFFFFFFFFu   /* the "return address" of a program: it ends at its STOP */

/* Entered as a routine: see the top of the file. */
static int entry_ok(uint32_t pc, uint32_t prev_end, const struct GsuState *st)
{
    if (pc == prev_end) return 0;
    return st->r[11] == (uint16_t)prev_end;
}

/* ---- the Super FX's bus as the C sees it ---- */

static int is_rom(uint32_t a) { return (a >> 16 & 0xFF) < 0x60; }
static int is_ram(uint32_t a) { uint32_t b = a >> 16 & 0xFF; return b >= 0x60 && b < 0x80; }

static uint32_t rom_off(uint32_t a)
{
    uint32_t bank = a >> 16 & 0xFF;
    if (bank < 0x40) return ((bank & 0x3F) << 15 | (a & 0x7FFF)) % rom_n;
    return (a & 0x3FFFFF) % rom_n;
}

static uint32_t ram_off(uint32_t a) { return a & (ram_n - 1); }

static int mode;   /* 0 off, 1 the A5 sandbox, 2 live (lockstep hybrid) */
static uint8_t sandbox_read(uint32_t off);
static void sandbox_write(uint32_t off, uint8_t v);
static uint8_t live_ram(int write, uint32_t a, uint8_t v);
static jmp_buf c_abort;
static int c_running, c_bad;
static long c_accesses;

static void count_access(void)
{
    if (mode == 1 && c_running && ++c_accesses > (1L << 22)) { c_bad = 2; longjmp(c_abort, 1); }
}

/* the Super FX's time and pixel caches (below) */
static GsuClock gclk;
static int in_pixels;   /* a pixel cache's own access: not through the RAM buffer */

uint8_t gsu_rd8(uint32_t a)
{
    a &= 0xFFFFFF;
    count_access();
    if (is_rom(a)) return rom[rom_off(a)];
    if (!is_ram(a)) { if (mode == 1) { c_bad = 1; if (c_running) longjmp(c_abort, 1); } return 0; }
    if (!in_pixels) gsuclk_ram_read(&gclk);
    if (mode == 1) return sandbox_read(ram_off(a));
    if (mode == 2) return live_ram(0, a, 0);
    return ram[ram_off(a)];
}

void gsu_wr8(uint32_t a, uint8_t v)
{
    a &= 0xFFFFFF;
    count_access();
    if (!is_ram(a)) { if (mode == 1) c_bad = 1; return; }
    if (!in_pixels) gsuclk_ram_write(&gclk);
    if (mode == 1) sandbox_write(ram_off(a), v);
    else if (mode == 2) live_ram(1, a, v);
    else ram[ram_off(a)] = v;
}

/* ---- the pixel caches and the Super FX's time: the port's model
   (ports/common/snes/gsu_clock.c), which here also times the C against the
   original ---- */

static uint8_t pix_read(GsuClock *c, uint32_t a) { (void)c; in_pixels = 1; uint8_t v = gsu_rd8(a); in_pixels = 0; return v; }
static void pix_write(GsuClock *c, uint32_t a, uint8_t v) { (void)c; in_pixels = 1; gsu_wr8(a, v); in_pixels = 0; }

static unsigned scmr_ht(void) { return (gsu.scmr & 0x20 ? 2u : 0u) | (gsu.scmr & 0x04 ? 1u : 0u); }
static unsigned scmr_md(void) { return gsu.scmr & 3u; }

static GsuScreen screen(void)
{
    GsuScreen s = { (uint8_t)(gsu.por & 0x10 ? 3 : scmr_ht()), (uint8_t)scmr_md(), gsu.scbr };
    return s;
}

void gsu_plot(uint8_t x, uint8_t y)
{
    if (!(gsu.por & 0x01)) {
        if (scmr_md() == 3) {
            if (gsu.por & 0x08) { if ((gsu.colr & 0x0F) == 0) return; }
            else if (gsu.colr == 0) return;
        } else if ((gsu.colr & 0x0F) == 0) return;
    }
    uint8_t color = gsu.colr;
    if ((gsu.por & 0x02) && scmr_md() != 3) {
        if ((x ^ y) & 1) color >>= 4;
        color &= 0x0F;
    }
    GsuScreen s = screen();
    gsuclk_plot(&gclk, &s, x, y, color);
}

uint8_t gsu_rpix(uint8_t x, uint8_t y)
{
    GsuScreen s = screen();
    return gsuclk_rpix(&gclk, &s, x, y);
}

/* the code the C stated in the interval being timed (the ops log's) */
static uint32_t mfetch[1 << 16], mfn;
static int mfetch_on;

void gsu_code(uint32_t addr, uint32_t n)
{
    if (mfetch_on)
        for (uint32_t i = 0; i < n && mfn < (uint32_t)(sizeof mfetch / sizeof mfetch[0]); i++)
            mfetch[mfn++] = (addr & 0xFF0000) | ((addr + i) & 0xFFFF);
    gsuclk_code(&gclk, addr, n);
}

void gsu_cache(uint16_t r15)
{
    if (mfetch_on && mfn < (uint32_t)(sizeof mfetch / sizeof mfetch[0])) mfetch[mfn++] = 0x80000000u | r15;
    gsuclk_cache(&gclk, r15);
}
void gsu_cyc(uint32_t clocks) { gsuclk_step(&gclk, GSUCLK_INSN, clocks); }

void gsu_time(int what)
{
    if (what == GSU_T_MULT) gsuclk_mult(&gclk);
    else if (what == GSU_T_FMULT) gsuclk_fmult(&gclk);
}

static void buffer_placed(int getb);   /* below */

void gsu_buffer(int what)
{
    switch (what) {
    case GSU_B_R14: buffer_placed(0); gsuclk_r14(&gclk); break;
    case GSU_B_ROMBUF: buffer_placed(1); gsuclk_rombuf(&gclk); break;
    case GSU_B_RAMBUF: gsuclk_rambuf(&gclk); break;
    }
}

/* ---- moving state between the core and the C ---- */

static void model_from(const struct GsuState *st)
{
    memcpy(gsu.r, st->r, sizeof gsu.r);
    gsu.z = !!(st->sfr & SFR_Z); gsu.cy = !!(st->sfr & SFR_CY); gsu.s = !!(st->sfr & SFR_S); gsu.ov = !!(st->sfr & SFR_OV);
    gsu.rombr = st->rombr; gsu.rambr = st->rambr; gsu.colr = st->colr; gsu.por = st->por;
    gsu.scmr = st->scmr; gsu.scbr = st->scbr; gsu.ramaddr = st->ramaddr;
    gsu.pre_b = !!(st->sfr & 1 << 12); gsu.pre_alt1 = !!(st->sfr & 1 << 8); gsu.pre_alt2 = !!(st->sfr & 1 << 9);
    gsu.pre_sreg = st->sreg; gsu.pre_dreg = st->dreg;
    /* The ROM buffer; a fill still pending lands before anything reads it,
       with the R14 and ROMBR of now (ROMB completes a pending fill first). */
    gsu.romdr = st->romcl ? rom[rom_off((uint32_t)st->rombr << 16 | st->r[14])] : st->romdr;
    for (int i = 0; i < 2; i++) {
        gclk.pix[i].offset = st->pixel[i].offset;
        gclk.pix[i].bitpend = st->pixel[i].bitpend;
        memcpy(gclk.pix[i].data, st->pixel[i].data, 8);
    }
    gclk.pix_read = pix_read;
    gclk.pix_write = pix_write;
}

/* A call that returned two levels up (04:C72F): it has just run a pull
   return (TO R15; LDW (R10); NOP, ending at prev_end) with R10 two words
   or more below where the call was entered, and the address it jumped to
   is the word it pulled (it took its caller's frame and returned through
   the return address saved under it). Code that uses R10 as an ordinary
   register (the rasterizer) does not return that way. */
static int two_up(uint32_t pc, uint32_t prev_end, uint16_t entry_r10, const struct GsuState *now)
{
    if ((int16_t)(entry_r10 - now->r[10]) < 4) return 0;
    uint32_t b = prev_end & 0xFF0000, o = prev_end & 0xFFFF;
    if (rom[rom_off(b | ((o - 3) & 0xFFFF))] != 0x1F || rom[rom_off(b | ((o - 2) & 0xFFFF))] != 0x4A
        || rom[rom_off(b | ((o - 1) & 0xFFFF))] != 0x01) return 0;
    uint32_t a = 0x700000u + ((uint32_t)now->rambr << 16) + now->r[10];
    return (pc & 0xFFFF) == (uint32_t)(ram[ram_off(a)] | ram[ram_off(a + 1)] << 8);
}

static uint16_t model_flags(uint16_t sfr)
{
    sfr &= (uint16_t)~(SFR_Z | SFR_CY | SFR_S | SFR_OV);
    return (uint16_t)(sfr | (gsu.z ? SFR_Z : 0) | (gsu.cy ? SFR_CY : 0) | (gsu.s ? SFR_S : 0) | (gsu.ov ? SFR_OV : 0));
}


/* ======================================================================
 * A5: the per-call oracle
 * ====================================================================== */

typedef struct { uint8_t *v; uint32_t *g; } ByteMap;
typedef struct {
    int idx, foreign;
    uint32_t ret, gen;
    struct GsuState entry;
    int pend; uint32_t pend_off; uint8_t pend_val;   /* a store still buffered at entry */
    ByteMap undo, orig, cw;
    uint32_t *touched; uint32_t ntouched, cap;
} Watch;

#define DEPTH 6
static Watch ws[DEPTH];
static int depth;
static Watch *cur;
static FILE *report;
typedef struct { long calls, match, diff, inconclusive, skipped; int shown; } Stat;
static Stat *stats;

/* A state loaded (sfxbox load, or a unit call's machine put back): the
   Super FX is stopped or in the middle of a program, whatever the
   instructions seen before said (program_start goes by them). Calls the
   per-call checks had open went with the machine they ran on: not judged. */
void gsu_state_loaded(int running)
{
    halted = !running;
    starting = 0;
    if (stats)   /* the per-call checks are on (mode is 1 only while a C runs) */
        while (depth) stats[ws[--depth].idx].skipped++;
}

static void bm_init(ByteMap *m) { m->v = calloc(ram_n, 1); m->g = calloc(ram_n, sizeof *m->g); }
static int bm_get(const ByteMap *m, uint32_t gen, uint32_t off, uint8_t *v) { if (m->g[off] != gen) return 0; *v = m->v[off]; return 1; }
static void bm_put(ByteMap *m, uint32_t gen, uint32_t off, uint8_t v, int keep)
{
    if (m->g[off] == gen && keep) return;
    m->g[off] = gen; m->v[off] = v;
}
static void touch(Watch *w, uint32_t off)
{
    if (w->ntouched == w->cap) {
        w->cap = w->cap ? w->cap * 2 : 1024;
        w->touched = realloc(w->touched, w->cap * sizeof *w->touched);
        if (!w->touched) abort();
    }
    w->touched[w->ntouched++] = off;
}

static uint8_t entry_value(Watch *w, uint32_t off)
{
    uint8_t v;
    if (w->pend && off == w->pend_off) return w->pend_val;
    if (bm_get(&w->undo, w->gen, off, &v)) return v;
    return ram[off];
}

static uint8_t sandbox_read(uint32_t off)
{
    uint8_t v;
    if (bm_get(&cur->cw, cur->gen, off, &v)) return v;
    return entry_value(cur, off);
}

static void sandbox_write(uint32_t off, uint8_t v)
{
    if (cur->cw.g[off] != cur->gen) touch(cur, off);
    bm_put(&cur->cw, cur->gen, off, v, 0);
}

static void finish(Watch *w)
{
    Stat *st = &stats[w->idx];
    const Routine *r = &rt[w->idx];
    struct GsuState fin;
    core_get(&fin);
    st->calls++;

    model_from(&w->entry);
    cur = w;
    mode = 1; c_running = 1; c_bad = 0; c_accesses = 0;
    if (setjmp(c_abort) == 0) r->abi();
    c_running = 0; mode = 0;

    char why[512] = "";
    int bad = 0;
#define WHY(...) do { bad = 1; if (strlen(why) < 400) snprintf(why + strlen(why), sizeof why - strlen(why), __VA_ARGS__); } while (0)
    for (int i = 0; i < 15; i++)
        if (gsu.r[i] != fin.r[i]) WHY(" R%d=%04X/%04X", i, gsu.r[i], fin.r[i]);
    uint16_t f = model_flags(fin.sfr);
    if ((f ^ fin.sfr) & (SFR_Z | SFR_CY | SFR_S | SFR_OV)) WHY(" flags=%02X/%02X", f & 0x1E, fin.sfr & 0x1E);
    if (gsu.colr != fin.colr) WHY(" colr=%02X/%02X", gsu.colr, fin.colr);
    if (gsu.por != fin.por) WHY(" por=%02X/%02X", gsu.por, fin.por);
    if (gsu.rombr != fin.rombr) WHY(" rombr=%02X/%02X", gsu.rombr, fin.rombr);
    if (gsu.rambr != fin.rambr) WHY(" rambr=%02X/%02X", gsu.rambr, fin.rambr);
    if (gsu.ramaddr != fin.ramaddr) WHY(" ramaddr=%04X/%04X", gsu.ramaddr, fin.ramaddr);
    if (!fin.romcl && gsu.romdr != fin.romdr) WHY(" romdr=%02X/%02X", gsu.romdr, fin.romdr);
    for (int i = 0; i < 2; i++) {
        struct GsuPixels *a = &gclk.pix[i];
        struct PixelCache *b = &fin.pixel[i];
        int same = a->offset == b->offset && a->bitpend == b->bitpend;
        for (int k = 0; k < 8 && same; k++) if (a->bitpend >> k & 1) same = a->data[k] == b->data[k];
        if (!same) WHY(" pixelcache%d", i);
    }
    /* The store the original still has in its RAM buffer at the return
       lands a few clocks later: counted as done. */
    uint32_t exit_off = fin.ramcl ? ram_off(0x700000u + ((uint32_t)fin.rambr << 16) + fin.ramar) : 0xFFFFFFFF;
    if (exit_off != 0xFFFFFFFF) touch(w, exit_off);
    for (uint32_t i = 0; i < w->ntouched; i++) {
        uint32_t off = w->touched[i];
        uint8_t ov, cv;
        if (off == exit_off) ov = fin.ramdr;
        else if (!bm_get(&w->orig, w->gen, off, &ov)) ov = entry_value(w, off);
        if (!bm_get(&w->cw, w->gen, off, &cv)) cv = entry_value(w, off);
        if (ov != cv) WHY(" [70:%05X]=%02X/%02X", off, cv, ov);
    }
    if (c_bad == 1) WHY(" access-off-the-bus");
    if (c_bad == 2) WHY(" runaway");
    if (!bad) { st->match++; return; }
    if (w->foreign) st->inconclusive++; else st->diff++;
    if (st->shown < 8) {
        st->shown++;
        const struct GsuState *e = &w->entry;
        fprintf(report, "%s %06X %s call %ld: entry R0=%04X R1=%04X R2=%04X R3=%04X R4=%04X R5=%04X R6=%04X R7=%04X"
                " R8=%04X R9=%04X R10=%04X R11=%04X R12=%04X R13=%04X R14=%04X SFR=%04X%s; C/orig:%s\n",
                w->foreign ? "INCONCLUSIVE" : "DIFF", r->addr, r->name, st->calls, e->r[0], e->r[1], e->r[2], e->r[3],
                e->r[4], e->r[5], e->r[6], e->r[7], e->r[8], e->r[9], e->r[10], e->r[11], e->r[12], e->r[13], e->r[14],
                e->sfr, w->foreign ? " (the S-CPU wrote Super FX RAM)" : "", why);
    }
}

static void check_insn(uint32_t pc, uint32_t prev_end, uint8_t opcode)
{
    int prog = program_start(opcode);
    /* A watched call ends at its return address, or where it lands when it
       returned two levels up (two_up), as its C does; that can be the
       return address of the watched call around it, which then ends there
       too. */
    while (depth) {
        Watch *w = &ws[depth - 1];
        if (pc != w->ret) {
            if (pc == prev_end || w->ret == PROGRAM) break;
            struct GsuState now;
            core_get(&now);
            if (!two_up(pc, prev_end, w->entry.r[10], &now)) break;
        }
        finish(w);
        depth--;
    }
    if (depth && opcode == 0x00) {
        /* STOP: a watched program ends here; a watched call inside it
           ended with the program, before it returned, and is not judged. */
        while (depth && ws[depth - 1].ret != PROGRAM) stats[ws[--depth].idx].skipped++;
        if (depth) { finish(&ws[depth - 1]); depth--; }
    }
    int idx = routine_at(pc);
    if (idx < 0) return;
    struct GsuState st;
    core_get(&st);
    if (!(prog || entry_ok(pc, prev_end, &st)) || depth == DEPTH) { stats[idx].skipped++; return; }
    /* SFXBOX_GSU_ENTRY=addr: print the registers at each entry of that
       routine (for writing unit tests that reach its other paths). */
    {
        static long want = -2;
        if (want == -2) want = getenv("SFXBOX_GSU_ENTRY") ? (long)strtoul(getenv("SFXBOX_GSU_ENTRY"), NULL, 16) : -1;
        if (want == (long)pc) {
            fprintf(stderr, "GSU ENTRY %06X:", pc);
            for (int r = 0; r < 16; r++) fprintf(stderr, " r%d=%04X", r, st.r[r]);
            fprintf(stderr, " rambr=%02X\n", st.rambr);
        }
    }
    Watch *w = &ws[depth++];
    w->idx = idx; w->foreign = 0; w->gen++; w->ntouched = 0;
    w->ret = prog ? PROGRAM : (pc & 0xFF0000) | st.r[11];
    w->entry = st;
    w->pend = st.ramcl != 0;
    if (w->pend) { w->pend_off = ram_off(0x700000u + ((uint32_t)st.rambr << 16) + st.ramar); w->pend_val = st.ramdr; }
}

static void check_ram(int write, uint32_t addr, uint8_t data)
{
    if (!write || !depth) return;
    uint32_t off = ram_off(addr);
    for (int i = 0; i < depth; i++) {
        Watch *w = &ws[i];
        bm_put(&w->undo, w->gen, off, ram[off], 1);
        if (w->orig.g[off] != w->gen) touch(w, off);
        bm_put(&w->orig, w->gen, off, data, 0);
    }
}

void gsu_check_foreign(uint32_t addr)
{
    if (!depth) return;
    uint32_t off = ram_off(addr);
    for (int i = 0; i < depth; i++) {
        bm_put(&ws[i].undo, ws[i].gen, off, ram[off], 1);
        ws[i].foreign = 1;
    }
}

void gsu_check_begin(FILE *rep)
{
    if (!nrt) return;
    report = rep;
    stats = calloc((size_t)nrt, sizeof *stats);
    for (int i = 0; i < DEPTH; i++) { bm_init(&ws[i].undo); bm_init(&ws[i].orig); bm_init(&ws[i].cw); }
    static struct GsuCallbacks cb = {check_insn, NULL, check_ram, NULL};
    core_set_cb(&cb, NULL);
}

void gsu_check_rows(FILE *rep)
{
    for (int i = 0; i < nrt; i++) {
        Stat *s = &stats[i];
        const char *v = s->diff ? "DIVERGENT" : s->calls == 0 ? "UNCOVERED" : s->inconclusive ? "UNVERIFIED" : "IDENTICAL";
        fprintf(rep, "%06X,%s,%ld,%ld,%ld,%ld,%ld,%s\n", rt[i].addr, rt[i].name, s->calls, s->match, s->diff,
                s->inconclusive, s->skipped, v);
        printf("%06X %-28s calls %ld match %ld diff %ld inconclusive %ld  %s\n", rt[i].addr, rt[i].name, s->calls,
               s->match, s->diff, s->inconclusive, v);
    }
}

/* ======================================================================
 * A7: lockstep
 * ====================================================================== */

#define NSLOT 128
#define CALLRING (1 << 20)
#define OPRING (1u << 24)

enum { OP_STEP = 1, OP_READ = 2, OP_WRITE = 3, OP_FILL = 4, OP_FLUSH = 5, OP_FLAGS = 6, OP_ISSUE = 7, OP_FETCH = 8,
       OP_R14 = 9, OP_GETB = 10, OP_CACHEI = 11 };

/* A step's clocks, and (core patch 0007, in the top byte) what it is for:
   STEP_INSN an instruction, the others the waits and transfers the
   hardware makes around them (the ROM and RAM buffers, the bus while the
   S-CPU has it, a code-cache fill, a pixel-cache flush, RPIX). */
enum { STEP_INSN = 0, STEP_ROMBUF = 1, STEP_RAMBUF = 2, STEP_BUS = 3, STEP_FILL = 4, STEP_FLUSH = 5, STEP_RPIX = 6 };
#define STEP_CLOCKS(a) ((a) & 0xFFFFFFu)
#define STEP_KIND(a) ((a) >> 24)

/* The status bits an instruction sets, which the S-CPU can read at $3030
   while the call runs: Z, CY, S, OV, R, ALT1, ALT2 and B. The C keeps its
   flags in the model until it returns, so the hybrid's core shows them as
   the original had them at each point of the call (OP_FLAGS). */
#define SFR_SHOWN (SFR_Z | SFR_CY | SFR_S | SFR_OV | SFR_R | SFR_PREFIX)

typedef struct {
    long frame;
    uint64_t opoff;
    uint32_t opn;
    int done, stopped, up;   /* up: it returned two levels up, at end */
    uint8_t romcl, ramcl;
    uint16_t ramar, prefix;
    uint8_t sreg, dreg;
    uint32_t last_byte, end;   /* the address the call ended at (a program's STOP) */
} CallRec;

typedef struct {
    uint64_t o_calls, h_calls, o_ops, h_ops;
    int o_done;
    uint32_t fcalls[NSLOT];
    CallRec calls[CALLRING];
    uint8_t ops[OPRING];
    uint32_t arg[OPRING];
} Shared;

static Shared *sh;
static int role;
static volatile int *stuck;
static FILE *lsrep;
static long frame_now;

#define LOAD(x) __atomic_load_n(&(x), __ATOMIC_SEQ_CST)
#define STORE(x, v) __atomic_store_n(&(x), (v), __ATOMIC_SEQ_CST)
#define WAIT_UNTIL(cond) do { long spins_ = 0; while (!(cond)) { \
        if (*stuck || (role == 2 && getppid() == 1)) _exit(6); \
        lockstep_drain(); \
        if (++spins_ > 600000) { \
            fprintf(stderr, "lockstep: %s side stuck in frame %ld waiting for Super FX %s; calls o/h %llu/%llu\n", \
                    role == 1 ? "original" : "hybrid", frame_now, #cond, (unsigned long long)LOAD(sh->o_calls), \
                    (unsigned long long)LOAD(sh->h_calls)); \
            *stuck = 1; _exit(6); } usleep(50); } } while (0)

int gsu_lockstep_prepare(void)
{
    if (!nrt) return 0;
    sh = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    return sh == MAP_FAILED ? -1 : 0;
}

/* ---- original side: record each call ---- */

static struct { int active; uint32_t ret; uint64_t call; uint16_t flags, r10; } m;

static void op(uint8_t kind, uint32_t arg)
{
    uint64_t n = LOAD(sh->o_ops);
    WAIT_UNTIL(n - LOAD(sh->h_ops) < OPRING);
    sh->ops[n % OPRING] = kind;
    sh->arg[n % OPRING] = arg;
    sh->calls[m.call % CALLRING].opn++;
    STORE(sh->o_ops, n + 1);
}

/* Asked by the original side before each frame: whether it may run on.
   It runs ahead of the hybrid by up to NSLOT frames, and every Super FX
   access it makes then waits in the op ring until the hybrid takes the
   call. A long program left open (the frame program runs over several
   frames) with that backlog can fill the ring: the original then waits
   inside the call for room, the hybrid for the call's end, and neither
   moves. So with the ring half full the original waits at the frame's
   start until the hybrid has taken every call before the open one (all
   it can take, so the wait always ends). */
int gsu_lockstep_ring_room(void)
{
    if (role != 1 || !sh) return 1;
    if (LOAD(sh->o_ops) - LOAD(sh->h_ops) < OPRING / 2) return 1;
    return LOAD(sh->h_calls) >= (m.active ? m.call : LOAD(sh->o_calls));
}

static int fetch_ops;
static void orig_insn(uint32_t pc, uint32_t prev_end, uint8_t opcode)
{
    int prog = program_start(opcode);
    if (m.active) {
        /* a CACHE run (the ops log: where the C has to state it) */
        if (fetch_ops && opcode == 0x02) op(OP_CACHEI, pc);
        CallRec *rec = &sh->calls[m.call % CALLRING];
        {
            struct GsuState fs;
            core_get(&fs);
            if ((fs.sfr & SFR_SHOWN) != m.flags) {
                m.flags = fs.sfr & SFR_SHOWN;
                op(OP_FLAGS, m.flags);
            }
            /* Two levels up (as in check_insn): the call ends where it
               lands. */
            if (m.ret != PROGRAM && pc != prev_end && two_up(pc, prev_end, m.r10, &fs)) {
                rec->up = 1;
                m.ret = pc;
            }
        }
        if (pc == m.ret || (m.ret == PROGRAM && opcode == 0x00)) {
            rec->end = pc;
            struct GsuState st;
            core_get(&st);
            rec->romcl = st.romcl; rec->ramcl = st.ramcl; rec->ramar = st.ramar;
            rec->prefix = st.sfr & SFR_PREFIX; rec->sreg = st.sreg; rec->dreg = st.dreg;
            rec->last_byte = st.last_byte;
            STORE(rec->done, 1);
            m.active = 0;
        } else if (opcode == 0x00) {
            rec->end = pc;
            rec->stopped = 1;
            STORE(rec->done, 1);
            m.active = 0;
        }
        return;
    }
    if (routine_at(pc) < 0) return;
    struct GsuState st;
    core_get(&st);
    if (!(prog || entry_ok(pc, prev_end, &st))) return;
    uint64_t k = LOAD(sh->o_calls);
    WAIT_UNTIL(k - LOAD(sh->h_calls) < CALLRING);
    CallRec *rec = &sh->calls[k % CALLRING];
    rec->frame = frame_now; rec->opoff = LOAD(sh->o_ops); rec->opn = 0; rec->stopped = 0; rec->up = 0;
    STORE(rec->done, 0);
    STORE(sh->o_calls, k + 1);
    sh->fcalls[frame_now % NSLOT]++;
    m.active = 1; m.ret = prog ? PROGRAM : (pc & 0xFF0000) | st.r[11]; m.call = k; m.flags = st.sfr & SFR_SHOWN;
    m.r10 = st.r[10];
}

static void orig_step(uint32_t clocks) { if (m.active) op(OP_STEP, clocks); }
static void orig_ram(int write, uint32_t addr, uint8_t data) { (void)data; if (m.active) op(write ? OP_WRITE : OP_READ, addr); }
/* kind 1 a code-cache line filled, 2 the cache flushed, 3 a store into the
   RAM buffer (core patch 0007: the RAM is written when it drains); patch
   0008: 4 a code byte about to be fetched (recorded only for the ops log,
   SFXBOX_GSUCYCLOG_OPS), 5 R14 written (the ROM buffer's fill starts), 6 a
   read of the ROM buffer (GETB, GETC) */
static void orig_cache(int kind, uint32_t v)
{
    static const uint8_t kinds[] = {0, OP_FILL, OP_FLUSH, OP_ISSUE, OP_FETCH, OP_R14, OP_GETB};
    if (!m.active || kind < 1 || kind > 6) return;
    if (kind == 4 && !fetch_ops) return;
    op(kinds[kind], v);
}

/* ---- hybrid side: run the C in place ---- */

static uint8_t *cops;
static uint32_t *carg;
static uint32_t cn, ci, ccap;
static int past_end;
static long h_calls, h_unmatched, h_stopped, h_fcalls, h_entered_other;
static long place_from = -1, place_to = -1;
static const char *cur_name;

/* Replay the recorded steps, fills and flushes in [ci, to). The original's
   RAM accesses in that range are the C's to make; one the C does not make
   is skipped (its effect, if any, shows in the comparison). */
static void replay_to(uint32_t to)
{
    for (uint32_t i = ci; i < to; i++) {
        switch (cops[i]) {
        case OP_STEP: core_time(STEP_CLOCKS(carg[i])); break;
        case OP_FILL: core_fill(carg[i]); break;
        case OP_FLUSH: core_flush(carg[i]); break;
        case OP_FLAGS: {
            struct GsuState fs;
            core_get(&fs);
            fs.sfr = (uint16_t)((fs.sfr & ~SFR_SHOWN) | carg[i]);
            core_put(&fs);
            break;
        }
        default: break;
        }
    }
    if (to > ci) ci = to;
}

/* SFXBOX_GSUCYCLOG=FILE (hybrid side, a library built by cycprobe.py): for
   each RAM access of the C placed on the original's, and each call's end,
   the probes the C passed since the one before, the clocks the original's
   Super FX spent between the two (its steps) and the clocks the C states
   (gsu_cyc), as records like check.c's SFXBOX_CYCLOG, which cycsolve.py
   reads. With or without the file, each is judged: the report's Super FX
   timing line. A probe or a stated time counts only on the Super FX's own
   thread (the S-CPU's and SPC700's C run on theirs, interleaved). */
static FILE *gcyclog;
static uint32_t gprobe[1 << 16], gpn, gfrom;
static int gover, gquiet, gcyclog_ops;
static long gtm_on, gtm_off, gtm_kind[GSUCLK_KINDS];
static uint64_t gmark[GSUCLK_KINDS];     /* the model's clocks at the interval's start */
static void *gthread;
extern void *(*sfxbox_co_active)(void);

static int on_gsu_thread(void) { return mode == 2 && sfxbox_co_active && sfxbox_co_active() == gthread; }

int gsu_probe(unsigned id)
{
    if (!gcyclog || !on_gsu_thread()) return 0;
    if (gpn < (uint32_t)(sizeof gprobe / sizeof gprobe[0])) gprobe[gpn++] = id;
    else gover = 1;
    return 1;
}

/* The interval ending at the record's op `to` (an access placed, or the
   call's end at cn): the original's steps since the access before against
   the clocks the model priced for the C's code and accesses there. The
   waits for the bus while the S-CPU has it are the original's (the C runs
   all at once at the call's start: the S-CPU's side of those times is not
   there yet), so they are left out of both. */
static void gcyc_put(uint8_t type, uint32_t to)
{
    uint32_t gap = 0, said = 0, hdr[4];
    uint64_t o[GSUCLK_KINDS] = {0};
    for (uint32_t q = gfrom; q < to; q++)
        if (cops[q] == OP_STEP && STEP_KIND(carg[q]) < GSUCLK_KINDS) o[STEP_KIND(carg[q])] += STEP_CLOCKS(carg[q]);
    uint32_t mkind[GSUCLK_KINDS];
    for (int k = 0; k < GSUCLK_KINDS; k++) {
        uint64_t mk = gclk.by[k] - gmark[k];
        mkind[k] = (uint32_t)mk;
        if (k == GSUCLK_BUS) continue;
        gap += (uint32_t)o[k];
        said += (uint32_t)mk;
        if (!gquiet && !gover && mk != o[k]) gtm_kind[k]++;
    }
    memcpy(gmark, gclk.by, sizeof gmark);
    if (!gquiet) {
        if (gover) gap = 0xFFFFFFFFu;
        else if (gap == said) gtm_on++;
        else gtm_off++;
        if (gcyclog) {
            fputc(type, gcyclog);
            hdr[0] = gap; hdr[1] = 0; hdr[2] = said; hdr[3] = gpn;
            fwrite(hdr, 4, 4, gcyclog);
            fwrite(gprobe, 4, gpn, gcyclog);
            if (gcyclog_ops) {
                /* the original's operations in the interval: kind, then
                   its argument (SFXBOX_GSUCYCLOG_OPS); its code fetches
                   among them (OP_FETCH) */
                uint32_t m = (to > gfrom ? to - gfrom : 0) + GSUCLK_KINDS + mfn;
                fwrite(&m, 4, 1, gcyclog);
                for (uint32_t q = gfrom; q < to; q++) { fputc(cops[q], gcyclog); fwrite(&carg[q], 4, 1, gcyclog); }
                /* then the model's clocks of each kind, as ops 100 + kind */
                for (int k = 0; k < GSUCLK_KINDS; k++) { fputc(100 + k, gcyclog); fwrite(&mkind[k], 4, 1, gcyclog); }
                /* and the code the C stated, as op 110 (a byte fetched) and
                   111 (a CACHE, R15) */
                for (uint32_t q = 0; q < mfn; q++) {
                    fputc(mfetch[q] >> 31 ? 111 : 110, gcyclog);
                    uint32_t a = mfetch[q] & 0x7FFFFFFFu;
                    fwrite(&a, 4, 1, gcyclog);
                }
            }
        }
    }
    gpn = 0; gover = 0; mfn = 0;
    gfrom = to + 1;
}

void gsu_timing_line(FILE *rep)
{
    if (!nrt) return;
    fprintf(rep, "super fx timing: %ld of the Super FX C's accesses and returns at the original's clocks, %ld not "
            "(bus waits as the original's); off in: instructions %ld, rom buffer %ld, ram buffer %ld, cache fills %ld, "
            "pixel flushes %ld, rpix %ld\n", gtm_on, gtm_off, gtm_kind[GSUCLK_INSN], gtm_kind[GSUCLK_ROMBUF],
            gtm_kind[GSUCLK_RAMBUF], gtm_kind[GSUCLK_FILL], gtm_kind[GSUCLK_FLUSH], gtm_kind[GSUCLK_RPIX]);
    if (gcyclog) { fclose(gcyclog); gcyclog = NULL; }
}

static uint8_t live_ram(int write, uint32_t a, uint8_t v)
{
    uint8_t want = write ? OP_WRITE : OP_READ;
    uint32_t off = ram_off(a), from = ci;
    for (uint32_t j = ci; j < cn; j++)
        if (cops[j] == want && ram_off(carg[j]) == off) {
            /* a store's time is when the instruction made it, not when the
               RAM buffer drained (the instructions after it run meanwhile) */
            uint32_t at = j;
            if (write)
                for (uint32_t q = j; q-- > gfrom;)
                    if (cops[q] == OP_ISSUE && ram_off(carg[q]) == off) { at = q; break; }
            gcyc_put('A', at);
            replay_to(j);
            ci = j + 1;
            break;
        }
    if (place_from >= 0 && frame_now >= place_from && frame_now <= place_to)
        fprintf(lsrep, "GPLACE %ld %s %s %06X %u -> %u\n", frame_now, cur_name, write ? "w" : "r", a, from, ci);
    return core_bus(write, a, v);
}

/* The C's R14 write or ROM-buffer read (gsu_buffer), placed on the
   original's next one before its next RAM access: the time up to there
   is an interval of its own (the buffer's wait depends on the code run
   between them). */
static void buffer_placed(int getb)
{
    uint8_t kind = getb ? OP_GETB : OP_R14;
    if (mode != 2 || !on_gsu_thread()) return;
    for (uint32_t q = gfrom; q < cn; q++) {
        if (cops[q] == OP_READ || cops[q] == OP_WRITE || cops[q] == OP_ISSUE) return;
        if (cops[q] == kind) { gcyc_put('A', q); return; }
    }
}

static int prog_now;
static void hybrid_insn(uint32_t pc, uint32_t prev_end, uint8_t opcode)
{
    (void)pc; (void)prev_end;
    prog_now = program_start(opcode);
}

static int hybrid_exec(uint32_t pc, uint32_t prev_end)
{
    int idx = routine_at(pc);
    if (idx < 0) return 0;
    struct GsuState st;
    core_get(&st);
    int prog = prog_now;
    if (!(prog || entry_ok(pc, prev_end, &st))) { h_entered_other++; return 0; }
    cur_name = rt[idx].name;
    uint64_t k = LOAD(sh->h_calls);
    WAIT_UNTIL((k < LOAD(sh->o_calls) && LOAD(sh->calls[k % CALLRING].done)) || LOAD(sh->o_done));
    CallRec rec = {0};
    cn = ci = 0;
    if (k < LOAD(sh->o_calls) && LOAD(sh->calls[k % CALLRING].done)) {
        rec = sh->calls[k % CALLRING];
        if (rec.frame != frame_now && !past_end) {
            h_unmatched++;
            fprintf(lsrep, "GUNMATCHED call %llu %s %06X: original frame %ld, here %ld\n",
                    (unsigned long long)k, cur_name, pc, rec.frame, frame_now);
        }
        if (rec.stopped) h_stopped++;
        cn = rec.opn;
        if (cn > ccap) {
            ccap = cn;
            cops = realloc(cops, ccap);
            carg = realloc(carg, (size_t)ccap * sizeof *carg);
            if (!cops || !carg) _exit(7);
        }
        for (uint32_t i = 0; i < cn; i++) {
            uint64_t o = (rec.opoff + i) % OPRING;
            cops[i] = sh->ops[o];
            carg[i] = sh->arg[o];
        }
        STORE(sh->h_ops, rec.opoff + rec.opn);
        STORE(sh->h_calls, k + 1);
    } else {
        past_end = 1;
        rec.last_byte = st.last_byte;
        rec.end = (pc & 0xFF0000) | st.r[11];
    }
    h_calls++;
    h_fcalls++;

    /* No buffered access is left pending in the core while the C runs: the
       ROM buffer is filled now (the C reads ROM itself), a store still in
       the RAM buffer is made at the original's matching write. */
    uint8_t entry_romcl = st.romcl, entry_ramcl = st.ramcl;
    int pend = st.ramcl != 0;
    uint32_t pend_addr = 0x700000u + ((uint32_t)st.rambr << 16) + st.ramar;
    uint8_t pend_val = st.ramdr;
    if (st.romcl) { st.romdr = rom[rom_off((uint32_t)st.rombr << 16 | st.r[14])]; st.romcl = 0; st.sfr &= (uint16_t)~SFR_R; }
    st.ramcl = 0;
    core_put(&st);
    model_from(&st);
    /* the model's state as the original's at the entry: the code cache,
       the buffers still counting (the store left buffered drains as the
       C's first code runs) */
    gclk.cbr = st.cbr; gclk.valid = core_cache_valid ? core_cache_valid() : 0;
    gclk.clsr = st.clsr & 1; gclk.ms0 = (uint8_t)(st.cfgr >> 5 & 1);
    gclk.romcl = entry_romcl; gclk.ramcl = entry_ramcl;
    gclk.bus = NULL;
    /* the call's time from its start; the store left buffered is the
       entry's, not the C's (unjudged); none past the record's end */
    gpn = 0; gover = 0; gfrom = 0; mfn = 0;
    memcpy(gmark, gclk.by, sizeof gmark);
    gthread = sfxbox_co_active ? sfxbox_co_active() : NULL;
    gquiet = past_end;
    if (gcyclog && !past_end) { uint32_t at = pc & 0xFFFFFF; fputc('S', gcyclog); fwrite(&at, 4, 1, gcyclog); }
    mode = 2;
    if (pend) { int q = gquiet; gquiet = 1; live_ram(1, pend_addr, pend_val); gquiet = q; }
    rt[idx].abi();
    gcyc_put('E', cn);
    mode = 0;
    replay_to(cn);

    struct GsuState out;
    core_get(&out);   /* cache base and lines as the replay left them */
    /* A call stops at its return address, a program at its STOP, a call
       that returned two levels up where it landed: the Super FX is about to
       run that instruction (R15 one past it). */
    uint16_t ret = (prog || rec.up) ? (uint16_t)rec.end : st.r[11];
    memcpy(out.r, gsu.r, sizeof out.r);
    out.r[15] = (uint16_t)(ret + 1);
    out.sfr = model_flags(out.sfr);
    out.sfr = (uint16_t)((out.sfr & ~SFR_PREFIX) | rec.prefix);
    out.sreg = rec.sreg; out.dreg = rec.dreg;
    out.colr = gsu.colr; out.por = gsu.por; out.rombr = gsu.rombr; out.rambr = gsu.rambr; out.ramaddr = gsu.ramaddr;
    for (int i = 0; i < 2; i++) {
        out.pixel[i].offset = gclk.pix[i].offset;
        out.pixel[i].bitpend = gclk.pix[i].bitpend;
        memcpy(out.pixel[i].data, gclk.pix[i].data, 8);
    }
    out.romcl = rec.romcl;
    out.romdr = gsu.romdr;   /* a fill still pending (romcl) refills it from R14 */
    out.sfr = (uint16_t)((out.sfr & ~SFR_R) | (rec.romcl ? SFR_R : 0));
    out.ramcl = rec.ramcl;
    out.ramar = rec.ramar;
    out.ramdr = ram[ram_off(0x700000u + ((uint32_t)gsu.rambr << 16) + rec.ramar)];
    out.pipe_addr = (pc & 0xFF0000) | ret;
    out.last_byte = rec.last_byte;
    core_put(&out);
    out.pipeline = core_fetch(ret);
    core_put(&out);
    return 1;
}

void gsu_lockstep_role(int r, FILE *rep, volatile int *stk)
{
    if (!nrt || !sh) return;
    role = r; lsrep = rep; stuck = stk;
    if (role == 2 && getenv("SFXBOX_GSUCYCLOG")) {
        gcyclog = fopen(getenv("SFXBOX_GSUCYCLOG"), "wb");
        if (!gcyclog) { perror("SFXBOX_GSUCYCLOG"); exit(2); }
        gcyclog_ops = getenv("SFXBOX_GSUCYCLOG_OPS") != NULL;
        mfetch_on = gcyclog_ops;
        if (gcyclog_ops) fputc('O', gcyclog);
    }
    fetch_ops = getenv("SFXBOX_GSUCYCLOG_OPS") != NULL;
    if (getenv("SFXBOX_LS_GPLACE")) sscanf(getenv("SFXBOX_LS_GPLACE"), "%ld,%ld", &place_from, &place_to);
    if (role == 1) {
        static struct GsuCallbacks cb = {orig_insn, orig_step, orig_ram, orig_cache};
        core_set_cb(&cb, NULL);
    } else if (role == 2) {
        static struct GsuCallbacks cb = {hybrid_insn, NULL, NULL, NULL};
        core_set_cb(&cb, hybrid_exec);
    }
}

void gsu_before_frame(long f)
{
    frame_now = f;
    if (role == 1 && sh) sh->fcalls[f % NSLOT] = 0;
    h_fcalls = 0;
}

/* The S-CPU side has run a call the original never finished (the script
   ended inside it): the Super FX programs it starts have no frame of the
   original's to be matched with. */
void gsu_lockstep_past_end(void)
{
    past_end = 1;
}

/* The script ended on the original side inside a Super FX call, which the
   C has finished: the frame it is in can no longer be compared. */
int gsu_lockstep_is_past_end(void)
{
    return past_end;
}

int gsu_after_frame(long f, char *buf, size_t n)
{
    if (role != 2 || !sh || past_end) return 0;
    uint32_t o = sh->fcalls[f % NSLOT];
    if ((uint32_t)h_fcalls != o) {
        snprintf(buf + strlen(buf), n - strlen(buf), " super fx calls %ld/%u", h_fcalls, o);
        return 1;
    }
    return 0;
}

void gsu_lockstep_done(void) { if (sh) STORE(sh->o_done, 1); }

int gsu_lockstep_summary(char *buf, size_t n)
{
    if (!nrt || !sh) { buf[0] = 0; return 0; }
    snprintf(buf, n, "; super fx calls swapped %ld, entered other ways %ld, unmatched %ld, not returning %ld",
             h_calls, h_entered_other, h_unmatched, h_stopped);
    return h_unmatched || h_stopped;
}
