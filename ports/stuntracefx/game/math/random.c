/*
 * random.c: the pseudo-random generator in direct page $E6-$E9.
 */
#include "machine.h"
#include "carried.h"
#include "wram_names.h"
#include "wram_struct.h"

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
    cyc(60);
    uint8_t a = RD8(random_e6);

    cyc(30);
    a = sbc8(a, RD8(random_e7), &c);
    cyc(16);
    WR8(random_e7, a);
    cyc(16);
    a = sbc8(a, RD8(random_e8), &c);
    cyc(16);
    WR8(random_e8, a);
    cyc(16);
    a = sbc8(a, RD8(random_e9), &c);
    cyc(16);
    WR8(random_e9, a);
    cyc(16);
    a = sbc8(a, RD8(random_e6), &c);
    cyc(16);
    WR8(random_e6, a);
    cyc(70);
    return a;
}

/* RandomStep on carried registers. */
void RandomStep_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | RandomStep());
}

void abi_RandomStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RandomStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 07:D9D1 RandomWordStep
 * WHAT IT DOES: Steps the 16-bit generator at $0D18 and returns the new
 * value. Called once per object update while racing.
 * HOW: The low byte is XORed into the high byte and the bytes swapped (lo'
 * = hi ^ lo, hi' = lo). That word is XORed with the old low byte shifted
 * left once, shifted right one bit, and XORed with $FF80; when the bit
 * shifted out is set it is also XORed with $8180. A result of $AA55 with
 * that bit clear stores 0 instead. Each EOR reads $0D18 again (the second
 * after the first store).
 * Entry: any widths; DB a bank where $0D18 is work RAM. Exit: A = the
 * result ($AA55 when 0 was stored); P unchanged.
 */
uint16_t RandomWordStep(uint8_t bank)
{
    (void)bank;
    uint32_t at = WA(random_word);
    cyc(68);
    uint16_t x = rd16(at), t, y, v;

    /* low byte = hi ^ lo, high byte = lo (the word read again for the EOR) */
    cyc(68);
    t = rd16(at);
    y = (uint16_t)(((x & 0xFF) << 8) ^ t);
    y = (uint16_t)(y >> 8 | y << 8);
    cyc(44);
    wr16(at, y);
    v = (uint16_t)((y >> 8) << 1);
    cyc(82);
    t = rd16(at);
    v ^= t;
    if (v & 1) {
        v = (uint16_t)((v >> 1) ^ 0xFF80 ^ 0x8180);
        cyc(102);
        wr16(at, v);
    } else {
        v = (uint16_t)((v >> 1) ^ 0xFF80);
        cyc(124);
        wr16(at, v == 0xAA55 ? 0x0000 : (cyc(6), v));
    }
    cyc(72);
    return v;
}

/* RandomWordStep on carried registers. */
void RandomWordStep_cr(Carried *cr)
{
    cr->a = RandomWordStep(cr->db);
}

void abi_RandomWordStep(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RandomWordStep_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}

/*
 * @routine 02:DF79 RandomStepLong
 * WHAT IT DOES: RandomStep for callers in other banks.
 * HOW: A near call to RandomStep, then a long return.
 * Entry: any widths, D = $0000. Exit: as RandomStep.
 */
uint8_t RandomStepLong(void)
{
    return cyc_then(44, RandomStep());
}

/* RandomStepLong on carried registers. */
void RandomStepLong_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | (cyc(46), RandomStepLong()));
}

void abi_RandomStepLong(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    RandomStepLong_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
