/*
 * spc.h: the SPC700 as the sound driver's C sees it.
 *
 * The C of an SPC700 routine works on `spc` (A, X, Y, SP and the PSW flags)
 * and makes every APU RAM and I/O access through spc_rd/spc_wr, in the
 * order and number the assembly makes them: the checks (tools/stuntracefx/
 * sfxbox, spccall.c) compare the accesses, and the hardware's reads (DSP
 * $F3, ports $F4-$F7, timers $FD-$FF) come back with the values the original
 * read, in order. The stack is page 1; direct page is page 0 or 1 by P.
 *
 * A routine is entered by CALL, PCALL or TCALL: the caller pushes the return
 * address (spc_call), the routine's RET pulls it (spc_ret). abi_Name(void)
 * is the routine at its entry, as the checks call it.
 */
#ifndef STUNTRACEFX_SPC_H
#define STUNTRACEFX_SPC_H

#include <stdint.h>

typedef struct {
    uint8_t a, x, y, sp;
    uint8_t n, v, p, b, h, i, z, c;   /* PSW flags, each 0 or 1 */
} SpcRegs;

extern SpcRegs spc;

uint8_t spc_rd(uint16_t addr);
void spc_wr(uint16_t addr, uint8_t v);

static inline uint16_t spc_dp(uint8_t d) { return (uint16_t)(spc.p ? 0x0100 | d : d); }
static inline uint16_t spc_ya(void) { return (uint16_t)(spc.y << 8 | spc.a); }
static inline void spc_set_ya(uint16_t v) { spc.a = (uint8_t)v; spc.y = (uint8_t)(v >> 8); }

static inline void spc_push(uint8_t v) { spc_wr((uint16_t)(0x0100 | spc.sp), v); spc.sp--; }
static inline uint8_t spc_pull(void) { spc.sp++; return spc_rd((uint16_t)(0x0100 | spc.sp)); }

static inline uint8_t spc_psw(void)
{
    return (uint8_t)(spc.n << 7 | spc.v << 6 | spc.p << 5 | spc.b << 4 | spc.h << 3 | spc.i << 2 | spc.z << 1 | spc.c);
}

static inline void spc_set_psw(uint8_t v)
{
    spc.n = v >> 7 & 1; spc.v = v >> 6 & 1; spc.p = v >> 5 & 1; spc.b = v >> 4 & 1;
    spc.h = v >> 3 & 1; spc.i = v >> 2 & 1; spc.z = v >> 1 & 1; spc.c = v & 1;
}

/* CALL: the return address pushed (high byte first), then the routine. */
static inline void spc_call(void (*abi)(void), uint16_t ret)
{
    spc_push((uint8_t)(ret >> 8));
    spc_push((uint8_t)ret);
    abi();
}

/* A jump from the C into code that stays original (the driver's reload
   loop, its start, the IPL ROM): the call ends here, as the checks end it
   at the original's first instruction at `addr` (hybrid.py lists every
   literal address passed). Write the address as a literal (0x0675). */
void spc_jump_original(uint16_t addr);

/* RET: the return address pulled (low byte first); the checks end the call
   at it. */
static inline uint16_t spc_ret(void)
{
    uint16_t lo = spc_pull();
    return (uint16_t)(lo | spc_pull() << 8);
}

#endif
