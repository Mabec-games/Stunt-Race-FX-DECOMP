/*
 * jump.h: a non-local jump within one stack (a coroutine's, or the
 * program's), with no unwinding and no signal mask: the builtin pair where
 * the compiler has it (x86-64, Windows included), else _setjmp/_longjmp.
 */
#ifndef PORT_JUMP_H
#define PORT_JUMP_H

#if defined(__x86_64__)
typedef void *port_jmp_buf[5];
#define port_setjmp(b) __builtin_setjmp(b)
#define port_longjmp(b) __builtin_longjmp(b, 1)
#else
#include <setjmp.h>
typedef jmp_buf port_jmp_buf;
#define port_setjmp(b) _setjmp(b)
#define port_longjmp(b) _longjmp(b, 1)
#endif

#endif
