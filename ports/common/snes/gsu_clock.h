/*
 * gsu_clock.h: the Super FX's time, as the GSU-1 spends it.
 *
 * Every clock the Super FX spends is one of these (a clock: one of the
 * 21.47 MHz master clock's; at CLSR's 10.7 MHz each count below but
 * FMULT's is 6 for 5 and 2 for 1, FMULT's doubles):
 *
 *   fetching a code byte: 1 from the code cache; a line of the cache that
 *     does not hold its code yet is filled first, 16 bytes at 5 each; a
 *     byte outside the cache's 512 (CBR on) costs 5, after the ROM buffer
 *     has finished;
 *   MULT and UMULT 1 more, FMULT and LMULT 3 more (7 without CFGR's MS0);
 *   waiting for the ROM buffer: writing R14 starts a 5-clock fill, which
 *     GETB, GETC, ROMB and a fetch from ROM wait for;
 *   waiting for the RAM buffer: a store takes 5 clocks to be made, which
 *     the next load, store, RAMB or fetch from RAM waits for;
 *   a pixel-cache flush, 5 a byte (bpp bytes), 5 more for each when the
 *     row is not all plotted (it reads the byte first); RPIX flushes both
 *     caches, then reads bpp bytes at 5;
 *   waiting for the bus while the S-CPU has it (SCMR's RON, RAN clear):
 *     6 at a time until it is given back (the runtime's, through `bus`).
 *
 * The Super FX's C says which of the original's code bytes it stands for
 * (gsu.h's gsu_code) and makes the accesses; these price them as the
 * hardware does, in the order the C makes them.
 */
#ifndef GSU_CLOCK_H
#define GSU_CLOCK_H

#include <stdint.h>

/* what a step is for (as core patch 0007 tags the original's) */
enum { GSUCLK_INSN, GSUCLK_ROMBUF, GSUCLK_RAMBUF, GSUCLK_BUS, GSUCLK_FILL, GSUCLK_FLUSH, GSUCLK_RPIX, GSUCLK_KINDS };

typedef struct GsuClock GsuClock;
struct GsuClock {
    uint64_t now;                 /* clocks spent */
    uint64_t by[GSUCLK_KINDS];    /* of them, by what they were for */
    uint32_t valid;               /* the code cache's lines that hold their code */
    uint16_t cbr;                 /* the cache's base */
    uint16_t cbr_next;            /* a CACHE's new base, from the next fetch on */
    uint8_t cache_pending;
    uint8_t clsr, ms0;            /* CLSR (21 MHz); CFGR's MS0 (the fast multiplier) */
    uint8_t romcl, ramcl;         /* clocks until the ROM buffer's fill, the RAM buffer's store, are made */
    /* the pixel caches: the row of 8 pixels each holds, which of them are
       plotted, their colours (bit 7 - x's low bits) */
    struct GsuPixels { uint16_t offset; uint8_t bitpend; uint8_t data[8]; } pix[2];
    /* A bus access the Super FX is about to make (ram: game pak RAM, else
       ROM): the runtime steps GSUCLK_BUS while the S-CPU has that bus.
       NULL: never waits. */
    void (*bus)(GsuClock *c, int ram);
    /* the pixel caches' own accesses to game pak RAM (a flush's, RPIX's) */
    uint8_t (*pix_read)(GsuClock *c, uint32_t addr);
    void (*pix_write)(GsuClock *c, uint32_t addr, uint8_t v);
};

/* The screen as PLOT and RPIX lay it out: ht the height (POR's OBJ: 3, the
   sprite layout), md SCMR's colour depth (2, 4, 4, 8 bits), scbr its base
   in 1 KB. */
typedef struct { uint8_t ht, md, scbr; } GsuScreen;

void gsuclk_step(GsuClock *c, int kind, unsigned clocks);
void gsuclk_code(GsuClock *c, uint32_t addr, unsigned n);   /* code bytes fetched: bank:addr on */
void gsuclk_cache(GsuClock *c, uint16_t r15);              /* CACHE fetched (R15 one past it): it runs after the next fetch */
void gsuclk_flush_code(GsuClock *c, uint16_t cbr);         /* the code cache emptied (PBR, GO cleared) */
void gsuclk_mult(GsuClock *c);                             /* MULT, UMULT */
void gsuclk_fmult(GsuClock *c);                            /* FMULT, LMULT */
void gsuclk_r14(GsuClock *c);                              /* R14 written: the ROM buffer fills */
void gsuclk_rombuf(GsuClock *c);                           /* GETB, GETC, ROMB: the fill made */
void gsuclk_ram_read(GsuClock *c);                         /* a byte loaded */
void gsuclk_ram_write(GsuClock *c);                        /* a byte stored */
void gsuclk_rambuf(GsuClock *c);                           /* RAMB: the store made */
/* PLOT of a pixel that is plotted (not transparent), its colour as COLR
   and POR's dither make it; RPIX. */
void gsuclk_plot(GsuClock *c, const GsuScreen *s, uint8_t x, uint8_t y, uint8_t color);
uint8_t gsuclk_rpix(GsuClock *c, const GsuScreen *s, uint8_t x, uint8_t y);

#endif
