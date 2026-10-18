/* spccall.c: SPC700 routines in the per-call oracle (A5).

   The library's SPC700 routines (hybrid.py: sfx_spc_routine_table, from C
   tags with an `@spc` APU address) are keyed by the ROM address of their
   first byte; the SPC700 runs them from APU RAM, where the race and battle
   drivers put different code at the same addresses. An instruction about
   to run at a routine's APU address is that routine when the APU RAM holds
   the ROM bytes its C depends on (hybrid.py: the runs from its @spcuses
   ranges, else its first four bytes) and the instruction before it was a
   CALL, PCALL or TCALL that jumped here.

   A call is watched from its entry to the instruction at the return address
   the call pushed, with SP back where it was before the push. The C then
   runs from the registers at the entry against the APU RAM as it was then
   (every byte the original wrote while the call was open has its old value
   kept), its hardware reads ($F0-$FF: the DSP data port, the CPU ports, the
   timers) answered with the values the original read at the same address,
   in order. It matches when A, X, Y, SP and PSW, every APU RAM byte either
   side wrote, and the list of I/O writes (address and value, in order) are
   the same. A call open when a state is loaded is not judged. */
#include <dlfcn.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spccall.h"
#include "spc.h"
#include "check.h"

struct SfxboxSmpCallbacks {
    void (*insn)(uint16_t pc, uint16_t prev_end, uint8_t opcode);
    void (*mem)(int write, uint16_t addr, uint8_t data);
    void (*step)(uint32_t clocks);
};
struct SmpState { uint16_t pc; uint8_t a, x, y, s, p; };

static void (*core_get)(struct SmpState *);
static void (*core_set_cb)(const struct SfxboxSmpCallbacks *, int (*)(uint16_t, uint16_t));

/* SPC700 unit calls (script `spccall`, `spcset`). Every mode's exec hook is
   chained behind unit_exec, which starts a staged call the next time the
   SPC700 reaches the instruction at `at` (on both lockstep sides the same
   point of the driver's loop): it puts CALL target; BRA $ in RAM no driver
   uses ($01F0), runs it with the given registers, and at the BRA puts the
   SPC700 back where it was and the stub's bytes back. The call is an
   ordinary CALL into the routine, so the checks open their watch on it. */
#define SPC_STUB 0x01F0
static int (*next_exec)(uint16_t, uint16_t);
static int unit_exec(uint16_t pc, uint16_t prev_end);
static int hooked;
static void set_cb(const struct SfxboxSmpCallbacks *cb, int (*ex)(uint16_t, uint16_t))
{
    next_exec = ex;
    core_set_cb(cb, unit_exec);
    hooked = 1;
}
static void (*u_put)(const struct SmpState *);
static struct {
    int state;                 /* 1 staged, 2 running */
    uint16_t target, at;
    int has[4]; uint8_t reg[4];   /* a, x, y, p */
    struct SmpState saved;
    uint8_t stub_save[5];
    int nset; uint16_t set_at[256]; uint8_t set_v[256];
} U;

static uint8_t *apu, *rom;
static size_t rom_n;

typedef struct { uint16_t apu, len; uint32_t rom; } SpcUse;
typedef struct { uint32_t addr; uint16_t apu; void (*abi)(void); const char *name; const SpcUse *uses; } SpcRoutine;
static SpcRoutine *rt;
static int nrt;
static uint8_t at_apu[0x10000];   /* 1 where some routine starts */
static uint8_t jump_at[0x10000];  /* 1 where the C jumps into original code (spc_jump_original) */

static uint32_t rom_off(uint32_t a) { return ((a >> 16 & 0x7F) * 0x8000 + (a & 0x7FFF)) % (uint32_t)rom_n; }

static int routine_here(uint16_t pc)
{
    if (!at_apu[pc]) return -1;
    for (int i = 0; i < nrt; i++) {
        if (rt[i].apu != pc) continue;
        /* Every APU byte its C depends on (hybrid.py: @spcuses, else the
           entry's first four) holds the ROM byte it stands for. */
        int ok = 1;
        for (const SpcUse *u = rt[i].uses; ok && u && u->len; u++)
            ok = !memcmp(apu + u->apu, rom + u->rom % rom_n, u->len);
        if (ok) return i;
    }
    return -1;
}

int spc_load(void *core, void *lib, const char *only)
{
    const SpcRoutine *table = dlsym(lib, "sfx_spc_routine_table");
    const int *count = dlsym(lib, "sfx_spc_routine_count");
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
    *(void **)&core_get = dlsym(core, "sfxbox_smp_get");
    *(void **)&core_set_cb = dlsym(core, "sfxbox_smp_set_callbacks");
    uint8_t *(*mem)(const char *, size_t *) = dlsym(core, "sfxbox_memory");
    size_t n;
    apu = mem ? mem("apuram", &n) : NULL;
    rom = mem ? mem("rom", &rom_n) : NULL;
    if (!core_get || !core_set_cb || !apu || !rom) {
        fprintf(stderr, "the library has SPC700 routines and the core has no SPC700 hooks "
                "(tools/toolchains/patches/bsnes/0006-sfxbox-spc700-hooks.patch): rebuild it with ./setup.sh bsnes\n");
        nrt = 0;
        return -1;
    }
    for (int i = 0; i < nrt; i++) at_apu[rt[i].apu] = 1;
    const uint16_t *jt = dlsym(lib, "sfx_spc_jump_entries");
    const int *jn = dlsym(lib, "sfx_spc_jump_entry_count");
    if (jt && jn)
        for (int i = 0; i < *jn; i++) jump_at[jt[i]] = 1;
    return nrt;
}

int spc_count(void) { return nrt; }

/* ---- the C's machine ---- */

SpcRegs spc;

typedef struct { uint16_t addr; uint8_t v; } Io;
typedef struct {
    int idx;
    uint16_t ret;
    uint8_t sp_after;          /* SP once the return address is pulled */
    struct SmpState entry;
    uint32_t gen;
    Io *ior, *iow; uint32_t nior, niow, capr, capw;   /* the original's I/O reads, writes */
} Watch;

#define DEPTH 8
static Watch ws[DEPTH];
static int depth;
static uint8_t undo_v[DEPTH][0x10000]; static uint32_t undo_g[DEPTH][0x10000];
static uint8_t orig_v[DEPTH][0x10000]; static uint32_t orig_g[DEPTH][0x10000];
static uint8_t cw_v[0x10000]; static uint32_t cw_g[0x10000], cw_gen;
static uint16_t *touched; static uint32_t ntouched, tcap;
static Io *ciow; static uint32_t nciow, ciowcap;
static Watch *cur; static int curdepth;
static uint32_t ior_at;
static int c_running, c_bad;
static jmp_buf c_abort;
static long c_jumped;   /* where the C jumped into original code, or -1 */
static long c_accesses;
static int prev_call;           /* the last instruction was a CALL, PCALL or TCALL */

typedef struct { long calls, match, diff, inconclusive, skipped; int shown; } Stat;
static Stat *stats;
static FILE *report;

static void push_io(Io **v, uint32_t *n, uint32_t *cap, uint16_t addr, uint8_t x)
{
    if (*n == *cap) { *cap = *cap ? *cap * 2 : 256; *v = realloc(*v, *cap * sizeof **v); if (!*v) abort(); }
    (*v)[(*n)++] = (Io){addr, x};
}

static int is_io(uint16_t a) { return (a & 0xFFF0) == 0x00F0; }

/* The byte at a as it was at the entry of the watch at depth d. */
static uint8_t entry_byte(int d, uint16_t a)
{
    return undo_g[d][a] == ws[d].gen ? undo_v[d][a] : apu[a];
}

static int live;   /* the hybrid's C runs on the live machine (lockstep) */
static uint8_t live_access(int write, uint16_t a, uint8_t v);

uint8_t spc_rd(uint16_t a)
{
    if (live) return live_access(0, a, 0);
    if (!c_running) return apu[a];
    if (++c_accesses > (1L << 22)) { c_bad = 2; longjmp(c_abort, 1); }
    if (is_io(a) && a != 0xF2 && a != 0xF8 && a != 0xF9) {
        /* A hardware register: the original's next read of it. */
        for (uint32_t k = ior_at; k < cur->nior; k++)
            if (cur->ior[k].addr == a) { ior_at = k + 1; return cur->ior[k].v; }
        c_bad = 3;
        return 0;
    }
    if (cw_g[a] == cw_gen) return cw_v[a];
    return entry_byte(curdepth, a);
}

static jmp_buf live_abort;
static void live_jump(uint16_t addr);

void spc_jump_original(uint16_t addr)
{
    if (live) live_jump(addr);
    if (c_running) { c_jumped = addr; longjmp(c_abort, 1); }
}

void spc_wr(uint16_t a, uint8_t v)
{
    if (live) { live_access(1, a, v); return; }
    if (!c_running) return;
    if (++c_accesses > (1L << 22)) { c_bad = 2; longjmp(c_abort, 1); }
    if (is_io(a)) push_io(&ciow, &nciow, &ciowcap, a, v);
    if (cw_g[a] != cw_gen) {
        if (ntouched == tcap) { tcap = tcap ? tcap * 2 : 1024; touched = realloc(touched, tcap * sizeof *touched); if (!touched) abort(); }
        touched[ntouched++] = a;
    }
    cw_g[a] = cw_gen; cw_v[a] = v;
}

static void finish(int d, long at_jump)
{
    Watch *w = &ws[d];
    Stat *st = &stats[w->idx];
    struct SmpState fin;
    core_get(&fin);
    st->calls++;

    cur = w; curdepth = d; cw_gen++; ntouched = 0; nciow = 0; ior_at = 0; c_bad = 0; c_accesses = 0; c_jumped = -1;
    spc.a = w->entry.a; spc.x = w->entry.x; spc.y = w->entry.y; spc.sp = w->entry.s;
    spc_set_psw(w->entry.p);
    c_running = 1;
    if (setjmp(c_abort) == 0) rt[w->idx].abi();
    c_running = 0;

    char why[512] = "";
    int bad = 0;
#define WHY(...) do { bad = 1; if (strlen(why) < 400) snprintf(why + strlen(why), sizeof why - strlen(why), __VA_ARGS__); } while (0)
    if (spc.a != fin.a) WHY(" A=%02X/%02X", spc.a, fin.a);
    if (spc.x != fin.x) WHY(" X=%02X/%02X", spc.x, fin.x);
    if (spc.y != fin.y) WHY(" Y=%02X/%02X", spc.y, fin.y);
    if (spc.sp != fin.s) WHY(" SP=%02X/%02X", spc.sp, fin.s);
    if (spc_psw() != fin.p) WHY(" PSW=%02X/%02X", spc_psw(), fin.p);
    /* Every byte either side wrote: the original's from its writes (orig),
       the C's from its own. */
    for (uint32_t i = 0; i < ntouched; i++) {
        uint16_t a = touched[i];
        uint8_t ov = orig_g[d][a] == w->gen ? orig_v[d][a] : entry_byte(d, a);
        if (cw_v[a] != ov) WHY(" [%04X]=%02X/%02X", a, cw_v[a], ov);
    }
    for (uint32_t a = 0; a < 0x10000; a++)
        if (orig_g[d][a] == w->gen && cw_g[a] != cw_gen && orig_v[d][a] != entry_byte(d, (uint16_t)a))
            WHY(" [%04X]=%02X/%02X", a, entry_byte(d, (uint16_t)a), orig_v[d][a]);
    uint32_t n = nciow < w->niow ? nciow : w->niow;
    for (uint32_t k = 0; k < n; k++)
        if (ciow[k].addr != w->iow[k].addr || ciow[k].v != w->iow[k].v) {
            WHY(" io#%u %02X=%02X/%02X=%02X", k, ciow[k].addr, ciow[k].v, w->iow[k].addr, w->iow[k].v);
            break;
        }
    if (nciow != w->niow) WHY(" io writes %u/%u", nciow, w->niow);
    if (c_jumped != at_jump)
        WHY(" ends %s/%s", c_jumped < 0 ? "returning" : "jumping", at_jump < 0 ? "returning" : "jumping");
    if (c_bad == 2) WHY(" runaway");
    if (c_bad == 3) WHY(" hardware read the original did not make");
    if (!bad) { st->match++; return; }
    st->diff++;
    if (st->shown < 8) {
        st->shown++;
        fprintf(report, "DIFF %06X %s call %ld: entry A=%02X X=%02X Y=%02X SP=%02X PSW=%02X; C/orig:%s\n",
                rt[w->idx].addr, rt[w->idx].name, st->calls, w->entry.a, w->entry.x, w->entry.y, w->entry.s,
                w->entry.p, why);
    }
}

static void on_insn(uint16_t pc, uint16_t prev_end, uint8_t opcode)
{
    if (depth) {
        struct SmpState now;
        core_get(&now);
        while (depth && pc == ws[depth - 1].ret && now.s == ws[depth - 1].sp_after) { finish(depth - 1, -1); depth--; }
        /* Original code the C jumps into: every open call ends here. */
        if (jump_at[pc] && pc != prev_end)
            while (depth) { finish(depth - 1, pc); depth--; }
    }
    int called = prev_call && pc != prev_end;
    prev_call = opcode == 0x3F || opcode == 0x4F || (opcode & 0x0F) == 0x01;
    if (!called) return;
    int idx = routine_here(pc);
    if (idx < 0) return;
    if (depth == DEPTH) { stats[idx].skipped++; return; }
    Watch *w = &ws[depth];
    core_get(&w->entry);
    /* The call pushed the return address: high byte at SP+2, low at SP+1. */
    uint8_t s = w->entry.s;
    w->ret = (uint16_t)(apu[0x100 | (uint8_t)(s + 1)] | apu[0x100 | (uint8_t)(s + 2)] << 8);
    w->sp_after = (uint8_t)(s + 2);
    w->idx = idx;
    w->gen++;
    w->nior = w->niow = 0;
    depth++;
}

static void on_mem(int write, uint16_t a, uint8_t v)
{
    if (!depth) return;
    for (int d = 0; d < depth; d++) {
        Watch *w = &ws[d];
        if (write) {
            if (undo_g[d][a] != w->gen) { undo_g[d][a] = w->gen; undo_v[d][a] = apu[a]; }
            orig_g[d][a] = w->gen; orig_v[d][a] = v;
            if (is_io(a)) push_io(&w->iow, &w->niow, &w->capw, a, v);
        } else if (is_io(a)) {
            push_io(&w->ior, &w->nior, &w->capr, a, v);
        }
    }
}

void spc_state_loaded(void)
{
    if (stats) while (depth) stats[ws[--depth].idx].skipped++;
    prev_call = 0;
}

void spc_check_begin(FILE *rep)
{
    if (!nrt) return;
    report = rep;
    stats = calloc((size_t)nrt, sizeof *stats);
    static struct SfxboxSmpCallbacks cb = {on_insn, on_mem, NULL};
    set_cb(&cb, NULL);
}

void spc_check_rows(FILE *rep)
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
 *
 * The original side records each call into a tagged SPC700 routine (its
 * frame, every clock step and every data access by address, from its entry
 * to its return) in rings shared with the hybrid. The hybrid, reaching the
 * same entry, runs the C in place on the live machine: before each access
 * the C makes, the original's steps up to its matching access (same kind
 * and address, searched forward) are replayed, so the DSP, the timers and
 * the S-CPU see the APU RAM and the ports change at the original's clocks;
 * then the rest of the call's steps, the C's registers, and the SPC700 is
 * put at the return address. Calls inside a recorded call are the C's. The
 * frames are compared as the S-CPU's (the snapshot holds APU RAM).
 * ====================================================================== */
#include <sys/mman.h>
#include <unistd.h>

#define SNSLOT 128
#define SCALLRING (1 << 18)
#define SOPRING (1u << 22)
enum { SOP_STEP = 1, SOP_READ = 2, SOP_WRITE = 3 };

/* cut: the call was open when the script loaded a state; its record ends
   there, and it never returns. */
typedef struct { long frame; uint64_t opoff; uint32_t opn; int done, cut; uint16_t ret; uint8_t sp_after; } SCall;
typedef struct {
    uint64_t o_calls, h_calls, o_ops, h_ops;
    int o_done;
    uint32_t fcalls[SNSLOT];
    SCall calls[SCALLRING];
    uint8_t ops[SOPRING];
    uint32_t arg[SOPRING];
} SShared;

static SShared *ssh;
static int srole;
static volatile int *sstuck;
static FILE *slsrep;
static long sframe;
static void (*core_put)(const struct SmpState *);
static void (*core_time)(uint32_t);
static uint8_t (*core_bus)(int, uint16_t, uint8_t);

#define SLOAD(x) __atomic_load_n(&(x), __ATOMIC_SEQ_CST)
#define SSTORE(x, v) __atomic_store_n(&(x), (v), __ATOMIC_SEQ_CST)
#define SWAIT(cond) do { long spins_ = 0; while (!(cond)) { \
        if (*sstuck || (srole == 2 && getppid() == 1)) _exit(6); \
        if (++spins_ > 600000) { \
            fprintf(stderr, "lockstep: %s side stuck in frame %ld waiting for SPC700 %s; calls o/h %llu/%llu, ops o/h %llu/%llu%s\n", \
                    srole == 1 ? "original" : "hybrid", sframe, #cond, (unsigned long long)SLOAD(ssh->o_calls), \
                    (unsigned long long)SLOAD(ssh->h_calls), (unsigned long long)SLOAD(ssh->o_ops), \
                    (unsigned long long)SLOAD(ssh->h_ops), swait_more()); \
            *sstuck = 1; _exit(6); } usleep(50); } } while (0)

static const char *swait_more(void);

int spc_lockstep_prepare(void *core)
{
    if (!nrt) return 0;
    *(void **)&core_put = dlsym(core, "sfxbox_smp_put");
    *(void **)&core_time = dlsym(core, "sfxbox_smp_time");
    *(void **)&core_bus = dlsym(core, "sfxbox_smp_bus");
    if (!core_put || !core_time || !core_bus) return -1;
    ssh = mmap(NULL, sizeof(SShared), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    return ssh == MAP_FAILED ? -1 : 0;
}

/* ---- original side ---- */

static struct { int active; uint16_t ret; uint8_t sp_after; uint64_t call; } sm;
static int sprev_call;

static void sop(uint8_t kind, uint32_t arg)
{
    uint64_t n = SLOAD(ssh->o_ops);
    SWAIT(n - SLOAD(ssh->h_ops) < SOPRING);
    ssh->ops[n % SOPRING] = kind;
    ssh->arg[n % SOPRING] = arg;
    __atomic_add_fetch(&ssh->calls[sm.call % SCALLRING].opn, 1, __ATOMIC_RELAXED);   /* read while it grows */
    SSTORE(ssh->o_ops, n + 1);
}

static void orig_insn(uint16_t pc, uint16_t prev_end, uint8_t opcode)
{
    int called = sprev_call && pc != prev_end;
    sprev_call = opcode == 0x3F || opcode == 0x4F || (opcode & 0x0F) == 0x01;
    if (sm.active) {
        struct SmpState now;
        core_get(&now);
        if ((pc == sm.ret && now.s == sm.sp_after) || (jump_at[pc] && pc != prev_end)) {
            SSTORE(ssh->calls[sm.call % SCALLRING].done, 1);
            sm.active = 0;
        }
        return;
    }
    if (!called) return;
    if (routine_here(pc) < 0) return;
    struct SmpState st;
    core_get(&st);
    uint64_t k = SLOAD(ssh->o_calls);
    SWAIT(k - SLOAD(ssh->h_calls) < SCALLRING);
    SCall *c = &ssh->calls[k % SCALLRING];
    c->frame = sframe; c->opoff = SLOAD(ssh->o_ops); c->opn = 0;
    c->ret = (uint16_t)(apu[0x100 | (uint8_t)(st.s + 1)] | apu[0x100 | (uint8_t)(st.s + 2)] << 8);
    c->sp_after = (uint8_t)(st.s + 2);
    SSTORE(c->cut, 0);
    SSTORE(c->done, 0);
    SSTORE(ssh->o_calls, k + 1);
    ssh->fcalls[sframe % SNSLOT]++;
    sm.active = 1; sm.ret = c->ret; sm.sp_after = c->sp_after; sm.call = k;
}

static void orig_mem(int write, uint16_t a, uint8_t v) { (void)v; if (sm.active) sop(write ? SOP_WRITE : SOP_READ, a); }
static void orig_step(uint32_t clocks) { if (sm.active) sop(SOP_STEP, clocks); }

/* ---- hybrid side ---- */

static uint8_t *scops; static uint32_t *scarg; static uint32_t scn, sci, sccap;
static long hs_calls, hs_unmatched, hs_fcalls;
static int spast_end, hprev_call, called_prev;

static uint32_t slast_step = 1;   /* the last step replayed, in clocks */
static void sreplay_to(uint32_t to)
{
    for (uint32_t i = sci; i < to; i++)
        if (scops[i] == SOP_STEP) { core_time(scarg[i]); slast_step = scarg[i]; }
    if (to > sci) sci = to;
}

/* The call's record is taken as it comes (a routine that never returns,
   such as the driver's start, never completes one): srec while more of it
   may come. sfetch copies what has come; with `wait`, it first waits for
   more, or for the record's end. 0 when no more will come. */
static SCall *srec;
static int sopen, scut;
static int sfetch(int wait)
{
    if (!sopen) return 0;
    uint32_t have = scn;
    for (;;) {
        int odone = SLOAD(ssh->o_done);   /* before the ops: none come after it */
        int done = SLOAD(srec->done);
        /* The ops published so far (o_ops), but only this call's (opn,
           read after: the original may have ended the call and gone on to
           the next one's since `done` was read). */
        uint64_t pub = SLOAD(ssh->o_ops);
        uint32_t avail = (uint32_t)(pub - srec->opoff), own = SLOAD(srec->opn);
        if (avail > own) avail = own;
        if (avail > scn) {
            if (avail > sccap) {
                sccap = avail + (avail >> 1) + 1024;
                scops = realloc(scops, sccap);
                scarg = realloc(scarg, (size_t)sccap * sizeof *scarg);
                if (!scops || !scarg) _exit(7);
            }
            for (uint32_t i = scn; i < avail; i++) {
                uint64_t o = (srec->opoff + i) % SOPRING;
                scops[i] = ssh->ops[o]; scarg[i] = ssh->arg[o];
            }
            scn = avail;
            SSTORE(ssh->h_ops, srec->opoff + scn);
        }
        if (done) { sopen = 0; scut = SLOAD(srec->cut); return scn > have; }
        if (!wait || scn > have) return 1;
        if (odone) {
            /* The script ended on the original side inside this call: the
               C has nothing more to follow, and nothing after can be
               compared. */
            spast_end = 1;
            lockstep_finish_now();
        }
        SWAIT(SLOAD(ssh->o_ops) > srec->opoff + scn || SLOAD(srec->done) || SLOAD(ssh->o_done));
    }
}

/* For a stuck report: the call the hybrid is following. */
static const char *swait_more(void)
{
    static char b[160];
    b[0] = 0;
    if (srole == 2 && srec)
        snprintf(b, sizeof b, "; record off %llu len %u done %d, taken %u, at %u, open %d",
                 (unsigned long long)srec->opoff, srec->opn, SLOAD(srec->done), scn, sci, sopen);
    return b;
}

/* The whole record, before the original's steps after the C's last access
   are replayed (at its return or its jump). */
static void sfetch_all(void)
{
    while (sopen) sfetch(1);
}

static uint8_t live_access(int write, uint16_t a, uint8_t v)
{
    uint8_t want = write ? SOP_WRITE : SOP_READ;
    for (uint32_t j = sci;; j++) {
        if (j == scn && !sfetch(1)) break;
        if (scops[j] == want && scarg[j] == a) { sreplay_to(j); sci = j + 1; return core_bus(write, a, v); }
    }
    /* The original's record was cut by a state load: the C runs on to the
       frame's end (the load replaces the machine), each access timed like
       the record's last step, so a loop polling a timer can end. */
    if (scut) { sreplay_to(scn); core_time(slast_step); }
    return core_bus(write, a, v);
}

static void hybrid_insn(uint16_t pc, uint16_t prev_end, uint8_t opcode)
{
    (void)pc; (void)prev_end;
    hprev_call = opcode == 0x3F || opcode == 0x4F || (opcode & 0x0F) == 0x01;
}

static int hybrid_exec(uint16_t pc, uint16_t prev_end)
{
    /* hybrid_insn has already seen this instruction: the flag it left is
       this one's; the call into it was the one before. */
    int called = called_prev && pc != prev_end;
    called_prev = hprev_call;
    if (!called) return 0;
    int idx = routine_here(pc);
    if (idx < 0) return 0;
    uint64_t k = SLOAD(ssh->h_calls);
    SWAIT(k < SLOAD(ssh->o_calls) || SLOAD(ssh->o_done));
    SCall rec = {0};
    scn = sci = 0;
    sopen = 0; scut = 0;
    struct SmpState st;
    core_get(&st);
    if (k < SLOAD(ssh->o_calls)) {
        srec = &ssh->calls[k % SCALLRING];
        rec = *srec;
        if (rec.frame != sframe && !spast_end) {
            hs_unmatched++;
            fprintf(slsrep, "SPC UNMATCHED call %llu %s %04X: original frame %ld, here %ld\n",
                    (unsigned long long)k, rt[idx].name, pc, rec.frame, sframe);
        }
        sopen = 1;
        sfetch(0);
        SSTORE(ssh->h_calls, k + 1);
    } else {
        spast_end = 1;
        rec.ret = (uint16_t)(apu[0x100 | (uint8_t)(st.s + 1)] | apu[0x100 | (uint8_t)(st.s + 2)] << 8);
    }
    hs_calls++; hs_fcalls++;
    spc.a = st.a; spc.x = st.x; spc.y = st.y; spc.sp = st.s; spc_set_psw(st.p);
    live = 1;
    if (setjmp(live_abort) == 0) {
        rt[idx].abi();
        live = 0;
        called_prev = 0;   /* the next instruction follows the C's RET */
        sfetch_all();
        sreplay_to(scn);
        struct SmpState out = {rec.ret, spc.a, spc.x, spc.y, spc.sp, spc_psw()};
        core_put(&out);
    }
    return 1;
}

/* The C jumps into original code: the call's record ends where the
   original got there; the core goes on from there. */
static void live_jump(uint16_t addr)
{
    live = 0;
    called_prev = 0;
    sfetch_all();
    sreplay_to(scn);
    struct SmpState out = {addr, spc.a, spc.x, spc.y, spc.sp, spc_psw()};
    core_put(&out);
    longjmp(live_abort, 1);
}

void spc_lockstep_role(int r, FILE *rep, volatile int *stk)
{
    if (!nrt || !ssh) return;
    srole = r; slsrep = rep; sstuck = stk;
    if (r == 1) {
        static struct SfxboxSmpCallbacks cb = {orig_insn, orig_mem, orig_step};
        set_cb(&cb, NULL);
    } else if (r == 2) {
        static struct SfxboxSmpCallbacks cb = {hybrid_insn, NULL, NULL};
        set_cb(&cb, hybrid_exec);
    }
}

void spc_before_frame(long f)
{
    sframe = f;
    if (srole == 1 && ssh) ssh->fcalls[f % SNSLOT] = 0;
    hs_fcalls = 0;
}

int spc_after_frame(long f, char *buf, size_t n)
{
    if (srole != 2 || !ssh || spast_end) return 0;
    uint32_t o = ssh->fcalls[f % SNSLOT];
    if ((uint32_t)hs_fcalls != o) {
        snprintf(buf + strlen(buf), n - strlen(buf), " spc700 calls %ld/%u", hs_fcalls, o);
        return 1;
    }
    return 0;
}

/* A state loaded by the script. Original side: a call open now belongs to
   the machine the load replaced and never returns; its record ends here,
   marked cut. Hybrid side: C left inside a call never resumes. */
void spc_lockstep_state_loaded(void)
{
    if (!ssh) return;
    if (srole == 1) {
        if (sm.active) {
            SSTORE(ssh->calls[sm.call % SCALLRING].cut, 1);
            SSTORE(ssh->calls[sm.call % SCALLRING].done, 1);
            sm.active = 0;
        }
        sprev_call = 0;
    } else if (srole == 2) {
        live = 0;
        sopen = 0; scut = 0;
        hprev_call = called_prev = 0;
    }
}

void spc_lockstep_done(void) { if (ssh) SSTORE(ssh->o_done, 1); }
void spc_lockstep_past_end(void) { spast_end = 1; }

int spc_lockstep_summary(char *buf, size_t n)
{
    if (!nrt || !ssh) { buf[0] = 0; return 0; }
    snprintf(buf, n, "; spc700 calls swapped %ld, unmatched %ld", hs_calls, hs_unmatched);
    return hs_unmatched != 0;
}

static int unit_exec(uint16_t pc, uint16_t prev_end)
{
    if (U.state == 1 && pc == U.at) {
        core_get(&U.saved);
        memcpy(U.stub_save, apu + SPC_STUB, 5);
        apu[SPC_STUB] = 0x3F;   /* CALL target */
        apu[SPC_STUB + 1] = (uint8_t)U.target;
        apu[SPC_STUB + 2] = (uint8_t)(U.target >> 8);
        apu[SPC_STUB + 3] = 0x2F;   /* BRA $ */
        apu[SPC_STUB + 4] = 0xFE;
        for (int i = 0; i < U.nset; i++) apu[U.set_at[i]] = U.set_v[i];
        U.nset = 0;
        struct SmpState r = U.saved;
        r.pc = SPC_STUB;
        if (U.has[0]) r.a = U.reg[0];
        if (U.has[1]) r.x = U.reg[1];
        if (U.has[2]) r.y = U.reg[2];
        if (U.has[3]) r.p = U.reg[3];
        u_put(&r);
        U.state = 2;
        return 1;
    }
    if (U.state == 2 && pc == SPC_STUB + 3) {
        memcpy(apu + SPC_STUB, U.stub_save, 5);
        u_put(&U.saved);
        U.state = 0;
        return 1;
    }
    return next_exec ? next_exec(pc, prev_end) : 0;
}

/* The core's SPC700 hooks for unit calls, whether or not the library has
   SPC700 routines; 0 when the core has none. */
int spc_unit_init(void *core)
{
    if (!core_get) *(void **)&core_get = dlsym(core, "sfxbox_smp_get");
    if (!core_set_cb) *(void **)&core_set_cb = dlsym(core, "sfxbox_smp_set_callbacks");
    *(void **)&u_put = dlsym(core, "sfxbox_smp_put");
    if (!apu) {
        uint8_t *(*mem)(const char *, size_t *) = dlsym(core, "sfxbox_memory");
        size_t n;
        apu = mem ? mem("apuram", &n) : NULL;
    }
    return core_get && core_set_cb && u_put && apu;
}

void spc_unit_set(uint16_t at, uint8_t v)
{
    if (U.nset < 256) { U.set_at[U.nset] = at; U.set_v[U.nset++] = v; }
}

void spc_unit_call(uint16_t target, uint16_t at, const int *has, const uint8_t *reg)
{
    U.target = target; U.at = at;
    for (int i = 0; i < 4; i++) { U.has[i] = has[i]; U.reg[i] = reg[i]; }
    U.state = 1;
    if (!hooked) {
        static struct SfxboxSmpCallbacks none = {NULL, NULL, NULL};
        set_cb(&none, NULL);
    }
}

int spc_unit_busy(void) { return U.state != 0; }
