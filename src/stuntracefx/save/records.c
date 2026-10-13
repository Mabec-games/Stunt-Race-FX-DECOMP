/*
 * records.c: five-byte records in the save data (Super FX RAM $70:FC4A and
 * $70:FC5E onwards).
 */
#include "machine.h"

/* Sum five bytes from `base` + X with 8-bit adds; X steps past them. */
static uint8_t sum5(uint32_t base, uint8_t *x, uint8_t *p)
{
    uint8_t a = 0, v;
    unsigned s, i;

    for (i = 0; i < 5; i++) {
        v = rd8(base + *x);
        s = (unsigned)a + v;
        *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0) | ((~(a ^ v) & (a ^ (uint8_t)s) & 0x80) ? P_V : 0));
        a = (uint8_t)s;
        (*x)++;
    }
    *p = (uint8_t)((*p & ~P_N) | P_Z);
    return a;
}

/*
 * @routine 07:ADCC SaveRecordSumA
 * WHAT IT DOES: Returns the 8-bit sum of the five bytes at $70:FC4A + X.
 * HOW: Five additions, each with carry cleared first.
 * Entry: M 8-bit, X 8-bit; X = offset. Exit: A low byte = the sum; X + 5;
 * Y = 0; Z set, N clear; C, V from the last add.
 */
uint8_t SaveRecordSumA(uint8_t *x, uint8_t *p)
{
    return sum5(0x70FC4A, x, p);
}

void abi_SaveRecordSumA(void)
{
    uint8_t x = (uint8_t)cpu.x, p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SaveRecordSumA(&x, &p));
    cpu.x = x;
    cpu.y = 0;
    cpu.p = p;
}

/*
 * @routine 07:ADF9 SaveRecordSumB
 * WHAT IT DOES: Returns the 8-bit sum of the five bytes at $70:FC5E + X.
 * HOW: As SaveRecordSumA.
 * Entry: as SaveRecordSumA. Exit: as SaveRecordSumA.
 */
uint8_t SaveRecordSumB(uint8_t *x, uint8_t *p)
{
    return sum5(0x70FC5E, x, p);
}

void abi_SaveRecordSumB(void)
{
    uint8_t x = (uint8_t)cpu.x, p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SaveRecordSumB(&x, &p));
    cpu.x = x;
    cpu.y = 0;
    cpu.p = p;
}

/* Check `count` records from `base` against their sum bytes at `sums`; a
   record whose sum differs or whose first byte is 0 is reset to FF FF 00 00
   00 with sum byte $FE. Returns A's low byte as the loop leaves it; V is
   left from the last sum. */
static uint8_t check_records(uint8_t bank, uint16_t base, uint16_t sums, unsigned count,
                             uint8_t (*sum)(uint8_t *, uint8_t *), uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t x = 0, a = 0;
    unsigned y;

    for (y = 0; y < count; y++, x = (uint8_t)(x + 5)) {
        uint8_t xs = x;

        a = sum(&xs, p);
        if (a == rd8(b | (uint16_t)(sums + y))) {
            a = rd8(b | (uint16_t)(base + x));
            if (a)
                continue;
        }
        wr8(b | (uint16_t)(base + x), 0xFF);
        wr8(b | (uint16_t)(base + x + 1), 0xFF);
        wr8(b | (uint16_t)(base + x + 2), 0x00);
        wr8(b | (uint16_t)(base + x + 3), 0x00);
        wr8(b | (uint16_t)(base + x + 4), 0x00);
        wr8(b | (uint16_t)(sums + y), 0xFE);
        a = 0xFE;
    }
    return a;
}

/*
 * @routine 07:B0D1 SaveRecordsCheckA
 * WHAT IT DOES: Validates the four records at $70:FC4A against their sum
 * bytes at $70:FC46; a record that fails, or whose first byte is 0, is reset
 * to FF FF 00 00 00 with sum byte $FE (which is its sum).
 * HOW: SaveRecordSumA for each record, through the data bank.
 * Entry: any widths; DB = $70. Exit: A low byte = $FE when the last record
 * was reset, else its first byte; X = 20; Y = 4; M 8-bit, X 8-bit; C, Z
 * set, N clear; V from the last sum.
 */
uint8_t SaveRecordsCheckA(uint8_t bank, uint8_t *p)
{
    return check_records(bank, 0xFC4A, 0xFC46, 4, SaveRecordSumA, p);
}

void abi_SaveRecordsCheckA(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SaveRecordsCheckA(cpu.db, &p));
    cpu.x = 20;
    cpu.y = 4;
    cpu.p = (uint8_t)((p & ~P_N) | P_M | P_X | P_C | P_Z);
}

/*
 * @routine 07:B097 SaveRecordsCheckB
 * WHAT IT DOES: As SaveRecordsCheckA, for the three records at $70:FC5E
 * and their sum bytes at $70:FC43.
 * HOW: SaveRecordSumB for each record, through the data bank.
 * Entry: any widths; DB = $70. Exit: A low byte = $FE when the last record
 * was reset, else its first byte; X = 15; Y = 3; M 8-bit, X 8-bit; C, Z
 * set, N clear; V from the last sum.
 */
uint8_t SaveRecordsCheckB(uint8_t bank, uint8_t *p)
{
    return check_records(bank, 0xFC5E, 0xFC43, 3, SaveRecordSumB, p);
}

void abi_SaveRecordsCheckB(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | SaveRecordsCheckB(cpu.db, &p));
    cpu.x = 15;
    cpu.y = 3;
    cpu.p = (uint8_t)((p & ~P_N) | P_M | P_X | P_C | P_Z);
}

/* 8-bit ADC with carry clear; V as the 65816 sets it. */
static uint8_t add8(uint8_t a, uint8_t m, uint8_t *p)
{
    uint8_t r = (uint8_t)(a + m);

    *p = (uint8_t)((*p & ~P_V) | ((~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0));
    return r;
}

/*
 * @routine 07:B190 SaveBlocksCheck
 * WHAT IT DOES: Validates blocks 12-15 of the 48-byte blocks at $70:FCD0:
 * each is twelve 4-byte entries whose second byte must be below $41, and
 * the 8-bit sum of all 48 bytes must equal the block's sum byte at
 * $70:FC33 + block. A block that fails is reset to twelve entries of 00 00
 * FF FF, with sum byte $E8 (which is their sum).
 * HOW: Direct page $F1 holds the running sum, $F3 the block, $F5 the
 * block's offset; tables are read through the data bank. A second byte of
 * $41 or more fails the block at once.
 * Entry: M 8-bit, X 16-bit, D = $0000; DB = $70. Exit: A = $0210; X = $0F;
 * Y = 0; $F3 = $10, $F5 = $02D0; M 8-bit, X 16-bit; C, Z set, N clear; V
 * from the last add.
 */
void SaveBlocksCheck(uint8_t bank, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t block, base, x;
    uint8_t sum;
    unsigned i;
    int bad;

    for (block = 12; block < 16; block++) {
        base = (uint16_t)(block * 48);
        wr16(0x0000F3, block);
        wr16(0x0000F5, base);
        sum = 0;
        bad = 0;
        for (i = 0, x = base; i < 12 && !bad; i++, x += 4) {
            uint8_t second;

            sum = add8(sum, rd8(b | (uint16_t)(0xFCD0 + x)), p);
            second = rd8(b | (uint16_t)(0xFCD1 + x));
            if (second >= 0x41) {
                bad = 1;
                break;
            }
            sum = add8(sum, second, p);
            sum = add8(sum, rd8(b | (uint16_t)(0xFCD2 + x)), p);
            sum = add8(sum, rd8(b | (uint16_t)(0xFCD3 + x)), p);
        }
        wr8(0x0000F1, sum);
        if (!bad && rd8(b | (uint16_t)(0xFC33 + block)) != sum)
            bad = 1;
        if (bad) {
            for (i = 0, x = base; i < 12; i++, x += 4) {
                wr8(b | (uint16_t)(0xFCD0 + x), 0x00);
                wr8(b | (uint16_t)(0xFCD1 + x), 0x00);
                wr8(b | (uint16_t)(0xFCD2 + x), 0xFF);
                wr8(b | (uint16_t)(0xFCD3 + x), 0xFF);
            }
            wr8(b | (uint16_t)(0xFC33 + block), 0xE8);
        }
    }
    wr8(0x0000F3, 0x10);
}

void abi_SaveBlocksCheck(void)
{
    uint8_t p = cpu.p;

    SaveBlocksCheck(cpu.db, &p);
    cpu.a = 0x0210;
    cpu.x = 0x000F;
    cpu.y = 0x0000;
    cpu.p = (uint8_t)(((p & ~P_N) | P_M | P_C | P_Z) & ~P_X);
}

/*
 * @routine 07:B10B SaveTablesCheck
 * WHAT IT DOES: Validates three save tables, last first: table k is twelve
 * two-byte entries at $70:FC88 + 24k and nine bytes at $70:FC6D + 9k, and
 * every entry's second byte must be $1F or more; the 8-bit sum of all 33
 * bytes must equal the sum byte at $70:FC2D + k. A table that fails is reset
 * to 24 bytes of $FF and nine of 0, with sum byte $E8 (which is their sum).
 * HOW: The table's two offsets come from the byte tables at $07:B18D and
 * $07:B18A into direct page $F3 and $F5 (high bytes $F4, $F6 cleared);
 * $F1 holds the running sum. Tables are read through the data bank. A
 * second byte below $1F fails the table at once.
 * Entry: any widths, D = $0000; DB = $70. Exit: A = the last sum byte read
 * or written (high byte clear); X = $FFFF; Y = 0; M 8-bit, X 16-bit; N
 * set, Z clear; C, V as the last table's check left them.
 */
uint8_t SaveTablesCheck(uint8_t bank, uint8_t *p)
{
    static const uint8_t first[3] = {0x00, 0x18, 0x30};
    static const uint8_t second[3] = {0x00, 0x09, 0x12};
    uint32_t b = (uint32_t)bank << 16;
    uint8_t a = 0, sum, v;
    uint16_t x;
    int k, i, bad;

    wr8(0x0000F4, 0x00);
    wr8(0x0000F6, 0x00);
    for (k = 2; k >= 0; k--) {
        wr8(0x0000F3, first[k]);
        wr8(0x0000F5, second[k]);
        sum = 0;
        bad = 0;
        for (i = 0, x = first[k]; i < 12; i++, x += 2) {
            sum = add8(sum, rd8(b | (uint16_t)(0xFC88 + x)), p);
            v = rd8(b | (uint16_t)(0xFC89 + x));
            if (v < 0x1F) {
                *p &= (uint8_t)~P_C;
                bad = 1;
                break;
            }
            sum = add8(sum, v, p);
        }
        if (!bad) {
            for (i = 0, x = second[k]; i < 9; i++, x++)
                sum = add8(sum, rd8(b | (uint16_t)(0xFC6D + x)), p);
        }
        wr8(0x0000F1, sum);
        if (!bad) {
            a = rd8(b | (uint16_t)(0xFC2D + k));
            *p = (uint8_t)((*p & ~P_C) | (a >= sum ? P_C : 0));
            bad = a != sum;
        }
        if (bad) {
            for (i = 0, x = first[k]; i < 24; i++, x++)
                wr8(b | (uint16_t)(0xFC88 + x), 0xFF);
            for (i = 0, x = second[k]; i < 9; i++, x++)
                wr8(b | (uint16_t)(0xFC6D + x), 0x00);
            a = 0xE8;
            wr8(0x70FC2D + k, a);
        }
    }
    return a;
}

void abi_SaveTablesCheck(void)
{
    uint8_t p = cpu.p, a = SaveTablesCheck(cpu.db, &p);

    cpu.a = a;
    cpu.x = 0xFFFF;
    cpu.y = 0x0000;
    cpu.p = (uint8_t)(((p & ~(P_Z | P_X)) | P_N | P_M));
}

/* One entry byte of SaveBlocksValidate: $FF passes; otherwise `bad` says
   whether it fails. */
static int entry_byte_bad(uint8_t v, int which)
{
    if (v == 0xFF)
        return 0;
    switch (which) {
    case 0: return (v & 0x7F) >= 0x64;
    case 1: return (v & 0x1F) >= 0x1E || (v & 0xE0) < 0x20;
    case 2: return (v & 0x1F) >= 0x1C;
    default: return (v & 0x03) >= 0x38;  /* never true */
    }
}

/*
 * @routine 07:B211 SaveBlocksValidate
 * WHAT IT DOES: Validates blocks 0-11 of the 48-byte blocks at $70:FCD0:
 * each is twelve 4-byte entries whose bytes must be $FF or in range (first
 * byte's low seven bits below 100, second byte's low five bits below 30 and
 * top three bits not 0, third byte's low five bits below 28); the 8-bit sum
 * of the block's four bytes at $70:FFD0 + 4 * block and its 48 bytes must
 * equal its sum byte at $70:FC33 + block. A block that fails sets $1FFE to
 * $11 and is reset to all $FF, its four bytes at $70:FFD0 too, with sum
 * byte $CC (which is their sum); then it is checked again.
 * HOW: Direct page $F1 holds the running sum, $F3 the block, $F5 its
 * offset. The block bytes are read through the data bank, the four-byte
 * groups long. The fourth entry byte's test, low two bits $38 or more, can
 * never fail.
 * Entry: M 8-bit, D = $0000; DB = $70. Exit: A = $020C; X = $0B; Y = 0;
 * $F3 = $0C; M 8-bit, X 16-bit; C, Z set, N clear; V from the last add.
 */
void SaveBlocksValidate(uint8_t bank, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t block = 0, base, x;
    uint8_t sum, v;
    int i, j, bad;

    wr16(0x0000F3, 0x0000);
    while (block < 12) {
        x = (uint16_t)(block << 2);
        sum = rd8(0x70FFD0 + x);
        wr8(0x0000F1, sum);
        sum = add8(rd8(0x70FFD1 + x), sum, p);
        sum = add8(rd8(0x70FFD2 + x), sum, p);
        sum = add8(rd8(0x70FFD3 + x), sum, p);
        wr16(0x0000F1, sum);
        base = (uint16_t)(block * 48);
        wr16(0x0000F5, base);
        bad = 0;
        for (i = 0, x = base; i < 12 && !bad; i++, x += 4) {
            for (j = 0; j < 4; j++) {
                v = rd8(b | (uint16_t)(0xFCD0 + x + j));
                if (entry_byte_bad(v, j)) {
                    bad = 1;
                    break;
                }
                sum = add8(sum, v, p);
            }
        }
        wr8(0x0000F1, sum);
        if (!bad && rd8(b | (uint16_t)(0xFC33 + block)) != sum)
            bad = 1;
        if (bad) {
            wr8(0x001FFE, 0x11);
            for (i = 0; i < 48; i++)
                wr8(b | (uint16_t)(0xFCD0 + base + i), 0xFF);
            for (i = 0; i < 4; i++)
                wr8(b | (uint16_t)(0xFFD0 + (block << 2) + i), 0xFF);
            wr8(b | (uint16_t)(0xFC33 + block), 0xCC);
            continue;  /* checked again */
        }
        block++;
        wr8(0x0000F3, (uint8_t)block);
    }
}

void abi_SaveBlocksValidate(void)
{
    uint8_t p = cpu.p;

    SaveBlocksValidate(cpu.db, &p);
    cpu.a = 0x020C;
    cpu.x = 0x000B;
    cpu.y = 0x0000;
    cpu.p = (uint8_t)(((p & ~(P_N | P_X)) | P_M | P_C | P_Z));
}

/*
 * @routine 07:B085 SaveValidate
 * WHAT IT DOES: Validates the whole save area: the three tables, blocks
 * 0-11 and 12-15, then the two record sets, resetting whatever fails.
 * HOW: SaveTablesCheck, SaveBlocksValidate, SaveBlocksCheck,
 * SaveRecordsCheckB and SaveRecordsCheckA in turn; P is saved around them.
 * Entry: any widths, D = $0000; DB = $70. Exit: A = $02 and the low byte
 * SaveRecordsCheckA leaves; X = 20, Y = 4; P unchanged.
 */
void SaveValidate(uint8_t bank)
{
    uint8_t p = 0;

    SaveTablesCheck(bank, &p);
    SaveBlocksValidate(bank, &p);
    SaveBlocksCheck(bank, &p);
    SaveRecordsCheckB(bank, &p);
    SaveRecordsCheckA(bank, &p);
}

void abi_SaveValidate(void)
{
    uint8_t p = cpu.p;

    abi_SaveTablesCheck();
    abi_SaveBlocksValidate();
    abi_SaveBlocksCheck();
    abi_SaveRecordsCheckB();
    abi_SaveRecordsCheckA();
    cpu.p = p;
}

/* The unlock bytes of the save in the order their checksum adds them. */
static const uint16_t unlock_bytes[10] = {
    0xFC0B, 0xFC0F, 0xFC06, 0xFC07, 0xFC0C, 0xFC0D, 0xFC0E, 0xFC08, 0xFC09, 0xFC0A};

/* The checksum: an 8-bit add chain, each carry going into the next add. */
static uint8_t unlock_sum(void)
{
    unsigned s = rd8(0x700000 | unlock_bytes[0]), c = 0, i;

    for (i = 1; i < 10; i++) {
        s = s + rd8(0x700000 | unlock_bytes[i]) + c;
        c = s >> 8;
        s &= 0xFF;
    }
    return (uint8_t)s;
}

/*
 * @routine 03:EFD2 SaveUnlocksCheck
 * WHAT IT DOES: Checks the unlock bytes of the save ($70:FC06-$FC0F, the
 * courses and classes opened) against their checksum at $70:FC14; when it
 * differs, clears them and the checksum word ($FC14-$FC15). Then validates
 * the whole save (SaveValidate).
 * HOW: Data bank $70; the checksum is an 8-bit add with carry over the ten
 * bytes in the order $FC0B, $FC0F, $FC06, $FC07, $FC0C-$FC0E, $FC08-$FC0A,
 * the carry cleared only before the first. P and the data bank are saved
 * and restored.
 * Entry: any widths, D = $0000. Exit: as SaveValidate (A = $02 and the
 * low byte SaveRecordsCheckA leaves; X = 20, Y = 4); P and the data bank
 * unchanged.
 */
void SaveUnlocksCheck(void)
{
    unsigned i;

    if (unlock_sum() != rd8(0x70FC14)) {
        for (i = 0; i < 10; i++)
            wr8(0x700000 | unlock_bytes[i], 0x00);
        wr8(0x70FC14, 0x00);
        wr8(0x70FC15, 0x00);
    }
    SaveValidate(0x70);
}

void abi_SaveUnlocksCheck(void)
{
    uint8_t p = cpu.p, db = cpu.db, s = unlock_sum();
    unsigned i;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | s);
    if (s != rd8(0x70FC14)) {
        for (i = 0; i < 10; i++)
            wr8(0x700000 | unlock_bytes[i], 0x00);
        wr8(0x70FC14, 0x00);
        wr8(0x70FC15, 0x00);
    }
    cpu.x &= 0x00FF;
    cpu.y &= 0x00FF;
    cpu.p |= P_M | P_X;
    cpu.db = 0x70;
    abi_SaveValidate();
    cpu.db = db;
    cpu.p = p;
}

void abi_TimeUnpackBcd(void);
void abi_TimeTotalsSum(void);
void abi_TimeRunningTotals(void);

/*
 * @routine 07:B518 RecordsUnpackCourse
 * WHAT IT DOES: Unpacks the records of course $0D4A for display: its twelve
 * packed times (the words at $70:FC88 + course * 24) each into three BCD
 * bytes (hundredths, seconds, minutes) from $0F76 on (TimeUnpackBcd), then
 * its nine name bytes ($70:FC6D + course * 9) to $10CF-$10D7.
 * HOW: The table offset is kept in direct page $F5; the time stores are
 * 8-bit (X and Y truncated by SEP #$30). P saved and restored.
 * Entry: DB a bank where $0D4A is work RAM, D = $0000. Exit: A low byte =
 * the last name byte; X = course * 9 (8-bit); Y as the last TimeUnpackBcd
 * left it, truncated; P unchanged.
 */
void abi_RecordsUnpackCourse(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, c;
    uint16_t x, a;
    unsigned i;

    cpu.p &= (uint8_t)~(P_M | P_X);
    a = rd16(b | 0x0D4A);
    a = (uint16_t)((uint16_t)(a << 1) + rd16(b | 0x0D4A));
    x = (uint16_t)((a & 0x00FF) << 3);
    wr16(0x0000F5, x);
    for (i = 0; i < 12; i++) {
        cpu.p &= (uint8_t)~(P_M | P_X);
        if (i)
            x = rd16(0x0000F5);
        cpu.x = x;
        cpu.a = rd16(0x70FC88 + (uint16_t)(x + 2 * i));
        abi_TimeUnpackBcd();
        cpu.p |= P_M | P_X;
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
        wr8(b | (uint16_t)(0x0F76 + 3 * i), (uint8_t)cpu.y);
        wr8(b | (uint16_t)(0x0F77 + 3 * i), (uint8_t)cpu.x);
        wr8(b | (uint16_t)(0x0F78 + 3 * i), (uint8_t)cpu.a);
    }
    c = rd8(b | 0x0D4A);
    c = (uint8_t)((uint8_t)(c << 3) + rd8(b | 0x0D4A));
    cpu.x = c;
    for (i = 0; i < 9; i++) {
        a = rd8(0x70FC6D + (uint16_t)(c + i));
        wr8(b | (uint16_t)(0x10CF + i), (uint8_t)a);
    }
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)a);
    cpu.p = p;
}

/*
 * @routine 07:B503 RecordsUnpackCourseTotals
 * WHAT IT DOES: Unpacks course $0D4A's records (RecordsUnpackCourse) and
 * sums each group of four times (TimeTotalsSum).
 * HOW: Two calls. Not reached in the traces so far.
 * Entry: as RecordsUnpackCourse. Exit: as TimeTotalsSum.
 */
void abi_RecordsUnpackCourseTotals(void)
{
    abi_RecordsUnpackCourse();
    abi_TimeTotalsSum();
}

/*
 * @routine 07:B50A RecordsUnpackCourseRunning
 * WHAT IT DOES: Unpacks course $0D4A's records (RecordsUnpackCourse) and
 * makes each group of four times running totals (TimeRunningTotals).
 * HOW: Two calls. Not reached in the traces so far.
 * Entry: as RecordsUnpackCourse. Exit: as TimeRunningTotals.
 */
void abi_RecordsUnpackCourseRunning(void)
{
    abi_RecordsUnpackCourse();
    abi_TimeRunningTotals();
}
