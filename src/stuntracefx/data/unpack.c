/*
 * unpack.c: the Super FX's decompressor. Packed data is read backward from
 * ROM (R14, in the ROM bank) through a 32-bit bit buffer, R2:R3 (R2 the
 * high word), shifted out from bit 0. A refill loads the next four bytes
 * and sets a marker bit above them, so the buffer is empty when R2:R3
 * shifts down to 0.
 */
#include "gsu.h"

/*
 * @routine 01:D0DB FxUnpackRefill
 * WHAT IT DOES: Refills the bit buffer from the four ROM bytes at R14 and
 * below and takes its first bit.
 * HOW: GETB/GETBH with R14 stepping down: R3 = byte at R14 with the byte
 * below as its high byte, R2 = the next two the same way. Then ROR of R0
 * (just set to 1) puts 1 in CY, and ROR R2, ROR R3 shift the 32 bits down
 * one with that 1 as the marker on top; the bit shifted out is the first
 * bit. The caller's delay slot steps R14 down to the first byte.
 * Entry: R14 = the address of the first byte; R11 = return address.
 * Exit: R2:R3 = the bit buffer; CY = the first bit; R14 = the address of
 * the fourth byte; R0 = R2 | R3 (not 0), S and Z from it; OV kept.
 * Returns the first bit. R14 and the ROM buffer move as on the Super FX.
 */
int FxUnpackRefill(uint16_t *hi, uint16_t *lo)
{
    uint8_t b[4];
    uint16_t w3, w2;
    int i;

    for (i = 0; i < 4; i++) {
        b[i] = gsu_getb();
        if (i < 3)
            gsu_r14((uint16_t)(gsu.r[14] - 1));
    }
    w3 = (uint16_t)(b[0] | b[1] << 8);
    w2 = (uint16_t)(b[2] | b[3] << 8);

    *hi = (uint16_t)(0x8000 | w2 >> 1);
    *lo = (uint16_t)((w2 & 1) << 15 | w3 >> 1);
    return w3 & 1;
}

void abi_FxUnpackRefill(void)
{
    gsu.cy = (uint8_t)FxUnpackRefill(&gsu.r[2], &gsu.r[3]);
    gsu.r[0] = (uint16_t)(gsu.r[2] | gsu.r[3]);
    gsu_sz(gsu.r[0]);
}
