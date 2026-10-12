/*
 * unpack.c: the Super FX's decompressor. Packed data is read backward from
 * ROM (R14, in the ROM bank) through a 32-bit bit buffer, R2:R3 (R2 the
 * high word), shifted out from bit 0. A refill loads the next four bytes
 * and sets a marker bit above them, so the buffer is empty when R2:R3
 * shifts down to 0.
 */
#include "gsu.h"
#include "gsu_ops.h"

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

/* The routine's first instruction, SUB R4, as the caller's pending prefix
   makes it (every call seen has WITH R4 there: R4 = R4 - R4 = 0). */
static void first_sub_r4(void)
{
    uint8_t s = gsu.pre_sreg, d = gsu.pre_dreg;

    if (gsu.pre_alt1 && gsu.pre_alt2)
        gsu_sub(gsu.r[s], gsu.r[4], 0);                 /* CMP */
    else if (gsu.pre_alt2)
        gsu.r[d] = gsu_sub(gsu.r[s], 4, 0);             /* SUB #4 */
    else
        gsu.r[d] = gsu_sub(gsu.r[s], gsu.r[4], gsu.pre_alt1 ? !gsu.cy : 0);
}

/*
 * @routine 01:D0F3 FxUnpackBits
 * WHAT IT DOES: Reads R12 bits from the bit buffer into R4, the first
 * read ending as the highest.
 * HOW: Its first SUB R4 runs with the WITH R4 the caller leaves in its
 * call's delay slot, clearing R4. R13 = R7, the loop address the caller
 * sets ($D0F7, the LSR after a WITH R2, so LOOP's delay slot WITH R2
 * belongs to the next pass). Each pass: LSR R2, ROR R3 take the next bit
 * into CY; when R2:R3 is then 0 the bit was the marker, so the buffer is
 * refilled (FxUnpackRefill's code, inline after a DEC R14) and its first
 * bit taken; ROL R4 shifts the bit in. R12 = 0 reads $10000 bits.
 * Entry: R12 = the count; R7 = $D0F7; R2:R3 = the bit buffer, R14 its
 * source; R11 = return address; WITH R4 pending. Exit: R4 = the bits;
 * R2:R3 and R14 moved on; R0 = R2 | R3 after the last bit (or $8000 when
 * that bit needed a refill); R12 = 0; R13 = R7; OV from the first SUB; CY
 * from the last ROL R4; Z set, S clear; WITH R2 pending at the return.
 * R14 and the ROM buffer move as on the Super FX.
 */
void FxUnpackBits(void)
{
    first_sub_r4();
    gsu.r[13] = gsu.r[7];
    do {
        gsu.r[2] = gsu_lsr(gsu.r[2]);
        gsu.r[3] = gsu_ror(gsu.r[3]);
        gsu.r[0] = gsu_or(gsu.r[2], gsu.r[3]);
        if (gsu.z) {
            uint8_t marker = gsu.cy;

            gsu_r14(gsu_dec(gsu.r[14]));
            gsu.cy = (uint8_t)FxUnpackRefill(&gsu.r[2], &gsu.r[3]);
            gsu.r[0] = (uint16_t)(marker << 15);        /* IBT R0,#1 then ROR */
        }
        gsu.r[4] = gsu_rol(gsu.r[4]);
    } while (gsu_count_down(&gsu.r[12]));
}

void abi_FxUnpackBits(void)
{
    FxUnpackBits();
}
