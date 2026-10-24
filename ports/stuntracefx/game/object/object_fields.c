/*
 * object_fields.c: small object helpers of bank $03 on the fields of one
 * or two object records ($62-byte records from $7E:2000, addressed by
 * their offset in X or Y): placing one at an offset from another, stepping
 * the byte at +$17 toward a target, clearing the velocity (+$2E, +$30,
 * +$32), and entries that return 0 or do nothing. No play runs them;
 * unit_objects_03 calls them.
 */
#include "machine.h"
#include "cpu_ops.h"
#include "object_calls.h"
#include "carried.h"
#include "wram_names.h"


#define FIELDS_SCRATCH 0x00003Eu

/*
 * @routine 03:CBE0 ObjectStubZero
 * WHAT IT DOES: Returns A = 0.
 * HOW: A 16-bit load of 0, then back to M 8-bit.
 * Entry: any widths. Exit: A = $0000; M 8-bit; Z set, N clear.
 */
uint16_t ObjectStubZero(void)
{
    return 0x0000;
}

void ObjectPlaceAtOffset_cr(Carried *cr);
void ObjectStepByte17_cr(Carried *cr);
void ObjectStopX_cr(Carried *cr);
void ObjectStopY_cr(Carried *cr);
void ObjectStubNothing_cr(Carried *cr);
void ObjectStubSwapTwice_cr(Carried *cr);
void ObjectStubZero_cr(Carried *cr);
void ObjectStubZeroCopy_cr(Carried *cr);

void ObjectStubZero_cr(Carried *cr)
{
    cr->a = (cyc(112), ObjectStubZero());
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M));
}

void abi_ObjectStubZero(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStubZero_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CBE8 ObjectStubZeroCopy
 * WHAT IT DOES: The same bytes as ObjectStubZero, at another address.
 * HOW: As ObjectStubZero.
 * Entry: as ObjectStubZero. Exit: as ObjectStubZero.
 */
uint16_t ObjectStubZeroCopy(void)
{
    return 0x0000;
}

void ObjectStubZeroCopy_cr(Carried *cr)
{
    cr->a = (cyc(112), ObjectStubZeroCopy());
    set_p(cr, (uint8_t)(nz16(cr->p, cr->a) | P_M));
}

void abi_ObjectStubZeroCopy(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStubZeroCopy_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CBF0 ObjectPlaceAtOffset
 * WHAT IT DOES: Puts object Y at object X's position plus an offset: +$0C
 * = X's +$0C + the word at $04, +$10 = X's +$10 + the word at $05F2, +$0E =
 * X's +$0E + the word at $0A.
 * HOW: Three 16-bit adds in that order, through the caller's data bank.
 * Entry: M 16-bit, D = $0000; X, Y = objects. Exit: A = the last sum; N,
 * Z, C, V from its add.
 */
uint16_t ObjectPlaceAtOffset(uint8_t bank, uint16_t from, uint16_t to, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t v;

    cyc(30);
    v = rd16(b | (uint16_t)(0x200C + from));
    *p &= (uint8_t)~P_C;
    cyc(30);
    v = alu_adc16(v, rd16(W(dp_04)), p);
    cyc(30);
    wr16(b | (uint16_t)(0x200C + to), v);
    cyc(30);
    v = rd16(b | (uint16_t)(0x2010 + from));
    *p &= (uint8_t)~P_C;
    cyc(38);
    v = alu_adc16(v, rd16(b | W(wram_05F2)), p);
    cyc(30);
    wr16(b | (uint16_t)(0x2010 + to), v);
    cyc(30);
    v = rd16(b | (uint16_t)(0x200E + from));
    *p &= (uint8_t)~P_C;
    cyc(30);
    v = alu_adc16(v, rd16(W(dp_0a)), p);
    cyc(30);
    wr16(b | (uint16_t)(0x200E + to), v);
    return v;
}

void ObjectPlaceAtOffset_cr(Carried *cr)
{
    uint8_t p = cr->p;

    cr->a = ObjectPlaceAtOffset(cr->db, cr->x, cr->y, &p);
    set_p(cr, p);
    cyc(44);                            /* RTL */
}

void abi_ObjectPlaceAtOffset(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectPlaceAtOffset_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:CD8F ObjectStepByte17
 * WHAT IT DOES: Steps the byte at object X's +$17 toward the byte at $1F2B
 * by A: onto the target when it is less than A away, else A nearer. Carry
 * set when it was already there (nothing written), clear when it moved.
 * HOW: The step kept in direct page $3E. The distance is the 8-bit
 * difference made positive; the direction comes from comparing the two
 * bytes, by the sign of the result (BMI), not as numbers. Through the
 * caller's data bank.
 * Entry: M 8-bit, X 16-bit, D = $0000; A = step, X = object. Exit: there:
 * A = 0, Z set, N clear, V from the subtract, C set; moved: A = the new
 * byte, N, Z from it, V from the last add or subtract, C clear.
 */
uint8_t ObjectStepByte17(uint8_t bank, uint8_t step, uint16_t obj, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint8_t v, d, s;

    cyc(16);
    wr8(FIELDS_SCRATCH, step);
    cyc(30);
    v = rd8(b | (uint16_t)(0x2017 + obj));
    *p |= P_C;
    cyc(38);
    d = alu_sbc8(v, rd8(b | W(wram_1F2B)), p);
    if (!d) {
        cyc(64);                        /* BEQ taken, SEC, RTL */
        *p |= P_C;
        return d;
    }
    if (d & 0x80)
        { cyc(24); d = (uint8_t)(~d + 1); }
    cyc(38);
    s = rd8(FIELDS_SCRATCH);
    if ((uint8_t)(d - s) & 0x80) {
        cyc(40);
        v = rd8(b | W(wram_1F2B));
        *p = nz8(*p, v);
        cyc(22);                        /* BRA */
    } else {
        cyc(52);
        v = rd8(b | (uint16_t)(0x2017 + obj));
        cyc(24);
        s = rd8(b | W(wram_1F2B));
        if (v != s) {
            if ((uint8_t)(v - s) & 0x80) {
                *p &= (uint8_t)~P_C;
                cyc(68);
                v = alu_adc8(v, rd8(FIELDS_SCRATCH), p);
            } else {
                *p |= P_C;
                cyc(62);
                v = alu_sbc8(v, rd8(FIELDS_SCRATCH), p);
                cyc(6);
            }
        } else {
            *p = nz8(*p, 0x00);
        }
    }
    cyc(30);
    wr8(b | (uint16_t)(0x2017 + obj), v);
    *p &= (uint8_t)~P_C;
    cyc(58);                            /* CLC, RTL */
    return v;
}

void ObjectStepByte17_cr(Carried *cr)
{
    uint8_t p = cr->p, v = ObjectStepByte17(cr->db, (uint8_t)cr->a, cr->x, &p);

    cr->a = (uint16_t)((cr->a & 0xFF00) | v);
    set_p(cr, p);
}

void abi_ObjectStepByte17(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStepByte17_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D1B2 ObjectStubNothing
 * WHAT IT DOES: Nothing but leave M 8-bit.
 * HOW: REP #$20, SEP #$20, RTL.
 * Entry: any widths. Exit: M 8-bit; all else unchanged.
 */
void ObjectStubNothing(void)
{
    cyc(88);
}

void ObjectStubNothing_cr(Carried *cr)
{
    ObjectStubNothing();
    set_p(cr, (uint8_t)(cr->p | (P_M)));
}

void abi_ObjectStubNothing(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStubNothing_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D1B7 ObjectStubSwapTwice
 * WHAT IT DOES: Nothing but leave M 8-bit: X and Y are swapped and swapped
 * back.
 * HOW: PHX / TYX / PLY twice, around REP #$20, SEP #$20.
 * Entry: any widths. Exit: X, Y unchanged; M 8-bit; N, Z from Y (index
 * width).
 */
void ObjectStubSwapTwice(int x8)
{
    cyc(x8 ? 216 : 248);
}

void ObjectStubSwapTwice_cr(Carried *cr)
{
    int x8 = (cr->p & P_X) != 0;

    set_p(cr, (uint8_t)(cr->p | (P_M)));
    ObjectStubSwapTwice(x8);
    set_p(cr, x8 ? nz8(cr->p, (uint8_t)cr->y) : nz16(cr->p, cr->y));
}

void abi_ObjectStubSwapTwice(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStubSwapTwice_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D23A ObjectStopX
 * WHAT IT DOES: Stops object X: its velocity (+$2E, +$30, +$32) and the
 * byte at +$17 to 0.
 * HOW: 16-bit STZs for the velocity, then an 8-bit STZ. Through the
 * caller's data bank.
 * Entry: X = object. Exit: M 8-bit; A and the flags unchanged.
 */
void ObjectStopX(uint8_t bank, uint16_t obj)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(52);
    wr16(b | (uint16_t)(0x202E + obj), 0x0000);
    cyc(30);
    wr16(b | (uint16_t)(0x2030 + obj), 0x0000);
    cyc(30);
    wr16(b | (uint16_t)(0x2032 + obj), 0x0000);
    cyc(52);
    wr8(b | (uint16_t)(0x2017 + obj), 0x00);
}

void ObjectStopX_cr(Carried *cr)
{
    ObjectStopX(cr->db, cr->x);
    set_p(cr, (uint8_t)(cr->p | (P_M)));
    cyc(44);                            /* RTL */
}

void abi_ObjectStopX(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStopX_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 03:D24B ObjectStopY
 * WHAT IT DOES: ObjectStopX for object Y.
 * HOW: A 16-bit load of 0 stored to the velocity, then its low byte to
 * +$17. Through the caller's data bank.
 * Entry: Y = object. Exit: A = $0000; M 8-bit; Z set, N clear.
 */
void ObjectStopY(uint8_t bank, uint16_t obj)
{
    uint32_t b = (uint32_t)bank << 16;

    cyc(76);
    wr16(b | (uint16_t)(0x202E + obj), 0x0000);
    cyc(30);
    wr16(b | (uint16_t)(0x2030 + obj), 0x0000);
    cyc(30);
    wr16(b | (uint16_t)(0x2032 + obj), 0x0000);
    cyc(52);
    wr8(b | (uint16_t)(0x2017 + obj), 0x00);
}

void ObjectStopY_cr(Carried *cr)
{
    ObjectStopY(cr->db, cr->y);
    cr->a = 0x0000;
    set_p(cr, (uint8_t)(nz16(cr->p, 0x0000) | P_M));
    cyc(44);                            /* RTL */
}

void abi_ObjectStopY(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    ObjectStopY_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

