/*
 * table.c: the Super FX's side of the 8-byte entry table whose address is
 * the word at $70:1092 (the S-CPU's side is in gsu/params.c).
 *
 * Entries are counted from 1; the words of an entry start 2 bytes into it.
 */
#include "gsu.h"

#define GSU_TABLE_ADDR 0x1092u  /* the word holding the table's address */

/* cyccopy: the code of each of the original's copies, by where it runs */
static const GsuCode FxTableEntryWords_code[4][6] = {
    {{0x04CD83, 13}, {0x04CED9, 6}, {0, 0}, {0x04CCA4, 7}, {0x04CD90, 8}, {0x04CEDF, 5}},
    {{0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}},
    {{0x04C5ED, 4}, {0x04C5F5, 5}, {0x04CD83, 13}, {0x04C5FA, 5}, {0x04CCA4, 7}, {0x04CD90, 8}},
    {{0x04C663, 4}, {0x04C66B, 5}, {0x04CD83, 13}, {0x04C670, 5}, {0x04CD56, 2}, {0x04CD90, 8}},
};

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
    gsu_codes(FxTableEntryWords_code[copy], 3);
    uint16_t offset = (uint16_t)((entry - 1) * 8);

    return gsu_codes_then(FxTableEntryWords_code[copy] + 3, 3, (uint16_t)(gsu_ldw(GSU_TABLE_ADDR) + 2 + offset));
}

void abi_FxTableEntryWords(void)
{
    uint16_t entry = gsu.r[0];
    uint16_t offset = (uint16_t)gsu_lmult((uint16_t)(entry - 1), 8);
    uint16_t addr = FxTableEntryWords(entry, GSU_COPY(0));

    gsu_add((uint16_t)(addr - offset), offset, 0);
    gsu.r[0] = entry;
    gsu.r[1] = entry;
    gsu.r[4] = offset;
    gsu.r[5] = addr;
    gsu.r[6] = 8;
}
