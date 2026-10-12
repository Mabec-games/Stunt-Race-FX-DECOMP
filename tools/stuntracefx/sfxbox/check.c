/*
 * check.c: the per-call oracle (gate A5).
 *
 * The original game runs untouched. When it enters a routine that has C,
 * the call is watched: every write the routine makes is recorded with the
 * value it replaced, and every hardware register read is recorded in order.
 * When the routine returns, its C runs on the same inputs: memory as it was
 * at entry (current memory with the routine's writes undone), the same
 * registers, and the original's hardware reads replayed in order. The C's
 * effect must equal the original's: the same registers out, the same final
 * memory, the same hardware writes in the same order.
 *
 * Writes made by an interrupt handler during the call are not the routine's;
 * they are undone for the entry snapshot but never compared. A call that an
 * interrupt or a DMA overlapped and that differs is INCONCLUSIVE, not a
 * divergence. Bytes below the stack pointer at entry are the routine's own
 * scratch and are not compared.
 */
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "machine.h"
#include "check.h"

/* ---- the machine model the C links against ---- */

Regs65816 cpu;

struct SfxboxRegs { uint16_t a, x, y, s, d; uint8_t db, p, e; uint32_t pc; };
struct SfxboxCallbacks {
    void (*insn)(uint32_t pc);
    void (*write)(uint32_t addr, uint8_t data, int kind);
    void (*read)(uint32_t addr, uint8_t data);
    void (*interrupt)(uint32_t resume_pc, uint16_t s);
    void (*timing)(int kind, int clocks);
};

static void (*core_regs)(struct SfxboxRegs *);
static uint8_t *core_opcode;
static uint8_t *wram, *gsuram, *rom;
static size_t wram_n, gsuram_n, rom_n;

typedef struct { uint32_t addr; void (*abi)(void); const char *name; } Routine;
static const Routine *routines;
static int nroutines;
static uint8_t *watched;  /* 16M-entry bitmap of routine entry points */

/* Normalised address: region in the top byte, offset below. */
enum { R_NONE = 0, R_WRAM = 1, R_GSURAM = 2, R_IO = 3, R_ROM = 4 };

static uint32_t norm(uint32_t a)
{
    uint32_t bank = a >> 16 & 0xFF, lo = a & 0xFFFF;
    if (bank == 0x7E || bank == 0x7F) return R_WRAM << 24 | (bank & 1) << 16 | lo;
    if (bank == 0x70 || bank == 0x71) return R_GSURAM << 24 | ((a & 0x1FFFF) % (uint32_t)gsuram_n);
    if ((bank & 0x40) == 0) {
        if (lo < 0x2000) return R_WRAM << 24 | lo;
        if (lo < 0x6000) return R_IO << 24 | lo;
        if (lo < 0x8000) return R_GSURAM << 24 | (lo - 0x6000);
        return R_ROM << 24 | (((bank & 0x3F) << 15 | (lo & 0x7FFF)) % (uint32_t)rom_n);
    }
    if ((bank & 0x7F) >= 0x40 && (bank & 0x7F) < 0x60) return R_ROM << 24 | ((a & 0x3FFFFF) % (uint32_t)rom_n);
    return R_NONE << 24 | a;
}

static uint8_t *phys(uint32_t n)
{
    uint32_t off = n & 0xFFFFFF;
    switch (n >> 24) {
    case R_WRAM: return off < wram_n ? &wram[off] : NULL;
    case R_GSURAM: return off < gsuram_n ? &gsuram[off] : NULL;
    case R_ROM: return off < rom_n ? &rom[off] : NULL;
    }
    return NULL;
}

/* ---- small address -> byte map, cleared per call ---- */

#define MAPN 8192
typedef struct { uint32_t key[MAPN]; uint8_t val[MAPN]; uint32_t gen[MAPN]; uint32_t g; int n; uint32_t order[MAPN]; } Map;

static void map_clear(Map *m) { m->g++; m->n = 0; }
static int map_find(Map *m, uint32_t k)
{
    uint32_t h = (k * 2654435761u) & (MAPN - 1);
    while (m->gen[h] == m->g) {
        if (m->key[h] == k) return (int)h;
        h = (h + 1) & (MAPN - 1);
    }
    return -1 - (int)h;
}
static int map_get(Map *m, uint32_t k, uint8_t *v) { int i = map_find(m, k); if (i < 0) return 0; *v = m->val[i]; return 1; }
static void map_put(Map *m, uint32_t k, uint8_t v, int keep)
{
    int i = map_find(m, k);
    if (i >= 0) { if (!keep) m->val[i] = v; return; }
    if (m->n >= MAPN / 2) return;  /* a call this large is reported as overflowed */
    i = -1 - i;
    m->gen[i] = m->g; m->key[i] = k; m->val[i] = v; m->order[m->n++] = (uint32_t)i;
}

typedef struct { uint32_t addr; uint8_t v; } Io;
#define ION 4096

static struct {
    int active, idx, in_irq, interrupted, dma, overflow;
    uint32_t ret_pc, irq_pc;
    uint16_t exit_s, irq_s;
    struct SfxboxRegs entry;
    Map undo, orig, cw;
    Io oread[ION], owrite[ION], cwrite[ION];
    int noread, nowrite, ncwrite, cread;
    int io_mismatch;
} w;

typedef struct { long calls, match, diff, inconclusive, skipped; int shown; } Stat;
static Stat *stats;
static FILE *report;

/* ---- the C's view: the sandbox during a check, the real bus in lockstep ---- */

static int machine_live;
static uint8_t (*core_bus_read)(uint32_t);
static void (*core_bus_write)(uint32_t, uint8_t);

uint8_t rd8(uint32_t addr)
{
    if (machine_live) return core_bus_read(addr & 0xFFFFFF);
    uint32_t n = norm(addr);
    uint8_t v;
    if (n >> 24 == R_IO) {
        if (w.cread < w.noread && w.oread[w.cread].addr == (addr & 0xFFFF)) return w.oread[w.cread++].v;
        w.io_mismatch = 1;
        return 0;
    }
    if (map_get(&w.cw, n, &v)) return v;
    if (map_get(&w.undo, n, &v)) return v;
    uint8_t *p = phys(n);
    if (!p) { w.io_mismatch = 1; return 0; }
    return *p;
}

void wr8(uint32_t addr, uint8_t value)
{
    if (machine_live) { core_bus_write(addr & 0xFFFFFF, value); return; }
    uint32_t n = norm(addr);
    if (n >> 24 == R_IO) {
        if (w.ncwrite < ION) { w.cwrite[w.ncwrite].addr = addr & 0xFFFF; w.cwrite[w.ncwrite].v = value; }
        w.ncwrite++;
        return;
    }
    if (n >> 24 == R_WRAM || n >> 24 == R_GSURAM) map_put(&w.cw, n, value, 0);
    else w.io_mismatch = 1;
}

/* ---- events from the core ---- */

static uint8_t entry_value(uint32_t n)
{
    uint8_t v;
    if (map_get(&w.undo, n, &v)) return v;
    uint8_t *p = phys(n);
    return p ? *p : 0;
}

static int is_scratch(uint32_t n)
{
    if (n >> 24 != R_WRAM) return 0;
    uint32_t off = n & 0xFFFFFF;
    return off <= w.entry.s && off + 0x100 > w.entry.s;
}

static void finish(void)
{
    Stat *st = &stats[w.idx];
    const Routine *r = &routines[w.idx];
    struct SfxboxRegs out;
    core_regs(&out);
    w.active = 0;
    st->calls++;

    cpu.a = w.entry.a; cpu.x = w.entry.x; cpu.y = w.entry.y; cpu.s = w.entry.s; cpu.d = w.entry.d;
    cpu.db = w.entry.db; cpu.p = w.entry.p; cpu.e = w.entry.e; cpu.pc = w.entry.pc;
    map_clear(&w.cw); w.ncwrite = 0; w.cread = 0; w.io_mismatch = 0;
    r->abi();

    char why[512] = "";
    int bad = 0;
#define DIFFR(f) if (cpu.f != out.f) { bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " %s=%X/%X", #f, (unsigned)cpu.f, (unsigned)out.f); }
    DIFFR(a) DIFFR(x) DIFFR(y) DIFFR(d) DIFFR(db) DIFFR(p) DIFFR(e)
    for (int pass = 0; pass < 2; pass++) {
        Map *m = pass ? &w.cw : &w.orig;
        for (int i = 0; i < m->n; i++) {
            uint32_t k = m->key[m->order[i]];
            if (is_scratch(k)) continue;
            uint8_t ov, cv;
            if (!map_get(&w.orig, k, &ov)) ov = entry_value(k);
            if (!map_get(&w.cw, k, &cv)) cv = entry_value(k);
            if (ov != cv) {
                if (!bad || strlen(why) < 400)
                    snprintf(why + strlen(why), sizeof why - strlen(why), " [%d:%06X]=%02X/%02X", k >> 24, k & 0xFFFFFF, cv, ov);
                bad = 1;
            }
        }
    }
    if (w.ncwrite != w.nowrite) { bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " io-writes %d/%d", w.ncwrite, w.nowrite); }
    else for (int i = 0; i < w.nowrite && i < ION; i++)
        if (w.cwrite[i].addr != w.owrite[i].addr || w.cwrite[i].v != w.owrite[i].v) {
            bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " io#%d %04X=%02X/%04X=%02X", i,
                              w.cwrite[i].addr, w.cwrite[i].v, w.owrite[i].addr, w.owrite[i].v); break;
        }
    if (w.cread != w.noread) { bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " io-reads %d/%d", w.cread, w.noread); }
    if (w.io_mismatch) { bad = 1; strncat(why, " unreplayable-access", sizeof why - strlen(why) - 1); }
    if (w.overflow) { bad = 1; strncat(why, " overflow", sizeof why - strlen(why) - 1); }

    if (!bad) { st->match++; return; }
    int unsure = w.interrupted || w.dma;
    if (unsure) st->inconclusive++; else st->diff++;
    if (st->shown < 8) {
        st->shown++;
        fprintf(report, "%s %06X %s call %ld: entry A=%04X X=%04X Y=%04X S=%04X D=%04X DB=%02X P=%02X%s; C/orig:%s\n",
                unsure ? "INCONCLUSIVE" : "DIFF", r->addr, r->name, st->calls, w.entry.a, w.entry.x, w.entry.y,
                w.entry.s, w.entry.d, w.entry.db, w.entry.p, w.interrupted ? " (interrupted)" : w.dma ? " (dma)" : "", why);
    }
}

static void on_insn(uint32_t pc)
{
    if (w.active) {
        if (w.in_irq) {
            if (pc != w.irq_pc) return;
            struct SfxboxRegs r; core_regs(&r);
            if (r.s == w.irq_s) w.in_irq = 0;
            return;
        }
        if (pc != w.ret_pc) return;
        struct SfxboxRegs r; core_regs(&r);
        if (r.s == w.exit_s) finish();
        return;
    }
    if (!(watched[pc >> 3] >> (pc & 7) & 1)) return;
    int idx = -1;
    for (int i = 0; i < nroutines; i++) if (routines[i].addr == pc) { idx = i; break; }
    if (idx < 0) return;
    struct SfxboxRegs r; core_regs(&r);
    uint8_t via = *core_opcode;
    uint32_t sp = r.s;
    if (sp + 3 >= wram_n || r.e) { stats[idx].skipped++; return; }
    uint16_t word = (uint16_t)(wram[(sp + 1) & 0xFFFF] | wram[(sp + 2) & 0xFFFF] << 8);
    if (via == 0x20 || via == 0xFC) {
        w.ret_pc = (pc & 0xFF0000) | (uint16_t)(word + 1);
        w.exit_s = (uint16_t)(sp + 2);
    } else if (via == 0x22) {
        w.ret_pc = (uint32_t)wram[(sp + 3) & 0xFFFF] << 16 | (uint16_t)(word + 1);
        w.exit_s = (uint16_t)(sp + 3);
    } else { stats[idx].skipped++; return; }
    w.active = 1; w.idx = idx; w.entry = r;
    w.in_irq = w.interrupted = w.dma = w.overflow = 0;
    map_clear(&w.undo); map_clear(&w.orig);
    w.noread = w.nowrite = 0;
}

static void on_write(uint32_t addr, uint8_t data, int kind)
{
    if (!w.active) return;
    uint32_t n = norm(addr);
    int ram = n >> 24 == R_WRAM || n >> 24 == R_GSURAM;
    if (ram) {
        uint8_t *p = phys(n);
        if (p) map_put(&w.undo, n, *p, 1);
        if (w.undo.n >= MAPN / 2) w.overflow = 1;
    }
    if (kind == 2) { w.dma = 1; return; }
    if (w.in_irq) return;
    if (ram) { map_put(&w.orig, n, data, 0); if (w.orig.n >= MAPN / 2) w.overflow = 1; }
    else if (n >> 24 == R_IO) {
        if (w.nowrite < ION) { w.owrite[w.nowrite].addr = addr & 0xFFFF; w.owrite[w.nowrite].v = data; }
        else w.overflow = 1;
        w.nowrite++;
    }
}

static void on_read(uint32_t addr, uint8_t data)
{
    if (!w.active || w.in_irq) return;
    if (norm(addr) >> 24 != R_IO) return;
    if (w.noread < ION) { w.oread[w.noread].addr = addr & 0xFFFF; w.oread[w.noread].v = data; w.noread++; }
    else w.overflow = 1;
}

static void on_interrupt(uint32_t resume_pc, uint16_t s)
{
    if (!w.active || w.in_irq) return;
    w.in_irq = 1; w.interrupted = 1; w.irq_pc = resume_pc; w.irq_s = s;
}

/* ---- setup and report ---- */

static int load_routines(void *core, const char *lib, const char *only)
{
    void *h = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
    if (!h) { fprintf(stderr, "dlopen %s: %s\n", lib, dlerror()); return -1; }
    const Routine *table = dlsym(h, "sfx_routine_table");
    const int *count = dlsym(h, "sfx_routine_count");
    uint8_t *(*mem)(const char *, size_t *) = dlsym(core, "sfxbox_memory");
    *(void **)&core_regs = dlsym(core, "sfxbox_regs");
    core_opcode = dlsym(core, "sfxbox_cpu_opcode");
    if (!table || !count || !mem || !core_regs || !core_opcode) { fprintf(stderr, "check: missing symbols\n"); return -1; }
    wram = mem("wram", &wram_n); gsuram = mem("gsuram", &gsuram_n); rom = mem("rom", &rom_n);
    static Routine sel[4096];
    for (int i = 0; i < *count && nroutines < 4096; i++) {
        if (only) {
            char buf[16]; snprintf(buf, sizeof buf, "%06X", table[i].addr);
            if (!strstr(only, buf)) continue;
        }
        sel[nroutines++] = table[i];
    }
    routines = sel;
    stats = calloc((size_t)(nroutines ? nroutines : 1), sizeof *stats);
    watched = calloc(1 << 21, 1);
    for (int i = 0; i < nroutines; i++) watched[routines[i].addr >> 3] |= (uint8_t)(1 << (routines[i].addr & 7));
    *(void **)&core_bus_read = dlsym(core, "sfxbox_core_bus_read");
    *(void **)&core_bus_write = dlsym(core, "sfxbox_core_bus_write");
    return core_bus_read && core_bus_write ? 0 : -1;
}

static void (*core_set_callbacks)(const struct SfxboxCallbacks *);

int check_begin(void *core, const char *lib, const char *only, const char *report_path)
{
    if (load_routines(core, lib, only)) return -1;
    *(void **)&core_set_callbacks = dlsym(core, "sfxbox_set_callbacks");
    void (*set_cb)(const struct SfxboxCallbacks *) = core_set_callbacks;
    report = fopen(report_path, "w");
    if (!report) return -1;
    static struct SfxboxCallbacks cb = {on_insn, on_write, on_read, on_interrupt};
    set_cb(&cb);
    return 0;
}

void check_end(void)
{
    fprintf(report, "routine,name,calls,match,diff,inconclusive,skipped,verdict\n");
    for (int i = 0; i < nroutines; i++) {
        Stat *s = &stats[i];
        const char *v = s->diff ? "DIVERGENT" : s->calls == 0 ? "UNCOVERED" : s->inconclusive ? "UNVERIFIED" : "IDENTICAL";
        fprintf(report, "%06X,%s,%ld,%ld,%ld,%ld,%ld,%s\n", routines[i].addr, routines[i].name, s->calls, s->match,
                s->diff, s->inconclusive, s->skipped, v);
        printf("%06X %-28s calls %ld match %ld diff %ld inconclusive %ld  %s\n", routines[i].addr, routines[i].name,
               s->calls, s->match, s->diff, s->inconclusive, v);
    }
    fclose(report);
}

/* ======================================================================
 * Lockstep (gate A7): the C really replaces its routines.
 *
 * After the game loads, the process forks. The parent runs the original
 * untouched and measures what every call of each routine costs in clocks,
 * leaving out time spent in interrupt handlers. The child runs the hybrid:
 * at each call it runs the C on the live machine instead, returns as the
 * routine would, and repeats the original call's exact sequence of bus cycles
 * (reads, writes, idle cycles and interrupt polls, without their bus
 * effects), so DMA, memory refresh and interrupts fall where they fell in the
 * original and the rest of the frame runs on the same timeline. Spending the
 * same number of clocks in one lump is not enough: a refresh landing in the
 * last cycles makes the hybrid up to 40 clocks late per call. After every frame the child
 * compares its machine state (WRAM, GSU RAM, VRAM, OAM, CGRAM, APU RAM) with
 * the parent's. The parent stays one frame ahead so a call's cost is known
 * before the child needs it.
 * ====================================================================== */
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAXC (1 << 17)
#define MAXOPS (1 << 23)
#define SNAP (0x20000 + 0x10000 + 0x10000 + 544 + 512 + 0x10000)
typedef struct {
    long frame;
    uint32_t ncalls, overflow, nops;
    uint32_t opoff[MAXC], opn[MAXC];
    uint8_t ops[MAXOPS];
    uint16_t s;
    int midcall;
    uint32_t clock;
    struct SfxboxRegs regs;
    uint8_t snap[SNAP];
} Slot;

static Slot *slots;
static int role;            /* 0 off, 1 original (parent), 2 hybrid (child) */
static int p_ready[2], p_ack[2];
static pid_t child;
static FILE *lsrep;
static uint32_t (*core_clock)(void);
static void (*core_replay)(const uint8_t *, uint32_t);
static void (*core_set_regs)(const struct SfxboxRegs *);
static void (*core_snapshot)(uint8_t *);
static int (**core_exec)(uint32_t);
static uint8_t mysnap[SNAP];
static long cur_frame, diff_frames, frames_compared, calls_swapped, calls_fallback, mismatched_calls;
static uint32_t hk;          /* hybrid: next cost index in this frame */

static struct {
    int active, in_irq;
    uint32_t ret_pc, irq_pc, slot_idx;
    uint16_t exit_s, irq_s;
    long frame;
} m;

static int entry(uint32_t pc, struct SfxboxRegs *r, uint32_t *ret, uint16_t *exit_s)
{
    uint8_t via = *core_opcode;
    uint32_t sp = r->s;
    if (r->e || sp + 3 >= wram_n) return 0;
    uint16_t word = (uint16_t)(wram[(sp + 1) & 0xFFFF] | wram[(sp + 2) & 0xFFFF] << 8);
    if (via == 0x20 || via == 0xFC) { *ret = (pc & 0xFF0000) | (uint16_t)(word + 1); *exit_s = (uint16_t)(sp + 2); return 1; }
    if (via == 0x22) { *ret = (uint32_t)wram[(sp + 3) & 0xFFFF] << 16 | (uint16_t)(word + 1); *exit_s = (uint16_t)(sp + 3); return 1; }
    return 0;
}

static int routine_at(uint32_t pc)
{
    if (!(watched[pc >> 3] >> (pc & 7) & 1)) return -1;
    for (int i = 0; i < nroutines; i++) if (routines[i].addr == pc) return i;
    return -1;
}

/* Debug: per-instruction trace of both sides (SFXBOX_LS_TRACE=first,last frame). */
static FILE *trf;
static long tr_lo = -1, tr_hi = -1;
static uint8_t (*core_bus_peek)(uint32_t);
static void trace_insn(uint32_t pc)
{
    if (!trf || cur_frame < tr_lo || cur_frame > tr_hi) return;
    struct SfxboxRegs r; core_regs(&r);
    fprintf(trf, "%ld %u %06X A=%04X X=%04X Y=%04X S=%04X P=%02X\n", cur_frame, core_clock(), pc, r.a, r.x, r.y, r.s, r.p);
}
static void trace_read(uint32_t addr, uint8_t data)
{
    if (!trf || cur_frame < tr_lo || cur_frame > tr_hi) return;
    if ((addr & 0x40FFFC) == 0x2140) fprintf(trf, "  rd %06X=%02X @%u\n", addr, data, core_clock());
}
static void trace_write(uint32_t addr, uint8_t data, int kind)
{
    if (!trf || cur_frame < tr_lo || cur_frame > tr_hi) return;
    if ((addr & 0x40FFFC) == 0x2140) fprintf(trf, "  wr %06X=%02X k%d @%u\n", addr, data, kind, core_clock());
}

/* Original side: measure each call's cost. */
static void ls_insn(uint32_t pc)
{
    trace_insn(pc);
    if (m.active) {
        if (m.in_irq) {
            if (pc != m.irq_pc) return;
            struct SfxboxRegs r; core_regs(&r);
            if (r.s == m.irq_s) m.in_irq = 0;
            return;
        }
        if (pc != m.ret_pc) return;
        struct SfxboxRegs r; core_regs(&r);
        if (r.s != m.exit_s) return;
        m.active = 0;
        return;
    }
    if (routine_at(pc) < 0) return;
    struct SfxboxRegs r; core_regs(&r);
    if (!entry(pc, &r, &m.ret_pc, &m.exit_s)) return;
    Slot *sl = &slots[cur_frame & 1];
    if (sl->ncalls >= MAXC) { sl->overflow = 1; return; }
    m.active = 1; m.in_irq = 0; m.frame = cur_frame;
    m.slot_idx = sl->ncalls++;
    sl->opoff[m.slot_idx] = sl->nops;
    sl->opn[m.slot_idx] = 0;
}

static void ls_interrupt(uint32_t resume_pc, uint16_t s)
{
    if (!m.active || m.in_irq) return;
    m.in_irq = 1; m.irq_pc = resume_pc; m.irq_s = s;
}

static void ls_timing(int kind, int clocks)
{
    if (!m.active || m.in_irq) return;
    Slot *sl = &slots[m.frame & 1];
    if (sl->frame != m.frame || sl->nops >= MAXOPS) { sl->overflow = 1; return; }
    sl->ops[sl->nops++] = (uint8_t)(kind << 4 | (clocks / 2));
    sl->opn[m.slot_idx]++;
}

/* Hybrid side: run the C in place of the routine. */
static int ls_exec(uint32_t pc)
{
    int idx = routine_at(pc);
    if (idx < 0) return 0;
    struct SfxboxRegs r; core_regs(&r);
    uint32_t ret; uint16_t exit_s;
    if (!entry(pc, &r, &ret, &exit_s)) { calls_fallback++; return 0; }
    Slot *sl = &slots[cur_frame & 1];
    /* A private copy: the replay can outlive the frame, and the original
       reuses this slot two frames later. */
    static uint8_t ops[1 << 20];
    uint32_t nops = 0;
    if (hk < sl->ncalls) {
        nops = sl->opn[hk];
        if (nops > sizeof ops) { nops = sizeof ops; mismatched_calls++; }
        memcpy(ops, sl->ops + sl->opoff[hk], nops);
    } else mismatched_calls++;
    hk++;
    cpu.a = r.a; cpu.x = r.x; cpu.y = r.y; cpu.s = r.s; cpu.d = r.d; cpu.db = r.db; cpu.p = r.p; cpu.e = r.e; cpu.pc = r.pc;
    machine_live = 1;
    routines[idx].abi();
    machine_live = 0;
    struct SfxboxRegs out = {cpu.a, cpu.x, cpu.y, exit_s, cpu.d, cpu.db, cpu.p, cpu.e, ret};
    core_set_regs(&out);
    core_replay(ops, nops);
    calls_swapped++;
    return 1;
}

static void wr_int(int fd, long v) { if (write(fd, &v, sizeof v) != sizeof v) _exit(3); }
static long rd_int(int fd) { long v; if (read(fd, &v, sizeof v) != sizeof v) _exit(4); return v; }

int lockstep_begin(void *core, const char *lib, const char *only, const char *report_path)
{
    if (load_routines(core, lib, only)) return -1;
    void (*set_cb)(const struct SfxboxCallbacks *) = dlsym(core, "sfxbox_set_callbacks");
    *(void **)&core_clock = dlsym(core, "sfxbox_core_clock");
    *(void **)&core_replay = dlsym(core, "sfxbox_core_replay");
    *(void **)&core_set_regs = dlsym(core, "sfxbox_core_set_regs");
    *(void **)&core_snapshot = dlsym(core, "sfxbox_core_snapshot");
    core_exec = dlsym(core, "sfxbox_cpu_exec");
    if (!set_cb || !core_clock || !core_replay || !core_set_regs || !core_snapshot || !core_exec) return -1;
    slots = mmap(NULL, sizeof(Slot) * 2, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (slots == MAP_FAILED) return -1;
    (void)core_bus_peek;
    slots[0].frame = slots[1].frame = -1;
    if (pipe(p_ready) || pipe(p_ack)) return -1;
    fflush(NULL);
    child = fork();
    if (child < 0) return -1;
    const char *tr = getenv("SFXBOX_LS_TRACE");
    if (child == 0) {
        role = 2;
        lsrep = fopen(report_path, "w");
        if (!lsrep) _exit(2);
        *core_exec = ls_exec;
        if (tr) {
            static struct SfxboxCallbacks hcb = {trace_insn, trace_write, trace_read, NULL, NULL};
            set_cb(&hcb);
        }
    } else {
        role = 1;
        static struct SfxboxCallbacks cb = {ls_insn, trace_write, trace_read, ls_interrupt, ls_timing};
        set_cb(&cb);
    }
    if (tr && sscanf(tr, "%ld,%ld", &tr_lo, &tr_hi) == 2) {
        char p[4096]; snprintf(p, sizeof p, "%s.%s", report_path, role == 1 ? "orig" : "hyb");
        trf = fopen(p, "w");
    }
    return role;
}

int lockstep_role(void) { return role; }

void lockstep_before_frame(long f)
{
    cur_frame = f;
    if (role == 1) {
        if (f >= 2) rd_int(p_ack[0]);
        Slot *sl = &slots[f & 1];
        sl->frame = f; sl->ncalls = 0; sl->overflow = 0; sl->nops = 0;
    } else if (role == 2) {
        long got = rd_int(p_ready[0]);
        if (got != f) { fprintf(lsrep, "lockstep: frame order %ld/%ld\n", got, f); fflush(lsrep); _exit(5); }
        hk = 0;
    }
}

static void compare(long f)
{
    static const struct { const char *name; uint32_t off, len; } reg[] = {
        {"wram", 0, 0x20000}, {"gsuram", 0x20000, 0x10000}, {"vram", 0x30000, 0x10000},
        {"oam", 0x40000, 544}, {"cgram", 0x40000 + 544, 512}, {"apuram", 0x40000 + 544 + 512, 0x10000}};
    Slot *sl = &slots[f & 1];
    core_snapshot(mysnap);
    uint32_t s = sl->s, lo = s > 0x200 ? s - 0x200 : 0;
    int bad = 0;
    char buf[1024]; buf[0] = 0;
    for (unsigned k = 0; k < sizeof reg / sizeof reg[0]; k++) {
        int shown = 0, count = 0;
        for (uint32_t i = 0; i < reg[k].len; i++) {
            uint32_t o = reg[k].off + i;
            if (k == 0 && i > lo && i <= s) continue;  /* dead stack below S */
            if (mysnap[o] != sl->snap[o]) {
                count++;
                if (shown < 6) { snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " %s[%05X]=%02X/%02X", reg[k].name, i, mysnap[o], sl->snap[o]); shown++; }
            }
        }
        if (count) { bad = 1; snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " (%s: %d bytes)", reg[k].name, count); }
    }
    if (bad && sl->midcall) strncat(buf, " (frame ended inside a call)", sizeof buf - strlen(buf) - 1);
    if (hk != sl->ncalls) { bad = 1; snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " calls %u/%u", hk, sl->ncalls); }
    if (sl->overflow) { bad = 1; strncat(buf, " cost-overflow", sizeof buf - strlen(buf) - 1); }
    {
        struct SfxboxRegs r; core_regs(&r);
        uint32_t c = core_clock();
        if (c != sl->clock || r.pc != sl->regs.pc) {
            if (getenv("SFXBOX_LS_DEBUG") && f < 60)
                fprintf(lsrep, "frame %ld: clock hybrid-original %ld, pc %06X/%06X%s\n", f, (long)(int32_t)(c - sl->clock),
                        r.pc, sl->regs.pc, sl->midcall ? " (inside a call)" : "");
        }
    }
    frames_compared++;
    if (bad) { if (diff_frames++ < 5) fprintf(lsrep, "DIFF frame %ld: hybrid/original%s\n", f, buf); }
}

void lockstep_after_frame(long f)
{
    if (trf) fflush(trf);
    if (role == 1) {
        Slot *sl = &slots[f & 1];
        struct SfxboxRegs r; core_regs(&r);
        /* A frame that ends inside a call: the hybrid has already finished it,
           so the stack is judged at the caller's level. */
        sl->s = m.active ? m.exit_s : r.s;
        sl->midcall = m.active;
        sl->clock = core_clock();
        sl->regs = r;
        core_snapshot(sl->snap);
        if (f >= 1) wr_int(p_ready[1], f - 1);
    } else if (role == 2) {
        compare(f);
        wr_int(p_ack[1], f);
    }
}

void lockstep_end(long frames)
{
    if (role == 1) {
        if (frames >= 1) wr_int(p_ready[1], frames - 1);
        int st = 0;
        waitpid(child, &st, 0);
    } else if (role == 2) {
        fprintf(lsrep, "frames %ld compared %ld, differing %ld; calls swapped %ld, entered other ways %ld, unmatched %ld: %s\n",
                frames, frames_compared, diff_frames, calls_swapped, calls_fallback, mismatched_calls,
                diff_frames || mismatched_calls ? "DIVERGENT" : calls_swapped ? "IDENTICAL" : "UNCOVERED");
        fclose(lsrep);
        _exit(0);
    }
}
