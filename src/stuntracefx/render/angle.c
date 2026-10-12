/*
 * angle.c: the Super FX's angles: a full turn is $10000, so $4000 is a
 * quarter turn. The arctangent comes from a table of 256 words in ROM
 * bank $00 at $9E40, indexed by the ratio of the smaller side to the
 * larger.
 */
#include "gsu.h"
#include "gsu_ops.h"

#define FX_ATAN_TABLE 0x009E40u

uint16_t FxDivide32By16(uint16_t hi, uint16_t lo, uint16_t divisor, uint16_t *remainder);

static uint16_t magnitude(uint16_t v)
{
    return v & 0x8000 ? (uint16_t)-v : v;
}

/*
 * @routine 01:81F7 FxAtan2
 * WHAT IT DOES: The angle of the vector (R2, R1): 0 along +R2, $4000
 * along +R1, as a word where a full turn is $10000.
 * HOW: Pushes R11 and R14 on the R10 stack. With R6 = |R1| and R0 = |R2|:
 * R2 = 0 gives $4000 and equal sides give $2000; otherwise the smaller
 * side << 14 is divided by the larger (FxDivide32By16), and the quotient
 * >> 5, made even, indexes the arctangent table (GETB/GETBH from ROM bank
 * 0). When |R1| is the larger side the angle is $4000 minus the table's.
 * Then it is negated when R1 and R2 differ in sign, and $8000 is added
 * when R2 is negative. Pulls R14 and returns by pulling R11's value into
 * R15.
 * Entry: R1, R2 = the vector; R10 = the stack; R11 = return address.
 * Exit: R0 = the angle; R4 = $8000 when R2 is negative, else R1 XOR R2;
 * R6 = the larger side (|R1| when R2 = 0 or the sides are equal); R5 =
 * $FFFF when |R2| is the larger side, 0 when |R1| is or they are equal,
 * kept when R2 = 0. When the table was used: R11 = $823D, R12 = 0, R13 =
 * $81D2, the ROM bank 0. R14 (written back, so the ROM buffer refilled
 * from it) and R10 as at entry; the last RAM address
 * R10; S and Z from R10; OV and CY from the ADD of $8000 when R2 is
 * negative, else OV = bit 7 of R2 and CY from the last ADD before (kept
 * when R2 = 0, set when the sides are equal).
 */
uint16_t FxAtan2(uint16_t y, uint16_t x)
{
    uint16_t ay = magnitude(y), ax = magnitude(x);
    uint16_t angle;

    if (ax == 0) {
        angle = 0x4000;
    } else if (ax == ay) {
        angle = 0x2000;
    } else {
        uint16_t small = ax, large = ay, rem, ratio, at;
        int x_larger = !((int16_t)ax < (int16_t)ay);

        if (x_larger) {
            small = ay;
            large = ax;
        }
        ratio = FxDivide32By16(small >> 2, (uint16_t)(small << 14), large, &rem);
        at = (uint16_t)(FX_ATAN_TABLE + ((ratio >> 5) & 0xFFFE));
        gsu.rombr = 0;
        gsu_r14(at);
        angle = gsu_getb();
        gsu_r14((uint16_t)(at + 1));
        angle = (uint16_t)(angle | gsu_getb() << 8);
        if (!x_larger)
            angle = (uint16_t)(0x4000 - angle);
    }
    if ((x ^ y) & 0x8000)
        angle = (uint16_t)-angle;
    if (x & 0x8000)
        angle = (uint16_t)(angle + 0x8000);
    return angle;
}

void abi_FxAtan2(void)
{
    uint16_t y = gsu.r[1], x = gsu.r[2];
    uint16_t ay = magnitude(y), ax = magnitude(x);
    uint16_t angle;

    gsu_push(&gsu.r[10], gsu.r[11]);
    gsu_push(&gsu.r[10], gsu.r[14]);
    angle = FxAtan2(y, x);
    gsu.r[6] = ay;
    if (ax == ay && ax != 0) {
        gsu.r[5] = 0;
        gsu.cy = 1;
    } else if (ax != 0) {
        int x_larger = !((int16_t)ax < (int16_t)ay);
        uint16_t pre;

        gsu.r[5] = x_larger ? 0xFFFF : 0;
        if (x_larger)
            gsu.r[6] = ax;
        gsu.r[11] = 0x823D;
        gsu.r[12] = 0;
        gsu.r[13] = 0x81D2;
        /* The table's word, before the sign steps. */
        pre = (uint16_t)((x ^ y) & 0x8000 ? -(x & 0x8000 ? (uint16_t)(angle - 0x8000) : angle)
                                           : (x & 0x8000 ? (uint16_t)(angle - 0x8000) : angle));
        if (!x_larger)
            gsu_add((uint16_t)-(uint16_t)(0x4000 - pre), 0x4000, 0);
        else
            gsu.cy = 0;  /* the ADD of the table's address */
    }
    gsu.r[4] = (uint16_t)(x ^ y);
    gsu_moves(x);
    if (x & 0x8000) {
        gsu.r[4] = 0x8000;
        gsu_add((uint16_t)(angle - 0x8000), 0x8000, 0);
    }
    gsu.r[0] = angle;
    gsu_r14(gsu_pull(&gsu.r[10]));
    gsu_pull(&gsu.r[10]);       /* the return address, into R15 */
}
