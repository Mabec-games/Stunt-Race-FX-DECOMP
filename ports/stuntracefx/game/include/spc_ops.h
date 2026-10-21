/*
 * spc_ops.h: the SPC700's operations with their flags, and its accesses.
 *
 * Flags as the SPC700 sets them (the pinned core's processor/spc700:
 * algorithms.cpp, instructions.cpp). Accesses: an SPC700 store reads its
 * address first (a dummy read, which on an I/O register reaches the
 * hardware: a store to $F3 reads the DSP register), except MOV (X+),A, MOV
 * dd,ds and PUSH; a read-modify-write reads once and writes once; TSET and
 * TCLR read twice. spc_st, spc_rmw and the others make those accesses, each with the cycles before it, so the C makes the
 * assembly's accesses in its order and number.
 */
#ifndef STUNTRACEFX_SPC_OPS_H
#define STUNTRACEFX_SPC_OPS_H

#include "spc.h"

static inline uint8_t spc_nz(uint8_t x) { spc.z = x == 0; spc.n = x >> 7; return x; }
static inline uint16_t spc_nz16(uint16_t x) { spc.z = x == 0; spc.n = x >> 15; return x; }

static inline uint8_t spc_adc(uint8_t x, uint8_t y)
{
    int z = x + y + spc.c;
    spc.c = z > 0xFF;
    spc.h = ((x ^ y ^ z) & 0x10) != 0;
    spc.v = (~(x ^ y) & (x ^ z) & 0x80) != 0;
    return spc_nz((uint8_t)z);
}

static inline uint8_t spc_sbc(uint8_t x, uint8_t y) { return spc_adc(x, (uint8_t)~y); }

/* CMP: the flags of x - y; x is kept. */
static inline void spc_cmp(uint8_t x, uint8_t y)
{
    int z = x - y;
    spc.c = z >= 0;
    spc_nz((uint8_t)z);
}

static inline uint8_t spc_and(uint8_t x, uint8_t y) { return spc_nz(x & y); }
static inline uint8_t spc_or(uint8_t x, uint8_t y) { return spc_nz(x | y); }
static inline uint8_t spc_eor(uint8_t x, uint8_t y) { return spc_nz(x ^ y); }
static inline uint8_t spc_inc(uint8_t x) { return spc_nz((uint8_t)(x + 1)); }
static inline uint8_t spc_dec(uint8_t x) { return spc_nz((uint8_t)(x - 1)); }
static inline uint8_t spc_asl(uint8_t x) { spc.c = x >> 7; return spc_nz((uint8_t)(x << 1)); }
static inline uint8_t spc_lsr(uint8_t x) { spc.c = x & 1; return spc_nz(x >> 1); }
static inline uint8_t spc_rol(uint8_t x) { uint8_t c = spc.c; spc.c = x >> 7; return spc_nz((uint8_t)(x << 1 | c)); }
static inline uint8_t spc_ror(uint8_t x) { uint8_t c = spc.c; spc.c = x & 1; return spc_nz((uint8_t)(c << 7 | x >> 1)); }
static inline uint8_t spc_xcn(uint8_t x) { return spc_nz((uint8_t)(x >> 4 | x << 4)); }

/* ADDW, SUBW (YA with a word), CMPW. */
static inline uint16_t spc_addw(uint16_t x, uint16_t y)
{
    uint16_t z;
    spc.c = 0;
    z = spc_adc((uint8_t)x, (uint8_t)y);
    z |= (uint16_t)(spc_adc((uint8_t)(x >> 8), (uint8_t)(y >> 8)) << 8);
    spc.z = z == 0;
    return z;
}

static inline uint16_t spc_subw(uint16_t x, uint16_t y)
{
    uint16_t z;
    spc.c = 1;
    z = spc_sbc((uint8_t)x, (uint8_t)y);
    z |= (uint16_t)(spc_sbc((uint8_t)(x >> 8), (uint8_t)(y >> 8)) << 8);
    spc.z = z == 0;
    return z;
}

static inline void spc_cmpw(uint16_t x, uint16_t y)
{
    int z = x - y;
    spc.c = z >= 0;
    spc_nz16((uint16_t)z);
}

/* MUL YA: Y * A; N, Z from Y. */
static inline void spc_mul(void)
{
    uint16_t ya = (uint16_t)(spc.y * spc.a);
    spc.a = (uint8_t)ya;
    spc.y = (uint8_t)(ya >> 8);
    spc_nz(spc.y);
}

/* DIV YA,X, with the SPC700's result for a quotient past 511. */
static inline void spc_div(void)
{
    uint16_t ya = spc_ya();
    uint8_t x = spc.x;
    spc.h = (spc.y & 15) >= (x & 15);
    spc.v = spc.y >= x;
    if (spc.y < (x << 1)) {
        spc.a = (uint8_t)(ya / x);
        spc.y = (uint8_t)(ya % x);
    } else {
        spc.a = (uint8_t)(255 - (ya - (x << 9)) / (256 - x));
        spc.y = (uint8_t)(x + (ya - (x << 9)) % (256 - x));
    }
    spc_nz(spc.a);
}

static inline void spc_daa(void)
{
    if (spc.c || spc.a > 0x99) { spc.a += 0x60; spc.c = 1; }
    if (spc.h || (spc.a & 15) > 9) spc.a += 0x06;
    spc_nz(spc.a);
}

static inline void spc_das(void)
{
    if (!spc.c || spc.a > 0x99) { spc.a -= 0x60; spc.c = 0; }
    if (!spc.h || (spc.a & 15) > 9) spc.a -= 0x06;
    spc_nz(spc.a);
}

/* The accesses that are several, with the cycles before each (spc.h: spc_cyc),
   in order. A store: the dummy read, then the write. */
static inline void spc_st(uint16_t addr, uint8_t v, unsigned before_read, unsigned before_write)
{
    spc_cyc(before_read);
    spc_rd(addr);
    spc_cyc(before_write);
    spc_wr(addr, v);
}

static inline void spc_rmw(uint16_t addr, uint8_t (*op)(uint8_t), unsigned before_read, unsigned before_write)
{
    spc_cyc(before_read);
    uint8_t v = spc_rd(addr);
    spc_cyc(before_write);
    spc_wr(addr, op(v));
}

static inline void spc_movw_st(uint8_t d, unsigned before_read, unsigned before_low, unsigned before_high)
{
    spc_cyc(before_read);
    spc_rd(spc_dp(d));
    spc_cyc(before_low);
    spc_wr(spc_dp(d), spc.a);
    spc_cyc(before_high);
    spc_wr(spc_dp((uint8_t)(d + 1)), spc.y);
}

static inline void spc_movw_ld(uint8_t d, unsigned before_low, unsigned before_high)
{
    spc_cyc(before_low);
    uint16_t v = spc_rd(spc_dp(d));
    spc_cyc(before_high);
    v |= (uint16_t)(spc_rd(spc_dp((uint8_t)(d + 1))) << 8);
    spc_set_ya(spc_nz16(v));
}

static inline void spc_incw(uint8_t d, int adjust, unsigned before_read_low, unsigned before_write_low,
                                unsigned before_read_high, unsigned before_write_high)
{
    spc_cyc(before_read_low);
    uint16_t v = (uint16_t)(spc_rd(spc_dp(d)) + adjust);
    spc_cyc(before_write_low);
    spc_wr(spc_dp(d), (uint8_t)v);
    spc_cyc(before_read_high);
    v = (uint16_t)(v + (spc_rd(spc_dp((uint8_t)(d + 1))) << 8));
    spc_cyc(before_write_high);
    spc_wr(spc_dp((uint8_t)(d + 1)), (uint8_t)(v >> 8));
    spc_nz16(v);
}

static inline void spc_tset(uint16_t addr, int set, unsigned before_read, unsigned before_reread, unsigned before_write)
{
    spc_cyc(before_read);
    uint8_t v = spc_rd(addr);
    uint8_t d = (uint8_t)(spc.a - v);
    spc.z = d == 0;
    spc.n = d >> 7;
    spc_cyc(before_reread);
    spc_rd(addr);
    spc_cyc(before_write);
    spc_wr(addr, set ? (uint8_t)(v | spc.a) : (uint8_t)(v & ~spc.a));
}

static inline void spc_bit_set(uint8_t d, int bit, int set, unsigned before_read, unsigned before_write)
{
    spc_cyc(before_read);
    uint8_t v = spc_rd(spc_dp(d));
    spc_cyc(before_write);
    spc_wr(spc_dp(d), set ? (uint8_t)(v | 1 << bit) : (uint8_t)(v & ~(1 << bit)));
}

#endif
