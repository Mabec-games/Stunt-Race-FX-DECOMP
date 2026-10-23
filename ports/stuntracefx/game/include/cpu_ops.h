/*
 * cpu_ops.h: single 65816 instructions on the register file, for routines
 * whose exit registers and flags follow every instruction they run. Each
 * step sets A (or X) and the flags as the instruction named does; memory
 * goes through rd8/wr8 as everywhere else.
 */
#ifndef STUNTRACEFX_CPU_OPS_H
#define STUNTRACEFX_CPU_OPS_H

#include "machine.h"
#include "native.h"

#include "alu.h"

static inline uint16_t op_adc16(uint16_t a, uint16_t m, uint8_t *p) { return alu_adc16(a, m, p); }
static inline uint16_t op_sbc16(uint16_t a, uint16_t m, uint8_t *p) { return alu_sbc16(a, m, p); }
static inline uint8_t op_adc8(uint8_t a, uint8_t m, uint8_t *p) { return alu_adc8(a, m, p); }
static inline uint8_t op_sbc8(uint8_t a, uint8_t m, uint8_t *p) { return alu_sbc8(a, m, p); }

/* LDA (16-bit), from memory or immediate. */
static inline void op_lda16(uint32_t addr)
{
    cpu.a = reg_rd16(addr);
    cpu.p = nz16(cpu.p, cpu.a);
}

static inline void op_lda16_imm(uint16_t v)
{
    cyc(38);
    cpu.a = v;
    cpu.p = nz16(cpu.p, v);
}

/* LDA (8-bit): the low byte of A. */
static inline void op_lda8(uint32_t addr)
{
    uint8_t v = reg_rd(addr);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* CMP # or CMP memory (16-bit). */
static inline void op_cmp16(uint16_t m)
{
    cyc(0);
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.a - m)) & ~P_C) | (cpu.a >= m ? P_C : 0));
}

static inline void op_and16(uint16_t m)
{
    cyc(2);
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
    cyc(30);
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
    cyc(30);
    cpu.a = cpu.x;
    cpu.p = nz16(cpu.p, cpu.a);
}

/* SEP and REP: 8-bit index registers clear the high bytes of X and Y. */
static inline void op_sep(uint8_t bits)
{
    cpu.p |= bits;
    if (bits & P_X) {
        cyc(22);
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

static inline void op_rep(uint8_t bits)
{
    cyc(22);
    cpu.p &= (uint8_t)~bits;
}

/* PLP: restoring 8-bit index registers clears the high bytes of X and Y. */
static inline void op_plp(uint8_t p)
{
    cyc(42);
    cpu.p = p;
    if (p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/* A delay loop of `turns` passes of a decrement (DEC A, DEX or DEY) and a
   BNE back: 14 clocks for each decrement (its fetch and an internal
   cycle), 22 for each BNE taken (two fetches and one), 16 for the last,
   not taken. */
static inline void delay_turns(uint32_t turns)
{
    cpu_idle(turns * 36u - 6u);
}

#endif
