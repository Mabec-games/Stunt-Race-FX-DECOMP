/*
 * tables.c: lookups into small tables in ROM: handler addresses and the
 * per-course table index ($0E3D holds the course number).
 */
#include "machine.h"
#include "carried.h"
#include "wram_names.h"

static uint16_t word_lookup(uint32_t table, uint16_t i)
{
    cyc(190);
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

/* HandlerTableA on carried registers. */
void HandlerTableA_cr(Carried *cr)
{
    uint16_t x = (uint16_t)(cr->a << 1);

    cr->a = HandlerTableA(cr->a);
    cr->x = (cr->p & P_X) ? (uint8_t)x : x;
}

void abi_HandlerTableA(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HandlerTableA_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:B6F9 HandlerTableB
 * WHAT IT DOES: Returns entry A of the handler address table at $03:B704.
 * HOW: Word lookup at A * 2.
 * Entry: as HandlerTableA. Exit: as HandlerTableA.
 */
uint16_t HandlerTableB(uint16_t i)
{
    return word_lookup(0x03B704, i);
}

/* HandlerTableB on carried registers. */
void HandlerTableB_cr(Carried *cr)
{
    uint16_t x = (uint16_t)(cr->a << 1);

    cr->a = HandlerTableB(cr->a);
    cr->x = (cr->p & P_X) ? (uint8_t)x : x;
}

void abi_HandlerTableB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    HandlerTableB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(62);
    wr8(0x0000F1, a);
    cyc(44);
    f = rd8(0x0000F1);
    a3 = (uint8_t)((uint8_t)(a << 1) + f);
    cyc(16);
    wr8(0x0000F1, a3);
    cyc(24);
    c = rd8(0x000E3D);
    cyc(52);
    c2 = rd8(0x000E3D);
    c12 = (uint8_t)((uint8_t)((uint8_t)(c << 1) + c2) << 2);
    cyc(58);
    a3 = rd8(0x0000F1);
    s = (unsigned)c12 + a3;
    *p = (uint8_t)((nz8(*p, (uint8_t)s) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(c12 ^ a3) & (c12 ^ (uint8_t)s) & 0x80) ? P_V : 0));
    cyc(56);
    return (uint8_t)s;
}

/* CourseIndex12 on carried registers. */
void CourseIndex12_cr(Carried *cr)
{
    uint8_t p = cr->p, i = CourseIndex12((uint8_t)cr->a, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | i);
    cr->x = i;
    set_p(cr, p);
}

void abi_CourseIndex12(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseIndex12_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

/* CourseDataPointer on carried registers. */
void CourseDataPointer_cr(Carried *cr)
{
    uint8_t bank, p = cr->p;
    uint16_t x = CourseDataPointer(&bank, &p);

    cr->x = x;
    cr->y = x;
    cr->a = (uint16_t)((x & 0xFF00) | bank);
    set_p(cr, (uint8_t)((nz16(p, x) & ~P_X) | P_M));
}

void abi_CourseDataPointer(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    CourseDataPointer_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

    cyc(76);
    base = rd16(cyc_then(100, 0x0BB00C + (uint16_t)(rd16(W(selection)) << 1)));
    wr16(b | W(scratch_11df), base);
    cyc(32);
    v = rd16(W(selection_1));
    cyc(60);
    w = rd16(W(selection_1));
    s1 = (uint16_t)((uint16_t)(v << 1) + w);
    cyc(24);
    wr16(b | W(scratch_11e1), s1);
    cyc(32);
    v = rd16(W(selection_2));
    cyc(88);
    w = rd16(W(selection_2));
    cyc(38);
    u = rd16(b | W(scratch_11e1));
    s1 = (uint16_t)((uint16_t)(v << 3) + w + u);
    cyc(24);
    wr16(b | W(scratch_11e1), s1);
    cyc(32);
    v = rd16(W(selection_3));
    cyc(60);
    w = rd16(W(selection_3));
    i3 = (uint16_t)((uint16_t)(v << 1) + w);
    cyc(24);
    wr16(b | W(scratch_11e3), i3);
    cyc(80);
    u = rd16(b | W(scratch_11e3));
    x = (uint16_t)((uint16_t)(i3 << 3) + u);
    cyc(38);
    u = rd16(b | W(scratch_11e1));
    x = (uint16_t)(x + u);
    cyc(38);
    u = rd16(b | W(scratch_11df));
    x = (uint16_t)(x + u);
    cyc(118);
    wr16(W(wram_18E5), rd16(0x0BB014 + x) & 3);
    cyc(104);
    wr16(W(wram_18E7), rd16(0x0BB015 + x) & 3);
    cyc(104);
    wr16(W(wram_18E9), rd16(0x0BB016 + x) & 3);
    cyc(32);
    wr16(W(selection_derived), rd16(cyc_then(118, 0x03B7B1 + rd16(W(selection)))) & 3);
    for (x = 0; x < 8; x += 2) {
        cyc(86);
        uint16_t idx = (uint16_t)(rd16(W(selection) + x) << 2);

        cyc(74);
        u = rd16(W(selection_derived) + x);
        idx = (uint16_t)(idx + u);
        cyc(154);
        v = rd16(0x03B7B5 + idx) & 3;
        wr16(W(selection_out) + x, v);
        cyc(50);
    }
    cyc(88);
    return v;
}

/* SelectionTables18CB on carried registers. */
void SelectionTables18CB_cr(Carried *cr)
{
    uint16_t v = SelectionTables18CB(cr->db);

    cr->a = v;
    cr->x = 0x0008;
}

void abi_SelectionTables18CB(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SelectionTables18CB_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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
    cyc(92);
    SelectionTables18CB(bank);
    cyc(44);
}

/* SelectionTables18CBLong on carried registers: SelectionTables18CB's. */
void SelectionTables18CBLong_cr(Carried *cr)
{
    SelectionTables18CB_cr(cr);
}

void abi_SelectionTables18CBLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SelectionTables18CBLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
