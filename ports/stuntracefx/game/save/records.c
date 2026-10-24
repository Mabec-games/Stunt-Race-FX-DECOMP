/*
 * records.c: five-byte records in the save data (Super FX RAM $70:FC4A and
 * $70:FC5E onwards).
 */
#include "machine.h"
#include "cpu_ops.h"
#include "hud_calls.h"
#include "save_calls.h"
#include "carried.h"

#include "audio_calls.h"
#include "wram_names.h"

/* Sum five bytes from `base` + X with 8-bit adds; X steps past them. */
static uint8_t sum5(uint32_t base, uint8_t *x, uint8_t *p)
{
    uint8_t a = 0, v;
    unsigned s, i;

    for (i = 0; i < 5; i++) {
        cyc(78);
        v = rd8(base + *x);
        s = (unsigned)a + v;
        *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0) | ((~(a ^ v) & (a ^ (uint8_t)s) & 0x80) ? (cyc(18), P_V) : (cyc(18), 0)));
        a = (uint8_t)s;
        (*x)++;
    }
    cyc(28);
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

void PreviewRowDrawAll_cr(Carried *cr);
void PreviewRowDrawStunt_cr(Carried *cr);
void RecordCarTimes12_cr(Carried *cr);
void RecordCompareTime0_cr(Carried *cr);
void RecordCompareTime1_cr(Carried *cr);
void RecordLetterTile_cr(Carried *cr);
void RecordNameLetters_cr(Carried *cr);
void RecordNameLettersLarge_cr(Carried *cr);
void RecordNameLettersSmall_cr(Carried *cr);
void RecordRow1_cr(Carried *cr);
void RecordRow1Time0_cr(Carried *cr);
void RecordRow2_cr(Carried *cr);
void RecordRow2Time4_cr(Carried *cr);
void RecordRow3_cr(Carried *cr);
void RecordRow3Time8_cr(Carried *cr);
void RecordRowDraw_cr(Carried *cr);
void RecordRowsFirstTimes_cr(Carried *cr);
void RecordRowsGroupFirsts(Carried *cr);
void RecordScoreDigits_cr(Carried *cr);
void RecordTagCar0_cr(Carried *cr);
void RecordTagCar4_cr(Carried *cr);
void RecordTagCar8_cr(Carried *cr);
void RecordTagCarSet_cr(Carried *cr);
void RecordTimeCompare_cr(Carried *cr);
void RecordTimeDigitsLarge_cr(Carried *cr);
void RecordTimeDigitsSmall_cr(Carried *cr);
void RecordsCharDrawAll_cr(Carried *cr);
void RecordsCharDrawStunt_cr(Carried *cr);
void RecordsCharOffsetAll_cr(Carried *cr);
void RecordsCharOffsetStunt_cr(Carried *cr);
void RecordsCharTileAll_cr(Carried *cr);
void RecordsCharTileStunt_cr(Carried *cr);
void RecordsEntryLoad_cr(Carried *cr);
void RecordsLoadCourse_cr(Carried *cr);
void RecordsLoadCourseLong_cr(Carried *cr);
void RecordsLoadForCourse_cr(Carried *cr);
void RecordsLoadSpecialCourse_cr(Carried *cr);
void RecordsLoadSpecialCourseLong_cr(Carried *cr);
void RecordsUnpackCourse_cr(Carried *cr);
void RecordsUnpackCourseRunning(Carried *cr);
void RecordsUnpackCourseRunningB(Carried *cr);
void RecordsUnpackCourseTotals(Carried *cr);
void SaveBlocksCheck_cr(Carried *cr);
void SaveBlocksFourthBad_cr(Carried *cr);
void SaveBlocksValidate_cr(Carried *cr);
void SaveRecordSumA_cr(Carried *cr);
void SaveRecordSumB_cr(Carried *cr);
void SaveRecordSumsTimeUp_cr(Carried *cr);
void SaveRecordsCheckA_cr(Carried *cr);
void SaveRecordsCheckB_cr(Carried *cr);
void SaveSumsRecordsB_cr(Carried *cr);
void SaveTablesCheck_cr(Carried *cr);
void SaveUnlocksCheck_cr(Carried *cr);
void SaveValidate_cr(Carried *cr);
void SpecialRecordInsert_cr(Carried *cr);
void SpecialRecordsChecksum_cr(Carried *cr);
void StuntRecordTime_cr(Carried *cr);
void TimeUpRecordUpdate_cr(Carried *cr);
uint16_t PreviewRowDrawAll(Carried *cr);
uint16_t PreviewRowDrawStunt(Carried *cr);
void RecordCompareTime0(Carried *cr, uint8_t bank);
void RecordCompareTime1(Carried *cr, uint8_t bank);
uint16_t RecordsCharDrawAll(Carried *cr, uint8_t bank);
uint16_t RecordsCharDrawStunt(Carried *cr, uint8_t bank);
uint16_t RecordsCharTileAll(Carried *cr, uint8_t bank);
uint16_t RecordsCharTileStunt(Carried *cr, uint8_t bank);
uint8_t StuntRecordTime(Carried *cr, uint8_t bank, uint16_t d);
static void char_draw_exit(Carried *cr);
static void char_offset_exit(Carried *cr, uint16_t r);
static void char_tile_exit(Carried *cr, uint16_t t);
static void checksum_exit(Carried *cr, uint8_t s);
static void compare_by_mode_exit(Carried *cr);
static void load_exit(Carried *cr);
static void preview_exit(Carried *cr, uint8_t p0);
static void record_compare_by_mode(Carried *cr, uint8_t bank, uint16_t x);
static void records_char_jump(Carried *cr, uint8_t bank, uint32_t table);
static void row_exit(Carried *cr);
void PlayerSound35_cr(Carried *cr);
void TimeRunningTotals_cr(Carried *cr);
void TimeTotalsSum_cr(Carried *cr);

void SaveRecordSumA_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveRecordSumA(&x, &p));
    cr->x = x;
    cr->y = 0;
    set_p(cr, p);
}

void abi_SaveRecordSumA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveRecordSumA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void SaveRecordSumB_cr(Carried *cr)
{
    uint8_t x = (uint8_t)cr->x, p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveRecordSumB(&x, &p));
    cr->x = x;
    cr->y = 0;
    set_p(cr, p);
}

void abi_SaveRecordSumB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveRecordSumB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:ADDA SaveSumsRecordsB
 * WHAT IT DOES: Keeps the sums of the first three records at $70:FC5E
 * (SaveRecordSumB) in the sum bytes $70:FC43-$70:FC45, after clearing
 * direct page $F1; then returns the fourth record's sum.
 * HOW: SEP #$30; three JSRs, then SaveRecordSumB's code, which follows.
 * Entry: D = $0000. Exit: as SaveRecordSumB, X = $14.
 */
static uint8_t sums_p;                  /* the adapter's: the flags the last sum leaves */

uint8_t SaveSumsRecordsB(void)
{
    uint8_t x, a;
    unsigned i;

    cyc(22);                            /* SEP #$30 */
    cyc(62);
    wr8(W(time_total), 0x00);
    sums_p = 0;
    for (i = 0; i < 3; i++) {
        cyc(62);
        x = (uint8_t)(i * 5);
        a = SaveRecordSumB(&x, &sums_p);
        cyc(72);
        wr8((uint32_t)(0x70FC43 + i), a);
    }
    x = 15;
    return SaveRecordSumB(&x, &sums_p);
}

void SaveSumsRecordsB_cr(Carried *cr)
{
    uint8_t a = SaveSumsRecordsB();

    cr->a = (uint16_t)((cr->a & 0xFF00) | a);
    cr->x = 20;
    cr->y = 0;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | (sums_p & (P_N | P_Z | P_C | P_V)) | P_M | P_X));
}

void abi_SaveSumsRecordsB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveSumsRecordsB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
        cyc(144);
        uint8_t xs = x;

        a = sum(&xs, p);
        cyc(120);
        if (a == rd8(b | (uint16_t)(sums + y))) {
            cyc(40);
            a = rd8(b | (uint16_t)(base + x));
            if (a)
                { cyc(90); continue; }
        } else { cyc(6); }
        cyc(62);
        wr8(b | (uint16_t)(base + x), 0xFF);
        cyc(30);
        wr8(b | (uint16_t)(base + x + 1), 0xFF);
        cyc(46);
        wr8(b | (uint16_t)(base + x + 2), 0x00);
        cyc(30);
        wr8(b | (uint16_t)(base + x + 3), 0x00);
        cyc(30);
        wr8(b | (uint16_t)(base + x + 4), 0x00);
        cyc(46);
        wr8(b | (uint16_t)(sums + y), 0xFE);
        a = 0xFE;
        cyc(68);
    }
    cyc(90);
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

void SaveRecordsCheckA_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveRecordsCheckA(cr->db, &p));
    cr->x = 20;
    cr->y = 4;
    set_p(cr, (uint8_t)((p & ~P_N) | P_M | P_X | P_C | P_Z));
}

void abi_SaveRecordsCheckA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveRecordsCheckA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

void SaveRecordsCheckB_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveRecordsCheckB(cr->db, &p));
    cr->x = 15;
    cr->y = 3;
    set_p(cr, (uint8_t)((p & ~P_N) | P_M | P_X | P_C | P_Z));
}

void abi_SaveRecordsCheckB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveRecordsCheckB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(44);
    uint8_t v = rd8(at), s;

    cyc(16);
    s = rd8(W(time_total));
    s = add8(v, s, p);
    cyc(16);
    wr8(W(time_total), s);
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

    cyc(62);
    wr16(W(record_row_name), 0x000C);
    do {
        cyc(38);
        w = rd16(W(record_row_name));
        base = (uint16_t)(w << 1);
        cyc(44);
        w = rd16(W(record_row_name));
        base = (uint16_t)((uint16_t)(base + w) << 4);
        cyc(86);
        wr16(W(dp_f5), base);
        cyc(16);
        wr16(W(time_total), 0x0000);
        for (i = 0, x = base; i < 12; i++, x += 4) {
            cyc(46);
            sum_byte(b | (uint16_t)(0xFCD0 + x), p);
            cyc(30);
            v = rd8(b | (uint16_t)(0xFCD1 + x));
            if (v >= 0x41)
                goto reset;
            cyc(62);
            s = rd8(W(time_total));
            s = add8(v, s, p);
            cyc(16);
            wr8(W(time_total), s);
            sum_byte(b | (uint16_t)(0xFCD2 + x), p);
            sum_byte(b | (uint16_t)(0xFCD3 + x), p);
            cyc(46);
        }
        cyc(56);
        x = rd16(W(record_row_name));
        cyc(30);
        v = rd8(b | (uint16_t)(0xFC33 + x));
        cyc(16);
        s = rd8(W(time_total));
        if (v == s)
            goto next;
reset:
        cyc(54);
        x = rd16(W(dp_f5));
        for (i = 0; i < 12; i++, x += 4) {
            cyc(92);
            wr8(b | (uint16_t)(0xFCD0 + x), 0x00);
            cyc(30);
            wr8(b | (uint16_t)(0xFCD1 + x), 0x00);
            cyc(46);
            wr8(b | (uint16_t)(0xFCD2 + x), 0xFF);
            cyc(30);
            wr8(b | (uint16_t)(0xFCD3 + x), 0xFF);
            cyc(46);
        }
        cyc(56);
        x = rd16(W(record_row_name));
        cyc(46);
        wr8(b | (uint16_t)(0xFC33 + x), 0xE8);
cyc(6);
next:
        cyc(32);
        blk = rd8(W(record_row_name));
        cyc(6);
        wr8(W(record_row_name), (uint8_t)(blk + 1));
        cyc(16);
        blk = rd8(W(record_row_name));
    } while (cyc_loop(38, blk < 0x10));
    cyc(74);
}

void SaveBlocksCheck_cr(Carried *cr)
{
    uint8_t p = cr->p;

    SaveBlocksCheck(cr->db, &p);
    cr->a = 0x0210;
    cr->x = 0x000F;
    cr->y = 0x0000;
    set_p(cr, (uint8_t)(((p & ~P_N) | P_M | P_C | P_Z) & ~P_X));
}

void abi_SaveBlocksCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveBlocksCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(238);
    wr8(W(dp_f4), 0x00);
    cyc(16);
    wr8(W(glyph_time_mid), 0x00);
    for (k = 2; k >= 0; k--) {
        cyc(86);
        wr8(W(record_row_name), first[k]);
        cyc(56);
        wr8(W(dp_f5), second[k]);
        cyc(16);
        wr8(W(time_total), 0x00);
        cyc(40);
        x = rd16(W(record_row_name));
        for (i = 0; i < 12; i++, x += 2) {
            cyc(44);
            v = rd8(b | (uint16_t)(0xFC88 + x));
            cyc(16);
            sum = rd8(W(time_total));
            sum = add8(v, sum, p);
            cyc(16);
            wr8(W(time_total), sum);
            cyc(30);
            v = rd8(b | (uint16_t)(0xFC89 + x));
            if (v < 0x1F) {
                cyc(52);
                *p &= (uint8_t)~P_C;
                goto reset;
            }
            cyc(62);
            sum = rd8(W(time_total));
            sum = add8(v, sum, p);
            cyc(16);
            wr8(W(time_total), sum);
            cyc(64);
        }
        cyc(10);
        x = rd16(W(dp_f5));
        for (i = 0; i < 9; i++, x++) {
            cyc(i ? 94 : 68);           /* the first round has no INX, DEY, BNE */
            v = rd8(b | (uint16_t)(0xFC6D + x));
            cyc(16);
            sum = rd8(W(time_total));
            sum = add8(v, sum, p);
            cyc(16);
            wr8(W(time_total), sum);
        }
        cyc(110);
        a = rd8(b | (uint16_t)(0xFC2D + k));
        cyc(16);
        sum = rd8(W(time_total));
        *p = (uint8_t)((*p & ~P_C) | (a >= sum ? P_C : 0));
        if (a == sum)
            { cyc(74); continue; }
reset:
        cyc(84);
        x = rd16(W(record_row_name));
        for (i = 0; i < 24; i++, x++)
            { cyc(54); wr8(b | (uint16_t)(0xFC88 + x), 0xFF); cyc(26); }
        cyc(34);
        x = rd16(W(dp_f5));
        for (i = 0; i < 9; i++, x++)
            { cyc(i ? 80 : 70); wr8(b | (uint16_t)(0xFC6D + x), 0x00); }
        a = 0xE8;
        cyc(128);
        wr8(0x70FC2D + k, a);
        cyc(36);
    }
    return a;
}

void SaveTablesCheck_cr(Carried *cr)
{
    uint8_t p = cr->p, a = SaveTablesCheck(cr->db, &p);

    cr->a = a;
    cr->x = 0xFFFF;
    cr->y = 0x0000;
    set_p(cr, (uint8_t)(((p & ~(P_Z | P_X)) | P_N | P_M)));
    cyc(36);
}

void abi_SaveTablesCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveTablesCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* One entry byte of SaveBlocksValidate: $FF passes; otherwise `bad` says
   whether it fails. */
static int entry_byte_bad(uint8_t v, int which)
{
    if (v == 0xFF)
        return 0; else { cyc(48); }
    switch (which) {
    case 0: return (v & 0x7F) >= 0x64;
    case 1: return (v & 0x1F) >= 0x1E || (cyc(104), (v & 0xE0) < 0x20);
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
        { cyc(28); goto reset; }
    cyc(62);
    wr16(W(record_row_name), 0x0000);
    for (;;) {
        cyc(38);
        w = rd16(W(record_row_name));
        x = (uint16_t)(w << 2);
        cyc(96);
        v = rd8(0x70FFD0 + x);
        cyc(16);
        wr8(W(time_total), v);
        for (j = 1; j < 4; j++) {
            cyc(32);
            v = rd8(0x70FFD0 + x + j);
            cyc(30);
            s = rd8(W(time_total));
            s = add8(v, s, p);
            if (j < 3)
                { cyc(16); wr8(W(time_total), s); }
        }
        cyc(62);
        wr16(W(time_total), s);
        cyc(16);
        w = rd16(W(record_row_name));
        x = (uint16_t)(w << 1);
        cyc(44);
        w = rd16(W(record_row_name));
        x = (uint16_t)((uint16_t)(x + w) << 4);
        cyc(86);
        wr16(W(dp_f5), x);
        for (i = 0; i < 12; i++, x += 4) {
            cyc(60);
            for (j = 0; j < 4; j++) {
                cyc(30);
                v = rd8(b | (uint16_t)(0xFCD0 + x + j));
                if (entry_byte_bad(v, j))
                    { cyc(82); goto reset; }
                cyc(118);
                s = rd8(W(time_total));
                s = add8(v, s, p);
                cyc(16);
                wr8(W(time_total), s);
            }
            cyc(32);
        }
        cyc(70);
        x = rd16(W(record_row_name));
        cyc(30);
        v = rd8(b | (uint16_t)(0xFC33 + x));
        cyc(16);
        s = rd8(W(time_total));
        if (v != s)
            goto reset;
        cyc(32);
        blk = rd8(W(record_row_name));
        cyc(6);
        wr8(W(record_row_name), (uint8_t)(blk + 1));
        cyc(16);
        blk = rd8(W(record_row_name));
        if (blk >= 0x0C)
            { cyc(80); return; } else { cyc(56); }
        continue;
reset:
        cyc(92);
        wr8(W(save_bad_flag), 0x11);
        cyc(66);
        x = rd16(W(dp_f5));
        cyc(12);
        for (i = 0; i < 48; i++, x++)
            { cyc(80); wr8(b | (uint16_t)(0xFCD0 + x), 0xFF); }
        cyc(60);
        x = rd16(W(record_row_name));
        cyc(38);
        w = rd16(W(record_row_name));
        y = (uint16_t)((w & 0x00FF) << 2);
        cyc(104);
        for (j = 0; j < 4; j++)
            { cyc(30); wr8(b | (uint16_t)(0xFFD0 + y + j), 0xFF); }
        cyc(46);
        wr8(b | (uint16_t)(0xFC33 + x), 0xCC);
        cyc(24);
    }
}

void SaveBlocksValidate(uint8_t bank, uint8_t *p)
{
    save_blocks_validate(bank, p, 0);
}

void SaveBlocksValidate_cr(Carried *cr)
{
    uint8_t p = cr->p;

    SaveBlocksValidate(cr->db, &p);
    cr->a = 0x020C;
    cr->x = 0x000B;
    cr->y = 0x0000;
    set_p(cr, (uint8_t)(((p & ~(P_N | P_X)) | P_M | P_C | P_Z)));
}

void abi_SaveBlocksValidate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveBlocksValidate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
void SaveBlocksFourthBad(uint8_t bank, uint8_t *p)
{
    cpu.s++;                            /* the entry byte the test pushed */
    save_blocks_validate(bank, p, 1);
}

void SaveBlocksFourthBad_cr(Carried *cr)
{
    uint8_t p = cr->p;

    SaveBlocksFourthBad(cr->db, &p);
    cr->a = 0x020C;
    cr->x = 0x000B;
    cr->y = 0x0000;
    set_p(cr, (uint8_t)(((p & ~(P_N | P_X)) | P_M | P_C | P_Z)));
}

void abi_SaveBlocksFourthBad(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveBlocksFourthBad_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint8_t SaveValidate(uint8_t bank)
{
    uint8_t p = 0, a;

    SaveTablesCheck(bank, &p);
    cyc(36);
    (cyc(46), SaveBlocksValidate(bank, &p));
    (cyc(46), SaveBlocksCheck(bank, &p));
    (cyc(46), SaveRecordsCheckB(bank, &p));
    a = (cyc(46), SaveRecordsCheckA(bank, &p));
    cyc(72);
    return a;
}

void SaveValidate_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x0200 | SaveValidate(cr->db));
    cr->x = 20;
    cr->y = 4;
}

void abi_SaveValidate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveValidate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The unlock bytes of the save in the order their checksum adds them. */
static const uint16_t unlock_bytes[10] = {
    0xFC0B, 0xFC0F, 0xFC06, 0xFC07, 0xFC0C, 0xFC0D, 0xFC0E, 0xFC08, 0xFC09, 0xFC0A};

/* The checksum: an 8-bit add chain, each carry going into the next add. */
static uint8_t unlock_sum(void)
{
    cyc(192);
    unsigned s = rd8(0x700000 | unlock_bytes[0]), c = 0, i;

    for (i = 1; i < 10; i++) {
        cyc(24);
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
uint8_t SaveUnlocksCheck(void)
{
    uint8_t s = unlock_sum(), a;
    unsigned i;

    cyc(24);
    if (s != rd8(0x70FC14)) {
        cyc(16);
        for (i = 0; i < 10; i++)
            { cyc(24); wr8(0x700000 | unlock_bytes[i], 0x00); }
        cyc(24);
        wr8(0x70FC14, 0x00);
        cyc(24);
        wr8(0x70FC15, 0x00);
    } else { cyc(22); }
    a = SaveValidate(0x70);
    cyc(100);
    return a;
}

void SaveUnlocksCheck_cr(Carried *cr)
{
    cr->a = (uint16_t)(0x0200 | SaveUnlocksCheck());
    cr->x = 20;
    cr->y = 4;
}

void abi_SaveUnlocksCheck(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveUnlocksCheck_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
static uint8_t unpack_high, unpack_y, unpack_x;   /* the adapter's: A's high byte, Y and X as left */

uint8_t RecordsUnpackCourse(uint8_t bank)
{
    uint16_t x, a, min = 0, sec, hund = 0;
    uint8_t c, p;
    unsigned i;

    cyc(68);
    a = rd16(0x7E0000 | W(wram_0D4A));
    cyc(52);
    a = (uint16_t)((uint16_t)(a << 1) + rd16(0x7E0000 | W(wram_0D4A)));
    x = (uint16_t)((a & 0x00FF) << 3);
    cyc(96);
    wr16(W(dp_f5), x);
    for (i = 0; i < 12; i++) {
        if (i)
            { cyc(38); x = rd16(W(dp_f5)); }
        cyc(32);
        a = rd16(0x70FC88 + (uint16_t)(x + 2 * i));
        p = 0;                          /* M and X 16-bit, binary */
        (cyc(46), TimeUnpackBcd(a, &min, &sec, &hund, &p));
        cyc(46);
        wr8(0x7E0000 | (uint16_t)(W(record_times) + 3 * i), (uint8_t)hund);
        cyc(24);
        wr8(0x7E0000 | (uint16_t)(W(record_times) + 1 + 3 * i), (uint8_t)sec);
        cyc(24);
        wr8(0x7E0000 | (uint16_t)(W(record_times) + 2 + 3 * i), (uint8_t)min);
    }
    cyc(46);
    c = rd8(0x7E0000 | W(wram_0D4A));
    cyc(80);
    c = (uint8_t)((uint8_t)(c << 3) + rd8(0x7E0000 | W(wram_0D4A)));
    cyc(14);
    for (i = 0; i < 9; i++) {
        cyc(32);
        a = rd8(0x70FC6D + (uint16_t)(c + i));
        cyc(24);
        wr8(0x7E0000 | (uint16_t)(W(wram_10CD) + 2 + i), (uint8_t)a);
    }
    cyc(70);
    unpack_high = (uint8_t)(min >> 8);
    unpack_y = (uint8_t)hund;
    unpack_x = c;
    return (uint8_t)a;
}

void RecordsUnpackCourse_cr(Carried *cr)
{
    uint8_t a = RecordsUnpackCourse(cr->db);

    cr->a = (uint16_t)(unpack_high << 8 | a);
    cr->x = unpack_x;
    cr->y = unpack_y;
}

void abi_RecordsUnpackCourse(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsUnpackCourse_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B503 RecordsUnpackCourseTotals
 * WHAT IT DOES: Unpacks course $0D4A's records (RecordsUnpackCourse) and
 * sums each group of four times (TimeTotalsSum).
 * HOW: Two calls. Not reached in the traces so far.
 * Entry: as RecordsUnpackCourse. Exit: as TimeTotalsSum.
 */
void RecordsUnpackCourseTotals(Carried *cr)
{
    cyc(46);
    RecordsUnpackCourse_cr(cr);
    (cyc(46), TimeTotalsSum_cr(cr));
    cyc(44);
}

void abi_RecordsUnpackCourseTotals(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsUnpackCourseTotals(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B50A RecordsUnpackCourseRunning
 * WHAT IT DOES: Unpacks course $0D4A's records (RecordsUnpackCourse) and
 * makes each group of four times running totals (TimeRunningTotals).
 * HOW: Two calls. Not reached in the traces so far.
 * Entry: as RecordsUnpackCourse. Exit: as TimeRunningTotals.
 */
void RecordsUnpackCourseRunning(Carried *cr)
{
    (cyc(46), RecordsUnpackCourse_cr(cr));
    (cyc(46), TimeRunningTotals_cr(cr));
    cyc(44);
}

void abi_RecordsUnpackCourseRunning(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsUnpackCourseRunning(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B511 RecordsUnpackCourseRunningB
 * WHAT IT DOES: A second copy of RecordsUnpackCourseRunning, which nothing
 * calls.
 * HOW: The same two calls.
 * Entry: as RecordsUnpackCourse. Exit: as TimeRunningTotals.
 */
void RecordsUnpackCourseRunningB(Carried *cr)
{
    cyc(46);
    RecordsUnpackCourse_cr(cr);
    (cyc(46), TimeRunningTotals_cr(cr));
    cyc(42);                            /* RTS */
}

void abi_RecordsUnpackCourseRunningB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsUnpackCourseRunningB(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The records screen's rows: direct page $F2 = the time's offset in the
   record times, $F3-$F5 = the row's name entry (two bytes of three 5-bit
   letters, then the car), $F7 = the page's offset. */
static uint32_t dp(uint16_t d, uint8_t a)
{
    return (uint16_t)(d + a);
}

/* The flags TimeUnpackBcdLong leaves, for the adapters: N, Z, C always,
   V when it computed one (not for $FFFF, no time). */
static uint8_t unpack_flags;
static int unpack_v_set;

static void time_unpack(uint16_t packed, uint16_t *min, uint16_t *sec, uint16_t *hund)
{
    uint8_t p = 0;                      /* binary: D is clear here */

    TimeUnpackBcdLong(packed, min, sec, hund, &p);
    unpack_flags = (uint8_t)(p & (P_N | P_Z | P_C | P_V));
    unpack_v_set = packed != 0xFFFF;
}

/* The unpack's flags over p. */
static uint8_t unpack_p(uint8_t p)
{
    uint8_t m = (uint8_t)(P_N | P_Z | P_C | (unpack_v_set ? P_V : 0));

    return (uint8_t)((p & ~m) | (unpack_flags & m));
}

uint8_t SpritePieces3(uint8_t bank, uint16_t *x, uint16_t *y, uint8_t *p, uint16_t *set);

/*
 * @routine 02:FF01 RecordNameLetters
 * WHAT IT DOES: Puts the three 5-bit letters of A (bits 0-4, 5-9, 10-14)
 * plus $0A into the tile bytes of sprites Y / 4, + 1 and + 2.
 * HOW: The letters are kept on the stack between them (PHA, PLA). Not
 * reached in the traces so far.
 * Entry: M 16-bit, X 16-bit; A = the letters, Y = sprite buffer offset.
 * Exit: A = the third tile; M 16-bit; N, Z, C, V clear (the last add).
 */
static uint16_t name_letters(uint8_t bank, uint16_t at, const uint16_t *clk, uint16_t v, uint16_t y)
{
    static const uint8_t shift[3] = {0, 5, 10};
    uint16_t a = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        a = (uint16_t)(((v >> shift[i]) & 0x001F) + 0x000A);
        cyc(clk[i]);
        wr8((uint32_t)bank << 16 | (uint16_t)(at + i * 4 + y), (uint8_t)a);
    }
    cyc(64);
    return a;
}

uint16_t RecordNameLetters(uint8_t bank, uint16_t letters, uint16_t y)
{
    static const uint16_t clk[3] = {190, 264, 220};

    return name_letters(bank, W(wram_039F), clk, letters, y);
}

void RecordNameLetters_cr(Carried *cr)
{
    cr->a = RecordNameLetters(cr->db, cr->a, cr->y);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_N | P_Z | P_C | P_V | P_M))));
}

void abi_RecordNameLetters(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordNameLetters_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint8_t RecordTagCarSet(uint8_t tag)
{
    uint8_t v;

    if (tag >= 0x24)
        v = 0x09;
    else if (tag >= 0x18)
        { cyc(54); v = 0x06; }
    else if (tag >= 0x0C)
        { cyc(86); v = 0x03; }
    else
        { cyc(80); v = 0x00; }
    cyc(118);
    cyc(22);                            /* REP #$20 */
    return v;
}

void RecordTagCarSet_cr(Carried *cr)
{
    uint8_t t = (uint8_t)cr->a, v = RecordTagCarSet(t);

    set_p(cr, (uint8_t)((nz8(cr->p, v) & ~(P_C | P_M)) | (t >= 0x0C ? P_C : 0)));
    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
}

void abi_RecordTagCarSet(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTagCarSet_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The time in A unpacked (TimeUnpackBcdLong) with Y kept: the hundredths
   to direct page $F1 and the seconds to $F3 (words, X 16-bit), then
   SEP #$20 with A = the minutes. */
static void record_time_unpack(uint16_t d, uint16_t packed, uint16_t *min, uint16_t *sec)
{
    uint16_t hund;

    time_unpack(packed, min, sec, &hund);
    wr16(dp(d, 0xF1), cyc_then(16, hund));
    wr16(dp(d, 0xF3), cyc_then(16, *sec));
}

/* Large digit d (0-9) at sprite buffer offset `at` + Y: its top tile $50 +
   d and its bottom tile $60 + d four bytes on, as CLC / ADC #$50 then ADC
   #$10 leave them. */
static uint8_t large_digit(uint32_t b, uint16_t y, uint16_t at, uint8_t d)
{
    uint8_t v = (uint8_t)(d + 0x50);

    cyc(76);
    wr8(b | (uint16_t)(at + y), v);
    v = (uint8_t)(v + 0x10);            /* no carry: d is a digit */
    cyc(46);
    wr8(b | (uint16_t)(at + 4 + y), v);
    return v;
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
static uint8_t digits_high;             /* the adapters': A's high byte (the minutes') */
static uint16_t digits_x;               /* X: the seconds */

uint8_t RecordTimeDigitsLarge(uint8_t bank, uint16_t d, uint16_t packed, uint16_t y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t min, sec;
    uint8_t v, t;

    (cyc(114), record_time_unpack(d, packed, &min, &sec));     /* PHP, PHY, JSL */
    digits_high = (uint8_t)(min >> 8);
    digits_x = sec;
    cyc(58);
    large_digit(b, y, W(wram_0387), (uint8_t)(min & 0x0F));
    v = rd8(cyc_then(16, dp(d, 0xF3)));
    cyc(40);
    large_digit(b, y, W(wram_0397), (uint8_t)(v >> 4));
    v = rd8(cyc_then(16, dp(d, 0xF3)));
    large_digit(b, y, W(wram_039F), (uint8_t)(v & 0x0F));
    v = rd8(cyc_then(16, dp(d, 0xF1)));
    cyc(40);
    large_digit(b, y, W(wram_03AF), (uint8_t)(v >> 4));
    v = rd8(cyc_then(16, dp(d, 0xF1)));
    t = large_digit(b, y, W(wram_03B7), (uint8_t)(v & 0x0F));
    cyc(28);
    cyc(42);                            /* PLP */
    return t;
}

void RecordTimeDigitsLarge_cr(Carried *cr)
{
    uint8_t t = RecordTimeDigitsLarge(cr->db, cr->d, cr->a, cr->y);

    cr->a = (uint16_t)(digits_high << 8 | t);
    cr->x = digits_x;
}

void abi_RecordTimeDigitsLarge(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTimeDigitsLarge_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP'd A low byte to `v`, N and Z from it (an 8-bit LDA, AND or LSR). */


/* LDA dp, LSR x 4: the high nibble, C = bit 3. */
static uint8_t nibble_c;                /* C as the last LSR x 4 left it: bit 3 */

static uint8_t dp_high_nibble(uint16_t d, uint8_t a)
{
    uint8_t v = rd8(cyc_then(16, dp(d, a)));

    nibble_c = (uint8_t)((v >> 3) & 1);
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
uint8_t RecordTimeDigitsSmall(uint8_t bank, uint16_t d, uint16_t packed, uint16_t y)
{
    uint16_t min, sec;
    uint8_t v;

    (cyc(92), record_time_unpack(d, packed, &min, &sec));      /* PHY, JSL */
    digits_high = (uint8_t)(min >> 8);
    digits_x = sec;
    v = (uint8_t)(min & 0x0F);
    cyc(104);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 14 + y), v);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 22 + y), cyc_then(86, dp_high_nibble(d, 0xF3)));
    v = (uint8_t)(rd8(cyc_then(16, dp(d, 0xF3))) & 0x0F);
    cyc(46);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 26 + y), v);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 34 + y), cyc_then(86, dp_high_nibble(d, 0xF1)));
    v = (uint8_t)(rd8(cyc_then(16, dp(d, 0xF1))) & 0x0F);
    cyc(46);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 38 + y), v);
    cyc(22);
    cyc(42);                            /* RTS */
    return v;
}

void RecordTimeDigitsSmall_cr(Carried *cr)
{
    uint8_t v = RecordTimeDigitsSmall(cr->db, cr->d, cr->a, cr->y);

    cr->a = (uint16_t)(digits_high << 8 | v);
    cr->x = digits_x;
    set_p(cr, nz8((uint8_t)((unpack_p(cr->p) & ~(P_C | P_M)) | nibble_c), v));
}

void abi_RecordTimeDigitsSmall(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTimeDigitsSmall_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint8_t RecordScoreDigits(uint8_t bank, uint16_t d, uint16_t s, uint16_t y)
{
    static const uint16_t perfect[7] = {0x3619, 0x360E, 0x361B, 0x360F, 0x360E, 0x360C, 0x361D};
    uint8_t v;
    unsigned i;

    wr16(dp(d, 0xF1), cyc_then(16, s));
    v = (uint8_t)(s & 0x0F);
    cyc(s >= 0x4000 ? 136 : 108);       /* BCS taken: + PHA */
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 46 + y), v);
    if (s >= 0x4000) {
        v = (uint8_t)s;
        nibble_c = (uint8_t)((v >> 3) & 1);
        cyc(114);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 42 + y), (uint8_t)(v >> 4));
        cyc(22);
        for (i = 0; i < 7; i++)
            { cyc(54); wr16(0x7E0000 | (uint16_t)(W(oam_buffer) + 14 + i * 4 + y), perfect[i]); }
        cyc(42);                        /* RTS */
        return 1;
    }
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 42 + y), cyc_then(86, dp_high_nibble(d, 0xF1)));
    v = (uint8_t)(rd8(cyc_then(16, dp(d, 0xF2))) & 0x0F);
    cyc(46);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 22 + y), v);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 18 + y), cyc_then(86, dp_high_nibble(d, 0xF2)));
    cyc(46);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 13 + y), 0xF0);
    cyc(30);
    wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 37 + y), 0xF0);
    cyc(22);
    cyc(42);                            /* RTS */
    return 0;
}

void RecordScoreDigits_cr(Carried *cr)
{
    if (RecordScoreDigits(cr->db, cr->d, cr->a, cr->y)) {
        cr->a = 0x361D;
        set_p(cr, (uint8_t)(cr->p & ~(P_N | P_Z | P_C | P_M)) | nibble_c);
    } else {
        cr->a = (uint16_t)((cr->a & 0xFF00) | 0xF0);
        set_p(cr, (uint8_t)(((cr->p & ~(P_Z | P_C | P_M)) | P_N) | nibble_c));
    }
}

void abi_RecordScoreDigits(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordScoreDigits_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
static uint8_t tile_flags;              /* the adapter's: N, Z, C, V of the last add */

/* CLC / ADC (16-bit), binary; the flags to tile_flags. */
static uint16_t tile_add(uint16_t a, uint16_t m)
{
    uint16_t r = (uint16_t)(a + m);

    tile_flags = (uint8_t)(nz16(0, r) | ((unsigned)a + m > 0xFFFF ? P_C : 0)
                           | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    return r;
}

uint16_t RecordLetterTile(uint16_t d, uint16_t letter)
{
    uint16_t a;

    a = tile_add(letter, 0x000A);
    wr16(dp(d, 0xF1), cyc_then(54, a));
    cyc(2);
    a = (uint16_t)((a & 0x0030) << 1);
    wr16(dp(d, 0xF3), cyc_then(52, a));
    a = rd16(cyc_then(16, dp(d, 0xF1)));
    cyc(2);
    a &= 0x000F;
    a |= rd16(cyc_then(38, dp(d, 0xF3)));
    a = tile_add(a, 0x0050);
    cyc(80);                            /* CLC, ADC #imm16, RTS */
    return a;
}

void RecordLetterTile_cr(Carried *cr)
{
    cr->a = RecordLetterTile(cr->d, cr->a);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | tile_flags));
}

void abi_RecordLetterTile(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordLetterTile_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* RecordLetterTile for letter `n`, then its top tile at `at` + Y and its
   bottom tile (+ $10, with the add's carry) four bytes on. */
static uint8_t large_letter(uint32_t b, uint16_t d, uint16_t y, uint16_t at, uint16_t n)
{
    unsigned s;
    uint8_t v;

    v = (uint8_t)(cyc(100), RecordLetterTile(d, n));
    cyc(52);
    wr8(b | (uint16_t)(at + y), v);
    s = (unsigned)v + 0x10 + (tile_flags & P_C);
    tile_flags = (uint8_t)(nz8(0, (uint8_t)s) | (s > 0xFF ? P_C : 0)
                           | ((~(v ^ 0x10) & (v ^ s) & 0x80) ? P_V : 0));
    v = (uint8_t)s;
    cyc(46);
    wr8(b | (uint16_t)(at + 4 + y), v);
    cyc(22);                            /* REP #$20 */
    return v;
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
uint8_t RecordNameLettersLarge(uint8_t bank, uint16_t d, uint16_t letters, uint16_t y)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v;

    large_letter(b, d, y, W(wram_03BF), (uint16_t)(letters & 0x001F));
    cyc(98);
    large_letter(b, d, y, 0x03C7, (uint16_t)(((uint16_t)(letters << 3) >> 8) & 0x001F));
    cyc(54);
    v = large_letter(b, d, y, 0x03CF, (uint16_t)(((uint16_t)(letters >> 2) >> 8) & 0x001F));
    cyc(42);                            /* RTS */
    return v;
}

void RecordNameLettersLarge_cr(Carried *cr)
{
    cr->a = RecordNameLettersLarge(cr->db, cr->d, cr->a, cr->y);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V | P_M)) | tile_flags));
}

void abi_RecordNameLettersLarge(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordNameLettersLarge_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:FF37 RecordNameLettersSmall
 * WHAT IT DOES: As RecordNameLetters into sprites Y / 4 + 15, + 16 and +
 * 17.
 * HOW: As RecordNameLetters. Not reached in the traces so far.
 * Entry: as RecordNameLetters. Exit: as RecordNameLetters.
 */
uint16_t RecordNameLettersSmall(uint8_t bank, uint16_t letters, uint16_t y)
{
    static const uint16_t clk[3] = {144, 264, 220};

    return name_letters(bank, W(wram_03B3), clk, letters, y);
}

void RecordNameLettersSmall_cr(Carried *cr)
{
    cr->a = RecordNameLettersSmall(cr->db, cr->a, cr->y);
    set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_N | P_Z | P_C | P_V | P_M))));
}

void abi_RecordNameLettersSmall(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordNameLettersSmall_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
/* The adapters' view of the last row drawn: X and Y as it leaves them;
   for a row with no time (hidden), V from the add of $F2. */
static int row_hidden;
static uint16_t row_x, row_y;
static uint8_t row_v;

uint16_t RecordRowDraw(uint8_t bank, uint16_t d, uint16_t sprite)
{
    static const uint16_t hi[3] = {0x039B, 0x038F, 0x0383}, lo[3] = {0x039F, 0x0393, 0x0387};
    uint32_t b = (uint32_t)bank << 16;
    uint16_t w, m, r, y, set;
    uint8_t v, t, p = 0;
    unsigned i;

    w = rd16(cyc_then(38, dp(d, 0xF7)));
    m = rd16(cyc_then(30, dp(d, 0xF2)));
    r = (uint16_t)(w + m);
    row_v = (~(w ^ m) & (w ^ r) & 0x8000) ? P_V : 0;
    row_x = (uint16_t)(r & 0x00FF);
    y = (uint16_t)(sprite << 2);
    row_y = y;
    cyc(146);
    v = rd8(0x7E0000 | (uint16_t)(W(record_times) + 1 + row_x));
    if (v == 0xFF) {
        row_hidden = 1;
        cyc(84);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 1 + y), 0xF0);
        cyc(30);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 5 + y), 0xF0);
        cyc(30);
        wr8(0x7E0000 | (uint16_t)(W(oam_buffer) + 9 + y), 0xF0);
        cyc(42);
        return (uint16_t)((y & 0xFF00) | 0xF0);
    }
    row_hidden = 0;
    for (i = 0; i < 3; i++) {
        cyc(i == 0 ? 62 : 30);
        v = rd8(0x7E0000 | (uint16_t)(W(record_times) + i + row_x));
        cyc(86);
        wr8(b | (uint16_t)(hi[i] + y), (uint8_t)(v >> 4));
        cyc(30);
        t = rd8(0x7E0000 | (uint16_t)(W(record_times) + i + row_x));
        cyc(46);
        wr8(b | (uint16_t)(lo[i] + y), (uint8_t)(t & 0x0F));
    }
    cyc(38);
    w = rd16(dp(d, 0xF5));
    cyc(44);
    w = (uint16_t)((w << 1) + rd16(dp(d, 0xF5)));
    cyc(48);
    wr16(0x7E0000 | W(sprite_piece_frame), (uint16_t)(w & 0x000F));
    cyc(48);
    wr16(0x7E0000 | W(sprite_piece_x), 0x0057);
    cyc(24);
    wr16(0x7E0000 | W(sprite_piece_palette), 0x0000);
    cyc(92);
    SpritePieces3(bank, &row_x, &y, &p, &set);
    row_y = (uint16_t)((sprite << 2) + 4);
    cyc(130);
    r = RecordNameLetters(bank, rd16(dp(d, 0xF3)), row_y);
    cyc(42);                            /* RTS */
    return r;
}

void RecordRowDraw_cr(Carried *cr)
{
    cr->a = RecordRowDraw(cr->db, cr->d, cr->y);
    cr->x = row_x;
    cr->y = row_y;
    if (row_hidden)
        set_p(cr, (uint8_t)((cr->p & ~(P_Z | P_V | P_X)) | P_N | P_C | P_M | row_v));
    else
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_N | P_Z | P_C | P_V | P_M | P_X))));
}

void abi_RecordRowDraw(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRowDraw_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Row n (0-2): its name entry ($10CF + n * 3) to $F3-$F5, its screen Y
   ($7C + n * $11) to $10BF, then RecordRowDraw from sprite $0D75 + n * 2. */
static uint16_t record_row(uint8_t bank, uint16_t d, unsigned n)
{
    uint16_t a;
    unsigned i;

    for (i = 0; i < 3; i++) {
        cyc(24);
        wr8(dp(d, (uint8_t)(0xF3 + i)), cyc_then(16, rd8(0x7E0000 | (uint16_t)(W(record_names) + n * 3 + i))));
    }
    cyc(40);
    wr8(0x7E0000 | W(sprite_piece_y), (uint8_t)(0x7C + n * 0x11));
    cyc(24);
    a = rd16(0x7E0000 | (uint16_t)(W(record_row_sprites) + n * 2));
    a = (cyc(46), RecordRowDraw(bank, d, a));
    cyc(42);
    return a;
}

/* SEP #$20, REP #$10, then the time offset to $F2. */
static void record_row_time(uint16_t d, uint8_t t)
{
    wr8(dp(d, 0xF2), cyc_then(68, t));
}

/* The registers a row draw leaves (A is the caller's to set). */
static void row_exit(Carried *cr)
{
    cr->x = row_x;
    cr->y = row_y;
    if (row_hidden)
        set_p(cr, (uint8_t)((cr->p & ~(P_Z | P_V | P_X)) | P_N | P_C | P_M | row_v));
    else
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~(P_N | P_Z | P_C | P_V | P_M | P_X))));
}

/*
 * @routine 02:F016 RecordRow1
 * WHAT IT DOES: Draws the first records row (RecordRowDraw) with the first
 * name entry, at Y = $7C from the sprite in $0D75.
 * HOW: record_row 0.
 * Entry: M 8-bit, X 16-bit; $F2 = the time's offset. Exit: as
 * RecordRowDraw.
 */
uint16_t RecordRow1(uint8_t bank, uint16_t d)
{
    return record_row(bank, d, 0);
}

void RecordRow1_cr(Carried *cr)
{
    cr->a = RecordRow1(cr->db, cr->d);
    row_exit(cr);
}

void abi_RecordRow1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRow1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F039 RecordRow2
 * WHAT IT DOES: As RecordRow1 for the second row: the second name entry,
 * Y = $8D, the sprite in $0D77.
 * HOW: record_row 1.
 * Entry: as RecordRow1. Exit: as RecordRow1.
 */
uint16_t RecordRow2(uint8_t bank, uint16_t d)
{
    return record_row(bank, d, 1);
}

void RecordRow2_cr(Carried *cr)
{
    cr->a = RecordRow2(cr->db, cr->d);
    row_exit(cr);
}

void abi_RecordRow2(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRow2_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F05C RecordRow3
 * WHAT IT DOES: As RecordRow1 for the third row: the third name entry,
 * Y = $9E, the sprite in $0D79.
 * HOW: record_row 2.
 * Entry: as RecordRow1. Exit: as RecordRow1.
 */
uint16_t RecordRow3(uint8_t bank, uint16_t d)
{
    return record_row(bank, d, 2);
}

void RecordRow3_cr(Carried *cr)
{
    cr->a = RecordRow3(cr->db, cr->d);
    row_exit(cr);
}

void abi_RecordRow3(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRow3_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F00E RecordRow1Time0
 * WHAT IT DOES: RecordRow1 with time offset 0.
 * HOW: Sets $F2 and falls into RecordRow1.
 * Entry: any widths. Exit: as RecordRow1.
 */
uint16_t RecordRow1Time0(uint8_t bank, uint16_t d)
{
    return (cyc(8), record_row_time(d, 0x00), record_row(bank, d, 0));
}

void RecordRow1Time0_cr(Carried *cr)
{
    cr->a = RecordRow1Time0(cr->db, cr->d);
    row_exit(cr);
}

void abi_RecordRow1Time0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRow1Time0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F031 RecordRow2Time4
 * WHAT IT DOES: RecordRow2 with time offset $0C (the second group's).
 * HOW: Sets $F2 and falls into RecordRow2.
 * Entry: any widths. Exit: as RecordRow2.
 */
uint16_t RecordRow2Time4(uint8_t bank, uint16_t d)
{
    return (cyc(8), record_row_time(d, 0x0C), record_row(bank, d, 1));
}

void RecordRow2Time4_cr(Carried *cr)
{
    cr->a = RecordRow2Time4(cr->db, cr->d);
    row_exit(cr);
}

void abi_RecordRow2Time4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRow2Time4_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:F054 RecordRow3Time8
 * WHAT IT DOES: RecordRow3 with time offset $18 (the third group's).
 * HOW: Sets $F2 and falls into RecordRow3.
 * Entry: any widths. Exit: as RecordRow3.
 */
uint16_t RecordRow3Time8(uint8_t bank, uint16_t d)
{
    return (cyc(8), record_row_time(d, 0x18), record_row(bank, d, 2));
}

void RecordRow3Time8_cr(Carried *cr)
{
    cr->a = RecordRow3Time8(cr->db, cr->d);
    row_exit(cr);
}

void abi_RecordRow3Time8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRow3Time8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EFE2 RecordRowsGroupFirsts
 * WHAT IT DOES: Draws the three records rows, each with the first time of
 * its group (offsets 0, $0C, $18).
 * HOW: RecordRow1Time0, RecordRow2Time4, RecordRow3Time8.
 * Entry: any widths. Exit: as RecordRow3.
 */
void RecordRowsGroupFirsts(Carried *cr)
{
    cyc(46);
    RecordRow1Time0_cr(cr);
    (cyc(46), RecordRow2Time4_cr(cr));
    (cyc(46), RecordRow3Time8_cr(cr));
    cyc(42);
}

void abi_RecordRowsGroupFirsts(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRowsGroupFirsts(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:EFEC RecordRowsFirstTimes
 * WHAT IT DOES: Draws the three records rows with times 0, 1 and 2
 * (offsets 0, 3, 6).
 * HOW: Sets $F2 before each of RecordRow1, RecordRow2, RecordRow3.
 * Entry: any widths. Exit: as RecordRow3.
 */
uint16_t RecordRowsFirstTimes(uint8_t bank, uint16_t d)
{
    uint16_t a = 0;
    unsigned n;

    for (n = 0; cyc_loop(8, n < 3); n++) {
        record_row_time(d, (uint8_t)(n * 3));
        cyc(46);
        a = record_row(bank, d, n);
    }
    cyc(42);
    return a;
}

void RecordRowsFirstTimes_cr(Carried *cr)
{
    cr->a = RecordRowsFirstTimes(cr->db, cr->d);
    row_exit(cr);
}

void abi_RecordRowsFirstTimes(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordRowsFirstTimes_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void SoundPort0QueuePush(uint8_t command);

/* $0FA3 = r, then sound $35 on port 0 unless $0E57 is set. */
/* The adapters' view of the compare: whether the sound was queued (A $35,
   N and Z from the data bank) or not (A and N, Z from $0E57); C from the
   compare that decided; X where it stopped. */
static int cmp_sound;
static uint8_t cmp_low, cmp_c;
static uint16_t cmp_x;

static void compare_result(uint32_t b, uint8_t r)
{
    uint8_t v;

    cyc(62);                            /* SEP, LDA #imm, STA.W */
    wr8(0x7E0000 | W(p1_lead), r);
    cyc(24);
    v = rd8(0x7E0000 | W(wram_0E57));
    cmp_low = v;
    cmp_sound = 0;
    if (v)
        { cyc(64); return; } else { cyc(94); }   /* BNE taken, RTS */
    cmp_sound = 1;
    cmp_low = 0x35;
    SoundPort0QueuePush(0x35);
    cyc(42);                            /* RTS */
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
static uint8_t cmp_high;                /* A's high byte: $0F2B's */

void RecordTimeCompare(uint8_t bank, uint16_t x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v, m;
    uint16_t w, i;

    cyc(24);
    w = rd16(0x7E0000 | W(special_times));
    cmp_high = (uint8_t)(w >> 8);
    cmp_x = x;
    if (w == 0xFFFF) {
        cyc(46);                        /* CMP #imm16, BEQ taken */
        cmp_c = P_C;
        compare_result(b, 0x01);
        return;
    }
    cyc(92);
    v = rd8(0x7E0000 | (uint16_t)(W(record_time_bcd) + x));
    cyc(62);
    wr8(0x7E0000 | W(wram_0F68), (uint8_t)(v & 0x0F));
    cyc(108);
    wr8(0x7E0000 | W(wram_0F67), (uint8_t)(v >> 4));
    cyc(30);
    v = rd8(0x7E0000 | (uint16_t)(W(record_time_bcd) + 1 + x));
    cyc(62);
    wr8(0x7E0000 | W(wram_0F66), (uint8_t)(v & 0x0F));
    cyc(108);
    wr8(0x7E0000 | W(wram_0F65), (uint8_t)(v >> 4));
    cyc(30);
    v = rd8(0x7E0000 | (uint16_t)(W(record_time_bcd) + 2 + x));
    cyc(40);
    wr8(0x7E0000 | W(lap_record_a), (uint8_t)(v & 0x0F));
    for (i = 0; i < 5; i++) {
        cyc(i == 0 ? 54 : 90);          /* LDX #imm16 / INX, CPX.W, BCC taken; LDA.W,X */
        v = rd8(0x7E0000 | (uint16_t)(W(lap_record_a) + i));
        cyc(30);
        m = rd8(0x7E0000 | (uint16_t)(W(lap_digits) + i));
        cmp_x = i;
        cmp_c = v >= m ? P_C : 0;
        if (v < m) {
            cyc(22);                    /* BCC taken */
            compare_result(b, 0xFF);
            return;
        }
        cyc(16);                        /* BCC not taken */
        if (v != m) {
            cyc(22);                    /* BNE taken */
            compare_result(b, 0x01);
            return;
        }
        cyc(16);                        /* BNE not taken */
    }
    cmp_x = 5;
    cmp_c = P_C;
    cyc(54);                            /* INX, CPX.W, BCC not taken */
    compare_result(b, 0x00);
}

void RecordTimeCompare_cr(Carried *cr)
{
    RecordTimeCompare(cr->db, cr->x);
    cr->a = (uint16_t)(cmp_high << 8 | cmp_low);
    cr->x = cmp_x;
    set_p(cr, (uint8_t)(((cmp_sound ? nz8(cr->p, cr->db) : nz8(cr->p, cmp_low)) & ~P_C) | P_M | cmp_c));
}

void abi_RecordTimeCompare(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTimeCompare_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

void abi_PlayerSound35(void);

/* By mode $0D2B: 0 sound $35 on port 0, 2 PlayerSound35, else
   RecordTimeCompare on record time x. P restored. */
/* What the mode dispatch did, for the adapters: 0 sound $35, 2
   PlayerSound35, else the compare. */
static uint16_t mode_seen;

static void record_compare_by_mode(Carried *cr, uint8_t bank, uint16_t x)
{

    cyc(68);
    mode_seen = rd16(0x7E0000 | W(hud_mode));
    if (mode_seen == 0x0000) {
        cyc(108);
        SoundPort0QueuePushW(0x35, 24);
        cyc(72);
    } else {
        cyc(108);
        if (mode_seen == 0x0002)
            { PlayerSound35_cr(cr); cyc(114); }  /* still in registers: it reads Y, the entry's */
        else {
            cyc(18);
            RecordTimeCompare(bank, x);
            cyc(72);                    /* PLP, RTL: RecordTimeCompare states its RTS */
        }
    }
}

/* The registers the dispatch leaves; P is the caller's, restored. */
static void compare_by_mode_exit(Carried *cr)
{
    if (mode_seen == 0x0000 || mode_seen == 0x0002) {
        cr->a = 0x0035;
    } else {
        cr->a = (uint16_t)(cmp_high << 8 | cmp_low);
        cr->x = cmp_x;
    }
}

/*
 * @routine 07:D62C RecordCompareTime0
 * WHAT IT DOES: By mode $0D2B: 0 plays sound $35, 2 PlayerSound35, others
 * compare record time 0 with the best (RecordTimeCompare).
 * HOW: 16-bit mode test.
 * Entry: any widths. Exit: as the call made; P restored.
 */
void RecordCompareTime0(Carried *cr, uint8_t bank)
{
    record_compare_by_mode(cr, bank, 0x0000);
}

void RecordCompareTime0_cr(Carried *cr)
{
    uint8_t p0 = cr->p;

    RecordCompareTime0(cr, cr->db);
    compare_by_mode_exit(cr);
    set_p(cr, p0);
}

void abi_RecordCompareTime0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordCompareTime0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D64F RecordCompareTime1
 * WHAT IT DOES: As RecordCompareTime0 for record time 1 (offset 3).
 * HOW: As RecordCompareTime0. The mode 0 sound is not reached in the
 * traces so far.
 * Entry: any widths. Exit: as RecordCompareTime0.
 */
void RecordCompareTime1(Carried *cr, uint8_t bank)
{
    record_compare_by_mode(cr, bank, 0x0003);
}

void RecordCompareTime1_cr(Carried *cr)
{
    uint8_t p0 = cr->p;

    RecordCompareTime1(cr, cr->db);
    compare_by_mode_exit(cr);
    set_p(cr, p0);
}

void abi_RecordCompareTime1(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordCompareTime1_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Swap the words at at + x and at + 2 + x (PHA/PLA through A). */
static void swap_words(uint32_t b, uint16_t at, uint16_t x, int first_read_done, uint16_t first)
{
    uint16_t v;

    if (first_read_done) {
        cyc(76);
        v = rd16(b | (uint16_t)(at + x));
        cyc(30);
        wr16(b | (uint16_t)(at + 2 + x), v);
        cyc(66);
        wr16(b | (uint16_t)(at + x), first);
    } else {
        cyc(30);
        v = rd16(b | (uint16_t)(at + x));
        cyc(60);
        wr16(b | (uint16_t)(at + x), cyc_then(30, rd16(b | (uint16_t)(at + 2 + x))));
        cyc(66);
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
        cyc(126);
        wr16(0x7E0000 | (uint16_t)(W(special_times) + y), cyc_then(30, rd16((uint32_t)(0x70FCD0 + x))));
        cyc(32);
        wr16(0x7E0000 | (uint16_t)(W(special_words) + y), cyc_then(30, rd16((uint32_t)(0x70FCD2 + x))));
        cyc(76);
        wr16(0x7E0000 | (uint16_t)(W(special_tags) + y), rd16(b | (uint16_t)(0xB4EB + y)));
        x = (uint16_t)(x + 4);
        cyc(36);
    }
    cyc(4);
    for (i = 0x14; i >= 0; i -= 2) {
        cyc(48);
        x = (uint16_t)i;
        for (;;) {
            cyc(120);
            v = rd16(0x7E0000 | (uint16_t)(W(special_times) + 2 + x));
            cyc(30);
            c = rd16(0x7E0000 | (uint16_t)(W(special_times) + x));
            if (descending ? (v < c || (cyc(16), v == c)) : v >= c)     /* BCC taken, or BCC not taken and BEQ */
                break;
            swap_words(b, W(special_times), x, 1, v);
            swap_words(b, W(special_tags), x, 0, 0);
            swap_words(b, W(special_words), x, 0, 0);
            if (x >= 0x0014)
                { cyc(24); break; }
            x = (uint16_t)(x + 2);
        }
    }
}

/* $10C9 = the place (1-3) of tag $10C7 among the first three, or 0. */
/* The loaders' exit, for the adapters: A, X and Y (X and Y when ld_xy),
   N, Z and C (ld_flags), and V when one was computed (ld_v_set). */
static uint16_t ld_a, ld_x, ld_y;
static int ld_xy, ld_v_set;
static uint8_t ld_flags, ld_v;

static uint16_t records_player_place(uint32_t b)
{
    static const uint16_t tag_at[3] = {0x0F13, 0x0F15, 0x0F17};
    cyc(126);
    uint16_t a = rd16(0x7E0000 | W(player_tag)), v = 0, t;
    uint8_t c = 0;
    int i;

    for (i = 0; i < 3; i++) {
        cyc(24);
        t = rd16(b | tag_at[i]);
        c = a >= t ? P_C : 0;           /* CMP */
        if (a == t) {
            v = (uint16_t)(i + 1);
            break;
        } else { cyc(16); }
    }
    ld_flags = (uint8_t)(nz16(0, v) | c);
    cyc(v == 2 || v == 3 ? 92 : 70);    /* the places 2 and 3 BRA to the store */
    wr16(0x7E0000 | W(player_place), v);
    return v;
}

/* The save offset of course a's entries: (a * 2 + $F3) * 16, $F3 = a. */
static uint16_t records_offset(uint16_t d, uint16_t a)
{
    uint16_t m, r;

    cyc(84);
    m = rd16((uint16_t)(d + W(dp_f3)));
    a = (uint16_t)(a << 1);
    r = (uint16_t)(a + m);
    ld_v = (~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    ld_v_set = 1;
    return (uint16_t)(r << 4);
}

/* LDA $0E3D, AND #$FF, STA $F3, CMP #$0C: the course, N, Z, C from the compare. */
static uint16_t records_course(uint32_t b, uint16_t d)
{
    uint16_t a;

    cyc(46);
    a = (uint16_t)(rd16(0x7E0000 | W(course)) & 0x00FF);
    cyc(40);
    wr16((uint16_t)(d + W(dp_f3)), a);
    ld_flags = (uint8_t)(nz16(0, (uint16_t)(a - 0x000C)) | (a >= 0x000C ? P_C : 0));
    return a;
}

uint16_t RecordsLoadSpecialCourse(uint8_t bank, uint16_t d)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t a;

    ld_xy = 0;
    a = records_course(b, d);
    ld_a = a;
    if (a < 0x000C)
        { cyc(88); return a; } else { cyc(46); }   /* CMP, BCC taken, RTS */
    if (a >= 0x0010) {
        ld_flags = (uint8_t)(nz16(0, (uint16_t)(a - 0x0010)) | P_C);
        cyc(76);
        return a;
    }
    records_load_sorted(b, records_offset(d, a), 1);
    ld_a = records_player_place(b);
    ld_x = 0xFFFE;
    ld_y = 0x0018;
    ld_xy = 1;
    cyc(42);                            /* RTS */
    return ld_a;
}

/* The loaders' registers over the current ones. */
static void load_exit(Carried *cr)
{
    cr->a = ld_a;
    if (ld_xy) {
        cr->x = ld_x;
        cr->y = ld_y;
    }
}

void RecordsLoadSpecialCourse_cr(Carried *cr)
{
    ld_v_set = 0;
    RecordsLoadSpecialCourse(cr->db, cr->d);
    load_exit(cr);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_M | P_X)) | ld_flags));
    if (ld_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | ld_v));
}

void abi_RecordsLoadSpecialCourse(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsLoadSpecialCourse_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B31B RecordsLoadSpecialCourseLong
 * WHAT IT DOES: RecordsLoadSpecialCourse for a JSL caller
 * (StuntPanelWait): with data bank $07, P and the data bank kept.
 * HOW: PHP, SEP #$20, PHB, PHK, PLB, JSR; then PLB, PLP, RTL.
 * Entry: any widths; D = $0000. Exit: P and the data bank unchanged; A,
 * X, Y as RecordsLoadSpecialCourse leaves them.
 */
uint16_t RecordsLoadSpecialCourseLong(uint16_t d)
{
    return RecordsLoadSpecialCourse(0x07, d);
}

void RecordsLoadSpecialCourseLong_cr(Carried *cr)
{
    RecordsLoadSpecialCourseLong(cr->d);
    load_exit(cr);
    if (cr->p & P_X) {                  /* the PLP; its time is the caller's */
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_RecordsLoadSpecialCourseLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsLoadSpecialCourseLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* TimeUnpackBcd on A, its Y, X, A (8-bit) stored at `to`, + 1, + 2. */
static void unpack_to(uint32_t b, uint16_t a, uint16_t to)
{
    uint16_t min, sec, hund;
    uint8_t p = 0;                      /* M and X 16-bit, binary */

    (cyc(46), TimeUnpackBcd(a, &min, &sec, &hund, &p));
    ld_flags = (uint8_t)(p & (P_N | P_Z | P_C));
    if (a != 0xFFFF)
        { ld_v = (uint8_t)(p & P_V); ld_v_set = 1; }
    cyc(22);                            /* SEP #$30 */
    cyc(24);
    wr8(b | to, (uint8_t)hund);
    cyc(24);
    wr8(b | (uint16_t)(to + 1), (uint8_t)sec);
    cyc(24);
    wr8(b | (uint16_t)(to + 2), (uint8_t)min);
    ld_a = min;
    ld_x = (uint8_t)sec;
    ld_y = (uint8_t)hund;
    ld_xy = 1;
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
void RecordsLoadCourse(uint8_t bank, uint16_t d)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t a, x;

    a = records_course(b, d);
    if (a < 0x000C)
        records_load_sorted(b, records_offset(d, a), 0);
    records_player_place(b);
    cyc(24);
    x = (uint16_t)((rd16(0x7E0000 | W(course)) & 0x00FF) << 2);
    cyc(150);
    unpack_to(b, rd16((uint32_t)(0x70FFD0 + x)), 0x0F5B);
    cyc(90);
    unpack_to(b, rd16((uint32_t)(0x70FFD2 + x)), 0x0F5E);
    cyc(46);
    unpack_to(b, rd16(0x7E0000 | W(special_times)), 0x0F61);
    cyc(64);                            /* REP, RTS */
}

void RecordsLoadCourse_cr(Carried *cr)
{
    ld_v_set = 0;
    RecordsLoadCourse(cr->db, cr->d);
    load_exit(cr);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C)) | ld_flags | P_M | P_X));
    if (ld_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | ld_v));
}

void abi_RecordsLoadCourse(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsLoadCourse_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B3D4 RecordsLoadForCourse
 * WHAT IT DOES: Loads course $0E3D's records: RecordsLoadSpecialCourse for
 * courses $0C-$0F, RecordsLoadCourse below $0C, nothing above.
 * HOW: The data bank is set to $07 (the tag table's) and restored.
 * Entry: any widths; D = $0000. Exit: P and the data bank restored.
 */
static int ld_none;                     /* RecordsLoadForCourse loaded nothing: A's low byte the course */

void RecordsLoadForCourse(uint16_t d)
{
    uint32_t tail = 144;
    uint8_t v;

    cyc(140);
    v = rd8(0x070000 | 0x0E3D);
    ld_xy = 0;
    if (v < 0x0C)
        { cyc(84); RecordsLoadCourse(0x07, d); tail = 122; }
    else if (v < 0x10)
        { cyc(110); RecordsLoadSpecialCourse(0x07, d); } else { cyc(6); tail = 186; ld_a = v; ld_none = 1; return (void)cyc(tail); }
    ld_none = 0;
    cyc(tail);                          /* (BRA; BCS taken) SEP, PLB, PLP, RTL */
}

void RecordsLoadForCourse_cr(Carried *cr)
{
    RecordsLoadForCourse(cr->d);
    if (ld_none)
        cr->a = (uint16_t)((cr->a & 0xFF00) | ld_a);
    else
        load_exit(cr);
    if (cr->p & P_X) {                  /* the PLP, its time in the tail */
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_RecordsLoadForCourse(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsLoadForCourse_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B3F2 RecordsLoadCourseLong
 * WHAT IT DOES: RecordsLoadCourse for callers in other banks, with the
 * data bank $07.
 * HOW: As RecordsLoadForCourse. Not reached in the traces so far.
 * Entry: any widths; D = $0000. Exit: P and the data bank restored.
 */
void RecordsLoadCourseLong(uint16_t d)
{
    cyc(162);
    RecordsLoadCourse(0x07, d);
    cyc(122);                           /* SEP, PLB, PLP, RTL */
}

void RecordsLoadCourseLong_cr(Carried *cr)
{
    RecordsLoadCourseLong(cr->d);
    load_exit(cr);
    if (cr->p & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void abi_RecordsLoadCourseLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsLoadCourseLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

uint16_t RecordTagCar0(uint8_t bank);
uint16_t RecordTagCar4(uint8_t bank);
uint16_t RecordTagCar8(uint8_t bank);

#define SPECIAL_SAVES 0x70FCD0u
#define SPECIAL_LAPS 0x70FFD0u

/* A direct page address (bank 0). */


/* CPX (16-bit): N, Z and C. */


/* CLC; ADC (8-bit) of a direct page byte. */
static uint8_t chk_cv;                  /* C and V of the checksum's last add */
static uint8_t chk_x;                   /* X on exit: the course byte */

/* CLC; ADC (8-bit) of the direct page byte $F1, binary. */
static uint8_t special_add8(uint16_t d, uint8_t a)
{
    uint8_t m = rd8(cyc_then(30, (uint16_t)(d + W(digit_count))));
    unsigned s = (unsigned)a + m;

    chk_cv = (uint8_t)((s > 0xFF ? P_C : 0) | ((~(a ^ m) & (a ^ s) & 0x80) ? P_V : 0));
    return (uint8_t)s;
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
uint8_t SpecialRecordsChecksum(uint8_t bank, uint16_t d)
{
    uint16_t a, x, y;
    uint8_t s;
    int i;

    cyc(24);
    a = (uint16_t)(rd16(0x7E0000 | W(course)) & 0x00FF);
    cyc(2);
    if (a >= 0x000C) {
        cyc(84);
        s = 0x00;
    } else {
        x = (uint16_t)(a << 2);
        cyc(30);                        /* TAX */
        cyc(128);
        s = rd8(SPECIAL_LAPS + x);
        for (i = 1; i < 4; i++) {
            wr8((uint16_t)(d + W(digit_count)), cyc_then(16, s));
            cyc(32);
            s = rd8(SPECIAL_LAPS + i + x);
            s = special_add8(d, s);
        }
    }
    wr8((uint16_t)(d + W(digit_count)), cyc_then(38, s));
    y = 0x0030;
    cyc(48);
    x = rd16(0x7E0000 | W(wram_0D14));
    do {
        cyc(46);
        s = rd8(SPECIAL_SAVES + x);
        s = special_add8(d, s);
        wr8((uint16_t)(d + W(digit_count)), cyc_then(16, s));
        x++;
        y--;
    } while (cyc_loop(36, y));
    cyc(22);                            /* SEP #$10 */
    cyc(68);
    chk_x = rd8(0x7E0000 | W(course));
    s = rd8(cyc_then(16, (uint16_t)(d + W(digit_count))));
    cyc(32);
    wr8(0x70FC33 + chk_x, s);
    cyc(22);                            /* REP #$30 */
    cyc(42);
    return s;
}

/* The registers SpecialRecordsChecksum leaves. */
static void checksum_exit(Carried *cr, uint8_t s)
{
    cr->a = s;
    cr->x = chk_x;
    cr->y = 0x0000;
    set_p(cr, nz8((uint8_t)((cr->p & ~(P_C | P_V | P_M | P_X)) | chk_cv), s));
}

void SpecialRecordsChecksum_cr(Carried *cr)
{
    checksum_exit(cr, SpecialRecordsChecksum(cr->db, cr->d));
}

void abi_SpecialRecordsChecksum(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpecialRecordsChecksum_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* Entry +a's word to entry +b (the course's entries, from X). */
static void special_move(uint16_t x, uint16_t from, uint16_t to)
{
    uint16_t v;

    cyc(32);
    v = rd16(SPECIAL_SAVES + from + x);
    cyc(32);
    wr16(SPECIAL_SAVES + to + x, v);
}

/* LDA abs, LDX abs, LDY abs; TimePackBcd: one lap's packed time. */
static uint16_t special_pack(uint32_t b, uint16_t at)
{
    uint16_t hund, sec, min;
    uint8_t p = 0;                      /* 16-bit registers, binary */

    cyc(40);
    hund = rd16(b | at);
    cyc(24);
    sec = rd16(b | (uint16_t)(at + 1));
    cyc(24);
    min = rd16(b | (uint16_t)(at + 2));
    return (cyc(46), TimePackBcd(hund, sec, min, &p));
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
/* SpecialRecordInsert's exit, for its adapter: none (A the time), $FFFF
   (no place) or the checksum's; X and Y when it changed them; C for the
   none case, V when an add computed one. */
enum { INS_NONE, INS_NO_PLACE, INS_CHECKSUM };
static int ins_kind, ins_x_set, ins_y_set, ins_v_set;
static uint16_t ins_x, ins_y;
static uint8_t ins_v, ins_sum;

/* CLC; ADC (16-bit), binary; V to ins_v. */
static uint16_t ins_add(uint16_t a, uint16_t m)
{
    uint16_t r = (uint16_t)(a + m);

    ins_v = (~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    ins_v_set = 1;
    return r;
}

void SpecialRecordInsert(uint8_t bank, uint16_t d, uint16_t t)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t a, x, y, n, r1, r2;

    ins_x_set = ins_y_set = ins_v_set = 0;
    cyc(54);
    if (!rd16(0x7E0000 | W(hud_mode))) {
        cyc(40);
        a = (uint16_t)((rd16(0x7E0000 | W(wram_0D48)) << 1) & 0x00FF);
        cyc(2);
        cyc(30);                        /* TAX */
        ins_x = a;
        ins_x_set = 1;
        cyc(50);
        wr16(b + 0x0F6E + a, a);
    } else { cyc(22); }
    cyc(24);
    a = (uint16_t)(rd16(0x7E0000 | W(course)) & 0x00FF);
    cyc(2);
    if (a >= 0x000C)
        goto none;
    wr16((uint16_t)(d + W(digit_count)), cyc_then(84, a));
    a = ins_add((uint16_t)(a << 1), rd16(cyc_then(44, (uint16_t)(d + W(digit_count)))));
    a = (uint16_t)(a << 4);
    wr16((uint16_t)(d + W(digit_count)), cyc_then(72, a));
    cyc(24);
    wr16(0x7E0000 | W(wram_0D14), a);
    y = a;                              /* TAY */
    ins_y = y;
    ins_y_set = 1;
    cyc(38);
    a = (uint16_t)(rd16(0x7E0000 | W(wram_0D89)) & 0x00FF);
    cyc(2);
    if (a >= 0x0004)
        { cyc(6); goto none; }
    wr16((uint16_t)(d + W(dp_f3)), cyc_then(78, a));
    a = ins_add((uint16_t)(a << 1), rd16(cyc_then(44, (uint16_t)(d + W(dp_f3)))));
    a = (uint16_t)(a << 2);
    a = ins_add(a, rd16(cyc_then(58, (uint16_t)(d + W(digit_count)))));
    cyc(30);                            /* TAX */
    x = a;
    ins_x = x;
    ins_x_set = 1;
    wr16((uint16_t)(d + W(record_page)), x);
    cyc(68);
    if (t < rd16(SPECIAL_SAVES + x)) {
        cyc(98);
        RecordTagCar0(bank);
        special_move(x, 4, 8);
        special_move(x, 6, 10);
        special_move(x, 0, 4);
        special_move(x, 2, 6);
        cyc(24);
        a = rd16(0x7E0000 | W(wram_0F11));
        cyc(32);
        wr16(SPECIAL_SAVES + 2 + x, a);
        cyc(68);
        wr16(SPECIAL_SAVES + x, t);
        wr16((uint16_t)(d + W(digit_count)), cyc_then(16, x));
        wr16((uint16_t)(d + W(save_block_offset)), cyc_then(40, 0x000C));
        x = y;
        do {
            if (x != rd16(cyc_then(30, (uint16_t)(d + W(digit_count))))) {
                cyc(48);
                if (t >= rd16(SPECIAL_SAVES + x))
                    { cyc(36); goto checksum; } else { cyc(4); }
            } else { cyc(4); }
            x = (uint16_t)(x + 4);
            n = (uint16_t)(rd16(cyc_then(90, (uint16_t)(d + W(save_block_offset)))) - 1);
            wr16((uint16_t)(d + W(save_block_offset)), cyc_then(14, n));
        } while (cyc_loop(8, n));
        r1 = special_pack(b, W(lap_times));
        cyc(14);
        r2 = special_pack(b, W(wram_0EF3));
        cyc(54);
        x = (uint16_t)((rd16(0x7E0000 | W(course)) & 0x00FF) << 2);
        cyc(2);
        cyc(30);                        /* TAX */
        cyc(102);
        wr16(SPECIAL_LAPS + 2 + x, r2);
        cyc(68);
        wr16(SPECIAL_LAPS + x, r1);
        goto checksum;
    }
    cyc(68);
    if (t < rd16(SPECIAL_SAVES + 4 + x)) {
        cyc(88);
        RecordTagCar4(bank);
        cyc(42);
        special_move(x, 4, 8);
        special_move(x, 6, 10);
        cyc(68);
        wr16(SPECIAL_SAVES + 4 + x, t);
        cyc(24);
        a = rd16(0x7E0000 | W(wram_0F11));
        cyc(32);
        wr16(SPECIAL_SAVES + 6 + x, a);
        goto checksum;
    }
    cyc(50);
    if (t >= rd16(SPECIAL_SAVES + 8 + x)) {
        cyc(22);
        cyc(38);
        cyc(28);
        wr16(0x7E0000 | W(player_tag), 0xFFFF);
        ins_kind = INS_NO_PLACE;
        return;
    }
    cyc(44);
    wr16(SPECIAL_SAVES + 8 + x, t);
    RecordTagCar8(bank);
    cyc(66);
    a = rd16(0x7E0000 | W(wram_0F11));
    cyc(32);
    wr16(SPECIAL_SAVES + 10 + x, a);
checksum:
    cyc(22);
    ins_sum = SpecialRecordsChecksum(bank, d);
    ins_kind = INS_CHECKSUM;
    return;
none:
    cyc(140);                           /* PLA, RTS */
    ins_kind = INS_NONE;
}

void SpecialRecordInsert_cr(Carried *cr)
{
    uint16_t t = cr->a;

    SpecialRecordInsert(cr->db, cr->d, t);
    if (ins_kind == INS_CHECKSUM) {
        checksum_exit(cr, ins_sum);
        return;
    }
    if (ins_x_set)
        cr->x = ins_x;
    if (ins_y_set)
        cr->y = ins_y;
    if (ins_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | ins_v));
    if (ins_kind == INS_NO_PLACE) {
        cr->a = 0xFFFF;
        set_p(cr, (uint8_t)((cr->p & ~(P_Z | P_M | P_X)) | P_N | P_C));
    } else {
        cr->a = t;
        set_p(cr, (uint8_t)(nz16(cr->p, t) | P_C));
    }
}

void abi_SpecialRecordInsert(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SpecialRecordInsert_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B077 RecordCarTimes12
 * WHAT IT DOES: A = $0D89 * 12, low byte only.
 * HOW: ASL, CLC, ADC $0D89, ASL, ASL, AND #$FF (16-bit); through the data
 * bank.
 * Entry: M 16-bit. Exit: A = the value; N, Z from it; C from the last
 * shift; V from the add.
 */
static uint8_t car12_flags;             /* N, Z, C, V as RecordCarTimes12 leaves them */

uint16_t RecordCarTimes12(uint8_t bank)
{
    uint16_t a, m, r;

    cyc(24);
    a = (uint16_t)(rd16(0x7E0000 | W(wram_0D89)) << 1);
    cyc(52);
    m = rd16(0x7E0000 | W(wram_0D89));
    r = (uint16_t)(a + m);
    car12_flags = (uint8_t)(((r >> 14) & 1)                        /* the last ASL */
                            | ((~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0));
    a = (uint16_t)((r << 2) & 0x00FF);
    car12_flags |= nz16(0, a);
    cyc(2);
    cyc(92);
    return a;
}

void RecordCarTimes12_cr(Carried *cr)
{
    cr->a = RecordCarTimes12(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | car12_flags));
}

void abi_RecordCarTimes12(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordCarTimes12_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* JSR RecordCarTimes12, (CLC, ADC #extra,) STA $10C7. */
static uint16_t record_tag(uint8_t bank, uint16_t extra)
{
    uint16_t a = RecordCarTimes12(bank), r;

    if (extra) {
        r = (uint16_t)(a + extra);
        car12_flags = (uint8_t)(nz16(0, r) | ((unsigned)a + extra > 0xFFFF ? P_C : 0)
                                | ((~(a ^ extra) & (a ^ r) & 0x8000) ? P_V : 0));
        a = r;
    }
    /* BRA, STA; CLC, ADC, BRA, STA for +4; CLC, ADC, STA for +8 */
    cyc(extra == 0x0004 ? 84 : extra ? 62 : 46);
    wr16(0x7E0000 | W(player_tag), a);
    cyc(42);
    return a;
}

/*
 * @routine 07:B05E RecordTagCar0
 * WHAT IT DOES: Sets the tag $10C7 to $0D89 * 12 (low byte), the place of
 * the car's first record.
 * HOW: RecordCarTimes12; the entries at $07:B063 and $07:B06C add 4 and 8.
 * Entry: M 16-bit. Exit: A = the tag; N, Z from RecordCarTimes12's AND.
 */
uint16_t RecordTagCar0(uint8_t bank)
{
    cyc(46);
    return record_tag(bank, 0);
}

void RecordTagCar0_cr(Carried *cr)
{
    cr->a = RecordTagCar0(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | car12_flags));
}

void abi_RecordTagCar0(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTagCar0_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B063 RecordTagCar4
 * WHAT IT DOES: As RecordTagCar0 plus 4 (the second record).
 * HOW: As RecordTagCar0 with CLC, ADC #4; no script reaches it (unit_cpu
 * does).
 * Entry: M 16-bit. Exit: A = the tag; N, Z, C, V from the add.
 */
uint16_t RecordTagCar4(uint8_t bank)
{
    cyc(46);
    return record_tag(bank, 0x0004);
}

void RecordTagCar4_cr(Carried *cr)
{
    cr->a = RecordTagCar4(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | car12_flags));
}

void abi_RecordTagCar4(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTagCar4_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:B06C RecordTagCar8
 * WHAT IT DOES: As RecordTagCar0 plus 8 (the third record).
 * HOW: As RecordTagCar4 with 8; no script reaches it (unit_cpu does).
 * Entry: M 16-bit. Exit: as RecordTagCar4.
 */
uint16_t RecordTagCar8(uint8_t bank)
{
    cyc(46);
    return record_tag(bank, 0x0008);
}

void RecordTagCar8_cr(Carried *cr)
{
    cr->a = RecordTagCar8(cr->db);
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | car12_flags));
}

void abi_RecordTagCar8(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordTagCar8_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint8_t SaveRecordSumsTimeUp(void)
{
    static const uint8_t at[4] = {0x00, 0x05, 0x0A, 0x0F};
    uint8_t x, v = 0;
    int i;

    cyc(22);                            /* SEP #$30 */
    cyc(62);
    wr8(W(time_total), 0x00);
    sums_p = 0;
    for (i = 0; i < 4; i++) {
        cyc(62);
        x = at[i];
        v = SaveRecordSumA(&x, &sums_p);
        cyc(72);
        wr8(0x70FC46 + (uint32_t)i, v);
    }
    cyc(42);                            /* RTS */
    return v;
}

void SaveRecordSumsTimeUp_cr(Carried *cr)
{
    uint8_t v = SaveRecordSumsTimeUp();

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    cr->x = 0x0014;
    cr->y = 0x0000;
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | (sums_p & (P_N | P_Z | P_C | P_V)) | P_M | P_X));
}

void abi_SaveRecordSumsTimeUp(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveRecordSumsTimeUp_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* SEP #$20, LDA #v, STA.L $000FA3: player 1's lead byte. */
static void time_up_lead(uint8_t v)
{
    cyc(48);
    wr8(W(p1_lead), v);
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
/* TimeUpRecordUpdate's exit, for its adapter: the lead byte it set and,
   when it was a new record, the sums' registers. */
static uint8_t up_lead, up_cv, up_sum;
static uint16_t up_t, up_x, up_y, up_high;

uint8_t TimeUpRecordUpdate(uint8_t bank)
{
    uint16_t a, sec, min, k, m, x, r, w;
    uint8_t v, p = 0;

    cyc(46);
    v = rd8(0x7E0000 | W(race_time_minutes));
    cyc(40);
    wr8(0x7E0000 | W(lap_digits), (uint8_t)(v & 0x0F));
    cyc(24);
    v = rd8(0x7E0000 | W(race_time_seconds));
    cyc(80);
    wr8(0x7E0000 | W(wram_0F6A), (uint8_t)(v >> 4));
    cyc(24);
    v = rd8(0x7E0000 | W(race_time_seconds));
    cyc(40);
    wr8(0x7E0000 | W(wram_0F6B), (uint8_t)(v & 0x0F));
    cyc(24);
    v = rd8(0x7E0000 | W(race_time_fraction));
    cyc(80);
    wr8(0x7E0000 | W(wram_0F6C), (uint8_t)(v >> 4));
    cyc(24);
    v = (uint8_t)(rd8(0x7E0000 | W(race_time_fraction)) & 0x0F);
    cyc(40);
    wr8(0x7E0000 | W(wram_0F6D), v);
    cyc(22);                            /* REP #$30 */
    cyc(24);
    a = rd16(0x7E0000 | W(race_time_fraction));
    cyc(24);
    sec = rd16(0x7E0000 | W(race_time_seconds));
    cyc(24);
    min = rd16(0x7E0000 | W(race_time_minutes));
    up_y = min;
    up_t = (cyc(46), TimePackBcd(a, sec, min, &p));   /* 16-bit, binary */
    cyc(54);
    k = (uint16_t)(rd16(0x7E0000 | W(course)) - 0x0017);
    cyc(6);                             /* the SBC */
    cyc(48);
    wr16(W(time_total), k);
    k = (uint16_t)(k << 2);
    cyc(58);
    m = rd16(W(time_total));
    r = (uint16_t)(k + m);
    up_cv = (~(k ^ m) & (k ^ r) & 0x8000) ? P_V : 0;
    x = (uint16_t)(r & 0x00FF);
    up_x = x;
    cyc(106);
    r = rd16(0x70FC4A + (uint32_t)x);
    if (up_t >= r)
        { cyc(44); up_cv |= P_C; }
    if (up_t == r) {
        time_up_lead(up_lead = 0x00);
        cyc(42);                        /* RTS */
        return 0;
    }
    if (up_t > r) {
        cyc(16);
        time_up_lead(up_lead = 0x01);
        cyc(42);                        /* RTS */
        return 0;
    }
    cyc(64);
    wr16(0x70FC4A + (uint32_t)x, up_t);
    cyc(24);
    w = rd16(0x7E0000 | W(wram_0F11));
    up_high = (uint16_t)(w & 0xFF00);
    cyc(32);
    wr16(0x70FC4C + (uint32_t)x, w);
    cyc(46);
    v = rd8(0x7E0000 | W(wram_0D89));
    cyc(32);
    wr8(0x70FC4E + (uint32_t)x, v);
    time_up_lead(up_lead = 0xFF);
    up_sum = SaveRecordSumsTimeUp();
    cyc(42);                            /* RTS */
    return 1;
}

void TimeUpRecordUpdate_cr(Carried *cr)
{
    if (TimeUpRecordUpdate(cr->db)) {
        cr->a = (uint16_t)(up_high | up_sum);
        cr->x = 0x0014;
        cr->y = 0x0000;
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V)) | (sums_p & (P_N | P_Z | P_C | P_V)) | P_M | P_X));
    } else {
        cr->a = (uint16_t)((up_t & 0xFF00) | up_lead);
        cr->x = up_x;
        cr->y = up_y;
        set_p(cr, nz8((uint8_t)((cr->p & ~(P_C | P_V | P_X)) | P_M | up_cv), up_lead));
    }
}

void abi_TimeUpRecordUpdate(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    TimeUpRecordUpdate_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
 * pushed as the return address (a score of $AE28 lands on that RTS, which
 * then returns; in play the courses here are $0C-$0F).
 * HOW: SED for the BCD add; $F1 holds course * 48 (also $0D14), $F3 the
 * car, $F7 the entry's offset; the score is kept on the stack.
 * Entry: any widths; data bank $07, D = $0000. Exit: M 16-bit, X 16-bit;
 * A as the last step leaves it.
 */
/* SED; CLC; ADC (8-bit): the BCD add as the 65816 does it, V from the
   sum before its last adjust; N, Z, C, V to *f. */
static uint8_t bcd_add8(uint8_t a, uint8_t m, uint8_t *f)
{
    int c, r;

    r = (a & 0x0F) + (m & 0x0F);
    if (r > 0x09) r += 0x06;
    c = r > 0x0F;
    r = (a & 0xF0) + (m & 0xF0) + (c << 4) + (r & 0x0F);
    *f = (uint8_t)((~(a ^ m) & (a ^ r) & 0x80) ? P_V : 0);
    if (r > 0x9F) r += 0x60;
    if (r > 0xFF) *f |= P_C;
    *f = nz8(*f, (uint8_t)r);
    return (uint8_t)r;
}

/* StuntRecordTime's exit, for its adapter. */
enum { STUNT_OTHER_COURSE, STUNT_NO_PLACE, STUNT_SUMMED };
static int stunt_kind;
static uint16_t stunt_a, stunt_x, stunt_y;
static uint8_t stunt_flags, stunt_v;

uint8_t StuntRecordTime(Carried *cr, uint8_t bank, uint16_t d)
{
    uint16_t t, a, x, y, n, m, r, sum_at, e;
    uint8_t h, lo, sum = 0, f;

    cyc(46);
    h = rd8(0x7E0000 | W(hud_digits_19ec));
    cyc(58);
    lo = rd8(0x7E0000 | W(race_time_bcd));
    cyc(38);
    lo = bcd_add8(lo, rd8(0x7E0000 | W(wram_19F0)), &f);
    stunt_v = (uint8_t)(f & P_V);
    cyc(22);                            /* REP #$30 */
    t = (uint16_t)(h << 8 | lo);
    cyc(68);
    a = (uint16_t)(rd16(0x7E0000 | W(course)) & 0x00FF);
    cyc(2);
    if (a < 0x000C || a >= 0x0010) {
        /* the score pushed (PHA, 16-bit: high byte first) is what the RTS
           at 07:AE29 pulls as its return address: on at $07:score + 1 */
        uint16_t ret;

        stunt_kind = STUNT_OTHER_COURSE;
        stunt_a = a;
        stunt_flags = a < 0x000C ? (uint8_t)(nz16(0, (uint16_t)(a - 0x000C)))
                                 : (uint8_t)(nz16(0, (uint16_t)(a - 0x0010)) | P_C);
        cyc(88);
        wr8((uint16_t)cpu.s, (uint8_t)(t >> 8));
        wr8((uint16_t)(cpu.s - 1), (uint8_t)t);
        ret = rd16((uint16_t)(cpu.s - 1));
        if ((uint16_t)(ret + 1) == 0xAE29)
            { cyc(48); return 0; }      /* this RTS again: it returns to the caller */
        /* the jump is to code still in registers: the state it has here */
        cr->a = a;
        set_p(cr, (uint8_t)((cr->p & P_I) | stunt_flags | stunt_v));   /* SED ... CLD: D clear */
        carried_jump(cr, 0x070000 | (uint16_t)(ret + 1));
        return 0;
    }
    wr16((uint16_t)(d + W(digit_count)), cyc_then(124, a));
    a = (uint16_t)(a << 1);
    m = rd16(cyc_then(44, (uint16_t)(d + W(digit_count))));
    r = (uint16_t)(a + m);
    stunt_v = (~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    a = (uint16_t)(r << 4);
    wr16((uint16_t)(d + W(digit_count)), cyc_then(72, a));
    cyc(24);
    wr16(0x7E0000 | W(wram_0D14), a);
    y = a;
    cyc(38);
    a = (uint16_t)(rd16(0x7E0000 | W(wram_0D89)) & 0x00FF);
    cyc(2);
    wr16((uint16_t)(d + W(dp_f3)), cyc_then(38, a));
    a = (uint16_t)(a << 1);
    m = rd16(cyc_then(44, (uint16_t)(d + W(dp_f3))));
    r = (uint16_t)(a + m);
    a = (uint16_t)(r << 2);
    m = rd16(cyc_then(58, (uint16_t)(d + W(digit_count))));
    r = (uint16_t)(a + m);
    stunt_v = (~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    cyc(30);                            /* TAX */
    x = r;
    wr16((uint16_t)(d + W(record_page)), x);
    cyc(68);
    e = rd16(SPECIAL_SAVES + x);
    if (t >= e && (cyc(108), t != e)) {
        RecordTagCar0(bank);
        special_move(x, 4, 8);
        special_move(x, 6, 10);
        special_move(x, 0, 4);
        special_move(x, 2, 6);
        cyc(24);
        a = rd16(0x7E0000 | W(wram_0F11));
        cyc(32);
        wr16(SPECIAL_SAVES + 2 + x, a);
        cyc(68);
        wr16(SPECIAL_SAVES + x, t);
        a = t;
        sum_at = 78;                    /* JMP */
    } else {
        cyc(t >= e ? 70 : 54);          /* BCC taken, or BCC not taken and BEQ taken; CMP.L */
        e = rd16(SPECIAL_SAVES + 4 + x);
        if (t >= e && (cyc(108), t != e)) {
            RecordTagCar4(bank);
            special_move(x, 4, 8);
            special_move(x, 6, 10);
            cyc(68);
            wr16(SPECIAL_SAVES + 4 + x, t);
            cyc(24);
            a = rd16(0x7E0000 | W(wram_0F11));
            cyc(32);
            wr16(SPECIAL_SAVES + 6 + x, a);
            sum_at = 76;                /* BRA */
        } else {
            cyc(t >= e ? 70 : 54);
            e = rd16(SPECIAL_SAVES + 8 + x);
            if (t < e || t == e) {
                cyc(t >= e ? 38 : 22);  /* BCC taken, or BCC not taken and BEQ taken */
                cyc(22);
                cyc(42);                /* RTS */
                stunt_kind = STUNT_NO_PLACE;
                stunt_a = t;
                stunt_x = x;
                stunt_y = y;
                stunt_flags = (uint8_t)(nz16(0, (uint16_t)(t - e)) | (t >= e ? P_C : 0));
                return 0;
            }
            cyc(64);
            wr16(SPECIAL_SAVES + 8 + x, t);
            (cyc(46), RecordTagCar8(bank));
            cyc(24);
            sum_at = 54;
            a = rd16(0x7E0000 | W(wram_0F11));
            cyc(32);
            wr16(SPECIAL_SAVES + 10 + x, a);
        }
    }
    wr8((uint16_t)(d + W(digit_count)), cyc_then(sum_at, 0x00));
    rd16(cyc_then(40, (uint16_t)(d + W(record_page))));
    cyc(24);
    x = rd16(0x7E0000 | W(wram_0D14));
    n = 0x0030;
    do {
        cyc(n == 0x0030 ? 32 : 82);
        uint8_t v = rd8(SPECIAL_SAVES + x), w = rd8(cyc_then(30, (uint16_t)(d + W(digit_count))));
        unsigned s = (unsigned)v + w;

        stunt_v = (~(v ^ w) & (v ^ s) & 0x80) ? P_V : 0;
        stunt_flags = s > 0xFF ? P_C : 0;
        sum = (uint8_t)s;
        wr8((uint16_t)(d + W(digit_count)), cyc_then(16, sum));
        x = (uint16_t)(x + 1);
        n = (uint16_t)(n - 1);
    } while (n);
    cyc(22);                            /* SEP #$10 */
    cyc(68);
    stunt_x = rd8(0x7E0000 | W(course));
    sum = rd8(cyc_then(16, (uint16_t)(d + W(digit_count))));
    cyc(32);
    wr8(0x70FC33 + stunt_x, sum);
    cyc(22);
    cyc(42);                            /* RTS */
    stunt_kind = STUNT_SUMMED;
    stunt_a = (uint16_t)((a & 0xFF00) | sum);
    stunt_flags = (uint8_t)(nz8(0, sum) | stunt_flags);
    return sum;
}

void StuntRecordTime_cr(Carried *cr)
{
    StuntRecordTime(cr, cr->db, cr->d);
    cr->a = stunt_a;
    if (stunt_kind == STUNT_SUMMED) {
        cr->x = stunt_x;
        cr->y = 0x0000;
    } else if (stunt_kind == STUNT_NO_PLACE) {
        cr->x = stunt_x;
        cr->y = stunt_y;
    }
    set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C | P_V | P_M | P_X | P_D)) | stunt_flags | stunt_v));
}

void abi_StuntRecordTime(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    StuntRecordTime_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * The records panel of the course screens: four rows of three saved entries
 * drawn one character a frame into the tilemap buffer at $7F:93D7.
 * $0FA6 is the character (0-6 for Stunt Trax, 0-7 for the other courses),
 * $0FA8 the row (0-3) and $0FAA the place (0-2).
 */

/* LDA $0FA8; ASL; TAX; LDA.L row_table,X; STA $F3; then + $0FAA * 64 and +
   the character's byte from char_table at $0FA6: Y = A = $F3, X = $0FA6. */
/* The records panel's flags, for the adapters: C (and V when rc_v_set)
   as the last step leaves them. */
static uint8_t rc_c, rc_v;
static int rc_v_set;
static uint16_t rc_x;                   /* X as left */
static int rc_m16;                      /* a step set M 16-bit (REP #$20) */
static int rc_in_draw;                  /* inside a draw: Y is the offset (rc_y) */
static int rc_preview;                  /* inside a preview's long entry: M 16-bit by its REP */
static uint16_t rc_y;

/* CLC; ADC (16-bit), binary: C and V to rc_c, rc_v. */
static uint16_t rc_add(uint16_t a, uint16_t m)
{
    uint16_t r = (uint16_t)(a + m);

    rc_c = (unsigned)a + m > 0xFFFF ? P_C : 0;
    rc_v = (~(a ^ m) & (a ^ r) & 0x8000) ? P_V : 0;
    rc_v_set = 1;
    return r;
}

static uint16_t records_char_offset(uint8_t bank, uint32_t row_table, uint32_t char_table)
{
    uint16_t a, x;

    cyc(232);
    x = (uint16_t)(rd16(0x7E0000 | W(records_row)) << 1);
    cyc(30);                            /* TAX */
    cyc(62);
    a = rd16(row_table + x);
    wr16(W(record_row_name), a);
    cyc(24);
    a = (uint16_t)(rd16(0x7E0000 | W(records_place)) & 0x00FF);
    cyc(2);
    a = (uint16_t)(a << 6);
    cyc(136);
    a = rc_add(a, rd16(W(record_row_name)));
    cyc(16);
    wr16(W(record_row_name), a);
    cyc(24);
    rc_x = rd16(0x7E0000 | W(records_char));
    cyc(100);
    a = (uint16_t)(rd16(char_table + rc_x) & 0x00FF);
    cyc(2);
    a = rc_add(a, rd16(W(record_row_name)));
    cyc(16);
    wr16(W(record_row_name), a);
    return a;
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
uint16_t RecordsCharOffsetStunt(uint8_t bank)
{
    return records_char_offset(bank, 0x07D8D4, 0x07D8DC);
}

/* The registers the offset routines leave: A = Y = the offset. */
static void char_offset_exit(Carried *cr, uint16_t r)
{
    cr->a = cr->y = r;
    cr->x = rc_x;
    set_p(cr, nz16((uint8_t)((cr->p & ~(P_C | P_V)) | rc_c | rc_v), r));
}

void RecordsCharOffsetStunt_cr(Carried *cr)
{
    char_offset_exit(cr, RecordsCharOffsetStunt(cr->db));
}

void abi_RecordsCharOffsetStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsCharOffsetStunt_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D8A7 RecordsCharOffsetAll
 * WHAT IT DOES: As RecordsCharOffsetStunt for the other courses' records:
 * row starts at $07:D8E3, columns at $07:D8EB.
 * HOW: As RecordsCharOffsetStunt.
 * Entry: M 16-bit, X 16-bit; D = $0000. Exit: as RecordsCharOffsetStunt.
 */
uint16_t RecordsCharOffsetAll(uint8_t bank)
{
    return records_char_offset(bank, 0x07D8E3, 0x07D8EB);
}

void RecordsCharOffsetAll_cr(Carried *cr)
{
    char_offset_exit(cr, RecordsCharOffsetAll(cr->db));
}

void abi_RecordsCharOffsetAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsCharOffsetAll_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint16_t RecordsEntryLoad(uint8_t bank)
{
    uint16_t a, v;

    cyc(126);
    a = (uint16_t)(rd16(0x7E0000 | W(course)) << 1);
    cyc(52);
    a = rc_add(a, rd16(0x7E0000 | W(course)));
    cyc(2);
    a = (uint16_t)((a & 0x00FF) << 4);
    cyc(94);
    wr16(W(time_total), a);
    cyc(24);
    a = (uint16_t)(rd16(0x7E0000 | W(records_row)) & 0x00FF);
    cyc(2);
    a = (uint16_t)(a << 1);
    cyc(74);
    a = rc_add(a, rd16(0x7E0000 | W(records_row)));
    a = (uint16_t)(a << 2);
    cyc(58);
    a = rc_add(a, rd16(W(time_total)));
    cyc(16);
    wr16(W(time_total), a);
    cyc(24);
    a = (uint16_t)(rd16(0x7E0000 | W(records_place)) << 2);
    cyc(58);
    a = rc_add(a, rd16(W(time_total)));
    cyc(16);
    wr16(W(time_total), a);
    cyc(30);                            /* TAX */
    rc_x = a;
    cyc(16);
    v = rd16(0x70FCD2 + (uint32_t)a);
    cyc(16);
    wr16(W(record_row_name), v);
    cyc(32);
    v = rd16(0x70FCD0 + (uint32_t)a);
    cyc(16);
    wr16(W(time_total), v);
    return v;
}

void RecordsEntryLoad_cr(Carried *cr)
{
    cr->a = RecordsEntryLoad(cr->db);
    cr->x = rc_x;
    set_p(cr, nz16((uint8_t)((cr->p & ~(P_C | P_V)) | rc_c | rc_v), cr->a));
}

void abi_RecordsEntryLoad(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsEntryLoad_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}


/* LDA $0FA6; ASL; TAX: the handler index, C from the shift. */
static uint16_t records_char_index(uint8_t bank)
{
    uint16_t v;

    cyc(128);
    v = rd16(0x7E0000 | W(records_char));
    rc_c = (uint8_t)(v >> 15);          /* the ASL */
    cyc(30);                            /* TAX */
    rc_x = (uint16_t)(v << 1);
    return rc_x;
}

/* A digit of $F2 (the high nibble, or the low one with `low`) when $F2 is
   below $40, else `blank`. */
static uint16_t stunt_score_digit(int low, uint16_t blank)
{
    uint8_t v;

    cyc(82);
    v = rd8(W(record_row_time));
    if (v >= 0x40) {
        cyc(2);
        rc_c = P_C;
        cyc(22);
        cyc(38);
        rc_m16 = 1;
        return blank;
    }
    cyc(32);
    rc_c = 0;
    if (!low) {
        cyc(14 * 4);
        rc_c = (uint8_t)((v >> 3) & 1);
        v >>= 4;
    }
    cyc(22);
    cyc(2);
    rc_m16 = 1;
    return (uint16_t)(v & 0x0F);
}

/* Letter `which` (0-2) of the name word at $F3: bits 0-4 (LDA $F3), 5-9
   (three ASLs and XBA) or 10-14 (LDA $F4, two LSRs); AND #$1F, + $0A. */
static uint16_t records_letter(int which)
{
    uint16_t a;

    if (which == 0) {
        cyc(60);
        a = rd16(W(record_row_name));
    } else if (which == 1) {
        cyc(60);
        a = rd16(W(record_row_name));
        cyc(2);
        cyc(60);
        a = (uint16_t)(a << 3);
        a = (uint16_t)(a << 8 | a >> 8);
    } else {
        cyc(60);
        a = (uint16_t)(rd16(W(dp_f4)) >> 2);
    }
    cyc(16);
    cyc(2);
    return rc_add((uint16_t)(a & 0x001F), 0x000A);
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
/* The jump of a records character past the table's end: to code still in
   registers, with the state it has there (A = X = the index, C the
   ASL's). */
static void records_char_jump(Carried *cr, uint8_t bank, uint32_t table)
{
    cr->db = bank;
    if (rc_preview)
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    cr->a = cr->x = rc_x;
    if (rc_in_draw)
        cr->y = rc_y;
    set_p(cr, nz16((uint8_t)((cr->p & ~P_C) | rc_c), rc_x));
    if (rc_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | rc_v));
    carried_jump(cr, 0x070000 | rd16(table + rc_x));
}

uint16_t RecordsCharTileStunt(Carried *cr, uint8_t bank)
{
    uint16_t a;

    switch (records_char_index(bank)) {
    case 0:
        return stunt_score_digit(0, 0x006D);
    case 2:
        return stunt_score_digit(1, 0x006E);
    case 4:
        cyc(60);
        a = rd16(W(time_total));
        cyc(56);
        rc_c = (uint8_t)((a >> 3) & 1);
        cyc(2);
        return (uint16_t)((a >> 4) & 0x000F);
    case 6:
        cyc(60);
        a = rd16(W(time_total));
        cyc(2);
        return (uint16_t)(a & 0x000F);
    case 8:
        a = records_letter(0);
        cyc(44);
        return a;
    case 10:
        a = records_letter(1);
        cyc(44);
        return a;
    case 12:
        a = records_letter(2);
        cyc(50);
        return a;
    default:
        records_char_jump(cr, bank, 0x07D79D);
        return 0;
    }
}

/* The registers a tile routine leaves: A the tile, X the index * 2. */
static void char_tile_exit(Carried *cr, uint16_t t)
{
    cr->a = t;
    cr->x = rc_x;
    set_p(cr, nz16((uint8_t)((cr->p & ~P_C) | rc_c), t));
    if (rc_m16)
        set_p(cr, (uint8_t)(cr->p & ((uint8_t)~P_M)));
    if (rc_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | rc_v));
}

void RecordsCharTileStunt_cr(Carried *cr)
{
    rc_v_set = rc_m16 = rc_in_draw = rc_preview = 0;  /* V and M are the entry's unless a step sets them */
    char_tile_exit(cr, RecordsCharTileStunt(cr, cr->db));
}

void abi_RecordsCharTileStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsCharTileStunt_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* A field of the packed time at $F1 (the seconds, ASL and XBA first, with
   `seconds`), masked, through BinaryToBcd: the tens with `tens`, else the
   units. */
/* BinaryToBcd, binary; its C and V to rc_c, rc_v. */
static uint16_t rc_bcd(uint16_t v)
{
    uint8_t p = 0;

    v = BinaryToBcd(v, &p);
    rc_c = (uint8_t)(p & P_C);
    rc_v = (uint8_t)(p & P_V);
    rc_v_set = 1;
    return v;
}

static uint16_t records_time_digit(uint16_t mask, int seconds, int tens)
{
    uint16_t a;

    cyc(60);
    a = rd16(W(time_total));
    if (seconds) {
        cyc(34);
        a = (uint16_t)(a << 1);
        a = (uint16_t)(a << 8 | a >> 8);
    }
    cyc(84);
    cyc(2);
    a = rc_bcd((uint16_t)(a & mask));
    if (tens) {
        cyc(34);
        rc_c = (uint8_t)((a >> 3) & 1);
        return (uint16_t)(a >> 4);
    }
    cyc(2);
    return (uint16_t)(a & 0x000F);
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
uint16_t RecordsCharTileAll(Carried *cr, uint8_t bank)
{
    uint16_t a;
    unsigned i;

    switch (records_char_index(bank)) {
    case 0:
        cyc(60);
        a = rd16(W(time_total));
        for (i = 0; i < 4; i++) {       /* ROL through carry */
            uint8_t c = (uint8_t)(a >> 15);

            cyc(35);
            a = (uint16_t)(a << 1 | rc_c);
            rc_c = c;
        }
        cyc(2);
        a = rc_bcd((uint16_t)(a & 0x0007));
        cyc(2);
        return (uint16_t)(a & 0x000F);
    case 2:
        return records_time_digit(0x003F, 1, 1);
    case 4:
        return records_time_digit(0x003F, 1, 0);
    case 6:
        return records_time_digit(0x007F, 0, 1);
    case 8:
        return records_time_digit(0x007F, 0, 0);
    case 10:
        a = records_letter(0);
        break;
    case 12:
        a = records_letter(1);
        break;
    case 14:
        a = records_letter(2);
        cyc(6);
        break;
    default:
        records_char_jump(cr, bank, 0x07D807);
        return 0;
    }
    cyc(44);
    return a;
}

void RecordsCharTileAll_cr(Carried *cr)
{
    rc_v_set = rc_m16 = rc_in_draw = rc_preview = 0;  /* V and M are the entry's unless a step sets them */
    char_tile_exit(cr, RecordsCharTileAll(cr, cr->db));
}

void abi_RecordsCharTileAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsCharTileAll_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The tile's low byte to $7F:93D7 + Y (SEP #$20, TYX, STA.L, REP #$20). */
static void records_char_store(uint16_t y, uint16_t t)
{
    cyc(132);
    wr8(W(tilemap_7f93d7) + (uint32_t)y, (uint8_t)t);
    cyc(22);
    cyc(42);                            /* RTS */
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
/* The draws' exit, for the adapters: drawn (A the tile, X = Y the
   offset) or empty (as RecordsEntryLoad leaves A, X; C, Z set for All). */
static int rc_drawn;
static uint16_t rc_a;
static uint8_t rc_empty_flags;

uint16_t RecordsCharDrawStunt(Carried *cr, uint8_t bank)
{
    uint16_t t;

    rc_y = RecordsCharOffsetStunt(bank);
    rc_in_draw = 1;
    rc_a = RecordsEntryLoad(bank);
    rc_drawn = 0;
    if (!rc_a) {
        rc_empty_flags = (uint8_t)(P_Z | rc_c);
        cyc(106);                       /* EntryLoad's RTS, BEQ taken, RTS */
        return rc_a;
    }
    t = RecordsCharTileStunt(cr, bank);
    records_char_store(rc_y, t);
    rc_drawn = 1;
    return rc_a = t;
}

/* The registers a records character draw leaves. */
static void char_draw_exit(Carried *cr)
{
    cr->a = rc_a;
    cr->y = rc_y;
    if (rc_drawn) {
        cr->x = rc_y;
        set_p(cr, nz16((uint8_t)((cr->p & ~(P_C | P_M)) | rc_c), rc_y));
    } else {
        cr->x = rc_x;
        set_p(cr, (uint8_t)((cr->p & ~(P_N | P_Z | P_C)) | rc_empty_flags));
    }
    if (rc_v_set)
        set_p(cr, (uint8_t)((cr->p & ~P_V) | rc_v));
}

void RecordsCharDrawStunt_cr(Carried *cr)
{
    rc_preview = 0;
    RecordsCharDrawStunt(cr, cr->db);
    char_draw_exit(cr);
}

void abi_RecordsCharDrawStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsCharDrawStunt_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
uint16_t RecordsCharDrawAll(Carried *cr, uint8_t bank)
{
    uint16_t t;

    rc_y = RecordsCharOffsetAll(bank);
    rc_in_draw = 1;
    rc_a = RecordsEntryLoad(bank);
    rc_drawn = 0;
    if (rc_a == 0xFFFF) {
        rc_empty_flags = P_Z | P_C;
        cyc(130);
        return rc_a;
    } else { cyc(24); }
    t = RecordsCharTileAll(cr, bank);
    records_char_store(rc_y, t);
    rc_drawn = 1;
    return rc_a = t;
}

void RecordsCharDrawAll_cr(Carried *cr)
{
    rc_preview = 0;
    RecordsCharDrawAll(cr, cr->db);
    char_draw_exit(cr);
}

void abi_RecordsCharDrawAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RecordsCharDrawAll_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* PHP, SEP #$20, PHB, PHK, PLB, REP #$20, the JSR, SEP #$20, PLB, PLP. */


/*
 * @routine 07:C663 PreviewRowDrawStunt
 * WHAT IT DOES: RecordsCharDrawStunt for a JSL caller (the course screens'
 * records panel): data bank $07, P and the data bank kept.
 * HOW: PHP, SEP #$20, PHB, PHK, PLB, REP #$20, JSR; then SEP #$20, PLB,
 * PLP, RTL.
 * Entry: X 16-bit; D = $0000. Exit: P and the data bank unchanged; A, X,
 * Y as RecordsCharDrawStunt leaves them.
 */
uint16_t PreviewRowDrawStunt(Carried *cr)
{
    uint16_t a;

    cyc(22);                            /* REP #$20 */
    rc_preview = 1;
    a = RecordsCharDrawStunt(cr, 0x07);
    rc_preview = 0;
    cyc(122);                           /* SEP, PLB, PLP, RTL */
    return a;
}

/* The draws' registers with the caller's P (PLP). */
static void preview_exit(Carried *cr, uint8_t p0)
{
    char_draw_exit(cr);
    set_p(cr, p0);
    if (p0 & P_X) {
        cr->x &= 0x00FF;
        cr->y &= 0x00FF;
    }
}

void PreviewRowDrawStunt_cr(Carried *cr)
{
    uint8_t p0 = cr->p;

    PreviewRowDrawStunt(cr);
    preview_exit(cr, p0);
}

void abi_PreviewRowDrawStunt(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PreviewRowDrawStunt_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:C673 PreviewRowDrawAll
 * WHAT IT DOES: As PreviewRowDrawStunt with RecordsCharDrawAll.
 * HOW: As PreviewRowDrawStunt.
 * Entry: X 16-bit; D = $0000. Exit: as PreviewRowDrawStunt.
 */
uint16_t PreviewRowDrawAll(Carried *cr)
{
    uint16_t a;

    cyc(22);
    rc_preview = 1;
    a = RecordsCharDrawAll(cr, 0x07);
    rc_preview = 0;
    cyc(122);
    return a;
}

void PreviewRowDrawAll_cr(Carried *cr)
{
    uint8_t p0 = cr->p;

    PreviewRowDrawAll(cr);
    preview_exit(cr, p0);
}

void abi_PreviewRowDrawAll(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    PreviewRowDrawAll_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

