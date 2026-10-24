/*
 * lap.c: lap splits. The race clock ($0E63 hundredths, $0E64 seconds, $0E66
 * minutes, all binary-coded decimal) is marked at each lap in $0EDB-$0EDD;
 * a lap's time is the clock minus the mark.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "race_calls.h"
#include "carried.h"
#include "wram_names.h"

/* An 8-bit SBC in decimal mode, as the 65816 does it: C is the decimal
   no-borrow, V from the binary sum of the top digit before its adjustment. */
static uint8_t sbc_bcd8(uint8_t a, uint8_t m, uint8_t *p)
{
    unsigned y = (uint8_t)~m, c = *p & P_C ? 1 : 0, r;

    r = (a & 0x0F) + (y & 0x0F) + c;
    if (r <= 0x0F) r -= 0x06;
    cyc(22);
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

    cyc(38);
    mins = rd8(b | W(race_time_minutes));
    cyc(16);
    wr8(0x0000F1, mins);
    cyc(24);
    frac = rd8(b | W(race_time_fraction));
    *p |= P_C;
    cyc(38);
    v = sbc_bcd8(frac, rd8(b | W(lap_mark)), p);
    cyc(8);
    wr8(b | (uint16_t)(0x0EDE + x), v);
    cyc(24);
    secs = rd8(b | W(race_time_seconds));
    cyc(24);
    v = sbc_bcd8(secs, rd8(b | W(wram_0EDC)), p);
    if (!(*p & P_C)) {
        *p &= (uint8_t)~P_C;
        v = adc_bcd8(v, 0x60, p);
        cyc(40);
        wr8(0x0000F1, cyc_then(6, (uint8_t)(rd8(0x0000F1) - 1)));
    }
    cyc(30);
    wr8(b | (uint16_t)(0x0EDF + x), v);
    *p |= P_C;
    cyc(16);
    v = rd8(0x0000F1);                          /* LDA, then SBC */
    cyc(38);
    v = sbc_bcd8(v, rd8(b | W(wram_0EDD)), p);
    cyc(8);
    wr8(b | (uint16_t)(0x0EE0 + x), v);

    cyc(24);
    frac = rd8(b | W(race_time_fraction));
    cyc(24);
    wr8(b | W(lap_mark), frac);
    cyc(30);
    wr8(b | (uint16_t)(0x0EF0 + x), frac);
    cyc(62);
    wr8(b | W(wram_0F6D), frac & 0x0F);
    cyc(108);
    wr8(b | W(wram_0F6C), frac >> 4);
    cyc(24);
    secs = rd8(b | W(race_time_seconds));
    cyc(24);
    wr8(b | W(wram_0EDC), secs);
    cyc(30);
    wr8(b | (uint16_t)(0x0EF1 + x), secs);
    cyc(62);
    wr8(b | W(wram_0F6B), secs & 0x0F);
    cyc(108);
    wr8(b | W(wram_0F6A), secs >> 4);
    cyc(24);
    mins = rd8(b | W(race_time_minutes));
    cyc(24);
    wr8(b | W(wram_0EDD), mins);
    cyc(30);
    wr8(b | (uint16_t)(0x0EF2 + x), mins);
    cyc(24);
    wr8(b | W(lap_digits), mins);

    *p = (uint8_t)((nz8(*p, mins) & ~(P_C | P_D)) | (secs & 0x08 ? (cyc(56), P_C) : (cyc(56), 0)));
    return mins;
}

/* LapSplit on carried registers. */
void LapSplit_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | LapSplit(cr->db, (uint8_t)cr->x, &p));
    set_p(cr, p);
}

void abi_LapSplit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    LapSplit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D5B2 RaceTimeKeep
 * WHAT IT DOES: Copies the race time (fraction $0E63, seconds $0E64,
 * minutes $0E66) to $0EE7-$0EE9.
 * HOW: Three 8-bit loads and stores through the data bank.
 * Entry: any widths; the data bank. Exit: returns the minutes; the adapter
 * sets M 8-bit, A low byte = the minutes, N, Z from them.
 */
uint8_t RaceTimeKeep(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;

    cyc(46);
    v = rd8(b | W(race_time_fraction));
    cyc(24);
    wr8(b | W(wram_0EE7), v);
    cyc(24);
    v = rd8(b | W(race_time_seconds));
    cyc(24);
    wr8(b | W(wram_0EE8), v);
    cyc(24);
    v = rd8(b | W(race_time_minutes));
    cyc(24);
    wr8(b | W(wram_0EE9), v);
    cyc(42);
    return v;
}

/* RaceTimeKeep on carried registers. */
void RaceTimeKeep_cr(Carried *cr)
{
    uint8_t v;

    set_p(cr, (uint8_t)(cr->p | (P_M)));                               /* what an interrupt pushes */
    v = RaceTimeKeep(cr->db);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, nz8(cr->p, v));
}

void abi_RaceTimeKeep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceTimeKeep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
