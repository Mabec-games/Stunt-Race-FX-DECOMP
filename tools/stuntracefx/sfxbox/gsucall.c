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

uint8_t gsu_rd8(uint32_t a)
{
    a &= 0xFFFFFF;
    count_access();
    if (is_rom(a)) return rom[rom_off(a)];
    if (!is_ram(a)) { if (mode == 1) { c_bad = 1; if (c_running) longjmp(c_abort, 1); } return 0; }
    if (mode == 1) return sandbox_read(ram_off(a));
    if (mode == 2) return live_ram(0, a, 0);
    return ram[ram_off(a)];
}

void gsu_wr8(uint32_t a, uint8_t v)
{
    a &= 0xFFFFFF;
    count_access();
    if (!is_ram(a)) { if (mode == 1) c_bad = 1; return; }
    if (mode == 1) sandbox_write(ram_off(a), v);
    else if (mode == 2) live_ram(1, a, v);
    else ram[ram_off(a)] = v;
}

/* ---- the pixel caches, as the core keeps them (sfc/coprocessor/superfx/core.cpp) ---- */

static struct PixelCache pix[2];

static unsigned scmr_ht(void) { return (gsu.scmr & 0x20 ? 2u : 0u) | (gsu.scmr & 0x04 ? 1u : 0u); }
static unsigned scmr_md(void) { return gsu.scmr & 3u; }

static uint32_t char_addr(uint8_t x, uint8_t y)
{
    unsigned cn = 0;
    switch (gsu.por & 0x10 ? 3 : scmr_ht()) {
    case 0: cn = ((x & 0xF8u) << 1) + ((y & 0xF8u) >> 3); break;
    case 1: cn = ((x & 0xF8u) << 1) + ((x & 0xF8u) >> 1) + ((y & 0xF8u) >> 3); break;
    case 2: cn = ((x & 0xF8u) << 1) + ((x & 0xF8u) << 0) + ((y & 0xF8u) >> 3); break;
    case 3: cn = ((y & 0x80u) << 2) + ((x & 0x80u) << 1) + ((y & 0x78u) << 1) + ((x & 0x78u) >> 3); break;
    }
    unsigned md = scmr_md(), bpp = 2u << (md - (md >> 1));
    return 0x700000u + cn * (bpp << 3) + ((uint32_t)gsu.scbr << 10) + (uint32_t)(y & 7) * 2;
}

static void flush_pixels(struct PixelCache *c)
{
    if (c->bitpend == 0) return;
    uint8_t x = (uint8_t)(c->offset << 3), y = (uint8_t)(c->offset >> 5);
    uint32_t addr = char_addr(x, y);
    unsigned md = scmr_md(), bpp = 2u << (md - (md >> 1));
    for (unsigned n = 0; n < bpp; n++) {
        uint32_t byte = ((n >> 1) << 4) + (n & 1);
        uint8_t data = 0;
        for (unsigned k = 0; k < 8; k++) data |= (uint8_t)(((c->data[k] >> n) & 1) << k);
        if (c->bitpend != 0xFF) {
            data &= c->bitpend;
            data |= (uint8_t)(gsu_rd8(addr + byte) & ~c->bitpend);
        }
        gsu_wr8(addr + byte, data);
    }
    c->bitpend = 0;
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
    uint16_t offset = (uint16_t)((y << 5) + (x >> 3));
    if (offset != pix[0].offset) {
        flush_pixels(&pix[1]);
        pix[1] = pix[0];
        pix[0].bitpend = 0;
        pix[0].offset = offset;
    }
    x = (uint8_t)((x & 7) ^ 7);
    pix[0].data[x] = color;
    pix[0].bitpend |= (uint8_t)(1 << x);
    if (pix[0].bitpend == 0xFF) {
        flush_pixels(&pix[1]);
        pix[1] = pix[0];
        pix[0].bitpend = 0;
    }
}

uint8_t gsu_rpix(uint8_t x, uint8_t y)
{
    flush_pixels(&pix[1]);
    flush_pixels(&pix[0]);
    uint32_t addr = char_addr(x, y);
    unsigned md = scmr_md(), bpp = 2u << (md - (md >> 1));
    uint8_t data = 0;
    x = (uint8_t)((x & 7) ^ 7);
    for (unsigned n = 0; n < bpp; n++) {
        uint32_t byte = ((n >> 1) << 4) + (n & 1);
        data |= (uint8_t)(((gsu_rd8(addr + byte) >> x) & 1) << n);
    }
    return data;
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
    pix[0] = st->pixel[0]; pix[1] = st->pixel[1];
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
        struct PixelCache *a = &pix[i], *b = &fin.pixel[i];
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
    while (depth && pc == ws[depth - 1].ret) { finish(&ws[depth - 1]); depth--; }
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

enum { OP_STEP = 1, OP_READ = 2, OP_WRITE = 3, OP_FILL = 4, OP_FLUSH = 5, OP_FLAGS = 6 };

/* The status bits an instruction sets, which the S-CPU can read at $3030
   while the call runs: Z, CY, S, OV, R, ALT1, ALT2 and B. The C keeps its
   flags in the model until it returns, so the hybrid's core shows them as
   the original had them at each point of the call (OP_FLAGS). */
#define SFR_SHOWN (SFR_Z | SFR_CY | SFR_S | SFR_OV | SFR_R | SFR_PREFIX)

typedef struct {
    long frame;
    uint64_t opoff;
    uint32_t opn;
    int done, stopped;
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
        if (*stuck) _exit(6); \
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

static struct { int active; uint32_t ret; uint64_t call; uint16_t flags; } m;

static void op(uint8_t kind, uint32_t arg)
{
    uint64_t n = LOAD(sh->o_ops);
    WAIT_UNTIL(n - LOAD(sh->h_ops) < OPRING);
    sh->ops[n % OPRING] = kind;
    sh->arg[n % OPRING] = arg;
    sh->calls[m.call % CALLRING].opn++;
    STORE(sh->o_ops, n + 1);
}

static void orig_insn(uint32_t pc, uint32_t prev_end, uint8_t opcode)
{
    int prog = program_start(opcode);
    if (m.active) {
        CallRec *rec = &sh->calls[m.call % CALLRING];
        {
            struct GsuState fs;
            core_get(&fs);
            if ((fs.sfr & SFR_SHOWN) != m.flags) {
                m.flags = fs.sfr & SFR_SHOWN;
                op(OP_FLAGS, m.flags);
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
    rec->frame = frame_now; rec->opoff = LOAD(sh->o_ops); rec->opn = 0; rec->stopped = 0;
    STORE(rec->done, 0);
    STORE(sh->o_calls, k + 1);
    sh->fcalls[frame_now % NSLOT]++;
    m.active = 1; m.ret = prog ? PROGRAM : (pc & 0xFF0000) | st.r[11]; m.call = k; m.flags = st.sfr & SFR_SHOWN;
}

static void orig_step(uint32_t clocks) { if (m.active) op(OP_STEP, clocks); }
static void orig_ram(int write, uint32_t addr, uint8_t data) { (void)data; if (m.active) op(write ? OP_WRITE : OP_READ, addr); }
static void orig_cache(int kind, uint32_t v) { if (m.active) op(kind == 1 ? OP_FILL : OP_FLUSH, v); }

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
        case OP_STEP: core_time(carg[i]); break;
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

static uint8_t live_ram(int write, uint32_t a, uint8_t v)
{
    uint8_t want = write ? OP_WRITE : OP_READ;
    uint32_t off = ram_off(a), from = ci;
    for (uint32_t j = ci; j < cn; j++)
        if (cops[j] == want && ram_off(carg[j]) == off) {
            replay_to(j);
            ci = j + 1;
            break;
        }
    if (place_from >= 0 && frame_now >= place_from && frame_now <= place_to)
        fprintf(lsrep, "GPLACE %ld %s %s %06X %u -> %u\n", frame_now, cur_name, write ? "w" : "r", a, from, ci);
    return core_bus(write, a, v);
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
    int pend = st.ramcl != 0;
    uint32_t pend_addr = 0x700000u + ((uint32_t)st.rambr << 16) + st.ramar;
    uint8_t pend_val = st.ramdr;
    if (st.romcl) { st.romdr = rom[rom_off((uint32_t)st.rombr << 16 | st.r[14])]; st.romcl = 0; st.sfr &= (uint16_t)~SFR_R; }
    st.ramcl = 0;
    core_put(&st);
    model_from(&st);
    mode = 2;
    if (pend) live_ram(1, pend_addr, pend_val);
    rt[idx].abi();
    mode = 0;
    replay_to(cn);

    struct GsuState out;
    core_get(&out);   /* cache base and lines as the replay left them */
    /* A call stops at its return address, a program at its STOP: the
       Super FX is about to run that instruction (R15 one past it). */
    uint16_t ret = prog ? (uint16_t)rec.end : st.r[11];
    memcpy(out.r, gsu.r, sizeof out.r);
    out.r[15] = (uint16_t)(ret + 1);
    out.sfr = model_flags(out.sfr);
    out.sfr = (uint16_t)((out.sfr & ~SFR_PREFIX) | rec.prefix);
    out.sreg = rec.sreg; out.dreg = rec.dreg;
    out.colr = gsu.colr; out.por = gsu.por; out.rombr = gsu.rombr; out.rambr = gsu.rambr; out.ramaddr = gsu.ramaddr;
    out.pixel[0] = pix[0]; out.pixel[1] = pix[1];
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
