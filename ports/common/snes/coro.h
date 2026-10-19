/*
 * coro.h: coroutines, for the processors that run beside the S-CPU.
 *
 * The Super FX's and the SPC700's C run on stacks of their own and hand
 * control back and forth with the S-CPU's (the program's own stack), one
 * running at a time: a deterministic schedule, no threads. The switch is a
 * few instructions of assembly on arm64 and x86-64 (coro_arm64.S,
 * coro_x86_64.S); on Windows, the system's fibers.
 */
#ifndef PORT_CORO_H
#define PORT_CORO_H

#include <stddef.h>

typedef struct Coro Coro;

/* the program's own stack, as a coroutine to switch back to */
Coro *coro_self(void);
/* a coroutine that runs fn(arg) on a stack of `stack` bytes when first
   switched to; fn never returns */
Coro *coro_new(void (*fn)(void *), void *arg, size_t stack);
/* leave the running coroutine `from` for `to` (until something switches
   back to `from`) */
void coro_switch(Coro *from, Coro *to);

#endif
