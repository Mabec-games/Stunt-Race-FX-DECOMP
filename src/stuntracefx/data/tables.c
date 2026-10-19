/*
 * tables.c: lookups into small tables in ROM: handler addresses and the
 * per-course table index ($0E3D holds the course number).
 */
#include "machine.h"

static uint16_t word_lookup(uint32_t table, uint16_t i)
{
    return rd16(table + (uint16_t)(i << 1));
}

/*
 * @routine 03:B6E6 HandlerTableA
 * WHAT IT DOES: Returns entry A of the four-entry handler address table at
 * $03:B6F1.
 * HOW: Word lookup at A * 2.
 * Entry: any widths. Exit: A = the entry; X = A * 2; P unchanged.
 */
uint16_t HandlerTableA(uint16_t i)
{
    return word_lookup(0x03B6F1, i);
}

void abi_HandlerTableA(void)
{
    uint16_t x = (uint16_t)(cpu.a << 1);

    cyc(268);
    cpu.a = HandlerTableA(cpu.a);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}

/*
 * @routine 03:B6F9 HandlerTableB
 * WHAT IT DOES: Returns entry A of the handler address table at $03:B704.
 * HOW: Word lookup at A * 2.
 * Entry: as HandlerTableA. Exit: as HandlerTableA.
 */
uint16_t HandlerTableB(uint16_t i)
{
    cyc(268);
    return word_lookup(0x03B704, i);
}

void abi_HandlerTableB(void)
{
    uint16_t x = (uint16_t)(cpu.a << 1);

    cpu.a = HandlerTableB(cpu.a);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}

/*
 * @routine 03:EFBB CourseIndex12
 * WHAT IT DOES: Returns the index course * 12 + A * 3 for tables with twelve
 * bytes per course and three per entry.
 * HOW: 8-bit arithmetic; A * 3 is kept in direct page $F1.
 * Entry: M 8-bit, X 8-bit, D = $0000; A = entry. Exit: X = A = the index;
 * $F1 = A * 3; flags from the last add.
 */
uint8_t CourseIndex12(uint8_t a, uint8_t *p)
{
    uint8_t a3, c, c2, c12, f;
    unsigned s;

    wr8(0x0000F1, a);
    f = rd8(0x0000F1);
    a3 = (uint8_t)((uint8_t)(a << 1) + f);
    wr8(0x0000F1, a3);
    c = rd8(0x000E3D);
    c2 = rd8(0x000E3D);
    c12 = (uint8_t)((uint8_t)((uint8_t)(c << 1) + c2) << 2);
    a3 = rd8(0x0000F1);
    s = (unsigned)c12 + a3;
    *p = (uint8_t)((nz8(*p, (uint8_t)s) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(c12 ^ a3) & (c12 ^ (uint8_t)s) & 0x80) ? P_V : 0));
    return (uint8_t)s;
}

void abi_CourseIndex12(void)
{
    cyc(62);
    uint8_t p = cpu.p, i = CourseIndex12((uint8_t)cpu.a, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | i);
    cpu.x = i;
    cpu.p = p;
}

/*
 * @routine 03:EC24 CourseDataPointer
 * WHAT IT DOES: Returns the current course's long pointer from the table at
 * $03:F4A5: X = address - $8000, A low byte = bank.
 * HOW: Three bytes per course; the index is course * 3 (8-bit).
 * Entry: M 8-bit, X 16-bit. Exit: X = address - $8000; A = the same high
 * byte, the bank low; M 8-bit, X 16-bit; N, Z from X; C, V from the subtract.
 */
uint16_t CourseDataPointer(uint8_t *bank, uint8_t *p)
{
    cyc(46);
    uint16_t c = rd16(0x000E3D), i = (uint16_t)(((uint16_t)(c << 1) + c) & 0x00FF);
    cyc(326);
    uint16_t addr = rd16(0x03F4A5 + i), r = (uint16_t)(addr - 0x8000);

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (addr >= 0x8000 ? P_C : 0)
                   | (((addr ^ 0x8000) & (addr ^ r) & 0x8000) ? P_V : 0));
    *bank = rd8(0x03F4A7 + i);
    return r;
}

void abi_CourseDataPointer(void)
{
    uint8_t bank, p = cpu.p;
    uint16_t x = CourseDataPointer(&bank, &p);

    cpu.x = x;
    cpu.y = x;
    cpu.a = (uint16_t)((x & 0xFF00) | bank);
    cpu.p = (uint8_t)((nz16(p, x) & ~P_X) | P_M);
}

/*
 * @routine 03:B70E SelectionTables18CB
 * WHAT IT DOES: Derives the 2-bit values at $18D3-$18E9 from the four
 * selections at $18CB, $18CD, $18CF, $18D1 through tables in banks $0B and
 * $03.
 * HOW: base = word table $0B:B00C[$18CB], kept at $11DF; index = base +
 * $18CD * 3 + $18CF * 9 + $18D1 * 27 (partial sums at $11E1 and $11E3);
 * $18E5, $18E7, $18E9 = bytes 0-2 of $0B:B014[index] & 3. $18E3 =
 * $03:B7B1[$18CB] & 3; then for i = 0-3, $18D3 + 2i = $03:B7B5[selection i
 * * 4 + the value at $18E3 + 2i] & 3. All 16-bit; $11DF-$11E3 through the
 * data bank.
 * Entry: any widths. Exit: A = the last value stored; X = 8; P unchanged.
 */
uint16_t SelectionTables18CB(uint8_t bank)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t base, s1, i3, x, v = 0, w, u;

    base = rd16(0x0BB00C + (uint16_t)(rd16(0x0018CB) << 1));
    wr16(b | 0x11DF, base);
    v = rd16(0x0018CD);
    w = rd16(0x0018CD);
    s1 = (uint16_t)((uint16_t)(v << 1) + w);
    wr16(b | 0x11E1, s1);
    v = rd16(0x0018CF);
    w = rd16(0x0018CF);
    u = rd16(b | 0x11E1);
    s1 = (uint16_t)((uint16_t)(v << 3) + w + u);
    wr16(b | 0x11E1, s1);
    v = rd16(0x0018D1);
    w = rd16(0x0018D1);
    i3 = (uint16_t)((uint16_t)(v << 1) + w);
    wr16(b | 0x11E3, i3);
    u = rd16(b | 0x11E3);
    x = (uint16_t)((uint16_t)(i3 << 3) + u);
    u = rd16(b | 0x11E1);
    x = (uint16_t)(x + u);
    u = rd16(b | 0x11DF);
    x = (uint16_t)(x + u);
    wr16(0x0018E5, rd16(0x0BB014 + x) & 3);
    wr16(0x0018E7, rd16(0x0BB015 + x) & 3);
    wr16(0x0018E9, rd16(0x0BB016 + x) & 3);
    wr16(0x0018E3, rd16(0x03B7B1 + rd16(0x0018CB)) & 3);
    for (x = 0; x < 8; x += 2) {
        uint16_t idx = (uint16_t)(rd16(0x0018CB + x) << 2);

        u = rd16(0x0018E3 + x);
        idx = (uint16_t)(idx + u);
        v = rd16(0x03B7B5 + idx) & 3;
        wr16(0x0018D3 + x, v);
    }
    return v;
}

void abi_SelectionTables18CB(void)
{
    cyc(122);
    uint16_t v = SelectionTables18CB(cpu.db);

    cpu.a = v;
    cpu.x = 0x0008;
}

void abi_SelectionTables18CB(void);

/*
 * @routine 03:B70A SelectionTables18CBLong
 * WHAT IT DOES: SelectionTables18CB for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as SelectionTables18CB. Exit: as SelectionTables18CB.
 */
void SelectionTables18CBLong(uint8_t bank)
{
    cyc(230);
    SelectionTables18CB(bank);
}

void abi_SelectionTables18CBLong(void)
{
    abi_SelectionTables18CB();
}
