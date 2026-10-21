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
