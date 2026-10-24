/*
 * time.c: the race time, kept in binary-coded decimal; $0E70 is a 16-bit
 * BCD value the course tables add to.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "race_calls.h"
#include "carried.h"
#include "wram_names.h"


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

/* $0E70 += the BCD byte of `table` for the entry at $0D89 of this course.
   `bra` is the clocks of the BRA into the shared tail (0 for the copy that
   falls into it). */
static uint16_t add_course_value(uint32_t table, uint8_t bra, uint16_t *x, uint8_t *p)
{
    cyc(24);
    uint8_t i = CourseIndex12(rd8(W(wram_0D89)), p);
    uint16_t v;

    *x = i;
    cyc(138 + bra);                     /* LDA long, [BRA,] REP, SED, AND #, CLC, ADC */
    v = adc_bcd16(rd8(table + i), rd16(W(race_time_bcd)), p);
    cyc(24);
    wr16(W(race_time_bcd), v);
    cyc(80);
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
    return add_course_value(0x03D815, 0, x, p);
}

void RaceClockStep_cr(Carried *cr);
void RaceTimeAddD814_cr(Carried *cr);
void RaceTimeAddD815_cr(Carried *cr);
void RaceTimeInit_cr(Carried *cr);

void RaceTimeAddD815_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = RaceTimeAddD815(&cr->x, &p);
    set_p(cr, (uint8_t)((p & ~P_D) | P_M));
}

void abi_RaceTimeAddD815(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceTimeAddD815_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:EF93 RaceTimeAddD814
 * WHAT IT DOES: As RaceTimeAddD815, from the table at $03:D814.
 * HOW: As RaceTimeAddD815, sharing its tail.
 * Entry: as RaceTimeAddD815. Exit: as RaceTimeAddD815.
 */
uint16_t RaceTimeAddD814(uint16_t *x, uint8_t *p)
{
    return add_course_value(0x03D814, 22, x, p);
}

void RaceTimeAddD814_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = RaceTimeAddD814(&cr->x, &p);
    set_p(cr, (uint8_t)((p & ~P_D) | P_M));
}

void abi_RaceTimeAddD814(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceTimeAddD814_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* ASL A, 8-bit: C from bit 7, N and Z from the result, in *p. */
static uint8_t asl8(uint8_t v, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (v & 0x80 ? P_C : 0));
    v = (uint8_t)(v << 1);
    *p = nz8(*p, v);
    return v;
}

/*
 * @routine 03:EF3F RaceTimeInit
 * WHAT IT DOES: Sets the four bytes at $0E68 to $99 and starts the BCD race
 * time at $0E70 (word) by the mode in $0D2B: mode 0 takes the course's
 * entry $0D89 of the table at $03:D813 (CourseIndex12), or with $0D4C set
 * byte $0D4A of the table at $03:D810; mode 1 (unless $0D4E is set) takes
 * entry ($0E3D - $0C) * 4 + $0D89 of the table at $03:D8A3; otherwise 0.
 * HOW: 8-bit throughout; the high byte is cleared with STZ. The $0D4C
 * path is not reached in the traces so far.
 * Entry: any widths, D = $0000. Exit: A low byte = the value, N, Z from
 * it; X as the path left it (8-bit); X and Y high bytes cleared; M 8-bit,
 * X 8-bit; C, V as the path left them. (The function returns the value;
 * *x and *p carry X and P.)
 */
uint8_t RaceTimeInit(uint8_t bank, uint8_t *x, uint8_t *p)
{
    (void)bank;
    uint8_t v;
    unsigned i;

    *p |= P_M | P_X;
    cyc(22);
    cyc(16);
    for (i = 0; i < 4; i++)
        { cyc(24); wr8(0x7E0000 | (uint16_t)(W(countdown_fraction) + i), 0x99); }
    cyc(24);
    v = rd8(0x7E0000 | W(hud_mode));
    *p = nz8(*p, v);
    if (v == 0) {
        cyc(46);
        v = rd8(0x7E0000 | W(digit62_override));
        *p = nz8(*p, v);
        if (v == 0) {
            cyc(40);
            v = rd8(0x7E0000 | W(wram_0D89));
            *x = CourseIndex12(v, p);
            cyc(0);
            v = rd8((uint32_t)(0x03D813 + *x));
        } else {
            cyc(46);
            *x = rd8(0x7E0000 | W(wram_0D4A));
            v = rd8((uint32_t)(0x03D810 + *x));
        }
    } else {
        cyc(8);
        *p = (uint8_t)((nz8(*p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
        if (v >= 0x02)
            v = 0x00;
        else { cyc(64); if (rd8(0x7E0000 | W(hud_blank)) != 0)
            v = 0x00;
        else {
            cyc(40);
            v = rd8(0x7E0000 | W(course));
            *p |= P_C;
            v = alu_sbc8(v, 0x0C, p);
            v = asl8(asl8(v, p), p);
            *p &= (uint8_t)~P_C;
            cyc(80);
            v = alu_adc8(v, rd8(0x7E0000 | W(wram_0D89)), p);
            *x = v;
            cyc(14);
            v = rd8((uint32_t)(0x03D8A3 + *x));
        } }
    }
    *p = nz8(*p, v);
    cyc(86);
    wr8(0x7E0000 | W(race_time_bcd), v);
    cyc(24);
    wr8(0x7E0000 | W(wram_0E71), 0x00);
    cyc(42);
    return v;
}

void RaceTimeInit_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;
    uint8_t v;

    set_p(cr, p | P_M | P_X);                      /* what an interrupt pushes */
    v = RaceTimeInit(cr->db, &x, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = x;
    cr->y &= 0x00FF;
    set_p(cr, p);
}

void abi_RaceTimeInit(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceTimeInit_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 7E:B072 RaceClockStep
 * WHAT IT DOES: While $0E5B is set, adds the step $0D29 to the race time
 * (BCD: $0E62 the fraction, $0E64 seconds, $0E66 minutes; seconds carry
 * at 60, and from 10 minutes it holds at 9:59.9999) and takes it off the
 * countdown $0E68, borrowing from $0E70; when $0E70 is already 0 the
 * countdown stops at 0.
 * HOW: 16-bit decimal adds and subtracts (SED); the compares are binary.
 * The 10 minute hold is not reached in the traces so far.
 * Entry: any widths. Exit: X and Y high bytes cleared (SEP #$30); A as
 * the last step left it; P restored (decimal off).
 */
uint16_t RaceClockStep(uint8_t bank, uint16_t acc, uint8_t p0)
{
    (void)bank;
    uint8_t p = p0 | P_M | P_X, v;
    uint16_t a;

    cyc(22);                                    /* SEP #$30 */
    cyc(54);
    v = rd8(W(wram_0E5B));
    acc = (uint16_t)((acc & 0xFF00) | v);
    if (!v) {
        cyc(44);
        cyc(42);                                /* PLP */
        return acc;
    }
    p = (uint8_t)((p & ~P_M) | P_D);
    cyc(82);
    a = rd16(0x7E0000 | W(wram_0E62));
    p &= (uint8_t)~P_C;
    cyc(38);
    a = alu_adc16(a, rd16(0x7E0000 | W(wram_0D29)), &p);
    cyc(24);
    wr16(0x7E0000 | W(wram_0E62), a);
    cyc(24);
    a = alu_adc16(rd16(0x7E0000 | W(race_time_seconds)), 0x0000, &p);
    cyc(48);
    wr16(0x7E0000 | W(race_time_seconds), a);
    acc = a;
    cyc(0);                                     /* CMP #$0060 */
    if (a >= 0x0060) {
        cyc(64);
        wr16(0x7E0000 | W(race_time_seconds), 0x0000);
        p &= (uint8_t)~P_C;
        cyc(24);
        a = alu_adc16(rd16(0x7E0000 | W(race_time_minutes)), 0x0001, &p);
        cyc(62);
        wr16(0x7E0000 | W(race_time_minutes), a);
        acc = a;
        cyc(0);                                 /* CMP #$0010 */
        if (a >= 0x0010) {
            cyc(88);
            wr16(0x7E0000 | W(race_time_minutes), 0x0009);
            cyc(48);
            wr16(0x7E0000 | W(race_time_seconds), 0x0059);
            cyc(48);
            wr16(0x7E0000 | W(wram_0E62), 0x9999);
            acc = 0x9999;
        }
    } else { cyc(46); }
    cyc(38);
    a = rd16(0x7E0000 | W(countdown_fraction));
    p |= P_C;
    cyc(38);
    a = alu_sbc16(a, rd16(0x7E0000 | W(wram_0D29)), &p);
    acc = a;
    cyc(40);
    wr16(0x7E0000 | W(countdown_fraction), a);
    if (!(p & P_C)) {
        cyc(24);
        a = rd16(0x7E0000 | W(race_time_bcd));
        acc = a;
        if (a) {
            p |= P_C;
            acc = alu_sbc16(a, 0x0001, &p);
            cyc(72);
            wr16(0x7E0000 | W(race_time_bcd), acc);
        } else {
            cyc(46);
            wr16(0x7E0000 | W(countdown_fraction), 0x0000);
            cyc(24);
            wr16(0x7E0000 | W(race_time_bcd), 0x0000);
        }
        cyc(22);
    }
    cyc(42);
    cyc(42);                                    /* PLP */
    return acc;
}

void RaceClockStep_cr(Carried *cr)
{
    uint8_t p0 = cr->p;

    cr->a = RaceClockStep(cr->db, cr->a, p0);
    set_p(cr, p0);
    cr->x &= 0x00FF;
    cr->y &= 0x00FF;
}

void abi_RaceClockStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RaceClockStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

