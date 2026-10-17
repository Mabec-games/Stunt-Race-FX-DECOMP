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

/* GETBS: the ROM buffer, sign-extended. */
static inline uint16_t gsu_getbs(void)
{
    return (uint16_t)(int16_t)(int8_t)gsu_getb();
}

/* GETBH: the ROM buffer as the high byte, the low byte of a kept. */
static inline uint16_t gsu_getbh(uint16_t a)
{
    return (uint16_t)(gsu_getb() << 8 | (a & 0xFF));
}

/* SEX: the low byte sign-extended; S and Z from the result. */
static inline uint16_t gsu_sex(uint16_t a)
{
    uint16_t r = (uint16_t)(int16_t)(int8_t)a;

    gsu_sz(r);
    return r;
}

/* MERGE: the high bytes of R7 and R8; the flags from its bit pairs, as
   the core sets them (Z is set when any of the bits $F0F0 is). */
static inline uint16_t gsu_merge(void)
{
    uint16_t r = (uint16_t)((gsu.r[7] & 0xFF00) | gsu.r[8] >> 8);

    gsu.ov = (r & 0xC0C0) != 0;
    gsu.s = (r & 0x8080) != 0;
    gsu.cy = (r & 0xE0E0) != 0;
    gsu.z = (r & 0xF0F0) != 0;
    return r;
}

/* COLOR (and GETC, from the ROM buffer): the plot colour from a byte, as
   the plot options shape it: the source's high nibble into the low one
   (bit 2), or the colour's high nibble kept (bit 3). */
static inline void gsu_color(uint8_t v)
{
    if (gsu.por & 0x04)
        gsu.colr = (uint8_t)((gsu.colr & 0xF0) | v >> 4);
    else if (gsu.por & 0x08)
        gsu.colr = (uint8_t)((gsu.colr & 0xF0) | (v & 0x0F));
    else
        gsu.colr = v;
}

/* CMODE: the plot options, five bits of the source. */
static inline void gsu_cmode(uint16_t a)
{
    gsu.por = (uint8_t)(a & 0x1F);
}

/* ROMB: the ROM bank, seven bits of the source. */
static inline void gsu_romb(uint16_t a)
{
    gsu.rombr = (uint8_t)(a & 0x7F);
}

/* PLOT: a pixel at (R1, R2), then R1 steps right. */
static inline void gsu_plot_r1(void)
{
    gsu_plot((uint8_t)gsu.r[1], (uint8_t)gsu.r[2]);
    gsu.r[1] = (uint16_t)(gsu.r[1] + 1);
}

/* MULT and UMULT (Rn or #n): the low bytes multiplied, signed or not;
   S and Z from the product. */
static inline uint16_t gsu_mult(uint16_t a, uint16_t b)
{
    uint16_t v = (uint16_t)((int8_t)a * (int8_t)b);

    gsu_sz(v);
    return v;
}

static inline uint16_t gsu_umult(uint16_t a, uint16_t b)
{
    uint16_t v = (uint16_t)((uint8_t)a * (uint8_t)b);

    gsu_sz(v);
    return v;
}

/* RPIX: the pixel at (x, y), the pixel caches written out first; S and Z
   from it. */
static inline uint16_t gsu_rpix_r(uint16_t x, uint16_t y)
{
    uint16_t v = gsu_rpix((uint8_t)x, (uint8_t)y);

    gsu_sz(v);
    return v;
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
