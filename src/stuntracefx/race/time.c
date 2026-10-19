/*
 * time.c: the race time, kept in binary-coded decimal; $0E70 is a 16-bit
 * BCD value the course tables add to.
 */
#include "machine.h"
#include "cpu_ops.h"

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

void abi_CourseIndex12(void);

/* ASL A, 8-bit: C from bit 7. */
static uint8_t asl8(uint8_t v)
{
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 0x80 ? P_C : 0));
    v = (uint8_t)(v << 1);
    cpu.p = nz8(cpu.p, v);
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
 * X 8-bit; C, V as the path left them.
 */
void abi_RaceTimeInit(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;
    unsigned i;

    op_sep(P_M | P_X);
    for (i = 0; i < 4; i++)
        { cyc(24); wr8(b | (uint16_t)(0x0E68 + i), 0x99); }
    cyc(24);
    v = rd8(b | 0x0D2B);
    cpu.p = nz8(cpu.p, v);
    if (v == 0) {
        cyc(46);
        v = rd8(b | 0x0D4C);
        cpu.p = nz8(cpu.p, v);
        if (v == 0) {
            cyc(40);
            cpu.a = (uint16_t)((cpu.a & 0xFF00) | rd8(b | 0x0D89));
            abi_CourseIndex12();
            cyc(56);
            v = rd8((uint32_t)(0x03D813 + cpu.x));
        } else {
            cyc(46);
            cpu.x = rd8(b | 0x0D4A);
            v = rd8((uint32_t)(0x03D810 + cpu.x));
        }
    } else {
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x02)) & ~P_C) | (v >= 0x02 ? P_C : 0));
        if (v >= 0x02)
            v = 0x00;
        else { cyc(72); if (rd8(b | 0x0D4E) != 0)
            v = 0x00;
        else {
            cyc(40);
            v = rd8(b | 0x0E3D);
            cpu.p |= P_C;
            v = op_sbc8(v, 0x0C, &cpu.p);
            cyc(80);
            v = asl8(asl8(v));
            cpu.p &= (uint8_t)~P_C;
            v = op_adc8(v, rd8(b | 0x0D89), &cpu.p);
            cpu.x = v;
            cyc(6);
            v = rd8((uint32_t)(0x03D8A3 + cpu.x));
        } }
    cyc(8);
        }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    cyc(86);
    wr8(b | 0x0E70, v);
    cyc(24);
    wr8(b | 0x0E71, 0x00);
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
void abi_RaceClockStep(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;
    uint16_t a;

    op_sep(P_M | P_X);
    cyc(54);
    v = rd8(0x000E5B);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (!v) {
        cyc(2);
        op_plp(p0);
        return;
    }
    cpu.p = (uint8_t)((cpu.p & ~P_M) | P_D);
    cyc(82);
    a = rd16(b | 0x0E62);
    cpu.p &= (uint8_t)~P_C;
    cyc(38);
    a = op_adc16(a, rd16(b | 0x0D29), &cpu.p);
    cyc(24);
    wr16(b | 0x0E62, a);
    cyc(24);
    a = op_adc16(rd16(b | 0x0E64), 0x0000, &cpu.p);
    cyc(48);
    wr16(b | 0x0E64, a);
    cpu.a = a;
    op_cmp16(0x0060);
    if (a >= 0x0060) {
        cyc(60);
        wr16(b | 0x0E64, 0x0000);
        cpu.p &= (uint8_t)~P_C;
        cyc(24);
        a = op_adc16(rd16(b | 0x0E66), 0x0001, &cpu.p);
        cyc(62);
        wr16(b | 0x0E66, a);
        cpu.a = a;
        op_cmp16(0x0010);
        if (a >= 0x0010) {
            cyc(84);
            wr16(b | 0x0E66, 0x0009);
            cyc(48);
            wr16(b | 0x0E64, 0x0059);
            cyc(48);
            wr16(b | 0x0E62, 0x9999);
            cpu.a = 0x9999;
            cpu.p = nz16(cpu.p, cpu.a);
        }
    } else { cyc(42); }
    cyc(38);
    a = rd16(b | 0x0E68);
    cpu.p |= P_C;
    cyc(38);
    a = op_sbc16(a, rd16(b | 0x0D29), &cpu.p);
    cpu.a = a;
    cyc(40);
    wr16(b | 0x0E68, a);
    if (!(cpu.p & P_C)) {
        cyc(24);
        a = rd16(b | 0x0E70);
        cpu.a = a;
        cpu.p = nz16(cpu.p, a);
        if (a) {
            cpu.p |= P_C;
            cpu.a = op_sbc16(a, 0x0001, &cpu.p);
            cyc(72);
            wr16(b | 0x0E70, cpu.a);
        } else {
            cyc(46);
            wr16(b | 0x0E68, 0x0000);
            cyc(24);
            wr16(b | 0x0E70, 0x0000);
        }
    cyc(22);
        }
    op_plp(p0);
}
