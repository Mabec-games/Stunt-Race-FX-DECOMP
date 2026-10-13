/*
 * cpu_ops.h: single 65816 instructions on the register file, for routines
 * whose exit registers and flags follow every instruction they run. Each
 * step sets A (or X) and the flags as the instruction named does; memory
 * goes through rd8/wr8 as everywhere else.
 */
#ifndef STUNTRACEFX_CPU_OPS_H
#define STUNTRACEFX_CPU_OPS_H

#include "machine.h"

/* The 65816's 16-bit ADC and SBC, binary or decimal as the D flag in *p
   says, as the emulator core computes them: the result, with N, Z, C and V
   in *p. */
static inline uint16_t op_adc16(uint16_t a, uint16_t m, uint8_t *p)
{
    int c = *p & P_C, r;

    if (!(*p & P_D)) {
        r = a + m + c;
    } else {
        r = (a & 0x000F) + (m & 0x000F) + c;
        if (r > 0x0009) r += 0x0006;
        c = r > 0x000F;
        r = (a & 0x00F0) + (m & 0x00F0) + (c << 4) + (r & 0x000F);
        if (r > 0x009F) r += 0x0060;
        c = r > 0x00FF;
        r = (a & 0x0F00) + (m & 0x0F00) + (c << 8) + (r & 0x00FF);
        if (r > 0x09FF) r += 0x0600;
        c = r > 0x0FFF;
        r = (a & 0xF000) + (m & 0xF000) + (c << 12) + (r & 0x0FFF);
    }
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    if ((*p & P_D) && r > 0x9FFF) r += 0x6000;
    if (r > 0xFFFF) *p |= P_C;
    *p = nz16(*p, (uint16_t)r);
    return (uint16_t)r;
}

static inline uint16_t op_sbc16(uint16_t a, uint16_t m, uint8_t *p)
{
    int c = *p & P_C, r;

    m = (uint16_t)~m;
    if (!(*p & P_D)) {
        r = a + m + c;
    } else {
        r = (a & 0x000F) + (m & 0x000F) + c;
        if (r <= 0x000F) r -= 0x0006;
        c = r > 0x000F;
        r = (a & 0x00F0) + (m & 0x00F0) + (c << 4) + (r & 0x000F);
        if (r <= 0x00FF) r -= 0x0060;
        c = r > 0x00FF;
        r = (a & 0x0F00) + (m & 0x0F00) + (c << 8) + (r & 0x00FF);
        if (r <= 0x0FFF) r -= 0x0600;
        c = r > 0x0FFF;
        r = (a & 0xF000) + (m & 0xF000) + (c << 12) + (r & 0x0FFF);
    }
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    if ((*p & P_D) && r <= 0xFFFF) r -= 0x6000;
    if (r > 0xFFFF) *p |= P_C;
    *p = nz16(*p, (uint16_t)r);
    return (uint16_t)r;
}

/* The 65816's 8-bit ADC and SBC, binary or decimal as the D flag in *p
   says, as the emulator core computes them. */
static inline uint8_t op_adc8(uint8_t a, uint8_t m, uint8_t *p)
{
    int c = *p & P_C, r;

    if (!(*p & P_D)) {
        r = a + m + c;
    } else {
        r = (a & 0x0F) + (m & 0x0F) + c;
        if (r > 0x09) r += 0x06;
        c = r > 0x0F;
        r = (a & 0xF0) + (m & 0xF0) + (c << 4) + (r & 0x0F);
    }
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0));
    if ((*p & P_D) && r > 0x9F) r += 0x60;
    if (r > 0xFF) *p |= P_C;
    *p = nz8(*p, (uint8_t)r);
    return (uint8_t)r;
}

static inline uint8_t op_sbc8(uint8_t a, uint8_t m, uint8_t *p)
{
    int c = *p & P_C, r;

    m = (uint8_t)~m;
    if (!(*p & P_D)) {
        r = a + m + c;
    } else {
        r = (a & 0x0F) + (m & 0x0F) + c;
        if (r <= 0x0F) r -= 0x06;
        c = r > 0x0F;
        r = (a & 0xF0) + (m & 0xF0) + (c << 4) + (r & 0x0F);
    }
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0));
    if ((*p & P_D) && r <= 0xFF) r -= 0x60;
    if (r > 0xFF) *p |= P_C;
    *p = nz8(*p, (uint8_t)r);
    return (uint8_t)r;
}

/* LDA (16-bit), from memory or immediate. */
static inline void op_lda16(uint32_t addr)
{
    cpu.a = rd16(addr);
    cpu.p = nz16(cpu.p, cpu.a);
}

static inline void op_lda16_imm(uint16_t v)
{
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
}

/* LDA (8-bit): the low byte of A. */
static inline void op_lda8(uint32_t addr)
{
    uint8_t v = rd8(addr);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* CMP # or CMP memory (16-bit). */
static inline void op_cmp16(uint16_t m)
{
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.a - m)) & ~P_C) | (cpu.a >= m ? P_C : 0));
}

static inline void op_and16(uint16_t m)
{
    cpu.a &= m;
    cpu.p = nz16(cpu.p, cpu.a);
}

static inline void op_lsr16(void)
{
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (cpu.a & 1));
    cpu.a >>= 1;
    cpu.p = nz16(cpu.p, cpu.a);
}

static inline void op_asl16(void)
{
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (cpu.a >> 15));
    cpu.a = (uint16_t)(cpu.a << 1);
    cpu.p = nz16(cpu.p, cpu.a);
}

static inline void op_rol16(void)
{
    uint8_t c = (uint8_t)(cpu.a >> 15);

    cpu.a = (uint16_t)(cpu.a << 1 | (cpu.p & P_C));
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_C), cpu.a) | c);
}

/* XBA: N and Z from the new low byte. */
static inline void op_xba(void)
{
    cpu.a = (uint16_t)(cpu.a << 8 | cpu.a >> 8);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.a);
}

/* CLC / ADC and SEC / SBC (16-bit). */
static inline void op_add16(uint16_t m)
{
    cpu.p &= (uint8_t)~P_C;
    cpu.a = op_adc16(cpu.a, m, &cpu.p);
}

static inline void op_sub16(uint16_t m)
{
    cpu.p |= P_C;
    cpu.a = op_sbc16(cpu.a, m, &cpu.p);
}

/* INC of a work-RAM word: one read, one write; N, Z from the result. */
static inline void op_inc16_mem(uint32_t addr)
{
    uint16_t v = (uint16_t)(rd16(addr) + 1);

    wr16(addr, v);
    cpu.p = nz16(cpu.p, v);
}

/* TAX, TAY, TXA, TYA with 16-bit registers. */
static inline void op_tax16(void)
{
    cpu.x = cpu.a;
    cpu.p = nz16(cpu.p, cpu.x);
}

static inline void op_tay16(void)
{
    cpu.y = cpu.a;
    cpu.p = nz16(cpu.p, cpu.y);
}

static inline void op_txa16(void)
{
    cpu.a = cpu.x;
    cpu.p = nz16(cpu.p, cpu.a);
}

/* SEP and REP: 8-bit index registers clear the high bytes of X and Y. */
static inline void op_sep(uint8_t bits)
{
    cpu.p |= bits;
    if (bits & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

static inline void op_rep(uint8_t bits)
{
    cpu.p &= (uint8_t)~bits;
}

#endif
