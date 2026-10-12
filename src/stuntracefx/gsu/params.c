/*
 * params.c: the parameter block in Super FX RAM that the renderer reads.
 */
#include "machine.h"

/*
 * @routine 7E:E2C9 GsuParamsUpdate
 * WHAT IT DOES: Copies the view parameters the renderer needs from work RAM
 * into its parameter block in Super FX RAM.
 * HOW: Eight 16-bit copies: $0D12 to $70:108A; direct page $C5, $C7, $C9 to
 * $70:00D8, 00DA, 00DC; $0664, $0666, $0668 to $70:00DE, 00E0, 00E2; $05FA to
 * $70:2736.
 * Entry: any widths, D = $0000. Exit: A = the last value copied; P unchanged.
 */
uint16_t GsuParamsUpdate(void)
{
    static const uint32_t from[8] = {0x000D12, 0x0000C5, 0x0000C7, 0x0000C9, 0x000664, 0x000666, 0x000668, 0x0005FA};
    static const uint32_t to[8] = {0x70108A, 0x7000D8, 0x7000DA, 0x7000DC, 0x7000DE, 0x7000E0, 0x7000E2, 0x702736};
    uint16_t v = 0;
    unsigned i;

    for (i = 0; i < 8; i++) {
        v = rd16(from[i]);
        wr16(to[i], v);
    }
    return v;
}

void abi_GsuParamsUpdate(void)
{
    cpu.a = GsuParamsUpdate();
}

/*
 * @routine 03:AEB6 GsuParamsFromDp
 * WHAT IT DOES: Marks $05EF with $FF and copies eight direct-page words into
 * the renderer's inputs at $70:0034-$70:003E, $70:0092 and $70:0094.
 * HOW: Word copies from $CE, $D0, $2A, $2E, $2C, $30, $32, $34, in that
 * order; $05EF is reached through the data bank.
 * Entry: M 8-bit, D = $0000. Exit: A = the last word; M 16-bit; N, Z from A.
 */
uint16_t GsuParamsFromDp(uint8_t bank)
{
    static const uint8_t from[8] = {0xCE, 0xD0, 0x2A, 0x2E, 0x2C, 0x30, 0x32, 0x34};
    static const uint32_t to[8] = {0x700034, 0x700036, 0x700038, 0x70003A, 0x70003C, 0x70003E, 0x700092, 0x700094};
    uint16_t v = 0;
    unsigned i;

    wr8((uint32_t)bank << 16 | 0x05EF, 0xFF);
    for (i = 0; i < 8; i++) {
        v = rd16(from[i]);
        wr16(to[i], v);
    }
    return v;
}

void abi_GsuParamsFromDp(void)
{
    cpu.a = GsuParamsFromDp(cpu.db);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) & ~P_M);
}

/* $70:0216 = the word at `table` + index in the data bank. */
static uint16_t param_216(uint8_t bank, uint16_t table, uint16_t index)
{
    uint16_t v = rd16((uint32_t)bank << 16 | (uint16_t)(table + index));

    wr16(0x700216, v);
    return v;
}

/* Registers: the index is A * 2, kept to 8 bits when X is 8-bit. */
static void abi_param_216(uint16_t table)
{
    uint16_t a = cpu.a, x = (uint16_t)(a << 1);

    if (cpu.p & P_X)
        x &= 0x00FF;
    cpu.x = x;
    cpu.a = param_216(cpu.db, table, x);
    cpu.p = (uint8_t)((nz16(cpu.p, cpu.a) & ~P_C) | (a & 0x8000 ? P_C : 0));
}

/*
 * @routine 07:8E2A GsuParam216A
 * WHAT IT DOES: Sets the renderer input at $70:0216 to entry A of the word
 * table at $8E3E in the data bank.
 * HOW: A * 2 as the index (8 bits when X is 8-bit).
 * Entry: M 16-bit; A = entry. Exit: A = the value; X = the index; N, Z from
 * A; C from the doubling.
 */
uint16_t GsuParam216A(uint8_t bank, uint16_t index)
{
    return param_216(bank, 0x8E3E, index);
}

void abi_GsuParam216A(void)
{
    abi_param_216(0x8E3E);
}

/*
 * @routine 07:8E34 GsuParam216B
 * WHAT IT DOES: As GsuParam216A, from the table at $8E52.
 * HOW: As GsuParam216A.
 * Entry: as GsuParam216A. Exit: as GsuParam216A.
 */
uint16_t GsuParam216B(uint8_t bank, uint16_t index)
{
    return param_216(bank, 0x8E52, index);
}

void abi_GsuParam216B(void)
{
    abi_param_216(0x8E52);
}

/*
 * @routine 08:DC13 GsuTableEntryTo1F4C
 * WHAT IT DOES: Copies three words of entry A (counted from 1) of a table of
 * 8-byte entries into $1F4C-$1F51; the table's bank is the byte at $70:1094
 * and its address the word at $70:1092, and the words start 2 bytes into
 * the entry.
 * HOW: The table's bank becomes the data bank for the copy, so $1F4C and
 * the table are both reached through it.
 * Entry: M 16-bit, X 16-bit; A = entry. Exit: A = the third word; X
 * unchanged; N, Z from X; C, V from the address add.
 */
uint16_t GsuTableEntryTo1F4C(uint8_t bank, uint16_t entry, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16, t;
    uint16_t a, base, s, v = 0;
    unsigned i;

    wr16(b | 0x1F4C, entry);
    t = (uint32_t)(rd16(0x701094) & 0x00FF) << 16;
    a = (uint16_t)((uint16_t)(rd16(t | 0x1F4C) - 1) << 3);
    base = rd16(0x701092);
    s = (uint16_t)(a + base);
    *p = (uint8_t)((*p & ~(P_C | P_V)) | ((unsigned)a + base > 0xFFFF ? P_C : 0)
                   | ((~(a ^ base) & (a ^ s) & 0x8000) ? P_V : 0));
    s = (uint16_t)(s + 2);
    for (i = 0; i < 3; i++) {
        v = rd16(t | (uint16_t)(s + 2 * i));
        wr16(t | (uint16_t)(0x1F4C + 2 * i), v);
    }
    return v;
}

void abi_GsuTableEntryTo1F4C(void)
{
    uint8_t p = cpu.p;

    cpu.a = GsuTableEntryTo1F4C(cpu.db, cpu.a, &p);
    cpu.p = nz16(p, cpu.x);
}
