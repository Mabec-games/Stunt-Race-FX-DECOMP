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
