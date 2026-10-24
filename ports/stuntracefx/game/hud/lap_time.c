/*
 * lap_time.c: race times as the HUD and the records show them. Times are
 * kept packed in a word: hundredths in bits 0-6, seconds in bits 7-12,
 * minutes in bits 13-15.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hud_calls.h"
#include "carried.h"
#include "wram_names.h"


/* The 65816 instructions these routines run, on a value and the flags in
   *p (a routine's P, kept for its exit and for the decimal adds). */

/* CMP: N, Z, C from a - m. */
static void cmp16(uint16_t a, uint16_t m, uint8_t *p)
{
    cyc(0);
    *p = (uint8_t)((nz16(*p, (uint16_t)(a - m)) & ~P_C) | (a >= m ? P_C : 0));
}

/* AND #: the time of an immediate AND is 2 more clocks. */
static uint16_t and16(uint16_t a, uint16_t m, uint8_t *p)
{
    cyc(2);
    a &= m;
    *p = nz16(*p, a);
    return a;
}

/* LDA # (16-bit). */
static uint16_t lda16_imm(uint16_t v, uint8_t *p)
{
    cyc(38);
    *p = nz16(*p, v);
    return v;
}

/* TAX / TXA with 16-bit registers: the value, N and Z from it. */
static uint16_t transfer30(uint16_t v, uint8_t *p)
{
    cyc(30);
    *p = nz16(*p, v);
    return v;
}

/* TAY, or a load of A from a register: the value, N and Z from it. */
static uint16_t transfer(uint16_t v, uint8_t *p)
{
    *p = nz16(*p, v);
    return v;
}

/* CLC / ADC (16-bit). */
static uint16_t add16(uint16_t a, uint16_t m, uint8_t *p)
{
    *p &= (uint8_t)~P_C;
    return alu_adc16(a, m, p);
}

/* ASL A (16-bit), C the bit out. */
static uint16_t asl16(uint16_t a, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (a >> 15));
    return (uint16_t)(a << 1);
}

/* ASL A with N and Z from the result. */
static uint16_t asl16nz(uint16_t a, uint8_t *p)
{
    a = asl16(a, p);
    *p = nz16(*p, a);
    return a;
}

/* ASL A (8-bit): C the bit out. */
static uint8_t asl8(uint8_t a, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (a >> 7));
    return (uint8_t)(a << 1);
}

/* XBA: N and Z from the new low byte. */
static uint16_t xba(uint16_t a, uint8_t *p)
{
    a = (uint16_t)(a << 8 | a >> 8);
    *p = nz8(*p, (uint8_t)a);
    return a;
}

/* LSR A (16-bit). */
static uint16_t lsr16(uint16_t a, uint8_t *p)
{
    *p = (uint8_t)((*p & ~P_C) | (a & 1));
    a >>= 1;
    *p = nz16(*p, a);
    return a;
}

/* ROL A (16-bit) through the carry. */
static uint16_t rol16(uint16_t a, uint8_t *p)
{
    uint8_t c = (uint8_t)(a >> 15);

    a = (uint16_t)(a << 1 | (*p & P_C));
    *p = (uint8_t)(nz16((uint8_t)(*p & ~P_C), a) | c);
    return a;
}

/* ROR A (16-bit) through the carry. */
static uint16_t ror16(uint16_t a, uint8_t *p)
{
    uint8_t c = (uint8_t)(a & 1);

    a = (uint16_t)(a >> 1 | (*p & P_C) << 15);
    *p = (uint8_t)(nz16((uint8_t)(*p & ~P_C), a) | c);
    return a;
}

/*
 * @routine 07:ACD9 BinaryToBcd
 * WHAT IT DOES: Converts A (below 10000) to binary-coded decimal.
 * HOW: Counts thousands, hundreds and tens by repeated subtraction into
 * direct page $F1 ($1000, $100, $10 per step), then adds the units. Not
 * reached in the traces so far.
 * Entry: M 16-bit, D = $0000; A = the value. Exit: A = the BCD value; $F1
 * = A less its units digit; X, Y unchanged; N, Z, C, V from the last add.
 * The function takes A and returns the BCD value; *p is P (flags in and out).
 */
uint16_t BinaryToBcd(uint16_t v, uint8_t *p)
{
    static const uint16_t unit[3] = {1000, 100, 10}, digit[3] = {0x1000, 0x0100, 0x0010};
    uint16_t saved;
    unsigned i;

    cyc(16);
    wr16(W(bcd_digits), 0x0000);
    for (i = 0; i < 3; i++) {
        for (;;) {
            cmp16(v, unit[i], p);
            if (!(*p & P_C)) {
                cyc(46);                /* CMP, BCC taken */
                break;
            }
            v = alu_sbc16(v, unit[i], p);
            saved = v;                  /* PHA, the count, PLA */
            cyc(104);
            v = rd16(W(bcd_digits));
            *p = nz16(*p, v);
            v = add16(v, digit[i], p);
            cyc(54);
            wr16(W(bcd_digits), v);
            v = lda16_imm(saved, p);
            cyc(20);                    /* BRA */
        }
    }
    cyc(30);
    v = add16(v, rd16(W(bcd_digits)), p);
    cyc(44);
    return v;
}

void BcdToBinary_cr(Carried *cr);
void BinaryToBcd_cr(Carried *cr);
void RecordNameStore0_cr(Carried *cr);
void RecordNameStore3_cr(Carried *cr);
void RecordNameStore6_cr(Carried *cr);
void RecordShiftFrom0_cr(Carried *cr);
void RecordShiftFrom1_cr(Carried *cr);
void RecordSlotCopy0_cr(Carried *cr);
void RecordSlotCopy3_cr(Carried *cr);
void RecordTimeInsert_cr(Carried *cr);
void TimeAddToNext_cr(Carried *cr);
void TimeAddToTotal_cr(Carried *cr);
void TimeBelowRecord_cr(Carried *cr);
void TimePackBcd_cr(Carried *cr);
void TimeRunningTotals_cr(Carried *cr);
void TimeTotalsSum_cr(Carried *cr);
void TimeUnpackBcd_cr(Carried *cr);
void TimeUnpackBcdLong_cr(Carried *cr);

void BinaryToBcd_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = BinaryToBcd(cr->a, &p);
    set_p(cr, p);
}

void abi_BinaryToBcd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BinaryToBcd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AC46 TimeUnpackBcd
 * WHAT IT DOES: Unpacks a packed time word into BCD: A = the minutes (bits
 * 13-15), X = the seconds (bits 7-12), Y = the hundredths (bits 0-6); $FFFF
 * (no time) gives $FFFF in all three.
 * HOW: Masks and shifts, each part through BinaryToBcd. Only the $FFFF
 * case is reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the packed time. Exit: A, X, Y
 * as above; $F1 as BinaryToBcd leaves it; N, Z, C, V from its last add, or
 * N set, Z and C set by the compare for $FFFF.
 * The function takes A and returns *min (A), *sec (X), *hund (Y); *p is P.
 */
void TimeUnpackBcd(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund, uint8_t *p)
{
    uint16_t a = packed, v;

    cmp16(a, 0xFFFF, p);
    if (*p & P_Z) {
        cyc(78);
        a = lda16_imm(0xFFFF, p);
        *min = *sec = *hund = a;
        cyc(44);
        return;
    } else { cyc(108); }
    *sec = transfer30(a, p);                    /* TAX */
    a = and16(a, 0x007F, p);
    a = BinaryToBcd(a, p);
    *hund = transfer(a, p);                     /* TAY */
    a = transfer30(*sec, p);                    /* TXA */
    v = a;                                      /* PHA */
    a = asl16nz(a, p);
    a = xba(a, p);
    a = and16(a, 0x003F, p);
    cyc(146);
    a = BinaryToBcd(a, p);
    *sec = transfer30(a, p);                    /* TAX */
    a = lda16_imm(v, p);                        /* PLA */
    a = rol16(a, p);
    a = rol16(a, p);
    a = rol16(a, p);
    a = rol16(a, p);
    a = and16(a, 0x0007, p);
    cyc(122);
    *min = BinaryToBcd(a, p);
    cyc(42);
}

void TimeUnpackBcd_cr(Carried *cr)
{
    uint16_t min, sec, hund;
    uint8_t p = cr->p;

    TimeUnpackBcd(cr->a, &min, &sec, &hund, &p);
    cr->a = min;
    cr->x = sec;
    cr->y = hund;
    set_p(cr, p);
}

void abi_TimeUnpackBcd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUnpackBcd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AC42 TimeUnpackBcdLong
 * WHAT IT DOES: TimeUnpackBcd for a JSL caller.
 * HOW: JSR TimeUnpackBcd, RTL. Not reached in the traces so far.
 * Entry: as TimeUnpackBcd. Exit: as TimeUnpackBcd.
 * As TimeUnpackBcd.
 */
void TimeUnpackBcdLong(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund, uint8_t *p)
{
    cyc(46);
    TimeUnpackBcd(packed, min, sec, hund, p);
    cyc(44);                            /* RTL */
}

void TimeUnpackBcdLong_cr(Carried *cr)
{
    uint16_t min, sec, hund;
    uint8_t p = cr->p;

    TimeUnpackBcdLong(cr->a, &min, &sec, &hund, &p);
    cr->a = min;
    cr->x = sec;
    cr->y = hund;
    set_p(cr, p);
}

void abi_TimeUnpackBcdLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUnpackBcdLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Adds the BCD time at $0F76 + X (hundredths, seconds, minutes) to the
   time at `to` (three bytes), with the decimal carries the original makes:
   hundredths into seconds, seconds past 59 (once on the add's carry, once
   on 60 or more) into minutes. A $FF hundredths byte means no time. The
   decimal flag is as the caller set it. dp: direct page
   operands (TimeAddToTotal), else absolute,X (TimeAddToNext). Returns the
   last byte (A's low byte on exit). */
static uint8_t time_add(uint32_t b, uint16_t x, uint32_t to[3], int dp, uint8_t *p)
{
    uint8_t v;
    int big;
    uint16_t xw = (*p & P_X) ? 0 : 6;       /* a read absolute,X with X 16-bit: one more cycle */
    uint16_t kr = dp ? 16 : 24 + xw;        /* ADC/LDA of the sum's byte */
    uint16_t kw = dp ? 16 : 30;             /* STA absolute,X: the extra cycle always */

    cyc(24 + xw);
    v = rd8(b | (uint16_t)(0x0F76 + x));
    *p = (uint8_t)((nz8(*p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? (cyc(80), P_C) : 0));
    if (v == 0xFF)
        return v;
    *p &= (uint8_t)~P_C;
    cyc(46 + kr);                       /* CMP, BEQ, CLC, ADC's fetch */
    v = alu_adc8(v, rd8(to[0]), p);
    cyc(kw);
    wr8(to[0], v);
    cyc(24 + xw);
    v = rd8(b | (uint16_t)(0x0F77 + x));
    cyc(kr);
    v = alu_adc8(v, rd8(to[1]), p);
    if (*p & P_C) {
        *p |= P_C;
        v = alu_sbc8(v, 0x60, p);
        cyc(30 + kw);
        wr8(to[1], v);
        cyc(kr);
        v = rd8(to[2]);
        *p &= (uint8_t)~P_C;
        cyc(30 + kw);
        wr8(to[2], alu_adc8(v, 0x01, p));
        cyc(kr);
        v = rd8(to[1]);
        cyc(kw);
    } else { cyc(22 + kw); /* BCC taken */ }
    wr8(to[1], v);
    *p = (uint8_t)((nz8(*p, (uint8_t)(v - 0x60)) & ~P_C) | (v >= 0x60 ? P_C : 0));
    big = v >= 0x60;
    if (big) {
        v = alu_sbc8(v, 0x60, p);
        cyc(32 + kw);
        wr8(to[1], v);
        cyc(kr);
        v = rd8(to[2]);
        *p &= (uint8_t)~P_C;
        cyc(30 + kw);
        wr8(to[2], alu_adc8(v, 0x01, p));
    }
    *p &= (uint8_t)~P_C;
    cyc((big ? 38 : 76) + xw);
    v = rd8(b | (uint16_t)(0x0F78 + x));
    cyc(kr);
    v = alu_adc8(v, rd8(to[2]), p);
    cyc(kw);
    wr8(to[2], v);
    cyc(42);
    return v;
}

/*
 * @routine 07:B75B TimeAddToTotal
 * WHAT IT DOES: Adds record time X (BCD hundredths, seconds, minutes at
 * $0F76 + X) to the total in direct page $F1-$F3, unless it is no time
 * ($FF).
 * HOW: Decimal adds (the caller sets D); seconds past 59 carry into the
 * minutes. Not reached in the traces so far.
 * Entry: M 8-bit, D = $0000, decimal mode; X = the time's offset. Exit: A
 * low byte = the new minutes, N, Z, C, V from their add; or, for no time,
 * $FF with Z and C set.
 * The function takes the bank and X and returns the new minutes (A low byte);
 * *p is P.
 */
uint8_t TimeAddToTotal(uint8_t bank, uint16_t x, uint8_t *p)
{
    uint32_t to[3] = {0x0000F1, W(dp_f2), W(dp_f3)};

    return time_add((uint32_t)bank << 16, x, to, 1, p);
}

void TimeAddToTotal_cr(Carried *cr)
{
    uint8_t p = cr->p, v = TimeAddToTotal(cr->db, cr->x, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, p);
}

void abi_TimeAddToTotal(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeAddToTotal_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B796 TimeAddToNext
 * WHAT IT DOES: Adds record time X to the time after it ($0F79 + X),
 * unless it is no time ($FF).
 * HOW: As TimeAddToTotal. Not reached in the traces so far.
 * Entry: as TimeAddToTotal. Exit: as TimeAddToTotal.
 * As TimeAddToTotal.
 */
uint8_t TimeAddToNext(uint8_t bank, uint16_t x, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint32_t to[3] = {b | (uint16_t)(0x0F79 + x), b | (uint16_t)(0x0F7A + x), b | (uint16_t)(0x0F7B + x)};

    return time_add(b, x, to, 0, p);
}

void TimeAddToNext_cr(Carried *cr)
{
    uint8_t p = cr->p, v = TimeAddToNext(cr->db, cr->x, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, p);
}

void abi_TimeAddToNext(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeAddToNext_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B67E TimeTotalsSum
 * WHAT IT DOES: Sums each group of four record times ($0F76, $0F82, $0F8E)
 * into its total ($0F9A, $0F9D, $0FA0); a group whose first time has no
 * minutes ($FF) totals $FF:$FF:$FF.
 * HOW: Decimal mode, $F1-$F3 as the running sum, TimeAddToTotal per time.
 * Not reached in the traces so far.
 * Entry: D = $0000. Exit: A low byte = the last total's minutes; X = $21;
 * P restored.
 * The function takes the bank and the caller's P and returns the last total's
 * minutes (A low byte); the adapter sets X = $21 and restores P.
 */
uint8_t TimeTotalsSum(uint8_t bank, uint8_t p0)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t p = p0 | P_M | P_X | P_D, v = 0;
    unsigned g, i;

    cyc(22);                            /* SEP #$30 */
    cyc(36);
    for (g = 0; g < 3; g++) {
        cyc(16);
        wr8(W(bcd_digits), 0);
        cyc(16);
        wr8(W(dp_f2), 0);
        cyc(16);
        wr8(W(dp_f3), 0);
        for (i = 0; i < 4; i++) {
            cyc(62);
            TimeAddToTotal(bank, (uint16_t)(g * 12 + i * 3), &p);
        }
        cyc(24);
        v = rd8(b | (uint16_t)(0x0F78 + g * 12));
        p = (uint8_t)((nz8(p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? P_C : 0));
        if (v == 0xFF) {
            cyc(48);
            wr8(W(bcd_digits), v);
            cyc(16);
            wr8(W(dp_f2), v);
            cyc(16);
            wr8(W(dp_f3), v);
        }
        for (i = 0; i < 3; i++) {
            cyc(16);
            v = rd8(W(bcd_digits) + i);
            cyc(24);
            wr8(b | (uint16_t)(0x0F9A + g * 3 + i), v);
        }
    }
    cyc(84);
    return v;
}

void TimeTotalsSum_cr(Carried *cr)
{
    uint8_t p0 = cr->p, v;

    set_p(cr, p0 | P_M | P_X | P_D);       /* what an interrupt pushes */
    v = TimeTotalsSum(cr->db, p0);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = 0x21;
    cr->y &= 0x00FF;
    set_p(cr, p0);
}

void abi_TimeTotalsSum(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeTotalsSum_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B727 TimeRunningTotals
 * WHAT IT DOES: Turns each group of four record times into running totals:
 * the first three are added into the time after each.
 * HOW: Decimal mode, TimeAddToNext for times 0-2, 4-6 and 8-10. Not
 * reached in the traces so far.
 * Entry: D = $0000. Exit: X = $1E; P restored; A as TimeAddToNext left it.
 * The function takes the bank and the caller's P and returns A's low byte;
 * the adapter sets X = $1E and restores P.
 */
uint8_t TimeRunningTotals(uint8_t bank, uint8_t p0)
{
    uint8_t p, v = 0;
    unsigned g, i;

    cyc(36);
    p = p0 | P_M | P_X | P_D;
    cyc(22);                            /* SEP #$30 */
    for (g = 0; g < 3; g++)
        for (i = 0; i < 3; i++) {
            cyc(62);
            v = TimeAddToNext(bank, (uint16_t)(g * 12 + i * 3), &p);
        }
    cyc(84);
    return v;
}

void TimeRunningTotals_cr(Carried *cr)
{
    uint8_t p0 = cr->p, v;

    set_p(cr, p0 | P_M | P_X | P_D);       /* what an interrupt pushes */
    v = TimeRunningTotals(cr->db, p0);
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = 0x1E;
    cr->y &= 0x00FF;
    set_p(cr, p0);
}

void abi_TimeRunningTotals(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeRunningTotals_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The X a record routine leaves (its offset, advanced by its copy loop),
   for the adapters. */
static uint8_t exit_x;

/* LDA $0D4A, ASL x3, CLC, ADC $0D4A (8-bit): the course times nine. */
static uint8_t course_times_9(uint8_t bank, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a;
    int i;

    cyc(24);
    a = rd8(b | W(wram_0D4A));
    for (i = 0; i < 3; i++)
        a = asl8(a, p);
    *p &= (uint8_t)~P_C;
    cyc(80);
    return alu_adc8(a, rd8(b | W(wram_0D4A)), p);
}

/* TAX, LDY #3, then three bytes from $70:FC6D + X to $70:FC70 + X. Returns
   the last byte; exit_x = X. */
static uint8_t record_slot_copy(uint8_t x, uint8_t *p)
{
    uint8_t v = 0, y = 3;

    do {
        cyc(78);
        v = rd8(0x70FC6D + (uint32_t)x);
        cyc(32);
        wr8(0x70FC70 + (uint32_t)x, v);
        x = (uint8_t)(x + 1);
        y = (uint8_t)(y - 1);
    } while (cyc_loop(4, y));
    cyc(86);
    *p = nz8(*p, 0);
    exit_x = x;
    return v;
}

/*
 * @routine 07:B93C RecordSlotCopy0
 * WHAT IT DOES: Copies the course $0D4A's first three-byte record entry
 * ($70:FC6D + course * 9) to the second.
 * HOW: The offset as course * 8 + course, then the copy loop of
 * RecordSlotCopy3 (its code from the TAX).
 * Entry: M 8-bit, X 8-bit. Exit: A = the last byte copied; X = the offset
 * + 3; Y = 0; Z set, N clear; C, V from the add.
 * The function returns the last byte copied (A low byte); *p is P; the
 * adapter sets X = the offset + 3, Y = 0.
 */
uint8_t RecordSlotCopy0(uint8_t bank, uint8_t *p)
{
    uint8_t a = course_times_9(bank, p);

    cyc(2);
    cyc(2);
    cyc(2);
    return record_slot_copy(a, p);
}

void RecordSlotCopy0_cr(Carried *cr)
{
    uint8_t p = cr->p, v = RecordSlotCopy0(cr->db, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = exit_x;
    cr->y = 0;
    set_p(cr, p);
}

void abi_RecordSlotCopy0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordSlotCopy0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B948 RecordSlotCopy3
 * WHAT IT DOES: Copies the course $0D4A's second three-byte record entry
 * ($70:FC70 + course * 9) to the third.
 * HOW: As RecordSlotCopy0 with 3 added (the carry of the first add in).
 * Entry: M 8-bit, X 8-bit. Exit: as RecordSlotCopy0.
 * As RecordSlotCopy0.
 */
uint8_t RecordSlotCopy3(uint8_t bank, uint8_t *p)
{
    uint8_t a = course_times_9(bank, p);

    return record_slot_copy(alu_adc8(a, 0x03, p), p);
}

void RecordSlotCopy3_cr(Carried *cr)
{
    uint8_t p = cr->p, v = RecordSlotCopy3(cr->db, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = exit_x;
    cr->y = 0;
    set_p(cr, p);
}

void abi_RecordSlotCopy3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordSlotCopy3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* LDA $0D4A, ASL, CLC, ADC $0D4A, ASL x3 (8-bit): the course times 24. */
static uint8_t course_times_24(uint8_t bank, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a;
    int i;

    cyc(24);
    a = rd8(b | W(wram_0D4A));
    a = asl8(a, p);
    *p &= (uint8_t)~P_C;
    cyc(52);
    a = alu_adc8(a, rd8(b | W(wram_0D4A)), p);
    cyc(2);
    for (i = 0; cyc_loop(12, i < 3); i++)
        a = asl8(a, p);
    *p = nz8(*p, a);
    return a;
}

/* TAX, LDY #8, then eight bytes from $70:FC88 + X to $70:FC90 + X. Returns
   the last byte; exit_x = X. */
static uint8_t times_slot_copy(uint8_t x, uint8_t *p)
{
    uint8_t v = 0, y = 8;

    do {
        cyc(82);
        v = rd8(0x70FC88 + (uint32_t)x);
        cyc(32);
        wr8(0x70FC90 + (uint32_t)x, v);
        x = (uint8_t)(x + 1);
        y = (uint8_t)(y - 1);
    } while (y);
    cyc(86);
    *p = nz8(*p, 0);
    exit_x = x;
    return v;
}

/*
 * @routine 07:B90C RecordShiftFrom0
 * WHAT IT DOES: Moves the course $0D4A's first record down a place: its
 * three-byte entry (RecordSlotCopy0) and its eight-byte times at $70:FC88
 * + course * 24 to the next eight.
 * HOW: The offset as (course * 2 + course) * 8, then the copy loop of
 * RecordShiftFrom1 (its code from the TAX).
 * Entry: M 8-bit, X 8-bit. Exit: A = the last byte copied; X = the offset
 * + 8; Y = 0; Z set, N clear; C from the last shift.
 * The function returns the last byte copied (A low byte); *p is P; the
 * adapter sets X = the offset + 8, Y = 0.
 */
uint8_t RecordShiftFrom0(uint8_t bank, uint8_t *p)
{
    uint8_t a;

    cyc(46);
    RecordSlotCopy0(bank, p);
    a = course_times_24(bank, p);
    cyc(6);
    return times_slot_copy(a, p);
}

void RecordShiftFrom0_cr(Carried *cr)
{
    uint8_t p = cr->p, v = RecordShiftFrom0(cr->db, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = exit_x;
    cr->y = 0;
    set_p(cr, p);
}

void abi_RecordShiftFrom0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordShiftFrom0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B91C RecordShiftFrom1
 * WHAT IT DOES: Moves the course $0D4A's second record down a place: its
 * entry (RecordSlotCopy3) and its times at $70:FC90 + course * 24 to the
 * next eight.
 * HOW: As RecordShiftFrom0 with 8 added (and the last shift's carry).
 * Entry: M 8-bit, X 8-bit. Exit: as RecordShiftFrom0, C from the add.
 * As RecordShiftFrom0.
 */
uint8_t RecordShiftFrom1(uint8_t bank, uint8_t *p)
{
    uint8_t a;

    cyc(46);
    RecordSlotCopy3(bank, p);
    a = course_times_24(bank, p);
    return times_slot_copy(alu_adc8(a, 0x08, p), p);
}

void RecordShiftFrom1_cr(Carried *cr)
{
    uint8_t p = cr->p, v = RecordShiftFrom1(cr->db, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = exit_x;
    cr->y = 0;
    set_p(cr, p);
}

void abi_RecordShiftFrom1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordShiftFrom1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* STA $F5, then the record entry at $70:FC6D + course * 9 + $F5 gets the
   name bytes $0F11, $0F12 and the car $0D89. Returns the car (A's low
   byte on exit); exit_x = the entry's offset. */
static uint8_t record_name_store(uint8_t bank, uint8_t slot, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a, x, v;
    int i;

    *p = nz8(*p, slot);
    cyc(32);
    wr8(W(dp_f5), slot);
    cyc(24);
    a = rd8(b | W(wram_0D4A));
    for (i = 0; i < 3; i++)
        a = asl8(a, p);
    *p &= (uint8_t)~P_C;
    cyc(80);
    a = alu_adc8(a, rd8(b | W(wram_0D4A)), p);
    cyc(16);
    a = alu_adc8(a, rd8(W(dp_f5)), p);
    x = a;
    *p = nz8(*p, a);
    cyc(38);
    v = rd8(b | W(wram_0F11));
    *p = nz8(*p, v);
    cyc(32);
    wr8(0x70FC6D + (uint32_t)x, v);
    cyc(24);
    v = rd8(b | W(wram_0F12));
    *p = nz8(*p, v);
    cyc(32);
    wr8(0x70FC6E + (uint32_t)x, v);
    cyc(24);
    v = rd8(b | W(wram_0D89));
    *p = nz8(*p, v);
    cyc(32);
    wr8(0x70FC6F + (uint32_t)x, v);
    cyc(42);
    exit_x = x;
    return v;
}

/*
 * @routine 07:B8DD RecordNameStore0
 * WHAT IT DOES: Writes the entered name ($0F11, $0F12) and car ($0D89)
 * into the course $0D4A's first three-byte record entry ($70:FC6D +
 * course * 9).
 * HOW: The entry's offset (0, 3 or 6 from the three entries) through
 * direct page $F5; 8-bit long stores.
 * Entry: M 8-bit, X 8-bit, D = $0000. Exit: A = the car; X = the entry's
 * offset; N, Z from the car.
 * The function returns the car (A low byte); *p is P; the adapter sets X =
 * the entry's offset.
 */
uint8_t RecordNameStore0(uint8_t bank, uint8_t *p)
{
    cyc(22);
    return record_name_store(bank, 0x00, p);
}

void RecordNameStore0_cr(Carried *cr)
{
    uint8_t p = cr->p, v = RecordNameStore0(cr->db, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = exit_x;
    set_p(cr, p);
}

void abi_RecordNameStore0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordNameStore0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B8E1 RecordNameStore3
 * WHAT IT DOES: As RecordNameStore0 for the second entry.
 * HOW: The same code with 3; no script reaches it (unit_cpu does).
 * Entry: as RecordNameStore0. Exit: as RecordNameStore0.
 * As RecordNameStore0.
 */
uint8_t RecordNameStore3(uint8_t bank, uint8_t *p)
{
    cyc(22);
    return record_name_store(bank, 0x03, p);
}

void RecordNameStore3_cr(Carried *cr)
{
    uint8_t p = cr->p, v = RecordNameStore3(cr->db, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = exit_x;
    set_p(cr, p);
}

void abi_RecordNameStore3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordNameStore3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B8E5 RecordNameStore6
 * WHAT IT DOES: As RecordNameStore0 for the third entry.
 * HOW: The same code with 6; no script reaches it (unit_cpu does).
 * Entry: as RecordNameStore0. Exit: as RecordNameStore0.
 * As RecordNameStore0.
 */
uint8_t RecordNameStore6(uint8_t bank, uint8_t *p)
{
    return record_name_store(bank, 0x06, p);
}

void RecordNameStore6_cr(Carried *cr)
{
    uint8_t p = cr->p, v = RecordNameStore6(cr->db, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = exit_x;
    set_p(cr, p);
}

void abi_RecordNameStore6(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordNameStore6_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The last byte TimeBelowRecord compared (A's low byte on exit), for the
   adapter. */
static uint8_t below_a;

/*
 * @routine 07:B964 TimeBelowRecord
 * WHAT IT DOES: Compares the time $0EEF/$0EEE/$0EED (most significant
 * first) with the record time at $0F81 + X: carry clear when it is
 * lower; when a byte is higher it returns at once with carry set.
 * HOW: 8-bit CMP per byte, BCC lower, BEQ on to the next.
 * Entry: M 8-bit; X = the record's offset. Exit: A = the last byte
 * compared; N, Z, C from that compare.
 * The function takes the bank and X and returns 1 when the time is lower
 * (carry clear); *p is P; the adapter sets A low byte = the last byte
 * compared.
 */
int TimeBelowRecord(uint8_t bank, uint16_t x, uint8_t *p)
{
    static const uint16_t mine[3] = {0x0EEF, 0x0EEE, 0x0EED}, rec[3] = {0x0F81, 0x0F80, 0x0F7F};
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a = 0, m;
    int i;

    for (i = 0; i < 3; i++) {
        cyc(24);
        a = rd8(b | mine[i]);
        *p = nz8(*p, a);
        cyc(24);
        m = rd8(b | (uint16_t)(rec[i] + x));
        *p = (uint8_t)((nz8(*p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(38), P_C) : (cyc(28), 0)));
        if (a != m)
            { cyc(36); below_a = a; return !(*p & P_C); }
    }
    cyc(4);
    below_a = a;
    return !(*p & P_C);
}

void TimeBelowRecord_cr(Carried *cr)
{
    uint8_t p = cr->p;

    TimeBelowRecord(cr->db, cr->x, &p);
    cr->a = (uint16_t)((cr->a & 0xFF00) | below_a);
    set_p(cr, p);
}

void abi_TimeBelowRecord(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeBelowRecord_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AC77 BcdToBinary
 * WHAT IT DOES: Converts the four BCD digits in A to binary.
 * HOW: For each of the thousands, hundreds and tens digits (when A is at
 * least $1000, $100, $10), adds 1000, 100 or 10 to $F1 that many times
 * and masks the digit off; then adds the units. X kept on the stack. The
 * thousands and hundreds run only from unit_cpu.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the BCD value. Exit: A = the
 * value; $F1 = A less the units; X unchanged, N and Z from it; C and V from
 * the last addition.
 * The function takes A and returns the value; *p is P with N, Z from the last
 * add (the adapter sets them from X).
 */
uint16_t BcdToBinary(uint16_t v, uint8_t *p)
{
    static const uint16_t thr[3] = {0x1000, 0x0100, 0x0010};
    static const uint16_t mul[3] = {1000, 100, 10};
    static const uint8_t sh[3] = {12, 8, 4};
    uint16_t n, t, pre = 30;
    int i;

    cyc(16);
    wr16(W(bcd_digits), 0x0000);
    for (i = 0; i < 3; i++) {
        if (v < thr[i])
            { pre += 46; continue; }   /* CMP, BCC taken */
        n = (uint16_t)(v >> sh[i]);
        cyc(pre + 100 + 14 * sh[i]);
        t = rd16(W(bcd_digits));
        do
            t = (uint16_t)(t + mul[i]);
        while (cyc_loop(74, --n));
        cyc(84);
        wr16(W(bcd_digits), t);
        v &= (uint16_t)(thr[i] - 1);
        pre = 60;                       /* PLA, AND */
    }
    cyc(pre + 30);
    v = add16(v, rd16(W(bcd_digits)), p);
    cyc(80);
    return v;
}

void BcdToBinary_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = BcdToBinary(cr->a, &p);
    set_p(cr, nz16(p, cr->x));
}

void abi_BcdToBinary(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    BcdToBinary_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:AC13 TimePackBcd
 * WHAT IT DOES: Packs a time given in BCD into one binary word: the
 * hundredths (A's low byte) in bits 0-6, the seconds (X's low byte) in
 * bits 7-12, the minutes (Y's low byte) in bits 13-15.
 * HOW: BcdToBinary on each; the parts gathered at $F3 (XBA and LSR for the
 * seconds, four RORs for the minutes).
 * Entry: M 16-bit, X 16-bit, D = $0000. Exit: A = the packed time, $F3 =
 * its bits 0-12; N, Z from A; C = bit 3 of the minutes; X and Y unchanged.
 * The function takes the hundredths (A), seconds (X) and minutes (Y) and
 * returns the packed time; *p is P.
 */
uint16_t TimePackBcd(uint16_t hund, uint16_t sec, uint16_t min, uint8_t *p)
{
    uint16_t a;
    int i;

    a = and16(hund, 0x00FF, p);
    cyc(84);
    a = BcdToBinary(a, p);
    a = and16(a, 0x007F, p);
    cyc(38);
    wr16(W(dp_f3), a);
    a = transfer(sec, p);
    a = and16(a, 0x00FF, p);
    cyc(98);
    a = BcdToBinary(a, p);
    a = xba(a, p);
    a = lsr16(a, p);
    a = and16(a, 0x1F80, p);
    cyc(72);
    a |= rd16(W(dp_f3));
    *p = nz16(*p, a);
    cyc(16);
    wr16(W(dp_f3), a);
    a = transfer(min, p);
    a = and16(a, 0x00FF, p);
    cyc(98);
    a = BcdToBinary(a, p);
    for (i = 0; i < 4; i++)
        a = ror16(a, p);
    a = and16(a, 0xE000, p);
    cyc(94);
    a |= rd16(W(dp_f3));
    *p = nz16(*p, a);
    cyc(42);
    return a;
}

void TimePackBcd_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = TimePackBcd(cr->a, cr->x, cr->y, &p);
    set_p(cr, p);
}

void abi_TimePackBcd(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimePackBcd_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* CLC, LDA.L, ADC $F1, STA $F1 over n bytes from addr + x (8-bit X).
   Returns the last sum (A's low byte on exit). */
static uint8_t sum_into_f1(uint32_t addr, uint8_t x, unsigned n, uint8_t *p)
{
    uint8_t a = 0;
    unsigned first = 1;

    while (n--) {
        *p &= (uint8_t)~P_C;
        /* the first round comes from TAX, LDY, CLC; later ones from INX, DEY, BNE, CLC */
        cyc(first ? 76 : 96);
        first = 0;
        a = rd8(addr + x);
        cyc(16);
        a = alu_adc8(a, rd8(W(bcd_digits)), p);
        cyc(16);
        wr8(W(bcd_digits), a);
        x++;
    }
    return a;
}

/*
 * @routine 07:B7DE RecordTimeInsert
 * WHAT IT DOES: Unless $0D24 is 3, places the race time ($0EED-$0EEF) in
 * the course $0D4A's records: below the first record (TimeBelowRecord) it
 * becomes the first, the first two moving down (RecordShiftFrom1, then
 * From0, RecordNameStore0; place 1 in $0FAC, page 0); below the second it
 * becomes the second (RecordShiftFrom1, RecordNameStore3; place 2, page
 * 8); below the third, the third (RecordNameStore6; place 3, page $10);
 * otherwise $10C7 = $FF and $0FAC = 0, and nothing else. Then its four lap
 * times ($0EF9 + 3n: hundredths, seconds, minutes) go packed (TimePackBcd)
 * to $70:FC88 + course * 24 + page + 2n, and the course's checksum at
 * $70:FC2D + course is remade: the sum of its 24 time bytes and its nine
 * name bytes at $70:FC6D + course * 9.
 * HOW: P pushed; 8-bit compares, the offset at $F7 (16-bit, with the
 * carry of its last shift), 16-bit loads for TimePackBcd, then the 8-bit
 * sums at $F1. Only the first place and the end are reached in the traces
 * so far.
 * Entry: any widths; D = $0000. Exit: P restored; A low byte the checksum
 * (high byte the last packed time's), X = course, Y = 0; or, at $0D24 = 3,
 * A low byte 3; with no record, A low byte 0 and X = $18.
 * The function takes A and the caller's P, with X and Y in *x and *y, and
 * returns A; *x, *y are X and Y on exit.
 */
uint16_t RecordTimeInsert(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t p0)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t p = p0 | P_M | P_X, place, page, v, sum;
    uint16_t w, xx, yy, aa;
    unsigned i;

    cyc(22);                            /* SEP #$30 */
    *x &= 0x00FF;
    *y &= 0x00FF;
    cyc(46);
    v = rd8(b | W(wram_0D24));
    p = nz8(p, v);
    a = (uint16_t)((a & 0xFF00) | v);
    if (v == 0x03) {
        cyc(32);
        cyc(42);                        /* PLP */
        cyc(28);
        return a;
    } else { cyc(122); }
    if (TimeBelowRecord(bank, 0x00, &p)) {
        cyc(62);
        RecordShiftFrom1(bank, &p);
        cyc(46);
        RecordShiftFrom0(bank, &p);
        cyc(46);
        RecordNameStore0(bank, &p);
        place = 0x01;
        page = 0x00;
    } else {
        cyc(84);
        if (TimeBelowRecord(bank, 0x0C, &p)) {
            cyc(62);                    /* BCS not taken, JSR */
            RecordShiftFrom1(bank, &p);
            cyc(46);
            RecordNameStore3(bank, &p);
            place = 0x02;
            page = 0x08;
        } else {
            cyc(84);
            if (!TimeBelowRecord(bank, 0x18, &p)) {
                cyc(56);
                wr8(b | W(player_tag), 0xFF);
                cyc(40);
                wr8(b | W(wram_0FAC), 0x00);
                cyc(42);                /* PLP */
                cyc(28);
                *x = 0x18;
                return (uint16_t)(a & 0xFF00);
            } else { cyc(68); }         /* BCC taken, JSR */
            RecordNameStore6(bank, &p);
            place = 0x03;
            page = 0x10;
        }
    }
    cyc(40);
    wr8(b | W(wram_0FAC), place);
    p &= (uint8_t)~(P_M | P_X);         /* REP #$30 */
    cyc(22);
    cyc(place == 0x03 ? 56 : 78);       /* the third place has no BRA */
    wr16(W(dp_f7), page);
    cyc(24);
    w = asl16(rd16(b | W(wram_0D4A)), &p);
    p &= (uint8_t)~P_C;
    cyc(52);
    w = (uint16_t)(alu_adc16(w, rd16(b | W(wram_0D4A)), &p) & 0x00FF);
    for (i = 0; i < 3; i++)
        w = asl16(w, &p);
    cyc(82);
    w = alu_adc16(w, rd16(W(dp_f7)), &p);
    cyc(16);
    wr16(W(dp_f7), w);
    aa = a;
    for (i = 0; i < 4; i++) {
        cyc(24);
        aa = rd16(b | (uint16_t)(0x0EF9 + 3 * i));
        cyc(24);
        xx = rd16(b | (uint16_t)(0x0EFA + 3 * i));
        cyc(24);
        yy = rd16(b | (uint16_t)(0x0EFB + 3 * i));
        cyc(46);
        aa = TimePackBcd(aa, xx, yy, &p);
        cyc(16);
        xx = rd16(W(dp_f7));
        cyc(32);
        wr16(0x70FC88 + xx + 2 * i, aa);
    }
    p |= P_M | P_X;                     /* SEP #$30 */
    cyc(22);
    cyc(16);
    wr8(W(bcd_digits), 0x00);
    v = course_times_24(bank, &p);
    cyc(4);
    sum_into_f1(0x70FC88, v, 24, &p);
    cyc(44);
    v = course_times_9(bank, &p);
    sum = sum_into_f1(0x70FC6D, v, 9, &p);
    cyc(68);
    xx = rd8(b | W(wram_0D4A));
    cyc(16);
    v = rd8(W(bcd_digits));
    cyc(32);
    wr8(0x70FC2D + xx, v);
    cyc(42);                            /* PLP */
    cyc(28);
    *x = xx;
    *y = 0;
    return (uint16_t)((aa & 0xFF00) | sum);
}

void RecordTimeInsert_cr(Carried *cr)
{
    uint8_t p0 = cr->p;
    uint16_t x = cr->x, y = cr->y;

    set_p(cr, p0 | P_M | P_X);             /* what an interrupt pushes */
    cr->a = RecordTimeInsert(cr->db, cr->a, &x, &y, p0);
    cr->x = x;
    cr->y = y;
    set_p(cr, p0);
}

void abi_RecordTimeInsert(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTimeInsert_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

