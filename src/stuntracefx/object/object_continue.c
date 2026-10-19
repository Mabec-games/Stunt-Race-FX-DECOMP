/*
 * object_continue.c: object commands of bank $03 that end by going on at
 * an address kept in the object's record: the long address at +$1A
 * (bank at +$1C) or at +$1D (bank at +$1F), reached by pushing it less
 * one and an RTL, so the code there returns to the command's caller; with
 * both bank and address 0 the command just returns. Seven of them first
 * set the byte at +$39. No play runs them; unit_object_continue calls them.
 */
#include "machine.h"

/* Go on at the long address at +addr_at (bank at +bank_at) of object X,
   or return when it is 0. */
static void continue_at(uint16_t addr_at, uint16_t bank_at)
{
    uint32_t b = (uint32_t)cpu.db << 16;
    uint8_t bank;
    uint16_t addr;

    cyc(30);
    bank = rd8(b | (uint16_t)(bank_at + cpu.x));
    addr = rd16(b | (uint16_t)(addr_at + cpu.x));
    if (!bank && (cyc(16), !addr)) {
        cyc(56);
        cpu.a = 0x0000;
        cpu.p = nz8((uint8_t)(cpu.p | P_M), 0x00);
        return;
    } else { cyc(110); }
    cpu.a = (uint16_t)(addr - 1);
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M);
    jump_routine((uint32_t)bank << 16 | addr);
}

/*
 * @routine 03:D29E ObjectContinue1A
 * WHAT IT DOES: Goes on at object X's long address at +$1A (bank +$1C);
 * returns when both are 0.
 * HOW: The bank pushed, then the address less one (16-bit), then RTL; the
 * code there returns to this routine's caller. With bank and address 0
 * the bank byte is pulled back and it returns.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: going on: the code there is
 * entered with A = the address less one, M 8-bit, N, Z from A (16-bit);
 * returning: A = 0, M 8-bit, Z set, N clear.
 */
void ObjectContinue1A(void)
{
    continue_at(0x201A, 0x201C);
}

void abi_ObjectContinue1A(void)
{
    continue_at(0x201A, 0x201C);
}

/*
 * @routine 03:D2BC ObjectContinue1D
 * WHAT IT DOES: As ObjectContinue1A for the long address at +$1D (bank
 * +$1F).
 * HOW: As ObjectContinue1A.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectContinue1D(void)
{
    continue_at(0x201D, 0x201F);
}

void abi_ObjectContinue1D(void)
{
    continue_at(0x201D, 0x201F);
}

/*
 * @routine 03:D29D ObjectStubReturn
 * WHAT IT DOES: Nothing.
 * HOW: A lone RTL.
 * Entry: any. Exit: everything unchanged.
 */
void ObjectStubReturn(void)
{
}

void abi_ObjectStubReturn(void)
{
    cyc(44);
}

/* The +$39 setters: the byte written (8-bit), then ObjectContinue1A. */
static void set_39(uint8_t v)
{
    cyc(44);
    wr8((uint32_t)cpu.db << 16 | (uint16_t)(0x2039 + cpu.x), v);
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cyc(62);
    continue_at(0x201A, 0x201C);
}

/*
 * @routine 03:D25F ObjectStep39Continue
 * WHAT IT DOES: Counts object X's +$39 up by one, then ObjectContinue1A.
 * HOW: An 8-bit LDA / INC / STA, then a long jump to ObjectContinue1A.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectStep39Continue(void)
{
    cyc(30);
    uint8_t v = rd8((uint32_t)cpu.db << 16 | (uint16_t)(0x2039 + cpu.x));

    set_39((uint8_t)(v + 1));
}

void abi_ObjectStep39Continue(void)
{
    ObjectStep39Continue();
}

/*
 * @routine 03:D269 ObjectSet39To0Continue
 * WHAT IT DOES: Sets object X's +$39 to 0, then ObjectContinue1A.
 * HOW: An 8-bit store, then a long jump to ObjectContinue1A.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectSet39To0Continue(void)
{
    set_39(0x00);
}

void abi_ObjectSet39To0Continue(void)
{
    cyc(2);
    set_39(0x00);
}

/*
 * @routine 03:D271 ObjectSet39To1Continue
 * WHAT IT DOES: Sets object X's +$39 to 1, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectSet39To1Continue(void)
{
    set_39(0x01);
}

void abi_ObjectSet39To1Continue(void)
{
    cyc(2);
    set_39(0x01);
}

/*
 * @routine 03:D279 ObjectSet39To2Continue
 * WHAT IT DOES: Sets object X's +$39 to 2, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectSet39To2Continue(void)
{
    set_39(0x02);
}

void abi_ObjectSet39To2Continue(void)
{
    cyc(2);
    set_39(0x02);
}

/*
 * @routine 03:D281 ObjectSet39To3Continue
 * WHAT IT DOES: Sets object X's +$39 to 3, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectSet39To3Continue(void)
{
    set_39(0x03);
}

void abi_ObjectSet39To3Continue(void)
{
    cyc(2);
    set_39(0x03);
}

/*
 * @routine 03:D289 ObjectSet39To4Continue
 * WHAT IT DOES: Sets object X's +$39 to 4, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectSet39To4Continue(void)
{
    set_39(0x04);
}

void abi_ObjectSet39To4Continue(void)
{
    cyc(2);
    set_39(0x04);
}

/*
 * @routine 03:D291 ObjectSet39To5Continue
 * WHAT IT DOES: Sets object X's +$39 to 5, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
void ObjectSet39To5Continue(void)
{
    set_39(0x05);
}

void abi_ObjectSet39To5Continue(void)
{
    cyc(2);
    set_39(0x05);
}
