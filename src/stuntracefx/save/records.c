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
            sum = add8(sum, rd8(b | (uint16_t)(0xFCD0 + x)), p);
            if (rd8(b | (uint16_t)(0xFCD1 + x)) >= 0x41) {
                bad = 1;
                break;
            }
            sum = add8(sum, rd8(b | (uint16_t)(0xFCD1 + x)), p);
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
