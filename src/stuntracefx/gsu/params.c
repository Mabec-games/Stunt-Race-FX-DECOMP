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


/* Store the four bytes 0, b1, b2, b3 at $70:0200-$70:0203. */
static uint8_t gsu_200(uint8_t b1, uint8_t b2, uint8_t b3)
{
    wr8(0x700200, 0x00);
    wr8(0x700201, b1);
    wr8(0x700202, b2);
    wr8(0x700203, b3);
    return b3;
}

/*
 * @routine 04:D7BF GsuBytes200_BFB7B6
 * WHAT IT DOES: Sets the four renderer input bytes at $70:0200-$70:0203 to
 * 0, $BF, $B7, $B6.
 * HOW: Four byte stores (gsu_200).
 * Entry: any widths. Exit: A low byte = $B6; P unchanged.
 */
void GsuBytes200_BFB7B6(void)
{
    gsu_200(0xBF, 0xB7, 0xB6);
}

void abi_GsuBytes200_BFB7B6(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0xBF, 0xB7, 0xB6));
}

/*
 * @routine 04:D938 GsuBytes200_0F060E
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $06, $0E.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $0E; P unchanged.
 */
void GsuBytes200_0F060E(void)
{
    gsu_200(0x0F, 0x06, 0x0E);
}

void abi_GsuBytes200_0F060E(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0x0F, 0x06, 0x0E));
}

/*
 * @routine 04:D7DC GsuBytes200_BFB5BB
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $B5, $BB.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $BB; P unchanged.
 */
void GsuBytes200_BFB5BB(void)
{
    gsu_200(0xBF, 0xB5, 0xBB);
}

void abi_GsuBytes200_BFB5BB(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0xBF, 0xB5, 0xBB));
}

/*
 * @routine 04:D7F9 GsuBytes200_BFBCBD
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $BC, $BD.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $BD; P unchanged.
 */
void GsuBytes200_BFBCBD(void)
{
    gsu_200(0xBF, 0xBC, 0xBD);
}

void abi_GsuBytes200_BFBCBD(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0xBF, 0xBC, 0xBD));
}

/*
 * @routine 04:D816 GsuBytes200_BFBAB9
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $BA, $B9.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $B9; P unchanged.
 */
void GsuBytes200_BFBAB9(void)
{
    gsu_200(0xBF, 0xBA, 0xB9);
}

void abi_GsuBytes200_BFBAB9(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0xBF, 0xBA, 0xB9));
}

/*
 * @routine 04:D833 GsuBytes200_BFB3BE
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $BF, $B3, $BE.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $BE; P unchanged.
 */
void GsuBytes200_BFB3BE(void)
{
    gsu_200(0xBF, 0xB3, 0xBE);
}

void abi_GsuBytes200_BFB3BE(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0xBF, 0xB3, 0xBE));
}

/*
 * @routine 04:D972 GsuBytes200_0F0405
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $04, $05.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $05; P unchanged.
 */
void GsuBytes200_0F0405(void)
{
    gsu_200(0x0F, 0x04, 0x05);
}

void abi_GsuBytes200_0F0405(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0x0F, 0x04, 0x05));
}

/*
 * @routine 04:D9AC GsuBytes200_0F090A
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $09, $0A.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $0A; P unchanged.
 */
void GsuBytes200_0F090A(void)
{
    gsu_200(0x0F, 0x09, 0x0A);
}

void abi_GsuBytes200_0F090A(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0x0F, 0x09, 0x0A));
}

/*
 * @routine 04:D9E6 GsuBytes200_0F050E
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $05, $0E.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $0E; P unchanged.
 */
void GsuBytes200_0F050E(void)
{
    gsu_200(0x0F, 0x05, 0x0E);
}

void abi_GsuBytes200_0F050E(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0x0F, 0x05, 0x0E));
}

/*
 * @routine 04:DA03 GsuBytes200_0F080E
 * WHAT IT DOES: As GsuBytes200_BFB7B6, with 0, $0F, $08, $0E.
 * HOW: As GsuBytes200_BFB7B6.
 * Entry: any widths. Exit: A low byte = $0E; P unchanged.
 */
void GsuBytes200_0F080E(void)
{
    gsu_200(0x0F, 0x08, 0x0E);
}

void abi_GsuBytes200_0F080E(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | gsu_200(0x0F, 0x08, 0x0E));
}

/*
 * @routine 08:DC48 GsuTableEntryNext
 * WHAT IT DOES: Returns entry A + 1 of the table whose bank is the byte at
 * $70:1094 and address the word at $70:1092, or 1 when A equals the
 * table's first word (its last entry).
 * HOW: A is stored at $1F4C through the caller's data bank and read back
 * through the table's (as GsuTableEntryTo1F4C does).
 * Entry: M 16-bit, X 16-bit; A = entry. Exit: A = the next entry; X
 * unchanged; N, Z from X; C from the compare.
 */
uint16_t GsuTableEntryNext(uint8_t bank, uint16_t entry, uint8_t *p)
{
    uint32_t t;
    uint16_t a, last;

    wr16((uint32_t)bank << 16 | 0x1F4C, entry);
    t = (uint32_t)(rd16(0x701094) & 0x00FF) << 16;
    a = rd16(t | 0x1F4C);
    last = rd16(t | rd16(0x701092));
    *p = (uint8_t)((*p & ~P_C) | (a >= last ? P_C : 0));
    if (a == last)
        a = 0x0000;
    return (uint16_t)(a + 1);
}

void abi_GsuTableEntryNext(void)
{
    uint8_t p = cpu.p;

    cpu.a = GsuTableEntryNext(cpu.db, cpu.a, &p);
    cpu.p = nz16(p, cpu.x);
}

/* The body; X, Y and the flags as the exit leaves them. */
static uint16_t gsu_records_upload(uint16_t *x, uint16_t *y, uint8_t *p)
{
    uint16_t n = rd16(0x000CEE), s, d;
    unsigned i;

    wr16(0x7001C8, n);
    if (!n)
        return 0;
    s = 0xF77F;
    d = 0x0AAA;
    wr16(0x001F2B, n);
    do {
        for (i = 0; i < 30; i += 2)
            wr16(0x700000 | (uint16_t)(d + i), rd16(0x7E0000 + s + i));
        d = (uint16_t)(d + 0x1E);
        *p = (uint8_t)(*p & ~(P_C | P_V));
        if ((unsigned)s + 0x1E > 0xFFFF)
            *p |= P_C;
        if (~s & (uint16_t)(s + 0x1E) & 0x8000)
            *p |= P_V;
        s = (uint16_t)(s + 0x1E);
        n = (uint16_t)(rd16(0x001F2B) - 1);
        wr16(0x001F2B, n);
    } while (n);
    *x = s;
    *y = d;
    return 0;
}

/*
 * @routine 02:DA34 GsuRecordsUpload
 * WHAT IT DOES: Copies the $0CEE 30-byte records at $7E:F77F to Super FX
 * RAM at $70:0AAA and stores the count at $70:01C8.
 * HOW: Fifteen word copies per record with the data bank at $70; the count
 * runs down in $1F2B.
 * Entry: any widths. Exit: X, Y past the copies (unchanged when the count
 * is 0); A = 0; M 16-bit, X 16-bit; N, Z from the data bank pulled back;
 * C, V from the last source step (unchanged when the count is 0).
 */
uint16_t GsuRecordsUpload(void)
{
    uint16_t x, y;
    uint8_t p = 0;

    return gsu_records_upload(&x, &y, &p);
}

void abi_GsuRecordsUpload(void)
{
    uint8_t p = cpu.p;

    cpu.a = gsu_records_upload(&cpu.x, &cpu.y, &p);
    cpu.p = (uint8_t)(nz8(p, cpu.db) & ~(P_M | P_X));
}
