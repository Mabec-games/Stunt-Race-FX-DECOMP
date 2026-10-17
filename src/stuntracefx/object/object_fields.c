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

void abi_ObjectStubZero(void)
{
    cpu.a = ObjectStubZero();
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M);
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

void abi_ObjectStubZeroCopy(void)
{
    cpu.a = ObjectStubZeroCopy();
    cpu.p = (uint8_t)(nz16(cpu.p, cpu.a) | P_M);
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

    v = rd16(b | (uint16_t)(0x200C + from));
    *p &= (uint8_t)~P_C;
    v = op_adc16(v, rd16(0x000004), p);
    wr16(b | (uint16_t)(0x200C + to), v);
    v = rd16(b | (uint16_t)(0x2010 + from));
    *p &= (uint8_t)~P_C;
    v = op_adc16(v, rd16(b | 0x05F2), p);
    wr16(b | (uint16_t)(0x2010 + to), v);
    v = rd16(b | (uint16_t)(0x200E + from));
    *p &= (uint8_t)~P_C;
    v = op_adc16(v, rd16(0x00000A), p);
    wr16(b | (uint16_t)(0x200E + to), v);
    return v;
}

void abi_ObjectPlaceAtOffset(void)
{
    uint8_t p = cpu.p;

    cpu.a = ObjectPlaceAtOffset(cpu.db, cpu.x, cpu.y, &p);
    cpu.p = p;
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

    wr8(FIELDS_SCRATCH, step);
    v = rd8(b | (uint16_t)(0x2017 + obj));
    *p |= P_C;
    d = op_sbc8(v, rd8(b | 0x1F2B), p);
    if (!d) {
        *p |= P_C;
        return d;
    }
    if (d & 0x80)
        d = (uint8_t)(~d + 1);
    s = rd8(FIELDS_SCRATCH);
    if ((uint8_t)(d - s) & 0x80) {
        v = rd8(b | 0x1F2B);
        *p = nz8(*p, v);
    } else {
        v = rd8(b | (uint16_t)(0x2017 + obj));
        s = rd8(b | 0x1F2B);
        if (v != s) {
            if ((uint8_t)(v - s) & 0x80) {
                *p &= (uint8_t)~P_C;
                v = op_adc8(v, rd8(FIELDS_SCRATCH), p);
            } else {
                *p |= P_C;
                v = op_sbc8(v, rd8(FIELDS_SCRATCH), p);
            }
        } else {
            *p = nz8(*p, 0x00);
        }
    }
    wr8(b | (uint16_t)(0x2017 + obj), v);
    *p &= (uint8_t)~P_C;
    return v;
}

void abi_ObjectStepByte17(void)
{
    uint8_t p = cpu.p, v = ObjectStepByte17(cpu.db, (uint8_t)cpu.a, cpu.x, &p);

    cpu.a = (uint16_t)((cpu.a & 0xFF00) | v);
    cpu.p = p;
}

/*
 * @routine 03:D1B2 ObjectStubNothing
 * WHAT IT DOES: Nothing but leave M 8-bit.
 * HOW: REP #$20, SEP #$20, RTL.
 * Entry: any widths. Exit: M 8-bit; all else unchanged.
 */
void ObjectStubNothing(void)
{
}

void abi_ObjectStubNothing(void)
{
    cpu.p |= P_M;
}

/*
 * @routine 03:D1B7 ObjectStubSwapTwice
 * WHAT IT DOES: Nothing but leave M 8-bit: X and Y are swapped and swapped
 * back.
 * HOW: PHX / TYX / PLY twice, around REP #$20, SEP #$20.
 * Entry: any widths. Exit: X, Y unchanged; M 8-bit; N, Z from Y (index
 * width).
 */
void ObjectStubSwapTwice(void)
{
}

void abi_ObjectStubSwapTwice(void)
{
    cpu.p |= P_M;
    cpu.p = (cpu.p & P_X) ? nz8(cpu.p, (uint8_t)cpu.y) : nz16(cpu.p, cpu.y);
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

    wr16(b | (uint16_t)(0x202E + obj), 0x0000);
    wr16(b | (uint16_t)(0x2030 + obj), 0x0000);
    wr16(b | (uint16_t)(0x2032 + obj), 0x0000);
    wr8(b | (uint16_t)(0x2017 + obj), 0x00);
}

void abi_ObjectStopX(void)
{
    ObjectStopX(cpu.db, cpu.x);
    cpu.p |= P_M;
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

    wr16(b | (uint16_t)(0x202E + obj), 0x0000);
    wr16(b | (uint16_t)(0x2030 + obj), 0x0000);
    wr16(b | (uint16_t)(0x2032 + obj), 0x0000);
    wr8(b | (uint16_t)(0x2017 + obj), 0x00);
}

void abi_ObjectStopY(void)
{
    ObjectStopY(cpu.db, cpu.y);
    cpu.a = 0x0000;
    cpu.p = (uint8_t)(nz16(cpu.p, 0x0000) | P_M);
}
