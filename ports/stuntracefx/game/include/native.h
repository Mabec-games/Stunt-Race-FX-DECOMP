/*
 * native.h: what the port gives the game's C in place of the console's
 * parts.
 *
 * Where the original reads or writes a console register and the port's C
 * now does that part's work itself (a multiply, a picture update, a sound
 * command), the access's time stays: io_rd8/io_wr8 spend the time the
 * access took on the original, and a read is a place an interrupt can come,
 * as at any read. A read returns the value the C worked out. So the game
 * keeps the original's pace (its frames take the video frames they took).
 */
#ifndef STUNTRACEFX_NATIVE_H
#define STUNTRACEFX_NATIVE_H

#include <stdint.h>

#include "machine.h"
#include "native.h"

/* The time of a register access the C does the work of (the port's clock). */
void io_read_time(uint32_t addr, uint8_t value);
void io_write_time(uint32_t addr, uint8_t value);

static inline uint8_t io_rd8(uint32_t addr, uint8_t v)
{
    io_read_time(addr, v);
    return v;
}

static inline uint16_t io_rd16(uint32_t addr, uint16_t v)
{
    io_read_time(addr, (uint8_t)v);
    io_read_time((addr + 1) & 0xFFFFFF, (uint8_t)(v >> 8));
    return v;
}

static inline void io_wr8(uint32_t addr, uint8_t v)
{
    io_write_time(addr, v);
}

static inline void io_wr16(uint32_t addr, uint16_t v)
{
    io_write_time(addr, (uint8_t)v);
    io_write_time((addr + 1) & 0xFFFFFF, (uint8_t)(v >> 8));
}

/* ---- the picture (ports/common/video) ----
   The screen state's registers ($2100-$2133): each write's value to the
   picture, at the access's time. */
void vid_reg(uint8_t reg, uint8_t v);

static inline int ppu_reg(uint32_t addr)
{
    return !(addr & 0x400000) && (addr & 0xFFFF) >= 0x2100 && (addr & 0xFFFF) <= 0x2133;
}

static inline void ppu_wr(uint32_t addr, uint8_t v)
{
    io_write_time(addr, v);
    vid_reg((uint8_t)addr, v);
}

static inline void ppu_wr16(uint32_t addr, uint16_t v)
{
    ppu_wr(addr, (uint8_t)v);
    ppu_wr((addr + 1) & 0xFFFFFF, (uint8_t)(v >> 8));
}

/* ---- transfers: the copies into the picture's tiles, palette and sprites
   and the per-line tables (the console's DMA, $4300-$437F, MDMAEN and
   HDMAEN): each channel's settings, a copy run on the channels in
   `channels`, the per-line tables on those channels ---- */
void dma_reg(uint8_t reg, uint8_t v);       /* $4300 + reg: channel reg >> 4 */
void dma_go(uint8_t channels);
void hdma_set(uint8_t channels);

static inline int dma_reg_addr(uint32_t addr)
{
    return !(addr & 0x400000) && (addr & 0xFF80) == 0x4300;
}

static inline void dma_wr(uint32_t addr, uint8_t v)
{
    io_write_time(addr, v);
    dma_reg((uint8_t)(addr & 0x7F), v);
}

static inline void dma_wr16(uint32_t addr, uint16_t v)
{
    dma_wr(addr, (uint8_t)v);
    dma_wr((addr + 1) & 0xFFFFFF, (uint8_t)(v >> 8));
}

static inline void mdmaen_wr(uint32_t addr, uint8_t v)
{
    io_write_time(addr, v);
    dma_go(v);
}

static inline void hdmaen_wr(uint32_t addr, uint8_t v)
{
    io_write_time(addr, v);
    hdma_set(v);
}

/* A write to a register the code computes (a table's, a queue's): to the
   part that register belongs to. */
static inline void reg_wr(uint32_t addr, uint8_t v)
{
    if (ppu_reg(addr))
        ppu_wr(addr, v);
    else if (dma_reg_addr(addr))
        dma_wr(addr, v);
    else if (!(addr & 0x400000) && (addr & 0xFFFF) == 0x420B)
        mdmaen_wr(addr, v);
    else if (!(addr & 0x400000) && (addr & 0xFFFF) == 0x420C)
        hdmaen_wr(addr, v);
    else
        wr8(addr, v);
}

static inline void reg_wr16(uint32_t addr, uint16_t v)
{
    reg_wr(addr, (uint8_t)v);
    reg_wr((addr + 1) & 0xFFFFFF, (uint8_t)(v >> 8));
}

/* ---- the joypads ----
   The buttons the frame's automatic read took (JOY1L, JOY2L: $4218,
   $421A), a bit each, from bit 15 down: B, Y, SELECT, START, up, down,
   left, right, A, X, L, R; bits 3-0 clear. The serial ports ($4016, $4017) answer as a
   plain joypad's: its second data line (bit 1, the Super Multitap's) low. */
uint16_t input_pad(int port);

/* ---- the multiply and divide units, as C ----
   The S-CPU's: 8 x 8 unsigned multiply ($4202, $4203; product $4216), and
   16 / 8 unsigned divide ($4204-$4206; quotient $4214, by 0 all ones). The
   PPU's: a 16-bit signed factor (M7A, $211B) times an 8-bit signed one
   (M7B, $211C), a 24-bit product (MPYL/M/H, $2134). The game never shows
   Mode 7, so M7A and M7B are only this multiplier. */
extern int16_t ppu_mul_factor;   /* M7A's value: math/multiply.c */

static inline uint16_t cpu_mul(uint8_t a, uint8_t b)
{
    return (uint16_t)(a * b);
}

static inline uint16_t cpu_div(uint16_t n, uint8_t d)
{
    return d ? (uint16_t)(n / d) : 0xFFFF;
}

static inline uint32_t ppu_mul(uint8_t b)
{
    return (uint32_t)((int32_t)ppu_mul_factor * (int8_t)b) & 0xFFFFFF;
}

#endif
