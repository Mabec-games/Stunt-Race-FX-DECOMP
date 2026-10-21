/*
 * lap_time.c: race times as the HUD and the records show them. Times are
 * kept packed in a word: hundredths in bits 0-6, seconds in bits 7-12,
 * minutes in bits 13-15.
 */
#include "machine.h"
#include "cpu_ops.h"

/*
 * @routine 07:ACD9 BinaryToBcd
 * WHAT IT DOES: Converts A (below 10000) to binary-coded decimal.
 * HOW: Counts thousands, hundreds and tens by repeated subtraction into
 * direct page $F1 ($1000, $100, $10 per step), then adds the units. Not
 * reached in the traces so far.
 * Entry: M 16-bit, D = $0000; A = the value. Exit: A = the BCD value; $F1
 * = A less its units digit; X, Y unchanged; N, Z, C, V from the last add.
 */
uint16_t BinaryToBcd(uint16_t v)
{
    static const uint16_t unit[3] = {1000, 100, 10}, digit[3] = {0x1000, 0x0100, 0x0010};
    unsigned i;

    wr16(0x0000F1, 0x0000);
    for (i = 0; i < 3; i++) {
        while (v >= unit[i]) {
            v = (uint16_t)(v - unit[i]);
            wr16(0x0000F1, (uint16_t)(rd16(0x0000F1) + digit[i]));
        }
    }
    return (uint16_t)(v + rd16(0x0000F1));
}

void abi_BinaryToBcd(void)
{
    static const uint16_t unit[3] = {1000, 100, 10}, digit[3] = {0x1000, 0x0100, 0x0010};
    uint16_t a;
    unsigned i;

    cyc(16);
    wr16(0x0000F1, 0x0000);
    for (i = 0; i < 3; i++) {
        for (;;) {
            op_cmp16(unit[i]);
            if (!(cpu.p & P_C)) {
                cyc(46);                /* CMP, BCC taken */
                break;
            }
            cpu.a = op_sbc16(cpu.a, unit[i], &cpu.p);
            a = cpu.a;                  /* PHA, the count, PLA */
            cyc(104);
            op_lda16(0x0000F1);
            op_add16(digit[i]);
            cyc(54);
            wr16(0x0000F1, cpu.a);
            op_lda16_imm(a);
            cyc(20);                    /* BRA */
        }
    }
    cyc(30);
    op_add16(rd16(0x0000F1));
    cyc(44);
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
 */
void TimeUnpackBcd(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund)
{
    if (packed == 0xFFFF) {
        *min = *sec = *hund = 0xFFFF;
        return;
    }
    *hund = BinaryToBcd(packed & 0x007F);
    *sec = BinaryToBcd((uint16_t)(packed << 1) >> 8 & 0x003F);
    *min = BinaryToBcd(packed >> 13);
}

void abi_TimeUnpackBcd(void)
{
    uint16_t v;

    op_cmp16(0xFFFF);
    if (cpu.p & P_Z) {
        cyc(78);
        op_lda16_imm(0xFFFF);
        cpu.x = cpu.y = 0xFFFF;
        cyc(44);
        return;
    } else { cyc(108); }
    op_tax16();
    op_and16(0x007F);
    abi_BinaryToBcd();
    op_tay16();
    op_txa16();
    v = cpu.a;                          /* PHA */
    op_asl16();
    op_xba();
    op_and16(0x003F);
    (cyc(146), abi_BinaryToBcd());
    op_tax16();
    op_lda16_imm(v);                    /* PLA */
    op_rol16();
    op_rol16();
    op_rol16();
    op_rol16();
    op_and16(0x0007);
    (cyc(122), abi_BinaryToBcd());
    cyc(42);
}

/*
 * @routine 07:AC42 TimeUnpackBcdLong
 * WHAT IT DOES: TimeUnpackBcd for a JSL caller.
 * HOW: JSR TimeUnpackBcd, RTL. Not reached in the traces so far.
 * Entry: as TimeUnpackBcd. Exit: as TimeUnpackBcd.
 */
void abi_TimeUnpackBcdLong(void)
{
    (cyc(46), abi_TimeUnpackBcd());
    cyc(44);                            /* RTL */
}

/* Adds the BCD time at $0F76 + X (hundredths, seconds, minutes) to the
   time at `to` (three bytes), with the decimal carries the original makes:
   hundredths into seconds, seconds past 59 (once on the add's carry, once
   on 60 or more) into minutes. A $FF hundredths byte means no time. The
   decimal flag is as the caller set it. dp: direct page
   operands (TimeAddToTotal), else absolute,X (TimeAddToNext). */
static void time_add(uint32_t b, uint16_t x, uint32_t to[3], int dp)
{
    uint8_t v;
    int big;
    uint16_t xw = (cpu.p & P_X) ? 0 : 6;    /* a read absolute,X with X 16-bit: one more cycle */
    uint16_t kr = dp ? 16 : 24 + xw;        /* ADC/LDA of the sum's byte */
    uint16_t kw = dp ? 16 : 30;             /* STA absolute,X: the extra cycle always */

    cyc(24 + xw);
    v = rd8(b | (uint16_t)(0x0F76 + x));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? (cyc(80), P_C) : 0));
    if (v == 0xFF)
        return;
    cpu.p &= (uint8_t)~P_C;
    cyc(46 + kr);                       /* CMP, BEQ, CLC, ADC's fetch */
    v = op_adc8(v, rd8(to[0]), &cpu.p);
    cyc(kw);
    wr8(to[0], v);
    cyc(24 + xw);
    v = rd8(b | (uint16_t)(0x0F77 + x));
    cyc(kr);
    v = op_adc8(v, rd8(to[1]), &cpu.p);
    if (cpu.p & P_C) {
        cpu.p |= P_C;
        v = op_sbc8(v, 0x60, &cpu.p);
        cyc(30 + kw);
        wr8(to[1], v);
        cyc(kr);
        v = rd8(to[2]);
        cpu.p &= (uint8_t)~P_C;
        cyc(30 + kw);
        wr8(to[2], op_adc8(v, 0x01, &cpu.p));
        cyc(kr);
        v = rd8(to[1]);
        cyc(kw);
    } else { cyc(22 + kw); /* BCC taken */ }
    wr8(to[1], v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x60)) & ~P_C) | (v >= 0x60 ? P_C : 0));
    big = v >= 0x60;
    if (big) {
        v = op_sbc8(v, 0x60, &cpu.p);
        cyc(32 + kw);
        wr8(to[1], v);
        cyc(kr);
        v = rd8(to[2]);
        cpu.p &= (uint8_t)~P_C;
        cyc(30 + kw);
        wr8(to[2], op_adc8(v, 0x01, &cpu.p));
    }
    cpu.p &= (uint8_t)~P_C;
    cyc((big ? 38 : 76) + xw);
    v = rd8(b | (uint16_t)(0x0F78 + x));
    cyc(kr);
    v = op_adc8(v, rd8(to[2]), &cpu.p);
    cyc(kw);
    wr8(to[2], v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cyc(42);
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
 */
void abi_TimeAddToTotal(void)
{
    uint32_t to[3] = {0x0000F1, 0x0000F2, 0x0000F3};

    time_add((uint32_t)cpu.db << 16, cpu.x, to, 1);
}

/*
 * @routine 07:B796 TimeAddToNext
 * WHAT IT DOES: Adds record time X to the time after it ($0F79 + X),
 * unless it is no time ($FF).
 * HOW: As TimeAddToTotal. Not reached in the traces so far.
 * Entry: as TimeAddToTotal. Exit: as TimeAddToTotal.
 */
void abi_TimeAddToNext(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint32_t to[3] = {b | (uint16_t)(0x0F79 + cpu.x), b | (uint16_t)(0x0F7A + cpu.x), b | (uint16_t)(0x0F7B + cpu.x)};

    time_add(b, cpu.x, to, 0);
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
 */
void abi_TimeTotalsSum(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, v;
    unsigned g, i;

    op_sep(P_M | P_X);
    cpu.p |= P_D;
    cyc(36);
    for (g = 0; g < 3; g++) {
        cyc(16);
        wr8(0x0000F1, 0);
        cyc(16);
        wr8(0x0000F2, 0);
        cyc(16);
        wr8(0x0000F3, 0);
        for (i = 0; i < 4; i++) {
            cyc(62);
            cpu.x = (uint16_t)(g * 12 + i * 3);
            abi_TimeAddToTotal();
        }
        cyc(24);
        v = rd8(b | (uint16_t)(0x0F78 + g * 12));
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? P_C : 0));
        if (v == 0xFF) {
            cyc(48);
            wr8(0x0000F1, v);
            cyc(16);
            wr8(0x0000F2, v);
            cyc(16);
            wr8(0x0000F3, v);
        }
        for (i = 0; i < 3; i++) {
            cyc(16);
            v = rd8(0x0000F1 + i);
            cyc(24);
            wr8(b | (uint16_t)(0x0F9A + g * 3 + i), v);
        }
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    }
    cpu.p = p0;
    cyc(84);
}

/*
 * @routine 07:B727 TimeRunningTotals
 * WHAT IT DOES: Turns each group of four record times into running totals:
 * the first three are added into the time after each.
 * HOW: Decimal mode, TimeAddToNext for times 0-2, 4-6 and 8-10. Not
 * reached in the traces so far.
 * Entry: D = $0000. Exit: X = $1E; P restored; A as TimeAddToNext left it.
 */
void abi_TimeRunningTotals(void)
{
    cyc(36);
    uint8_t p0 = cpu.p;
    unsigned g, i;

    op_sep(P_M | P_X);
    cpu.p |= P_D;
    for (g = 0; g < 3; g++)
        for (i = 0; i < 3; i++) {
            cyc(62);
            cpu.x = (uint16_t)(g * 12 + i * 3);
            abi_TimeAddToNext();
        }
    cyc(84);
    cpu.p = p0;
}

/* LDA $0D4A, ASL x3, CLC, ADC $0D4A (8-bit): the course times nine. */
static uint8_t course_times_9(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    cyc(24);
    uint8_t v = rd8(b | 0x0D4A), a = v;
    int i;

    for (i = 0; i < 3; i++) {
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
        a = (uint8_t)(a << 1);
    }
    cpu.p &= (uint8_t)~P_C;
    cyc(80);
    return op_adc8(a, rd8(b | 0x0D4A), &cpu.p);
}

/* TAX, LDY #3, then three bytes from $70:FC6D + X to $70:FC70 + X. */
static void record_slot_copy(uint8_t a)
{
    uint8_t v = 0;

    cpu.x = a;
    cpu.y = 3;
    do {
        cyc(78);
        v = rd8(0x70FC6D + (uint32_t)cpu.x);
        cyc(32);
        wr8(0x70FC70 + (uint32_t)cpu.x, v);
        cpu.x = (uint8_t)(cpu.x + 1);
        cpu.y = (uint8_t)(cpu.y - 1);
    } while (cyc_loop(4, cpu.y));
    cyc(86);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, 0);
}

/*
 * @routine 07:B93C RecordSlotCopy0
 * WHAT IT DOES: Copies the course $0D4A's first three-byte record entry
 * ($70:FC6D + course * 9) to the second.
 * HOW: The offset as course * 8 + course, then the copy loop of
 * RecordSlotCopy3 (its code from the TAX).
 * Entry: M 8-bit, X 8-bit. Exit: A = the last byte copied; X = the offset
 * + 3; Y = 0; Z set, N clear; C, V from the add.
 */
void abi_RecordSlotCopy0(void)
{
    record_slot_copy(cyc_then(2, cyc_then(2, cyc_then(2, course_times_9()))));
}

/*
 * @routine 07:B948 RecordSlotCopy3
 * WHAT IT DOES: Copies the course $0D4A's second three-byte record entry
 * ($70:FC70 + course * 9) to the third.
 * HOW: As RecordSlotCopy0 with 3 added (the carry of the first add in).
 * Entry: M 8-bit, X 8-bit. Exit: as RecordSlotCopy0.
 */
void abi_RecordSlotCopy3(void)
{
    uint8_t a = course_times_9();

    record_slot_copy(op_adc8(a, 0x03, &cpu.p));
}

void abi_RecordSlotCopy0(void);
void abi_RecordSlotCopy3(void);

/* LDA $0D4A, ASL, CLC, ADC $0D4A, ASL x3 (8-bit): the course times 24. */
static uint8_t course_times_24(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    cyc(24);
    uint8_t a = rd8(b | 0x0D4A);
    int i;

    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
    a = (uint8_t)(a << 1);
    cpu.p &= (uint8_t)~P_C;
    cyc(52);
    a = op_adc8(a, rd8(b | 0x0D4A), &cpu.p);
    cyc(2);
    for (i = 0; cyc_loop(12, i < 3); i++) {
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
        a = (uint8_t)(a << 1);
    }
    cpu.p = nz8(cpu.p, a);
    return a;
}

/* TAX, LDY #8, then eight bytes from $70:FC88 + X to $70:FC90 + X. */
static void times_slot_copy(uint8_t a)
{
    uint8_t v = 0;

    cpu.x = a;
    cpu.y = 8;
    do {
        cyc(82);
        v = rd8(0x70FC88 + (uint32_t)cpu.x);
        cyc(32);
        wr8(0x70FC90 + (uint32_t)cpu.x, v);
        cpu.x = (uint8_t)(cpu.x + 1);
        cpu.y = (uint8_t)(cpu.y - 1);
    } while (cpu.y);
    cyc(86);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, 0);
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
 */
void abi_RecordShiftFrom0(void)
{
    cyc(46);
    abi_RecordSlotCopy0();
    times_slot_copy(cyc_then(6, course_times_24()));
}

/*
 * @routine 07:B91C RecordShiftFrom1
 * WHAT IT DOES: Moves the course $0D4A's second record down a place: its
 * entry (RecordSlotCopy3) and its times at $70:FC90 + course * 24 to the
 * next eight.
 * HOW: As RecordShiftFrom0 with 8 added (and the last shift's carry).
 * Entry: M 8-bit, X 8-bit. Exit: as RecordShiftFrom0, C from the add.
 */
void abi_RecordShiftFrom1(void)
{
    cyc(46);
    uint8_t a;

    abi_RecordSlotCopy3();
    a = course_times_24();
    times_slot_copy(op_adc8(a, 0x08, &cpu.p));
}

/* STA $F5, then the record entry at $70:FC6D + course * 9 + $F5 gets the
   name bytes $0F11, $0F12 and the car $0D89. */
static void record_name_store(uint8_t slot)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t a;
    int i;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | slot);
    cpu.p = nz8(cpu.p, slot);
    cyc(32);
    wr8(0x0000F5, slot);
    cyc(24);
    a = rd8(b | 0x0D4A);
    for (i = 0; i < 3; i++) {
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
        a = (uint8_t)(a << 1);
    }
    cpu.p &= (uint8_t)~P_C;
    cyc(80);
    a = op_adc8(a, rd8(b | 0x0D4A), &cpu.p);
    cyc(16);
    a = op_adc8(a, rd8(0x0000F5), &cpu.p);
    cpu.x = a;
    cpu.p = nz8(cpu.p, a);
    cyc(38);
    op_lda8(b | 0x0F11);
    cyc(32);
    wr8(0x70FC6D + (uint32_t)cpu.x, (uint8_t)cpu.a);
    cyc(24);
    op_lda8(b | 0x0F12);
    cyc(32);
    wr8(0x70FC6E + (uint32_t)cpu.x, (uint8_t)cpu.a);
    cyc(24);
    op_lda8(b | 0x0D89);
    cyc(32);
    wr8(0x70FC6F + (uint32_t)cpu.x, (uint8_t)cpu.a);
    cyc(42);
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
 */
void abi_RecordNameStore0(void)
{
    cyc(22);
    record_name_store(0x00);
}

/*
 * @routine 07:B8E1 RecordNameStore3
 * WHAT IT DOES: As RecordNameStore0 for the second entry.
 * HOW: The same code with 3; no script reaches it (unit_cpu does).
 * Entry: as RecordNameStore0. Exit: as RecordNameStore0.
 */
void abi_RecordNameStore3(void)
{
    cyc(22);
    record_name_store(0x03);
}

/*
 * @routine 07:B8E5 RecordNameStore6
 * WHAT IT DOES: As RecordNameStore0 for the third entry.
 * HOW: The same code with 6; no script reaches it (unit_cpu does).
 * Entry: as RecordNameStore0. Exit: as RecordNameStore0.
 */
void abi_RecordNameStore6(void)
{
    record_name_store(0x06);
}

/*
 * @routine 07:B964 TimeBelowRecord
 * WHAT IT DOES: Compares the time $0EEF/$0EEE/$0EED (most significant
 * first) with the record time at $0F81 + X: carry clear when it is
 * lower; when a byte is higher it returns at once with carry set.
 * HOW: 8-bit CMP per byte, BCC lower, BEQ on to the next.
 * Entry: M 8-bit; X = the record's offset. Exit: A = the last byte
 * compared; N, Z, C from that compare.
 */
void abi_TimeBelowRecord(void)
{
    static const uint16_t mine[3] = {0x0EEF, 0x0EEE, 0x0EED}, rec[3] = {0x0F81, 0x0F80, 0x0F7F};
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t a, m;
    int i;

    for (i = 0; i < 3; i++) {
        cyc(24);
        op_lda8(b | mine[i]);
        a = (uint8_t)cpu.a;
        cyc(24);
        m = rd8(b | (uint16_t)(rec[i] + cpu.x));
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(a - m)) & ~P_C) | (a >= m ? (cyc(38), P_C) : (cyc(28), 0)));
        if (a != m)
            { cyc(36); return; }
    }
    cyc(4);
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
 */
void abi_BcdToBinary(void)
{
    static const uint16_t thr[3] = {0x1000, 0x0100, 0x0010};
    static const uint16_t mul[3] = {1000, 100, 10};
    static const uint8_t sh[3] = {12, 8, 4};
    uint16_t x0 = cpu.x, v = cpu.a, n, t, pre = 30;
    int i;

    cyc(16);
    wr16(0x0000F1, 0x0000);
    for (i = 0; i < 3; i++) {
        if (v < thr[i])
            { pre += 46; continue; }   /* CMP, BCC taken */
        n = (uint16_t)(v >> sh[i]);
        cyc(pre + 100 + 14 * sh[i]);
        t = rd16(0x0000F1);
        do
            t = (uint16_t)(t + mul[i]);
        while (cyc_loop(74, --n));
        cyc(84);
        wr16(0x0000F1, t);
        v &= (uint16_t)(thr[i] - 1);
        pre = 60;                       /* PLA, AND */
    }
    cpu.a = v;
    cyc(pre + 30);
    op_add16(rd16(0x0000F1));
    cpu.x = x0;
    cpu.p = nz16(cpu.p, x0);
    cyc(80);
}

/* ROR A (16-bit) through the carry. */
static void ror16(void)
{
    uint8_t c = (uint8_t)(cpu.a & 1);

    cpu.a = (uint16_t)(cpu.a >> 1 | (cpu.p & P_C) << 15);
    cpu.p = (uint8_t)(nz16((uint8_t)(cpu.p & ~P_C), cpu.a) | c);
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
 */
void abi_TimePackBcd(void)
{
    int i;

    op_and16(0x00FF);
    (cyc(84), abi_BcdToBinary());
    op_and16(0x007F);
    cyc(38);
    wr16(0x0000F3, cpu.a);
    cpu.a = cpu.x;
    cpu.p = nz16(cpu.p, cpu.a);
    op_and16(0x00FF);
    (cyc(98), abi_BcdToBinary());
    op_xba();
    op_lsr16();
    op_and16(0x1F80);
    cyc(72);
    cpu.a |= rd16(0x0000F3);
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(16);
    wr16(0x0000F3, cpu.a);
    cpu.a = cpu.y;
    cpu.p = nz16(cpu.p, cpu.a);
    op_and16(0x00FF);
    (cyc(98), abi_BcdToBinary());
    for (i = 0; i < 4; i++)
        ror16();
    op_and16(0xE000);
    cyc(94);
    cpu.a |= rd16(0x0000F3);
    cpu.p = nz16(cpu.p, cpu.a);
    cyc(42);
}

/* CLC, LDA.L, ADC $F1, STA $F1 over n bytes from addr + x (8-bit X). */
static void sum_into_f1(uint32_t addr, uint8_t x, unsigned n)
{
    uint8_t a = 0;
    unsigned first = 1;

    while (n--) {
        cpu.p &= (uint8_t)~P_C;
        /* the first round comes from TAX, LDY, CLC; later ones from INX, DEY, BNE, CLC */
        cyc(first ? 76 : 96);
        first = 0;
        a = rd8(addr + x);
        cyc(16);
        a = op_adc8(a, rd8(0x0000F1), &cpu.p);
        cyc(16);
        wr8(0x0000F1, a);
        x++;
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | a);
    cpu.x = x;
    cpu.y = 0;
}

/* ASL A (16-bit), C the bit out. */
static uint16_t asl16(uint16_t a)
{
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 15));
    return (uint16_t)(a << 1);
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
 */
void abi_RecordTimeInsert(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p, place, page;
    uint16_t a;
    unsigned i;

    op_sep(P_M | P_X);
    cyc(46);
    op_lda8(b | 0x0D24);
    if ((uint8_t)cpu.a == 0x03) {
        cyc(32);
        op_plp(p0);
        cyc(28);
        return;
    } else { cyc(122); }
    cpu.x = 0x00;
    abi_TimeBelowRecord();
    if (!(cpu.p & P_C)) {
        cyc(62);
        abi_RecordShiftFrom1();
        (cyc(46), abi_RecordShiftFrom0());
        (cyc(46), abi_RecordNameStore0());
        place = 0x01;
        page = 0x00;
    } else {
        cyc(84);
        cpu.x = 0x0C;
        abi_TimeBelowRecord();
        if (!(cpu.p & P_C)) {
            cyc(62);                    /* BCS not taken, JSR */
            abi_RecordShiftFrom1();
            (cyc(46), abi_RecordNameStore3());
            place = 0x02;
            page = 0x08;
        } else {
            cyc(84);
            cpu.x = 0x18;
            abi_TimeBelowRecord();
            if (cpu.p & P_C) {
                cyc(56);
                wr8(b | 0x10C7, 0xFF);
                cyc(40);
                wr8(b | 0x0FAC, 0x00);
                cpu.a = (uint16_t)(cpu.a & 0xFF00);
                op_plp(p0);
                cyc(28);
                return;
            } else { cyc(68); }         /* BCC taken, JSR */
            abi_RecordNameStore6();
            place = 0x03;
            page = 0x10;
        }
    }
    cyc(40);
    wr8(b | 0x0FAC, place);
    op_rep(P_M | P_X);
    cyc(place == 0x03 ? 56 : 78);       /* the third place has no BRA */
    wr16(0x0000F7, page);
    cyc(24);
    a = asl16(rd16(b | 0x0D4A));
    cpu.p &= (uint8_t)~P_C;
    cyc(52);
    a = (uint16_t)(op_adc16(a, rd16(b | 0x0D4A), &cpu.p) & 0x00FF);
    for (i = 0; i < 3; i++)
        a = asl16(a);
    cyc(82);
    a = op_adc16(a, rd16(0x0000F7), &cpu.p);
    cyc(16);
    wr16(0x0000F7, a);
    for (i = 0; i < 4; i++) {
        cyc(24);
        cpu.a = rd16(b | (uint16_t)(0x0EF9 + 3 * i));
        cyc(24);
        cpu.x = rd16(b | (uint16_t)(0x0EFA + 3 * i));
        cyc(24);
        cpu.y = rd16(b | (uint16_t)(0x0EFB + 3 * i));
        (cyc(46), abi_TimePackBcd());
        cyc(16);
        cpu.x = rd16(0x0000F7);
        cyc(32);
        wr16(0x70FC88 + cpu.x + 2 * i, cpu.a);
    }
    op_sep(P_M | P_X);
    cyc(16);
    wr8(0x0000F1, 0x00);
    sum_into_f1(0x70FC88, course_times_24(), cyc_then(4, 24));
    sum_into_f1(0x70FC6D, (cyc(44), course_times_9()), 9);
    cyc(68);
    cpu.x = rd8(b | 0x0D4A);
    cyc(16);
    wr8(0x70FC2D + cpu.x, cyc_then(32, rd8(0x0000F1)));
    op_plp(p0);
    cyc(28);
}
