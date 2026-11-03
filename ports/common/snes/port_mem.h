/*
 * port_mem.h: on Windows, the program's own memory (malloc and the rest)
 * from one range of addresses at a fixed place (port_mem.c), the same in
 * every process of this program: a snapshot of it, with the program's
 * static data, is the whole running game (the look-ahead's copy, la_win.c,
 * where other systems fork). Elsewhere nothing changes.
 *
 * Every C file of the game, the machine and the runtime includes this
 * first (link.sh: -include); the system's and SDL's own memory stay theirs.
 */
#ifndef PORT_MEM_H
#define PORT_MEM_H

#include <stddef.h>

#if defined(_WIN32) && !defined(PORT_MEM_IMPL)
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void *pm_malloc(size_t n);
void *pm_calloc(size_t n, size_t k);
void *pm_realloc(void *p, size_t n);
void pm_free(void *p);
char *pm_strdup(const char *s);

#define malloc(n) pm_malloc(n)
#define calloc(n, k) pm_calloc(n, k)
#define realloc(p, n) pm_realloc(p, n)
#define free(p) pm_free(p)
#define strdup(s) pm_strdup(s)
#endif

/* memory for pictures only the shown game makes (the composed hi-res
   pictures, the renderer's samples): outside the range on Windows, so a
   snapshot leaves them out (the look-ahead's copy never draws them);
   freed with free(). Elsewhere plain malloc. */
void *port_pic_alloc(size_t n);

#ifdef _WIN32
/* the range: where it starts, how far it is in use (a snapshot's length) */
#define PM_BASE ((uintptr_t)0x200000000ull)
#define PM_RESERVE ((size_t)1 << 30)
#include <stdint.h>
uintptr_t pm_used(void);
/* the range reserved here (each process, before anything is in it) and its
   first n bytes made usable (a copy's, before the snapshot goes in) */
int pm_reserve(void);
int pm_commit(size_t n);
#endif

#endif
