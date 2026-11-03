/*
 * port_mem.c: the program's own memory on Windows (see port_mem.h).
 *
 * One range reserved at PM_BASE, used from the bottom up and made usable
 * (committed) as it grows. Blocks of up to 1 MB come in powers of two with
 * a free list for each size; bigger ones are whole runs of pages, kept on a
 * list of their own when freed (first that fits). Each block has a 16-byte
 * header: its size class (or its page run's length). The state (the top,
 * the lists) is static data of this file, so a snapshot of the program's
 * static data and of the range is consistent: the copy allocates and frees
 * on from where the game was.
 */
#define PORT_MEM_IMPL
#include <stdlib.h>
#ifndef _WIN32
void *port_pic_alloc(size_t n) { return malloc(n); }
#else
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "port_mem.h"
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef strdup

enum { MIN_SHIFT = 4, MAX_SHIFT = 20, NCLASS = MAX_SHIFT - MIN_SHIFT + 1, HDR = 16 };

typedef struct Big { size_t len; struct Big *next; } Big;   /* a freed run of pages (its header) */

static int reserved;
static uintptr_t top = PM_BASE;         /* the next free address */
static size_t committed;                 /* bytes usable from PM_BASE */
static void *freelist[NCLASS];
static Big *bigfree;
static volatile LONG lock;              /* (the render threads may allocate too) */
static void take(void) { while (InterlockedCompareExchange(&lock, 1, 0)) YieldProcessor(); }
static void give(void) { InterlockedExchange(&lock, 0); }

uintptr_t pm_used(void) { return top - PM_BASE; }
size_t pm_committed(void) { return committed; }

int pm_reserve(void)
{
    if (reserved)
        return 1;
    if (!VirtualAlloc((void *)PM_BASE, PM_RESERVE, MEM_RESERVE, PAGE_READWRITE)) {
        fprintf(stderr, "port: the memory range at %p could not be reserved\n", (void *)PM_BASE);
        return 0;
    }
    reserved = 1;
    return 1;
}

int pm_commit(size_t n)
{
    size_t want = (n + 0xFFFFF) & ~(size_t)0xFFFFF;     /* a MB at a time */
    if (!pm_reserve())
        return 0;
    if (want > PM_RESERVE)
        return 0;
    if (want > committed) {
        if (!VirtualAlloc((void *)(PM_BASE + committed), want - committed, MEM_COMMIT, PAGE_READWRITE))
            return 0;
        committed = want;
    }
    return 1;
}

static void *grab(size_t n)
{
    uintptr_t p = top;
    if (!pm_commit(p + n - PM_BASE)) {
        fprintf(stderr, "port: out of memory (%zu bytes more, %zu in use)\n", n, (size_t)(top - PM_BASE));
        abort();
    }
    top += n;
    return (void *)p;
}

static int class_of(size_t n)
{
    int c = MIN_SHIFT;
    while (((size_t)1 << c) < n + HDR)
        c++;
    return c;
}

static void *alloc_locked(size_t n);
void *pm_malloc(size_t n)
{
    void *p;
    take();
    p = alloc_locked(n);
    give();
    return p;
}

static void *alloc_locked(size_t n)
{
    uint8_t *b;
    if (!n)
        n = 1;
    if (n + HDR <= ((size_t)1 << MAX_SHIFT)) {
        int c = class_of(n), k = c - MIN_SHIFT;
        if (freelist[k]) {
            b = (uint8_t *)freelist[k];
            freelist[k] = *(void **)(b + HDR);
        } else
            b = (uint8_t *)grab((size_t)1 << c);
        *(size_t *)b = (size_t)c;
    } else {
        size_t len = (n + HDR + 0xFFF) & ~(size_t)0xFFF;
        Big **q = &bigfree, *f;
        for (; (f = *q); q = &f->next)
            if (f->len >= len)
                break;
        if (f) {
            *q = f->next;
            len = f->len;
            b = (uint8_t *)f;
        } else
            b = (uint8_t *)grab(len);
        *(size_t *)b = len;         /* (above any class number) */
    }
    return b + HDR;
}

static int mine(const void *p) { return (uintptr_t)p >= PM_BASE && (uintptr_t)p < PM_BASE + PM_RESERVE; }

void pm_free(void *p)
{
    uint8_t *b;
    size_t c;
    if (!p)
        return;
    if (!mine(p)) {
        free(p);                    /* the system's (a pointer from it) */
        return;
    }
    take();
    b = (uint8_t *)p - HDR;
    c = *(size_t *)b;
    if (c <= MAX_SHIFT) {
        *(void **)(b + HDR) = freelist[c - MIN_SHIFT];
        freelist[c - MIN_SHIFT] = b;
    } else {
        Big *f = (Big *)b;
        f->len = c;
        f->next = bigfree;
        bigfree = f;
    }
    give();
}

static size_t capacity(const void *p)
{
    size_t c = *(const size_t *)((const uint8_t *)p - HDR);
    return (c <= MAX_SHIFT ? (size_t)1 << c : c) - HDR;
}

void *pm_calloc(size_t n, size_t k)
{
    size_t t = n * k;
    void *p;
    if (k && t / k != n)
        return NULL;
    p = pm_malloc(t);
    memset(p, 0, t);
    return p;
}

void *pm_realloc(void *p, size_t n)
{
    void *q;
    size_t have;
    if (!p)
        return pm_malloc(n);
    if (!mine(p))
        return realloc(p, n);
    have = capacity(p);
    if (n <= have)
        return p;
    q = pm_malloc(n);
    memcpy(q, p, have);
    pm_free(p);
    return q;
}

void *port_pic_alloc(size_t n) { return malloc(n); }   /* (the system's: pm_free hands it back to free) */

char *pm_strdup(const char *s)
{
    size_t n = strlen(s) + 1;
    char *d = (char *)pm_malloc(n);
    memcpy(d, s, n);
    return d;
}
#endif
