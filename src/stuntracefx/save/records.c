/*
 * records.c: five-byte records in the save data (Super FX RAM $70:FC4A and
 * $70:FC5E onwards).
 */
#include "machine.h"
#include "cpu_ops.h"

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

/* The records screen's rows: direct page $F2 = the time's offset in the
   record times, $F3-$F5 = the row's name entry (two bytes of three 5-bit
   letters, then the car), $F7 = the page's offset. */
static uint32_t dp(uint8_t a)
{
    return (uint16_t)(cpu.d + a);
}

void abi_SpritePieces3(void);

/*
 * @routine 02:FF01 RecordNameLetters
 * WHAT IT DOES: Puts the three 5-bit letters of A (bits 0-4, 5-9, 10-14)
 * plus $0A into the tile bytes of sprites Y / 4, + 1 and + 2.
 * HOW: The letters are kept on the stack between them (PHA, PLA). Not
 * reached in the traces so far.
 * Entry: M 16-bit, X 16-bit; A = the letters, Y = sprite buffer offset.
 * Exit: A = the third tile; M 16-bit; N, Z, C, V clear (the last add).
 */
void abi_RecordNameLetters(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v = cpu.a;
    unsigned i;
    static const uint8_t shift[3] = {0, 5, 10};

    for (i = 0; i < 3; i++) {
        cpu.a = (uint16_t)(((v >> shift[i]) & 0x001F) + 0x000A);
        wr8(b | (uint16_t)(0x039F + i * 4 + cpu.y), (uint8_t)cpu.a);
    }
    cpu.p &= (uint8_t)~(P_N | P_Z | P_C | P_V | P_M);
}

/*
 * @routine 02:F077 RecordRowDraw
 * WHAT IT DOES: Draws one records row from sprite Y: the time at $0F76 +
 * ($F7 + $F2) as six digit sprites (minutes, seconds, hundredths, a digit
 * a sprite), the car (piece set (name byte 3 * 3) & 15 at X = $57) and the
 * three letters of the name; a time with no seconds ($FF) hides the row's
 * first three sprites instead (Y = $F0).
 * HOW: Digit nibbles to the sprites' tile bytes; SpritePieces3 for the car
 * with palette 0; RecordNameLetters four bytes on. All but the no-time
 * path are not reached in the traces so far.
 * Entry: Y = sprite number.
 * Exit: no time: A low byte = $F0, M 8-bit, X 16-bit, X = the time's
 * offset, Y = sprite * 4, N set, Z clear, C set (the compare), V from
 * adding $F2. Otherwise as RecordNameLetters, X as SpritePieces3 left it,
 * Y = sprite * 4 + 4.
 */
void abi_RecordRowDraw(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t w, y;
    uint8_t v, t;
    unsigned i;
    static const uint16_t hi[3] = {0x039B, 0x038F, 0x0383}, lo[3] = {0x039F, 0x0393, 0x0387};

    cpu.p &= (uint8_t)~(P_M | P_X);
    w = rd16(dp(0xF7));
    cpu.p &= (uint8_t)~P_C;
    w = op_adc16(w, rd16(dp(0xF2)), &cpu.p);
    cpu.x = (uint16_t)(w & 0x00FF);
    y = (uint16_t)(cpu.y << 2);
    cpu.p = (uint8_t)((nz16(cpu.p, y) & ~P_C) | ((cpu.y & 0x4000) ? P_C : 0));
    cpu.y = y;
    cpu.p |= P_M;
    v = rd8(b | (uint16_t)(0x0F77 + cpu.x));
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - 0xFF)) & ~P_C) | (v >= 0xFF ? P_C : 0));
    if (v == 0xFF) {
        cpu.a = (uint16_t)((y & 0xFF00) | 0xF0);
        cpu.p = nz8(cpu.p, 0xF0);
        wr8(b | (uint16_t)(0x0376 + y), 0xF0);
        wr8(b | (uint16_t)(0x037A + y), 0xF0);
        wr8(b | (uint16_t)(0x037E + y), 0xF0);
        return;
    }
    for (i = 0; i < 3; i++) {
        v = rd8(b | (uint16_t)(0x0F76 + i + cpu.x));
        wr8(b | (uint16_t)(hi[i] + y), (uint8_t)(v >> 4));
        t = rd8(b | (uint16_t)(0x0F76 + i + cpu.x));
        wr8(b | (uint16_t)(lo[i] + y), (uint8_t)(t & 0x0F));
    }
    cpu.p &= (uint8_t)~P_M;
    w = rd16(dp(0xF5));
    w = (uint16_t)((w << 1) + rd16(dp(0xF5)));
    wr16(b | 0x10C3, (uint16_t)(w & 0x000F));
    wr16(b | 0x10BD, 0x0057);
    wr16(b | 0x10C5, 0x0000);
    cpu.a = 0x0057;
    abi_SpritePieces3();
    cpu.y = (uint16_t)(y + 4);
    cpu.p &= (uint8_t)~P_M;
    cpu.a = rd16(dp(0xF3));
    abi_RecordNameLetters();
}

/* Row n (0-2): its name entry ($10CF + n * 3) to $F3-$F5, its screen Y
   ($7C + n * $11) to $10BF, then RecordRowDraw from sprite $0D75 + n * 2. */
static void record_row(unsigned n)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        v = rd8(b | (uint16_t)(0x10CF + n * 3 + i));
        wr8(dp((uint8_t)(0xF3 + i)), v);
    }
    wr8(b | 0x10BF, (uint8_t)(0x7C + n * 0x11));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | (uint8_t)(0x7C + n * 0x11));
    cpu.y = rd16(b | (uint16_t)(0x0D75 + n * 2));
    abi_RecordRowDraw();
}

/* SEP #$20, REP #$10, then the time offset to $F2. */
static void record_row_time(uint8_t t)
{
    cpu.p = (uint8_t)((cpu.p | P_M) & ~P_X);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | t);
    wr8(dp(0xF2), t);
}

/*
 * @routine 02:F016 RecordRow1
 * WHAT IT DOES: Draws the first records row (RecordRowDraw) with the first
 * name entry, at Y = $7C from the sprite in $0D75.
 * HOW: record_row 0.
 * Entry: M 8-bit, X 16-bit; $F2 = the time's offset. Exit: as
 * RecordRowDraw.
 */
void abi_RecordRow1(void)
{
    record_row(0);
}

/*
 * @routine 02:F039 RecordRow2
 * WHAT IT DOES: As RecordRow1 for the second row: the second name entry,
 * Y = $8D, the sprite in $0D77.
 * HOW: record_row 1.
 * Entry: as RecordRow1. Exit: as RecordRow1.
 */
void abi_RecordRow2(void)
{
    record_row(1);
}

/*
 * @routine 02:F05C RecordRow3
 * WHAT IT DOES: As RecordRow1 for the third row: the third name entry,
 * Y = $9E, the sprite in $0D79.
 * HOW: record_row 2.
 * Entry: as RecordRow1. Exit: as RecordRow1.
 */
void abi_RecordRow3(void)
{
    record_row(2);
}

/*
 * @routine 02:F00E RecordRow1Time0
 * WHAT IT DOES: RecordRow1 with time offset 0.
 * HOW: Sets $F2 and falls into RecordRow1.
 * Entry: any widths. Exit: as RecordRow1.
 */
void abi_RecordRow1Time0(void)
{
    record_row_time(0x00);
    record_row(0);
}

/*
 * @routine 02:F031 RecordRow2Time4
 * WHAT IT DOES: RecordRow2 with time offset $0C (the second group's).
 * HOW: Sets $F2 and falls into RecordRow2.
 * Entry: any widths. Exit: as RecordRow2.
 */
void abi_RecordRow2Time4(void)
{
    record_row_time(0x0C);
    record_row(1);
}

/*
 * @routine 02:F054 RecordRow3Time8
 * WHAT IT DOES: RecordRow3 with time offset $18 (the third group's).
 * HOW: Sets $F2 and falls into RecordRow3.
 * Entry: any widths. Exit: as RecordRow3.
 */
void abi_RecordRow3Time8(void)
{
    record_row_time(0x18);
    record_row(2);
}

/*
 * @routine 02:EFE2 RecordRowsGroupFirsts
 * WHAT IT DOES: Draws the three records rows, each with the first time of
 * its group (offsets 0, $0C, $18).
 * HOW: RecordRow1Time0, RecordRow2Time4, RecordRow3Time8.
 * Entry: any widths. Exit: as RecordRow3.
 */
void abi_RecordRowsGroupFirsts(void)
{
    abi_RecordRow1Time0();
    abi_RecordRow2Time4();
    abi_RecordRow3Time8();
}

/*
 * @routine 02:EFEC RecordRowsFirstTimes
 * WHAT IT DOES: Draws the three records rows with times 0, 1 and 2
 * (offsets 0, 3, 6).
 * HOW: Sets $F2 before each of RecordRow1, RecordRow2, RecordRow3.
 * Entry: any widths. Exit: as RecordRow3.
 */
void abi_RecordRowsFirstTimes(void)
{
    unsigned n;

    for (n = 0; n < 3; n++) {
        record_row_time((uint8_t)(n * 3));
        record_row(n);
    }
}

void abi_SoundPort0QueuePush(void);

/* $0FA3 = r, then sound $35 on port 0 unless $0E57 is set. */
static void compare_result(uint32_t b, uint8_t r)
{
    uint8_t v;

    cpu.p |= P_M;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | r);
    wr8(b | 0x0FA3, r);
    v = rd8(b | 0x0E57);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    if (v)
        return;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x35);
    abi_SoundPort0QueuePush();
}

/*
 * @routine 07:D6E8 RecordTimeCompare
 * WHAT IT DOES: Compares record time X (BCD minutes, seconds, hundredths
 * at $0F5B + X, unpacked to the five digits at $0F64-$0F68) with the best
 * time's digits at $0F69, most significant first: $0FA3 = $FF when it is
 * better, 1 when worse or when there is no best time ($0F2B = $FFFF), 0
 * when equal; then sound $35 on port 0 unless $0E57 is set.
 * HOW: Nibbles split with AND and LSR (the byte kept on the stack); an
 * 8-bit compare per digit. The no-best, worse and equal results are not
 * reached in the traces so far.
 * Entry: M 16-bit, X 16-bit; X = the time's offset. Exit: M 8-bit; X = the
 * digit the compare stopped at (or X unchanged with no best time); A low
 * byte = $0E57's value, or as SoundPort0QueuePush leaves it.
 */
void abi_RecordTimeCompare(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v, m;
    uint16_t w;

    w = rd16(b | 0x0F2B);
    cpu.a = w;
    op_cmp16(0xFFFF);
    if (w == 0xFFFF) {
        compare_result(b, 0x01);
        return;
    }
    cpu.p |= P_M;
    v = rd8(b | (uint16_t)(0x0F5B + cpu.x));
    wr8(b | 0x0F68, (uint8_t)(v & 0x0F));
    wr8(b | 0x0F67, (uint8_t)(v >> 4));
    v = rd8(b | (uint16_t)(0x0F5C + cpu.x));
    wr8(b | 0x0F66, (uint8_t)(v & 0x0F));
    wr8(b | 0x0F65, (uint8_t)(v >> 4));
    v = rd8(b | (uint16_t)(0x0F5D + cpu.x));
    wr8(b | 0x0F64, (uint8_t)(v & 0x0F));
    for (cpu.x = 0; cpu.x < 5; cpu.x++) {
        v = rd8(b | (uint16_t)(0x0F64 + cpu.x));
        m = rd8(b | (uint16_t)(0x0F69 + cpu.x));
        cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(v - m)) & ~P_C) | (v >= m ? P_C : 0));
        if (v < m) {
            compare_result(b, 0xFF);
            return;
        }
        if (v != m) {
            compare_result(b, 0x01);
            return;
        }
    }
    cpu.p = (uint8_t)((nz16(cpu.p, 0) & ~P_C) | P_C);
    compare_result(b, 0x00);
}

void abi_PlayerSound35(void);

/* By mode $0D2B: 0 sound $35 on port 0, 2 PlayerSound35, else
   RecordTimeCompare on record time x. P restored. */
static void record_compare_by_mode(uint16_t x)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p0 = cpu.p;

    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = rd16(b | 0x0D2B);
    cpu.p = nz16(cpu.p, cpu.a);
    if (cpu.a == 0x0000) {
        cpu.a = 0x0035;
        cpu.p = nz16(cpu.p, cpu.a);
        abi_SoundPort0QueuePush();
    } else {
        op_cmp16(0x0002);
        if (cpu.a == 0x0002)
            abi_PlayerSound35();
        else {
            cpu.x = x;
            cpu.p = nz16(cpu.p, x);
            abi_RecordTimeCompare();
        }
    }
    cpu.p = p0;
}

/*
 * @routine 07:D62C RecordCompareTime0
 * WHAT IT DOES: By mode $0D2B: 0 plays sound $35, 2 PlayerSound35, others
 * compare record time 0 with the best (RecordTimeCompare).
 * HOW: 16-bit mode test.
 * Entry: any widths. Exit: as the call made; P restored.
 */
void abi_RecordCompareTime0(void)
{
    record_compare_by_mode(0x0000);
}

/*
 * @routine 07:D64F RecordCompareTime1
 * WHAT IT DOES: As RecordCompareTime0 for record time 1 (offset 3).
 * HOW: As RecordCompareTime0. The mode 0 sound is not reached in the
 * traces so far.
 * Entry: any widths. Exit: as RecordCompareTime0.
 */
void abi_RecordCompareTime1(void)
{
    record_compare_by_mode(0x0003);
}

/* Swap the words at at + x and at + 2 + x (PHA/PLA through A). */
static void swap_words(uint32_t b, uint16_t at, uint16_t x, int first_read_done, uint16_t first)
{
    uint16_t v;

    if (first_read_done) {
        v = rd16(b | (uint16_t)(at + x));
        wr16(b | (uint16_t)(at + 2 + x), v);
        wr16(b | (uint16_t)(at + x), first);
    } else {
        v = rd16(b | (uint16_t)(at + x));
        wr16(b | (uint16_t)(at + x), rd16(b | (uint16_t)(at + 2 + x)));
        wr16(b | (uint16_t)(at + 2 + x), v);
    }
}

/*
 * @routine 07:B329 RecordsLoadSpecialCourse
 * WHAT IT DOES: For courses $0C-$0F ($0E3D): loads the course's twelve
 * saved entries ($70:FCD0 + course * 48, four bytes each) into the times
 * at $0F2B and the words at $0F43, with the tags from the table at $B4EB
 * (data bank) at $0F13; sorts them so a larger time never follows a
 * smaller one, the three arrays together; then $10C9 = the place (1-3) of
 * tag $10C7 among the first three, or 0.
 * HOW: Direct page $F3 holds the course for course * 3; an insertion pass
 * from the end. The swaps and places other than 1 are not reached in the
 * traces so far.
 * Entry: any widths; D = $0000. Exit: M, X 16-bit; A = the place; X =
 * $FFFE; Y = $18; or, for other courses, A = the course.
 */
/* The course's twelve saved entries (from save offset x) into the times
   at $0F2B, the words at $0F43 and the tags at $0F13, then sorted the
   three arrays together: descending (a larger time first) or ascending. */
static void records_load_sorted(uint32_t b, uint16_t x, int descending)
{
    uint16_t y, v, c;
    int i;

    for (y = 0; y < 0x0018; y = (uint16_t)(y + 2)) {
        wr16(b | (uint16_t)(0x0F2B + y), rd16((uint32_t)(0x70FCD0 + x)));
        wr16(b | (uint16_t)(0x0F43 + y), rd16((uint32_t)(0x70FCD2 + x)));
        wr16(b | (uint16_t)(0x0F13 + y), rd16(b | (uint16_t)(0xB4EB + y)));
        x = (uint16_t)(x + 4);
    }
    for (i = 0x14; i >= 0; i -= 2) {
        x = (uint16_t)i;
        for (;;) {
            v = rd16(b | (uint16_t)(0x0F2D + x));
            c = rd16(b | (uint16_t)(0x0F2B + x));
            if (descending ? v <= c : v >= c)
                break;
            swap_words(b, 0x0F2B, x, 1, v);
            swap_words(b, 0x0F13, x, 0, 0);
            swap_words(b, 0x0F43, x, 0, 0);
            if (x >= 0x0014)
                break;
            x = (uint16_t)(x + 2);
        }
    }
}

/* $10C9 = the place (1-3) of tag $10C7 among the first three, or 0. */
static uint16_t records_player_place(uint32_t b)
{
    static const uint16_t tag_at[3] = {0x0F13, 0x0F15, 0x0F17};
    uint16_t a = rd16(b | 0x10C7), v = 0, t;
    int i;

    for (i = 0; i < 3; i++) {
        t = rd16(b | tag_at[i]);
        cpu.p = (uint8_t)((cpu.p & ~P_C) | (a >= t ? P_C : 0));     /* CMP */
        if (a == t) {
            v = (uint16_t)(i + 1);
            break;
        }
    }
    cpu.p = nz16(cpu.p, v);
    wr16(b | 0x10C9, v);
    return v;
}

/* The save offset of course a's entries: (a * 2 + $F3) * 16, $F3 = a. */
static uint16_t records_offset(uint16_t a)
{
    cpu.p &= (uint8_t)~P_C;
    a = op_adc16((uint16_t)(a << 1), rd16((uint16_t)(cpu.d + 0xF3)), &cpu.p);
    cpu.p = (uint8_t)((cpu.p & ~P_C) | ((a >> 12) & 1));    /* the last ASL */
    return (uint16_t)(a << 4);
}

void abi_RecordsLoadSpecialCourse(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t a;

    cpu.p &= (uint8_t)~(P_M | P_X);
    a = (uint16_t)(rd16(b | 0x0E3D) & 0x00FF);
    wr16((uint16_t)(cpu.d + 0xF3), a);
    cpu.a = a;
    op_cmp16(0x000C);
    if (a < 0x000C)
        return;
    op_cmp16(0x0010);
    if (a >= 0x0010)
        return;
    records_load_sorted(b, records_offset(a), 1);
    cpu.a = records_player_place(b);
    cpu.x = 0xFFFE;
    cpu.y = 0x0018;
}

/* TimeUnpackBcd on A, its Y, X, A (8-bit) stored at `to`, + 1, + 2. */
static void unpack_to(uint32_t b, uint16_t a, uint16_t to)
{
    cpu.p &= (uint8_t)~(P_M | P_X);
    cpu.a = a;
    cpu.p = nz16(cpu.p, a);
    abi_TimeUnpackBcd();
    op_sep(P_M | P_X);
    wr8(b | to, (uint8_t)cpu.y);
    wr8(b | (uint16_t)(to + 1), (uint8_t)cpu.x);
    wr8(b | (uint16_t)(to + 2), (uint8_t)cpu.a);
}

/*
 * @routine 07:B400 RecordsLoadCourse
 * WHAT IT DOES: For a normal course (below $0C): loads its twelve saved
 * entries and sorts them, smallest time first (as RecordsLoadSpecialCourse
 * otherwise); for any course finds the player's place ($10C9); then
 * unpacks the course's two best times ($70:FFD0 + course * 4) and the
 * first time to BCD at $0F5B, $0F5E and $0F61 (TimeUnpackBcd).
 * HOW: Direct page $F3 holds the course. The swaps and places other than
 * 1 are not reached in the traces so far.
 * Entry: any widths; D = $0000. Exit: M 8-bit, X 8-bit; A low byte = the
 * last minutes; X, Y the last seconds and hundredths.
 */
void abi_RecordsLoadCourse(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t a, x;

    cpu.p &= (uint8_t)~(P_M | P_X);
    a = (uint16_t)(rd16(b | 0x0E3D) & 0x00FF);
    wr16((uint16_t)(cpu.d + 0xF3), a);
    cpu.a = a;
    op_cmp16(0x000C);
    if (a < 0x000C)
        records_load_sorted(b, records_offset(a), 0);
    records_player_place(b);
    x = (uint16_t)((rd16(b | 0x0E3D) & 0x00FF) << 2);
    unpack_to(b, rd16((uint32_t)(0x70FFD0 + x)), 0x0F5B);
    unpack_to(b, rd16((uint32_t)(0x70FFD2 + x)), 0x0F5E);
    unpack_to(b, rd16(b | 0x0F2B), 0x0F61);
}

/*
 * @routine 07:B3D4 RecordsLoadForCourse
 * WHAT IT DOES: Loads course $0E3D's records: RecordsLoadSpecialCourse for
 * courses $0C-$0F, RecordsLoadCourse below $0C, nothing above.
 * HOW: The data bank is set to $07 (the tag table's) and restored.
 * Entry: any widths; D = $0000. Exit: P and the data bank restored.
 */
void abi_RecordsLoadForCourse(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db, v;

    cpu.p |= P_M;
    cpu.db = 0x07;
    v = rd8(0x070000 | 0x0E3D);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    if (v < 0x0C)
        abi_RecordsLoadCourse();
    else if (v < 0x10)
        abi_RecordsLoadSpecialCourse();
    cpu.db = db0;
    op_plp(p0);
}

/*
 * @routine 07:B3F2 RecordsLoadCourseLong
 * WHAT IT DOES: RecordsLoadCourse for callers in other banks, with the
 * data bank $07.
 * HOW: As RecordsLoadForCourse. Not reached in the traces so far.
 * Entry: any widths; D = $0000. Exit: P and the data bank restored.
 */
void abi_RecordsLoadCourseLong(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db;

    cpu.p |= P_M;
    cpu.db = 0x07;
    abi_RecordsLoadCourse();
    cpu.db = db0;
    op_plp(p0);
}

/*
 * @routine 07:B077 RecordCarTimes12
 * WHAT IT DOES: A = $0D89 * 12, low byte only.
 * HOW: ASL, CLC, ADC $0D89, ASL, ASL, AND #$FF (16-bit); through the data
 * bank.
 * Entry: M 16-bit. Exit: A = the value; N, Z from it; C from the last
 * shift; V from the add.
 */
void abi_RecordCarTimes12(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_lda16(b | 0x0D89);
    op_asl16();
    op_add16(rd16(b | 0x0D89));
    op_asl16();
    op_asl16();
    op_and16(0x00FF);
}
