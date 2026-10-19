/*
 * table.c: the Super FX's side of the 8-byte entry table whose address is
 * the word at $70:1092 (the S-CPU's side is in gsu/params.c).
 *
 * Entries are counted from 1; the words of an entry start 2 bytes into it.
 */
#include "gsu.h"

#define GSU_TABLE_ADDR 0x1092u  /* the word holding the table's address */

/*
 * @routine 04:CD83 FxTableEntryWords
 * WHAT IT DOES: Returns the address of the words of entry R0 of the table
 * whose address is the word at $1092: the Super FX side of
 * GsuTableEntryTo1F4C.
 * HOW: (entry - 1) * 8 with LMULT (R6 = 8; R4 takes the low word of the
 * product), then the table address + 2 + that.
 * Entry: R0 = entry; R11 = return address. Exit: R5 = the address; R0 and
 * R1 = the entry; R4 = (entry - 1) * 8; R6 = 8; the last RAM address $1092;
 * OV, S, CY, Z from the final add (table + 2, plus R4).
 */
uint16_t FxTableEntryWords(uint16_t entry, int copy)
{
    uint16_t offset = (uint16_t)((entry - 1) * 8);

    gsu_code(0x04cd8c, 4);
    return (uint16_t)(gsu_ldw(GSU_TABLE_ADDR) + 2 + offset);
}

void abi_FxTableEntryWords(void)
{
    uint16_t entry = gsu.r[0];
    uint16_t offset = (uint16_t)gsu_lmult((uint16_t)(entry - 1), 8);
    gsu_code(0x04cd84, 8);
    uint16_t addr = FxTableEntryWords(entry, GSU_COPY(0));

    gsu_add((uint16_t)(addr - offset), offset, 0);
    gsu.r[0] = entry;
    gsu.r[1] = entry;
    gsu.r[4] = offset;
    gsu.r[5] = addr;
    gsu.r[6] = 8;
    gsu_code(0x04cd90, 8);
}
