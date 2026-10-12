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

/* ---- the C's view during a check ---- */

uint8_t rd8(uint32_t addr)
{
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

int check_begin(void *core, const char *lib, const char *only, const char *report_path)
{
    void *h = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
    if (!h) { fprintf(stderr, "dlopen %s: %s\n", lib, dlerror()); return -1; }
    const Routine *table = dlsym(h, "sfx_routine_table");
    const int *count = dlsym(h, "sfx_routine_count");
    void (*set_cb)(const struct SfxboxCallbacks *) = dlsym(core, "sfxbox_set_callbacks");
    uint8_t *(*mem)(const char *, size_t *) = dlsym(core, "sfxbox_memory");
    *(void **)&core_regs = dlsym(core, "sfxbox_regs");
    core_opcode = dlsym(core, "sfxbox_cpu_opcode");
    if (!table || !count || !set_cb || !mem || !core_regs || !core_opcode) { fprintf(stderr, "check: missing symbols\n"); return -1; }
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
