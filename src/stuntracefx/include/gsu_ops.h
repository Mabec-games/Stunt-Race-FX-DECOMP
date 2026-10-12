/*
 * gsu_ops.h: Super FX instructions with their flags, for the routines' C.
 *
 * Each leaves the status flags as the instruction does (gsu.h has the
 * loads, stores, ADD, SUB and LMULT); the C calls them where the assembly
 * runs the instruction so that the flags a routine exits with are the
 * original's.
 */
#ifndef STUNTRACEFX_GSU_OPS_H
#define STUNTRACEFX_GSU_OPS_H

#include "gsu.h"

/* ROL: a shifted left, the carry into bit 0, bit 15 out to the carry. */
static inline uint16_t gsu_rol(uint16_t a)
{
    uint16_t r = (uint16_t)(a << 1 | gsu.cy);

    gsu.cy = (uint8_t)(a >> 15);
    gsu_sz(r);
    return r;
}

/* LSR: a shifted right, bit 0 out to the carry. */
static inline uint16_t gsu_lsr(uint16_t a)
{
    uint16_t r = (uint16_t)(a >> 1);

    gsu.cy = (uint8_t)(a & 1);
    gsu_sz(r);
    return r;
}

/* ASR: a shifted right keeping the sign. */
static inline uint16_t gsu_asr(uint16_t a)
{
    uint16_t r = (uint16_t)((int16_t)a >> 1);

    gsu.cy = (uint8_t)(a & 1);
    gsu_sz(r);
    return r;
}

/* DIV2: as ASR, except that -1 gives 0. */
static inline uint16_t gsu_div2(uint16_t a)
{
    uint16_t r = a == 0xFFFF ? 0 : (uint16_t)((int16_t)a >> 1);

    gsu.cy = (uint8_t)(a & 1);
    gsu_sz(r);
    return r;
}

/* FMULT: the high word of the signed product, CY from its bit 15. */
static inline uint16_t gsu_fmult(uint16_t a, uint16_t b)
{
    return (uint16_t)(gsu_lmult(a, b) >> 16);
}

/* AND, OR, XOR, BIC (a & ~b), NOT: S and Z from the result. */
static inline uint16_t gsu_and(uint16_t a, uint16_t b) { gsu_sz((uint16_t)(a & b)); return (uint16_t)(a & b); }
static inline uint16_t gsu_or(uint16_t a, uint16_t b) { gsu_sz((uint16_t)(a | b)); return (uint16_t)(a | b); }
static inline uint16_t gsu_xor(uint16_t a, uint16_t b) { gsu_sz((uint16_t)(a ^ b)); return (uint16_t)(a ^ b); }
static inline uint16_t gsu_bic(uint16_t a, uint16_t b) { gsu_sz((uint16_t)(a & ~b)); return (uint16_t)(a & ~b); }
static inline uint16_t gsu_not(uint16_t a) { gsu_sz((uint16_t)~a); return (uint16_t)~a; }

/* INC and DEC: S and Z from the result. */
static inline uint16_t gsu_inc(uint16_t a) { gsu_sz((uint16_t)(a + 1)); return (uint16_t)(a + 1); }
static inline uint16_t gsu_dec(uint16_t a) { gsu_sz((uint16_t)(a - 1)); return (uint16_t)(a - 1); }

/* SWAP: the bytes exchanged; S and Z from the result. */
static inline uint16_t gsu_swap(uint16_t a)
{
    uint16_t r = (uint16_t)(a << 8 | a >> 8);

    gsu_sz(r);
    return r;
}

/* LOB and HIB: the low or high byte; S from its bit 7, Z from the byte. */
static inline uint16_t gsu_lob(uint16_t a)
{
    uint16_t r = (uint16_t)(a & 0xFF);

    gsu.s = (uint8_t)(r >> 7);
    gsu.z = r == 0;
    return r;
}

static inline uint16_t gsu_hib(uint16_t a)
{
    uint16_t r = (uint16_t)(a >> 8);

    gsu.s = (uint8_t)(r >> 7);
    gsu.z = r == 0;
    return r;
}

/* ROR: a shifted right, the carry into bit 15, bit 0 out to the carry. */
static inline uint16_t gsu_ror(uint16_t a)
{
    uint16_t r = (uint16_t)(a >> 1 | gsu.cy << 15);

    gsu.cy = (uint8_t)(a & 1);
    gsu_sz(r);
    return r;
}

/* LOOP's count: one less, S and Z from it; nonzero means go again. */
static inline int gsu_count_down(uint16_t *count)
{
    *count = (uint16_t)(*count - 1);
    gsu_sz(*count);
    return *count != 0;
}

/* GETBH: the ROM buffer as the high byte, the low byte of a kept. */
static inline uint16_t gsu_getbh(uint16_t a)
{
    return (uint16_t)(gsu_getb() << 8 | (a & 0xFF));
}

/* A word pushed on the R10 stack (STW, INC, INC) and pulled back (DEC,
   DEC, LDW). */
static inline void gsu_push(uint16_t *sp, uint16_t v)
{
    gsu_stw(*sp, v);
    *sp = gsu_inc(gsu_inc(*sp));
}

static inline uint16_t gsu_pull(uint16_t *sp)
{
    *sp = gsu_dec(gsu_dec(*sp));
    return gsu_ldw(*sp);
}

#endif
