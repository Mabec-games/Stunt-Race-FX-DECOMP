/*
 * path.c: objects that follow a path of commands in the table at $08:85B9.
 * An object's +$29 is its offset into the table; each command is a word,
 * then its operands, and the object steps +$29 past them. The commands act
 * on the object and on the object its +$4F points at (a full address in the
 * data bank, not an offset).
 */
#include "machine.h"

#define PATH_TABLE 0x0885B9u

/* A field of the object at offset `obj` in the data bank. */
static uint32_t field(uint8_t bank, uint16_t obj, uint16_t off)
{
    return (uint32_t)bank << 16 | (uint16_t)(0x2000 + off + obj);
}

/* One halving as CMP #$8000 / ROR A / BPL / ADC #0 does it: an arithmetic
   shift whose negative results round toward zero. */
static uint16_t halve(uint16_t v)
{
    uint16_t r = (uint16_t)((v >> 1) | (v & 0x8000));

    if (r & 0x8000)
        { cyc(18); r = (uint16_t)(r + (v & 1)); }
    return r;
}

/* +$29 += n, with the add's V. */
static uint16_t advance(uint8_t bank, uint16_t obj, uint16_t n, uint8_t *p)
{
    uint16_t a = rd16(field(bank, obj, 0x29)), s = (uint16_t)(a + n);

    wr16(field(bank, obj, 0x29), s);
    *p = (uint8_t)((*p & ~P_V) | ((~(a ^ n) & (a ^ s) & 0x8000) ? P_V : 0));
    return s;
}

/*
 * @routine 08:88D8 ObjectPathCommand
 * WHAT IT DOES: Runs object X's next path command. 3: places the linked
 * object (+$1A/+$20, +$1C/+$22, +$1E/+$24 from the three operand words),
 * stops the object (velocity and +$27 zero). 2: sets the linked object's
 * first word. 1: sets +$27 (the step count) and gives the object the
 * velocity that covers the distance from the linked object's +$20/+$22/+$24
 * to the target in 2^n frames (n the second operand byte). Any other
 * command does nothing and returns carry set.
 * HOW: The command word at $08:85B9 + +$29 picks the case; the velocity is
 * the difference halved n times, rounding toward zero; $1F4C-$1F53 hold n
 * and the three differences, each read back from there for every halving
 * and for the velocity. The linked object is read only by the commands
 * that use it. +$29 steps by 8, 4 or 10.
 * Entry: M 8-bit, X 16-bit; X = object. Exit: X unchanged; M 8-bit; N, Z
 * from X; carry set only for an unknown command (A = the command word, Y =
 * X); otherwise carry clear, A = the new +$29, V from its add, Y = X (3),
 * the linked object (2), or 0 (1).
 */
int ObjectPathCommand(uint8_t bank, uint16_t obj, uint16_t *a, uint16_t *y, uint8_t *p)
{
    uint32_t b = (uint32_t)bank << 16;
    uint16_t t = rd16(field(bank, obj, 0x29)), cmd = rd16(PATH_TABLE + t);
    uint16_t link, n, v, w;
    unsigned i;

    if (cmd == 3) {
        link = rd16(field(bank, obj, 0x4F));
        for (i = 0; i < 3; i++) {
            v = rd16(PATH_TABLE + 2 + 2 * i + t);
            wr16(b | (uint16_t)(link + 0x20 + 2 * i), v);
            wr16(b | (uint16_t)(link + 0x1A + 2 * i), v);
        }
        wr16(field(bank, obj, 0x2E), 0);
        wr16(field(bank, obj, 0x30), 0);
        wr16(field(bank, obj, 0x32), 0);
        wr16(field(bank, obj, 0x27), 0);
        *a = advance(bank, obj, 8, p);
        *y = obj;
    } else if (cmd == 2) {
        v = rd16(PATH_TABLE + 2 + t);
        link = rd16(field(bank, obj, 0x4F));
        wr16(b | link, v);
        *a = advance(bank, obj, 4, p);
        *y = link;
    } else if (cmd == 1) {
        v = rd16(PATH_TABLE + 2 + t);
        wr16(field(bank, obj, 0x27), v & 0x00FF);
        link = rd16(field(bank, obj, 0x4F));
        n = rd16(PATH_TABLE + 3 + t);
        wr16(b | 0x1F4C, n & 0x00FF);
        for (i = 0; i < 3; i++) {
            v = rd16(PATH_TABLE + 4 + 2 * i + t);
            w = rd16(b | (uint16_t)(link + 0x20 + 2 * i));
            wr16(b | (uint16_t)(0x1F4E + 2 * i), (uint16_t)(v - w));
        }
        n = rd16(b | 0x1F4C);
        while (n--) {
            for (i = 0; i < 3; i++) {
                v = rd16(b | (uint16_t)(0x1F4E + 2 * i));
                wr16(b | (uint16_t)(0x1F4E + 2 * i), halve(v));
            }
        }
        for (i = 0; i < 3; i++) {
            v = rd16(b | (uint16_t)(0x1F4E + 2 * i));
            wr16(field(bank, obj, (uint16_t)(0x2E + 2 * i)), v);
        }
        *a = advance(bank, obj, 10, p);
        *y = 0;
    } else {
        *a = cmd;
        *y = obj;
        *p |= P_C;
        return 0;
    }
    *p &= (uint8_t)~P_C;
    return 1;
}

void abi_ObjectPathCommand(void)
{
    uint8_t p = cpu.p;

    ObjectPathCommand(cpu.db, cpu.x, &cpu.a, &cpu.y, &p);
    cpu.p = (uint8_t)(nz16(p, cpu.x) | P_M);
}

/*
 * @routine 08:88C7 ObjectPathStart
 * WHAT IT DOES: Starts object X on the path at Y (an address in the
 * $08:85B9 table) and runs its first command.
 * HOW: +$29 = Y - $85B9, then ObjectPathCommand.
 * Entry: M 8-bit, X 16-bit; X = object, Y = path. Exit: as
 * ObjectPathCommand; for an unknown command V is from the subtract.
 */
int ObjectPathStart(uint8_t bank, uint16_t obj, uint16_t path, uint16_t *a, uint16_t *y, uint8_t *p)
{
    uint16_t t = (uint16_t)(path - 0x85B9);

    *p = (uint8_t)((*p & ~P_V) | (((path ^ 0x85B9) & (path ^ t) & 0x8000) ? P_V : 0));
    cyc(190);
    wr16(field(bank, obj, 0x29), t);
    cyc(180);
    return ObjectPathCommand(bank, obj, a, y, p);
}

void abi_ObjectPathStart(void)
{
    uint8_t p = cpu.p;

    ObjectPathStart(cpu.db, cpu.x, cpu.y, &cpu.a, &cpu.y, &p);
    cpu.p = (uint8_t)(nz16(p, cpu.x) | P_M);
}
