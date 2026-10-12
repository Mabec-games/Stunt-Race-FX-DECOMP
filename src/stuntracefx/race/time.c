/*
 * time.c: the race time, kept in binary-coded decimal; $0E70 is a 16-bit
 * BCD value the course tables add to.
 */
#include "machine.h"

uint8_t CourseIndex12(uint8_t a, uint8_t *p);

/* A 16-bit ADC in decimal mode with carry clear, as the 65816 does it: N, Z
   from the result, C the decimal carry, V from the binary sum of the top
   digit before its adjustment. */
static uint16_t adc_bcd16(uint16_t a, uint16_t m, uint8_t *p)
{
    unsigned r, c;

    r = (a & 0x000F) + (m & 0x000F);
    if (r > 0x0009) r += 0x0006;
    c = r > 0x000F;
    r = (a & 0x00F0) + (m & 0x00F0) + (c << 4) + (r & 0x000F);
    if (r > 0x009F) r += 0x0060;
    c = r > 0x00FF;
    r = (a & 0x0F00) + (m & 0x0F00) + (c << 8) + (r & 0x00FF);
    if (r > 0x09FF) r += 0x0600;
    c = r > 0x0FFF;
    r = (a & 0xF000) + (m & 0xF000) + (c << 12) + (r & 0x0FFF);
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    if (r > 0x9FFF) r += 0x6000;
    if (r > 0xFFFF) *p |= P_C;
    *p = nz16(*p, (uint16_t)r);
    return (uint16_t)r;
}

/* $0E70 += the BCD byte of `table` for the entry at $0D89 of this course. */
static uint16_t add_course_value(uint32_t table, uint16_t *x, uint8_t *p)
{
    uint8_t i = CourseIndex12(rd8(0x000D89), p);
    uint16_t v;

    *x = i;
    v = adc_bcd16(rd8(table + i), rd16(0x000E70), p);
    wr16(0x000E70, v);
    return v;
}

/*
 * @routine 03:EF9F RaceTimeAddD815
 * WHAT IT DOES: Adds to the BCD value at $0E70 the BCD byte of the table at
 * $03:D815 for the course and the entry at $0D89 (12 bytes per course,
 * three per entry).
 * HOW: CourseIndex12, then a 16-bit ADC in decimal mode.
 * Entry: M 8-bit, X 8-bit, D = $0000. Exit: A = the new value (M 8-bit); X
 * = the index; $F1 = entry * 3; D clear; N, Z, C, V from the decimal add.
 */
uint16_t RaceTimeAddD815(uint16_t *x, uint8_t *p)
{
    return add_course_value(0x03D815, x, p);
}

void abi_RaceTimeAddD815(void)
{
    uint8_t p = cpu.p;

    cpu.a = RaceTimeAddD815(&cpu.x, &p);
    cpu.p = (uint8_t)((p & ~P_D) | P_M);
}

/*
 * @routine 03:EF93 RaceTimeAddD814
 * WHAT IT DOES: As RaceTimeAddD815, from the table at $03:D814.
 * HOW: As RaceTimeAddD815, sharing its tail.
 * Entry: as RaceTimeAddD815. Exit: as RaceTimeAddD815.
 */
uint16_t RaceTimeAddD814(uint16_t *x, uint8_t *p)
{
    return add_course_value(0x03D814, x, p);
}

void abi_RaceTimeAddD814(void)
{
    uint8_t p = cpu.p;

    cpu.a = RaceTimeAddD814(&cpu.x, &p);
    cpu.p = (uint8_t)((p & ~P_D) | P_M);
}
