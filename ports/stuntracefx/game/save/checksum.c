/*
 * checksum.c: the checksum of the battery-backed save data in Super FX RAM.
 */
#include "machine.h"
#include "carried.h"

/*
 * @routine 03:F02B SaveChecksum
 * WHAT IT DOES: Computes the save data's checksum and stores it twice
 * ($70:FC14 and $70:FC15).
 * HOW: An 8-bit sum with carry of the nine bytes $70:FC06-$70:FC0E, added in
 * the order FC0B, FC06, FC07, FC0C, FC0D, FC0E, FC08, FC09, FC0A, each add
 * taking the previous carry.
 * Entry: any widths. Exit: A low byte = the checksum; P unchanged.
 */
uint8_t SaveChecksum(void)
{
    static const uint16_t order[9] = {0xFC0B, 0xFC06, 0xFC07, 0xFC0C, 0xFC0D, 0xFC0E, 0xFC08, 0xFC09, 0xFC0A};
    unsigned sum = 0, carry = 0, i;

    cyc(58);
    for (i = 0; i < 9; i++) {
        cyc(32);
        sum = (sum & 0xFF) + rd8(0x700000 + order[i]) + carry;
        carry = sum > 0xFF;
    }
    cyc(32);
    wr8(0x70FC14, (uint8_t)sum);
    cyc(32);
    wr8(0x70FC15, (uint8_t)sum);
    cyc(72);
    return (uint8_t)sum;
}

/* SaveChecksum on carried registers. */
void SaveChecksum_cr(Carried *cr)
{
    cr->a = (uint16_t)((cr->a & 0xFF00) | SaveChecksum());
}

void abi_SaveChecksum(void)
{
    Carried r = {cpu.a, cpu.x, cpu.y, cpu.d, cpu.p, cpu.db};

    SaveChecksum_cr(&r);
    cpu.a = r.a;
    cpu.x = r.x;
    cpu.y = r.y;
    cpu.d = r.d;
    cpu.p = r.p;
    cpu.db = r.db;
}
