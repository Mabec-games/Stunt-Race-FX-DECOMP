/*
 * course.c: the course number $0E3D, from the menu's selections.
 */
#include "machine.h"
#include "cpu_ops.h"

static uint32_t dbw(uint8_t bank, uint16_t a)
{
    return (uint32_t)bank << 16 | a;
}

/*
 * @routine 02:EB51 CourseFromSelection
 * WHAT IT DOES: Sets the course number $0E3D: $0D4A * 4 + $0D48, or $0D4A
 * + $14 when $0D4C is set (not reached in the traces so far).
 * HOW: 8-bit arithmetic through the data bank.
 * Entry: M 8-bit. Exit: A low byte = the course; N, Z, C, V from the add.
 */
uint8_t CourseFromSelection(uint8_t bank, uint8_t *p)
{
    uint8_t a, b;
    unsigned s;

    if (rd8(dbw(bank, 0x0D4C))) {
        a = rd8(dbw(bank, 0x0D4A));
        b = 0x14;
    } else {
        a = rd8(dbw(bank, 0x0D4A));
        a = (uint8_t)(a << 2);
        b = rd8(dbw(bank, 0x0D48));
    }
    s = (unsigned)a + b;
    *p = (uint8_t)((nz8(*p, (uint8_t)s) & ~(P_C | P_V)) | (s > 0xFF ? P_C : 0)
                   | ((~(a ^ b) & (a ^ s) & 0x80) ? P_V : 0));
    wr8(dbw(bank, 0x0E3D), (uint8_t)s);
    return (uint8_t)s;
}

void abi_CourseFromSelection(void)
{
    uint8_t p = cpu.p;

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | CourseFromSelection(cpu.db, &p));
    cpu.p = p;
}

/*
 * @routine 02:EB4D CourseFromSelectionLong
 * WHAT IT DOES: CourseFromSelection for callers in other banks.
 * HOW: JSR to it, then RTL.
 * Entry: as CourseFromSelection. Exit: as CourseFromSelection.
 */
uint8_t CourseFromSelectionLong(uint8_t bank, uint8_t *p)
{
    return CourseFromSelection(bank, p);
}

void abi_CourseFromSelectionLong(void)
{
    abi_CourseFromSelection();
}

/*
 * @routine 02:EC10 RaceFadeStageDispatch
 * WHAT IT DOES: Runs the handler for stage $0D48 from the table at
 * $02:EC18 ($ECB8, $ECD9, $EC30, $EC73).
 * HOW: ASL, TAX, JMP (table,X) with 8-bit registers; the handler returns
 * to the caller.
 * Entry: M 8-bit, X 8-bit. Exit: as the handler leaves it.
 */
void abi_RaceFadeStageDispatch(void)
{
    uint8_t v;

    op_lda8((uint32_t)cpu.db << 16 | 0x0D48);
    v = (uint8_t)cpu.a;
    cpu.p = (uint8_t)((cpu.p & ~P_C) | (v >> 7));
    v = (uint8_t)(v << 1);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.x = v;
    cpu.p = nz8(cpu.p, v);
    call_routine(0x020000 | rd16(0x020000 | (uint16_t)(0xEC18 + v)), 0);
}
