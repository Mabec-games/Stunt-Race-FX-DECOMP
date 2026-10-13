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

    wr16(0x0000F1, 0x0000);
    for (i = 0; i < 3; i++) {
        for (;;) {
            op_cmp16(unit[i]);
            if (!(cpu.p & P_C))
                break;
            cpu.a = op_sbc16(cpu.a, unit[i], &cpu.p);
            a = cpu.a;                  /* PHA, the count, PLA */
            op_lda16(0x0000F1);
            op_add16(digit[i]);
            wr16(0x0000F1, cpu.a);
            op_lda16_imm(a);
        }
    }
    op_add16(rd16(0x0000F1));
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
        op_lda16_imm(0xFFFF);
        cpu.x = cpu.y = 0xFFFF;
        return;
    }
    op_tax16();
    op_and16(0x007F);
    abi_BinaryToBcd();
    op_tay16();
    op_txa16();
    v = cpu.a;                          /* PHA */
    op_asl16();
    op_xba();
    op_and16(0x003F);
    abi_BinaryToBcd();
    op_tax16();
    op_lda16_imm(v);                    /* PLA */
    op_rol16();
    op_rol16();
    op_rol16();
    op_rol16();
    op_and16(0x0007);
    abi_BinaryToBcd();
}

/* Adds the BCD time at $0F76 + X (hundredths, seconds, minutes) to the
   time at `to` (three bytes), with the decimal carries the original makes:
   hundredths into seconds, seconds past 59 (once on the add's carry, once
   on 60 or more) into minutes. A $FF hundredths byte means no time. The
   decimal flag is as the caller set it. */
static void time_add(uint32_t b, uint16_t x, uint32_t to[3])
{
    uint8_t v;

    v = rd8(b | (uint16_t)(0x0F76 + x));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? P_C : 0));
    if (v == 0xFF)
        return;
    cpu.p &= (uint8_t)~P_C;
    v = op_adc8(v, rd8(to[0]), &cpu.p);
    wr8(to[0], v);
    v = rd8(b | (uint16_t)(0x0F77 + x));
    v = op_adc8(v, rd8(to[1]), &cpu.p);
    if (cpu.p & P_C) {
        cpu.p |= P_C;
        v = op_sbc8(v, 0x60, &cpu.p);
        wr8(to[1], v);
        v = rd8(to[2]);
        cpu.p &= (uint8_t)~P_C;
        wr8(to[2], op_adc8(v, 0x01, &cpu.p));
        v = rd8(to[1]);
    }
    wr8(to[1], v);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0x60)) & ~P_C) | (v >= 0x60 ? P_C : 0));
    if (v >= 0x60) {
        v = op_sbc8(v, 0x60, &cpu.p);
        wr8(to[1], v);
        v = rd8(to[2]);
        cpu.p &= (uint8_t)~P_C;
        wr8(to[2], op_adc8(v, 0x01, &cpu.p));
    }
    cpu.p &= (uint8_t)~P_C;
    v = rd8(b | (uint16_t)(0x0F78 + x));
    v = op_adc8(v, rd8(to[2]), &cpu.p);
    wr8(to[2], v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
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

    time_add((uint32_t)cpu.db << 16, cpu.x, to);
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

    time_add(b, cpu.x, to);
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
    for (g = 0; g < 3; g++) {
        wr8(0x0000F1, 0);
        wr8(0x0000F2, 0);
        wr8(0x0000F3, 0);
        for (i = 0; i < 4; i++) {
            cpu.x = (uint16_t)(g * 12 + i * 3);
            abi_TimeAddToTotal();
        }
        v = rd8(b | (uint16_t)(0x0F78 + g * 12));
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? P_C : 0));
        if (v == 0xFF) {
            wr8(0x0000F1, v);
            wr8(0x0000F2, v);
            wr8(0x0000F3, v);
        }
        for (i = 0; i < 3; i++) {
            v = rd8(0x0000F1 + i);
            wr8(b | (uint16_t)(0x0F9A + g * 3 + i), v);
        }
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    }
    cpu.p = p0;
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
    uint8_t p0 = cpu.p;
    unsigned g, i;

    op_sep(P_M | P_X);
    cpu.p |= P_D;
    for (g = 0; g < 3; g++)
        for (i = 0; i < 3; i++) {
            cpu.x = (uint16_t)(g * 12 + i * 3);
            abi_TimeAddToNext();
        }
    cpu.p = p0;
}

/* LDA $0D4A, ASL x3, CLC, ADC $0D4A (8-bit): the course times nine. */
static uint8_t course_times_9(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v = rd8(b | 0x0D4A), a = v;
    int i;

    for (i = 0; i < 3; i++) {
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >> 7));
        a = (uint8_t)(a << 1);
    }
    cpu.p &= (uint8_t)~P_C;
    return op_adc8(a, rd8(b | 0x0D4A), &cpu.p);
}

/* TAX, LDY #3, then three bytes from $70:FC6D + X to $70:FC70 + X. */
static void record_slot_copy(uint8_t a)
{
    uint8_t v = 0;

    cpu.x = a;
    cpu.y = 3;
    do {
        v = rd8(0x70FC6D + (uint32_t)cpu.x);
        wr8(0x70FC70 + (uint32_t)cpu.x, v);
        cpu.x = (uint8_t)(cpu.x + 1);
        cpu.y = (uint8_t)(cpu.y - 1);
    } while (cpu.y);
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
    record_slot_copy(course_times_9());
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
