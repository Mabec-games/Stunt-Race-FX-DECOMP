/*
 * misc.c: small routines of the frame code that have no other home yet.
 */
#include "machine.h"

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
