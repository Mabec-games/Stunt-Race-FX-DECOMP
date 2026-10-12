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
#include <setjmp.h>
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
    void (*timing)(int kind, int clocks, uint32_t addr);
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

#define MAPN (1 << 16)
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

/* Growable logs of hardware accesses: a call can poll a port thousands of times. */
typedef struct { Io *e; int n, cap; } IoLog;
static void io_add(IoLog *l, uint32_t addr, uint8_t v)
{
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 4096;
        l->e = realloc(l->e, (size_t)l->cap * sizeof *l->e);
        if (!l->e) abort();
    }
    l->e[l->n].addr = addr; l->e[l->n].v = v; l->n++;
}

/* One watched call. Calls nest (a checked routine can call another), so the
   watches form a stack; every access is recorded in every open watch. */
typedef struct {
    int idx, in_irq, interrupted, dma, overflow;
    uint32_t ret_pc, irq_pc;
    uint16_t exit_s, irq_s;
    struct SfxboxRegs entry;
    Map undo, orig, cw;
    IoLog oread, owrite, cwrite;
    int cread;
    int io_mismatch;
} Watch;

#define DEPTH 6
static Watch ws[DEPTH];
static int depth;
static Watch *cur;  /* the watch whose C is running */

/* A C routine that reads past the replay is stopped here, not left spinning. */
static jmp_buf abort_c;
static int c_running;
/* A C routine that waits on memory only an interrupt changes (a frame
   handshake) would spin forever against the frozen entry state: past this
   many accesses the call is abandoned and reported as a runaway. */
#define RUNAWAY (1L << 22)
static long c_accesses;
static int c_runaway;
static void count_access(void)
{
    if (c_running && ++c_accesses > RUNAWAY) { c_runaway = 1; longjmp(abort_c, 1); }
}

typedef struct { long calls, match, diff, inconclusive, skipped; int shown; } Stat;
static Stat *stats;
static FILE *report;

/* ---- the C's view: the sandbox during a check, the real bus in lockstep ---- */

static int machine_live;
static uint8_t (*core_bus_read)(uint32_t);
static void (*core_bus_write)(uint32_t, uint8_t);
static uint8_t live_access(int write, uint32_t addr, uint8_t value);

uint8_t rd8(uint32_t addr)
{
    if (machine_live) return live_access(0, addr & 0xFFFFFF, 0);
    count_access();
    uint32_t n = norm(addr);
    uint8_t v;
    if (n >> 24 == R_IO) {
        if (cur->cread < cur->oread.n && cur->oread.e[cur->cread].addr == (addr & 0xFFFF)) return cur->oread.e[cur->cread++].v;
        cur->io_mismatch = 1;
        if (c_running) longjmp(abort_c, 1);
        return 0;
    }
    if (map_get(&cur->cw, n, &v)) return v;
    if (map_get(&cur->undo, n, &v)) return v;
    uint8_t *p = phys(n);
    if (!p) { cur->io_mismatch = 1; if (c_running) longjmp(abort_c, 1); return 0; }
    return *p;
}

void wr8(uint32_t addr, uint8_t value)
{
    if (machine_live) { live_access(1, addr & 0xFFFFFF, value); return; }
    count_access();
    uint32_t n = norm(addr);
    if (n >> 24 == R_IO) {
        io_add(&cur->cwrite, addr & 0xFFFF, value);
        return;
    }
    if (n >> 24 == R_WRAM || n >> 24 == R_GSURAM) map_put(&cur->cw, n, value, 0);
    else cur->io_mismatch = 1;
}

/* ---- events from the core ---- */

static uint8_t entry_value(Watch *w, uint32_t n)
{
    uint8_t v;
    if (map_get(&w->undo, n, &v)) return v;
    uint8_t *p = phys(n);
    return p ? *p : 0;
}

static int is_scratch(Watch *w, uint32_t n)
{
    if (n >> 24 != R_WRAM) return 0;
    uint32_t off = n & 0xFFFFFF;
    return off <= w->entry.s && off + 0x100 > w->entry.s;
}

static void finish(Watch *w)
{
    Stat *st = &stats[w->idx];
    const Routine *r = &routines[w->idx];
    struct SfxboxRegs out;
    core_regs(&out);
    st->calls++;

    cpu.a = w->entry.a; cpu.x = w->entry.x; cpu.y = w->entry.y; cpu.s = w->entry.s; cpu.d = w->entry.d;
    cpu.db = w->entry.db; cpu.p = w->entry.p; cpu.e = w->entry.e; cpu.pc = w->entry.pc;
    map_clear(&w->cw); w->cwrite.n = 0; w->cread = 0; w->io_mismatch = 0;
    cur = w;
    c_accesses = 0; c_runaway = 0;
    c_running = 1;
    if (setjmp(abort_c) == 0) r->abi();
    c_running = 0;

    char why[512] = "";
    int bad = 0;
#define DIFFR(f) if (cpu.f != out.f) { bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " %s=%X/%X", #f, (unsigned)cpu.f, (unsigned)out.f); }
    DIFFR(a) DIFFR(x) DIFFR(y) DIFFR(d) DIFFR(db) DIFFR(p) DIFFR(e)
    for (int pass = 0; pass < 2; pass++) {
        Map *m = pass ? &w->cw : &w->orig;
        for (int i = 0; i < m->n; i++) {
            uint32_t k = m->key[m->order[i]];
            if (is_scratch(w, k)) continue;
            uint8_t ov, cv;
            if (!map_get(&w->orig, k, &ov)) ov = entry_value(w, k);
            if (!map_get(&w->cw, k, &cv)) cv = entry_value(w, k);
            if (ov != cv) {
                if (!bad || strlen(why) < 400)
                    snprintf(why + strlen(why), sizeof why - strlen(why), " [%d:%06X]=%02X/%02X", k >> 24, k & 0xFFFFFF, cv, ov);
                bad = 1;
            }
        }
    }
    if (w->cwrite.n != w->owrite.n) { bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " io-writes %d/%d", w->cwrite.n, w->owrite.n); }
    else for (int i = 0; i < w->owrite.n; i++)
        if (w->cwrite.e[i].addr != w->owrite.e[i].addr || w->cwrite.e[i].v != w->owrite.e[i].v) {
            bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " io#%d %04X=%02X/%04X=%02X", i,
                              w->cwrite.e[i].addr, w->cwrite.e[i].v, w->owrite.e[i].addr, w->owrite.e[i].v); break;
        }
    if (w->cread != w->oread.n) { bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " io-reads %d/%d", w->cread, w->oread.n); }
    if (w->io_mismatch) { bad = 1; strncat(why, " unreplayable-access", sizeof why - strlen(why) - 1); }
    if (w->overflow) { bad = 1; strncat(why, " overflow", sizeof why - strlen(why) - 1); }
    if (c_runaway) { bad = 1; strncat(why, " runaway", sizeof why - strlen(why) - 1); }

    if (!bad) { st->match++; return; }
    int unsure = w->interrupted || w->dma;
    if (unsure) st->inconclusive++; else st->diff++;
    if (st->shown < 8) {
        st->shown++;
        fprintf(report, "%s %06X %s call %ld: entry A=%04X X=%04X Y=%04X S=%04X D=%04X DB=%02X P=%02X%s; C/orig:%s\n",
                unsure ? "INCONCLUSIVE" : "DIFF", r->addr, r->name, st->calls, w->entry.a, w->entry.x, w->entry.y,
                w->entry.s, w->entry.d, w->entry.db, w->entry.p, w->interrupted ? " (interrupted)" : w->dma ? " (dma)" : "", why);
    }
}

static void on_insn(uint32_t pc)
{
    struct SfxboxRegs r;
    int have = 0;
    for (int i = 0; i < depth; i++)
        if (ws[i].in_irq && pc == ws[i].irq_pc) {
            if (!have) { core_regs(&r); have = 1; }
            if (r.s == ws[i].irq_s) ws[i].in_irq = 0;
        }
    if (depth) {
        Watch *t = &ws[depth - 1];
        if (!t->in_irq && pc == t->ret_pc) {
            if (!have) { core_regs(&r); have = 1; }
            if (r.s == t->exit_s) { finish(t); depth--; }
        }
    }
    if (!(watched[pc >> 3] >> (pc & 7) & 1)) return;
    int idx = -1;
    for (int i = 0; i < nroutines; i++) if (routines[i].addr == pc) { idx = i; break; }
    if (idx < 0) return;
    if (!have) { core_regs(&r); have = 1; }
    uint8_t via = *core_opcode;
    uint32_t sp = r.s;
    if (sp + 3 >= wram_n || r.e || depth == DEPTH) { stats[idx].skipped++; return; }
    Watch *w = &ws[depth];
    uint16_t word = (uint16_t)(wram[(sp + 1) & 0xFFFF] | wram[(sp + 2) & 0xFFFF] << 8);
    if (via == 0x20 || via == 0xFC) {
        w->ret_pc = (pc & 0xFF0000) | (uint16_t)(word + 1);
        w->exit_s = (uint16_t)(sp + 2);
    } else if (via == 0x22) {
        w->ret_pc = (uint32_t)wram[(sp + 3) & 0xFFFF] << 16 | (uint16_t)(word + 1);
        w->exit_s = (uint16_t)(sp + 3);
    } else { stats[idx].skipped++; return; }
    w->idx = idx; w->entry = r;
    w->in_irq = w->interrupted = w->dma = w->overflow = 0;
    map_clear(&w->undo); map_clear(&w->orig);
    w->oread.n = w->owrite.n = 0;
    depth++;
}

static void on_write(uint32_t addr, uint8_t data, int kind)
{
    if (!depth) return;
    uint32_t n = norm(addr);
    int ram = n >> 24 == R_WRAM || n >> 24 == R_GSURAM;
    uint8_t *p = ram ? phys(n) : NULL;
    for (int i = 0; i < depth; i++) {
        Watch *w = &ws[i];
        if (p) { map_put(&w->undo, n, *p, 1); if (w->undo.n >= MAPN / 2) w->overflow = 1; }
        if (kind == 2) { w->dma = 1; continue; }
        if (w->in_irq) continue;
        if (ram) { map_put(&w->orig, n, data, 0); if (w->orig.n >= MAPN / 2) w->overflow = 1; }
        else if (n >> 24 == R_IO) io_add(&w->owrite, addr & 0xFFFF, data);
    }
}

static void on_read(uint32_t addr, uint8_t data)
{
    if (!depth || norm(addr) >> 24 != R_IO) return;
    for (int i = 0; i < depth; i++)
        if (!ws[i].in_irq) io_add(&ws[i].oread, addr & 0xFFFF, data);
}

static void on_interrupt(uint32_t resume_pc, uint16_t s)
{
    for (int i = 0; i < depth; i++)
        if (!ws[i].in_irq) { ws[i].in_irq = 1; ws[i].interrupted = 1; ws[i].irq_pc = resume_pc; ws[i].irq_s = s; }
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
 * the parent's. The parent runs ahead of the child as far as it must: a call
 * can span many frames (the boot sound upload spans dozens), and its cycles
 * must be complete before the child can replay them. Frames live in a ring of
 * snapshots and calls in a ring of cycle records; each side waits only for
 * what it needs.
 * ====================================================================== */
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define NSLOT 128
#define CALLRING (1 << 18)
#define OPRING (1u << 24)
#define SNAP (0x20000 + 0x10000 + 0x10000 + 544 + 512 + 0x10000)

typedef struct { long frame; uint64_t opoff; uint32_t opn; int done; } CallRec;
typedef struct {
    long frame;
    uint32_t ncalls;
    uint16_t s;
    int midcall;
    uint32_t clock;
    struct SfxboxRegs regs;
    uint8_t snap[SNAP];
} FrameSlot;
typedef struct {
    long o_frames, h_frames;          /* frames each side has finished */
    uint64_t o_calls, h_calls;        /* calls recorded / consumed */
    uint64_t o_ops, h_ops;            /* cycles recorded / no longer needed */
    int o_done, stuck;
    CallRec calls[CALLRING];
    FrameSlot fs[NSLOT];
    uint8_t ops[OPRING];
    uint32_t opaddr[OPRING];
} Shared;

static Shared *sh;
static int role;            /* 0 off, 1 original (parent), 2 hybrid (child) */
static pid_t child;
static uint32_t (*core_clock)(void);
static void (*core_replay)(const uint8_t *, uint32_t);
static int (*core_interrupt_here)(const struct SfxboxRegs *);
static uint8_t (*core_access)(int, uint32_t, uint8_t);
static void (*core_set_regs)(const struct SfxboxRegs *);
static void (*core_snapshot)(uint8_t *);
static int (**core_exec)(uint32_t);
static uint8_t mysnap[SNAP];
static long diff_frames, frames_compared, calls_swapped, calls_fallback, mismatched_calls;
static uint32_t hk;          /* hybrid: calls made this frame */

#define LOAD(x) __atomic_load_n(&(x), __ATOMIC_SEQ_CST)
#define STORE(x, v) __atomic_store_n(&(x), (v), __ATOMIC_SEQ_CST)

static long cur_frame;
static FILE *lsrep;
static void stuck_report(const char *cond)
{
    fprintf(stderr, "lockstep: %s side stuck in frame %ld waiting for %s; frames o/h %ld/%ld, calls o/h %llu/%llu,"
            " ops o/h %llu/%llu\n", role == 1 ? "original" : "hybrid", cur_frame, cond, LOAD(sh->o_frames),
            LOAD(sh->h_frames), (unsigned long long)LOAD(sh->o_calls), (unsigned long long)LOAD(sh->h_calls),
            (unsigned long long)LOAD(sh->o_ops), (unsigned long long)LOAD(sh->h_ops));
    if (lsrep) fflush(lsrep);
}

/* Wait for a condition the other side will make true; give up (and say so)
   if it never does, rather than hang. */
#define WAIT_UNTIL(cond) do { long spins_ = 0; while (!(cond)) { \
        if (LOAD(sh->stuck)) _exit(6); \
        if (++spins_ > 600000) { stuck_report(#cond); STORE(sh->stuck, 1); _exit(6); } usleep(50); } } while (0)

static struct {
    int active, in_irq;
    uint32_t ret_pc, irq_pc;
    uint16_t exit_s, irq_s;
    uint64_t call;
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
    if ((addr & 0x40FFFC) == 0x2140 || (addr & 0x40FFC0) == 0x3000) fprintf(trf, "  rd %06X=%02X @%u\n", addr, data, core_clock());
}
static void trace_write(uint32_t addr, uint8_t data, int kind)
{
    if (!trf || cur_frame < tr_lo || cur_frame > tr_hi) return;
    if ((addr & 0x40FFFC) == 0x2140 || (addr & 0x40FFC0) == 0x3000) fprintf(trf, "  wr %06X=%02X k%d @%u\n", addr, data, kind, core_clock());
}

/* Original side: measure each call's cost. */
static void ls_insn(uint32_t pc)
{
    trace_insn(pc);
    if (m.active) {
        if (m.in_irq) {
            if (pc != m.irq_pc) return;
            struct SfxboxRegs r; core_regs(&r);
            if (r.s != m.irq_s) return;
            m.in_irq = 0;  /* back from the handler: this may also be the return */
        }
        if (pc != m.ret_pc) return;
        struct SfxboxRegs r; core_regs(&r);
        if (r.s != m.exit_s) return;
        STORE(sh->calls[m.call % CALLRING].done, 1);
        m.active = 0;
        return;
    }
    if (routine_at(pc) < 0) return;
    struct SfxboxRegs r; core_regs(&r);
    if (!entry(pc, &r, &m.ret_pc, &m.exit_s)) return;
    uint64_t k = LOAD(sh->o_calls);
    WAIT_UNTIL(k - LOAD(sh->h_calls) < CALLRING);
    CallRec *rec = &sh->calls[k % CALLRING];
    rec->frame = cur_frame; rec->opoff = LOAD(sh->o_ops); rec->opn = 0;
    STORE(rec->done, 0);
    STORE(sh->o_calls, k + 1);
    sh->fs[cur_frame % NSLOT].ncalls++;
    m.active = 1; m.in_irq = 0; m.call = k;
}

static void ls_op(uint8_t op, uint32_t addr);

/* An interrupt taken inside a call: its place in the call's cycles is marked
   with the registers at that moment (ops 5-8), so the hybrid can take it at
   the same point; the handler's own cycles are not recorded. */
static void ls_interrupt(uint32_t resume_pc, uint16_t s)
{
    if (!m.active) return;
    /* A handler that returns straight into another interrupt: no instruction
       runs at the call's level in between, but this one is taken there too. */
    if (m.in_irq && resume_pc == m.irq_pc && s == m.irq_s) m.in_irq = 0;
    if (m.in_irq) return;
    struct SfxboxRegs r; core_regs(&r);
    ls_op(0x50, (resume_pc & 0xFFFFFF) | (uint32_t)r.e << 24);
    ls_op(0x60, r.a | (uint32_t)r.x << 16);
    ls_op(0x70, r.y | (uint32_t)s << 16);
    ls_op(0x80, r.d | (uint32_t)r.db << 16 | (uint32_t)r.p << 24);
    m.in_irq = 1; m.irq_pc = resume_pc; m.irq_s = s;
}

static void ls_timing(int kind, int clocks, uint32_t addr)
{
    if (!m.active || m.in_irq) return;
    ls_op((uint8_t)(kind << 4 | (clocks / 2)), addr);
}

static void ls_op(uint8_t op, uint32_t addr)
{
    uint64_t n = LOAD(sh->o_ops);
    WAIT_UNTIL(n - LOAD(sh->h_ops) < OPRING);
    sh->opaddr[n % OPRING] = addr;
    sh->ops[n % OPRING] = op;
    sh->calls[m.call % CALLRING].opn++;
    STORE(sh->o_ops, n + 1);
}

/* Hybrid: the current call's recorded cycles, consumed as its C runs. A C
   access to hardware or Super FX RAM first replays the original's cycles up
   to its matching access, then makes that access for real with the CPU's own
   timing, so concurrent hardware sees it at the original's clock. Work RAM
   is the S-CPU's alone and is accessed at once. */
static uint8_t *cur_ops;
static uint32_t *cur_addr;
static uint32_t cur_n, cur_i, cur_cap;
static long timed_unmatched;

static int timed(uint32_t a)
{
    uint32_t bank = a >> 16 & 0xFF, lo = a & 0xFFFF;
    if (bank == 0x70 || bank == 0x71) return 1;
    if ((bank & 0x40) == 0 && lo >= 0x2000 && lo < 0x8000) return 1;
    return 0;
}

static uint32_t same_place(uint32_t a)  /* banks $00-$3F and $80-$BF alias for these */
{
    uint32_t bank = a >> 16 & 0xFF;
    if (bank < 0x40 || (bank >= 0x80 && bank < 0xC0)) return a & 0xFFFF;
    return a;
}

static long irq_missed;

/* Replay the recorded cycles [cur_i, to), taking each recorded interrupt at
   its place: the core takes the pending interrupt from the original's
   registers and runs the handler until it returns. */
static void replay_to(uint32_t to)
{
    uint32_t i = cur_i, run = cur_i;
    while (i < to) {
        if (cur_ops[i] >> 4 != 5) { i++; continue; }
        if (i > run) core_replay(cur_ops + run, i - run);
        if (i + 3 < cur_n) {
            struct SfxboxRegs at = {
                (uint16_t)cur_addr[i + 1], (uint16_t)(cur_addr[i + 1] >> 16), (uint16_t)cur_addr[i + 2],
                (uint16_t)(cur_addr[i + 2] >> 16), (uint16_t)cur_addr[i + 3], (uint8_t)(cur_addr[i + 3] >> 16),
                (uint8_t)(cur_addr[i + 3] >> 24), (uint8_t)(cur_addr[i] >> 24), cur_addr[i] & 0xFFFFFF};
            struct SfxboxRegs mine; core_regs(&mine);
            if (!core_interrupt_here(&at)) irq_missed++;
            core_set_regs(&mine);
        }
        i += 4;
        run = i;
    }
    if (to > run) core_replay(cur_ops + run, to - run);
    cur_i = to;
}

static long snap_offset(uint32_t a);
static int past_end;    /* the original's run ended inside this call */

static uint8_t live_access(int write, uint32_t addr, uint8_t value)
{
    uint8_t want = (uint8_t)(write ? 2 : 1);
    if (!timed(addr)) {
        /* Work RAM (and the Super FX RAM window) is the S-CPU's alone, but an
           interrupt handler or a frame end can see it: the access is placed
           at the original's matching access, so both see the same writes
           done. The search stops at the next hardware access, which the C
           must make first; a RAM access with no counterpart (the original
           kept the value in a register) happens at once. */
        long o = snap_offset(addr);
        if (o >= 0)
            for (uint32_t j = cur_i; j < cur_n; j++) {
                uint8_t k = cur_ops[j] >> 4;
                if ((k == 1 || k == 2) && timed(cur_addr[j])) break;
                if (k == want && snap_offset(cur_addr[j]) == o) { replay_to(j + 1); break; }
            }
        if (write) { core_bus_write(addr, value); return value; }
        return core_bus_read(addr);
    }
    for (uint32_t j = cur_i; j < cur_n; j++)
        if (cur_ops[j] >> 4 == want && same_place(cur_addr[j]) == same_place(addr)) {
            replay_to(j);
            cur_i = j + 1;
            return core_access(write, addr, value);
        }
    /* No matching cycle left in the original's record: the run has already
       diverged. The access is still made as a real, timed CPU access, so the
       clock moves and a polling loop in the C can end. */
    if (past_end) return core_access(write, addr, value);
    if (timed_unmatched++ < 5)
        fprintf(lsrep, "OFF frame %ld: %s %06X with %u of %u recorded cycles used\n", cur_frame, write ? "write" : "read",
                addr, cur_i, cur_n);
    return core_access(write, addr, value);
}

/* Hybrid side: run the C in place of the routine. */
static int ls_exec(uint32_t pc)
{
    int idx = routine_at(pc);
    if (idx < 0) return 0;
    struct SfxboxRegs r; core_regs(&r);
    uint32_t ret; uint16_t exit_s;
    if (!entry(pc, &r, &ret, &exit_s)) { calls_fallback++; return 0; }
    /* The original's next call: wait until it has returned, then take a
       private copy of its cycles (the ring moves on). */
    uint64_t k = LOAD(sh->h_calls);
    WAIT_UNTIL((k < LOAD(sh->o_calls) && LOAD(sh->calls[k % CALLRING].done)) || LOAD(sh->o_done));
    cur_n = 0; cur_i = 0;
    if (k < LOAD(sh->o_calls) && LOAD(sh->calls[k % CALLRING].done)) {
        CallRec *rec = &sh->calls[k % CALLRING];
        if (rec->frame != cur_frame) mismatched_calls++;
        cur_n = rec->opn;
        if (cur_n > cur_cap) {
            cur_cap = cur_n;
            cur_ops = realloc(cur_ops, cur_cap);
            cur_addr = realloc(cur_addr, (size_t)cur_cap * sizeof *cur_addr);
            if (!cur_ops || !cur_addr) _exit(7);
        }
        for (uint32_t i = 0; i < cur_n; i++) {
            uint64_t o = (rec->opoff + i) % OPRING;
            cur_ops[i] = sh->ops[o];
            cur_addr[i] = sh->opaddr[o];
        }
        STORE(sh->h_ops, rec->opoff + rec->opn);
        STORE(sh->h_calls, k + 1);
    } else if (LOAD(sh->o_done) && cur_frame >= LOAD(sh->o_frames) - 1) {
        /* The script ended inside this call on the original side: there is
           nothing left to compare it with. */
        past_end = 1;
    } else mismatched_calls++;
    hk++;
    cpu.a = r.a; cpu.x = r.x; cpu.y = r.y; cpu.s = r.s; cpu.d = r.d; cpu.db = r.db; cpu.p = r.p; cpu.e = r.e; cpu.pc = r.pc;
    machine_live = 1;
    routines[idx].abi();
    machine_live = 0;
    replay_to(cur_n);
    struct SfxboxRegs out = {cpu.a, cpu.x, cpu.y, exit_s, cpu.d, cpu.db, cpu.p, cpu.e, ret};
    core_set_regs(&out);
    calls_swapped++;
    return 1;
}

int lockstep_begin(void *core, const char *lib, const char *only, const char *report_path)
{
    if (load_routines(core, lib, only)) return -1;
    void (*set_cb)(const struct SfxboxCallbacks *) = dlsym(core, "sfxbox_set_callbacks");
    *(void **)&core_clock = dlsym(core, "sfxbox_core_clock");
    *(void **)&core_replay = dlsym(core, "sfxbox_core_replay");
    *(void **)&core_interrupt_here = dlsym(core, "sfxbox_core_interrupt_here");
    *(void **)&core_access = dlsym(core, "sfxbox_core_access");
    *(void **)&core_set_regs = dlsym(core, "sfxbox_core_set_regs");
    *(void **)&core_snapshot = dlsym(core, "sfxbox_core_snapshot");
    core_exec = dlsym(core, "sfxbox_cpu_exec");
    if (!set_cb || !core_clock || !core_replay || !core_interrupt_here || !core_access || !core_set_regs || !core_snapshot || !core_exec) return -1;
    sh = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (sh == MAP_FAILED) return -1;
    (void)core_bus_peek;
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
        WAIT_UNTIL(f - LOAD(sh->h_frames) < NSLOT - 1);
        FrameSlot *fs = &sh->fs[f % NSLOT];
        fs->frame = f; fs->ncalls = 0;
    } else if (role == 2) {
        WAIT_UNTIL(LOAD(sh->o_frames) > f || LOAD(sh->o_done));
        hk = 0;
    }
}

/* Where a CPU write lands in the frame snapshot: work RAM, or Super FX RAM
   in bank $70 or its window at $6000-$7FFF; -1 for anything else. */
static long snap_offset(uint32_t a)
{
    uint32_t bank = a >> 16 & 0xFF, lo = a & 0xFFFF;
    if (bank == 0x7E || bank == 0x7F) return (long)((bank - 0x7E) << 16 | lo);
    if (bank == 0x70) return 0x20000 + (long)lo;
    if ((bank & 0x40) == 0 && lo < 0x2000) return (long)lo;
    if ((bank & 0x40) == 0 && lo >= 0x6000 && lo < 0x8000) return 0x20000 + (long)(lo - 0x6000);
    return -1;
}

static uint8_t inflight[SNAP];

static void compare(long f)
{
    static const struct { const char *name; uint32_t off, len; } reg[] = {
        {"wram", 0, 0x20000}, {"gsuram", 0x20000, 0x10000}, {"vram", 0x30000, 0x10000},
        {"oam", 0x40000, 544}, {"cgram", 0x40000 + 544, 512}, {"apuram", 0x40000 + 544 + 512, 0x10000}};
    FrameSlot *sl = &sh->fs[f % NSLOT];
    core_snapshot(mysnap);
    uint32_t s = sl->s, lo = s > 0x200 ? s - 0x200 : 0;
    int bad = 0;
    char buf[1024]; buf[0] = 0;
    /* A frame that ended inside a call: the C made all of the call's RAM
       writes at its start, the original only some of them so far. The bytes
       the call writes are judged at the next frame instead. */
    if (sl->midcall)
        for (uint32_t j = 0; j < cur_n; j++)
            if (cur_ops[j] >> 4 == 2) { long o = snap_offset(cur_addr[j]); if (o >= 0) inflight[o] = 1; }
    for (unsigned k = 0; k < sizeof reg / sizeof reg[0]; k++) {
        int shown = 0, count = 0;
        for (uint32_t i = 0; i < reg[k].len; i++) {
            uint32_t o = reg[k].off + i;
            if (k == 0 && i > lo && i <= s) continue;  /* dead stack below S */
            if (o < 0x30000 && inflight[o]) continue;
            if (mysnap[o] != sl->snap[o]) {
                count++;
                if (shown < 6) { snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " %s[%05X]=%02X/%02X", reg[k].name, i, mysnap[o], sl->snap[o]); shown++; }
            }
        }
        if (count) { bad = 1; snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " (%s: %d bytes)", reg[k].name, count); }
    }
    if (sl->midcall) memset(inflight, 0, sizeof inflight);
    if (bad && sl->midcall) strncat(buf, " (frame ended inside a call)", sizeof buf - strlen(buf) - 1);
    if (hk != sl->ncalls) { bad = 1; snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " calls %u/%u", hk, sl->ncalls); }
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
        FrameSlot *sl = &sh->fs[f % NSLOT];
        struct SfxboxRegs r; core_regs(&r);
        /* A frame that ends inside a call: the hybrid has already finished it,
           so the stack is judged at the caller's level. */
        sl->s = m.active ? m.exit_s : r.s;
        sl->midcall = m.active;
        sl->clock = core_clock();
        sl->regs = r;
        core_snapshot(sl->snap);
        STORE(sh->o_frames, f + 1);
    } else if (role == 2) {
        if (!past_end) compare(f);
        STORE(sh->h_frames, f + 1);
    }
}

void lockstep_end(long frames)
{
    if (role == 1) {
        STORE(sh->o_done, 1);
        int st = 0;
        waitpid(child, &st, 0);
        if (LOAD(sh->stuck)) fprintf(stderr, "lockstep: a side waited too long; see the report\n");
    } else if (role == 2) {
        fprintf(lsrep, "frames %ld compared %ld, differing %ld; calls swapped %ld, entered other ways %ld, unmatched %ld, "
                "hardware accesses off the timeline %ld, interrupts not pending where taken %ld: %s\n",
                frames, frames_compared, diff_frames, calls_swapped, calls_fallback, mismatched_calls, timed_unmatched, irq_missed,
                diff_frames || mismatched_calls || timed_unmatched || irq_missed ? "DIVERGENT" : calls_swapped ? "IDENTICAL" : "UNCOVERED");
        fclose(lsrep);
        _exit(0);
    }
}
