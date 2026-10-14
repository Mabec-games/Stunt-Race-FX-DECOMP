/*
 * misc.c: small routines of the frame code that have no other home yet.
 */
#include "machine.h"
#include "cpu_ops.h"

/*
 * @routine 7E:E18B DataBankTest
 * WHAT IT DOES: Leaves every register as it was except N and Z, which it
 * sets from the data bank.
 * HOW: Pushes DB and PB, pulls PB into DB and then the old DB back.
 * Entry: any widths. Exit: DB unchanged; N, Z from DB.
 */
void DataBankTest(void)
{
}

void abi_DataBankTest(void)
{
    cpu.p = nz8(cpu.p, cpu.db);
}

/*
 * @routine 7E:F217 KeepRegisters
 * WHAT IT DOES: Nothing: it saves P, changes the widths and restores P.
 * HOW: PHP, SEP #$20, REP #$30, PLP.
 * Entry: any widths. Exit: every register unchanged.
 */
void KeepRegisters(void)
{
}

void abi_KeepRegisters(void)
{
}

/*
 * @routine 03:F329 ReturnOnly
 * WHAT IT DOES: Nothing: a lone RTS that callers use as an empty handler.
 * HOW: RTS.
 * Entry: any widths. Exit: every register unchanged.
 */
void ReturnOnly(void)
{
}

void abi_ReturnOnly(void)
{
}

/*
 * @routine 03:AEEE ReadTable9022
 * WHAT IT DOES: Reads 128 words' low bytes from the table at $9022 in the
 * data bank and keeps none of them; it changes only registers and $64.
 * HOW: A loop of 128 loads stepping Y by 2, X by 1, counting down $64.
 * Entry: any widths, D = $0000. Exit: A low byte = the last byte read; X =
 * $80, Y = 0; $64 = 0; P unchanged.
 */
uint8_t ReadTable9022(uint8_t bank)
{
    uint8_t v = 0;
    unsigned i;

    for (i = 0; i < 0x80; i++)
        v = rd8((uint32_t)bank << 16 | (uint16_t)(0x9022 + 2 * i));
    wr8(0x000064, 0);
    return v;
}

void abi_ReadTable9022(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | ReadTable9022(cpu.db));
    cpu.x = 0x80;
    cpu.y = 0x00;
}

/*
 * @routine 02:FC44 Set0D37FromTable
 * WHAT IT DOES: Sets $0D37 to the word table $02:FC58 entry for the byte at
 * $0D48.
 * HOW: 16-bit lookup; work RAM through the data bank.
 * Entry: any widths. Exit: A = the entry; X = the byte * 2 (low byte only
 * when X is 8-bit); P unchanged.
 */
uint16_t Set0D37FromTable(uint8_t bank, uint16_t *x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;

    *x = (uint16_t)((rd16(b | 0x0D48) & 0x00FF) << 1);
    v = rd16(0x02FC58 + *x);
    wr16(b | 0x0D37, v);
    return v;
}

void abi_Set0D37FromTable(void)
{
    uint16_t x;

    cpu.a = Set0D37FromTable(cpu.db, &x);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}

/*
 * @routine 02:D65A Set7EBC2BTrio
 * WHAT IT DOES: Stores A's low byte at $7E:BC2B, $7E:BC2D and $7E:BC2F.
 * HOW: Three long byte stores.
 * Entry: any widths. Exit: every register unchanged.
 */
void Set7EBC2BTrio(uint8_t v)
{
    wr8(0x7EBC2B, v);
    wr8(0x7EBC2D, v);
    wr8(0x7EBC2F, v);
}

void abi_Set7EBC2BTrio(void)
{
    Set7EBC2BTrio((uint8_t)cpu.a);
}

/*
 * @routine 07:EE11 Countdown0DF8
 * WHAT IT DOES: Counts $0DF8 down by one; once it is zero, instead counts
 * $0E5C down and clears $1160.
 * HOW: 8-bit memory decrements through the data bank.
 * Entry: any widths. Exit: A low byte = $0DF8's value before; P unchanged.
 */
uint8_t Countdown0DF8(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v = rd8(b | 0x0DF8);

    if (v) {
        wr8(b | 0x0DF8, (uint8_t)(rd8(b | 0x0DF8) - 1));
        return v;
    }
    wr8(b | 0x0E5C, (uint8_t)(rd8(b | 0x0E5C) - 1));
    wr8(b | 0x1160, 0x00);
    return v;
}

void abi_Countdown0DF8(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | Countdown0DF8(cpu.db));
}

/*
 * @routine 08:B893 Set7EFFA6Sequence
 * WHAT IT DOES: Stores the words 1, 2, 3, 4 at $7E:FFA6-$7E:FFAD.
 * HOW: Four 16-bit long stores.
 * Entry: any widths. Exit: A = 4 (16-bit); P unchanged.
 */
void Set7EFFA6Sequence(void)
{
    unsigned i;

    for (i = 0; i < 4; i++)
        wr16(0x7EFFA6 + 2 * i, (uint16_t)(i + 1));
}

void abi_Set7EFFA6Sequence(void)
{
    Set7EFFA6Sequence();
    cpu.a = 0x0004;
}

/*
 * @routine 02:D53D WordCopyStep
 * WHAT IT DOES: While the count at $0CF6 is non-zero, copies one word from
 * $02:D65A + $0CF2 to $07A2 + $0CF6 and steps both offsets down by 2. No
 * script has reached the copy yet: $0CF6 has always been 0.
 * HOW: 16-bit offsets and stores through the data bank; the source is a
 * long read, and each offset is stepped with two DECs.
 * Entry: any widths. Exit: when $0CF6 is 0, Y = 0 and A unchanged; else A
 * = the word copied, X and Y the offsets before the step; M 16-bit, X
 * 16-bit; N, Z from the last step (Z set when there was nothing to copy).
 */
uint16_t WordCopyStep(uint8_t bank, uint16_t a, uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t n;

    *y = rd16(b | 0x0CF6);
    if (!*y) {
        *p = nz16(*p, 0);
        return a;
    }
    *x = rd16(b | 0x0CF2);
    a = rd16(0x02D65A + *x);
    wr16(b | (uint16_t)(0x07A2 + *y), a);
    wr16(b | 0x0CF2, (uint16_t)(*x - 2));
    n = (uint16_t)(*y - 2);
    wr16(b | 0x0CF6, n);
    *p = nz16(*p, n);
    return a;
}

void abi_WordCopyStep(void)
{
    uint8_t p = cpu.p;

    cpu.a = WordCopyStep(cpu.db, cpu.a, &cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)(p & ~(P_M | P_X));
}

/*
 * @routine 03:8CF6 WramFillZero
 * WHAT IT DOES: Clears Y bytes of work RAM from bank A, address X.
 * HOW: DMA channel 0, fixed source (the zero byte at $03:8D26), to WMDATA
 * ($2180) with the work RAM address set at $2181-$2183; starts it.
 * Entry: M 8-bit, X 16-bit; A = bank, X = address, Y = byte count (0 means
 * 65536). Exit: A low byte = 1; X = $8008; N, Z clear.
 */
void WramFillZero(uint8_t bank, uint16_t addr, uint16_t count)
{
    wr16(0x002181, addr);
    wr8(0x002183, bank);
    wr16(0x004305, count);
    wr16(0x004302, 0x8D26);
    wr8(0x004304, 0x03);
    wr16(0x004300, 0x8008);
    wr8(0x00420B, 0x01);
}

void abi_WramFillZero(void)
{
    WramFillZero((uint8_t)cpu.a, cpu.x, cpu.y);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | 0x01);
    cpu.x = 0x8008;
    cpu.p = (uint8_t)(cpu.p & ~(P_N | P_Z));
}

/*
 * @routine 03:863D DemoRunFirst
 * WHAT IT DOES: Runs the first of the attract demos: $1A57 = 0 (its offset
 * in the demo tables at $03:8004), then the shared demo run at $03:8683
 * (course, settings and the race itself).
 * HOW: A tail JMP into the demo runs of the routine at $03:8648, reached
 * as original code (jump_routine).
 * Entry: D = $0000. Exit: as the demo run leaves it.
 */
void abi_DemoRunFirst(void)
{
    op_rep(P_M | P_X);
    cpu.a = 0x0000;
    cpu.p = nz16(cpu.p, 0);
    wr16((uint32_t)cpu.db << 16 | 0x1A57, 0x0000);
    jump_routine(0x038683);
}

/*
 * @routine 03:8CF2 WramFillZeroLong
 * WHAT IT DOES: WramFillZero for callers in other banks: clears Y bytes of
 * work RAM from bank A, address X.
 * HOW: A near call, then a long return.
 * Entry: as WramFillZero. Exit: as WramFillZero.
 */
void abi_WramFillZeroLong(void)
{
    abi_WramFillZero();
}
