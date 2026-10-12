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

/*
 * @routine 07:D9D1 RandomWordStep
 * WHAT IT DOES: Steps the 16-bit generator at $0D18 and returns the new
 * value. Called once per object update while racing.
 * HOW: The low byte is XORed into the high byte and the bytes swapped (lo'
 * = hi ^ lo, hi' = lo). That word is XORed with the old low byte shifted
 * left once, shifted right one bit, and XORed with $FF80; when the bit
 * shifted out is set it is also XORed with $8180. A result of $AA55 with
 * that bit clear stores 0 instead.
 * Entry: any widths; DB a bank where $0D18 is work RAM. Exit: A = the
 * result ($AA55 when 0 was stored); P unchanged.
 */
uint16_t RandomWordStep(uint8_t bank)
{
    uint32_t at = (uint32_t)bank << 16 | 0x0D18;
    uint16_t x = rd16(at);
    uint16_t y, v;

    /* low byte = hi ^ lo, high byte = lo */
    y = (uint16_t)((x & 0xFF) << 8 | ((x >> 8) ^ (x & 0xFF)));
    wr16(at, y);
    v = (uint16_t)(((x & 0xFF) << 1) ^ y);
    if (v & 1) {
        v = (uint16_t)((v >> 1) ^ 0xFF80 ^ 0x8180);
        wr16(at, v);
    } else {
        v = (uint16_t)((v >> 1) ^ 0xFF80);
        wr16(at, v == 0xAA55 ? 0x0000 : v);
    }
    return v;
}

void abi_RandomWordStep(void)
{
    cpu.a = RandomWordStep(cpu.db);
}

/*
 * @routine 02:DF79 RandomStepLong
 * WHAT IT DOES: RandomStep for callers in other banks.
 * HOW: A near call to RandomStep, then a long return.
 * Entry: any widths, D = $0000. Exit: as RandomStep.
 */
uint8_t RandomStepLong(void)
{
    return RandomStep();
}

void abi_RandomStepLong(void)
{
    cpu.a = (uint16_t)((cpu.a & 0xFF00) | RandomStepLong());
}
