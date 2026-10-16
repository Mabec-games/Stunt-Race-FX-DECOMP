/*
 * lap.c: lap splits. The race clock ($0E63 hundredths, $0E64 seconds, $0E66
 * minutes, all binary-coded decimal) is marked at each lap in $0EDB-$0EDD;
 * a lap's time is the clock minus the mark.
 */
#include "machine.h"
#include "cpu_ops.h"

/* An 8-bit SBC in decimal mode, as the 65816 does it: C is the decimal
   no-borrow, V from the binary sum of the top digit before its adjustment. */
static uint8_t sbc_bcd8(uint8_t a, uint8_t m, uint8_t *p)
{
    unsigned y = (uint8_t)~m, c = *p & P_C ? 1 : 0, r;

    r = (a & 0x0F) + (y & 0x0F) + c;
    if (r <= 0x0F) r -= 0x06;
    c = r > 0x0F;
    r = (a & 0xF0) + (y & 0xF0) + (c << 4) + (r & 0x0F);
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ y) & (a ^ r) & 0x80) ? P_V : 0));
    if (r <= 0xFF) r -= 0x60;
    if (r > 0xFF) *p |= P_C;
    return (uint8_t)r;
}

/* An 8-bit ADC in decimal mode, as the 65816 does it. */
static uint8_t adc_bcd8(uint8_t a, uint8_t m, uint8_t *p)
{
    unsigned c = *p & P_C ? 1 : 0, r;

    r = (a & 0x0F) + (m & 0x0F) + c;
    if (r > 0x09) r += 0x06;
    c = r > 0x0F;
    r = (a & 0xF0) + (m & 0xF0) + (c << 4) + (r & 0x0F);
    *p = (uint8_t)((*p & ~(P_V | P_C)) | ((~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0));
    if (r > 0x9F) r += 0x60;
    if (r > 0xFF) *p |= P_C;
    return (uint8_t)r;
}

/*
 * @routine 07:D5C7 LapSplit
 * WHAT IT DOES: Records a lap: the clock minus the last mark goes into the
 * three bytes at $0EDE + X (hundredths, seconds, minutes), the clock itself
 * into $0EF0 + X, the mark moves to the clock, and the clock's digits are
 * spread one per byte into $0F69 (minutes) and $0F6A-$0F6D (seconds tens and
 * units, hundredths tens and units).
 * HOW: Decimal-mode subtraction byte by byte; a seconds borrow adds 60 and
 * takes a minute from the copy of the minutes in direct page $F1. The
 * clock's bytes are read again for the copies, as the asm does.
 * Entry: M 8-bit, X 8-bit, D = $0000; X = the lap's record offset. Exit: A
 * = the minutes; N, Z from them; C = bit 3 of the seconds; V from the
 * minutes subtraction; D clear; X, Y unchanged.
 */
uint8_t LapSplit(uint8_t bank, uint8_t x, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t frac, secs, mins, v;

    mins = rd8(b | 0x0E66);
    wr8(0x0000F1, mins);
    frac = rd8(b | 0x0E63);
    *p |= P_C;
    v = sbc_bcd8(frac, rd8(b | 0x0EDB), p);
    wr8(b | (uint16_t)(0x0EDE + x), v);
    secs = rd8(b | 0x0E64);
    v = sbc_bcd8(secs, rd8(b | 0x0EDC), p);
    if (!(*p & P_C)) {
        *p &= (uint8_t)~P_C;
        v = adc_bcd8(v, 0x60, p);
        wr8(0x0000F1, (uint8_t)(rd8(0x0000F1) - 1));
    }
    wr8(b | (uint16_t)(0x0EDF + x), v);
    *p |= P_C;
    v = sbc_bcd8(rd8(0x0000F1), rd8(b | 0x0EDD), p);
    wr8(b | (uint16_t)(0x0EE0 + x), v);

    frac = rd8(b | 0x0E63);
    wr8(b | 0x0EDB, frac);
    wr8(b | (uint16_t)(0x0EF0 + x), frac);
    wr8(b | 0x0F6D, frac & 0x0F);
    wr8(b | 0x0F6C, frac >> 4);
    secs = rd8(b | 0x0E64);
    wr8(b | 0x0EDC, secs);
    wr8(b | (uint16_t)(0x0EF1 + x), secs);
    wr8(b | 0x0F6B, secs & 0x0F);
    wr8(b | 0x0F6A, secs >> 4);
    mins = rd8(b | 0x0E66);
    wr8(b | 0x0EDD, mins);
    wr8(b | (uint16_t)(0x0EF2 + x), mins);
    wr8(b | 0x0F69, mins);

    *p = (uint8_t)((nz8(*p, mins) & ~(P_C | P_D)) | (secs & 0x08 ? P_C : 0));
    return mins;
}

void abi_LapSplit(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | LapSplit(cpu.db, (uint8_t)cpu.x, &p));
    cpu.p = p;
}

/*
 * @routine 07:D5B2 RaceTimeKeep
 * WHAT IT DOES: Copies the race time (fraction $0E63, seconds $0E64,
 * minutes $0E66) to $0EE7-$0EE9.
 * HOW: Three 8-bit loads and stores through the data bank.
 * Entry: any widths. Exit: M 8-bit; A low byte = the minutes; N, Z from
 * them.
 */
void abi_RaceTimeKeep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_sep(P_M);
    op_lda8(b | 0x0E63);
    wr8(b | 0x0EE7, (uint8_t)cpu.a);
    op_lda8(b | 0x0E64);
    wr8(b | 0x0EE8, (uint8_t)cpu.a);
    op_lda8(b | 0x0E66);
    wr8(b | 0x0EE9, (uint8_t)cpu.a);
}
