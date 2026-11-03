/*
 * la_win.c: the look-ahead's copy of the running game on Windows, where
 * there is no fork() (interp.c's look-ahead forks a copy at each of the
 * game's draws: it plays the next race frames with the controls held now,
 * and hands its draws over through shared memory).
 *
 * The program is built so that its running game is two blocks of memory
 * at fixed addresses (link.sh): its static data (between srfx_data_begin
 * and srfx_data_end, srfx_bss_begin and srfx_bss_end: the game's,
 * machine's and runtime's objects, not the hosts' nor this file's) and
 * its own memory range (port_mem.c, the coroutines' stacks in it; the game
 * itself runs on one, main.c). The executable is loaded where it was
 * linked, so the addresses are the same in every process of it.
 *
 * A second process of the same program (the worker: --la-worker) waits.
 * la_win_fork(), on the game's stack where interp.c would fork, copies both
 * blocks into a section shared with it (the stacks only from their
 * pointers up) and signals it; the worker puts them in place in itself and
 * jumps into the game's stack where la_win_fork's caller set its jump
 * buffer: from there it is the copy, as a forked child is, until it ends
 * (la_win_exit, where interp.c's child calls _exit) and waits again. The
 * draws' shared slots are a section mapped at one fixed address in both.
 * waitpid and kill: whether the copy has ended (its code), and a request
 * to end it (the copy looks at each of its frames).
 */
#ifdef _WIN32
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define PORT_MEM_IMPL                   /* (this file's own memory is the system's) */
#include "port_mem.h"
#include "coro.h"

extern char srfx_data_begin[], srfx_data_end[], srfx_bss_begin[], srfx_bss_end[];
size_t pm_committed(void);

#define LA_SHARED_BASE ((uintptr_t)0x300000000ull)     /* the draws' slots, in both processes */
#define SNAP_SIZE ((size_t)768 << 20)                  /* the largest snapshot (reserved, used as it goes) */
#define MAX_CHUNKS 16

typedef struct {
    volatile LONG ready;            /* the worker waits for snapshots */
    volatile LONG abort;            /* the copy is to end */
    volatile LONG status;           /* the copy's ending code */
    void **ctx;                     /* where it goes on: the jump buffer (in the copied static data) */
    uint64_t stack_base, stack_limit;   /* the game's stack (the TEB's bounds for it) */
    uint64_t data_len, bss_len, committed;
    int nchunk;
    uint64_t chunk_at[MAX_CHUNKS], chunk_len[MAX_CHUNKS];   /* the memory range's parts copied */
} Ctl;

static Ctl *ctl;
static uint8_t *snap;
static size_t snap_committed;
static HANDLE h_ctl, h_snap, h_slot, h_go, h_done, h_job, h_worker;
static size_t slot_size;
static int started, failed, reaped = 1;
static long snaps;
static double snap_ms, snap_ms_max, snap_mb;

static double now_ms(void)
{
    LARGE_INTEGER c, f;
    QueryPerformanceCounter(&c);
    QueryPerformanceFrequency(&f);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
}

static void report(void)
{
    if (snaps)
        fprintf(stderr, "look-ahead copies (Windows): %ld snapshots, %.1f MB each on average, %.2f ms each, %.2f at most\n", snaps,
                snap_mb / snaps, snap_ms / snaps, snap_ms_max);
}

static uint64_t teb_get(uint64_t off)
{
    uint64_t v;
    __asm__ volatile("movq %%gs:(%1), %0" : "=r"(v) : "r"(off));
    return v;
}

static void teb_set(uint64_t off, uint64_t v)
{
    __asm__ volatile("movq %0, %%gs:(%1)" : : "r"(v), "r"(off) : "memory");
}

static HANDLE shared(size_t n, int reserve)
{
    SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };     /* (inherited by the worker) */
    return CreateFileMappingW(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE | (reserve ? SEC_RESERVE : SEC_COMMIT),
                              (DWORD)((uint64_t)n >> 32), (DWORD)n, NULL);
}

/* the draws' shared slots (interp.c's mmap on Windows): mapped at
   LA_SHARED_BASE, the worker maps them there too */
void *la_win_shared(size_t n)
{
    void *m;
    if (h_slot)
        return NULL;
    h_slot = shared(n, 0);
    if (!h_slot)
        return NULL;
    m = MapViewOfFileEx(h_slot, FILE_MAP_ALL_ACCESS, 0, 0, n, (void *)LA_SHARED_BASE);
    if (!m) {
        fprintf(stderr, "look-ahead: the shared slots could not be mapped at %p\n", (void *)LA_SHARED_BASE);
        return NULL;
    }
    slot_size = n;
    return m;
}

static int start_worker(void)
{
    wchar_t exe[MAX_PATH], cmd[2 * MAX_PATH + 256];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    HANDLE me;
    SECURITY_ATTRIBUTES sa = { sizeof sa, NULL, TRUE };
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jl;
    DWORD t0;
    if (started || failed)
        return started;
    failed = 1;
    if (!h_slot)
        return 0;
    h_ctl = shared(sizeof(Ctl), 0);
    h_snap = shared(SNAP_SIZE, 1);
    h_go = CreateEventW(&sa, FALSE, FALSE, NULL);
    h_done = CreateEventW(&sa, TRUE, TRUE, NULL);
    if (!h_ctl || !h_snap || !h_go || !h_done)
        return 0;
    ctl = (Ctl *)MapViewOfFile(h_ctl, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Ctl));
    snap = (uint8_t *)MapViewOfFile(h_snap, FILE_MAP_ALL_ACCESS, 0, 0, SNAP_SIZE);
    if (!ctl || !snap)
        return 0;
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &me, SYNCHRONIZE, TRUE, 0))
        return 0;
    GetModuleFileNameW(NULL, exe, MAX_PATH);
    _snwprintf(cmd, sizeof cmd / sizeof cmd[0], L"\"%ls\" --la-worker %llx %llx %llx %llx %llx %llx %llx", exe,
               (unsigned long long)(uintptr_t)h_ctl, (unsigned long long)(uintptr_t)h_snap, (unsigned long long)(uintptr_t)h_slot,
               (unsigned long long)slot_size, (unsigned long long)(uintptr_t)h_go, (unsigned long long)(uintptr_t)h_done,
               (unsigned long long)(uintptr_t)me);
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    if (!CreateProcessW(exe, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        fprintf(stderr, "look-ahead: the worker could not start (%lu)\n", (unsigned long)GetLastError());
        return 0;
    }
    /* the worker ends with this program */
    h_job = CreateJobObjectW(NULL, NULL);
    if (h_job) {
        memset(&jl, 0, sizeof jl);
        jl.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(h_job, JobObjectExtendedLimitInformation, &jl, sizeof jl);
        AssignProcessToJobObject(h_job, pi.hProcess);
    }
    h_worker = pi.hProcess;
    CloseHandle(pi.hThread);
    t0 = GetTickCount();
    while (!ctl->ready) {
        if (WaitForSingleObject(h_worker, 1) == WAIT_OBJECT_0 || GetTickCount() - t0 > 10000) {
            fprintf(stderr, "look-ahead: the worker did not come up\n");
            return 0;
        }
    }
    failed = 0;
    started = 1;
    atexit(report);
    return 1;
}

static int put(size_t at, const void *p, size_t n)
{
    if (at + n > SNAP_SIZE)
        return 0;
    if (at + n > snap_committed) {
        size_t want = (at + n + 0xFFFFF) & ~(size_t)0xFFFFF;
        if (!VirtualAlloc(snap + snap_committed, want - snap_committed, MEM_COMMIT, PAGE_READWRITE))
            return 0;
        snap_committed = want;
    }
    memcpy(snap + at, p, n);
    return 1;
}

/* the copy made from here: ctx is the jump buffer the caller just set (it
   goes on there in the worker); 1 made, 0 not (no worker) */
int la_win_fork(void **ctx)
{
    size_t at, used = pm_used(), k;
    uintptr_t lo = PM_BASE, hi = PM_BASE + used, sp = (uintptr_t)__builtin_frame_address(0);
    uintptr_t skip_lo[8], skip_hi[8];
    int ns = 0, i, n;
    double t0;
    if (!start_worker())
        return 0;
    t0 = now_ms();
    /* each coroutine's stack below its pointer (the running one's: here)
       is not in use */
    n = coro_stack_count();
    for (i = 0; i < n && ns < 8; i++) {
        void *s, *p;
        size_t sz;
        uintptr_t a, b, q;
        coro_stack_info(i, &s, &sz, &p);
        a = (uintptr_t)s;
        b = a + sz;
        q = sp >= a && sp < b ? sp : (uintptr_t)p;
        if (q > a + 4096 && q <= b) {
            skip_lo[ns] = (a + 0xFFF) & ~(uintptr_t)0xFFF;
            skip_hi[ns] = (q - 4096) & ~(uintptr_t)0xFFF;
            if (skip_hi[ns] > skip_lo[ns])
                ns++;
        }
    }
    ResetEvent(h_done);
    ctl->abort = 0;
    ctl->status = 0;
    ctl->ctx = ctx;
    ctl->stack_base = teb_get(8);
    ctl->stack_limit = teb_get(16);
    ctl->data_len = (uint64_t)(srfx_data_end - srfx_data_begin);
    ctl->bss_len = (uint64_t)(srfx_bss_end - srfx_bss_begin);
    ctl->committed = pm_committed();
    at = 0;
    if (!put(at, srfx_data_begin, ctl->data_len))
        return 0;
    at += ctl->data_len;
    if (!put(at, srfx_bss_begin, ctl->bss_len))
        return 0;
    at += ctl->bss_len;
    /* the memory range, the unused stack parts left out (in order) */
    ctl->nchunk = 0;
    while (lo < hi && ctl->nchunk < MAX_CHUNKS) {
        uintptr_t end = hi, next = hi;
        for (i = 0; i < ns; i++)
            if (skip_lo[i] >= lo && skip_lo[i] < end) {
                end = skip_lo[i];
                next = skip_hi[i];
            }
        k = (size_t)ctl->nchunk++;
        ctl->chunk_at[k] = lo;
        ctl->chunk_len[k] = end - lo;
        if (!put(at, (void *)lo, end - lo))
            return 0;
        at += end - lo;
        lo = next > end ? next : end;
    }
    reaped = 0;
    SetEvent(h_go);
    {
        double t = now_ms() - t0;
        snaps++;
        snap_ms += t;
        snap_ms_max = t > snap_ms_max ? t : snap_ms_max;
        snap_mb += at / 1048576.0;
    }
    return 1;
}

/* the copy ended? (its code in *st); wait: until it has */
int la_win_wait(int *st, int wait)
{
    if (!started || reaped)
        return -1;
    if (WaitForSingleObject(h_done, wait ? INFINITE : 0) != WAIT_OBJECT_0)
        return 0;
    if (st)
        *st = ctl->status;
    reaped = 1;
    return 1;
}

void la_win_kill(void)
{
    if (started)
        InterlockedExchange(&ctl->abort, 1);
}

/* ---- the worker ---- */
static int worker;
static void *idle[5];               /* where a copy's end comes back to (this file's: not copied) */
static uint64_t own_base, own_limit;

int la_win_in_worker(void) { return worker; }
int la_win_aborted(void) { return worker && ctl->abort; }

/* the copy's end (interp.c's _exit): back to the worker's wait */
void la_win_exit(int code)
{
    if (!worker)
        ExitProcess((UINT)code);
    ctl->status = code;
    __builtin_longjmp(idle, 1);
}

static void resume(void)
{
    teb_set(8, ctl->stack_base);
    teb_set(16, ctl->stack_limit);
    __builtin_longjmp(ctl->ctx, 1);
}

int la_win_worker_main(int argc, char **argv)
{
    unsigned long long v[7];
    HANDLE parent, waits[2];
    int i;
    if (argc < 9)
        return 2;
    for (i = 0; i < 7; i++)
        v[i] = strtoull(argv[2 + i], NULL, 16);
    h_ctl = (HANDLE)(uintptr_t)v[0];
    h_snap = (HANDLE)(uintptr_t)v[1];
    h_slot = (HANDLE)(uintptr_t)v[2];
    slot_size = (size_t)v[3];
    h_go = (HANDLE)(uintptr_t)v[4];
    h_done = (HANDLE)(uintptr_t)v[5];
    parent = (HANDLE)(uintptr_t)v[6];
    if (!pm_reserve())
        return 3;
    ctl = (Ctl *)MapViewOfFile(h_ctl, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Ctl));
    snap = (uint8_t *)MapViewOfFile(h_snap, FILE_MAP_READ, 0, 0, SNAP_SIZE);
    if (!ctl || !snap || !MapViewOfFileEx(h_slot, FILE_MAP_ALL_ACCESS, 0, 0, slot_size, (void *)LA_SHARED_BASE))
        return 4;
    own_base = teb_get(8);
    own_limit = teb_get(16);
    worker = 1;
    ctl->ready = 1;
    waits[0] = h_go;
    waits[1] = parent;
    for (;;) {
        size_t at;
        int k;
        if (WaitForMultipleObjects(2, waits, FALSE, INFINITE) != WAIT_OBJECT_0)
            ExitProcess(0);         /* the game has ended */
        /* the game's memory made usable as far as it was, then the copy in place */
        if (!VirtualAlloc((void *)PM_BASE, (size_t)ctl->committed, MEM_COMMIT, PAGE_READWRITE))
            ExitProcess(5);
        memcpy(srfx_data_begin, snap, (size_t)ctl->data_len);
        at = (size_t)ctl->data_len;
        memcpy(srfx_bss_begin, snap + at, (size_t)ctl->bss_len);
        at += (size_t)ctl->bss_len;
        for (k = 0; k < ctl->nchunk; k++) {
            memcpy((void *)(uintptr_t)ctl->chunk_at[k], snap + at, (size_t)ctl->chunk_len[k]);
            at += (size_t)ctl->chunk_len[k];
        }
        if (!__builtin_setjmp(idle))
            resume();
        /* the copy has ended (la_win_exit) */
        teb_set(8, own_base);
        teb_set(16, own_limit);
        SetEvent(h_done);
    }
}
#endif
