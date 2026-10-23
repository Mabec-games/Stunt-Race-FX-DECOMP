/*
 * carried.h: cpu_ops.h's single 65816 instructions on a carried copy of
 * the registers instead of the register file, for routines in plain C
 * whose callers still read the registers they leave: each step sets A (or
 * X) and the flags as the instruction named does.
 */
#ifndef STUNTRACEFX_CARRIED_H
#define STUNTRACEFX_CARRIED_H

#include "machine.h"
#include "native.h"
#include "alu.h"

/* The registers a routine leaves for its callers: A, X, Y, the direct
   page, P and the data bank. */
typedef struct {
    uint16_t a, x, y, d;
    uint8_t p, db;
} Carried;

/* A routine on carried registers, passed as a value. */
typedef void (*CarriedFn)(Carried *);

/* A carried routine called from code still on the register file: the
   registers go in and come back. */
static inline void cr_call(CarriedFn f)
{
    Carried c = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    f(&c);
    cpu.a = c.a;
    cpu.x = c.x;
    cpu.y = c.y;
    cpu.d = c.d;
    cpu.p = c.p;
    cpu.db = c.db;
}

/* LDA (16-bit), from memory or immediate. */
static inline void r_lda16(Carried *cr, uint32_t addr)
{
    cr->a = reg_rd16(addr);
    cr->p = nz16(cr->p, cr->a);
}

static inline void r_lda16_imm(Carried *cr, uint16_t v)
{
    cyc(38);
    cr->a = v;
    cr->p = nz16(cr->p, v);
}

/* LDA (8-bit): the low byte of A. */
static inline void r_lda8(Carried *cr, uint32_t addr)
{
    uint8_t v = reg_rd(addr);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = nz8(cr->p, v);
}

/* CMP # or CMP memory (16-bit). */
static inline void r_cmp16(Carried *cr, uint16_t m)
{
    cyc(0);
    cr->p = (uint8_t)((nz16(cr->p, (uint16_t)(cr->a - m)) & ~P_C) | (cr->a >= m ? P_C : 0));
}

static inline void r_and16(Carried *cr, uint16_t m)
{
    cyc(2);
    cr->a &= m;
    cr->p = nz16(cr->p, cr->a);
}

static inline void r_lsr16(Carried *cr)
{
    cr->p = (uint8_t)((cr->p & ~P_C) | (cr->a & 1));
    cr->a >>= 1;
    cr->p = nz16(cr->p, cr->a);
}

static inline void r_asl16(Carried *cr)
{
    cr->p = (uint8_t)((cr->p & ~P_C) | (cr->a >> 15));
    cr->a = (uint16_t)(cr->a << 1);
    cr->p = nz16(cr->p, cr->a);
}

static inline void r_rol16(Carried *cr)
{
    uint8_t c = (uint8_t)(cr->a >> 15);

    cr->a = (uint16_t)(cr->a << 1 | (cr->p & P_C));
    cr->p = (uint8_t)(nz16((uint8_t)(cr->p & ~P_C), cr->a) | c);
}

/* XBA: N and Z from the new low byte. */
static inline void r_xba(Carried *cr)
{
    cr->a = (uint16_t)(cr->a << 8 | cr->a >> 8);
    cr->p = nz8(cr->p, (uint8_t)cr->a);
}

/* CLC / ADC and SEC / SBC (16-bit). */
static inline void r_add16(Carried *cr, uint16_t m)
{
    cr->p &= (uint8_t)~P_C;
    cr->a = alu_adc16(cr->a, m, &cr->p);
}

static inline void r_sub16(Carried *cr, uint16_t m)
{
    cr->p |= P_C;
    cr->a = alu_sbc16(cr->a, m, &cr->p);
}

/* INC of a work-RAM word: one read, one write; N, Z from the result. */
static inline void r_inc16_mem(Carried *cr, uint32_t addr)
{
    uint16_t v = (uint16_t)(rd16(addr) + 1);

    wr16(addr, v);
    cr->p = nz16(cr->p, v);
}

/* TAX, TAY, TXA, TYA with 16-bit registers. */
static inline void r_tax16(Carried *cr)
{
    cyc(30);
    cr->x = cr->a;
    cr->p = nz16(cr->p, cr->x);
}

static inline void r_tay16(Carried *cr)
{
    cr->y = cr->a;
    cr->p = nz16(cr->p, cr->y);
}

static inline void r_txa16(Carried *cr)
{
    cyc(30);
    cr->a = cr->x;
    cr->p = nz16(cr->p, cr->a);
}

/* SEP and REP: 8-bit index registers clear the high bytes of X and Y. */
static inline void r_sep(Carried *cr, uint8_t bits)
{
    cr->p |= bits;
    if (bits & P_X) {
        cyc(22);
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

static inline void r_rep(Carried *cr, uint8_t bits)
{
    cyc(22);
    cr->p &= (uint8_t)~bits;
}

/* PLP: restoring 8-bit index registers clears the high bytes of X and Y. */
static inline void r_plp(Carried *cr, uint8_t p)
{
    cyc(42);
    cr->p = p;
    if (p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

/* P as an approach (approach.c) leaves it; an approach of $3E toward A
   on the carried registers, as approach_regs does on the register file. */
uint8_t ApproachExitP(uint8_t p, uint16_t v, int wide, int moved);

static inline void r_approach(Carried *cr, uint16_t (*f)(uint16_t, int *))
{
    int moved;

    cr->a = f(cr->a, &moved);
    cr->p = ApproachExitP(cr->p, cr->a, 1, moved);
}

static inline void r_approach_byte(Carried *cr, uint8_t (*f)(uint8_t, int *))
{
    int moved;
    uint8_t v = f((uint8_t)cr->a, &moved);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->p = ApproachExitP(cr->p, v, 0, moved);
}

#endif
