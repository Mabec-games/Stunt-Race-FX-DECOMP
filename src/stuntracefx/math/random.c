/*
 * random.c: the pseudo-random generator in direct page $E6-$E9.
 */
#include "machine.h"

/* SBC with borrow in: a - m - (1 - C), C out = no borrow. */
static uint8_t sbc8(uint8_t a, uint8_t m, int *c)
{
    unsigned r = (unsigned)a - m - (*c ? 0 : 1);

    *c = r < 0x100;
    return (uint8_t)r;
}

/*
 * @routine 02:DF7D RandomStep
 * WHAT IT DOES: Steps the generator: each of $E7, $E8, $E9 becomes the
 * previous result minus it, with the borrow carried along, starting from
 * $E6 - $E7 - 1; finally $E6 becomes the last result minus $E6.
 * HOW: A chain of 8-bit SBCs from carry clear.
 * Entry: any widths, D = $0000. Exit: A low byte = the new $E6; P
 * unchanged.
 */
uint8_t RandomStep(void)
{
    int c = 0;
    uint8_t a = rd8(0x0000E6);

    a = sbc8(a, rd8(0x0000E7), &c);
    wr8(0x0000E7, a);
    a = sbc8(a, rd8(0x0000E8), &c);
    wr8(0x0000E8, a);
    a = sbc8(a, rd8(0x0000E9), &c);
    wr8(0x0000E9, a);
    a = sbc8(a, rd8(0x0000E6), &c);
    wr8(0x0000E6, a);
    return a;
}

void abi_RandomStep(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | RandomStep());
}
