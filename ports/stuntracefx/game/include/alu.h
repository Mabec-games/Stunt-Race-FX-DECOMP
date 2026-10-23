/*
 * alu.h: the 65816's ADC and SBC on values, binary or decimal as the D flag
 * in *p says, with N, Z, C and V in *p: for plain C that keeps the flags
 * a routine's callers read. cpu_ops.h's op_ forms are these.
 */
#ifndef STUNTRACEFX_ALU_H
#define STUNTRACEFX_ALU_H

#include "machine.h"
#include "native.h"

/* The 65816's 16-bit ADC and SBC, binary or decimal as the D flag in *p
   says, as the emulator core computes them: the result, with N, Z, C and V
   in *p. */
static inline uint16_t alu_adc16(uint16_t a, uint16_t m, uint8_t *p)
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

static inline uint16_t alu_sbc16(uint16_t a, uint16_t m, uint8_t *p)
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
    if ((*p & P_D) && r <= 0xFFFF) r -= 0x6000; else { cyc(6); }
    if (r > 0xFFFF) *p |= P_C;
    *p = nz16(*p, (uint16_t)r);
    return (uint16_t)r;
}

/* The 65816's 8-bit ADC and SBC, binary or decimal as the D flag in *p
   says, as the emulator core computes them. */
static inline uint8_t alu_adc8(uint8_t a, uint8_t m, uint8_t *p)
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

static inline uint8_t alu_sbc8(uint8_t a, uint8_t m, uint8_t *p)
{
    cyc(16);
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

#endif
