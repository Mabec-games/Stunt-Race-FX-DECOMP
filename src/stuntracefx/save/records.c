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

/*
 * @routine 07:ADDA SaveSumsRecordsB
 * WHAT IT DOES: Keeps the sums of the first three records at $70:FC5E
 * (SaveRecordSumB) in the sum bytes $70:FC43-$70:FC45, after clearing
 * direct page $F1; then returns the fourth record's sum.
 * HOW: SEP #$30; three JSRs, then SaveRecordSumB's code, which follows.
 * Entry: D = $0000. Exit: as SaveRecordSumB, X = $14.
 */
void abi_SaveSumsRecordsB(void)
{
    unsigned i;

    op_sep(P_M | P_X);
    wr8(0x0000F1, 0x00);
    for (i = 0; i < 3; i++) {
        cpu.x = (uint16_t)(i * 5);
        cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
        abi_SaveRecordSumB();
        wr8((uint32_t)(0x70FC43 + i), (uint8_t)cpu.a);
    }
    abi_SaveRecordSumB();
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

/* One byte of a check's running sum: the byte at `at` added to $F1. */
static void sum_byte(uint32_t at, uint8_t *p)
{
    uint8_t v = rd8(at), s;

    s = rd8(0x0000F1);
    s = add8(v, s, p);
    wr8(0x0000F1, s);
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
    uint16_t base, w, x;
    uint8_t v, s, blk;
    unsigned i;

    wr16(0x0000F3, 0x000C);
    do {
        w = rd16(0x0000F3);
        base = (uint16_t)(w << 1);
        w = rd16(0x0000F3);
        base = (uint16_t)((uint16_t)(base + w) << 4);
        wr16(0x0000F5, base);
        wr16(0x0000F1, 0x0000);
        for (i = 0, x = base; i < 12; i++, x += 4) {
            sum_byte(b | (uint16_t)(0xFCD0 + x), p);
            v = rd8(b | (uint16_t)(0xFCD1 + x));
            if (v >= 0x41)
                goto reset;
            s = rd8(0x0000F1);
            s = add8(v, s, p);
            wr8(0x0000F1, s);
            sum_byte(b | (uint16_t)(0xFCD2 + x), p);
            sum_byte(b | (uint16_t)(0xFCD3 + x), p);
        }
        x = rd16(0x0000F3);
        v = rd8(b | (uint16_t)(0xFC33 + x));
        s = rd8(0x0000F1);
        if (v == s)
            goto next;
reset:
        x = rd16(0x0000F5);
        for (i = 0; i < 12; i++, x += 4) {
            wr8(b | (uint16_t)(0xFCD0 + x), 0x00);
            wr8(b | (uint16_t)(0xFCD1 + x), 0x00);
            wr8(b | (uint16_t)(0xFCD2 + x), 0xFF);
            wr8(b | (uint16_t)(0xFCD3 + x), 0xFF);
        }
        x = rd16(0x0000F3);
        wr8(b | (uint16_t)(0xFC33 + x), 0xE8);
next:
        blk = rd8(0x0000F3);
        wr8(0x0000F3, (uint8_t)(blk + 1));
        blk = rd8(0x0000F3);
    } while (blk < 0x10);
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
    int k, i;

    wr8(0x0000F4, 0x00);
    wr8(0x0000F6, 0x00);
    for (k = 2; k >= 0; k--) {
        wr8(0x0000F3, first[k]);
        wr8(0x0000F5, second[k]);
        wr8(0x0000F1, 0x00);
        x = rd16(0x0000F3);
        for (i = 0; i < 12; i++, x += 2) {
            v = rd8(b | (uint16_t)(0xFC88 + x));
            sum = rd8(0x0000F1);
            sum = add8(v, sum, p);
            wr8(0x0000F1, sum);
            v = rd8(b | (uint16_t)(0xFC89 + x));
            if (v < 0x1F) {
                *p &= (uint8_t)~P_C;
                goto reset;
            }
            sum = rd8(0x0000F1);
            sum = add8(v, sum, p);
            wr8(0x0000F1, sum);
        }
        x = rd16(0x0000F5);
        for (i = 0; i < 9; i++, x++) {
            v = rd8(b | (uint16_t)(0xFC6D + x));
            sum = rd8(0x0000F1);
            sum = add8(v, sum, p);
            wr8(0x0000F1, sum);
        }
        a = rd8(b | (uint16_t)(0xFC2D + k));
        sum = rd8(0x0000F1);
        *p = (uint8_t)((*p & ~P_C) | (a >= sum ? P_C : 0));
        if (a == sum)
            continue;
reset:
        x = rd16(0x0000F3);
        for (i = 0; i < 24; i++, x++)
            wr8(b | (uint16_t)(0xFC88 + x), 0xFF);
        x = rd16(0x0000F5);
        for (i = 0; i < 9; i++, x++)
            wr8(b | (uint16_t)(0xFC6D + x), 0x00);
        a = 0xE8;
        wr8(0x70FC2D + k, a);
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
/* From the top, or (from_bad) at the fourth entry byte's failure, which
   cannot happen: the block in $F3 reset, its offset from $F5. */
static void save_blocks_validate(uint8_t bank, uint8_t *p, int from_bad)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t w, x, y;
    uint8_t v, s, blk;
    int i, j;

    if (from_bad)
        goto reset;
    wr16(0x0000F3, 0x0000);
    for (;;) {
        w = rd16(0x0000F3);
        x = (uint16_t)(w << 2);
        v = rd8(0x70FFD0 + x);
        wr8(0x0000F1, v);
        for (j = 1; j < 4; j++) {
            v = rd8(0x70FFD0 + x + j);
            s = rd8(0x0000F1);
            s = add8(v, s, p);
            if (j < 3)
                wr8(0x0000F1, s);
        }
        wr16(0x0000F1, s);
        w = rd16(0x0000F3);
        x = (uint16_t)(w << 1);
        w = rd16(0x0000F3);
        x = (uint16_t)((uint16_t)(x + w) << 4);
        wr16(0x0000F5, x);
        for (i = 0; i < 12; i++, x += 4) {
            for (j = 0; j < 4; j++) {
                v = rd8(b | (uint16_t)(0xFCD0 + x + j));
                if (entry_byte_bad(v, j))
                    goto reset;
                s = rd8(0x0000F1);
                s = add8(v, s, p);
                wr8(0x0000F1, s);
            }
        }
        x = rd16(0x0000F3);
        v = rd8(b | (uint16_t)(0xFC33 + x));
        s = rd8(0x0000F1);
        if (v != s)
            goto reset;
        blk = rd8(0x0000F3);
        wr8(0x0000F3, (uint8_t)(blk + 1));
        blk = rd8(0x0000F3);
        if (blk >= 0x0C)
            return;
        continue;
reset:
        wr8(0x001FFE, 0x11);
        x = rd16(0x0000F5);
        for (i = 0; i < 48; i++, x++)
            wr8(b | (uint16_t)(0xFCD0 + x), 0xFF);
        x = rd16(0x0000F3);
        w = rd16(0x0000F3);
        y = (uint16_t)((w & 0x00FF) << 2);
        for (j = 0; j < 4; j++)
            wr8(b | (uint16_t)(0xFFD0 + y + j), 0xFF);
        wr8(b | (uint16_t)(0xFC33 + x), 0xCC);
    }
}

void SaveBlocksValidate(uint8_t bank, uint8_t *p)
{
    save_blocks_validate(bank, p, 0);
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
 * @routine 07:B2B8 SaveBlocksFourthBad
 * WHAT IT DOES: SaveBlocksValidate's failure of an entry's fourth byte,
 * which cannot happen (its low two bits are never $38 or more): the block
 * $F3 is reset as for any failure and checked again, then the rest.
 * HOW: Pulls the byte the test pushed, then as SaveBlocksValidate's reset.
 * Entry: M 8-bit, X 16-bit, D = $0000; DB = $70; $F3 = the block, $F5 its
 * offset; the entry byte on the stack above the return address. Exit: as
 * SaveBlocksValidate.
 */
void abi_SaveBlocksFourthBad(void)
{
    uint8_t p = cpu.p;

    cpu.s++;
    save_blocks_validate(cpu.db, &p, 1);
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

void abi_TimeUnpackBcdLong(void);

/*
 * @routine 02:FD89 RecordTagCarSet
 * WHAT IT DOES: Turns a record's tag byte (A, its time offset 0-$2F) into
 * the car's piece set: 0, 3, 6 or 9 for each block of $0C.
 * HOW: 8-bit compares from the top ($24, $18, $0C). Not reached in the
 * traces so far.
 * Entry: any widths; A low byte = the tag. Exit: A low byte = the set, the
 * high byte kept; M 16-bit; N clear, Z set for set 0; C set unless the tag
 * is below $0C.
 */
void abi_RecordTagCarSet(void)
{
    uint8_t t = (uint8_t)cpu.a, v;

    op_sep(P_M);
    if (t >= 0x24)
        v = 0x09;
    else if (t >= 0x18)
        v = 0x06;
    else if (t >= 0x0C)
        v = 0x03;
    else
        v = 0x00;
    cpu.p = (uint8_t)((nz8(cpu.p, v) & ~P_C) | (t >= 0x0C ? P_C : 0));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    op_rep(P_M);
}

/* The time in A unpacked (TimeUnpackBcdLong) with Y kept: the hundredths
   to direct page $F1 and the seconds to $F3 (words, X 16-bit), then
   SEP #$20 with A = the minutes. */
static void record_time_unpack(void)
{
    uint16_t y = cpu.y;

    abi_TimeUnpackBcdLong();
    wr16(dp(0xF1), cpu.y);
    wr16(dp(0xF3), cpu.x);
    cpu.y = y;
    cpu.p = nz16(cpu.p, y);
    op_sep(P_M);
}

/* Large digit d (0-9) at sprite buffer offset `at` + Y: its top tile $50 +
   d and its bottom tile $60 + d four bytes on, as CLC / ADC #$50 then ADC
   #$10 leave them. */
static void large_digit(uint32_t b, uint16_t at, uint8_t d)
{
    uint8_t p = (uint8_t)(cpu.p & ~P_C), v;

    v = op_adc8(d, 0x50, &p);
    wr8(b | (uint16_t)(at + cpu.y), v);
    v = op_adc8(v, 0x10, &p);
    wr8(b | (uint16_t)(at + 4 + cpu.y), v);
    cpu.p = p;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/*
 * @routine 02:FDA8 RecordTimeDigitsLarge
 * WHAT IT DOES: Draws the packed time A as five large digits (two tiles
 * each) from sprite buffer offset Y: minutes into sprite Y / 4 + 4,
 * seconds into + 8 and + 10, hundredths into + 14 and + 16.
 * HOW: TimeUnpackBcdLong, then each BCD nibble + $50 and + $60 to the tile
 * bytes. Not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the packed time, Y = sprite
 * buffer offset. Exit: P as on entry (PLP); A low byte = the last bottom
 * tile, high byte the minutes'; X = the seconds (BCD); Y kept; $F1 = the
 * hundredths, $F3 = the seconds.
 */
void abi_RecordTimeDigitsLarge(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t p = cpu.p, v;

    record_time_unpack();
    large_digit(b, 0x0387, (uint8_t)(cpu.a & 0x0F));
    v = rd8(dp(0xF3));
    large_digit(b, 0x0397, (uint8_t)(v >> 4));
    v = rd8(dp(0xF3));
    large_digit(b, 0x039F, (uint8_t)(v & 0x0F));
    v = rd8(dp(0xF1));
    large_digit(b, 0x03AF, (uint8_t)(v >> 4));
    v = rd8(dp(0xF1));
    large_digit(b, 0x03B7, (uint8_t)(v & 0x0F));
    op_plp(p);
}

/* SEP'd A low byte to `v`, N and Z from it (an 8-bit LDA, AND or LSR). */
static void set_a8(uint8_t v)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
}

/* LDA dp, LSR x 4: the high nibble, C = bit 3. */
static uint8_t dp_high_nibble(uint8_t a)
{
    uint8_t v = rd8(dp(a));

    cpu.p = (uint8_t)((cpu.p & ~P_C) | ((v >> 3) & 1));
    set_a8((uint8_t)(v >> 4));
    return (uint8_t)(v >> 4);
}

/*
 * @routine 02:FE04 RecordTimeDigitsSmall
 * WHAT IT DOES: Draws the packed time A as five small digits from sprite
 * buffer offset Y: minutes into sprite Y / 4 + 3, seconds into + 5 and +
 * 6, hundredths into + 8 and + 9.
 * HOW: TimeUnpackBcdLong, then each BCD nibble to a tile byte. Not reached
 * in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the packed time, Y = sprite
 * buffer offset. Exit: A low byte = the hundredths' low digit, high byte
 * the minutes'; X = the seconds (BCD); Y kept; M 16-bit; N clear, Z from
 * the digit, C = bit 3 of the hundredths; $F1, $F3 as RecordTimeDigitsLarge.
 */
void abi_RecordTimeDigitsSmall(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t v;

    record_time_unpack();
    v = (uint8_t)(cpu.a & 0x0F);
    set_a8(v);
    wr8(b | (uint16_t)(0x0383 + cpu.y), v);
    wr8(b | (uint16_t)(0x038B + cpu.y), dp_high_nibble(0xF3));
    v = (uint8_t)(rd8(dp(0xF3)) & 0x0F);
    set_a8(v);
    wr8(b | (uint16_t)(0x038F + cpu.y), v);
    wr8(b | (uint16_t)(0x0397 + cpu.y), dp_high_nibble(0xF1));
    v = (uint8_t)(rd8(dp(0xF1)) & 0x0F);
    set_a8(v);
    wr8(b | (uint16_t)(0x039B + cpu.y), v);
    op_rep(P_M);
}

/*
 * @routine 02:FE38 RecordScoreDigits
 * WHAT IT DOES: Draws a Stunt Trax score A from sprite buffer offset Y:
 * below $4000 its four BCD digits into sprites Y / 4 + 4, + 5, + 10 and +
 * 11 with sprites + 3 and + 9 hidden; from $4000 on, the low byte's two
 * digits and the word PERFECT (sprites + 3 to + 9).
 * HOW: The score kept in direct page $F1; 8-bit digit stores; PERFECT as
 * seven tile and attribute words ($36 attributes). Not reached in the
 * traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the score, Y = sprite buffer
 * offset. Exit: below $4000: A low byte $F0, M 16-bit, N set, Z clear, C
 * = bit 11 of the score; from $4000: A = $361D, N and Z clear, C = bit 3
 * of the score.
 */
void abi_RecordScoreDigits(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t s = cpu.a;
    uint8_t v;
    unsigned i;
    static const uint16_t perfect[7] = {0x3619, 0x360E, 0x361B, 0x360F, 0x360E, 0x360C, 0x361D};

    wr16(dp(0xF1), s);
    op_cmp16(0x4000);
    op_sep(P_M);
    v = (uint8_t)(s & 0x0F);
    set_a8(v);
    wr8(b | (uint16_t)(0x03A3 + cpu.y), v);
    if (s >= 0x4000) {
        v = (uint8_t)s;
        cpu.p = (uint8_t)((cpu.p & ~P_C) | ((v >> 3) & 1));
        set_a8((uint8_t)(v >> 4));
        wr8(b | (uint16_t)(0x039F + cpu.y), (uint8_t)(v >> 4));
        op_rep(P_M);
        for (i = 0; i < 7; i++)
            wr16(b | (uint16_t)(0x0383 + i * 4 + cpu.y), perfect[i]);
        cpu.a = perfect[6];
        cpu.p = nz16(cpu.p, cpu.a);
        return;
    }
    wr8(b | (uint16_t)(0x039F + cpu.y), dp_high_nibble(0xF1));
    v = (uint8_t)(rd8(dp(0xF2)) & 0x0F);
    set_a8(v);
    wr8(b | (uint16_t)(0x038B + cpu.y), v);
    wr8(b | (uint16_t)(0x0387 + cpu.y), dp_high_nibble(0xF2));
    set_a8(0xF0);
    wr8(b | (uint16_t)(0x0382 + cpu.y), 0xF0);
    wr8(b | (uint16_t)(0x039A + cpu.y), 0xF0);
    op_rep(P_M);
}

/*
 * @routine 02:FEA7 RecordLetterTile
 * WHAT IT DOES: The top tile of large letter A (0-31, plus $0A: letters
 * follow the ten digits): $50 + the low four bits, + $20 for each of the
 * next two bits (the sheet's rows of 16, each two tiles high).
 * HOW: The letter + $0A in direct page $F1, its bits 4-5 doubled in $F3.
 * Not reached in the traces so far.
 * Entry: M 16-bit, D = $0000; A = the letter. Exit: A = the tile; N, Z, C,
 * V from the last add (C and V clear).
 */
void abi_RecordLetterTile(void)
{
    uint16_t v;

    op_add16(0x000A);
    wr16(dp(0xF1), cpu.a);
    op_and16(0x0030);
    op_asl16();
    wr16(dp(0xF3), cpu.a);
    op_lda16(dp(0xF1));
    op_and16(0x000F);
    v = rd16(dp(0xF3));
    cpu.a |= v;
    op_add16(0x0050);
}

/* RecordLetterTile for letter `n`, then its top tile at `at` + Y and its
   bottom tile (+ $10, with the add's carry) four bytes on. */
static void large_letter(uint32_t b, uint16_t at, uint16_t n)
{
    uint8_t p, v;

    cpu.a = n;
    cpu.p = nz16(cpu.p, n);
    abi_RecordLetterTile();
    op_sep(P_M);
    v = (uint8_t)cpu.a;
    wr8(b | (uint16_t)(at + cpu.y), v);
    p = cpu.p;
    v = op_adc8(v, 0x10, &p);
    cpu.p = p;
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    wr8(b | (uint16_t)(at + 4 + cpu.y), v);
    op_rep(P_M);
}

/*
 * @routine 02:FEBF RecordNameLettersLarge
 * WHAT IT DOES: Draws the three 5-bit letters of A (bits 0-4, 5-9, 10-14)
 * as large letters (two tiles each) into sprites Y / 4 + 18, + 20 and +
 * 22.
 * HOW: RecordLetterTile for each; the name word is kept on the stack
 * between them. Not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit, D = $0000; A = the letters, Y = sprite buffer
 * offset. Exit: A = the third letter's bottom tile; M 16-bit; N, Z, C, V
 * from its 8-bit add.
 */
void abi_RecordNameLettersLarge(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v = cpu.a;

    large_letter(b, 0x03BF, (uint16_t)(v & 0x001F));
    large_letter(b, 0x03C7, (uint16_t)(((uint16_t)(v << 3) >> 8) & 0x001F));
    large_letter(b, 0x03CF, (uint16_t)(((uint16_t)(v >> 2) >> 8) & 0x001F));
}

/*
 * @routine 02:FF37 RecordNameLettersSmall
 * WHAT IT DOES: As RecordNameLetters into sprites Y / 4 + 15, + 16 and +
 * 17.
 * HOW: As RecordNameLetters. Not reached in the traces so far.
 * Entry: as RecordNameLetters. Exit: as RecordNameLetters.
 */
void abi_RecordNameLettersSmall(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t v = cpu.a;
    unsigned i;
    static const uint8_t shift[3] = {0, 5, 10};

    for (i = 0; i < 3; i++) {
        cpu.a = (uint16_t)(((v >> shift[i]) & 0x001F) + 0x000A);
        wr8(b | (uint16_t)(0x03B3 + i * 4 + cpu.y), (uint8_t)cpu.a);
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

/*
 * @routine 07:B31B RecordsLoadSpecialCourseLong
 * WHAT IT DOES: RecordsLoadSpecialCourse for a JSL caller
 * (StuntPanelWait): with data bank $07, P and the data bank kept.
 * HOW: PHP, SEP #$20, PHB, PHK, PLB, JSR; then PLB, PLP, RTL.
 * Entry: any widths; D = $0000. Exit: P and the data bank unchanged; A,
 * X, Y as RecordsLoadSpecialCourse leaves them.
 */
void abi_RecordsLoadSpecialCourseLong(void)
{
    uint8_t p0 = cpu.p, db0 = cpu.db;

    op_sep(P_M);
    cpu.db = 0x07;
    cpu.p = nz8(cpu.p, 0x07);
    abi_RecordsLoadSpecialCourse();
    op_sep(P_M);
    cpu.db = db0;
    op_plp(p0);
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

void abi_RecordTagCar0(void);
void abi_RecordTagCar4(void);
void abi_RecordTagCar8(void);
void abi_TimePackBcd(void);

#define SPECIAL_SAVES 0x70FCD0u
#define SPECIAL_LAPS 0x70FFD0u

/* A direct page address (bank 0). */
static uint32_t special_dp(uint8_t off)
{
    return (uint16_t)(cpu.d + off);
}

/* CPX (16-bit): N, Z and C. */
static void special_cpx(uint16_t m)
{
    cpu.p = (uint8_t)((nz16(cpu.p, (uint16_t)(cpu.x - m)) & ~P_C) | (cpu.x >= m ? P_C : 0));
}

/* CLC; ADC (8-bit) of a direct page byte. */
static void special_add8(uint8_t off)
{
    uint8_t v;

    cpu.p &= (uint8_t)~P_C;
    v = op_adc8((uint8_t)cpu.a, rd8(special_dp(off)), &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
}

/*
 * @routine 07:AFFC SpecialRecordsChecksum
 * WHAT IT DOES: The check sum of a special course's records: the bytes of
 * its four packed lap times at $70:FFD0 + course * 4 (0 for course $0C or
 * more) and its 48 bytes from $70:FCD0 + $0D14, summed with carries, at
 * $70:FC33 + course.
 * HOW: 8-bit sums through $F1 (direct page), X 8-bit for the store.
 * Entry: M 16-bit, X 16-bit; $0D14 = the course's offset. Exit: A = $00 and
 * the low byte of the sum; X = the course, Y = 0; M 16-bit, X 16-bit; N, Z
 * from the last count; C, V from the last add.
 */
void abi_SpecialRecordsChecksum(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    int i;

    op_lda16(b | 0x0E3D);
    op_and16(0x00FF);
    op_cmp16(0x000C);
    if (cpu.p & P_C) {
        op_sep(P_M);
        cpu.a &= 0xFF00;
        cpu.p = nz8(cpu.p, 0x00);
    } else {
        op_asl16();
        op_asl16();
        op_tax16();
        op_sep(P_M);
        op_lda8(SPECIAL_LAPS + cpu.x);
        for (i = 1; i < 4; i++) {
            wr8(special_dp(0xF1), (uint8_t)cpu.a);
            op_lda8(SPECIAL_LAPS + i + cpu.x);
            special_add8(0xF1);
        }
    }
    wr8(special_dp(0xF1), (uint8_t)cpu.a);
    cpu.y = 0x0030;
    cpu.p = nz16(cpu.p, cpu.y);
    cpu.x = rd16(b | 0x0D14);
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.p &= (uint8_t)~P_C;
    do {
        op_lda8(SPECIAL_SAVES + cpu.x);
        special_add8(0xF1);
        wr8(special_dp(0xF1), (uint8_t)cpu.a);
        cpu.x++;
        cpu.p = nz16(cpu.p, cpu.x);
        cpu.y--;
        cpu.p = nz16(cpu.p, cpu.y);
    } while (cpu.y);
    op_sep(P_X);
    cpu.x = rd8(b | 0x0E3D);
    cpu.p = nz8(cpu.p, (uint8_t)cpu.x);
    op_lda8(special_dp(0xF1));
    wr8(0x70FC33 + cpu.x, (uint8_t)cpu.a);
    op_rep(P_M | P_X);
}

/* Entry +a's word to entry +b (the course's entries, from X). */
static void special_move(uint16_t from, uint16_t to)
{
    op_lda16(SPECIAL_SAVES + from + cpu.x);
    wr16(SPECIAL_SAVES + to + cpu.x, cpu.a);
}

/* LDA abs, LDX abs, LDY abs; TimePackBcd: one lap's packed time. */
static uint16_t special_pack(uint32_t b, uint16_t at)
{
    op_lda16(b | at);
    cpu.x = rd16(b | (uint16_t)(at + 1));
    cpu.p = nz16(cpu.p, cpu.x);
    cpu.y = rd16(b | (uint16_t)(at + 2));
    cpu.p = nz16(cpu.p, cpu.y);
    abi_TimePackBcd();
    return cpu.a;
}

/*
 * @routine 07:AEF3 SpecialRecordInsert
 * WHAT IT DOES: Puts time A into a special course's three best times for
 * car $0D89, at $70:FCD0 + course * 48 + car * 12 (each entry the time and
 * the player's level $0F11): a time below the first moves the others down
 * (the third lost) and tags the car's first place (RecordTagCar0), then,
 * unless the time is still above another of the course's entries, keeps
 * the lap's packed times (TimePackBcd of $0EF0-$0EF2 and $0EF3-$0EF5) at
 * $70:FFD0 + course * 4; below the second or third likewise (RecordTagCar4,
 * RecordTagCar8); no place: the tag $10C7 = $FFFF. Then the course's check
 * sum (SpecialRecordsChecksum). First, with HUD mode $0D2B at 0, the word at
 * $0F6E + $0D48 * 2 is set to it. Course $0C or more, or car 4 or more:
 * nothing else.
 * HOW: $F1, $F3, $F5 and $F7 (direct page) hold the course's offset, the
 * car, a loop count and the entry's offset ($0D14 keeps the course's).
 * Entry: M 16-bit, X 16-bit; A = the time. Exit: as SpecialRecordsChecksum
 * leaves it; or A = $FFFF; or, for no special course or car, A = the time
 * and N, Z from it.
 */
void abi_SpecialRecordInsert(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t t = cpu.a, r1, r2, n;

    op_lda16(b | 0x0D2B);
    if (!cpu.a) {
        op_lda16(b | 0x0D48);
        op_asl16();
        op_and16(0x00FF);
        op_tax16();
        wr16(b + 0x0F6E + cpu.x, cpu.a);
    }
    op_lda16(b | 0x0E3D);
    op_and16(0x00FF);
    op_cmp16(0x000C);
    if (cpu.p & P_C)
        goto none;
    wr16(special_dp(0xF1), cpu.a);
    op_asl16();
    op_add16(rd16(special_dp(0xF1)));
    op_asl16();
    op_asl16();
    op_asl16();
    op_asl16();
    wr16(special_dp(0xF1), cpu.a);
    wr16(b | 0x0D14, cpu.a);
    op_tay16();
    op_lda16(b | 0x0D89);
    op_and16(0x00FF);
    op_cmp16(0x0004);
    if (cpu.p & P_C)
        goto none;
    wr16(special_dp(0xF3), cpu.a);
    op_asl16();
    op_add16(rd16(special_dp(0xF3)));
    op_asl16();
    op_asl16();
    op_add16(rd16(special_dp(0xF1)));
    op_tax16();
    wr16(special_dp(0xF7), cpu.a);
    cpu.a = t;
    cpu.p = nz16(cpu.p, t);
    op_cmp16(rd16(SPECIAL_SAVES + cpu.x));
    if (!(cpu.p & P_C)) {
        abi_RecordTagCar0();
        special_move(4, 8);
        special_move(6, 10);
        special_move(0, 4);
        special_move(2, 6);
        op_lda16(b | 0x0F11);
        wr16(SPECIAL_SAVES + 2 + cpu.x, cpu.a);
        cpu.a = t;
        cpu.p = nz16(cpu.p, t);
        wr16(SPECIAL_SAVES + cpu.x, t);
        wr16(special_dp(0xF1), cpu.x);
        cpu.x = 0x000C;
        cpu.p = nz16(cpu.p, cpu.x);
        wr16(special_dp(0xF5), cpu.x);
        cpu.x = cpu.y;
        cpu.p = nz16(cpu.p, cpu.x);
        do {
            special_cpx(rd16(special_dp(0xF1)));
            if (!(cpu.p & P_Z)) {
                op_cmp16(rd16(SPECIAL_SAVES + cpu.x));
                if (cpu.p & P_C)
                    goto checksum;
            }
            cpu.x = (uint16_t)(cpu.x + 4);
            cpu.p = nz16(cpu.p, cpu.x);
            n = (uint16_t)(rd16(special_dp(0xF5)) - 1);
            wr16(special_dp(0xF5), n);
            cpu.p = nz16(cpu.p, n);
        } while (n);
        r1 = special_pack(b, 0x0EF0);
        r2 = special_pack(b, 0x0EF3);
        op_lda16(b | 0x0E3D);
        op_and16(0x00FF);
        op_asl16();
        op_asl16();
        op_tax16();
        cpu.a = r2;
        cpu.p = nz16(cpu.p, r2);
        wr16(SPECIAL_LAPS + 2 + cpu.x, r2);
        cpu.a = r1;
        cpu.p = nz16(cpu.p, r1);
        wr16(SPECIAL_LAPS + cpu.x, r1);
        goto checksum;
    }
    op_cmp16(rd16(SPECIAL_SAVES + 4 + cpu.x));
    if (!(cpu.p & P_C)) {
        abi_RecordTagCar4();
        special_move(4, 8);
        special_move(6, 10);
        cpu.a = t;
        cpu.p = nz16(cpu.p, t);
        wr16(SPECIAL_SAVES + 4 + cpu.x, t);
        op_lda16(b | 0x0F11);
        wr16(SPECIAL_SAVES + 6 + cpu.x, cpu.a);
        goto checksum;
    }
    op_cmp16(rd16(SPECIAL_SAVES + 8 + cpu.x));
    if (cpu.p & P_C) {
        op_rep(P_M | P_X);
        op_lda16_imm(0xFFFF);
        wr16(b | 0x10C7, 0xFFFF);
        return;
    }
    wr16(SPECIAL_SAVES + 8 + cpu.x, t);
    abi_RecordTagCar8();
    op_lda16(b | 0x0F11);
    wr16(SPECIAL_SAVES + 10 + cpu.x, cpu.a);
checksum:
    abi_SpecialRecordsChecksum();
    return;
none:
    cpu.a = t;
    cpu.p = nz16(cpu.p, t);
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

void abi_RecordCarTimes12(void);

/* JSR RecordCarTimes12, (CLC, ADC #extra,) STA $10C7. */
static void record_tag(uint16_t extra)
{
    abi_RecordCarTimes12();
    if (extra)
        op_add16(extra);
    wr16((uint32_t)cpu.db << 16 | 0x10C7, cpu.a);
}

/*
 * @routine 07:B05E RecordTagCar0
 * WHAT IT DOES: Sets the tag $10C7 to $0D89 * 12 (low byte), the place of
 * the car's first record.
 * HOW: RecordCarTimes12; the entries at $07:B063 and $07:B06C add 4 and 8.
 * Entry: M 16-bit. Exit: A = the tag; N, Z from RecordCarTimes12's AND.
 */
void abi_RecordTagCar0(void)
{
    record_tag(0);
}

/*
 * @routine 07:B063 RecordTagCar4
 * WHAT IT DOES: As RecordTagCar0 plus 4 (the second record).
 * HOW: As RecordTagCar0 with CLC, ADC #4; no script reaches it (unit_cpu
 * does).
 * Entry: M 16-bit. Exit: A = the tag; N, Z, C, V from the add.
 */
void abi_RecordTagCar4(void)
{
    record_tag(0x0004);
}

/*
 * @routine 07:B06C RecordTagCar8
 * WHAT IT DOES: As RecordTagCar0 plus 8 (the third record).
 * HOW: As RecordTagCar4 with 8; no script reaches it (unit_cpu does).
 * Entry: M 16-bit. Exit: as RecordTagCar4.
 */
void abi_RecordTagCar8(void)
{
    record_tag(0x0008);
}
/*
 * @routine 07:ADA3 SaveRecordSumsTimeUp
 * WHAT IT DOES: The sums of the four five-byte time-up records from
 * $70:FC4A (SaveRecordSumA at offsets 0, 5, 10, 15) into
 * $70:FC46-$70:FC49; $F1 cleared first.
 * HOW: 8-bit registers.
 * Entry: any widths, D = $0000. Exit: M 8-bit, X 8-bit; A low byte = the
 * last sum; X = $14, Y = 0; Z set, N clear; C, V from the last add.
 */
void abi_SaveRecordSumsTimeUp(void)
{
    static const uint8_t at[4] = {0x00, 0x05, 0x0A, 0x0F};
    uint8_t x, p, v;
    int i;

    op_sep(P_M | P_X);
    wr8(0x0000F1, 0x00);
    for (i = 0; i < 4; i++) {
        x = at[i];
        p = cpu.p;
        v = SaveRecordSumA(&x, &p);
        cpu.p = p;
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.x = x;
        cpu.y = 0;
        wr8(0x70FC46 + (uint32_t)i, v);
    }
}

/* SEP #$20, LDA #v, STA.L $000FA3: player 1's lead byte. */
static void time_up_lead(uint8_t v)
{
    op_sep(P_M);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(0x000FA3, v);
}

/*
 * @routine 07:AD1B TimeUpRecordUpdate
 * WHAT IT DOES: The time-up run's end: its time ($0E66 minutes, $0E64
 * seconds, $0E63 hundredths, BCD) as five digits at $0F69, then packed
 * (TimePackBcd) and set against the record for the course ($0E3D - $17,
 * five bytes each from $70:FC4A): lower, it becomes the record with $0F11
 * and the car $0D89, $0FA3 = $FF and the sums made again
 * (SaveRecordSumsTimeUp); higher, $0FA3 = 1; equal, $0FA3 = 0.
 * HOW: 8-bit digits through the data bank; the compare 16-bit; the record
 * by long stores; $F1 holds the course for course * 5.
 * Entry: any widths; data bank $07, D = $0000. Exit: M 8-bit; X the
 * record's offset (or as the sums leave it); A low byte = $0FA3's value.
 */
void abi_TimeUpRecordUpdate(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t t, k, x, r;
    uint8_t v, p;

    op_sep(P_M);
    v = rd8(b | 0x0E66);
    wr8(b | 0x0F69, (uint8_t)(v & 0x0F));
    v = rd8(b | 0x0E64);
    wr8(b | 0x0F6A, (uint8_t)(v >> 4));
    v = rd8(b | 0x0E64);
    wr8(b | 0x0F6B, (uint8_t)(v & 0x0F));
    v = rd8(b | 0x0E63);
    wr8(b | 0x0F6C, (uint8_t)(v >> 4));
    v = rd8(b | 0x0E63);
    v = (uint8_t)(v & 0x0F);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = nz8(cpu.p, v);
    wr8(b | 0x0F6D, v);
    op_rep(P_M | P_X);
    op_lda16(b | 0x0E63);
    cpu.x = rd16(b | 0x0E64);
    cpu.y = rd16(b | 0x0E66);
    abi_TimePackBcd();
    t = cpu.a;
    op_lda16(b | 0x0E3D);
    p = (uint8_t)(cpu.p | P_C);
    k = op_sbc16(cpu.a, 0x0017, &p);
    wr16(0x0000F1, k);
    k = (uint16_t)(k << 2);
    p = (uint8_t)(p & ~P_C);
    k = op_adc16(k, rd16(0x0000F1), &p);
    x = (uint16_t)(k & 0x00FF);
    cpu.x = x;
    cpu.a = t;
    r = rd16(0x70FC4A + (uint32_t)x);
    cpu.p = (uint8_t)((nz16(p, (uint16_t)(t - r)) & ~P_C) | (t >= r ? P_C : 0));
    if (t == r) {
        time_up_lead(0x00);
        return;
    }
    if (t > r) {
        time_up_lead(0x01);
        return;
    }
    wr16(0x70FC4A + (uint32_t)x, t);
    op_lda16(b | 0x0F11);
    wr16(0x70FC4C + (uint32_t)x, cpu.a);
    op_sep(P_M);
    op_lda8(b | 0x0D89);
    wr8(0x70FC4E + (uint32_t)x, (uint8_t)cpu.a);
    time_up_lead(0xFF);
    abi_SaveRecordSumsTimeUp();
}

/*
 * @routine 07:AE07 StuntRecordTime
 * WHAT IT DOES: Stunt Trax's score against the course's three best for
 * the car: the score is $19EC as the high byte and the BCD sum of $0E70
 * and $19F0 as the low; for courses $0C-$0F, at $70:FCD0 + course * 48 +
 * car ($0D89) * 12 (three entries of score and $0F11, best first), a
 * higher score goes in at its place (tag RecordTagCar0, 4 or 8), the
 * lower ones moving down, and the course's 48 bytes are summed into
 * $70:FC33 + course. For any other course its RTS takes the score it
 * pushed as the return address (no run reaches that: the courses here
 * are $0C-$0F).
 * HOW: SED for the BCD add; $F1 holds course * 48 (also $0D14), $F3 the
 * car, $F7 the entry's offset; the score is kept on the stack.
 * Entry: any widths; data bank $07, D = $0000. Exit: M 16-bit, X 16-bit;
 * A as the last step leaves it.
 */
void abi_StuntRecordTime(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint16_t t, k, n;
    uint8_t lo, sum, pushed[2];

    op_sep(P_M);
    op_lda8(b | 0x19EC);
    op_xba();
    cpu.p |= P_D;
    op_lda8(b | 0x0E70);
    cpu.p &= (uint8_t)~P_C;
    lo = op_adc8((uint8_t)cpu.a, rd8(b | 0x19F0), &cpu.p);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | lo);
    cpu.p &= (uint8_t)~P_D;
    op_rep(P_M | P_X);
    t = cpu.a;
    op_lda16(b | 0x0E3D);
    op_and16(0x00FF);
    op_cmp16(0x000C);
    if (cpu.p & P_C)
        op_cmp16(0x0010);
    if (cpu.a < 0x000C || cpu.a >= 0x0010) {
        pushed[0] = (uint8_t)(t >> 8);
        pushed[1] = (uint8_t)t;
        jump_routine_pushed(0x07AE29, pushed, 2);
        return;
    }
    wr16(special_dp(0xF1), cpu.a);
    op_asl16();
    op_add16(rd16(special_dp(0xF1)));
    op_asl16();
    op_asl16();
    op_asl16();
    op_asl16();
    wr16(special_dp(0xF1), cpu.a);
    wr16(b | 0x0D14, cpu.a);
    cpu.y = cpu.a;
    op_lda16(b | 0x0D89);
    op_and16(0x00FF);
    wr16(special_dp(0xF3), cpu.a);
    op_asl16();
    op_add16(rd16(special_dp(0xF3)));
    op_asl16();
    op_asl16();
    op_add16(rd16(special_dp(0xF1)));
    op_tax16();
    wr16(special_dp(0xF7), cpu.a);
    cpu.a = t;
    cpu.p = nz16(cpu.p, t);
    op_cmp16(rd16(SPECIAL_SAVES + cpu.x));
    if ((cpu.p & P_C) && !(cpu.p & P_Z)) {
        abi_RecordTagCar0();
        special_move(4, 8);
        special_move(6, 10);
        special_move(0, 4);
        special_move(2, 6);
        op_lda16(b | 0x0F11);
        wr16(SPECIAL_SAVES + 2 + cpu.x, cpu.a);
        cpu.a = t;
        cpu.p = nz16(cpu.p, t);
        wr16(SPECIAL_SAVES + cpu.x, t);
    } else {
        op_cmp16(rd16(SPECIAL_SAVES + 4 + cpu.x));
        if ((cpu.p & P_C) && !(cpu.p & P_Z)) {
            abi_RecordTagCar4();
            special_move(4, 8);
            special_move(6, 10);
            cpu.a = t;
            cpu.p = nz16(cpu.p, t);
            wr16(SPECIAL_SAVES + 4 + cpu.x, t);
            op_lda16(b | 0x0F11);
            wr16(SPECIAL_SAVES + 6 + cpu.x, cpu.a);
        } else {
            op_cmp16(rd16(SPECIAL_SAVES + 8 + cpu.x));
            if (!(cpu.p & P_C) || (cpu.p & P_Z)) {
                op_rep(P_M | P_X);
                return;
            }
            wr16(SPECIAL_SAVES + 8 + cpu.x, t);
            abi_RecordTagCar8();
            op_lda16(b | 0x0F11);
            wr16(SPECIAL_SAVES + 10 + cpu.x, cpu.a);
        }
    }
    op_sep(P_M);
    cpu.a = (uint16_t)(cpu.a & 0xFF00);
    cpu.p = nz8(cpu.p, 0);
    wr8(special_dp(0xF1), 0x00);
    cpu.y = 0x0030;
    cpu.x = rd16(special_dp(0xF7));
    cpu.x = rd16(b | 0x0D14);
    n = 0x0030;
    sum = 0;
    do {
        uint8_t v = rd8(SPECIAL_SAVES + cpu.x);

        cpu.p &= (uint8_t)~P_C;
        sum = op_adc8(v, rd8(special_dp(0xF1)), &cpu.p);
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | sum);
        wr8(special_dp(0xF1), sum);
        cpu.x = (uint16_t)(cpu.x + 1);
        n = (uint16_t)(n - 1);
        cpu.y = n;
    } while (n);
    op_sep(P_X);
    cpu.x = rd8(b | 0x0E3D);
    k = rd8(special_dp(0xF1));
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | k);
    cpu.p = nz8(cpu.p, (uint8_t)k);
    wr8(0x70FC33 + cpu.x, (uint8_t)k);
    op_rep(P_M | P_X);
}

/*
 * The records panel of the course screens: four rows of three saved entries
 * drawn one character a frame into the tilemap buffer at $7F:93D7.
 * $0FA6 is the character (0-6 for Stunt Trax, 0-7 for the other courses),
 * $0FA8 the row (0-3) and $0FAA the place (0-2).
 */

/* LDA $0FA8; ASL; TAX; LDA.L row_table,X; STA $F3; then + $0FAA * 64 and +
   the character's byte from char_table at $0FA6: Y = A = $F3, X = $0FA6. */
static void records_char_offset(uint32_t row_table, uint32_t char_table)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    unsigned i;

    op_lda16(b | 0x0FA8);
    op_asl16();
    op_tax16();
    op_lda16(row_table + cpu.x);
    wr16(0x0000F3, cpu.a);
    op_lda16(b | 0x0FAA);
    op_and16(0x00FF);
    for (i = 0; i < 6; i++)
        op_asl16();
    op_add16(rd16(0x0000F3));
    wr16(0x0000F3, cpu.a);
    cpu.x = rd16(b | 0x0FA6);
    cpu.p = nz16(cpu.p, cpu.x);
    op_lda16(char_table + cpu.x);
    op_and16(0x00FF);
    op_add16(rd16(0x0000F3));
    wr16(0x0000F3, cpu.a);
    op_tay16();
}

/*
 * @routine 07:D87A RecordsCharOffsetStunt
 * WHAT IT DOES: The tilemap offset of the Stunt Trax records character:
 * the row's start (words at $07:D8D4) + place * 64 + the character's
 * column (bytes at $07:D8DC).
 * HOW: Summed in direct page $F3.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: A = Y = $F3 = the offset;
 * X = $0FA6; N, Z from Y; C, V from the last add.
 */
void abi_RecordsCharOffsetStunt(void)
{
    records_char_offset(0x07D8D4, 0x07D8DC);
}

/*
 * @routine 07:D8A7 RecordsCharOffsetAll
 * WHAT IT DOES: As RecordsCharOffsetStunt for the other courses' records:
 * row starts at $07:D8E3, columns at $07:D8EB.
 * HOW: As RecordsCharOffsetStunt.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: as RecordsCharOffsetStunt.
 */
void abi_RecordsCharOffsetAll(void)
{
    records_char_offset(0x07D8E3, 0x07D8EB);
}

/*
 * @routine 07:D8F3 RecordsEntryLoad
 * WHAT IT DOES: Loads saved entry ($0FA8 * 3 + $0FAA) of the course
 * ($0E3D) from $70:FCD0 (48 bytes a course, 4 an entry): $F1 = its first
 * word, $F3 its second.
 * HOW: The offset summed in $F1: (course * 3, low byte) * 16, + row * 12,
 * + place * 4.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: A = $F1 = the first word,
 * N and Z from it; X = the offset; C, V from the last add.
 */
void abi_RecordsEntryLoad(void)
{
    uint32_t b = (uint32_t)cpu.db << 16;

    op_lda16(b | 0x0E3D);
    op_asl16();
    op_add16(rd16(b | 0x0E3D));
    op_and16(0x00FF);
    op_asl16();
    op_asl16();
    op_asl16();
    op_asl16();
    wr16(0x0000F1, cpu.a);
    op_lda16(b | 0x0FA8);
    op_and16(0x00FF);
    op_asl16();
    op_add16(rd16(b | 0x0FA8));
    op_asl16();
    op_asl16();
    op_add16(rd16(0x0000F1));
    wr16(0x0000F1, cpu.a);
    op_lda16(b | 0x0FAA);
    op_asl16();
    op_asl16();
    op_add16(rd16(0x0000F1));
    wr16(0x0000F1, cpu.a);
    op_tax16();
    op_lda16(0x70FCD2 + cpu.x);
    wr16(0x0000F3, cpu.a);
    op_lda16(0x70FCD0 + cpu.x);
    wr16(0x0000F1, cpu.a);
}

void abi_BinaryToBcd(void);

/* LDA $0FA6; ASL; TAX: the handler index, C from the shift. */
static uint16_t records_char_index(void)
{
    op_lda16(((uint32_t)cpu.db << 16) | 0x0FA6);
    op_asl16();
    op_tax16();
    return cpu.x;
}

/* A digit of $F2 (the high nibble, or the low one with `low`) when $F2 is
   below $40, else `blank`. */
static void stunt_score_digit(int low, uint16_t blank)
{
    op_sep(P_M);
    op_lda8(0x0000F2);
    cpu.p = (uint8_t)((nz8(cpu.p, (uint8_t)(cpu.a - 0x40)) & ~P_C) | ((uint8_t)cpu.a >= 0x40 ? P_C : 0));
    if ((uint8_t)cpu.a >= 0x40) {
        op_rep(P_M);
        op_lda16_imm(blank);
        return;
    }
    if (!low) {
        unsigned i;
        uint8_t v = (uint8_t)cpu.a;

        for (i = 0; i < 4; i++) {
            cpu.p = (uint8_t)((cpu.p & ~P_C) | (v & 1));
            v >>= 1;
        }
        cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
        cpu.p = nz8(cpu.p, v);
    }
    op_rep(P_M);
    op_and16(0x000F);
}

/* Letter `which` (0-2) of the name word at $F3: bits 0-4 (LDA $F3), 5-9
   (three ASLs and XBA) or 10-14 (LDA $F4, two LSRs); AND #$1F, + $0A. */
static void records_letter(int which)
{
    unsigned i;

    if (which == 0) {
        op_lda16(0x0000F3);
    } else if (which == 1) {
        op_lda16(0x0000F3);
        for (i = 0; i < 3; i++)
            op_asl16();
        op_xba();
    } else {
        op_lda16(0x0000F4);
        op_lsr16();
        op_lsr16();
    }
    op_and16(0x001F);
    op_add16(0x000A);
}

/*
 * @routine 07:D795 RecordsCharTileStunt
 * WHAT IT DOES: The tile of Stunt Trax records character $0FA6 for the
 * entry loaded at $F1 (RecordsEntryLoad): 0-1 the score's digits (high
 * byte $F2, high then low nibble; $6D and $6E when $F2 is $40 or more),
 * 2-3 the digits of $F1, 4-6 the three letters of the name word at $F3
 * (five bits each, + $0A).
 * HOW: JMP ($D79D,X) on $0FA6 * 2. The low score digit, the low digit of
 * $F1 and the blanks are not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: M 16-bit; A = the tile;
 * X = $0FA6 * 2.
 */
void abi_RecordsCharTileStunt(void)
{
    switch (records_char_index()) {
    case 0:
        stunt_score_digit(0, 0x006D);
        break;
    case 2:
        stunt_score_digit(1, 0x006E);
        break;
    case 4:
        op_lda16(0x0000F1);
        op_lsr16();
        op_lsr16();
        op_lsr16();
        op_lsr16();
        op_and16(0x000F);
        break;
    case 6:
        op_lda16(0x0000F1);
        op_and16(0x000F);
        break;
    case 8:
        records_letter(0);
        break;
    case 10:
        records_letter(1);
        break;
    case 12:
        records_letter(2);
        break;
    default:
        jump_routine(0x070000 | rd16(0x07D79D + cpu.x));
        break;
    }
}

/* A field of the packed time at $F1 (the seconds, ASL and XBA first, with
   `seconds`), masked, through BinaryToBcd: the tens with `tens`, else the
   units. */
static void records_time_digit(uint16_t mask, int seconds, int tens)
{
    op_lda16(0x0000F1);
    if (seconds) {
        op_asl16();
        op_xba();
    }
    op_and16(mask);
    abi_BinaryToBcd();
    if (tens) {
        op_lsr16();
        op_lsr16();
        op_lsr16();
        op_lsr16();
    } else {
        op_and16(0x000F);
    }
}

/*
 * @routine 07:D7FF RecordsCharTileAll
 * WHAT IT DOES: As RecordsCharTileStunt for the other courses, whose
 * entries hold a packed time at $F1: character 0 the minutes (bits 13-15),
 * 1-2 the seconds' tens and units (bits 7-12), 3-4 the hundredths' (bits
 * 0-6), all through BinaryToBcd; 5-7 the three letters of $F3.
 * HOW: JMP ($D807,X) on $0FA6 * 2; the minutes come out of four ROLs
 * through carry. Not reached in the traces so far.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: M 16-bit; A = the tile;
 * X = $0FA6 * 2.
 */
void abi_RecordsCharTileAll(void)
{
    unsigned i;

    switch (records_char_index()) {
    case 0:
        op_lda16(0x0000F1);
        for (i = 0; i < 4; i++)
            op_rol16();
        op_and16(0x0007);
        abi_BinaryToBcd();
        op_and16(0x000F);
        break;
    case 2:
        records_time_digit(0x003F, 1, 1);
        break;
    case 4:
        records_time_digit(0x003F, 1, 0);
        break;
    case 6:
        records_time_digit(0x007F, 0, 1);
        break;
    case 8:
        records_time_digit(0x007F, 0, 0);
        break;
    case 10:
        records_letter(0);
        break;
    case 12:
        records_letter(1);
        break;
    case 14:
        records_letter(2);
        break;
    default:
        jump_routine(0x070000 | rd16(0x07D807 + cpu.x));
        break;
    }
}

/* The tile's low byte to $7F:93D7 + Y (SEP #$20, TYX, STA.L, REP #$20). */
static void records_char_store(void)
{
    op_sep(P_M);
    cpu.x = cpu.y;
    cpu.p = nz16(cpu.p, cpu.x);
    wr8(0x7F93D7 + cpu.x, (uint8_t)cpu.a);
    op_rep(P_M);
}

/*
 * @routine 07:D768 RecordsCharDrawStunt
 * WHAT IT DOES: Draws the current character of the Stunt Trax records
 * panel: when the entry's first word is not 0, its tile
 * (RecordsCharTileStunt) at the offset from RecordsCharOffsetStunt.
 * HOW: RecordsCharOffsetStunt, RecordsEntryLoad, BEQ on the loaded word.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: M 16-bit; Y = the offset;
 * A = the tile and X = Y, or as RecordsEntryLoad leaves them for an empty
 * entry.
 */
void abi_RecordsCharDrawStunt(void)
{
    abi_RecordsCharOffsetStunt();
    abi_RecordsEntryLoad();
    if (cpu.p & P_Z)
        return;
    abi_RecordsCharTileStunt();
    records_char_store();
}

/*
 * @routine 07:D77D RecordsCharDrawAll
 * WHAT IT DOES: As RecordsCharDrawStunt for the other courses; an entry
 * is empty when its first word is $FFFF.
 * HOW: RecordsCharOffsetAll, RecordsEntryLoad, CMP #$FFFF. Drawing is not
 * reached in the traces so far.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: as RecordsCharDrawStunt;
 * for an empty entry C and Z set.
 */
void abi_RecordsCharDrawAll(void)
{
    abi_RecordsCharOffsetAll();
    abi_RecordsEntryLoad();
    op_cmp16(0xFFFF);
    if (cpu.a == 0xFFFF)
        return;
    abi_RecordsCharTileAll();
    records_char_store();
}

/* PHP, SEP #$20, PHB, PHK, PLB, REP #$20, the JSR, SEP #$20, PLB, PLP. */
static void records_char_long(void (*draw)(void))
{
    uint8_t p0 = cpu.p, db0 = cpu.db;

    op_sep(P_M);
    cpu.db = 0x07;
    cpu.p = nz8(cpu.p, 0x07);
    op_rep(P_M);
    draw();
    op_sep(P_M);
    cpu.db = db0;
    op_plp(p0);
}

/*
 * @routine 07:C663 PreviewRowDrawStunt
 * WHAT IT DOES: RecordsCharDrawStunt for a JSL caller (the course screens'
 * records panel): data bank $07, P and the data bank kept.
 * HOW: PHP, SEP #$20, PHB, PHK, PLB, REP #$20, JSR; then SEP #$20, PLB,
 * PLP, RTL.
 * Entry: X 16-bit; D = $0000. Exit: P and the data bank unchanged; A, X,
 * Y as RecordsCharDrawStunt leaves them.
 */
void abi_PreviewRowDrawStunt(void)
{
    records_char_long(abi_RecordsCharDrawStunt);
}

/*
 * @routine 07:C673 PreviewRowDrawAll
 * WHAT IT DOES: As PreviewRowDrawStunt with RecordsCharDrawAll.
 * HOW: As PreviewRowDrawStunt.
 * Entry: X 16-bit; D = $0000. Exit: as PreviewRowDrawStunt.
 */
void abi_PreviewRowDrawAll(void)
{
    records_char_long(abi_RecordsCharDrawAll);
}
