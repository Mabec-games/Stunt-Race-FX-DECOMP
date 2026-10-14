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
#include "gsucall.h"

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
static const Routine *all_routines;     /* every routine with C, for call_routine */
static int all_count;
static uint8_t catalog_bits[1 << 21];   /* routine entries the catalog knows */
static int is_catalog_entry(uint32_t pc) { return catalog_bits[pc >> 3] >> (pc & 7) & 1; }
static uint8_t jump_bits[1 << 21];      /* code C calls that the original jumps to */
static int is_jump_entry(uint32_t pc) { return jump_bits[pc >> 3] >> (pc & 7) & 1; }
static int nroutines;
static uint8_t *watched;  /* 16M-entry bitmap of routine entry points */
static uint8_t *has_c;    /* the same for every routine with C in the library */

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
    int idx, in_irq, interrupted, dma, gsu, needs_asm, overflow;
    int stack_reset;         /* a TXS or TCS ran inside the call */
    uint32_t ret_pc, irq_pc;
    uint16_t exit_s, irq_s;
    struct SfxboxRegs entry;
    Map undo, orig, cw;
    IoLog oread, owrite, cwrite;
    IoLog oseq, cseq;        /* work RAM accesses in order (SFXBOX_SEQ): addr | 1 << 24 for a write */
    int cread;
    int io_mismatch;
} Watch;

#define DEPTH 12
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
static int seq_on;           /* SFXBOX_SEQ: compare the order of work RAM accesses */
static long *seq_bad;        /* calls per routine whose C would be misplaced in lockstep */

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
    if (seq_on && n >> 24 == R_WRAM) io_add(&cur->cseq, n & 0xFFFFFF, 0);
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
    if (seq_on && n >> 24 == R_WRAM) io_add(&cur->cseq, (n & 0xFFFFFF) | 1u << 24, 0);
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

/* SFXBOX_SEQ: place the C's work RAM accesses on the original's as the
   lockstep run does (each at the next original access of the same kind and
   address; a high byte written straight after its low byte with the
   original's write just before). An access placed past an original access
   to the same address that it left unmatched means the C reads or writes
   that byte more often, or in another order, than the original: in a
   lockstep call that spans frames or runs original code, the misplacement
   moves the rest of the call. */
static int u32cmp(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return x < y ? -1 : x > y;
}

static void seq_report(Watch *w, const Routine *r, const char *what, uint32_t c, int k, uint32_t j)
{
    if (seq_bad[w->idx]++ < 3)
        fprintf(report, "SEQ %06X %s call %ld: %s: C access %d (%s %06X)%s; %d C / %d original accesses\n", r->addr, r->name,
                stats[w->idx].calls, what, k, (c >> 24) ? "wr" : "rd", c & 0xFFFFFF, j ? " placed past an original access it needs later" : "",
                w->cseq.n, w->oseq.n);
}

static void seq_compare(Watch *w, const Routine *r)
{
    /* More accesses of a kind to an address than the original makes: the
       extra one matches a later access outside the call. */
    uint32_t nc = (uint32_t)w->cseq.n, no = (uint32_t)w->oseq.n;
    uint32_t *cs = malloc((nc + 1) * sizeof *cs), *os = malloc((no + 1) * sizeof *os);
    for (uint32_t i = 0; i < nc; i++) cs[i] = w->cseq.e[i].addr;
    for (uint32_t i = 0; i < no; i++) os[i] = w->oseq.e[i].addr;
    qsort(cs, nc, sizeof *cs, u32cmp); qsort(os, no, sizeof *os, u32cmp);
    for (uint32_t i = 0, j = 0; i < nc;) {
        uint32_t v = cs[i], ci = 0, oj = 0;
        while (i < nc && cs[i] == v) { i++; ci++; }
        while (j < no && os[j] < v) j++;
        while (j < no && os[j] == v) { j++; oj++; }
        if (ci > oj && oj) {
            int k = 0;
            while (k < w->cseq.n && w->cseq.e[k].addr != v) k++;
            seq_report(w, r, "extra", v, k, 0);
            free(cs); free(os);
            return;
        }
    }
    free(cs); free(os);
    /* An original access skipped over that a later C access of the same
       kind and address would have matched: that one now matches later. */
    uint32_t i = 0, pj = 0, plo = 0xFFFFFFFF;
    for (int k = 0; k < w->cseq.n; k++) {
        uint32_t c = w->cseq.e[k].addr, a = c & 0xFFFFFF;
        if ((c >> 24) && plo != 0xFFFFFFFF && a == plo + 1 && pj > 0 && w->oseq.e[pj - 1].addr == c) { plo = 0xFFFFFFFF; continue; }
        plo = 0xFFFFFFFF;
        uint32_t j = i;
        while (j < no && w->oseq.e[j].addr != c) j++;
        if (j == no) continue;
        for (uint32_t q = i; q < j; q++) {
            uint32_t o = w->oseq.e[q].addr;
            if ((o & 0xFFFFFF) != a) continue;
            for (int k2 = k + 1; k2 < w->cseq.n; k2++)
                if (w->cseq.e[k2].addr == o) { seq_report(w, r, "stranded", c, k, j); return; }
        }
        if (c >> 24) { plo = a; pj = j; }
        i = j + 1;
    }
}

static void finish(Watch *w)
{
    Stat *st = &stats[w->idx];
    const Routine *r = &routines[w->idx];
    struct SfxboxRegs out;
    core_regs(&out);
    st->calls++;

    cpu.a = w->entry.a; cpu.x = w->entry.x; cpu.y = w->entry.y; cpu.s = w->entry.s; cpu.d = w->entry.d;
    cpu.db = w->entry.db; cpu.p = w->entry.p; cpu.e = w->entry.e; cpu.pc = w->entry.pc; cpu.unwound = 0;
    map_clear(&w->cw); w->cwrite.n = 0; w->cread = 0; w->io_mismatch = 0; w->cseq.n = 0;
    cur = w;
    c_accesses = 0; c_runaway = 0;
    c_running = 1;
    if (setjmp(abort_c) == 0) r->abi();
    c_running = 0;

    if (seq_on && !c_runaway && !w->needs_asm) seq_compare(w, r);

    char why[512] = "";
    int bad = 0;
#define DIFFR(f) if (cpu.f != out.f) { bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " %s=%X/%X", #f, (unsigned)cpu.f, (unsigned)out.f); }
    DIFFR(a) DIFFR(x) DIFFR(y) DIFFR(d) DIFFR(db) DIFFR(p) DIFFR(e)
    /* A routine that left through its caller's frame: the C must too, to
       the same place. */
    if (!c_runaway && (cpu.unwound || out.s != w->exit_s)) {
        if (!cpu.unwound || out.s == w->exit_s) {
            bad = 1; snprintf(why + strlen(why), sizeof why - strlen(why), " unwound=%d/%d", cpu.unwound, out.s != w->exit_s);
        } else { DIFFR(s) DIFFR(pc) }
    }
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
    /* A call that started the Super FX reads results the Super FX wrote
       while it ran; the C, run afterwards against the entry state, cannot
       see them, so such a call is only judged by the lockstep run. */
    /* A call whose C reached a routine that has no C yet (through
       call_routine) cannot be finished here; the lockstep run judges it. */
    /* The original set S (TXS, TCS: the game's start resetting the
       stack) and left the call's frame that way; the C goes on past where
       the original's call was taken as ended, so only lockstep's frames
       can judge it. */
    int reset_left = w->stack_reset && !cpu.unwound && out.s != w->exit_s;
    int unsure = w->interrupted || w->dma || w->gsu || w->needs_asm || reset_left;
    if (unsure) st->inconclusive++; else st->diff++;
    if (st->shown < 8) {
        st->shown++;
        fprintf(report, "%s %06X %s call %ld: entry A=%04X X=%04X Y=%04X S=%04X D=%04X DB=%02X P=%02X%s; C/orig:%s\n",
                unsure ? "INCONCLUSIVE" : "DIFF", r->addr, r->name, st->calls, w->entry.a, w->entry.x, w->entry.y,
                w->entry.s, w->entry.d, w->entry.db, w->entry.p, w->interrupted ? " (interrupted)" : w->dma ? " (dma)" : w->gsu ? " (super fx ran)" : w->needs_asm ? " (calls original code)" : "", why);
    }
}

/* Interrupts (and unit calls) taken and not yet returned from: where each
   resumes and the opcode before it, so the instruction it resumes at is
   judged by that opcode, not the handler's RTI (a raster IRQ between a
   JSR and the routine's first instruction would hide the call). */
static struct { uint32_t pc; uint16_t s; uint8_t op; } irqs[16];
static int nirqs;

/* A unit call entered by a jump (sfxbox call frame=): where it starts (PC
   and S) and where it ends, given since no JSR or JSL says. */
static struct { int set; uint32_t pc, ret; uint16_t exit_s, entry_s; } ue;

void check_unit_entry(uint32_t pc, uint32_t ret, uint16_t exit_s, uint16_t entry_s)
{
    ue.set = 1; ue.pc = pc; ue.ret = ret; ue.exit_s = exit_s; ue.entry_s = entry_s;
}

static void on_insn(uint32_t pc)
{
    struct SfxboxRegs r;
    int have = 0;
    uint8_t prev_op = *core_opcode;
    if (prev_op == 0x9A || prev_op == 0x1B)
        for (int i = 0; i < depth; i++)
            if (!ws[i].in_irq) ws[i].stack_reset = 1;
    while (nirqs && pc == irqs[nirqs - 1].pc) {
        if (!have) { core_regs(&r); have = 1; }
        if (r.s != irqs[nirqs - 1].s) break;
        prev_op = irqs[--nirqs].op;
    }
    for (int i = 0; i < depth; i++)
        if (ws[i].in_irq && pc == ws[i].irq_pc) {
            if (!have) { core_regs(&r); have = 1; }
            if (r.s == ws[i].irq_s) ws[i].in_irq = 0;
        }
    /* A watch ends at its return, or when an RTS or RTL takes S above its
       exit level: the routine left through its caller's frame, which may
       end the caller's watch at the same place. (An RTI is an interrupt
       handler's: a routine that stops the game may reset S above its frame
       and wait there for interrupts.) */
    while (depth) {
        Watch *t = &ws[depth - 1];
        if (t->in_irq) break;
        if (!have) { core_regs(&r); have = 1; }
        if ((pc == t->ret_pc && r.s == t->exit_s)
            || (r.s > t->exit_s && (prev_op == 0x60 || prev_op == 0x6B))) { finish(t); depth--; }
        else break;
    }
    if (!(watched[pc >> 3] >> (pc & 7) & 1)) return;
    int idx = -1;
    for (int i = 0; i < nroutines; i++) if (routines[i].addr == pc) { idx = i; break; }
    if (idx < 0) return;
    if (!have) { core_regs(&r); have = 1; }
    uint8_t via = prev_op;
    uint32_t sp = r.s;
    if (sp + 3 >= wram_n || r.e || depth == DEPTH) { stats[idx].skipped++; return; }
    Watch *w = &ws[depth];
    uint16_t word = (uint16_t)(wram[(sp + 1) & 0xFFFF] | wram[(sp + 2) & 0xFFFF] << 8);
    if (ue.set && pc == ue.pc && r.s == ue.entry_s) {
        w->ret_pc = ue.ret;
        w->exit_s = ue.exit_s;
        ue.set = 0;
    } else if (via == 0x20 || via == 0xFC) {
        w->ret_pc = (pc & 0xFF0000) | (uint16_t)(word + 1);
        w->exit_s = (uint16_t)(sp + 2);
    } else if (via == 0x22) {
        w->ret_pc = (uint32_t)wram[(sp + 3) & 0xFFFF] << 16 | (uint16_t)(word + 1);
        w->exit_s = (uint16_t)(sp + 3);
    } else { stats[idx].skipped++; return; }
    w->idx = idx; w->entry = r;
    w->in_irq = w->interrupted = w->dma = w->gsu = w->needs_asm = w->overflow = 0;
    w->stack_reset = 0;
    map_clear(&w->undo); map_clear(&w->orig);
    w->oread.n = w->owrite.n = w->oseq.n = 0;
    depth++;
}

static void on_write(uint32_t addr, uint8_t data, int kind)
{
    if (norm(addr) >> 24 == R_GSURAM) gsu_check_foreign(0x700000u | (norm(addr) & 0xFFFFFF));
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
        if (seq_on && n >> 24 == R_WRAM) io_add(&w->oseq, (n & 0xFFFFFF) | 1u << 24, 0);
        else if (n >> 24 == R_IO) {
            io_add(&w->owrite, addr & 0xFFFF, data);
            if ((addr & 0xFFFF) == 0x301F) w->gsu = 1;  /* R15 high byte: the Super FX starts */
        }
    }
}

static void on_read(uint32_t addr, uint8_t data)
{
    if (!depth) return;
    if (seq_on && norm(addr) >> 24 == R_WRAM) {
        uint32_t n = norm(addr) & 0xFFFFFF;
        for (int i = 0; i < depth; i++) if (!ws[i].in_irq) io_add(&ws[i].oseq, n, 0);
        return;
    }
    if (norm(addr) >> 24 != R_IO) return;
    for (int i = 0; i < depth; i++)
        if (!ws[i].in_irq) io_add(&ws[i].oread, addr & 0xFFFF, data);
}

static void on_interrupt(uint32_t resume_pc, uint16_t s)
{
    if (nirqs < 16) {
        irqs[nirqs].pc = resume_pc; irqs[nirqs].s = s; irqs[nirqs].op = *core_opcode;
        nirqs++;
    }
    for (int i = 0; i < depth; i++)
        if (!ws[i].in_irq) { ws[i].in_irq = 1; ws[i].interrupted = 1; ws[i].irq_pc = resume_pc; ws[i].irq_s = s; }
}

/* A unit call (sfxbox call) starting while calls are open suspends them as
   an interrupt would: it can change what they read (the PPU multiplier,
   work RAM), so a call it overlaps is judged inconclusive. */
void check_unit_call(uint32_t resume_pc, uint16_t s)
{
    on_interrupt(resume_pc, s);
}

/* The calls open now, and dropping those opened since: a unit call puts
   the machine back as it was, so calls begun after that point never
   happened. One still open from before is judged inconclusive. */
int check_depth(void) { return depth; }

void check_rollback(int d)
{
    nirqs = 0;
    if (d < depth) depth = d;
    for (int i = 0; i < depth; i++) ws[i].interrupted = 1;
}

/* ---- setup and report ---- */

static int load_routines(void *core, const char *lib, const char *only)
{
    void *h = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
    if (!h) { fprintf(stderr, "dlopen %s: %s\n", lib, dlerror()); return -1; }
    const Routine *table = dlsym(h, "sfx_routine_table");
    const uint32_t *cat = dlsym(h, "sfx_catalog");
    const int *catn = dlsym(h, "sfx_catalog_count");
    if (cat && catn)
        for (int i = 0; i < *catn; i++) catalog_bits[cat[i] >> 3] |= (uint8_t)(1 << (cat[i] & 7));
    const uint32_t *jmp = dlsym(h, "sfx_jump_entries");
    const int *jmpn = dlsym(h, "sfx_jump_entry_count");
    if (jmp && jmpn)
        for (int i = 0; i < *jmpn; i++) jump_bits[jmp[i] >> 3] |= (uint8_t)(1 << (jmp[i] & 7));
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
    /* A selected routine's other addresses (@alias) come with it. */
    if (only)
        for (int i = 0; i < *count && nroutines < 4096; i++) {
            int have = 0, alias = 0;
            for (int j = 0; j < nroutines; j++) {
                if (sel[j].addr == table[i].addr) have = 1;
                if (sel[j].abi == table[i].abi) alias = 1;
            }
            if (alias && !have) sel[nroutines++] = table[i];
        }
    routines = sel;
    all_routines = table; all_count = *count;
    stats = calloc((size_t)(nroutines ? nroutines : 1), sizeof *stats);
    watched = calloc(1 << 21, 1);
    for (int i = 0; i < nroutines; i++) watched[routines[i].addr >> 3] |= (uint8_t)(1 << (routines[i].addr & 7));
    has_c = calloc(1 << 21, 1);
    for (int i = 0; i < all_count; i++) has_c[all_routines[i].addr >> 3] |= (uint8_t)(1 << (all_routines[i].addr & 7));
    if (gsu_load(core, h, only) < 0) return -1;
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
    seq_on = getenv("SFXBOX_SEQ") != NULL;
    seq_bad = calloc((size_t)(nroutines ? nroutines : 1), sizeof *seq_bad);
    static struct SfxboxCallbacks cb = {on_insn, on_write, on_read, on_interrupt};
    set_cb(&cb);
    gsu_check_begin(report);
    return 0;
}

void check_end(void)
{
    fprintf(report, "routine,name,calls,match,diff,inconclusive,skipped,verdict\n");
    /* A routine's other addresses (@alias: its code run from a copy) are
       counted under its first. */
    for (int i = 0; i < nroutines; i++)
        for (int j = 0; j < i; j++)
            if (routines[j].abi == routines[i].abi) {
                stats[j].calls += stats[i].calls; stats[j].match += stats[i].match; stats[j].diff += stats[i].diff;
                stats[j].inconclusive += stats[i].inconclusive; stats[j].skipped += stats[i].skipped;
                stats[i].calls = -1;
                break;
            }
    for (int i = 0; i < nroutines; i++) {
        Stat *s = &stats[i];
        if (s->calls < 0) continue;
        const char *v = s->diff ? "DIVERGENT" : s->calls == 0 ? "UNCOVERED" : s->inconclusive ? "UNVERIFIED" : "IDENTICAL";
        fprintf(report, "%06X,%s,%ld,%ld,%ld,%ld,%ld,%s\n", routines[i].addr, routines[i].name, s->calls, s->match,
                s->diff, s->inconclusive, s->skipped, v);
        printf("%06X %-28s calls %ld match %ld diff %ld inconclusive %ld  %s\n", routines[i].addr, routines[i].name,
               s->calls, s->match, s->diff, s->inconclusive, v);
    }
    gsu_check_rows(report);
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

typedef struct { long frame; uint64_t opoff; uint32_t opn; int done, cut; } CallRec;
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
    uint64_t o_units;                 /* unit calls (sfxbox call) the original has run */
    long unit_end[256];               /* the frame each of them ended its frames at */
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
/* SFXBOX_LS_CALLS=frame: from that frame, each swapped call as each side
   opens it (the original's on stderr) and the original's returns. */
static long calls_from = -1;
static const char *ls_cur_name;
static void stuck_report(const char *cond)
{
    fprintf(stderr, "lockstep: %s side stuck in frame %ld waiting for %s; frames o/h %ld/%ld, calls o/h %llu/%llu,"
            " ops o/h %llu/%llu\n", role == 1 ? "original" : "hybrid", cur_frame, cond, LOAD(sh->o_frames),
            LOAD(sh->h_frames), (unsigned long long)LOAD(sh->o_calls), (unsigned long long)LOAD(sh->h_calls),
            (unsigned long long)LOAD(sh->o_ops), (unsigned long long)LOAD(sh->h_ops));
    if (role == 2) {
        uint64_t k = LOAD(sh->h_calls);
        fprintf(stderr, "lockstep: hybrid at call %llu (%s); that call: frame %ld, %u ops, done %d\n",
                (unsigned long long)k, ls_cur_name ? ls_cur_name : "-", sh->calls[k % CALLRING].frame,
                sh->calls[k % CALLRING].opn, sh->calls[k % CALLRING].done);
    }
    if (lsrep) fflush(lsrep);
}

/* Wait for a condition the other side will make true; give up (and say so)
   if it never does, rather than hang. */
/* The hybrid is a child of the original's process: if that one is killed
   (a timeout), the child must not run on alone. */
static pid_t parent_pid;
#define PARENT_GONE() (role == 2 && getppid() != parent_pid)
#define WAIT_UNTIL(cond) do { long spins_ = 0; while (!(cond)) { \
        if (LOAD(sh->stuck) || PARENT_GONE()) _exit(6); \
        if (++spins_ > 600000) { stuck_report(#cond); STORE(sh->stuck, 1); _exit(6); } usleep(50); } } while (0)

static struct {
    int active, in_irq;
    uint32_t ret_pc, irq_pc;
    uint16_t exit_s, irq_s;
    uint64_t call;
    int depth;               /* catalog routines entered inside the call */
    uint8_t pre_irq_op;      /* the last opcode before an interrupt */
    int stack_reset;         /* a TXS or TCS ran in the call */
    int noreturn;            /* a unit call marked noreturn */
    int irq_resumed;         /* the handler has just returned */
    uint16_t open[64];       /* their stack pointers at entry */
    uint32_t open_pc[64];    /* and their entries */
    uint16_t entry_s;        /* S at the call's own entry */
    uint32_t ret[0x10000];   /* by S at a callee's entry: 1 << 24 | its return address */
} m;
static uint32_t m_pc;        /* the routine of the open call */

static struct { int set; uint32_t pc, ret; uint16_t exit_s, entry_s; } lue;

/* The routine a unit call marked noreturn enters (sfxbox call noreturn). */
static uint32_t ls_noreturn_pc = (uint32_t)-1;

void lockstep_unit_noreturn(uint32_t pc)
{
    ls_noreturn_pc = pc;
}

void lockstep_unit_entry(uint32_t pc, uint32_t ret, uint16_t exit_s, uint16_t entry_s)
{
    lue.set = 1; lue.pc = pc; lue.ret = ret; lue.exit_s = exit_s; lue.entry_s = entry_s;
}

static int entry(uint32_t pc, struct SfxboxRegs *r, uint32_t *ret, uint16_t *exit_s)
{
    uint8_t via = *core_opcode;
    uint32_t sp = r->s;
    if (lue.set && pc == lue.pc && r->s == lue.entry_s) {
        *ret = lue.ret;
        *exit_s = lue.exit_s;
        lue.set = 0;
        return 1;
    }
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

static int timed(uint32_t a);
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
    if (timed(addr)) fprintf(trf, "  rd %06X=%02X @%u\n", addr, data, core_clock());
}
static void trace_write(uint32_t addr, uint8_t data, int kind)
{
    if (!trf || cur_frame < tr_lo || cur_frame > tr_hi) return;
    if (timed(addr)) fprintf(trf, "  wr %06X=%02X k%d @%u\n", addr, data, kind, core_clock());
}

/* Original side: measure each call's cost. */
static void ls_op(uint8_t op, uint32_t addr);

/* S at the start of the instruction now running: a frame can end inside
   an instruction that pushes, with S already moved down past a byte whose
   write has not landed yet. */
static uint16_t insn_s;

static void ls_insn(uint32_t pc)
{
    trace_insn(pc);
    {
        struct SfxboxRegs r; core_regs(&r);
        insn_s = r.s;
    }
    if (m.active) {
        if (m.in_irq) {
            if (pc != m.irq_pc) return;
            struct SfxboxRegs r; core_regs(&r);
            if (r.s != m.irq_s) return;
            m.in_irq = 0;  /* back from the handler: this may also be the return */
            m.irq_resumed = 1;
        }
        struct SfxboxRegs r; core_regs(&r);
        /* Inside the call, the entry into any routine the catalog knows is
           marked (op 9: its address, op A: S), and so is its return, the
           first instruction with S above its entry S (op C), so that C can
           run that routine as original code at the same place (call_routine). */
        while (m.depth && r.s > m.open[m.depth - 1]) {
            m.depth--;
            ls_op(0xC0, pc);
        }
        /* The previous instruction's opcode (before an interrupt the
           original took between it and this one). */
        uint8_t prev = m.irq_resumed ? m.pre_irq_op : *core_opcode;
        /* The call ends at its return, or where a return took S above its
           exit level: it left through its caller's frame. */
        /* A unit call marked noreturn that set S itself (the game's start
           resetting the stack) has left every frame below it and never
           returns: a return above its exit level after that is its own
           code's, not its end. (In play, such a call ends there, as the
           hybrid's original code does: leave_frame.) */
        if (m.noreturn && (prev == 0x9A || prev == 0x1B))
            m.stack_reset = 1;
        if ((pc == m.ret_pc && r.s == m.exit_s)
            || (!m.stack_reset && r.s > m.exit_s && (prev == 0x60 || prev == 0x6B))) {
            if (calls_from >= 0 && cur_frame >= calls_from)
                fprintf(stderr, "ODONE frame %ld call %llu at %06X S=%04X\n", cur_frame, (unsigned long long)m.call, pc, r.s);
            STORE(sh->calls[m.call % CALLRING].done, 1);
            m.active = 0;
            m.irq_resumed = 0;
            return;
        }
        /* After an RTL, RTS, JMP/JML through memory or JSR (abs,X), this
           instruction may be a routine entered through a computed address (a
           handler pushed for an RTL, a jump table), which C reaches with
           call_routine. */
        m.irq_resumed = 0;
        /* A JSR or JSL has just pushed its return address: remember it by
           the callee's S, so the RTS or RTL that comes back to it is known
           as a return. Counted as an entry, every return would open a level
           at the caller's S that only closes when the caller returns, and a
           caller making many calls would fill the 64 levels. */
        if (prev == 0x20 || prev == 0xFC) {
            uint16_t w = (uint16_t)(wram[(r.s + 1) & 0xFFFF] | wram[(r.s + 2) & 0xFFFF] << 8);
            m.ret[r.s] = 0x1000000 | (pc & 0xFF0000) | (uint16_t)(w + 1);
        } else if (prev == 0x22) {
            uint16_t w = (uint16_t)(wram[(r.s + 1) & 0xFFFF] | wram[(r.s + 2) & 0xFFFF] << 8);
            m.ret[r.s] = 0x1000000 | (uint32_t)wram[(r.s + 3) & 0xFFFF] << 16 | (uint16_t)(w + 1);
        }
        int returned = 0;
        if (prev == 0x60 || prev == 0x6B) {
            uint16_t at = (uint16_t)(r.s - (prev == 0x60 ? 2 : 3));
            returned = m.ret[at] == (pc | 0x1000000);
            if (returned) m.ret[at] = 0;
        }
        /* Or a jump or branch (BRL, JML, JMP, BRA, Bxx) to code the C
           reaches with call_routine or jump_routine that the catalog does
           not have as an entry (07:9EE9, the tail jump of BigSpritesRow6):
           only those, since a jump inside a loop would open a level each
           time round. */
        int computed = (!returned && (prev == 0x6B || prev == 0x60)) || prev == 0x6C || prev == 0x7C || prev == 0xDC || prev == 0xFC
                       || ((prev == 0x82 || prev == 0x5C || prev == 0x4C || prev == 0x80 || (prev & 0x1F) == 0x10)
                           && is_jump_entry(pc));
        /* Back at the entry of the level open at this S (the call's own,
           or a routine's inside it) by a branch: a loop, not an entry. */
        int again = !computed && prev != 0x20 && prev != 0x22 && prev != 0xFC
                    && ((m.depth && m.open_pc[m.depth - 1] == pc && m.open[m.depth - 1] == r.s)
                        || (!m.depth && pc == m_pc && r.s == m.entry_s));
        /* Entries of routines with C count too: an @alias address (a work
           RAM routine run from its ROM copy) is not in the catalog. */
        int with_c = has_c[pc >> 3] >> (pc & 7) & 1;
        if ((computed || is_catalog_entry(pc) || with_c) && !again && m.depth < 64) {
            ls_op(0x90, pc);
            ls_op(0xA0, r.s);
            /* Op B: the return address a JSR or JSL pushed to enter it
               (its byte count in the top byte), so that the hybrid can put
               back the frames of calls its C made without pushing one. */
            if (prev == 0x20 || prev == 0xFC)
                ls_op(0xB0, 2u << 24 | wram[(r.s + 1) & 0xFFFF] | (uint32_t)wram[(r.s + 2) & 0xFFFF] << 8);
            else if (prev == 0x22)
                ls_op(0xB0, 3u << 24 | wram[(r.s + 1) & 0xFFFF] | (uint32_t)wram[(r.s + 2) & 0xFFFF] << 8
                            | (uint32_t)wram[(r.s + 3) & 0xFFFF] << 16);
            m.open_pc[m.depth] = pc;
            m.open[m.depth++] = r.s;
        }
        return;
    }
    if (routine_at(pc) < 0) return;
    struct SfxboxRegs r; core_regs(&r);
    if (!entry(pc, &r, &m.ret_pc, &m.exit_s)) return;
    uint64_t k = LOAD(sh->o_calls);
    WAIT_UNTIL(k - LOAD(sh->h_calls) < CALLRING);
    CallRec *rec = &sh->calls[k % CALLRING];
    rec->frame = cur_frame; rec->opoff = LOAD(sh->o_ops); rec->opn = 0;
    STORE(rec->cut, 0);
    STORE(rec->done, 0);
    STORE(sh->o_calls, k + 1);
    sh->fs[cur_frame % NSLOT].ncalls++;
    if (calls_from >= 0 && cur_frame >= calls_from)
        fprintf(stderr, "OCALL frame %ld call %llu %06X S=%04X\n", cur_frame, (unsigned long long)k, pc, r.s);
    m.active = 1; m.in_irq = 0; m.call = k; m.depth = 0; m.entry_s = r.s; m.stack_reset = 0;
    m.noreturn = ls_noreturn_pc == pc;
    ls_noreturn_pc = (uint32_t)-1;
    m_pc = pc;
}


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
    m.pre_irq_op = *core_opcode;
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

/* The call's record is taken while the original is still making it: a call
   that runs for many frames (a screen's loop, a phase handler) would
   otherwise have the hybrid wait at its start for more frames than the
   original may run ahead, and neither side would move. cur_rec is the
   record, cur_open while more of it may come. fetch_more copies what has
   come (giving its place in the ring back) until there are `need` ops or
   the record is complete; 1 when there are. */
static CallRec *cur_rec;
static int cur_open;
/* The record ended at a state load in the script (lockstep_state_loaded):
   the original's call was cut there, at a frame's end, and never returned. */
static int cur_cut;
static void hybrid_finish(long frames);
static int past_end;
/* What has come of the record so far, without waiting (each frame, so the
   ring keeps moving while the hybrid runs original code in a long call). */
static void fetch_avail(void)
{
    if (!cur_open || !cur_rec) return;
    uint64_t top = LOAD(cur_rec->done) ? cur_rec->opoff + cur_rec->opn : LOAD(sh->o_ops);
    uint32_t avail = (uint32_t)(top - cur_rec->opoff);
    if (avail <= cur_n) return;
    if (avail > cur_cap) {
        cur_cap = avail + (avail >> 1) + 1024;
        cur_ops = realloc(cur_ops, cur_cap);
        cur_addr = realloc(cur_addr, (size_t)cur_cap * sizeof *cur_addr);
        if (!cur_ops || !cur_addr) _exit(7);
    }
    for (uint32_t i = cur_n; i < avail; i++) {
        uint64_t o = (cur_rec->opoff + i) % OPRING;
        cur_ops[i] = sh->ops[o];
        cur_addr[i] = sh->opaddr[o];
    }
    cur_n = avail;
    STORE(sh->h_ops, cur_rec->opoff + cur_n);
}

static int fetch_more(uint32_t need)
{
    long spins = 0;

    while (cur_n < need) {
        if (!cur_open) return 0;
        int done = LOAD(cur_rec->done);
        /* Ops are published by o_ops; while the call is open every new op
           is its own, and once it is done its count is final. */
        uint64_t top = done ? cur_rec->opoff + cur_rec->opn : LOAD(sh->o_ops);
        uint32_t avail = (uint32_t)(top - cur_rec->opoff);
        if (avail > cur_n) {
            if (avail > cur_cap) {
                cur_cap = avail + (avail >> 1) + 1024;
                cur_ops = realloc(cur_ops, cur_cap);
                cur_addr = realloc(cur_addr, (size_t)cur_cap * sizeof *cur_addr);
                if (!cur_ops || !cur_addr) _exit(7);
            }
            for (uint32_t i = cur_n; i < avail; i++) {
                uint64_t o = (cur_rec->opoff + i) % OPRING;
                cur_ops[i] = sh->ops[o];
                cur_addr[i] = sh->opaddr[o];
            }
            cur_n = avail;
            STORE(sh->h_ops, cur_rec->opoff + cur_n);
            spins = 0;
            continue;
        }
        if (done) { cur_open = 0; cur_cut = LOAD(cur_rec->cut); return 0; }
        if (LOAD(sh->o_done)) {
            /* The script ended on the original side inside this call: as
               for a call that never returned (ls_exec), nothing after it
               can be compared. */
            past_end = 1;
            gsu_lockstep_past_end();
            hybrid_finish(cur_frame + 1);
        }
        if (LOAD(sh->stuck) || PARENT_GONE()) _exit(6);
        if (++spins > 600000) { stuck_report("more of the call's record"); STORE(sh->stuck, 1); _exit(6); }
        usleep(50);
    }
    return 1;
}
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

/* The core replays a copy of the ops: a frame can end inside a replay (a
   long DMA), and the main loop then drains more of the record
   (fetch_avail), which may move cur_ops. */
static void replay_ops(uint32_t from, uint32_t n)
{
    static uint8_t *buf;
    static uint32_t cap;

    if (n > cap) {
        cap = n + (n >> 1) + 1024;
        buf = realloc(buf, cap);
        if (!buf) _exit(7);
    }
    memcpy(buf, cur_ops + from, n);
    /* The core polls for interrupts with its own P (the I flag), which
       stays at the call's entry while the C runs: it is given the C's. */
    {
        struct SfxboxRegs r;

        core_regs(&r);
        r.p = cpu.p;
        core_set_regs(&r);
    }
    core_replay(buf, n);
}

/* Replay the recorded cycles [cur_i, to), taking each recorded interrupt at
   its place: the core takes the pending interrupt from the original's
   registers and runs the handler until it returns. */
static void replay_to(uint32_t to)
{
    uint32_t i = cur_i, run = cur_i;
    while (i < to) {
        if (cur_ops[i] >> 4 != 5) { i++; continue; }
        if (i > run) replay_ops(run, i - run);
        if (i + 3 < cur_n || fetch_more(i + 4)) {
            struct SfxboxRegs at = {
                (uint16_t)cur_addr[i + 1], (uint16_t)(cur_addr[i + 1] >> 16), (uint16_t)cur_addr[i + 2],
                (uint16_t)(cur_addr[i + 2] >> 16), (uint16_t)cur_addr[i + 3], (uint8_t)(cur_addr[i + 3] >> 16),
                (uint8_t)(cur_addr[i + 3] >> 24), (uint8_t)(cur_addr[i] >> 24), cur_addr[i] & 0xFFFFFF};
            struct SfxboxRegs mine; core_regs(&mine);
            if (!core_interrupt_here(&at) && irq_missed++ < 5)
                fprintf(lsrep, "OFF frame %ld: interrupt recorded at %06X (op %u of %u) not pending there (in %s; clock %u)\n",
                        cur_frame, at.pc, i, cur_n, ls_cur_name ? ls_cur_name : "-", core_clock());
            core_set_regs(&mine);
        }
        i += 4;
        run = i;
    }
    if (to > run) replay_ops(run, to - run);
    cur_i = to;
}

static long snap_offset(uint32_t a);
static void (*core_run_until)(const struct SfxboxRegs *, uint32_t, uint16_t);
static void (*core_run_out)(const struct SfxboxRegs *, uint16_t);
static long asm_unplaced;

/* The hybrid's report, and its end. */
static void hybrid_finish(long frames)
{
    char g[256];
    int gbad = gsu_lockstep_summary(g, sizeof g);
    fprintf(lsrep, "frames %ld compared %ld, differing %ld; calls swapped %ld, entered other ways %ld, unmatched %ld, "
            "hardware accesses off the timeline %ld, interrupts not pending where taken %ld, original-code calls not placed %ld%s: %s\n",
            frames, frames_compared, diff_frames, calls_swapped, calls_fallback, mismatched_calls, timed_unmatched, irq_missed, asm_unplaced,
            g, diff_frames || mismatched_calls || timed_unmatched || irq_missed || asm_unplaced || gbad ? "DIVERGENT" : calls_swapped || gsu_count() ? "IDENTICAL" : "UNCOVERED");
    fclose(lsrep);
    _exit(0);
}

/* call_routine for a target without C, on the live machine: replay the
   original's cycles to where it entered the routine (op 9), run the
   routine as original code from there with the original's stack pointer
   until it returns (to a stand-in return address), and go on in the
   record after its return (op C). */
static void record_rest(void);
static void leave_frame(struct SfxboxRegs *out);
static jmp_buf ls_escape;
static int ls_escape_set;

static void run_original(uint32_t addr, int long_call)
{
    uint32_t j, c = cur_n;
    int nest = 0;
    uint16_t s = (uint16_t)(cpu.s - (long_call ? 3 : 2));

    int found;
    for (j = cur_i; j + 1 < cur_n || fetch_more(j + 2); j++)
        if (cur_ops[j] >> 4 == 9 && cur_addr[j] == addr && cur_ops[j + 1] >> 4 == 0xA) break;
    found = j + 1 < cur_n;
    if (found) {
        replay_to(j);
        s = (uint16_t)cur_addr[j + 1];
    } else if (!past_end) {
        /* Past the end, there is no record to place it in (as for a
           hardware access): nothing after it is compared. */
        if (asm_unplaced++ < 5) fprintf(lsrep, "OFF frame %ld: call to %06X not in the original's record\n", cur_frame, addr);
    }
    /* The return address: the one the original's JSR or JSL pushed (op B
       after the entry's marks), so the stack holds what the original's
       does while the routine runs (a frame can end there); else a
       stand-in, $FFFF, so the return lands on $0000 (bank 0 after an RTL,
       the routine's bank after an RTS). */
    uint32_t back = long_call ? 0x000000 : (addr & 0xFF0000);
    if (found && j + 2 < cur_n && cur_ops[j + 2] >> 4 == 0xB && cur_addr[j + 2] >> 24 == (uint32_t)(long_call ? 3 : 2)) {
        uint32_t v = cur_addr[j + 2];
        core_bus_write((uint32_t)(uint16_t)(s + 1), (uint8_t)v);
        core_bus_write((uint32_t)(uint16_t)(s + 2), (uint8_t)(v >> 8));
        if (long_call) core_bus_write((uint32_t)(uint16_t)(s + 3), (uint8_t)(v >> 16));
        back = (long_call ? (v & 0xFF0000) : (addr & 0xFF0000)) | (uint16_t)((v & 0xFFFF) + 1);
    } else {
        core_bus_write((uint32_t)(uint16_t)(s + 1), 0xFF);
        core_bus_write((uint32_t)(uint16_t)(s + 2), 0xFF);
        if (long_call) core_bus_write((uint32_t)(uint16_t)(s + 3), 0x00);
    }
    struct SfxboxRegs from = {cpu.a, cpu.x, cpu.y, s, cpu.d, cpu.db, cpu.p, cpu.e, addr}, out, mine;
    core_regs(&mine);
    core_run_out(&from, s);
    core_regs(&out);
    core_set_regs(&mine);
    cpu.a = out.a; cpu.x = out.x; cpu.y = out.y; cpu.d = out.d; cpu.db = out.db; cpu.p = out.p; cpu.e = out.e;
    /* Not back at the return address: it left through the C's frame
       (02:E33D's way out of a loop); the C is abandoned where it landed. */
    if (out.pc != back || out.s != (uint16_t)(s + (long_call ? 3 : 2))) {
        leave_frame(&out);
        cpu.a = out.a; cpu.x = out.x; cpu.y = out.y; cpu.d = out.d; cpu.db = out.db; cpu.p = out.p; cpu.e = out.e;
        cpu.s = out.s; cpu.pc = out.pc; cpu.unwound = 1;
        if (found) record_rest();
        else cur_i = cur_n;
        if (ls_escape_set) longjmp(ls_escape, 1);
        return;
    }
    /* Its return mark, looked for only now: a routine that runs for many
       frames has its return far ahead in a record still being made. */
    if (found)
        for (c = j + 2; c < cur_n || fetch_more(c + 1); c++) {
            if (cur_ops[c] >> 4 == 9) nest++;
            else if (cur_ops[c] >> 4 == 0xC && !nest--) break;
        }
    cur_i = c < cur_n ? c + 1 : cur_n;
    /* An interrupt the original took straight after the routine's return
       (between its RTS/RTL and the next instruction at the caller's level)
       is recorded before the return mark: the run above stops at the stand-in
       return with it still pending, so it is left in the record to be taken
       at its place; return marks of inner levels closed there too come
       between them. */
    if (c < cur_n) {
        uint32_t e = c;
        while (e > j + 2 && cur_ops[e - 1] >> 4 == 0xC) e--;
        if (e >= j + 6 && cur_ops[e - 4] >> 4 == 5) cur_i = e - 4;
    }
}

/* jump_routine on the live machine: as run_original, but in the calling
   routine's frame (no stand-in return), run until S rises above it; the
   routine then ends where the original's run left it. */
/* The stack between the original's S where it entered the code a call or
   jump reaches (lo) and the hybrid's (hi): the frames the original's JSRs
   and JSLs pushed there for calls the C made directly, C to C, which push
   nothing. Put back from the record (op B), the latest at each place; the
   size of the innermost one (at lo), 0 when there is none. */
static unsigned restore_frames(uint32_t j, uint16_t lo, uint16_t hi)
{
    uint8_t put[64] = {0};
    unsigned need = (uint16_t)(hi - lo), have = 0, inner = 0;

    if (!need || need > sizeof put) return 0;
    for (uint32_t q = j; q-- > 1 && have < need;) {
        if (cur_ops[q] >> 4 != 0xB || cur_ops[q - 1] >> 4 != 0xA) continue;
        uint16_t at = (uint16_t)cur_addr[q - 1];
        unsigned n = cur_addr[q] >> 24, k, clash = 0;
        if (at < lo || at + n > hi) continue;
        for (k = 0; k < n; k++) clash |= put[at - lo + k];
        if (clash) continue;
        for (k = 0; k < n; k++) {
            core_bus_write((uint32_t)(uint16_t)(at + 1 + k), (uint8_t)(cur_addr[q] >> (8 * k)));
            put[at - lo + k] = 1;
        }
        if (at == lo) inner = n;
        have += n;
    }
    return inner;
}

/* Code run as original code has left the C's frame: the rest of the
   call's record is that code's, which has just run. (Its levels' return
   marks are no guide: a level whose code pulls its own return address is
   marked returned at the pull.) An interrupt the original took right at
   the landing is recorded last, before the return marks there, and is left
   to be taken in its place. */
static void record_rest(void)
{
    uint32_t e;

    while (fetch_more(cur_n + 1)) {}
    e = cur_n;
    while (e > cur_i && cur_ops[e - 1] >> 4 == 0xC) e--;
    cur_i = e >= cur_i + 4 && cur_ops[e - 4] >> 4 == 5 ? e - 4 : cur_n;
}

/* ls_escape is the C's way out when original code it called left through
   its caller's frame: the C is abandoned there (ls_exec). Code that lands
   still inside the swapped routine's call runs on as original code until
   the call ends as the original's record of it ends (ls_insn): at its
   return address with S at ls_exit_s, or at a return that takes S above
   ls_exit_s. A return to S between the routine's entry S and ls_exit_s
   elsewhere (code that has reset the stack and runs at its top, 0B:F7C3)
   is not the call's end. */
static uint16_t ls_exit_s;
static uint32_t ls_ret;
static void leave_frame(struct SfxboxRegs *out)
{
    struct SfxboxRegs mine;
    if (getenv("SFXBOX_LS_LEAVE"))
        fprintf(lsrep, "LEAVE frame %ld at %06X S=%04X exit_s %04X ret %06X\n", cur_frame, out->pc, out->s, ls_exit_s, ls_ret);
    core_regs(&mine);
    while (!(out->s > ls_exit_s || (out->s == ls_exit_s && out->pc == ls_ret))) {
        core_run_out(out, (uint16_t)(ls_exit_s - 1));
        core_regs(out);
    }
    if (getenv("SFXBOX_LS_LEAVE"))
        fprintf(lsrep, "LEFT frame %ld at %06X S=%04X\n", cur_frame, out->pc, out->s);
    core_set_regs(&mine);
}

static uint32_t ls_entry_pc;   /* where the C call ls_exec swapped in started */
static int ls_unit_noreturn;   /* that call is a unit call marked noreturn */

static void jump_original(uint32_t addr, const uint8_t *pushed, int n, int search)
{
    uint32_t j = 0, c = cur_n;
    int nest = 0;
    uint16_t s = (uint16_t)(cpu.s - n), hs = cpu.s;
    unsigned inner = 0;

    int found = 0;
    if (search) {
        for (j = cur_i; j + 1 < cur_n || fetch_more(j + 2); j++)
            if (cur_ops[j] >> 4 == 9 && cur_addr[j] == addr && cur_ops[j + 1] >> 4 == 0xA) break;
        found = j + 1 < cur_n;
    }
    if (found) {
        replay_to(j);
        s = (uint16_t)cur_addr[j + 1];
        inner = restore_frames(j, (uint16_t)(s + n), hs);
    } else if (search && !past_end) {
        if (asm_unplaced++ < 5) fprintf(lsrep, "OFF frame %ld: jump to %06X not in the original's record\n", cur_frame, addr);
    }
    /* What the original pushed before it went there, at its S. */
    for (int k = 0; k < n; k++)
        core_bus_write((uint32_t)(uint16_t)(s + n - k), pushed[k]);
    /* The return address in the innermost frame put back: the landing a
       return through it makes (low 16 bits). */
    uint16_t back = 0;
    if (found && inner)
        back = (uint16_t)((core_bus_read((uint32_t)(uint16_t)(s + n + 1)) | core_bus_read((uint32_t)(uint16_t)(s + n + 2)) << 8) + 1);
    struct SfxboxRegs from = {cpu.a, cpu.x, cpu.y, s, cpu.d, cpu.db, cpu.p, cpu.e, addr}, out, mine;
    core_regs(&mine);
    core_run_out(&from, s);
    core_regs(&out);
    core_set_regs(&mine);
    cpu.a = out.a; cpu.x = out.x; cpu.y = out.y; cpu.d = out.d; cpu.db = out.db; cpu.p = out.p; cpu.e = out.e;
    /* Called from C (a frame put back): back through that frame (at its S
       and its return address: code that reset the stack can return at
       that S from elsewhere) is the routine's return to the C that called
       it, which goes on. */
    if (found && hs != (uint16_t)(s + n) && inner && out.s == (uint16_t)(s + n + inner) && (uint16_t)out.pc == back) {
        /* The C goes on from the original's first fetch at the landing
           (return marks are no guide: a level whose code pulls what it
           pushed is marked returned at the pull). */
        (void)nest;
        for (c = j + 2; c < cur_n || fetch_more(c + 1); c++)
            if (cur_ops[c] >> 4 == 1 && (cur_addr[c] & 0xFFFFFF) == out.pc) break;
        cur_i = c < cur_n ? c : cur_n;
        /* An interrupt the original took right at the landing is marked
           before the fetch, ahead of the return marks of the levels it
           closed there: it is taken in its place. */
        if (c < cur_n) {
            uint32_t e = c;
            while (e > j + 2 && cur_ops[e - 1] >> 4 == 0xC) e--;
            if (e >= j + 6 && cur_ops[e - 4] >> 4 == 5) cur_i = e - 4;
        }
        return;
    }
    /* A noreturn unit call whose original code has reset the stack (the
       game restarting): the original keeps the call's record open and runs
       on as the game, and so does the hybrid, as original code, for the
       rest of the run (the frames are still compared). */
    if (ls_unit_noreturn && out.s > ls_exit_s)
        for (;;) core_run_out(&out, 0xFFFF);
    /* Otherwise it left the routine's frame: its own return when the C
       was entered from original code, or through its caller's. */
    leave_frame(&out);
    cpu.a = out.a; cpu.x = out.x; cpu.y = out.y; cpu.d = out.d; cpu.db = out.db; cpu.p = out.p; cpu.e = out.e;
    cpu.s = out.s; cpu.pc = out.pc; cpu.unwound = 1;
    if (found) record_rest();
    else cur_i = cur_n;
    if (hs != (uint16_t)(s + n) && ls_escape_set) longjmp(ls_escape, 1);
}

void jump_routine_pushed(uint32_t addr, const uint8_t *pushed, int n)
{
    if (!n)
        for (int i = 0; i < nroutines; i++)
            if (routines[i].addr == addr) { routines[i].abi(); return; }
    if (!machine_live) {
        if (cur) cur->needs_asm = 1;
        if (c_running) longjmp(abort_c, 1);
        return;
    }
    jump_original(addr, pushed, n, 1);
}

void jump_routine(uint32_t addr)
{
    jump_routine_pushed(addr, NULL, 0);
}

void jump_original_code(uint32_t addr)
{
    if (!machine_live) {
        if (cur) cur->needs_asm = 1;
        if (c_running) longjmp(abort_c, 1);
        return;
    }
    /* A branch back to the start of the routine whose C is running (Hang)
       is no entry the original marks: nothing to look for in its record. */
    jump_original(addr, NULL, 0, addr != ls_entry_pc);
}

void call_routine(uint32_t addr, int long_call)
{
    /* The C of the routines this run swaps in (all of them in a gate run;
       those named by --only otherwise), so a focused run checks one
       routine's C against everything else as original code. */
    for (int i = 0; i < nroutines; i++)
        if (routines[i].addr == addr) { routines[i].abi(); return; }
    if (!machine_live) {
        if (cur) cur->needs_asm = 1;
        if (c_running) longjmp(abort_c, 1);
        return;
    }
    run_original(addr, long_call);
}    /* the original's run ended inside this call */

/* SFXBOX_LS_SKIPS=frame: from that frame, report every work RAM access
   placed past an interrupt, or the entry of a routine with no C, in the
   original's record:
   where the C reads or writes a byte more often, or in another order, than
   the original, that is where the run starts to drift. */
static long skips_from = -1, skips_shown;
static uint32_t recent[256];  /* the C's last accesses: addr | 1 << 24 for a write */
static unsigned recent_n;
static void log_skip(int write, uint32_t addr, uint32_t j)
{
    for (uint32_t q = cur_i; q < j; q++) {
        uint8_t k = cur_ops[q] >> 4;
        if ((k == 5 || (k == 9 && is_catalog_entry(cur_addr[q] & 0xFFFFFF) && !(has_c[cur_addr[q] >> 3] >> (cur_addr[q] & 7) & 1)))
            && skips_shown++ < 200) {
            fprintf(lsrep, "SKIP frame %ld in %s: %s %06X placed at %u from %u, past %s %06X at %u\n", cur_frame,
                    ls_cur_name ? ls_cur_name : "-", write ? "write" : "read", addr, j, cur_i,
                    k == 5 ? "an interrupt at" : "the entry of", cur_addr[q] & 0xFFFFFF, q);
            fprintf(lsrep, "  the C's accesses before it:");
            for (unsigned i = recent_n > 256 ? recent_n - 256 : 0; i < recent_n; i++)
                fprintf(lsrep, " %s%06X", recent[i % 256] >> 24 ? "w" : "r", recent[i % 256] & 0xFFFFFF);
            fprintf(lsrep, "\n");
            return;
        }
    }
}

/* SFXBOX_LS_PLACE=first,last: in those frames, report where each work RAM
   access of the C is placed in the original's record (from the index it
   searched from to the one after its match; equal when it found none). An
   access that jumps far ahead is one the original made elsewhere: the C
   reads or writes that byte more often, or in another order. */
static long place_from = -1, place_to = -1;
static long pair_lo = -1;  /* the last work RAM write placed, and where */
static uint32_t pair_j;

static uint8_t live_access(int write, uint32_t addr, uint8_t value)
{
    uint8_t want = (uint8_t)(write ? 2 : 1);
    /* The original side has given up waiting for the hybrid: the C is in a
       loop that waits for something the record no longer brings (an
       interrupt), and would spin for ever. */
    if (PARENT_GONE()) _exit(6);
    if (LOAD(sh->stuck)) {
        fprintf(lsrep, "frame %ld: the C of %s is still running, %u of %u recorded cycles used: DIVERGENT\n", cur_frame,
                ls_cur_name ? ls_cur_name : "-", cur_i, cur_n);
        fflush(lsrep);
        _exit(6);
    }
    if (skips_from >= 0) recent[recent_n++ % 256] = (addr & 0xFFFFFF) | (uint32_t)write << 24;
    if (!timed(addr)) {
        /* Work RAM (and the Super FX RAM window) is the S-CPU's alone, but an
           interrupt handler or a frame end can see it: the access is placed
           at the original's matching access, so both see the same writes
           done. The search stops at the next hardware access, which the C
           must make first; a RAM access with no counterpart (the original
           kept the value in a register) happens at once. */
        long o = snap_offset(addr);
        /* A 16-bit read-modify-write (INC, ASL, TSB...) writes the high
           byte first, C's 16-bit write the low byte first: the high byte
           written straight after the low one is placed with it when the
           original wrote it just before. Searched forward, it would match a
           later write of that byte and skip everything in between. */
        int placed = 0;
        if (o >= 0 && write && pair_lo >= 0 && o == pair_lo + 1) {
            uint32_t q = pair_j;
            while (q > 0 && (cur_ops[q - 1] >> 4 == 0 || cur_ops[q - 1] >> 4 == 3 || cur_ops[q - 1] >> 4 == 4)) q--;
            placed = q > 0 && cur_ops[q - 1] >> 4 == 2 && snap_offset(cur_addr[q - 1]) == o;
        }
        pair_lo = -1;
        uint32_t from_i = cur_i, j = cur_i;
        if (o >= 0 && !placed)
            for (; j < cur_n || fetch_more(j + 1); j++) {
                uint8_t k = cur_ops[j] >> 4;
                if ((k == 1 || k == 2) && timed(cur_addr[j])) break;
                /* Nor past the original's entry into a routine with no C
                   here: the C has to reach it first, through call_routine,
                   which places the call there. Matched beyond it, an extra
                   or reordered RAM access would leave the call unplaceable.
                   Only catalogued routines count: a computed jump inside a
                   routine (a handler table the C runs inline) is not one. */
                if (k == 9 && is_catalog_entry(cur_addr[j] & 0xFFFFFF) && !(has_c[cur_addr[j] >> 3] >> (cur_addr[j] & 7) & 1)) break;
                if (k == want && snap_offset(cur_addr[j]) == o) {
                    if (skips_from >= 0 && cur_frame >= skips_from) log_skip(write, addr, j);
                    replay_to(j + 1);
                    if (write) { pair_lo = o; pair_j = j; }
                    break;
                }
            }
        if (place_from >= 0 && cur_frame >= place_from && cur_frame <= place_to)
            fprintf(lsrep, "PLACE %ld %s %06X %u -> %u%s\n", cur_frame, write ? "w" : "r", addr, from_i, cur_i, placed ? " pair" : "");
        /* Past the end of a record cut by a load: the rest of it is
           replayed and the access made with the CPU's own timing, so the
           clock reaches the frame's end, where the hybrid makes the same
           load (a C waiting for the vertical blank would otherwise spin
           with the clock standing still). */
        if (cur_cut && o >= 0 && !placed && j >= cur_n) {
            replay_to(cur_n);
            return core_access(write, addr, value);
        }
        if (write) { core_bus_write(addr, value); return value; }
        return core_bus_read(addr);
    }
    pair_lo = -1;
    for (uint32_t j = cur_i; j < cur_n || fetch_more(j + 1); j++)
        if (cur_ops[j] >> 4 == want && same_place(cur_addr[j]) == same_place(addr)) {
            replay_to(j);
            cur_i = j + 1;
            if (place_from >= 0 && cur_frame >= place_from && cur_frame <= place_to) {
                uint32_t c0 = core_clock();
                uint8_t r = core_access(write, addr, value);
                fprintf(lsrep, "HW %ld %s %06X at %u: clock %u -> %u\n", cur_frame, write ? "w" : "r", addr, j, c0, core_clock());
                return r;
            }
            return core_access(write, addr, value);
        }
    /* No matching cycle left in the original's record: the run has already
       diverged. The access is still made as a real, timed CPU access, so the
       clock moves and a polling loop in the C can end. */
    if (past_end) return core_access(write, addr, value);
    if (cur_cut) { replay_to(cur_n); return core_access(write, addr, value); }
    if (timed_unmatched++ < 5)
        fprintf(lsrep, "OFF frame %ld: %s %06X with %u of %u recorded cycles used (in %s)\n", cur_frame, write ? "write" : "read",
                addr, cur_i, cur_n, ls_cur_name ? ls_cur_name : "-");
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
    ls_cur_name = routines[idx].name;
    ls_entry_pc = pc;
    ls_unit_noreturn = ls_noreturn_pc == pc;
    if (ls_unit_noreturn) ls_noreturn_pc = (uint32_t)-1;
    /* The original's next call: wait until it has returned, then take a
       private copy of its cycles (the ring moves on). */
    uint64_t k = LOAD(sh->h_calls);
    WAIT_UNTIL(k < LOAD(sh->o_calls) || LOAD(sh->o_done));
    cur_n = 0; cur_i = 0; pair_lo = -1; cur_open = 0; cur_cut = 0;
    if (k < LOAD(sh->o_calls)) {
        CallRec *rec = &sh->calls[k % CALLRING];
        if (rec->frame != cur_frame) mismatched_calls++;
        cur_rec = rec;
        cur_open = 1;
        fetch_more(1);
        STORE(sh->h_calls, k + 1);
    } else if (LOAD(sh->o_done) && k + 1 == LOAD(sh->o_calls)) {
        /* The script ended on the original side before this call returned
           (a long Super FX run can span the last frames): there is nothing
           left to compare it, or the frames after it, with. */
        past_end = 1;
        gsu_lockstep_past_end();
        /* Its C would run live with no interrupts to come: one that waits
           for the vertical blank would never return. The hybrid ends here,
           at the frame the original's record ends in. */
        hybrid_finish(cur_frame + 1);
    } else mismatched_calls++;
    hk++;
    if (calls_from >= 0 && cur_frame >= calls_from)
        fprintf(lsrep, "HCALL frame %ld call %llu %s S=%04X\n", cur_frame, (unsigned long long)k, routines[idx].name, r.s);
    cpu.a = r.a; cpu.x = r.x; cpu.y = r.y; cpu.s = r.s; cpu.d = r.d; cpu.db = r.db; cpu.p = r.p; cpu.e = r.e; cpu.pc = r.pc;
    cpu.unwound = 0;
    ls_exit_s = exit_s;
    ls_ret = ret;
    machine_live = 1;
    if (setjmp(ls_escape) == 0) {
        ls_escape_set = 1;
        routines[idx].abi();
    }
    ls_escape_set = 0;
    machine_live = 0;
    while (fetch_more(cur_n + 1)) {}
    replay_to(cur_n);
    struct SfxboxRegs out = {cpu.a, cpu.x, cpu.y, cpu.unwound ? cpu.s : exit_s, cpu.d, cpu.db, cpu.p, cpu.e, cpu.unwound ? cpu.pc : ret};
    core_set_regs(&out);
    /* The original's last instruction was the routine's RTS or RTL, not
       the JSR or JSL that entered it: an entry right after the call (a
       routine's inner entry, 7E:E4E4 after JSR ColourBlendBy) must not be
       taken as entered by that JSR. */
    if (!cpu.unwound)
        *core_opcode = (uint8_t)(exit_s - r.s == 3 ? 0x6B : 0x60);
    if (calls_from >= 0 && cur_frame >= calls_from)
        fprintf(lsrep, "HDONE frame %ld call %llu at %06X S=%04X%s\n", cur_frame, (unsigned long long)(LOAD(sh->h_calls) - 1),
                out.pc, out.s, cpu.unwound ? " unwound" : "");
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
    *(void **)&core_run_until = dlsym(core, "sfxbox_core_run_until");
    *(void **)&core_run_out = dlsym(core, "sfxbox_core_run_out");
    *(void **)&core_access = dlsym(core, "sfxbox_core_access");
    *(void **)&core_set_regs = dlsym(core, "sfxbox_core_set_regs");
    *(void **)&core_snapshot = dlsym(core, "sfxbox_core_snapshot");
    core_exec = dlsym(core, "sfxbox_cpu_exec");
    if (!set_cb || !core_clock || !core_replay || !core_interrupt_here || !core_run_until || !core_run_out || !core_access || !core_set_regs || !core_snapshot || !core_exec) return -1;
    sh = mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (sh == MAP_FAILED) return -1;
    if (gsu_lockstep_prepare()) return -1;
    (void)core_bus_peek;
    fflush(NULL);
    parent_pid = getpid();
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
    gsu_lockstep_role(role, lsrep, &sh->stuck);
    if (getenv("SFXBOX_LS_PLACE")) sscanf(getenv("SFXBOX_LS_PLACE"), "%ld,%ld", &place_from, &place_to);
    if (getenv("SFXBOX_LS_SKIPS")) skips_from = atol(getenv("SFXBOX_LS_SKIPS"));
    if (getenv("SFXBOX_LS_CALLS")) calls_from = atol(getenv("SFXBOX_LS_CALLS"));
    if (tr && sscanf(tr, "%ld,%ld", &tr_lo, &tr_hi) == 2) {
        char p[4096]; snprintf(p, sizeof p, "%s.%s", report_path, role == 1 ? "orig" : "hyb");
        trf = fopen(p, "w");
        if (trf) setvbuf(trf, NULL, _IOLBF, 0);
    }
    return role;
}

int lockstep_role(void) { return role; }

/* A state loaded by the script: a call the original has open belongs to
   the machine the load replaced and never returns. Its record ends here,
   marked cut, so the hybrid can finish the frame in the same place and
   make the same load; later calls are recorded as calls of their own. */
void lockstep_state_loaded(void)
{
    if (role == 2) {
        /* The hybrid's C (or original code it ran, an interrupt handler
           taken in a replay, with routine swapping off) was left where the
           frame ended: it never resumes. (sfxbox puts its hook back.) */
        machine_live = 0;
        ls_escape_set = 0;
        cur_open = 0;
        cur_cut = 0;
        return;
    }
    if (role != 1 || !m.active) return;
    if (calls_from >= 0 && cur_frame >= calls_from)
        fprintf(stderr, "OCUT frame %ld call %llu\n", cur_frame, (unsigned long long)m.call);
    STORE(sh->calls[m.call % CALLRING].cut, 1);
    STORE(sh->calls[m.call % CALLRING].done, 1);
    m.active = 0;
    m.in_irq = 0;
    m.irq_resumed = 0;
    m.depth = 0;
}

/* Whether a unit call (sfxbox call) may start at this instruction. The
   hybrid runs a routine with C in one step, so it is never inside one at
   an instruction boundary; the original is, while it records the call. The
   unit call starts on both sides at the same place only outside every such
   call, and not at the entry of one either (the hybrid would start it before
   running the routine, the original after). Nor, on the hybrid, while a C
   routine runs original code (call_routine), which the original runs inside
   its recorded call. */
uint32_t lockstep_clock(void) { return role ? core_clock() : 0; }
void lockstep_debug_state(void)
{
    fprintf(stderr, "  side %d: recording %d (%06X, in irq %d), C running %d, hybrid frame %ld\n", role, m.active, m_pc, m.in_irq, machine_live, cur_frame);
}

/* The frames a unit call takes: the original runs the routine in real
   time, and a frame can end inside it, where the hybrid's C has already
   returned; it then runs one frame more for the call than the hybrid. Both
   sides go on from the same frame, the original's: it says where its call
   ended, and the hybrid runs on to there. */
long lockstep_unit_end(long f)
{
    static uint64_t n;
    if (role == 1) {
        sh->unit_end[n % 256] = f;
        STORE(sh->o_units, ++n);
        return f;
    }
    if (role == 2) {
        WAIT_UNTIL(LOAD(sh->o_units) > n || LOAD(sh->o_done));
        if (LOAD(sh->o_units) <= n) return f;
        return sh->unit_end[n++ % 256];
    }
    return f;
}
int lockstep_call_ok(uint32_t pc)
{
    /* Asked at every instruction while a unit call waits for a place to
       start: a hybrid that never finds one must not outlive its parent. */
    static unsigned asked;
    if (!role) return 1;
    if (!(++asked & 0xFFFF) && PARENT_GONE()) _exit(6);
    return !m.active && !machine_live && routine_at(pc) < 0;
}

void lockstep_before_frame(long f)
{
    cur_frame = f;
    if (role == 1) {
        WAIT_UNTIL(f - LOAD(sh->h_frames) < NSLOT - 1);
        FrameSlot *fs = &sh->fs[f % NSLOT];
        fs->frame = f; fs->ncalls = 0;
    } else if (role == 2) {
        /* Waiting for the original's frame, the record is drained as it
           comes: the original may be waiting for ring space. */
        long spins = 0;
        while (!(LOAD(sh->o_frames) > f || LOAD(sh->o_done))) {
            fetch_avail();
            if (LOAD(sh->stuck) || PARENT_GONE()) _exit(6);
            if (++spins > 600000) { stuck_report("LOAD(sh->o_frames) > f || LOAD(sh->o_done)"); STORE(sh->stuck, 1); _exit(6); }
            usleep(50);
        }
        hk = 0;
    }
    gsu_before_frame(f);
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
    /* SFXBOX_LS_INFLIGHT=frame: from that frame, also list the bytes left
       out above because the unfinished call writes them. */
    if (sl->midcall && getenv("SFXBOX_LS_INFLIGHT") && f >= atol(getenv("SFXBOX_LS_INFLIGHT"))) {
        int n = 0;
        for (uint32_t o = 0; o < 0x30000; o++)
            if (inflight[o] && mysnap[o] != sl->snap[o] && !(o > lo && o <= s)) {
                if (n++ < 24) fprintf(lsrep, "%s %05X=%02X/%02X", n == 1 ? "IN FLIGHT" : "", o, mysnap[o], sl->snap[o]);
            }
        if (n) fprintf(lsrep, " (%d bytes) frame %ld\n", n, f);
    }
    if (sl->midcall) memset(inflight, 0, sizeof inflight);
    if (bad && sl->midcall) strncat(buf, " (frame ended inside a call)", sizeof buf - strlen(buf) - 1);
    if (hk != sl->ncalls) { bad = 1; snprintf(buf + strlen(buf), sizeof buf - strlen(buf), " calls %u/%u", hk, sl->ncalls); }
    if (gsu_after_frame(f, buf, sizeof buf)) bad = 1;
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
        /* Otherwise at the start of the instruction it is in: the bytes an
           unfinished push is still to write hold whatever was there before
           (in the original, often a byte the C of an earlier call never
           pushed), and the push overwrites them before anything reads them. */
        sl->s = m.active ? m.exit_s : r.s > insn_s ? r.s : insn_s;
        sl->midcall = m.active;
        /* SFXBOX_LS_MIDCALL=frame: from that frame, name the call each
           frame ends inside. */
        if (m.active && getenv("SFXBOX_LS_MIDCALL") && f >= atol(getenv("SFXBOX_LS_MIDCALL")))
            fprintf(stderr, "MIDCALL frame %ld ends inside call %llu at %06X\n", f, (unsigned long long)m.call, m_pc);
        sl->clock = core_clock();
        sl->regs = r;
        core_snapshot(sl->snap);
        STORE(sh->o_frames, f + 1);
    } else if (role == 2) {
        if (!past_end && !gsu_lockstep_is_past_end()) compare(f);
        STORE(sh->h_frames, f + 1);
    }
}

void lockstep_end(long frames)
{
    if (role == 1) {
        STORE(sh->o_done, 1);
        gsu_lockstep_done();
        int st = 0;
        waitpid(child, &st, 0);
        if (LOAD(sh->stuck)) fprintf(stderr, "lockstep: a side waited too long; see the report\n");
    } else if (role == 2)
        hybrid_finish(frames);
}
