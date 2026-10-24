/*
 * object_continue.c: object commands of bank $03 that end by going on at
 * an address kept in the object's record: the long address at +$1A
 * (bank at +$1C) or at +$1D (bank at +$1F), reached by pushing it less
 * one and an RTL, so the code there returns to the command's caller; with
 * both bank and address 0 the command just returns. Seven of them first
 * set the byte at +$39. No play runs them; unit_object_continue calls them.
 */
#include "machine.h"
#include "object_calls.h"
#include "carried.h"
#include "wram_names.h"


/* Go on at the long address at +addr_at (bank at +bank_at) of object obj
   in bank `db`: returns it (bank << 16 | address) for the caller to go on
   at, or 0 when it is 0 and the command just returns. *a is the A the code
   there is entered (or the command returns) with: the address less one. */
static uint32_t continue_at(uint8_t db, uint16_t obj, uint16_t addr_at, uint16_t bank_at, uint16_t *a)
{
    uint32_t b = (uint32_t)db << 16;
    uint8_t bank;
    uint16_t addr;

    cyc(30);
    bank = rd8(b | (uint16_t)(bank_at + obj));
    cyc(bank ? 96 : 90);                /* PHA, REP, BNE, LDA abs,X */
    addr = rd16(b | (uint16_t)(addr_at + obj));
    if (!bank && (cyc(addr ? 16 : 22), !addr)) {   /* BEQ not taken / taken */
        cyc(94);                        /* SEP, PLA, RTL */
        *a = 0x0000;
        return 0;
    } else { cyc(110); }
    *a = (uint16_t)(addr - 1);
    return (uint32_t)bank << 16 | addr;
}

void ObjectContinue1A_cr(Carried *cr);
void ObjectContinue1D_cr(Carried *cr);
void ObjectSet39To0Continue_cr(Carried *cr);
void ObjectSet39To1Continue_cr(Carried *cr);
void ObjectSet39To2Continue_cr(Carried *cr);
void ObjectSet39To3Continue_cr(Carried *cr);
void ObjectSet39To4Continue_cr(Carried *cr);
void ObjectSet39To5Continue_cr(Carried *cr);
void ObjectStep39Continue_cr(Carried *cr);
static void continue_regs(Carried *cr, uint32_t target, uint16_t a);

/* The register exit of the commands above: A as given, M 8-bit, N and Z
   from A (16-bit); then on to `target` in the command's own frame. */
static void continue_regs(Carried *cr, uint32_t target, uint16_t a)
{
    cr->a = a;
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M));
    if (target)
        carried_jump(cr, target);
}

/*
 * @routine 03:D29E ObjectContinue1A
 * WHAT IT DOES: Goes on at object X's long address at +$1A (bank +$1C);
 * returns when both are 0.
 * HOW: The bank pushed, then the address less one (16-bit), then RTL; the
 * code there returns to this routine's caller. With bank and address 0
 * the bank byte is pulled back and it returns.
 * Entry: M 8-bit, X 16-bit; X = object (obj). Exit: going on: the code there is
 * entered with A = the address less one, M 8-bit, N, Z from A (16-bit);
 * returning: A = 0, M 8-bit, Z set, N clear.
 */
uint32_t ObjectContinue1A(uint8_t bank, uint16_t obj, uint16_t *a)
{
    return continue_at(bank, obj, 0x201A, 0x201C, a);
}

void ObjectContinue1A_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectContinue1A(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectContinue1A(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectContinue1A_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D2BC ObjectContinue1D
 * WHAT IT DOES: As ObjectContinue1A for the long address at +$1D (bank
 * +$1F).
 * HOW: As ObjectContinue1A.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectContinue1D(uint8_t bank, uint16_t obj, uint16_t *a)
{
    return continue_at(bank, obj, 0x201D, 0x201F, a);
}

void ObjectContinue1D_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectContinue1D(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectContinue1D(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectContinue1D_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
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

/* ObjectStubReturn on carried registers: no register work. */
void ObjectStubReturn_cr(Carried *cr)
{
    (void)cr;
    cyc(44);
}

void abi_ObjectStubReturn(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStubReturn_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/* The +$39 setters: the byte written (8-bit), then ObjectContinue1A. */
static uint32_t set_39(uint8_t bank, uint16_t obj, uint8_t v, uint16_t *a)
{
    cyc(44);
    wr8(0x7E0000 | (uint16_t)(W(obj_39) + obj), v);
    cyc(62);
    return continue_at(bank, obj, 0x201A, 0x201C, a);
}

/*
 * @routine 03:D25F ObjectStep39Continue
 * WHAT IT DOES: Counts object X's +$39 up by one, then ObjectContinue1A.
 * HOW: An 8-bit LDA / INC / STA, then a long jump to ObjectContinue1A.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectStep39Continue(uint8_t bank, uint16_t obj, uint16_t *a)
{
    cyc(30);
    uint8_t v = rd8(0x7E0000 | (uint16_t)(W(obj_39) + obj));

    return set_39(bank, obj, (uint8_t)(v + 1), a);
}

void ObjectStep39Continue_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectStep39Continue(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectStep39Continue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStep39Continue_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D269 ObjectSet39To0Continue
 * WHAT IT DOES: Sets object X's +$39 to 0, then ObjectContinue1A.
 * HOW: An 8-bit store, then a long jump to ObjectContinue1A.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectSet39To0Continue(uint8_t bank, uint16_t obj, uint16_t *a)
{
    cyc(2);
    return set_39(bank, obj, 0x00, a);
}

void ObjectSet39To0Continue_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectSet39To0Continue(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectSet39To0Continue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet39To0Continue_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D271 ObjectSet39To1Continue
 * WHAT IT DOES: Sets object X's +$39 to 1, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectSet39To1Continue(uint8_t bank, uint16_t obj, uint16_t *a)
{
    cyc(2);
    return set_39(bank, obj, 0x01, a);
}

void ObjectSet39To1Continue_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectSet39To1Continue(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectSet39To1Continue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet39To1Continue_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D279 ObjectSet39To2Continue
 * WHAT IT DOES: Sets object X's +$39 to 2, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectSet39To2Continue(uint8_t bank, uint16_t obj, uint16_t *a)
{
    cyc(2);
    return set_39(bank, obj, 0x02, a);
}

void ObjectSet39To2Continue_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectSet39To2Continue(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectSet39To2Continue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet39To2Continue_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D281 ObjectSet39To3Continue
 * WHAT IT DOES: Sets object X's +$39 to 3, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectSet39To3Continue(uint8_t bank, uint16_t obj, uint16_t *a)
{
    cyc(2);
    return set_39(bank, obj, 0x03, a);
}

void ObjectSet39To3Continue_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectSet39To3Continue(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectSet39To3Continue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet39To3Continue_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D289 ObjectSet39To4Continue
 * WHAT IT DOES: Sets object X's +$39 to 4, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectSet39To4Continue(uint8_t bank, uint16_t obj, uint16_t *a)
{
    cyc(2);
    return set_39(bank, obj, 0x04, a);
}

void ObjectSet39To4Continue_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectSet39To4Continue(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectSet39To4Continue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet39To4Continue_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D291 ObjectSet39To5Continue
 * WHAT IT DOES: Sets object X's +$39 to 5, then ObjectContinue1A.
 * HOW: As ObjectSet39To0Continue.
 * Entry: as ObjectContinue1A. Exit: as ObjectContinue1A.
 */
uint32_t ObjectSet39To5Continue(uint8_t bank, uint16_t obj, uint16_t *a)
{
    cyc(2);
    return set_39(bank, obj, 0x05, a);
}

void ObjectSet39To5Continue_cr(Carried *cr)
{
    uint16_t a;
    uint32_t target = ObjectSet39To5Continue(cr->db, cr->x, &a);

    continue_regs(cr, target, a);
}

void abi_ObjectSet39To5Continue(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectSet39To5Continue_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

