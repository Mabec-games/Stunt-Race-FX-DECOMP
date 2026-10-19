/*
 * gsu.h: the Super FX (GSU-1) as the decomp's C sees it.
 *
 * A Super FX routine takes its inputs in the GSU's sixteen registers and
 * leaves its results there, and reads and writes the game pak through the
 * GSU's own bus: ROM in banks $00-$5F (banks $00-$3F in the S-CPU's LoROM
 * layout), the game pak RAM in bank $70. Its C works on `gsu`, the register
 * file, and on the bus through gsu_rd8/gsu_wr8, exactly as the original
 * code does, so the C has the same inputs and outputs as its assembly.
 * Inside the verification harness these go to the emulated Super FX; a
 * native port supplies its own. Plotting goes through gsu_plot and
 * gsu_rpix, which keep the GSU's two pixel caches and write the screen in
 * its character layout.
 *
 * Calls between Super FX routines are LINK (R11 = the return address) and a
 * jump; a routine returns by jumping to R11. Each routine with C has an abi_
 * wrapper that maps the registers and flags to the C's arguments and results
 * and leaves every register the assembly changes as the assembly leaves it.
 */
#ifndef STUNTRACEFX_GSU_H
#define STUNTRACEFX_GSU_H

#include <stdint.h>

typedef struct {
    uint16_t r[16];
    uint8_t z, cy, s, ov;    /* status flags: zero, carry, sign, overflow */
    uint8_t rombr;           /* bank the ROM buffer (GETB, GETC) reads from */
    uint8_t rambr;           /* bank of game pak RAM for loads and stores */
    uint8_t colr, por;       /* plot colour and plot options (COLOR, CMODE) */
    uint8_t scmr, scbr;      /* screen mode and base, set by the S-CPU */
    uint16_t ramaddr;        /* the last RAM address loaded or stored: SBK writes there */
    uint8_t romdr;           /* the ROM buffer: GETB and GETC read it */
    /* A prefix the caller left pending for the routine's first instruction
       (a WITH, TO, FROM or ALT in the delay slot of its call): B (set by
       WITH), ALT1, ALT2, and the source and destination registers. All 0
       when there is none. Set at entry; the C reads them, never writes. */
    uint8_t pre_b, pre_alt1, pre_alt2, pre_sreg, pre_dreg;
} RegsGSU;

extern RegsGSU gsu;

/* One byte on the Super FX's bus (24-bit: bank, address). */
uint8_t gsu_rd8(uint32_t addr);
void gsu_wr8(uint32_t addr, uint8_t value);

/* PLOT and RPIX at (x, y) with the current colour, options and screen. */
void gsu_plot(uint8_t x, uint8_t y);
uint8_t gsu_rpix(uint8_t x, uint8_t y);

/* The Super FX's time, as the original spends it. Nearly all of it is
   fetching its code: the C says where it stands for which of the
   original's code bytes, gsu_code(bank:addr, n) for the n bytes fetched
   from addr on, and where the original fetches a CACHE instruction (R15
   one past it: as it runs, after the next byte's fetch, the code cache's
   base moves there and the cache empties when it changes). The rest comes with what the C does: its loads and stores
   (the RAM buffer), its R14 writes and ROM-buffer reads, MULT and FMULT,
   PLOT and RPIX (the pixel caches). The runtime prices each as the GSU-1
   does (a byte from the code cache, a cache line's fill, a byte outside
   it; the buffers' waits; ports/common/snes/gsu_clock.h). gsu_cyc states
   any clocks beyond those (master clocks, 21.47 MHz). The stated forms of
   a branch and a loop, as machine.h's cyc_then and cyc_loop. */
void gsu_code(uint32_t addr, uint32_t n);
static inline uint32_t gsu_code_then(uint32_t addr, uint32_t n, uint32_t v) { gsu_code(addr, n); return v; }
static inline int gsu_code_loop(uint32_t addr, uint32_t n, int again) { if (again) gsu_code(addr, n); return again; }
void gsu_cache(uint16_t r15);
static inline uint32_t gsu_cache_then(uint16_t r15, uint32_t v) { gsu_cache(r15); return v; }
static inline int gsu_cache_loop(uint16_t r15, int again) { if (again) gsu_cache(r15); return again; }
/* A helper that stands for several copies of the original's code (one
   beside each caller, or one for each case of a table) states the code of
   the copy it runs as from a table of them: its caller passes the copy's
   number. Each entry is a stretch of code, or with n 0 a CACHE (R15 =
   addr); both 0, none (a copy with fewer stretches there). */
typedef struct { uint32_t addr, n; } GsuCode;
#define GSU_COPY(k) (k)     /* the copy a call runs its helper as */
static inline void gsu_codes(const GsuCode *c, int k)
{
    for (int i = 0; i < k; i++) {
        if (c[i].n)
            gsu_code(c[i].addr, c[i].n);
        else if (c[i].addr)
            gsu_cache((uint16_t)c[i].addr);
    }
}
static inline uint32_t gsu_codes_then(const GsuCode *c, int k, uint32_t v) { gsu_codes(c, k); return v; }
static inline int gsu_codes_loop(const GsuCode *c, int k, int again) { if (again) gsu_codes(c, k); return again; }
void gsu_cyc(uint32_t clocks);
static inline uint32_t gsu_cyc_then(uint32_t clocks, uint32_t v) { gsu_cyc(clocks); return v; }
static inline int gsu_cyc_loop(uint32_t clocks, int again) { if (again) gsu_cyc(clocks); return again; }

/* The time of an instruction beyond its fetch, for the helpers here and
   in gsu_ops.h: MULT or UMULT, FMULT or LMULT. */
enum { GSU_T_MULT, GSU_T_FMULT };
void gsu_time(int what);

/* The buffers, for the helpers here and in gsu_ops.h: an R14 write (the
   ROM buffer starts its fill), a read of the ROM buffer (GETB, GETC),
   RAMB (the RAM buffer's store made first), ROMB (the ROM buffer's fill
   finished first). Their time depends on the code run between them, so
   each is a place in the C's time of its own, as an access is. */
enum { GSU_B_R14, GSU_B_ROMBUF, GSU_B_RAMBUF, GSU_B_ROMB };
void gsu_buffer(int what);

/* Game pak RAM address `a` in the current RAM bank. */
static inline uint32_t gsu_ram(uint16_t a)
{
    return 0x700000u | (uint32_t)(gsu.rambr & 1) << 16 | a;
}

/* LDW, LM, LMS: the low byte at a, the high byte at a ^ 1. */
static inline uint16_t gsu_ldw(uint16_t a)
{
    uint8_t lo;

    gsu.ramaddr = a;
    lo = gsu_rd8(gsu_ram(a));
    return (uint16_t)(lo | gsu_rd8(gsu_ram(a ^ 1)) << 8);
}

/* LDB. */
static inline uint8_t gsu_ldb(uint16_t a)
{
    gsu.ramaddr = a;
    return gsu_rd8(gsu_ram(a));
}

/* STW, SM, SMS: the low byte to a, the high byte to a ^ 1. */
static inline void gsu_stw(uint16_t a, uint16_t v)
{
    gsu.ramaddr = a;
    gsu_wr8(gsu_ram(a), (uint8_t)v);
    gsu_wr8(gsu_ram(a ^ 1), (uint8_t)(v >> 8));
}

/* STB. */
static inline void gsu_stb(uint16_t a, uint8_t v)
{
    gsu.ramaddr = a;
    gsu_wr8(gsu_ram(a), v);
}

/* SBK: store back to the last RAM address used. */
static inline void gsu_sbk(uint16_t v)
{
    gsu_wr8(gsu_ram(gsu.ramaddr), (uint8_t)v);
    gsu_wr8(gsu_ram(gsu.ramaddr ^ 1), (uint8_t)(v >> 8));
}

/* Every write of R14 (even of the value it holds) refills the ROM buffer
   from ROMBR:R14 as they are then; ROMB alone does not. C that writes R14
   does it through gsu_r14, so the buffer follows as on the hardware. */
static inline void gsu_r14(uint16_t v)
{
    gsu.r[14] = v;
    gsu.romdr = gsu_rd8((uint32_t)gsu.rombr << 16 | v);
    gsu_buffer(GSU_B_R14);
}

/* GETB, GETC: the ROM buffer. */
static inline uint8_t gsu_getb(void)
{
    gsu_buffer(GSU_B_ROMBUF);
    return gsu.romdr;
}

/* ---- flags, as the instructions that set them leave them ---- */

/* S and Z from a 16-bit result (logic, shifts, INC, DEC, loads that set flags). */
static inline void gsu_sz(uint16_t v)
{
    gsu.s = (uint8_t)(v >> 15);
    gsu.z = v == 0;
}

/* MOVES: OV from bit 7, S and Z. */
static inline void gsu_moves(uint16_t v)
{
    gsu.ov = (uint8_t)(v >> 7 & 1);
    gsu_sz(v);
}

/* ADD and ADC: the sum, with OV, S, CY and Z. */
static inline uint16_t gsu_add(uint16_t a, uint16_t b, int carry)
{
    uint32_t r = (uint32_t)a + b + (carry ? 1 : 0);

    gsu.ov = (uint8_t)((~(a ^ b) & (b ^ r) & 0x8000) != 0);
    gsu.cy = r >= 0x10000;
    gsu_sz((uint16_t)r);
    return (uint16_t)r;
}

/* SUB, SBC and CMP: the difference, with OV, S, CY (no borrow) and Z. */
static inline uint16_t gsu_sub(uint16_t a, uint16_t b, int borrow)
{
    int32_t r = (int32_t)a - b - (borrow ? 1 : 0);

    gsu.ov = (uint8_t)(((a ^ b) & (a ^ (uint32_t)r) & 0x8000) != 0);
    gsu.cy = r >= 0;
    gsu_sz((uint16_t)r);
    return (uint16_t)r;
}

/* LMULT: the signed 32-bit product of a and b (R4 takes the low word, the
   destination the high word); FMULT keeps only the high word. Flags from
   the high word, CY from bit 15 of the product. */
static inline uint32_t gsu_lmult(uint16_t a, uint16_t b)
{
    uint32_t p = (uint32_t)((int32_t)(int16_t)a * (int16_t)b);

    gsu_time(GSU_T_FMULT);
    gsu.cy = (uint8_t)(p >> 15 & 1);
    gsu_sz((uint16_t)(p >> 16));
    return p;
}

#endif
