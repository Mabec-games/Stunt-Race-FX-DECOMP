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
