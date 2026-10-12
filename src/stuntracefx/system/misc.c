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
