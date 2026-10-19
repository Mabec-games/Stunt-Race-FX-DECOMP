/*
 * coro.c: coroutines (see coro.h).
 */
#include <stdint.h>
#include <stdlib.h>

#include "coro.h"
#include "snes.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

struct Coro {
    void *fiber;
    void (*fn)(void *);
    void *arg;
};

static Coro s_self;

Coro *coro_self(void)
{
    if (!s_self.fiber)
        s_self.fiber = ConvertThreadToFiber(NULL);
    return &s_self;
}

static void WINAPI fiber_main(void *p)
{
    Coro *c = (Coro *)p;
    c->fn(c->arg);
    snes_fatal("a coroutine returned");
}

Coro *coro_new(void (*fn)(void *), void *arg, size_t stack)
{
    Coro *c = (Coro *)calloc(1, sizeof *c);
    coro_self();
    c->fn = fn;
    c->arg = arg;
    c->fiber = CreateFiber(stack, fiber_main, c);
    if (!c->fiber)
        snes_fatal("no fiber");
    return c;
}

void coro_switch(Coro *from, Coro *to)
{
    (void)from;
    SwitchToFiber(to->fiber);
}

#else

struct Coro {
    void *sp;                  /* the saved stack pointer, while switched away */
    void (*fn)(void *);
    void *arg;
    void *stack;
};

/* save the callee-saved registers on this stack, *save = sp; sp = load;
   restore them from there and return into that coroutine */
void coro_swap(void **save, void *load);
void coro_start(void);         /* the first frame of a new coroutine */

static Coro s_self;

Coro *coro_self(void) { return &s_self; }

void coro_entry(Coro *c)
{
    c->fn(c->arg);
    snes_fatal("a coroutine returned");
}

Coro *coro_new(void (*fn)(void *), void *arg, size_t stack)
{
    Coro *c = (Coro *)calloc(1, sizeof *c);
    uintptr_t top;
    void **sp;
    c->fn = fn;
    c->arg = arg;
    c->stack = malloc(stack);
    if (!c->stack)
        snes_fatal("no coroutine stack");
    top = ((uintptr_t)c->stack + stack) & ~(uintptr_t)15;
    sp = (void **)top;
#if defined(__aarch64__)
    /* coro_swap pops 20 words: x19-x28, x29, x30, d8-d15; x19 = the
       coroutine, x30 = coro_start */
    sp -= 20;
    {
        int i;
        for (i = 0; i < 20; i++)
            sp[i] = 0;
    }
    sp[0] = c;                 /* x19 */
    sp[11] = (void *)coro_start; /* x30 */
#elif defined(__x86_64__)
    /* coro_swap pops r15, r14, r13, r12, rbx, rbp, then returns: r12 = the
       coroutine, the return address coro_start */
    *--sp = 0;                 /* alignment */
    *--sp = (void *)coro_start;
    *--sp = 0;                 /* rbp */
    *--sp = 0;                 /* rbx */
    *--sp = c;                 /* r12 */
    *--sp = 0;                 /* r13 */
    *--sp = 0;                 /* r14 */
    *--sp = 0;                 /* r15 */
#else
#error "coroutines: no switch for this processor"
#endif
    c->sp = sp;
    return c;
}

void coro_switch(Coro *from, Coro *to)
{
    coro_swap(&from->sp, to->sp);
}

#endif
