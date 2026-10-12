/*
 * selection.c: small steps of the menu screens' selection logic: the
 * choices at $18C9-$18D3, values derived from them at $18E3-$1907, and the
 * menu's exit flag at $11EB.
 */
#include "machine.h"

/*
 * @routine 0B:C58D MenuRowPosition
 * WHAT IT DOES: Returns a position from object X's +$12 and the value at
 * $18ED: (2 - $18ED) * 16 + the field + 2.
 * HOW: 16-bit arithmetic; the field is read as direct page $12 + X.
 * Entry: M 16-bit, D = $0000; X = record. Exit: A = the position; N, Z from
 * it; C, V from the add.
 */
uint16_t MenuRowPosition(uint16_t rec, uint8_t *p)
{
    uint16_t a = (uint16_t)((uint16_t)(2 - rd16(0x0018ED)) << 4), b = rd16((uint16_t)(rec + 0x12));
    unsigned s = (unsigned)a + b;

    *p = (uint8_t)((*p & ~(P_C | P_V)) | (s > 0xFFFF ? P_C : 0) | ((~(a ^ b) & (a ^ (uint16_t)s) & 0x8000) ? P_V : 0));
    a = (uint16_t)(s + 2);
    *p = nz16(*p, a);
    return a;
}

void abi_MenuRowPosition(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuRowPosition(cpu.x, &p);
    cpu.p = p;
}

/*
 * @routine 0B:C736 MenuChoiceRepeat
 * WHAT IT DOES: Sets $18D3 to the table value for $18C9 when the choice at
 * $18CB equals it, and to 0 otherwise.
 * HOW: Clears $18D3; on a match reads word table $0B:C758[$18C9].
 * Entry: any widths. Exit: M 16-bit, X 16-bit. No match: A = $18CB; N, Z, C
 * from the compare. Match: A = the value, X = $18C9 * 2; N, Z from A; C =
 * bit 15 of $18C9 (shifted out by the doubling).
 */
uint16_t MenuChoiceRepeat(uint16_t *x, uint8_t *p)
{
    uint16_t a = rd16(0x0018CB), b;

    wr16(0x0018D3, 0x0000);
    b = rd16(0x0018C9);
    *p = (uint8_t)((nz16(*p, (uint16_t)(a - b)) & ~P_C) | (a >= b ? P_C : 0));
    if (a != b)
        return a;
    b = rd16(0x0018C9);
    *x = (uint16_t)(b << 1);
    a = rd16(0x0BC758 + *x);
    wr16(0x0018D3, a);
    *p = (uint8_t)((nz16(*p, a) & ~P_C) | (b & 0x8000 ? P_C : 0));
    return a;
}

void abi_MenuChoiceRepeat(void)
{
    uint8_t p = cpu.p;

    cpu.a = MenuChoiceRepeat(&cpu.x, &p);
    cpu.p = (uint8_t)(p & ~(P_M | P_X));
}

/*
 * @routine 0B:BCD2 MenuWaitFor0357
 * WHAT IT DOES: An object handler that waits for the byte at $0357 to become
 * non-zero, then sets $1905 = 2, $1907 = 0 and moves on to handler $BCF1.
 * HOW: Byte test through the data bank; the handler is object X's +$08.
 * Entry: D = $0000; X = record. Exit: M 16-bit. Waiting: A low byte = 0; Z
 * set, N clear. Moved on: A = $BCF1; N set, Z clear.
 */
int MenuWaitFor0357(uint8_t bank, uint16_t rec)
{
    if (!rd8((uint32_t)bank << 16 | 0x0357))
        return 0;
    wr16(0x001905, 0x0002);
    wr16(0x001907, 0x0000);
    wr16((uint16_t)(rec + 0x08), 0xBCF1);
    return 1;
}

void abi_MenuWaitFor0357(void)
{
    if (MenuWaitFor0357(cpu.db, cpu.x)) {
        cpu.a = 0xBCF1;
        cpu.p = (uint8_t)((cpu.p & ~(P_Z | P_M)) | P_N);
    } else {
        cpu.a &= 0xFF00;
        cpu.p = (uint8_t)((cpu.p & ~(P_N | P_M)) | P_Z);
    }
}

/*
 * @routine 0B:D4A5 MenuSetExit
 * WHAT IT DOES: Sets the menu's exit flag at $11EB.
 * HOW: Stores 1 (16-bit) through the data bank.
 * Entry: any widths. Exit: A = 1; M 16-bit; N, Z clear.
 */
void MenuSetExit(uint8_t bank)
{
    wr16((uint32_t)bank << 16 | 0x11EB, 0x0001);
}

void abi_MenuSetExit(void)
{
    MenuSetExit(cpu.db);
    cpu.a = 0x0001;
    cpu.p = (uint8_t)(cpu.p & ~(P_N | P_Z | P_M));
}

/*
 * @routine 0B:C723 MenuSet1A66
 * WHAT IT DOES: Copies entry A of the word table at $00:116C to $1A66.
 * HOW: Long read at $116C + A * 2; the store goes through the data bank.
 * Entry: any widths; A = entry (16 bits). Exit: A unchanged; X = A * 2; Y =
 * $1A64 (low bytes only when X is 8-bit); P unchanged.
 */
void MenuSet1A66(uint8_t bank, uint16_t i)
{
    wr16((uint32_t)bank << 16 | 0x1A66, rd16(0x00116C + (uint16_t)(i << 1)));
}

void abi_MenuSet1A66(void)
{
    MenuSet1A66(cpu.db, cpu.a);
    cpu.x = (uint16_t)(cpu.a << 1);
    cpu.y = 0x1A64;
    if (cpu.p & P_X) {
        cpu.x &= 0x00FF;
        cpu.y &= 0x00FF;
    }
}

/*
 * @routine 0B:D4C3 MenuStateInit
 * WHAT IT DOES: Sets the menu's selection state to its defaults: the four
 * choices at $18CB-$18D1 to 0, 0, 1, 2, the derived values and positions to
 * their starting values, the draw-buffer offset $0D7D to 6, and three
 * values at $18EF-$18F3 from the save data ($70:FC08-$70:FC0A).
 * HOW: 16-bit long stores in the original's order.
 * Entry: any widths. Exit: A = $001F; P unchanged.
 */
void MenuStateInit(void)
{
    static const struct { uint32_t addr; uint16_t v; } set[] = {
        {0x000D7D, 0x0006}, {0x0018FD, 0x0000}, {0x001903, 0x0000}, {0x0018B9, 0x0001},
        {0x001901, 0x0000}, {0x0018FF, 0x0000}, {0x0018F7, 0x0000}, {0x0018C3, 0x0000},
        {0x0018CB, 0x0000}, {0x0018C9, 0x0000}, {0x0018ED, 0x0000}, {0x0018CD, 0x0000},
        {0x0018F5, 0x0000}, {0x0018D3, 0x0000}, {0x0018D5, 0x0000}, {0x0018D7, 0x0000},
        {0x0018D9, 0x0003}, {0x0018C7, 0x0001}, {0x0018EB, 0x0001}, {0x0018CF, 0x0001},
        {0x0018D1, 0x0002}};
    unsigned i;

    for (i = 0; i < sizeof set / sizeof set[0]; i++)
        wr16(set[i].addr, set[i].v);
    wr16(0x0018EF, rd16(0x70FC08) & 0x00FF);
    wr16(0x0018F1, rd16(0x70FC09) & 0x00FF);
    wr16(0x0018F3, rd16(0x70FC0A) & 0x00FF);
    wr16(0x0018C5, 0xFFFF);
    wr16(0x0018F9, 0x001F);
}

void abi_MenuStateInit(void)
{
    MenuStateInit();
    cpu.a = 0x001F;
}

uint16_t HandlerTableA(uint16_t i);
uint8_t SaveChecksum(void);

/*
 * @routine 03:B548 SelectionCommit
 * WHAT IT DOES: Records the two choices at $18CB and $18C9: their handler
 * addresses (HandlerTableA) at $18DB and $18DD; both packed as a byte (the
 * table at $03:B540 for each low two bits, $18C9's in the high nibble) into
 * the save data at $70:FC0E, with the checksum renewed; and the derived
 * values $18E3 and $18E5 from the table at $03:B46C, where $18D5 is the
 * table $03:B464 entry for $18C9 when the two choices are the same, else 0.
 * HOW: 16-bit lookups; $11DF holds the high nibble.
 * Entry: any widths. Exit: A = $18E5's value; X = its table offset; P
 * unchanged.
 */
uint16_t SelectionCommit(uint8_t bank, uint16_t *x)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;

    wr16(0x0018DB, HandlerTableA(rd16(0x0018CB)));
    wr16(0x0018DD, HandlerTableA(rd16(0x0018C9)));
    wr16(b | 0x11DF, (uint16_t)(rd16(0x03B540 + ((rd16(0x0018C9) & 3) << 1)) << 4));
    v = (uint16_t)(rd16(0x03B540 + ((rd16(0x0018CB) & 3) << 1)) | rd16(b | 0x11DF));
    wr8(0x70FC0E, (uint8_t)v);
    SaveChecksum();
    wr16(0x0018D3, 0x0000);
    wr16(0x0018D5, 0x0000);
    if (rd16(0x0018CB) == rd16(0x0018C9))
        wr16(b | 0x18D5, rd16(0x03B464 + (uint16_t)(rd16(0x0018C9) << 1)));
    *x = (uint16_t)((uint16_t)((uint16_t)(rd16(0x0018CB) << 2) + rd16(0x0018D3)) << 1);
    wr16(0x0018E3, rd16(0x03B46C + *x));
    *x = (uint16_t)((uint16_t)((uint16_t)(rd16(0x0018C9) << 2) + rd16(0x0018D5)) << 1);
    v = rd16(0x03B46C + *x);
    wr16(0x0018E5, v);
    return v;
}

void abi_SelectionCommit(void)
{
    uint16_t x;

    cpu.a = SelectionCommit(cpu.db, &x);
    cpu.x = (cpu.p & P_X) ? (uint8_t)x : x;
}
